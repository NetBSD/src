/* test-api.c
 *
 * Copyright (c) 2023-2024 Apple Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * srp host API test harness
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>

typedef struct test_state test_state_t;
typedef struct io_context io_context_t;
typedef struct dns_service_event dns_service_event_t;
typedef struct test_packet_state test_packet_state_t;

extern ready_callback_t NULLABLE srp_test_dnssd_tls_listener_ready;
extern void *NULLABLE srp_test_tls_listener_context;
extern void (*NULLABLE srp_test_dso_message_finished)(void *NULLABLE context, message_t *NONNULL message,
                                                      dso_state_t *NONNULL dso);
typedef bool (*dns_service_query_record_callback_intercept_t)(DNSServiceRef NONNULL sdRef, DNSServiceFlags flags,
                                                              uint32_t interfaceIndex, DNSServiceErrorType errorCode,
                                                              const char *NULLABLE fullname, uint16_t rrtype,
                                                              uint16_t rrclass, uint16_t rdlen,
                                                              const void *NULLABLE rdata, uint32_t ttl,
                                                              void *NULLABLE context);

struct test_state {
    test_state_t *NULLABLE next, *NULLABLE finished_tests;
    srp_server_t *NULLABLE primary;
    comm_t *NULLABLE srp_listener;
    io_context_t *NULLABLE current_io_context;
    test_packet_state_t *NULLABLE test_packet_state;
    dns_service_event_t *NULLABLE dns_service_events;
    void *NULLABLE context;
    int counter;
    const char *NONNULL title;
    const char *NULLABLE variant_title;
    const char *NONNULL explanation;
    const char *NULLABLE variant_info;
    void (*NULLABLE continue_testing)(test_state_t *NONNULL next_state);
    bool (*NULLABLE dnssd_proxy_configurer)(void);
    int (*NULLABLE getifaddrs)(srp_server_t *NULLABLE server_state, struct ifaddrs *NULLABLE *NONNULL ifaddrs,
                               void *NULLABLE context);
    void (*NULLABLE freeifaddrs)(srp_server_t *NULLABLE server_state, struct ifaddrs *NONNULL ifaddrs,
                                 void *NULLABLE context);
    DNSServiceErrorType (*NULLABLE query_record_intercept)(test_state_t *NONNULL state,
                                                           DNSServiceRef NONNULL *NULLABLE sdRef, DNSServiceFlags flags,
                                                           uint32_t interfaceIndex, const char *NONNULL fullname,
                                                           uint16_t rrtype, uint16_t rrclass,
                                                           DNSServiceAttribute const *NULLABLE attr,
                                                           DNSServiceQueryRecordReply NONNULL callBack,
                                                           void *NULLABLE context);
    dns_service_query_record_callback_intercept_t NULLABLE dns_service_query_callback_intercept;
    int variant;
    bool test_complete;
};

#define TEST_FAIL(test_state, message)                          \
    do {                                                        \
        srp_test_state_explain(test_state);                     \
        fprintf(stderr, "test failed: " message "\n\n");        \
        exit(1);                                                \
    } while (0)

#define TEST_FAIL_CHECK(test_state, success_condition, message) \
    do {                                                        \
        if (!(success_condition)) {                             \
            TEST_FAIL(test_state, message);                     \
        }                                                       \
    } while (0)

#define TEST_FAIL_STATUS(test_state, message, status)               \
    do {                                                            \
           srp_test_state_explain(test_state);                      \
           fprintf(stderr, "test failed: " message "\n\n", status); \
           exit(1);                                                 \
    } while (0)

#define TEST_FAIL_CHECK_STATUS(test_state, success_condition, message, status) \
    do {                                                                       \
       if (!(success_condition)) {                                             \
           TEST_FAIL_STATUS(test_state, message, status);                      \
       }                                                                       \
    } while (0)

#define TEST_PASSED(test_state)         \
    srp_test_state_explain(test_state); \
    INFO("test passed\n");              \
    srp_test_state_next(test_state);

void srp_test_set_local_example_address(test_state_t *NONNULL state);
void srp_test_network_localhost_start(test_state_t *NONNULL state);
test_state_t *NULLABLE test_state_create(srp_server_t *NONNULL primary,
                                         const char *NONNULL title, const char *NULLABLE variant_title,
                                         const char *NONNULL explanation, const char *NULLABLE variant_name);
void test_state_add_srp_server(test_state_t *NONNULL state, srp_server_t *NONNULL server);
void srp_test_state_explain(test_state_t *NULLABLE state);
void srp_test_state_next(test_state_t *NONNULL state);
void srp_test_state_add_timeout(test_state_t *NONNULL state, int timeout);
struct ifaddrs;
int srp_test_getifaddrs(srp_server_t *NULLABLE server_state, struct ifaddrs *NULLABLE *NONNULL ifaddrs, void *NULLABLE context);
void srp_test_freeifaddrs(srp_server_t *NULLABLE server_state, struct ifaddrs *NONNULL ifaddrs, void *NULLABLE context);
void srp_test_enable_stub_router(test_state_t *NONNULL state, srp_server_t *NONNULL server_state);

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
