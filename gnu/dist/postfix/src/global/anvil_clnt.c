/*++
/* NAME
/*	anvil_clnt 3
/* SUMMARY
/*	connection count and rate management client interface
/* SYNOPSIS
/*	#include <anvil_clnt.h>
/*
/*	ANVIL_CLNT *anvil_clnt_create(void)
/*
/*	void	anvil_clnt_free(anvil_clnt)
/*	ANVIL_CLNT *anvil_clnt;
/*
/*	int	anvil_clnt_connect(anvil_clnt, service, addr,
/*					count, rate)
/*	ANVIL_CLNT *anvil_clnt;
/*	const char *service;
/*	const char *addr;
/*	int	*count;
/*	int	*rate;
/*
/*	int	anvil_clnt_mail(anvil_clnt, service, addr, msgs)
/*	ANVIL_CLNT *anvil_clnt;
/*	const char *service;
/*	const char *addr;
/*	int	*msgs;
/*
/*	int	anvil_clnt_rcpt(anvil_clnt, service, addr, rcpts)
/*	ANVIL_CLNT *anvil_clnt;
/*	const char *service;
/*	const char *addr;
/*	int	*rcpts;
/*
/*	int	anvil_clnt_newtls(anvil_clnt, service, addr, newtls)
/*	ANVIL_CLNT *anvil_clnt;
/*	const char *service;
/*	const char *addr;
/*	int	*newtls;
/*
/*	int	anvil_clnt_newtls_stat(anvil_clnt, service, addr, newtls)
/*	ANVIL_CLNT *anvil_clnt;
/*	const char *service;
/*	const char *addr;
/*	int	*newtls;
/*
/*	int	anvil_clnt_disconnect(anvil_clnt, service, addr)
/*	ANVIL_CLNT *anvil_clnt;
/*	const char *service;
/*	const char *addr;
/*
/*	int	anvil_clnt_lookup(anvil_clnt, service, addr,
/*					count, rate, msgs, rcpts)
/*	ANVIL_CLNT *anvil_clnt;
/*	const char *service;
/*	const char *addr;
/*	int	*count;
/*	int	*rate;
/*	int	*msgs;
/*	int	*rcpts;
/* DESCRIPTION
/*	anvil_clnt_create() instantiates a local anvil service
/*	client endpoint.
/*
/*	anvil_clnt_connect() informs the anvil server that a
/*	remote client has connected, and returns the current
/*	connection count and connection rate for that remote client.
/*
/*	anvil_clnt_mail() registers a MAIL FROM event and
/*	returns the current MAIL FROM rate for the specified remote
/*	client.
/*
/*	anvil_clnt_rcpt() registers a RCPT TO event and
/*	returns the current RCPT TO rate for the specified remote
/*	client.
/*
/*	anvil_clnt_newtls() registers a remote client request
/*	to negotiate a new (uncached) TLS session and returns the
/*	current newtls request rate for the specified remote client.
/*
/*	anvil_clnt_newtls_stat() returns the current newtls request
/*	rate for the specified remote client.
/*
/*	anvil_clnt_disconnect() informs the anvil server that a remote
/*	client has disconnected.
/*
/*	anvil_clnt_lookup() returns the current count and rate
/*	information for the specified client.
/*
/*	anvil_clnt_free() destroys a local anvil service client
/*	endpoint.
/*
/*	Arguments:
/* .IP anvil_clnt
/*	Client rate control service handle.
/* .IP service
/*	The service that the remote client is connected to.
/* .IP addr
/*	Null terminated string that identifies the remote client.
/* .IP count
/*	Pointer to storage for the current number of connections from
/*	this remote client.
/* .IP rate
/*	Pointer to storage for the current connection rate for this
/*	remote client.
/* .IP msgs
/*	Pointer to storage for the current message rate for this
/*	remote client.
/* .IP rcpts
/*	Pointer to storage for the current recipient rate for this
/*	remote client.
/* .IP newtls
/*	Pointer to storage for the current "new TLS session" rate
/*	for this remote client.
/* DIAGNOSTICS
/*	The update and status query routines return
/*	ANVIL_STAT_OK in case of success, ANVIL_STAT_FAIL otherwise
/*	(either the communication with the server is broken or the
/*	server experienced a problem).
/* SEE ALSO
/*	anvil(8), connection/rate limiting
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

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <mymalloc.h>
#include <msg.h>
#include <attr_clnt.h>
#include <stringops.h>

/* Global library. */

#include <mail_proto.h>
#include <mail_params.h>
#include <anvil_clnt.h>

/* Application specific. */

#define ANVIL_IDENT(service, addr) \
    printable(concatenate(service, ":", addr, (char *) 0), '?')

/* anvil_clnt_create - instantiate connection rate service client */

ANVIL_CLNT *anvil_clnt_create(void)
{
    ATTR_CLNT *anvil_clnt;

    /*
     * Use whatever IPC is preferred for internal use: UNIX-domain sockets or
     * Solaris streams.
     */
#ifndef VAR_ANVIL_SERVICE
    anvil_clnt = attr_clnt_create("local:" ANVIL_CLASS "/" ANVIL_SERVICE,
				  var_ipc_timeout, 0, 0);
#else
    anvil_clnt = attr_clnt_create(var_anvil_service, var_ipc_timeout, 0, 0);
#endif
    return ((ANVIL_CLNT *) anvil_clnt);
}

/* anvil_clnt_free - destroy connection rate service client */

void    anvil_clnt_free(ANVIL_CLNT *anvil_clnt)
{
    attr_clnt_free((ATTR_CLNT *) anvil_clnt);
}

/* anvil_clnt_lookup - status query */

int     anvil_clnt_lookup(ANVIL_CLNT *anvil_clnt, const char *service,
			          const char *addr, int *count, int *rate,
			          int *msgs, int *rcpts, int *newtls)
{
    char   *ident = ANVIL_IDENT(service, addr);
    int     status;

    if (attr_clnt_request((ATTR_CLNT *) anvil_clnt,
			  ATTR_FLAG_NONE,	/* Query attributes. */
			  ATTR_TYPE_STR, ANVIL_ATTR_REQ, ANVIL_REQ_LOOKUP,
			  ATTR_TYPE_STR, ANVIL_ATTR_IDENT, ident,
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply attributes. */
			  ATTR_TYPE_INT, ANVIL_ATTR_STATUS, &status,
			  ATTR_TYPE_INT, ANVIL_ATTR_COUNT, count,
			  ATTR_TYPE_INT, ANVIL_ATTR_RATE, rate,
			  ATTR_TYPE_INT, ANVIL_ATTR_MAIL, msgs,
			  ATTR_TYPE_INT, ANVIL_ATTR_RCPT, rcpts,
			  ATTR_TYPE_INT, ANVIL_ATTR_NTLS, newtls,
			  ATTR_TYPE_END) != 6)
	status = ANVIL_STAT_FAIL;
    else if (status != ANVIL_STAT_OK)
	status = ANVIL_STAT_FAIL;
    myfree(ident);
    return (status);
}

/* anvil_clnt_connect - heads-up and status query */

int     anvil_clnt_connect(ANVIL_CLNT *anvil_clnt, const char *service,
			           const char *addr, int *count, int *rate)
{
    char   *ident = ANVIL_IDENT(service, addr);
    int     status;

    if (attr_clnt_request((ATTR_CLNT *) anvil_clnt,
			  ATTR_FLAG_NONE,	/* Query attributes. */
			  ATTR_TYPE_STR, ANVIL_ATTR_REQ, ANVIL_REQ_CONN,
			  ATTR_TYPE_STR, ANVIL_ATTR_IDENT, ident,
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply attributes. */
			  ATTR_TYPE_INT, ANVIL_ATTR_STATUS, &status,
			  ATTR_TYPE_INT, ANVIL_ATTR_COUNT, count,
			  ATTR_TYPE_INT, ANVIL_ATTR_RATE, rate,
			  ATTR_TYPE_END) != 3)
	status = ANVIL_STAT_FAIL;
    else if (status != ANVIL_STAT_OK)
	status = ANVIL_STAT_FAIL;
    myfree(ident);
    return (status);
}

/* anvil_clnt_mail - heads-up and status query */

int     anvil_clnt_mail(ANVIL_CLNT *anvil_clnt, const char *service,
			        const char *addr, int *msgs)
{
    char   *ident = ANVIL_IDENT(service, addr);
    int     status;

    if (attr_clnt_request((ATTR_CLNT *) anvil_clnt,
			  ATTR_FLAG_NONE,	/* Query attributes. */
			  ATTR_TYPE_STR, ANVIL_ATTR_REQ, ANVIL_REQ_MAIL,
			  ATTR_TYPE_STR, ANVIL_ATTR_IDENT, ident,
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply attributes. */
			  ATTR_TYPE_INT, ANVIL_ATTR_STATUS, &status,
			  ATTR_TYPE_INT, ANVIL_ATTR_RATE, msgs,
			  ATTR_TYPE_END) != 2)
	status = ANVIL_STAT_FAIL;
    else if (status != ANVIL_STAT_OK)
	status = ANVIL_STAT_FAIL;
    myfree(ident);
    return (status);
}

/* anvil_clnt_rcpt - heads-up and status query */

int     anvil_clnt_rcpt(ANVIL_CLNT *anvil_clnt, const char *service,
			        const char *addr, int *rcpts)
{
    char   *ident = ANVIL_IDENT(service, addr);
    int     status;

    if (attr_clnt_request((ATTR_CLNT *) anvil_clnt,
			  ATTR_FLAG_NONE,	/* Query attributes. */
			  ATTR_TYPE_STR, ANVIL_ATTR_REQ, ANVIL_REQ_RCPT,
			  ATTR_TYPE_STR, ANVIL_ATTR_IDENT, ident,
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply attributes. */
			  ATTR_TYPE_INT, ANVIL_ATTR_STATUS, &status,
			  ATTR_TYPE_INT, ANVIL_ATTR_RATE, rcpts,
			  ATTR_TYPE_END) != 2)
	status = ANVIL_STAT_FAIL;
    else if (status != ANVIL_STAT_OK)
	status = ANVIL_STAT_FAIL;
    myfree(ident);
    return (status);
}

/* anvil_clnt_newtls - heads-up and status query */

int     anvil_clnt_newtls(ANVIL_CLNT *anvil_clnt, const char *service,
			          const char *addr, int *newtls)
{
    char   *ident = ANVIL_IDENT(service, addr);
    int     status;

    if (attr_clnt_request((ATTR_CLNT *) anvil_clnt,
			  ATTR_FLAG_NONE,	/* Query attributes. */
			  ATTR_TYPE_STR, ANVIL_ATTR_REQ, ANVIL_REQ_NTLS,
			  ATTR_TYPE_STR, ANVIL_ATTR_IDENT, ident,
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply attributes. */
			  ATTR_TYPE_INT, ANVIL_ATTR_STATUS, &status,
			  ATTR_TYPE_INT, ANVIL_ATTR_RATE, newtls,
			  ATTR_TYPE_END) != 2)
	status = ANVIL_STAT_FAIL;
    else if (status != ANVIL_STAT_OK)
	status = ANVIL_STAT_FAIL;
    myfree(ident);
    return (status);
}

/* anvil_clnt_newtls_stat - status query */

int     anvil_clnt_newtls_stat(ANVIL_CLNT *anvil_clnt, const char *service,
			               const char *addr, int *newtls)
{
    char   *ident = ANVIL_IDENT(service, addr);
    int     status;

    if (attr_clnt_request((ATTR_CLNT *) anvil_clnt,
			  ATTR_FLAG_NONE,	/* Query attributes. */
			  ATTR_TYPE_STR, ANVIL_ATTR_REQ, ANVIL_REQ_NTLS_STAT,
			  ATTR_TYPE_STR, ANVIL_ATTR_IDENT, ident,
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply attributes. */
			  ATTR_TYPE_INT, ANVIL_ATTR_STATUS, &status,
			  ATTR_TYPE_INT, ANVIL_ATTR_RATE, newtls,
			  ATTR_TYPE_END) != 2)
	status = ANVIL_STAT_FAIL;
    else if (status != ANVIL_STAT_OK)
	status = ANVIL_STAT_FAIL;
    myfree(ident);
    return (status);
}

/* anvil_clnt_disconnect - heads-up only */

int     anvil_clnt_disconnect(ANVIL_CLNT *anvil_clnt, const char *service,
			              const char *addr)
{
    char   *ident = ANVIL_IDENT(service, addr);
    int     status;

    if (attr_clnt_request((ATTR_CLNT *) anvil_clnt,
			  ATTR_FLAG_NONE,	/* Query attributes. */
			  ATTR_TYPE_STR, ANVIL_ATTR_REQ, ANVIL_REQ_DISC,
			  ATTR_TYPE_STR, ANVIL_ATTR_IDENT, ident,
			  ATTR_TYPE_END,
			  ATTR_FLAG_MISSING,	/* Reply attributes. */
			  ATTR_TYPE_INT, ANVIL_ATTR_STATUS, &status,
			  ATTR_TYPE_END) != 1)
	status = ANVIL_STAT_FAIL;
    else if (status != ANVIL_STAT_OK)
	status = ANVIL_STAT_FAIL;
    myfree(ident);
    return (status);
}

#ifdef TEST

 /*
  * Stand-alone client for testing.
  */
#include <unistd.h>
#include <string.h>
#include <msg_vstream.h>
#include <mail_conf.h>
#include <mail_params.h>
#include <vstring_vstream.h>

static void usage(void)
{
    vstream_printf("usage: "
		   ANVIL_REQ_CONN " service addr | "
		   ANVIL_REQ_DISC " service addr | "
		   ANVIL_REQ_MAIL " service addr | "
		   ANVIL_REQ_RCPT " service addr | "
		   ANVIL_REQ_NTLS " service addr | "
		   ANVIL_REQ_NTLS_STAT " service addr | "
		   ANVIL_REQ_LOOKUP " service addr\n");
}

int     main(int unused_argc, char **argv)
{
    VSTRING *inbuf = vstring_alloc(1);
    char   *bufp;
    char   *cmd;
    ssize_t cmd_len;
    char   *service;
    char   *addr;
    int     count;
    int     rate;
    int     msgs;
    int     rcpts;
    int     newtls;
    ANVIL_CLNT *anvil;

    msg_vstream_init(argv[0], VSTREAM_ERR);

    mail_conf_read();
    msg_info("using config files in %s", var_config_dir);
    if (chdir(var_queue_dir) < 0)
	msg_fatal("chdir %s: %m", var_queue_dir);

    msg_verbose++;

    anvil = anvil_clnt_create();

    while (vstring_fgets_nonl(inbuf, VSTREAM_IN)) {
	bufp = vstring_str(inbuf);
	if ((cmd = mystrtok(&bufp, " ")) == 0 || *bufp == 0
	    || (service = mystrtok(&bufp, " ")) == 0 || *service == 0
	    || (addr = mystrtok(&bufp, " ")) == 0 || *addr == 0
	    || mystrtok(&bufp, " ") != 0) {
	    vstream_printf("bad command syntax\n");
	    usage();
	    vstream_fflush(VSTREAM_OUT);
	    continue;
	}
	cmd_len = strlen(cmd);
	if (strncmp(cmd, ANVIL_REQ_CONN, cmd_len) == 0) {
	    if (anvil_clnt_connect(anvil, service, addr, &count, &rate) != ANVIL_STAT_OK)
		msg_warn("error!");
	    else
		vstream_printf("count=%d, rate=%d\n", count, rate);
	} else if (strncmp(cmd, ANVIL_REQ_MAIL, cmd_len) == 0) {
	    if (anvil_clnt_mail(anvil, service, addr, &msgs) != ANVIL_STAT_OK)
		msg_warn("error!");
	    else
		vstream_printf("rate=%d\n", msgs);
	} else if (strncmp(cmd, ANVIL_REQ_RCPT, cmd_len) == 0) {
	    if (anvil_clnt_rcpt(anvil, service, addr, &rcpts) != ANVIL_STAT_OK)
		msg_warn("error!");
	    else
		vstream_printf("rate=%d\n", rcpts);
	} else if (strncmp(cmd, ANVIL_REQ_NTLS, cmd_len) == 0) {
	    if (anvil_clnt_newtls(anvil, service, addr, &newtls) != ANVIL_STAT_OK)
		msg_warn("error!");
	    else
		vstream_printf("rate=%d\n", newtls);
	} else if (strncmp(cmd, ANVIL_REQ_NTLS_STAT, cmd_len) == 0) {
	    if (anvil_clnt_newtls_stat(anvil, service, addr, &newtls) != ANVIL_STAT_OK)
		msg_warn("error!");
	    else
		vstream_printf("rate=%d\n", newtls);
	} else if (strncmp(cmd, ANVIL_REQ_DISC, cmd_len) == 0) {
	    if (anvil_clnt_disconnect(anvil, service, addr) != ANVIL_STAT_OK)
		msg_warn("error!");
	    else
		vstream_printf("OK\n");
	} else if (strncmp(cmd, ANVIL_REQ_LOOKUP, cmd_len) == 0) {
	    if (anvil_clnt_lookup(anvil, service, addr, &count, &rate,
				  &msgs, &rcpts, &newtls) != ANVIL_STAT_OK)
		msg_warn("error!");
	    else
		vstream_printf("count=%d, rate=%d msgs=%d rcpts=%d newtls=%d\n",
			       count, rate, msgs, rcpts, newtls);
	} else {
	    vstream_printf("bad command: \"%s\"\n", cmd);
	    usage();
	}
	vstream_fflush(VSTREAM_OUT);
    }
    vstring_free(inbuf);
    anvil_clnt_free(anvil);
    return (0);
}

#endif
