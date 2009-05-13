/*	$NetBSD: link_mon.h,v 1.1.1.1.2.2 2009/05/13 18:52:40 jym Exp $	*/

#ifndef __LINK_MON_DOT_H__
#define __LINK_MON_DOT_H__

int links_register(int fd, char *name, int (*callback)(void *data), void *data);
int links_unregister(int fd);
int links_monitor(void);
int links_issue_callbacks(void);

#endif /* __LINK_MON_DOT_H__ */
