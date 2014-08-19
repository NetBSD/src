/* $NetBSD: conffile.c,v 1.3.8.3 2014/08/20 00:05:09 tls Exp $ */

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
#include <sys/mman.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "conffile.h"
#include "ldp_errors.h"

#define NextCommand(x) strsep(&x, " ")
#define LINEMAXSIZE 1024

char *mapped, *nextline;
size_t mapsize;

extern int ldp_hello_time, ldp_keepalive_time, ldp_holddown_time, command_port,
	min_label, max_label, no_default_route, loop_detection;
struct in_addr conf_ldp_id;

static int conf_dispatch(char*);
static char * conf_getlinelimit(void);
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
	char line[LINEMAXSIZE+1];
	struct stat fs;

	SLIST_INIT(&conei_head);
	SLIST_INIT(&coifs_head);
	conf_ldp_id.s_addr = 0;

	int confh = open(fname, O_RDONLY, 0);

	if (confh == -1 || fstat(confh, &fs) == -1 ||
	    (mapped = mmap(NULL, fs.st_size, PROT_READ, MAP_SHARED, confh, 0))
	    == MAP_FAILED) {
		if (confh != -1)
			close(confh);
		return E_CONF_IO;
	}

	mapsize = fs.st_size;
	nextline = mapped;
	for (parseline = 1; ; parseline++) {
		char *prev = nextline;
		if ((nextline = conf_getlinelimit()) == NULL)
			break;
		while (isspace((int)*prev) != 0 && prev < nextline)
			prev++;
		if (nextline - prev < 2)
			continue;
		else if (nextline - prev > LINEMAXSIZE)
			goto parerr;
		memcpy(line, prev, nextline - prev);
		if (line[0] == '#')
			continue;
		else
			line[nextline - prev] = '\0';
		if (conf_dispatch(line) != 0)
			goto parerr;
	}
	munmap(mapped, mapsize);
	close(confh);
	return 0;
parerr:
	munmap(mapped, mapsize);
	close(confh);
	return parseline;
}

char *
conf_getlinelimit(void)
{
	char *p = nextline;

	if (nextline < mapped || (size_t)(nextline - mapped) >= mapsize)
		return NULL;

	for (p = nextline; *p != '\n' && (size_t)(p - mapped) < mapsize; p++);
	return p + 1;
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

	if (nline == NULL || checkeol(nline) != 0)
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
	if (len > 0 && line[len - 1] == '\n') {
		line[len - 1] = '\0';
		len--;
	}
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
	char buf[LINEMAXSIZE];

	peer = NextCommand(line);
	if (inet_pton(AF_INET, peer, &ad) != 1)
		return E_CONF_PARAM;

	nei = calloc(1, sizeof(*nei));
	if (nei == NULL)
		return E_CONF_MEM;
	nei->address.s_addr = ad.s_addr;
	SLIST_INSERT_HEAD(&conei_head, nei, neilist);

	for ( ; ; ) {
		char *prev = nextline;
		parseline++;
		nextline = conf_getlinelimit();
		if (nextline == NULL || (size_t)(nextline - prev) > LINEMAXSIZE)
			return -1;
		while (isspace((int)*prev) != 0 && prev < nextline)
			prev++;
		memcpy(buf, prev, nextline - prev);
		if (nextline - prev < 2 || buf[0] == '#')
			continue;
		else if (buf[0] == '}')
			break;
		else
			buf[nextline - prev] = '\0';
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
	struct conf_interface *conf_if;
	char buf[LINEMAXSIZE];

	if ((ifname = NextCommand(line)) == NULL)
		return -1;
	if ((conf_if = calloc(1, sizeof(*conf_if))) == NULL)
		return -1;

	strlcpy(conf_if->if_name, ifname, IF_NAMESIZE);
	SLIST_INSERT_HEAD(&coifs_head, conf_if, iflist);

	for ( ; ; ) {
		char *prev = nextline;
		parseline++;
		nextline = conf_getlinelimit();
		if (nextline == NULL || (size_t)(nextline - prev) > LINEMAXSIZE)
			return -1;
		while (isspace((int)*prev) != 0 && prev < nextline)
			prev++;
		memcpy(buf, prev, nextline - prev);
		if (nextline - prev < 2 || buf[0] == '#')
			continue;
		else if (buf[0] == '}')
			break;
		else
			buf[nextline - prev] = '\0';
		if (Ginterface(conf_if, buf) == -1)
			return -1;
	}
	return 0;
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
