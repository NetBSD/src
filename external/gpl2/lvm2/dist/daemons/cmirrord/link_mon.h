/*	$NetBSD: link_mon.h,v 1.1.1.1 2009/12/02 00:27:10 haad Exp $	*/

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
#ifndef __LINK_MON_DOT_H__
#define __LINK_MON_DOT_H__

int links_register(int fd, char *name, int (*callback)(void *data), void *data);
int links_unregister(int fd);
int links_monitor(void);
int links_issue_callbacks(void);

#endif /* __LINK_MON_DOT_H__ */
