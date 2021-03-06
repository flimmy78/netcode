/* Redis benchmark utility.
 *
 * Copyright (c) 2009-2010, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "fmacros.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#include <sys/time.h>
#endif
#include <errno.h>
#include <signal.h>
#include <assert.h>

#include "ae.h"
#include "hiredis.h"
#include "sds.h"
#include "adlist.h"
#include "zmalloc.h"
#ifdef _WIN32
#include "win32fixes.h"
#endif
#ifdef LIBUV
#include "uv.h"
#endif

#define REDIS_NOTUSED(V) ((void) V)

static struct config {
    aeEventLoop *el;
#ifdef LIBUV
    uv_loop_t *uvloop;
#endif
    const char *hostip;
    int hostport;
    const char *hostsocket;
    int numclients;
    int liveclients;
    int requests;
    int requests_issued;
    int requests_finished;
    int keysize;
    int datasize;
    int randomkeys;
    int randomkeys_keyspacelen;
    int keepalive;
    long long start;
    long long totlatency;
    long long *latency;
    const char *title;
    list *clients;
    int quiet;
    int loop;
    int idlemode;
} config;

typedef struct _client {
    redisContext *context;
#ifdef LIBUV
    uv_connect_t connect_req;
    uv_write_t wreq;
    uv_tcp_t uv_tcp_strm;
    uv_pipe_t uv_pipe_strm;
    uv_stream_t *uv_strm;
    uv_buf_t bufs[1];
    char rbuf[2048];
#endif
    sds obuf;
    char *randptr[10]; /* needed for MSET against 10 keys */
    size_t randlen;
    unsigned int written; /* bytes of 'obuf' already written */
    long long start; /* start time of a request */
    long long latency; /* request latency */
} *client;

/* Prototypes */
static void writeHandler(aeEventLoop *el, int fd, void *privdata, int mask);
static void createMissingClients(client c);

/* Implementation */
static long long ustime(void) {
    struct timeval tv;
    long long ust;

    gettimeofday(&tv, NULL);
    ust = ((long)tv.tv_sec)*1000000;
    ust += tv.tv_usec;
    return ust;
}

static long long mstime(void) {
    struct timeval tv;
    long long mst;

    gettimeofday(&tv, NULL);
    mst = ((long)tv.tv_sec)*1000;
    mst += tv.tv_usec/1000;
    return mst;
}

#ifdef LIBUV
void closeCb(uv_handle_t* handle) {
    client c = (client)handle->data;
    zfree(c);
}
#endif

static void freeClient(client c) {
    listNode *ln;
#ifdef LIBUV
    uv_close((uv_handle_t *)c->uv_strm, closeCb);
    redisFree(c->context);
    sdsfree(c->obuf);
    /* free client in callback */
#else
    aeDeleteFileEvent(config.el,c->context->fd,AE_WRITABLE);
    aeDeleteFileEvent(config.el,c->context->fd,AE_READABLE);
    redisFree(c->context);
    sdsfree(c->obuf);
    zfree(c);
#endif
    config.liveclients--;
    ln = listSearchKey(config.clients,c);
    assert(ln != NULL);
    listDelNode(config.clients,ln);
}

static void freeAllClients(void) {
    listNode *ln = config.clients->head, *next;

    while(ln) {
        next = ln->next;
        freeClient(ln->value);
        ln = next;
    }
}

static void resetClient(client c) {
#ifdef LIBUV
    c->written = 0;
    uv_read_stop(c->uv_strm);
    writeHandler(NULL, 0, c, 0);
#else
    aeDeleteFileEvent(config.el,c->context->fd,AE_WRITABLE);
    aeDeleteFileEvent(config.el,c->context->fd,AE_READABLE);
    aeCreateFileEvent(config.el,c->context->fd,AE_WRITABLE,writeHandler,c);
    c->written = 0;
#endif
}

static void randomizeClientKey(client c) {
    char buf[32];
    size_t i, r;

    for (i = 0; i < c->randlen; i++) {
        r = random() % config.randomkeys_keyspacelen;
#ifdef _WIN32
        snprintf(buf,sizeof(buf),"%012lu",(long unsigned)r);
#else
        snprintf(buf,sizeof(buf),"%012zu",r);
#endif
        memcpy(c->randptr[i],buf,12);
    }
}

static void clientDone(client c) {
    if (config.requests_finished == config.requests) {
        freeClient(c);
#ifndef LIBUV
        aeStop(config.el);
#endif
        return;
    }
    if (config.keepalive) {
        resetClient(c);
    } else {
        config.liveclients--;
        createMissingClients(c);
        config.liveclients++;
        freeClient(c);
    }
}

/* common read handling used by Libuv and regular */
static void readReplyReady(client c) {
    void *reply = NULL;

    if (redisGetReply(c->context,&reply) != REDIS_OK) {
        fprintf(stderr,"Error: %s\n",c->context->errstr);
        exit(1);
    }
    if (reply != NULL) {
        if (reply == (void*)REDIS_REPLY_ERROR) {
            fprintf(stderr,"Unexpected error reply, exiting...\n");
            exit(1);
        }

        if (config.requests_finished < config.requests)
            config.latency[config.requests_finished++] = c->latency;
        clientDone(c);
    }
}

#ifdef LIBUV
void readDone(uv_stream_t *stream, ssize_t nread, uv_buf_t buf) {
    client c = (client) stream->data;

    if (nread < 0)  {
        uv_err_t err = uv_last_error(config.uvloop);
        errno = err.sys_errno_;
        fprintf(stderr,"Reading from client: %s\n",c->context->errstr);
        exit(1);
    }
    /* Calculate latency only for the first read event. This means that the
     * server already sent the reply and we need to parse it. Parsing overhead
     * is not part of the latency, so calculate it only once, here. */
    if (c->latency < 0) c->latency = ustime()-(c->start);

    if (redisBufferReadDone(c->context, buf.base, nread) != REDIS_OK) {
        fprintf(stderr,"Error: %s\n",c->context->errstr);
        exit(1);
    } else {
        readReplyReady(c);
    }
}

static uv_buf_t readbuf_alloc(uv_handle_t *handle, size_t suggested_size) {
    client c = (client) handle->data;
    return uv_buf_init(c->rbuf, 2048);
}

static void writeCb(uv_write_t *req, int status) {
    client c = (client)req->data;
    if (status != 0) {
        uv_err_t err = uv_last_error(config.uvloop);
        errno = err.sys_errno_;
        fprintf(stderr, "Writing to socket: %s\n", strerror(errno));
        freeClient(c);
    }
}
#else
static void readHandler(aeEventLoop *el, int fd, void *privdata, int mask) {
    client c = privdata;
    void *reply = NULL;
    REDIS_NOTUSED(el);
    REDIS_NOTUSED(fd);
    REDIS_NOTUSED(mask);

    /* Calculate latency only for the first read event. This means that the
     * server already sent the reply and we need to parse it. Parsing overhead
     * is not part of the latency, so calculate it only once, here. */
    if (c->latency < 0) c->latency = ustime()-(c->start);

    if (redisBufferRead(c->context) != REDIS_OK) {
        fprintf(stderr,"Error: %s\n",c->context->errstr);
        exit(1);
    } else {
        readReplyReady(c);
    }
}
#endif

static void writeHandler(aeEventLoop *el, int fd, void *privdata, int mask) {
    client c = privdata;
    REDIS_NOTUSED(el);
    REDIS_NOTUSED(fd);
    REDIS_NOTUSED(mask);

    /* Initialize request when nothing was written. */
    if (c->written == 0) {
        /* Enforce upper bound to number of requests. */
        if (config.requests_issued++ >= config.requests) {
            freeClient(c);
            return;
        }

        /* Really initialize: randomize keys and set start time. */
        if (config.randomkeys) randomizeClientKey(c);
        c->start = ustime();
        c->latency = -1;
    }

    if (sdslen(c->obuf) > c->written) {
        void *ptr = c->obuf+c->written;
#ifdef LIBUV
        c->bufs[0] = uv_buf_init(ptr, sdslen(c->obuf)-c->written);
        c->wreq.data = c;
        if (uv_write(&c->wreq, c->uv_strm, c->bufs, 1, writeCb) != 0) {
            uv_err_t err = uv_last_error(config.uvloop);
            errno = err.sys_errno_;
            fprintf(stderr, "Writing to socket: %s\n", strerror(errno));
            freeClient(c);
            return;
        } else {
            c->written += sdslen(c->obuf)-c->written;
            /* Enable reading */
            if (uv_read_start(c->uv_strm, readbuf_alloc, readDone) != 0) {
                uv_err_t err = uv_last_error(config.uvloop);
                errno = err.sys_errno_;
                fprintf(stderr, "Starting to read: %s\n", strerror(errno));
                freeClient(c);
            }
        }
#else
        int nwritten = write(c->context->fd,ptr,sdslen(c->obuf)-c->written);
        if (nwritten == -1) {
            if (errno != EPIPE)
                fprintf(stderr, "Writing to socket: %s\n", strerror(errno));
            freeClient(c);
            return;
        }
        c->written += nwritten;
        if (sdslen(c->obuf) == c->written) {
            aeDeleteFileEvent(config.el,c->context->fd,AE_WRITABLE);
            aeCreateFileEvent(config.el,c->context->fd,AE_READABLE,readHandler,c);
        }
#endif
    }
}

#ifdef LIBUV
static void connectCb(uv_connect_t *req, int status) {
    if (status != UV_OK) {
        uv_err_t err = uv_last_error(config.uvloop);
        errno = err.sys_errno_;
        fprintf(stderr,"Could not connect to Redis at ");
        if (config.hostsocket == NULL)
            fprintf(stderr,"%s:%d: %s\n",config.hostip,config.hostport,strerror(errno));
        else
            fprintf(stderr,"%s: %s\n",config.hostsocket,strerror(errno));
        exit(1);
    }
    
    writeHandler(NULL, 0, req->handle->data, 0);
}

static void connectUv(client c) {
    int uvret;
    if (config.hostsocket == NULL) {
        struct sockaddr_in addr = uv_ip4_addr(config.hostip, config.hostport);
        uvret = uv_tcp_init(config.uvloop, &c->uv_tcp_strm);
        if (uvret == 0) {
            c->uv_tcp_strm.data = c;
            c->uv_strm = (uv_stream_t *)&c->uv_tcp_strm;
            uvret = uv_tcp_connect(&c->connect_req, &c->uv_tcp_strm, addr, connectCb);
        }
    } else {
        uvret = uv_pipe_init(config.uvloop, &c->uv_pipe_strm, 0);
        if (uvret == 0) {
            c->uv_pipe_strm.data = c;
            c->uv_strm = (uv_stream_t *)&c->uv_pipe_strm;
            uv_pipe_connect(&c->connect_req, &c->uv_pipe_strm, config.hostsocket, connectCb);
        }
    }
    if (uvret != 0) {
        uv_err_t err = uv_last_error(config.uvloop);
        errno = err.sys_errno_;
        fprintf(stderr,"Could not connect to Redis using Libuv at ");
        if (config.hostsocket == NULL)
            fprintf(stderr,"%s:%d: %s\n",config.hostip,config.hostport,strerror(errno));
        else
            fprintf(stderr,"%s: %s\n",config.hostsocket,strerror(errno));
        exit(1);
    }

    /* get context using our own connection */
    c->context = redisConnectedNonBlock();
}
#endif

static client createClient(const char *cmd, size_t len) {
    client c = zmalloc(sizeof(struct _client));
#ifdef LIBUV
    connectUv(c);
#else
    if (config.hostsocket == NULL) {
        c->context = redisConnectNonBlock(config.hostip,config.hostport);
    } else {
        c->context = redisConnectUnixNonBlock(config.hostsocket);
    }
    if (c->context->err) {
        fprintf(stderr,"Could not connect to Redis at ");
        if (config.hostsocket == NULL)
            fprintf(stderr,"%s:%d: %s\n",config.hostip,config.hostport,c->context->errstr);
        else
            fprintf(stderr,"%s: %s\n",config.hostsocket,c->context->errstr);
        exit(1);
    }
#endif
    c->obuf = sdsnewlen(cmd,len);
    c->randlen = 0;
    c->written = 0;

    /* Find substrings in the output buffer that need to be randomized. */
    if (config.randomkeys) {
        char *p = c->obuf, *newline;
        while ((p = strstr(p,":rand:")) != NULL) {
            newline = strstr(p,"\r\n");
            assert(newline-(p+6) == 12); /* 12 chars for randomness */
            assert(c->randlen < (signed)(sizeof(c->randptr)/sizeof(char*)));
            c->randptr[c->randlen++] = p+6;
            p = newline+2;
        }
    }

    redisSetReplyObjectFunctions(c->context,NULL);
#ifndef LIBUV
    aeCreateFileEvent(config.el,c->context->fd,AE_WRITABLE,writeHandler,c);
#endif
    listAddNodeTail(config.clients,c);
    config.liveclients++;
    return c;
}

static void createMissingClients(client c) {
    int n = 0;

    while(config.liveclients < config.numclients) {
        createClient(c->obuf,sdslen(c->obuf));

        /* Listen backlog is quite limited on most systems */
        if (++n > 64) {
            usleep(50000);
            n = 0;
        }
    }
}

static int compareLatency(const void *a, const void *b) {
    return (*(long long*)a)-(*(long long*)b);
}

static void showLatencyReport(void) {
    int i, curlat = 0;
    float perc, reqpersec;

    reqpersec = (float)config.requests_finished/((float)config.totlatency/1000);
    if (!config.quiet) {
        printf("====== %s ======\n", config.title);
        printf("  %d requests completed in %.2f seconds\n", config.requests_finished,
            (float)config.totlatency/1000);
        printf("  %d parallel clients\n", config.numclients);
        printf("  %d bytes payload\n", config.datasize);
        printf("  keep alive: %d\n", config.keepalive);
        printf("\n");

        qsort(config.latency,config.requests,sizeof(long long),compareLatency);
        for (i = 0; i < config.requests; i++) {
            if (config.latency[i]/1000 != curlat || i == (config.requests-1)) {
                curlat = config.latency[i]/1000;
                perc = ((float)(i+1)*100)/config.requests;
                printf("%.2f%% <= %d milliseconds\n", perc, curlat);
            }
        }
        printf("%.2f requests per second\n\n", reqpersec);
    } else {
        printf("%s: %.2f requests per second\n", config.title, reqpersec);
    }
}

static void benchmark(const char *title, const char *cmd, int len) {
    client c;

#ifdef LIBUV
    config.uvloop = uv_default_loop();
#endif
    config.title = title;
    config.requests_issued = 0;
    config.requests_finished = 0;

    c = createClient(cmd,len);
    createMissingClients(c);

    config.start = mstime();
#ifdef LIBUV
    uv_run(config.uvloop);
#else
    aeMain(config.el);
#endif
    config.totlatency = mstime()-config.start;

    showLatencyReport();
    freeAllClients();
}

/* Returns number of consumed options. */
int parseOptions(int argc, const char **argv) {
    int i;
    int lastarg;
    int exit_status = 1;

    for (i = 1; i < argc; i++) {
        lastarg = (i == (argc-1));

        if (!strcmp(argv[i],"-c")) {
            if (lastarg) goto invalid;
            config.numclients = atoi(argv[++i]);
        } else if (!strcmp(argv[i],"-n")) {
            if (lastarg) goto invalid;
            config.requests = atoi(argv[++i]);
        } else if (!strcmp(argv[i],"-k")) {
            if (lastarg) goto invalid;
            config.keepalive = atoi(argv[++i]);
        } else if (!strcmp(argv[i],"-h")) {
            if (lastarg) goto invalid;
            config.hostip = strdup(argv[++i]);
        } else if (!strcmp(argv[i],"-p")) {
            if (lastarg) goto invalid;
            config.hostport = atoi(argv[++i]);
        } else if (!strcmp(argv[i],"-s")) {
            if (lastarg) goto invalid;
            config.hostsocket = strdup(argv[++i]);
        } else if (!strcmp(argv[i],"-d")) {
            if (lastarg) goto invalid;
            config.datasize = atoi(argv[++i]);
            if (config.datasize < 1) config.datasize=1;
            if (config.datasize > 1024*1024) config.datasize = 1024*1024;
        } else if (!strcmp(argv[i],"-r")) {
            if (lastarg) goto invalid;
            config.randomkeys = 1;
            config.randomkeys_keyspacelen = atoi(argv[++i]);
            if (config.randomkeys_keyspacelen < 0)
                config.randomkeys_keyspacelen = 0;
        } else if (!strcmp(argv[i],"-q")) {
            config.quiet = 1;
        } else if (!strcmp(argv[i],"-l")) {
            config.loop = 1;
        } else if (!strcmp(argv[i],"-I")) {
            config.idlemode = 1;
        } else if (!strcmp(argv[i],"--help")) {
            exit_status = 0;
            goto usage;
        } else {
            /* Assume the user meant to provide an option when the arg starts
             * with a dash. We're done otherwise and should use the remainder
             * as the command and arguments for running the benchmark. */
            if (argv[i][0] == '-') goto invalid;
            return i;
        }
    }

    return i;

invalid:
    printf("Invalid option \"%s\" or option argument missing\n\n",argv[i]);

usage:
    printf("Usage: redis-benchmark [-h <host>] [-p <port>] [-c <clients>] [-n <requests]> [-k <boolean>]\n\n");
    printf(" -h <hostname>      Server hostname (default 127.0.0.1)\n");
    printf(" -p <port>          Server port (default 6379)\n");
    printf(" -s <socket>        Server socket (overrides host and port)\n");
    printf(" -c <clients>       Number of parallel connections (default 50)\n");
    printf(" -n <requests>      Total number of requests (default 10000)\n");
    printf(" -d <size>          Data size of SET/GET value in bytes (default 2)\n");
    printf(" -k <boolean>       1=keep alive 0=reconnect (default 1)\n");
    printf(" -r <keyspacelen>   Use random keys for SET/GET/INCR, random values for SADD\n");
    printf("  Using this option the benchmark will get/set keys\n");
    printf("  in the form mykey_rand000000012456 instead of constant\n");
    printf("  keys, the <keyspacelen> argument determines the max\n");
    printf("  number of values for the random number. For instance\n");
    printf("  if set to 10 only rand000000000000 - rand000000000009\n");
    printf("  range will be allowed.\n");
    printf(" -q                 Quiet. Just show query/sec values\n");
    printf(" -l                 Loop. Run the tests forever\n");
    printf(" -I                 Idle mode. Just open N idle connections and wait.\n");
    exit(exit_status);
}

int showThroughput(struct aeEventLoop *eventLoop, long long id, void *clientData) {
    float dt;
    float rps;
    REDIS_NOTUSED(eventLoop);
    REDIS_NOTUSED(id);
    REDIS_NOTUSED(clientData);

    dt = (float)(mstime()-config.start)/1000.0;
    rps = (float)config.requests_finished/dt;
    printf("%s: %.2f\r", config.title, rps);
    fflush(stdout);
    return 250; /* every 250ms */
}

int main(int argc, const char **argv) {
    int i;
    char *data, *cmd;
    int len;

    client c;

    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    config.numclients = 50;
    config.requests = 10000;
    config.liveclients = 0;
    config.el = aeCreateEventLoop();
    aeCreateTimeEvent(config.el,1,showThroughput,NULL,NULL);
    config.keepalive = 1;
    config.datasize = 3;
    config.randomkeys = 0;
    config.randomkeys_keyspacelen = 0;
    config.quiet = 0;
    config.loop = 0;
    config.idlemode = 0;
    config.latency = NULL;
    config.clients = listCreate();
    config.hostip = "127.0.0.1";
    config.hostport = 6379;
    config.hostsocket = NULL;

    i = parseOptions(argc,argv);
    argc -= i;
    argv += i;

    config.latency = (long long *)zmalloc(sizeof(long long)*config.requests);

    if (config.keepalive == 0) {
        printf("WARNING: keepalive disabled, you probably need 'echo 1 > /proc/sys/net/ipv4/tcp_tw_reuse' for Linux and 'sudo sysctl -w net.inet.tcp.msl=1000' for Mac OS X in order to use a lot of clients/requests\n");
    }

    if (config.idlemode) {
        printf("Creating %d idle connections and waiting forever (Ctrl+C when done)\n", config.numclients);
        c = createClient("",0); /* will never receive a reply */
        createMissingClients(c);
        aeMain(config.el);
        /* and will wait for every */
    }

    /* Run benchmark with command in the remainder of the arguments. */
    if (argc) {
        sds title = sdsnew(argv[0]);
        for (i = 1; i < argc; i++) {
            title = sdscatlen(title, " ", 1);
            title = sdscatlen(title, (char*)argv[i], strlen(argv[i]));
        }

        do {
            len = redisFormatCommandArgv(&cmd,argc,argv,NULL);
            benchmark(title,cmd,len);
            free(cmd);
        } while(config.loop);

        return 0;
    }

    /* Run default benchmark suite. */
    do {
        const char *argv[21];
        data = (char*)zmalloc(config.datasize+1);
        memset(data,'x',config.datasize);
        data[config.datasize] = '\0';

        benchmark("PING (inline)","PING\r\n",6);

        len = redisFormatCommand(&cmd,"PING");
        benchmark("PING",cmd,len);
        free(cmd);

        argv[0] = "MSET";
        for (i = 1; i < 21; i += 2) {
            argv[i] = "foo:rand:000000000000";
            argv[i+1] = data;
        }
        len = redisFormatCommandArgv(&cmd,21,argv,NULL);
        benchmark("MSET (10 keys)",cmd,len);
        free(cmd);

        len = redisFormatCommand(&cmd,"SET foo:rand:000000000000 %s",data);
        benchmark("SET",cmd,len);
        free(cmd);

        len = redisFormatCommand(&cmd,"GET foo:rand:000000000000");
        benchmark("GET",cmd,len);
        free(cmd);

        len = redisFormatCommand(&cmd,"INCR counter:rand:000000000000");
        benchmark("INCR",cmd,len);
        free(cmd);

        len = redisFormatCommand(&cmd,"LPUSH mylist %s",data);
        benchmark("LPUSH",cmd,len);
        free(cmd);

        len = redisFormatCommand(&cmd,"LPOP mylist");
        benchmark("LPOP",cmd,len);
        free(cmd);

        len = redisFormatCommand(&cmd,"SADD myset counter:rand:000000000000");
        benchmark("SADD",cmd,len);
        free(cmd);

        len = redisFormatCommand(&cmd,"SPOP myset");
        benchmark("SPOP",cmd,len);
        free(cmd);

        len = redisFormatCommand(&cmd,"LPUSH mylist %s",data);
        benchmark("LPUSH (again, in order to bench LRANGE)",cmd,len);
        free(cmd);

        len = redisFormatCommand(&cmd,"LRANGE mylist 0 99");
        benchmark("LRANGE (first 100 elements)",cmd,len);
        free(cmd);

        len = redisFormatCommand(&cmd,"LRANGE mylist 0 299");
        benchmark("LRANGE (first 300 elements)",cmd,len);
        free(cmd);

        len = redisFormatCommand(&cmd,"LRANGE mylist 0 449");
        benchmark("LRANGE (first 450 elements)",cmd,len);
        free(cmd);

        len = redisFormatCommand(&cmd,"LRANGE mylist 0 599");
        benchmark("LRANGE (first 600 elements)",cmd,len);
        free(cmd);

        printf("\n");
    } while(config.loop);

    return 0;
}
