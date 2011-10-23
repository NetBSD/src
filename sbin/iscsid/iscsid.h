/*	$NetBSD: iscsid.h,v 1.1 2011/10/23 21:11:23 agc Exp $	*/

/*-
 * Copyright (c) 2004,2006,2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _ISCSID_H_
#define _ISCSID_H_

#include <dev/iscsi/iscsi.h>

#ifndef __BEGIN_DECLS
#  if defined(__cplusplus)
#  define __BEGIN_DECLS           extern "C" {
#  define __END_DECLS             }
#  else
#  define __BEGIN_DECLS
#  define __END_DECLS
#  endif
#endif

__BEGIN_DECLS

/* The socket name */

#define ISCSID_SOCK_NAME   "/tmp/iscsid_socket"


/* ==== Requests ==== */

#define ISCSID_ADD_TARGET                 1
#define ISCSID_ADD_PORTAL                 2
#define ISCSID_SET_TARGET_OPTIONS         3
#define ISCSID_GET_TARGET_OPTIONS         4
#define ISCSID_SET_TARGET_AUTHENTICATION  5
#define ISCSID_SLP_FIND_TARGETS           6
#define ISCSID_REFRESH_TARGETS            7
#define ISCSID_REMOVE_TARGET              8
#define ISCSID_SEARCH_LIST                9
#define ISCSID_GET_LIST                   10
#define ISCSID_GET_TARGET_INFO            11
#define ISCSID_GET_PORTAL_INFO            12
#define ISCSID_ADD_ISNS_SERVER            13
#define ISCSID_GET_ISNS_SERVER            14
#define ISCSID_SLP_FIND_ISNS_SERVERS      15
#define ISCSID_REMOVE_ISNS_SERVER         17
#define ISCSID_ADD_INITIATOR_PORTAL       18
#define ISCSID_GET_INITIATOR_PORTAL       19
#define ISCSID_REMOVE_INITIATOR_PORTAL    20
#define ISCSID_LOGIN                      21
#define ISCSID_ADD_CONNECTION             22
#define ISCSID_LOGOUT                     23
#define ISCSID_REMOVE_CONNECTION          24
#define ISCSID_GET_SESSION_LIST           25
#define ISCSID_GET_CONNECTION_LIST        26
#define ISCSID_GET_CONNECTION_INFO        27
#define ISCSID_SET_NODE_NAME              28

#define ISCSID_GET_VERSION				  100

#define ISCSID_DAEMON_TEST                900
#define ISCSID_DAEMON_TERMINATE           999

/* ==== List kind used in some requests ==== */

typedef enum {
	TARGET_LIST,			/* list of targets */
	PORTAL_LIST,			/* list of target portals */
	SEND_TARGETS_LIST,		/* list of send targets portals */
	ISNS_LIST,			/* list of isns servers */
	SESSION_LIST,			/* list of sessions */
	INITIATOR_LIST,			/* list of initiator portals */
	NUM_DAEMON_LISTS		/* Number of lists the daemon keeps */
} iscsid_list_kind_t;

/* ==== Search kind for search_list request ==== */

typedef enum {
	FIND_ID,		/* search for numeric ID */
	FIND_NAME,		/* search for symbolic name */
	FIND_TARGET_NAME,	/* search for target or initiator name */
	FIND_ADDRESS		/* search for target or server address */
} iscsid_search_kind_t;

/* ==== Symbolic or numeric ID ==== */

typedef struct {
	int	id;
	uint8_t	name[ISCSI_STRING_LENGTH];
} iscsid_sym_id_t;

/*
   id
      Numeric ID.
   name
      Symbolic ID. Ignored if numeric ID is nonzero.
*/

/* ==== Symbolic/Numeric ID with list kind ==== */

typedef struct {
	iscsid_list_kind_t	list_kind;
	iscsid_sym_id_t		id;
} iscsid_list_id_t;

/*
   list_kind
      Which list (generally TARGET_LIST or SEND_TARGETS_LIST)
   id
      numeric/symbolic ID
*/


typedef struct {
	struct {
		int	HeaderDigest:1;
		int	DataDigest:1;
		int	MaxRecvDataSegmentLength:1;
	} is_present;
	iscsi_digest_t	HeaderDigest;
	iscsi_digest_t	DataDigest;
	uint32_t	MaxRecvDataSegmentLength;
} iscsid_portal_options_t;

/*
   is_present
      Contains a bitfield that indicates which members of the structure
      contain valid data.
   HeaderDigest
      Indicates the digest to use for PDU headers.
   DataDigest
      Indicates the digest to use for PDU data.
   MaxRecvDataSegmentLength
      Allows limiting or extending the maximum receive data segment length.
      Must contain a value between 512 and 2**24-1 if specified.
*/


/* ==== General request structure ==== */

typedef struct {
	uint32_t	request;
	uint32_t	parameter_length;
	uint8_t		parameter[0];
} iscsid_request_t;

/*
   request
      Is the request ID.
   parameter_length
      Specifies the size in bytes of the parameter structure contained in
      parameter.
   parameter
      Contains a structure defining the parameters for the request.
*/


/* ==== General response structure ==== */

typedef struct {
	uint32_t	status;
	uint32_t	parameter_length;
	uint8_t		parameter[0];
} iscsid_response_t;

/*
   status
      Is the result of the request.
   parameter_length
      Specifies the size in bytes of the parameter structure contained in
      parameter.
   parameter
      Contains a structure defining the parameters for the response.
*/

/* ==== ADD_TARGET ==== */

/* Request */

typedef struct {
	iscsid_list_kind_t	list_kind;
	uint8_t			sym_name[ISCSI_STRING_LENGTH];
	uint8_t			TargetName[ISCSI_STRING_LENGTH];
	uint32_t		num_portals;
	iscsi_portal_address_t	portal[0];
} iscsid_add_target_req_t;

/*
   list_kind
      Kind of target list (TARGET_LIST or SEND_TARGETS_LIST)
   sym_name
      Symbolic name of the target (optional)
   TargetName
      Indicates the name of the target (zero terminated UTF-8 string).
   num_portals
      Number of portal addresses (may be zero).
   portal
      Array of portals for this target.
*/

typedef struct {
	uint32_t	target_id;
	uint32_t	num_portals;
	uint32_t	portal_id[0];
} iscsid_add_target_rsp_t;

/*
   target_id
      Is the unique ID assigned to this target.
   num_portals
      Number of portal IDs following.
   portal_id
      Array of unique IDs for the given portals, in the same order as in
      the request.
*/

/* ==== ADD_PORTAL ==== */

/* Request */

typedef struct {
	iscsid_sym_id_t		target_id;
	uint8_t			sym_name[ISCSI_STRING_LENGTH];
	iscsi_portal_address_t	portal;
	iscsid_portal_options_t	options;
} iscsid_add_portal_req_t;

/*
   target_id
      Is the unique ID for the target.
   sym_name
      Symbolic name of the portal (optional).
   portal
      Portal address.
   options
      Portal options.
*/

typedef struct {
	iscsid_sym_id_t	target_id;
	iscsid_sym_id_t	portal_id;
} iscsid_add_portal_rsp_t;

/*
   target_id
      Reflects the target ID.
   portal_id
      Returns the unique ID of the portal and its name.
*/

/* ==== SET_TARGET_OPTIONS ==== */

/* Request */

typedef struct {
	iscsid_list_kind_t	list_kind;
	iscsid_sym_id_t		target_id;
	struct {
		int		HeaderDigest:1;
		int		DataDigest:1;
		int		MaxConnections:1;
		int		DefaultTime2Wait:1;
		int		DefaultTime2Retain:1;
		int		MaxRecvDataSegmentLength:1;
		int		ErrorRecoveryLevel:1;
	} is_present;
	iscsi_digest_t		HeaderDigest;
	iscsi_digest_t		DataDigest;
	uint32_t		MaxRecvDataSegmentLength;
	uint16_t		MaxConnections;
	uint16_t		DefaultTime2Wait;
	uint16_t		DefaultTime2Retain;
	uint16_t		ErrorRecoveryLevel;
} iscsid_get_set_target_options_t;

/*
   list_kind
      Which list (TARGET_LIST or SEND_TARGETS_LIST)
   target_id
      Is the unique ID for the target.
   is_present
      Contains a bitfield that indicates which members of the structure
      contain valid data.
   HeaderDigest
      Indicates the digest to use for PDU headers.
   DataDigest
      Indicates the digest to use for PDU data.
   MaxRecvDataSegmentLength
      Allows limiting or extending the maximum receive data segment length.
      Must contain a value between 512 and 2**24-1 if specified.
   MaxConnections
      Contains a value between 1 and 65535 that specifies the maximum
      number of connections to target devices that can be associated with
      a single logon session. A value of 0 indicates that there no limit
      to the number of connections.
   DefaultTime2Wait
      Specifies the minimum time to wait, in seconds, before attempting
      to reconnect or reassign a connection that has been dropped.
   DefaultTime2Retain
      Specifies the maximum time, in seconds, allowed to reassign a
      connection after the initial wait indicated in DefaultTime2Retain
      has elapsed.
   ErrorRecoveryLevel
      Specifies the desired error recovery level for the session.
	  The default and maximum is 2.
*/

/*
   Response: Status only.
*/

/* ==== GET_TARGET_OPTIONS ==== */

/*
 * Request: iscsid_list_id_t
*/

/*
   Response: iscsid_get_set_target_options_t, see SET_TARGET_OPTIONS.
*/

/* ==== SET_TARGET_AUTHENTICATION ==== */

/* Request */

typedef struct {
	iscsid_list_kind_t	list_kind;
	iscsid_sym_id_t		target_id;
	iscsi_auth_info_t	auth_info;
	uint8_t			user_name[ISCSI_STRING_LENGTH];
	uint8_t			password[ISCSI_STRING_LENGTH];
	uint8_t			target_password[ISCSI_STRING_LENGTH];
} iscsid_set_target_authentication_req_t;

/*
   list_kind
      Which list (TARGET_LIST or SEND_TARGETS_LIST)
   target_id
      Is the unique ID for the target or target portal.
   auth_info
      Is the information about authorization types and options.
   user_name
      Sets the user (or CHAP) name to use during login authentication of
      the initiator (zero terminated UTF-8 string). Default is initiator
      name.
   password
      Contains the password to use during login authentication of the
      initiator (zero terminated UTF-8 string). Required if
      authentication is requested.
   target_password
      Contains the password to use during login authentication of the
      target (zero terminated UTF-8 string). Required if mutual
      authentication is requested.
*/
/*
   Response: Status only.
*/

/* ==== SLP_FIND_TARGETS ==== */

/*
   Request:
      The parameter contains the LDAPv3 filter string for the SLP search.
*/

typedef struct {
	uint32_t	num_portals;
	uint32_t	portal_id[1];
} iscsid_slp_find_targets_rsp_t;

/*
   num_portals
      Number of portal IDs following.
   portal_id
      Array of unique IDs for the discovered portals.
*/
/*
   Response: Status only.
*/

/* ==== REFRESH_TARGETS ==== */

/* Request */

typedef struct {
	iscsid_list_kind_t	kind;
	uint32_t		num_ids;
	uint32_t		id[1];
} iscsid_refresh_req_t;

/*
   kind
      The kind of list to refresh - either SEND_TARGETS_LIST or ISNS_LIST.
   num_ids
      Number of IDs following. If zero, all list members are used to
      refresh the target list.
   id
      Array of IDs to refresh.
*/
/*
   Response: Status only.
*/

/* ==== REMOVE_TARGET ==== */

/*
 * Request: iscsid_list_id_t
*/

/*
   Response: Status only.
*/

/* ==== SEARCH_LIST ==== */

typedef struct {
	iscsid_list_kind_t	list_kind;
	iscsid_search_kind_t	search_kind;
	uint8_t			strval[ISCSI_STRING_LENGTH];
	uint32_t		intval;
} iscsid_search_list_req_t;

/*
   list_kind
      Is the list kind.
   search_kind
      What to search for, also defines the contents of the 'strval' and/or
      'intval' fields.
   strval
      Is the string to look for.
   intval
      Is the integer value to look for.
*/

/*
 * Response: iscsid_sym_id_t
*/

/* ==== GET_LIST ==== */

/* Request */

typedef struct {
	iscsid_list_kind_t	list_kind;
} iscsid_get_list_req_t;

/*
   list_kind
      Is the list kind.
*/

typedef struct {
	uint32_t		num_entries;
	uint32_t		id[1];
} iscsid_get_list_rsp_t;

/*
   num_entries
      Number of ID entries following.
   id
      Array of IDs in the requested list.
*/

/* ==== GET_TARGET_INFO ==== */

/*
 * Request: iscsid_list_id_t
*/

typedef struct {
	iscsid_sym_id_t		target_id;
	uint8_t			TargetName[ISCSI_STRING_LENGTH];
	uint8_t			TargetAlias[ISCSI_STRING_LENGTH];
	uint32_t		num_portals;
	uint32_t		portal[1];
} iscsid_get_target_rsp_t;

/*
   TargetName
      The name of the target (zero terminated UTF-8 string).
   TargetAlias
      The alias of the target (zero terminated UTF-8 string).
   num_portals
      Number of portal IDs following.
   portal
      Array of portal IDs for this target.
*/

/* ==== GET_PORTAL_INFO ==== */

/*
 * Request: iscsid_list_id_t
*/

typedef struct {
	iscsid_sym_id_t		portal_id;
	iscsid_sym_id_t		target_id;
	iscsi_portal_address_t	portal;
	iscsid_portal_options_t	options;
} iscsid_get_portal_rsp_t;

/*
   portal_id
      ID and symbolic name for the portal
   target_id
      ID and symbolic name for the associated target
   portal
      Portal address
   options
      Portal options
*/

/* ==== ADD_ISNS_SERVER ==== */

/* Request */

typedef struct {
	uint8_t		name[ISCSI_STRING_LENGTH];
	uint8_t		address[ISCSI_ADDRESS_LENGTH];
	uint16_t	port;
} iscsid_add_isns_server_req_t;

/*
   name
      Symbolic name (optional)
   address
      Address (DNS name or IP address) of the iSNS server
   port
      IP port number.
*/

typedef struct {
	uint32_t	server_id;
} iscsid_add_isns_server_rsp_t;

/*
   server_id
      Unique ID for the iSNS server.
*/

/* ==== GET_ISNS_SERVER ==== */

/*
 * Request: iscsid_sym_id_t
*/

typedef struct {
	iscsid_sym_id_t	server_id;
	uint8_t		address[ISCSI_STRING_LENGTH];
	uint16_t	port;
} iscsid_get_isns_server_rsp_t;

/*
   server_id
      ID and symbolic name for the server
   address
      Server address
   port
      IP port number.
*/

/* ==== SLP_FIND_ISNS_SERVERS ==== */

/*
   Request:
      The parameter may optionally contain a comma separated list of
      scope names.
*/

typedef struct {
	uint32_t	num_servers;
	uint32_t	server_id[1];
} iscsid_find_isns_rsp_t;

/*
   num_servers
      Number of iSNS server IDs following.
   server_id
      Array of server IDs.
*/

/* ==== REMOVE_ISNS_SERVER ==== */

/*
 * Request: iscsid_sym_id_t
*/
/*
   Response: Status only.
*/

/* ==== ADD_INITIATOR_PORTAL ==== */

/* Request */

typedef struct {
	uint8_t		name[ISCSI_STRING_LENGTH];
	uint8_t		address[ISCSI_ADDRESS_LENGTH];
} iscsid_add_initiator_req_t;

/*
   name
      Symbolic name for this entry. Optional.
   address
      Interface address to add. Required.
*/

typedef struct {
	uint32_t	portal_id;
} iscsid_add_initiator_rsp_t;

/*
   id
      Unique ID for the portal.
*/

/* ==== GET_INITIATOR_PORTAL ==== */

/*
 * Request: iscsid_sym_id_t
*/

typedef struct {
	iscsid_sym_id_t	portal_id;
	uint8_t		address[ISCSI_ADDRESS_LENGTH];
} iscsid_get_initiator_rsp_t;

/*
   portal_id
      numeric and symbolic ID
   address
      Portal address.
*/

/* ==== REMOVE_INITIATOR_PORTAL ==== */

/*
 * Request: iscsid_sym_id_t
*/
/*
   Response: status only.
*/

/* ==== LOGIN ==== */

/* Request */

typedef struct {
	iscsid_sym_id_t			initiator_id;
	iscsid_sym_id_t			session_id;
	iscsid_sym_id_t			portal_id;
	uint8_t				sym_name[ISCSI_STRING_LENGTH];
	iscsi_login_session_type_t	login_type;
} iscsid_login_req_t;

/*
   initiator_id
      Contains the initiator portal ID. When 0, the initiator portal
      is selected automatically.
   session_id
      Contains the session ID for this connection. Must be 0 for login, a valid
      session ID for add_connection.
   portal_id
      Contains the target portal ID to connect to.
   sym_name
      Optional unique non-numeric symbolic session (or connection) name.
   login_type
      Contains an enumerator value of type LOGINSESSIONTYPE that
      indicates the type of logon session (discovery, non-mapped, or
      mapped).
*/

typedef struct {
	iscsid_sym_id_t		session_id;
	iscsid_sym_id_t		connection_id;
} iscsid_login_rsp_t;

/*
   session_id
      Receives an integer that identifies the session.
   connection_id
      Receives an integer that identifies the connection.
*/

/* ==== ADD_CONNECTION ==== */

/*
   Request and Response: see LOGIN.
*/

/* ==== LOGOUT ==== */

/*
 * Request: iscsid_sym_id_t
*/
/*
   Response: Status only.
*/


/* ==== REMOVE_CONNECTION ==== */

typedef struct {
	iscsid_sym_id_t		session_id;
	iscsid_sym_id_t		connection_id;
} iscsid_remove_connection_req_t;

/*
   session_id
      Contains an integer that identifies the session.
   connection_id
      Identifies the connection to remove.
*/
/*
   Response: Status only.
*/

/* ==== GET_SESSION_LIST ==== */

/*
   Request: No parameter.
*/

typedef struct {
	iscsid_sym_id_t		session_id;
	uint32_t		first_connection_id;
	uint32_t		num_connections;
	uint32_t		portal_id;
	uint32_t		initiator_id;
} iscsid_session_list_entry_t;


/*
   session_id
      Contains the session identifier.
   first_connection_id
      Contains the connection identifier for the first connection.
   num_connections
      The number of active connections in this session.
   portal_id
      Target portal ID.
   initiator_id
      Index of the initiator portal. May be zero.
*/

typedef struct {
	uint32_t			num_entries;
	iscsid_session_list_entry_t	session[1];
} iscsid_get_session_list_rsp_t;

/*
   num_entries
      The number of entries following.
   session
      The list entries (see above)
*/


/* ==== GET_CONNECTION_LIST ==== */

/*
 * Request: iscsid_sym_id_t - session ID
*/

typedef struct {
	iscsid_sym_id_t		connection_id;
	iscsid_sym_id_t		target_portal_id;
	iscsi_portal_address_t	target_portal;
} iscsid_connection_list_entry_t;

/*
   connection_id
      Connection ID.
   target_portal_id
      Target portal ID.
   target_portal
      Portal addresses of the target.
*/

typedef struct {
	uint32_t			num_connections;
	iscsid_connection_list_entry_t	connection[1];
} iscsid_get_connection_list_rsp_t;

/*
   num_connections
      The number of connection descriptors following.
   connection
      The list entries (see above).
*/


/* ==== GET_CONNECTION_INFO ==== */

typedef struct {
	iscsid_sym_id_t		session_id;
	iscsid_sym_id_t		connection_id;
} iscsid_get_connection_info_req_t;

/*
   session_id
      Contains an integer that identifies the session.
   connection_id
      Identifies the connection to retrieve.
*/

typedef struct {
	iscsid_sym_id_t		session_id;
	iscsid_sym_id_t		connection_id;
	iscsid_sym_id_t		initiator_id;
	iscsid_sym_id_t		target_portal_id;
	uint8_t			initiator_address[ISCSI_ADDRESS_LENGTH];
	uint8_t			TargetName[ISCSI_STRING_LENGTH];
	uint8_t			TargetAlias[ISCSI_STRING_LENGTH];
	iscsi_portal_address_t	target_portal;
} iscsid_get_connection_info_rsp_t;

/*
   session_id
      Reflects session ID
   connection_id
      Reflects  connection ID
   initiator_id
      Initiator portal ID. May be empty.
   target_portal_id
      Target portal ID.
   initiator_address
      Portal addresses of the initiator. May be empty if no initiators defined.
   TargetName
      The name of the target (zero terminated UTF-8 string).
   TargetAlias
      The alias of the target (zero terminated UTF-8 string).
   target_portal
      Portal addresses of the target.
*/

/* ===== set_node_name ===== */

typedef struct {
	uint8_t			InitiatorName[ISCSI_STRING_LENGTH];
	uint8_t			InitiatorAlias[ISCSI_STRING_LENGTH];
	uint8_t			ISID[6];
} iscsid_set_node_name_req_t;

/*
   InitiatorName
      Specifies the InitiatorName used during login. Required.
   InitiatorAlias
      Specifies the InitiatorAlias for use during login. May be empty.
   ISID
      Specifies the ISID (a 6 byte binary value) for use during login.
      May be zero (all bytes) for the initiator to use a default value.
*/
/*
   Response: Status only.
*/

/* ===== get_version ===== */

/*
   Request: No parameter.
*/

typedef struct {
	uint16_t		interface_version;
	uint16_t		major;
	uint16_t		minor;
	uint8_t			version_string[ISCSI_STRING_LENGTH];
	uint16_t		driver_interface_version;
	uint16_t		driver_major;
	uint16_t		driver_minor;
	uint8_t			driver_version_string[ISCSI_STRING_LENGTH];
} iscsid_get_version_rsp_t;

/*
   interface_version
      Updated when interface changes. Current Version is 2.
   major
      Major version number.
   minor
      Minor version number.
   version_string
      Displayable version string (zero terminated).
   driver_xxx
      Corresponding version information for driver.
*/

__END_DECLS

#endif /* !_ISCSID_H_ */
