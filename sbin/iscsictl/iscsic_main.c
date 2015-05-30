/*	$NetBSD: iscsic_main.c,v 1.6 2015/05/30 15:57:32 joerg Exp $	*/

/*-
 * Copyright (c) 2005,2006,2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wasabi Systems, Inc.
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
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "iscsic_globals.h"

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>

#define DEVICE    "/dev/iscsi0"

#define ISCSICTL_VERSION	"20110805"

/*
 *-------- Table of commands and the associated handler function -------------
*/

static command_t cmds[] = {
	{"version", get_version},
	{"add_target", add_target},
	{"add_portal", add_portal},
	{"remove_target", remove_target},
	{"slp_find_targets", slp_find_targets},
	{"refresh_targets", refresh_targets},
	{"list_targets", list_targets},
	{"add_send_target", add_send_target},
	{"remove_send_target", remove_send_target},
	{"list_send_targets", list_send_targets},
	{"add_isns_server", add_isns_server},
	{"remove_isns_server", remove_isns_server},
	{"find_isns_servers", find_isns_servers},
	{"list_isns_servers", list_isns_servers},
	{"refresh_isns", refresh_isns},
	{"login", login},
	{"logout", logout},
	{"add_connection", add_connection},
	{"remove_connection", remove_connection},
	{"inquiry", inquiry},
	{"read_capacity", read_capacity},
	{"report_luns", report_luns},
	{"test_unit_ready", test_unit_ready},
	{"add_initiator", add_initiator},
	{"remove_initiator", remove_initiator},
	{"list_initiators", list_initiators},
	{"list_sessions", list_sessions},
	{"set_node_name", set_node_name},
	{NULL, NULL}
};


/*
 *-------- Table of error codes and the associated message text -------------
*/

typedef struct {
	unsigned	 code;
	const char	*str;
} status_msg_t;

static status_msg_t status_msg[] = {
	{ISCSI_STATUS_LIST_EMPTY, "The list is empty"},
	{ISCSI_STATUS_DUPLICATE_NAME, "The specified name is not unique"},
	{ISCSI_STATUS_GENERAL_ERROR, "A non-specific error occurred"},
	{ISCSI_STATUS_LOGIN_FAILED, "The login failed"},
	{ISCSI_STATUS_CONNECTION_FAILED, "The attempt to establish a connection failed"},
	{ISCSI_STATUS_AUTHENTICATION_FAILED, "Authentication negotiation failed"},
	{ISCSI_STATUS_NO_RESOURCES, "Could not allocate resources (e.g. memory)"},
	{ISCSI_STATUS_MAXED_CONNECTIONS, "Maximum number of connections exceeded"},
	{ISCSI_STATUS_INVALID_SESSION_ID, "Session ID not found"},
	{ISCSI_STATUS_INVALID_CONNECTION_ID, "Connection ID not found"},
	{ISCSI_STATUS_INVALID_SOCKET, "Specified socket is invalid"},
	{ISCSI_STATUS_NOTIMPL, "Feature not implemented"},
	{ISCSI_STATUS_CHECK_CONDITION, "Target reported CHECK CONDITION"},
	{ISCSI_STATUS_TARGET_BUSY, "Target reported BUSY"},
	{ISCSI_STATUS_TARGET_ERROR, "Target reported other error"},
	{ISCSI_STATUS_TARGET_FAILURE, "Command Response was Target Failure"},
	{ISCSI_STATUS_TARGET_DROP, "Target dropped connection"},
	{ISCSI_STATUS_SOCKET_ERROR, "Communication failure"},
	{ISCSI_STATUS_PARAMETER_MISSING, "A required ioctl parameter is missing"},
	{ISCSI_STATUS_PARAMETER_INVALID, "A parameter is malformed (string too long etc.)"},
	{ISCSI_STATUS_MAP_FAILED, "Mapping the LUNs failed"},
	{ISCSI_STATUS_NO_INITIATOR_NAME, "Initiator name not set"},
	{ISCSI_STATUS_NEGOTIATION_ERROR, "Negotiation failure (invalid key or value)"},
	{ISCSI_STATUS_TIMEOUT, "Command timed out (at iSCSI level)"},
	{ISCSI_STATUS_PROTOCOL_ERROR, "Internal Error (Protocol error reject)"},
	{ISCSI_STATUS_PDU_ERROR, "Internal Error (Invalid PDU field reject)"},
	{ISCSI_STATUS_CMD_NOT_SUPPORTED, "Target does not support iSCSI command"},
	{ISCSI_STATUS_DRIVER_UNLOAD, "Driver is unloading"},
	{ISCSI_STATUS_LOGOUT, "Session was logged out"},
	{ISCSI_STATUS_PDUS_LOST, "Excessive PDU loss"},
	{ISCSI_STATUS_INVALID_EVENT_ID, "Invalid Event ID"},
	{ISCSI_STATUS_EVENT_DEREGISTERED, "Wait for event cancelled by deregistration"},
	{ISCSI_STATUS_EVENT_WAITING, "Already waiting for event"},
	{ISCSI_STATUS_TASK_NOT_FOUND, "Task Management: task not found"},
	{ISCSI_STATUS_LUN_NOT_FOUND, "Task Management: LUN not found"},
	{ISCSI_STATUS_TASK_ALLEGIANT, "Task Management: Task still allegiant"},
	{ISCSI_STATUS_CANT_REASSIGN, "Task Management: Task reassignment not supported"},
	{ISCSI_STATUS_FUNCTION_UNSUPPORTED, "Task Management: Function unsupported"},
	{ISCSI_STATUS_FUNCTION_NOT_AUTHORIZED, "Task Management: Function not authorized"},
	{ISCSI_STATUS_FUNCTION_REJECTED, "Task Management: Function rejected"},
	{ISCSI_STATUS_UNKNOWN_REASON, "Task Management: Unknown reason code"},
	{ISCSI_STATUS_DUPLICATE_ID, "Duplicate ID"},
	{ISCSI_STATUS_INVALID_ID, "ID not found"},
	{ISCSI_STATUS_TARGET_LOGOUT, "Target requested logout"},
	{ISCSI_STATUS_LOGOUT_CID_NOT_FOUND, "Logout error: Connection ID not found"},
	{ISCSI_STATUS_LOGOUT_RECOVERY_NS, "Logout error: Recovery not supported"},
	{ISCSI_STATUS_LOGOUT_ERROR, "Logout error: Unknown reason"},

	{ISCSID_STATUS_LIST_EMPTY, "The list is empty"},
	{ISCSID_STATUS_DUPLICATE_NAME, "The specified name is not unique"},
	{ISCSID_STATUS_GENERAL_ERROR, "A non-specific error occurred"},
	{ISCSID_STATUS_CONNECT_ERROR, "Failed to connect to target"},
	{ISCSID_STATUS_NO_RESOURCES, "Could not allocate resources (e.g. memory)"},
	{ISCSID_STATUS_INVALID_SESSION_ID, "Session ID not found"},
	{ISCSID_STATUS_INVALID_CONNECTION_ID, "Connection ID not found"},
	{ISCSID_STATUS_NOTIMPL, "Feature not implemented"},
	{ISCSID_STATUS_SOCKET_ERROR, "Failed to create socket"},
	{ISCSID_STATUS_PARAMETER_MISSING, "A required parameter is missing"},
	{ISCSID_STATUS_PARAMETER_INVALID, "Request parameter is invalid"},
	{ISCSID_STATUS_NO_INITIATOR_NAME, "Initiator name was not set"},
	{ISCSID_STATUS_TIMEOUT, "Request timed out"},
	{ISCSID_STATUS_DRIVER_NOT_LOADED, "iSCSI Driver not loaded"},
	{ISCSID_STATUS_INVALID_REQUEST, "Unknown request code"},
	{ISCSID_STATUS_INVALID_PORTAL_ID, "Portal ID not found"},
	{ISCSID_STATUS_INVALID_TARGET_ID, "Target ID not found"},
	{ISCSID_STATUS_NOT_FOUND, "Requested item not found"},
	{ISCSID_STATUS_HOST_NOT_FOUND, "Target address not found"},
	{ISCSID_STATUS_HOST_TRY_AGAIN, "Target address retrieval failed, try again later"},
	{ISCSID_STATUS_HOST_ERROR, "Target address invalid"},
	{ISCSID_STATUS_NO_TARGETS_FOUND, "No targets found"},
	{ISCSID_STATUS_INVALID_ISNS_ID, "iSNS ID not found"},
	{ISCSID_STATUS_ISNS_ERROR, "Problem connecting to iSNS"},
	{ISCSID_STATUS_ISNS_SERVER_ERROR, "iSNS server returned garbage"},
	{ISCSID_STATUS_DUPLICATE_ENTRY, "The entry already exists"},
	{ISCSID_STATUS_INVALID_INITIATOR_ID, "Initiator ID not found"},
	{ISCSID_STATUS_INITIATOR_BIND_ERROR, "Bind to initiator portal failed"},

	{0, NULL}
};

/* -------------------------------------------------------------------------- */
/* local variables */

static struct sockaddr_un	daemon_name;	/* daemon socket name */

static char		 	sockdir[MAXPATHLEN]; /* where myname lives */
static struct sockaddr_un	myname;		/* my socket name */
static int			sock;		/* the socket */

static char *cmdname;			/* pointer to command name for error msgs */

/* global variables */

uint8_t buf[BUF_SIZE];			/* buffer for daemon comm and driver I/O */

int driver;						/* driver handle */

/* -------------------------------------------------------------------------- */

#define progname  getprogname()


/*
 * bye:
 *    Cleanup and exit. Does not return.
*/

__dead static void
bye(void)
{
	close(sock);
	(void) unlink(myname.sun_path);
	(void) rmdir(sockdir);
	exit(EXIT_FAILURE);
}


/*
 * arg_error:
 *    Display error message about an invalid argument, exit.
 *
 *    Parameters:
 *       argp        Argument value
 *       fmt         Error message
 *       ...         additional output arguments
 *
 *    Does not return.
*/

void
arg_error(char *argp, const char *fmt, ...)
{
	va_list args;
	char	lbuf[BUF_SIZE];

	va_start(args, fmt);
	vsnprintf(lbuf, sizeof(lbuf), fmt, args);
	fprintf(stderr, "%s: %s: Invalid option at or near '%s': %s\n",
		progname, cmdname, argp, lbuf);
	bye();
}


/*
 * arg_missing:
 *    Display error message about a missing argument, exit.
 *
 *    Parameters:
 *       arg      Argument name
 *
 *    Does not return.
 */

void
arg_missing(const char *arg)
{
	warnx("%s: Missing argument: %s", cmdname, arg);
	bye();
}


/*
 * io_error:
 *    Display error message about an I/O error (includes system error code)
 *
 *    Parameters:
 *       fmt         format string
 *       ...         additional output arguments
 *
 *    Does not return.
 */

void
io_error(const char *fmt, ...)
{
	va_list args;
	char	lbuf[BUF_SIZE];

	va_start(args, fmt);
	vsnprintf(lbuf, sizeof(lbuf), fmt, args);
	fprintf(stderr, "%s: %s: %s: %s\n",
		progname, cmdname, lbuf, strerror(errno));
	bye();
}


/*
 * gen_error:
 *    Display general error message.
 *
 *    Parameters:
 *       fmt         format string
 *       ...         additional output arguments
 *
 *    Does not return.
 */

void
gen_error(const char *fmt, ...)
{
	va_list args;
	char	lbuf[BUF_SIZE];

	va_start(args, fmt);
	vsnprintf(lbuf, sizeof(lbuf), fmt, args);
	fprintf(stderr, "%s: %s: %s\n", progname, cmdname, lbuf);
	bye();
}


/*
 * check_extra_args:
 *    Display error message & exit if there is an unrecognized argument.
 *
 *    Parameters:
 *       argc     Argument count
 *       argv     Argument value array.
 *
 *    Does not return if an extra arg is found.
 */

void
check_extra_args(int argc, char **argv)
{
	int i;

	for (i = 0; i < argc; i++)
		if (argv[i] != NULL) {
			warnx("%s: Unrecognized argument '%s'", cmdname, argv[i]);
			bye();
		}
}


/*
 * status_error:
 *    Display error message for status returned by daemon or driver, exit.
 *
 *    Parameters:
 *          n     Status code.
 *
 *    Does not return.
 */

void
status_error(unsigned n)
{
	int i;

	for (i = 0; status_msg[i].code; i++)
		if (status_msg[i].code == n)
			break;

	if (status_msg[i].code)
		warnx("%s: %s", cmdname, status_msg[i].str);
	else
		warnx("%s: Undefined error code %d", cmdname, n);

	bye();
}


/*
 * status_error_slist:
 *    Display error message for status returned by daemon or driver, but
 *	  replace a "list is empty" code by an "ID not found" code, exit.
 *
 *    Parameters:
 *          n     Status code.
 *
 *    Does not return.
 */

void
status_error_slist(unsigned n)
{
	if (n == ISCSI_STATUS_LIST_EMPTY || n == ISCSID_STATUS_LIST_EMPTY)
		n = ISCSI_STATUS_INVALID_ID;
	status_error (n);
}


/*
 * get_response:
 *    Read the response from the daemon.
 *
 *    Parameters:
 *          temp  If TRUE, the response is dynamically allocated (so it is not
 *                overwritten by further requests or responses).
 *
 *    Returns:
 *          Pointer to the response.
 *
 *    Notes:
 *       This routine allocates an extra integer to mark whether the returned
 *       buffer is dynamic or static. This marker is one int before the
 *       returned pointer.
 */

iscsid_response_t *
get_response(int temp)
{
	ssize_t ret;
	size_t len;
	iscsid_response_t *rsp;
	int *pbuf;

	pbuf = (int *)(void *)buf;
	rsp = (iscsid_response_t *)(void *)&pbuf[1];
	*pbuf = 0;

	/* get size of response */
	len = sizeof(iscsid_response_t);
	ret = recv(sock, rsp, len, MSG_PEEK | MSG_WAITALL);
	if ((size_t)ret != len)
		io_error("Receiving daemon data");

	len += rsp->parameter_length;

	/*
	   if a temp buffer has been requested, or if the response is too large
	   to fit into the static buffer, alloc a temp buffer.
	 */

	temp = temp || (len > (int)(sizeof(buf) - sizeof(int)));

	if (temp) {
		if (NULL == (pbuf = (int *) malloc(len + sizeof(int))))
			gen_error("Can't allocate response buffer (%zu bytes)",
				len + sizeof(int));

		rsp = (iscsid_response_t *)(void *)&pbuf[1];
		*pbuf = 1;
	}
	/* get the complete response */

	ret = recv(sock, rsp, len, MSG_WAITALL);
	if ((size_t)ret != len)
		io_error("Receiving daemon data");

	return rsp;
}


/*
 * free_response:
 *    If the response buffer was dynamically allocated, free it.
 *
 *    Parameters:
 *          rsp   The response buffer.
 *                The dynamic allocation marker is the int preceding
 *                this address.
 */

void
free_response(iscsid_response_t * rsp)
{
	int *pbuf;

	pbuf = ((int *)(void *)rsp) - 1;
	if (*pbuf)
		free(pbuf);
}


/*
 * send_request:
 *    Send a request to the daemon.
 *
 *    Parameters:
 *          request  The request code.
 *          par_len  The parameter length.
 *          par      The parameter.
 */

void
send_request(unsigned request, size_t par_len, void *par)
{
	iscsid_request_t *req;
	size_t len;
	ssize_t ret;
	int req_temp;

	len = sizeof(iscsid_request_t) + par_len;

	/* alloc buffer if static one is too small to hold request */
	req_temp = len > sizeof(buf);

	if (req_temp) {
		req = malloc(len);
		if (req == NULL)
			gen_error("Out of memory allocating %zu bytes\n", len);
	} else
		req = (iscsid_request_t *)(void *)buf;

	/* setup request */
	req->request = request;
	req->parameter_length = (uint32_t)par_len;
	if (par_len)
		memcpy(req->parameter, par, par_len);

	/* and send it out */
	ret = sendto(sock, req, len, 0, (struct sockaddr *)(void *)&daemon_name,
				 (socklen_t)sizeof(struct sockaddr_un));
	if ((size_t)ret != len) {
		io_error("Sending daemon message");
	}
	if (req_temp)
		free(req);
}


/*
 * main:
 *    check command, init driver handle and socket, dispatch command.
 *
 *    Parameter:  argc, argv - passed on to commands with offset of 2, so
 *                argv [0] is first argument after command verb.
 *
 *    Returns:
 *          Whatever the command handler returns, which is currently always 0.
 */

int
main(int argc, char **argv)
{
	command_t	*c;
	int		 res;
	int		 i;

	(void) snprintf(sockdir, sizeof(sockdir), "/tmp/iscsictl.XXXXXX");
	while ((i = getopt(argc, argv, "d:")) != -1) {
		switch(i) {
		case 'd':
			(void) snprintf(sockdir, sizeof(sockdir), "%s", optarg);
			break;
		default:
			break;
		}
	}
	if (argc - optind  < 1) {
		errx(EXIT_FAILURE, "Usage: %s <command> <options>, see manual for details.",
			progname);
	}

	cmdname = argv[optind];

	for (c = cmds; c->cmd != NULL; c++) {
		if (strcmp(c->cmd, cmdname) == 0) {
			break;
		}
	}
	if (c->cmd == NULL) {
		errx(EXIT_FAILURE, "Unknown command: '%s'", cmdname);
	}
	if ((driver = open(DEVICE, O_RDONLY)) < 0)
		err(EXIT_FAILURE, "Opening " DEVICE);

	sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock < 0)
		err(EXIT_FAILURE, "opening datagram socket");

	/* bind socket to unique name */
	if (mkdtemp(sockdir) == NULL) {
		errx(EXIT_FAILURE, "can't create iscsictl dir '%s'", sockdir);
	}
	myname.sun_family = AF_UNIX;
	(void) snprintf(myname.sun_path, sizeof(myname.sun_path), "%s/socket", sockdir);
	if (bind(sock, (struct sockaddr *)(void *)&myname,
	    (socklen_t)sizeof(struct sockaddr_un)) < 0) {
		io_error("Binding name to datagram socket");
	}
	daemon_name.sun_family = AF_UNIX;
	strlcpy(daemon_name.sun_path, ISCSID_SOCK_NAME,
		sizeof(daemon_name.sun_path));

	/* dispatch command */
	res = (*c->proc)(argc - optind - 1, &argv[optind + 1]);

	/* cleanup */
	close(sock);
	unlink(myname.sun_path);
	rmdir(sockdir);

	return res;
}
