/* $NetBSD: conffile.c,v 1.7 2013/10/17 18:10:23 kefren Exp $ */

/*
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

#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "conffile.h"
#include "ldp_errors.h"

#define NextCommand(x) strsep(&x, " ")
#define LINEMAXSIZE 1024

extern int ldp_hello_time, ldp_keepalive_time, ldp_holddown_time, command_port,
	min_label, max_label, no_default_route, loop_detection;
int confh;
struct in_addr conf_ldp_id;

static int conf_dispatch(char*);
static int conf_readline(char*, size_t);
static int checkeol(char*);
static int Fhellotime(char*);
static int Fport(char*);
static int Fholddown(char*);
static int Fkeepalive(char*);
static int Fmaxlabel(char*);
static int Fminlabel(char*);
static int Fldpid(char*);
static int Fneighbour(char*);
static int Gneighbour(struct conf_neighbour *, char *);
static int Fnodefault(char*);
static int Floopdetection(char*);
static int Finterface(char*);
static int Ginterface(struct conf_interface *, char *);
static int Ipassive(struct conf_interface *, char *);
static int Itaddr(struct conf_interface *, char *);

struct conf_func {
	char com[64];
	int (* func)(char *);
};

struct intf_func {
	char com[64];
	int (* func)(struct conf_interface *, char *);
};

struct conf_func main_commands[] = {
	{ "hello-time", Fhellotime },
	{ "keepalive-time", Fkeepalive },
	{ "holddown-time", Fholddown },
	{ "command-port", Fport },
	{ "min-label", Fminlabel },
	{ "max-label", Fmaxlabel },
	{ "ldp-id", Fldpid },
	{ "neighbor", Fneighbour },
	{ "neighbour", Fneighbour },
	{ "no-default-route", Fnodefault },
	{ "loop-detection", Floopdetection },
	{ "interface", Finterface },
	{ "", NULL },
};

struct intf_func intf_commands[] = {
	{ "passive", Ipassive },
	{ "transport-address", Itaddr },
	{ "", NULL },
};

static int parseline;

/*
 * Parses config file
 */
int
conf_parsefile(const char *fname)
{
	char buf[LINEMAXSIZE + 1];

	SLIST_INIT(&conei_head);
	SLIST_INIT(&coifs_head);
	conf_ldp_id.s_addr = 0;

	confh = open(fname, O_RDONLY, 0);

	if (confh == -1)
		return E_CONF_IO;

	for (parseline = 1; conf_readline(buf, sizeof(buf)) >= 0; parseline++)
		if (conf_dispatch(buf) != 0) {
			close(confh);
			return parseline;
		}

	close(confh);
	return 0;
}

/*
 * Reads a line from config file
 */
int
conf_readline(char *buf, size_t bufsize)
{
	size_t i;

	for (i = 0; i < bufsize; i++) {
		if (read(confh, &buf[i], 1) != 1) {
			if (i == 0)
				return E_CONF_IO;
			break;
		}
		if (buf[i] == '\n')
			break;
		if (i == 0 && isspace((unsigned char)buf[i]) != 0) {
			i--;
			continue;
		}
	}
	if (i == bufsize)
		return E_CONF_MEM;
	buf[i] = '\0';
	return i;
}

/*
 * Looks for a matching command on a line
 */
int
conf_dispatch(char *line)
{
	int i, last_match = -1, matched = 0;
	char *command, *nline = line;

	if (strlen(line) == 0 || line[0] == '#')
		return E_CONF_OK;
	command = NextCommand(nline);
	for (i = 0; main_commands[i].func != NULL; i++)
		if (strncasecmp(main_commands[i].com, command,
		    strlen(main_commands[i].com)) == 0) {
			matched++;
			last_match = i;
		}
	if (matched == 0)
		return E_CONF_NOMATCH;
	else if (matched > 1)
		return E_CONF_AMBIGUOUS;

	if (checkeol(nline) != 0)
		return E_CONF_PARAM;
	return main_commands[last_match].func(nline);
}

/*
 * Checks if a line is terminated or else if it contains
 * a start block bracket. If it's semicolon terminated
 * then trim it.
 */
int
checkeol(char *line)
{
	size_t len = strlen(line);
	if (len > 0 && line[len - 1] == ';') {
		line[len - 1] = '\0';
		return 0;
	}
	for (size_t i = 0; i < len; i++)
		if (line[i] == '{')
			return 0;
	return -1;
}

/*
 * Sets hello time
 */
int
Fhellotime(char *line)
{
	int ht = atoi(line);
	if (ht <= 0)
		return E_CONF_PARAM;
	ldp_hello_time = ht;
	return 0;
}

/*
 * Sets command port
 */
int
Fport(char *line)
{
	int cp = atoi(line);
	if (cp <= 0 || cp > 65535)
		return E_CONF_PARAM;
	command_port = cp;
	return 0;
}

/*
 * Sets neighbour keepalive
 */
int
Fkeepalive(char *line)
{
	int kt = atoi(line);
	if (kt <= 0)
		return E_CONF_PARAM;
	ldp_keepalive_time = kt;
	return 0;
}

/*
 * Sets neighbour holddown timer
 */
int
Fholddown(char *line)
{
	int hdt = atoi(line);
	if (hdt <= 0)
		return E_CONF_PARAM;
	ldp_holddown_time = hdt;
	return 0;
}

int
Fminlabel(char *line)
{
	int ml = atoi(line);
	if (ml <= 0)
		return E_CONF_PARAM;
	min_label = ml;
	return 0;
}

int
Fmaxlabel(char *line)
{
	int ml = atoi(line);
	if (ml <= 0)
		return E_CONF_PARAM;
	max_label = ml;
	return 0;
}

int
Fldpid(char *line)
{
	if (inet_pton(AF_INET, line, &conf_ldp_id) != 1)
		return E_CONF_PARAM;
	return 0;
}

int
Fneighbour(char *line)
{
	char *peer;
	struct conf_neighbour *nei;
	struct in_addr ad;
	char buf[1024];

	peer = NextCommand(line);
	if (inet_pton(AF_INET, peer, &ad) != 1)
		return E_CONF_PARAM;

	nei = calloc(1, sizeof(*nei));
	if (nei == NULL)
		return E_CONF_MEM;
	nei->address.s_addr = ad.s_addr;
	SLIST_INSERT_HEAD(&conei_head, nei, neilist);

	while (conf_readline(buf, sizeof(buf)) >= 0) {
		parseline++;
		if (buf[0] == '}')
			return 0;
		if (Gneighbour(nei, buf) == -1)
			return -1;
	}
	return -1;
}

/*
 * neighbour { } sub-commands
 */
int
Gneighbour(struct conf_neighbour *nei, char *line)
{
	if (strncasecmp("authenticate", line, 12) == 0) {
		nei->authenticate = 1;
		return 0;
	}
	return -1;
}

int
Fnodefault(char *line)
{
	int nd = atoi(line);
	if (nd < 0)
		return E_CONF_PARAM;
	no_default_route = nd;
	return 0;
}

int
Floopdetection(char *line)
{
	int loopd = atoi(line);
	if (loopd < 0)
		return E_CONF_PARAM;
	loop_detection = loopd;
	return 0;
}

/*
 * Interface sub-commands
 */
int
Finterface(char *line)
{
	char *ifname;
	struct conf_interface *conf_if = calloc(1, sizeof(*conf_if));
	char buf[1024];

	ifname = NextCommand(line);
	if (conf_if == NULL || ifname == NULL)
		return -1;
	strlcpy(conf_if->if_name, ifname, IF_NAMESIZE);
	SLIST_INSERT_HEAD(&coifs_head, conf_if, iflist);

	while (conf_readline(buf, sizeof(buf)) >= 0) {
		parseline++;
		if (buf[0] == '}')
			return 0;
		if (Ginterface(conf_if, buf) == -1)
			return -1;
	}
	return -1;
}

int
Ginterface(struct conf_interface *conf_if, char *buf)
{
	int i;

	for (i = 0; intf_commands[i].func != NULL; i++)
		if (strncasecmp(buf, intf_commands[i].com,
		    strlen(intf_commands[i].com)) == 0)
			return intf_commands[i].func(conf_if, buf +
			    strlen(intf_commands[i].com) + 1);
	/* command not found */
	return -1;
}

/* sets transport address */
int
Itaddr(struct conf_interface *conf_if, char *buf)
{
	if (inet_pton(AF_INET, buf, &conf_if->tr_addr) != 1)
		return -1;
	return 0;
}

/* sets passive-interface on */
int
Ipassive(struct conf_interface *conf_if, char *buf)
{
	conf_if->passive = 1;
	return 0;
}
