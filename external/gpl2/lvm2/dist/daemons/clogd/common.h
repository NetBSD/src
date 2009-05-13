/*	$NetBSD: common.h,v 1.1.1.1.2.2 2009/05/13 18:52:40 jym Exp $	*/

#ifndef __CLUSTER_LOG_COMMON_DOT_H__
#define __CLUSTER_LOG_COMMON_DOT_H__

/*
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
*/

#define EXIT_LOCKFILE              2

#define EXIT_KERNEL_TFR_SOCKET     3 /* Failed netlink socket create */
#define EXIT_KERNEL_TFR_BIND       4
#define EXIT_KERNEL_TFR_SETSOCKOPT 5

#define EXIT_CLUSTER_CKPT_INIT     6 /* Failed to init checkpoint */

#define EXIT_QUEUE_NOMEM           7

/* Located in dm-clog-tfr.h
#define RQ_TYPE(x) \
	((x) == DM_CLOG_CTR) ? "DM_CLOG_CTR" : \
	((x) == DM_CLOG_DTR) ? "DM_CLOG_DTR" : \
	((x) == DM_CLOG_PRESUSPEND) ? "DM_CLOG_PRESUSPEND" : \
	((x) == DM_CLOG_POSTSUSPEND) ? "DM_CLOG_POSTSUSPEND" : \
	((x) == DM_CLOG_RESUME) ? "DM_CLOG_RESUME" : \
	((x) == DM_CLOG_GET_REGION_SIZE) ? "DM_CLOG_GET_REGION_SIZE" : \
	((x) == DM_CLOG_IS_CLEAN) ? "DM_CLOG_IS_CLEAN" : \
	((x) == DM_CLOG_IN_SYNC) ? "DM_CLOG_IN_SYNC" : \
	((x) == DM_CLOG_FLUSH) ? "DM_CLOG_FLUSH" : \
	((x) == DM_CLOG_MARK_REGION) ? "DM_CLOG_MARK_REGION" : \
	((x) == DM_CLOG_CLEAR_REGION) ? "DM_CLOG_CLEAR_REGION" : \
	((x) == DM_CLOG_GET_RESYNC_WORK) ? "DM_CLOG_GET_RESYNC_WORK" : \
	((x) == DM_CLOG_SET_REGION_SYNC) ? "DM_CLOG_SET_REGION_SYNC" : \
	((x) == DM_CLOG_GET_SYNC_COUNT) ? "DM_CLOG_GET_SYNC_COUNT" : \
	((x) == DM_CLOG_STATUS_INFO) ? "DM_CLOG_STATUS_INFO" : \
	((x) == DM_CLOG_STATUS_TABLE) ? "DM_CLOG_STATUS_TABLE" : \
	NULL
*/

#endif /* __CLUSTER_LOG_COMMON_DOT_H__ */
