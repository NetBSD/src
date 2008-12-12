/*	$NetBSD: dmeventd.h,v 1.1.1.1 2008/12/12 11:42:06 haad Exp $	*/

/*
 * Copyright (C) 2005-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of the device-mapper userspace tools.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __DMEVENTD_DOT_H__
#define __DMEVENTD_DOT_H__

/* FIXME This stuff must be configurable. */

#define	DM_EVENT_DAEMON		"/sbin/dmeventd"
#define DM_EVENT_LOCKFILE	"/var/lock/dmeventd"
#define	DM_EVENT_FIFO_CLIENT	"/var/run/dmeventd-client"
#define	DM_EVENT_FIFO_SERVER	"/var/run/dmeventd-server"
#define DM_EVENT_PIDFILE	"/var/run/dmeventd.pid"

#define DM_EVENT_DEFAULT_TIMEOUT 10

/* Commands for the daemon passed in the message below. */
enum dm_event_command {
	DM_EVENT_CMD_ACTIVE = 1,
	DM_EVENT_CMD_REGISTER_FOR_EVENT,
	DM_EVENT_CMD_UNREGISTER_FOR_EVENT,
	DM_EVENT_CMD_GET_REGISTERED_DEVICE,
	DM_EVENT_CMD_GET_NEXT_REGISTERED_DEVICE,
	DM_EVENT_CMD_SET_TIMEOUT,
	DM_EVENT_CMD_GET_TIMEOUT,
	DM_EVENT_CMD_HELLO,
};

/* Message passed between client and daemon. */
struct dm_event_daemon_message {
	uint32_t cmd;
	uint32_t size;
	char *data;
};

/* FIXME Is this meant to be exported?  I can't see where the
   interface uses it. */
/* Fifos for client/daemon communication. */
struct dm_event_fifos {
	int client;
	int server;
	const char *client_path;
	const char *server_path;
};

/*      EXIT_SUCCESS             0 -- stdlib.h */
/*      EXIT_FAILURE             1 -- stdlib.h */
#define EXIT_LOCKFILE_INUSE      2
#define EXIT_DESC_CLOSE_FAILURE  3
#define EXIT_DESC_OPEN_FAILURE   4
#define EXIT_OPEN_PID_FAILURE    5
#define EXIT_FIFO_FAILURE        6
#define EXIT_CHDIR_FAILURE       7

#endif /* __DMEVENTD_DOT_H__ */
