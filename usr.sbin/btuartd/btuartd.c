/*	$NetBSD: btuartd.c,v 1.2 2007/06/12 10:05:24 kiyohara Exp $	*/
/*
 * Copyright (c) 2006, 2007 KIYOHARA Takashi
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: btuartd.c,v 1.2 2007/06/12 10:05:24 kiyohara Exp $");

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <bluetooth.h>
#include <err.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>
#include <util.h>

#include <dev/bluetooth/btuart.h>

#include <netbt/hci.h>

#define BTUARTD_CONFFILE	"/etc/bluetooth/btuartd.conf"
#define BTUARTD_CONF_MIN	4

struct btuartd_conf {
	int comfd;
	int type;
	char comdev[PATH_MAX];
	int speed;
	int flow;
	int init_speed;
};

static void usage(void);
static int read_config(int, struct btuartd_conf **, char *);
static void btuartd_sigcaught(int);
static int btuart_setting(int, int, int, int, int);
static int btuartd(int, struct btuartd_conf *);


int btuartd_signal = 0;

static struct hcitypetbl {
	char *hciname;
	int hcitype;
} hcitypetbl[] = {
	{ "ericsson",		BTUART_HCITYPE_ERICSSON },
	{ "digi",		BTUART_HCITYPE_DIGI },
	{ "texas",		BTUART_HCITYPE_TEXAS },
	{ "csr",		BTUART_HCITYPE_CSR },
	{ "swave",		BTUART_HCITYPE_SWAVE },
	{ "st",			BTUART_HCITYPE_ST },
	{ "stlc2500",		BTUART_HCITYPE_STLC2500 },
	{ "bt2000c",		BTUART_HCITYPE_BT2000C },
	{ "bcm2035",		BTUART_HCITYPE_BCM2035 },
	{ "*",			BTUART_HCITYPE_ANY },
	{ NULL },
};


static void
usage()
{
	const char *progname = getprogname();

	printf("usage: %s [-f] [-i init_speed] [type] comdev speed\n",
	    progname);
	printf("       %s [-d] [-c conffile]\n", progname);
	printf("    comdev: com device. e.g. /dev/dty00\n");
	printf("    speed : baud rate. e.g. 115200\n");
	exit(EXIT_SUCCESS);
}

static int
btuart_setting(int comfd, int type, int speed, int flow, int init_speed)
{
	struct termios t;
	int rv;

	tcflush(comfd, TCIOFLUSH);
	if ((rv = tcgetattr(comfd, &t)) == -1) {
		syslog(LOG_ERR, "tcgetattr failed: %s", strerror(errno));
		return rv;
	}
	cfmakeraw(&t);
	t.c_cflag |= CLOCAL;
	if (flow)
		t.c_cflag |= CRTSCTS;

	tcflush(comfd, TCIOFLUSH);
	if ((rv = cfsetspeed(&t, speed)) == -1) {
		syslog(LOG_ERR, "cfsetspeed failed: %s", strerror(errno));
		return rv;
	}
	if ((rv = tcsetattr(comfd, TCSANOW, &t)) == -1) {
		syslog(LOG_ERR, "tcsetattr failed: %s", strerror(errno));
		return rv;
	}

	/* set btuart discipline */
	if ((rv = ioctl(comfd, TIOCSLINED, "btuart")) == -1) {
		syslog(LOG_ERR,
		    "ioctl TIOCSLINE btuart failed: %s", strerror(errno));
		return rv;
	}

	if (init_speed != B0)
		if ((rv = ioctl(comfd, BTUART_INITSPEED, &init_speed)) == -1) {
			syslog(LOG_ERR, "ioctl BTUART_INITSPEED failed: %s",
			    strerror(errno));
			return rv;
		}
	if ((rv = ioctl(comfd, BTUART_HCITYPE, &type)) == -1) {
		syslog(LOG_ERR, "ioctl BTUART_HCITYPE failed: %s",
		    strerror(errno));
		return rv;
	}
	if ((rv = ioctl(comfd, BTUART_START)) == -1)
		syslog(LOG_ERR, "ioctl BTUART_HCITYPE failed: %s",
		    strerror(errno));

	return rv;
}

static int
read_config(int onbtuart, struct btuartd_conf **btuartd_conf, char *conffile)
{
	FILE *fp;
	struct btuartd_conf *nbtuartd_conf;
	int nbtuart, threshold, i, j, n;
	char buf[256], *bufp, type[256];

	if ((fp = fopen(conffile, "r")) == NULL) {
		syslog(LOG_ERR, "fopen failed: %s", strerror(errno));
		return -1;
	}

	nbtuart = 0;
	for (threshold = BTUARTD_CONF_MIN;
	    threshold < onbtuart; threshold <<= 1);
	i = 0;
	while ((bufp = fgets(buf, sizeof(buf), fp)) != NULL) {
		i++;
		if (buf[0] == '#')	/* comment line */
			continue;
		if (buf[0] == '\n')	/* null line */
			continue;

		if (nbtuart == threshold) {
			threshold <<= 1;
			nbtuartd_conf = realloc(*btuartd_conf,
			    sizeof(struct btuartd_conf) * threshold);
			if (nbtuartd_conf == NULL) {
				syslog(LOG_ERR, "realloc failed: %s",
				    strerror(errno));
				nbtuart = -1;
				break;
			}
			*btuartd_conf = nbtuartd_conf;
		}

		/* comdev[PATH_MAX] size is larger then buf. */
		if (sscanf(buf, "btuart at %s type %255s speed %d %n",
		    (*btuartd_conf)[nbtuart].comdev, type,
		    &(*btuartd_conf)[nbtuart].speed, &n) < 3) {
			syslog(LOG_ERR, "format error: line %d", i);
			nbtuart = -1;
			break;
		}
		for (j = 0; hcitypetbl[j].hciname != NULL; j++)
			if (!strncmp(hcitypetbl[j].hciname, type, strlen(type)))
				break;
		if (hcitypetbl[j].hciname == NULL) {
			syslog(LOG_ERR, "unknwon type: line %d: type '%s'",
			    i, type);
			nbtuart = -1;
			break;
		}
		(*btuartd_conf)[nbtuart].type = hcitypetbl[j].hcitype;
		(*btuartd_conf)[nbtuart].flow = 0;
		(*btuartd_conf)[nbtuart].init_speed = B0;

		while (1 /* CONSTCOND */) {
			if (buf[n] == '#' || buf[n] == '\n' || buf[n] == '\0')
				break;

			if (strstr(&buf[n], "flow") == &buf[n])
				(*btuartd_conf)[nbtuart].flow = 1;
			else if (strstr(&buf[n], "init_speed") == &buf[n]) {
				n += strcspn(&buf[n], " \t");
				n += strspn(&buf[n], " \t");
				(*btuartd_conf)[nbtuart].init_speed =
				    atoi(&buf[n]);
			} else {
				syslog(LOG_ERR, "unknwon word '%s': line %d",
				    &buf[n], i);
				nbtuart = -1;
				break;
			}

			n += strcspn(&buf[n], " \t");
			n += strspn(&buf[n], " \t");
		}
		if (nbtuart == -1)
			break;
		nbtuart++;
	}
	if (nbtuart < onbtuart && threshold > BTUARTD_CONF_MIN) {
		for (threshold = BTUARTD_CONF_MIN;
		    threshold < nbtuart; threshold <<= 1);
		nbtuartd_conf = realloc(*btuartd_conf,
		    sizeof(struct btuartd_conf) * threshold);
		if (nbtuartd_conf == NULL) {
			syslog(LOG_ERR, "realloc failed: %s",
			    strerror(errno));
			nbtuart = -1;
		}
	}

	fclose(fp);

	return nbtuart;
}

static void
btuartd_sigcaught(int sig)
{

	btuartd_signal = sig;
}

static int
btuartd(int nbtuart, struct btuartd_conf *btuartd_conf)
{
	int comfd, i, error = 0;
	char strbuf[256];

	for (i = 0; i < nbtuart; i++) {
		comfd = open(btuartd_conf[i].comdev, O_RDWR | O_NOCTTY);
		if (comfd == -1) {
			syslog(LOG_ERR, "open failed: %s: %s",
			    btuartd_conf[i].comdev, strerror(errno));
			continue;
		}

		if (btuartd_conf[i].init_speed != B0)
			snprintf(strbuf, sizeof(strbuf),
			    " init_speed %d", btuartd_conf[i].init_speed);
		else
			strbuf[0] = '\0';
		printf("btuartd at %s type %d speed %d%s%s\n",
		    btuartd_conf[i].comdev, btuartd_conf[i].type,
		    btuartd_conf[i].speed,
		    (btuartd_conf[i].flow == 1) ? " flow" : "", strbuf);
		error = btuart_setting(comfd,
		    btuartd_conf[i].type, btuartd_conf[i].speed,
		    btuartd_conf[i].flow, btuartd_conf[i].init_speed);
		if (error != 0) {
			close(comfd);
			syslog(LOG_ERR, "btuart setting failed: %s",
				    btuartd_conf[i].comdev);
			continue;
		}
		btuartd_conf[i].comfd = comfd;
	}

	while (btuartd_signal == 0) {
		if (usleep(100000 /*=0.1sec*/) == -1) {
			syslog(LOG_ERR, "usleep failed: %s", strerror(errno));
			break;
		}
	}

	for (i = 0; i < nbtuart; i++) {
		if (btuartd_conf[i].comfd == -1)
			continue;

		if (close(btuartd_conf[i].comfd) == -1)
			syslog(LOG_ERR, "close failed %s: %s",
			    btuartd_conf[i].comdev, strerror(errno));
	}
	return 0;
}

int
main(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	struct btuartd_conf *btuartd_conf;
	int nbtuart, ch, i, status;
	int type, init_speed, flow, debug;
	char *conffile;
	const char *progname = getprogname();

	conffile = BTUARTD_CONFFILE;
	init_speed = B0;
	flow = 0;
	debug = 0;

	nbtuart = 0;
	btuartd_conf = malloc(sizeof(struct btuartd_conf) * BTUARTD_CONF_MIN);

	while ((ch = getopt(argc, argv, "c:dfi:")) != -1) {
		switch (ch) {
		case 'c':
			conffile = optarg;
			break;

		case 'd':
			debug = 1;
			break;

		case 'f':
			flow = 1;
			break;

		case 'i':
			init_speed = atoi(optarg);
			break;

		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	openlog(progname, LOG_NDELAY | LOG_PERROR | LOG_PID, LOG_DAEMON);

	if (argc == 0) {
		if (getuid() != 0)
			errx(EXIT_FAILURE,
			    "** ERROR: You should run %s as privileged user!",
			    progname);

		if (!debug) {
			if (daemon(0, 0) < 0)
				err(EXIT_FAILURE, "daemon failed: ");
			if (pidfile(NULL) < 0) {
				syslog(LOG_ERR, "pidfile failed: %s", 
				    strerror(errno)); 
				exit(EXIT_FAILURE);
			}
		}

		nbtuart = read_config(nbtuart, &btuartd_conf, conffile);
		if (nbtuart == -1) {
			free(btuartd_conf);
			exit(EXIT_FAILURE);
		}

		signal(SIGHUP, btuartd_sigcaught);
	} else {
		type = BTUART_HCITYPE_ANY;
		if (argc == 3) {
			for (i = 0; hcitypetbl[i].hciname != NULL; i++)
				if (!strncmp(hcitypetbl[i].hciname,
				    argv[0], strlen(argv[0])))
					break;
			if (hcitypetbl[i].hciname == NULL)
				usage();
			type = hcitypetbl[i].hcitype;
			argc--;
			argv++;
		}
		if (argc != 2)
			usage();

		btuartd_conf[0].type = type;
		strncpy(btuartd_conf[0].comdev, argv[0],
		    sizeof(btuartd_conf[0].comdev));;
		btuartd_conf[0].speed = atoi(argv[1]);
		btuartd_conf[0].flow = flow;
		btuartd_conf[0].init_speed = init_speed;
		nbtuart = 1;
	}

	signal(SIGINT, btuartd_sigcaught);
	signal(SIGTERM, btuartd_sigcaught);

	while ((status = btuartd(nbtuart, btuartd_conf)) == 0) {
		if (btuartd_signal == SIGHUP) {
			nbtuart = read_config(nbtuart, &btuartd_conf, conffile);
			if (nbtuart == -1)
				exit(EXIT_FAILURE);
			btuartd_signal = 0;
		} else
			break;	/* caught SIGINT or SIGTERM */
	}

	closelog();
	free(btuartd_conf);

	if (status != 0)
		exit(EXIT_FAILURE);
	exit(EXIT_SUCCESS);
}
