/*	$NetBSD: main.c,v 1.36 2000/07/16 22:10:13 tron Exp $	*/

/*
 * main.c - Point-to-Point Protocol main module
 *
 * Copyright (c) 1989 Carnegie Mellon University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Carnegie Mellon University.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
#define RCSID	"Id: main.c,v 1.88 1999/12/23 01:28:27 paulus Exp "
#else
__RCSID("$NetBSD: main.c,v 1.36 2000/07/16 22:10:13 tron Exp $");
#endif
#endif

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <netdb.h>
#include <utmp.h>
#include <pwd.h>
#include <setjmp.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "pppd.h"
#include "magic.h"
#include "fsm.h"
#include "lcp.h"
#include "ipcp.h"
#ifdef INET6
#include "ipv6cp.h"
#endif
#include "upap.h"
#include "chap.h"
#include "ccp.h"
#include "pathnames.h"
#include "patchlevel.h"

#ifdef CBCP_SUPPORT
#include "cbcp.h"
#endif

#ifdef IPX_CHANGE
#include "ipxcp.h"
#endif /* IPX_CHANGE */
#ifdef AT_CHANGE
#include "atcp.h"
#endif

#ifdef RCSID
static const char rcsid[] = RCSID;
#endif

/* interface vars */
char ifname[32];		/* Interface name */
int ifunit;			/* Interface unit number */

char *progname;			/* Name of this program */
char hostname[MAXNAMELEN];	/* Our hostname */
static char pidfilename[MAXPATHLEN];	/* name of pid file */
static char linkpidfile[MAXPATHLEN];	/* name of linkname pid file */
static char ppp_devnam[MAXPATHLEN]; /* name of PPP tty (maybe ttypx) */
static uid_t uid;		/* Our real user-id */
static int conn_running;	/* we have a [dis]connector running */

int ttyfd;			/* Serial port file descriptor */
mode_t tty_mode = -1;		/* Original access permissions to tty */
int baud_rate;			/* Actual bits/second for serial device */
int hungup;			/* terminal has been hung up */
int privileged;			/* we're running as real uid root */
int need_holdoff;		/* need holdoff period before restarting */
int detached;			/* have detached from terminal */
struct stat devstat;		/* result of stat() on devnam */
int prepass = 0;		/* doing prepass to find device name */
int devnam_fixed;		/* set while in options.ttyxx file */
volatile int status;		/* exit status for pppd */
int unsuccess;			/* # unsuccessful connection attempts */
int do_callback;		/* != 0 if we should do callback next */
int doing_callback;		/* != 0 if we are doing callback */
char *callback_script;		/* script for doing callback */

int (*holdoff_hook) __P((void)) = NULL;
int (*new_phase_hook) __P((int)) = NULL;

static int fd_ppp = -1;		/* fd for talking PPP */
static int fd_loop;		/* fd for getting demand-dial packets */
static int pty_master;		/* fd for master side of pty */
static int pty_slave;		/* fd for slave side of pty */
static int real_ttyfd;		/* fd for actual serial port (not pty) */

int phase;			/* where the link is at */
int kill_link;
int open_ccp_flag;

static int waiting;
static sigjmp_buf sigjmp;

char **script_env;		/* Env. variable values for scripts */
int s_env_nalloc;		/* # words avail at script_env */

u_char outpacket_buf[PPP_MRU+PPP_HDRLEN]; /* buffer for outgoing packet */
u_char inpacket_buf[PPP_MRU+PPP_HDRLEN]; /* buffer for incoming packet */

static int n_children;		/* # child processes still running */
static int got_sigchld;		/* set if we have received a SIGCHLD */

static int locked;		/* lock() has succeeded */
static int privopen;		/* don't lock, open device as root */

char *no_ppp_msg = "Sorry - this system lacks PPP kernel support\n";

GIDSET_TYPE groups[NGROUPS_MAX];/* groups the user is in */
int ngroups;			/* How many groups valid in groups */

static struct timeval start_time;	/* Time when link was started. */

struct pppd_stats link_stats;
int link_connect_time;
int link_stats_valid;

static int charshunt_pid;	/* Process ID for charshunt */

/*
 * We maintain a list of child process pids and
 * functions to call when they exit.
 */
struct subprocess {
    pid_t	pid;
    char	*prog;
    void	(*done) __P((void *));
    void	*arg;
    struct subprocess *next;
};

static struct subprocess *children;

/* Prototypes for procedures local to this file. */

static void create_pidfile __P((void));
static void create_linkpidfile __P((void));
static void cleanup __P((void));
static void close_tty __P((void));
static void get_input __P((void));
static const char *protocol_name __P((int));
static void calltimeout __P((void));
static struct timeval *timeleft __P((struct timeval *));
static void kill_my_pg __P((int));
static void hup __P((int));
static void term __P((int));
static void chld __P((int));
static void toggle_debug __P((int));
static void open_ccp __P((int));
static void bad_signal __P((int));
static void holdoff_end __P((void *));
static int device_script __P((char *, int, int, int));
static int reap_kids __P((int waitfor));
static void record_child __P((int, char *, void (*) (void *), void *));
static int start_charshunt __P((int, int));
static void charshunt_done __P((void *));
static void charshunt __P((int, int, char *));
static int record_write __P((FILE *, int code, u_char *buf, int nb,
			     struct timeval *));

extern	char	*ttyname __P((int));
extern	char	*getlogin __P((void));
int main __P((int, char *[]));

#ifdef ultrix
#undef	O_NONBLOCK
#define	O_NONBLOCK	O_NDELAY
#endif

#ifdef ULTRIX
#define setlogmask(x)
#endif

/*
 * PPP Data Link Layer "protocol" table.
 * One entry per supported protocol.
 * The last entry must be NULL.
 */
struct protent *protocols[] = {
    &lcp_protent,
    &pap_protent,
    &chap_protent,
#ifdef CBCP_SUPPORT
    &cbcp_protent,
#endif
    &ipcp_protent,
#ifdef INET6
    &ipv6cp_protent,
#endif
    &ccp_protent,
#ifdef IPX_CHANGE
    &ipxcp_protent,
#endif
#ifdef AT_CHANGE
    &atcp_protent,
#endif
    NULL
};

int
main(argc, argv)
    int argc;
    char *argv[];
{
    int i, fdflags, t;
    struct sigaction sa;
    char *p, *connector;
    struct passwd *pw;
    struct timeval timo;
    sigset_t mask;
    struct protent *protp;
    struct stat statbuf;
    char numbuf[16];

    new_phase(PHASE_INITIALIZE);

    /*
     * Ensure that fds 0, 1, 2 are open, to /dev/null if nowhere else.
     * This way we can close 0, 1, 2 in detach() without clobbering
     * a fd that we are using.
     */
    if ((i = open("/dev/null", O_RDWR)) >= 0) {
	while (0 <= i && i <= 2)
	    i = dup(i);
	if (i >= 0)
	    close(i);
    }

    script_env = NULL;

    /* Initialize syslog facilities */
    reopen_log();

    if (gethostname(hostname, MAXNAMELEN) < 0 ) {
	option_error("Couldn't get hostname: %m");
	exit(1);
    }
    hostname[MAXNAMELEN-1] = 0;

    /* make sure we don't create world or group writable files. */
    umask(umask(0777) | 022);

    uid = getuid();
    privileged = uid == 0;
    slprintf(numbuf, sizeof(numbuf), "%d", uid);
    script_setenv("ORIG_UID", numbuf);

    ngroups = getgroups(NGROUPS_MAX, groups);

    /*
     * Initialize magic number generator now so that protocols may
     * use magic numbers in initialization.
     */
    magic_init();

    /*
     * Initialize to the standard option set, then parse, in order,
     * the system options file, the user's options file,
     * the tty's options file, and the command line arguments.
     */
    for (i = 0; (protp = protocols[i]) != NULL; ++i)
        (*protp->init)(0);

    progname = *argv;

    prepass = 0;
    if (!options_from_file(_PATH_SYSOPTIONS, !privileged, 0, 1)
	|| !options_from_user())
	exit(EXIT_OPTION_ERROR);

    /* scan command line and options files to find device name */
    prepass = 1;
    parse_args(argc-1, argv+1);
    prepass = 0;

    /*
     * Work out the device name, if it hasn't already been specified.
     */
    using_pty = notty || ptycommand != NULL;
    if (!using_pty && default_device) {
	char *p;
	if (!isatty(0) || (p = ttyname(0)) == NULL) {
	    option_error("no device specified and stdin is not a tty");
	    exit(EXIT_OPTION_ERROR);
	}
	strlcpy(devnam, p, sizeof(devnam));
	if (stat(devnam, &devstat) < 0)
	    fatal("Couldn't stat default device %s: %m", devnam);
    }

    /*
     * Parse the tty options file and the command line.
     * The per-tty options file should not change
     * ptycommand, notty or devnam.
     */
    devnam_fixed = 1;
    if (!using_pty) {
	if (!options_for_tty())
	    exit(EXIT_OPTION_ERROR);
    }

    devnam_fixed = 0;
    if (!parse_args(argc-1, argv+1))
	exit(EXIT_OPTION_ERROR);

    /*
     * Check that we are running as root.
     */
    if (geteuid() != 0) {
	option_error("must be root to run %s, since it is not setuid-root",
		     argv[0]);
	exit(EXIT_NOT_ROOT);
    }

    if (!ppp_available()) {
	option_error(no_ppp_msg);
	exit(EXIT_NO_KERNEL_SUPPORT);
    }

    /*
     * Check that the options given are valid and consistent.
     */
    if (!sys_check_options())
	exit(EXIT_OPTION_ERROR);
    auth_check_options();
    for (i = 0; (protp = protocols[i]) != NULL; ++i)
	if (protp->check_options != NULL)
	    (*protp->check_options)();
    if (demand && connect_script == 0) {
	option_error("connect script is required for demand-dialling\n");
	exit(EXIT_OPTION_ERROR);
    }
    /* default holdoff to 0 if no connect script has been given */
    if (connect_script == 0 && !holdoff_specified)
	holdoff = 0;

    if (using_pty) {
	if (!default_device) {
	    option_error("%s option precludes specifying device name",
			 notty? "notty": "pty");
	    exit(EXIT_OPTION_ERROR);
	}
	if (ptycommand != NULL && notty) {
	    option_error("pty option is incompatible with notty option");
	    exit(EXIT_OPTION_ERROR);
	}
	default_device = notty;
	lockflag = 0;
	modem = 0;
	if (notty && log_to_fd <= 1)
	    log_to_fd = -1;
    } else {
	/*
	 * If the user has specified a device which is the same as
	 * the one on stdin, pretend they didn't specify any.
	 * If the device is already open read/write on stdin,
	 * we assume we don't need to lock it, and we can open it as root.
	 */
	if (fstat(0, &statbuf) >= 0 && S_ISCHR(statbuf.st_mode)
	    && statbuf.st_rdev == devstat.st_rdev) {
	    default_device = 1;
	    fdflags = fcntl(0, F_GETFL);
	    if (fdflags != -1 && (fdflags & O_ACCMODE) == O_RDWR)
		privopen = 1;
	}
    }
    if (default_device)
	nodetach = 1;

    /*
     * Don't send log messages to the serial port, it tends to
     * confuse the peer. :-)
     */
    if (log_to_fd >= 0 && fstat(log_to_fd, &statbuf) >= 0
	&& S_ISCHR(statbuf.st_mode) && statbuf.st_rdev == devstat.st_rdev)
	log_to_fd = -1;

    script_setenv("DEVICE", devnam);

    /*
     * Initialize system-dependent stuff.
     */
    sys_init();
    if (debug)
	setlogmask(LOG_UPTO(LOG_DEBUG));

    /*
     * Detach ourselves from the terminal, if required,
     * and identify who is running us.
     */
    if (!nodetach && !updetach)
	detach();
    p = getlogin();
    if (p == NULL) {
	pw = getpwuid(uid);
	if (pw != NULL && pw->pw_name != NULL)
	    p = pw->pw_name;
	else
	    p = "(unknown)";
    }
    syslog(LOG_NOTICE, "pppd %s.%d%s started by %s, uid %d",
	   VERSION, PATCHLEVEL, IMPLEMENTATION, p, uid);
    script_setenv("PPPLOGNAME", p);

    /*
     * Compute mask of all interesting signals and install signal handlers
     * for each.  Only one signal handler may be active at a time.  Therefore,
     * all other signals should be masked when any handler is executing.
     */
    sigemptyset(&mask);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGUSR2);

#define SIGNAL(s, handler)	do { \
	sa.sa_handler = handler; \
	if (sigaction(s, &sa, NULL) < 0) \
	    fatal("Couldn't establish signal handler (%d): %m", s); \
    } while (0)

    sa.sa_mask = mask;
    sa.sa_flags = 0;
    SIGNAL(SIGHUP, hup);		/* Hangup */
    SIGNAL(SIGINT, term);		/* Interrupt */
    SIGNAL(SIGTERM, term);		/* Terminate */
    SIGNAL(SIGCHLD, chld);

    SIGNAL(SIGUSR1, toggle_debug);	/* Toggle debug flag */
    SIGNAL(SIGUSR2, open_ccp);		/* Reopen CCP */

    /*
     * Install a handler for other signals which would otherwise
     * cause pppd to exit without cleaning up.
     */
    SIGNAL(SIGABRT, bad_signal);
    SIGNAL(SIGALRM, bad_signal);
    SIGNAL(SIGFPE, bad_signal);
    SIGNAL(SIGILL, bad_signal);
    SIGNAL(SIGPIPE, bad_signal);
    SIGNAL(SIGQUIT, bad_signal);
    SIGNAL(SIGSEGV, bad_signal);
#ifdef SIGBUS
    SIGNAL(SIGBUS, bad_signal);
#endif
#ifdef SIGEMT
    SIGNAL(SIGEMT, bad_signal);
#endif
#ifdef SIGPOLL
    SIGNAL(SIGPOLL, bad_signal);
#endif
#ifdef SIGPROF
    SIGNAL(SIGPROF, bad_signal);
#endif
#ifdef SIGSYS
    SIGNAL(SIGSYS, bad_signal);
#endif
#ifdef SIGTRAP
    SIGNAL(SIGTRAP, bad_signal);
#endif
#ifdef SIGVTALRM
    SIGNAL(SIGVTALRM, bad_signal);
#endif
#ifdef SIGXCPU
    SIGNAL(SIGXCPU, bad_signal);
#endif
#ifdef SIGXFSZ
    SIGNAL(SIGXFSZ, bad_signal);
#endif

    /*
     * Apparently we can get a SIGPIPE when we call syslog, if
     * syslogd has died and been restarted.  Ignoring it seems
     * be sufficient.
     */
    signal(SIGPIPE, SIG_IGN);

    waiting = 0;

    create_linkpidfile();

    /*
     * If we're doing dial-on-demand, set up the interface now.
     */
    if (demand) {
	/*
	 * Open the loopback channel and set it up to be the ppp interface.
	 */
	fd_loop = open_ppp_loopback();

	syslog(LOG_INFO, "Using interface ppp%d", ifunit);
	slprintf(ifname, sizeof(ifname), "ppp%d", ifunit);
	script_setenv("IFNAME", ifname);

	create_pidfile();	/* write pid to file */

	/*
	 * Configure the interface and mark it up, etc.
	 */
	demand_conf();
    }

    do_callback = 0;
    for (;;) {

	need_holdoff = 1;
	ttyfd = -1;
	real_ttyfd = -1;
	status = EXIT_OK;
	++unsuccess;
	doing_callback = do_callback;
	do_callback = 0;

	if (demand && !doing_callback) {
	    /*
	     * Don't do anything until we see some activity.
	     */
	    kill_link = 0;
	    new_phase(PHASE_DORMANT);
	    demand_unblock();
	    add_fd(fd_loop);
	    for (;;) {
		if (sigsetjmp(sigjmp, 1) == 0) {
		    sigprocmask(SIG_BLOCK, &mask, NULL);
		    if (kill_link || got_sigchld) {
			sigprocmask(SIG_UNBLOCK, &mask, NULL);
		    } else {
			waiting = 1;
			sigprocmask(SIG_UNBLOCK, &mask, NULL);
			wait_input(timeleft(&timo));
		    }
		}
		waiting = 0;
		calltimeout();
		if (kill_link) {
		    if (!persist)
			break;
		    kill_link = 0;
		}
		if (get_loop_output())
		    break;
		if (got_sigchld)
		    reap_kids(0);
	    }
	    remove_fd(fd_loop);
	    if (kill_link && !persist)
		break;

	    /*
	     * Now we want to bring up the link.
	     */
	    demand_block();
	    info("Starting link");
	}

	new_phase(PHASE_SERIALCONN);

	/*
	 * Get a pty master/slave pair if the pty, notty, or record
	 * options were specified.
	 */
	strlcpy(ppp_devnam, devnam, sizeof(ppp_devnam));
	pty_master = -1;
	pty_slave = -1;
	if (ptycommand != NULL || notty || record_file != NULL) {
	    if (!get_pty(&pty_master, &pty_slave, ppp_devnam, uid)) {
		error("Couldn't allocate pseudo-tty");
		status = EXIT_FATAL_ERROR;
		goto fail;
	    }
	    set_up_tty(pty_slave, 1);
	}

	/*
	 * Lock the device if we've been asked to.
	 */
	status = EXIT_LOCK_FAILED;
	if (lockflag && !privopen) {
	    if (lock(devnam) < 0)
		goto fail;
	    locked = 1;
	}

	/*
	 * Open the serial device and set it up to be the ppp interface.
	 * First we open it in non-blocking mode so we can set the
	 * various termios flags appropriately.  If we aren't dialling
	 * out and we want to use the modem lines, we reopen it later
	 * in order to wait for the carrier detect signal from the modem.
	 */
	hungup = 0;
	kill_link = 0;
	connector = doing_callback? callback_script: connect_script;
	if (devnam[0] != 0) {
	    for (;;) {
		/* If the user specified the device name, become the
		   user before opening it. */
		int err;
		if (!devnam_info.priv && !privopen)
		    seteuid(uid);
		ttyfd = open(devnam, O_NONBLOCK | O_RDWR, 0);
		err = errno;
		if (!devnam_info.priv && !privopen)
		    seteuid(0);
		if (ttyfd >= 0)
		    break;
		errno = err;
		if (err != EINTR) {
		    error("Failed to open %s: %m", devnam);
		    status = EXIT_OPEN_FAILED;
		}
		if (!persist || err != EINTR)
		    goto fail;
	    }
	    if ((fdflags = fcntl(ttyfd, F_GETFL)) == -1
		|| fcntl(ttyfd, F_SETFL, fdflags & ~O_NONBLOCK) < 0)
		warn("Couldn't reset non-blocking mode on device: %m");

	    /*
	     * Do the equivalent of `mesg n' to stop broadcast messages.
	     */
	    if (fstat(ttyfd, &statbuf) < 0
		|| fchmod(ttyfd, statbuf.st_mode & ~(S_IWGRP | S_IWOTH)) < 0) {
		warn("Couldn't restrict write permissions to %s: %m", devnam);
	    } else
		tty_mode = statbuf.st_mode;

	    /*
	     * Set line speed, flow control, etc.
	     * If we have a non-null connection or initializer script,
	     * on most systems we set CLOCAL for now so that we can talk
	     * to the modem before carrier comes up.  But this has the
	     * side effect that we might miss it if CD drops before we
	     * get to clear CLOCAL below.  On systems where we can talk
	     * successfully to the modem with CLOCAL clear and CD down,
	     * we could clear CLOCAL at this point.
	     */
	    set_up_tty(ttyfd, ((connector != NULL && connector[0] != 0)
			       || initializer != NULL));
	    real_ttyfd = ttyfd;
	}

	/*
	 * If the notty and/or record option was specified,
	 * start up the character shunt now.
	 */
	status = EXIT_PTYCMD_FAILED;
	if (ptycommand != NULL) {
	    if (record_file != NULL) {
		int ipipe[2], opipe[2], ok;

		if (pipe(ipipe) < 0 || pipe(opipe) < 0)
		    fatal("Couldn't create pipes for record option: %m");
		ok = device_script(ptycommand, opipe[0], ipipe[1], 1) == 0
		    && start_charshunt(ipipe[0], opipe[1]);
		close(ipipe[0]);
		close(ipipe[1]);
		close(opipe[0]);
		close(opipe[1]);
		if (!ok)
		    goto fail;
	    } else {
		if (device_script(ptycommand, pty_master, pty_master, 1) < 0)
		    goto fail;
		ttyfd = pty_slave;
		close(pty_master);
		pty_master = -1;
	    }
	} else if (notty) {
	    if (!start_charshunt(0, 1))
		goto fail;
	} else if (record_file != NULL) {
	    if (!start_charshunt(ttyfd, ttyfd))
		goto fail;
	}

	/* run connection script */
	if ((connector && connector[0]) || initializer) {
	    if (real_ttyfd != -1) {
		/* XXX do this if doing_callback == CALLBACK_DIALIN? */
		if (!default_device && modem) {
		    setdtr(real_ttyfd, 0);	/* in case modem is off hook */
		    sleep(1);
		    setdtr(real_ttyfd, 1);
		}
	    }

	    if (initializer && initializer[0]) {
		if (device_script(initializer, ttyfd, ttyfd, 0) < 0) {
		    error("Initializer script failed");
		    status = EXIT_INIT_FAILED;
		    goto fail;
		}
		if (kill_link)
		    goto disconnect;

		info("Serial port initialized.");
	    }

	    if (connector && connector[0]) {
		if (device_script(connector, ttyfd, ttyfd, 0) < 0) {
		    error("Connect script failed");
		    status = EXIT_CONNECT_FAILED;
		    goto fail;
		}
		if (kill_link)
		    goto disconnect;

		info("Serial connection established.");
	    }

	    /* set line speed, flow control, etc.;
	       clear CLOCAL if modem option */
	    if (real_ttyfd != -1)
		set_up_tty(real_ttyfd, 0);

	    if (doing_callback == CALLBACK_DIALIN)
		connector = NULL;
	}

	/* reopen tty if necessary to wait for carrier */
	if (connector == NULL && modem && devnam[0] != 0) {
	    for (;;) {
		if ((i = open(devnam, O_RDWR)) >= 0)
		    break;
		if (errno != EINTR) {
		    error("Failed to reopen %s: %m", devnam);
		    status = EXIT_OPEN_FAILED;
		}
		if (!persist || errno != EINTR || hungup || kill_link)
		    goto fail;
	    }
	    close(i);
	}

	slprintf(numbuf, sizeof(numbuf), "%d", baud_rate);
	script_setenv("SPEED", numbuf);

	/* run welcome script, if any */
	if (welcomer && welcomer[0]) {
	    if (device_script(welcomer, ttyfd, ttyfd, 0) < 0)
		warn("Welcome script failed");
	}

	/* set up the serial device as a ppp interface */
	fd_ppp = establish_ppp(ttyfd);
	if (fd_ppp < 0) {
	    status = EXIT_FATAL_ERROR;
	    goto disconnect;
	}

	if (!demand) {
	    
	    info("Using interface ppp%d", ifunit);
	    slprintf(ifname, sizeof(ifname), "ppp%d", ifunit);
	    script_setenv("IFNAME", ifname);

	    create_pidfile();	/* write pid to file */
	}

	/*
	 * Start opening the connection and wait for
	 * incoming events (reply, timeout, etc.).
	 */
	notice("Connect: %s <--> %s", ifname, ppp_devnam);
	gettimeofday(&start_time, NULL);
	link_stats_valid = 0;
	script_unsetenv("CONNECT_TIME");
	script_unsetenv("BYTES_SENT");
	script_unsetenv("BYTES_RCVD");
	lcp_lowerup(0);

	/*
	 * If we are initiating this connection, wait for a short
	 * time for something from the peer.  This can avoid bouncing
	 * our packets off his tty before he has it set up.
	 */
	add_fd(fd_ppp);
	if (connect_delay != 0 && (connector != NULL || ptycommand != NULL)) {
	    struct timeval t;
	    t.tv_sec = connect_delay / 1000;
	    t.tv_usec = connect_delay % 1000;
	    wait_input(&t);
	}

	lcp_open(0);		/* Start protocol */
	open_ccp_flag = 0;
	status = EXIT_NEGOTIATION_FAILED;
	new_phase(PHASE_ESTABLISH);
	while (phase != PHASE_DEAD) {
	    if (sigsetjmp(sigjmp, 1) == 0) {
		sigprocmask(SIG_BLOCK, &mask, NULL);
		if (kill_link || open_ccp_flag || got_sigchld) {
		    sigprocmask(SIG_UNBLOCK, &mask, NULL);
		} else {
		    waiting = 1;
		    sigprocmask(SIG_UNBLOCK, &mask, NULL);
		    wait_input(timeleft(&timo));
		}
	    }
	    waiting = 0;
	    calltimeout();
	    get_input();
	    if (kill_link) {
		lcp_close(0, "User request");
		kill_link = 0;
	    }
	    if (open_ccp_flag) {
		if (phase == PHASE_NETWORK || phase == PHASE_RUNNING) {
		    ccp_fsm[0].flags = OPT_RESTART; /* clears OPT_SILENT */
		    (*ccp_protent.open)(0);
		}
		open_ccp_flag = 0;
	    }
	    if (got_sigchld)
		reap_kids(0);	/* Don't leave dead kids lying around */
	}

	/*
	 * Print connect time and statistics.
	 */
	if (link_stats_valid) {
	    int t = (link_connect_time + 5) / 6;    /* 1/10ths of minutes */
	    info("Connect time %d.%d minutes.", t/10, t%10);
	    info("Sent %d bytes, received %d bytes.",
		 link_stats.bytes_out, link_stats.bytes_in);
	}

	/*
	 * Delete pid file before disestablishing ppp.  Otherwise it
	 * can happen that another pppd gets the same unit and then
	 * we delete its pid file.
	 */
	if (!demand) {
	    if (pidfilename[0] != 0
		&& unlink(pidfilename) < 0 && errno != ENOENT) 
		warn("unable to delete pid file %s: %m", pidfilename);
	    pidfilename[0] = 0;
	}

	/*
	 * If we may want to bring the link up again, transfer
	 * the ppp unit back to the loopback.  Set the
	 * real serial device back to its normal mode of operation.
	 */
	remove_fd(fd_ppp);
	clean_check();
	if (demand)
	    restore_loop();
	disestablish_ppp(ttyfd);
	fd_ppp = -1;
	if (!hungup)
	    lcp_lowerdown(0);

	/*
	 * Run disconnector script, if requested.
	 * XXX we may not be able to do this if the line has hung up!
	 */
    disconnect:
	if (disconnect_script && !hungup) {
	    new_phase(PHASE_DISCONNECT);
	    if (real_ttyfd >= 0)
		set_up_tty(real_ttyfd, 1);
	    if (device_script(disconnect_script, ttyfd, ttyfd, 0) < 0) {
		warn("disconnect script failed");
	    } else {
		info("Serial link disconnected.");
	    }
	}

    fail:
	if (pty_master >= 0)
	    close(pty_master);
	if (pty_slave >= 0)
	    close(pty_slave);
	if (real_ttyfd >= 0)
	    close_tty();
	if (locked) {
	    unlock();
	    locked = 0;
	}

	if (!demand) {
	    if (pidfilename[0] != 0
		&& unlink(pidfilename) < 0 && errno != ENOENT) 
		warn("unable to delete pid file %s: %m", pidfilename);
	    pidfilename[0] = 0;
	}

	if (!persist || (maxfail > 0 && unsuccess >= maxfail))
	    break;

	kill_link = 0;
	if (demand)
	    demand_discard();
	t = need_holdoff? holdoff: 0;
	if (holdoff_hook)
	    t = (*holdoff_hook)();
	if (t > 0) {
	    new_phase(PHASE_HOLDOFF);
	    TIMEOUT(holdoff_end, NULL, t);
	    do {
		if (sigsetjmp(sigjmp, 1) == 0) {
		    sigprocmask(SIG_BLOCK, &mask, NULL);
		    if (kill_link || got_sigchld) {
			sigprocmask(SIG_UNBLOCK, &mask, NULL);
		    } else {
			waiting = 1;
			sigprocmask(SIG_UNBLOCK, &mask, NULL);
			wait_input(timeleft(&timo));
		    }
		}
		waiting = 0;
		calltimeout();
		if (kill_link) {
		    kill_link = 0;
		    new_phase(PHASE_DORMANT); /* allow signal to end holdoff */
		}
		if (got_sigchld)
		    reap_kids(0);
	    } while (phase == PHASE_HOLDOFF);
	    if (!persist)
		break;
	}
    }

    /* Wait for scripts to finish */
    /* XXX should have a timeout here */
    while (n_children > 0) {
	if (debug) {
	    struct subprocess *chp;
	    dbglog("Waiting for %d child processes...", n_children);
	    for (chp = children; chp != NULL; chp = chp->next)
		dbglog("  script %s, pid %d", chp->prog, chp->pid);
	}
	if (reap_kids(1) < 0)
	    break;
    }

    die(status);
    return 0;
}

/*
 * detach - detach us from the controlling terminal.
 */
void
detach()
{
    int pid;

    if (detached)
	return;
    if ((pid = fork()) < 0) {
	error("Couldn't detach (fork failed: %m)");
	die(1);			/* or just return? */
    }
    if (pid != 0) {
	/* parent */
	if (locked)
	    relock(pid);
	exit(0);		/* parent dies */
    }
    setsid();
    chdir("/");
    close(0);
    close(1);
    close(2);
    detached = 1;
    log_to_fd = -1;
    /* update pid files if they have been written already */
    if (pidfilename[0])
	create_pidfile();
    if (linkpidfile[0])
	create_linkpidfile();
}

/*
 * reopen_log - (re)open our connection to syslog.
 */
void
reopen_log()
{
#ifdef ULTRIX
    openlog("pppd", LOG_PID);
#else
    openlog("pppd", LOG_PID | LOG_NDELAY, LOG_PPP);
    setlogmask(LOG_UPTO(LOG_INFO));
#endif
}

/*
 * Create a file containing our process ID.
 */
static void
create_pidfile()
{
    FILE *pidfile;
    char numbuf[16];

    slprintf(pidfilename, sizeof(pidfilename), "%s%s.pid",
	     _PATH_VARRUN, ifname);
    if ((pidfile = fopen(pidfilename, "w")) != NULL) {
	fprintf(pidfile, "%d\n", getpid());
	(void) fclose(pidfile);
    } else {
	error("Failed to create pid file %s: %m", pidfilename);
	pidfilename[0] = 0;
    }
    slprintf(numbuf, sizeof(numbuf), "%d", getpid());
    script_setenv("PPPD_PID", numbuf);
    if (linkpidfile[0])
	create_linkpidfile();
}

static void
create_linkpidfile()
{
    FILE *pidfile;

    if (linkname[0] == 0)
	return;
    slprintf(linkpidfile, sizeof(linkpidfile), "%sppp-%s.pid",
	     _PATH_VARRUN, linkname);
    if ((pidfile = fopen(linkpidfile, "w")) != NULL) {
	fprintf(pidfile, "%d\n", getpid());
	if (pidfilename[0])
	    fprintf(pidfile, "%s\n", ifname);
	(void) fclose(pidfile);
    } else {
	error("Failed to create pid file %s: %m", linkpidfile);
	linkpidfile[0] = 0;
    }
    script_setenv("LINKNAME", linkname);
}

/*
 * holdoff_end - called via a timeout when the holdoff period ends.
 */
static void
holdoff_end(arg)
    void *arg;
{
    new_phase(PHASE_DORMANT);
}

/* List of protocol names, to make our messages a little more informative. */
struct protocol_list {
    u_short	proto;
    const char	*name;
} protocol_list[] = {
    { 0x21,	"IP" },
    { 0x23,	"OSI Network Layer" },
    { 0x25,	"Xerox NS IDP" },
    { 0x27,	"DECnet Phase IV" },
    { 0x29,	"Appletalk" },
    { 0x2b,	"Novell IPX" },
    { 0x2d,	"VJ compressed TCP/IP" },
    { 0x2f,	"VJ uncompressed TCP/IP" },
    { 0x31,	"Bridging PDU" },
    { 0x33,	"Stream Protocol ST-II" },
    { 0x35,	"Banyan Vines" },
    { 0x39,	"AppleTalk EDDP" },
    { 0x3b,	"AppleTalk SmartBuffered" },
    { 0x3d,	"Multi-Link" },
    { 0x3f,	"NETBIOS Framing" },
    { 0x41,	"Cisco Systems" },
    { 0x43,	"Ascom Timeplex" },
    { 0x45,	"Fujitsu Link Backup and Load Balancing (LBLB)" },
    { 0x47,	"DCA Remote Lan" },
    { 0x49,	"Serial Data Transport Protocol (PPP-SDTP)" },
    { 0x4b,	"SNA over 802.2" },
    { 0x4d,	"SNA" },
    { 0x4f,	"IP6 Header Compression" },
    { 0x6f,	"Stampede Bridging" },
    { 0xfb,	"single-link compression" },
    { 0xfd,	"1st choice compression" },
    { 0x0201,	"802.1d Hello Packets" },
    { 0x0203,	"IBM Source Routing BPDU" },
    { 0x0205,	"DEC LANBridge100 Spanning Tree" },
    { 0x0231,	"Luxcom" },
    { 0x0233,	"Sigma Network Systems" },
    { 0x8021,	"Internet Protocol Control Protocol" },
    { 0x8023,	"OSI Network Layer Control Protocol" },
    { 0x8025,	"Xerox NS IDP Control Protocol" },
    { 0x8027,	"DECnet Phase IV Control Protocol" },
    { 0x8029,	"Appletalk Control Protocol" },
    { 0x802b,	"Novell IPX Control Protocol" },
    { 0x8031,	"Bridging NCP" },
    { 0x8033,	"Stream Protocol Control Protocol" },
    { 0x8035,	"Banyan Vines Control Protocol" },
    { 0x803d,	"Multi-Link Control Protocol" },
    { 0x803f,	"NETBIOS Framing Control Protocol" },
    { 0x8041,	"Cisco Systems Control Protocol" },
    { 0x8043,	"Ascom Timeplex" },
    { 0x8045,	"Fujitsu LBLB Control Protocol" },
    { 0x8047,	"DCA Remote Lan Network Control Protocol (RLNCP)" },
    { 0x8049,	"Serial Data Control Protocol (PPP-SDCP)" },
    { 0x804b,	"SNA over 802.2 Control Protocol" },
    { 0x804d,	"SNA Control Protocol" },
    { 0x804f,	"IP6 Header Compression Control Protocol" },
    { 0x006f,	"Stampede Bridging Control Protocol" },
    { 0x80fb,	"Single Link Compression Control Protocol" },
    { 0x80fd,	"Compression Control Protocol" },
    { 0xc021,	"Link Control Protocol" },
    { 0xc023,	"Password Authentication Protocol" },
    { 0xc025,	"Link Quality Report" },
    { 0xc027,	"Shiva Password Authentication Protocol" },
    { 0xc029,	"CallBack Control Protocol (CBCP)" },
    { 0xc081,	"Container Control Protocol" },
    { 0xc223,	"Challenge Handshake Authentication Protocol" },
    { 0xc281,	"Proprietary Authentication Protocol" },
    { 0,	NULL },
};

/*
 * protocol_name - find a name for a PPP protocol.
 */
static const char *
protocol_name(proto)
    int proto;
{
    struct protocol_list *lp;

    for (lp = protocol_list; lp->proto != 0; ++lp)
	if (proto == lp->proto)
	    return lp->name;
    return NULL;
}

/*
 * get_input - called when incoming data is available.
 */
static void
get_input()
{
    int len, i;
    u_char *p;
    u_short protocol;
    struct protent *protp;

    p = inpacket_buf;	/* point to beginning of packet buffer */

    len = read_packet(inpacket_buf);
    if (len < 0)
	return;

    if (len == 0) {
	notice("Modem hangup");
	hungup = 1;
	status = EXIT_HANGUP;
	lcp_lowerdown(0);	/* serial link is no longer available */
	link_terminated(0);
	return;
    }

    if (debug /*&& (debugflags & DBG_INPACKET)*/)
	dbglog("rcvd %P", p, len);

    if (len < PPP_HDRLEN) {
	MAINDEBUG(("io(): Received short packet."));
	return;
    }

    p += 2;				/* Skip address and control */
    GETSHORT(protocol, p);
    len -= PPP_HDRLEN;

    /*
     * Toss all non-LCP packets unless LCP is OPEN.
     */
    if (protocol != PPP_LCP && lcp_fsm[0].state != OPENED) {
	MAINDEBUG(("get_input: Received non-LCP packet when LCP not open."));
	return;
    }

    /*
     * Until we get past the authentication phase, toss all packets
     * except LCP, LQR and authentication packets.
     */
    if (phase <= PHASE_AUTHENTICATE
	&& !(protocol == PPP_LCP || protocol == PPP_LQR
	     || protocol == PPP_PAP || protocol == PPP_CHAP)) {
	MAINDEBUG(("get_input: discarding proto 0x%x in phase %d",
		   protocol, phase));
	return;
    }

    /*
     * Upcall the proper protocol input routine.
     */
    for (i = 0; (protp = protocols[i]) != NULL; ++i) {
	if (protp->protocol == protocol && protp->enabled_flag) {
	    (*protp->input)(0, p, len);
	    return;
	}
        if (protocol == (protp->protocol & ~0x8000) && protp->enabled_flag
	    && protp->datainput != NULL) {
	    (*protp->datainput)(0, p, len);
	    return;
	}
    }

    if (debug) {
	const char *pname = protocol_name(protocol);
	if (pname != NULL)
	    warn("Unsupported protocol '%s' (0x%x) received", pname, protocol);
	else
	    warn("Unsupported protocol 0x%x received", protocol);
    }
    lcp_sprotrej(0, p - PPP_HDRLEN, len + PPP_HDRLEN);
}

/*
 * new_phase - signal the start of a new phase of pppd's operation.
 */
void
new_phase(p)
    int p;
{
    phase = p;
    if (new_phase_hook)
	(*new_phase_hook)(p);
}

/*
 * die - clean up state and exit with the specified status.
 */
void
die(status)
    int status;
{
    cleanup();
    syslog(LOG_INFO, "Exit.");
    exit(status);
}

/*
 * cleanup - restore anything which needs to be restored before we exit
 */
/* ARGSUSED */
static void
cleanup()
{
    sys_cleanup();

    if (fd_ppp >= 0)
	disestablish_ppp(ttyfd);
    if (real_ttyfd >= 0)
	close_tty();

    if (pidfilename[0] != 0 && unlink(pidfilename) < 0 && errno != ENOENT) 
	warn("unable to delete pid file %s: %m", pidfilename);
    pidfilename[0] = 0;
    if (linkpidfile[0] != 0 && unlink(linkpidfile) < 0 && errno != ENOENT) 
	warn("unable to delete pid file %s: %m", linkpidfile);
    linkpidfile[0] = 0;

    if (locked)
	unlock();
}

/*
 * close_tty - restore the terminal device and close it.
 */
static void
close_tty()
{
    /* drop dtr to hang up */
    if (!default_device && modem) {
	setdtr(real_ttyfd, 0);
	/*
	 * This sleep is in case the serial port has CLOCAL set by default,
	 * and consequently will reassert DTR when we close the device.
	 */
	sleep(1);
    }

    restore_tty(real_ttyfd);

    if (tty_mode != (mode_t) -1) {
	if (fchmod(real_ttyfd, tty_mode) != 0) {
	    /* XXX if devnam is a symlink, this will change the link */
	    chmod(devnam, tty_mode);
	}
    }

    close(real_ttyfd);
    real_ttyfd = -1;
}

/*
 * update_link_stats - get stats at link termination.
 */
void
update_link_stats(u)
    int u;
{
    struct timeval now;
    char numbuf[32];

    if (!get_ppp_stats(u, &link_stats)
	|| gettimeofday(&now, NULL) < 0)
	return;
    link_connect_time = now.tv_sec - start_time.tv_sec;
    link_stats_valid = 1;

    slprintf(numbuf, sizeof(numbuf), "%d", link_connect_time);
    script_setenv("CONNECT_TIME", numbuf);
    slprintf(numbuf, sizeof(numbuf), "%d", link_stats.bytes_out);
    script_setenv("BYTES_SENT", numbuf);
    slprintf(numbuf, sizeof(numbuf), "%d", link_stats.bytes_in);
    script_setenv("BYTES_RCVD", numbuf);
}


struct	callout {
    struct timeval	c_time;		/* time at which to call routine */
    void		*c_arg;		/* argument to routine */
    void		(*c_func) __P((void *)); /* routine */
    struct		callout *c_next;
};

static struct callout *callout = NULL;	/* Callout list */
static struct timeval timenow;		/* Current time */

/*
 * timeout - Schedule a timeout.
 *
 * Note that this timeout takes the number of seconds, NOT hz (as in
 * the kernel).
 */
void
timeout(func, arg, time)
    void (*func) __P((void *));
    void *arg;
    int time;
{
    struct callout *newp, *p, **pp;
  
    MAINDEBUG(("Timeout %p:%p in %d seconds.", func, arg, time));
  
    /*
     * Allocate timeout.
     */
    if ((newp = (struct callout *) malloc(sizeof(struct callout))) == NULL)
	fatal("Out of memory in timeout()!");
    newp->c_arg = arg;
    newp->c_func = func;
    gettimeofday(&timenow, NULL);
    newp->c_time.tv_sec = timenow.tv_sec + time;
    newp->c_time.tv_usec = timenow.tv_usec;
  
    /*
     * Find correct place and link it in.
     */
    for (pp = &callout; (p = *pp); pp = &p->c_next)
	if (newp->c_time.tv_sec < p->c_time.tv_sec
	    || (newp->c_time.tv_sec == p->c_time.tv_sec
		&& newp->c_time.tv_usec < p->c_time.tv_usec))
	    break;
    newp->c_next = p;
    *pp = newp;
}


/*
 * untimeout - Unschedule a timeout.
 */
void
untimeout(func, arg)
    void (*func) __P((void *));
    void *arg;
{
    struct callout **copp, *freep;
  
    MAINDEBUG(("Untimeout %p:%p.", func, arg));
  
    /*
     * Find first matching timeout and remove it from the list.
     */
    for (copp = &callout; (freep = *copp); copp = &freep->c_next)
	if (freep->c_func == func && freep->c_arg == arg) {
	    *copp = freep->c_next;
	    free((char *) freep);
	    break;
	}
}


/*
 * calltimeout - Call any timeout routines which are now due.
 */
static void
calltimeout()
{
    struct callout *p;

    while (callout != NULL) {
	p = callout;

	if (gettimeofday(&timenow, NULL) < 0)
	    fatal("Failed to get time of day: %m");
	if (!(p->c_time.tv_sec < timenow.tv_sec
	      || (p->c_time.tv_sec == timenow.tv_sec
		  && p->c_time.tv_usec <= timenow.tv_usec)))
	    break;		/* no, it's not time yet */

	callout = p->c_next;
	(*p->c_func)(p->c_arg);

	free((char *) p);
    }
}


/*
 * timeleft - return the length of time until the next timeout is due.
 */
static struct timeval *
timeleft(tvp)
    struct timeval *tvp;
{
    if (callout == NULL)
	return NULL;

    gettimeofday(&timenow, NULL);
    tvp->tv_sec = callout->c_time.tv_sec - timenow.tv_sec;
    tvp->tv_usec = callout->c_time.tv_usec - timenow.tv_usec;
    if (tvp->tv_usec < 0) {
	tvp->tv_usec += 1000000;
	tvp->tv_sec -= 1;
    }
    if (tvp->tv_sec < 0)
	tvp->tv_sec = tvp->tv_usec = 0;

    return tvp;
}


/*
 * kill_my_pg - send a signal to our process group, and ignore it ourselves.
 */
static void
kill_my_pg(sig)
    int sig;
{
    struct sigaction act, oldact;

    act.sa_handler = SIG_IGN;
    act.sa_flags = 0;
    kill(0, sig);
    sigaction(sig, &act, &oldact);
    sigaction(sig, &oldact, NULL);
}


/*
 * hup - Catch SIGHUP signal.
 *
 * Indicates that the physical layer has been disconnected.
 * We don't rely on this indication; if the user has sent this
 * signal, we just take the link down.
 */
static void
hup(sig)
    int sig;
{
    info("Hangup (SIGHUP)");
    kill_link = 1;
    if (status != EXIT_HANGUP)
	status = EXIT_USER_REQUEST;
    if (conn_running)
	/* Send the signal to the [dis]connector process(es) also */
	kill_my_pg(sig);
    if (charshunt_pid)
	kill(charshunt_pid, sig);
    if (waiting)
	siglongjmp(sigjmp, 1);
}


/*
 * term - Catch SIGTERM signal and SIGINT signal (^C/del).
 *
 * Indicates that we should initiate a graceful disconnect and exit.
 */
/*ARGSUSED*/
static void
term(sig)
    int sig;
{
    info("Terminating on signal %d.", sig);
    persist = 0;		/* don't try to restart */
    kill_link = 1;
    status = EXIT_USER_REQUEST;
    if (conn_running)
	/* Send the signal to the [dis]connector process(es) also */
	kill_my_pg(sig);
    if (charshunt_pid)
	kill(charshunt_pid, sig);
    if (waiting)
	siglongjmp(sigjmp, 1);
}


/*
 * chld - Catch SIGCHLD signal.
 * Sets a flag so we will call reap_kids in the mainline.
 */
static void
chld(sig)
    int sig;
{
    got_sigchld = 1;
    if (waiting)
	siglongjmp(sigjmp, 1);
}


/*
 * toggle_debug - Catch SIGUSR1 signal.
 *
 * Toggle debug flag.
 */
/*ARGSUSED*/
static void
toggle_debug(sig)
    int sig;
{
    debug = !debug;
    if (debug) {
	setlogmask(LOG_UPTO(LOG_DEBUG));
    } else {
	setlogmask(LOG_UPTO(LOG_WARNING));
    }
}


/*
 * open_ccp - Catch SIGUSR2 signal.
 *
 * Try to (re)negotiate compression.
 */
/*ARGSUSED*/
static void
open_ccp(sig)
    int sig;
{
    open_ccp_flag = 1;
    if (waiting)
	siglongjmp(sigjmp, 1);
}


/*
 * bad_signal - We've caught a fatal signal.  Clean up state and exit.
 */
static void
bad_signal(sig)
    int sig;
{
    static int crashed = 0;

    if (crashed)
	_exit(127);
    crashed = 1;
    error("Fatal signal %d", sig);
    if (conn_running)
	kill_my_pg(SIGTERM);
    if (charshunt_pid)
	kill(charshunt_pid, SIGTERM);
    die(127);
}


/*
 * device_script - run a program to talk to the serial device
 * (e.g. to run the connector or disconnector script).
 */
static int
device_script(program, in, out, dont_wait)
    char *program;
    int in, out;
    int dont_wait;
{
    int pid;
    int status = -1;
    int errfd;

    ++conn_running;
    pid = fork();

    if (pid < 0) {
	--conn_running;
	error("Failed to create child process: %m");
	return -1;
    }

    if (pid == 0) {
	sys_close();
	closelog();
	if (in == 2) {
	    /* aargh!!! */
	    int newin = dup(in);
	    if (in == out)
		out = newin;
	    in = newin;
	} else if (out == 2) {
	    out = dup(out);
	}
	if (log_to_fd >= 0) {
	    if (log_to_fd != 2)
		dup2(log_to_fd, 2);
	} else {
	    close(2);
	    errfd = open(_PATH_CONNERRS, O_WRONLY | O_APPEND | O_CREAT, 0600);
	    if (errfd >= 0 && errfd != 2) {
		dup2(errfd, 2);
		close(errfd);
	    }
	}
	if (in != 0) {
	    if (out == 0)
		out = dup(out);
	    dup2(in, 0);
	}
	if (out != 1) {
	    dup2(out, 1);
	}
	if (real_ttyfd > 2)
	    close(real_ttyfd);
	if (pty_master > 2)
	    close(pty_master);
	if (pty_slave > 2)
	    close(pty_slave);
	setuid(uid);
	if (getuid() != uid) {
	    error("setuid failed");
	    exit(1);
	}
	setgid(getgid());
	execl("/bin/sh", "sh", "-c", program, (char *)0);
	error("could not exec /bin/sh: %m");
	exit(99);
	/* NOTREACHED */
    }

    if (dont_wait) {
	record_child(pid, program, NULL, NULL);
	status = 0;
    } else {
	while (waitpid(pid, &status, 0) < 0) {
	    if (errno == EINTR)
		continue;
	    fatal("error waiting for (dis)connection process: %m");
	}
	--conn_running;
    }

    return (status == 0 ? 0 : -1);
}


/*
 * run-program - execute a program with given arguments,
 * but don't wait for it.
 * If the program can't be executed, logs an error unless
 * must_exist is 0 and the program file doesn't exist.
 * Returns -1 if it couldn't fork, 0 if the file doesn't exist
 * or isn't an executable plain file, or the process ID of the child.
 * If done != NULL, (*done)(arg) will be called later (within
 * reap_kids) iff the return value is > 0.
 */
pid_t
run_program(prog, args, must_exist, done, arg)
    char *prog;
    char **args;
    int must_exist;
    void (*done) __P((void *));
    void *arg;
{
    int pid;
    struct stat sbuf;

    /*
     * First check if the file exists and is executable.
     * We don't use access() because that would use the
     * real user-id, which might not be root, and the script
     * might be accessible only to root.
     */
    errno = EINVAL;
    if (stat(prog, &sbuf) < 0 || !S_ISREG(sbuf.st_mode)
	|| (sbuf.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)) == 0) {
	if (must_exist || errno != ENOENT)
	    warn("Can't execute %s: %m", prog);
	return 0;
    }

    pid = fork();
    if (pid == -1) {
	error("Failed to create child process for %s: %m", prog);
	return -1;
    }
    if (pid == 0) {
	int new_fd;

	/* Leave the current location */
	(void) setsid();	/* No controlling tty. */
	(void) umask (S_IRWXG|S_IRWXO);
	(void) chdir ("/");	/* no current directory. */
	setuid(0);		/* set real UID = root */
	setgid(getegid());

	/* Ensure that nothing of our device environment is inherited. */
	sys_close();
	closelog();
	close (0);
	close (1);
	close (2);
	close (ttyfd);  /* tty interface to the ppp device */
	if (real_ttyfd >= 0)
	    close(real_ttyfd);

        /* Don't pass handles to the PPP device, even by accident. */
	new_fd = open (_PATH_DEVNULL, O_RDWR);
	if (new_fd >= 0) {
	    if (new_fd != 0) {
	        dup2  (new_fd, 0); /* stdin <- /dev/null */
		close (new_fd);
	    }
	    dup2 (0, 1); /* stdout -> /dev/null */
	    dup2 (0, 2); /* stderr -> /dev/null */
	}

#ifdef BSD
	/* Force the priority back to zero if pppd is running higher. */
	if (setpriority (PRIO_PROCESS, 0, 0) < 0)
	    warn("can't reset priority to 0: %m"); 
#endif

	/* SysV recommends a second fork at this point. */

	/* run the program */
	execve(prog, args, script_env);
	if (must_exist || errno != ENOENT) {
	    /* have to reopen the log, there's nowhere else
	       for the message to go. */
	    reopen_log();
	    syslog(LOG_ERR, "Can't execute %s: %m", prog);
	    closelog();
	}
	_exit(-1);
    }

    if (debug)
	dbglog("Script %s started (pid %d)", prog, pid);
    record_child(pid, prog, done, arg);

    return pid;
}


/*
 * record_child - add a child process to the list for reap_kids
 * to use.
 */
static void
record_child(pid, prog, done, arg)
    int pid;
    char *prog;
    void (*done) __P((void *));
    void *arg;
{
    struct subprocess *chp;

    ++n_children;

    chp = (struct subprocess *) malloc(sizeof(struct subprocess));
    if (chp == NULL) {
	warn("losing track of %s process", prog);
    } else {
	chp->pid = pid;
	chp->prog = prog;
	chp->done = done;
	chp->arg = arg;
	chp->next = children;
	children = chp;
    }
}


/*
 * reap_kids - get status from any dead child processes,
 * and log a message for abnormal terminations.
 */
static int
reap_kids(waitfor)
    int waitfor;
{
    int pid, status;
    struct subprocess *chp, **prevp;

    got_sigchld = 0;
    if (n_children == 0)
	return 0;
    while ((pid = waitpid(-1, &status, (waitfor? 0: WNOHANG))) != -1
	   && pid != 0) {
	--n_children;
	for (prevp = &children; (chp = *prevp) != NULL; prevp = &chp->next) {
	    if (chp->pid == pid) {
		*prevp = chp->next;
		break;
	    }
	}
	if (WIFSIGNALED(status)) {
	    warn("Child process %s (pid %d) terminated with signal %d",
		 (chp? chp->prog: "??"), pid, WTERMSIG(status));
	} else if (debug)
	    dbglog("Script %s finished (pid %d), status = 0x%x",
		   (chp? chp->prog: "??"), pid, status);
	if (chp && chp->done)
	    (*chp->done)(chp->arg);
	if (chp)
	    free(chp);
    }
    if (pid == -1) {
	if (errno == ECHILD)
	    return -1;
	if (errno != EINTR)
	    error("Error waiting for child process: %m");
    }
    return 0;
}


/*
 * novm - log an error message saying we ran out of memory, and die.
 */
void
novm(msg)
    char *msg;
{
    fatal("Virtual memory exhausted allocating %s\n", msg);
}

/*
 * script_setenv - set an environment variable value to be used
 * for scripts that we run (e.g. ip-up, auth-up, etc.)
 */
void
script_setenv(var, value)
    char *var, *value;
{
    size_t vl = strlen(var) + strlen(value) + 2;
    int i;
    char *p, *newstring;

    newstring = (char *) malloc(vl);
    if (newstring == 0)
	return;
    slprintf(newstring, vl, "%s=%s", var, value);

    /* check if this variable is already set */
    if (script_env != 0) {
	for (i = 0; (p = script_env[i]) != 0; ++i) {
	    if (strncmp(p, var, vl) == 0 && p[vl] == '=') {
		free(p);
		script_env[i] = newstring;
		return;
	    }
	}
    } else {
	/* no space allocated for script env. ptrs. yet */
	i = 0;
	script_env = (char **) malloc(16 * sizeof(char *));
	if (script_env == 0)
	    return;
	s_env_nalloc = 16;
    }

    /* reallocate script_env with more space if needed */
    if (i + 1 >= s_env_nalloc) {
	int new_n = i + 17;
	char **newenv = (char **) realloc((void *)script_env,
					  new_n * sizeof(char *));
	if (newenv == 0)
	    return;
	script_env = newenv;
	s_env_nalloc = new_n;
    }

    script_env[i] = newstring;
    script_env[i+1] = 0;
}

/*
 * script_unsetenv - remove a variable from the environment
 * for scripts.
 */
void
script_unsetenv(var)
    char *var;
{
    int vl = strlen(var);
    int i;
    char *p;

    if (script_env == 0)
	return;
    for (i = 0; (p = script_env[i]) != 0; ++i) {
	if (strncmp(p, var, vl) == 0 && p[vl] == '=') {
	    free(p);
	    while ((script_env[i] = script_env[i+1]) != 0)
		++i;
	    break;
	}
    }
}

/*
 * start_charshunt - create a child process to run the character shunt.
 */
static int
start_charshunt(ifd, ofd)
    int ifd, ofd;
{
    int cpid;

    cpid = fork();
    if (cpid == -1) {
	error("Can't fork process for character shunt: %m");
	return 0;
    }
    if (cpid == 0) {
	/* child */
	close(pty_slave);
	setuid(uid);
	if (getuid() != uid)
	    fatal("setuid failed");
	setgid(getgid());
	if (!nodetach)
	    log_to_fd = -1;
	charshunt(ifd, ofd, record_file);
	exit(0);
    }
    charshunt_pid = cpid;
    close(pty_master);
    pty_master = -1;
    ttyfd = pty_slave;
    record_child(cpid, "pppd (charshunt)", charshunt_done, NULL);
    return 1;
}

static void
charshunt_done(arg)
    void *arg;
{
    charshunt_pid = 0;
}

/*
 * charshunt - the character shunt, which passes characters between
 * the pty master side and the serial port (or stdin/stdout).
 * This runs as the user (not as root).
 * (We assume ofd >= ifd which is true the way this gets called. :-).
 */
static void
charshunt(ifd, ofd, record_file)
    int ifd, ofd;
    char *record_file;
{
    int n, nfds;
    fd_set ready, writey;
    u_char *ibufp, *obufp;
    int nibuf, nobuf;
    int flags;
    int pty_readable, stdin_readable;
    struct timeval lasttime;
    FILE *recordf = NULL;

    /*
     * Reset signal handlers.
     */
    signal(SIGHUP, SIG_IGN);		/* Hangup */
    signal(SIGINT, SIG_DFL);		/* Interrupt */
    signal(SIGTERM, SIG_DFL);		/* Terminate */
    signal(SIGCHLD, SIG_DFL);
    signal(SIGUSR1, SIG_DFL);
    signal(SIGUSR2, SIG_DFL);
    signal(SIGABRT, SIG_DFL);
    signal(SIGALRM, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGILL, SIG_DFL);
    signal(SIGPIPE, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGSEGV, SIG_DFL);
#ifdef SIGBUS
    signal(SIGBUS, SIG_DFL);
#endif
#ifdef SIGEMT
    signal(SIGEMT, SIG_DFL);
#endif
#ifdef SIGPOLL
    signal(SIGPOLL, SIG_DFL);
#endif
#ifdef SIGPROF
    signal(SIGPROF, SIG_DFL);
#endif
#ifdef SIGSYS
    signal(SIGSYS, SIG_DFL);
#endif
#ifdef SIGTRAP
    signal(SIGTRAP, SIG_DFL);
#endif
#ifdef SIGVTALRM
    signal(SIGVTALRM, SIG_DFL);
#endif
#ifdef SIGXCPU
    signal(SIGXCPU, SIG_DFL);
#endif
#ifdef SIGXFSZ
    signal(SIGXFSZ, SIG_DFL);
#endif

    /*
     * Open the record file if required.
     */
    if (record_file != NULL) {
	recordf = fopen(record_file, "a");
	if (recordf == NULL)
	    error("Couldn't create record file %s: %m", record_file);
    }

    /* set all the fds to non-blocking mode */
    flags = fcntl(pty_master, F_GETFL);
    if (flags == -1
	|| fcntl(pty_master, F_SETFL, flags | O_NONBLOCK) == -1)
	warn("couldn't set pty master to nonblock: %m");
    flags = fcntl(ifd, F_GETFL);
    if (flags == -1
	|| fcntl(ifd, F_SETFL, flags | O_NONBLOCK) == -1)
	warn("couldn't set %s to nonblock: %m", (ifd==0? "stdin": "tty"));
    if (ofd != ifd) {
	flags = fcntl(ofd, F_GETFL);
	if (flags == -1
	    || fcntl(ofd, F_SETFL, flags | O_NONBLOCK) == -1)
	    warn("couldn't set stdout to nonblock: %m");
    }

    nibuf = nobuf = 0;
    ibufp = obufp = NULL;
    pty_readable = stdin_readable = 1;
    nfds = (ofd > pty_master? ofd: pty_master) + 1;
    if (recordf != NULL) {
	gettimeofday(&lasttime, NULL);
	putc(7, recordf);	/* put start marker */
	putc(lasttime.tv_sec >> 24, recordf);
	putc(lasttime.tv_sec >> 16, recordf);
	putc(lasttime.tv_sec >> 8, recordf);
	putc(lasttime.tv_sec, recordf);
	lasttime.tv_usec = 0;
    }

    while (nibuf != 0 || nobuf != 0 || pty_readable || stdin_readable) {
	FD_ZERO(&ready);
	FD_ZERO(&writey);
	if (nibuf != 0)
	    FD_SET(pty_master, &writey);
	else if (stdin_readable)
	    FD_SET(ifd, &ready);
	if (nobuf != 0)
	    FD_SET(ofd, &writey);
	else if (pty_readable)
	    FD_SET(pty_master, &ready);
	if (select(nfds, &ready, &writey, NULL, NULL) < 0) {
	    if (errno != EINTR)
		fatal("select");
	    continue;
	}
	if (FD_ISSET(ifd, &ready)) {
	    ibufp = inpacket_buf;
	    nibuf = read(ifd, ibufp, sizeof(inpacket_buf));
	    if (nibuf < 0 && errno == EIO)
		nibuf = 0;
	    if (nibuf < 0) {
		if (!(errno == EINTR || errno == EAGAIN)) {
		    error("Error reading standard input: %m");
		    break;
		}
		nibuf = 0;
	    } else if (nibuf == 0) {
		/* end of file from stdin */
		stdin_readable = 0;
		/* do a 0-length write, hopefully this will generate
		   an EOF (hangup) on the slave side. */
		write(pty_master, inpacket_buf, 0);
		if (recordf)
		    if (!record_write(recordf, 4, NULL, 0, &lasttime))
			recordf = NULL;
	    } else {
		FD_SET(pty_master, &writey);
		if (recordf)
		    if (!record_write(recordf, 2, ibufp, nibuf, &lasttime))
			recordf = NULL;
	    }
	}
	if (FD_ISSET(pty_master, &ready)) {
	    obufp = outpacket_buf;
	    nobuf = read(pty_master, obufp, sizeof(outpacket_buf));
	    if (nobuf < 0 && errno == EIO)
		nobuf = 0;
	    if (nobuf < 0) {
		if (!(errno == EINTR || errno == EAGAIN)) {
		    error("Error reading pseudo-tty master: %m");
		    break;
		}
		nobuf = 0;
	    } else if (nobuf == 0) {
		/* end of file from the pty - slave side has closed */
		pty_readable = 0;
		stdin_readable = 0;	/* pty is not writable now */
		nibuf = 0;
		close(ofd);
		if (recordf)
		    if (!record_write(recordf, 3, NULL, 0, &lasttime))
			recordf = NULL;
	    } else {
		FD_SET(ofd, &writey);
		if (recordf)
		    if (!record_write(recordf, 1, obufp, nobuf, &lasttime))
			recordf = NULL;
	    }
	}
	if (FD_ISSET(ofd, &writey)) {
	    n = write(ofd, obufp, nobuf);
	    if (n < 0) {
		if (errno != EIO) {
		    error("Error writing standard output: %m");
		    break;
		}
		pty_readable = 0;
		nobuf = 0;
	    } else {
		obufp += n;
		nobuf -= n;
	    }
	}
	if (FD_ISSET(pty_master, &writey)) {
	    n = write(pty_master, ibufp, nibuf);
	    if (n < 0) {
		if (errno != EIO) {
		    error("Error writing pseudo-tty master: %m");
		    break;
		}
		stdin_readable = 0;
		nibuf = 0;
	    } else {
		ibufp += n;
		nibuf -= n;
	    }
	}
    }
    exit(0);
}

static int
record_write(f, code, buf, nb, tp)
    FILE *f;
    int code;
    u_char *buf;
    int nb;
    struct timeval *tp;
{
    struct timeval now;
    int diff;

    gettimeofday(&now, NULL);
    now.tv_usec /= 100000;	/* actually 1/10 s, not usec now */
    diff = (now.tv_sec - tp->tv_sec) * 10 + (now.tv_usec - tp->tv_usec);
    if (diff > 0) {
	if (diff > 255) {
	    putc(5, f);
	    putc(diff >> 24, f);
	    putc(diff >> 16, f);
	    putc(diff >> 8, f);
	    putc(diff, f);
	} else {
	    putc(6, f);
	    putc(diff, f);
	}
	*tp = now;
    }
    putc(code, f);
    if (buf != NULL) {
	putc(nb >> 8, f);
	putc(nb, f);
	fwrite(buf, nb, 1, f);
    }
    fflush(f);
    if (ferror(f)) {
	error("Error writing record file: %m");
	return 0;
    }
    return 1;
}
