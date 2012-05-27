/*	$NetBSD: iscsid_globals.h,v 1.5 2012/05/27 17:27:33 riz Exp $	*/

/*-
 * Copyright (c) 2005,2006,2011 The NetBSD Foundation, Inc.
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

#ifndef _ISCSID_GLOBALS_H
#define _ISCSID_GLOBALS_H

#ifndef _THREAD_SAFE
#define _THREAD_SAFE 1
#endif

#include <sys/queue.h>
#include <sys/scsiio.h>
#include <sys/param.h>

#include <uvm/uvm_param.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#ifndef ISCSI_NOTHREAD
#include <pthread.h>
#endif

#include <iscsi.h>
#include <iscsi_ioctl.h>

#include "iscsid.h"

/* -------------------------  Global Constants  ----------------------------- */

/* Version information */

#define INTERFACE_VERSION	2
#define VERSION_MAJOR		3
#define VERSION_MINOR		1
#define VERSION_STRING		"NetBSD iSCSI Software Initiator Daemon 20110407 "

/* Sizes for the static request and response buffers. */
/* 8k should be more than enough for both. */
#define REQ_BUFFER_SIZE    8192
#define RSP_BUFFER_SIZE    8192

#define ISCSI_DEFAULT_PORT 3260
#define ISCSI_DEFAULT_ISNS_PORT 3205

/* ---------------------------  Global Types  ------------------------------- */

#ifndef TRUE
typedef int boolean_t;
#define	TRUE	1
#define	FALSE	0
#endif


/*
 * The generic list entry.
 * Almost all lists in the daemon use this structure as the base for
 * list processing. It contains both a numeric ID and a symbolic name.
 * Using the same structure for all lists greatly simplifies processing.
 *
 * All structures that will be linked into searchable lists have to define
 * their first structure field as "generic_entry_t entry".
*/

struct generic_entry_s
{
	TAILQ_ENTRY(generic_entry_s) link;	/* the list link */
	iscsid_sym_id_t sid;		/* the entry ID and name */
};

typedef struct generic_entry_s generic_entry_t;
TAILQ_HEAD(generic_list_s, generic_entry_s);
typedef struct generic_list_s generic_list_t;

/*
 * The iSNS list structure.
 * This structure contains the list of iSNS servers that have been added
 */

struct isns_s
{
	generic_entry_t entry;		/* global list link */

	uint8_t address[ISCSI_ADDRESS_LENGTH];	/* iSNS Server Address */
	uint16_t port;				/* Port (0 = default) */

	int sock;					/* socket if registered, else -1 */
	/* following fields only valid if sock >= 0 */
	uint8_t reg_iscsi_name[ISCSI_STRING_LENGTH];	/* Registered ISCSI Name */
	uint8_t reg_entity_id[ISCSI_STRING_LENGTH];	/* Registered Entity Identifier */
	uint8_t reg_ip_addr[16];	/* registered IP address */
	uint32_t reg_ip_port;		/* registered IP port */
};


TAILQ_HEAD(isns_list_s, isns_s);
typedef struct isns_s isns_t;
typedef struct isns_list_s isns_list_t;


/*
 *  The initiator portal list structure.
 */

typedef struct initiator_s initiator_t;

struct initiator_s
{
	generic_entry_t entry;		/* global list link */

	uint8_t address[ISCSI_ADDRESS_LENGTH];	/* address */
	uint32_t active_connections;	/* connection count */
};

TAILQ_HEAD(initiator_list_s, initiator_s);
typedef struct initiator_list_s initiator_list_t;


/*
 * The portal structure.
 * This structure is linked into two lists - a global portal list (this list
 * is used for searches and to verify unique IDs) and a portal group list
 * attached to the owning target.
 */

typedef enum
{
	PORTAL_TYPE_STATIC = 0,
	PORTAL_TYPE_SENDTARGET = 1,
	PORTAL_TYPE_ISNS = 2,
	PORTAL_TYPE_REFRESHING = 99
} iscsi_portal_types_t;
/*
   PORTAL_TYPE_STATIC
      Indicates that target was statically added
   PORTAL_TYPE_SENDTARGET
      Indicates that target was added as result of SendTargets discovery
   PORTAL_TYPE_ISNS
      Indicates that target was added as result of iSNS discovery
   PORTAL_TYPE_REFRESHING
      Discovered portals are set to this when we are refreshing
      (via REFRESH_TARGETS). As a portal is discovered, its type is reset to
      SENDTARGET or ISNS, so any portals which remain set to REFRESHING were not
      discovered and thus can be removed.
*/

typedef struct portal_s portal_t;
typedef struct portal_group_s portal_group_t;
typedef struct target_s target_t;
typedef struct send_target_s send_target_t;

struct portal_s
{
	generic_entry_t entry;		/* global list link */

	  TAILQ_ENTRY(portal_s) group_list;	/* group list link */

	iscsi_portal_address_t addr;	/* address */
	iscsid_portal_options_t options; /* portal options (override target options) */
	target_t *target;			/* back pointer to target */
	portal_group_t *group;		/* back pointer to group head */
	iscsi_portal_types_t portaltype;   /* Type of portal (how it was discovered) */
	uint32_t discoveryid;		/* ID of sendtargets or isnsserver */
	uint32_t active_connections; /* Number of connections active on this portal */
};

TAILQ_HEAD(portal_list_s, portal_s);
typedef struct portal_list_s portal_list_t;


/*
 * The portal group structure.
 * This structure is not searchable, and has no generic list entry field.
 * It links all portals with the same group tag to the owning target.
*/

struct portal_group_s
{
	TAILQ_ENTRY(portal_group_s) groups;	/* link to next group */

	portal_list_t portals;		/* the list of portals for this tag */

	uint32_t tag;				/* the group tag */
	u_short num_portals;		/* the number of portals in this list */
};

TAILQ_HEAD(portal_group_list_s, portal_group_s);
typedef struct portal_group_list_s portal_group_list_t;


/*
 * The target structure.
 * Contains target information including connection and authentication options.
 *****************************************************************************
 * WARNING: This structure is used interchangeably with a send_target structure
 *          in many routines dealing with targets to avoid duplicating code.
 *          The first fields in both structures up to and including
 *          the authentication options MUST match for this to work.
 *          If you change one, you MUST change the other accordingly.
 *****************************************************************************
*/

struct target_s
{
	generic_entry_t entry;		/* global list link */

	uint8_t TargetName[ISCSI_STRING_LENGTH];	/* TargetName */
	uint8_t TargetAlias[ISCSI_STRING_LENGTH];	/* TargetAlias */

	u_short num_portals;		/* the number of portals */
	u_short num_groups;			/* the number of groups */

	iscsid_get_set_target_options_t options;	/* connection options */
	iscsid_set_target_authentication_req_t auth;	/* authentication options */

	portal_group_list_t group_list;	/* the list of portal groups */
};

TAILQ_HEAD(target_list_s, target_s);
typedef struct target_list_s target_list_t;


/*
 * The Send Target structure.
 * Contains target information including connection and authentication options
 * plus a single portal.
 *****************************************************************************
 * WARNING: This structure is used interchangeably with a target structure
 *          in many routines dealing with targets to avoid duplicating code.
 *          The first fields in both structures up to and including
 *          the authentication options MUST match for this to work.
 *          If you change one, you MUST change the other accordingly.
 *****************************************************************************
 */

struct send_target_s
{
	generic_entry_t entry;		/* global list link */

	uint8_t TargetName[ISCSI_STRING_LENGTH];	/* TargetName */
	uint8_t TargetAlias[ISCSI_STRING_LENGTH];	/* TargetAlias */

	u_short num_portals;		/* the number of portals */
	u_short num_groups;			/* the number of groups */
	/* */
	iscsid_get_set_target_options_t options;	/* connection options */
	iscsid_set_target_authentication_req_t auth;	/* authentication options */

	iscsi_portal_address_t addr;	/* address */
};

TAILQ_HEAD(send_target_list_s, send_target_s);
typedef struct send_target_list_s send_target_list_t;

/*
   Target and Portal information maintained in the connection structure.
*/

struct target_info_s
{
	iscsid_sym_id_t sid;		/* the entry ID and name */
	uint8_t TargetName[ISCSI_STRING_LENGTH];	/* TargetName */
	uint8_t TargetAlias[ISCSI_STRING_LENGTH];	/* TargetAlias */
	iscsid_get_set_target_options_t options;	/* connection options */
	iscsid_set_target_authentication_req_t auth;	/* authentication options */
};
typedef struct target_info_s target_info_t;


struct portal_info_s
{
	iscsid_sym_id_t sid;		/* the entry ID and name */
	iscsi_portal_address_t addr;	/* address */
};
typedef struct portal_info_s portal_info_t;

/*
   Per connection data: the connection structure.
*/

typedef struct connection_s connection_t;
typedef struct session_s session_t;


struct connection_s
{
	generic_entry_t entry;		/* connection list link */

	session_t *session;			/* back pointer to the owning session */
	target_info_t target;		/* connected target */
	portal_info_t portal;		/* connected portal */
	uint32_t initiator_id;		/* connected initiator portal */

	iscsi_login_parameters_t loginp;	/* Login parameters for recovery */
};


/*
   Per session data: the session structure
*/

struct session_s
{
	generic_entry_t entry;		/* global list link */

	target_info_t target;		/* connected target */
	iscsi_login_session_type_t login_type;	/* session type */

	uint32_t max_connections;	/* maximum connections */
	uint32_t num_connections;	/* currently active connections */
	generic_list_t connections;	/* the list of connections */
};

/* the session list type */

TAILQ_HEAD(session_list_s, session_s);
typedef struct session_list_s session_list_t;


/* list head with entry count */

typedef struct
{
	generic_list_t list;
	int num_entries;
} list_head_t;

/* -------------------------  Global Variables  ----------------------------- */

/* In iscsid_main.c */

int driver;						/* the driver's file desc */
int client_sock;				/* the client communication socket */

list_head_t list[NUM_DAEMON_LISTS];	/* the lists this daemon keeps */

#ifndef ISCSI_NOTHREAD
pthread_t event_thread;			/* event handler thread ID */
pthread_mutex_t sesslist_lock;	/* session list lock */
#endif

/* in iscsid_discover.c */

iscsid_set_node_name_req_t node_name;


/* -------------------------  Global Functions  ----------------------------- */

/* Debugging stuff */

#ifdef ISCSI_DEBUG

int debug_level;				/* How much info to display */

#define DEBOUT(x) printf x
#define DEB(lev,x) {if (debug_level >= lev) printf x ;}

#else

#define DEBOUT(x)
#define DEB(lev,x)

#endif

/* Session list protection shortcuts */

#if 0
#define LOCK_SESSIONS   verify_sessions()
#define UNLOCK_SESSIONS
#endif
#ifdef ISCSI_NOTHREAD
#define LOCK_SESSIONS   event_handler(NULL)
#define UNLOCK_SESSIONS do {} while(0)
#else
#define LOCK_SESSIONS   pthread_mutex_lock(&sesslist_lock)
#define UNLOCK_SESSIONS pthread_mutex_unlock(&sesslist_lock)
#endif

/* Check whether ID is present */

#define NO_ID(sid) (!(sid)->id && !(sid)->name[0])

/* iscsid_main.c */

iscsid_response_t *make_rsp(size_t, iscsid_response_t **, int *);
void exit_daemon(void) __dead;

/* iscsid_lists.c */

generic_entry_t *find_id(generic_list_t *, uint32_t);
generic_entry_t *find_name(generic_list_t *, uint8_t *);
generic_entry_t *find_sym_id(generic_list_t *, iscsid_sym_id_t *);
uint32_t get_id(generic_list_t *, iscsid_sym_id_t *);
target_t *find_target(iscsid_list_kind_t, iscsid_sym_id_t *);
target_t *find_TargetName(iscsid_list_kind_t, uint8_t *);
portal_t *find_portal_by_addr(target_t *, iscsi_portal_address_t *);
send_target_t *find_send_target_by_addr(iscsi_portal_address_t *);

#define find_isns_id(id) \
   (isns_t *)(void *)find_id(&list [ISNS_LIST].list, id)
#define find_session_id(id) \
   (session_t *)(void *)find_id(&list [SESSION_LIST].list, id)
#define find_connection_id(session, id) \
   (connection_t *)(void *)find_id(&session->connections, id)
#define find_portal_id(id) \
   (portal_t *)(void *)find_id(&list [PORTAL_LIST].list, id)
#define find_target_id(lst, id) \
   (target_t *)(void *)find_id(&list [lst].list, id)
#define find_send_target_id(id) \
   (send_target_t *)(void *)find_id(&list [SEND_TARGETS_LIST].list, id)
#define find_initiator_id(id) \
   (initiator_t *)(void *)find_id(&list [INITIATOR_LIST].list, id)
#define find_isns_name(name) \
   (isns_t *)(void *)find_name(&list [ISNS_LIST].list, name)
#define find_session_name(name) \
   (session_t *)(void *)find_name(&list [SESSION_LIST].list, name)
#define find_connection_name(session, name) \
   (connection_t *)(void *)find_name(&session->connections, name)
#define find_portal_name(name) \
   (portal_t *)(void *)find_name(&list [PORTAL_LIST].list, name)
#define find_target_symname(lst, name) \
   (target_t *)(void *)find_name(&list [lst].list, name)
#define find_initiator_name(name) \
   (initiator_t *)(void *)find_name(&list [INITIATOR_LIST].list, name)
#define find_isns(sid) \
   (isns_t *)(void *)find_sym_id(&list [ISNS_LIST].list, sid)
#define find_session(sid) \
   (session_t *)(void *)find_sym_id(&list [SESSION_LIST].list, sid)
#define find_connection(session, sid) \
   (connection_t *)(void *)find_sym_id(&session->connections, sid)
#define find_portal(sid) \
   (portal_t *)(void *)find_sym_id(&list [PORTAL_LIST].list, sid)
#define find_initiator(sid) \
   (initiator_t *)(void *)find_sym_id(&list [INITIATOR_LIST].list, sid)

void get_list(iscsid_get_list_req_t *, iscsid_response_t **, int *);
void search_list(iscsid_search_list_req_t *, iscsid_response_t **, int *);

void get_session_list(iscsid_response_t **, int *);
void get_connection_info(iscsid_get_connection_info_req_t *,
						 iscsid_response_t **, int *);
void get_connection_list(iscsid_sym_id_t *, iscsid_response_t **, int *);

void add_initiator_portal(iscsid_add_initiator_req_t *, iscsid_response_t **,
						  int *);
uint32_t remove_initiator_portal(iscsid_sym_id_t *);
void get_initiator_portal(iscsid_sym_id_t *, iscsid_response_t **, int *);
initiator_t *select_initiator(void);

void event_kill_session(uint32_t);
void event_kill_connection(uint32_t, uint32_t);

/* iscsid_targets.c */

void add_target(iscsid_add_target_req_t *, iscsid_response_t **, int *);
uint32_t set_target_options(iscsid_get_set_target_options_t *);
uint32_t set_target_auth(iscsid_set_target_authentication_req_t *);
void add_portal(iscsid_add_portal_req_t *, iscsid_response_t **, int *);
void delete_portal(portal_t *, boolean_t);

void get_target_info(iscsid_list_id_t *, iscsid_response_t **, int *);
void get_portal_info(iscsid_list_id_t *, iscsid_response_t **, int *);
uint32_t remove_target(iscsid_list_id_t *);
uint32_t refresh_targets(iscsid_refresh_req_t *);
target_t *add_discovered_target(uint8_t *, iscsi_portal_address_t *,
								iscsi_portal_types_t, uint32_t);

/* iscsid_driverif.c */

boolean_t register_event_handler(void);
void deregister_event_handler(void);
void *event_handler(void *);

uint32_t set_node_name(iscsid_set_node_name_req_t *);
void login(iscsid_login_req_t *, iscsid_response_t *);
void add_connection(iscsid_login_req_t *, iscsid_response_t *);
uint32_t send_targets(uint32_t, uint8_t **, uint32_t *);
uint32_t logout(iscsid_sym_id_t *);
uint32_t remove_connection(iscsid_remove_connection_req_t *);
void get_version(iscsid_response_t **, int *);

/* iscsid_discover.c */

#ifndef ISCSI_MINIMAL
void add_isns_server(iscsid_add_isns_server_req_t *, iscsid_response_t **,
					 int *);
void get_isns_server(iscsid_sym_id_t *, iscsid_response_t **, int *);
uint32_t refresh_isns_server(uint32_t);
uint32_t remove_isns_server(iscsid_sym_id_t *);
void dereg_all_isns_servers(void);
#endif

#endif /* !_ISCSID_GLOBALS_H */
