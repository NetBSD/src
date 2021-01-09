/*	$NetBSD: pppd.h,v 1.6 2021/01/09 16:39:28 christos Exp $	*/

/*
 * pppd.h - PPP daemon global declarations.
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
 *
 * Id: pppd.h,v 1.96 2008/06/23 11:47:18 paulus Exp 
 */

/*
 * TODO:
 */

#ifndef __PPPD_H__
#define __PPPD_H__

#include <stdio.h>		/* for FILE */
#include <stdlib.h>		/* for encrypt */
#include <unistd.h>		/* for setkey */
#include <stdarg.h>
#include <limits.h>		/* for NGROUPS_MAX */
#include <stddef.h>		/* offsetof() */
#include <sys/param.h>		/* for MAXPATHLEN and BSD4_4, if defined */
#include <sys/types.h>		/* for u_int32_t, if defined */
#include <sys/time.h>		/* for struct timeval */
#include <net/ppp_defs.h>
#include "patchlevel.h"

#ifdef INET6
#include "eui64.h"
#endif

/*
 * Limits.
 */

#define NUM_PPP		1	/* One PPP interface supported (per process) */
#define MAXWORDLEN	1024	/* max length of word in file (incl null) */
#define MAXARGS		1	/* max # args to a command */
#define MAXNAMELEN	256	/* max length of hostname or name for auth */
#define MAXSECRETLEN	256	/* max length of password or secret */
#define MAXIFNAMELEN	32	/* max length of interface name; or use IFNAMSIZ, can we
				   always include net/if.h? */

/*
 * If PPP_DRV_NAME is not defined, use the default "ppp" as the device name.
 * Where should PPP_DRV_NAME come from? Do we include it here?
 */
#if !defined(PPP_DRV_NAME)
#define PPP_DRV_NAME	"ppp"
#endif /* !defined(PPP_DRV_NAME) */

/*
 * Option descriptor structure.
 */
#include <stdbool.h>

enum opt_type {
	o_special_noarg = 0,
	o_special = 1,
	o_bool,
	o_int,
	o_uint32,
	o_string,
	o_wild
};

typedef struct {
	char	*name;		/* name of the option */
	enum opt_type type;
	void	*addr;
	char	*description;
	unsigned int flags;
	void	*addr2;
	int	upper_limit;
	int	lower_limit;
	const char *source;
	short int priority;
	short int winner;
} option_t;

/* Values for flags */
#define OPT_VALUE	0xff	/* mask for presupplied value */
#define OPT_HEX		0x100	/* int option is in hex */
#define OPT_NOARG	0x200	/* option doesn't take argument */
#define OPT_OR		0x400	/* for u32, OR in argument to value */
#define OPT_INC		0x400	/* for o_int, increment value */
#define OPT_A2OR	0x800	/* for o_bool, OR arg to *(u_char *)addr2 */
#define OPT_PRIV	0x1000	/* privileged option */
#define OPT_STATIC	0x2000	/* string option goes into static array */
#define OPT_NOINCR	0x2000	/* for o_int, value mustn't be increased */
#define OPT_LLIMIT	0x4000	/* check value against lower limit */
#define OPT_ULIMIT	0x8000	/* check value against upper limit */
#define OPT_LIMITS	(OPT_LLIMIT|OPT_ULIMIT)
#define OPT_ZEROOK	0x10000	/* 0 value is OK even if not within limits */
#define OPT_HIDE	0x10000	/* for o_string, print value as ?????? */
#define OPT_A2LIST	0x20000 /* for o_special, keep list of values */
#define OPT_A2CLRB	0x20000 /* o_bool, clr val bits in *(u_char *)addr2 */
#define OPT_ZEROINF	0x40000	/* with OPT_NOINCR, 0 == infinity */
#define OPT_PRIO	0x80000	/* process option priorities for this option */
#define OPT_PRIOSUB	0x100000 /* subsidiary member of priority group */
#define OPT_ALIAS	0x200000 /* option is alias for previous option */
#define OPT_A2COPY	0x400000 /* addr2 -> second location to rcv value */
#define OPT_ENABLE	0x800000 /* use *addr2 as enable for option */
#define OPT_A2CLR	0x1000000 /* clear *(bool *)addr2 */
#define OPT_PRIVFIX	0x2000000 /* user can't override if set by root */
#define OPT_INITONLY	0x4000000 /* option can only be set in init phase */
#define OPT_DEVEQUIV	0x8000000 /* equiv to device name */
#define OPT_DEVNAM	(OPT_INITONLY | OPT_DEVEQUIV)
#define OPT_A2PRINTER	0x10000000 /* *addr2 printer_func to print option */
#define OPT_A2STRVAL	0x20000000 /* *addr2 points to current string value */
#define OPT_NOPRINT	0x40000000 /* don't print this option at all */

#define OPT_VAL(x)	((x) & OPT_VALUE)

/* Values for priority */
#define OPRIO_DEFAULT	0	/* a default value */
#define OPRIO_CFGFILE	1	/* value from a configuration file */
#define OPRIO_CMDLINE	2	/* value from the command line */
#define OPRIO_SECFILE	3	/* value from options in a secrets file */
#define OPRIO_ROOT	100	/* added to priority if OPT_PRIVFIX && root */

#ifndef GIDSET_TYPE
#define GIDSET_TYPE	gid_t
#endif

/* Structure representing a list of permitted IP addresses. */
struct permitted_ip {
    int		permit;		/* 1 = permit, 0 = forbid */
    u_int32_t	base;		/* match if (addr & mask) == base */
    u_int32_t	mask;		/* base and mask are in network byte order */
};

/*
 * Unfortunately, the linux kernel driver uses a different structure
 * for statistics from the rest of the ports.
 * This structure serves as a common representation for the bits
 * pppd needs.
 */
struct pppd_stats {
    unsigned int	bytes_in;
    unsigned int	bytes_out;
    unsigned int	pkts_in;
    unsigned int	pkts_out;
};

/* Used for storing a sequence of words.  Usually malloced. */
struct wordlist {
    struct wordlist	*next;
    char		*word;
};

/* An endpoint discriminator, used with multilink. */
#define MAX_ENDP_LEN	20	/* maximum length of discriminator value */
struct epdisc {
    unsigned char	class;
    unsigned char	length;
    unsigned char	value[MAX_ENDP_LEN];
};

/* values for epdisc.class */
#define EPD_NULL	0	/* null discriminator, no data */
#define EPD_LOCAL	1
#define EPD_IP		2
#define EPD_MAC		3
#define EPD_MAGIC	4
#define EPD_PHONENUM	5

typedef void (*notify_func)(void *, int);
typedef void (*printer_func)(void *, char *, ...);

struct notifier {
    struct notifier *next;
    notify_func	    func;
    void	    *arg;
};

/*
 * Global variables.
 */

extern int	got_sigterm;	/* SIGINT or SIGTERM was received */
extern int	hungup;		/* Physical layer has disconnected */
extern int	ifunit;		/* Interface unit number */
extern char	ifname[];	/* Interface name */
extern char	hostname[];	/* Our hostname */
extern u_char	outpacket_buf[]; /* Buffer for outgoing packets */
extern int	devfd;		/* fd of underlying device */
extern int	fd_ppp;		/* fd for talking PPP */
extern int	phase;		/* Current state of link - see values below */
extern int	baud_rate;	/* Current link speed in bits/sec */
extern char	*progname;	/* Name of this program */
extern int	redirect_stderr;/* Connector's stderr should go to file */
extern char	peer_authname[];/* Authenticated name of peer */
extern int	auth_done[NUM_PPP]; /* Methods actually used for auth */
extern int	privileged;	/* We were run by real-uid root */
extern int	need_holdoff;	/* Need holdoff period after link terminates */
extern char	**script_env;	/* Environment variables for scripts */
extern int	detached;	/* Have detached from controlling tty */
extern GIDSET_TYPE groups[NGROUPS_MAX];	/* groups the user is in */
extern int	ngroups;	/* How many groups valid in groups */
extern struct pppd_stats link_stats; /* byte/packet counts etc. for link */
extern int	link_stats_valid; /* set if link_stats is valid */
extern unsigned	link_connect_time; /* time the link was up for */
extern int	using_pty;	/* using pty as device (notty or pty opt.) */
extern int	log_to_fd;	/* logging to this fd as well as syslog */
extern bool	log_default;	/* log_to_fd is default (stdout) */
extern char	*no_ppp_msg;	/* message to print if ppp not in kernel */
extern volatile int status;	/* exit status for pppd */
extern bool	devnam_fixed;	/* can no longer change devnam */
extern int	unsuccess;	/* # unsuccessful connection attempts */
extern int	do_callback;	/* set if we want to do callback next */
extern int	doing_callback;	/* set if this is a callback */
extern int	error_count;	/* # of times error() has been called */
extern char	ppp_devnam[MAXPATHLEN];
extern char     remote_number[MAXNAMELEN]; /* Remote telephone number, if avail. */
extern int      ppp_session_number; /* Session number (eg PPPoE session) */
extern int	fd_devnull;	/* fd open to /dev/null */

extern int	listen_time;	/* time to listen first (ms) */
extern bool	doing_multilink;
extern bool	multilink_master;
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
extern char	devnam[MAXPATHLEN];	/* Device name */
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
extern bool	master_detach;	/* Detach when multilink master without link */
extern char	*initializer;	/* Script to initialize physical link */
extern char	*connect_script; /* Script to establish physical link */
extern char	*disconnect_script; /* Script to disestablish physical link */
extern char	*welcomer;	/* Script to welcome client after connection */
extern char	*ptycommand;	/* Command to run on other side of pty */
extern int	maxconnect;	/* Maximum connect time (seconds) */
extern char	user[MAXNAMELEN];/* Our name for authenticating ourselves */
extern char	passwd[MAXSECRETLEN];	/* Password for PAP or CHAP */
extern bool	auth_required;	/* Peer is required to authenticate */
extern bool	persist;	/* Reopen link after it goes down */
extern bool	uselogin;	/* Use /etc/passwd for checking PAP */
extern bool	session_mgmt;	/* Do session management (login records) */
extern char	our_name[MAXNAMELEN];/* Our name for authentication purposes */
extern char	remote_name[MAXNAMELEN]; /* Peer's name for authentication */
extern bool	explicit_remote;/* remote_name specified with remotename opt */
extern bool	demand;		/* Do dial-on-demand */
extern char	*ipparam;	/* Extra parameter for ip up/down scripts */
extern bool	cryptpap;	/* Others' PAP passwords are encrypted */
extern int	idle_time_limit;/* Shut down link if idle for this long */
extern int	holdoff;	/* Dead time before restarting */
extern bool	holdoff_specified; /* true if user gave a holdoff value */
extern bool	notty;		/* Stdin/out is not a tty */
extern char	*pty_socket;	/* Socket to connect to pty */
extern char	*record_file;	/* File to record chars sent/received */
extern bool	sync_serial;	/* Device is synchronous serial device */
extern int	maxfail;	/* Max # of unsuccessful connection attempts */
extern char	linkname[MAXPATHLEN]; /* logical name for link */
extern bool	tune_kernel;	/* May alter kernel settings as necessary */
extern int	connect_delay;	/* Time to delay after connect script */
extern int	max_data_rate;	/* max bytes/sec through charshunt */
extern int	req_unit;	/* interface unit number to use */
extern char	path_ipup[MAXPATHLEN]; /* pathname of ip-up script */
extern char	path_ipdown[MAXPATHLEN]; /* pathname of ip-down script */
extern char	req_ifname[MAXIFNAMELEN]; /* interface name to use */
extern bool	multilink;	/* enable multilink operation */
extern bool	noendpoint;	/* don't send or accept endpt. discrim. */
extern char	*bundle_name;	/* bundle name for multilink */
extern bool	dump_options;	/* print out option values */
extern bool	dryrun;		/* check everything, print options, exit */
extern int	child_wait;	/* # seconds to wait for children at end */

#ifdef USE_EAPTLS
extern char	*crl_dir;
extern char	*crl_file;
extern char *max_tls_version;
#endif /* USE_EAPTLS */

#ifdef MAXOCTETS
extern unsigned int maxoctets;	     /* Maximum octetes per session (in bytes) */
extern int       maxoctets_dir;      /* Direction :
				      0 - in+out (default)
				      1 - in 
				      2 - out
				      3 - max(in,out) */
extern int       maxoctets_timeout;  /* Timeout for check of octets limit */
#define PPP_OCTETS_DIRECTION_SUM        0
#define PPP_OCTETS_DIRECTION_IN         1
#define PPP_OCTETS_DIRECTION_OUT        2
#define PPP_OCTETS_DIRECTION_MAXOVERAL  3
/* same as previos, but little different on RADIUS side */
#define PPP_OCTETS_DIRECTION_MAXSESSION 4	
#endif

#ifdef PPP_FILTER
/* Filter for packets to pass */
extern struct   bpf_program pass_filter_in;
extern struct   bpf_program pass_filter_out;

/* Filter for link-active packets */
extern struct   bpf_program active_filter_in;
extern struct   bpf_program active_filter_out;
#endif

#ifdef MSLANMAN
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

extern char *current_option;	/* the name of the option being parsed */
extern int  privileged_option;	/* set iff the current option came from root */
extern char *option_source;	/* string saying where the option came from */
extern int  option_priority;	/* priority of current options */

/*
 * Values for phase.
 */
#define PHASE_DEAD		0
#define PHASE_INITIALIZE	1
#define PHASE_SERIALCONN	2
#define PHASE_DORMANT		3
#define PHASE_ESTABLISH		4
#define PHASE_AUTHENTICATE	5
#define PHASE_CALLBACK		6
#define PHASE_NETWORK		7
#define PHASE_RUNNING		8
#define PHASE_TERMINATE		9
#define PHASE_DISCONNECT	10
#define PHASE_HOLDOFF		11
#define PHASE_MASTER		12

/*
 * The following struct gives the addresses of procedures to call
 * for a particular protocol.
 */
struct protent {
    u_short protocol;		/* PPP protocol number */
    /* Initialization procedure */
    void (*init)(int unit);
    /* Process a received packet */
    void (*input)(int unit, u_char *pkt, int len);
    /* Process a received protocol-reject */
    void (*protrej)(int unit);
    /* Lower layer has come up */
    void (*lowerup)(int unit);
    /* Lower layer has gone down */
    void (*lowerdown)(int unit);
    /* Open the protocol */
    void (*open)(int unit);
    /* Close the protocol */
    void (*close)(int unit, char *reason);
    /* Print a packet in readable form */
    int  (*printpkt)(u_char *pkt, int len, printer_func printer, void *arg);
    /* Process a received data packet */
    void (*datainput)(int unit, u_char *pkt, int len);
    bool enabled_flag;		/* 0 iff protocol is disabled */
    char *name;			/* Text name of protocol */
    char *data_name;		/* Text name of corresponding data protocol */
    option_t *options;		/* List of command-line options */
    /* Check requested options, assign defaults */
    void (*check_options)(void);
    /* Configure interface for demand-dial */
    int  (*demand_conf)(int unit);
    /* Say whether to bring up link for this pkt */
    int  (*active_pkt)(u_char *pkt, int len);
};

/* Table of pointers to supported protocols */
extern struct protent *protocols[];

/*
 * This struct contains pointers to a set of procedures for
 * doing operations on a "channel".  A channel provides a way
 * to send and receive PPP packets - the canonical example is
 * a serial port device in PPP line discipline (or equivalently
 * with PPP STREAMS modules pushed onto it).
 */
struct channel {
	/* set of options for this channel */
	option_t *options;
	/* find and process a per-channel options file */
	void (*process_extra_options)(void);
	/* check all the options that have been given */
	void (*check_options)(void);
	/* get the channel ready to do PPP, return a file descriptor */
	int  (*connect)(void);
	/* we're finished with the channel */
	void (*disconnect)(void);
	/* put the channel into PPP `mode' */
	int  (*establish_ppp)(int);
	/* take the channel out of PPP `mode', restore loopback if demand */
	void (*disestablish_ppp)(int);
	/* set the transmit-side PPP parameters of the channel */
	void (*send_config)(int, u_int32_t, int, int);
	/* set the receive-side PPP parameters of the channel */
	void (*recv_config)(int, u_int32_t, int, int);
	/* cleanup on error or normal exit */
	void (*cleanup)(void);
	/* close the device, called in children after fork */
	void (*close)(void);
};

extern struct channel *the_channel;

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
void novm(char *);	/* Say we ran out of memory, and die */
void timeout(void (*func)(void *), void *arg, int s, int us);
				/* Call func(arg) after s.us seconds */
void untimeout(void (*func)(void *), void *arg);
				/* Cancel call to func(arg) */
void record_child(int, char *, void (*) (void *), void *, int);
pid_t safe_fork(int, int, int);	/* Fork & close stuff in child */
int  device_script(char *cmd, int in, int out, int dont_wait);
				/* Run `cmd' with given stdin and stdout */
pid_t run_program(char *prog, char **args, int must_exist,
		  void (*done)(void *), void *arg, int wait);
				/* Run program prog with args in child */
void reopen_log(void);	/* (re)open the connection to syslog */
void print_link_stats(void); /* Print stats, if available */
void reset_link_stats(int); /* Reset (init) stats when link goes up */
void update_link_stats(int); /* Get stats at link termination */
void script_setenv(char *, char *, int);	/* set script env var */
void script_unsetenv(char *);		/* unset script env var */
void new_phase(int);	/* signal start of new phase */
void add_notifier(struct notifier **, notify_func, void *);
void remove_notifier(struct notifier **, notify_func, void *);
void notify(struct notifier *, int);
int  ppp_send_config(int, int, u_int32_t, int, int);
int  ppp_recv_config(int, int, u_int32_t, int, int);
const char *protocol_name(int);
void remove_pidfiles(void);
void lock_db(void);
void unlock_db(void);

/* Procedures exported from tty.c. */
void tty_init(void);

/* Procedures exported from utils.c. */
void log_packet(u_char *, int, char *, int);
				/* Format a packet and log it with syslog */
void print_string(char *, int,  printer_func, void *);
				/* Format a string for output */
int slprintf(char *, int, char *, ...);		/* sprintf++ */
int vslprintf(char *, int, char *, va_list);	/* vsprintf++ */
size_t strlcpy(char *, const char *, size_t);	/* safe strcpy */
size_t strlcat(char *, const char *, size_t);	/* safe strncpy */
void dbglog(char *, ...);	/* log a debug message */
void info(char *, ...);	/* log an informational message */
void notice(char *, ...);	/* log a notice-level message */
void warn(char *, ...);	/* log a warning message */
void error(char *, ...);	/* log an error message */
void fatal(char *, ...);	/* log an error message and die(1) */
void init_pr_log(const char *, int); /* initialize for using pr_log */
void pr_log(void *, char *, ...);	/* printer fn, output to syslog */
void end_pr_log(void);	/* finish up after using pr_log */
void dump_packet(const char *, u_char *, int);
				/* dump packet to debug log if interesting */
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
int  bad_ip_adrs(u_int32_t);
				/* check if IP address is unreasonable */

/* Procedures exported from demand.c */
void demand_conf(void);	/* config interface(s) for demand-dial */
void demand_block(void);	/* set all NPs to queue up packets */
void demand_unblock(void); /* set all NPs to pass packets */
void demand_discard(void); /* set all NPs to discard packets */
void demand_rexmit(int);	/* retransmit saved frames for an NP */
int  loop_chars(unsigned char *, int); /* process chars from loopback */
int  loop_frame(unsigned char *, int); /* should we bring link up? */

/* Procedures exported from multilink.c */
#ifdef HAVE_MULTILINK
void mp_check_options(void); /* Check multilink-related options */
int  mp_join_bundle(void);  /* join our link to an appropriate bundle */
void mp_exit_bundle(void);  /* have disconnected our link from bundle */
void mp_bundle_terminated(void);
char *epdisc_to_str(struct epdisc *); /* string from endpoint discrim. */
int  str_to_epdisc(struct epdisc *, char *); /* endpt disc. from str */
#else
#define mp_bundle_terminated()	/* nothing */
#define mp_exit_bundle()	/* nothing */
#define doing_multilink		0
#define multilink_master	0
#endif

/* Procedures exported from sys-*.c */
void sys_init(void);	/* Do system-dependent initialization */
void sys_cleanup(void);	/* Restore system state before exiting */
int  sys_check_options(void); /* Check options specified */
void sys_close(void);	/* Clean up in a child before execing */
int  ppp_available(void);	/* Test whether ppp kernel support exists */
int  get_pty(int *, int *, char *, int);	/* Get pty master/slave */
int  open_ppp_loopback(void); /* Open loopback for demand-dialling */
int  tty_establish_ppp(int);  /* Turn serial port into a ppp interface */
void tty_disestablish_ppp(int); /* Restore port to normal operation */
void generic_disestablish_ppp(int dev_fd); /* Restore device setting */
int  generic_establish_ppp(int dev_fd); /* Make a ppp interface */
void make_new_bundle(int, int, int, int); /* Create new bundle */
int  bundle_attach(int);	/* Attach link to existing bundle */
void cfg_bundle(int, int, int, int); /* Configure existing bundle */
void destroy_bundle(void); /* Tell driver to destroy bundle */
void clean_check(void);	/* Check if line was 8-bit clean */
void set_up_tty(int, int); /* Set up port's speed, parameters, etc. */
void restore_tty(int);	/* Restore port's original parameters */
void setdtr(int, int);	/* Raise or lower port's DTR line */
void output(int, u_char *, int); /* Output a PPP packet */
void wait_input(struct timeval *);
				/* Wait for input, with timeout */
void add_fd(int);		/* Add fd to set to wait for */
void remove_fd(int);	/* Remove fd from set to wait for */
int  read_packet(u_char *); /* Read PPP packet */
int  get_loop_output(void); /* Read pkts from loopback */
void tty_send_config(int, u_int32_t, int, int);
				/* Configure i/f transmit parameters */
void tty_set_xaccm(ext_accm);
				/* Set extended transmit ACCM */
void tty_recv_config(int, u_int32_t, int, int);
				/* Configure i/f receive parameters */
int  ccp_test(int, u_char *, int, int);
				/* Test support for compression scheme */
void ccp_flags_set(int, int, int);
				/* Set kernel CCP state */
int  ccp_fatal_error(int); /* Test for fatal decomp error in kernel */
int  get_idle_time(int, struct ppp_idle *);
				/* Find out how long link has been idle */
int  get_ppp_stats(int, struct pppd_stats *);
				/* Return link statistics */
void netif_set_mtu(int, int); /* Set PPP interface MTU */
int  netif_get_mtu(int);      /* Get PPP interface MTU */
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
#ifdef INET6
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
#ifdef INET6
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
int  lock(char *);	/* Create lock file for device */
int  relock(int);		/* Rewrite lock file with new pid */
void unlock(void);	/* Delete previously-created lock file */
void logwtmp(const char *, const char *, const char *);
				/* Write entry to wtmp file */
int  get_host_seed(void);	/* Get host-dependent random number seed */
int  have_route_to(u_int32_t); /* Check if route to addr exists */
#ifdef PPP_FILTER
int  set_filters(struct bpf_program *passin, struct bpf_program *passout,
    struct bpf_program *activein, struct bpf_program *activeout);
				/* Set filter programs in kernel */
#endif
#ifdef IPX_CHANGE
int  sipxfaddr(int, unsigned long, unsigned char *);
int  cipxfaddr(int);
#endif
int  get_if_hwaddr(u_char *addr, char *name);
int  get_first_ether_hwaddr(u_char *addr);
int get_time(struct timeval *);
				/* Get current time, monotonic if possible. */

/* Procedures exported from options.c */
int setipaddr(char *, char **, int); /* Set local/remote ip addresses */
int  parse_args(int argc, char **argv);
				/* Parse options from arguments given */
int  options_from_file(char *filename, int must_exist, int check_prot,
		       int privileged);
				/* Parse options from an options file */
int  options_from_user(void); /* Parse options from user's .ppprc */
int  options_for_tty(void); /* Parse options from /etc/ppp/options.tty */
int  options_from_list(struct wordlist *, int privileged);
				/* Parse options from a wordlist */
int  getword(FILE *f, char *word, int *newlinep, char *filename);
				/* Read a word from a file */
void option_error(char *fmt, ...);
				/* Print an error message about an option */
int int_option(char *, int *);
				/* Simplified number_option for decimal ints */
void add_options(option_t *); /* Add extra options */
void check_options(void);	/* check values after all options parsed */
int  override_value(char *, int, const char *);
				/* override value if permitted by priority */
void print_options(printer_func, void *);
				/* print out values of all options */

int parse_dotted_ip(char *, u_int32_t *);

/*
 * Hooks to enable plugins to change various things.
 */
extern int (*new_phase_hook)(int);
extern int (*idle_time_hook)(struct ppp_idle *);
extern int (*holdoff_hook)(void);
extern int (*pap_check_hook)(void);
extern int (*pap_auth_hook)(char *user, char *passwd, char **msgp,
			    struct wordlist **paddrs,
			    struct wordlist **popts);
extern void (*pap_logout_hook)(void);
extern int (*pap_passwd_hook)(char *user, char *passwd);
extern int (*allowed_address_hook)(u_int32_t addr);
extern void (*ip_up_hook)(void);
extern void (*ip_down_hook)(void);
extern void (*ip_choose_hook)(u_int32_t *);
extern void (*ipv6_up_hook)(void);
extern void (*ipv6_down_hook)(void);

extern int (*chap_check_hook)(void);
extern int (*chap_passwd_hook)(char *user, char *passwd);
extern void (*multilink_join_hook)(void);

#ifdef USE_EAPTLS
extern int (*eaptls_passwd_hook)(char *user, char *passwd);
#endif

/* Let a plugin snoop sent and received packets.  Useful for L2TP */
extern void (*snoop_recv_hook)(unsigned char *p, int len);
extern void (*snoop_send_hook)(unsigned char *p, int len);

/*
 * Inline versions of get/put char/short/long.
 * Pointer is advanced; we assume that both arguments
 * are lvalues and will already be in registers.
 * cp MUST be u_char *.
 */
#define GETCHAR(c, cp) { \
	(c) = *(cp)++; \
}
#define PUTCHAR(c, cp) { \
	*(cp)++ = (u_char) (c); \
}


#define GETSHORT(s, cp) { \
	(s) = *(cp)++ << 8; \
	(s) |= *(cp)++; \
}
#define PUTSHORT(s, cp) { \
	*(cp)++ = (u_char) ((s) >> 8); \
	*(cp)++ = (u_char) (s); \
}

#define GETLONG(l, cp) { \
	(l) = *(cp)++ << 8; \
	(l) |= *(cp)++; (l) <<= 8; \
	(l) |= *(cp)++; (l) <<= 8; \
	(l) |= *(cp)++; \
}
#define PUTLONG(l, cp) { \
	*(cp)++ = (u_char) ((l) >> 24); \
	*(cp)++ = (u_char) ((l) >> 16); \
	*(cp)++ = (u_char) ((l) >> 8); \
	*(cp)++ = (u_char) (l); \
}

#define INCPTR(n, cp)	((cp) += (n))
#define DECPTR(n, cp)	((cp) -= (n))

/*
 * System dependent definitions for user-level 4.3BSD UNIX implementation.
 */

#define TIMEOUT(r, f, t)	timeout((r), (f), (t), 0)
#define UNTIMEOUT(r, f)		untimeout((r), (f))

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
 * Exit status values.
 */
#define EXIT_OK			0
#define EXIT_FATAL_ERROR	1
#define EXIT_OPTION_ERROR	2
#define EXIT_NOT_ROOT		3
#define EXIT_NO_KERNEL_SUPPORT	4
#define EXIT_USER_REQUEST	5
#define EXIT_LOCK_FAILED	6
#define EXIT_OPEN_FAILED	7
#define EXIT_CONNECT_FAILED	8
#define EXIT_PTYCMD_FAILED	9
#define EXIT_NEGOTIATION_FAILED	10
#define EXIT_PEER_AUTH_FAILED	11
#define EXIT_IDLE_TIMEOUT	12
#define EXIT_CONNECT_TIME	13
#define EXIT_CALLBACK		14
#define EXIT_PEER_DEAD		15
#define EXIT_HANGUP		16
#define EXIT_LOOPBACK		17
#define EXIT_INIT_FAILED	18
#define EXIT_AUTH_TOPEER_FAILED	19
#ifdef MAXOCTETS
#define EXIT_TRAFFIC_LIMIT	20
#endif
#define EXIT_CNID_AUTH_FAILED	21

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

#ifdef DEBUGIPXCP
#define IPXCPDEBUG(x)	if (debug) dbglog x
#else
#define IPXCPDEBUG(x)
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

#endif /* __PPP_H__ */
