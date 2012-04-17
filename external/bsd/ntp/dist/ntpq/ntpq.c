/*	$NetBSD: ntpq.c,v 1.3.2.1 2012/04/17 00:03:49 yamt Exp $	*/

/*
 * ntpq - query an NTP server using mode 6 commands
 */

#include <stdio.h>

#include <ctype.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/time.h>

#include "ntpq.h"
#include "ntp_unixtime.h"
#include "ntp_calendar.h"
#include "ntp_io.h"
#include "ntp_select.h"
#include "ntp_stdlib.h"
#include "ntp_assert.h"
#include "ntp_lineedit.h"
#include "ntp_debug.h"
#include "isc/net.h"
#include "isc/result.h"
#include <ssl_applink.c>

#include "ntp_libopts.h"
#include "ntpq-opts.h"

#ifdef SYS_WINNT
# include <Mswsock.h>
# include <io.h>
#endif /* SYS_WINNT */

#ifdef SYS_VXWORKS
				/* vxWorks needs mode flag -casey*/
# define open(name, flags)   open(name, flags, 0777)
# define SERVER_PORT_NUM     123
#endif

/* we use COMMAND as an autogen keyword */
#ifdef COMMAND
# undef COMMAND
#endif

/*
 * Because we potentially understand a lot of commands we will run
 * interactive if connected to a terminal.
 */
int interactive = 0;		/* set to 1 when we should prompt */
const char *prompt = "ntpq> ";	/* prompt to ask him about */

/*
 * use old readvars behavior?  --old-rv processing in ntpq resets
 * this value based on the presence or absence of --old-rv.  It is
 * initialized to 1 here to maintain backward compatibility with
 * libntpq clients such as ntpsnmpd, which are free to reset it as
 * desired.
 */
int	old_rv = 1;


/*
 * for get_systime()
 */
s_char	sys_precision;		/* local clock precision (log2 s) */

/*
 * Keyid used for authenticated requests.  Obtained on the fly.
 */
u_long info_auth_keyid = 0;

static	int	info_auth_keytype = NID_md5;	/* MD5 */
static	size_t	info_auth_hashlen = 16;		/* MD5 */
u_long	current_time;		/* needed by authkeys; not used */

/*
 * Flag which indicates we should always send authenticated requests
 */
int always_auth = 0;

/*
 * Flag which indicates raw mode output.
 */
int rawmode = 0;

/*
 * Packet version number we use
 */
u_char pktversion = NTP_OLDVERSION + 1;

/*
 * Don't jump if no set jmp.
 */
volatile int jump = 0;

/*
 * Format values
 */
#define	PADDING	0
#define	TS	1	/* time stamp */
#define	FL	2	/* l_fp type value */
#define	FU	3	/* u_fp type value */
#define	FS	4	/* s_fp type value */
#define	UI	5	/* unsigned integer value */
#define	SI	6	/* signed integer value */
#define	HA	7	/* host address */
#define	NA	8	/* network address */
#define	ST	9	/* string value */
#define	RF	10	/* refid (sometimes string, sometimes not) */
#define	LP	11	/* leap (print in binary) */
#define	OC	12	/* integer, print in octal */
#define	MD	13	/* mode */
#define	AR	14	/* array of times */
#define FX	15	/* test flags */
#define	EOV	255	/* end of table */


/*
 * System variable values.  The array can be indexed by
 * the variable index to find the textual name.
 */
struct ctl_var sys_var[] = {
	{ 0,		PADDING, "" },		/* 0 */
	{ CS_LEAP,	LP,	"leap" },	/* 1 */
	{ CS_STRATUM,	UI,	"stratum" },	/* 2 */
	{ CS_PRECISION,	SI,	"precision" },	/* 3 */
	{ CS_ROOTDELAY,	FS,	"rootdelay" },	/* 4 */
	{ CS_ROOTDISPERSION, FU, "rootdispersion" }, /* 5 */
	{ CS_REFID,	RF,	"refid" },	/* 6 */
	{ CS_REFTIME,	TS,	"reftime" },	/* 7 */
	{ CS_POLL,	UI,	"poll" },	/* 8 */
	{ CS_PEERID,	UI,	"peer" },	/* 9 */
	{ CS_OFFSET,	FL,	"offset" },	/* 10 */
	{ CS_DRIFT,	FS,	"frequency" },	/* 11 */
	{ CS_JITTER,	FU,	"jitter" },	/* 12 */
	{ CS_CLOCK,	TS,	"clock" },	/* 13 */
	{ CS_PROCESSOR,	ST,	"processor" },	/* 14 */
	{ CS_SYSTEM,	ST,	"system" },	/* 15 */
	{ CS_VERSION,	ST,	"version" },	/* 16 */
	{ CS_STABIL,	FS,	"stability" },	/* 17 */
	{ CS_VARLIST,	ST,	"sys_var_list" }, /* 18 */
	{ 0,		EOV,	""	}
};


/*
 * Peer variable list
 */
struct ctl_var peer_var[] = {
	{ 0,		PADDING, "" },		/* 0 */
	{ CP_CONFIG,	UI,	"config" },	/* 1 */
	{ CP_AUTHENABLE, UI,	"authenable" },	/* 2 */
	{ CP_AUTHENTIC,	UI,	"authentic" },	/* 3 */
	{ CP_SRCADR,	HA,	"srcadr" },	/* 4 */
	{ CP_SRCPORT,	UI,	"srcport" },	/* 5 */
	{ CP_DSTADR,	NA,	"dstadr" },	/* 6 */
	{ CP_DSTPORT,	UI,	"dstport" },	/* 7 */
	{ CP_LEAP,	LP,	"leap" },	/* 8 */
	{ CP_HMODE,	MD,	"hmode" },	/* 9 */
	{ CP_STRATUM,	UI,	"stratum" },	/* 10 */
	{ CP_PPOLL,	UI,	"ppoll" },	/* 11 */
	{ CP_HPOLL,	UI,	"hpoll" },	/* 12 */
	{ CP_PRECISION,	SI,	"precision" },	/* 13 */
	{ CP_ROOTDELAY,	FS,	"rootdelay" },	/* 14 */
	{ CP_ROOTDISPERSION, FU, "rootdisp" },	/* 15 */
	{ CP_REFID,	RF,	"refid" },	/* 16 */
	{ CP_REFTIME,	TS,	"reftime" },	/* 17 */
	{ CP_ORG,	TS,	"org" },	/* 18 */
	{ CP_REC,	TS,	"rec" },	/* 19 */
	{ CP_XMT,	TS,	"xmt" },	/* 20 */
	{ CP_REACH,	OC,	"reach" },	/* 21 */
	{ CP_UNREACH,	UI,	"unreach" },	/* 22 */
	{ CP_TIMER,	UI,	"timer" },	/* 23 */
	{ CP_DELAY,	FS,	"delay" },	/* 24 */
	{ CP_OFFSET,	FL,	"offset" },	/* 25 */
	{ CP_JITTER,	FU,	"jitter" },	/* 26 */
	{ CP_DISPERSION, FU,	"dispersion" },	/* 27 */
	{ CP_KEYID,	UI,	"keyid" },	/* 28 */
	{ CP_FILTDELAY,	AR,	"filtdelay" },	/* 29 */
	{ CP_FILTOFFSET, AR,	"filtoffset" },	/* 30 */
	{ CP_PMODE,	ST,	"pmode" },	/* 31 */
	{ CP_RECEIVED,	UI,	"received" },	/* 32 */
	{ CP_SENT,	UI,	"sent" },	/* 33 */
	{ CP_FILTERROR,	AR,	"filtdisp" },	/* 34 */
	{ CP_FLASH,     FX,	"flash" },	/* 35 */ 
	{ CP_TTL,	UI,	"ttl" },	/* 36 */
	/*
	 * These are duplicate entries so that we can
	 * process deviant version of the ntp protocol.
	 */
	{ CP_SRCADR,	HA,	"peeraddr" },	/* 4 */
	{ CP_SRCPORT,	UI,	"peerport" },	/* 5 */
	{ CP_PPOLL,	UI,	"peerpoll" },	/* 11 */
	{ CP_HPOLL,	UI,	"hostpoll" },	/* 12 */
	{ CP_FILTERROR,	AR,	"filterror" },	/* 34 */
	{ 0,		EOV,	""	}
};


/*
 * Clock variable list
 */
struct ctl_var clock_var[] = {
	{ 0,		PADDING, "" },		/* 0 */
	{ CC_TYPE,	UI,	"type" },	/* 1 */
	{ CC_TIMECODE,	ST,	"timecode" },	/* 2 */
	{ CC_POLL,	UI,	"poll" },	/* 3 */
	{ CC_NOREPLY,	UI,	"noreply" },	/* 4 */
	{ CC_BADFORMAT,	UI,	"badformat" },	/* 5 */
	{ CC_BADDATA,	UI,	"baddata" },	/* 6 */
	{ CC_FUDGETIME1, FL,	"fudgetime1" },	/* 7 */
	{ CC_FUDGETIME2, FL,	"fudgetime2" },	/* 8 */
	{ CC_FUDGEVAL1,	UI,	"stratum" },	/* 9 */
	{ CC_FUDGEVAL2,	RF,	"refid" },	/* 10 */
	{ CC_FLAGS,	UI,	"flags" },	/* 11 */
	{ CC_DEVICE,	ST,	"device" },	/* 12 */
	{ 0,		EOV,	""	}
};


/*
 * flasher bits
 */
static const char *tstflagnames[] = {
	"pkt_dup",		/* TEST1 */
	"pkt_bogus",		/* TEST2 */
	"pkt_unsync",		/* TEST3 */
	"pkt_denied",		/* TEST4 */
	"pkt_auth",		/* TEST5 */
	"pkt_stratum",		/* TEST6 */
	"pkt_header",		/* TEST7 */
	"pkt_autokey",		/* TEST8 */
	"pkt_crypto",		/* TEST9 */
	"peer_stratum",		/* TEST10 */
	"peer_dist",		/* TEST11 */
	"peer_loop",		/* TEST12 */
	"peer_unreach"		/* TEST13 */
};


int		ntpqmain	(int,	char **);
/*
 * Built in command handler declarations
 */
static	int	openhost	(const char *);

static	int	sendpkt		(void *, size_t);
static	int	getresponse	(int, int, u_short *, int *, const char **, int);
static	int	sendrequest	(int, int, int, int, char *);
static	char *	tstflags	(u_long);
#ifndef BUILD_AS_LIB
static	void	getcmds		(void);
#ifndef SYS_WINNT
static	RETSIGTYPE abortcmd	(int);
#endif	/* SYS_WINNT */
static	void	docmd		(const char *);
static	void	tokenize	(const char *, char **, int *);
static	int	getarg		(char *, int, arg_v *);
#endif	/* BUILD_AS_LIB */
static	int	findcmd		(char *, struct xcmd *, struct xcmd *, struct xcmd **);
static	int	rtdatetolfp	(char *, l_fp *);
static	int	decodearr	(char *, int *, l_fp *);
static	void	help		(struct parse *, FILE *);
static	int	helpsort	(const void *, const void *);
static	void	printusage	(struct xcmd *, FILE *);
static	void	timeout		(struct parse *, FILE *);
static	void	auth_delay	(struct parse *, FILE *);
static	void	host		(struct parse *, FILE *);
static	void	ntp_poll	(struct parse *, FILE *);
static	void	keyid		(struct parse *, FILE *);
static	void	keytype		(struct parse *, FILE *);
static	void	passwd		(struct parse *, FILE *);
static	void	hostnames	(struct parse *, FILE *);
static	void	setdebug	(struct parse *, FILE *);
static	void	quit		(struct parse *, FILE *);
static	void	version		(struct parse *, FILE *);
static	void	raw		(struct parse *, FILE *);
static	void	cooked		(struct parse *, FILE *);
static	void	authenticate	(struct parse *, FILE *);
static	void	ntpversion	(struct parse *, FILE *);
static	void	warning		(const char *, ...)
    __attribute__((__format__(__printf__, 1, 2)));
static	void	error		(const char *, ...)
    __attribute__((__format__(__printf__, 1, 2)));
static	u_long	getkeyid	(const char *);
static	void	atoascii	(const char *, size_t, char *, size_t);
static	void	cookedprint	(int, int, const char *, int, int, FILE *);
static	void	rawprint	(int, int, const char *, int, int, FILE *);
static	void	startoutput	(void);
static	void	output		(FILE *, char *, const char *);
static	void	endoutput	(FILE *);
static	void	outputarr	(FILE *, char *, int, l_fp *);
static	int	assoccmp	(const void *, const void *);
void	ntpq_custom_opt_handler	(tOptions *, tOptDesc *);


/*
 * Built-in commands we understand
 */
struct xcmd builtins[] = {
	{ "?",		help,		{  OPT|NTP_STR, NO, NO, NO },
	  { "command", "", "", "" },
	  "tell the use and syntax of commands" },
	{ "help",	help,		{  OPT|NTP_STR, NO, NO, NO },
	  { "command", "", "", "" },
	  "tell the use and syntax of commands" },
	{ "timeout",	timeout,	{ OPT|NTP_UINT, NO, NO, NO },
	  { "msec", "", "", "" },
	  "set the primary receive time out" },
	{ "delay",	auth_delay,	{ OPT|NTP_INT, NO, NO, NO },
	  { "msec", "", "", "" },
	  "set the delay added to encryption time stamps" },
	{ "host",	host,		{ OPT|NTP_STR, OPT|NTP_STR, NO, NO },
	  { "-4|-6", "hostname", "", "" },
	  "specify the host whose NTP server we talk to" },
	{ "poll",	ntp_poll,	{ OPT|NTP_UINT, OPT|NTP_STR, NO, NO },
	  { "n", "verbose", "", "" },
	  "poll an NTP server in client mode `n' times" },
	{ "passwd",	passwd,		{ NO, NO, NO, NO },
	  { "", "", "", "" },
	  "specify a password to use for authenticated requests"},
	{ "hostnames",	hostnames,	{ OPT|NTP_STR, NO, NO, NO },
	  { "yes|no", "", "", "" },
	  "specify whether hostnames or net numbers are printed"},
	{ "debug",	setdebug,	{ OPT|NTP_STR, NO, NO, NO },
	  { "no|more|less", "", "", "" },
	  "set/change debugging level" },
	{ "quit",	quit,		{ NO, NO, NO, NO },
	  { "", "", "", "" },
	  "exit ntpq" },
	{ "exit",	quit,		{ NO, NO, NO, NO },
	  { "", "", "", "" },
	  "exit ntpq" },
	{ "keyid",	keyid,		{ OPT|NTP_UINT, NO, NO, NO },
	  { "key#", "", "", "" },
	  "set keyid to use for authenticated requests" },
	{ "version",	version,	{ NO, NO, NO, NO },
	  { "", "", "", "" },
	  "print version number" },
	{ "raw",	raw,		{ NO, NO, NO, NO },
	  { "", "", "", "" },
	  "do raw mode variable output" },
	{ "cooked",	cooked,		{ NO, NO, NO, NO },
	  { "", "", "", "" },
	  "do cooked mode variable output" },
	{ "authenticate", authenticate,	{ OPT|NTP_STR, NO, NO, NO },
	  { "yes|no", "", "", "" },
	  "always authenticate requests to this server" },
	{ "ntpversion",	ntpversion,	{ OPT|NTP_UINT, NO, NO, NO },
	  { "version number", "", "", "" },
	  "set the NTP version number to use for requests" },
	{ "keytype",	keytype,	{ OPT|NTP_STR, NO, NO, NO },
	  { "key type (md5|des)", "", "", "" },
	  "set key type to use for authenticated requests (des|md5)" },
	{ 0,		0,		{ NO, NO, NO, NO },
	  { "", "", "", "" }, "" }
};


/*
 * Default values we use.
 */
#define	DEFHOST		"localhost"	/* default host name */
#define	DEFTIMEOUT	(5)		/* 5 second time out */
#define	DEFSTIMEOUT	(2)		/* 2 second time out after first */
#define	DEFDELAY	0x51EB852	/* 20 milliseconds, l_fp fraction */
#define	LENHOSTNAME	256		/* host name is 256 characters long */
#define	MAXCMDS		100		/* maximum commands on cmd line */
#define	MAXHOSTS	200		/* maximum hosts on cmd line */
#define	MAXLINE		512		/* maximum line length */
#define	MAXTOKENS	(1+MAXARGS+2)	/* maximum number of usable tokens */
#define	MAXVARLEN	256		/* maximum length of a variable name */
#define	MAXVALLEN	400		/* maximum length of a variable value */
#define	MAXOUTLINE	72		/* maximum length of an output line */
#define SCREENWIDTH	76		/* nominal screen width in columns */

/*
 * Some variables used and manipulated locally
 */
struct sock_timeval tvout = { DEFTIMEOUT, 0 };	/* time out for reads */
struct sock_timeval tvsout = { DEFSTIMEOUT, 0 };/* secondary time out */
l_fp delay_time;				/* delay time */
char currenthost[LENHOSTNAME];			/* current host name */
int currenthostisnum;				/* is prior text from IP? */
struct sockaddr_in hostaddr;			/* host address */
int showhostnames = 1;				/* show host names by default */

int ai_fam_templ;				/* address family */
int ai_fam_default;				/* default address family */
SOCKET sockfd;					/* fd socket is opened on */
int havehost = 0;				/* set to 1 when host open */
int s_port = 0;
struct servent *server_entry = NULL;		/* server entry for ntp */


/*
 * Sequence number used for requests.  It is incremented before
 * it is used.
 */
u_short sequence;

/*
 * Holds data returned from queries.  Declare buffer long to be sure of
 * alignment.
 */
#define	MAXFRAGS	24		/* maximum number of fragments */
#define	DATASIZE	(MAXFRAGS*480)	/* maximum amount of data */
long pktdata[DATASIZE/sizeof(long)];

/*
 * Holds association data for use with the &n operator.
 */
struct association assoc_cache[MAXASSOC];
int numassoc = 0;		/* number of cached associations */

/*
 * For commands typed on the command line (with the -c option)
 */
int numcmds = 0;
const char *ccmds[MAXCMDS];
#define	ADDCMD(cp)	if (numcmds < MAXCMDS) ccmds[numcmds++] = (cp)

/*
 * When multiple hosts are specified.
 */
int numhosts = 0;
const char *chosts[MAXHOSTS];
#define	ADDHOST(cp)	if (numhosts < MAXHOSTS) chosts[numhosts++] = (cp)

/*
 * Error codes for internal use
 */
#define	ERR_UNSPEC		256
#define	ERR_INCOMPLETE	257
#define	ERR_TIMEOUT		258
#define	ERR_TOOMUCH		259

/*
 * Macro definitions we use
 */
#define	ISSPACE(c)	((c) == ' ' || (c) == '\t')
#define	ISEOL(c)	((c) == '\n' || (c) == '\r' || (c) == '\0')
#define	STREQ(a, b)	(*(a) == *(b) && strcmp((a), (b)) == 0)

/*
 * Jump buffer for longjumping back to the command level
 */
jmp_buf interrupt_buf;

/*
 * Points at file being currently printed into
 */
FILE *current_output;

/*
 * Command table imported from ntpdc_ops.c
 */
extern struct xcmd opcmds[];

char *progname;
volatile int debug;

#ifdef NO_MAIN_ALLOWED
#ifndef BUILD_AS_LIB
CALL(ntpq,"ntpq",ntpqmain);

void clear_globals(void)
{
	extern int ntp_optind;
	showhostnames = 0;	/* don'tshow host names by default */
	ntp_optind = 0;
	server_entry = NULL;	/* server entry for ntp */
	havehost = 0;		/* set to 1 when host open */
	numassoc = 0;		/* number of cached associations */
	numcmds = 0;
	numhosts = 0;
}
#endif /* !BUILD_AS_LIB */
#endif /* NO_MAIN_ALLOWED */

/*
 * main - parse arguments and handle options
 */
#ifndef NO_MAIN_ALLOWED
int
main(
	int argc,
	char *argv[]
	)
{
	return ntpqmain(argc, argv);
}
#endif

#ifndef BUILD_AS_LIB
int
ntpqmain(
	int argc,
	char *argv[]
	)
{
	extern int ntp_optind;

#ifdef SYS_VXWORKS
	clear_globals();
	taskPrioritySet(taskIdSelf(), 100 );
#endif

	delay_time.l_ui = 0;
	delay_time.l_uf = DEFDELAY;

	init_lib();	/* sets up ipv4_works, ipv6_works */
	ssl_applink();

	/* Check to see if we have IPv6. Otherwise default to IPv4 */
	if (!ipv6_works)
		ai_fam_default = AF_INET;

	progname = argv[0];

	{
		int optct = ntpOptionProcess(&ntpqOptions, argc, argv);
		argc -= optct;
		argv += optct;
	}

	/*
	 * Process options other than -c and -p, which are specially
	 * handled by ntpq_custom_opt_handler().
	 */

	debug = DESC(DEBUG_LEVEL).optOccCt;

	if (HAVE_OPT(IPV4))
		ai_fam_templ = AF_INET;
	else if (HAVE_OPT(IPV6))
		ai_fam_templ = AF_INET6;
	else
		ai_fam_templ = ai_fam_default;

	if (HAVE_OPT(INTERACTIVE))
		interactive = 1;

	if (HAVE_OPT(NUMERIC))
		showhostnames = 0;

	old_rv = HAVE_OPT(OLD_RV);

#if 0
	while ((c = ntp_getopt(argc, argv, "46c:dinp")) != EOF)
	    switch (c) {
		case '4':
		    ai_fam_templ = AF_INET;
		    break;
		case '6':
		    ai_fam_templ = AF_INET6;
		    break;
		case 'c':
		    ADDCMD(ntp_optarg);
		    break;
		case 'd':
		    ++debug;
		    break;
		case 'i':
		    interactive = 1;
		    break;
		case 'n':
		    showhostnames = 0;
		    break;
		case 'p':
		    ADDCMD("peers");
		    break;
		default:
		    errflg++;
		    break;
	    }
	if (errflg) {
		(void) fprintf(stderr,
			       "usage: %s [-46dinp] [-c cmd] host ...\n",
			       progname);
		exit(2);
	}
#endif
	NTP_INSIST(ntp_optind <= argc);
	if (ntp_optind == argc) {
		ADDHOST(DEFHOST);
	} else {
		for (; ntp_optind < argc; ntp_optind++)
			ADDHOST(argv[ntp_optind]);
	}

	if (numcmds == 0 && interactive == 0
	    && isatty(fileno(stdin)) && isatty(fileno(stderr))) {
		interactive = 1;
	}

#ifndef SYS_WINNT /* Under NT cannot handle SIGINT, WIN32 spawns a handler */
	if (interactive)
	    (void) signal_no_reset(SIGINT, abortcmd);
#endif /* SYS_WINNT */

	if (numcmds == 0) {
		(void) openhost(chosts[0]);
		getcmds();
	} else {
		int ihost;
		int icmd;

		for (ihost = 0; ihost < numhosts; ihost++) {
			if (openhost(chosts[ihost]))
				for (icmd = 0; icmd < numcmds; icmd++)
					docmd(ccmds[icmd]);
		}
	}
#ifdef SYS_WINNT
	WSACleanup();
#endif /* SYS_WINNT */
	return 0;
}
#endif /* !BUILD_AS_LIB */

/*
 * openhost - open a socket to a host
 */
static	int
openhost(
	const char *hname
	)
{
	char temphost[LENHOSTNAME];
	int a_info, i;
	struct addrinfo hints, *ai = NULL;
	register const char *cp;
	char name[LENHOSTNAME];
	char service[5];

	/*
	 * We need to get by the [] if they were entered
	 */
	
	cp = hname;
	
	if (*cp == '[') {
		cp++;
		for (i = 0; *cp && *cp != ']'; cp++, i++)
			name[i] = *cp;
		if (*cp == ']') {
			name[i] = '\0';
			hname = name;
		} else {
			return 0;
		}
	}

	/*
	 * First try to resolve it as an ip address and if that fails,
	 * do a fullblown (dns) lookup. That way we only use the dns
	 * when it is needed and work around some implementations that
	 * will return an "IPv4-mapped IPv6 address" address if you
	 * give it an IPv4 address to lookup.
	 */
	strcpy(service, "ntp");
	ZERO(hints);
	hints.ai_family = ai_fam_templ;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = Z_AI_NUMERICHOST;

	a_info = getaddrinfo(hname, service, &hints, &ai);
	if (a_info == EAI_NONAME
#ifdef EAI_NODATA
	    || a_info == EAI_NODATA
#endif
	   ) {
		hints.ai_flags = AI_CANONNAME;
#ifdef AI_ADDRCONFIG
		hints.ai_flags |= AI_ADDRCONFIG;
#endif
		a_info = getaddrinfo(hname, service, &hints, &ai);	
	}
#ifdef AI_ADDRCONFIG
	/* Some older implementations don't like AI_ADDRCONFIG. */
	if (a_info == EAI_BADFLAGS) {
		hints.ai_flags = AI_CANONNAME;
		a_info = getaddrinfo(hname, service, &hints, &ai);	
	}
#endif
	if (a_info != 0) {
		(void) fprintf(stderr, "%s\n", gai_strerror(a_info));
		return 0;
	}

	if (!showhostnames || ai->ai_canonname == NULL) {
		strncpy(temphost, 
			stoa((sockaddr_u *)ai->ai_addr),
			LENHOSTNAME);
		currenthostisnum = TRUE;
	} else {
		strncpy(temphost, ai->ai_canonname, LENHOSTNAME);
		currenthostisnum = FALSE;
	}
	temphost[LENHOSTNAME-1] = '\0';

	if (debug > 2)
		printf("Opening host %s\n", temphost);

	if (havehost == 1) {
		if (debug > 2)
			printf("Closing old host %s\n", currenthost);
		(void) closesocket(sockfd);
		havehost = 0;
	}
	(void) strcpy(currenthost, temphost);

	/* port maps to the same location in both families */
	s_port = ((struct sockaddr_in6 *)ai->ai_addr)->sin6_port;
#ifdef SYS_VXWORKS
	((struct sockaddr_in6 *)&hostaddr)->sin6_port = htons(SERVER_PORT_NUM);
	if (ai->ai_family == AF_INET)
		*(struct sockaddr_in *)&hostaddr=
			*((struct sockaddr_in *)ai->ai_addr);
	else
		*(struct sockaddr_in6 *)&hostaddr=
			*((struct sockaddr_in6 *)ai->ai_addr);
#endif /* SYS_VXWORKS */

#ifdef SYS_WINNT
	{
		int optionValue = SO_SYNCHRONOUS_NONALERT;
		int err;

		err = setsockopt(INVALID_SOCKET, SOL_SOCKET, SO_OPENTYPE,
				 (char *)&optionValue, sizeof(optionValue));
		if (err) {
			err = WSAGetLastError();
			fprintf(stderr,
				"setsockopt(SO_SYNCHRONOUS_NONALERT) "
				"error: %s\n", strerror(err));
			exit(1);
		}
	}
#endif /* SYS_WINNT */

	sockfd = socket(ai->ai_family, SOCK_DGRAM, 0);
	if (sockfd == INVALID_SOCKET) {
		error("socket");
	}

	
#ifdef NEED_RCVBUF_SLOP
# ifdef SO_RCVBUF
	{ int rbufsize = DATASIZE + 2048;	/* 2K for slop */
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF,
		       &rbufsize, sizeof(int)) == -1)
	    error("setsockopt");
	}
# endif
#endif

#ifdef SYS_VXWORKS
	if (connect(sockfd, (struct sockaddr *)&hostaddr,
		    sizeof(hostaddr)) == -1)
#else
	if (connect(sockfd, (struct sockaddr *)ai->ai_addr,
		    ai->ai_addrlen) == -1)
#endif /* SYS_VXWORKS */
	    error("connect");
	if (a_info == 0)
		freeaddrinfo(ai);
	havehost = 1;
	return 1;
}


/* XXX ELIMINATE sendpkt similar in ntpq.c, ntpdc.c, ntp_io.c, ntptrace.c */
/*
 * sendpkt - send a packet to the remote host
 */
static int
sendpkt(
	void *	xdata,
	size_t	xdatalen
	)
{
	if (debug >= 3)
		printf("Sending %zu octets\n", xdatalen);

	if (send(sockfd, xdata, (size_t)xdatalen, 0) == -1) {
		warning("write to %s failed", currenthost);
		return -1;
	}

	if (debug >= 4) {
		int first = 8;
		char *cdata = xdata;

		printf("Packet data:\n");
		while (xdatalen-- > 0) {
			if (first-- == 0) {
				printf("\n");
				first = 7;
			}
			printf(" %02x", *cdata++ & 0xff);
		}
		printf("\n");
	}
	return 0;
}



/*
 * getresponse - get a (series of) response packet(s) and return the data
 */
static int
getresponse(
	int opcode,
	int associd,
	u_short *rstatus,
	int *rsize,
	const char **rdata,
	int timeo
	)
{
	struct ntp_control rpkt;
	struct sock_timeval tvo;
	u_short offsets[MAXFRAGS+1];
	u_short counts[MAXFRAGS+1];
	u_short offset;
	u_short count;
	size_t numfrags;
	size_t f;
	size_t ff;
	int seenlastfrag;
	int shouldbesize;
	fd_set fds;
	int n;
	int len;
	int first;
	char *data;

	/*
	 * This is pretty tricky.  We may get between 1 and MAXFRAG packets
	 * back in response to the request.  We peel the data out of
	 * each packet and collect it in one long block.  When the last
	 * packet in the sequence is received we'll know how much data we
	 * should have had.  Note we use one long time out, should reconsider.
	 */
	*rsize = 0;
	if (rstatus)
		*rstatus = 0;
	*rdata = (char *)pktdata;

	numfrags = 0;
	seenlastfrag = 0;

	FD_ZERO(&fds);

	/*
	 * Loop until we have an error or a complete response.  Nearly all
	 * code paths to loop again use continue.
	 */
	for (;;) {

		if (numfrags == 0)
			tvo = tvout;
		else
			tvo = tvsout;
		
		FD_SET(sockfd, &fds);
		n = select(sockfd + 1, &fds, NULL, NULL, &tvo);

		if (n == -1) {
			warning("select fails");
			return -1;
		}
		if (n == 0) {
			/*
			 * Timed out.  Return what we have
			 */
			if (numfrags == 0) {
				if (timeo)
					fprintf(stderr,
						"%s: timed out, nothing received\n",
						currenthost);
				return ERR_TIMEOUT;
			}
			if (timeo)
				fprintf(stderr,
					"%s: timed out with incomplete data\n",
					currenthost);
			if (debug) {
				fprintf(stderr,
					"ERR_INCOMPLETE: Received fragments:\n");
				for (f = 0; f < numfrags; f++)
					fprintf(stderr,
						"%2zu: %5d %5d\t%3d octets\n",
						f, offsets[f],
						offsets[f] +
						counts[f],
						counts[f]);
				fprintf(stderr,
					"last fragment %sreceived\n",
					(seenlastfrag)
					    ? ""
					    : "not ");
			}
			return ERR_INCOMPLETE;
		}

		n = recv(sockfd, (char *)&rpkt, sizeof(rpkt), 0);
		if (n == -1) {
			warning("read");
			return -1;
		}

		if (debug >= 4) {
			len = n;
			first = 8;
			data = (char *)&rpkt;

			printf("Packet data:\n");
			while (len-- > 0) {
				if (first-- == 0) {
					printf("\n");
					first = 7;
				}
				printf(" %02x", *data++ & 0xff);
			}
			printf("\n");
		}

		/*
		 * Check for format errors.  Bug proofing.
		 */
		if (n < (int)CTL_HEADER_LEN) {
			if (debug)
				printf("Short (%d byte) packet received\n", n);
			continue;
		}
		if (PKT_VERSION(rpkt.li_vn_mode) > NTP_VERSION
		    || PKT_VERSION(rpkt.li_vn_mode) < NTP_OLDVERSION) {
			if (debug)
				printf("Packet received with version %d\n",
				       PKT_VERSION(rpkt.li_vn_mode));
			continue;
		}
		if (PKT_MODE(rpkt.li_vn_mode) != MODE_CONTROL) {
			if (debug)
				printf("Packet received with mode %d\n",
				       PKT_MODE(rpkt.li_vn_mode));
			continue;
		}
		if (!CTL_ISRESPONSE(rpkt.r_m_e_op)) {
			if (debug)
				printf("Received request packet, wanted response\n");
			continue;
		}

		/*
		 * Check opcode and sequence number for a match.
		 * Could be old data getting to us.
		 */
		if (ntohs(rpkt.sequence) != sequence) {
			if (debug)
				printf("Received sequnce number %d, wanted %d\n",
				       ntohs(rpkt.sequence), sequence);
			continue;
		}
		if (CTL_OP(rpkt.r_m_e_op) != opcode) {
			if (debug)
			    printf(
				    "Received opcode %d, wanted %d (sequence number okay)\n",
				    CTL_OP(rpkt.r_m_e_op), opcode);
			continue;
		}

		/*
		 * Check the error code.  If non-zero, return it.
		 */
		if (CTL_ISERROR(rpkt.r_m_e_op)) {
			int errcode;

			errcode = (ntohs(rpkt.status) >> 8) & 0xff;
			if (debug && CTL_ISMORE(rpkt.r_m_e_op)) {
				printf("Error code %d received on not-final packet\n",
				       errcode);
			}
			if (errcode == CERR_UNSPEC)
			    return ERR_UNSPEC;
			return errcode;
		}

		/*
		 * Check the association ID to make sure it matches what
		 * we sent.
		 */
		if (ntohs(rpkt.associd) != associd) {
			if (debug)
			    printf("Association ID %d doesn't match expected %d\n",
				   ntohs(rpkt.associd), associd);
			/*
			 * Hack for silly fuzzballs which, at the time of writing,
			 * return an assID of sys.peer when queried for system variables.
			 */
#ifdef notdef
			continue;
#endif
		}

		/*
		 * Collect offset and count.  Make sure they make sense.
		 */
		offset = ntohs(rpkt.offset);
		count = ntohs(rpkt.count);

		/*
		 * validate received payload size is padded to next 32-bit
		 * boundary and no smaller than claimed by rpkt.count
		 */
		if (n & 0x3) {
			if (debug)
				printf("Response packet not padded, "
					"size = %d\n", n);
			continue;
		}

		shouldbesize = (CTL_HEADER_LEN + count + 3) & ~3;

		if (n < shouldbesize) {
			printf("Response packet claims %u octets "
				"payload, above %u received\n",
				count,
				(u_int)(n - CTL_HEADER_LEN)
				);
			return ERR_INCOMPLETE;
		}

		if (debug >= 3 && shouldbesize > n) {
			u_int32 key;
			u_int32 *lpkt;
			int maclen;

			/*
			 * Usually we ignore authentication, but for debugging purposes
			 * we watch it here.
			 */
			/* round to 8 octet boundary */
			shouldbesize = (shouldbesize + 7) & ~7;

			maclen = n - shouldbesize;
			if (maclen >= (int)MIN_MAC_LEN) {
				printf(
					"Packet shows signs of authentication (total %d, data %d, mac %d)\n",
					n, shouldbesize, maclen);
				lpkt = (u_int32 *)&rpkt;
				printf("%08lx %08lx %08lx %08lx %08lx %08lx\n",
				       (u_long)ntohl(lpkt[(n - maclen)/sizeof(u_int32) - 3]),
				       (u_long)ntohl(lpkt[(n - maclen)/sizeof(u_int32) - 2]),
				       (u_long)ntohl(lpkt[(n - maclen)/sizeof(u_int32) - 1]),
				       (u_long)ntohl(lpkt[(n - maclen)/sizeof(u_int32)]),
				       (u_long)ntohl(lpkt[(n - maclen)/sizeof(u_int32) + 1]),
				       (u_long)ntohl(lpkt[(n - maclen)/sizeof(u_int32) + 2]));
				key = ntohl(lpkt[(n - maclen) / sizeof(u_int32)]);
				printf("Authenticated with keyid %lu\n", (u_long)key);
				if (key != 0 && key != info_auth_keyid) {
					printf("We don't know that key\n");
				} else {
					if (authdecrypt(key, (u_int32 *)&rpkt,
					    n - maclen, maclen)) {
						printf("Auth okay!\n");
					} else {
						printf("Auth failed!\n");
					}
				}
			}
		}

		if (debug >= 2)
			printf("Got packet, size = %d\n", n);
		if (count > (u_int)(n - CTL_HEADER_LEN)) {
			if (debug)
				printf("Received count of %d octets, "
					"data in packet is %u\n",
					count, (u_int)(n-CTL_HEADER_LEN));
			continue;
		}
		if (count == 0 && CTL_ISMORE(rpkt.r_m_e_op)) {
			if (debug)
				printf("Received count of 0 in non-final fragment\n");
			continue;
		}
		if (offset + count > sizeof(pktdata)) {
			if (debug)
				printf("Offset %d, count %d, too big for buffer\n",
				       offset, count);
			return ERR_TOOMUCH;
		}
		if (seenlastfrag && !CTL_ISMORE(rpkt.r_m_e_op)) {
			if (debug)
				printf("Received second last fragment packet\n");
			continue;
		}

		/*
		 * So far, so good.  Record this fragment, making sure it doesn't
		 * overlap anything.
		 */
		if (debug >= 2)
			printf("Packet okay\n");;

		if (numfrags > (MAXFRAGS - 1)) {
			if (debug)
				printf("Number of fragments exceeds maximum %d\n",
				       MAXFRAGS - 1);
			return ERR_TOOMUCH;
		}

		/*
		 * Find the position for the fragment relative to any
		 * previously received.
		 */
		for (f = 0; 
		     f < numfrags && offsets[f] < offset; 
		     f++) {
			/* empty body */ ;
		}

		if (f < numfrags && offset == offsets[f]) {
			if (debug)
				printf("duplicate %u octets at %u ignored, prior %u at %u\n",
				       count, offset, counts[f],
				       offsets[f]);
			continue;
		}

		if (f > 0 && (offsets[f-1] + counts[f-1]) > offset) {
			if (debug)
				printf("received frag at %u overlaps with %u octet frag at %u\n",
				       offset, counts[f-1],
				       offsets[f-1]);
			continue;
		}

		if (f < numfrags && (offset + count) > offsets[f]) {
			if (debug)
				printf("received %u octet frag at %u overlaps with frag at %u\n",
				       count, offset, offsets[f]);
			continue;
		}

		for (ff = numfrags; ff > f; ff--) {
			offsets[ff] = offsets[ff-1];
			counts[ff] = counts[ff-1];
		}
		offsets[f] = offset;
		counts[f] = count;
		numfrags++;

		/*
		 * Got that stuffed in right.  Figure out if this was the last.
		 * Record status info out of the last packet.
		 */
		if (!CTL_ISMORE(rpkt.r_m_e_op)) {
			seenlastfrag = 1;
			if (rstatus != 0)
				*rstatus = ntohs(rpkt.status);
		}

		/*
		 * Copy the data into the data buffer.
		 */
		memcpy((char *)pktdata + offset, rpkt.data, count);

		/*
		 * If we've seen the last fragment, look for holes in the sequence.
		 * If there aren't any, we're done.
		 */
		if (seenlastfrag && offsets[0] == 0) {
			for (f = 1; f < numfrags; f++)
				if (offsets[f-1] + counts[f-1] !=
				    offsets[f])
					break;
			if (f == numfrags) {
				*rsize = offsets[f-1] + counts[f-1];
				if (debug)
					fprintf(stderr,
						"%zu packets reassembled into response\n",
						numfrags);
				return 0;
			}
		}
	}  /* giant for (;;) collecting response packets */
}  /* getresponse() */


/*
 * sendrequest - format and send a request packet
 */
static int
sendrequest(
	int opcode,
	int associd,
	int auth,
	int qsize,
	char *qdata
	)
{
	struct ntp_control qpkt;
	int	pktsize;
	u_long	key_id;
	char *	pass;
	int	maclen;

	/*
	 * Check to make sure the data will fit in one packet
	 */
	if (qsize > CTL_MAX_DATA_LEN) {
		fprintf(stderr,
			"***Internal error!  qsize (%d) too large\n",
			qsize);
		return 1;
	}

	/*
	 * Fill in the packet
	 */
	qpkt.li_vn_mode = PKT_LI_VN_MODE(0, pktversion, MODE_CONTROL);
	qpkt.r_m_e_op = (u_char)(opcode & CTL_OP_MASK);
	qpkt.sequence = htons(sequence);
	qpkt.status = 0;
	qpkt.associd = htons((u_short)associd);
	qpkt.offset = 0;
	qpkt.count = htons((u_short)qsize);

	pktsize = CTL_HEADER_LEN;

	/*
	 * If we have data, copy and pad it out to a 32-bit boundary.
	 */
	if (qsize > 0) {
		memcpy(qpkt.data, qdata, (size_t)qsize);
		pktsize += qsize;
		while (pktsize & (sizeof(u_int32) - 1)) {
			qpkt.data[qsize++] = 0;
			pktsize++;
		}
	}

	/*
	 * If it isn't authenticated we can just send it.  Otherwise
	 * we're going to have to think about it a little.
	 */
	if (!auth && !always_auth) {
		return sendpkt(&qpkt, pktsize);
	} 

	/*
	 * Pad out packet to a multiple of 8 octets to be sure
	 * receiver can handle it.
	 */
	while (pktsize & 7) {
		qpkt.data[qsize++] = 0;
		pktsize++;
	}

	/*
	 * Get the keyid and the password if we don't have one.
	 */
	if (info_auth_keyid == 0) {
		key_id = getkeyid("Keyid: ");
		if (key_id == 0 || key_id > NTP_MAXKEY) {
			fprintf(stderr, 
				"Invalid key identifier\n");
			return 1;
		}
		info_auth_keyid = key_id;
	}
	if (!authistrusted(info_auth_keyid)) {
		pass = getpass_keytype(info_auth_keytype);
		if ('\0' == pass[0]) {
			fprintf(stderr, "Invalid password\n");
			return 1;
		}
		authusekey(info_auth_keyid, info_auth_keytype,
			   (u_char *)pass);
		authtrust(info_auth_keyid, 1);
	}

	/*
	 * Do the encryption.
	 */
	maclen = authencrypt(info_auth_keyid, (void *)&qpkt, pktsize);
	if (!maclen) {  
		fprintf(stderr, "Key not found\n");
		return 1;
	} else if ((size_t)maclen != (info_auth_hashlen + sizeof(keyid_t))) {
		fprintf(stderr,
			"%d octet MAC, %zu expected with %zu octet digest\n",
			maclen, (info_auth_hashlen + sizeof(keyid_t)),
			info_auth_hashlen);
		return 1;
	}
	
	return sendpkt((char *)&qpkt, pktsize + maclen);
}


/*
 * show_error_msg - display the error text for a mode 6 error response.
 */
void
show_error_msg(
	int		m6resp,
	associd_t	associd
	)
{
	if (numhosts > 1)
		fprintf(stderr, "server=%s ", currenthost);

	switch(m6resp) {

	case CERR_BADFMT:
		fprintf(stderr,
		    "***Server reports a bad format request packet\n");
		break;

	case CERR_PERMISSION:
		fprintf(stderr,
		    "***Server disallowed request (authentication?)\n");
		break;

	case CERR_BADOP:
		fprintf(stderr,
		    "***Server reports a bad opcode in request\n");
		break;

	case CERR_BADASSOC:
		fprintf(stderr,
		    "***Association ID %d unknown to server\n",
		    associd);
		break;

	case CERR_UNKNOWNVAR:
		fprintf(stderr,
		    "***A request variable unknown to the server\n");
		break;

	case CERR_BADVALUE:
		fprintf(stderr,
		    "***Server indicates a request variable was bad\n");
		break;

	case ERR_UNSPEC:
		fprintf(stderr,
		    "***Server returned an unspecified error\n");
		break;

	case ERR_TIMEOUT:
		fprintf(stderr, "***Request timed out\n");
		break;

	case ERR_INCOMPLETE:
		fprintf(stderr,
		    "***Response from server was incomplete\n");
		break;

	case ERR_TOOMUCH:
		fprintf(stderr,
		    "***Buffer size exceeded for returned data\n");
		break;

	default:
		fprintf(stderr,
		    "***Server returns unknown error code %d\n",
		    m6resp);
	}
}

/*
 * doquery - send a request and process the response, displaying
 *	     error messages for any error responses.
 */
int
doquery(
	int opcode,
	associd_t associd,
	int auth,
	int qsize,
	char *qdata,
	u_short *rstatus,
	int *rsize,
	const char **rdata
	)
{
	return doqueryex(opcode, associd, auth, qsize, qdata, rstatus,
			 rsize, rdata, FALSE);
}


/*
 * doqueryex - send a request and process the response, optionally
 *	       displaying error messages for any error responses.
 */
int
doqueryex(
	int opcode,
	associd_t associd,
	int auth,
	int qsize,
	char *qdata,
	u_short *rstatus,
	int *rsize,
	const char **rdata,
	int quiet
	)
{
	int res;
	int done;

	/*
	 * Check to make sure host is open
	 */
	if (!havehost) {
		fprintf(stderr, "***No host open, use `host' command\n");
		return -1;
	}

	done = 0;
	sequence++;

    again:
	/*
	 * send a request
	 */
	res = sendrequest(opcode, associd, auth, qsize, qdata);
	if (res != 0)
		return res;
	
	/*
	 * Get the response.  If we got a standard error, print a message
	 */
	res = getresponse(opcode, associd, rstatus, rsize, rdata, done);

	if (res > 0) {
		if (!done && (res == ERR_TIMEOUT || res == ERR_INCOMPLETE)) {
			if (res == ERR_INCOMPLETE) {
				/*
				 * better bump the sequence so we don't
				 * get confused about differing fragments.
				 */
				sequence++;
			}
			done = 1;
			goto again;
		}
		if (!quiet)
			show_error_msg(res, associd);

	}
	return res;
}


#ifndef BUILD_AS_LIB
/*
 * getcmds - read commands from the standard input and execute them
 */
static void
getcmds(void)
{
	char *	line;
	int	count;

	ntp_readline_init(interactive ? prompt : NULL);

	for (;;) {
		line = ntp_readline(&count);
		if (NULL == line)
			break;
		docmd(line);
		free(line);
	}

	ntp_readline_uninit();
}
#endif /* !BUILD_AS_LIB */


#if !defined(SYS_WINNT) && !defined(BUILD_AS_LIB)
/*
 * abortcmd - catch interrupts and abort the current command
 */
static RETSIGTYPE
abortcmd(
	int sig
	)
{
	if (current_output == stdout)
	    (void) fflush(stdout);
	putc('\n', stderr);
	(void) fflush(stderr);
	if (jump) longjmp(interrupt_buf, 1);
}
#endif	/* !SYS_WINNT && !BUILD_AS_LIB */


#ifndef	BUILD_AS_LIB
/*
 * docmd - decode the command line and execute a command
 */
static void
docmd(
	const char *cmdline
	)
{
	char *tokens[1+MAXARGS+2];
	struct parse pcmd;
	int ntok;
	static int i;
	struct xcmd *xcmd;

	/*
	 * Tokenize the command line.  If nothing on it, return.
	 */
	tokenize(cmdline, tokens, &ntok);
	if (ntok == 0)
	    return;
	
	/*
	 * Find the appropriate command description.
	 */
	i = findcmd(tokens[0], builtins, opcmds, &xcmd);
	if (i == 0) {
		(void) fprintf(stderr, "***Command `%s' unknown\n",
			       tokens[0]);
		return;
	} else if (i >= 2) {
		(void) fprintf(stderr, "***Command `%s' ambiguous\n",
			       tokens[0]);
		return;
	}
	
	/*
	 * Save the keyword, then walk through the arguments, interpreting
	 * as we go.
	 */
	pcmd.keyword = tokens[0];
	pcmd.nargs = 0;
	for (i = 0; i < MAXARGS && xcmd->arg[i] != NO; i++) {
		if ((i+1) >= ntok) {
			if (!(xcmd->arg[i] & OPT)) {
				printusage(xcmd, stderr);
				return;
			}
			break;
		}
		if ((xcmd->arg[i] & OPT) && (*tokens[i+1] == '>'))
			break;
		if (!getarg(tokens[i+1], (int)xcmd->arg[i], &pcmd.argval[i]))
			return;
		pcmd.nargs++;
	}

	i++;
	if (i < ntok && *tokens[i] == '>') {
		char *fname;

		if (*(tokens[i]+1) != '\0')
			fname = tokens[i]+1;
		else if ((i+1) < ntok)
			fname = tokens[i+1];
		else {
			(void) fprintf(stderr, "***No file for redirect\n");
			return;
		}

		current_output = fopen(fname, "w");
		if (current_output == NULL) {
			(void) fprintf(stderr, "***Error opening %s: ", fname);
			perror("");
			return;
		}
		i = 1;		/* flag we need a close */
	} else {
		current_output = stdout;
		i = 0;		/* flag no close */
	}

	if (interactive && setjmp(interrupt_buf)) {
		jump = 0;
		return;
	} else {
		jump++;
		(xcmd->handler)(&pcmd, current_output);
		jump = 0;	/* HMS: 961106: was after fclose() */
		if (i) (void) fclose(current_output);
	}
}


/*
 * tokenize - turn a command line into tokens
 *
 * SK: Modified to allow a quoted string 
 *
 * HMS: If the first character of the first token is a ':' then (after
 * eating inter-token whitespace) the 2nd token is the rest of the line.
 */

static void
tokenize(
	const char *line,
	char **tokens,
	int *ntok
	)
{
	register const char *cp;
	register char *sp;
	static char tspace[MAXLINE];

	sp = tspace;
	cp = line;
	for (*ntok = 0; *ntok < MAXTOKENS; (*ntok)++) {
		tokens[*ntok] = sp;

		/* Skip inter-token whitespace */
		while (ISSPACE(*cp))
		    cp++;

		/* If we're at EOL we're done */
		if (ISEOL(*cp))
		    break;

		/* If this is the 2nd token and the first token begins
		 * with a ':', then just grab to EOL.
		 */

		if (*ntok == 1 && tokens[0][0] == ':') {
			do {
				*sp++ = *cp++;
			} while (!ISEOL(*cp));
		}

		/* Check if this token begins with a double quote.
		 * If yes, continue reading till the next double quote
		 */
		else if (*cp == '\"') {
			++cp;
			do {
				*sp++ = *cp++;
			} while ((*cp != '\"') && !ISEOL(*cp));
			/* HMS: a missing closing " should be an error */
		}
		else {
			do {
				*sp++ = *cp++;
			} while ((*cp != '\"') && !ISSPACE(*cp) && !ISEOL(*cp));
			/* HMS: Why check for a " in the previous line? */
		}

		*sp++ = '\0';
	}
}


/*
 * getarg - interpret an argument token
 */
static int
getarg(
	char *str,
	int code,
	arg_v *argp
	)
{
	int isneg;
	char *cp, *np;
	static const char *digits = "0123456789";

	switch (code & ~OPT) {
	    case NTP_STR:
		argp->string = str;
		break;
	    case NTP_ADD:
		if (!getnetnum(str, &(argp->netnum), (char *)0, 0)) {
			return 0;
		}
		break;
	    case NTP_INT:
	    case NTP_UINT:
		isneg = 0;
		np = str;
		if (*np == '&') {
			np++;
			isneg = atoi(np);
			if (isneg <= 0) {
				(void) fprintf(stderr,
					       "***Association value `%s' invalid/undecodable\n", str);
				return 0;
			}
			if (isneg > numassoc) {
				if (numassoc == 0) {
					(void) fprintf(stderr,
						       "***Association for `%s' unknown (max &%d)\n",
						       str, numassoc);
					return 0;
				} else {
					isneg = numassoc;
				}
			}
			argp->uval = assoc_cache[isneg-1].assid;
			break;
		}

		if (*np == '-') {
			np++;
			isneg = 1;
		}

		argp->uval = 0;
		do {
			cp = strchr(digits, *np);
			if (cp == NULL) {
				(void) fprintf(stderr,
					       "***Illegal integer value %s\n", str);
				return 0;
			}
			argp->uval *= 10;
			argp->uval += (cp - digits);
		} while (*(++np) != '\0');

		if (isneg) {
			if ((code & ~OPT) == NTP_UINT) {
				(void) fprintf(stderr,
					       "***Value %s should be unsigned\n", str);
				return 0;
			}
			argp->ival = -argp->ival;
		}
		break;
	     case IP_VERSION:
		if (!strcmp("-6", str))
			argp->ival = 6 ;
		else if (!strcmp("-4", str))
			argp->ival = 4 ;
		else {
			(void) fprintf(stderr,
			    "***Version must be either 4 or 6\n");
			return 0;
		}
		break;
	}

	return 1;
}
#endif	/* !BUILD_AS_LIB */


/*
 * findcmd - find a command in a command description table
 */
static int
findcmd(
	register char *str,
	struct xcmd *clist1,
	struct xcmd *clist2,
	struct xcmd **cmd
	)
{
	register struct xcmd *cl;
	register int clen;
	int nmatch;
	struct xcmd *nearmatch = NULL;
	struct xcmd *clist;

	clen = strlen(str);
	nmatch = 0;
	if (clist1 != 0)
	    clist = clist1;
	else if (clist2 != 0)
	    clist = clist2;
	else
	    return 0;

    again:
	for (cl = clist; cl->keyword != 0; cl++) {
		/* do a first character check, for efficiency */
		if (*str != *(cl->keyword))
		    continue;
		if (strncmp(str, cl->keyword, (unsigned)clen) == 0) {
			/*
			 * Could be extact match, could be approximate.
			 * Is exact if the length of the keyword is the
			 * same as the str.
			 */
			if (*((cl->keyword) + clen) == '\0') {
				*cmd = cl;
				return 1;
			}
			nmatch++;
			nearmatch = cl;
		}
	}

	/*
	 * See if there is more to do.  If so, go again.  Sorry about the
	 * goto, too much looking at BSD sources...
	 */
	if (clist == clist1 && clist2 != 0) {
		clist = clist2;
		goto again;
	}

	/*
	 * If we got extactly 1 near match, use it, else return number
	 * of matches.
	 */
	if (nmatch == 1) {
		*cmd = nearmatch;
		return 1;
	}
	return nmatch;
}


/*
 * getnetnum - given a host name, return its net number
 *	       and (optional) full name
 */
int
getnetnum(
	const char *hname,
	sockaddr_u *num,
	char *fullhost,
	int af
	)
{
	struct addrinfo hints, *ai = NULL;

	ZERO(hints);
	hints.ai_flags = AI_CANONNAME;
#ifdef AI_ADDRCONFIG
	hints.ai_flags |= AI_ADDRCONFIG;
#endif
	
	/*
	 * decodenetnum only works with addresses, but handles syntax
	 * that getaddrinfo doesn't:  [2001::1]:1234
	 */
	if (decodenetnum(hname, num)) {
		if (fullhost != NULL)
			getnameinfo(&num->sa, SOCKLEN(num), fullhost,
				    LENHOSTNAME, NULL, 0, 0);
		return 1;
	} else if (getaddrinfo(hname, "ntp", &hints, &ai) == 0) {
		NTP_INSIST(sizeof(*num) >= ai->ai_addrlen);
		memcpy(num, ai->ai_addr, ai->ai_addrlen);
		if (fullhost != NULL) {
			if (ai->ai_canonname != NULL) {
				strncpy(fullhost, ai->ai_canonname,
					LENHOSTNAME);
				fullhost[LENHOSTNAME - 1] = '\0';
			} else {
				getnameinfo(&num->sa, SOCKLEN(num),
					    fullhost, LENHOSTNAME, NULL,
					    0, 0);
			}
		}
		return 1;
	}
	fprintf(stderr, "***Can't find host %s\n", hname);

	return 0;
}

/*
 * nntohost - convert network number to host name.  This routine enforces
 *	       the showhostnames setting.
 */
const char *
nntohost(
	sockaddr_u *netnum
	)
{
	return nntohost_col(netnum, LIB_BUFLENGTH - 1, FALSE);
}


/*
 * nntohost_col - convert network number to host name in fixed width.
 *		  This routine enforces the showhostnames setting.
 *		  When displaying hostnames longer than the width,
 *		  the first part of the hostname is displayed.  When
 *		  displaying numeric addresses longer than the width,
 *		  Such as IPv6 addresses, the caller decides whether
 *		  the first or last of the numeric address is used.
 */
const char *
nntohost_col(
	sockaddr_u *	addr,
	size_t		width,
	int		preserve_lowaddrbits
	)
{
	const char *	out;

	if (!showhostnames) {
		if (preserve_lowaddrbits)
			out = trunc_left(stoa(addr), width);
		else
			out = trunc_right(stoa(addr), width);
	} else if (ISREFCLOCKADR(addr)) {
		out = refnumtoa(addr);
	} else {
		out = trunc_right(socktohost(addr), width);
	}
	return out;
}


/*
 * rtdatetolfp - decode an RT-11 date into an l_fp
 */
static int
rtdatetolfp(
	char *str,
	l_fp *lfp
	)
{
	register char *cp;
	register int i;
	struct calendar cal;
	char buf[4];
	static const char *months[12] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

	cal.yearday = 0;

	/*
	 * An RT-11 date looks like:
	 *
	 * d[d]-Mth-y[y] hh:mm:ss
	 *
	 * (No docs, but assume 4-digit years are also legal...)
	 *
	 * d[d]-Mth-y[y[y[y]]] hh:mm:ss
	 */
	cp = str;
	if (!isdigit((int)*cp)) {
		if (*cp == '-') {
			/*
			 * Catch special case
			 */
			L_CLR(lfp);
			return 1;
		}
		return 0;
	}

	cal.monthday = (u_char) (*cp++ - '0');	/* ascii dependent */
	if (isdigit((int)*cp)) {
		cal.monthday = (u_char)((cal.monthday << 3) + (cal.monthday << 1));
		cal.monthday = (u_char)(cal.monthday + *cp++ - '0');
	}

	if (*cp++ != '-')
	    return 0;
	
	for (i = 0; i < 3; i++)
	    buf[i] = *cp++;
	buf[3] = '\0';

	for (i = 0; i < 12; i++)
	    if (STREQ(buf, months[i]))
		break;
	if (i == 12)
	    return 0;
	cal.month = (u_char)(i + 1);

	if (*cp++ != '-')
	    return 0;
	
	if (!isdigit((int)*cp))
	    return 0;
	cal.year = (u_short)(*cp++ - '0');
	if (isdigit((int)*cp)) {
		cal.year = (u_short)((cal.year << 3) + (cal.year << 1));
		cal.year = (u_short)(*cp++ - '0');
	}
	if (isdigit((int)*cp)) {
		cal.year = (u_short)((cal.year << 3) + (cal.year << 1));
		cal.year = (u_short)(cal.year + *cp++ - '0');
	}
	if (isdigit((int)*cp)) {
		cal.year = (u_short)((cal.year << 3) + (cal.year << 1));
		cal.year = (u_short)(cal.year + *cp++ - '0');
	}

	/*
	 * Catch special case.  If cal.year == 0 this is a zero timestamp.
	 */
	if (cal.year == 0) {
		L_CLR(lfp);
		return 1;
	}

	if (*cp++ != ' ' || !isdigit((int)*cp))
	    return 0;
	cal.hour = (u_char)(*cp++ - '0');
	if (isdigit((int)*cp)) {
		cal.hour = (u_char)((cal.hour << 3) + (cal.hour << 1));
		cal.hour = (u_char)(cal.hour + *cp++ - '0');
	}

	if (*cp++ != ':' || !isdigit((int)*cp))
	    return 0;
	cal.minute = (u_char)(*cp++ - '0');
	if (isdigit((int)*cp)) {
		cal.minute = (u_char)((cal.minute << 3) + (cal.minute << 1));
		cal.minute = (u_char)(cal.minute + *cp++ - '0');
	}

	if (*cp++ != ':' || !isdigit((int)*cp))
	    return 0;
	cal.second = (u_char)(*cp++ - '0');
	if (isdigit((int)*cp)) {
		cal.second = (u_char)((cal.second << 3) + (cal.second << 1));
		cal.second = (u_char)(cal.second + *cp++ - '0');
	}

	/*
	 * For RT-11, 1972 seems to be the pivot year
	 */
	if (cal.year < 72)
		cal.year += 2000;
	if (cal.year < 100)
		cal.year += 1900;

	lfp->l_ui = caltontp(&cal);
	lfp->l_uf = 0;
	return 1;
}


/*
 * decodets - decode a timestamp into an l_fp format number, with
 *	      consideration of fuzzball formats.
 */
int
decodets(
	char *str,
	l_fp *lfp
	)
{
	char *cp;
	char buf[30];
	size_t b;

	/*
	 * If it starts with a 0x, decode as hex.
	 */
	if (*str == '0' && (*(str+1) == 'x' || *(str+1) == 'X'))
		return hextolfp(str+2, lfp);

	/*
	 * If it starts with a '"', try it as an RT-11 date.
	 */
	if (*str == '"') {
		cp = str + 1;
		b = 0;
		while ('"' != *cp && '\0' != *cp &&
		       b < COUNTOF(buf) - 1)
			buf[b++] = *cp++;
		buf[b] = '\0';
		return rtdatetolfp(buf, lfp);
	}

	/*
	 * Might still be hex.  Check out the first character.  Talk
	 * about heuristics!
	 */
	if ((*str >= 'A' && *str <= 'F') || (*str >= 'a' && *str <= 'f'))
		return hextolfp(str, lfp);

	/*
	 * Try it as a decimal.  If this fails, try as an unquoted
	 * RT-11 date.  This code should go away eventually.
	 */
	if (atolfp(str, lfp))
		return 1;

	return rtdatetolfp(str, lfp);
}


/*
 * decodetime - decode a time value.  It should be in milliseconds
 */
int
decodetime(
	char *str,
	l_fp *lfp
	)
{
	return mstolfp(str, lfp);
}


/*
 * decodeint - decode an integer
 */
int
decodeint(
	char *str,
	long *val
	)
{
	if (*str == '0') {
		if (*(str+1) == 'x' || *(str+1) == 'X')
		    return hextoint(str+2, (u_long *)val);
		return octtoint(str, (u_long *)val);
	}
	return atoint(str, val);
}


/*
 * decodeuint - decode an unsigned integer
 */
int
decodeuint(
	char *str,
	u_long *val
	)
{
	if (*str == '0') {
		if (*(str + 1) == 'x' || *(str + 1) == 'X')
			return (hextoint(str + 2, val));
		return (octtoint(str, val));
	}
	return (atouint(str, val));
}


/*
 * decodearr - decode an array of time values
 */
static int
decodearr(
	char *str,
	int *narr,
	l_fp *lfparr
	)
{
	register char *cp, *bp;
	register l_fp *lfp;
	char buf[60];

	lfp = lfparr;
	cp = str;
	*narr = 0;

	while (*narr < 8) {
		while (isspace((int)*cp))
		    cp++;
		if (*cp == '\0')
		    break;

		bp = buf;
		while (!isspace((int)*cp) && *cp != '\0')
		    *bp++ = *cp++;
		*bp++ = '\0';

		if (!decodetime(buf, lfp))
		    return 0;
		(*narr)++;
		lfp++;
	}
	return 1;
}


/*
 * Finally, the built in command handlers
 */

/*
 * help - tell about commands, or details of a particular command
 */
static void
help(
	struct parse *pcmd,
	FILE *fp
	)
{
	struct xcmd *xcp = NULL;	/* quiet warning */
	char *cmd;
	const char *list[100];
	size_t word, words;
	size_t row, rows;
	size_t col, cols;
	size_t length;

	if (pcmd->nargs == 0) {
		words = 0;
		for (xcp = builtins; xcp->keyword != NULL; xcp++) {
			if (*(xcp->keyword) != '?')
				list[words++] = xcp->keyword;
		}
		for (xcp = opcmds; xcp->keyword != NULL; xcp++)
			list[words++] = xcp->keyword;

		qsort((void *)list, (size_t)words, sizeof(list[0]),
		      helpsort);
		col = 0;
		for (word = 0; word < words; word++) {
		 	length = strlen(list[word]);
			col = max(col, length);
		}

		cols = SCREENWIDTH / ++col;
		rows = (words + cols - 1) / cols;

		fprintf(fp, "ntpq commands:\n");

		for (row = 0; row < rows; row++) {
			for (word = row; word < words; word += rows)
				fprintf(fp, "%-*.*s", (int)col,  (int)col-1,
					list[word]);
			fprintf(fp, "\n");
		}
	} else {
		cmd = pcmd->argval[0].string;
		words = findcmd(cmd, builtins, opcmds, &xcp);
		if (words == 0) {
			fprintf(stderr,
				"Command `%s' is unknown\n", cmd);
			return;
		} else if (words >= 2) {
			fprintf(stderr,
				"Command `%s' is ambiguous\n", cmd);
			return;
		}
		fprintf(fp, "function: %s\n", xcp->comment);
		printusage(xcp, fp);
	}
}


/*
 * helpsort - do hostname qsort comparisons
 */
static int
helpsort(
	const void *t1,
	const void *t2
	)
{
	const char * const *	name1 = t1;
	const char * const *	name2 = t2;

	return strcmp(*name1, *name2);
}


/*
 * printusage - print usage information for a command
 */
static void
printusage(
	struct xcmd *xcp,
	FILE *fp
	)
{
	register int i;

	(void) fprintf(fp, "usage: %s", xcp->keyword);
	for (i = 0; i < MAXARGS && xcp->arg[i] != NO; i++) {
		if (xcp->arg[i] & OPT)
		    (void) fprintf(fp, " [ %s ]", xcp->desc[i]);
		else
		    (void) fprintf(fp, " %s", xcp->desc[i]);
	}
	(void) fprintf(fp, "\n");
}


/*
 * timeout - set time out time
 */
static void
timeout(
	struct parse *pcmd,
	FILE *fp
	)
{
	int val;

	if (pcmd->nargs == 0) {
		val = (int)tvout.tv_sec * 1000 + tvout.tv_usec / 1000;
		(void) fprintf(fp, "primary timeout %d ms\n", val);
	} else {
		tvout.tv_sec = pcmd->argval[0].uval / 1000;
		tvout.tv_usec = (pcmd->argval[0].uval - ((long)tvout.tv_sec * 1000))
			* 1000;
	}
}


/*
 * auth_delay - set delay for auth requests
 */
static void
auth_delay(
	struct parse *pcmd,
	FILE *fp
	)
{
	int isneg;
	u_long val;

	if (pcmd->nargs == 0) {
		val = delay_time.l_ui * 1000 + delay_time.l_uf / 4294967;
		(void) fprintf(fp, "delay %lu ms\n", val);
	} else {
		if (pcmd->argval[0].ival < 0) {
			isneg = 1;
			val = (u_long)(-pcmd->argval[0].ival);
		} else {
			isneg = 0;
			val = (u_long)pcmd->argval[0].ival;
		}

		delay_time.l_ui = val / 1000;
		val %= 1000;
		delay_time.l_uf = val * 4294967;	/* 2**32/1000 */

		if (isneg)
		    L_NEG(&delay_time);
	}
}


/*
 * host - set the host we are dealing with.
 */
static void
host(
	struct parse *pcmd,
	FILE *fp
	)
{
	int i;

	if (pcmd->nargs == 0) {
		if (havehost)
			(void) fprintf(fp, "current host is %s\n",
					   currenthost);
		else
			(void) fprintf(fp, "no current host\n");
		return;
	}

	i = 0;
	ai_fam_templ = ai_fam_default;
	if (pcmd->nargs == 2) {
		if (!strcmp("-4", pcmd->argval[i].string))
			ai_fam_templ = AF_INET;
		else if (!strcmp("-6", pcmd->argval[i].string))
			ai_fam_templ = AF_INET6;
		else {
			if (havehost)
				(void) fprintf(fp,
					       "current host remains %s\n",
					       currenthost);
			else
				(void) fprintf(fp, "still no current host\n");
			return;
		}
		i = 1;
	}
	if (openhost(pcmd->argval[i].string)) {
		(void) fprintf(fp, "current host set to %s\n", currenthost);
		numassoc = 0;
	} else {
		if (havehost)
			(void) fprintf(fp,
				       "current host remains %s\n", 
				       currenthost);
		else
			(void) fprintf(fp, "still no current host\n");
	}
}


/*
 * poll - do one (or more) polls of the host via NTP
 */
/*ARGSUSED*/
static void
ntp_poll(
	struct parse *pcmd,
	FILE *fp
	)
{
	(void) fprintf(fp, "poll not implemented yet\n");
}


/*
 * keyid - get a keyid to use for authenticating requests
 */
static void
keyid(
	struct parse *pcmd,
	FILE *fp
	)
{
	if (pcmd->nargs == 0) {
		if (info_auth_keyid == 0)
		    (void) fprintf(fp, "no keyid defined\n");
		else
		    (void) fprintf(fp, "keyid is %lu\n", (u_long)info_auth_keyid);
	} else {
		/* allow zero so that keyid can be cleared. */
		if(pcmd->argval[0].uval > NTP_MAXKEY)
		    (void) fprintf(fp, "Invalid key identifier\n");
		info_auth_keyid = pcmd->argval[0].uval;
	}
}

/*
 * keytype - get type of key to use for authenticating requests
 */
static void
keytype(
	struct parse *pcmd,
	FILE *fp
	)
{
	const char *	digest_name;
	size_t		digest_len;
	int		key_type;

	if (!pcmd->nargs) {
		fprintf(fp, "keytype is %s with %lu octet digests\n",
			keytype_name(info_auth_keytype),
			(u_long)info_auth_hashlen);
		return;
	}

	digest_name = pcmd->argval[0].string;
	digest_len = 0;
	key_type = keytype_from_text(digest_name, &digest_len);

	if (!key_type) {
		fprintf(fp, "keytype must be 'md5'%s\n",
#ifdef OPENSSL
			" or a digest type provided by OpenSSL");
#else
			"");
#endif
		return;
	}

	info_auth_keytype = key_type;
	info_auth_hashlen = digest_len;
}


/*
 * passwd - get an authentication key
 */
/*ARGSUSED*/
static void
passwd(
	struct parse *pcmd,
	FILE *fp
	)
{
	char *pass;

	if (info_auth_keyid == 0) {
		int u_keyid = getkeyid("Keyid: ");
		if (u_keyid == 0 || u_keyid > NTP_MAXKEY) {
			(void)fprintf(fp, "Invalid key identifier\n");
			return;
		}
		info_auth_keyid = u_keyid;
	}
	if (pcmd->nargs >= 1)
		pass = pcmd->argval[0].string;
	else {
		pass = getpass_keytype(info_auth_keytype);
		if ('\0' == pass[0]) {
			fprintf(fp, "Password unchanged\n");
			return;
		}
	}
	authusekey(info_auth_keyid, info_auth_keytype, (u_char *)pass);
	authtrust(info_auth_keyid, 1);
}


/*
 * hostnames - set the showhostnames flag
 */
static void
hostnames(
	struct parse *pcmd,
	FILE *fp
	)
{
	if (pcmd->nargs == 0) {
		if (showhostnames)
		    (void) fprintf(fp, "hostnames being shown\n");
		else
		    (void) fprintf(fp, "hostnames not being shown\n");
	} else {
		if (STREQ(pcmd->argval[0].string, "yes"))
		    showhostnames = 1;
		else if (STREQ(pcmd->argval[0].string, "no"))
		    showhostnames = 0;
		else
		    (void)fprintf(stderr, "What?\n");
	}
}



/*
 * setdebug - set/change debugging level
 */
static void
setdebug(
	struct parse *pcmd,
	FILE *fp
	)
{
	if (pcmd->nargs == 0) {
		(void) fprintf(fp, "debug level is %d\n", debug);
		return;
	} else if (STREQ(pcmd->argval[0].string, "no")) {
		debug = 0;
	} else if (STREQ(pcmd->argval[0].string, "more")) {
		debug++;
	} else if (STREQ(pcmd->argval[0].string, "less")) {
		debug--;
	} else {
		(void) fprintf(fp, "What?\n");
		return;
	}
	(void) fprintf(fp, "debug level set to %d\n", debug);
}


/*
 * quit - stop this nonsense
 */
/*ARGSUSED*/
static void
quit(
	struct parse *pcmd,
	FILE *fp
	)
{
	if (havehost)
	    closesocket(sockfd);	/* cleanliness next to godliness */
	exit(0);
}


/*
 * version - print the current version number
 */
/*ARGSUSED*/
static void
version(
	struct parse *pcmd,
	FILE *fp
	)
{

	(void) fprintf(fp, "%s\n", Version);
	return;
}


/*
 * raw - set raw mode output
 */
/*ARGSUSED*/
static void
raw(
	struct parse *pcmd,
	FILE *fp
	)
{
	rawmode = 1;
	(void) fprintf(fp, "Output set to raw\n");
}


/*
 * cooked - set cooked mode output
 */
/*ARGSUSED*/
static void
cooked(
	struct parse *pcmd,
	FILE *fp
	)
{
	rawmode = 0;
	(void) fprintf(fp, "Output set to cooked\n");
	return;
}


/*
 * authenticate - always authenticate requests to this host
 */
static void
authenticate(
	struct parse *pcmd,
	FILE *fp
	)
{
	if (pcmd->nargs == 0) {
		if (always_auth) {
			(void) fprintf(fp,
				       "authenticated requests being sent\n");
		} else
		    (void) fprintf(fp,
				   "unauthenticated requests being sent\n");
	} else {
		if (STREQ(pcmd->argval[0].string, "yes")) {
			always_auth = 1;
		} else if (STREQ(pcmd->argval[0].string, "no")) {
			always_auth = 0;
		} else
		    (void)fprintf(stderr, "What?\n");
	}
}


/*
 * ntpversion - choose the NTP version to use
 */
static void
ntpversion(
	struct parse *pcmd,
	FILE *fp
	)
{
	if (pcmd->nargs == 0) {
		(void) fprintf(fp,
			       "NTP version being claimed is %d\n", pktversion);
	} else {
		if (pcmd->argval[0].uval < NTP_OLDVERSION
		    || pcmd->argval[0].uval > NTP_VERSION) {
			(void) fprintf(stderr, "versions %d to %d, please\n",
				       NTP_OLDVERSION, NTP_VERSION);
		} else {
			pktversion = (u_char) pcmd->argval[0].uval;
		}
	}
}


static void __attribute__((__format__(__printf__, 1, 0)))
vwarning(const char *fmt, va_list ap)
{
	int serrno = errno;
	(void) fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, fmt, ap);
	(void) fprintf(stderr, ": %s", strerror(serrno));
}

/*
 * warning - print a warning message
 */
static void __attribute__((__format__(__printf__, 1, 2)))
warning(
	const char *fmt,
	...
	)
{
	va_list ap;
	va_start(ap, fmt);
	vwarning(fmt, ap);
	va_end(ap);
}


/*
 * error - print a message and exit
 */
static void __attribute__((__format__(__printf__, 1, 2)))
error(
	const char *fmt,
	...
	)
{
	va_list ap;
	va_start(ap, fmt);
	vwarning(fmt, ap);
	va_end(ap);
	exit(1);
}
/*
 * getkeyid - prompt the user for a keyid to use
 */
static u_long
getkeyid(
	const char *keyprompt
	)
{
	int c;
	FILE *fi;
	char pbuf[20];
	size_t i;
	size_t ilim;

#ifndef SYS_WINNT
	if ((fi = fdopen(open("/dev/tty", 2), "r")) == NULL)
#else
	if ((fi = _fdopen(open("CONIN$", _O_TEXT), "r")) == NULL)
#endif /* SYS_WINNT */
		fi = stdin;
	else
		setbuf(fi, (char *)NULL);
	fprintf(stderr, "%s", keyprompt); fflush(stderr);
	for (i = 0, ilim = COUNTOF(pbuf) - 1;
	     i < ilim && (c = getc(fi)) != '\n' && c != EOF;
	     )
		pbuf[i++] = (char)c;
	pbuf[i] = '\0';
	if (fi != stdin)
		fclose(fi);

	return (u_long) atoi(pbuf);
}


/*
 * atoascii - printable-ize possibly ascii data using the character
 *	      transformations cat -v uses.
 */
static void
atoascii(
	const char *in,
	size_t in_octets,
	char *out,
	size_t out_octets
	)
{
	register const u_char *	pchIn;
		 const u_char *	pchInLimit;
	register u_char *	pchOut;
	register u_char		c;

	pchIn = (const u_char *)in;
	pchInLimit = pchIn + in_octets;
	pchOut = (u_char *)out;

	if (NULL == pchIn) {
		if (0 < out_octets)
			*pchOut = '\0';
		return;
	}

#define	ONEOUT(c)					\
do {							\
	if (0 == --out_octets) {			\
		*pchOut = '\0';				\
		return;					\
	}						\
	*pchOut++ = (c);				\
} while (0)

	for (	; pchIn < pchInLimit; pchIn++) {
		c = *pchIn;
		if ('\0' == c)
			break;
		if (c & 0x80) {
			ONEOUT('M');
			ONEOUT('-');
			c &= 0x7f;
		}
		if (c < ' ') {
			ONEOUT('^');
			ONEOUT((u_char)(c + '@'));
		} else if (0x7f == c) {
			ONEOUT('^');
			ONEOUT('?');
		} else
			ONEOUT(c);
	}
	ONEOUT('\0');

#undef ONEOUT
}


/*
 * makeascii - print possibly ascii data using the character
 *	       transformations that cat -v uses.
 */
void
makeascii(
	int length,
	const char *data,
	FILE *fp
	)
{
	const u_char *data_u_char;
	const u_char *cp;
	int c;

	data_u_char = (const u_char *)data;

	for (cp = data_u_char; cp < data_u_char + length; cp++) {
		c = (int)*cp;
		if (c & 0x80) {
			putc('M', fp);
			putc('-', fp);
			c &= 0x7f;
		}

		if (c < ' ') {
			putc('^', fp);
			putc(c + '@', fp);
		} else if (0x7f == c) {
			putc('^', fp);
			putc('?', fp);
		} else
			putc(c, fp);
	}
}


/*
 * asciize - same thing as makeascii except add a newline
 */
void
asciize(
	int length,
	char *data,
	FILE *fp
	)
{
	makeascii(length, data, fp);
	putc('\n', fp);
}


/*
 * truncate string to fit clipping excess at end.
 *	"too long"	->	"too l"
 * Used for hostnames.
 */
const char *
trunc_right(
	const char *	src,
	size_t		width
	)
{
	size_t	sl;
	char *	out;

	
	sl = strlen(src);
	if (sl > width && LIB_BUFLENGTH - 1 > width && width > 0) {
		LIB_GETBUF(out);
		memcpy(out, src, width);
		out[width] = '\0';

		return out;
	}

	return src;
}


/*
 * truncate string to fit by preserving right side and using '_' to hint
 *	"too long"	->	"_long"
 * Used for local IPv6 addresses, where low bits differentiate.
 */
const char *
trunc_left(
	const char *	src,
	size_t		width
	)
{
	size_t	sl;
	char *	out;


	sl = strlen(src);
	if (sl > width && LIB_BUFLENGTH - 1 > width && width > 1) {
		LIB_GETBUF(out);
		out[0] = '_';
		memcpy(&out[1], &src[sl + 1 - width], width);

		return out;
	}

	return src;
}


/*
 * Some circular buffer space
 */
#define	CBLEN	80
#define	NUMCB	6

char circ_buf[NUMCB][CBLEN];
int nextcb = 0;

/*
 * nextvar - find the next variable in the buffer
 */
int
nextvar(
	int *datalen,
	const char **datap,
	char **vname,
	char **vvalue
	)
{
	const char *cp;
	char *np;
	const char *cpend;
	char *npend;	/* character after last */
	int quoted = 0;
	static char name[MAXVARLEN];
	static char value[MAXVALLEN];

	cp = *datap;
	cpend = cp + *datalen;

	/*
	 * Space past commas and white space
	 */
	while (cp < cpend && (*cp == ',' || isspace((int)*cp)))
		cp++;
	if (cp == cpend)
		return 0;
	
	/*
	 * Copy name until we hit a ',', an '=', a '\r' or a '\n'.  Backspace
	 * over any white space and terminate it.
	 */
	np = name;
	npend = &name[MAXVARLEN];
	while (cp < cpend && np < npend && *cp != ',' && *cp != '='
	       && *cp != '\r' && *cp != '\n')
	    *np++ = *cp++;
	/*
	 * Check if we ran out of name space, without reaching the end or a
	 * terminating character
	 */
	if (np == npend && !(cp == cpend || *cp == ',' || *cp == '=' ||
			     *cp == '\r' || *cp == '\n'))
	    return 0;
	while (isspace((int)(*(np-1))))
	    np--;
	*np = '\0';
	*vname = name;

	/*
	 * Check if we hit the end of the buffer or a ','.  If so we are done.
	 */
	if (cp == cpend || *cp == ',' || *cp == '\r' || *cp == '\n') {
		if (cp != cpend)
		    cp++;
		*datap = cp;
		*datalen = cpend - cp;
		*vvalue = (char *)0;
		return 1;
	}

	/*
	 * So far, so good.  Copy out the value
	 */
	cp++;	/* past '=' */
	while (cp < cpend && (isspace((int)*cp) && *cp != '\r' && *cp != '\n'))
	    cp++;
	np = value;
	npend = &value[MAXVALLEN];
	while (cp < cpend && np < npend && ((*cp != ',') || quoted))
	{
		quoted ^= ((*np++ = *cp++) == '"');
	}

	/*
	 * Check if we overran the value buffer while still in a quoted string
	 * or without finding a comma
	 */
	if (np == npend && (quoted || *cp != ','))
	    return 0;
	/*
	 * Trim off any trailing whitespace
	 */
	while (np > value && isspace((int)(*(np-1))))
	    np--;
	*np = '\0';

	/*
	 * Return this.  All done.
	 */
	if (cp != cpend)
	    cp++;
	*datap = cp;
	*datalen = cpend - cp;
	*vvalue = value;
	return 1;
}


/*
 * findvar - see if this variable is known to us.
 * If "code" is 1, return ctl_var->code.
 * Otherwise return the ordinal position of the found variable.
 */
int
findvar(
	char *varname,
	struct ctl_var *varlist,
	int code
	)
{
	register char *np;
	register struct ctl_var *vl;

	vl = varlist;
	np = varname;
	while (vl->fmt != EOV) {
		if (vl->fmt != PADDING && STREQ(np, vl->text))
		    return (code)
				? vl->code
				: (vl - varlist)
			    ;
		vl++;
	}
	return 0;
}



/*
 * printvars - print variables returned in response packet
 */
void
printvars(
	int length,
	const char *data,
	int status,
	int sttype,
	int quiet,
	FILE *fp
	)
{
	if (rawmode)
	    rawprint(sttype, length, data, status, quiet, fp);
	else
	    cookedprint(sttype, length, data, status, quiet, fp);
}


/*
 * rawprint - do a printout of the data in raw mode
 */
static void
rawprint(
	int datatype,
	int length,
	const char *data,
	int status,
	int quiet,
	FILE *fp
	)
{
	const char *cp;
	const char *cpend;

	/*
	 * Essentially print the data as is.  We reformat unprintables, though.
	 */
	cp = data;
	cpend = data + length;

	if (!quiet)
		(void) fprintf(fp, "status=0x%04x,\n", status);

	while (cp < cpend) {
		if (*cp == '\r') {
			/*
			 * If this is a \r and the next character is a
			 * \n, supress this, else pretty print it.  Otherwise
			 * just output the character.
			 */
			if (cp == (cpend - 1) || *(cp + 1) != '\n')
			    makeascii(1, cp, fp);
		} else if (isspace((unsigned char)*cp) || isprint((unsigned char)*cp))
			putc(*cp, fp);
		else
			makeascii(1, cp, fp);
		cp++;
	}
}


/*
 * Global data used by the cooked output routines
 */
int out_chars;		/* number of characters output */
int out_linecount;	/* number of characters output on this line */


/*
 * startoutput - get ready to do cooked output
 */
static void
startoutput(void)
{
	out_chars = 0;
	out_linecount = 0;
}


/*
 * output - output a variable=value combination
 */
static void
output(
	FILE *fp,
	char *name,
	const char *value
	)
{
	size_t len;

	/* strlen of "name=value" */
	len = strlen(name) + 1 + strlen(value);

	if (out_chars != 0) {
		out_chars += 2;
		if ((out_linecount + len + 2) > MAXOUTLINE) {
			fputs(",\n", fp);
			out_linecount = 0;
		} else {
			fputs(", ", fp);
			out_linecount += 2;
		}
	}

	fputs(name, fp);
	putc('=', fp);
	fputs(value, fp);
	out_chars += len;
	out_linecount += len;
}


/*
 * endoutput - terminate a block of cooked output
 */
static void
endoutput(
	FILE *fp
	)
{
	if (out_chars != 0)
		putc('\n', fp);
}


/*
 * outputarr - output an array of values
 */
static void
outputarr(
	FILE *fp,
	char *name,
	int narr,
	l_fp *lfp
	)
{
	register char *bp;
	register char *cp;
	register int i;
	register int len;
	char buf[256];

	bp = buf;
	/*
	 * Hack to align delay and offset values
	 */
	for (i = (int)strlen(name); i < 11; i++)
	    *bp++ = ' ';
	
	for (i = narr; i > 0; i--) {
		if (i != narr)
		    *bp++ = ' ';
		cp = lfptoms(lfp, 2);
		len = strlen(cp);
		if (len > 7) {
			cp[7] = '\0';
			len = 7;
		}
		while (len < 7) {
			*bp++ = ' ';
			len++;
		}
		while (*cp != '\0')
		    *bp++ = *cp++;
		lfp++;
	}
	*bp = '\0';
	output(fp, name, buf);
}

static char *
tstflags(
	u_long val
	)
{
	register char *cp, *s;
	size_t cb;
	register int i;
	register const char *sep;

	sep = "";
	i = 0;
	s = cp = circ_buf[nextcb];
	if (++nextcb >= NUMCB)
		nextcb = 0;
	cb = sizeof(circ_buf[0]);

	snprintf(cp, cb, "%02lx", val);
	cp += strlen(cp);
	cb -= strlen(cp);
	if (!val) {
		strncat(cp, " ok", cb);
		cp += strlen(cp);
		cb -= strlen(cp);
	} else {
		if (cb) {
			*cp++ = ' ';
			cb--;
		}
		for (i = 0; i < (int)COUNTOF(tstflagnames); i++) {
			if (val & 0x1) {
				snprintf(cp, cb, "%s%s", sep,
					 tstflagnames[i]);
				sep = ", ";
				cp += strlen(cp);
				cb -= strlen(cp);
			}
			val >>= 1;
		}
	}
	if (cb)
		*cp = '\0';

	return s;
}

/*
 * cookedprint - output variables in cooked mode
 */
static void
cookedprint(
	int datatype,
	int length,
	const char *data,
	int status,
	int quiet,
	FILE *fp
	)
{
	register int varid;
	char *name;
	char *value;
	char output_raw;
	int fmt;
	struct ctl_var *varlist;
	l_fp lfp;
	long ival;
	sockaddr_u hval;
	u_long uval;
	l_fp lfparr[8];
	int narr;

	switch (datatype) {
	case TYPE_PEER:
		varlist = peer_var;
		break;
	case TYPE_SYS:
		varlist = sys_var;
		break;
	case TYPE_CLOCK:
		varlist = clock_var;
		break;
	default:
		fprintf(stderr, "Unknown datatype(0x%x) in cookedprint\n",
			datatype);
		return;
	}

	if (!quiet)
		fprintf(fp, "status=%04x %s,\n", status,
			statustoa(datatype, status));

	startoutput();
	while (nextvar(&length, &data, &name, &value)) {
		varid = findvar(name, varlist, 0);
		if (varid == 0) {
			output_raw = '*';
		} else {
			output_raw = 0;
			fmt = varlist[varid].fmt;
			switch(fmt) {
			    case TS:
				if (!decodets(value, &lfp))
				    output_raw = '?';
				else
				    output(fp, name, prettydate(&lfp));
				break;
			    case FL:
			    case FU:
			    case FS:
				if (!decodetime(value, &lfp))
				    output_raw = '?';
				else {
					switch (fmt) {
					    case FL:
						output(fp, name,
						       lfptoms(&lfp, 3));
						break;
					    case FU:
						output(fp, name,
						       ulfptoms(&lfp, 3));
						break;
					    case FS:
						output(fp, name,
						       lfptoms(&lfp, 3));
						break;
					}
				}
				break;
			
			    case UI:
				if (!decodeuint(value, &uval))
				    output_raw = '?';
				else
				    output(fp, name, uinttoa(uval));
				break;
			
			    case SI:
				if (!decodeint(value, &ival))
				    output_raw = '?';
				else
				    output(fp, name, inttoa(ival));
				break;

			    case HA:
			    case NA:
				if (!decodenetnum(value, &hval))
				    output_raw = '?';
				else if (fmt == HA){
				    output(fp, name, nntohost(&hval));
				} else {
				    output(fp, name, stoa(&hval));
				}
				break;
			
			    case ST:
				output_raw = '*';
				break;
			
			    case RF:
				if (decodenetnum(value, &hval)) {
					if (ISREFCLOCKADR(&hval))
    						output(fp, name,
						    refnumtoa(&hval));
					else
				    		output(fp, name, stoa(&hval));
				} else if ((int)strlen(value) <= 4)
				    output(fp, name, value);
				else
				    output_raw = '?';
				break;

			    case LP:
				if (!decodeuint(value, &uval) || uval > 3)
				    output_raw = '?';
				else {
					char b[3];
					b[0] = b[1] = '0';
					if (uval & 0x2)
					    b[0] = '1';
					if (uval & 0x1)
					    b[1] = '1';
					b[2] = '\0';
					output(fp, name, b);
				}
				break;

			    case OC:
				if (!decodeuint(value, &uval))
				    output_raw = '?';
				else {
					char b[12];

					(void) snprintf(b, sizeof b, "%03lo", uval);
					output(fp, name, b);
				}
				break;
			
			    case MD:
				if (!decodeuint(value, &uval))
				    output_raw = '?';
				else
				    output(fp, name, uinttoa(uval));
				break;
			
			    case AR:
				if (!decodearr(value, &narr, lfparr))
				    output_raw = '?';
				else
				    outputarr(fp, name, narr, lfparr);
				break;

			    case FX:
				if (!decodeuint(value, &uval))
				    output_raw = '?';
				else
				    output(fp, name, tstflags(uval));
				break;
			
			    default:
				(void) fprintf(stderr,
				    "Internal error in cookedprint, %s=%s, fmt %d\n",
				    name, value, fmt);
				break;
			}

		}
		if (output_raw != 0) {
			char bn[401];
			char bv[401];
			int len;

			atoascii(name, MAXVARLEN, bn, sizeof(bn));
			atoascii(value, MAXVARLEN, bv, sizeof(bv));
			if (output_raw != '*') {
				len = strlen(bv);
				bv[len] = output_raw;
				bv[len+1] = '\0';
			}
			output(fp, bn, bv);
		}
	}
	endoutput(fp);
}


/*
 * sortassoc - sort associations in the cache into ascending order
 */
void
sortassoc(void)
{
	if (numassoc > 1)
		qsort((void *)assoc_cache, (size_t)numassoc,
		    sizeof(assoc_cache[0]), assoccmp);
}


/*
 * assoccmp - compare two associations
 */
static int
assoccmp(
	const void *t1,
	const void *t2
	)
{
	const struct association *ass1 = t1;
	const struct association *ass2 = t2;

	if (ass1->assid < ass2->assid)
		return -1;
	if (ass1->assid > ass2->assid)
		return 1;
	return 0;
}


/*
 * ntpq_custom_opt_handler - autoopts handler for -c and -p
 *
 * By default, autoopts loses the relative order of -c and -p options
 * on the command line.  This routine replaces the default handler for
 * those routines and builds a list of commands to execute preserving
 * the order.
 */
void
ntpq_custom_opt_handler(
	tOptions *pOptions,
	tOptDesc *pOptDesc
	)
{
	switch (pOptDesc->optValue) {
	
	default:
		fprintf(stderr, 
			"ntpq_custom_opt_handler unexpected option '%c' (%d)\n",
			pOptDesc->optValue, pOptDesc->optValue);
		exit(-1);

	case 'c':
		ADDCMD(pOptDesc->pzLastArg);
		break;

	case 'p':
		ADDCMD("peers");
		break;
	}
}
