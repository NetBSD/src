/*	$NetBSD: clvmd-comms.h,v 1.1.1.1.2.1 2009/05/13 18:52:40 jym Exp $	*/

/*
 * Copyright (C) 2002-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU General Public License v.2.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Abstraction layer for clvmd cluster communications
 */

#ifndef _CLVMD_COMMS_H
#define _CLVMD_COMMS_H

struct local_client;

struct cluster_ops {
	void (*cluster_init_completed) (void);

	int (*cluster_send_message) (const void *buf, int msglen,
				     const char *csid,
				     const char *errtext);
	int (*name_from_csid) (const char *csid, char *name);
	int (*csid_from_name) (char *csid, const char *name);
	int (*get_num_nodes) (void);
	int (*cluster_fd_callback) (struct local_client *fd, char *buf, int len,
				    const char *csid,
				    struct local_client **new_client);
	int (*get_main_cluster_fd) (void);	/* gets accept FD or cman cluster socket */
	int (*cluster_do_node_callback) (struct local_client *client,
					 void (*callback) (struct local_client *,
							   const char *csid,
							   int node_up));
	int (*is_quorate) (void);

	void (*get_our_csid) (char *csid);
	void (*add_up_node) (const char *csid);
	void (*reread_config) (void);
	void (*cluster_closedown) (void);

	int (*get_cluster_name)(char *buf, int buflen);

	int (*sync_lock) (const char *resource, int mode,
			  int flags, int *lockid);
	int (*sync_unlock) (const char *resource, int lockid);

};

#ifdef USE_GULM
#  include "tcp-comms.h"
struct cluster_ops *init_gulm_cluster(void);
#define MAX_CSID_LEN 			GULM_MAX_CSID_LEN
#define MAX_CLUSTER_MEMBER_NAME_LEN	GULM_MAX_CLUSTER_MEMBER_NAME_LEN
#endif

#ifdef USE_CMAN
#  include <netinet/in.h>
#  include "libcman.h"
#  define CMAN_MAX_CSID_LEN 4
#  ifndef MAX_CSID_LEN
#    define MAX_CSID_LEN CMAN_MAX_CSID_LEN
#  endif
#  undef MAX_CLUSTER_MEMBER_NAME_LEN
#  define MAX_CLUSTER_MEMBER_NAME_LEN   CMAN_MAX_NODENAME_LEN
#  define CMAN_MAX_CLUSTER_MESSAGE 1500
#  define CLUSTER_PORT_CLVMD 11
struct cluster_ops *init_cman_cluster(void);
#endif

#ifdef USE_OPENAIS
#  include <openais/saAis.h>
#  include <openais/totem/totem.h>
#  define OPENAIS_CSID_LEN (sizeof(int))
#  define OPENAIS_MAX_CLUSTER_MESSAGE         MESSAGE_SIZE_MAX
#  define OPENAIS_MAX_CLUSTER_MEMBER_NAME_LEN SA_MAX_NAME_LENGTH
#  ifndef MAX_CLUSTER_MEMBER_NAME_LEN
#    define MAX_CLUSTER_MEMBER_NAME_LEN       SA_MAX_NAME_LENGTH
#  endif
#  ifndef CMAN_MAX_CLUSTER_MESSAGE
#    define CMAN_MAX_CLUSTER_MESSAGE          MESSAGE_SIZE_MAX
#  endif
#  ifndef MAX_CSID_LEN
#    define MAX_CSID_LEN sizeof(int)
#  endif
struct cluster_ops *init_openais_cluster(void);
#endif

#ifdef USE_COROSYNC
#  include <corosync/corotypes.h>
#  define COROSYNC_CSID_LEN (sizeof(int))
#  define COROSYNC_MAX_CLUSTER_MESSAGE         65535
#  define COROSYNC_MAX_CLUSTER_MEMBER_NAME_LEN CS_MAX_NAME_LENGTH
#  ifndef MAX_CLUSTER_MEMBER_NAME_LEN
#    define MAX_CLUSTER_MEMBER_NAME_LEN       CS_MAX_NAME_LENGTH
#  endif
#  ifndef CMAN_MAX_CLUSTER_MESSAGE
#    define CMAN_MAX_CLUSTER_MESSAGE          65535
#  endif
#  ifndef MAX_CSID_LEN
#    define MAX_CSID_LEN sizeof(int)
#  endif
struct cluster_ops *init_corosync_cluster(void);
#endif


#endif
