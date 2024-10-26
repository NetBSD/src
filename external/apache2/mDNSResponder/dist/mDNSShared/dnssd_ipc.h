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

#ifndef DNSSD_IPC_H
#define DNSSD_IPC_H

#include "dns_sd.h"
#include "general.h"

//
// Common cross platform services
//
#if defined(WIN32)
#   include <winsock2.h>
#   define dnssd_InvalidSocket  INVALID_SOCKET
#   define dnssd_SocketValid(s) ((s) != INVALID_SOCKET)
#   define dnssd_EWOULDBLOCK    WSAEWOULDBLOCK
#   define dnssd_EINTR          WSAEINTR
#   define dnssd_ECONNRESET     WSAECONNRESET
#   define dnssd_socklen_t      int
#   define dnssd_close(sock)    closesocket(sock)
#   define dnssd_errno          WSAGetLastError()
#   define dnssd_strerror(X)    win32_strerror(X)
#   define ssize_t              int
#   define getpid               _getpid
#   define unlink               _unlink
extern char *win32_strerror(int inErrorCode);
#else
#   include <sys/types.h>
#   include <unistd.h>
#   include <sys/un.h>
#   include <string.h>
#   include <stdio.h>
#   include <stdlib.h>
#   include <sys/stat.h>
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#   define dnssd_InvalidSocket  -1
#   define dnssd_SocketValid(s) ((s) >= 0)
#   define dnssd_EWOULDBLOCK    EWOULDBLOCK
#   define dnssd_EINTR          EINTR
#   define dnssd_ECONNRESET     ECONNRESET
#   define dnssd_EPIPE          EPIPE
#   define dnssd_socklen_t      unsigned int
#   define dnssd_close(sock)    close(sock)
#   define dnssd_errno          errno
#   define dnssd_strerror(X)    strerror(X)
#endif

#if defined(USE_TCP_LOOPBACK)
#   define AF_DNSSD             AF_INET
#   define MDNS_TCP_SERVERADDR  "127.0.0.1"
#ifdef WIN32_CENTENNIAL
#   define MDNS_TCP_SERVERPORT_CENTENNIAL  53545
#endif
#   define MDNS_TCP_SERVERPORT  5354
#   define LISTENQ              5
#   define dnssd_sockaddr_t     struct sockaddr_in
#else
#   define AF_DNSSD             AF_LOCAL
#   ifndef MDNS_UDS_SERVERPATH
#       define MDNS_UDS_SERVERPATH  "/var/run/mDNSResponder"
#   endif
#   define MDNS_UDS_SERVERPATH_ENVVAR "DNSSD_UDS_PATH"
#   define LISTENQ              100
// longest legal control path length
#   define MAX_CTLPATH          (sizeof(((struct sockaddr_un*)0)->sun_path))
#   define dnssd_sockaddr_t     struct sockaddr_un
#endif

// Compatibility workaround
#ifndef AF_LOCAL
#define AF_LOCAL    AF_UNIX
#endif

// General UDS constants
#define TXT_RECORD_INDEX ((uint32_t)(-1))   // record index for default text record

// IPC data encoding constants and types
#define VERSION 1
#define IPC_FLAGS_NOREPLY       (1U << 0) // Set flag if no asynchronous replies are to be sent to client.
#define IPC_FLAGS_TRAILING_TLVS (1U << 1) // Set flag if TLVs follow the standard request data.
#define IPC_FLAGS_NOERRSD       (1U << 2) // Set flag if flag kDNSServiceFlagsMoreComing is set on client side.

#define IPC_TLV_TYPE_RESOLVER_CONFIG_PLIST_DATA     1 // An nw_resolver_config as a binary property list.
#define IPC_TLV_TYPE_REQUIRE_PRIVACY                2 // A uint8. Non-zero means privacy required, zero means not required.
#define IPC_TLV_TYPE_SERVICE_ATTR_AAAA_POLICY       3 // A uint32 for a DNSServiceAAAAPolicy value.
#define IPC_TLV_TYPE_SERVICE_ATTR_FAILOVER_POLICY   4 // A uint32 for a DNSServiceFailoverPolicy value.
#define IPC_TLV_TYPE_SERVICE_ATTR_TIMESTAMP         5 // A uint32 value for the time, in seconds, since Jan 1st 1970 UTC.
#define IPC_TLV_TYPE_SERVICE_ATTR_VALIDATION_POLICY 6 // A uint32 for a DNSServiceValidationPolicy value.
#define IPC_TLV_TYPE_SERVICE_ATTR_VALIDATION_DATA   7 // A ptr for the validation data.
#define IPC_TLV_TYPE_GET_TRACKER_STR                8 // A uint8. If non-zero, include tracker domain if applicable.
#define IPC_TLV_TYPE_SERVICE_ATTR_TRACKER_STR       9 // A null-terminated string. The domain (original hostname or resolved CNAME)
                                                      // that was identified as a tracker
#define IPC_TLV_TYPE_RESOLVER_OVERRIDE             10 // UUID of resolver configuration to use as an override. [1]
#define IPC_TLV_TYPE_SERVICE_ATTR_HOST_KEY_HASH    11 // A uint32 value for a host key hash.

// Notes:
// 1. If this TLV is specified, then the UUID identifies the libnetwork resolver configuration to use for the DNS-SD
//    API operation (currently limited to DNSServiceQueryRecordWithAttribute()). This overrides the normal DNS service
//    select process.

// Structure packing macro. If we're not using GNUC, it's not fatal. Most compilers naturally pack the on-the-wire
// structures correctly anyway, so a plain "struct" is usually fine. In the event that structures are not packed
// correctly, our compile-time assertion checks will catch it and prevent inadvertent generation of non-working code.
#ifndef packedstruct
 #if ((__GNUC__ > 2) || ((__GNUC__ == 2) && (__GNUC_MINOR__ >= 9)))
  #define packedstruct struct __attribute__((__packed__))
  #define packedunion  union  __attribute__((__packed__))
 #else
  #define packedstruct struct
  #define packedunion  union
 #endif
#endif

typedef enum
{
    request_op_none = 0,    // No request yet received on this connection
    connection_request = 1, // connected socket via DNSServiceConnect()
    reg_record_request,     // reg/remove record only valid for connected sockets
    remove_record_request,
    enumeration_request,
    reg_service_request,
    browse_request,
    resolve_request,
    query_request,
    reconfirm_record_request,
    add_record_request,
    update_record_request,
    setdomain_request,      // Up to here is in Tiger and B4W 1.0.3
    getproperty_request,    // New in B4W 1.0.4
    port_mapping_request,   // New in Leopard and B4W 2.0
    addrinfo_request,
    send_bpf_OBSOLETE,      // New in SL (obsolete in 2023)
    getpid_request,
    release_request,
    connection_delegate_request,

    cancel_request = 63
} request_op_t;

typedef enum
{
    enumeration_reply_op = 64,
    reg_service_reply_op,
    browse_reply_op,
    resolve_reply_op,
    query_reply_op,
    reg_record_reply_op,    // Up to here is in Tiger and B4W 1.0.3
    getproperty_reply_op,   // New in B4W 1.0.4
    port_mapping_reply_op,  // New in Leopard and B4W 2.0
    addrinfo_reply_op,
    async_error_op
} reply_op_t;

#if defined(_WIN64)
#   pragma pack(push,4)
#endif

// Define context object big enough to hold a 64-bit pointer,
// to accomodate 64-bit clients communicating with 32-bit daemon.
// There's no reason for the daemon to ever be a 64-bit process, but its clients might be
typedef packedunion
{
    void *context;
    uint32_t u32[2];
} client_context_t;

typedef packedstruct
{
    uint32_t version;
    uint32_t datalen;
    uint32_t ipc_flags;
    uint32_t op;        // request_op_t or reply_op_t
    client_context_t client_context; // context passed from client, returned by server in corresponding reply
    uint32_t reg_index;            // identifier for a record registered via DNSServiceRegisterRecord() on a
    // socket connected by DNSServiceCreateConnection().  Must be unique in the scope of the connection, such that and
    // index/socket pair uniquely identifies a record.  (Used to select records for removal by DNSServiceRemoveRecord())
} ipc_msg_hdr;

#if defined(_WIN64)
#   pragma pack(pop)
#endif

// routines to write to and extract data from message buffers.
// caller responsible for bounds checking.
// ptr is the address of the pointer to the start of the field.
// it is advanced to point to the next field, or the end of the message

void put_uint32(const uint32_t l, uint8_t **ptr);
uint32_t get_uint32(const uint8_t **ptr, const uint8_t *end);

void put_uint16(uint16_t s, uint8_t **ptr);
uint16_t get_uint16(const uint8_t **ptr, const uint8_t *end);

#define put_flags put_uint32
#define get_flags get_uint32

#define put_error_code put_uint32
#define get_error_code get_uint32

int put_string(const char *str, uint8_t **ptr);
int get_string(const uint8_t **ptr, const uint8_t *end, char *buffer, size_t buflen);

void put_rdata(const size_t rdlen, const uint8_t *rdata, uint8_t **ptr);
const uint8_t *get_rdata(const uint8_t **ptr, const uint8_t *end, int rdlen);  // return value is rdata pointed to by *ptr -
// rdata is not copied from buffer.

size_t get_required_tlv_length(uint16_t value_length);
size_t get_required_tlv_string_length(const char *str_value);
size_t get_required_tlv_uint8_length(void);
size_t get_required_tlv_uint32_length(void);
size_t put_tlv(uint16_t type, uint16_t length, const uint8_t *value, uint8_t **ptr, const uint8_t *limit);
void put_tlv_string(const uint16_t type, const char *const str_value, uint8_t **const ptr, const uint8_t *const limit,
    int *const out_error);
void put_tlv_uint8(uint16_t type, uint8_t u8, uint8_t **ptr, const uint8_t *limit);
void put_tlv_uint16(uint16_t type, uint16_t u16, uint8_t **ptr, const uint8_t *limit);
size_t put_tlv_uint32(uint16_t type, uint32_t u32, uint8_t **ptr, const uint8_t *limit);
size_t put_tlv_uuid(uint16_t type, const uint8_t uuid[MDNS_STATIC_ARRAY_PARAM MDNS_UUID_SIZE], uint8_t **ptr,
    const uint8_t *limit);
const uint8_t *get_tlv(const uint8_t *src, const uint8_t *end, uint16_t type, size_t *out_length);
const char *get_tlv_string(const uint8_t *const start, const uint8_t *const end, const uint16_t type);
uint32_t get_tlv_uint32(const uint8_t *src, const uint8_t *end, uint16_t type, int *out_error);
const uint8_t *get_tlv_uuid(const uint8_t *src, const uint8_t *end, uint16_t type);

void ConvertHeaderBytes(ipc_msg_hdr *hdr);

struct CompileTimeAssertionChecks_dnssd_ipc
{
    // Check that the compiler generated our on-the-wire packet format structure definitions
    // properly packed, without adding padding bytes to align fields on 32-bit or 64-bit boundaries.
#ifndef __lint__
    char assert0[(sizeof(client_context_t) ==  8) ? 1 : -1];
    char assert1[(sizeof(ipc_msg_hdr)      == 28) ? 1 : -1];
#endif
};

#endif // DNSSD_IPC_H
