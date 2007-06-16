/*	$NetBSD: test-milter.c,v 1.1.1.2.6.1 2007/06/16 17:00:21 snj Exp $	*/

/*++
/* NAME
/*	test-milter 1
/* SUMMARY
/*	Simple test mail filter program.
/* SYNOPSIS
/* .fi
/*	\fBtest-milter\fR [\fIoptions\fR] -p \fBinet:\fIport\fB@\fIhost\fR
/*
/*	\fBtest-milter\fR [\fIoptions\fR] -p \fBunix:\fIpathname\fR
/* DESCRIPTION
/*	\fBtest-milter\fR is a Milter (mail filter) application that
/*	exercises selected features.
/*
/*	Note: this is an unsupported test program. No attempt is made
/*	to maintain compatibility between successive versions.
/*
/*	Arguments (multiple alternatives are separated by "\fB|\fR"):
/* .IP "\fB-a accept|tempfail|reject|discard|\fIddd x.y.z text\fR"
/*	Specifies a non-default reply. The default is to continue
/*	(i.e. \fBtempfail\fR).
/* .IP "\fB-d\fI level\fR"
/*	Enable libmilter debugging at the specified level.
/* .IP "\fB-c connect|helo|mail|rcpt|data|header|eoh|body|eom|unknown|close|abort\fR"
/*	When to send the non-default reply specified with \fB-a\fR.
/*	The default protocol stage is \fBconnect\fR.
/* .IP "\fB-C\fI count\fR"
/*	Terminate after \fIcount\fR connections.
/* .IP "\fB-i \"\fIindex header-label header-value\"\fR"
/*	Insert header at specified position.
/* .IP "\fB-p inet:\fIport\fB@\fIhost\fB|unix:\fIpathname\fR"
/*	The mail filter listen endpoint.
/* .IP "\fB-r "index header-label header-value"
/*	Replace the message header at the specified position.
/* .IP "\fB-R pathname
/*	Replace the message body by the content of the specified file.
/* .IP "\fB-v\fR"
/*	Make the program more verbose.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "libmilter/mfapi.h"
#include "libmilter/mfdef.h"

static int conn_count;
static int verbose;

static int test_connect_reply = SMFIS_CONTINUE;
static int test_helo_reply = SMFIS_CONTINUE;
static int test_mail_reply = SMFIS_CONTINUE;
static int test_rcpt_reply = SMFIS_CONTINUE;

#if SMFI_VERSION > 3
static int test_data_reply = SMFIS_CONTINUE;

#endif
static int test_header_reply = SMFIS_CONTINUE;
static int test_eoh_reply = SMFIS_CONTINUE;
static int test_body_reply = SMFIS_CONTINUE;
static int test_eom_reply = SMFIS_CONTINUE;

#if SMFI_VERSION > 2
static int test_unknown_reply = SMFIS_CONTINUE;

#endif
static int test_close_reply = SMFIS_CONTINUE;
static int test_abort_reply = SMFIS_CONTINUE;

struct command_map {
    const char *name;
    int    *reply;
};

static struct command_map command_map[] = {
    "connect", &test_connect_reply,
    "helo", &test_helo_reply,
    "mail", &test_mail_reply,
    "rcpt", &test_rcpt_reply,
#if SMFI_VERSION > 3
    "data", &test_data_reply,
#endif
    "header", &test_header_reply,
    "eoh", &test_eoh_reply,
    "body", &test_body_reply,
    "eom", &test_eom_reply,
#if SMFI_VERSION > 2
    "unknown", &test_unknown_reply,
#endif
    "close", &test_close_reply,
    "abort", &test_abort_reply,
    0, 0,
};

static char *reply_code;
static char *reply_dsn;
static char *reply_message;

#ifdef SMFIR_INSHEADER
static char *ins_hdr;
static int ins_idx;
static char *ins_val;

#endif

#ifdef SMFIR_CHGHEADER
static char *chg_hdr;
static int chg_idx;
static char *chg_val;

#endif

#ifdef SMFIR_REPLBODY
static char *body_file;

#endif

static int test_reply(SMFICTX *ctx, int code)
{
    (void) fflush(stdout);		/* In case output redirected. */

    if (code == SMFIR_REPLYCODE) {
	if (smfi_setreply(ctx, reply_code, reply_dsn, reply_message) == MI_FAILURE)
	    fprintf(stderr, "smfi_setreply failed\n");
	return (reply_code[0] == '4' ? SMFIS_TEMPFAIL : SMFIS_REJECT);
    } else {
	return (code);
    }
}

static sfsistat test_connect(SMFICTX *ctx, char *name, struct sockaddr * sa)
{
    const char *print_addr;
    char    buf[BUFSIZ];

    printf("test_connect %s ", name);
    switch (sa->sa_family) {
    case AF_INET:
	{
	    struct sockaddr_in *sin = (struct sockaddr_in *) sa;

	    print_addr = inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf));
	    if (print_addr == 0)
		print_addr = strerror(errno);
	    printf("AF_INET (%s)\n", print_addr);
	}
	break;
#ifdef HAS_IPV6
    case AF_INET6:
	{
	    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) sa;

	    print_addr = inet_ntop(AF_INET, &sin6->sin6_addr, buf, sizeof(buf));
	    if (print_addr == 0)
		print_addr = strerror(errno);
	    printf("AF_INET6 (%s)\n", print_addr);
	}
	break;
#endif
    case AF_UNIX:
	{
#undef sun
	    struct sockaddr_un *sun = (struct sockaddr_un *) sa;

	    printf("AF_UNIX (%s)\n", sun->sun_path);
	}
	break;
    default:
	printf(" [unknown address family]\n");
	break;
    }
    return (test_reply(ctx, test_connect_reply));
}

static sfsistat test_helo(SMFICTX *ctx, char *arg)
{
    printf("test_helo \"%s\"\n", arg ? arg : "NULL");
    return (test_reply(ctx, test_helo_reply));
}

static sfsistat test_mail(SMFICTX *ctx, char **argv)
{
    char  **cpp;

    printf("test_mail");
    for (cpp = argv; *cpp; cpp++)
	printf(" \"%s\"", *cpp);
    printf("\n");
    return (test_reply(ctx, test_mail_reply));
}

static sfsistat test_rcpt(SMFICTX *ctx, char **argv)
{
    char  **cpp;

    printf("test_rcpt");
    for (cpp = argv; *cpp; cpp++)
	printf(" \"%s\"", *cpp);
    printf("\n");
    return (test_reply(ctx, test_rcpt_reply));
}


sfsistat test_header(SMFICTX *ctx, char *name, char *value)
{
    printf("test_header \"%s\" \"%s\"\n", name, value);
    return (test_reply(ctx, test_header_reply));
}

static sfsistat test_eoh(SMFICTX *ctx)
{
    printf("test_eoh\n");
    return (test_reply(ctx, test_eoh_reply));
}

static sfsistat test_body(SMFICTX *ctx, unsigned char *data, size_t data_len)
{
    if (verbose == 0)
	printf("test_body %ld bytes\n", (long) data_len);
    else
	printf("%.*s", (int) data_len, data);
    return (test_reply(ctx, test_body_reply));
}

static sfsistat test_eom(SMFICTX *ctx)
{
    printf("test_eom\n");
#ifdef SMFIR_INSHEADER
    if (ins_hdr && smfi_insheader(ctx, ins_idx, ins_hdr, ins_val) == MI_FAILURE)
	fprintf(stderr, "smfi_insheader failed");
#endif
#ifdef SMFIR_CHGHEADER
    if (chg_hdr && smfi_chgheader(ctx, chg_hdr, chg_idx, chg_val) == MI_FAILURE)
	fprintf(stderr, "smfi_chgheader failed");
#endif
#ifdef SMFIR_REPLBODY
    if (body_file) {
	char    buf[BUFSIZ + 2];
	FILE   *fp;
	size_t  len;
	int     count;

	if ((fp = fopen(body_file, "r")) == 0) {
	    perror(body_file);
	} else {
	    printf("replace body with content of %s\n", body_file);
	    for (count = 0; fgets(buf, BUFSIZ, fp) != 0; count++) {
		len = strcspn(buf, "\n");
		buf[len+0] = '\r';
		buf[len+1] = '\n';
		if (smfi_replacebody(ctx, buf, len + 2) == MI_FAILURE) {
		    fprintf(stderr, "body replace failure\n");
		    exit(1);
		}
		if (verbose)
		    printf("%.*s\n", (int) len, buf);
	    }
	    if (count == 0)
		perror("fgets");
	    (void) fclose(fp);
	}
    }
#endif
    return (test_reply(ctx, test_eom_reply));
}

static sfsistat test_abort(SMFICTX *ctx)
{
    printf("test_abort\n");
    return (test_reply(ctx, test_abort_reply));
}

static sfsistat test_close(SMFICTX *ctx)
{
    printf("test_close\n");
    if (verbose)
	printf("conn_count %d\n", conn_count);
    if (conn_count > 0 && --conn_count == 0)
	exit(0);
    return (test_reply(ctx, test_close_reply));
}

#if SMFI_VERSION > 3

static sfsistat test_data(SMFICTX *ctx)
{
    printf("test_data\n");
    return (test_reply(ctx, test_data_reply));
}

#endif

#if SMFI_VERSION > 2

static sfsistat test_unknown(SMFICTX *ctx)
{
    printf("test_unknown\n");
    return (test_reply(ctx, test_unknown_reply));
}

#endif

static struct smfiDesc smfilter =
{
    "test-milter",
    SMFI_VERSION,
    SMFIF_ADDRCPT | SMFIF_DELRCPT | SMFIF_ADDHDRS | SMFIF_CHGHDRS | SMFIF_CHGBODY,
    test_connect,
    test_helo,
    test_mail,
    test_rcpt,
    test_header,
    test_eoh,
    test_body,
    test_eom,
    test_abort,
    test_close,
#if SMFI_VERSION > 2
    test_unknown,
#endif
#if SMFI_VERSION > 3
    test_data,
#endif
};

static void parse_hdr_info(const char *optarg, int *idx,
			           char **hdr, char **value)
{
    int     len;

    len = strlen(optarg) + 1;
    if ((*hdr = malloc(len)) == 0 || (*value = malloc(len)) == 0) {
	fprintf(stderr, "out of memory\n");
	exit(1);
    }
    if (sscanf(optarg, "%d %s %[^\n]", idx, *hdr, *value) != 3) {
	fprintf(stderr, "bad header info: %s\n", optarg);
	exit(1);
    }
}

int     main(int argc, char **argv)
{
    char   *action = 0;
    char   *command = 0;
    struct command_map *cp;
    int     ch;
    int     code;

    while ((ch = getopt(argc, argv, "a:c:C:d:i:p:r:R:v")) > 0) {
	switch (ch) {
	case 'a':
	    action = optarg;
	    break;
	case 'c':
	    command = optarg;
	    break;
	case 'd':
	    if (smfi_setdbg(atoi(optarg)) == MI_FAILURE) {
		fprintf(stderr, "smfi_setdbg failed\n");
		exit(1);
	    }
	    break;
	case 'i':
#ifdef SMFIR_INSHEADER
	    if (ins_hdr) {
		fprintf(stderr, "too many -i options\n");
		exit(1);
	    }
	    parse_hdr_info(optarg, &ins_idx, &ins_hdr, &ins_val);
#else
	    fprintf(stderr, "no libmilter support to insert header\n");
	    exit(1);
#endif
	    break;
	case 'p':
	    if (smfi_setconn(optarg) == MI_FAILURE) {
		fprintf(stderr, "smfi_setconn failed\n");
		exit(1);
	    }
	    break;
	case 'r':
#ifdef SMFIR_CHGHEADER
	    if (chg_hdr) {
		fprintf(stderr, "too many -r options\n");
		exit(1);
	    }
	    parse_hdr_info(optarg, &chg_idx, &chg_hdr, &chg_val);
#else
	    fprintf(stderr, "no libmilter support to change header\n");
	    exit(1);
#endif
	    break;
	case 'v':
	    verbose++;
	    break;
	case 'C':
	    conn_count = atoi(optarg);
	    break;
#ifdef SMFIR_REPLBODY
	case 'R':
	    if (body_file) {
		fprintf(stderr, "too many -R options\n");
		exit(1);
	    }
	    body_file = optarg;
#endif
	    break;
	default:
	    fprintf(stderr,
		    "usage: %s [-dv] \n"
		    "\t[-a action]              non-default action\n"
		    "\t[-c command]             non-default action trigger\n"
		    "\t[-i 'index label value'] insert header\n"
		    "\t-p port                  milter application\n"
		    "\t[-r 'index label value'] replace header\n"
		    "\t[-C conn_count]          when to exit\n",
		    "\t[-R body_text]           replace body\n",
		    argv[0]);
	    exit(1);
	}
    }
    if (smfi_register(smfilter) == MI_FAILURE) {
	fprintf(stderr, "smfi_register failed\n");
	exit(1);
    }
    if (command) {
	for (cp = command_map; /* see below */ ; cp++) {
	    if (cp->name == 0) {
		fprintf(stderr, "bad -c argument: %s\n", command);
		exit(1);
	    }
	    if (strcmp(command, cp->name) == 0)
		break;
	}
    }
    if (action) {
	if (command == 0)
	    cp = command_map;
	if (strcmp(action, "tempfail") == 0) {
	    cp->reply[0] = SMFIS_TEMPFAIL;
	} else if (strcmp(action, "reject") == 0) {
	    cp->reply[0] = SMFIS_REJECT;
	} else if (strcmp(action, "accept") == 0) {
	    cp->reply[0] = SMFIS_ACCEPT;
	} else if (strcmp(action, "discard") == 0) {
	    cp->reply[0] = SMFIS_DISCARD;
	} else if ((code = atoi(action)) >= 400
		   && code <= 599
		   && action[3] == ' ') {
	    cp->reply[0] = SMFIR_REPLYCODE;
	    reply_code = action;
	    reply_dsn = action + 3;
	    if (*reply_dsn != 0) {
		*reply_dsn++ = 0;
		reply_dsn += strspn(reply_dsn, " ");
	    }
	    if (*reply_dsn == 0) {
		reply_dsn = reply_message = 0;
	    } else {
		reply_message = reply_dsn + strcspn(reply_dsn, " ");
		if (*reply_message != 0) {
		    *reply_message++ = 0;
		    reply_message += strspn(reply_message, " ");
		}
		if (*reply_message == 0)
		    reply_message = 0;
	    }
	} else {
	    fprintf(stderr, "bad -a argument: %s\n", action);
	    exit(1);
	}
	if (verbose) {
	    printf("command %s action %d\n", cp->name, cp->reply[0]);
	    if (reply_code)
		printf("reply code %s dsn %s message %s\n",
		       reply_code, reply_dsn ? reply_dsn : "(null)",
		       reply_message ? reply_message : "(null)");
	}
    }
    return (smfi_main());
}
