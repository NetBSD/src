/*
   nslcd.h - file describing client/server protocol

   Copyright (C) 2006 West Consulting
   Copyright (C) 2006, 2007 Arthur de Jong

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA
*/

#ifndef _NSLCD_H
#define _NSLCD_H 1

/*
   The protocol used between the nslcd client and server is a simple binary
   protocol. It is request/response based where the client initiates a
   connection, does a single request and closes the connection again. Any
   mangled or not understood messages will be silently ignored by the server.

   A request looks like:
     int32 NSLCD_VERSION
     int32 NSLCD_ACTION_*
     [request parameters if any]
   A response looks like:
     int32 NSLCD_VERSION
     int32 NSLCD_ACTION_* (the original request type)
     [result(s)]
     NSLCD_RESULT_END
   A result looks like:
     int32 NSLCD_RESULT_SUCCESS
     [result value(s)]
   If a response would return multiple values (e.g. for NSLCD_ACTION_*_ALL
   functions) each return value will be preceded by a NSLCD_RESULT_SUCCESS
   value. After the last returned result the server sends
   NSLCD_RESULT_END. If some error occurs the server terminates the
   connection to signal an error condition (breaking the protocol).

   These are the available data types:
     INT32  - 32-bit integer value
     TYPE   - a typed field that is transferred using sizeof()
     STRING - a string length (32bit) followed by the string value (not
              null-terminted) the string itself is assumed to be UTF-8
     STRINGLIST - a 32-bit number noting the number of strings followed by
                  the strings one at a time

   Compound datatypes (such as PASSWD) are defined below as a combination of
   the above types. They are defined as macros so they can be expanded to
   code later on.

   The protocol uses host-byte order for all types (except where the normal
   value in-memory is already in network-byte order like with some
   addresses). This simple protocol makes it easy to support diffenrent NSS
   implementations.
*/

/* used for transferring alias information */
#define NSLCD_ALIAS \
  NSLCD_STRING(ALIAS_NAME) \
  NSLCD_STRINGLIST(ALIAS_RCPTS)

/* used for transferring mac addresses */
#define NSLCD_ETHER \
  NSLCD_STRING(ETHER_NAME) \
  NSLCD_TYPE(ETHER_ADDR,uint8_t[6])

/* used for transferring group and membership information */
#define NSLCD_GROUP \
  NSLCD_STRING(GROUP_NAME) \
  NSLCD_STRING(GROUP_PASSWD) \
  NSLCD_TYPE(GROUP_GID,gid_t) \
  NSLCD_STRINGLIST(GROUP_MEMBERS)

/* used for storing address information for the host database */
/* Note: this marcos is not expanded to code, check manually */
#define NSLCD_ADDRESS \
  NSLCD_INT32(ADDRESS_TYPE) /* type of address: e.g. AF_INET or AF_INET6 */ \
  NSLCD_INT32(ADDRESS_LEN)  /* length of the address to follow */ \
  NSLCD_BUF(ADDRESS_ADDR)   /* the address itself in network byte order */

/* used for transferring host (/etc/hosts) information */
/* Note: this marco is not expanded to code, check manually */
#define NSLCD_HOST \
  NSLCD_STRING(HOST_NAME) \
  NSLCD_STRINGLIST(HOST_ALIASES) \
  NSLCD_ADDRESSLIST(HOST_ADDRS)

/* used for transferring netgroup entries one at a time */
/* Note: this marcos is not expanded to code, check manually */
/* netgroup messages are split into two parts, first a part
   determining the type */
#define NETGROUP_TYPE_NETGROUP 123
#define NETGROUP_TYPE_TRIPLE   456
#define NSLCD_NETGROUP_TYPE \
  NSLCD_INT32(NETGROUP_TYPE) /* one of the above values */
/* followed by one of these message parts */
#define NSLCD_NETGROUP_NETGROUP \
  NSLCD_STRING(NETGROUP_NETGROUP)
#define NSLCD_NETGROUP_TRIPLE \
  NSLCD_STRING(NETGROUP_HOST) \
  NSLCD_STRING(NETGROUP_USER) \
  NSLCD_STRING(NETGROUP_DOMAIN)

/* user for transferring network (/etc/networks) information */
/* Note: this marco is not expanded to code, check manually */
#define NSLCD_NETWORK \
  NSLCD_STRING(NETWORK_NAME) \
  NSLCD_STRINGLIST(NETWORK_ALIASES) \
  NSLCD_ADDRESSLIST(NETWORK_ADDRS)

/* used for transferring user (/etc/passwd) information */
#define NSLCD_PASSWD \
  NSLCD_STRING(PASSWD_NAME) \
  NSLCD_STRING(PASSWD_PASSWD) \
  NSLCD_TYPE(PASSWD_UID,uid_t) \
  NSLCD_TYPE(PASSWD_GID,gid_t) \
  NSLCD_STRING(PASSWD_GECOS) \
  NSLCD_STRING(PASSWD_DIR) \
  NSLCD_STRING(PASSWD_SHELL)

/* used for transferring protocol information */
#define NSLCD_PROTOCOL \
  NSLCD_STRING(PROTOCOL_NAME) \
  NSLCD_STRINGLIST(PROTOCOL_ALIASES) \
  NSLCD_INT32(PROTOCOL_NUMBER)

/* for transferring struct rpcent structs */
#define NSLCD_RPC \
  NSLCD_STRING(RPC_NAME) \
  NSLCD_STRINGLIST(RPC_ALIASES) \
  NSLCD_INT32(RPC_NUMBER)

/* for transferring struct servent information */
#define NSLCD_SERVICE \
  NSLCD_STRING(SERVICE_NAME) \
  NSLCD_STRINGLIST(SERVICE_ALIASES) \
  NSLCD_INT32(SERVICE_NUMBER) \
  NSLCD_STRING(SERVICE_PROTOCOL)

/* used for transferring account (/etc/shadow) information */
#define NSLCD_SHADOW \
  NSLCD_STRING(SHADOW_NAME) \
  NSLCD_STRING(SHADOW_PASSWD) \
  NSLCD_INT32(SHADOW_LASTCHANGE) \
  NSLCD_INT32(SHADOW_MINDAYS) \
  NSLCD_INT32(SHADOW_MAXDAYS) \
  NSLCD_INT32(SHADOW_WARN) \
  NSLCD_INT32(SHADOW_INACT) \
  NSLCD_INT32(SHADOW_EXPIRE) \
  NSLCD_INT32(SHADOW_FLAG)

/* The current version of the protocol. Note that version 1
   is experimental and this version will be used until a
   1.0 release of nss-ldapd is made. */
#define NSLCD_VERSION 1

/* Request types. */
#define NSLCD_ACTION_ALIAS_BYNAME       4001
#define NSLCD_ACTION_ALIAS_ALL          4002
#define NSLCD_ACTION_ETHER_BYNAME       3001
#define NSLCD_ACTION_ETHER_BYETHER      3002
#define NSLCD_ACTION_ETHER_ALL          3005
#define NSLCD_ACTION_GROUP_BYNAME       5001
#define NSLCD_ACTION_GROUP_BYGID        5002
#define NSLCD_ACTION_GROUP_BYMEMBER     5003
#define NSLCD_ACTION_GROUP_ALL          5004
#define NSLCD_ACTION_HOST_BYNAME        6001
#define NSLCD_ACTION_HOST_BYADDR        6002
#define NSLCD_ACTION_HOST_ALL           6005
#define NSLCD_ACTION_NETGROUP_BYNAME   12001
#define NSLCD_ACTION_NETWORK_BYNAME     8001
#define NSLCD_ACTION_NETWORK_BYADDR     8002
#define NSLCD_ACTION_NETWORK_ALL        8005
#define NSLCD_ACTION_PASSWD_BYNAME      1001
#define NSLCD_ACTION_PASSWD_BYUID       1002
#define NSLCD_ACTION_PASSWD_ALL         1004
#define NSLCD_ACTION_PROTOCOL_BYNAME    9001
#define NSLCD_ACTION_PROTOCOL_BYNUMBER  9002
#define NSLCD_ACTION_PROTOCOL_ALL       9003
#define NSLCD_ACTION_RPC_BYNAME        10001
#define NSLCD_ACTION_RPC_BYNUMBER      10002
#define NSLCD_ACTION_RPC_ALL           10003
#define NSLCD_ACTION_SERVICE_BYNAME    11001
#define NSLCD_ACTION_SERVICE_BYNUMBER  11002
#define NSLCD_ACTION_SERVICE_ALL       11005
#define NSLCD_ACTION_SHADOW_BYNAME      2001
#define NSLCD_ACTION_SHADOW_ALL         2005

/* Request result codes. */
#define NSLCD_RESULT_END              3 /* key was not found */
#define NSLCD_RESULT_SUCCESS               0 /* everything ok */

#endif /* not _NSLCD_H */
