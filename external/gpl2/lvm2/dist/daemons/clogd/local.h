/*	$NetBSD: local.h,v 1.1.1.1 2009/02/18 11:16:33 haad Exp $	*/

#ifndef __CLUSTER_LOG_LOCAL_DOT_H__
#define __CLUSTER_LOG_LOCAL_DOT_H__

int init_local(void);
void cleanup_local(void);

int kernel_send(struct clog_tfr *tfr);

#endif /* __CLUSTER_LOG_LOCAL_DOT_H__ */
