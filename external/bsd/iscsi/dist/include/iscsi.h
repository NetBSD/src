/* $NetBSD: iscsi.h,v 1.3 2009/06/30 02:44:52 agc Exp $ */

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Alistair Crooks (agc@netbsd.org)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef ISCSI_H_
#define ISCSI_H_	1

enum {
	ISCSI_MAXSOCK = 8
};

/* variables which relate to an embedded iSCSI target */
typedef struct iscsi_target_t {
	int		sockc;			/* sockets where listening */
	int		sockv[ISCSI_MAXSOCK];	/* listening sockets */
	int		famv[ISCSI_MAXSOCK];	/* address families */
	int      	state;			/* current state of target */
	int		listener_pid;		/* PID */
	int		main_pid;		/* main IQN PID */
	volatile int    listener_listening;	/* a listener is listening? */
	void	       *lunv;			/* array of target devices */
	void	       *devv;			/* array of devices */
	void	       *extentv;		/* array of extents */
	uint32_t	last_tsih;		/* the last TSIH used */
	uint32_t	c;			/* # of vars */
	uint32_t	size;			/* size of var array */
	char	      **name;			/* variable names */
	char	      **value;			/* variable values */
} iscsi_target_t;

/* target functions */
int iscsi_target_set_defaults(iscsi_target_t *);
int iscsi_target_start(iscsi_target_t *);
int iscsi_target_listen(iscsi_target_t *);
int iscsi_target_shutdown(iscsi_target_t *);
void iscsi_target_write_pidfile(const char *);
int iscsi_target_setvar(iscsi_target_t *, const char *, const char *);
char *iscsi_target_getvar(iscsi_target_t *, const char *);
int iscsi_target_reconfigure(iscsi_target_t *);

typedef struct iscsi_initiator_t {
	uint32_t	c;			/* # of vars */
	uint32_t	size;			/* size of var array */
	char	      **name;			/* variable names */
	char	      **value;			/* variable values */
} iscsi_initiator_t;

int iscsi_initiator_set_defaults(iscsi_initiator_t *);
int iscsi_initiator_start(iscsi_initiator_t *);
int iscsi_initiator_info(char *, int, int);
int iscsi_initiator_shutdown(void);
int iscsi_initiator_discover(char *, uint64_t, int);

int iscsi_initiator_setvar(iscsi_initiator_t *, const char *, const char *);
char *iscsi_initiator_getvar(iscsi_initiator_t *, const char *);

#endif
