/*	$NetBSD: functions.h,v 1.1.1.1 2009/12/02 00:27:10 haad Exp $	*/

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
#ifndef __CLOG_FUNCTIONS_DOT_H__
#define __CLOG_FUNCTIONS_DOT_H__

#include "dm-log-userspace.h"
#include "cluster.h"

#define LOG_RESUMED   1
#define LOG_SUSPENDED 2

int local_resume(struct dm_ulog_request *rq);
int cluster_postsuspend(char *, uint64_t);

int do_request(struct clog_request *rq, int server);
int push_state(const char *uuid, uint64_t luid,
	       const char *which, char **buf, uint32_t debug_who);
int pull_state(const char *uuid, uint64_t luid,
	       const char *which, char *buf, int size);

int log_get_state(struct dm_ulog_request *rq);
int log_status(void);
void log_debug(void);

#endif /* __CLOG_FUNCTIONS_DOT_H__ */
