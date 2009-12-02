/*	$NetBSD: logging.c,v 1.1.1.1 2009/12/02 00:27:10 haad Exp $	*/

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
#include <stdio.h>
#include <syslog.h>

char *__rq_types_off_by_one[] = {
	"DM_ULOG_CTR",
	"DM_ULOG_DTR",
	"DM_ULOG_PRESUSPEND",
	"DM_ULOG_POSTSUSPEND",
	"DM_ULOG_RESUME",
	"DM_ULOG_GET_REGION_SIZE",
	"DM_ULOG_IS_CLEAN",
	"DM_ULOG_IN_SYNC",
	"DM_ULOG_FLUSH",
	"DM_ULOG_MARK_REGION",
	"DM_ULOG_CLEAR_REGION",
	"DM_ULOG_GET_RESYNC_WORK",
	"DM_ULOG_SET_REGION_SYNC",
	"DM_ULOG_GET_SYNC_COUNT",
	"DM_ULOG_STATUS_INFO",
	"DM_ULOG_STATUS_TABLE",
	"DM_ULOG_IS_REMOTE_RECOVERING",
	NULL
};

int log_tabbing = 0;
int log_is_open = 0;

/*
 * Variables for various conditional logging
 */
#ifdef MEMB
int log_membership_change = 1;
#else
int log_membership_change = 0;
#endif

#ifdef CKPT
int log_checkpoint = 1;
#else
int log_checkpoint = 0;
#endif

#ifdef RESEND
int log_resend_requests = 1;
#else
int log_resend_requests = 0;
#endif
