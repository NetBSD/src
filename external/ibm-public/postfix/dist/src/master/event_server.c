/*	$NetBSD: event_server.c,v 1.1.1.2.10.1 2013/02/25 00:27:20 tls Exp $	*/

/*++
/* NAME
/*	event_server 3
/* SUMMARY
/*	skeleton multi-threaded mail subsystem
/* SYNOPSIS
/*	#include <mail_server.h>
/*
/*	NORETURN event_server_main(argc, argv, service, key, value, ...)
/*	int	argc;
/*	char	**argv;
/*	void	(*service)(VSTREAM *stream, char *service_name, char **argv);
/*	int	key;
/*
/*	void	event_server_disconnect(fd)
/*	VSTREAM	*stream;
/*
/*	void	event_server_drain()
/* DESCRIPTION
/*	This module implements a skeleton for multi-threaded
/*	mail subsystems: mail subsystem programs that service multiple
/*	clients at the same time. The resulting program expects to be run
/*	from the \fBmaster\fR process.
/*
/*	event_server_main() is the skeleton entry point. It should be
/*	called from the application main program.  The skeleton does all
/*	the generic command-line processing, initialization of
/*	configurable parameters, and connection management.
/*	Unlike multi_server, this skeleton does not attempt to manage
/*	all the events on a client connection.
/*	The skeleton never returns.
/*
/*	Arguments:
/* .IP "void (*service)(VSTREAM *stream, char *service_name, char **argv)"
/*	A pointer to a function that is called by the skeleton each
/*	time a client connects to the program's service port. The
/*	function is run after the program has optionally dropped
/*	its privileges. The application is responsible for managing
/*	subsequent I/O events on the stream, and is responsible for
/*	calling event_server_disconnect() when the stream is closed.
/*	The stream initial state is non-blocking mode.  The service
/*	name argument corresponds to the service name in the master.cf
/*	file.  The argv argument specifies command-line arguments
/*	left over after options processing.
/* .PP
/*	Optional arguments are specified as a null-terminated (key, value)
/*	list. Keys and expected values are:
/* .IP "MAIL_SERVER_INT_TABLE (CONFIG_INT_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed.
/* .IP "MAIL_SERVER_LONG_TABLE (CONFIG_LONG_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed.
/* .IP "MAIL_SERVER_STR_TABLE (CONFIG_STR_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed.
/* .IP "MAIL_SERVER_BOOL_TABLE (CONFIG_BOOL_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed.
/* .IP "MAIL_SERVER_TIME_TABLE (CONFIG_TIME_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed.
/* .IP "MAIL_SERVER_RAW_TABLE (CONFIG_RAW_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed. Raw parameters are not subjected to $name
/*	evaluation.
/* .IP "MAIL_SERVER_NINT_TABLE (CONFIG_NINT_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed.
/* .IP "MAIL_SERVER_NBOOL_TABLE (CONFIG_NBOOL_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed.
/* .IP "MAIL_SERVER_PRE_INIT (void *(char *service_name, char **argv))"
/*	A pointer to a function that is called once
/*	by the skeleton after it has read the global configuration file
/*	and after it has processed command-line arguments, but before
/*	the skeleton has optionally relinquished the process privileges.
/* .sp
/*	Only the last instance of this parameter type is remembered.
/* .IP "MAIL_SERVER_POST_INIT (void *(char *service_name, char **argv))"
/*	A pointer to a function that is called once
/*	by the skeleton after it has optionally relinquished the process
/*	privileges, but before servicing client connection requests.
/* .sp
/*	Only the last instance of this parameter type is remembered.
/* .IP "MAIL_SERVER_LOOP (int *(char *service_name, char **argv))"
/*	A pointer to function that is executed from
/*	within the event loop, whenever an I/O or timer event has happened,
/*	or whenever nothing has happened for a specified amount of time.
/*	The result value of the function specifies how long to wait until
/*	the next event. Specify -1 to wait for "as long as it takes".
/* .sp
/*	Only the last instance of this parameter type is remembered.
/* .IP "MAIL_SERVER_EXIT (void *(char *service_name, char **argv))"
/*	A pointer to function that is executed immediately before normal
/*	process termination.
/* .IP "MAIL_SERVER_PRE_ACCEPT (void *(char *service_name, char **argv))"
/*	Function to be executed prior to accepting a new connection.
/* .sp
/*	Only the last instance of this parameter type is remembered.
/* .IP "MAIL_SERVER_PRE_DISCONN (VSTREAM *, char *service_name, char **argv)"
/*	A pointer to a function that is called
/*	by the event_server_disconnect() function (see below).
/* .sp
/*	Only the last instance of this parameter type is remembered.
/* .IP "MAIL_SERVER_IN_FLOW_DELAY (none)"
/*	Pause $in_flow_delay seconds when no "mail flow control token"
/*	is available. A token is consumed for each connection request.
/* .IP MAIL_SERVER_SOLITARY
/*	This service must be configured with process limit of 1.
/* .IP MAIL_SERVER_UNLIMITED
/*	This service must be configured with process limit of 0.
/* .IP MAIL_SERVER_PRIVILEGED
/*	This service must be configured as privileged.
/* .IP "MAIL_SERVER_SLOW_EXIT (void *(char *service_name, char **argv))"
/*	A pointer to a function that is called after "postfix reload"
/*	or "master exit".  The application can call event_server_drain()
/*	(see below) to finish ongoing activities in the background.
/* .IP "MAIL_SERVER_WATCHDOG (int *)"
/*	Override the default 1000s watchdog timeout. The value is
/*	used after command-line and main.cf file processing.
/* .PP
/*	event_server_disconnect() should be called by the application
/*	to close a client connection.
/*
/*	event_server_drain() should be called when the application
/*	no longer wishes to accept new client connections. Existing
/*	clients are handled in a background process, and the process
/*	terminates when the last client is disconnected. A non-zero
/*	result means this call should be tried again later.
/*
/*	The var_use_limit variable limits the number of clients
/*	that a server can service before it commits suicide.  This
/*	value is taken from the global \fBmain.cf\fR configuration
/*	file. Setting \fBvar_use_limit\fR to zero disables the
/*	client limit.
/*
/*	The var_idle_limit variable limits the time that a service
/*	receives no client connection requests before it commits
/*	suicide.  This value is taken from the global \fBmain.cf\fR
/*	configuration file. Setting \fBvar_idle_limit\fR to zero
/*	disables the idle limit.
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/* SEE ALSO
/*	master(8), master process
/*	syslogd(8) system logging
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
#include <sys/socket.h>
#include <sys/time.h>			/* select() */
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif
#include <time.h>

#ifdef USE_SYS_SELECT_H
#include <sys/select.h>			/* select() */
#endif

/* Utility library. */

#include <msg.h>
#include <msg_syslog.h>
#include <msg_vstream.h>
#include <chroot_uid.h>
#include <listen.h>
#include <events.h>
#include <vstring.h>
#include <vstream.h>
#include <msg_vstream.h>
#include <mymalloc.h>
#include <iostuff.h>
#include <stringops.h>
#include <sane_accept.h>
#include <myflock.h>
#include <safe_open.h>
#include <listen.h>
#include <watchdog.h>
#include <split_at.h>

/* Global library. */

#include <mail_task.h>
#include <debug_process.h>
#include <mail_params.h>
#include <mail_conf.h>
#include <mail_dict.h>
#include <timed_ipc.h>
#include <resolve_local.h>
#include <mail_flow.h>
#include <mail_version.h>

/* Process manager. */

#include "master_proto.h"

/* Application-specific */

#include "mail_server.h"

 /*
  * Global state.
  */
static int client_count;
static int use_count;
static int socket_count = 1;

static void (*event_server_service) (VSTREAM *, char *, char **);
static char *event_server_name;
static char **event_server_argv;
static void (*event_server_accept) (int, char *);
static void (*event_server_onexit) (char *, char **);
static void (*event_server_pre_accept) (char *, char **);
static VSTREAM *event_server_lock;
static int event_server_in_flow_delay;
static unsigned event_server_generation;
static void (*event_server_pre_disconn) (VSTREAM *, char *, char **);
static void (*event_server_slow_exit) (char *, char **);
static int event_server_watchdog = 1000;

/* event_server_exit - normal termination */

static NORETURN event_server_exit(void)
{
    if (event_server_onexit)
	event_server_onexit(event_server_name, event_server_argv);
    exit(0);
}

/* event_server_abort - terminate after abnormal master exit */

static void event_server_abort(int unused_event, char *unused_context)
{
    if (msg_verbose)
	msg_info("master disconnect -- exiting");
    event_disable_readwrite(MASTER_STATUS_FD);
    if (event_server_slow_exit)
	event_server_slow_exit(event_server_name, event_server_argv);
    else
	event_server_exit();
}

/* event_server_timeout - idle time exceeded */

static void event_server_timeout(int unused_event, char *unused_context)
{
    if (msg_verbose)
	msg_info("idle timeout -- exiting");
    event_server_exit();
}

/*  event_server_drain - stop accepting new clients */

int     event_server_drain(void)
{
    const char *myname = "event_server_drain";
    int     fd;

    switch (fork()) {
	/* Try again later. */
    case -1:
	return (-1);
	/* Finish existing clients in the background, then terminate. */
    case 0:
	(void) msg_cleanup((MSG_CLEANUP_FN) 0);
	event_fork();
	for (fd = MASTER_LISTEN_FD; fd < MASTER_LISTEN_FD + socket_count; fd++) {
	    event_disable_readwrite(fd);
	    (void) close(fd);
	    /* Play safe - don't reuse this file number. */
	    if (DUP2(STDIN_FILENO, fd) < 0)
		msg_warn("%s: dup2(%d, %d): %m", myname, STDIN_FILENO, fd);
	}
	var_use_limit = 1;
	return (0);
	/* Let the master start a new process. */
    default:
	exit(0);
    }
}

/* event_server_disconnect - terminate client session */

void    event_server_disconnect(VSTREAM *stream)
{
    if (msg_verbose)
	msg_info("connection closed fd %d", vstream_fileno(stream));
    if (event_server_pre_disconn)
	event_server_pre_disconn(stream, event_server_name, event_server_argv);
    (void) vstream_fclose(stream);
    client_count--;
    /* Avoid integer wrap-around in a persistent process.  */
    if (use_count < INT_MAX)
	use_count++;
    if (client_count == 0 && var_idle_limit > 0)
	event_request_timer(event_server_timeout, (char *) 0, var_idle_limit);
}

/* event_server_execute - in case (char *) != (struct *) */

static void event_server_execute(int unused_event, char *context)
{
    VSTREAM *stream = (VSTREAM *) context;

    if (event_server_lock != 0
	&& myflock(vstream_fileno(event_server_lock), INTERNAL_LOCK,
		   MYFLOCK_OP_NONE) < 0)
	msg_fatal("select unlock: %m");

    /*
     * Do bother the application when the client disconnected. Don't drop the
     * already accepted client request after "postfix reload"; that would be
     * rude.
     */
    if (master_notify(var_pid, event_server_generation, MASTER_STAT_TAKEN) < 0)
	 /* void */ ;
    event_server_service(stream, event_server_name, event_server_argv);
    if (master_notify(var_pid, event_server_generation, MASTER_STAT_AVAIL) < 0)
	event_server_abort(EVENT_NULL_TYPE, EVENT_NULL_CONTEXT);
}

/* event_server_wakeup - wake up application */

static void event_server_wakeup(int fd)
{
    VSTREAM *stream;
    char   *tmp;

#if defined(F_DUPFD) && (EVENTS_STYLE != EVENTS_STYLE_SELECT)
#ifndef THRESHOLD_FD_WORKAROUND
#define THRESHOLD_FD_WORKAROUND 128
#endif
    int     new_fd;

    /*
     * Leave some handles < FD_SETSIZE for DBMS libraries, in the unlikely
     * case of a multi-server with a thousand clients.
     */
    if (fd < THRESHOLD_FD_WORKAROUND) {
	if ((new_fd = fcntl(fd, F_DUPFD, THRESHOLD_FD_WORKAROUND)) < 0)
	    msg_fatal("fcntl F_DUPFD: %m");
	(void) close(fd);
	fd = new_fd;
    }
#endif
    if (msg_verbose)
	msg_info("connection established fd %d", fd);
    non_blocking(fd, BLOCKING);
    close_on_exec(fd, CLOSE_ON_EXEC);
    client_count++;
    stream = vstream_fdopen(fd, O_RDWR);
    tmp = concatenate(event_server_name, " socket", (char *) 0);
    vstream_control(stream, VSTREAM_CTL_PATH, tmp, VSTREAM_CTL_END);
    myfree(tmp);
    timed_ipc_setup(stream);
    if (event_server_in_flow_delay && mail_flow_get(1) < 0)
	event_request_timer(event_server_execute, (char *) stream,
			    var_in_flow_delay);
    else
	event_server_execute(0, (char *) stream);
}

/* event_server_accept_local - accept client connection request */

static void event_server_accept_local(int unused_event, char *context)
{
    int     listen_fd = CAST_CHAR_PTR_TO_INT(context);
    int     time_left = -1;
    int     fd;

    /*
     * Be prepared for accept() to fail because some other process already
     * got the connection (the number of processes competing for clients is
     * kept small, so this is not a "thundering herd" problem). If the
     * accept() succeeds, be sure to disable non-blocking I/O, in order to
     * minimize confusion.
     */
    if (client_count == 0 && var_idle_limit > 0)
	time_left = event_cancel_timer(event_server_timeout, (char *) 0);

    if (event_server_pre_accept)
	event_server_pre_accept(event_server_name, event_server_argv);
    fd = LOCAL_ACCEPT(listen_fd);
    if (event_server_lock != 0
	&& myflock(vstream_fileno(event_server_lock), INTERNAL_LOCK,
		   MYFLOCK_OP_NONE) < 0)
	msg_fatal("select unlock: %m");
    if (fd < 0) {
	if (errno != EAGAIN)
	    msg_error("accept connection: %m");
	if (time_left >= 0)
	    event_request_timer(event_server_timeout, (char *) 0, time_left);
	return;
    }
    event_server_wakeup(fd);
}

#ifdef MASTER_XPORT_NAME_PASS

/* event_server_accept_pass - accept descriptor */

static void event_server_accept_pass(int unused_event, char *context)
{
    int     listen_fd = CAST_CHAR_PTR_TO_INT(context);
    int     time_left = -1;
    int     fd;

    /*
     * Be prepared for accept() to fail because some other process already
     * got the connection (the number of processes competing for clients is
     * kept small, so this is not a "thundering herd" problem). If the
     * accept() succeeds, be sure to disable non-blocking I/O, in order to
     * minimize confusion.
     */
    if (client_count == 0 && var_idle_limit > 0)
	time_left = event_cancel_timer(event_server_timeout, (char *) 0);

    if (event_server_pre_accept)
	event_server_pre_accept(event_server_name, event_server_argv);
    fd = PASS_ACCEPT(listen_fd);
    if (event_server_lock != 0
	&& myflock(vstream_fileno(event_server_lock), INTERNAL_LOCK,
		   MYFLOCK_OP_NONE) < 0)
	msg_fatal("select unlock: %m");
    if (fd < 0) {
	if (errno != EAGAIN)
	    msg_error("accept connection: %m");
	if (time_left >= 0)
	    event_request_timer(event_server_timeout, (char *) 0, time_left);
	return;
    }
    event_server_wakeup(fd);
}

#endif

/* event_server_accept_inet - accept client connection request */

static void event_server_accept_inet(int unused_event, char *context)
{
    int     listen_fd = CAST_CHAR_PTR_TO_INT(context);
    int     time_left = -1;
    int     fd;

    /*
     * Be prepared for accept() to fail because some other process already
     * got the connection (the number of processes competing for clients is
     * kept small, so this is not a "thundering herd" problem). If the
     * accept() succeeds, be sure to disable non-blocking I/O, in order to
     * minimize confusion.
     */
    if (client_count == 0 && var_idle_limit > 0)
	time_left = event_cancel_timer(event_server_timeout, (char *) 0);

    if (event_server_pre_accept)
	event_server_pre_accept(event_server_name, event_server_argv);
    fd = inet_accept(listen_fd);
    if (event_server_lock != 0
	&& myflock(vstream_fileno(event_server_lock), INTERNAL_LOCK,
		   MYFLOCK_OP_NONE) < 0)
	msg_fatal("select unlock: %m");
    if (fd < 0) {
	if (errno != EAGAIN)
	    msg_error("accept connection: %m");
	if (time_left >= 0)
	    event_request_timer(event_server_timeout, (char *) 0, time_left);
	return;
    }
    event_server_wakeup(fd);
}

/* event_server_main - the real main program */

NORETURN event_server_main(int argc, char **argv, MULTI_SERVER_FN service,...)
{
    const char *myname = "event_server_main";
    VSTREAM *stream = 0;
    char   *root_dir = 0;
    char   *user_name = 0;
    int     debug_me = 0;
    int     daemon_mode = 1;
    char   *service_name = basename(argv[0]);
    int     delay;
    int     c;
    int     fd;
    va_list ap;
    MAIL_SERVER_INIT_FN pre_init = 0;
    MAIL_SERVER_INIT_FN post_init = 0;
    MAIL_SERVER_LOOP_FN loop = 0;
    int     key;
    char   *transport = 0;

#if 0
    char   *lock_path;
    VSTRING *why;

#endif
    int     alone = 0;
    int     zerolimit = 0;
    WATCHDOG *watchdog;
    char   *oname_val;
    char   *oname;
    char   *oval;
    const char *err;
    char   *generation;
    int     msg_vstream_needed = 0;
    int     redo_syslog_init = 0;

    /*
     * Process environment options as early as we can.
     */
    if (getenv(CONF_ENV_VERB))
	msg_verbose = 1;
    if (getenv(CONF_ENV_DEBUG))
	debug_me = 1;

    /*
     * Don't die when a process goes away unexpectedly.
     */
    signal(SIGPIPE, SIG_IGN);

    /*
     * Don't die for frivolous reasons.
     */
#ifdef SIGXFSZ
    signal(SIGXFSZ, SIG_IGN);
#endif

    /*
     * May need this every now and then.
     */
    var_procname = mystrdup(basename(argv[0]));
    set_mail_conf_str(VAR_PROCNAME, var_procname);

    /*
     * Initialize logging and exit handler. Do the syslog first, so that its
     * initialization completes before we enter the optional chroot jail.
     */
    msg_syslog_init(mail_task(var_procname), LOG_PID, LOG_FACILITY);
    if (msg_verbose)
	msg_info("daemon started");

    /*
     * Check the Postfix library version as soon as we enable logging.
     */
    MAIL_VERSION_CHECK;

    /*
     * Initialize from the configuration file. Allow command-line options to
     * override compiled-in defaults or configured parameter values.
     */
    mail_conf_suck();

    /*
     * Register dictionaries that use higher-level interfaces and protocols.
     */
    mail_dict_init();

    /*
     * After database open error, continue execution with reduced
     * functionality.
     */
    dict_allow_surrogate = 1;

    /*
     * Pick up policy settings from master process. Shut up error messages to
     * stderr, because no-one is going to see them.
     */
    opterr = 0;
    while ((c = GETOPT(argc, argv, "cdDi:lm:n:o:s:St:uvVz")) > 0) {
	switch (c) {
	case 'c':
	    root_dir = "setme";
	    break;
	case 'd':
	    daemon_mode = 0;
	    break;
	case 'D':
	    debug_me = 1;
	    break;
	case 'i':
	    mail_conf_update(VAR_MAX_IDLE, optarg);
	    break;
	case 'l':
	    alone = 1;
	    break;
	case 'm':
	    mail_conf_update(VAR_MAX_USE, optarg);
	    break;
	case 'n':
	    service_name = optarg;
	    break;
	case 'o':
	    oname_val = mystrdup(optarg);
	    if ((err = split_nameval(oname_val, &oname, &oval)) != 0)
		msg_fatal("invalid \"-o %s\" option value: %s", optarg, err);
	    mail_conf_update(oname, oval);
	    if (strcmp(oname, VAR_SYSLOG_NAME) == 0)
		redo_syslog_init = 1;
	    myfree(oname_val);
	    break;
	case 's':
	    if ((socket_count = atoi(optarg)) <= 0)
		msg_fatal("invalid socket_count: %s", optarg);
	    break;
	case 'S':
	    stream = VSTREAM_IN;
	    break;
	case 'u':
	    user_name = "setme";
	    break;
	case 't':
	    transport = optarg;
	    break;
	case 'v':
	    msg_verbose++;
	    break;
	case 'V':
	    if (++msg_vstream_needed == 1)
		msg_vstream_init(mail_task(var_procname), VSTREAM_ERR);
	    break;
	case 'z':
	    zerolimit = 1;
	    break;
	default:
	    msg_fatal("invalid option: %c", c);
	    break;
	}
    }

    /*
     * Initialize generic parameters.
     */
    mail_params_init();
    if (redo_syslog_init)
	msg_syslog_init(mail_task(var_procname), LOG_PID, LOG_FACILITY);

    /*
     * If not connected to stdin, stdin must not be a terminal.
     */
    if (daemon_mode && stream == 0 && isatty(STDIN_FILENO)) {
	msg_vstream_init(var_procname, VSTREAM_ERR);
	msg_fatal("do not run this command by hand");
    }

    /*
     * Application-specific initialization.
     */
    va_start(ap, service);
    while ((key = va_arg(ap, int)) != 0) {
	switch (key) {
	case MAIL_SERVER_INT_TABLE:
	    get_mail_conf_int_table(va_arg(ap, CONFIG_INT_TABLE *));
	    break;
	case MAIL_SERVER_LONG_TABLE:
	    get_mail_conf_long_table(va_arg(ap, CONFIG_LONG_TABLE *));
	    break;
	case MAIL_SERVER_STR_TABLE:
	    get_mail_conf_str_table(va_arg(ap, CONFIG_STR_TABLE *));
	    break;
	case MAIL_SERVER_BOOL_TABLE:
	    get_mail_conf_bool_table(va_arg(ap, CONFIG_BOOL_TABLE *));
	    break;
	case MAIL_SERVER_TIME_TABLE:
	    get_mail_conf_time_table(va_arg(ap, CONFIG_TIME_TABLE *));
	    break;
	case MAIL_SERVER_RAW_TABLE:
	    get_mail_conf_raw_table(va_arg(ap, CONFIG_RAW_TABLE *));
	    break;
	case MAIL_SERVER_NINT_TABLE:
	    get_mail_conf_nint_table(va_arg(ap, CONFIG_NINT_TABLE *));
	    break;
	case MAIL_SERVER_NBOOL_TABLE:
	    get_mail_conf_nbool_table(va_arg(ap, CONFIG_NBOOL_TABLE *));
	    break;
	case MAIL_SERVER_PRE_INIT:
	    pre_init = va_arg(ap, MAIL_SERVER_INIT_FN);
	    break;
	case MAIL_SERVER_POST_INIT:
	    post_init = va_arg(ap, MAIL_SERVER_INIT_FN);
	    break;
	case MAIL_SERVER_LOOP:
	    loop = va_arg(ap, MAIL_SERVER_LOOP_FN);
	    break;
	case MAIL_SERVER_EXIT:
	    event_server_onexit = va_arg(ap, MAIL_SERVER_EXIT_FN);
	    break;
	case MAIL_SERVER_PRE_ACCEPT:
	    event_server_pre_accept = va_arg(ap, MAIL_SERVER_ACCEPT_FN);
	    break;
	case MAIL_SERVER_PRE_DISCONN:
	    event_server_pre_disconn = va_arg(ap, MAIL_SERVER_DISCONN_FN);
	    break;
	case MAIL_SERVER_IN_FLOW_DELAY:
	    event_server_in_flow_delay = 1;
	    break;
	case MAIL_SERVER_SOLITARY:
	    if (stream == 0 && !alone)
		msg_fatal("service %s requires a process limit of 1",
			  service_name);
	    break;
	case MAIL_SERVER_UNLIMITED:
	    if (stream == 0 && !zerolimit)
		msg_fatal("service %s requires a process limit of 0",
			  service_name);
	    break;
	case MAIL_SERVER_PRIVILEGED:
	    if (user_name)
		msg_fatal("service %s requires privileged operation",
			  service_name);
	    break;
	case MAIL_SERVER_WATCHDOG:
	    event_server_watchdog = *va_arg(ap, int *);
	    break;
	case MAIL_SERVER_SLOW_EXIT:
	    event_server_slow_exit = va_arg(ap, MAIL_SERVER_SLOW_EXIT_FN);
	    break;
	default:
	    msg_panic("%s: unknown argument type: %d", myname, key);
	}
    }
    va_end(ap);

    if (root_dir)
	root_dir = var_queue_dir;
    if (user_name)
	user_name = var_mail_owner;

    /*
     * Can options be required?
     */
    if (stream == 0) {
	if (transport == 0)
	    msg_fatal("no transport type specified");
	if (strcasecmp(transport, MASTER_XPORT_NAME_INET) == 0)
	    event_server_accept = event_server_accept_inet;
	else if (strcasecmp(transport, MASTER_XPORT_NAME_UNIX) == 0)
	    event_server_accept = event_server_accept_local;
#ifdef MASTER_XPORT_NAME_PASS
	else if (strcasecmp(transport, MASTER_XPORT_NAME_PASS) == 0)
	    event_server_accept = event_server_accept_pass;
#endif
	else
	    msg_fatal("unsupported transport type: %s", transport);
    }

    /*
     * Retrieve process generation from environment.
     */
    if ((generation = getenv(MASTER_GEN_NAME)) != 0) {
	if (!alldig(generation))
	    msg_fatal("bad generation: %s", generation);
	OCTAL_TO_UNSIGNED(event_server_generation, generation);
	if (msg_verbose)
	    msg_info("process generation: %s (%o)",
		     generation, event_server_generation);
    }

    /*
     * Optionally start the debugger on ourself.
     */
    if (debug_me)
	debug_process();

    /*
     * Traditionally, BSD select() can't handle multiple processes selecting
     * on the same socket, and wakes up every process in select(). See TCP/IP
     * Illustrated volume 2 page 532. We avoid select() collisions with an
     * external lock file.
     */

    /*
     * XXX Can't compete for exclusive access to the listen socket because we
     * also have to monitor existing client connections for service requests.
     */
#if 0
    if (stream == 0 && !alone) {
	lock_path = concatenate(DEF_PID_DIR, "/", transport,
				".", service_name, (char *) 0);
	why = vstring_alloc(1);
	if ((event_server_lock = safe_open(lock_path, O_CREAT | O_RDWR, 0600,
				      (struct stat *) 0, -1, -1, why)) == 0)
	    msg_fatal("open lock file %s: %s", lock_path, vstring_str(why));
	close_on_exec(vstream_fileno(event_server_lock), CLOSE_ON_EXEC);
	myfree(lock_path);
	vstring_free(why);
    }
#endif

    /*
     * Set up call-back info.
     */
    event_server_service = service;
    event_server_name = service_name;
    event_server_argv = argv + optind;

    /*
     * Run pre-jail initialization.
     */
    if (chdir(var_queue_dir) < 0)
	msg_fatal("chdir(\"%s\"): %m", var_queue_dir);
    if (pre_init)
	pre_init(event_server_name, event_server_argv);

    /*
     * Optionally, restrict the damage that this process can do.
     */
    resolve_local_init();
    tzset();
    chroot_uid(root_dir, user_name);

    /*
     * Run post-jail initialization.
     */
    if (post_init)
	post_init(event_server_name, event_server_argv);

    /*
     * Are we running as a one-shot server with the client connection on
     * standard input? If so, make sure the output is written to stdout so as
     * to satisfy common expectation.
     */
    if (stream != 0) {
	vstream_control(stream,
			VSTREAM_CTL_DOUBLE,
			VSTREAM_CTL_WRITE_FD, STDOUT_FILENO,
			VSTREAM_CTL_END);
	service(stream, event_server_name, event_server_argv);
	vstream_fflush(stream);
	event_server_exit();
    }

    /*
     * Running as a semi-resident server. Service connection requests.
     * Terminate when we have serviced a sufficient number of clients, when
     * no-one has been talking to us for a configurable amount of time, or
     * when the master process terminated abnormally.
     */
    if (var_idle_limit > 0)
	event_request_timer(event_server_timeout, (char *) 0, var_idle_limit);
    for (fd = MASTER_LISTEN_FD; fd < MASTER_LISTEN_FD + socket_count; fd++) {
	event_enable_read(fd, event_server_accept, CAST_INT_TO_CHAR_PTR(fd));
	close_on_exec(fd, CLOSE_ON_EXEC);
    }
    event_enable_read(MASTER_STATUS_FD, event_server_abort, (char *) 0);
    close_on_exec(MASTER_STATUS_FD, CLOSE_ON_EXEC);
    close_on_exec(MASTER_FLOW_READ, CLOSE_ON_EXEC);
    close_on_exec(MASTER_FLOW_WRITE, CLOSE_ON_EXEC);
    watchdog = watchdog_create(event_server_watchdog,
			       (WATCHDOG_FN) 0, (char *) 0);

    /*
     * The event loop, at last.
     */
    while (var_use_limit == 0 || use_count < var_use_limit || client_count > 0) {
	if (event_server_lock != 0) {
	    watchdog_stop(watchdog);
	    if (myflock(vstream_fileno(event_server_lock), INTERNAL_LOCK,
			MYFLOCK_OP_EXCLUSIVE) < 0)
		msg_fatal("select lock: %m");
	}
	watchdog_start(watchdog);
	delay = loop ? loop(event_server_name, event_server_argv) : -1;
	event_loop(delay);
    }
    event_server_exit();
}
