/*      $NetBSD: usbhidaction.c,v 1.29 2018/05/15 01:41:29 jmcneill Exp $ */

/*
 * Copyright (c) 2000, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson <lennart@augustsson.net>.
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
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: usbhidaction.c,v 1.29 2018/05/15 01:41:29 jmcneill Exp $");
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <dev/usb/usb.h>
#include <dev/hid/hid.h>
#include <usbhid.h>
#include <util.h>
#include <syslog.h>
#include <signal.h>
#include <util.h>

static int verbose = 0;
static int isdemon = 0;
static int reparse = 0;

struct command {
	struct command *next;
	int line;

	struct hid_item item;
	int value;
	char anyvalue;
	char *name;
	char *action;
};
static struct command *commands;

#define SIZE 4000

static void usage(void) __dead;
static struct command *parse_conf(const char *, report_desc_t, int, int);
static void docmd(struct command *, int, const char *, int, char **);
static void freecommands(struct command *);

static void
/*ARGSUSED*/
sighup(int sig)
{
	reparse = 1;
}

int
main(int argc, char **argv)
{
	const char *conf = NULL;
	const char *dev = NULL;
	int fd, ch, sz, n, val, i;
	int demon, ignore;
	report_desc_t repd;
	char buf[100];
	char devnamebuf[PATH_MAX];
	struct command *cmd;
	int reportid;
	const char *table = NULL;
	const char *pidfn = NULL;

	setprogname(argv[0]);
	(void)setlinebuf(stdout);

	demon = 1;
	ignore = 0;
	while ((ch = getopt(argc, argv, "c:df:ip:t:v")) != -1) {
		switch(ch) {
		case 'c':
			conf = optarg;
			break;
		case 'd':
			demon ^= 1;
			break;
		case 'i':
			ignore++;
			break;
		case 'f':
			dev = optarg;
			break;
		case 'p':
			pidfn = optarg;
			break;
		case 't':
			table = optarg;
			break;
		case 'v':
			demon = 0;
			verbose++;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (conf == NULL || dev == NULL)
		usage();

	hid_init(table);

	if (dev[0] != '/') {
		(void)snprintf(devnamebuf, sizeof(devnamebuf), "/dev/%s%s",
		     isdigit((unsigned char)dev[0]) ? "uhid" : "", dev);
		dev = devnamebuf;
	}

	if (demon && conf[0] != '/')
		errx(EXIT_FAILURE,
		    "config file must have an absolute path, %s", conf);

	fd = open(dev, O_RDWR | O_CLOEXEC);
	if (fd < 0)
		err(EXIT_FAILURE, "%s", dev);

	if (ioctl(fd, USB_GET_REPORT_ID, &reportid) < 0)
		reportid = -1;
	repd = hid_get_report_desc(fd);
	if (repd == NULL)
		err(EXIT_FAILURE, "hid_get_report_desc() failed");

	commands = parse_conf(conf, repd, reportid, ignore);

	sz = hid_report_size(repd, hid_input, reportid);

	if (verbose)
		(void)printf("report size %d\n", sz);
	if ((size_t)sz > sizeof(buf))
		errx(EXIT_FAILURE, "report too large");

	(void)signal(SIGHUP, sighup);

	if (demon) {
		if (daemon(0, 0) < 0)
			err(EXIT_FAILURE, "daemon()");
		(void)pidfile(pidfn);
		isdemon = 1;
	}

	for(;;) {
		n = read(fd, buf, (size_t)sz);
		if (verbose > 2) {
			(void)printf("read %d bytes:", n);
			for (i = 0; i < n; i++)
				(void)printf(" %02x", buf[i]);
			(void)printf("\n");
		}
		if (n < 0) {
			if (verbose)
				err(EXIT_FAILURE, "read");
			else
				exit(EXIT_FAILURE);
		}
#if 0
		if (n != sz) {
			err(EXIT_FAILURE, "read size");
		}
#endif
		for (cmd = commands; cmd; cmd = cmd->next) {
			val = hid_get_data(buf, &cmd->item);
			if (cmd->value == val || cmd->anyvalue)
				docmd(cmd, val, dev, argc, argv);
		}
		if (reparse) {
			struct command *cmds =
			    parse_conf(conf, repd, reportid, ignore);
			if (cmds) {
				freecommands(commands);
				commands = cmds;
			}
			reparse = 0;
		}
	}
}

static void
usage(void)
{

	(void)fprintf(stderr, "usage: %s -c config_file [-d] -f hid_dev "
		"[-i] [-p pidfile] [-t table] [-v]\n", getprogname());
	exit(EXIT_FAILURE);
}

static int
peek(FILE *f)
{
	int c;

	c = getc(f);
	if (c != EOF)
		(void)ungetc(c, f);
	return c;
}

static struct command *
parse_conf(const char *conf, report_desc_t repd, int reportid, int ignore)
{
	FILE *f;
	char *p;
	int line;
	char buf[SIZE], name[SIZE], value[SIZE], action[SIZE];
	char usagestr[SIZE], coll[SIZE];
	struct command *cmd, *cmds;
	struct hid_data *d;
	struct hid_item h;
	int u, lo, hi, range;
	
	f = fopen(conf, "r");
	if (f == NULL)
		err(EXIT_FAILURE, "%s", conf);

	cmds = NULL;
	for (line = 1; ; line++) {
		if (fgets(buf, sizeof buf, f) == NULL)
			break;
		if (buf[0] == '#' || buf[0] == '\n')
			continue;
		p = strchr(buf, '\n');
		while (p && isspace(peek(f))) {
			if (fgets(p, (int)(sizeof buf - strlen(buf)), f)
			    == NULL)
				break;
			p = strchr(buf, '\n');
		}
		if (p)
			*p = '\0';
		/* XXX SIZE == 4000 */
		if (sscanf(buf, "%3999s %3999s %[^\n]", name, value, action) != 3) {
			if (isdemon) {
				syslog(LOG_WARNING, "config file `%s', line %d"
				       ", syntax error: %s", conf, line, buf);
				freecommands(cmds);
				(void)fclose(f);
				return (NULL);
			} else {
				errx(EXIT_FAILURE, "config file `%s', line %d,"
				     ", syntax error: %s", conf, line, buf);
			}
		}

		cmd = emalloc(sizeof *cmd);
		cmd->next = cmds;
		cmds = cmd;
		cmd->line = line;

		if (strcmp(value, "*") == 0) {
			cmd->anyvalue = 1;
		} else {
			cmd->anyvalue = 0;
			if (sscanf(value, "%d", &cmd->value) != 1) {
				if (isdemon) {
					syslog(LOG_WARNING,
					       "config file `%s', line %d, "
					       "bad value: %s\n",
					       conf, line, value);
					freecommands(cmds);
					(void)fclose(f);
					return (NULL);
				} else {
					errx(EXIT_FAILURE,
					    "config file `%s', line %d, "
					    "bad value: %s\n",
					    conf, line, value);
				}
			}
		}

		coll[0] = 0;
		for (d = hid_start_parse(repd, 1 << hid_input, reportid);
		     hid_get_item(d, &h); ) {
			if (verbose > 2)
				(void)printf("kind=%d usage=%x flags=%x\n",
				       h.kind, h.usage, h.flags);
			switch (h.kind) {
			case hid_input:
				if (h.flags & HIO_CONST)
					continue;
				if (h.usage_minimum != 0 ||
				    h.usage_maximum != 0) {
					lo = h.usage_minimum;
					hi = h.usage_maximum;
					range = 1;
				} else {
					lo = h.usage;
					hi = h.usage;
					range = 0;
				}
				for (u = lo; u <= hi; u++) {
					(void)snprintf(usagestr,
					    sizeof usagestr,
					    "%s:%s",
					    hid_usage_page((int)HID_PAGE(u)), 
					    hid_usage_in_page((u_int)u));
					if (verbose > 2)
						(void)printf("usage %s\n",
						    usagestr);
					if (!strcasecmp(usagestr, name))
						goto foundhid;
					if (coll[0]) {
						(void)snprintf(usagestr,
						    sizeof usagestr,
						    "%s.%s:%s", coll + 1,
						    hid_usage_page((int)HID_PAGE(u)),
						    hid_usage_in_page((u_int)u));
						if (verbose > 2)
							(void)printf(
							    "usage %s\n",
							    usagestr);
						if (!strcasecmp(usagestr, name))
							goto foundhid;
					}
				}
				break;
			case hid_collection:
				(void)snprintf(coll + strlen(coll),
				    sizeof coll - strlen(coll),  ".%s:%s",
				    hid_usage_page((int)HID_PAGE(h.usage)), 
				    hid_usage_in_page(h.usage));
				if (verbose > 2)
					(void)printf("coll '%s'\n", coll);
				break;
			case hid_endcollection:
				if (coll[0])
					*strrchr(coll, '.') = 0;
				break;
			default:
				break;
			}
		}
		if (ignore) {
			if (verbose)
				warnx("ignore item '%s'", name);
			continue;
		}
		if (isdemon) {
			syslog(LOG_WARNING, "config file `%s', line %d, HID "
			       "item not found: `%s'", conf, line, name);
			freecommands(cmds);
			(void)fclose(f);
			return (NULL);
		} else {
			errx(EXIT_FAILURE, "config file `%s', line %d,"
			    " HID item not found: `%s'", conf, line, name);
		}

	foundhid:
		hid_end_parse(d);
		cmd->item = h;
		cmd->name = estrdup(name);
		cmd->action = estrdup(action);
		if (range) {
			if (cmd->value == 1)
				cmd->value = u - lo;
			else
				cmd->value = -1;
		}

		if (verbose) {
			char valuebuf[16];

			if (cmd->anyvalue)
				snprintf(valuebuf, sizeof(valuebuf), "%s", "*");
			else
				snprintf(valuebuf, sizeof(valuebuf), "%d",
				    cmd->value);
			(void)printf("PARSE:%d %s, %s, '%s'\n", cmd->line, name,
				valuebuf, cmd->action);
		}
	}
	(void)fclose(f);
	return (cmds);
}

static void
docmd(struct command *cmd, int value, const char *hid, int argc, char **argv)
{
	char cmdbuf[SIZE], *p, *q;
	size_t len;
	int n, r;

	for (p = cmd->action, q = cmdbuf; *p && q < &cmdbuf[SIZE-1]; ) {
		if (*p == '$') {
			p++;
			len = &cmdbuf[SIZE-1] - q;
			if (isdigit((unsigned char)*p)) {
				n = strtol(p, &p, 10) - 1;
				if (n >= 0 && n < argc) {
					(void)strlcpy(q, argv[n], len);
					q += strlen(q);
				}
			} else if (*p == 'V') {
				p++;
				(void)snprintf(q, len, "%d", value);
				q += strlen(q);
			} else if (*p == 'N') {
				p++;
				(void)strlcpy(q, cmd->name, len);
				q += strlen(q);
			} else if (*p == 'H') {
				p++;
				(void)strlcpy(q, hid, len);
				q += strlen(q);
			} else if (*p) {
				*q++ = *p++;
			}
		} else {
			*q++ = *p++;
		}
	}
	*q = 0;

	if (verbose)
		(void)printf("system '%s'\n", cmdbuf);
	r = system(cmdbuf);
	if (verbose > 1 && r)
		(void)printf("return code = 0x%x\n", r);
}

static void
freecommand(struct command *cmd)
{
	free(cmd->name);
	free(cmd->action);
	free(cmd);
}

static void
freecommands(struct command *cmd)
{
	struct command *next;

	while (cmd) {
		next = cmd->next;
		freecommand(cmd);
		cmd = next;
	}
}
