/*
 * pppd-private.h - PPP daemon private declarations.
 *
 * Copyright (c) 1984-2000 Carnegie Mellon University. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name "Carnegie Mellon University" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For permission or any legal
 *    details, please contact
 *      Office of Technology Transfer
 *      Carnegie Mellon University
 *      5000 Forbes Avenue
 *      Pittsburgh, PA  15213-3890
 *      (412) 268-4387, fax: (412) 268-7395
 *      tech-transfer@andrew.cmu.edu
 *
 * 4. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Computing Services
 *     at Carnegie Mellon University (http://www.cmu.edu/computing/)."
 *
 * CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef PPP_PPPD_PRIVATE_H
#define PPP_PPPD_PRIVATE_H

#include <stdio.h>		/* for FILE */
#include <stdlib.h>		/* for encrypt */
#include <unistd.h>		/* for setkey */
#if defined(__linux__)
#include <linux/ppp_defs.h>
#else
#include <net/ppp_defs.h>
#endif

#include "pppd.h"

#ifdef PPP_WITH_IPV6CP
#include "eui64.h"
#endif

/*
 * If PPP_DRV_NAME is not defined, use the default "ppp" as the device name.
 * Where should PPP_DRV_NAME come from? Do we include it here?
 */
#if !defined(PPP_DRV_NAME)
#if defined(SOL2)
#define PPP_DRV_NAME	"sppp"
#else
#define PPP_DRV_NAME	"ppp"
#endif /* defined(SOL2) */
#endif /* !defined(PPP_DRV_NAME) */


#ifndef GIDSET_TYPE
#define GIDSET_TYPE	gid_t
#endif

/* Structure representing a list of permitted IP addresses. */
struct permitted_ip {
    int		permit;		/* 1 = permit, 0 = forbid */
    u_int32_t	base;		/* match if (addr & mask) == base */
    u_int32_t	mask;		/* base and mask are in network byte order */
};

struct notifier {
    struct notifier *next;
    ppp_notify_fn *func;
    void *arg;
};

/*
 * Global variables.
 */

extern int	hungup;		/* Physical layer has disconnected */
extern int	ifunit;		/* Interface unit number */
extern char	ifname[];	/* Interface name (IFNAMSIZ) */
extern char	hostname[];	/* Our hostname */
extern unsigned char	outpacket_buf[]; /* Buffer for outgoing packets */
extern int	devfd;		/* fd of underlying device */
extern int	fd_ppp;		/* fd for talking PPP */
extern int	baud_rate;	/* Current link speed in bits/sec */
extern char	*progname;	/* Name of this program */
extern int	redirect_stderr;/* Connector's stderr should go to file */
extern char	peer_authname[];/* Authenticated name of peer */
extern int	auth_done[NUM_PPP]; /* Methods actually used for auth */
extern int	privileged;	/* We were run by real-uid root */
extern int	need_holdoff;	/* Need holdoff period after link terminates */
extern char	**script_env;	/* Environment variables for scripts */
extern int	detached;	/* Have detached from controlling tty */
extern GIDSET_TYPE groups[];	/* groups the user is in */
extern int	ngroups;	/* How many groups valid in groups */
extern int	link_stats_valid; /* set if link_stats is valid */
extern int	link_stats_print; /* set if link_stats is to be printed on link termination */
extern int	log_to_fd;	/* logging to this fd as well as syslog */
extern bool	log_default;	/* log_to_fd is default (stdout) */
extern char	*no_ppp_msg;	/* message to print if ppp not in kernel */
extern bool	devnam_fixed;	/* can no longer change devnam */
extern int	unsuccess;	/* # unsuccessful connection attempts */
extern int	do_callback;	/* set if we want to do callback next */
extern int	doing_callback;	/* set if this is a callback */
extern int	error_count;	/* # of times error() has been called */
extern char	ppp_devname[];	/* name of PPP tty (maybe ttypx) */
extern int	fd_devnull;	/* fd open to /dev/null */

extern int	listen_time;	/* time to listen first (ms) */
extern bool	bundle_eof;
extern bool	bundle_terminating;

extern struct notifier *pidchange;   /* for notifications of pid changing */
extern struct notifier *phasechange; /* for notifications of phase changes */
extern struct notifier *exitnotify;  /* for notification that we're exiting */
extern struct notifier *sigreceived; /* notification of received signal */
extern struct notifier *ip_up_notifier;     /* IPCP has come up */
extern struct notifier *ip_down_notifier;   /* IPCP has gone down */
extern struct notifier *ipv6_up_notifier;   /* IPV6CP has come up */
extern struct notifier *ipv6_down_notifier; /* IPV6CP has gone down */
extern struct notifier *auth_up_notifier; /* peer has authenticated */
extern struct notifier *link_down_notifier; /* link has gone down */
extern struct notifier *fork_notifier;	/* we are a new child process */


/* Values for do_callback and doing_callback */
#define CALLBACK_DIALIN		1	/* we are expecting the call back */
#define CALLBACK_DIALOUT	2	/* we are dialling out to call back */

/*
 * Variables set by command-line options.
 */

extern int	debug;		/* Debug flag */
extern int	kdebugflag;	/* Tell kernel to print debug messages */
extern int	default_device;	/* Using /dev/tty or equivalent */
extern char	devnam[];	/* Device name */
extern char remote_number[MAXNAMELEN]; /* Remote telephone number, if avail. */
extern int  ppp_session_number; /* Session number (eg PPPoE session) */
extern int	crtscts;	/* Use hardware flow control */
extern int	stop_bits;	/* Number of serial port stop bits */
extern bool	modem;		/* Use modem control lines */
extern int	inspeed;	/* Input/Output speed requested */
extern u_int32_t netmask;	/* IP netmask to set on interface */
extern bool	lockflag;	/* Create lock file to lock the serial dev */
extern bool	nodetach;	/* Don't detach from controlling tty */
#ifdef SYSTEMD
extern bool	up_sdnotify;	/* Notify systemd once link is up (implies nodetach) */
#endif
extern bool	updetach;	/* Detach from controlling tty when link up */
extern bool	master_detach;	/* Detach when multilink master without link (options.c) */
extern char	*initializer;	/* Script to initialize physical link */
extern char	*connect_script; /* Script to establish physical link */
extern char	*disconnect_script; /* Script to disestablish physical link */
extern char	*welcomer;	/* Script to welcome client after connection */
extern char	*ptycommand;	/* Command to run on other side of pty */
extern char	user[MAXNAMELEN];/* Our name for authenticating ourselves */
extern char	passwd[MAXSECRETLEN];	/* Password for PAP or CHAP */
extern bool	auth_required;	/* Peer is required to authenticate */
extern bool	persist;	/* Reopen link after it goes down */
extern bool	uselogin;	/* Use /etc/passwd for checking PAP */
extern bool	session_mgmt;	/* Do session management (login records) */
extern char	our_name[MAXNAMELEN];/* Our name for authentication purposes */
extern char	remote_name[MAXNAMELEN]; /* Peer's name for authentication */
extern char	path_upapfile[];/* Pathname of pap-secrets file */
extern char	path_chapfile[];/* Pathname of chap-secrets file */
extern bool	explicit_remote;/* remote_name specified with remotename opt */
extern bool	demand;		/* Do dial-on-demand */
extern char	*ipparam;	/* Extra parameter for ip up/down scripts */
extern bool	cryptpap;	/* Others' PAP passwords are encrypted */
extern int	holdoff;	/* Dead time before restarting */
extern bool	holdoff_specified; /* true if user gave a holdoff value */
extern bool	notty;		/* Stdin/out is not a tty */
extern char	*pty_socket;	/* Socket to connect to pty */
extern char	*record_file;	/* File to record chars sent/received */
extern int	maxfail;	/* Max # of unsuccessful connection attempts */
extern char	linkname[];	/* logical name for link */
extern bool	tune_kernel;	/* May alter kernel settings as necessary */
extern int	connect_delay;	/* Time to delay after connect script */
extern int	max_data_rate;	/* max bytes/sec through charshunt */
extern int	req_unit;	/* interface unit number to use */
extern char	path_net_init[]; /* pathname of net-init script */
extern char	path_net_preup[];/* pathname of net-pre-up script */
extern char	path_net_down[]; /* pathname of net-down script */
extern char	path_ipup[]; 	/* pathname of ip-up script */
extern char	path_ipdown[];	/* pathname of ip-down script */
extern char	path_ippreup[];	/* pathname of ip-pre-up script */
extern char	req_ifname[]; /* interface name to use (IFNAMSIZ) */
extern bool	multilink;	/* enable multilink operation (options.c) */
extern bool	noendpoint;	/* don't send or accept endpt. discrim. */
extern char	*bundle_name;	/* bundle name for multilink */
extern bool	dump_options;	/* print out option values */
extern bool	show_options;	/* show all option names and descriptions */
extern bool	dryrun;		/* check everything, print options, exit */
extern int	child_wait;	/* # seconds to wait for children at end */
extern char *current_option;    /* the name of the option being parsed */
extern int  privileged_option;  /* set iff the current option came from root */
extern char *option_source;     /* string saying where the option came from */
extern int  option_priority;    /* priority of current options */

#ifdef PPP_WITH_IPV6CP
extern char	path_ipv6up[]; /* pathname of ipv6-up script */
extern char	path_ipv6down[]; /* pathname of ipv6-down script */
#endif

#if defined(PPP_WITH_EAPTLS) || defined(PPP_WITH_PEAP)
#define TLS_VERIFY_NONE     "none"
#define TLS_VERIFY_NAME     "name"
#define TLS_VERIFY_SUBJECT  "subject"
#define TLS_VERIFY_SUFFIX   "suffix"

extern char *crl_dir;
extern char *crl_file;
extern char *ca_path;
extern char *cacert_file;

extern char *max_tls_version;
extern bool tls_verify_key_usage;
extern char *tls_verify_method;
#endif /* PPP_WITH_EAPTLS || PPP_WITH_PEAP */

#ifdef PPP_WITH_EAPTLS
extern char *pkcs12_file;
#endif /* PPP_WITH_EAPTLS */

typedef enum {
    PPP_OCTETS_DIRECTION_SUM,
    PPP_OCTETS_DIRECTION_IN,
    PPP_OCTETS_DIRECTION_OUT,
    PPP_OCTETS_DIRECTION_MAXOVERAL,
    PPP_OCTETS_DIRECTION_MAXSESSION             /* Same as MAXOVERALL, but a little different for RADIUS */
} session_limit_dir_t;

extern unsigned int        maxoctets;           /* Maximum octetes per session (in bytes) */
extern session_limit_dir_t maxoctets_dir;       /* Direction */
extern int                 maxoctets_timeout;   /* Timeout for check of octets limit */

#ifdef PPP_WITH_FILTER
/* Filter for pkts to pass */
extern struct	bpf_program pass_filter_in;
extern struct	bpf_program pass_filter_out;
/* Filter for link-active pkts */
extern struct	bpf_program active_filter_in;
extern struct	bpf_program active_filter_out;
#endif

#ifdef PPP_WITH_MSLANMAN
extern bool	ms_lanman;	/* Use LanMan password instead of NT */
				/* Has meaning only with MS-CHAP challenges */
#endif

/* Values for auth_pending, auth_done */
#define PAP_WITHPEER	0x1
#define PAP_PEER	0x2
#define CHAP_WITHPEER	0x4
#define CHAP_PEER	0x8
#define EAP_WITHPEER	0x10
#define EAP_PEER	0x20

/* Values for auth_done only */
#define CHAP_MD5_WITHPEER	0x40
#define CHAP_MD5_PEER		0x80
#define CHAP_MS_SHIFT		8	/* LSB position for MS auths */
#define CHAP_MS_WITHPEER	0x100
#define CHAP_MS_PEER		0x200
#define CHAP_MS2_WITHPEER	0x400
#define CHAP_MS2_PEER		0x800


/*
 * This structure contains environment variables that are set or unset
 * by the user.
 */
struct userenv {
	struct userenv *ue_next;
	char *ue_value;		/* value (set only) */
	bool ue_isset;		/* 1 for set, 0 for unset */
	bool ue_priv;		/* from privileged source */
	const char *ue_source;	/* source name */
	char ue_name[1];	/* variable name */
};

extern struct userenv *userenv_list;

/*
 * Prototypes.
 */

/* Procedures exported from main.c. */
void set_ifunit(int);	/* set stuff that depends on ifunit */
void detach(void);	/* Detach from controlling tty */
void die(int);		/* Cleanup and exit */
void quit(void);		/* like die(1) */

void record_child(int, char *, void (*) (void *), void *, int);
int  device_script(char *cmd, int in, int out, int dont_wait);
				/* Run `cmd' with given stdin and stdout */
pid_t run_program(char *prog, char * const * args, int must_exist,
		  void (*done)(void *), void *arg, int wait);
				/* Run program prog with args in child */
void reopen_log(void);	/* (re)open the connection to syslog */
void print_link_stats(void); /* Print stats, if available */
void reset_link_stats(int); /* Reset (init) stats when link goes up */
void new_phase(ppp_phase_t);	/* signal start of new phase */
bool in_phase(ppp_phase_t);
void notify(struct notifier *, int);
int  ppp_send_config(int, int, u_int32_t, int, int);
int  ppp_recv_config(int, int, u_int32_t, int, int);
const char *protocol_name(int);
void remove_pidfiles(void);
void lock_db(void);
void unlock_db(void);

/* Procedures exported from tty.c. */
void tty_init(void);

void print_string(char *, int,  printer_func, void *);
				/* Format a string for output */
ssize_t complete_read(int, void *, size_t);
				/* read a complete buffer */

/* Procedures exported from auth.c */
void link_required(int);	  /* we are starting to use the link */
void start_link(int);	  /* bring the link up now */
void link_terminated(int);  /* we are finished with the link */
void link_down(int);	  /* the LCP layer has left the Opened state */
void upper_layers_down(int);/* take all NCPs down */
void link_established(int); /* the link is up; authenticate now */
void start_networks(int);   /* start all the network control protos */
void continue_networks(int); /* start network [ip, etc] control protos */
void np_up(int, int);	  /* a network protocol has come up */
void np_down(int, int);	  /* a network protocol has gone down */
void np_finished(int, int); /* a network protocol no longer needs link */
void auth_peer_fail(int, int);
				/* peer failed to authenticate itself */
void auth_peer_success(int, int, int, char *, int);
				/* peer successfully authenticated itself */
void auth_withpeer_fail(int, int);
				/* we failed to authenticate ourselves */
void auth_withpeer_success(int, int, int);
				/* we successfully authenticated ourselves */
void auth_check_options(void);
				/* check authentication options supplied */
void auth_reset(int);	/* check what secrets we have */
int  check_passwd(int, char *, int, char *, int, char **);
				/* Check peer-supplied username/password */
int  get_secret(int, char *, char *, char *, int *, int);
				/* get "secret" for chap */
int  get_srp_secret(int unit, char *client, char *server, char *secret,
    int am_server);
int  auth_ip_addr(int, u_int32_t);
				/* check if IP address is authorized */
int  auth_number(void);	/* check if remote number is authorized */

/* Procedures exported from demand.c */
void demand_conf(void);	/* config interface(s) for demand-dial */
void demand_block(void);	/* set all NPs to queue up packets */
void demand_unblock(void); /* set all NPs to pass packets */
void demand_discard(void); /* set all NPs to discard packets */
void demand_rexmit(int);	/* retransmit saved frames for an NP */
int  loop_chars(unsigned char *, int); /* process chars from loopback */
int  loop_frame(unsigned char *, int); /* should we bring link up? */

/* Procedures exported from sys-*.c */
void sys_init(void);	/* Do system-dependent initialization */
void sys_cleanup(void);	/* Restore system state before exiting */
int  sys_check_options(void); /* Check options specified */
int  get_pty(int *, int *, char *, int);	/* Get pty master/slave */
int  open_ppp_loopback(void); /* Open loopback for demand-dialling */
int  tty_establish_ppp(int);  /* Turn serial port into a ppp interface */
void tty_disestablish_ppp(int); /* Restore port to normal operation */
void make_new_bundle(int, int, int, int); /* Create new bundle */
int  bundle_attach(int);	/* Attach link to existing bundle */
void cfg_bundle(int, int, int, int); /* Configure existing bundle */
void destroy_bundle(void); /* Tell driver to destroy bundle */
void clean_check(void);	/* Check if line was 8-bit clean */
void set_up_tty(int, int); /* Set up port's speed, parameters, etc. */
void restore_tty(int);	/* Restore port's original parameters */
void setdtr(int, int);	/* Raise or lower port's DTR line */
void output(int, unsigned char *, int); /* Output a PPP packet */
void wait_input(struct timeval *);
				/* Wait for input, with timeout */
void add_fd(int);		/* Add fd to set to wait for */
void remove_fd(int);	/* Remove fd from set to wait for */
int  read_packet(unsigned char *); /* Read PPP packet */
int  get_loop_output(void); /* Read pkts from loopback */
void tty_send_config(int, u_int32_t, int, int);
				/* Configure i/f transmit parameters */
void tty_set_xaccm(ext_accm);
				/* Set extended transmit ACCM */
void tty_recv_config(int, u_int32_t, int, int);
				/* Configure i/f receive parameters */
int  ccp_test(int, unsigned char *, int, int);
				/* Test support for compression scheme */
void ccp_flags_set(int, int, int);
				/* Set kernel CCP state */
int  ccp_fatal_error(int); /* Test for fatal decomp error in kernel */
int  get_idle_time(int, struct ppp_idle *);
				/* Find out how long link has been idle */
int  get_ppp_stats(int, struct pppd_stats *);
				/* Return link statistics */
int  sifvjcomp(int, int, int, int);
				/* Configure VJ TCP header compression */
int  sifup(int);		/* Configure i/f up for one protocol */
int  sifnpmode(int u, int proto, enum NPmode mode);
				/* Set mode for handling packets for proto */
int  sifdown(int);	/* Configure i/f down for one protocol */
int  sifaddr(int, u_int32_t, u_int32_t, u_int32_t);
				/* Configure IPv4 addresses for i/f */
int  cifaddr(int, u_int32_t, u_int32_t);
				/* Reset i/f IP addresses */
#ifdef PPP_WITH_IPV6CP
int  sif6up(int);		/* Configure i/f up for IPv6 */
int  sif6down(int);	/* Configure i/f down for IPv6 */
int  sif6addr(int, eui64_t, eui64_t);
				/* Configure IPv6 addresses for i/f */
int  cif6addr(int, eui64_t, eui64_t);
				/* Remove an IPv6 address from i/f */
#endif
int  sifdefaultroute(int, u_int32_t, u_int32_t, bool replace_default_rt);
				/* Create default route through i/f */
int  cifdefaultroute(int, u_int32_t, u_int32_t);
				/* Delete default route through i/f */
#ifdef PPP_WITH_IPV6CP
int  sif6defaultroute(int, eui64_t, eui64_t);
				/* Create default IPv6 route through i/f */
int  cif6defaultroute(int, eui64_t, eui64_t);
				/* Delete default IPv6 route through i/f */
#endif
int  sifproxyarp(int, u_int32_t);
				/* Add proxy ARP entry for peer */
int  cifproxyarp(int, u_int32_t);
				/* Delete proxy ARP entry for peer */
u_int32_t GetMask(u_int32_t); /* Get appropriate netmask for address */
int  mkdir_recursive(const char *); /* Recursively create directory */
int  lock(char *);	/* Create lock file for device */
int  relock(int);		/* Rewrite lock file with new pid */
void unlock(void);	/* Delete previously-created lock file */
void logwtmp(const char *, const char *, const char *);
				/* Write entry to wtmp file */
int  get_host_seed(void);	/* Get host-dependent random number seed */
int  have_route_to(u_int32_t); /* Check if route to addr exists */
#ifdef PPP_WITH_FILTER
int  set_filters(struct bpf_program *pass_in, struct bpf_program *pass_out,
    struct bpf_program *active_in, struct bpf_program *active_out);
				/* Set filter programs in kernel */
#endif
int  get_if_hwaddr(unsigned char *addr, char *name);
int  get_first_ether_hwaddr(unsigned char *addr);

/* Procedures exported from options.c */
int setipaddr(char *, char **, int); /* Set local/remote ip addresses */
int  parse_args(int argc, char **argv);
				/* Parse options from arguments given */
int  getword(FILE *f, char *word, int *newlinep, char *filename);
				/* Read a word from a file */
int  options_from_user(void); /* Parse options from user's .ppprc */
int  options_for_tty(void); /* Parse options from /etc/ppp/options.tty */
struct wordlist;
int  options_from_list(struct wordlist *, int privileged);
				/* Parse options from a wordlist */
void check_options(void);	/* check values after all options parsed */
int  override_value(char *, int, const char *);
				/* override value if permitted by priority */
void print_options(printer_func, void *);
				/* print out values of all options */
void showopts(void);
                /* show all option names and description */
int parse_dotted_ip(char *, u_int32_t *);

/*
 * Inline versions of get/put char/short/long.
 * Pointer is advanced; we assume that both arguments
 * are lvalues and will already be in registers.
 * cp MUST be unsigned char *.
 */
#define GETCHAR(c, cp) { \
	(c) = *(cp)++; \
}
#define PUTCHAR(c, cp) { \
	*(cp)++ = (unsigned char) (c); \
}


#define GETSHORT(s, cp) { \
	(s) = *(cp)++ << 8; \
	(s) |= *(cp)++; \
}
#define PUTSHORT(s, cp) { \
	*(cp)++ = (unsigned char) ((s) >> 8); \
	*(cp)++ = (unsigned char) (s); \
}

#define GETLONG(l, cp) { \
	(l) = *(cp)++ << 8; \
	(l) |= *(cp)++; (l) <<= 8; \
	(l) |= *(cp)++; (l) <<= 8; \
	(l) |= *(cp)++; \
}
#define PUTLONG(l, cp) { \
	*(cp)++ = (unsigned char) ((l) >> 24); \
	*(cp)++ = (unsigned char) ((l) >> 16); \
	*(cp)++ = (unsigned char) ((l) >> 8); \
	*(cp)++ = (unsigned char) (l); \
}

#define INCPTR(n, cp)	((cp) += (n))
#define DECPTR(n, cp)	((cp) -= (n))

/*
 * System dependent definitions for user-level 4.3BSD UNIX implementation.
 */

#define TIMEOUT(r, f, t)	ppp_timeout((r), (f), (t), 0)
#define UNTIMEOUT(r, f)		ppp_untimeout((r), (f))

#define BCOPY(s, d, l)		memcpy(d, s, l)
#define BZERO(s, n)		memset(s, 0, n)
#define	BCMP(s1, s2, l)		memcmp(s1, s2, l)

#define PRINTMSG(m, l)		{ info("Remote message: %0.*v", l, m); }

/*
 * MAKEHEADER - Add Header fields to a packet.
 */
#define MAKEHEADER(p, t) { \
    PUTCHAR(PPP_ALLSTATIONS, p); \
    PUTCHAR(PPP_UI, p); \
    PUTSHORT(t, p); }

/*
 * Debug macros.  Slightly useful for finding bugs in pppd, not particularly
 * useful for finding out why your connection isn't being established.
 */
#ifdef DEBUGALL
#define DEBUGMAIN	1
#define DEBUGFSM	1
#define DEBUGLCP	1
#define DEBUGIPCP	1
#define DEBUGIPV6CP	1
#define DEBUGUPAP	1
#define DEBUGCHAP	1
#endif

#ifndef LOG_PPP			/* we use LOG_LOCAL2 for syslog by default */
#if defined(DEBUGMAIN) || defined(DEBUGFSM) || defined(DEBUGSYS) \
  || defined(DEBUGLCP) || defined(DEBUGIPCP) || defined(DEBUGUPAP) \
  || defined(DEBUGCHAP) || defined(DEBUG) || defined(DEBUGIPV6CP)
#define LOG_PPP LOG_LOCAL2
#else
#define LOG_PPP LOG_DAEMON
#endif
#endif /* LOG_PPP */

#ifdef DEBUGMAIN
#define MAINDEBUG(x)	if (debug) dbglog x
#else
#define MAINDEBUG(x)
#endif

#ifdef DEBUGSYS
#define SYSDEBUG(x)	if (debug) dbglog x
#else
#define SYSDEBUG(x)
#endif

#ifdef DEBUGFSM
#define FSMDEBUG(x)	if (debug) dbglog x
#else
#define FSMDEBUG(x)
#endif

#ifdef DEBUGLCP
#define LCPDEBUG(x)	if (debug) dbglog x
#else
#define LCPDEBUG(x)
#endif

#ifdef DEBUGIPCP
#define IPCPDEBUG(x)	if (debug) dbglog x
#else
#define IPCPDEBUG(x)
#endif

#ifdef DEBUGIPV6CP
#define IPV6CPDEBUG(x)  if (debug) dbglog x
#else
#define IPV6CPDEBUG(x)
#endif

#ifdef DEBUGUPAP
#define UPAPDEBUG(x)	if (debug) dbglog x
#else
#define UPAPDEBUG(x)
#endif

#ifdef DEBUGCHAP
#define CHAPDEBUG(x)	if (debug) dbglog x
#else
#define CHAPDEBUG(x)
#endif

#ifndef SIGTYPE
#if defined(sun) || defined(SYSV) || defined(POSIX_SOURCE)
#define SIGTYPE void
#else
#define SIGTYPE int
#endif /* defined(sun) || defined(SYSV) || defined(POSIX_SOURCE) */
#endif /* SIGTYPE */

#ifndef MIN
#define MIN(a, b)	((a) < (b)? (a): (b))
#endif
#ifndef MAX
#define MAX(a, b)	((a) > (b)? (a): (b))
#endif

#ifndef offsetof
#define offsetof(type, member) ((size_t) &((type *)0)->member)
#endif

#endif
