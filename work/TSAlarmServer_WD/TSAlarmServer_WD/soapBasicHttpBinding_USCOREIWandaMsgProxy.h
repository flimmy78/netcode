/* soapBasicHttpBinding_USCOREIWandaMsgProxy.h
   Generated by gSOAP 2.8.4 from WandaMsg.h

Copyright(C) 2000-2011, Robert van Engelen, Genivia Inc. All Rights Reserved.
The generated code is released under one of the following licenses:
1) GPL or 2) Genivia's license for commercial use.
This program is released under the GPL with the additional exemption that
compiling, linking, and/or using OpenSSL is allowed.
*/

#ifndef soapBasicHttpBinding_USCOREIWandaMsgProxy_H
#define soapBasicHttpBinding_USCOREIWandaMsgProxy_H
#include "soapH.h"
class BasicHttpBinding_USCOREIWandaMsg
{   public:
	/// Runtime engine context allocated in constructor
	struct soap *soap;
	/// Endpoint URL of service 'BasicHttpBinding_USCOREIWandaMsg' (change as needed)
	const char *endpoint;
	/// Constructor allocates soap engine context, sets default endpoint URL, and sets namespace mapping table
	BasicHttpBinding_USCOREIWandaMsg()
	{ soap = soap_new(); endpoint = "http://10.199.82.184:20001/WandaMsgService.svc"; if (soap && !soap->namespaces) { static const struct Namespace namespaces[] = 
{
	{"SOAP-ENV", "http://schemas.xmlsoap.org/soap/envelope/", "http://www.w3.org/*/soap-envelope", NULL},
	{"SOAP-ENC", "http://schemas.xmlsoap.org/soap/encoding/", "http://www.w3.org/*/soap-encoding", NULL},
	{"xsi", "http://www.w3.org/2001/XMLSchema-instance", "http://www.w3.org/*/XMLSchema-instance", NULL},
	{"xsd", "http://www.w3.org/2001/XMLSchema", "http://www.w3.org/*/XMLSchema", NULL},
	{"ns4", "http://schemas.datacontract.org/2004/07/NTS.WEB.DataContact.EMCS", NULL, NULL},
	{"ns3", "http://schemas.microsoft.com/2003/10/Serialization/", NULL, NULL},
	{"ns1", "http://tempuri.org/", NULL, NULL},
	{NULL, NULL, NULL, NULL}
};
	soap->namespaces = namespaces; } };
	/// Destructor frees deserialized data and soap engine context
	virtual ~BasicHttpBinding_USCOREIWandaMsg() { if (soap) { soap_destroy(soap); soap_end(soap); soap_free(soap); } };
	/// Invoke 'SetRTXInfo' of service 'BasicHttpBinding_USCOREIWandaMsg' and return error code (or SOAP_OK)
	virtual int __ns1__SetRTXInfo(_ns1__SetRTXInfo *ns1__SetRTXInfo, _ns1__SetRTXInfoResponse *ns1__SetRTXInfoResponse) { return soap ? soap_call___ns1__SetRTXInfo(soap, endpoint, NULL, ns1__SetRTXInfo, ns1__SetRTXInfoResponse) : SOAP_EOM; };
	/// Invoke 'SetSMSInfo' of service 'BasicHttpBinding_USCOREIWandaMsg' and return error code (or SOAP_OK)
	virtual int __ns1__SetSMSInfo(_ns1__SetSMSInfo *ns1__SetSMSInfo, _ns1__SetSMSInfoResponse *ns1__SetSMSInfoResponse) { return soap ? soap_call___ns1__SetSMSInfo(soap, endpoint, NULL, ns1__SetSMSInfo, ns1__SetSMSInfoResponse) : SOAP_EOM; };
	/// Invoke 'SetEmailInfo' of service 'BasicHttpBinding_USCOREIWandaMsg' and return error code (or SOAP_OK)
	virtual int __ns1__SetEmailInfo(_ns1__SetEmailInfo *ns1__SetEmailInfo, _ns1__SetEmailInfoResponse *ns1__SetEmailInfoResponse) { return soap ? soap_call___ns1__SetEmailInfo(soap, endpoint, NULL, ns1__SetEmailInfo, ns1__SetEmailInfoResponse) : SOAP_EOM; };
	/// Invoke 'SetMMSInfo' of service 'BasicHttpBinding_USCOREIWandaMsg' and return error code (or SOAP_OK)
	virtual int __ns1__SetMMSInfo(_ns1__SetMMSInfo *ns1__SetMMSInfo, _ns1__SetMMSInfoResponse *ns1__SetMMSInfoResponse) { return soap ? soap_call___ns1__SetMMSInfo(soap, endpoint, NULL, ns1__SetMMSInfo, ns1__SetMMSInfoResponse) : SOAP_EOM; };
	/// Invoke 'SetRTXNotify' of service 'BasicHttpBinding_USCOREIWandaMsg' and return error code (or SOAP_OK)
	virtual int __ns1__SetRTXNotify(_ns1__SetRTXNotify *ns1__SetRTXNotify, _ns1__SetRTXNotifyResponse *ns1__SetRTXNotifyResponse) { return soap ? soap_call___ns1__SetRTXNotify(soap, endpoint, NULL, ns1__SetRTXNotify, ns1__SetRTXNotifyResponse) : SOAP_EOM; };
	/// Invoke 'SetRTXJSON' of service 'BasicHttpBinding_USCOREIWandaMsg' and return error code (or SOAP_OK)
	virtual int __ns1__SetRTXJSON(_ns1__SetRTXJSON *ns1__SetRTXJSON, _ns1__SetRTXJSONResponse *ns1__SetRTXJSONResponse) { return soap ? soap_call___ns1__SetRTXJSON(soap, endpoint, NULL, ns1__SetRTXJSON, ns1__SetRTXJSONResponse) : SOAP_EOM; };
	/// Invoke 'SetSMSJSON' of service 'BasicHttpBinding_USCOREIWandaMsg' and return error code (or SOAP_OK)
	virtual int __ns1__SetSMSJSON(_ns1__SetSMSJSON *ns1__SetSMSJSON, _ns1__SetSMSJSONResponse *ns1__SetSMSJSONResponse) { return soap ? soap_call___ns1__SetSMSJSON(soap, endpoint, NULL, ns1__SetSMSJSON, ns1__SetSMSJSONResponse) : SOAP_EOM; };
	/// Invoke 'SetEmailJSON' of service 'BasicHttpBinding_USCOREIWandaMsg' and return error code (or SOAP_OK)
	virtual int __ns1__SetEmailJSON(_ns1__SetEmailJSON *ns1__SetEmailJSON, _ns1__SetEmailJSONResponse *ns1__SetEmailJSONResponse) { return soap ? soap_call___ns1__SetEmailJSON(soap, endpoint, NULL, ns1__SetEmailJSON, ns1__SetEmailJSONResponse) : SOAP_EOM; };
	/// Invoke 'SetSMSInfoC' of service 'BasicHttpBinding_USCOREIWandaMsg' and return error code (or SOAP_OK)
	virtual int __ns1__SetSMSInfoC(_ns1__SetSMSInfoC *ns1__SetSMSInfoC, _ns1__SetSMSInfoCResponse *ns1__SetSMSInfoCResponse) { return soap ? soap_call___ns1__SetSMSInfoC(soap, endpoint, NULL, ns1__SetSMSInfoC, ns1__SetSMSInfoCResponse) : SOAP_EOM; };
	/// Invoke 'SetRTXInfoC' of service 'BasicHttpBinding_USCOREIWandaMsg' and return error code (or SOAP_OK)
	virtual int __ns1__SetRTXInfoC(_ns1__SetRTXInfoC *ns1__SetRTXInfoC, _ns1__SetRTXInfoCResponse *ns1__SetRTXInfoCResponse) { return soap ? soap_call___ns1__SetRTXInfoC(soap, endpoint, NULL, ns1__SetRTXInfoC, ns1__SetRTXInfoCResponse) : SOAP_EOM; };
	/// Invoke 'SetEmailInfoC' of service 'BasicHttpBinding_USCOREIWandaMsg' and return error code (or SOAP_OK)
	virtual int __ns1__SetEmailInfoC(_ns1__SetEmailInfoC *ns1__SetEmailInfoC, _ns1__SetEmailInfoCResponse *ns1__SetEmailInfoCResponse) { return soap ? soap_call___ns1__SetEmailInfoC(soap, endpoint, NULL, ns1__SetEmailInfoC, ns1__SetEmailInfoCResponse) : SOAP_EOM; };
	/// Invoke 'SetMMSInfoC' of service 'BasicHttpBinding_USCOREIWandaMsg' and return error code (or SOAP_OK)
	virtual int __ns1__SetMMSInfoC(_ns1__SetMMSInfoC *ns1__SetMMSInfoC, _ns1__SetMMSInfoCResponse *ns1__SetMMSInfoCResponse) { return soap ? soap_call___ns1__SetMMSInfoC(soap, endpoint, NULL, ns1__SetMMSInfoC, ns1__SetMMSInfoCResponse) : SOAP_EOM; };
	/// Invoke 'SetRTXNotifyC' of service 'BasicHttpBinding_USCOREIWandaMsg' and return error code (or SOAP_OK)
	virtual int __ns1__SetRTXNotifyC(_ns1__SetRTXNotifyC *ns1__SetRTXNotifyC, _ns1__SetRTXNotifyCResponse *ns1__SetRTXNotifyCResponse) { return soap ? soap_call___ns1__SetRTXNotifyC(soap, endpoint, NULL, ns1__SetRTXNotifyC, ns1__SetRTXNotifyCResponse) : SOAP_EOM; };
};
#endif
