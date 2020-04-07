/*	$NetBSD: single_server.c,v 1.2 2017/02/14 01:16:45 christos Exp $	*/

/*++
/* NAME
/*	single_server 3
/* SUMMARY
/*	skeleton single-threaded mail subsystem
/* SYNOPSIS
/*	#include <mail_server.h>
/*
/*	NORETURN single_server_main(argc, argv, service, key, value, ...)
/*	int	argc;
/*	char	**argv;
/*	void	(*service)(VSTREAM *stream, char *service_name, char **argv);
/*	int	key;
/* DESCRIPTION
/*	This module implements a skeleton for single-threaded
/*	mail subsystems: mail subsystem programs that service one
/*	client at a time. The resulting program expects to be run
/*	from the \fBmaster\fR process.
/*
/*	single_server_main() is the skeleton entry point. It should be
/*	called from the application main program.  The skeleton does the
/*	generic command-line options processing, initialization of
/*	configurable parameters, and connection management.
/*	The skeleton never returns.
/*
/*	Arguments:
/* .IP "void (*service)(VSTREAM *fp, char *service_name, char **argv)"
/*	A pointer to a function that is called by the skeleton each time
/*	a client connects to the program's service port. The function is
/*	run after the program has irrevocably dropped its privileges.
/*	The stream initial state is non-blocking mode.
/*	Optional connection attributes are provided as a hash that
/*	is attached as stream context.
/*	The service name argument corresponds to the service name in the
/*	master.cf file.
/*	The argv argument specifies command-line arguments left over
/*	after options processing.
/* .PP
/*	Optional arguments are specified as a null-terminated list
/*	with macros that have zero or more arguments:
/* .IP "CA_MAIL_SERVER_INT_TABLE(CONFIG_INT_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed.
/* .IP "CA_MAIL_SERVER_LONG_TABLE(CONFIG_LONG_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed.
/* .IP "CA_MAIL_SERVER_STR_TABLE(CONFIG_STR_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed.
/* .IP "CA_MAIL_SERVER_BOOL_TABLE(CONFIG_BOOL_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed.
/* .IP "CA_MAIL_SERVER_TIME_TABLE(CONFIG_TIME_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed.
/* .IP "CA_MAIL_SERVER_RAW_TABLE(CONFIG_RAW_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed. Raw parameters are not subjected to $name
/*	evaluation.
/* .IP "CA_MAIL_SERVER_NINT_TABLE(CONFIG_NINT_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed.
/* .IP "CA_MAIL_SERVER_NBOOL_TABLE(CONFIG_NBOOL_TABLE *)"
/*	A table with configurable parameters, to be loaded from the
/*	global Postfix configuration file. Tables are loaded in the
/*	order as specified, and multiple instances of the same type
/*	are allowed.
/* .IP "CA_MAIL_SERVER_PRE_INIT(void *(char *service_name, char **argv))"
/*	A pointer to a function that is called once
/*	by the skeleton after it has read the global configuration file
/*	and after it has processed command-line arguments, but before
/*	the skeleton has optionally relinquished the process privileges.
/* .sp
/*	Only the last instance of this parameter type is remembered.
/* .IP "CA_MAIL_SERVER_POST_INIT(void *(char *service_name, char **argv))"
/*	A pointer to a function that is called once
/*	by the skeleton after it has optionally relinquished the process
/*	privileges, but before servicing client connection requests.
/* .sp
/*	Only the last instance of this parameter type is remembered.
/* .IP "CA_MAIL_SERVER_LOOP(int *(char *service_name, char **argv))"
/*	A pointer to function that is executed from
/*	within the event loop, whenever an I/O or timer event has happened,
/*	or whenever nothing has happened for a specified amount of time.
/*	The result value of the function specifies how long to wait until
/*	the next event. Specify -1 to wait for "as long as it takes".
/* .sp
/*	Only the last instance of this parameter type is remembered.
/* .IP "CA_MAIL_SERVER_EXIT(void *(void))"
/*	A pointer to function that is executed immediately before normal
/*	process termination.
/* .sp
/*	Only the last instance of this parameter type is remembered.
/* .IP "CA_MAIL_SERVER_PRE_ACCEPT(void *(char *service_name, char **argv))"
/*	Function to be executed prior to accepting a new connection.
/* .sp
/*	Only the last instance of this parameter type is remembered.
/* .IP "CA_MAIL_SERVER_IN_FLOW_DELAY(none)"
/*	Pause $in_flow_delay seconds when no "mail flow control token"
/*	is available. A token is consumed for each connection request.
/* .IP CA_MAIL_SERVER_SOLITARY
/*	This service must be configured with process limit of 1.
/* .IP CA_MAIL_SERVER_UNLIMITED
/*	This service must be configured with process limit of 0.
/* .IP CA_MAIL_SERVER_PRIVILEGED
/*	This service must be configured as privileged.
/* .IP "CA_MAIL_SERVER_BOUNCE_INIT(const char *, const char **)"
/*	Initialize the DSN filter for the bounce/defer service
/*	clients with the specified map source and map names.
/* .PP
/*	The var_use_limit variable limits the number of clients that
/*	a server can service before it commits suicide.
/*	This value is taken from the global \fBmain.cf\fR configuration
/*	file. Setting \fBvar_idle_limit\fR to zero disables the client limit.
/*
/*	The var_idle_limit variable limits the time that a service
/*	receives no client connection requests before it commits suicide.
/*	This value is taken from the global \fBmain.cf\fR configuration
/*	file. Setting \fBvar_use_limit\fR to zero disables the idle limit.
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/* BUGS
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

/* Utility library. */

#include <msg.h>
#include <msg_syslog.h>
#include <msg_vstream.h>
#include <chroot_uid.h>
#include <vstring.h>
#include <vstream.h>
#include <msg_vstream.h>
#include <mymalloc.h>
#include <events.h>
#include <iostuff.h>
#include <stringops.h>
#include <sane_accept.h>
#include <myflock.h>
#include <safe_open.h>
#include <listen.h>
#include <watchdog.h>
#include <split_at.h>

/* Global library. */

#include <mail_params.h>
#include <mail_task.h>
#include <debug_process.h>
#include <mail_conf.h>
#include <mail_dict.h>
#include <timed_ipc.h>
#include <resolve_local.h>
#include <mail_flow.h>
#include <mail_version.h>
#include <bounce.h>

/* Process manager. */

#include "master_proto.h"

/* Application-specific */

#include "mail_server.h"

 /*
  * Global state.
  */
static int use_count;

static void (*single_server_service) (VSTREAM *, char *, char **);
static char *single_server_name;
static char **single_server_argv;
static void (*single_server_accept) (int, void *);
static void (*single_server_onexit) (char *, char **);
static void (*single_server_pre_accept) (char *, char **);
static VSTREAM *single_server_lock;
static int single_server_in_flow_delay;
static unsigned single_server_generation;

/* single_server_exit - normal termination */

static NORETURN single_server_exit(void)
{
    if (single_server_onexit)
	single_server_onexit(single_server_name, single_server_argv);
    exit(0);
}

/* single_server_abort - terminate after abnormal master exit */

static void single_server_abort(int unused_event, void *unused_context)
{
    if (msg_verbose)
	msg_info("master disconnect -- exiting");
    single_server_exit();
}

/* single_server_timeout - idle time exceeded */

static void single_server_timeout(int unused_event, void *unused_context)
{
    if (msg_verbose)
	msg_info("idle timeout -- exiting");
    single_server_exit();
}

/* single_server_wakeup - wake up application */

static void single_server_wakeup(int fd, HTABLE *attr)
{
    VSTREAM *stream;
    char   *tmp;

    /*
     * If the accept() succeeds, be sure to disable non-blocking I/O, because
     * the application is supposed to be single-threaded. Notice the master
     * of our (un)availability to service connection requests. Commit suicide
     * when the master process disconnected from us. Don't drop the already
     * accepted client request after "postfix reload"; that would be rude.
     */
    if (msg_verbose)
	msg_info("connection established");
    non_blocking(fd, BLOCKING);
    close_on_exec(fd, CLOSE_ON_EXEC);
    stream = vstream_fdopen(fd, O_RDWR);
    tmp = concatenate(single_server_name, " socket", (char *) 0);
    vstream_control(stream,
		    CA_VSTREAM_CTL_PATH(tmp),
		    CA_VSTREAM_CTL_CONTEXT((void *) attr),
		    CA_VSTREAM_CTL_END);
    myfree(tmp);
    timed_ipc_setup(stream);
    if (master_notify(var_pid, single_server_generation, MASTER_STAT_TAKEN) < 0)
	 /* void */ ;
    if (single_server_in_flow_delay && mail_flow_get(1) < 0)
	doze(var_in_flow_delay * 1000000);
    single_server_service(stream, single_server_name, single_server_argv);
    (void) vstream_fclose(stream);
    if (master_notify(var_pid, single_server_generation, MASTER_STAT_AVAIL) < 0)
	single_server_abort(EVENT_NULL_TYPE, EVENT_NULL_CONTEXT);
    if (msg_verbose)
	msg_info("connection closed");
    /* Avoid integer wrap-around in a persistent process.  */
    if (use_count < INT_MAX)
	use_count++;
    if (var_idle_limit > 0)
	event_request_timer(single_server_timeout, (void *) 0, var_idle_limit);
    if (attr)
	htable_free(attr, myfree);
}

/* single_server_accept_local - accept client connection request */

static void single_server_accept_local(int unused_event, void *context)
{
    int     listen_fd = CAST_ANY_PTR_TO_INT(context);
    int     time_left = -1;
    int     fd;

    /*
     * Be prepared for accept() to fail because some other process already
     * got the connection. We use select() + accept(), instead of simply
     * blocking in accept(), because we must be able to detect that the
     * master process has gone away unexpectedly.
     */
    if (var_idle_limit > 0)
	time_left = event_cancel_timer(single_server_timeout, (void *) 0);

    if (single_server_pre_accept)
	single_server_pre_accept(single_server_name, single_server_argv);
    fd = LOCAL_ACCEPT(listen_fd);
    if (single_server_lock != 0
	&& myflock(vstream_fileno(single_server_lock), INTERNAL_LOCK,
		   MYFLOCK_OP_NONE) < 0)
	msg_fatal("select unlock: %m");
    if (fd < 0) {
	if (errno != EAGAIN)
	    msg_error("accept connection: %m");
	if (time_left >= 0)
	    event_request_timer(single_server_timeout, (void *) 0, time_left);
	return;
    }
    single_server_wakeup(fd, (HTABLE *) 0);
}

#ifdef MASTER_XPORT_NAME_PASS

/* single_server_accept_pass - accept descriptor */

static void single_server_accept_pass(int unused_event, void *context)
{
    int     listen_fd = CAST_ANY_PTR_TO_INT(context);
    int     time_left = -1;
    int     fd;
    HTABLE *attr = 0;

    /*
     * Be prepared for accept() to fail because some other process already
     * got the connection. We use select() + accept(), instead of simply
     * blocking in accept(), because we must be able to detect that the
     * master process has gone away unexpectedly.
     */
    if (var_idle_limit > 0)
	time_left = event_cancel_timer(single_server_timeout, (void *) 0);

    if (single_server_pre_accept)
	single_server_pre_accept(single_server_name, single_server_argv);
    fd = pass_accept_attr(listen_fd, &attr);
    if (single_server_lock != 0
	&& myflock(vstream_fileno(single_server_lock), INTERNAL_LOCK,
		   MYFLOCK_OP_NONE) < 0)
	msg_fatal("select unlock: %m");
    if (fd < 0) {
	if (errno != EAGAIN)
	    msg_error("accept connection: %m");
	if (time_left >= 0)
	    event_request_timer(single_server_timeout, (void *) 0, time_left);
	return;
    }
    single_server_wakeup(fd, attr);
}

#endif

/* single_server_accept_inet - accept client connection request */

static void single_server_accept_inet(int unused_event, void *context)
{
    int     listen_fd = CAST_ANY_PTR_TO_INT(context);
    int     time_left = -1;
    int     fd;

    /*
     * Be prepared for accept() to fail because some other process already
     * got the connection. We use select() + accept(), instead of simply
     * blocking in accept(), because we must be able to detect that the
     * master process has gone away unexpectedly.
     */
    if (var_idle_limit > 0)
	time_left = event_cancel_timer(single_server_timeout, (void *) 0);

    if (single_server_pre_accept)
	single_server_pre_accept(single_server_name, single_server_argv);
    fd = inet_accept(listen_fd);
    if (single_server_lock != 0
	&& myflock(vstream_fileno(single_server_lock), INTERNAL_LOCK,
		   MYFLOCK_OP_NONE) < 0)
	msg_fatal("select unlock: %m");
    if (fd < 0) {
	if (errno != EAGAIN)
	    msg_error("accept connection: %m");
	if (time_left >= 0)
	    event_request_timer(single_server_timeout, (void *) 0, time_left);
	return;
    }
    single_server_wakeup(fd, (HTABLE *) 0);
}

/* single_server_main - the real main program */

NORETURN single_server_main(int argc, char **argv, SINGLE_SERVER_FN service,...)
{
    const char *myname = "single_server_main";
    VSTREAM *stream = 0;
    char   *root_dir = 0;
    char   *user_name = 0;
    int     debug_me = 0;
    int     daemon_mode = 1;
    char   *service_name = basename(argv[0]);
    int     delay;
    int     c;
    int     socket_count = 1;
    int     fd;
    va_list ap;
    MAIL_SERVER_INIT_FN pre_init = 0;
    MAIL_SERVER_INIT_FN post_init = 0;
    MAIL_SERVER_LOOP_FN loop = 0;
    int     key;
    char   *transport = 0;
    char   *lock_path;
    VSTRING *why;
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
    const char *dsn_filter_title;
    const char **dsn_filter_maps;

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
     * Register higher-level dictionaries and initialize the support for
     * dynamically-loaded dictionarles.
     */
    mail_dict_init();

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
	    single_server_onexit = va_arg(ap, MAIL_SERVER_EXIT_FN);
	    break;
	case MAIL_SERVER_PRE_ACCEPT:
	    single_server_pre_accept = va_arg(ap, MAIL_SERVER_ACCEPT_FN);
	    break;
	case MAIL_SERVER_IN_FLOW_DELAY:
	    single_server_in_flow_delay = 1;
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
	case MAIL_SERVER_BOUNCE_INIT:
	    dsn_filter_title = va_arg(ap, const char *);
	    dsn_filter_maps = va_arg(ap, const char **);
	    bounce_client_init(dsn_filter_title, *dsn_filter_maps);
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
	    single_server_accept = single_server_accept_inet;
	else if (strcasecmp(transport, MASTER_XPORT_NAME_UNIX) == 0)
	    single_server_accept = single_server_accept_local;
#ifdef MASTER_XPORT_NAME_PASS
	else if (strcasecmp(transport, MASTER_XPORT_NAME_PASS) == 0)
	    single_server_accept = single_server_accept_pass;
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
	OCTAL_TO_UNSIGNED(single_server_generation, generation);
	if (msg_verbose)
	    msg_info("process generation: %s (%o)",
		     generation, single_server_generation);
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
    if (stream == 0 && !alone) {
	lock_path = concatenate(DEF_PID_DIR, "/", transport,
				".", service_name, (void *) 0);
	why = vstring_alloc(1);
	if ((single_server_lock = safe_open(lock_path, O_CREAT | O_RDWR, 0600,
				      (struct stat *) 0, -1, -1, why)) == 0)
	    msg_fatal("open lock file %s: %s", lock_path, vstring_str(why));
	close_on_exec(vstream_fileno(single_server_lock), CLOSE_ON_EXEC);
	myfree(lock_path);
	vstring_free(why);
    }

    /*
     * Set up call-back info.
     */
    single_server_service = service;
    single_server_name = service_name;
    single_server_argv = argv + optind;

    /*
     * Run pre-jail initialization.
     */
    if (chdir(var_queue_dir) < 0)
	msg_fatal("chdir(\"%s\"): %m", var_queue_dir);
    if (pre_init)
	pre_init(single_server_name, single_server_argv);

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
	post_init(single_server_name, single_server_argv);

    /*
     * Are we running as a one-shot server with the client connection on
     * standard input? If so, make sure the output is written to stdout so as
     * to satisfy common expectation.
     */
    if (stream != 0) {
	vstream_control(stream,
			CA_VSTREAM_CTL_DOUBLE,
			CA_VSTREAM_CTL_WRITE_FD(STDOUT_FILENO),
			CA_VSTREAM_CTL_END);
	service(stream, single_server_name, single_server_argv);
	vstream_fflush(stream);
	single_server_exit();
    }

    /*
     * Running as a semi-resident server. Service connection requests.
     * Terminate when we have serviced a sufficient number of clients, when
     * no-one has been talking to us for a configurable amount of time, or
     * when the master process terminated abnormally.
     */
    if (var_idle_limit > 0)
	event_request_timer(single_server_timeout, (void *) 0, var_idle_limit);
    for (fd = MASTER_LISTEN_FD; fd < MASTER_LISTEN_FD + socket_count; fd++) {
	event_enable_read(fd, single_server_accept, CAST_INT_TO_VOID_PTR(fd));
	close_on_exec(fd, CLOSE_ON_EXEC);
    }
    event_enable_read(MASTER_STATUS_FD, single_server_abort, (void *) 0);
    close_on_exec(MASTER_STATUS_FD, CLOSE_ON_EXEC);
    close_on_exec(MASTER_FLOW_READ, CLOSE_ON_EXEC);
    close_on_exec(MASTER_FLOW_WRITE, CLOSE_ON_EXEC);
    watchdog = watchdog_create(var_daemon_timeout, (WATCHDOG_FN) 0, (void *) 0);

    /*
     * The event loop, at last.
     */
    while (var_use_limit == 0 || use_count < var_use_limit) {
	if (single_server_lock != 0) {
	    watchdog_stop(watchdog);
	    if (myflock(vstream_fileno(single_server_lock), INTERNAL_LOCK,
			MYFLOCK_OP_EXCLUSIVE) < 0)
		msg_fatal("select lock: %m");
	}
	watchdog_start(watchdog);
	delay = loop ? loop(single_server_name, single_server_argv) : -1;
	event_loop(delay);
    }
    single_server_exit();
}
