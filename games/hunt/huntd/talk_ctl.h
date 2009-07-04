/*	$NetBSD: talk_ctl.h,v 1.7 2009/07/04 04:29:54 dholland Exp $	*/

/*
 * Copyright (c) 1983-2003, Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met:
 * 
 * + Redistributions of source code must retain the above copyright 
 *   notice, this list of conditions and the following disclaimer.
 * + Redistributions in binary form must reproduce the above copyright 
 *   notice, this list of conditions and the following disclaimer in the 
 *   documentation and/or other materials provided with the distribution.
 * + Neither the name of the University of California, San Francisco nor 
 *   the names of its contributors may be used to endorse or promote 
 *   products derived from this software without specific prior written 
 *   permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <netinet/in.h>

#ifdef TALK_43
#include <protocols/talkd.h>
#else

#include <sys/socket.h>

#define NAME_SIZE	9
#define TTY_SIZE	16
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	256
#endif

#define MAX_LIFE	60	/* max time daemon saves invitations */
/* RING_WAIT should be 10's of seconds less than MAX_LIFE */
#define RING_WAIT	30	/* time to wait before refreshing invitation */

/* type values */
#define LEAVE_INVITE	0
#define LOOK_UP		1
#define DELETE		2
#define ANNOUNCE	3

/* answer values */
#define SUCCESS		0
#define NOT_HERE	1
#define FAILED		2
#define MACHINE_UNKNOWN	3
#define PERMISSION_DENIED 4
#define UNKNOWN_REQUEST	5

typedef struct ctl_response {
	char	type;
	char	answer;
	int	id_num;
	struct	sockaddr_in addr;
} CTL_RESPONSE;

typedef struct ctl_msg {
	char	type;
	char	l_name[NAME_SIZE];
	char	r_name[NAME_SIZE];
	int	id_num;
	int	pid;
	char	r_tty[TTY_SIZE];
	struct	sockaddr_in addr;
	struct	sockaddr_in ctl_addr;
} CTL_MSG;
#endif

#include <errno.h>
#ifdef LOG
#include <syslog.h>
#endif

extern struct sockaddr_in daemon_addr;
extern struct sockaddr_in ctl_addr;
extern struct sockaddr_in my_addr;
extern struct in_addr my_machine_addr;
extern struct in_addr his_machine_addr;
extern u_short daemon_port;
extern int ctl_sockt;
extern CTL_MSG msg;

#ifdef LOG
#define p_error(str)	syslog(LOG_WARNING, "faketalk %s: %m", str)
#else
#define p_error(str)	warn(str)
#endif

void ctl_transact(struct in_addr, CTL_MSG, int, CTL_RESPONSE *);
