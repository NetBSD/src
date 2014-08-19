/* $NetBSD: ldp_command.c,v 1.7.6.2 2014/08/20 00:05:09 tls Exp $ */

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mihai Chelaru <kefren@NetBSD.org>
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

#include <arpa/inet.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <sys/socket.h>
#include <sys/queue.h>

#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "label.h"
#include "ldp.h"
#include "ldp_command.h"
#include "ldp_errors.h"
#include "ldp_peer.h"
#include "socketops.h"

struct com_sock csockets[MAX_COMMAND_SOCKETS];
extern int ldp_hello_time, ldp_keepalive_time, ldp_holddown_time,
	min_label, max_label, debug_f, warn_f;

#define	writestr(soc, str) write(soc, str, strlen(str))

#define	MAXSEND 1024
char sendspace[MAXSEND];

static void send_prompt(int);
static void send_pwd_prompt(int);
static int command_match(struct com_func*, int, char*, char*);

static int	verify_root_pwd(char *);
static void	echo_on(int s);
static void	echo_off(int s);

/* Main functions */
static int show_func(int, char *);
static int set_func(int, char *);
static int exit_func(int, char *);
 
/* Show functions */
static int show_debug(int, char *);
static int show_hellos(int, char *);
static int show_parameters(int, char *);
static int show_version(int, char *);
static int show_warning(int, char *);

/* Set functions */
static int set_hello_time(int, char *);
static int set_debug(int, char *);
static int set_warning(int, char *);

static struct com_func main_commands[] = {
	{ "show", show_func },
	{ "set", set_func },
	{ "quit", exit_func },
	{ "exit", exit_func },
	{ "", NULL }
};

static struct com_func show_commands[] = {
	{ "neighbours", show_neighbours },
	{ "bindings", show_bindings },
	{ "debug", show_debug },
	{ "hellos", show_hellos },
	{ "labels", show_labels },
	{ "parameters", show_parameters },
	{ "version", show_version },
	{ "warning", show_warning },
	{ "", NULL }
};

struct com_func set_commands[] = {
	{ "debug", set_debug },
	{ "hello-time", set_hello_time },
	{ "warning", set_warning },
	{ "", NULL }
};

static int
verify_root_pwd(char *pw)
{
	struct passwd *p;

	if ((p = getpwuid(0)) == NULL)
		return 0;

	if (strcmp(crypt(pw, p->pw_passwd), p->pw_passwd))
		return 0;

	return 1;
}


void
init_command_sockets()
{
	int i;

	for (i = 0; i<MAX_COMMAND_SOCKETS; i++) {
		csockets[i].socket = -1;
		csockets[i].auth = 0;
	}
}

int
create_command_socket(int port)
{
	struct sockaddr_in sin;
	int s;

	sin.sin_len = sizeof(sin);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);

	s = socket(PF_INET, SOCK_STREAM, 6);
	if (s < 0)
		return s;

	if (bind(s, (struct sockaddr *) &sin, sizeof(sin))) {
		fatalp("bind: %s", strerror(errno));
		close(s);
		return -1;
	}

	if (listen(s, 5) == -1) {
		fatalp("listen: %s", strerror(errno));
		close(s);
		return -1;
	}
	debugp("Command socket created (%d)\n", s);
	return s;
}

void
command_accept(int s)
{
	int as = accept(s, NULL, 0);

	if (as < 0) {
		fatalp("Cannot accept new command socket %s",
		    strerror(errno));
		return;
	}

	if (add_command_socket(as) != 0) {
		fatalp("Cannot accept command. Too many connections\n");
		close(as);
		return;
	}

	/* auth */
	send_pwd_prompt(as);
}

struct com_sock *
is_command_socket(int s)
{
	int i;

	if (s == -1)
		return NULL;
	for (i=0; i<MAX_COMMAND_SOCKETS; i++)
		if (s == csockets[i].socket)
			return &csockets[i];
	return NULL;
}

int
add_command_socket(int s)
{
	int i;

	for (i=0; i<MAX_COMMAND_SOCKETS; i++)
		if (csockets[i].socket == -1) {
			csockets[i].socket = s;
			csockets[i].auth = 0;
			return 0;
		}
	return -1;
}

void
command_dispatch(struct com_sock *cs)
{
	char recvspace[MAX_COMMAND_SIZE + 1];
	char *nextc = recvspace;
	int r = recv(cs->socket, recvspace, MAX_COMMAND_SIZE, MSG_PEEK);

	if (r < 0) {
		command_close(cs->socket);
		return;
	}

	recv(cs->socket, recvspace, r, MSG_WAITALL);

	if (r < 3) { /*at least \r\n */
	    if (cs->auth) {
		/*writestr(cs->socket, "Unknown command. Use ? for help\n");*/
		send_prompt(cs->socket);
	    } else {
		writestr(cs->socket, "Bad password\n");
		command_close(cs->socket);
	    }
		return;
	}

	recvspace[r - 2] = '\0';

	if (!cs->auth) {
		if (verify_root_pwd(recvspace)) {
			echo_on(cs->socket);
			cs->auth = 1;
			writestr(cs->socket, "\n");
			send_prompt(cs->socket);
		} else {
			echo_on(cs->socket);
			writestr(cs->socket, "Bad password\n");
			command_close(cs->socket);
		}
		return;
	}

	strsep(&nextc, " ");

	command_match(main_commands, cs->socket, recvspace, nextc);

}

void
command_close(int s)
{
	int i;

	for (i=0; i<MAX_COMMAND_SOCKETS; i++)
		if (s == csockets[i].socket) {
			close(s);
			csockets[i].socket = -1;
			csockets[i].auth = 0;
			break;
		}
}

static void
send_prompt(int s) {
	writestr(s, "LDP> ");
}

static void
send_pwd_prompt(int s) {
	echo_off(s);
	writestr(s, "Password: ");
}

static void
echo_off(int s)
{
	char iac_will_echo[3] = { 0xff, 0xfb, 0x01 }, bf[32];
	write(s, iac_will_echo, sizeof(iac_will_echo));
	read(s, bf, sizeof(bf));
}

static void
echo_on(int s)
{
	char iac_wont_echo[3] = { 0xff, 0xfc, 0x01 }, bf[32];
	write(s, iac_wont_echo, sizeof(iac_wont_echo));
	read(s, bf, sizeof(bf));
}

/*
 * Matching function
 * Returns 1 if matched anything
 */
static int
command_match(struct com_func *cf, int s, char *orig, char *next)
{
	size_t i, len;
	int last_match = -1;
	const char *msg = NULL;

	if (orig == NULL || orig[0] == '\0')
		goto out;

	if (!strcmp(orig, "?")) {
		for (i = 0; cf[i].func != NULL; i++) {
			snprintf(sendspace, MAXSEND, "\t%s\n", cf[i].com);
			writestr(s, sendspace);
		}
		goto out;
	}

	len = strlen(orig);
	for (i = 0; cf[i].func != NULL; i++) {
		if (strncasecmp(orig, cf[i].com, len) == 0) {
			if (last_match != -1) {
				msg = "Ambiguous";
				goto out;
			} else
				last_match = i;
		}
	}

	if (last_match == -1) {
		msg = "Unknown";
		goto out;
	}

	if (cf[last_match].func(s, next) != 0)
		send_prompt(s);
	return 1;
out:
	if (msg) {
		writestr(s, msg);
		writestr(s, " command. Use ? for help\n");
	}
	send_prompt(s);
	return 0;
}

/*
 * Main CLI functions
 */
static int
set_func(int s, char *recvspace)
{
	char *nextc = recvspace;

	if (recvspace == NULL || recvspace[0] == '\0') {
		writestr(s, "Unknown set command. Use set ? for help\n");
		return 1;
	}

	strsep(&nextc, " ");

	command_match(set_commands, s, recvspace, nextc);
	return 0;
}

static int
show_func(int s, char *recvspace)
{
	char *nextc = recvspace;

	if (recvspace == NULL || recvspace[0] == '\0') {
		writestr(s, "Unknown show command. Use show ? for help\n");
		return 1;
	}

	strsep(&nextc, " ");

	command_match(show_commands, s, recvspace, nextc);
	return 0;
}

static int
exit_func(int s, char *recvspace)
{
	command_close(s);
	return 0;
}

/*
 * Show functions
 */
int
show_neighbours(int s, char *recvspace)
{
	struct ldp_peer *p;
	struct ldp_peer_address *wp;
	struct sockaddr_in ssin;
	socklen_t sin_len = sizeof(struct sockaddr_in);
	int enc;
	socklen_t enclen = sizeof(enc);

	SLIST_FOREACH(p, &ldp_peer_head, peers) {
		snprintf(sendspace, MAXSEND, "LDP peer: %s\n",
		    inet_ntoa(p->ldp_id));
		writestr(s, sendspace);
		snprintf(sendspace, MAXSEND, "Transport address: %s\n",
		    satos(p->transport_address));
		writestr(s, sendspace);
		snprintf(sendspace, MAXSEND, "Next-hop address: %s\n",
		    satos(p->address));
		writestr(s, sendspace);
		snprintf(sendspace, MAXSEND, "State: %s\n",
		    ldp_state_to_name(p->state));
		writestr(s, sendspace);
		if (p->state == LDP_PEER_ESTABLISHED) {
			snprintf(sendspace, MAXSEND, "Since: %s",
			    ctime(&p->established_t));
			writestr(s, sendspace);
		}
		snprintf(sendspace, MAXSEND, "Holdtime: %d\nTimeout: %d\n",
			p->holdtime, p->timeout);
		writestr(s, sendspace);

		switch(p->state) {
		    case LDP_PEER_CONNECTING:
		    case LDP_PEER_CONNECTED:
		    case LDP_PEER_ESTABLISHED:
			if (getsockname(p->socket,(struct sockaddr *) &ssin,
			    &sin_len))
				break;

			if (getsockopt(p->socket, IPPROTO_TCP, TCP_MD5SIG,
			    &enc, &enclen) == 0) {
				snprintf(sendspace, MAXSEND,
				    "Authenticated: %s\n",
				    enc != 0 ? "YES" : "NO");
				writestr(s, sendspace);
			}

			snprintf(sendspace, MAXSEND,"Socket: %d\nLocal %s:%d\n",
			    p->socket, inet_ntoa(ssin.sin_addr),
			    ntohs(ssin.sin_port));
			writestr(s, sendspace);

			if (getpeername(p->socket,(struct sockaddr *) &ssin,
			    &sin_len))
				break;
			snprintf(sendspace, MAXSEND, "Remote %s:%d\n",
				inet_ntoa(ssin.sin_addr), ntohs(ssin.sin_port));
			writestr(s, sendspace);
		}

		snprintf(sendspace, MAXSEND,"Addresses bounded to this peer: ");
		writestr(s, sendspace);
		SLIST_FOREACH(wp, &p->ldp_peer_address_head, addresses) {
			/* XXX: TODO */
			if (wp->address.sa.sa_family != AF_INET)
				continue;
			snprintf(sendspace, MAXSEND, "%s ",
			    inet_ntoa(wp->address.sin.sin_addr));
			writestr(s, sendspace);
		}
		sendspace[0] = sendspace[1] = '\n';
		write(s, sendspace, 2);
	}       
	return 1;
}

/* Shows labels grabbed from unsolicited label maps */
int
show_labels(int s, char *recvspace)
{
	struct ldp_peer *p;
	struct label_mapping *lm = NULL;

	SLIST_FOREACH(p, &ldp_peer_head, peers) {
		if (p->state != LDP_PEER_ESTABLISHED)
			continue;
		while ((lm = ldp_peer_lm_right(p, lm)) != NULL) {
			char lma[256];
			/* XXX: TODO */
			if (lm->address.sa.sa_family != AF_INET)
				continue;
			strlcpy(lma, inet_ntoa(lm->address.sin.sin_addr),
			    sizeof(lma));
			snprintf(sendspace, MAXSEND, "%s:%d\t%s/%d\n",
			    inet_ntoa(p->ldp_id), lm->label, lma, lm->prefix);
			writestr(s, sendspace);
		}
	}
	return 1;
}

int
show_bindings(int s, char *recvspace)
{
	struct label *l = NULL;

	snprintf(sendspace, MAXSEND, "Local label\tNetwork\t\t\t\tNexthop\n");
	writestr(s, sendspace);
	while((l = label_get_right(l)) != NULL) {
		snprintf(sendspace, MAXSEND, "%d\t\t%s/", l->binding,
		    satos(&l->so_dest.sa));
		writestr(s, sendspace);
		snprintf(sendspace, MAXSEND, "%s", satos(&l->so_pref.sa));
		writestr(s, sendspace);
		if (l->p)
			snprintf(sendspace, MAXSEND, "\t%s:%d\n",
			    satos(l->p->address), l->label);
		else
			snprintf(sendspace, MAXSEND, "\n");
		writestr(s, sendspace);
	}
	return 1;
}

static int
show_debug(int s, char *recvspace)
{
	if (recvspace) {
		writestr(s, "Invalid command\n");
		return 1;
	}

	snprintf(sendspace, MAXSEND, "Debug: %s\n",
		debug_f ? "YES" : "NO");
	writestr(s, sendspace);
	return 1;
}

static int
show_hellos(int s, char *recvspace)
{
	struct hello_info *hi;

	SLIST_FOREACH(hi, &hello_info_head, infos) {
		snprintf(sendspace, MAXSEND,
		    "ID: %s\nKeepalive: %ds\nTransport address: %s\n\n",
		    inet_ntoa(hi->ldp_id),
		    hi->keepalive,
		    hi->transport_address.sa.sa_family != 0 ?
		    satos(&hi->transport_address.sa) : "None");
		writestr(s, sendspace);
	}
	return 1;
}

static int
show_parameters(int s, char *recvspace)
{
	snprintf(sendspace, MAXSEND, "LDP ID: %s\nProtocol version: %d\n"
		 "Hello time: %d\nKeepalive time: %d\nHoldtime: %d\n"
		 "Minimum label: %d\nMaximum label: %d\n",
		my_ldp_id,
		LDP_VERSION,
		ldp_hello_time,
		ldp_keepalive_time,
		ldp_holddown_time,
		min_label,
		max_label);
	writestr(s, sendspace);
	return 1;
}

static int
show_version(int s, char *recvspace)
{
	if (recvspace) {	/* Nothing more after this */
		writestr(s, "Invalid command\n");
		return 1;
	}

	snprintf(sendspace, MAXSEND, "NetBSD LDP daemon version: %s\n",
	    LDPD_VER);
	writestr(s, sendspace);
	return 1;
}

static int
show_warning(int s, char *recvspace)
{
	if (recvspace) {
		writestr(s, "Invalid command\n");
		return 1;
	}

	snprintf(sendspace, MAXSEND, "Warnings: %s\n",
		warn_f ? "YES" : "NO");
	writestr(s, sendspace);
	return 1;
}

/* Set commands */
static int
set_hello_time(int s, char *recvspace)
{
	if (!recvspace || atoi(recvspace) < 1) {
		writestr(s, "Invalid timeout\n");
		return 1;
	}

	ldp_hello_time = atoi(recvspace);
	return 1;
}

static int
set_debug(int s, char *recvspace)
{
	if (!recvspace || atoi(recvspace) < 0) {
		writestr(s, "Invalid command\n");
		return 1;
	}

	debug_f = atoi(recvspace);
	return 1;
}

static int
set_warning(int s, char *recvspace)
{
	if (!recvspace || atoi(recvspace) < 0) {
		writestr(s, "Invalid command\n");
		return 1;
	}

	warn_f = atoi(recvspace);
	return 1;
}
