/*	$NetBSD: local.h,v 1.1.1.1.2.2 2009/05/13 18:52:40 jym Exp $	*/

#ifndef __CLUSTER_LOG_LOCAL_DOT_H__
#define __CLUSTER_LOG_LOCAL_DOT_H__

int init_local(void);
void cleanup_local(void);

int kernel_send(struct clog_tfr *tfr);

#endif /* __CLUSTER_LOG_LOCAL_DOT_H__ */
