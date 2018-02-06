/*	$NetBSD: nslcd.h,v 1.1.1.4 2018/02/06 01:53:06 christos Exp $	*/

/*
   nslcd.h - file describing client/server protocol

   Copyright (C) 2006 West Consulting
   Copyright (C) 2006, 2007, 2009, 2010, 2011, 2012, 2013 Arthur de Jong

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
     INT32  NSLCD_VERSION
     INT32  NSLCD_ACTION_*
     [request parameters if any]
   A response looks like:
     INT32  NSLCD_VERSION
     INT32  NSLCD_ACTION_* (the original request type)
     [result(s)]
     INT32  NSLCD_RESULT_END
   A single result entry looks like:
     INT32  NSLCD_RESULT_BEGIN
     [result value(s)]
   If a response would return multiple values (e.g. for NSLCD_ACTION_*_ALL
   functions) each return value will be preceded by a NSLCD_RESULT_BEGIN
   value. After the last returned result the server sends
   NSLCD_RESULT_END. If some error occurs (e.g. LDAP server unavailable,
   error in the request, etc) the server terminates the connection to signal
   an error condition (breaking the protocol).

   These are the available basic data types:
     INT32  - 32-bit integer value
     TYPE   - a typed field that is transferred using sizeof()
     STRING - a string length (32bit) followed by the string value (not
              null-terminted) the string itself is assumed to be UTF-8
     STRINGLIST - a 32-bit number noting the number of strings followed by
                  the strings one at a time

   Furthermore the ADDRESS compound data type is defined as:
     INT32  type of address: e.g. AF_INET or AF_INET6
     INT32  lenght of address
     RAW    the address itself
   With the ADDRESSLIST using the same construct as with STRINGLIST.

   The protocol uses network byte order for all types.
*/

/* The current version of the protocol. This protocol should only be
   updated with major backwards-incompatible changes. */
#define NSLCD_VERSION 0x00000002

/* Get a NSLCD configuration option. There is one request parameter:
    INT32   NSLCD_CONFIG_*
  the result value is:
    STRING  value, interpretation depending on request */
#define NSLCD_ACTION_CONFIG_GET        0x00010001

/* return the message, if any, that is presented to the user when password
   modification through PAM is prohibited */
#define NSLCD_CONFIG_PAM_PASSWORD_PROHIBIT_MESSAGE 1

/* Email alias (/etc/aliases) NSS requests. The result values for a
   single entry are:
     STRING      alias name
     STRINGLIST  alias rcpts */
#define NSLCD_ACTION_ALIAS_BYNAME      0x00020001
#define NSLCD_ACTION_ALIAS_ALL         0x00020008

/* Ethernet address/name mapping NSS requests. The result values for a
   single entry are:
     STRING            ether name
     TYPE(uint8_t[6])  ether address */
#define NSLCD_ACTION_ETHER_BYNAME      0x00030001
#define NSLCD_ACTION_ETHER_BYETHER     0x00030002
#define NSLCD_ACTION_ETHER_ALL         0x00030008

/* Group and group membership related NSS requests. The result values
   for a single entry are:
     STRING       group name
     STRING       group password
     INT32        group id
     STRINGLIST   members (usernames) of the group
     (not that the BYMEMER call returns an empty members list) */
#define NSLCD_ACTION_GROUP_BYNAME      0x00040001
#define NSLCD_ACTION_GROUP_BYGID       0x00040002
#define NSLCD_ACTION_GROUP_BYMEMBER    0x00040006
#define NSLCD_ACTION_GROUP_ALL         0x00040008

/* Hostname (/etc/hosts) lookup NSS requests. The result values
   for an entry are:
     STRING       host name
     STRINGLIST   host aliases
     ADDRESSLIST  host addresses */
#define NSLCD_ACTION_HOST_BYNAME       0x00050001
#define NSLCD_ACTION_HOST_BYADDR       0x00050002
#define NSLCD_ACTION_HOST_ALL          0x00050008

/* Netgroup NSS result entries contain a number of parts. A result entry
   starts with:
     STRING  netgroup name
   followed by zero or more references to other netgroups or netgroup
   triples. A reference to another netgroup looks like:
     INT32   NSLCD_NETGROUP_TYPE_NETGROUP
     STRING  other netgroup name
   A a netgroup triple looks like:
     INT32   NSLCD_NETGROUP_TYPE_TRIPLE
     STRING  host
     STRING  user
     STRING  domain
   A netgroup result entry is terminated by:
     INT32   NSLCD_NETGROUP_TYPE_END
   */
#define NSLCD_ACTION_NETGROUP_BYNAME   0x00060001
#define NSLCD_ACTION_NETGROUP_ALL      0x00060008
#define NSLCD_NETGROUP_TYPE_NETGROUP 1
#define NSLCD_NETGROUP_TYPE_TRIPLE   2
#define NSLCD_NETGROUP_TYPE_END      3

/* Network name (/etc/networks) NSS requests. Result values for a single
   entry are:
     STRING       network name
     STRINGLIST   network aliases
     ADDRESSLIST  network addresses */
#define NSLCD_ACTION_NETWORK_BYNAME    0x00070001
#define NSLCD_ACTION_NETWORK_BYADDR    0x00070002
#define NSLCD_ACTION_NETWORK_ALL       0x00070008

/* User account (/etc/passwd) NSS requests. Result values are:
     STRING       user name
     STRING       user password
     INT32        user id
     INT32        group id
     STRING       gecos information
     STRING       home directory
     STRING       login shell */
#define NSLCD_ACTION_PASSWD_BYNAME     0x00080001
#define NSLCD_ACTION_PASSWD_BYUID      0x00080002
#define NSLCD_ACTION_PASSWD_ALL        0x00080008

/* Protocol information requests. Result values are:
     STRING      protocol name
     STRINGLIST  protocol aliases
     INT32       protocol number */
#define NSLCD_ACTION_PROTOCOL_BYNAME   0x00090001
#define NSLCD_ACTION_PROTOCOL_BYNUMBER 0x00090002
#define NSLCD_ACTION_PROTOCOL_ALL      0x00090008

/* RPC information requests. Result values are:
     STRING      rpc name
     STRINGLIST  rpc aliases
     INT32       rpc number */
#define NSLCD_ACTION_RPC_BYNAME        0x000a0001
#define NSLCD_ACTION_RPC_BYNUMBER      0x000a0002
#define NSLCD_ACTION_RPC_ALL           0x000a0008

/* Service (/etc/services) information requests. The BYNAME and BYNUMBER
   requests contain an extra protocol string in the request which, if not
   blank, will filter the services by this protocol. Result values are:
     STRING      service name
     STRINGLIST  service aliases
     INT32       service (port) number
     STRING      service protocol */
#define NSLCD_ACTION_SERVICE_BYNAME    0x000b0001
#define NSLCD_ACTION_SERVICE_BYNUMBER  0x000b0002
#define NSLCD_ACTION_SERVICE_ALL       0x000b0008

/* Extended user account (/etc/shadow) information requests. Result
   values for a single entry are:
     STRING  user name
     STRING  user password
     INT32   last password change
     INT32   mindays
     INT32   maxdays
     INT32   warn
     INT32   inact
     INT32   expire
     INT32   flag */
#define NSLCD_ACTION_SHADOW_BYNAME     0x000c0001
#define NSLCD_ACTION_SHADOW_ALL        0x000c0008

/* PAM-related requests. The request parameters for all these requests
   begin with:
     STRING  user name
     STRING  service name
     STRING  ruser
     STRING  rhost
     STRING  tty
   If the user is not known in LDAP no result may be returned (immediately
   return NSLCD_RESULT_END instead of a PAM error code). */

/* PAM authentication check request. The extra request values are:
     STRING  password
   and the result value consists of:
     INT32   authc NSLCD_PAM_* result code
     STRING  user name (the cannonical user name)
     INT32   authz NSLCD_PAM_* result code
     STRING  authorisation error message
   If the username is empty in this request an attempt is made to
   authenticate as the administrator (set using rootpwmoddn).
   Some authorisation checks are already done during authentication so the
   response also includes authorisation information. */
#define NSLCD_ACTION_PAM_AUTHC         0x000d0001

/* PAM authorisation check request. The result value consists of:
     INT32   authz NSLCD_PAM_* result code
     STRING  authorisation error message
   The authentication check may have already returned some authorisation
   information. The authorisation error message, if supplied, will be used
   by the PAM module instead of a message that is generated by the PAM
   module itself. */
#define NSLCD_ACTION_PAM_AUTHZ         0x000d0002

/* PAM session open request. The result value consists of:
     STRING   session id
   This session id may be used to close this session with. */
#define NSLCD_ACTION_PAM_SESS_O        0x000d0003

/* PAM session close request. This request has the following
   extra request value:
     STRING   session id
   and this calls only returns an empty response value. */
#define NSLCD_ACTION_PAM_SESS_C        0x000d0004

/* PAM password modification request. This requests has the following extra
   request values:
     INT32   asroot: 0=oldpasswd is user passwd, 1=oldpasswd is root passwd
     STRING  old password
     STRING  new password
   and returns there extra result values:
     INT32   NSLCD_PAM_* result code
     STRING  error message */
#define NSLCD_ACTION_PAM_PWMOD         0x000d0005

/* User information change request. This request allows one to change
   their full name and other information. The request parameters for this
   request are:
     STRING  user name
     INT32   asroot: 0=passwd is user passwd, 1=passwd is root passwd
     STRING  password
   followed by one or more of the below, terminated by NSLCD_USERMOD_END
     INT32   NSLCD_USERMOD_*
     STRING  new value
   the response consists of one or more of the entries below, terminated
   by NSLCD_USERMOD_END:
     INT32   NSLCD_USERMOD_*
     STRING  response
   (if the response is blank, the change went OK, otherwise the string
   contains an error message)
   */
#define NSLCD_ACTION_USERMOD           0x000e0001

/* These are the possible values for the NSLCD_ACTION_USERMOD operation
   above. */
#define NSLCD_USERMOD_END        0 /* end of change values */
#define NSLCD_USERMOD_RESULT     1 /* global result value */
#define NSLCD_USERMOD_FULLNAME   2 /* full name */
#define NSLCD_USERMOD_ROOMNUMBER 3 /* room number */
#define NSLCD_USERMOD_WORKPHONE  4 /* office phone number */
#define NSLCD_USERMOD_HOMEPHONE  5 /* home phone number */
#define NSLCD_USERMOD_OTHER      6 /* other info */
#define NSLCD_USERMOD_HOMEDIR    7 /* home directory */
#define NSLCD_USERMOD_SHELL      8 /* login shell */

/* Request result codes. */
#define NSLCD_RESULT_BEGIN 1
#define NSLCD_RESULT_END   2

/* Partial list of PAM result codes. */
#define NSLCD_PAM_SUCCESS             0 /* everything ok */
#define NSLCD_PAM_PERM_DENIED         6 /* Permission denied */
#define NSLCD_PAM_AUTH_ERR            7 /* Authc failure */
#define NSLCD_PAM_CRED_INSUFFICIENT   8 /* Cannot access authc data */
#define NSLCD_PAM_AUTHINFO_UNAVAIL    9 /* Cannot retrieve authc info */
#define NSLCD_PAM_USER_UNKNOWN       10 /* User not known */
#define NSLCD_PAM_MAXTRIES           11 /* Retry limit reached */
#define NSLCD_PAM_NEW_AUTHTOK_REQD   12 /* Password expired */
#define NSLCD_PAM_ACCT_EXPIRED       13 /* Account expired */
#define NSLCD_PAM_SESSION_ERR        14 /* Cannot make/remove session record */
#define NSLCD_PAM_AUTHTOK_ERR        20 /* Authentication token manipulation error */
#define NSLCD_PAM_AUTHTOK_DISABLE_AGING 23 /* Password aging disabled */
#define NSLCD_PAM_IGNORE             25 /* Ignore module */
#define NSLCD_PAM_ABORT              26 /* Fatal error */
#define NSLCD_PAM_AUTHTOK_EXPIRED    27 /* authentication token has expired */

#endif /* not _NSLCD_H */
