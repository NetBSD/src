/*	$NetBSD: test-milter.c,v 1.1.1.1.2.2 2009/09/15 06:03:20 snj Exp $	*/

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
/* .IP "\fB-a accept|tempfail|reject|discard|skip|\fIddd x.y.z text\fR"
/*	Specifies a non-default reply for the MTA command specified
/*	with \fB-c\fR. The default is \fBtempfail\fR.
/* .IP "\fB-A address\fR"
/*	Add the specified recipient address. Multiple -A options
/*	are supported.
/* .IP "\fB-b pathname
/*	Replace the message body by the content of the specified file.
/* .IP "\fB-c connect|helo|mail|rcpt|data|header|eoh|body|eom|unknown|close|abort\fR"
/*	When to send the non-default reply specified with \fB-a\fR.
/*	The default protocol stage is \fBconnect\fR.
/* .IP "\fB-C\fI count\fR"
/*	Terminate after \fIcount\fR connections.
/* .IP "\fB-d\fI level\fR"
/*	Enable libmilter debugging at the specified level.
/* .IP "\fB-f \fIsender\fR
/*	Replace the sender by the specified address.
/* .IP "\fB-h \fI'index header-label header-value'\fR"
/*	Replace the message header at the specified position.
/* .IP "\fB-i \fI'index header-label header-value'\fR"
/*	Insert header at specified position.
/* .IP "\fB-l\fR"
/*	Header values include leading space. Specify this option
/*	before \fB-i\fR or \fB-r\fR.
/* .IP "\fB-m connect|helo|mail|rcpt|data|eoh|eom\fR"
/*	The protocol stage that receives the list of macros specified
/*	with \fB-M\fR.  The default protocol stage is \fBconnect\fR.
/* .IP "\fB-M \fIset_macro_list\fR"
/*	A non-default list of macros that the MTA should send at
/*	the protocol stage specified with \fB-m\fR.
/* .IP "\fB-n connect|helo|mail|rcpt|data|header|eoh|body|eom|unknown\fR"
/*	The event that the MTA should not send.
/* .IP "\fB-N connect|helo|mail|rcpt|data|header|eoh|body|eom|unknown\fR"
/*	The event for which the filter will not reply.
/* .IP "\fB-p inet:\fIport\fB@\fIhost\fB|unix:\fIpathname\fR"
/*	The mail filter listen endpoint.
/* .IP "\fB-r\fR"
/*	Request rejected recipients from the MTA.
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

static const struct command_map command_map[] = {
    "connect", &test_connect_reply,
    "helo", &test_helo_reply,
    "mail", &test_mail_reply,
    "rcpt", &test_rcpt_reply,
    "header", &test_header_reply,
    "eoh", &test_eoh_reply,
    "body", &test_body_reply,
    "eom", &test_eom_reply,
    "abort", &test_abort_reply,
    "close", &test_close_reply,
#if SMFI_VERSION > 2
    "unknown", &test_unknown_reply,
#endif
#if SMFI_VERSION > 3
    "data", &test_data_reply,
#endif
    0, 0,
};

static char *reply_code;
static char *reply_dsn;
static char *reply_message;

#ifdef SMFIR_CHGFROM
static char *chg_from;

#endif

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

#define MAX_RCPT	10
int     rcpt_count = 0;
char   *rcpt_addr[MAX_RCPT];

static const char *macro_names[] = {
    "_",
    "i",
    "j",
    "v",
    "{auth_authen}",
    "{auth_author}",
    "{auth_type}",
    "{cert_issuer}",
    "{cert_subject}",
    "{cipher}",
    "{cipher_bits}",
    "{client_addr}",
    "{client_connections}",
    "{client_name}",
    "{client_port}",
    "{client_ptr}",
    "{client_resolve}",
    "{daemon_name}",
    "{if_addr}",
    "{if_name}",
    "{mail_addr}",
    "{mail_host}",
    "{mail_mailer}",
    "{rcpt_addr}",
    "{rcpt_host}",
    "{rcpt_mailer}",
    "{tls_version}",
    0,
};

static int test_reply(SMFICTX *ctx, int code)
{
    const char **cpp;
    const char *symval;

    for (cpp = macro_names; *cpp; cpp++)
	if ((symval = smfi_getsymval(ctx, (char *) *cpp)) != 0)
	    printf("macro: %s=\"%s\"\n", *cpp, symval);
    (void) fflush(stdout);			/* In case output redirected. */

    if (code == SMFIR_REPLYCODE) {
	if (smfi_setmlreply(ctx, reply_code, reply_dsn, reply_message, reply_message, (char *) 0) == MI_FAILURE)
	    fprintf(stderr, "smfi_setmlreply failed\n");
	printf("test_reply %s\n", reply_code);
	return (reply_code[0] == '4' ? SMFIS_TEMPFAIL : SMFIS_REJECT);
    } else {
	printf("test_reply %d\n", code);
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
	    printf("AF_INET (%s:%d)\n", print_addr, ntohs(sin->sin_port));
	}
	break;
#ifdef HAS_IPV6
    case AF_INET6:
	{
	    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) sa;

	    print_addr = inet_ntop(AF_INET, &sin6->sin6_addr, buf, sizeof(buf));
	    if (print_addr == 0)
		print_addr = strerror(errno);
	    printf("AF_INET6 (%s:%d)\n", print_addr, ntohs(sin6->sin6_port));
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
		buf[len + 0] = '\r';
		buf[len + 1] = '\n';
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
#ifdef SMFIR_CHGFROM
    if (chg_from != 0 && smfi_chgfrom(ctx, chg_from, "whatever") == MI_FAILURE)
	fprintf(stderr, "smfi_chgfrom failed\n");
    else
	printf("smfi_chgfrom OK\n");
#endif
#ifdef SMFIR_INSHEADER
    if (ins_hdr && smfi_insheader(ctx, ins_idx, ins_hdr, ins_val) == MI_FAILURE)
	fprintf(stderr, "smfi_insheader failed\n");
#endif
#ifdef SMFIR_CHGHEADER
    if (chg_hdr && smfi_chgheader(ctx, chg_hdr, chg_idx, chg_val) == MI_FAILURE)
	fprintf(stderr, "smfi_chgheader failed\n");
#endif
    {
	int     count;

	for (count = 0; count < rcpt_count; count++)
	    if (smfi_addrcpt(ctx, rcpt_addr[count]) == MI_FAILURE)
		fprintf(stderr, "smfi_addrcpt `%s' failed\n", rcpt_addr[count]);
    }
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

static sfsistat test_unknown(SMFICTX *ctx, const char *what)
{
    printf("test_unknown %s\n", what);
    return (test_reply(ctx, test_unknown_reply));
}

#endif

static sfsistat test_negotiate(SMFICTX *, unsigned long, unsigned long,
			               unsigned long, unsigned long,
			               unsigned long *, unsigned long *,
			               unsigned long *, unsigned long *);

static struct smfiDesc smfilter =
{
    "test-milter",
    SMFI_VERSION,
    SMFIF_ADDRCPT | SMFIF_DELRCPT | SMFIF_ADDHDRS | SMFIF_CHGHDRS | SMFIF_CHGBODY | SMFIF_CHGFROM,
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
#if SMFI_VERSION > 5
    test_negotiate,
#endif
};

#if SMFI_VERSION > 5

static const char *macro_states[] = {
    "connect",				/* SMFIM_CONNECT */
    "helo",				/* SMFIM_HELO */
    "mail",				/* SMFIM_ENVFROM */
    "rcpt",				/* SMFIM_ENVRCPT */
    "data",				/* SMFIM_DATA */
    "eom",				/* SMFIM_EOM < SMFIM_EOH */
    "eoh",				/* SMFIM_EOH > SMFIM_EOM */
    0,
};

static int set_macro_state;
static char *set_macro_list;

typedef sfsistat (*FILTER_ACTION) ();

struct noproto_map {
    const char *name;
    int     send_mask;
    int     reply_mask;
    int    *reply;
    FILTER_ACTION *action;
};

static const struct noproto_map noproto_map[] = {
    "connect", SMFIP_NOCONNECT, SMFIP_NR_CONN, &test_connect_reply, &smfilter.xxfi_connect,
    "helo", SMFIP_NOHELO, SMFIP_NR_HELO, &test_helo_reply, &smfilter.xxfi_helo,
    "mail", SMFIP_NOMAIL, SMFIP_NR_MAIL, &test_mail_reply, &smfilter.xxfi_envfrom,
    "rcpt", SMFIP_NORCPT, SMFIP_NR_RCPT, &test_rcpt_reply, &smfilter.xxfi_envrcpt,
    "data", SMFIP_NODATA, SMFIP_NR_DATA, &test_data_reply, &smfilter.xxfi_data,
    "header", SMFIP_NOHDRS, SMFIP_NR_HDR, &test_header_reply, &smfilter.xxfi_header,
    "eoh", SMFIP_NOEOH, SMFIP_NR_EOH, &test_eoh_reply, &smfilter.xxfi_eoh,
    "body", SMFIP_NOBODY, SMFIP_NR_BODY, &test_body_reply, &smfilter.xxfi_body,
    "unknown", SMFIP_NOUNKNOWN, SMFIP_NR_UNKN, &test_connect_reply, &smfilter.xxfi_unknown,
    0,
};

static int nosend_mask;
static int noreply_mask;
static int misc_mask;

static sfsistat test_negotiate(SMFICTX *ctx,
			               unsigned long f0,
			               unsigned long f1,
			               unsigned long f2,
			               unsigned long f3,
			               unsigned long *pf0,
			               unsigned long *pf1,
			               unsigned long *pf2,
			               unsigned long *pf3)
{
    if (set_macro_list) {
	if (verbose)
	    printf("set symbol list %s to \"%s\"\n",
		   macro_states[set_macro_state], set_macro_list);
	smfi_setsymlist(ctx, set_macro_state, set_macro_list);
    }
    if (verbose)
	printf("negotiate f0=%lx *pf0 = %lx f1=%lx *pf1=%lx nosend=%lx noreply=%lx misc=%lx\n",
	       f0, *pf0, f1, *pf1, (long) nosend_mask, (long) noreply_mask, (long) misc_mask);
    *pf0 = f0;
    *pf1 = f1 & (nosend_mask | noreply_mask | misc_mask);
    return (SMFIS_CONTINUE);
}

#endif

static void parse_hdr_info(const char *optarg, int *idx,
			           char **hdr, char **value)
{
    int     len;

    len = strlen(optarg) + 1;
    if ((*hdr = malloc(len)) == 0 || (*value = malloc(len)) == 0) {
	fprintf(stderr, "out of memory\n");
	exit(1);
    }
    if ((misc_mask & SMFIP_HDR_LEADSPC) == 0 ?
	sscanf(optarg, "%d %s %[^\n]", idx, *hdr, *value) != 3 :
	sscanf(optarg, "%d %[^ ]%[^\n]", idx, *hdr, *value) != 3) {
	fprintf(stderr, "bad header info: %s\n", optarg);
	exit(1);
    }
}

int     main(int argc, char **argv)
{
    char   *action = 0;
    char   *command = 0;
    const struct command_map *cp;
    int     ch;
    int     code;
    const char **cpp;
    char   *set_macro_state_arg = 0;
    char   *nosend = 0;
    char   *noreply = 0;
    const struct noproto_map *np;

    while ((ch = getopt(argc, argv, "a:A:b:c:C:d:f:h:i:lm:M:n:N:p:rv")) > 0) {
	switch (ch) {
	case 'a':
	    action = optarg;
	    break;
	case 'A':
	    if (rcpt_count >= MAX_RCPT) {
		fprintf(stderr, "too many -A options\n");
		exit(1);
	    }
	    rcpt_addr[rcpt_count++] = optarg;
	    break;
	case 'b':
#ifdef SMFIR_REPLBODY
	    if (body_file) {
		fprintf(stderr, "too many -b options\n");
		exit(1);
	    }
	    body_file = optarg;
#else
	    fprintf(stderr, "no libmilter support to replace body\n");
#endif
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
	case 'f':
#ifdef SMFIR_CHGFROM
	    if (chg_from) {
		fprintf(stderr, "too many -f options\n");
		exit(1);
	    }
	    chg_from = optarg;
#else
	    fprintf(stderr, "no libmilter support to change sender\n");
	    exit(1);
#endif
	    break;
	case 'h':
#ifdef SMFIR_CHGHEADER
	    if (chg_hdr) {
		fprintf(stderr, "too many -h options\n");
		exit(1);
	    }
	    parse_hdr_info(optarg, &chg_idx, &chg_hdr, &chg_val);
#else
	    fprintf(stderr, "no libmilter support to change header\n");
	    exit(1);
#endif
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
	case 'l':
#if SMFI_VERSION > 5
	    if (ins_hdr || chg_hdr) {
		fprintf(stderr, "specify -l before -i or -r\n");
		exit(1);
	    }
	    misc_mask |= SMFIP_HDR_LEADSPC;
#else
	    fprintf(stderr, "no libmilter support for leading space\n");
	    exit(1);
#endif
	    break;
	case 'm':
#if SMFI_VERSION > 5
	    if (set_macro_state_arg) {
		fprintf(stderr, "too many -m options\n");
		exit(1);
	    }
	    set_macro_state_arg = optarg;
#else
	    fprintf(stderr, "no libmilter support to specify macro list\n");
	    exit(1);
#endif
	    break;
	case 'M':
#if SMFI_VERSION > 5
	    if (set_macro_list) {
		fprintf(stderr, "too many -M options\n");
		exit(1);
	    }
	    set_macro_list = optarg;
#else
	    fprintf(stderr, "no libmilter support to specify macro list\n");
#endif
	    break;
	case 'n':
#if SMFI_VERSION > 5
	    if (nosend) {
		fprintf(stderr, "too many -n options\n");
		exit(1);
	    }
	    nosend = optarg;
#else
	    fprintf(stderr, "no libmilter support for negotiate callback\n");
#endif
	    break;
	case 'N':
#if SMFI_VERSION > 5
	    if (noreply) {
		fprintf(stderr, "too many -n options\n");
		exit(1);
	    }
	    noreply = optarg;
#else
	    fprintf(stderr, "no libmilter support for negotiate callback\n");
#endif
	    break;
	case 'p':
	    if (smfi_setconn(optarg) == MI_FAILURE) {
		fprintf(stderr, "smfi_setconn failed\n");
		exit(1);
	    }
	    break;
	case 'r':
#ifdef SMFIP_RCPT_REJ
	    misc_mask |= SMFIP_RCPT_REJ;
#else
	    fprintf(stderr, "no libmilter support for rejected recipients\n");
#endif
	    break;
	case 'v':
	    verbose++;
	    break;
	case 'C':
	    conn_count = atoi(optarg);
	    break;
	default:
	    fprintf(stderr,
		    "usage: %s [-dv] \n"
		    "\t[-a action]              non-default action\n"
		    "\t[-b body_text]           replace body\n",
		    "\t[-c command]             non-default action trigger\n"
		    "\t[-h 'index label value'] replace header\n"
		    "\t[-i 'index label value'] insert header\n"
		    "\t[-m macro_state]		non-default macro state\n"
		    "\t[-M macro_list]		non-default macro list\n"
		    "\t[-n events]		don't receive these events\n"
		  "\t[-N events]		don't reply to these events\n"
		    "\t-p port                  milter application\n"
		  "\t-r                       request rejected recipients\n"
		    "\t[-C conn_count]          when to exit\n",
		    argv[0]);
	    exit(1);
	}
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
#ifdef SMFIS_SKIP
	} else if (strcmp(action, "skip") == 0) {
	    cp->reply[0] = SMFIS_SKIP;
#endif
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
#if SMFI_VERSION > 5
    if (set_macro_state_arg) {
	for (cpp = macro_states; /* see below */ ; cpp++) {
	    if (*cpp == 0) {
		fprintf(stderr, "bad -m argument: %s\n", set_macro_state_arg);
		exit(1);
	    }
	    if (strcmp(set_macro_state_arg, *cpp) == 0)
		break;
	}
	set_macro_state = cpp - macro_states;
    }
    if (nosend) {
	for (np = noproto_map; /* see below */ ; np++) {
	    if (np->name == 0) {
		fprintf(stderr, "bad -n argument: %s\n", nosend);
		exit(1);
	    }
	    if (strcmp(nosend, np->name) == 0)
		break;
	}
	nosend_mask = np->send_mask;
	np->action[0] = 0;
    }
    if (noreply) {
	for (np = noproto_map; /* see below */ ; np++) {
	    if (np->name == 0) {
		fprintf(stderr, "bad -N argument: %s\n", noreply);
		exit(1);
	    }
	    if (strcmp(noreply, np->name) == 0)
		break;
	}
	noreply_mask = np->reply_mask;
	*np->reply = SMFIS_NOREPLY;
    }
#endif
    if (smfi_register(smfilter) == MI_FAILURE) {
	fprintf(stderr, "smfi_register failed\n");
	exit(1);
    }
    return (smfi_main());
}
