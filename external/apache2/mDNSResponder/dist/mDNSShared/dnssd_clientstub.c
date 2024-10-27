/* -*- Mode: C; tab-width: 4 -*-
 *
 * Copyright (c) 2003-2024 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1.  Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of its
 *     contributors may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "dnssd_clientstub.h"

#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>

#include "dnssd_ipc.h"


#ifndef DEBUG_64BIT_SCM_RIGHTS
#define DEBUG_64BIT_SCM_RIGHTS 0
#endif

#if defined(_WIN32)

    #define _SSIZE_T
    #include <CommonServices.h>
    #include <DebugServices.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #include <stdarg.h>
    #include <stdio.h>
    #include <stdint.h>

    #define sockaddr_mdns sockaddr_in
    #define AF_MDNS AF_INET

// Disable warning: "'type cast' : from data pointer 'void *' to function pointer"
    #pragma warning(disable:4055)

// Disable warning: "nonstandard extension, function/data pointer conversion in expression"
    #pragma warning(disable:4152)

extern BOOL IsSystemServiceDisabled();

    #define sleep(X) Sleep((X) * 1000)

static int g_initWinsock = 0;
    #define LOG_WARNING kDebugLevelWarning
    #define LOG_INFO kDebugLevelInfo
static void syslog( int priority, const char * message, ...)
{
    va_list args;
    int len;
    char * buffer;
    DWORD err = WSAGetLastError();
    (void) priority;
    va_start( args, message );
    len = _vscprintf( message, args ) + 1;
    buffer = mdns_malloc( len * sizeof(char) );
    if ( buffer ) { vsnprintf( buffer, len, message, args ); OutputDebugString( buffer ); mdns_free( buffer ); }
    WSASetLastError( err );
}
#else

    #include <fcntl.h>      // For O_RDWR etc.
    #include <sys/time.h>
    #include <sys/socket.h>
    #include <syslog.h>
    #include <sys/uio.h>

#endif

#include "mdns_strict.h"

#if !defined(SETIOV)
    #if defined(_WIN32)
        #define iovec_t         WSABUF
        #define iov_len         len
        #define iov_base        buf

        #define SETIOV(IOV, PTR, LEN) \
            do \
            { \
                (IOV)->iov_base = (char *)(PTR); \
                (IOV)->iov_len  = (LEN); \
            } while(0)

        static
        ssize_t writev(SocketRef inSock, const iovec_t *inArray, int inCount)
        {
            int         err;
            DWORD       n;

            err = WSASend(inSock, (iovec_t *)inArray, inCount, &n, 0, NULL, NULL);
            return(err ? err : n);
        }

    #else
        typedef struct iovec    iovec_t;

        #define SETIOV(IOV, PTR, LEN) \
            do \
            { \
                (IOV)->iov_base = (void *)(PTR); \
                (IOV)->iov_len  = (LEN); \
            } while(0)
    #endif
#endif

#if defined(_WIN32)
// <rdar://problem/4096913> Specifies how many times we'll try and connect to the server.

#define DNSSD_CLIENT_MAXTRIES 4
#endif // _WIN32

// Uncomment the line below to use the old error return mechanism of creating a temporary named socket (e.g. in /var/tmp)
//#define USE_NAMED_ERROR_RETURN_SOCKET 1

// If the UDS client has not received a response from the daemon in 60 secs, it is unlikely to get one
// Note: Timeout of 3 secs should be sufficient in normal scenarios, but 60 secs is chosen as a safeguard since
// some clients may come up before mDNSResponder itself after a BOOT and on rare ocassions IOPM/Keychain/D2D calls
// in mDNSResponder's INIT may take a much longer time to return
#define DNSSD_CLIENT_TIMEOUT 60

#ifdef USE_NAMED_ERROR_RETURN_SOCKET
#ifndef CTL_PATH_PREFIX
#define CTL_PATH_PREFIX "/var/tmp/dnssd_result_socket."
#endif
#endif

typedef struct
{
    ipc_msg_hdr ipc_hdr;
    DNSServiceFlags cb_flags;
    uint32_t cb_interface;
    DNSServiceErrorType cb_err;
} CallbackHeader;

typedef struct _DNSServiceRef_t DNSServiceOp;
typedef struct _DNSRecordRef_t DNSRecord;

#if !defined(_WIN32)
typedef struct
{
    void             *AppCallback;      // Client callback function and context
    void             *AppContext;
} SleepKAContext;
#endif

// client stub callback to process message from server and deliver results to client application
typedef void (*ProcessReplyFn)(DNSServiceOp *sdr, const CallbackHeader *cbh, const uint8_t *msg, const uint8_t *end);

#define ValidatorBits 0x12345678
#define DNSServiceRefValid(X) (dnssd_SocketValid((X)->sockfd) && (((X)->sockfd ^ (X)->validator) == ValidatorBits))

// When using kDNSServiceFlagsShareConnection, there is one primary _DNSServiceOp_t, and zero or more subordinates
// For the primary, the 'next' field points to the first subordinate, and its 'next' field points to the next, and so on.
// For the primary, the 'primary' field is NULL; for subordinates the 'primary' field points back to the associated primary
//
// _DNS_SD_LIBDISPATCH is defined where libdispatch/GCD is available. This does not mean that the application will use the
// DNSServiceSetDispatchQueue API. Hence any new code guarded with _DNS_SD_LIBDISPATCH should still be backwards compatible.
struct _DNSServiceRef_t
{
    DNSServiceOp     *next;             // When sharing a connection, this is the next subordinate DNSServiceRef in
                                        // the list. The connection being shared is the first in the list.
    DNSServiceOp     *primary;          // When sharing a connection, the primary pointer of each subordinate
                                        // DNSServiceRef points to the head of the list. If primary is null, and next is
                                        // null, this is not a shared connection. If primary is null and next is
                                        // non-null, this is the primary DNSServiceRef of the shared connection. If
                                        // primary is non-null, this is a subordinate DNSServiceRef for the connection
                                        // that is managed by the DNSServiceRef pointed to by primary.
    dnssd_sock_t sockfd;                // Connected socket between client and daemon
    dnssd_sock_t validator;             // Used to detect memory corruption, double disposals, etc.
    client_context_t uid;               // For shared connection requests, each subordinate DNSServiceRef has its own ID,
                                        // unique within the scope of the same shared parent DNSServiceRef. On the
                                        // primary DNSServiceRef, uid matches the uid of the most recently allocated
                                        // subordinate DNSServiceRef. Each time a new subordinate DNSServiceRef is
                                        // allocated, uid on the primary is incremented by one and copied to the
                                        // subordinate.
    uint32_t op;                        // request_op_t or reply_op_t
    uint32_t max_index;                 // Largest assigned record index - 0 if no additional records registered
    uint32_t logcounter;                // Counter used to control number of syslog messages we write
    int              *moreptr;          // Set while DNSServiceProcessResult working on this particular DNSServiceRef
    ProcessReplyFn ProcessReply;        // Function pointer to the code to handle received messages
    void             *AppCallback;      // Client callback function and context
    void             *AppContext;
    DNSRecord        *rec;
#if _DNS_SD_LIBDISPATCH
    dispatch_source_t disp_source;
    dispatch_queue_t disp_queue;
#endif
    void             *kacontext;
};

// Any DNSServiceRef can have a list of one or more DNSRecordRefs. These DNSRecordRefs either come from
// DNSServiceRegisterRecord calls on a DNSServiceRef allocated using DNSServiceCreateConnection, or else
// they come from DNSServiceAddRecord calls on a DNSServiceRef allocated using DNSServiceRegisterRecord.
// A connection created using DNSServiceCreateConnection can also be used as a shared connection for registering
// services using DNSServiceRegister. Or a connection can have both records registered with
// DNSServiceRegisterRecord and services registered with DNSServiceRegister. A shared connection can also have
// browses, resolves, and so on, each of which will have a subordinate DNSServiceRef.
// This data structure is mirrored in the server daemon. To summarize:
// For any given DNSServiceRef created by DNSServiceCreateConnection,
//   - there may be zero or more subordinate DNSServiceRefs.
//   - there may be zero or more DNSRecordRefs
// Any DNSServiceRef created by DNSServiceRegister may
//   - be a subordinate DNSServiceRef of a DNSServiceRef created by DNSServiceCreateConnection, or not
//   - be a standalone DNSServiceRef with its own connection
//   - may have zero or more records registered with DNSServiceAddRecord
//   - may not have any records registered with DNSServiceRegisterRecord
//   - may not be used as a shared connection

struct _DNSRecordRef_t
{
    DNSRecord       *recnext;
    void *AppContext;
    DNSServiceRegisterRecordReply AppCallback;
    DNSRecordRef recref;
    uint32_t record_index;  // index is unique within the set of DNSRecordRefs owned by its parent (sdr).
    client_context_t uid;   // a copy of sdr->uid made when the recordref is created. if sdr is a shared
                            // connection, this may not match sdr->uid later on. If it is a subordinate
                            // connection or a non-shared connection, then sdr->uid should always match
                            // uid.
    DNSServiceOp *sdr;
    ipc_msg_hdr *msg;
};

struct DNSServiceAttribute_s {
    DNSServiceAAAAPolicy aaaa_policy;
    uint32_t hostkeyhash;
    uint32_t timestamp;    //Timestamp in seconds since epoch time to indicate when the service/record is registered.
    bool hostkeyhash_is_set;
    bool timestamp_is_set;
};

const DNSServiceAttribute kDNSServiceAttributeAAAAFallback = {
    .aaaa_policy = kDNSServiceAAAAPolicyFallback
};


DNSServiceErrorType DNSSD_API DNSServiceQueryRecordWithAttribute
(
    DNSServiceRef * const sdRef,
    const DNSServiceFlags flags,
    const uint32_t ifindex,
    const char * const name,
    const uint16_t rrtype,
    const uint16_t rrclass,
    const DNSServiceAttribute * const attr,
    const DNSServiceQueryRecordReply callback,
    void * const context
)
{
    return DNSServiceQueryRecordInternal(sdRef, flags, ifindex, name, rrtype, rrclass, attr, callback, context);
}

// send out all the linked requets in sdr->rec
DNSServiceErrorType DNSSD_API DNSServiceSendQueuedRequests
(
    DNSServiceRef sdr
)
{
    return DNSServiceSendQueuedRequestsInternal(sdr);
}

DNSServiceAttributeRef DNSSD_API DNSServiceAttributeCreate
(
    void
)
{
#ifdef MEMORY_OBJECT_TRACKING
    extern int saref_created;
    saref_created++;
#endif
    DNSServiceAttributeRef attr = (DNSServiceAttributeRef)mdns_calloc(1, sizeof(*attr));
    return attr;
}

DNSServiceErrorType DNSSD_API DNSServiceAttributeSetAAAAPolicy
(
    const DNSServiceAttributeRef attr,
    const DNSServiceAAAAPolicy policy
)
{
    attr->aaaa_policy = policy;
    return kDNSServiceErr_NoError;
}


DNSServiceErrorType DNSSD_API DNSServiceAttributeSetHostKeyHash
(
    const DNSServiceAttributeRef attr,
    uint32_t host_key
)
{
    attr->hostkeyhash_is_set = true;
    attr->hostkeyhash = host_key;
    return kDNSServiceErr_NoError;
}

DNSServiceErrorType DNSSD_API DNSServiceAttributeSetTimestamp
(
    const DNSServiceAttributeRef attr,
    const uint32_t timestamp
)
{
    attr->timestamp_is_set = true;
    attr->timestamp = timestamp;
    return kDNSServiceErr_NoError;
}

void DNSSD_API DNSServiceAttributeDeallocate(DNSServiceAttributeRef attr)
{
#ifdef MEMORY_OBJECT_TRACKING
    extern int saref_finalized;
    saref_finalized++;
#endif
    DNSServiceAttributeRef tmp = attr;
    mdns_free(tmp);
}

static bool
validate_attribute_tlvs(const DNSServiceAttribute * const attr)
{
    if (!attr)
    {
        return true;
    }
    // If either is set, require both
    if ((attr->timestamp_is_set || attr->hostkeyhash_is_set) &&
        (!attr->timestamp_is_set || !attr->hostkeyhash_is_set))
    {
        return false;
    }
    return true;
}

static size_t
put_attribute_tlvs(const DNSServiceAttribute * const attr, ipc_msg_hdr * const hdr, uint8_t ** const ptr,
    const uint8_t * const limit)
{
    size_t required_len = 0;
    required_len += put_tlv_uint32(IPC_TLV_TYPE_SERVICE_ATTR_AAAA_POLICY, attr->aaaa_policy, ptr, limit);
    if (attr->timestamp_is_set)
    {
        required_len += put_tlv_uint32(IPC_TLV_TYPE_SERVICE_ATTR_TIMESTAMP, attr->timestamp, ptr, limit);
    }
    if (attr->hostkeyhash_is_set)
    {
        required_len += put_tlv_uint32(IPC_TLV_TYPE_SERVICE_ATTR_HOST_KEY_HASH, attr->hostkeyhash, ptr, limit);
    }
    if (hdr)
    {
        hdr->ipc_flags |= IPC_FLAGS_TRAILING_TLVS;
    }
    return required_len;
}

static size_t
get_required_length_for_attribute_tlvs(const DNSServiceAttribute * const attr)
{
    return put_attribute_tlvs(attr, NULL, NULL, NULL);
}

static bool _should_return_noauth_error(void)
{
    static bool s_should = false;
    return s_should;
}

#if !defined(USE_TCP_LOOPBACK)
static void SetUDSPath(struct sockaddr_un *saddr, const char *path)
{
    size_t pathLen;

    pathLen = strlen(path);
    if (pathLen < sizeof(saddr->sun_path))
        memcpy(saddr->sun_path, path, pathLen + 1);
    else
        saddr->sun_path[0] = '\0';
}
#endif

enum { write_all_success = 0, write_all_fail = -1, write_all_defunct = -2 };

// Write len bytes. Return 0 on success, -1 on error
static int write_all(dnssd_sock_t sd, char *buf, size_t len)
{
    // Don't use "MSG_WAITALL"; it returns "Invalid argument" on some Linux versions; use an explicit while() loop instead.
    //if (send(sd, buf, len, MSG_WAITALL) != len) return write_all_fail;
    while (len)
    {
        ssize_t num_written = send(sd, buf, (long)len, 0);
        if (num_written < 0 || (size_t)num_written > len)
        {
            // Check whether socket has gone defunct,
            // otherwise, an error here indicates some OS bug
            // or that the mDNSResponder daemon crashed (which should never happen).
#if !defined(__ppc__) && defined(SO_ISDEFUNCT)
            int defunct = 0;
            socklen_t dlen = sizeof (defunct);
            if (getsockopt(sd, SOL_SOCKET, SO_ISDEFUNCT, &defunct, &dlen) < 0)
                syslog(LOG_WARNING, "dnssd_clientstub write_all: SO_ISDEFUNCT failed %d %s", dnssd_errno, dnssd_strerror(dnssd_errno));
            if (!defunct)
                syslog(LOG_WARNING, "dnssd_clientstub write_all(%d) failed %ld/%ld %d %s", sd,
                       (long)num_written, (long)len,
                       (num_written < 0) ? dnssd_errno                 : 0,
                       (num_written < 0) ? dnssd_strerror(dnssd_errno) : "");
            else
                syslog(LOG_INFO, "dnssd_clientstub write_all(%d) DEFUNCT", sd);
            return defunct ? write_all_defunct : write_all_fail;
#else
            syslog(LOG_WARNING, "dnssd_clientstub write_all(%d) failed %ld/%ld %d %s", sd,
                   (long)num_written, (long)len,
                   (num_written < 0) ? dnssd_errno                 : 0,
                   (num_written < 0) ? dnssd_strerror(dnssd_errno) : "");
            return write_all_fail;
#endif
        }
        buf += num_written;
        len -= num_written;
    }
    return write_all_success;
}

enum { read_all_success = 0, read_all_fail = -1, read_all_wouldblock = -2, read_all_defunct = -3 };

// Read len bytes. Return 0 on success, read_all_fail on error, or read_all_wouldblock for
static int read_all(const dnssd_sock_t sd, uint8_t *buf, size_t len)
{
    // Don't use "MSG_WAITALL"; it returns "Invalid argument" on some Linux versions; use an explicit while() loop instead.
    //if (recv(sd, buf, len, MSG_WAITALL) != len) return -1;

    while (len)
    {
        ssize_t num_read = recv(sd, buf, len, 0);
        // It is valid to get an interrupted system call error e.g., somebody attaching
        // in a debugger, retry without failing
        if ((num_read < 0) && (errno == EINTR)) 
        { 
            syslog(LOG_INFO, "dnssd_clientstub read_all: EINTR continue"); 
            continue; 
        }
        if ((num_read == 0) || (num_read < 0) || (((size_t)num_read) > len))
        {
            int printWarn = 0;
            int defunct = 0;

            // Check whether socket has gone defunct,
            // otherwise, an error here indicates some OS bug
            // or that the mDNSResponder daemon crashed (which should never happen).
#if defined(WIN32)
            // <rdar://problem/7481776> Suppress logs for "A non-blocking socket operation
            //                          could not be completed immediately"
            if (WSAGetLastError() != WSAEWOULDBLOCK)
                printWarn = 1;
#endif
#if !defined(__ppc__) && defined(SO_ISDEFUNCT)
            {
                socklen_t dlen = sizeof (defunct);
                if (getsockopt(sd, SOL_SOCKET, SO_ISDEFUNCT, &defunct, &dlen) < 0)
                    syslog(LOG_WARNING, "dnssd_clientstub read_all: SO_ISDEFUNCT failed %d %s", dnssd_errno, dnssd_strerror(dnssd_errno));
            }
            if (!defunct)
                printWarn = 1;
#endif
            if (printWarn)
                syslog(LOG_WARNING, "dnssd_clientstub read_all(%d) failed %ld/%ld %d %s", sd,
                       (long)num_read, (long)len,
                       (num_read < 0) ? dnssd_errno                 : 0,
                       (num_read < 0) ? dnssd_strerror(dnssd_errno) : "");
            else if (defunct)
                syslog(LOG_INFO, "dnssd_clientstub read_all(%d) DEFUNCT", sd);
            return (num_read < 0 && dnssd_errno == dnssd_EWOULDBLOCK) ? read_all_wouldblock : (defunct ? read_all_defunct : read_all_fail);
        }
        buf += num_read;
        len -= num_read;
    }
    return read_all_success;
}

// Returns 1 if more bytes remain to be read on socket descriptor sd, 0 otherwise
static int more_bytes(dnssd_sock_t sd)
{
    struct timeval tv = { 0, 0 };
    fd_set readfds;
    fd_set *fs;
    int ret;

#if defined(_WIN32)
    fs = &readfds;
    FD_ZERO(fs);
    FD_SET(sd, fs);
    ret = select((int)sd+1, fs, (fd_set*)NULL, (fd_set*)NULL, &tv);
#else
    // This whole thing would probably be better done using kevent() instead of select()
    if (sd < FD_SETSIZE)
    {
        fs = &readfds;
        FD_ZERO(fs);
    }
    else
    {
        // Compute the number of integers needed for storing "sd". Internally fd_set is stored
        // as an array of ints with one bit for each fd and hence we need to compute
        // the number of ints needed rather than the number of bytes. If "sd" is 32, we need
        // two ints and not just one.
        int nfdbits = sizeof (int) * 8;
        int nints = (sd/nfdbits) + 1;
        fs = (fd_set *)mdns_calloc(nints, (size_t)sizeof(int));
        if (fs == NULL) 
        { 
            syslog(LOG_WARNING, "dnssd_clientstub more_bytes: malloc failed"); 
            return 0; 
        }
    }
    FD_SET(sd, fs);
    ret = select((int)sd+1, fs, (fd_set*)NULL, (fd_set*)NULL, &tv);
    if (fs != &readfds) 
        mdns_free(fs);
#endif
    return (ret > 0);
}

// set_waitlimit() implements a timeout using select. It is called from deliver_request() before recv() OR accept()
// to ensure the UDS clients are not blocked in these system calls indefinitely.
// Note: Ideally one should never be blocked here, because it indicates either mDNSResponder daemon is not yet up/hung/
// superbusy/crashed or some other OS bug. For eg: On Windows which suffers from 3rd party software 
// (primarily 3rd party firewall software) interfering with proper functioning of the TCP protocol stack it is possible 
// the next operation on this socket(recv/accept) is blocked since we depend on TCP to communicate with the system service.
static int set_waitlimit(dnssd_sock_t sock, int timeout)
{
    int gDaemonErr = kDNSServiceErr_NoError;

    // The comment below is wrong. The select() routine does not cause stack corruption.
    // The use of FD_SET out of range for the bitmap is what causes stack corruption.
    // For how to do this correctly, see the example using calloc() in more_bytes() above.
    // Even better, both should be changed to use kevent() instead of select().
    // To prevent stack corruption since select does not work with timeout if fds > FD_SETSIZE(1024)
    if (!gDaemonErr && sock < FD_SETSIZE)
    {
        struct timeval tv;
        fd_set set;

        FD_ZERO(&set);
        FD_SET(sock, &set);
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        if (!select((int)(sock + 1), &set, NULL, NULL, &tv))
        {
            // Ideally one should never hit this case: See comments before set_waitlimit()
            syslog(LOG_WARNING, "dnssd_clientstub set_waitlimit:_daemon timed out (%d secs) without any response: Socket %d", timeout, sock);
            gDaemonErr = kDNSServiceErr_Timeout;
        }
    }
    return gDaemonErr;
}

/* create_hdr
 *
 * allocate and initialize an ipc message header. Value of len should initially be the
 * length of the data, and is set to the value of the data plus the header. data_start
 * is set to point to the beginning of the data section. SeparateReturnSocket should be
 * non-zero for calls that can't receive an immediate error return value on their primary
 * socket, and therefore require a separate return path for the error code result.
 * if zero, the path to a control socket is appended at the beginning of the message buffer.
 * data_start is set past this string.
 */
static ipc_msg_hdr *create_hdr(uint32_t op, size_t *len, uint8_t **data_start, int SeparateReturnSocket, DNSServiceOp *ref)
{
    uint8_t *msg = NULL;
    ipc_msg_hdr *hdr;
    int datalen;
#if !defined(USE_TCP_LOOPBACK)
    char ctrl_path[64] = "";    // "/var/tmp/dnssd_result_socket.xxxxxxxxxx-xxx-xxxxxx"
#endif

    if (SeparateReturnSocket)
    {
#if defined(USE_TCP_LOOPBACK)
        *len += 2;  // Allocate space for two-byte port number
#elif defined(USE_NAMED_ERROR_RETURN_SOCKET)
        struct timeval tv;
        if (gettimeofday(&tv, NULL) < 0)
        { syslog(LOG_WARNING, "dnssd_clientstub create_hdr: gettimeofday failed %d %s", dnssd_errno, dnssd_strerror(dnssd_errno)); return NULL; }
        snprintf(ctrl_path, sizeof(ctrl_path), "%s%d-%.3lx-%.6lu", CTL_PATH_PREFIX, (int)getpid(),
                (unsigned long)(tv.tv_sec & 0xFFF), (unsigned long)(tv.tv_usec));
        *len += strlen(ctrl_path) + 1;
#else
        *len += 1;      // Allocate space for single zero byte (empty C string)
#endif
    }

    datalen = (int) *len;
    *len += sizeof(ipc_msg_hdr);

    // Write message to buffer
    msg = (uint8_t *)mdns_malloc(*len);
    if (!msg) { syslog(LOG_WARNING, "dnssd_clientstub create_hdr: malloc failed"); return NULL; }

    memset(msg, 0, *len);
    hdr = (ipc_msg_hdr *)msg;
    hdr->version                = VERSION;
    hdr->datalen                = datalen;
    hdr->ipc_flags              = 0;
    hdr->op                     = op;
    hdr->client_context         = ref->uid;
    hdr->reg_index              = 0;
    *data_start = msg + sizeof(ipc_msg_hdr);
#if defined(USE_TCP_LOOPBACK)
    // Put dummy data in for the port, since we don't know what it is yet.
    // The data will get filled in before we send the message. This happens in deliver_request().
    if (SeparateReturnSocket) put_uint16(0, data_start);
#else
    if (SeparateReturnSocket) put_string(ctrl_path, data_start);
#endif
    return hdr;
}

static void FreeDNSRecords(DNSServiceOp *sdRef)
{
    DNSRecord *rec = sdRef->rec;
    while (rec)
    {
        DNSRecord *next = rec->recnext;
#ifdef MEMORY_OBJECT_TRACKING
        extern int rref_finalized;
        rref_finalized++;
#endif
        mdns_free(rec->msg);
        mdns_free(rec);
        rec = next;
    }
}

static void FreeDNSServiceOp(DNSServiceOp *x)
{
#ifdef MEMORY_OBJECT_TRACKING
    extern void *dns_service_op_not_to_be_freed;
    if (x != NULL && x == dns_service_op_not_to_be_freed) {
        syslog(LOG_ERR, "dnssd_clientstub attempt to dispose protected DNSServiceRef %p", x);
        abort();
    }
#endif
    // We don't use our DNSServiceRefValid macro here because if we're cleaning up after a socket() call failed
    // then sockfd could legitimately contain a failing value (e.g. dnssd_InvalidSocket)
    if ((x->sockfd ^ x->validator) != ValidatorBits)
    {
    }
    else
    {
        x->next         = NULL;
        x->primary      = NULL;
        x->sockfd       = dnssd_InvalidSocket;
        x->validator    = 0xDDDDDDDD;
        x->op           = request_op_none;
        x->max_index    = 0;
        x->logcounter   = 0;
        x->moreptr      = NULL;
        x->ProcessReply = NULL;
        x->AppCallback  = NULL;
        x->AppContext   = NULL;
#if _DNS_SD_LIBDISPATCH
        MDNS_DISPOSE_DISPATCH(x->disp_source);
        x->disp_queue   = NULL;
#endif
        // DNSRecords may have been added to subordinate sdRef e.g., DNSServiceRegister/DNSServiceAddRecord
        // or on the main sdRef e.g., DNSServiceCreateConnection/DNSServiceRegisterRecord.
        // DNSRecords may have been freed if the application called DNSRemoveRecord.
        FreeDNSRecords(x);
        if (x->kacontext)
        {
            mdns_free(x->kacontext);
            x->kacontext = NULL;
        }
        mdns_free(x);
#ifdef MEMORY_OBJECT_TRACKING
        extern int sdref_finalized;
        sdref_finalized++;
#endif
    }
}

// Return a connected service ref (deallocate with DNSServiceRefDeallocate)
static DNSServiceErrorType ConnectToServer(DNSServiceRef *ref, DNSServiceFlags flags, uint32_t op, ProcessReplyFn ProcessReply, void *AppCallback, void *AppContext)
{
    #if defined(_WIN32)
    int NumTries = 0;
    #endif // _WIN32

    dnssd_sockaddr_t saddr;
    DNSServiceOp *sdr;

    if (!ref) 
    { 
        syslog(LOG_WARNING, "dnssd_clientstub DNSService operation with NULL DNSServiceRef"); 
        return kDNSServiceErr_BadParam; 
    }

    if (flags & kDNSServiceFlagsShareConnection)
    {
        if (!*ref)
        {
            syslog(LOG_WARNING, "dnssd_clientstub kDNSServiceFlagsShareConnection used with NULL DNSServiceRef");
            return kDNSServiceErr_BadParam;
        }
        if (!DNSServiceRefValid(*ref) || ((*ref)->op != connection_request && (*ref)->op != connection_delegate_request) || (*ref)->primary)
        {
            syslog(LOG_WARNING, "dnssd_clientstub kDNSServiceFlagsShareConnection used with invalid DNSServiceRef %p %08X %08X op %d",
                   (*ref), (*ref)->sockfd, (*ref)->validator, (*ref)->op);
            *ref = NULL;
            return kDNSServiceErr_BadReference;
        }
    }

    #if defined(_WIN32)
    if (!g_initWinsock)
    {
        WSADATA wsaData;
        g_initWinsock = 1;
        if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) { *ref = NULL; return kDNSServiceErr_ServiceNotRunning; }
    }

#ifndef WIN32_CENTENNIAL
    // <rdar://problem/4096913> If the system service is disabled, we only want to try to connect once
    if (IsSystemServiceDisabled())
        NumTries = DNSSD_CLIENT_MAXTRIES;
    #endif
#endif

    sdr = mdns_malloc(sizeof(DNSServiceOp));
    if (!sdr) 
    { 
        syslog(LOG_WARNING, "dnssd_clientstub ConnectToServer: malloc failed"); 
        *ref = NULL; 
        return kDNSServiceErr_NoMemory; 
    }
    sdr->next          = NULL;
    sdr->primary       = NULL;
    sdr->sockfd        = dnssd_InvalidSocket;
    sdr->validator     = sdr->sockfd ^ ValidatorBits;
    sdr->op            = op;
    sdr->max_index     = 0;
    sdr->logcounter    = 0;
    sdr->moreptr       = NULL;
    sdr->uid.u32[0]    = 0;
    sdr->uid.u32[1]    = 0;
    sdr->ProcessReply  = ProcessReply;
    sdr->AppCallback   = AppCallback;
    sdr->AppContext    = AppContext;
    sdr->rec           = NULL;
#if _DNS_SD_LIBDISPATCH
    sdr->disp_source   = NULL;
    sdr->disp_queue    = NULL;
#endif
    sdr->kacontext     = NULL;
#ifdef MEMORY_OBJECT_TRACKING
    extern int sdref_created;
    sdref_created++;
#endif
    
    if (flags & kDNSServiceFlagsShareConnection)
    {
        DNSServiceOp **p = &(*ref)->next;       // Append ourselves to end of primary's list
        while (*p) 
            p = &(*p)->next;
        *p = sdr;
        // Preincrement counter before we use it -- it helps with debugging if we know the all-zeroes ID should never appear
        if (++(*ref)->uid.u32[0] == 0) 
            ++(*ref)->uid.u32[1];               // In parent DNSServiceOp increment UID counter
        sdr->primary    = *ref;                 // Set our primary pointer
        sdr->sockfd     = (*ref)->sockfd;       // Inherit primary's socket
        sdr->validator  = (*ref)->validator;
        sdr->uid        = (*ref)->uid;
        //printf("ConnectToServer sharing socket %d\n", sdr->sockfd);
    }
    else
    {
        #ifdef SO_NOSIGPIPE
        const unsigned int optval = 1;
        #endif
        #ifndef USE_TCP_LOOPBACK
        char* uds_serverpath = getenv(MDNS_UDS_SERVERPATH_ENVVAR);
        if (uds_serverpath == NULL)
            uds_serverpath = MDNS_UDS_SERVERPATH;
        else if (strlen(uds_serverpath) >= MAX_CTLPATH)
        {
            uds_serverpath = MDNS_UDS_SERVERPATH;
            syslog(LOG_WARNING, "dnssd_clientstub ConnectToServer: using default path since env len is invalid");
        }
        #endif
        *ref = NULL;
        sdr->sockfd    = socket(AF_DNSSD, SOCK_STREAM, 0);
        sdr->validator = sdr->sockfd ^ ValidatorBits;
        if (!dnssd_SocketValid(sdr->sockfd))
        {
            syslog(LOG_WARNING, "dnssd_clientstub ConnectToServer: socket failed %d %s", dnssd_errno, dnssd_strerror(dnssd_errno));
            FreeDNSServiceOp(sdr);
            return kDNSServiceErr_NoMemory;
        }
#if !defined(_WIN32)
        int fcntl_flags = fcntl(sdr->sockfd, F_GETFD);
        if (fcntl_flags != -1)
        {
            fcntl_flags |= FD_CLOEXEC;
            int ret = fcntl(sdr->sockfd, F_SETFD, fcntl_flags);
            if (ret == -1)
                syslog(LOG_WARNING, "dnssd_clientstub ConnectToServer: Failed to set FD_CLOEXEC on socket %d %s",
                       dnssd_errno, dnssd_strerror(dnssd_errno));
        }
        else
        {
            syslog(LOG_WARNING, "dnssd_clientstub ConnectToServer: Failed to get the file descriptor flags of socket %d %s",
                   dnssd_errno, dnssd_strerror(dnssd_errno));
        }
#endif // !defined(_WIN32)
        #ifdef SO_NOSIGPIPE
        // Some environments (e.g. OS X) support turning off SIGPIPE for a socket
        if (setsockopt(sdr->sockfd, SOL_SOCKET, SO_NOSIGPIPE, &optval, sizeof(optval)) < 0)
            syslog(LOG_WARNING, "dnssd_clientstub ConnectToServer: SO_NOSIGPIPE failed %d %s", dnssd_errno, dnssd_strerror(dnssd_errno));
        #endif
        #if defined(USE_TCP_LOOPBACK)
        saddr.sin_family      = AF_INET;
        saddr.sin_addr.s_addr = inet_addr(MDNS_TCP_SERVERADDR);
        saddr.sin_port        = IsSystemServiceDisabled() ? htons(MDNS_TCP_SERVERPORT_CENTENNIAL) : htons(MDNS_TCP_SERVERPORT);
        #else
        saddr.sun_family      = AF_LOCAL;
        SetUDSPath(&saddr, uds_serverpath);
        #if !defined(__ppc__) && defined(SO_DEFUNCTOK)
        {
            int defunct = 1;
            if (setsockopt(sdr->sockfd, SOL_SOCKET, SO_DEFUNCTOK, &defunct, sizeof(defunct)) < 0)
                syslog(LOG_WARNING, "dnssd_clientstub ConnectToServer: SO_DEFUNCTOK failed %d %s", dnssd_errno, dnssd_strerror(dnssd_errno));
        }
        #endif
        #endif

        #if defined(_WIN32)
        while (1)
        {
#ifdef WIN32_CENTENNIAL
            char port[128];
            // Access to the process environment block is thread-safe
            if (GetEnvironmentVariableA("MDNS_TCP_SERVERPORT_CENTENNIAL", port, sizeof(port)))
            {
                saddr.sin_port = htons((u_short)atoi(port));
            }
#endif
            int err = connect(sdr->sockfd, (struct sockaddr*)&saddr, sizeof(saddr));
            if (!err)
                break; // If we succeeded, return sdr

            // If we failed, then it may be because the daemon is still launching.
            // This can happen for processes that launch early in the boot process, while the
            // daemon is still coming up. Rather than fail here, we wait 1 sec and try again.
            // If, after DNSSD_CLIENT_MAXTRIES, we still can't connect to the daemon,
            // then we give up and return a failure code.
            if (++NumTries < DNSSD_CLIENT_MAXTRIES)
            {
                syslog(LOG_WARNING, "dnssd_clientstub ConnectToServer: connect()-> No of tries: %d", NumTries);
                sleep(1); // Sleep a bit, then try again
            }
            else
            {
                #if !defined(USE_TCP_LOOPBACK)
                syslog(LOG_WARNING, "dnssd_clientstub ConnectToServer: connect() failed path:%s Socket:%d Err:%d Errno:%d %s",
                       uds_serverpath, sdr->sockfd, err, dnssd_errno, dnssd_strerror(dnssd_errno));
                #endif
                dnssd_close(sdr->sockfd);
                FreeDNSServiceOp(sdr);
                return kDNSServiceErr_ServiceNotRunning;
            }
        }
        #else
        int err = connect(sdr->sockfd, (struct sockaddr *) &saddr, sizeof(saddr));
        if (err)
        {
            #if !defined(USE_TCP_LOOPBACK)
            syslog(LOG_WARNING, "dnssd_clientstub ConnectToServer: connect() failed path:%s Socket:%d Err:%d Errno:%d %s",
                   uds_serverpath, sdr->sockfd, err, dnssd_errno, dnssd_strerror(dnssd_errno));
            #endif
            dnssd_close(sdr->sockfd);
            FreeDNSServiceOp(sdr);
            return kDNSServiceErr_ServiceNotRunning;
        }
        #endif
    }

    *ref = sdr;
    return kDNSServiceErr_NoError;
}

#define deliver_request_bailout(MSG) \
    do { syslog(LOG_WARNING, "dnssd_clientstub deliver_request: %s failed %d (%s)", (MSG), dnssd_errno, dnssd_strerror(dnssd_errno)); goto cleanup; } while(0)

static DNSServiceErrorType deliver_request(ipc_msg_hdr *hdr, DNSServiceOp *sdr)
{
    uint32_t datalen;
    dnssd_sock_t listenfd = dnssd_InvalidSocket, errsd = dnssd_InvalidSocket;
    DNSServiceErrorType err = kDNSServiceErr_Unknown;   // Default for the "goto cleanup" cases
    int MakeSeparateReturnSocket;
    int ioresult;
    #if defined(USE_TCP_LOOPBACK) || defined(USE_NAMED_ERROR_RETURN_SOCKET)
    char *data;
    #endif

    if (!hdr)
    {
        syslog(LOG_WARNING, "dnssd_clientstub deliver_request: !hdr");
        return kDNSServiceErr_Unknown;
    }
    
    datalen = hdr->datalen;    // We take a copy here because we're going to convert hdr->datalen to network byte order
    #if defined(USE_TCP_LOOPBACK) || defined(USE_NAMED_ERROR_RETURN_SOCKET)
    data = (char *)hdr + sizeof(ipc_msg_hdr);
    #endif

    // Note: need to check hdr->op, not sdr->op.
    // hdr->op contains the code for the specific operation we're currently doing, whereas sdr->op
    // contains the original parent DNSServiceOp (e.g. for an add_record_request, hdr->op will be
    // add_record_request but the parent sdr->op will be connection_request or reg_service_request)
    MakeSeparateReturnSocket = (sdr->primary ||
        hdr->op == reg_record_request || hdr->op == add_record_request || hdr->op == update_record_request || hdr->op == remove_record_request);

    if (!DNSServiceRefValid(sdr))
    {
        if (hdr)
            mdns_free(hdr);
        syslog(LOG_WARNING, "dnssd_clientstub deliver_request: invalid DNSServiceRef %p %08X %08X", sdr, sdr->sockfd, sdr->validator);
        return kDNSServiceErr_BadReference;
    }

    if (MakeSeparateReturnSocket)
    {
        #if defined(USE_TCP_LOOPBACK)
        {
            union { uint16_t s; u_char b[2]; } port;
            dnssd_sockaddr_t caddr;
            dnssd_socklen_t len = (dnssd_socklen_t) sizeof(caddr);
            listenfd = socket(AF_DNSSD, SOCK_STREAM, 0);
            if (!dnssd_SocketValid(listenfd)) deliver_request_bailout("TCP socket");

            caddr.sin_family      = AF_INET;
            caddr.sin_port        = 0;
            caddr.sin_addr.s_addr = inet_addr(MDNS_TCP_SERVERADDR);
            if (bind(listenfd, (struct sockaddr*) &caddr, sizeof(caddr)) < 0) deliver_request_bailout("TCP bind");
            if (getsockname(listenfd, (struct sockaddr*) &caddr, &len)   < 0) deliver_request_bailout("TCP getsockname");
            if (listen(listenfd, 1)                                      < 0) deliver_request_bailout("TCP listen");
            port.s = caddr.sin_port;
            data[0] = port.b[0];  // don't switch the byte order, as the
            data[1] = port.b[1];  // daemon expects it in network byte order
        }
        #elif defined(USE_NAMED_ERROR_RETURN_SOCKET)
        {
            mode_t mask;
            int bindresult;
            dnssd_sockaddr_t caddr;
            listenfd = socket(AF_DNSSD, SOCK_STREAM, 0);
            if (!dnssd_SocketValid(listenfd)) deliver_request_bailout("USE_NAMED_ERROR_RETURN_SOCKET socket");

            caddr.sun_family = AF_LOCAL;
            // According to Stevens (section 3.2), there is no portable way to
            // determine whether sa_len is defined on a particular platform.
            #ifndef NOT_HAVE_SA_LEN
            caddr.sun_len = sizeof(struct sockaddr_un);
            #endif
            SetUDSPath(&caddr, data);
            mask = umask(0);
            bindresult = bind(listenfd, (struct sockaddr *)&caddr, sizeof(caddr));
            umask(mask);
            if (bindresult          < 0) deliver_request_bailout("USE_NAMED_ERROR_RETURN_SOCKET bind");
            if (listen(listenfd, 1) < 0) deliver_request_bailout("USE_NAMED_ERROR_RETURN_SOCKET listen");
        }
        #else
        {
            dnssd_sock_t sp[2];
            if (socketpair(AF_DNSSD, SOCK_STREAM, 0, sp) < 0) deliver_request_bailout("socketpair");
            else
            {
                errsd    = sp[0];   // We'll read our four-byte error code from sp[0]
                listenfd = sp[1];   // We'll send sp[1] to the daemon
                #if !defined(__ppc__) && defined(SO_DEFUNCTOK)
                {
                    int defunct = 1;
                    if (setsockopt(errsd, SOL_SOCKET, SO_DEFUNCTOK, &defunct, sizeof(defunct)) < 0)
                        syslog(LOG_WARNING, "dnssd_clientstub deliver_request: SO_DEFUNCTOK failed %d %s", dnssd_errno, dnssd_strerror(dnssd_errno));
                }
                #endif
            }
        }
        #endif
    }

#if !defined(USE_TCP_LOOPBACK) && !defined(USE_NAMED_ERROR_RETURN_SOCKET)
    // If we're going to make a separate error return socket, and pass it to the daemon
    // using sendmsg, then we'll hold back one data byte to go with it.
    // On some versions of Unix (including Leopard) sending a control message without
    // any associated data does not work reliably -- e.g. one particular issue we ran
    // into is that if the receiving program is in a kqueue loop waiting to be notified
    // of the received message, it doesn't get woken up when the control message arrives.
    if (MakeSeparateReturnSocket)
    {
        datalen--;
    }
#endif

    // At this point, our listening socket is set up and waiting, if necessary, for the daemon to connect back to
    ConvertHeaderBytes(hdr);
    //syslog(LOG_WARNING, "dnssd_clientstub deliver_request writing %lu bytes", (unsigned long)(datalen + sizeof(ipc_msg_hdr)));
    //if (MakeSeparateReturnSocket) syslog(LOG_WARNING, "dnssd_clientstub deliver_request name is %s", data);
#if defined(TEST_SENDING_ONE_BYTE_AT_A_TIME) && TEST_SENDING_ONE_BYTE_AT_A_TIME
    unsigned int i;
    for (i=0; i<datalen + sizeof(ipc_msg_hdr); i++)
    {
        syslog(LOG_WARNING, "dnssd_clientstub deliver_request writing %d", i);
        ioresult = write_all(sdr->sockfd, ((char *)hdr)+i, 1);
        if (ioresult < write_all_success)
        {
            syslog(LOG_WARNING, "dnssd_clientstub deliver_request write_all (byte %u) failed", i);
            err = (ioresult == write_all_defunct) ? kDNSServiceErr_DefunctConnection : kDNSServiceErr_ServiceNotRunning;
            goto cleanup;
        }
        usleep(10000);
    }
#else
    ioresult = write_all(sdr->sockfd, (char *)hdr, datalen + sizeof(ipc_msg_hdr));
    if (ioresult < write_all_success)
    {
        // write_all already prints an error message if there is an error writing to
        // the socket except for DEFUNCT. Logging here is unnecessary and also wrong
        // in the case of DEFUNCT sockets
        syslog(LOG_INFO, "dnssd_clientstub deliver_request ERROR: write_all(%d, %lu bytes) failed",
               sdr->sockfd, (unsigned long)(datalen + sizeof(ipc_msg_hdr)));
        err = (ioresult == write_all_defunct) ? kDNSServiceErr_DefunctConnection : kDNSServiceErr_ServiceNotRunning;
        goto cleanup;
    }
#endif

    if (!MakeSeparateReturnSocket) 
        errsd = sdr->sockfd;
    if (MakeSeparateReturnSocket)
    {
#if defined(USE_TCP_LOOPBACK) || defined(USE_NAMED_ERROR_RETURN_SOCKET)
        // At this point we may wait in accept for a few milliseconds waiting for the daemon to connect back to us,
        // but that's okay -- the daemon should not take more than a few milliseconds to respond.
        // set_waitlimit() ensures we do not block indefinitely just in case something is wrong
        dnssd_sockaddr_t daddr;
        dnssd_socklen_t len = sizeof(daddr);
        if ((err = set_waitlimit(listenfd, DNSSD_CLIENT_TIMEOUT)) != kDNSServiceErr_NoError) 
            goto cleanup;
        errsd = accept(listenfd, (struct sockaddr *)&daddr, &len);
        if (!dnssd_SocketValid(errsd)) 
            deliver_request_bailout("accept");
#else

        struct iovec vec = { ((char *)hdr) + sizeof(ipc_msg_hdr) + datalen, 1 }; // Send the last byte along with the SCM_RIGHTS
        struct msghdr msg;
        struct cmsghdr *cmsg;
        char cbuf[CMSG_SPACE(4 * sizeof(dnssd_sock_t))];

        msg.msg_name       = 0;
        msg.msg_namelen    = 0;
        msg.msg_iov        = &vec;
        msg.msg_iovlen     = 1;
        msg.msg_flags      = 0;
        msg.msg_control    = cbuf;
        msg.msg_controllen = CMSG_LEN(sizeof(dnssd_sock_t));

        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_len     = CMSG_LEN(sizeof(dnssd_sock_t));
        cmsg->cmsg_level   = SOL_SOCKET;
        cmsg->cmsg_type    = SCM_RIGHTS;
        *((dnssd_sock_t *)CMSG_DATA(cmsg)) = listenfd;

#if defined(TEST_KQUEUE_CONTROL_MESSAGE_BUG) && TEST_KQUEUE_CONTROL_MESSAGE_BUG
        sleep(1);
#endif

#if DEBUG_64BIT_SCM_RIGHTS
        syslog(LOG_WARNING, "dnssd_clientstub deliver_request sendmsg read sd=%d write sd=%d %ld %ld %ld/%ld/%ld/%ld",
               errsd, listenfd, sizeof(dnssd_sock_t), sizeof(void*),
               sizeof(struct cmsghdr) + sizeof(dnssd_sock_t),
               CMSG_LEN(sizeof(dnssd_sock_t)), (long)CMSG_SPACE(sizeof(dnssd_sock_t)),
               (long)((char*)CMSG_DATA(cmsg) + 4 - cbuf));
#endif // DEBUG_64BIT_SCM_RIGHTS

        if (sendmsg(sdr->sockfd, &msg, 0) < 0)
        {
            syslog(LOG_WARNING, "dnssd_clientstub deliver_request ERROR: sendmsg failed read sd=%d write sd=%d errno %d (%s)",
                   errsd, listenfd, dnssd_errno, dnssd_strerror(dnssd_errno));
            err = kDNSServiceErr_Incompatible;
            goto cleanup;
        }

#if DEBUG_64BIT_SCM_RIGHTS
        syslog(LOG_WARNING, "dnssd_clientstub deliver_request sendmsg read sd=%d write sd=%d okay", errsd, listenfd);
#endif // DEBUG_64BIT_SCM_RIGHTS

#endif
        // Close our end of the socketpair *before* calling read_all() to get the four-byte error code.
        // Otherwise, if the daemon closes our socket (or crashes), we will have to wait for a timeout
        // in read_all() because the socket is not closed (we still have an open reference to it)
        dnssd_close(listenfd);
        listenfd = dnssd_InvalidSocket; // Make sure we don't close it a second time in the cleanup handling below
    }

    // At this point we may wait in read_all for a few milliseconds waiting for the daemon to send us the error code,
    // but that's okay -- the daemon should not take more than a few milliseconds to respond.
    // set_waitlimit() ensures we do not block indefinitely just in case something is wrong
    if ((err = set_waitlimit(errsd, DNSSD_CLIENT_TIMEOUT)) == kDNSServiceErr_NoError)
    {
        ioresult = read_all(errsd, (uint8_t *)&err, (int)sizeof(err));
        if (ioresult < read_all_success)
            err = (ioresult == read_all_defunct) ? kDNSServiceErr_DefunctConnection : kDNSServiceErr_ServiceNotRunning; // On failure read_all will have written a message to syslog for us
        else
            err = ntohl(err);
    }
    //syslog(LOG_WARNING, "dnssd_clientstub deliver_request: retrieved error code %d", err);

cleanup:
    if (MakeSeparateReturnSocket)
    {
        if (dnssd_SocketValid(listenfd)) dnssd_close(listenfd);
        if (dnssd_SocketValid(errsd)) dnssd_close(errsd);
#if defined(USE_NAMED_ERROR_RETURN_SOCKET)
        // syslog(LOG_WARNING, "dnssd_clientstub deliver_request: removing UDS: %s", data);
        if (unlink(data) != 0)
            syslog(LOG_WARNING, "dnssd_clientstub WARNING: unlink(\"%s\") failed errno %d (%s)", data, dnssd_errno, dnssd_strerror(dnssd_errno));
        // else syslog(LOG_WARNING, "dnssd_clientstub deliver_request: removed UDS: %s", data);
#endif
    }

    mdns_free(hdr);
    return err;
}

dnssd_sock_t DNSSD_API DNSServiceRefSockFD(DNSServiceRef sdRef)
{
    if (!sdRef) { syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRefSockFD called with NULL DNSServiceRef"); return dnssd_InvalidSocket; }

    if (!DNSServiceRefValid(sdRef))
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRefSockFD called with invalid DNSServiceRef %p %08X %08X",
               sdRef, sdRef->sockfd, sdRef->validator);
        return dnssd_InvalidSocket;
    }

    if (sdRef->primary)
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRefSockFD undefined for kDNSServiceFlagsShareConnection subordinate DNSServiceRef %p", sdRef);
        return dnssd_InvalidSocket;
    }

    return sdRef->sockfd;
}

#if _DNS_SD_LIBDISPATCH
static void CallbackWithError(DNSServiceRef sdRef, DNSServiceErrorType error)
{
    DNSServiceOp *sdr = sdRef;
    DNSServiceOp *sdrNext;
    DNSRecord *rec;
    DNSRecord *recnext;
    int morebytes;

    while (sdr)
    {
        // We can't touch the sdr after the callback as it can be deallocated in the callback
        sdrNext = sdr->next;
        morebytes = 1;
        sdr->moreptr = &morebytes;
        switch (sdr->op)
        {
        case resolve_request:
            if (sdr->AppCallback) ((DNSServiceResolveReply)    sdr->AppCallback)(sdr, 0, 0, error, NULL, 0, 0, 0, NULL,    sdr->AppContext);
            break;
        case query_request:
            if (sdr->AppCallback) ((DNSServiceQueryRecordReply)sdr->AppCallback)(sdr, 0, 0, error, NULL, 0, 0, 0, NULL, 0, sdr->AppContext);
            break;
        case addrinfo_request:
            if (sdr->AppCallback) ((DNSServiceGetAddrInfoReply)sdr->AppCallback)(sdr, 0, 0, error, NULL, NULL, 0,          sdr->AppContext);
            break;
        case browse_request:
            if (sdr->AppCallback) ((DNSServiceBrowseReply)     sdr->AppCallback)(sdr, 0, 0, error, NULL, 0, NULL,          sdr->AppContext);
            break;
        case reg_service_request:
            if (sdr->AppCallback) ((DNSServiceRegisterReply)   sdr->AppCallback)(sdr, 0,    error, NULL, 0, NULL,          sdr->AppContext);
            break;
        case enumeration_request:
            if (sdr->AppCallback) ((DNSServiceDomainEnumReply) sdr->AppCallback)(sdr, 0, 0, error, NULL,                   sdr->AppContext);
            break;
        case connection_request:
        case connection_delegate_request:
            // This means Register Record, walk the list of DNSRecords to do the callback
            rec = sdr->rec;
            while (rec)
            {
                recnext = rec->recnext;
                if (rec->AppCallback) ((DNSServiceRegisterRecordReply)rec->AppCallback)(sdr, 0, 0, error, rec->AppContext);
                // The Callback can call DNSServiceRefDeallocate which in turn frees sdr and all the records.
                // Detect that and return early
                if (!morebytes) { syslog(LOG_WARNING, "dnssd_clientstub:Record: CallbackwithError morebytes zero"); return; }
                rec = recnext;
            }
            break;
        case port_mapping_request:
            if (sdr->AppCallback) ((DNSServiceNATPortMappingReply)sdr->AppCallback)(sdr, 0, 0, error, 0, 0, 0, 0, 0, sdr->AppContext);
            break;
        default:
            syslog(LOG_WARNING, "dnssd_clientstub CallbackWithError called with bad op %d", sdr->op);
        }
        // If DNSServiceRefDeallocate was called in the callback, morebytes will be zero. As the sdRef
        // (and its subordinates) have been freed, we should not proceed further. Note that when we
        // call the callback with a subordinate sdRef the application can call DNSServiceRefDeallocate
        // on the main sdRef and DNSServiceRefDeallocate handles this case by walking all the sdRefs and
        // clears the moreptr so that we can terminate here.
        //
        // If DNSServiceRefDeallocate was not called in the callback, then set moreptr to NULL so that
        // we don't access the stack variable after we return from this function.
        if (!morebytes) { syslog(LOG_WARNING, "dnssd_clientstub:sdRef: CallbackwithError morebytes zero sdr %p", sdr); return; }
        else {sdr->moreptr = NULL;}
        sdr = sdrNext;
    }
}
#endif // _DNS_SD_LIBDISPATCH

// Handle reply from server, calling application client callback. If there is no reply
// from the daemon on the socket contained in sdRef, the call will block.
DNSServiceErrorType DNSSD_API DNSServiceProcessResult(DNSServiceRef sdRef)
{
    int morebytes = 0;
    int ioresult;
    DNSServiceErrorType error;

    if (!sdRef) { syslog(LOG_WARNING, "dnssd_clientstub DNSServiceProcessResult called with NULL DNSServiceRef"); return kDNSServiceErr_BadParam; }

    if (!DNSServiceRefValid(sdRef))
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceProcessResult called with invalid DNSServiceRef %p %08X %08X", sdRef, sdRef->sockfd, sdRef->validator);
        return kDNSServiceErr_BadReference;
    }

    if (sdRef->primary)
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceProcessResult undefined for kDNSServiceFlagsShareConnection subordinate DNSServiceRef %p", sdRef);
        return kDNSServiceErr_BadReference;
    }

    if (!sdRef->ProcessReply)
    {
        static int num_logs = 0;
        if (num_logs < 10) syslog(LOG_WARNING, "dnssd_clientstub DNSServiceProcessResult called with DNSServiceRef with no ProcessReply function");
        if (num_logs < 1000) num_logs++;else sleep(1);
        return kDNSServiceErr_BadReference;
    }

    do
    {
        CallbackHeader cbh;
        uint8_t *data;

        // return NoError on EWOULDBLOCK. This will handle the case
        // where a non-blocking socket is told there is data, but it was a false positive.
        // On error, read_all will write a message to syslog for us, so don't need to duplicate that here
        // Note: If we want to properly support using non-blocking sockets in the future
        ioresult = read_all(sdRef->sockfd, (void *)&cbh.ipc_hdr, sizeof(cbh.ipc_hdr));
        if (ioresult == read_all_fail || ioresult == read_all_defunct)
        {
            error = (ioresult == read_all_defunct) ? kDNSServiceErr_DefunctConnection : kDNSServiceErr_ServiceNotRunning;
            
            // Set the ProcessReply to NULL before callback as the sdRef can get deallocated
            // in the callback.
            sdRef->ProcessReply = NULL;
#if _DNS_SD_LIBDISPATCH
            // Call the callbacks with an error if using the dispatch API, as DNSServiceProcessResult
            // is not called by the application and hence need to communicate the error. Cancel the
            // source so that we don't get any more events
            // Note: read_all fails if we could not read from the daemon which can happen if the
            // daemon dies or the file descriptor is disconnected (defunct).
            if (sdRef->disp_source)
            {
                dispatch_source_cancel(sdRef->disp_source);
                MDNS_DISPOSE_DISPATCH(sdRef->disp_source);
                CallbackWithError(sdRef, error);
            }
#endif
            // Don't touch sdRef anymore as it might have been deallocated
            return error;
        }
        else if (ioresult == read_all_wouldblock)
        {
            if (morebytes && sdRef->logcounter < 100)
            {
                sdRef->logcounter++;
                syslog(LOG_WARNING, "dnssd_clientstub DNSServiceProcessResult error: select indicated data was waiting but read_all returned EWOULDBLOCK");
            }
            return kDNSServiceErr_NoError;
        }

        ConvertHeaderBytes(&cbh.ipc_hdr);
        if (cbh.ipc_hdr.version != VERSION)
        {
            syslog(LOG_WARNING, "dnssd_clientstub DNSServiceProcessResult daemon version %d does not match client version %d", cbh.ipc_hdr.version, VERSION);
            sdRef->ProcessReply = NULL;
            return kDNSServiceErr_Incompatible;
        }

        data = mdns_malloc(cbh.ipc_hdr.datalen);
        if (!data) return kDNSServiceErr_NoMemory;
        ioresult = read_all(sdRef->sockfd, data, cbh.ipc_hdr.datalen);
        if (ioresult < read_all_success) // On error, read_all will write a message to syslog for us
        {
            error = (ioresult == read_all_defunct) ? kDNSServiceErr_DefunctConnection : kDNSServiceErr_ServiceNotRunning;
            
            // Set the ProcessReply to NULL before callback as the sdRef can get deallocated
            // in the callback.
            sdRef->ProcessReply = NULL;
#if _DNS_SD_LIBDISPATCH
            // Call the callbacks with an error if using the dispatch API, as DNSServiceProcessResult
            // is not called by the application and hence need to communicate the error. Cancel the
            // source so that we don't get any more events
            if (sdRef->disp_source)
            {
                dispatch_source_cancel(sdRef->disp_source);
                MDNS_DISPOSE_DISPATCH(sdRef->disp_source);
                CallbackWithError(sdRef, error);
            }
#endif
            // Don't touch sdRef anymore as it might have been deallocated
            mdns_free(data);
            return error;
        }
        else
        {
            const uint8_t *ptr = data;
            cbh.cb_flags     = get_flags     (&ptr, data + cbh.ipc_hdr.datalen);
            cbh.cb_interface = get_uint32    (&ptr, data + cbh.ipc_hdr.datalen);
            cbh.cb_err       = get_error_code(&ptr, data + cbh.ipc_hdr.datalen);

            // CAUTION: We have to handle the case where the client calls DNSServiceRefDeallocate from within the callback function.
            // To do this we set moreptr to point to morebytes. If the client does call DNSServiceRefDeallocate(),
            // then that routine will clear morebytes for us, and cause us to exit our loop.
            morebytes = more_bytes(sdRef->sockfd);
            if (morebytes)
            {
                cbh.cb_flags |= kDNSServiceFlagsMoreComing;
                sdRef->moreptr = &morebytes;
            }
            if (ptr) sdRef->ProcessReply(sdRef, &cbh, ptr, data + cbh.ipc_hdr.datalen);
            // Careful code here:
            // If morebytes is non-zero, that means we set sdRef->moreptr above, and the operation was not
            // cancelled out from under us, so now we need to clear sdRef->moreptr so we don't leave a stray
            // dangling pointer pointing to a long-gone stack variable.
            // If morebytes is zero, then one of two thing happened:
            // (a) morebytes was 0 above, so we didn't set sdRef->moreptr, so we don't need to clear it
            // (b) morebytes was 1 above, and we set sdRef->moreptr, but the operation was cancelled (with DNSServiceRefDeallocate()),
            //     so we MUST NOT try to dereference our stale sdRef pointer.
            if (morebytes) sdRef->moreptr = NULL;
        }
        mdns_free(data);
    } while (morebytes);

    return kDNSServiceErr_NoError;
}

void DNSSD_API DNSServiceRefDeallocate(DNSServiceRef sdRef)
{
    if (!sdRef) { syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRefDeallocate called with NULL DNSServiceRef"); return; }

    if (!DNSServiceRefValid(sdRef))     // Also verifies dnssd_SocketValid(sdRef->sockfd) for us too
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRefDeallocate called with invalid DNSServiceRef %p %08X %08X", sdRef, sdRef->sockfd, sdRef->validator);
        return;
    }

    // If we're in the middle of a DNSServiceProcessResult() invocation for this DNSServiceRef, clear its morebytes flag to break it out of its while loop
    if (sdRef->moreptr) *(sdRef->moreptr) = 0;

    if (sdRef->primary)     // If this is a subordinate DNSServiceOp, just send a 'stop' command
    {
        DNSServiceOp **p = &sdRef->primary->next;
        while (*p && *p != sdRef) p = &(*p)->next;
        if (*p)
        {
            uint8_t *ptr;
            size_t len = 0;
            ipc_msg_hdr *hdr = create_hdr(cancel_request, &len, &ptr, 0, sdRef);
            if (hdr)
            {
                ConvertHeaderBytes(hdr);
                write_all(sdRef->sockfd, (char *)hdr, len);
                mdns_free(hdr);
            }
            *p = sdRef->next;
            FreeDNSServiceOp(sdRef);
        }
    }
    else                    // else, make sure to terminate all subordinates as well
    {
#if _DNS_SD_LIBDISPATCH
        // The cancel handler will close the fd if a dispatch source has been set
        if (sdRef->disp_source)
        {
            // By setting the ProcessReply to NULL, we make sure that we never call
            // the application callbacks ever, after returning from this function. We
            // assume that DNSServiceRefDeallocate is called from the serial queue
            // that was passed to DNSServiceSetDispatchQueue. Hence, dispatch_source_cancel
            // should cancel all the blocks on the queue and hence there should be no more
            // callbacks when we return from this function. Setting ProcessReply to NULL
            // provides extra protection.
            sdRef->ProcessReply = NULL;
            shutdown(sdRef->sockfd, SHUT_WR);
            dispatch_source_cancel(sdRef->disp_source);
            MDNS_DISPOSE_DISPATCH(sdRef->disp_source);
        }
        // if disp_queue is set, it means it used the DNSServiceSetDispatchQueue API. In that case,
        // when the source was cancelled, the fd was closed in the handler. Currently the source
        // is cancelled only when the mDNSResponder daemon dies
        else if (!sdRef->disp_queue) dnssd_close(sdRef->sockfd);
#else
        dnssd_close(sdRef->sockfd);
#endif
        // Free DNSRecords added in DNSRegisterRecord if they have not
        // been freed in DNSRemoveRecord
        while (sdRef)
        {
            DNSServiceOp *p = sdRef;
            sdRef = sdRef->next;
            // When there is an error reading from the daemon e.g., bad fd, CallbackWithError
            // is called which sets moreptr. It might set the moreptr on a subordinate sdRef
            // but the application might call DNSServiceRefDeallocate with the main sdRef from
            // the callback. Hence, when we loop through the subordinate sdRefs, we need
            // to clear the moreptr so that CallbackWithError can terminate itself instead of
            // walking through the freed sdRefs.
            if (p->moreptr) *(p->moreptr) = 0;
            FreeDNSServiceOp(p);
        }
    }
}

DNSServiceErrorType DNSSD_API DNSServiceGetProperty(const char *property, void *result, uint32_t *size)
{
    DNSServiceErrorType err;
    uint8_t *ptr;
    size_t len;
    ipc_msg_hdr *hdr;
    DNSServiceOp *tmp;
    uint32_t actualsize;
    int ioresult;

    if (!property || !result || !size)
        return kDNSServiceErr_BadParam;

    len = strlen(property) + 1;
    err = ConnectToServer(&tmp, 0, getproperty_request, NULL, NULL, NULL);
    if (err) return err;

    hdr = create_hdr(getproperty_request, &len, &ptr, 0, tmp);
    if (!hdr) { DNSServiceRefDeallocate(tmp); return kDNSServiceErr_NoMemory; }

    put_string(property, &ptr);
    err = deliver_request(hdr, tmp);        // Will free hdr for us
    if (err) { DNSServiceRefDeallocate(tmp); return err; }

    ioresult = read_all(tmp->sockfd, (uint8_t *)&actualsize, sizeof(actualsize));
    if (ioresult < read_all_success)
    { DNSServiceRefDeallocate(tmp); return (ioresult == read_all_defunct) ? kDNSServiceErr_DefunctConnection : kDNSServiceErr_ServiceNotRunning; }

    actualsize = ntohl(actualsize);
    ioresult = read_all(tmp->sockfd, (uint8_t *)result, actualsize < *size ? actualsize : *size);
    if (ioresult < read_all_success)
    { DNSServiceRefDeallocate(tmp); return (ioresult == read_all_defunct) ? kDNSServiceErr_DefunctConnection : kDNSServiceErr_ServiceNotRunning; }
    DNSServiceRefDeallocate(tmp);

    // Swap version result back to local process byte order
    if (!strcmp(property, kDNSServiceProperty_DaemonVersion) && *size >= 4)
        *(uint32_t*)result = ntohl(*(uint32_t*)result);

    *size = actualsize;
    return kDNSServiceErr_NoError;
}

DNSServiceErrorType DNSSD_API DNSServiceGetPID(const uint16_t srcport, int32_t *pid)
{
    uint8_t *ptr;
    ipc_msg_hdr *hdr;
    DNSServiceOp *tmp = NULL;
    size_t len = sizeof(int32_t);
    int ioresult;

    DNSServiceErrorType err = ConnectToServer(&tmp, 0, getpid_request, NULL, NULL, NULL);
    if (err) return err;

    hdr = create_hdr(getpid_request, &len, &ptr, 0, tmp);
    if (!hdr) { DNSServiceRefDeallocate(tmp); return kDNSServiceErr_NoMemory; }

    put_uint16(srcport, &ptr);
    err = deliver_request(hdr, tmp);        // Will free hdr for us
    if (err) { DNSServiceRefDeallocate(tmp); return err; }

    ioresult = read_all(tmp->sockfd, (uint8_t *)pid, sizeof(int32_t));
    if (ioresult < read_all_success)
    { DNSServiceRefDeallocate(tmp); return (ioresult == read_all_defunct) ? kDNSServiceErr_DefunctConnection : kDNSServiceErr_ServiceNotRunning; }

    DNSServiceRefDeallocate(tmp);
    return kDNSServiceErr_NoError;
}

static void handle_resolve_response(DNSServiceOp *const sdr, const CallbackHeader *const cbh, const uint8_t *data, const uint8_t *const end)
{
    char fullname[kDNSServiceMaxDomainName];
    char target[kDNSServiceMaxDomainName];
    uint16_t txtlen;
    union { uint16_t s; u_char b[2]; } port;
    const unsigned char *txtrecord;

    get_string(&data, end, fullname, kDNSServiceMaxDomainName);
    get_string(&data, end, target,   kDNSServiceMaxDomainName);
    if (!data || data + 2 > end) goto fail;

    port.b[0] = *data++;
    port.b[1] = *data++;
    txtlen = get_uint16(&data, end);
    txtrecord = (const unsigned char *)get_rdata(&data, end, txtlen);

    if (!data) goto fail;
    ((DNSServiceResolveReply)sdr->AppCallback)(sdr, cbh->cb_flags, cbh->cb_interface, cbh->cb_err, fullname, target, port.s, txtlen, txtrecord, sdr->AppContext);
    return;
    // MUST NOT touch sdr after invoking AppCallback -- client is allowed to dispose it from within callback function
fail:
    syslog(LOG_WARNING, "dnssd_clientstub handle_resolve_response: error reading result from daemon");
}

DNSServiceErrorType DNSSD_API DNSServiceResolve
(
    DNSServiceRef           *sdRef,
    DNSServiceFlags         flags,
    uint32_t                interfaceIndex,
    const char              *name,
    const char              *regtype,
    const char              *domain,
    DNSServiceResolveReply  callBack,
    void                    *context
)
{
    return DNSServiceResolveInternal(sdRef, flags, interfaceIndex, name, regtype, domain, NULL, callBack, context);
}

DNSServiceErrorType DNSServiceResolveInternal
(
    DNSServiceRef *sdRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    const char *name,
    const char *regtype,
    const char *domain,
    const DNSServiceAttribute *attr,
    DNSServiceResolveReply callBack,
    void *context
)
{
    uint8_t *ptr;
    size_t len;
    ipc_msg_hdr *hdr;
    DNSServiceErrorType err;

    if (!sdRef || !name || !regtype || !domain || !callBack) return kDNSServiceErr_BadParam;

    // Need a real InterfaceID for WakeOnResolve
    if ((flags & kDNSServiceFlagsWakeOnResolve) != 0 &&
        ((interfaceIndex == kDNSServiceInterfaceIndexAny) ||
         (interfaceIndex == kDNSServiceInterfaceIndexLocalOnly) ||
         (interfaceIndex == kDNSServiceInterfaceIndexUnicast) ||
         (interfaceIndex == kDNSServiceInterfaceIndexP2P) ||
         (interfaceIndex == kDNSServiceInterfaceIndexBLE)))
    {
        return kDNSServiceErr_BadParam;
    }

    err = ConnectToServer(sdRef, flags, resolve_request, handle_resolve_response, (void *)callBack, context);
    if (err) return err;    // On error ConnectToServer leaves *sdRef set to NULL

    // Calculate total message length
    len = sizeof(flags);
    len += sizeof(interfaceIndex);
    len += strlen(name) + 1;
    len += strlen(regtype) + 1;
    len += strlen(domain) + 1;

    (void)attr;

    hdr = create_hdr(resolve_request, &len, &ptr, (*sdRef)->primary ? 1 : 0, *sdRef);
    if (!hdr) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; return kDNSServiceErr_NoMemory; }

    put_flags(flags, &ptr);
    put_uint32(interfaceIndex, &ptr);
    put_string(name, &ptr);
    put_string(regtype, &ptr);
    put_string(domain, &ptr);

    err = deliver_request(hdr, *sdRef);     // Will free hdr for us
    if (err == kDNSServiceErr_NoAuth && !_should_return_noauth_error())
    {
        err = kDNSServiceErr_NoError;
    }
    if (err) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; }
    return err;
}

static void handle_query_response(DNSServiceOp *const sdr, const CallbackHeader *const cbh, const uint8_t *data, const uint8_t *const end)
{
    if (cbh->cb_err == kDNSServiceErr_PolicyDenied && !_should_return_noauth_error())
    {
        return;
    }
    uint32_t ttl;
    char name[kDNSServiceMaxDomainName];
    uint16_t rrtype, rrclass, rdlen;
    const uint8_t *rdata;

    get_string(&data, end, name, kDNSServiceMaxDomainName);
    rrtype  = get_uint16(&data, end);
    rrclass = get_uint16(&data, end);
    rdlen   = get_uint16(&data, end);
    rdata   = get_rdata(&data, end, rdlen);
    ttl     = get_uint32(&data, end);

    if (!data) syslog(LOG_WARNING, "dnssd_clientstub handle_query_response: error reading result from daemon");
    else ((DNSServiceQueryRecordReply)sdr->AppCallback)(sdr, cbh->cb_flags, cbh->cb_interface, cbh->cb_err, name, rrtype, rrclass, rdlen, rdata, ttl, sdr->AppContext);
    // MUST NOT touch sdr after invoking AppCallback -- client is allowed to dispose it from within callback function
}

DNSServiceErrorType DNSSD_API DNSServiceQueryRecord
(
    DNSServiceRef              *sdRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    const char                 *name,
    uint16_t rrtype,
    uint16_t rrclass,
    DNSServiceQueryRecordReply callBack,
    void                       *context
)
{
    return DNSServiceQueryRecordInternal(sdRef, flags, interfaceIndex, name, rrtype, rrclass, NULL, callBack, context);
}

DNSServiceErrorType DNSServiceQueryRecordInternal
(
    DNSServiceRef *sdRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    const char *name,
    uint16_t rrtype,
    uint16_t rrclass,
    const DNSServiceAttribute *attr,
    DNSServiceQueryRecordReply callBack,
    void *context
)
{
    uint8_t *ptr;
    size_t len;
    ipc_msg_hdr *hdr;
    DNSServiceErrorType err;
    // NULL name handled below.
    if (!sdRef || !callBack) return kDNSServiceErr_BadParam;

    err = ConnectToServer(sdRef, flags, query_request, handle_query_response, (void *)callBack, context);
    if (err) return err;    // On error ConnectToServer leaves *sdRef set to NULL

    if (!name) name = "\0";

    // Calculate total message length
    len = sizeof(flags);
    len += sizeof(uint32_t);  // interfaceIndex
    len += strlen(name) + 1;
    len += 2 * sizeof(uint16_t);  // rrtype, rrclass
    (void)attr;
    hdr = create_hdr(query_request, &len, &ptr, (*sdRef)->primary ? 1 : 0, *sdRef);
    if (!hdr)
    {
        DNSServiceRefDeallocate(*sdRef);
        *sdRef = NULL;
        return kDNSServiceErr_NoMemory;
    }
    put_flags(flags, &ptr);
    put_uint32(interfaceIndex, &ptr);
    put_string(name, &ptr);
    put_uint16(rrtype, &ptr);
    put_uint16(rrclass, &ptr);
    err = deliver_request(hdr, *sdRef);     // Will free hdr for us
    if (err == kDNSServiceErr_NoAuth && !_should_return_noauth_error())
    {
        err = kDNSServiceErr_NoError;
    }
    if (err) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; }
    return err;
}

static void handle_addrinfo_response(DNSServiceOp *const sdr, const CallbackHeader *const cbh, const uint8_t *data, const uint8_t *const end)
{
    if (cbh->cb_err == kDNSServiceErr_PolicyDenied && !_should_return_noauth_error())
    {
        return;
    }
    char hostname[kDNSServiceMaxDomainName];
    uint16_t rrtype, rrclass, rdlen;
    const uint8_t *rdata;
    uint32_t ttl;

    get_string(&data, end, hostname, kDNSServiceMaxDomainName);
    rrtype  = get_uint16(&data, end);
    rrclass = get_uint16(&data, end);
    rdlen   = get_uint16(&data, end);
    rdata   = get_rdata (&data, end, rdlen);
    ttl     = get_uint32(&data, end);
    (void)rrclass; // Unused
    // We only generate client callbacks for A and AAAA results (including NXDOMAIN results for
    // those types, if the client has requested those with the kDNSServiceFlagsReturnIntermediates).
    // Other result types, specifically CNAME referrals, are not communicated to the client, because
    // the DNSServiceGetAddrInfoReply interface doesn't have any meaningful way to communiate CNAME referrals.
    if (!data) syslog(LOG_WARNING, "dnssd_clientstub handle_addrinfo_response: error reading result from daemon");
    else if (rrtype == kDNSServiceType_A || rrtype == kDNSServiceType_AAAA)
    {
        struct sockaddr_in sa4;
        struct sockaddr_in6 sa6;
        const struct sockaddr *const sa = (rrtype == kDNSServiceType_A) ? (struct sockaddr*)&sa4 : (struct sockaddr*)&sa6;
        if (rrtype == kDNSServiceType_A)
        {
            memset(&sa4, 0, sizeof(sa4));
            #ifndef NOT_HAVE_SA_LEN
            sa4.sin_len = sizeof(struct sockaddr_in);
            #endif
            sa4.sin_family = AF_INET;
            //  sin_port   = 0;
            if (!cbh->cb_err) memcpy(&sa4.sin_addr, rdata, rdlen);
        }
        else
        {
            memset(&sa6, 0, sizeof(sa6));
            #ifndef NOT_HAVE_SA_LEN
            sa6.sin6_len = sizeof(struct sockaddr_in6);
            #endif
            sa6.sin6_family     = AF_INET6;
            //  sin6_port     = 0;
            //  sin6_flowinfo = 0;
            //  sin6_scope_id = 0;
            if (!cbh->cb_err)
            {
                memcpy(&sa6.sin6_addr, rdata, rdlen);
                if (IN6_IS_ADDR_LINKLOCAL(&sa6.sin6_addr)) sa6.sin6_scope_id = cbh->cb_interface;
            }
        }

        ((DNSServiceGetAddrInfoReply)sdr->AppCallback)(sdr, cbh->cb_flags, cbh->cb_interface, cbh->cb_err, hostname, sa, ttl, sdr->AppContext);
    }
    else if (cbh->cb_err == kDNSServiceErr_PolicyDenied)
    {
        ((DNSServiceGetAddrInfoReply)sdr->AppCallback)(sdr, cbh->cb_flags, cbh->cb_interface, cbh->cb_err, hostname, NULL, ttl, sdr->AppContext);
    }
}

DNSServiceErrorType DNSSD_API DNSServiceGetAddrInfo
(
    DNSServiceRef               *sdRef,
    DNSServiceFlags             flags,
    uint32_t                    interfaceIndex,
    uint32_t                    protocol,
    const char                  *hostname,
    DNSServiceGetAddrInfoReply  callBack,
    void                        *context          /* may be NULL */
)
{
    return DNSServiceGetAddrInfoInternal(sdRef, flags, interfaceIndex, protocol, hostname, NULL, callBack, context);
}

DNSServiceErrorType DNSServiceGetAddrInfoInternal
(
    DNSServiceRef *sdRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    uint32_t protocol,
    const char *hostname,
    const DNSServiceAttribute *attr,
    DNSServiceGetAddrInfoReply callBack,
    void *context
)
{
    uint8_t *ptr;
    size_t len;
    ipc_msg_hdr *hdr;
    DNSServiceErrorType err;

    if (!sdRef || !hostname || !callBack) return kDNSServiceErr_BadParam;

    err = ConnectToServer(sdRef, flags, addrinfo_request, handle_addrinfo_response, (void *)callBack, context);
    if (err)
    {
         return err;    // On error ConnectToServer leaves *sdRef set to NULL
    }

    // Calculate total message length
    len = sizeof(flags);
    len += sizeof(uint32_t);      // interfaceIndex
    len += sizeof(uint32_t);      // protocol
    len += strlen(hostname) + 1;
    (void)attr;
    hdr = create_hdr(addrinfo_request, &len, &ptr, (*sdRef)->primary ? 1 : 0, *sdRef);
    if (!hdr)
    {
        DNSServiceRefDeallocate(*sdRef);
        *sdRef = NULL;
        return kDNSServiceErr_NoMemory;
    }
    put_flags(flags, &ptr);
    put_uint32(interfaceIndex, &ptr);
    put_uint32(protocol, &ptr);
    put_string(hostname, &ptr);
    err = deliver_request(hdr, *sdRef);     // Will free hdr for us
    if (err == kDNSServiceErr_NoAuth && !_should_return_noauth_error())
    {
        err = kDNSServiceErr_NoError;
    }
    if (err) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; }
    return err;
}

static void handle_browse_response(DNSServiceOp *const sdr, const CallbackHeader *const cbh, const uint8_t *data, const uint8_t *const end)
{
    if (cbh->cb_err == kDNSServiceErr_PolicyDenied && !_should_return_noauth_error())
    {
        return;
    }
    char replyName[256], replyType[kDNSServiceMaxDomainName], replyDomain[kDNSServiceMaxDomainName];
    get_string(&data, end, replyName, 256);
    get_string(&data, end, replyType, kDNSServiceMaxDomainName);
    get_string(&data, end, replyDomain, kDNSServiceMaxDomainName);
    if (!data) syslog(LOG_WARNING, "dnssd_clientstub handle_browse_response: error reading result from daemon");
    else ((DNSServiceBrowseReply)sdr->AppCallback)(sdr, cbh->cb_flags, cbh->cb_interface, cbh->cb_err, replyName, replyType, replyDomain, sdr->AppContext);
    // MUST NOT touch sdr after invoking AppCallback -- client is allowed to dispose it from within callback function
}

DNSServiceErrorType DNSSD_API DNSServiceBrowse
(
    DNSServiceRef         *sdRef,
    DNSServiceFlags       flags,
    uint32_t              interfaceIndex,
    const char            *regtype,
    const char            *domain,
    DNSServiceBrowseReply callBack,
    void                  *context
)
{
    return DNSServiceBrowseInternal(sdRef, flags, interfaceIndex, regtype, domain, NULL, callBack, context);
}

DNSServiceErrorType DNSServiceBrowseInternal
(
    DNSServiceRef *sdRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    const char *regtype,
    const char *domain,
    const DNSServiceAttribute *attr,
    DNSServiceBrowseReply callBack,
    void *context
)
{
    uint8_t *ptr;
    size_t len;
    ipc_msg_hdr *hdr;
    DNSServiceErrorType err;

    // NULL domain handled below
    if (!sdRef || !regtype || !callBack) return kDNSServiceErr_BadParam;

    err = ConnectToServer(sdRef, flags, browse_request, handle_browse_response, (void *)callBack, context);
    if (err) return err;    // On error ConnectToServer leaves *sdRef set to NULL

    if (!domain) domain = "";
    len = sizeof(flags);
    len += sizeof(interfaceIndex);
    len += strlen(regtype) + 1;
    len += strlen(domain) + 1;

    (void)attr;

    hdr = create_hdr(browse_request, &len, &ptr, (*sdRef)->primary ? 1 : 0, *sdRef);
    if (!hdr) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; return kDNSServiceErr_NoMemory; }

    put_flags(flags, &ptr);
    put_uint32(interfaceIndex, &ptr);
    put_string(regtype, &ptr);
    put_string(domain, &ptr);

    err = deliver_request(hdr, *sdRef);     // Will free hdr for us
    if (err == kDNSServiceErr_NoAuth && !_should_return_noauth_error())
    {
        err = kDNSServiceErr_NoError;
    }
    if (err) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; }
    return err;
}

DNSServiceErrorType DNSSD_API DNSServiceSetDefaultDomainForUser(DNSServiceFlags flags, const char *domain)
{
    DNSServiceErrorType err;
    DNSServiceOp *tmp;
    uint8_t *ptr;
    size_t len;
    ipc_msg_hdr *hdr;

    if (!domain) return kDNSServiceErr_BadParam;
    len = sizeof(flags) + strlen(domain) + 1;

    err = ConnectToServer(&tmp, 0, setdomain_request, NULL, NULL, NULL);
    if (err) return err;

    hdr = create_hdr(setdomain_request, &len, &ptr, 0, tmp);
    if (!hdr) { DNSServiceRefDeallocate(tmp); return kDNSServiceErr_NoMemory; }

    put_flags(flags, &ptr);
    put_string(domain, &ptr);
    err = deliver_request(hdr, tmp);        // Will free hdr for us
    DNSServiceRefDeallocate(tmp);
    return err;
}

static void handle_regservice_response(DNSServiceOp *const sdr, const CallbackHeader *const cbh, const uint8_t *data, const uint8_t *const end)
{
    if (cbh->cb_err == kDNSServiceErr_PolicyDenied && !_should_return_noauth_error())
    {
        return;
    }
    char name[256], regtype[kDNSServiceMaxDomainName], domain[kDNSServiceMaxDomainName];
    get_string(&data, end, name, 256);
    get_string(&data, end, regtype, kDNSServiceMaxDomainName);
    get_string(&data, end, domain,  kDNSServiceMaxDomainName);
    if (!data) syslog(LOG_WARNING, "dnssd_clientstub handle_regservice_response: error reading result from daemon");
    else ((DNSServiceRegisterReply)sdr->AppCallback)(sdr, cbh->cb_flags, cbh->cb_err, name, regtype, domain, sdr->AppContext);
    // MUST NOT touch sdr after invoking AppCallback -- client is allowed to dispose it from within callback function
}

DNSServiceErrorType DNSSD_API DNSServiceRegister
(
    DNSServiceRef                       *sdRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    const char                          *name,
    const char                          *regtype,
    const char                          *domain,
    const char                          *host,
    uint16_t PortInNetworkByteOrder,
    uint16_t txtLen,
    const void                          *txtRecord,
    DNSServiceRegisterReply callBack,
    void                                *context
)
{
    return DNSServiceRegisterInternal(sdRef, flags, interfaceIndex, name, regtype, domain, host, PortInNetworkByteOrder, txtLen, txtRecord, NULL, callBack, context);
}

DNSServiceErrorType DNSSD_API DNSServiceRegisterWithAttribute
(
    DNSServiceRef *sdRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    const char *name,
    const char *regtype,
    const char *domain,
    const char *host,
    uint16_t portInNetworkByteOrder,
    uint16_t txtLen,
    const void *txtRecord,
    const DNSServiceAttributeRef attr,
    DNSServiceRegisterReply callBack,
    void *context
)
{
    return DNSServiceRegisterInternal(sdRef, flags, interfaceIndex, name, regtype, domain, host, portInNetworkByteOrder, txtLen, txtRecord, attr, callBack, context);
}

DNSServiceErrorType DNSServiceRegisterInternal
(
    DNSServiceRef *sdRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    const char *name,
    const char *regtype,
    const char *domain,
    const char *host,
    uint16_t portInNetworkByteOrder,
    uint16_t txtLen,
    const void *txtRecord,
    const DNSServiceAttribute *attr,
    DNSServiceRegisterReply callBack,
    void *context
)
{
    uint8_t *ptr;
    size_t len;
    ipc_msg_hdr *hdr;
    DNSServiceErrorType err;
    union { uint16_t s; u_char b[2]; } port = { portInNetworkByteOrder };
    (void)attr;

    if (!sdRef || !regtype) return kDNSServiceErr_BadParam;
    if (!name) name = "";
    if (!domain) domain = "";
    if (!host) host = "";
    if (!txtRecord) txtRecord = (void*)"";

    // No callback must have auto-rename
    if (!callBack && (flags & kDNSServiceFlagsNoAutoRename)) return kDNSServiceErr_BadParam;

    err = ConnectToServer(sdRef, flags, reg_service_request, callBack ? handle_regservice_response : NULL, (void *)callBack, context);
    if (err) return err;    // On error ConnectToServer leaves *sdRef set to NULL

    len = sizeof(DNSServiceFlags);
    len += sizeof(uint32_t);  // interfaceIndex
    len += strlen(name) + strlen(regtype) + strlen(domain) + strlen(host) + 4;
    len += 2 * sizeof(uint16_t);  // port, txtLen
    len += txtLen;
    if (attr)
    {
        if (!validate_attribute_tlvs(attr))
        {
            return kDNSServiceErr_BadParam;
        }
        len += get_required_length_for_attribute_tlvs(attr);
    }

    hdr = create_hdr(reg_service_request, &len, &ptr, (*sdRef)->primary ? 1 : 0, *sdRef);
    if (!hdr) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; return kDNSServiceErr_NoMemory; }
    if (!callBack) hdr->ipc_flags |= IPC_FLAGS_NOREPLY;

    put_flags(flags, &ptr);
    put_uint32(interfaceIndex, &ptr);
    put_string(name, &ptr);
    put_string(regtype, &ptr);
    put_string(domain, &ptr);
    put_string(host, &ptr);
    *ptr++ = port.b[0];
    *ptr++ = port.b[1];
    put_uint16(txtLen, &ptr);
    put_rdata(txtLen, txtRecord, &ptr);
#if 0
    if (attr)
    {
        put_attribute_tlvs(attr, hdr, &ptr, limit);
    }
#endif

    err = deliver_request(hdr, *sdRef);     // Will free hdr for us
    if (err == kDNSServiceErr_NoAuth && !_should_return_noauth_error())
    {
        err = kDNSServiceErr_NoError;
    }
    if (err) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; }
    return err;
}

static void handle_enumeration_response(DNSServiceOp *const sdr, const CallbackHeader *const cbh, const uint8_t *data, const uint8_t *const end)
{
    char domain[kDNSServiceMaxDomainName];
    get_string(&data, end, domain, kDNSServiceMaxDomainName);
    if (!data) syslog(LOG_WARNING, "dnssd_clientstub handle_enumeration_response: error reading result from daemon");
    else ((DNSServiceDomainEnumReply)sdr->AppCallback)(sdr, cbh->cb_flags, cbh->cb_interface, cbh->cb_err, domain, sdr->AppContext);
    // MUST NOT touch sdr after invoking AppCallback -- client is allowed to dispose it from within callback function
}

DNSServiceErrorType DNSSD_API DNSServiceEnumerateDomains
(
    DNSServiceRef             *sdRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    DNSServiceDomainEnumReply callBack,
    void                      *context
)
{
    uint8_t *ptr;
    size_t len;
    ipc_msg_hdr *hdr;
    DNSServiceErrorType err;
    int f1;
    int f2;

    if (!sdRef || !callBack) return kDNSServiceErr_BadParam;

    f1 = (flags & kDNSServiceFlagsBrowseDomains) != 0;
    f2 = (flags & kDNSServiceFlagsRegistrationDomains) != 0;
    if (f1 + f2 != 1) return kDNSServiceErr_BadParam;

    err = ConnectToServer(sdRef, flags, enumeration_request, handle_enumeration_response, (void *)callBack, context);
    if (err) return err;    // On error ConnectToServer leaves *sdRef set to NULL

    len = sizeof(DNSServiceFlags);
    len += sizeof(uint32_t);

    hdr = create_hdr(enumeration_request, &len, &ptr, (*sdRef)->primary ? 1 : 0, *sdRef);
    if (!hdr) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; return kDNSServiceErr_NoMemory; }

    put_flags(flags, &ptr);
    put_uint32(interfaceIndex, &ptr);

    err = deliver_request(hdr, *sdRef);     // Will free hdr for us
    if (err) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; }
    return err;
}

static void ConnectionResponse(DNSServiceOp *const sdr, const CallbackHeader *const cbh, const uint8_t *const data, const uint8_t *const end)
{
    (void)data; // Unused

    //printf("ConnectionResponse got %d\n", cbh->ipc_hdr.op);
    if (cbh->ipc_hdr.op != reg_record_reply_op && cbh->ipc_hdr.op != async_error_op)
    {
        // When using kDNSServiceFlagsShareConnection, need to search the list of associated DNSServiceOps
        // to find the one this response is intended for, and then call through to its ProcessReply handler.
        // We start with our first subordinate DNSServiceRef -- don't want to accidentally match the parent DNSServiceRef.
        DNSServiceOp *op = sdr->next;
        while (op && (op->uid.u32[0] != cbh->ipc_hdr.client_context.u32[0] || op->uid.u32[1] != cbh->ipc_hdr.client_context.u32[1]))
            op = op->next;
        // Note: We may sometimes not find a matching DNSServiceOp, in the case where the client has
        // cancelled the subordinate DNSServiceOp, but there are still messages in the pipeline from the daemon
        if (op && op->ProcessReply) op->ProcessReply(op, cbh, data, end);
        // WARNING: Don't touch op or sdr after this -- client may have called DNSServiceRefDeallocate
        return;
    }
    else
    {
        if (cbh->cb_err == kDNSServiceErr_PolicyDenied && !_should_return_noauth_error())
        {
            return;
        }
        DNSRecordRef rec;
        for (rec = sdr->rec; rec; rec = rec->recnext)
        {
            if (rec->uid.u32[0] == cbh->ipc_hdr.client_context.u32[0] && rec->uid.u32[1] == cbh->ipc_hdr.client_context.u32[1])
                break;
        }
        // The record might have been freed already and hence not an
        // error if the record is not found.
        if (!rec)
        {
            syslog(LOG_INFO, "dnssd_clientstub ConnectionResponse: Record not found");
            return;
        }
        if (rec->sdr != sdr)
        {
            syslog(LOG_WARNING, "dnssd_clientstub ConnectionResponse: Record sdr mismatch: rec %p sdr %p", rec->sdr, sdr);
            return;
        }

        if (sdr->op == connection_request || sdr->op == connection_delegate_request)
        {
            rec->AppCallback(rec->sdr, rec, cbh->cb_flags, cbh->cb_err, rec->AppContext);
        }
        else
        {
            syslog(LOG_WARNING, "dnssd_clientstub ConnectionResponse: sdr->op != connection_request");
            rec->AppCallback(rec->sdr, rec, 0, kDNSServiceErr_Unknown, rec->AppContext);
        }
        // MUST NOT touch sdr after invoking AppCallback -- client is allowed to dispose it from within callback function
    }
}

DNSServiceErrorType DNSSD_API DNSServiceCreateConnection(DNSServiceRef *sdRef)
{
    DNSServiceErrorType err;
    uint8_t *ptr;
    size_t len = 0;
    ipc_msg_hdr *hdr;

    if (!sdRef) return kDNSServiceErr_BadParam;
    err = ConnectToServer(sdRef, 0, connection_request, ConnectionResponse, NULL, NULL);
    if (err) return err;    // On error ConnectToServer leaves *sdRef set to NULL

    hdr = create_hdr(connection_request, &len, &ptr, 0, *sdRef);
    if (!hdr) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; return kDNSServiceErr_NoMemory; }

    err = deliver_request(hdr, *sdRef);     // Will free hdr for us
    if (err) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; }
    return err;
}

#if   TARGET_OS_SIMULATOR // This hack is for Simulator platform only
DNSServiceErrorType DNSSD_API DNSServiceCreateDelegateConnection(DNSServiceRef *sdRef, int32_t pid, uuid_t uuid)
{
    (void) pid;
    (void) uuid;
    return DNSServiceCreateConnection(sdRef);
}
#endif

DNSServiceErrorType DNSServiceSendQueuedRequestsInternal(DNSServiceRef sdr)
{
    iovec_t *iov;
    ssize_t totalLength = 0, bytesWritten;
    uint32_t numMsg, i;
    DNSRecordRef rref;
    DNSServiceErrorType err = kDNSServiceErr_NoError;

    if (!sdr)
    {
        syslog(LOG_WARNING, "DNSServiceSendQueuedRequestsInternal: !sdr");
        return kDNSServiceErr_BadParam;
    }
    for (rref = sdr->rec, numMsg = 0; rref != NULL; rref = rref->recnext)
    {
        if(rref->msg)
        {
            numMsg++;
            totalLength += rref->msg->datalen + sizeof(ipc_msg_hdr);
        }
    }
    if (numMsg == 0)
    {
        syslog(LOG_INFO, "DNSServiceSendQueuedRequestsInternal: numMsg is 0");
        return kDNSServiceErr_Invalid;
    }
    iov = mdns_malloc(numMsg * sizeof(*iov));
    if (!iov)
    {
        return kDNSServiceErr_NoMemory;
    }
    for (rref = sdr->rec, i = 0; rref != NULL; rref = rref->recnext)
    {
        if(rref->msg)
        {
            uint32_t datalen = rref->msg->datalen;
            ConvertHeaderBytes(rref->msg);
            SETIOV(&iov[i], rref->msg, datalen + sizeof(ipc_msg_hdr));
            i++;
        }
    }
    bytesWritten = writev(sdr->sockfd, iov, numMsg);
    if (bytesWritten != totalLength)
    {
        syslog(LOG_WARNING,"DNSServiceSendQueuedRequestsInternal ERROR: writev(fd:%d, written:%zu, total:%zu bytes) failed, errno[%d]:%s",
               sdr->sockfd, bytesWritten, totalLength, errno, strerror(errno));
        err = kDNSServiceErr_Unknown;
    }
    else
    {
        syslog(LOG_INFO, "DNSServiceSendQueuedRequestsInternal: writev(fd:%d, numMsg:%d, %zu bytes) succeed",
               sdr->sockfd, numMsg, totalLength);
    }
    for (rref = sdr->rec; rref != NULL; rref = rref->recnext)
    {
        mdns_free(rref->msg);
    }
    mdns_free(iov);
    return err;
}

DNSServiceErrorType DNSSD_API DNSServiceRegisterRecord
(
    DNSServiceRef sdRef,
    DNSRecordRef *recordRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    const char *fullname,
    uint16_t rrtype,
    uint16_t rrclass,
    uint16_t rdlen,
    const void *rdata,
    uint32_t ttl,
    DNSServiceRegisterRecordReply callBack,
    void *context
)
{
    return DNSServiceRegisterRecordInternal(sdRef, recordRef, flags, interfaceIndex, fullname, rrtype, rrclass, rdlen, rdata, ttl, NULL, callBack, context);
}

DNSServiceErrorType DNSSD_API DNSServiceRegisterRecordWithAttribute
(
    DNSServiceRef sdRef,
    DNSRecordRef *recordRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    const char *fullname,
    uint16_t rrtype,
    uint16_t rrclass,
    uint16_t rdlen,
    const void *rdata,
    uint32_t ttl,
    const DNSServiceAttributeRef attr,
    DNSServiceRegisterRecordReply callBack,
    void *context
)
{
    return DNSServiceRegisterRecordInternal(sdRef, recordRef, flags, interfaceIndex, fullname, rrtype, rrclass, rdlen, rdata, ttl, attr, callBack, context);
}

DNSServiceErrorType DNSServiceRegisterRecordInternal
 (
    DNSServiceRef sdRef,
    DNSRecordRef *RecordRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    const char *fullname,
    uint16_t rrtype,
    uint16_t rrclass,
    uint16_t rdlen,
    const void *rdata,
    uint32_t ttl,
    const DNSServiceAttribute *attr,
    DNSServiceRegisterRecordReply callBack,
    void *context
)
{
    DNSServiceErrorType err;
    uint8_t *ptr;
    size_t len;
    ipc_msg_hdr *hdr = NULL;
    DNSRecordRef rref = NULL;
    DNSRecord **p;
    (void)attr;

    // Verify that only one of the following flags is set.
    int f1 = (flags & kDNSServiceFlagsShared) != 0;
    int f2 = (flags & kDNSServiceFlagsUnique) != 0;
    int f3 = (flags & kDNSServiceFlagsKnownUnique) != 0;
    if (f1 + f2 + f3 != 1) return kDNSServiceErr_BadParam;

    if (!sdRef || !RecordRef || !fullname || (!rdata && rdlen) || !callBack)
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRegisterRecord called with NULL parameter");
        return kDNSServiceErr_BadParam;
    }

    if (!DNSServiceRefValid(sdRef))
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRegisterRecord called with invalid DNSServiceRef %p %08X %08X", sdRef, sdRef->sockfd, sdRef->validator);
        return kDNSServiceErr_BadReference;
    }

    if (sdRef->op != connection_request && sdRef->op != connection_delegate_request)
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRegisterRecord called with non-DNSServiceCreateConnection DNSServiceRef %p %d", sdRef, sdRef->op);
        return kDNSServiceErr_BadReference;
    }

    *RecordRef = NULL;

    len = sizeof(DNSServiceFlags);
    len += 2 * sizeof(uint32_t);  // interfaceIndex, ttl
    len += 3 * sizeof(uint16_t);  // rrtype, rrclass, rdlen
    len += strlen(fullname) + 1;
    len += rdlen;
    if (attr)
    {
        if (!validate_attribute_tlvs(attr))
        {
            return kDNSServiceErr_BadParam;
        }
        len += get_required_length_for_attribute_tlvs(attr);
    }

    // Bump up the uid. Normally for shared operations (kDNSServiceFlagsShareConnection), this
    // is done in ConnectToServer. For DNSServiceRegisterRecord, ConnectToServer has already
    // been called. As multiple DNSServiceRegisterRecords can be multiplexed over a single
    // connection, we need a way to demultiplex the response so that the callback corresponding
    // to the right DNSServiceRegisterRecord instance can be called. Use the same mechanism that
    // is used by kDNSServiceFlagsShareConnection. create_hdr copies the uid value to ipc
    // hdr->client_context which will be returned in the ipc response.
    if (++sdRef->uid.u32[0] == 0)
        ++sdRef->uid.u32[1];
    //If kDNSServiceFlagsQueueRequest flag is set, do not make separate return socket.
    hdr = create_hdr(reg_record_request, &len, &ptr, !(flags & kDNSServiceFlagsQueueRequest), sdRef);
    if (!hdr) return kDNSServiceErr_NoMemory;

    put_flags(flags, &ptr);
    put_uint32(interfaceIndex, &ptr);
    put_string(fullname, &ptr);
    put_uint16(rrtype, &ptr);
    put_uint16(rrclass, &ptr);
    put_uint16(rdlen, &ptr);
    put_rdata(rdlen, rdata, &ptr);
    put_uint32(ttl, &ptr);
#if 0
    if (attr)
    {
        put_attribute_tlvs(attr, hdr, &ptr, limit);
    }
#endif
    if (flags & kDNSServiceFlagsQueueRequest)
    {
        hdr->ipc_flags |= IPC_FLAGS_NOERRSD;
    }
    rref = mdns_calloc(1, sizeof(*rref));
    if (!rref) { mdns_free(hdr); return kDNSServiceErr_NoMemory; }
#ifdef MEMORY_OBJECT_TRACKING
    extern int rref_created;
    rref_created++;
#endif
    rref->AppContext = context;
    rref->AppCallback = callBack;
    rref->record_index = sdRef->max_index++;
    rref->sdr = sdRef;
    *RecordRef = rref;
    // Remember the uid that we are sending across so that we can match
    // when the response comes back.
    rref->uid = sdRef->uid;
    hdr->reg_index = rref->record_index;

    p = &(sdRef)->rec;
    while (*p) p = &(*p)->recnext;
    *p = rref;
    // If kDNSServiceFlagsQueueRequest flag is set, put the hdr in linked records
    if (flags & kDNSServiceFlagsQueueRequest)
    {
        rref->msg = hdr;
        err = kDNSServiceErr_NoError;
    }
    else
    {
        err = deliver_request(hdr, sdRef);     // Will free hdr for us
        if (err == kDNSServiceErr_NoAuth && !_should_return_noauth_error())
        {
            err = kDNSServiceErr_NoError;
        }
    }
    return err;
}

// sdRef returned by DNSServiceRegister()
DNSServiceErrorType DNSSD_API DNSServiceAddRecord
(
    DNSServiceRef sdRef,
    DNSRecordRef    *RecordRef,
    DNSServiceFlags flags,
    uint16_t rrtype,
    uint16_t rdlen,
    const void      *rdata,
    uint32_t ttl
)
{
    ipc_msg_hdr *hdr;
    size_t len = 0;
    uint8_t *ptr;
    DNSRecordRef rref;
    DNSRecord **p;

    if (!sdRef || !RecordRef || (!rdata && rdlen))
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceAddRecord called with NULL parameter");
        return kDNSServiceErr_BadParam;
    }
    if (sdRef->op != reg_service_request)
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceAddRecord called with non-DNSServiceRegister DNSServiceRef %p %d", sdRef, sdRef->op);
        return kDNSServiceErr_BadReference;
    }

    if (!DNSServiceRefValid(sdRef))
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceAddRecord called with invalid DNSServiceRef %p %08X %08X", sdRef, sdRef->sockfd, sdRef->validator);
        return kDNSServiceErr_BadReference;
    }

    *RecordRef = NULL;

    len += 2 * sizeof(uint16_t);  // rrtype, rdlen
    len += rdlen;
    len += sizeof(uint32_t);
    len += sizeof(DNSServiceFlags);

    hdr = create_hdr(add_record_request, &len, &ptr, 1, sdRef);
    if (!hdr) return kDNSServiceErr_NoMemory;
    put_flags(flags, &ptr);
    put_uint16(rrtype, &ptr);
    put_uint16(rdlen, &ptr);
    put_rdata(rdlen, rdata, &ptr);
    put_uint32(ttl, &ptr);

    rref = mdns_calloc(1, sizeof(*rref));
    if (!rref) { mdns_free(hdr); return kDNSServiceErr_NoMemory; }
#ifdef MEMORY_OBJECT_TRACKING
    extern int rref_created;
    rref_created++;
#endif
    rref->record_index = sdRef->max_index++;
    rref->sdr = sdRef;
    *RecordRef = rref;
    hdr->reg_index = rref->record_index;

    p = &(sdRef)->rec;
    while (*p) p = &(*p)->recnext;
    *p = rref;

    return deliver_request(hdr, sdRef);     // Will free hdr for us
}

static DNSServiceErrorType DNSServiceUpdateRecordInternal
(
    DNSServiceRef sdRef,
    DNSRecordRef recordRef,
    DNSServiceFlags flags,
    uint16_t rdlen,
    const void *rdata,
    uint32_t ttl,
    const DNSServiceAttributeRef attr
)
{
    ipc_msg_hdr *hdr;
    size_t len = 0;
    uint8_t *ptr;
    const uint8_t *limit;

    if (!sdRef || (!rdata && rdlen))
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceUpdateRecord called with NULL parameter");
        return kDNSServiceErr_BadParam;
    }

    if (!DNSServiceRefValid(sdRef))
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceUpdateRecord called with invalid DNSServiceRef %p %08X %08X", sdRef, sdRef->sockfd, sdRef->validator);
        return kDNSServiceErr_BadReference;
    }

    // Note: RecordRef is allowed to be NULL

    len += sizeof(uint16_t);
    len += rdlen;
    len += sizeof(uint32_t);
    len += sizeof(DNSServiceFlags);
    if (attr)
    {
        if (!validate_attribute_tlvs(attr))
        {
            return kDNSServiceErr_BadParam;
        }
        len += get_required_length_for_attribute_tlvs(attr);
    }

    hdr = create_hdr(update_record_request, &len, &ptr, 1, sdRef);
    if (!hdr) return kDNSServiceErr_NoMemory;
    // This function can update records added with DNSServiceRegisterRecord or DNSServiceAddRecord. In the
    // former case, these records are added on a connection that was created using DNSServiceCreateConnection(), and so
    // they don't have a subordinate request. In the latter case, they are added on a connection that was created with
    // DNSServiceRegister(); if these are created with the kDNSServiceFlagsSharedConnection flag set, then they will have
    // a subordinate operation.
    // In the case where there is no subordinate operations, we need to send a UID of zero, to avoid matching any subordinate
    // operation that might have the same UID as the primary connection (this will be the case if there is an outstanding
    // subordinate request that hasn't been canceled). Failure to send a zero UID can result in this function
    // having no effect. Refer to rdar://93274463
    if (sdRef->primary == NULL)
    {
        hdr->client_context.u32[0] = 0;
        hdr->client_context.u32[1] = 0;
    }
    hdr->reg_index = recordRef ? recordRef->record_index : TXT_RECORD_INDEX;
    limit = ptr + len;
    put_flags(flags, &ptr);
    put_uint16(rdlen, &ptr);
    put_rdata(rdlen, rdata, &ptr);
    put_uint32(ttl, &ptr);
    if (attr)
    {
        put_attribute_tlvs(attr, hdr, &ptr, limit);
    }
    return deliver_request(hdr, sdRef);     // Will free hdr for us
}

// DNSRecordRef returned by DNSServiceRegisterRecord or DNSServiceAddRecord
DNSServiceErrorType DNSSD_API DNSServiceUpdateRecord
(
    DNSServiceRef sdRef,
    DNSRecordRef recordRef,
    DNSServiceFlags flags,
    uint16_t rdlen,
    const void *rdata,
    uint32_t ttl
)
{
    return DNSServiceUpdateRecordInternal(sdRef, recordRef, flags, rdlen, rdata, ttl, NULL);
}

DNSServiceErrorType DNSSD_API DNSServiceUpdateRecordWithAttribute
(
    DNSServiceRef sdRef,
    DNSRecordRef recordRef,
    DNSServiceFlags flags,
    uint16_t rdlen,
    const void *rdata,
    uint32_t ttl,
    const DNSServiceAttributeRef attr
)
{
    return DNSServiceUpdateRecordInternal(sdRef, recordRef, flags, rdlen, rdata, ttl, attr);
}

DNSServiceErrorType DNSSD_API DNSServiceRemoveRecord
(
    DNSServiceRef sdRef,
    DNSRecordRef RecordRef,
    DNSServiceFlags flags
)
{
    ipc_msg_hdr *hdr;
    size_t len = 0;
    uint8_t *ptr;
    DNSServiceErrorType err;

    if (!sdRef)            { syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRemoveRecord called with NULL DNSServiceRef"); return kDNSServiceErr_BadParam; }
    if (!RecordRef)        { syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRemoveRecord called with NULL DNSRecordRef");  return kDNSServiceErr_BadParam; }
    if (!sdRef->max_index) { syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRemoveRecord called with bad DNSServiceRef");  return kDNSServiceErr_BadReference; }

    if (!DNSServiceRefValid(sdRef))
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRemoveRecord called with invalid DNSServiceRef %p %08X %08X",
               sdRef, sdRef->sockfd, sdRef->validator);
        return kDNSServiceErr_BadReference;
    }

    // Ensure that this rref is actually dependent on the sdref. An rref can't not be dependent on an sdref.
    DNSRecord **p = &sdRef->rec;
    while (*p && *p != RecordRef) p = &(*p)->recnext;
    if (*p == NULL)
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceRemoveRecord called with invalid DNSRecordRef %p %08X %08X",
               RecordRef, sdRef->sockfd, sdRef->validator);
        return kDNSServiceErr_BadReference;
    }

    len += sizeof(flags);
    hdr = create_hdr(remove_record_request, &len, &ptr, 1, sdRef);
    if (!hdr) return kDNSServiceErr_NoMemory;
    // DNSServiceRemoveRecord can remove records added with DNSServiceRegisterRecord or DNSServiceAddRecord. In the
    // former case, these records are added on a connection that was created using DNSServiceCreateConnection(), and so
    // they don't have a subordinate request. In the latter case, they are added on a connection that was created with
    // DNSServiceRegister(); if these are created with the kDNSServiceFlagsSharedConnection flag set, then they will have
    // a subordinate operation.
    // In the case where there is no subordinate operation, we need to send a UID of zero, to avoid matching any subordinate
    // operation that might have the same UID as the primary connection (this will be the case if there is an outstanding
    // subordinate request that hasn't been canceled). Failure to send a zero UID can result in the DNSServiceRemoveRecord
    // having no effect.
    if (sdRef->primary == NULL)
    {
        hdr->client_context.u32[0] = 0;
        hdr->client_context.u32[1] = 0;
    }
    hdr->reg_index = RecordRef->record_index;
    put_flags(flags, &ptr);
    err = deliver_request(hdr, sdRef);      // Will free hdr for us
    if (!err || err == kDNSServiceErr_BadReference)
    {
        // This RecordRef could only have been allocated in DNSServiceRegisterRecord or DNSServiceAddRecord.
        // Delink from the list before freeing
        *p = RecordRef->recnext;
#ifdef MEMORY_OBJECT_TRACKING
        extern int rref_finalized;
        rref_finalized++;
#endif
        mdns_free(RecordRef->msg);
        mdns_free(RecordRef);

        // In the event that we got a BadReference from mDNSResponder, this means that the DNSServiceRegisterRecord
        // or DNSServiceAddRecord call that created the rref data structure and added it to the sdref didn't succeed
        // in creating a registration in the mDNSResponder process, so when we told mDNSResponder to remove it, it
        // didn't find anything to remove. In this case, it doesn't make sense to return an error to the caller, because
        // we have successfully removed the rref.
        err = kDNSServiceErr_NoError;
    }
    return err;
}

DNSServiceErrorType DNSSD_API DNSServiceReconfirmRecord
(
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    const char      *fullname,
    uint16_t rrtype,
    uint16_t rrclass,
    uint16_t rdlen,
    const void      *rdata
)
{
    DNSServiceErrorType err;
    uint8_t *ptr;
    size_t len;
    ipc_msg_hdr *hdr;
    DNSServiceOp *tmp = NULL;

    if (!fullname || (!rdata && rdlen)) return kDNSServiceErr_BadParam;

    err = ConnectToServer(&tmp, flags, reconfirm_record_request, NULL, NULL, NULL);
    if (err) return err;

    len = sizeof(DNSServiceFlags);
    len += sizeof(uint32_t);
    len += strlen(fullname) + 1;
    len += 3 * sizeof(uint16_t);
    len += rdlen;
    hdr = create_hdr(reconfirm_record_request, &len, &ptr, 0, tmp);
    if (!hdr) { DNSServiceRefDeallocate(tmp); return kDNSServiceErr_NoMemory; }

    put_flags(flags, &ptr);
    put_uint32(interfaceIndex, &ptr);
    put_string(fullname, &ptr);
    put_uint16(rrtype, &ptr);
    put_uint16(rrclass, &ptr);
    put_uint16(rdlen, &ptr);
    put_rdata(rdlen, rdata, &ptr);

    err = deliver_request(hdr, tmp);        // Will free hdr for us
    DNSServiceRefDeallocate(tmp);
    return err;
}


static void handle_port_mapping_response(DNSServiceOp *const sdr, const CallbackHeader *const cbh, const uint8_t *data, const uint8_t *const end)
{
    union { uint32_t l; u_char b[4]; } addr;
    uint8_t protocol;
    union { uint16_t s; u_char b[2]; } internalPort;
    union { uint16_t s; u_char b[2]; } externalPort;
    uint32_t ttl;

    if (!data || data + 13 > end) goto fail;

    addr.b[0] = *data++;
    addr.b[1] = *data++;
    addr.b[2] = *data++;
    addr.b[3] = *data++;
    protocol          = *data++;
    internalPort.b[0] = *data++;
    internalPort.b[1] = *data++;
    externalPort.b[0] = *data++;
    externalPort.b[1] = *data++;
    ttl               = get_uint32(&data, end);
    if (!data) goto fail;

    ((DNSServiceNATPortMappingReply)sdr->AppCallback)(sdr, cbh->cb_flags, cbh->cb_interface, cbh->cb_err, addr.l, protocol, internalPort.s, externalPort.s, ttl, sdr->AppContext);
    return;
    // MUST NOT touch sdr after invoking AppCallback -- client is allowed to dispose it from within callback function

    fail :
    syslog(LOG_WARNING, "dnssd_clientstub handle_port_mapping_response: error reading result from daemon");
}

DNSServiceErrorType DNSSD_API DNSServiceNATPortMappingCreate
(
    DNSServiceRef                       *sdRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    uint32_t protocol,                                /* TCP and/or UDP */
    uint16_t internalPortInNetworkByteOrder,
    uint16_t externalPortInNetworkByteOrder,
    uint32_t ttl,                                     /* time to live in seconds */
    DNSServiceNATPortMappingReply callBack,
    void                                *context      /* may be NULL */
)
{
    uint8_t *ptr;
    size_t len;
    ipc_msg_hdr *hdr;
    union { uint16_t s; u_char b[2]; } internalPort = { internalPortInNetworkByteOrder };
    union { uint16_t s; u_char b[2]; } externalPort = { externalPortInNetworkByteOrder };

    DNSServiceErrorType err = ConnectToServer(sdRef, flags, port_mapping_request, handle_port_mapping_response, (void *)callBack, context);
    if (err) return err;    // On error ConnectToServer leaves *sdRef set to NULL

    len = sizeof(flags);
    len += sizeof(interfaceIndex);
    len += sizeof(protocol);
    len += sizeof(internalPort);
    len += sizeof(externalPort);
    len += sizeof(ttl);

    hdr = create_hdr(port_mapping_request, &len, &ptr, (*sdRef)->primary ? 1 : 0, *sdRef);
    if (!hdr) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; return kDNSServiceErr_NoMemory; }

    put_flags(flags, &ptr);
    put_uint32(interfaceIndex, &ptr);
    put_uint32(protocol, &ptr);
    *ptr++ = internalPort.b[0];
    *ptr++ = internalPort.b[1];
    *ptr++ = externalPort.b[0];
    *ptr++ = externalPort.b[1];
    put_uint32(ttl, &ptr);

    err = deliver_request(hdr, *sdRef);     // Will free hdr for us
    if (err) { DNSServiceRefDeallocate(*sdRef); *sdRef = NULL; }
    return err;
}

#if _DNS_SD_LIBDISPATCH
DNSServiceErrorType DNSSD_API DNSServiceSetDispatchQueue
(
    DNSServiceRef service,
    dispatch_queue_t queue
)
{
    int dnssd_fd  = DNSServiceRefSockFD(service);
    if (dnssd_fd == dnssd_InvalidSocket) return kDNSServiceErr_BadParam;
    if (!queue)
    {
        syslog(LOG_WARNING, "dnssd_clientstub: DNSServiceSetDispatchQueue dispatch queue NULL");
        return kDNSServiceErr_BadParam;
    }
    if (service->disp_queue)
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceSetDispatchQueue dispatch queue set already");
        return kDNSServiceErr_BadParam;
    }
    if (service->disp_source)
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceSetDispatchQueue dispatch source set already");
        return kDNSServiceErr_BadParam;
    }
    service->disp_source = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, dnssd_fd, 0, queue);
    if (!service->disp_source)
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceSetDispatchQueue dispatch_source_create failed");
        return kDNSServiceErr_NoMemory;
    }
    service->disp_queue = queue;
    dispatch_source_set_event_handler(service->disp_source, ^{DNSServiceProcessResult(service);});
    dispatch_source_set_cancel_handler(service->disp_source, ^{dnssd_close(dnssd_fd);});
    dispatch_resume(service->disp_source);
    return kDNSServiceErr_NoError;
}
#endif // _DNS_SD_LIBDISPATCH

#if !defined(_WIN32)

static void DNSSD_API SleepKeepaliveCallback(DNSServiceRef sdRef, DNSRecordRef rec, const DNSServiceFlags flags,
                                             DNSServiceErrorType errorCode, void *context)
{
    SleepKAContext *ka = (SleepKAContext *)context;
    (void)rec;      // Unused
    (void)flags;    // Unused

    if (sdRef->kacontext != context)
        syslog(LOG_WARNING, "dnssd_clientstub SleepKeepaliveCallback context mismatch");

    if (ka->AppCallback)
        ((DNSServiceSleepKeepaliveReply)ka->AppCallback)(sdRef, errorCode, ka->AppContext);
}

static DNSServiceErrorType _DNSServiceSleepKeepalive_sockaddr
(
    DNSServiceRef *                 sdRef,
    DNSServiceFlags                 flags,
    const struct sockaddr *         localAddr,
    const struct sockaddr *         remoteAddr,
    unsigned int                    timeout,
    DNSServiceSleepKeepaliveReply   callBack,
    void *                          context
);

DNSServiceErrorType DNSSD_API DNSServiceSleepKeepalive
(
    DNSServiceRef                       *sdRef,
    DNSServiceFlags flags,
    int fd,
    unsigned int timeout,
    DNSServiceSleepKeepaliveReply callBack,
    void                                *context
)
{
    struct sockaddr_storage lss;
    struct sockaddr_storage rss;
    socklen_t len1, len2;

    len1 = sizeof(lss);
    if (getsockname(fd, (struct sockaddr *)&lss, &len1) < 0)
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceSleepKeepalive: getsockname %d\n", errno);
        return kDNSServiceErr_BadParam;
    }

    len2 = sizeof(rss);
    if (getpeername(fd, (struct sockaddr *)&rss, &len2) < 0)
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceSleepKeepalive: getpeername %d\n", errno);
        return kDNSServiceErr_BadParam;
    }

    if (len1 != len2)
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceSleepKeepalive local/remote info not same");
        return kDNSServiceErr_Unknown;
    }
    return _DNSServiceSleepKeepalive_sockaddr(sdRef, flags, (const struct sockaddr *)&lss, (const struct sockaddr *)&rss,
        timeout, callBack, context);
}

DNSServiceErrorType DNSSD_API DNSServiceSleepKeepalive_sockaddr
(
    DNSServiceRef *                 sdRef,
    DNSServiceFlags                 flags,
    const struct sockaddr *         localAddr,
    const struct sockaddr *         remoteAddr,
    unsigned int                    timeout,
    DNSServiceSleepKeepaliveReply   callBack,
    void *                          context
)
{
    return _DNSServiceSleepKeepalive_sockaddr(sdRef, flags, localAddr, remoteAddr, timeout, callBack, context );
}

static DNSServiceErrorType _DNSServiceSleepKeepalive_sockaddr
(
    DNSServiceRef *                 sdRef,
    DNSServiceFlags                 flags,
    const struct sockaddr *         localAddr,
    const struct sockaddr *         remoteAddr,
    unsigned int                    timeout,
    DNSServiceSleepKeepaliveReply   callBack,
    void *                          context
)
{
    char source_str[INET6_ADDRSTRLEN];
    char target_str[INET6_ADDRSTRLEN];
    unsigned int len, proxyreclen;
    char buf[256];
    DNSServiceErrorType err;
    DNSRecordRef record = NULL;
    char name[10];
    char recname[128];
    SleepKAContext *ka;
    unsigned int i, unique;

    (void) flags; //unused
    if (!timeout) return kDNSServiceErr_BadParam;

    unique = 0;
    if ((localAddr->sa_family == AF_INET) && (remoteAddr->sa_family == AF_INET))
    {
        const struct sockaddr_in *sl = (const struct sockaddr_in *)localAddr;
        const struct sockaddr_in *sr = (const struct sockaddr_in *)remoteAddr;
        const unsigned char *ptr = (const unsigned char *)&sl->sin_addr;

        if (!inet_ntop(AF_INET, (const void *)&sr->sin_addr, target_str, sizeof (target_str)))
        {
            syslog(LOG_WARNING, "dnssd_clientstub DNSServiceSleepKeepalive remote info failed %d", errno);
            return kDNSServiceErr_Unknown;
        }
        if (!inet_ntop(AF_INET, (const void *)&sl->sin_addr, source_str, sizeof (source_str)))
        {
            syslog(LOG_WARNING, "dnssd_clientstub DNSServiceSleepKeepalive local info failed %d", errno);
            return kDNSServiceErr_Unknown;
        }
        // Sum of all bytes in the local address and port should result in a unique
        // number in the local network
        for (i = 0; i < sizeof(struct in_addr); i++)
            unique += ptr[i];
        unique += sl->sin_port;
        len = snprintf(buf+1, sizeof(buf) - 1, "t=%u h=%s d=%s l=%u r=%u", timeout, source_str, target_str, ntohs(sl->sin_port), ntohs(sr->sin_port));
    }
    else if ((localAddr->sa_family == AF_INET6) && (remoteAddr->sa_family == AF_INET6))
    {
        const struct sockaddr_in6 *sl6 = (const struct sockaddr_in6 *)localAddr;
        const struct sockaddr_in6 *sr6 = (const struct sockaddr_in6 *)remoteAddr;
        const unsigned char *ptr = (const unsigned char *)&sl6->sin6_addr;

        if (!inet_ntop(AF_INET6, (const void *)&sr6->sin6_addr, target_str, sizeof (target_str)))
        {
            syslog(LOG_WARNING, "dnssd_clientstub DNSServiceSleepKeepalive remote6 info failed %d", errno);
            return kDNSServiceErr_Unknown;
        }
        if (!inet_ntop(AF_INET6, (const void *)&sl6->sin6_addr, source_str, sizeof (source_str)))
        {
            syslog(LOG_WARNING, "dnssd_clientstub DNSServiceSleepKeepalive local6 info failed %d", errno);
            return kDNSServiceErr_Unknown;
        }
        for (i = 0; i < sizeof(struct in6_addr); i++)
            unique += ptr[i];
        unique += sl6->sin6_port;
        len = snprintf(buf+1, sizeof(buf) - 1, "t=%u H=%s D=%s l=%u r=%u", timeout, source_str, target_str, ntohs(sl6->sin6_port), ntohs(sr6->sin6_port));
    }
    else
    {
        return kDNSServiceErr_BadParam;
    }

    if (len >= (sizeof(buf) - 1))
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceSleepKeepalive could not fit local/remote info");
        return kDNSServiceErr_Unknown;
    }
    // Include the NULL byte also in the first byte. The total length of the record includes the
    // first byte also.
    buf[0] = len + 1;
    proxyreclen = len + 2;

    len = snprintf(name, sizeof(name), "%u", unique);
    if (len >= sizeof(name))
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceSleepKeepalive could not fit unique");
        return kDNSServiceErr_Unknown;
    }

    len = snprintf(recname, sizeof(recname), "%s.%s", name, "_keepalive._dns-sd._udp.local");
    if (len >= sizeof(recname))
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceSleepKeepalive could not fit name");
        return kDNSServiceErr_Unknown;
    }

    ka = mdns_malloc(sizeof(SleepKAContext));
    if (!ka) return kDNSServiceErr_NoMemory;
    ka->AppCallback = (DNSServiceSleepKeepaliveReply*)callBack;
    ka->AppContext = context;

    err = DNSServiceCreateConnection(sdRef);
    if (err)
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceSleepKeepalive cannot create connection");
        mdns_free(ka);
        return err;
    }

    // we don't care about the "record". When sdRef gets deallocated later, it will be freed too
    err = DNSServiceRegisterRecord(*sdRef, &record, kDNSServiceFlagsUnique, 0, recname,
                                   kDNSServiceType_NULL,  kDNSServiceClass_IN, proxyreclen, buf,  kDNSServiceInterfaceIndexAny, SleepKeepaliveCallback, ka);
    if (err)
    {
        syslog(LOG_WARNING, "dnssd_clientstub DNSServiceSleepKeepalive cannot create connection");
        mdns_free(ka);
        return err;
    }
    (*sdRef)->kacontext = ka;
    return kDNSServiceErr_NoError;
}
#endif
