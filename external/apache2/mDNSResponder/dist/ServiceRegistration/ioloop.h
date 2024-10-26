/* ioloop.h
 *
 * Copyright (c) 2018-2024 Apple Inc. All rights reserved.
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
 * Definitions for simple dispatch implementation.
 */

#ifndef __IOLOOP_H
#define __IOLOOP_H


#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>

#include <dns_sd.h>

#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC  1000000000ull
#endif
#ifndef NSEC_PER_MSEC
#define NSEC_PER_MSEC    1000000ull
#endif

#ifndef MSEC_PER_SEC
#define MSEC_PER_SEC (NSEC_PER_SEC / NSEC_PER_MSEC)
#endif

#ifndef IN_LINKLOCAL
#define IN_LINKLOCAL(x) (((uint32_t)(x) & 0xffff0000) == 0xA9FE0000) // 169.254.*
#endif
#ifndef IN_LOOPBACK
#define IN_LOOPBACK(x) (((uint32_t)(x) & 0xff000000) == 0x7f000000) // 127.*
#endif

#ifndef UDP_LISTENER_USES_CONNECTION_GROUPS
#define UDP_LISTENER_USES_CONNECTION_GROUPS 0
#endif

#ifndef __DSO_H
typedef struct dso_state dso_state_t;
#endif

typedef union addr addr_t;
union addr {
    struct sockaddr sa;
    struct sockaddr_in sin;
    struct sockaddr_in6 sin6;
    struct {
        char len;
        char family;
        int index;
        uint8_t addr[8];
    } ether_addr;
};

#define IOLOOP_NTOP(addr, buf) \
    (((addr)->sa.sa_family == AF_INET || (addr)->sa.sa_family == AF_INET6) \
     ? (inet_ntop((addr)->sa.sa_family, ((addr)->sa.sa_family == AF_INET \
                                        ? (void *)&(addr)->sin.sin_addr \
                                        : (void *)&(addr)->sin6.sin6_addr), buf, sizeof buf) != NULL) \
    : snprintf(buf, sizeof buf, "Address type %d", (addr)->sa.sa_family))

struct message {
    int ref_count;
#if !defined(IOLOOP_MACOS) || !UDP_LISTENER_USES_CONNECTION_GROUPS
    addr_t src;
    addr_t local;
#endif
    int ifindex;
    uint16_t length;
    time_t received_time;      // Only for SRP Replication, zero otherwise.
    uint32_t lease, key_lease; // For SRP replication, leases agreed to by original registrar
    dns_wire_t wire;
};


typedef struct dso_transport comm_t;
typedef struct io io_t;
typedef struct subproc subproc_t;
typedef struct wakeup wakeup_t;
typedef struct dnssd_txn dnssd_txn_t;
typedef struct interface_address_state interface_address_state_t;
typedef struct ifpermit_list ifpermit_list_t;

typedef void (*dnssd_txn_finalize_callback_t)(void *NONNULL context);
typedef void (*dnssd_txn_failure_callback_t)(void *NONNULL context, int status);
typedef void (*wakeup_callback_t)(void *NONNULL context);
typedef void (*finalize_callback_t)(void *NONNULL context);
typedef void (*cancel_callback_t)(comm_t *NONNULL comm, void *NONNULL context);
typedef void (*ready_callback_t)(void *NONNULL context, uint16_t port);
typedef void (*io_callback_t)(io_t *NONNULL io, void *NONNULL context);
typedef void (*comm_callback_t)(comm_t *NONNULL comm);
typedef void (*datagram_callback_t)(comm_t *NONNULL comm, message_t *NONNULL message, void *NULLABLE context);
typedef void (*connect_callback_t)(comm_t *NONNULL connection, void *NULLABLE context);
typedef void (*disconnect_callback_t)(comm_t *NONNULL comm, void *NULLABLE context, int error);
enum interface_address_change { interface_address_added, interface_address_deleted, interface_address_unchanged };
typedef struct srp_server_state srp_server_t;
typedef void (*interface_callback_t)(srp_server_t *NULLABLE server_state, void *NULLABLE context,
                                     const char *NONNULL name, const addr_t *NONNULL address,
                                     const addr_t *NONNULL netmask, uint32_t flags,
                                     enum interface_address_change event_type);
typedef void (*subproc_callback_t)(void *NULLABLE context, int status, const char *NULLABLE error);
typedef void (*tls_config_callback_t)(void *NONNULL context);
typedef void (*async_callback_t)(void *NULLABLE context);

typedef struct tls_context tls_context_t;

#define IOLOOP_SECOND   1000LL
#define IOLOOP_MINUTE   60 * IOLOOP_SECOND
#define IOLOOP_HOUR     60 * IOLOOP_MINUTE
#define IOLOOP_DAY      24 * IOLOOP_HOUR

struct interface_address_state {
    interface_address_state_t *NULLABLE next;
    char *NONNULL name;
    addr_t addr;
    addr_t mask;
    uint32_t flags;
};

struct io {
    int ref_count;
    io_t *NULLABLE next;
    io_callback_t NULLABLE read_callback;
    io_callback_t NULLABLE write_callback;
    finalize_callback_t NULLABLE finalize;
    finalize_callback_t NULLABLE io_finalize;
    finalize_callback_t NULLABLE context_release;
    void *NULLABLE context;
    io_t *NULLABLE cancel_on_close;
    io_callback_t NULLABLE ready;
    bool want_read : 1;
    bool want_write : 1;
    int fd;
};

struct wakeup {
    int ref_count;
    wakeup_t *NULLABLE next;
    void *NULLABLE context;
    wakeup_callback_t NULLABLE wakeup;
    finalize_callback_t NULLABLE finalize;
#ifdef IOLOOP_MACOS
    dispatch_source_t NULLABLE dispatch_source;
#else
    int64_t wakeup_time;
#endif
};

struct dso_transport {
#ifdef IOLOOP_MACOS
    nw_connection_t NULLABLE connection;
    nw_listener_t NULLABLE listener;
#if UDP_LISTENER_USES_CONNECTION_GROUPS
    nw_connection_group_t NULLABLE connection_group;
    nw_content_context_t NULLABLE content_context;
#endif
    nw_parameters_t NULLABLE parameters;
    comm_t *NULLABLE listener_state;
    int ref_count;
    int writes_pending;
    wakeup_t *NULLABLE idle_timer;
    // nw_connection objects aren't necessarily ready to write to immediately. But when we create an outgoing connection, we
    // typically want to write to it immediately. So we have a one-datum queue in case this happens; if the connection takes
    // so long to get ready that another write happens, we drop the first write. This will work okay for UDP connections, where
    // the retransmit logic is in the application. For future, we may want to rearchitect the flow so that the write is always
    // done in a callback.
    dispatch_data_t NULLABLE pending_write;
#if !UDP_LISTENER_USES_CONNECTION_GROUPS
    io_t io;
    message_t *NULLABLE message;
#endif
#else
    io_t io;
    message_t *NULLABLE message;
    int multicast_ifindex;
#endif
    uint16_t listen_port;
    uint16_t *NULLABLE avoid_ports;
    int num_avoid_ports;
    int serial;
    bool avoiding;
    char *NONNULL name;
    void *NULLABLE context;
#ifdef SRP_TEST_SERVER
    void *NULLABLE test_context;
    bool (*NULLABLE test_send_intercept)(comm_t *NONNULL connection, message_t *NULLABLE responding_to,
                                         struct iovec *NONNULL iov, int iov_len, bool final, bool send_length);
    void *NULLABLE srp_server;
#endif
    datagram_callback_t NULLABLE datagram_callback;
    comm_callback_t NULLABLE close_callback;
    connect_callback_t NULLABLE connected;
    disconnect_callback_t NULLABLE disconnected;
    finalize_callback_t NULLABLE finalize;
    cancel_callback_t NULLABLE cancel;
    ready_callback_t NULLABLE ready;
    uint8_t *NULLABLE buf;
    dso_state_t *NULLABLE dso;
    tls_context_t *NULLABLE tls_context;
    addr_t address, multicast, local;
    size_t message_length_len;
    size_t message_length, message_cur;
    uint8_t message_length_bytes[2];


#ifdef IOLOOP_MACOS
    bool read_pending: 1; // Only ever one.
    bool server: 1;       // Indicates that this connection was created by a listener
    bool connection_ready: 1;
    bool final_data : 1; // Indicates that the next message written will be the final message, so send a FIN.
#else
    bool tls_handshake_incomplete: 1;
#endif // IOLOOP_MACOS
    bool tls_rotation_ready: 1; // Indicates if the listener should rotate its TLS certificate.
    bool tcp_stream: 1;
    bool is_multicast: 1;
    bool is_connected: 1;
    bool is_listener: 1;
    bool opportunistic: 1;
    bool canceled: 1;
};

#define MAX_SUBPROC_ARGS 20
struct subproc {
    int ref_count;
#ifdef IOLOOP_MACOS
    dispatch_source_t NULLABLE dispatch_source;
#else
    subproc_t *NULLABLE next;
#endif
    int pipe_fds[2];
    io_t *NULLABLE output_fd;
    void *NULLABLE context;
    subproc_callback_t NONNULL callback;
    finalize_callback_t NULLABLE finalize;
    char *NULLABLE argv[MAX_SUBPROC_ARGS + 1];
    int argc;
    pid_t pid;
    bool finished : 1;
};

struct dnssd_txn {
#ifndef IOLOOP_MACOS
    io_t *NULLABLE io;
#endif
    int ref_count;
    DNSServiceRef NULLABLE sdref;
    void *NULLABLE context;
    void *NULLABLE aux_pointer;
    dnssd_txn_finalize_callback_t NULLABLE finalize_callback;
    dnssd_txn_failure_callback_t NULLABLE failure_callback;
};

extern int64_t ioloop_now;
int getipaddr(addr_t *NONNULL addr, const char *NONNULL p);
int64_t ioloop_timenow(void);
message_t *NULLABLE message_allocate(size_t message_size);
void message_free(message_t *NONNULL message);
void ioloop_close(io_t *NONNULL io);
void ioloop_add_reader(io_t *NONNULL io, io_callback_t NONNULL callback);
#define ioloop_wakeup_create() ioloop_wakeup_create_(__FILE__, __LINE__)
wakeup_t *NULLABLE ioloop_wakeup_create_(const char *NONNULL file, int line);
#define ioloop_wakeup_retain(wakeup) ioloop_wakeup_retain_(wakeup, __FILE__, __LINE__)
void ioloop_wakeup_retain_(wakeup_t *NONNULL wakeup, const char *NONNULL file, int line);
#define ioloop_wakeup_release(wakeup) ioloop_wakeup_release_(wakeup, __FILE__, __LINE__)
#ifndef ioloop_wakeup_forget
    #define ioloop_wakeup_forget(ptr) _SRP_STRICT_DISPOSE_TEMPLATE(ptr, ioloop_wakeup_release)
#endif
void ioloop_wakeup_release_(wakeup_t *NONNULL wakeup, const char *NONNULL file, int line);
bool ioloop_add_wake_event(wakeup_t *NONNULL wakeup, void *NULLABLE context,
                           wakeup_callback_t NONNULL callback, finalize_callback_t NULLABLE finalize,
                           int32_t milliseconds);
void ioloop_cancel_wake_event(wakeup_t *NONNULL wakeup);

bool ioloop_init(void);
int ioloop(void);

#define ioloop_comm_retain(comm) ioloop_comm_retain_(comm, __FILE__, __LINE__)
void ioloop_comm_retain_(comm_t *NONNULL comm, const char *NONNULL file, int line);
#define ioloop_comm_release(wakeup) ioloop_comm_release_(wakeup, __FILE__, __LINE__)
void ioloop_comm_release_(comm_t *NONNULL comm, const char *NONNULL file, int line);
void ioloop_comm_context_set(comm_t *NONNULL connection,
                             void *NULLABLE context, finalize_callback_t NULLABLE callback);
void ioloop_comm_connect_callback_set(comm_t *NONNULL comm, connect_callback_t NULLABLE callback);
void ioloop_comm_disconnect_callback_set(comm_t *NONNULL comm, disconnect_callback_t NULLABLE callback);
void ioloop_comm_cancel(comm_t *NONNULL comm);
#define ioloop_listener_retain(comm) ioloop_listener_retain_(comm, __FILE__, __LINE__)
void ioloop_listener_retain_(comm_t *NONNULL listener, const char *NONNULL file, int line);
#define ioloop_listener_release(wakeup) ioloop_listener_release_(wakeup, __FILE__, __LINE__)
void ioloop_listener_release_(comm_t *NONNULL listener, const char *NONNULL file, int line);
void ioloop_listener_cancel(comm_t *NONNULL comm);
comm_t *NULLABLE ioloop_listener_create(bool stream, bool tls, bool init_daemon, uint16_t *NULLABLE avoid_ports,
                                        int num_avoid_ports, const addr_t *NULLABLE ip_address,
                                        const char *NULLABLE multicast, const char *NONNULL name,
                                        datagram_callback_t NONNULL datagram_callback,
                                        connect_callback_t NULLABLE connected, cancel_callback_t NULLABLE cancel,
                                        ready_callback_t NULLABLE ready, finalize_callback_t NULLABLE finalize,
                                        tls_config_callback_t NULLABLE tls_config, unsigned ifindex,
                                        void *NULLABLE context);
comm_t *NULLABLE ioloop_connection_create(addr_t *NONNULL remote_address, bool tls, bool stream, bool stable,
                                          bool opportunistic, datagram_callback_t NONNULL datagram_callback,
                                          connect_callback_t NULLABLE connected,
                                          disconnect_callback_t NULLABLE disconnected,
                                          finalize_callback_t NULLABLE finalize,
                                          void *NULLABLE context);
#define ioloop_message_create(x) ioloop_message_create_(x, __FILE__, __LINE__)
message_t *NULLABLE ioloop_message_create_(size_t message_size, const char *NONNULL file, int line);
#define ioloop_message_retain(wakeup) ioloop_message_retain_(wakeup, __FILE__, __LINE__)
void ioloop_message_retain_(message_t *NONNULL message, const char *NONNULL file, int line);
#define ioloop_message_release(wakeup) ioloop_message_release_(wakeup, __FILE__, __LINE__)
void ioloop_message_release_(message_t *NONNULL message, const char *NONNULL file, int line);
bool ioloop_send_multicast(comm_t *NONNULL comm, int ifindex, struct iovec *NONNULL iov, int iov_len);
bool ioloop_send_message(comm_t *NONNULL connection, message_t *NULLABLE responding_to,
                         struct iovec *NONNULL iov, int iov_len);
bool ioloop_send_final_message(comm_t *NONNULL connection, message_t *NULLABLE responding_to,
                               struct iovec *NONNULL iov, int iov_len);
bool ioloop_send_data(comm_t *NONNULL connection, message_t *NULLABLE responding_to,
                      struct iovec *NONNULL iov, int iov_len);
bool ioloop_send_final_data(comm_t *NONNULL connection, message_t *NULLABLE responding_to,
                            struct iovec *NONNULL iov, int iov_len);
void ioloop_dump_object_allocation_stats(void);
void ioloop_strcpy(char *NONNULL dest, const char *NONNULL src, size_t lim);
bool ioloop_map_interface_addresses(srp_server_t *NULLABLE server_state,
                                    const char *NULLABLE ifname, void *NULLABLE context, interface_callback_t NULLABLE callback);
#define ioloop_map_interface_addresses_here(server_state, here, ifname, context, callback) \
    ioloop_map_interface_addresses_here_(server_state, here, ifname, context, callback, __FILE__, __LINE__)
bool ioloop_map_interface_addresses_here_(srp_server_t *NULLABLE server_state,
                                          interface_address_state_t *NONNULL *NULLABLE here,
                                          const char *NULLABLE ifname,
                                          void *NULLABLE context, interface_callback_t NULLABLE callback,
                                          const char *NONNULL file, int line);
ssize_t ioloop_recvmsg(int sock, uint8_t *NONNULL buffer, size_t buffer_length, int *NONNULL ifindex,
                       int *NONNULL hoplimit, addr_t *NONNULL source, addr_t *NONNULL destination);
#define ioloop_subproc_release(subproc) ioloop_subproc_release_(subproc, __FILE__, __LINE__)
void ioloop_subproc_release_(subproc_t *NONNULL subproc, const char *NONNULL file, int line);
#define ioloop_subproc_retain(subproc) ioloop_subproc_retain_(subproc, __FILE__, __LINE__)
void ioloop_subproc_retain_(subproc_t *NONNULL subproc, const char *NONNULL file, int line);
subproc_t *NULLABLE ioloop_subproc(const char *NONNULL exepath, char *NULLABLE *NONNULL argv, int argc,
                                   subproc_callback_t NULLABLE callback, io_callback_t NULLABLE output_callback,
                                   void *NULLABLE context);
void ioloop_subproc_run_sync(subproc_t *NONNULL subproc);
#define ioloop_dnssd_txn_add(ref, context, finalize_callback, failure_callback) \
    ioloop_dnssd_txn_add_(ref, context, finalize_callback, failure_callback, __FILE__, __LINE__)
dnssd_txn_t *NULLABLE
ioloop_dnssd_txn_add_(DNSServiceRef NONNULL ref, void *NULLABLE context,
                      dnssd_txn_finalize_callback_t NULLABLE callback,
                      dnssd_txn_failure_callback_t NULLABLE failure_callback, const char *NONNULL file, int line);
#define ioloop_dnssd_txn_add_subordinate(ref, context, finalize_callback, failure_callback) \
    ioloop_dnssd_txn_add_subordinate_(ref, context, finalize_callback, failure_callback, __FILE__, __LINE__)
dnssd_txn_t *NULLABLE
ioloop_dnssd_txn_add_subordinate_(DNSServiceRef NONNULL ref, void *NULLABLE context,
                                  dnssd_txn_finalize_callback_t NULLABLE callback,
                                  dnssd_txn_failure_callback_t NULLABLE failure_callback, const char *NONNULL file, int line);
void ioloop_dnssd_txn_cancel(dnssd_txn_t *NONNULL txn);
#define ioloop_dnssd_txn_retain(txn) ioloop_dnssd_txn_retain_(txn, __FILE__, __LINE__)
void ioloop_dnssd_txn_retain_(dnssd_txn_t *NONNULL txn, const char *NONNULL file, int line);
#define ioloop_dnssd_txn_release(txn) ioloop_dnssd_txn_release_(txn, __FILE__, __LINE__)
#ifndef ioloop_dnssd_txn_forget
    #define ioloop_dnssd_txn_forget(ptr) _SRP_STRICT_DISPOSE_TEMPLATE(ptr, ioloop_dnssd_txn_release)
#endif
void ioloop_dnssd_txn_release_(dnssd_txn_t *NONNULL txn, const char *NONNULL file, int line);
#endif
void ioloop_dnssd_txn_set_aux_pointer(dnssd_txn_t *NONNULL txn, void *NULLABLE aux_pointer);
void *NULLABLE ioloop_dnssd_txn_get_aux_pointer(dnssd_txn_t *NONNULL txn);
void *NULLABLE ioloop_dnssd_txn_get_context(dnssd_txn_t *NONNULL txn);

#define ioloop_file_descriptor_create(fd, context, finalize) \
    ioloop_file_descriptor_create_(fd, context, finalize, __FILE__, __LINE__)
io_t *NULLABLE ioloop_file_descriptor_create_(int fd, void *NULLABLE context, finalize_callback_t NULLABLE finalize,
                                              const char *NONNULL file, int line);
#define ioloop_file_descriptor_retain(file_descriptor) ioloop_file_descriptor_retain_(file_descriptor, __FILE__, \
                                                            __LINE__)
void ioloop_file_descriptor_retain_(io_t *NONNULL file_descriptor, const char *NONNULL file, int line);
#define ioloop_file_descriptor_release(file_descriptor) ioloop_file_descriptor_release_(file_descriptor, __FILE__, \
                                                            __LINE__)
void ioloop_file_descriptor_release_(io_t *NONNULL file_descriptor, const char *NONNULL file, int line);

bool ioloop_interface_monitor_start(void);
void ioloop_run_async(async_callback_t NULLABLE callback, void *NULLABLE context);


bool srp_load_file_data(void *NULLABLE host_context, const char *NONNULL filename, uint8_t *NONNULL buffer,
                        uint16_t *NONNULL length, uint16_t buffer_size);
bool srp_store_file_data(void *NULLABLE host_context, const char *NONNULL filename, uint8_t *NONNULL buffer,
                         uint16_t length);
time_t srp_time(void);
double srp_fractional_time(void);
int64_t srp_utime(void);
void srp_format_time_offset(char *NONNULL buf, size_t buf_len, time_t offset);

const struct sockaddr *NULLABLE connection_get_local_address(message_t *NULLABLE message);

#if !UDP_LISTENER_USES_CONNECTION_GROUPS
bool ioloop_udp_send_message(comm_t *NONNULL comm, addr_t *NULLABLE source, addr_t *NONNULL dest, int ifindex,
                             struct iovec *NONNULL iov, int iov_len);
void ioloop_udp_read_callback(io_t *NONNULL io, void *NULLABLE context);
#endif
int get_num_fds(void);


// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
