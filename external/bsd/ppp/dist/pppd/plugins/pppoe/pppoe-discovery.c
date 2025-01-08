/*
 * Perform PPPoE discovery
 *
 * Copyright (C) 2000-2001 by Roaring Penguin Software Inc.
 * Copyright (C) 2004 Marco d'Itri <md@linux.it>
 *
 * This program may be distributed according to the terms of the GNU
 * General Public License, version 2 or (at your option) any later version.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <stdbool.h>
#include <stdint.h>

#include "pppoe.h"

int debug;
int got_sigterm;
int pppoe_verbose;
static FILE *debugFile;

void
fatal(const char *fmt, ...)
{
    va_list pvar;
    va_start(pvar, fmt);
    vfprintf(stderr, fmt, pvar);
    va_end(pvar);
    fputc('\n', stderr);
    exit(1);
}

void
error(const char *fmt, ...)
{
    va_list pvar;
    va_start(pvar, fmt);
    vfprintf(stderr, fmt, pvar);
    fputc('\n', stderr);
    va_end(pvar);
}

void
warn(const char *fmt, ...)
{
    va_list pvar;
    va_start(pvar, fmt);
    vfprintf(stderr, fmt, pvar);
    fputc('\n', stderr);
    va_end(pvar);
}

void
info(const char *fmt, ...)
{
    va_list pvar;
    va_start(pvar, fmt);
    vprintf(fmt, pvar);
    putchar('\n');
    va_end(pvar);
}

void
init_pr_log(const char *prefix, int level)
{
}

void
end_pr_log(void)
{
    fflush(debugFile);
}

void
pr_log(void *arg, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(debugFile, fmt, ap);
    va_end(ap);
}

size_t
strlcpy(char *dest, const char *src, size_t len)
{
    size_t ret = strlen(src);

    if (len != 0) {
	if (ret < len)
	    strcpy(dest, src);
	else {
	    strncpy(dest, src, len - 1);
	    dest[len-1] = 0;
	}
    }
    return ret;
}

static char *
xstrdup(const char *s)
{
    char *ret = strdup(s);
    if (!ret) {
        perror("strdup");
        exit(1);
    }
    return ret;
}

int
get_time(struct timeval *tv)
{
    return gettimeofday(tv, NULL);
}

int signaled(int signal) {
    if (signal == SIGTERM)
        return got_sigterm;
    return 0;
}

bool debug_on()
{
    return !!debug;
}

static void
term_handler(int signum)
{
    got_sigterm = 1;
}

static void usage(void);

int main(int argc, char *argv[])
{
    int opt;
    PPPoEConnection *conn;

    signal(SIGINT, term_handler);
    signal(SIGTERM, term_handler);

    conn = malloc(sizeof(PPPoEConnection));
    if (!conn) {
        perror("malloc");
        exit(1);
    }

    memset(conn, 0, sizeof(PPPoEConnection));

    pppoe_verbose = 1;
    conn->discoveryTimeout = PADI_TIMEOUT;
    conn->discoveryAttempts = MAX_PADI_ATTEMPTS;

    while ((opt = getopt(argc, argv, "I:D:VUQS:C:W:t:a:h")) > 0) {
	switch(opt) {
	case 'S':
	    conn->serviceName = xstrdup(optarg);
	    break;
	case 'C':
	    conn->acName = xstrdup(optarg);
	    break;
	case 't':
	    if (sscanf(optarg, "%d", &conn->discoveryTimeout) != 1) {
		fprintf(stderr, "Illegal argument to -t: Should be -t timeout\n");
		exit(EXIT_FAILURE);
	    }
	    if (conn->discoveryTimeout < 1) {
		conn->discoveryTimeout = 1;
	    }
	    break;
	case 'a':
	    if (sscanf(optarg, "%d", &conn->discoveryAttempts) != 1) {
		fprintf(stderr, "Illegal argument to -a: Should be -a attempts\n");
		exit(EXIT_FAILURE);
	    }
	    if (conn->discoveryAttempts < 1) {
		conn->discoveryAttempts = 1;
	    }
	    break;
	case 'U':
	    if(conn->hostUniq.length) {
		fprintf(stderr, "-U and -W are mutually exclusive\n");
		exit(EXIT_FAILURE);
	    } else {
		pid_t pid = getpid();
		conn->hostUniq.type = htons(TAG_HOST_UNIQ);
		conn->hostUniq.length = htons(sizeof(pid));
		memcpy(conn->hostUniq.payload, &pid, sizeof(pid));
	    }
	    break;
	case 'W':
	    if(conn->hostUniq.length) {
		fprintf(stderr, "-U and -W are mutually exclusive\n");
		exit(EXIT_FAILURE);
	    }
	    if (!parseHostUniq(optarg, &conn->hostUniq)) {
		fprintf(stderr, "Invalid host-uniq argument: %s\n", optarg);
		exit(EXIT_FAILURE);
            }
	    break;
	case 'D':
	    pppoe_verbose = 2;
	    debug = 1;
	    debugFile = fopen(optarg, "w");
	    if (!debugFile) {
		fprintf(stderr, "Could not open %s: %s\n",
			optarg, strerror(errno));
		exit(1);
	    }
	    fprintf(debugFile, "pppoe-discovery from pppd %s\n", PPPD_VERSION);
	    break;
	case 'I':
	    conn->ifName = xstrdup(optarg);
	    break;
	case 'Q':
	    pppoe_verbose = 0;
	    break;
	case 'V':
	case 'h':
	    usage();
	    exit(0);
	default:
	    usage();
	    exit(1);
	}
    }

    if (optind != argc) {
	fprintf(stderr, "%s: extra argument '%s'\n", argv[0], argv[optind]);
	usage();
	exit(EXIT_FAILURE);
    }

    if (!conn->ifName) {
	fprintf(stderr, "Interface was not specified\n");
	exit(EXIT_FAILURE);
    }

    conn->sessionSocket = -1;

    conn->discoverySocket = openInterface(conn->ifName, Eth_PPPOE_Discovery, conn->myEth);
    if (conn->discoverySocket < 0) {
	perror("Cannot create PPPoE discovery socket");
	exit(1);
    }

    discovery1(conn, 1);

    if (!conn->numPADOs)
	exit(1);
    else
	exit(0);
}

static void
usage(void)
{
    fprintf(stderr, "Usage: pppoe-discovery [options]\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "   -I if_name     -- Specify interface (mandatory option)\n");
    fprintf(stderr, "   -D filename    -- Log debugging information in filename.\n");
    fprintf(stderr,
	    "   -t timeout     -- Initial timeout for discovery packets in seconds\n"
	    "   -a attempts    -- Number of discovery attempts\n"
	    "   -V             -- Print version and exit.\n"
	    "   -Q             -- Quit Mode: Do not print access concentrator names\n"
	    "   -S name        -- Set desired service name.\n"
	    "   -C name        -- Set desired access concentrator name.\n"
	    "   -U             -- Use Host-Unique to allow multiple PPPoE sessions.\n"
	    "   -W hexvalue    -- Set the Host-Unique to the supplied hex string.\n"
	    "   -h             -- Print usage information.\n");
    fprintf(stderr, "\npppoe-discovery from pppd " PPPD_VERSION "\n");
}
