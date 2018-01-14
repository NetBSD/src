#include "LocalOnlyTimeoutTests.h"
#include "unittest_common.h"

mDNSlocal int InitUnitTest(void);
mDNSlocal int StartLocalOnlyClientQueryRequest(void);
mDNSlocal int PopulateCacheWithClientLOResponseRecords(void);
mDNSlocal int RestartLocalOnlyClientQueryRequest(void);
mDNSlocal int FinalizeUnitTest(void);
mDNSlocal mStatus InitEtcHostsRecords();

// This unit test's variables
static request_state* client_request_message;
static UDPSocket* local_socket;
static char domainname_cstr[MAX_ESCAPED_DOMAIN_NAME];

// This query request message was generated from the following command: "dns-sd -lo -timeout -Q cardinal2.apple.com. A"
char query_req_msgbuf[33]= {
	0x00, 0x01, 0x90, 0x00,
	// DNSServiceFlags.L = (kDNSServiceFlagsReturnIntermediates |kDNSServiceFlagsSuppressUnusable | kDNSServiceFlagsTimeout)
	0xff, 0xff, 0xff, 0xff,
	// interfaceIndex = mDNSInterface_LocalOnly
	0x63, 0x61, 0x72, 0x64, 0x69, 0x6e, 0x61, 0x6c,
	0x32, 0x2e, 0x61, 0x70, 0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d, 0x2e, 0x00, 0x00, 0x01, 0x00,
	0x01
};

UNITTEST_HEADER(LocalOnlyTimeoutTests)
	UNITTEST_TEST(InitUnitTest)
	UNITTEST_TEST(StartLocalOnlyClientQueryRequest)
	UNITTEST_TEST(PopulateCacheWithClientLOResponseRecords)
	UNITTEST_TEST(RestartLocalOnlyClientQueryRequest)
	UNITTEST_TEST(FinalizeUnitTest)
UNITTEST_FOOTER

// The InitUnitTest() initializes a minimal mDNSResponder environment as
// well as allocates memory for a local_socket and client request.
// It also sets the domainname_cstr specified in the client's query request.
// Note: This unit test does not send packets on the wire and it does not open sockets.
UNITTEST_HEADER(InitUnitTest)

	// Init mDNSStorage
	mStatus result = init_mdns_storage();
	if (result != mStatus_NoError)
		return result;

	// Allocate a client request
	local_socket = calloc(1, sizeof(request_state));

	// Allocate memory for a request that is used to make client requests.
	client_request_message = calloc(1, sizeof(request_state));

	// Init domainname that is used by unit tests
	strlcpy(domainname_cstr, "cardinal2.apple.com.", sizeof(domainname_cstr));

UNITTEST_FOOTER

// This unit test starts a local only request for "cardinal2.apple.com.".  It first
// calls start_client_request to start a query, it then verifies the
// req and query data structures are set as expected. Next, the cache is verified to
// be empty by AnswerNewLocalOnlyQuestion() and so results in GenerateNegativeResponse()
// getting called which sets up a reply with a negative answer in it for the client.
// On return from mDNS_Execute, the client's reply structure is verified to be set as
// expected. Lastly the timeout is simulated and mDNS_Execute is called. This results
// in a call to TimeoutQuestions(). And again, the GenerateNegativeResponse() is called
// which returns a negative response to the client.  This time the client reply is verified
// to be setup with a timeout result.
UNITTEST_HEADER(StartLocalOnlyClientQueryRequest)

	mDNS *const m = &mDNSStorage;
    request_state* req = client_request_message;
	char *msgptr = (char *)query_req_msgbuf;
	size_t msgsz = sizeof(query_req_msgbuf);
	DNSQuestion *q;
	mDNSs32 min_size = sizeof(DNSServiceFlags) + sizeof(mDNSu32) + 4;
	mStatus err = mStatus_NoError;
	char qname_cstr[MAX_ESCAPED_DOMAIN_NAME];
	struct reply_state *reply;
	size_t len;

	// Process the unit test's client request
	start_client_request(req, msgptr, msgsz, query_request, local_socket);
	UNITTEST_ASSERT(err == mStatus_NoError);

	// Verify the query initialized and request fields were set as expected
	UNITTEST_ASSERT(err == mStatus_NoError);
	UNITTEST_ASSERT(req->hdr.version == VERSION);
	UNITTEST_ASSERT((mDNSs32)req->data_bytes > min_size);
	UNITTEST_ASSERT(req->flags == (kDNSServiceFlagsSuppressUnusable | kDNSServiceFlagsReturnIntermediates | kDNSServiceFlagsTimeout));
	UNITTEST_ASSERT(req->interfaceIndex == kDNSServiceInterfaceIndexLocalOnly);
	UNITTEST_ASSERT(req->terminate != mDNSNULL);

	q = &req->u.queryrecord.q;
	UNITTEST_ASSERT(q == m->NewLocalOnlyQuestions);
	UNITTEST_ASSERT(m->Questions == NULL);
	UNITTEST_ASSERT(m->NewQuestions == NULL);
	UNITTEST_ASSERT(q->SuppressUnusable == 1);
	UNITTEST_ASSERT(q->ReturnIntermed == 1);
	UNITTEST_ASSERT(q->SuppressQuery == 0);									// Regress <rdar://problem/27571734>

	UNITTEST_ASSERT(q->qnameOrig == mDNSNULL);
	ConvertDomainNameToCString(&q->qname, qname_cstr);
	UNITTEST_ASSERT(!strcmp(qname_cstr, domainname_cstr));
	UNITTEST_ASSERT(q->qnamehash == DomainNameHashValue(&q->qname));

	UNITTEST_ASSERT(q->InterfaceID == mDNSInterface_LocalOnly);
	UNITTEST_ASSERT(q->flags == req->flags);
	UNITTEST_ASSERT(q->qtype == 1);
	UNITTEST_ASSERT(q->qclass == 1);
	UNITTEST_ASSERT(q->LongLived == 0);
	UNITTEST_ASSERT(q->ExpectUnique == mDNSfalse);
	UNITTEST_ASSERT(q->ForceMCast == 0);
	UNITTEST_ASSERT(q->TimeoutQuestion == 1);
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
	UNITTEST_ASSERT(q->StopTime != 0);
	UNITTEST_ASSERT(q->AppendSearchDomains == 0);
	UNITTEST_ASSERT(q->AppendLocalSearchDomains == 0);
	UNITTEST_ASSERT(q->DuplicateOf == mDNSNULL);

	// At this point the the cache is empty. Calling mDNS_Execute will answer the local-only
	// question with a negative response.
	m->NextScheduledEvent = mDNS_TimeNow_NoLock(m);
	mDNS_Execute(m);  // Regress <rdar://problem/28721294>

	// Verify reply is a negative response and error code is set to kDNSServiceErr_NoSuchRecord error.
	reply = req->replies;
	UNITTEST_ASSERT(reply != mDNSNULL);

	UNITTEST_ASSERT(m->NewLocalOnlyQuestions == NULL);
	UNITTEST_ASSERT(q->LOAddressAnswers == 0);

	len = get_reply_len(qname_cstr, 0);

	UNITTEST_ASSERT(reply->next == mDNSNULL);
	UNITTEST_ASSERT(reply->totallen == reply->mhdr->datalen + sizeof(ipc_msg_hdr));
	UNITTEST_ASSERT(reply->mhdr->version == VERSION);
	UNITTEST_ASSERT(reply->mhdr->datalen == len);
	UNITTEST_ASSERT(reply->mhdr->ipc_flags == 0);
	UNITTEST_ASSERT(reply->mhdr->op == query_reply_op);

	UNITTEST_ASSERT(reply->rhdr->flags == htonl(kDNSServiceFlagsAdd));
	UNITTEST_ASSERT(reply->rhdr->ifi == kDNSServiceInterfaceIndexLocalOnly);	// Regress <rdar://problem/27340874>
	UNITTEST_ASSERT(reply->rhdr->error ==
					(DNSServiceErrorType)htonl(kDNSServiceErr_NoSuchRecord));	// Regress <rdar://problem/24827555>

	// Simulate what udsserver_idle normally does for clean up
	freeL("StartLocalOnlyClientQueryRequest:reply", reply);
	req->replies = NULL;

	// Simulate the query time out of the local-only question.
	// The expected behavior is a negative answer with time out error
	m->NextScheduledEvent = mDNS_TimeNow_NoLock(m);
	q->StopTime = mDNS_TimeNow_NoLock(m);
	m->NextScheduledStopTime -= mDNSPlatformOneSecond*5;
	mDNS_Execute(m);

	// Verify the reply is a negative response with timeout error.
	reply = req->replies;
	UNITTEST_ASSERT(reply != NULL);
	UNITTEST_ASSERT(m->NewLocalOnlyQuestions == NULL);
	UNITTEST_ASSERT(q->LOAddressAnswers == 0);

	len = get_reply_len(qname_cstr, 0);

	UNITTEST_ASSERT(reply->next == mDNSNULL);
	UNITTEST_ASSERT(reply->totallen == len + sizeof(ipc_msg_hdr));
	UNITTEST_ASSERT(reply->mhdr->version == VERSION);
	UNITTEST_ASSERT(reply->mhdr->datalen == len);
	UNITTEST_ASSERT(reply->mhdr->ipc_flags == 0);
	UNITTEST_ASSERT(reply->mhdr->op == query_reply_op);
	UNITTEST_ASSERT(reply->rhdr->flags == htonl(kDNSServiceFlagsAdd));
	UNITTEST_ASSERT(reply->rhdr->ifi == kDNSServiceInterfaceIndexLocalOnly);	// Regress <rdar://problem/27340874>
	UNITTEST_ASSERT(reply->rhdr->error ==
					(DNSServiceErrorType)htonl(kDNSServiceErr_Timeout));		// Regress <rdar://problem/27562965>

	// Free request and reallocate to use when query is restarted
	free_req(req);
	client_request_message = calloc(1, sizeof(request_state));

UNITTEST_FOOTER

// This unit test populates the cache with four /etc/hosts records and then
// verifies there are four entries in the cache.
UNITTEST_HEADER(PopulateCacheWithClientLOResponseRecords)

	mDNS *const m = &mDNSStorage;

	// Verify cache is empty
	int count = LogEtcHosts_ut(m);
	UNITTEST_ASSERT(count == 0);

	// Populate /etc/hosts
	mStatus result = InitEtcHostsRecords();
	UNITTEST_ASSERT(result == mStatus_NoError);

	// mDNS_Execute is called to populate the /etc/hosts cache.
	m->NextScheduledEvent = mDNS_TimeNow_NoLock(m);
	mDNS_Execute(m);

	count = LogEtcHosts_ut(m);
	UNITTEST_ASSERT(count == 4);

UNITTEST_FOOTER

// This unit test starts a local only request for "cardinal2.apple.com.".  It first
// calls start_client_request to start a query, it then verifies the
// req and query data structures are set as expected. Next, the cache is verified to
// contain the answer by AnswerNewLocalOnlyQuestion() and so results in setting up an
// answer reply to the client. On return from mDNS_Execute, the client's reply structure
// is verified to be set as expected. Lastly the timeout is simulated and mDNS_Execute is
// called. This results in a call to TimeoutQuestions(). And this time, the
// GenerateNegativeResponse() is called which returns a negative response to the client
// which specifies the timeout occurred. Again, the answer reply is verified to
// to specify a timeout.
UNITTEST_HEADER(RestartLocalOnlyClientQueryRequest)

	mDNS *const m = &mDNSStorage;
	request_state* req = client_request_message;
	char *msgptr = (char *)query_req_msgbuf;
	size_t msgsz = sizeof(query_req_msgbuf);	DNSQuestion *q;
	mDNSs32 min_size = sizeof(DNSServiceFlags) + sizeof(mDNSu32) + 4;
	mStatus err = mStatus_NoError;
	char qname_cstr[MAX_ESCAPED_DOMAIN_NAME];
	struct reply_state *reply;
	size_t len;

	// Process the unit test's client request
	start_client_request(req, msgptr, msgsz, query_request, local_socket);
	UNITTEST_ASSERT(err == mStatus_NoError);

	UNITTEST_ASSERT(err == mStatus_NoError);
	UNITTEST_ASSERT(req->hdr.version == VERSION);
	UNITTEST_ASSERT((mDNSs32)req->data_bytes > min_size);
	UNITTEST_ASSERT(req->flags == (kDNSServiceFlagsSuppressUnusable | kDNSServiceFlagsReturnIntermediates | kDNSServiceFlagsTimeout));
	UNITTEST_ASSERT(req->interfaceIndex == kDNSServiceInterfaceIndexLocalOnly);
	UNITTEST_ASSERT(req->terminate != mDNSNULL);
	UNITTEST_ASSERT(m->Questions == NULL);

	q = &req->u.queryrecord.q;
	UNITTEST_ASSERT(q == m->NewLocalOnlyQuestions);
	UNITTEST_ASSERT(q->SuppressUnusable == 1);
	UNITTEST_ASSERT(q->ReturnIntermed == 1);
	UNITTEST_ASSERT(q->SuppressQuery == 0);										// Regress <rdar://problem/27571734>
	UNITTEST_ASSERT(q->qnamehash == DomainNameHashValue(&q->qname));
	UNITTEST_ASSERT(q->InterfaceID == mDNSInterface_LocalOnly);
	UNITTEST_ASSERT(q->flags == req->flags);
	UNITTEST_ASSERT(q->qtype == 1);
	UNITTEST_ASSERT(q->qclass == 1);
	UNITTEST_ASSERT(q->LongLived == 0);
	UNITTEST_ASSERT(q->ExpectUnique == mDNSfalse);
	UNITTEST_ASSERT(q->ForceMCast == 0);
	UNITTEST_ASSERT(q->TimeoutQuestion == 1);
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
	UNITTEST_ASSERT(q->StopTime != 0);
	UNITTEST_ASSERT(q->AppendSearchDomains == 0);
	UNITTEST_ASSERT(q->AppendLocalSearchDomains == 0);
	UNITTEST_ASSERT(q->DuplicateOf == mDNSNULL);
	ConvertDomainNameToCString(&q->qname, qname_cstr);
	UNITTEST_ASSERT(!strcmp(qname_cstr, domainname_cstr));

	// Answer local-only question with found cache entry
	m->NextScheduledEvent = mDNS_TimeNow_NoLock(m);
	mDNS_Execute(m);															// Regress <rdar://problem/28721294>
	UNITTEST_ASSERT(m->NewLocalOnlyQuestions == NULL);
	UNITTEST_ASSERT(req->u.queryrecord.ans == 1);
	UNITTEST_ASSERT(q->LOAddressAnswers == 1);
	UNITTEST_ASSERT(q == m->LocalOnlyQuestions);

	reply = req->replies;
	len = get_reply_len(qname_cstr, 4);

	UNITTEST_ASSERT(reply->next == mDNSNULL);
	UNITTEST_ASSERT(reply->totallen == len + sizeof(ipc_msg_hdr));
	UNITTEST_ASSERT(reply->mhdr->version == VERSION);
	UNITTEST_ASSERT(reply->mhdr->datalen == len);
	UNITTEST_ASSERT(reply->mhdr->ipc_flags == 0);
	UNITTEST_ASSERT(reply->mhdr->op == query_reply_op);
	UNITTEST_ASSERT(reply->rhdr->flags == htonl(kDNSServiceFlagsAdd));
	UNITTEST_ASSERT(reply->rhdr->ifi == kDNSServiceInterfaceIndexLocalOnly);	// Regress <rdar://problem/27340874>
	UNITTEST_ASSERT(reply->rhdr->error == kDNSServiceErr_NoError);

	// Simulate the query time out of the local-only question.
	// The expected behavior is a negative answer with time out error
	m->NextScheduledEvent = mDNS_TimeNow_NoLock(m);
	q->StopTime = mDNS_TimeNow_NoLock(m);
	m->NextScheduledStopTime -= mDNSPlatformOneSecond*5;
	mDNS_Execute(m);

	reply = req->replies->next;
	UNITTEST_ASSERT(reply != NULL);
	UNITTEST_ASSERT(reply->next == NULL);
	UNITTEST_ASSERT(m->NewLocalOnlyQuestions == NULL);
	UNITTEST_ASSERT(q->LOAddressAnswers == 0);
	len = get_reply_len(qname_cstr, 0);

	UNITTEST_ASSERT(reply->next == mDNSNULL);
	UNITTEST_ASSERT(reply->totallen == len + + sizeof(ipc_msg_hdr));
	UNITTEST_ASSERT(reply->mhdr->version == VERSION);
	UNITTEST_ASSERT(reply->mhdr->datalen == len);
	UNITTEST_ASSERT(reply->mhdr->ipc_flags == 0);
	UNITTEST_ASSERT(reply->mhdr->op == query_reply_op);
	UNITTEST_ASSERT(reply->rhdr->flags == htonl(kDNSServiceFlagsAdd));
	UNITTEST_ASSERT(reply->rhdr->ifi == kDNSServiceInterfaceIndexLocalOnly);	// Regress <rdar://problem/27340874>
	UNITTEST_ASSERT(reply->rhdr->error ==
					(DNSServiceErrorType)htonl(kDNSServiceErr_Timeout));		// Regress <rdar://problem/27562965>

	free_req(req);
UNITTEST_FOOTER

// This function does memory cleanup and no verification.
UNITTEST_HEADER(FinalizeUnitTest)
	mDNSPlatformMemFree(local_socket);
UNITTEST_FOOTER

mDNSlocal mStatus InitEtcHostsRecords(void)
{
	mDNS *m = &mDNSStorage;
	struct sockaddr_storage hostaddr;

	AuthHash newhosts;
	mDNSPlatformMemZero(&newhosts, sizeof(AuthHash));

	memset(&hostaddr, 0, sizeof(hostaddr));
	get_ip("127.0.0.1", &hostaddr);

	domainname domain;
	MakeDomainNameFromDNSNameString(&domain, "localhost");

	mDNSMacOSXCreateEtcHostsEntry_ut(&domain, (struct sockaddr *) &hostaddr, mDNSNULL, mDNSNULL, &newhosts);

	memset(&hostaddr, 0, sizeof(hostaddr));
	get_ip("0000:0000:0000:0000:0000:0000:0000:0001", &hostaddr);

	MakeDomainNameFromDNSNameString(&domain, "localhost");

	mDNSMacOSXCreateEtcHostsEntry_ut(&domain, (struct sockaddr *) &hostaddr, mDNSNULL, mDNSNULL, &newhosts);

	memset(&hostaddr, 0, sizeof(hostaddr));
	get_ip("255.255.255.255", &hostaddr);

	MakeDomainNameFromDNSNameString(&domain, "broadcasthost");

	mDNSMacOSXCreateEtcHostsEntry_ut(&domain, (struct sockaddr *) &hostaddr, mDNSNULL, mDNSNULL, &newhosts);

	memset(&hostaddr, 0, sizeof(hostaddr));
	get_ip("17.226.40.200", &hostaddr);

	MakeDomainNameFromDNSNameString(&domain, "cardinal2.apple.com");

	mDNSMacOSXCreateEtcHostsEntry_ut(&domain, (struct sockaddr *) &hostaddr, mDNSNULL, mDNSNULL, &newhosts);
	UpdateEtcHosts_ut(&newhosts);

	m->NextScheduledEvent = mDNS_TimeNow_NoLock(m);
	mDNS_Execute(m);

	return mStatus_NoError;
}
