/*	$NetBSD: link_mon.h,v 1.1.1.1 2009/02/18 11:16:32 haad Exp $	*/

#ifndef __LINK_MON_DOT_H__
#define __LINK_MON_DOT_H__

int links_register(int fd, char *name, int (*callback)(void *data), void *data);
int links_unregister(int fd);
int links_monitor(void);
int links_issue_callbacks(void);

#endif /* __LINK_MON_DOT_H__ */
