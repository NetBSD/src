/*
 *	Chat -- a program for automatic session establishment (i.e. dial
 *		the phone and log in).
 *
 * Standard termination codes:
 *  0 - successful completion of the script
 *  1 - invalid argument, expect string too large, etc.
 *  2 - error on an I/O operation or fatal error condition.
 *  3 - timeout waiting for a simple string.
 *  4 - the first string declared as "ABORT"
 *  5 - the second string declared as "ABORT"
 *  6 - ... and so on for successive ABORT strings.
 *
 *	This software is in the public domain.
 *
 * -----------------
 *	22-May-99 added environment substitutuion, enabled with -E switch.
 *	Andreas Arens <andras@cityweb.de>.
 *
 *	12-May-99 added a feature to read data to be sent from a file,
 *	if the send string starts with @.  Idea from gpk <gpk@onramp.net>.
 *
 *	added -T and -U option and \T and \U substitution to pass a phone
 *	number into chat script. Two are needed for some ISDN TA applications.
 *	Keith Dart <kdart@cisco.com>
 *	
 *
 *	Added SAY keyword to send output to stderr.
 *      This allows to turn ECHO OFF and to output specific, user selected,
 *      text to give progress messages. This best works when stderr
 *      exists (i.e.: pppd in nodetach mode).
 *
 * 	Added HANGUP directives to allow for us to be called
 *      back. When HANGUP is set to NO, chat will not hangup at HUP signal.
 *      We rely on timeouts in that case.
 *
 *      Added CLR_ABORT to clear previously set ABORT string. This has been
 *      dictated by the HANGUP above as "NO CARRIER" (for example) must be
 *      an ABORT condition until we know the other host is going to close
 *      the connection for call back. As soon as we have completed the
 *      first stage of the call back sequence, "NO CARRIER" is a valid, non
 *      fatal string. As soon as we got called back (probably get "CONNECT"),
 *      we should re-arm the ABORT "NO CARRIER". Hence the CLR_ABORT command.
 *      Note that CLR_ABORT packs the abort_strings[] array so that we do not
 *      have unused entries not being reclaimed.
 *
 *      In the same vein as above, added CLR_REPORT keyword.
 *
 *      Allow for comments. Line starting with '#' are comments and are
 *      ignored. If a '#' is to be expected as the first character, the 
 *      expect string must be quoted.
 *
 *
 *		Francis Demierre <Francis@SwissMail.Com>
 * 		Thu May 15 17:15:40 MET DST 1997
 *
 *
 *      Added -r "report file" switch & REPORT keyword.
 *              Robert Geer <bgeer@xmission.com>
 *
 *      Added -s "use stderr" and -S "don't use syslog" switches.
 *              June 18, 1997
 *              Karl O. Pinc <kop@meme.com>
 *
 *
 *	Added -e "echo" switch & ECHO keyword
 *		Dick Streefland <dicks@tasking.nl>
 *
 *
 *	Considerable updates and modifications by
 *		Al Longyear <longyear@pobox.com>
 *		Paul Mackerras <paulus@cs.anu.edu.au>
 *
 *
 *	The original author is:
 *
 *		Karl Fox <karl@MorningStar.Com>
 *		Morning Star Technologies, Inc.
 *		1760 Zollinger Road
 *		Columbus, OH  43221
 *		(614)451-1883
 *
 */

#ifndef __STDC__
#define const
#endif
#include <sys/cdefs.h>
#if 0
#ifndef lint
static const char rcsid[] = "Id: chat.c,v 1.30 2004/01/17 05:47:55 carlsonj Exp ";
#endif
#else
__RCSID("NetBSD: chat.c,v 1.2 2013/11/28 22:33:42 christos Exp ");
#endif

#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

#ifndef TERMIO
#undef	TERMIOS
#define TERMIOS
#endif

#ifdef TERMIO
#include <termio.h>
#endif
#ifdef TERMIOS
#include <termios.h>
#endif

#define	STR_LEN	1024

#ifndef SIGTYPE
#define SIGTYPE void
#endif

#undef __P
#undef __V

#ifdef __STDC__
#include <stdarg.h>
#define __V(x)	x
#define __P(x)	x
#else
#include <varargs.h>
#define __V(x)	(va_alist) va_dcl
#define __P(x)	()
#define const
#endif

#ifndef O_NONBLOCK
#define O_NONBLOCK	O_NDELAY
#endif

#ifdef SUNOS
extern int sys_nerr;
extern char *sys_errlist[];
#define memmove(to, from, n)	bcopy(from, to, n)
#define strerror(n)		((unsigned)(n) < sys_nerr? sys_errlist[(n)] :\
				 "unknown error")
#endif

char *program_name;

#define	BUFFER_SIZE		256
#define	MAX_ABORTS		50
#define	MAX_REPORTS		50
#define	DEFAULT_CHAT_TIMEOUT	45

int echo          = 0;
int verbose       = 0;
int to_log        = 1;
int to_stderr     = 0;
int Verbose       = 0;
int quiet         = 0;
int report        = 0;
int use_env       = 0;
int exit_code     = 0;
FILE* report_fp   = (FILE *) 0;
char *report_file = (char *) 0;
char *chat_file   = (char *) 0;
char *phone_num   = (char *) 0;
char *phone_num2  = (char *) 0;
int timeout       = DEFAULT_CHAT_TIMEOUT;

int have_tty_parameters = 0;

#ifdef TERMIO
#define term_parms struct termio
#define get_term_param(param) ioctl(0, TCGETA, param)
#define set_term_param(param) ioctl(0, TCSETA, param)
struct termio saved_tty_parameters;
#endif

#ifdef TERMIOS
#define term_parms struct termios
#define get_term_param(param) tcgetattr(0, param)
#define set_term_param(param) tcsetattr(0, TCSANOW, param)
struct termios saved_tty_parameters;
#endif

char *abort_string[MAX_ABORTS], *fail_reason = (char *)0,
	fail_buffer[BUFFER_SIZE];
int n_aborts = 0, abort_next = 0, timeout_next = 0, echo_next = 0;
int clear_abort_next = 0;

char *report_string[MAX_REPORTS] ;
char  report_buffer[BUFFER_SIZE] ;
int n_reports = 0, report_next = 0, report_gathering = 0 ; 
int clear_report_next = 0;

int say_next = 0, hup_next = 0;

void *dup_mem __P((void *b, size_t c));
void *copy_of __P((char *s));
char *grow __P((char *s, char **p, size_t len));
void usage __P((void));
void msgf __P((const char *fmt, ...));
void fatal __P((int code, const char *fmt, ...));
SIGTYPE sigalrm __P((int signo));
SIGTYPE sigint __P((int signo));
SIGTYPE sigterm __P((int signo));
SIGTYPE sighup __P((int signo));
void unalarm __P((void));
void init __P((void));
void set_tty_parameters __P((void));
void echo_stderr __P((int));
void break_sequence __P((void));
void terminate __P((int status));
void do_file __P((char *chat_file));
int  get_string __P((char *string));
int  put_string __P((char *s));
int  write_char __P((int c));
int  put_char __P((int c));
int  get_char __P((void));
void chat_send __P((char *s));
char *character __P((int c));
void chat_expect __P((char *s));
char *clean __P((char *s, int sending));
void break_sequence __P((void));
void terminate __P((int status));
void pack_array __P((char **array, int end));
char *expect_strtok __P((char *, char *));
int vfmtmsg __P((char *, int, const char *, va_list));	/* vsprintf++ */

int main __P((int, char *[]));

void *dup_mem(b, c)
void *b;
size_t c;
{
    void *ans = malloc (c);
    if (!ans)
	fatal(2, "memory error!");

    memcpy (ans, b, c);
    return ans;
}

void *copy_of (s)
char *s;
{
    return dup_mem (s, strlen (s) + 1);
}

/* grow a char buffer and keep a pointer offset */
char *grow(s, p, len)
char *s;
char **p;
size_t len;
{
    size_t l = *p - s;		/* save p as distance into s */

    s = realloc(s, len);
    if (!s)
	fatal(2, "memory error!");
    *p = s + l;			/* restore p */
    return s;
}

/*
 * chat [ -v ] [ -E ] [ -T number ] [ -U number ] [ -t timeout ] [ -f chat-file ] \
 * [ -r report-file ] \
 *		[...[[expect[-say[-expect...]] say expect[-say[-expect]] ...]]]
 *
 *	Perform a UUCP-dialer-like chat script on stdin and stdout.
 */
int
main(argc, argv)
     int argc;
     char **argv;
{
    int option;
    int i;

    program_name = *argv;
    tzset();

    while ((option = getopt(argc, argv, ":eEvVf:t:r:sST:U:")) != -1) {
	switch (option) {
	case 'e':
	    ++echo;
	    break;

	case 'E':
	    ++use_env;
	    break;

	case 'v':
	    ++verbose;
	    break;

	case 'V':
	    ++Verbose;
	    break;

	case 's':
	    ++to_stderr;
	    break;

	case 'S':
	    to_log = 0;
	    break;

	case 'f':
	    if (optarg != NULL)
		    chat_file = copy_of(optarg);
	    else
		usage();
	    break;

	case 't':
	    if (optarg != NULL)
		timeout = atoi(optarg);
	    else
		usage();
	    break;

	case 'r':
	    if (optarg) {
		if (report_fp != NULL)
		    fclose (report_fp);
		report_file = copy_of (optarg);
		report_fp   = fopen (report_file, "a");
		if (report_fp != NULL) {
		    if (verbose)
			fprintf (report_fp, "Opening \"%s\"...\n",
				 report_file);
		    report = 1;
		}
	    }
	    break;

	case 'T':
	    if (optarg != NULL)
		phone_num = copy_of(optarg);
	    else
		usage();
	    break;

	case 'U':
	    if (optarg != NULL)
		phone_num2 = copy_of(optarg);
	    else
		usage();
	    break;

	default:
	    usage();
	    break;
	}
    }
    argc -= optind;
    argv += optind;
/*
 * Default the report file to the stderr location
 */
    if (report_fp == NULL)
	report_fp = stderr;

    if (to_log) {
#ifdef ultrix
	openlog("chat", LOG_PID);
#else
	openlog("chat", LOG_PID | LOG_NDELAY, LOG_LOCAL2);

	if (verbose)
	    setlogmask(LOG_UPTO(LOG_INFO));
	else
	    setlogmask(LOG_UPTO(LOG_WARNING));
#endif
    }

    init();
    
    if (chat_file != NULL) {
	if (argc)
	    usage();
	else
	    do_file (chat_file);
    } else {
	for (i = 0; i < argc; i++) {
	    chat_expect(argv[i]);
	    if (++i < argc)
		chat_send(argv[i]);
	}
    }

    terminate(0);
    return 0;
}

/*
 *  Process a chat script when read from a file.
 */

void do_file (chat_file)
char *chat_file;
{
    int linect, sendflg;
    char *sp, *arg, quote;
    char buf [STR_LEN];
    FILE *cfp;

    cfp = fopen (chat_file, "r");
    if (cfp == NULL)
	fatal(1, "%s -- open failed: %m", chat_file);

    linect = 0;
    sendflg = 0;

    while (fgets(buf, STR_LEN, cfp) != NULL) {
	sp = strchr (buf, '\n');
	if (sp)
	    *sp = '\0';

	linect++;
	sp = buf;

        /* lines starting with '#' are comments. If a real '#'
           is to be expected, it should be quoted .... */
        if ( *sp == '#' )
	    continue;

	while (*sp != '\0') {
	    if (*sp == ' ' || *sp == '\t') {
		++sp;
		continue;
	    }

	    if (*sp == '"' || *sp == '\'') {
		quote = *sp++;
		arg = sp;
		while (*sp != quote) {
		    if (*sp == '\0')
			fatal(1, "unterminated quote (line %d)", linect);

		    if (*sp++ == '\\') {
			if (*sp != '\0')
			    ++sp;
		    }
		}
	    }
	    else {
		arg = sp;
		while (*sp != '\0' && *sp != ' ' && *sp != '\t')
		    ++sp;
	    }

	    if (*sp != '\0')
		*sp++ = '\0';

	    if (sendflg)
		chat_send (arg);
	    else
		chat_expect (arg);
	    sendflg = !sendflg;
	}
    }
    fclose (cfp);
}

/*
 *	We got an error parsing the command line.
 */
void usage()
{
    fprintf(stderr, "\
Usage: %s [-e] [-E] [-v] [-V] [-t timeout] [-r report-file]\n\
     [-T phone-number] [-U phone-number2] {-f chat-file | chat-script}\n", program_name);
    exit(1);
}

char line[1024];

/*
 * Send a message to syslog and/or stderr.
 */
void msgf __V((const char *fmt, ...))
{
    va_list args;

#ifdef __STDC__
    va_start(args, fmt);
#else
    char *fmt;
    va_start(args);
    fmt = va_arg(args, char *);
#endif

    vfmtmsg(line, sizeof(line), fmt, args);
    va_end(args);
    if (to_log)
	syslog(LOG_INFO, "%s", line);
    if (to_stderr)
	fprintf(stderr, "%s\n", line);
}

/*
 *	Print an error message and terminate.
 */

void fatal __V((int code, const char *fmt, ...))
{
    va_list args;

#ifdef __STDC__
    va_start(args, fmt);
#else
    int code;
    char *fmt;
    va_start(args);
    code = va_arg(args, int);
    fmt = va_arg(args, char *);
#endif

    vfmtmsg(line, sizeof(line), fmt, args);
    va_end(args);
    if (to_log)
	syslog(LOG_ERR, "%s", line);
    if (to_stderr)
	fprintf(stderr, "%s\n", line);
    terminate(code);
}

int alarmed = 0;

SIGTYPE sigalrm(signo)
int signo;
{
    int flags;

    alarm(1);
    alarmed = 1;		/* Reset alarm to avoid race window */
    signal(SIGALRM, sigalrm);	/* that can cause hanging in read() */

    if ((flags = fcntl(0, F_GETFL, 0)) == -1)
	fatal(2, "Can't get file mode flags on stdin: %m");

    if (fcntl(0, F_SETFL, flags | O_NONBLOCK) == -1)
	fatal(2, "Can't set file mode flags on stdin: %m");

    if (verbose)
	msgf("alarm");
}

void unalarm()
{
    int flags;

    if ((flags = fcntl(0, F_GETFL, 0)) == -1)
	fatal(2, "Can't get file mode flags on stdin: %m");

    if (fcntl(0, F_SETFL, flags & ~O_NONBLOCK) == -1)
	fatal(2, "Can't set file mode flags on stdin: %m");
}

SIGTYPE sigint(signo)
int signo;
{
    fatal(2, "SIGINT");
}

SIGTYPE sigterm(signo)
int signo;
{
    fatal(2, "SIGTERM");
}

SIGTYPE sighup(signo)
int signo;
{
    fatal(2, "SIGHUP");
}

void init()
{
    signal(SIGINT, sigint);
    signal(SIGTERM, sigterm);
    signal(SIGHUP, sighup);

    set_tty_parameters();
    signal(SIGALRM, sigalrm);
    alarm(0);
    alarmed = 0;
}

void set_tty_parameters()
{
#if defined(get_term_param)
    term_parms t;

    if (get_term_param (&t) < 0)
	fatal(2, "Can't get terminal parameters: %m");

    saved_tty_parameters = t;
    have_tty_parameters  = 1;

    t.c_iflag     |= IGNBRK | ISTRIP | IGNPAR;
    t.c_oflag     |= OPOST | ONLCR;
    t.c_lflag      = 0;
    t.c_cc[VERASE] =
    t.c_cc[VKILL]  = 0;
    t.c_cc[VMIN]   = 1;
    t.c_cc[VTIME]  = 0;

    if (set_term_param (&t) < 0)
	fatal(2, "Can't set terminal parameters: %m");
#endif
}

void break_sequence()
{
#ifdef TERMIOS
    tcsendbreak (0, 0);
#endif
}

void terminate(status)
int status;
{
    static int terminating = 0;

    if (terminating)
	exit(status);
    terminating = 1;
    echo_stderr(-1);
/*
 * Allow the last of the report string to be gathered before we terminate.
 */
    if (report_gathering) {
	int c, rep_len;

	rep_len = strlen(report_buffer);
	while (rep_len < sizeof(report_buffer) - 1) {
	    alarm(1);
	    c = get_char();
	    alarm(0);
	    if (c < 0 || iscntrl(c))
		break;
	    report_buffer[rep_len] = c;
	    ++rep_len;
	}
	report_buffer[rep_len] = 0;
	fprintf (report_fp, "chat:  %s\n", report_buffer);
    }
    if (report_file != (char *) 0 && report_fp != (FILE *) NULL) {
	if (verbose)
	    fprintf (report_fp, "Closing \"%s\".\n", report_file);
	fclose (report_fp);
	report_fp = (FILE *) NULL;
    }

#if defined(get_term_param)
    if (have_tty_parameters) {
	if (set_term_param (&saved_tty_parameters) < 0)
	    fatal(2, "Can't restore terminal parameters: %m");
    }
#endif

    exit(status);
}

/*
 *	'Clean up' this string.
 */
char *clean(s, sending)
char *s;
int sending;  /* set to 1 when sending (putting) this string. */
{
    char cur_chr;
    char *s1, *p, *phchar;
    int add_return = sending;
    size_t len = strlen(s) + 3;		/* see len comments below */

#define isoctal(chr)	(((chr) >= '0') && ((chr) <= '7'))
#define isalnumx(chr)	((((chr) >= '0') && ((chr) <= '9')) \
			 || (((chr) >= 'a') && ((chr) <= 'z')) \
			 || (((chr) >= 'A') && ((chr) <= 'Z')) \
			 || (chr) == '_')

    p = s1 = malloc(len);
    if (!p)
	fatal(2, "memory error!");
    while (*s) {
	cur_chr = *s++;
	if (cur_chr == '^') {
	    cur_chr = *s++;
	    if (cur_chr == '\0') {
		*p++ = '^';
		break;
	    }
	    cur_chr &= 0x1F;
	    if (cur_chr != 0) {
		*p++ = cur_chr;
	    }
	    continue;
	}

	if (use_env && cur_chr == '$') {		/* ARI */
	    char c;

	    phchar = s;
	    while (isalnumx(*s))
		s++;
	    c = *s;		/* save */
	    *s = '\0';
	    phchar = getenv(phchar);
	    *s = c;		/* restore */
	    if (phchar) {
		len += strlen(phchar);
		s1 = grow(s1, &p, len);
		while (*phchar)
		    *p++ = *phchar++;
	    }
	    continue;
	}

	if (cur_chr != '\\') {
	    *p++ = cur_chr;
	    continue;
	}

	cur_chr = *s++;
	if (cur_chr == '\0') {
	    if (sending) {
		*p++ = '\\';
		*p++ = '\\';	/* +1 for len */
	    }
	    break;
	}

	switch (cur_chr) {
	case 'b':
	    *p++ = '\b';
	    break;

	case 'c':
	    if (sending && *s == '\0')
		add_return = 0;
	    else
		*p++ = cur_chr;
	    break;

	case '\\':
	case 'K':
	case 'p':
	case 'd':
	    if (sending)
		*p++ = '\\';
	    *p++ = cur_chr;
	    break;

	case 'T':
	    if (sending && phone_num) {
		len += strlen(phone_num);
		s1 = grow(s1, &p, len);
		for (phchar = phone_num; *phchar != '\0'; phchar++) 
		    *p++ = *phchar;
	    }
	    else {
		*p++ = '\\';
		*p++ = 'T';
	    }
	    break;

	case 'U':
	    if (sending && phone_num2) {
		len += strlen(phone_num2);
		s1 = grow(s1, &p, len);
		for (phchar = phone_num2; *phchar != '\0'; phchar++) 
		    *p++ = *phchar;
	    }
	    else {
		*p++ = '\\';
		*p++ = 'U';
	    }
	    break;

	case 'q':
	    quiet = 1;
	    break;

	case 'r':
	    *p++ = '\r';
	    break;

	case 'n':
	    *p++ = '\n';
	    break;

	case 's':
	    *p++ = ' ';
	    break;

	case 't':
	    *p++ = '\t';
	    break;

	case 'N':
	    if (sending) {
		*p++ = '\\';
		*p++ = '\0';
	    }
	    else
		*p++ = 'N';
	    break;

	case '$':			/* ARI */
	    if (use_env) {
		*p++ = cur_chr;
		break;
	    }
	    /* FALL THROUGH */

	default:
	    if (isoctal (cur_chr)) {
		cur_chr &= 0x07;
		if (isoctal (*s)) {
		    cur_chr <<= 3;
		    cur_chr |= *s++ - '0';
		    if (isoctal (*s)) {
			cur_chr <<= 3;
			cur_chr |= *s++ - '0';
		    }
		}

		if (cur_chr != 0 || sending) {
		    if (sending && (cur_chr == '\\' || cur_chr == 0))
			*p++ = '\\';
		    *p++ = cur_chr;
		}
		break;
	    }

	    if (sending)
		*p++ = '\\';
	    *p++ = cur_chr;
	    break;
	}
    }

    if (add_return)
	*p++ = '\r';	/* +2 for len */

    *p = '\0';		/* +3 for len */
    return s1;
}

/*
 * A modified version of 'strtok'. This version skips \ sequences.
 */

char *expect_strtok (s, term)
     char *s, *term;
{
    static  char *str   = "";
    int	    escape_flag = 0;
    char   *result;

/*
 * If a string was specified then do initial processing.
 */
    if (s)
	str = s;

/*
 * If this is the escape flag then reset it and ignore the character.
 */
    if (*str)
	result = str;
    else
	result = (char *) 0;

    while (*str) {
	if (escape_flag) {
	    escape_flag = 0;
	    ++str;
	    continue;
	}

	if (*str == '\\') {
	    ++str;
	    escape_flag = 1;
	    continue;
	}

/*
 * If this is not in the termination string, continue.
 */
	if (strchr (term, *str) == (char *) 0) {
	    ++str;
	    continue;
	}

/*
 * This is the terminator. Mark the end of the string and stop.
 */
	*str++ = '\0';
	break;
    }
    return (result);
}

/*
 * Process the expect string
 */

void chat_expect (s)
char *s;
{
    char *expect;
    char *reply;

    if (strcmp(s, "HANGUP") == 0) {
	++hup_next;
        return;
    }
 
    if (strcmp(s, "ABORT") == 0) {
	++abort_next;
	return;
    }

    if (strcmp(s, "CLR_ABORT") == 0) {
	++clear_abort_next;
	return;
    }

    if (strcmp(s, "REPORT") == 0) {
	++report_next;
	return;
    }

    if (strcmp(s, "CLR_REPORT") == 0) {
	++clear_report_next;
	return;
    }

    if (strcmp(s, "TIMEOUT") == 0) {
	++timeout_next;
	return;
    }

    if (strcmp(s, "ECHO") == 0) {
	++echo_next;
	return;
    }

    if (strcmp(s, "SAY") == 0) {
	++say_next;
	return;
    }

/*
 * Fetch the expect and reply string.
 */
    for (;;) {
	expect = expect_strtok (s, "-");
	s      = (char *) 0;

	if (expect == (char *) 0)
	    return;

	reply = expect_strtok (s, "-");

/*
 * Handle the expect string. If successful then exit.
 */
	if (get_string (expect))
	    return;

/*
 * If there is a sub-reply string then send it. Otherwise any condition
 * is terminal.
 */
	if (reply == (char *) 0 || exit_code != 3)
	    break;

	chat_send (reply);
    }

/*
 * The expectation did not occur. This is terminal.
 */
    if (fail_reason)
	msgf("Failed (%s)", fail_reason);
    else
	msgf("Failed");
    terminate(exit_code);
}

/*
 * Translate the input character to the appropriate string for printing
 * the data.
 */

char *character(c)
int c;
{
    static char string[10];
    char *meta;

    meta = (c & 0x80) ? "M-" : "";
    c &= 0x7F;

    if (c < 32)
	snprintf(string, sizeof(string), "%s^%c", meta, (int)c + '@');
    else if (c == 127)
	snprintf(string, sizeof(string), "%s^?", meta);
    else
	snprintf(string, sizeof(string), "%s%c", meta, c);

    return (string);
}

/*
 *  process the reply string
 */
void chat_send (s)
char *s;
{
    char file_data[STR_LEN];

    if (say_next) {
	say_next = 0;
	s = clean(s, 1);
	write(2, s, strlen(s));
        free(s);
	return;
    }

    if (hup_next) {
        hup_next = 0;
	if (strcmp(s, "OFF") == 0)
           signal(SIGHUP, SIG_IGN);
        else
           signal(SIGHUP, sighup);
        return;
    }

    if (echo_next) {
	echo_next = 0;
	echo = (strcmp(s, "ON") == 0);
	return;
    }

    if (abort_next) {
	char *s1;
	
	abort_next = 0;
	
	if (n_aborts >= MAX_ABORTS)
	    fatal(2, "Too many ABORT strings");
	
	s1 = clean(s, 0);
	
	if (strlen(s1) > strlen(s)
	    || strlen(s1) + 1 > sizeof(fail_buffer))
	    fatal(1, "Illegal or too-long ABORT string ('%v')", s);

	abort_string[n_aborts++] = s1;

	if (verbose)
	    msgf("abort on (%v)", s);
	return;
    }

    if (clear_abort_next) {
	char *s1;
	int   i;
        int   old_max;
	int   pack = 0;
	
	clear_abort_next = 0;
	
	s1 = clean(s, 0);
	
	if (strlen(s1) > strlen(s)
	    || strlen(s1) + 1 > sizeof(fail_buffer))
	    fatal(1, "Illegal or too-long CLR_ABORT string ('%v')", s);

        old_max = n_aborts;
	for (i=0; i < n_aborts; i++) {
	    if ( strcmp(s1,abort_string[i]) == 0 ) {
		free(abort_string[i]);
		abort_string[i] = NULL;
		pack++;
		n_aborts--;
		if (verbose)
		    msgf("clear abort on (%v)", s);
	    }
	}
        free(s1);
	if (pack)
	    pack_array(abort_string,old_max);
	return;
    }

    if (report_next) {
	char *s1;
	
	report_next = 0;
	if (n_reports >= MAX_REPORTS)
	    fatal(2, "Too many REPORT strings");
	
	s1 = clean(s, 0);
	if (strlen(s1) > strlen(s)
	    || strlen(s1) + 1 > sizeof(fail_buffer))
	    fatal(1, "Illegal or too-long REPORT string ('%v')", s);
	
	report_string[n_reports++] = s1;
	
	if (verbose)
	    msgf("report (%v)", s);
	return;
    }

    if (clear_report_next) {
	char *s1;
	int   i;
	int   old_max;
	int   pack = 0;
	
	clear_report_next = 0;
	
	s1 = clean(s, 0);
	
	if (strlen(s1) > strlen(s)
	    || strlen(s1) + 1 > sizeof(fail_buffer))
	    fatal(1, "Illegal or too-long REPORT string ('%v')", s);

	old_max = n_reports;
	for (i=0; i < n_reports; i++) {
	    if ( strcmp(s1,report_string[i]) == 0 ) {
		free(report_string[i]);
		report_string[i] = NULL;
		pack++;
		n_reports--;
		if (verbose)
		    msgf("clear report (%v)", s);
	    }
	}
        free(s1);
        if (pack)
	    pack_array(report_string,old_max);
	
	return;
    }

    if (timeout_next) {
	timeout_next = 0;
	s = clean(s, 0);
	timeout = atoi(s);
	free(s);
	
	if (timeout <= 0)
	    timeout = DEFAULT_CHAT_TIMEOUT;

	if (verbose)
	    msgf("timeout set to %d seconds", timeout);

	return;
    }

    /*
     * The syntax @filename means read the string to send from the
     * file `filename'.
     */
    if (s[0] == '@') {
	/* skip the @ and any following white-space */
	char *fn = s;
	while (*++fn == ' ' || *fn == '\t')
	    ;

	if (*fn != 0) {
	    FILE *f;
	    int n = 0;

	    /* open the file and read until STR_LEN-1 bytes or end-of-file */
	    f = fopen(fn, "r");
	    if (f == NULL)
		fatal(1, "%s -- open failed: %m", fn);
	    while (n < STR_LEN - 1) {
		int nr = fread(&file_data[n], 1, STR_LEN - 1 - n, f);
		if (nr < 0)
		    fatal(1, "%s -- read error", fn);
		if (nr == 0)
		    break;
		n += nr;
	    }
	    fclose(f);

	    /* use the string we got as the string to send,
	       but trim off the final newline if any. */
	    if (n > 0 && file_data[n-1] == '\n')
		--n;
	    file_data[n] = 0;
	    s = file_data;
	}
    }

    if (strcmp(s, "EOT") == 0)
	s = "^D\\c";
    else if (strcmp(s, "BREAK") == 0)
	s = "\\K\\c";

    if (!put_string(s))
	fatal(1, "Failed");
}

int get_char()
{
    int status;
    char c;

    status = read(0, &c, 1);

    switch (status) {
    case 1:
	return ((int)c & 0x7F);

    default:
	msgf("warning: read() on stdin returned %d", status);

    case -1:
	if ((status = fcntl(0, F_GETFL, 0)) == -1)
	    fatal(2, "Can't get file mode flags on stdin: %m");

	if (fcntl(0, F_SETFL, status & ~O_NONBLOCK) == -1)
	    fatal(2, "Can't set file mode flags on stdin: %m");
	
	return (-1);
    }
}

int put_char(c)
int c;
{
    int status;
    char ch = c;

    usleep(10000);		/* inter-character typing delay (?) */

    status = write(1, &ch, 1);

    switch (status) {
    case 1:
	return (0);
	
    default:
	msgf("warning: write() on stdout returned %d", status);
	
    case -1:
	if ((status = fcntl(0, F_GETFL, 0)) == -1)
	    fatal(2, "Can't get file mode flags on stdin, %m");

	if (fcntl(0, F_SETFL, status & ~O_NONBLOCK) == -1)
	    fatal(2, "Can't set file mode flags on stdin: %m");
	
	return (-1);
    }
}

int write_char (c)
int c;
{
    if (alarmed || put_char(c) < 0) {
	alarm(0);
	alarmed = 0;

	if (verbose) {
	    if (errno == EINTR || errno == EWOULDBLOCK)
		msgf(" -- write timed out");
	    else
		msgf(" -- write failed: %m");
	}
	return (0);
    }
    return (1);
}

int put_string (s)
char *s;
{
	char *ss;
    quiet = 0;
    s = ss = clean(s, 1);

    if (verbose) {
	if (quiet)
	    msgf("send (?????\?)");
	else
	    msgf("send (%v)", s);
    }

    alarm(timeout); alarmed = 0;

    while (*s) {
	char c = *s++;

	if (c != '\\') {
	    if (!write_char (c)) {
		free(ss);
		return 0;
	    }
	    continue;
	}

	c = *s++;
	switch (c) {
	case 'd':
	    sleep(1);
	    break;

	case 'K':
	    break_sequence();
	    break;

	case 'p':
	    usleep(10000); 	/* 1/100th of a second (arg is microseconds) */
	    break;

	default:
	    if (!write_char (c)) {
		free(ss);
		return 0;
	    }
	    break;
	}
    }

    alarm(0);
    alarmed = 0;
    free(ss);
    return (1);
}

/*
 *	Echo a character to stderr.
 *	When called with -1, a '\n' character is generated when
 *	the cursor is not at the beginning of a line.
 */
void echo_stderr(n)
int n;
{
    static int need_lf;
    char *s;

    switch (n) {
    case '\r':		/* ignore '\r' */
	break;
    case -1:
	if (need_lf == 0)
	    break;
	/* fall through */
    case '\n':
	write(2, "\n", 1);
	need_lf = 0;
	break;
    default:
	s = character(n);
	write(2, s, strlen(s));
	need_lf = 1;
	break;
    }
}

/*
 *	'Wait for' this string to appear on this file descriptor.
 */
int get_string(string)
char *string;
{
    char temp[STR_LEN];
    int c, len, minlen;
    char *s = temp, *end = s + STR_LEN;
    char *logged = temp;

    fail_reason = (char *)0;
    string = clean(string, 0);
    len = strlen(string);
    minlen = (len > sizeof(fail_buffer)? len: sizeof(fail_buffer)) - 1;

    if (verbose)
	msgf("expect (%v)", string);

    if (len > STR_LEN) {
	msgf("expect string is too long");
	exit_code = 1;
	free(string);
	return 0;
    }

    if (len == 0) {
	if (verbose)
	    msgf("got it");
	free(string);
	return (1);
    }

    alarm(timeout);
    alarmed = 0;

    while ( ! alarmed && (c = get_char()) >= 0) {
	int n, abort_len, report_len;

	if (echo)
	    echo_stderr(c);
	if (verbose && c == '\n') {
	    if (s == logged)
		msgf("");	/* blank line */
	    else
		msgf("%0.*v", s - logged, logged);
	    logged = s + 1;
	}

	*s++ = c;

	if (verbose && s >= logged + 80) {
	    msgf("%0.*v", s - logged, logged);
	    logged = s;
	}

	if (Verbose) {
	   if (c == '\n')
	       fputc( '\n', stderr );
	   else if (c != '\r')
	       fprintf( stderr, "%s", character(c) );
	}

	if (!report_gathering) {
	    for (n = 0; n < n_reports; ++n) {
		if ((report_string[n] != (char*) NULL) &&
		    s - temp >= (report_len = strlen(report_string[n])) &&
		    strncmp(s - report_len, report_string[n], report_len) == 0) {
		    time_t time_now   = time ((time_t*) NULL);
		    struct tm* tm_now = localtime (&time_now);

		    strftime (report_buffer, 20, "%b %d %H:%M:%S ", tm_now);
		    strcat (report_buffer, report_string[n]);
		    strlcat(report_buffer, report_string[n],
		      sizeof(report_buffer));

		    report_string[n] = (char *) NULL;
		    report_gathering = 1;
		    break;
		}
	    }
	}
	else {
	    if (!iscntrl (c)) {
		int rep_len = strlen (report_buffer);
		report_buffer[rep_len]     = c;
		report_buffer[rep_len + 1] = '\0';
	    }
	    else {
		report_gathering = 0;
		fprintf (report_fp, "chat:  %s\n", report_buffer);
	    }
	}

	if (s - temp >= len &&
	    c == string[len - 1] &&
	    strncmp(s - len, string, len) == 0) {
	    if (verbose) {
		if (s > logged)
		    msgf("%0.*v", s - logged, logged);
		msgf(" -- got it\n");
	    }

	    alarm(0);
	    alarmed = 0;
	    free(string);
	    return (1);
	}

	for (n = 0; n < n_aborts; ++n) {
	    if (s - temp >= (abort_len = strlen(abort_string[n])) &&
		strncmp(s - abort_len, abort_string[n], abort_len) == 0) {
		if (verbose) {
		    if (s > logged)
			msgf("%0.*v", s - logged, logged);
		    msgf(" -- failed");
		}

		alarm(0);
		alarmed = 0;
		exit_code = n + 4;
		strlcpy(fail_buffer, abort_string[n], sizeof(fail_buffer));
		fail_reason = fail_buffer;
		free(string);
		return (0);
	    }
	}

	if (s >= end) {
	    if (logged < s - minlen) {
		if (verbose)
		    msgf("%0.*v", s - logged, logged);
		logged = s;
	    }
	    s -= minlen;
	    memmove(temp, s, minlen);
	    logged = temp + (logged - s);
	    s = temp + minlen;
	}

	if (alarmed && verbose)
	    msgf("warning: alarm synchronization problem");
    }

    alarm(0);

    exit_code = 3;
    alarmed   = 0;
    free(string);
    return (0);
}

/*
 * Gross kludge to handle Solaris versions >= 2.6 having usleep.
 */
#ifdef SOL2
#include <sys/param.h>
#if MAXUID > 65536		/* then this is Solaris 2.6 or later */
#undef NO_USLEEP
#endif
#endif /* SOL2 */

#ifdef NO_USLEEP
#include <sys/types.h>
#include <sys/time.h>

/*
  usleep -- support routine for 4.2BSD system call emulations
  last edit:  29-Oct-1984     D A Gwyn
  */

extern int	  select();

int
usleep( usec )				  /* returns 0 if ok, else -1 */
    long		usec;		/* delay in microseconds */
{
    static struct {		/* `timeval' */
	long	tv_sec;		/* seconds */
	long	tv_usec;	/* microsecs */
    } delay;	    		/* _select() timeout */

    delay.tv_sec  = usec / 1000000L;
    delay.tv_usec = usec % 1000000L;

    return select(0, (long *)0, (long *)0, (long *)0, &delay);
}
#endif

void
pack_array (array, end)
    char **array; /* The address of the array of string pointers */
    int    end;   /* The index of the next free entry before CLR_ */
{
    int i, j;

    for (i = 0; i < end; i++) {
	if (array[i] == NULL) {
	    for (j = i+1; j < end; ++j)
		if (array[j] != NULL)
		    array[i++] = array[j];
	    for (; i < end; ++i)
		array[i] = NULL;
	    break;
	}
    }
}

/*
 * vfmtmsg - format a message into a buffer.  Like vsprintf except we
 * also specify the length of the output buffer, and we handle the
 * %m (error message) format.
 * Doesn't do floating-point formats.
 * Returns the number of chars put into buf.
 */
#define OUTCHAR(c)	(buflen > 0? (--buflen, *buf++ = (c)): 0)

int
vfmtmsg(buf, buflen, fmt, args)
    char *buf;
    int buflen;
    const char *fmt;
    va_list args;
{
    int c, i, n;
    int width, prec, fillch;
    int base, len, neg, quoted;
    unsigned long val = 0;
    char *str, *buf0;
    const char *f;
    unsigned char *p;
    char num[32];
    static char hexchars[] = "0123456789abcdef";

    buf0 = buf;
    --buflen;
    while (buflen > 0) {
	for (f = fmt; *f != '%' && *f != 0; ++f)
	    ;
	if (f > fmt) {
	    len = f - fmt;
	    if (len > buflen)
		len = buflen;
	    memcpy(buf, fmt, len);
	    buf += len;
	    buflen -= len;
	    fmt = f;
	}
	if (*fmt == 0)
	    break;
	c = *++fmt;
	width = prec = 0;
	fillch = ' ';
	if (c == '0') {
	    fillch = '0';
	    c = *++fmt;
	}
	if (c == '*') {
	    width = va_arg(args, int);
	    c = *++fmt;
	} else {
	    while (isdigit(c)) {
		width = width * 10 + c - '0';
		c = *++fmt;
	    }
	}
	if (c == '.') {
	    c = *++fmt;
	    if (c == '*') {
		prec = va_arg(args, int);
		c = *++fmt;
	    } else {
		while (isdigit(c)) {
		    prec = prec * 10 + c - '0';
		    c = *++fmt;
		}
	    }
	}
	str = 0;
	base = 0;
	neg = 0;
	++fmt;
	switch (c) {
	case 'd':
	    i = va_arg(args, int);
	    if (i < 0) {
		neg = 1;
		val = -i;
	    } else
		val = i;
	    base = 10;
	    break;
	case 'o':
	    val = va_arg(args, unsigned int);
	    base = 8;
	    break;
	case 'x':
	    val = va_arg(args, unsigned int);
	    base = 16;
	    break;
	case 'p':
	    val = (unsigned long) va_arg(args, void *);
	    base = 16;
	    neg = 2;
	    break;
	case 's':
	    str = va_arg(args, char *);
	    break;
	case 'c':
	    num[0] = va_arg(args, int);
	    num[1] = 0;
	    str = num;
	    break;
	case 'm':
	    str = strerror(errno);
	    break;
	case 'v':		/* "visible" string */
	case 'q':		/* quoted string */
	    quoted = c == 'q';
	    p = va_arg(args, unsigned char *);
	    if (fillch == '0' && prec > 0) {
		n = prec;
	    } else {
		n = strlen((char *)p);
		if (prec > 0 && prec < n)
		    n = prec;
	    }
	    while (n > 0 && buflen > 0) {
		c = *p++;
		--n;
		if (!quoted && c >= 0x80) {
		    OUTCHAR('M');
		    OUTCHAR('-');
		    c -= 0x80;
		}
		if (quoted && (c == '"' || c == '\\'))
		    OUTCHAR('\\');
		if (c < 0x20 || (0x7f <= c && c < 0xa0)) {
		    if (quoted) {
			OUTCHAR('\\');
			switch (c) {
			case '\t':	OUTCHAR('t');	break;
			case '\n':	OUTCHAR('n');	break;
			case '\b':	OUTCHAR('b');	break;
			case '\f':	OUTCHAR('f');	break;
			default:
			    OUTCHAR('x');
			    OUTCHAR(hexchars[c >> 4]);
			    OUTCHAR(hexchars[c & 0xf]);
			}
		    } else {
			if (c == '\t')
			    OUTCHAR(c);
			else {
			    OUTCHAR('^');
			    OUTCHAR(c ^ 0x40);
			}
		    }
		} else
		    OUTCHAR(c);
	    }
	    continue;
	default:
	    *buf++ = '%';
	    if (c != '%')
		--fmt;		/* so %z outputs %z etc. */
	    --buflen;
	    continue;
	}
	if (base != 0) {
	    str = num + sizeof(num);
	    *--str = 0;
	    while (str > num + neg) {
		*--str = hexchars[val % base];
		val = val / base;
		if (--prec <= 0 && val == 0)
		    break;
	    }
	    switch (neg) {
	    case 1:
		*--str = '-';
		break;
	    case 2:
		*--str = 'x';
		*--str = '0';
		break;
	    }
	    len = num + sizeof(num) - 1 - str;
	} else {
	    len = strlen(str);
	    if (prec > 0 && len > prec)
		len = prec;
	}
	if (width > 0) {
	    if (width > buflen)
		width = buflen;
	    if ((n = width - len) > 0) {
		buflen -= n;
		for (; n > 0; --n)
		    *buf++ = fillch;
	    }
	}
	if (len > buflen)
	    len = buflen;
	memcpy(buf, str, len);
	buf += len;
	buflen -= len;
    }
    *buf = 0;
    return buf - buf0;
}
