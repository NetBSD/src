/*	$NetBSD: ntpd.h,v 1.2.6.1 2012/04/17 00:03:44 yamt Exp $	*/

/*
 * ntpd.h - Prototypes for ntpd.
 */

#include "ntp.h"
#include "ntp_debug.h"
#include "ntp_syslog.h"
#include "ntp_select.h"
#include "ntp_malloc.h"
#include "ntp_refclock.h"
#include "recvbuff.h"

/* ntp_config.c */

#define	TAI_1972	10	/* initial TAI offset (s) */
extern	char	*keysdir;	/* crypto keys and leaptable directory */
extern	char *	saveconfigdir;	/* ntpq saveconfig output directory */

extern	void	getconfig	(int, char **);
extern	void	ctl_clr_stats	(void);
extern	int	ctlclrtrap	(sockaddr_u *, struct interface *, int);
extern	u_short ctlpeerstatus	(struct peer *);
extern	int	ctlsettrap	(sockaddr_u *, struct interface *, int, int);
extern	u_short ctlsysstatus	(void);
extern	void	init_control	(void);
extern	void	init_logging	(char const *, int);
extern	void	setup_logfile	(void);
extern	void	process_control (struct recvbuf *, int);
extern	void	report_event	(int, struct peer *, const char *);

/* ntp_control.c */
/*
 * Structure for translation tables between internal system
 * variable indices and text format.
 */
struct ctl_var {
	u_short code;
	u_short flags;
	const char *text;
};
/*
 * Flag values
 */
#define	CAN_READ	0x01
#define	CAN_WRITE	0x02

#define DEF		0x20
#define	PADDING		0x40
#define	EOV		0x80

#define	RO	(CAN_READ)
#define	WO	(CAN_WRITE)
#define	RW	(CAN_READ|CAN_WRITE)

extern  char *  add_var (struct ctl_var **, u_long, u_short);
extern  void    free_varlist (struct ctl_var *);
extern  void    set_var (struct ctl_var **, const char *, u_long, u_short);
extern  void    set_sys_var (const char *, u_long, u_short);

/* ntp_intres.c */
extern	void	ntp_res_name	(sockaddr_u, u_short);
extern	void	ntp_res_recv	(void);
extern	void	ntp_intres	(void);
#ifdef SYS_WINNT
extern	unsigned WINAPI	ntp_intres_thread	(void *);
#endif

/* ntp_io.c */
typedef struct interface_info {
	endpt *	ep;
	u_char	action;
} interface_info_t;

typedef void	(*interface_receiver_t)	(void *, interface_info_t *);

extern  int	disable_dynamic_updates;

extern	void	interface_enumerate	(interface_receiver_t, void *);
extern	endpt *	findinterface		(sockaddr_u *);
extern	endpt *	findbcastinter		(sockaddr_u *);
extern	void	enable_broadcast	(endpt *, sockaddr_u *);
extern	void	enable_multicast_if	(endpt *, sockaddr_u *);
extern	void	interface_update	(interface_receiver_t, void *);

extern	void	init_io 	(void);
extern	void	io_open_sockets	(void);
extern	void	input_handler	(l_fp *);
extern	void	io_clr_stats	(void);
extern	void	io_setbclient	(void);
extern	void	io_unsetbclient	(void);
extern	void	io_multicast_add(sockaddr_u *);
extern	void	io_multicast_del(sockaddr_u *);
extern	void	sendpkt 	(sockaddr_u *, struct interface *, int, struct pkt *, int);
#ifndef SYS_WINNT
extern	void	kill_asyncio	(int);
#endif
#ifdef DEBUG
extern	void	collect_timing  (struct recvbuf *, const char *, int, l_fp *);
#endif
#ifdef HAVE_SIGNALED_IO
extern	void	wait_for_signal		(void);
extern	void	unblock_io_and_alarm	(void);
extern	void	block_io_and_alarm	(void);
#define UNBLOCK_IO_AND_ALARM()		unblock_io_and_alarm()
#define BLOCK_IO_AND_ALARM()		block_io_and_alarm()
#else
#define UNBLOCK_IO_AND_ALARM()
#define BLOCK_IO_AND_ALARM()
#endif
#define		latoa(pif)	localaddrtoa(pif)
extern const char * localaddrtoa(endpt *);

/* ntp_loopfilter.c */
extern	void	init_loopfilter(void);
extern	int 	local_clock(struct peer *, double);
extern	void	adj_host_clock(void);
extern	void	loop_config(int, double);
extern	void	huffpuff(void);
extern	u_long	sys_clocktime;
extern	u_int	sys_tai;

/* ntp_monitor.c */
extern	void	init_mon	(void);
extern	void	mon_start	(int);
extern	void	mon_stop	(int);
extern	int	ntp_monitor	(struct recvbuf *, int);
extern  void    ntp_monclearinterface (struct interface *interface);

/* ntp_peer.c */
extern	void	init_peer	(void);
extern	struct peer *findexistingpeer (sockaddr_u *, struct peer *, int, u_char);
extern	struct peer *findpeer	(struct recvbuf *, int, int *);
extern	struct peer *findpeerbyassoc (u_int);
extern  void	set_peerdstadr	(struct peer *peer, struct interface *interface);
extern	struct peer *newpeer	(sockaddr_u *, struct interface *, int, int, int, int, u_int, u_char, int, keyid_t);
extern	void	peer_all_reset	(void);
extern	void	peer_clr_stats	(void);
extern	struct peer *peer_config (sockaddr_u *, struct interface *, int, int, int, int, u_int, int, keyid_t, const u_char *);
extern	void	peer_reset	(struct peer *);
extern	void	refresh_all_peerinterfaces (void);
extern	void	unpeer		(struct peer *);
extern	void	clear_all	(void);
extern	int	score_all	(struct peer *);
extern	struct	peer *findmanycastpeer	(struct recvbuf *);

/* ntp_crypto.c */
#ifdef OPENSSL
extern	int	crypto_recv	(struct peer *, struct recvbuf *);
extern	int	crypto_xmit	(struct peer *, struct pkt *,
				    struct recvbuf *, int,
				    struct exten *, keyid_t);
extern	keyid_t	session_key	(sockaddr_u *, sockaddr_u *, keyid_t,
				    keyid_t, u_long);
extern	int	make_keylist	(struct peer *, struct interface *);
extern	void	key_expire	(struct peer *);
extern	void	crypto_update	(void);
extern	void	crypto_config	(int, char *);
extern	void	crypto_setup	(void);
extern	u_int	crypto_ident	(struct peer *);
extern	struct exten *crypto_args (struct peer *, u_int, associd_t, char *);
extern	int	crypto_public	(struct peer *, u_char *, u_int);
extern	void	value_free	(struct value *);
extern	char	*iffpar_file;
extern	EVP_PKEY *iffpar_pkey;
extern	char	*gqpar_file;
extern	EVP_PKEY *gqpar_pkey;
extern	char	*mvpar_file;
extern	EVP_PKEY *mvpar_pkey;
extern struct value tai_leap;
#endif /* OPENSSL */

/* ntp_proto.c */
extern	void	transmit	(struct peer *);
extern	void	receive 	(struct recvbuf *);
extern	void	peer_clear	(struct peer *, const char *);
extern	void 	process_packet	(struct peer *, struct pkt *, u_int);
extern	void	clock_select	(void);

extern	int	leap_tai;	/* TAI at next leap */
extern	u_long	leap_sec;	/* next scheduled leap from file */
extern	u_long	leap_peers;	/* next scheduled leap from peers */
extern	u_long	leapsec;	/* seconds to next leap */
extern	u_long	leap_expire;	/* leap information expiration */
extern	int	sys_orphan;
extern	double	sys_mindisp;
extern	double	sys_maxdist;

/*
 * there seems to be a bug in the IRIX 4 compiler which prevents
 * u_char from beeing used in prototyped functions.
 * This is also true AIX compiler.
 * So give up and define it to be int. WLJ
 */
extern	void	poll_update (struct peer *, int);

extern	void	clear		(struct peer *);
extern	void	clock_filter	(struct peer *, double, double, double);
extern	void	init_proto	(void);
extern	void	proto_config	(int, u_long, double, sockaddr_u *);
extern	void	proto_clr_stats (void);

#ifdef	REFCLOCK
/* ntp_refclock.c */
extern	int	refclock_newpeer (struct peer *);
extern	void	refclock_unpeer (struct peer *);
extern	void	refclock_receive (struct peer *);
extern	void	refclock_transmit (struct peer *);
extern	void	init_refclock	(void);
#endif	/* REFCLOCK */

/* ntp_request.c */
extern	void	init_request	(void);
extern	void	process_private (struct recvbuf *, int);

/* ntp_restrict.c */
extern	void	init_restrict	(void);
extern	u_short	restrictions	(sockaddr_u *);
extern	void	hack_restrict	(int, sockaddr_u *, sockaddr_u *, u_short, u_short);

/* ntp_timer.c */
extern	void	init_timer	(void);
extern	void	reinit_timer	(void);
extern	void	timer		(void);
extern	void	timer_clr_stats (void);
extern  void    timer_interfacetimeout (u_long);
extern  volatile int interface_interval;
#ifdef OPENSSL
extern	char	*sys_hostname;	/* host name */
extern	char	*sys_groupname;	/* group name */
extern	char	*group_name;	/* group name */
extern u_long	sys_revoke;	/* keys revoke timeout */
extern u_long	sys_automax;	/* session key timeout */
#endif /* OPENSSL */

/* ntp_util.c */
extern	void	init_util	(void);
extern	void	write_stats	(void);
extern	void	stats_config	(int, const char *);
extern	void	record_peer_stats (sockaddr_u *, int, double, double, double, double);
extern	void	record_proto_stats (char *);
extern	void	record_loop_stats (double, double, double, double, int);
extern	void	record_clock_stats (sockaddr_u *, const char *);
extern	void	record_raw_stats (sockaddr_u *, sockaddr_u *, l_fp *, l_fp *, l_fp *, l_fp *);
extern	u_long	leap_month(u_long);
extern	void	record_crypto_stats (sockaddr_u *, const char *);
#ifdef DEBUG
extern	void	record_timing_stats (const char *);
#endif
extern  u_short	sock_hash (sockaddr_u *);
extern	char *	fstostr(time_t);	/* NTP timescale seconds */
extern	double	old_drift;
extern	int	drift_file_sw;
extern	double	wander_threshold;
extern	double	wander_resid;

/*
 * Variable declarations for ntpd.
 */
/* ntp_config.c */
extern char const *	progname;
extern char	*sys_phone[];		/* ACTS phone numbers */
#if defined(HAVE_SCHED_SETSCHEDULER)
extern int	config_priority_override;
extern int	config_priority;
#endif
extern char *ntp_signd_socket;
extern struct config_tree *cfg_tree_history;

#ifdef BC_LIST_FRAMEWORK_NOT_YET_USED
/*
 * backwards compatibility flags
 */
typedef struct bc_entry_tag {
	int	token;
	int	enabled;
} bc_entry;

extern bc_entry bc_list[];
#endif

/* ntp_control.c */
extern int	num_ctl_traps;
extern keyid_t	ctl_auth_keyid;		/* keyid used for authenticating write requests */

/*
 * Statistic counters to keep track of requests and responses.
 */
extern u_long	ctltimereset;		/* time stats reset */
extern u_long	numctlreq;		/* number of requests we've received */
extern u_long	numctlbadpkts;		/* number of bad control packets */
extern u_long	numctlresponses; 	/* number of resp packets sent with data */
extern u_long	numctlfrags; 		/* number of fragments sent */
extern u_long	numctlerrors;		/* number of error responses sent */
extern u_long	numctltooshort;		/* number of too short input packets */
extern u_long	numctlinputresp; 	/* number of responses on input */
extern u_long	numctlinputfrag; 	/* number of fragments on input */
extern u_long	numctlinputerr;		/* number of input pkts with err bit set */
extern u_long	numctlbadoffset; 	/* number of input pkts with nonzero offset */
extern u_long	numctlbadversion;	/* number of input pkts with unknown version */
extern u_long	numctldatatooshort;	/* data too short for count */
extern u_long	numctlbadop; 		/* bad op code found in packet */
extern u_long	numasyncmsgs;		/* number of async messages we've sent */

/* ntp_intres.c */
extern keyid_t	req_keyid;		/* request keyid */
extern int	req_keytype;		/* OpenSSL NID such as NID_md5 */
extern size_t	req_hashlen;		/* digest size for req_keytype */
extern char *	req_file;		/* name of the file with configuration info */
#ifdef SYS_WINNT
extern HANDLE ResolverEventHandle;
#else
extern int resolver_pipe_fd[2];  /* used to let the resolver process alert the parent process */
#endif /* SYS_WINNT */

/*
 * Other statistics of possible interest
 */
extern volatile u_long packets_dropped;	/* total number of packets dropped on reception */
extern volatile u_long packets_ignored;	/* packets received on wild card interface */
extern volatile u_long packets_received;/* total number of packets received */
extern u_long	packets_sent;		/* total number of packets sent */
extern u_long	packets_notsent; 	/* total number of packets which couldn't be sent */

extern volatile u_long handler_calls;	/* number of calls to interrupt handler */
extern volatile u_long handler_pkts;	/* number of pkts received by handler */
extern u_long	io_timereset;		/* time counters were reset */

/*
 * Interface stuff
 */
extern endpt *	any_interface;		/* IPv4 wildcard */
extern endpt *	any6_interface;		/* IPv6 wildcard */
extern endpt *	loopback_interface;	/* IPv4 loopback for refclocks */

/*
 * File descriptor masks etc. for call to select
 */
extern fd_set	activefds;
extern int	maxactivefd;

/* ntp_loopfilter.c */
extern double	drift_comp;		/* clock frequency (s/s) */
extern double	clock_stability;	/* clock stability (s/s) */
extern double	clock_max;		/* max offset before step (s) */
extern double	clock_panic;		/* max offset before panic (s) */
extern double	clock_phi;		/* dispersion rate (s/s) */
extern double	clock_minstep;		/* step timeout (s) */
extern double	clock_codec;		/* codec frequency */
#ifdef KERNEL_PLL
extern int	pll_status;		/* status bits for kernel pll */
#endif /* KERNEL_PLL */

/*
 * Clock state machine control flags
 */
extern int	ntp_enable;		/* clock discipline enabled */
extern int	pll_control;		/* kernel support available */
extern int	kern_enable;		/* kernel support enabled */
extern int	pps_enable;		/* kernel PPS discipline enabled */
extern int	ext_enable;		/* external clock enabled */
extern int	cal_enable;		/* refclock calibrate enable */
extern int	allow_panic;		/* allow panic correction */
extern int	mode_ntpdate;		/* exit on first clock set */
extern int	peer_ntpdate;		/* count of ntpdate peers */

/*
 * Clock state machine variables
 */
extern u_char	sys_poll;		/* system poll interval (log2 s) */
extern int	state;			/* clock discipline state */
extern int	tc_counter;		/* poll-adjust counter */
extern u_long	last_time;		/* time of last clock update (s) */
extern double	last_offset;		/* last clock offset (s) */
extern u_char	allan_xpt;		/* Allan intercept (log2 s) */
extern double	clock_jitter;		/* clock jitter (s) */
extern double	sys_offset;		/* system offset (s) */
extern double	sys_jitter;		/* system jitter (s) */

/* ntp_monitor.c */
extern struct mon_data mon_mru_list;
extern struct mon_data mon_fifo_list;
extern int	mon_enabled;

/* ntp_peer.c */
extern struct peer *peer_hash[];	/* peer hash table */
extern int	peer_hash_count[];	/* count of peers in each bucket */
extern struct peer *assoc_hash[];	/* association ID hash table */
extern int	assoc_hash_count[];
extern int	peer_free_count;

/*
 * Miscellaneous statistic counters which may be queried.
 */
extern u_long	peer_timereset;		/* time stat counters were zeroed */
extern u_long	findpeer_calls;		/* number of calls to findpeer */
extern u_long	assocpeer_calls;	/* number of calls to findpeerbyassoc */
extern u_long	peer_allocations;	/* number of allocations from the free list */
extern u_long	peer_demobilizations;	/* number of structs freed to free list */
extern int	total_peer_structs;	/* number of peer structs in circulation */
extern int	peer_associations;	/* mobilized associations */
extern int	peer_preempt;		/* preemptable associations */
/* ntp_proto.c */
/*
 * System variables are declared here.	See Section 3.2 of the
 * specification.
 */
extern u_char	sys_leap;		/* system leap indicator */
extern u_char	sys_stratum;		/* system stratum */
extern s_char	sys_precision;		/* local clock precision */
extern double	sys_rootdelay;		/* roundtrip delay to primary source */
extern double	sys_rootdisp;		/* dispersion to primary source */
extern u_int32	sys_refid;		/* reference id */
extern l_fp	sys_reftime;		/* last update time */
extern struct peer *sys_peer;		/* current peer */

/*
 * Nonspecified system state variables.
 */
extern int	sys_bclient;		/* we set our time to broadcasts */
extern double	sys_bdelay; 		/* broadcast client default delay */
extern int	sys_authenticate;	/* requre authentication for config */
extern l_fp	sys_authdelay;		/* authentication delay */
extern keyid_t	sys_private;		/* private value for session seed */
extern int	sys_manycastserver;	/* respond to manycast client pkts */
extern int	sys_minclock;		/* minimum survivors */
extern int	sys_minsane;		/* minimum candidates */
extern int	sys_floor;		/* cluster stratum floor */
extern int	sys_ceiling;		/* cluster stratum ceiling */
extern u_char	sys_ttl[MAX_TTL];	/* ttl mapping vector */
extern int	sys_ttlmax;		/* max ttl mapping vector index */

/*
 * Statistics counters
 */
extern u_long	sys_stattime;		/* time since reset */
extern u_long	sys_received;		/* packets received */
extern u_long	sys_processed;		/* packets for this host */
extern u_long	sys_restricted;	 	/* restricted packets */
extern u_long	sys_newversion;		/* current version  */
extern u_long	sys_oldversion;		/* old version */
extern u_long	sys_restricted;		/* access denied */
extern u_long	sys_badlength;		/* bad length or format */
extern u_long	sys_badauth;		/* bad authentication */
extern u_long	sys_declined;		/* declined */
extern u_long	sys_limitrejected;	/* rate exceeded */
extern u_long	sys_kodsent;		/* KoD sent */

/* ntp_refclock.c */
#ifdef REFCLOCK
#ifdef PPS
extern int	fdpps;			/* pps file descriptor */
#endif /* PPS */
#endif

/* ntp_request.c */
extern keyid_t	info_auth_keyid;	/* keyid used to authenticate requests */

/* ntp_restrict.c */
extern restrict_u *	restrictlist4;	/* IPv4 restriction list */
extern restrict_u *	restrictlist6;	/* IPv6 restriction list */
extern int	ntp_minpkt;
extern int	ntp_minpoll;
extern int	mon_age;		/* monitor preempt age */

/* ntp_timer.c */
extern volatile int alarm_flag;		/* alarm flag */
extern volatile u_long alarm_overflow;
extern u_long	current_time;		/* current time (s) */
extern u_long	timer_timereset;
extern u_long	timer_overflows;
extern u_long	timer_xmtcalls;

/* ntp_util.c */
extern int	stats_control;		/* write stats to fileset? */
extern int	stats_write_period;	/* # of seconds between writes. */
extern double	stats_write_tolerance;

/* ntpd.c */
extern volatile int debug;		/* debugging flag */
extern int	nofork;			/* no-fork flag */
extern int 	initializing;		/* initializing flag */
#ifdef HAVE_DROPROOT
extern int droproot;			/* flag: try to drop root privileges after startup */
extern char *user;			/* user to switch to */
extern char *group;			/* group to switch to */
extern const char *chrootdir;		/* directory to chroot to */
#endif

/* refclock_conf.c */
#ifdef REFCLOCK
/* refclock configuration table */
extern struct refclock * const refclock_conf[];
extern u_char	num_refclock_conf;
#endif

/* ntp_signd.c */
#ifdef HAVE_NTP_SIGND
extern void 
send_via_ntp_signd(
	struct recvbuf *rbufp,	/* receive packet pointer */
	int	xmode,
	keyid_t	xkeyid, 
	int flags,
	struct pkt  *xpkt
	);
#endif
