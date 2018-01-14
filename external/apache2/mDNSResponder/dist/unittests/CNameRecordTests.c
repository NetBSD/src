#include "CNameRecordTests.h"
#include "unittest_common.h"

mDNSlocal int InitThisUnitTest(void);
mDNSlocal int StartClientQueryRequest(void);
mDNSlocal int PopulateCacheWithClientResponseRecords(void);
mDNSlocal int SimulateNetworkChangeAndVerifyTest(void);
mDNSlocal int FinalizeUnitTest(void);
mDNSlocal mStatus AddDNSServer(void);

// This unit test's variables
static UDPSocket* local_socket;
static request_state* client_request_message;

struct UDPSocket_struct
{
	mDNSIPPort port; // MUST BE FIRST FIELD -- mDNSCoreReceive expects every UDPSocket_struct to begin with mDNSIPPort port
};
typedef struct UDPSocket_struct UDPSocket;

// This client request was generated using the following command: "dns-sd -Q 123server.dotbennu.com. A".
uint8_t query_client_msgbuf[35] = {
	0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x32, 0x33, 0x73, 0x65, 0x72, 0x76, 0x65,
	0x72, 0x2e, 0x64, 0x6f, 0x74, 0x62, 0x65, 0x6e, 0x6e, 0x75, 0x2e, 0x63, 0x6f, 0x6d, 0x00, 0x00,
	0x01, 0x00, 0x01
};

// This uDNS message is a canned response that was originally captured by wireshark.
uint8_t query_response_msgbuf[108] = {
    0x69, 0x41, // transaction id
	0x85, 0x80, // flags
	0x00, 0x01, // 1 question for 123server.dotbennu.com. Addr
	0x00, 0x02,	// 2 anwsers: 123server.dotbennu.com. CNAME test212.dotbennu.com., test212.dotbennu.com. Addr 10.100.0.1,
	0x00, 0x01,	// 1 authorities anwser: dotbennu.com. NS cardinal2.apple.com.
	0x00, 0x00, 0x09, 0x31, 0x32, 0x33,
    0x73, 0x65, 0x72, 0x76, 0x65, 0x72, 0x08, 0x64, 0x6f, 0x74, 0x62, 0x65, 0x6e, 0x6e, 0x75, 0x03,
    0x63, 0x6f, 0x6d, 0x00, 0x00, 0x01, 0x00, 0x01, 0xc0, 0x0c, 0x00, 0x05, 0x00, 0x01, 0x00, 0x00,
    0x02, 0x56, 0x00, 0x0a, 0x07, 0x74, 0x65, 0x73, 0x74, 0x32, 0x31, 0x32, 0xc0, 0x16, 0xc0, 0x34,
    0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x04, 0x0a, 0x64, 0x00, 0x01, 0xc0, 0x16,
    0x00, 0x02, 0x00, 0x01, 0x00, 0x01, 0x51, 0x80, 0x00, 0x12, 0x09, 0x63, 0x61, 0x72, 0x64, 0x69,
    0x6e, 0x61, 0x6c, 0x32, 0x05, 0x61, 0x70, 0x70, 0x6c, 0x65, 0xc0, 0x1f
};

// Variables associated with contents of the above uDNS message
#define uDNS_TargetQID 16745
char udns_original_domainname_cstr[] = "123server.dotbennu.com.";
char udns_cname_domainname_cstr[] = "test212.dotbennu.com.";
static const mDNSv4Addr dns_response_ipv4 = {{ 10, 100, 0, 1 }};

UNITTEST_HEADER(CNameRecordTests)
	UNITTEST_TEST(InitThisUnitTest)
	UNITTEST_TEST(StartClientQueryRequest)
    UNITTEST_TEST(PopulateCacheWithClientResponseRecords)
    UNITTEST_TEST(SimulateNetworkChangeAndVerifyTest)
    UNITTEST_TEST(FinalizeUnitTest)
UNITTEST_FOOTER

// The InitThisUnitTest() initializes the mDNSResponder environment as well as
// a DNSServer. It also allocates memory for a local_socket and client request.
// Note: This unit test does not send packets on the wire and it does not open sockets.
UNITTEST_HEADER(InitThisUnitTest)
	// Init unit test environment and verify no error occurred.
	mStatus result = init_mdns_environment(mDNStrue);
	UNITTEST_ASSERT(result == mStatus_NoError);

	// Add one DNS server and verify it was added.
	AddDNSServer();
	UNITTEST_ASSERT(NumUnicastDNSServers == 1);

	// Create memory for a socket that is never used or opened.
	local_socket = mDNSPlatformMemAllocate(sizeof(UDPSocket));
	mDNSPlatformMemZero(local_socket, sizeof(UDPSocket));

	// Create memory for a request that is used to make this unit test's client request.
	client_request_message = calloc(1, sizeof(request_state));
UNITTEST_FOOTER

// This test simulates a uds client request by setting up a client request and then
// calling mDNSResponder's handle_client_request.  The handle_client_request function
// processes the request and starts a query.  This unit test verifies
// the client request and query were setup as expected.  This unit test also calls
// mDNS_execute which determines the cache does not contain the new question's
// answer.
UNITTEST_HEADER(StartClientQueryRequest)
    mDNS *const m = &mDNSStorage;
    request_state* req = client_request_message;
	char *msgptr = (char *)query_client_msgbuf;
	size_t msgsz = sizeof(query_client_msgbuf);
	mDNSs32 min_size = sizeof(DNSServiceFlags) + sizeof(mDNSu32) + 4;
    DNSQuestion *q;
    mStatus err = mStatus_NoError;
	char qname_cstr[MAX_ESCAPED_DOMAIN_NAME];

    // Process the unit test's client request
	start_client_request(req, msgptr, msgsz, query_request, local_socket);
    UNITTEST_ASSERT(err == mStatus_NoError);

	// Verify the request fields were set as expected
	UNITTEST_ASSERT(req->next == mDNSNULL);
	UNITTEST_ASSERT(req->primary == mDNSNULL);
	UNITTEST_ASSERT(req->sd == client_req_sd);
	UNITTEST_ASSERT(req->process_id == client_req_process_id);
	UNITTEST_ASSERT(!strcmp(req->pid_name, client_req_pid_name));
	UNITTEST_ASSERT(req->validUUID == mDNSfalse);
	UNITTEST_ASSERT(req->errsd == 0);
	UNITTEST_ASSERT(req->uid == client_req_uid);
	UNITTEST_ASSERT(req->ts == t_complete);
	UNITTEST_ASSERT((mDNSs32)req->data_bytes > min_size);
	UNITTEST_ASSERT(req->msgend == msgptr+msgsz);
	UNITTEST_ASSERT(req->msgbuf == mDNSNULL);
    UNITTEST_ASSERT(req->hdr.version == VERSION);
    UNITTEST_ASSERT(req->replies == mDNSNULL);
    UNITTEST_ASSERT(req->terminate != mDNSNULL);
	UNITTEST_ASSERT(req->flags == kDNSServiceFlagsReturnIntermediates);
    UNITTEST_ASSERT(req->interfaceIndex == kDNSServiceInterfaceIndexAny);

	// Verify the query fields were set as expected
	q = &req->u.queryrecord.q;
    UNITTEST_ASSERT(q != mDNSNULL);
    UNITTEST_ASSERT(q == m->Questions);
    UNITTEST_ASSERT(q == m->NewQuestions);
    UNITTEST_ASSERT(q->SuppressUnusable == mDNSfalse);
    UNITTEST_ASSERT(q->ReturnIntermed == mDNStrue);
    UNITTEST_ASSERT(q->SuppressQuery == mDNSfalse);

	UNITTEST_ASSERT(q->qnameOrig == mDNSNULL);
	ConvertDomainNameToCString(&q->qname, qname_cstr);
	UNITTEST_ASSERT(!strcmp(qname_cstr, udns_original_domainname_cstr));
	UNITTEST_ASSERT(q->qnamehash == DomainNameHashValue(&q->qname));

	UNITTEST_ASSERT(q->InterfaceID == mDNSInterface_Any);
    UNITTEST_ASSERT(q->flags == req->flags);
    UNITTEST_ASSERT(q->qtype == 1);
    UNITTEST_ASSERT(q->qclass == 1);
    UNITTEST_ASSERT(q->LongLived == 0);
    UNITTEST_ASSERT(q->ExpectUnique == mDNSfalse);
    UNITTEST_ASSERT(q->ForceMCast == 0);
    UNITTEST_ASSERT(q->TimeoutQuestion == 0);
    UNITTEST_ASSERT(q->WakeOnResolve == 0);
    UNITTEST_ASSERT(q->UseBackgroundTrafficClass == 0);
    UNITTEST_ASSERT(q->ValidationRequired == 0);
    UNITTEST_ASSERT(q->ValidatingResponse == 0);
    UNITTEST_ASSERT(q->ProxyQuestion == 0);
    UNITTEST_ASSERT(q->AnonInfo == mDNSNULL);
    UNITTEST_ASSERT(q->QuestionCallback != mDNSNULL);
    UNITTEST_ASSERT(q->QuestionContext == req);
    UNITTEST_ASSERT(q->SearchListIndex == 0);
    UNITTEST_ASSERT(q->DNSSECAuthInfo == mDNSNULL);
    UNITTEST_ASSERT(q->DAIFreeCallback == mDNSNULL);
    UNITTEST_ASSERT(q->RetryWithSearchDomains == 0);
    UNITTEST_ASSERT(q->AppendSearchDomains == 0);
    UNITTEST_ASSERT(q->AppendLocalSearchDomains == 0);
    UNITTEST_ASSERT(q->DuplicateOf == mDNSNULL);

    // Call mDNS_Execute to see if the new question, q, has an answer in the cache.
	// It won't be yet because the cache is empty.
	m->NextScheduledEvent = mDNS_TimeNow_NoLock(m);
    mDNS_Execute(m);

    // Verify mDNS_Execute processed the new question.
	UNITTEST_ASSERT(m->NewQuestions == mDNSNULL);

	// Verify the cache is empty and the request got no reply.
	UNITTEST_ASSERT(m->rrcache_totalused == 0);
    UNITTEST_ASSERT(req->replies == mDNSNULL);

UNITTEST_FOOTER

// This unit test receives a canned uDNS response message by calling the mDNSCoreReceive() function.
// It then verifies cache entries were added for the CNAME and A records that were contained in the
// answers of the canned response, query_response_msgbuf.  This unit test also verifies that
// 2 add events were generated for the client.
UNITTEST_HEADER(PopulateCacheWithClientResponseRecords)
    mDNS *const m = &mDNSStorage;
	DNSMessage *msgptr = (DNSMessage *)query_response_msgbuf;
	size_t msgsz = sizeof(query_response_msgbuf);
    struct reply_state *reply;
    request_state* req = client_request_message;
    DNSQuestion *q = &req->u.queryrecord.q;
    const char *data;
	const char *end;
    char name[kDNSServiceMaxDomainName];
    uint16_t rrtype, rrclass, rdlen;
    const char *rdata;
    size_t len;
    char domainname_cstr[MAX_ESCAPED_DOMAIN_NAME];

	// Receive and populate the cache with canned response
    receive_response(req, msgptr, msgsz);

    // Verify 2 cache entries for CName and A record are present
    mDNSu32 CacheUsed =0, notUsed =0;
    LogCacheRecords_ut(mDNS_TimeNow(m), &CacheUsed, &notUsed);
    UNITTEST_ASSERT(CacheUsed == m->rrcache_totalused);
    UNITTEST_ASSERT(CacheUsed  == 4); // 2 for the CacheGroup object plus 2 for the A and CNAME records
    UNITTEST_ASSERT(m->PktNum  == 1); // one packet was received

	// Verify question's qname is now set with the A record's domainname
	UNITTEST_ASSERT(q->qnameOrig == mDNSNULL);
	ConvertDomainNameToCString(&q->qname, domainname_cstr);
	UNITTEST_ASSERT(q->qnamehash == DomainNameHashValue(&q->qname));
	UNITTEST_ASSERT(!strcmp(domainname_cstr, udns_cname_domainname_cstr));

	// Verify client's add event for CNAME is properly formed
    reply = req->replies;
    UNITTEST_ASSERT(reply != mDNSNULL);
    UNITTEST_ASSERT(reply->next == mDNSNULL);

    data    = (char *)&reply->rhdr[1];
    end     = data+reply->totallen;
    get_string(&data, data+reply->totallen, name, kDNSServiceMaxDomainName);
    rrtype  = get_uint16(&data, end);
    rrclass = get_uint16(&data, end);
    rdlen   = get_uint16(&data, end);
    rdata   = get_rdata(&data, end, rdlen);
	len     = get_reply_len(name, rdlen);

    UNITTEST_ASSERT(reply->totallen == len + sizeof(ipc_msg_hdr));
    UNITTEST_ASSERT(reply->mhdr->version == VERSION);
    UNITTEST_ASSERT(reply->mhdr->datalen == len);
    UNITTEST_ASSERT(reply->mhdr->ipc_flags == 0);
    UNITTEST_ASSERT(reply->mhdr->op == query_reply_op);

    UNITTEST_ASSERT(reply->rhdr->flags == htonl(kDNSServiceFlagsAdd));
    UNITTEST_ASSERT(reply->rhdr->ifi == kDNSServiceInterfaceIndexAny);
    UNITTEST_ASSERT(reply->rhdr->error == kDNSServiceErr_NoError);

    UNITTEST_ASSERT(rrtype == kDNSType_CNAME);
    UNITTEST_ASSERT(rrclass == kDNSClass_IN);
    ConvertDomainNameToCString((const domainname *const)rdata, domainname_cstr);
    UNITTEST_ASSERT(!strcmp(domainname_cstr, "test212.dotbennu.com."));

    // The mDNS_Execute call generates an add event for the A record
    m->NextScheduledEvent = mDNS_TimeNow_NoLock(m);
    mDNS_Execute(m);

    // Verify the client's reply contains a properly formed add event for the A record.
    reply = req->replies;
    UNITTEST_ASSERT(reply != mDNSNULL);
    UNITTEST_ASSERT(reply->next != mDNSNULL);
    reply = reply->next;

    data    = (char *)&reply->rhdr[1];
    end     = data+reply->totallen;
    get_string(&data, data+reply->totallen, name, kDNSServiceMaxDomainName);
    rrtype  = get_uint16(&data, end);
    rrclass = get_uint16(&data, end);
    rdlen   = get_uint16(&data, end);
    rdata   = get_rdata(&data, end, rdlen);
	len     = get_reply_len(name, rdlen);

    UNITTEST_ASSERT(reply->totallen == len + sizeof(ipc_msg_hdr));
    UNITTEST_ASSERT(reply->mhdr->version == VERSION);
    UNITTEST_ASSERT(reply->mhdr->datalen == len);

    UNITTEST_ASSERT(reply->mhdr->ipc_flags == 0);
    UNITTEST_ASSERT(reply->mhdr->op == query_reply_op);

    UNITTEST_ASSERT(reply->rhdr->flags == htonl(kDNSServiceFlagsAdd));
    UNITTEST_ASSERT(reply->rhdr->ifi == kDNSServiceInterfaceIndexAny);
    UNITTEST_ASSERT(reply->rhdr->error == kDNSServiceErr_NoError);

    UNITTEST_ASSERT(rrtype == kDNSType_A);
    UNITTEST_ASSERT(rrclass == kDNSClass_IN);
	UNITTEST_ASSERT(rdata[0] == dns_response_ipv4.b[0]);
	UNITTEST_ASSERT(rdata[1] == dns_response_ipv4.b[1]);
	UNITTEST_ASSERT(rdata[2] == dns_response_ipv4.b[2]);
	UNITTEST_ASSERT(rdata[3] == dns_response_ipv4.b[3]);

UNITTEST_FOOTER

// This function verifies the cache and event handling occurred as expected when a network change happened.
// The uDNS_SetupDNSConfig is called to simulate a network change and two outcomes occur. First the A record
// query is restarted and sent to a new DNS server. Second the cache records are purged. Then mDNS_Execute
// is called and it removes the purged cache records and generates a remove event for the A record.
// The following are verified:
// 	1.) The restart of query for A record.
//	2.) The cache is empty after mDNS_Execute removes the cache entres.
//  3.) The remove event is verified by examining the request's reply data.
UNITTEST_HEADER(SimulateNetworkChangeAndVerifyTest)
    mDNS *const m = &mDNSStorage;
    request_state*  req = client_request_message;
    DNSQuestion*    q = &req->u.queryrecord.q;
    mDNSu32 CacheUsed =0, notUsed =0;
    const char *data;	const char *end;
    char name[kDNSServiceMaxDomainName];
    uint16_t rrtype, rrclass, rdlen;
    const char *rdata;
    size_t len;

    // The uDNS_SetupDNSConfig reconfigures the resolvers so the A record query is restarted and
    // both the CNAME and A record are purged.
    uDNS_SetupDNSConfig(m);

    // Verify the A record query was restarted.  This is done indirectly by noticing the transaction id and interval have changed.
    UNITTEST_ASSERT(q->ThisQInterval == InitialQuestionInterval);
    UNITTEST_ASSERT(q->TargetQID.NotAnInteger != uDNS_TargetQID);

    // Then mDNS_Execute removes both records from the cache and calls the client back with a remove event for A record.
    m->NextScheduledEvent = mDNS_TimeNow_NoLock(m);
    mDNS_Execute(m);

    // Verify the cache entries are removed
    LogCacheRecords_ut(mDNS_TimeNow(m), &CacheUsed, &notUsed);
    UNITTEST_ASSERT(CacheUsed == m->rrcache_totalused);
    UNITTEST_ASSERT(CacheUsed == 0);

    // Verify the A record's remove event is setup as expected in the reply data
    struct reply_state *reply;
    reply = req->replies;
    UNITTEST_ASSERT(reply != mDNSNULL);

    UNITTEST_ASSERT(reply != mDNSNULL);
    UNITTEST_ASSERT(reply->next != mDNSNULL);
    UNITTEST_ASSERT(reply->next->next != mDNSNULL);

    reply = reply->next->next; // Get to last event to verify remove event
    data    = (char *)&reply->rhdr[1];
    end     = data+reply->totallen;
    get_string(&data, data+reply->totallen, name, kDNSServiceMaxDomainName);
    rrtype  = get_uint16(&data, end);
    rrclass = get_uint16(&data, end);
    rdlen   = get_uint16(&data, end);
    rdata   = get_rdata(&data, end, rdlen);
	len     = get_reply_len(name, rdlen);

    UNITTEST_ASSERT(reply->totallen == reply->mhdr->datalen + sizeof(ipc_msg_hdr));
    UNITTEST_ASSERT(reply->mhdr->version == VERSION);
    UNITTEST_ASSERT(reply->mhdr->datalen == len);
    UNITTEST_ASSERT(reply->mhdr->ipc_flags == 0);
    UNITTEST_ASSERT(reply->mhdr->op == query_reply_op);

    UNITTEST_ASSERT(reply->rhdr->flags != htonl(kDNSServiceFlagsAdd));
    UNITTEST_ASSERT(reply->rhdr->ifi == kDNSServiceInterfaceIndexAny);
    UNITTEST_ASSERT(reply->rhdr->error == kDNSServiceErr_NoError);

    UNITTEST_ASSERT(rrtype == kDNSType_A);
    UNITTEST_ASSERT(rrclass == kDNSClass_IN);
	UNITTEST_ASSERT(rdata[0] == dns_response_ipv4.b[0]);
	UNITTEST_ASSERT(rdata[1] == dns_response_ipv4.b[1]);
	UNITTEST_ASSERT(rdata[2] == dns_response_ipv4.b[2]);
	UNITTEST_ASSERT(rdata[3] == dns_response_ipv4.b[3]);

UNITTEST_FOOTER

// This function does memory cleanup and no verification.
UNITTEST_HEADER(FinalizeUnitTest)
    mDNS *m = &mDNSStorage;
    request_state* req = client_request_message;
    DNSServer   *ptr, **p = &m->DNSServers;

    while (req->replies)
    {
        reply_state *reply = req->replies;
        req->replies = req->replies->next;
		mDNSPlatformMemFree(reply);
    }
    mDNSPlatformMemFree(req);

    mDNSPlatformMemFree(local_socket);

    while (*p)
    {
        ptr = *p;
        *p = (*p)->next;
        LogInfo("FinalizeUnitTest: Deleting server %p %#a:%d (%##s)", ptr, &ptr->addr, mDNSVal16(ptr->port), ptr->domain.c);
        mDNSPlatformMemFree(ptr);
    }
UNITTEST_FOOTER

// The mDNS_AddDNSServer function adds a dns server to mDNSResponder's list.
mDNSlocal mStatus AddDNSServer(void)
{
    mDNS *m = &mDNSStorage;
    m->timenow = 0;
    mDNS_Lock(m);
    domainname	d;
    mDNSAddr	addr;
    mDNSIPPort	port;
    mDNSs32		serviceID		= 0;
    mDNSu32		scoped			= 0;
    mDNSu32		timeout			= dns_server_timeout;
    mDNSBool	cellIntf		= 0;
    mDNSBool	isExpensive		= 0;
    mDNSu16		resGroupID		= dns_server_resGroupID;
    mDNSBool	reqA			= mDNStrue;
    mDNSBool	reqAAAA			= mDNStrue;
    mDNSBool	reqDO			= mDNSfalse;
    d.c[0]						= 0;
    addr.type					= mDNSAddrType_IPv4;
    addr.ip.v4.NotAnInteger		= dns_server_ipv4.NotAnInteger;
    port.NotAnInteger			= client_resp_src_port;
	mDNS_AddDNSServer(m, &d, primary_interfaceID, serviceID, &addr, port, scoped, timeout,
                      cellIntf, isExpensive, resGroupID,
					  reqA, reqAAAA, reqDO);
	mDNS_Unlock(m);
	return mStatus_NoError;
}


