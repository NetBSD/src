/*	$NetBSD: clvmd.h,v 1.1.1.1.2.3 2008/12/13 14:39:31 haad Exp $	*/

/*
 * Copyright (C) 2002-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU General Public License v.2.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _CLVMD_H
#define _CLVMD_H

#define CLVMD_MAJOR_VERSION 0
#define CLVMD_MINOR_VERSION 2
#define CLVMD_PATCH_VERSION 1

/* Name of the cluster LVM admin lock */
#define ADMIN_LOCK_NAME "CLVMD_ADMIN"

/* Default time (in seconds) we will wait for all remote commands to execute
   before declaring them dead */
#define DEFAULT_CMD_TIMEOUT 60

/* One of these for each reply we get from command execution on a node */
struct node_reply {
	char node[MAX_CLUSTER_MEMBER_NAME_LEN];
	char *replymsg;
	int status;
	struct node_reply *next;
};

typedef enum {DEBUG_OFF, DEBUG_STDERR, DEBUG_SYSLOG} debug_t;

/*
 * These exist for the use of local sockets only when we are
 * collecting responses from all cluster nodes
 */
struct localsock_bits {
	struct node_reply *replies;
	int num_replies;
	int expected_replies;
	time_t sent_time;	/* So we can check for timeouts */
	int in_progress;	/* Only execute one cmd at a time per client */
	int sent_out;		/* Flag to indicate that a command was sent
				   to remote nodes */
	void *private;		/* Private area for command processor use */
	void *cmd;		/* Whole command as passed down local socket */
	int cmd_len;		/* Length of above */
	int pipe;		/* Pipe to send PRE completion status down */
	int finished;		/* Flag to tell subthread to exit */
	int all_success;	/* Set to 0 if any node (or the pre_command)
				   failed */
	struct local_client *pipe_client;
	pthread_t threadid;
	enum { PRE_COMMAND, POST_COMMAND, QUIT } state;
	pthread_mutex_t mutex;	/* Main thread and worker synchronisation */
	pthread_cond_t cond;

	pthread_mutex_t reply_mutex;	/* Protect reply structure */
};

/* Entries for PIPE clients */
struct pipe_bits {
	struct local_client *client;	/* Actual (localsock) client */
	pthread_t threadid;		/* Our own copy of the thread id */
};

/* Entries for Network socket clients */
struct netsock_bits {
	void *private;
	int flags;
};

typedef int (*fd_callback_t) (struct local_client * fd, char *buf, int len,
			      const char *csid,
			      struct local_client ** new_client);

/* One of these for each fd we are listening on */
struct local_client {
	int fd;
	enum { CLUSTER_MAIN_SOCK, CLUSTER_DATA_SOCK, LOCAL_RENDEZVOUS,
		    LOCAL_SOCK, THREAD_PIPE, CLUSTER_INTERNAL } type;
	struct local_client *next;
	unsigned short xid;
	fd_callback_t callback;
	uint8_t removeme;

	union {
		struct localsock_bits localsock;
		struct pipe_bits pipe;
		struct netsock_bits net;
	} bits;
};

#define DEBUGLOG(fmt, args...) debuglog(fmt, ## args);

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

/* The real command processor is in clvmd-command.c */
extern int do_command(struct local_client *client, struct clvm_header *msg,
		      int msglen, char **buf, int buflen, int *retlen);

/* Pre and post command routines are called only on the local node */
extern int do_pre_command(struct local_client *client);
extern int do_post_command(struct local_client *client);
extern void cmd_client_cleanup(struct local_client *client);
extern int add_client(struct local_client *new_client);

extern void clvmd_cluster_init_completed(void);
extern void process_message(struct local_client *client, const char *buf,
			    int len, const char *csid);
extern void debuglog(const char *fmt, ... )
  __attribute__ ((format(printf, 1, 2)));

int sync_lock(const char *resource, int mode, int flags, int *lockid);
int sync_unlock(const char *resource, int lockid);

#endif
