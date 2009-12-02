/*	$NetBSD: cluster.h,v 1.1.1.1 2009/12/02 00:27:08 haad Exp $	*/

/*
 * Copyright (C) 2004-2009 Red Hat, Inc. All rights reserved.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __CLUSTER_LOG_CLUSTER_DOT_H__
#define __CLUSTER_LOG_CLUSTER_DOT_H__

#include "libdevmapper.h"
#include "dm-log-userspace.h"

/*
 * There is other information in addition to what can
 * be found in the dm_ulog_request structure that we
 * need for processing.  'clog_request' is the wrapping
 * structure we use to make the additional fields
 * available.
 */
struct clog_request {
	struct dm_list list;

	/*
	 * 'originator' is the machine from which the requests
	 * was made.
	 */
	uint32_t originator;

	/*
	 * 'pit_server' is the "point-in-time" server for the
	 * request.  (I.e.  The machine that was the server at
	 * the time the request was issued - only important during
	 * startup.
	 */
	uint32_t pit_server;

	/*
	 * The request from the kernel that is being processed
	 */
	struct dm_ulog_request u_rq;
};

int init_cluster(void);
void cleanup_cluster(void);
void cluster_debug(void);

int create_cluster_cpg(char *uuid, uint64_t luid);
int destroy_cluster_cpg(char *uuid);

int cluster_send(struct clog_request *rq);

#endif /* __CLUSTER_LOG_CLUSTER_DOT_H__ */
