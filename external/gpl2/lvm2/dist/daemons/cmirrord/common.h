/*	$NetBSD: common.h,v 1.1.1.1 2009/12/02 00:27:08 haad Exp $	*/

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
#ifndef __CLUSTER_LOG_COMMON_DOT_H__
#define __CLUSTER_LOG_COMMON_DOT_H__

/*
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
*/

#define EXIT_LOCKFILE              2

#define EXIT_KERNEL_SOCKET         3 /* Failed netlink socket create */
#define EXIT_KERNEL_BIND           4
#define EXIT_KERNEL_SETSOCKOPT     5

#define EXIT_CLUSTER_CKPT_INIT     6 /* Failed to init checkpoint */

#define EXIT_QUEUE_NOMEM           7


#define DM_ULOG_REQUEST_SIZE 1024

#endif /* __CLUSTER_LOG_COMMON_DOT_H__ */
