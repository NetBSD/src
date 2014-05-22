/*	$NetBSD: ftp.y,v 1.4.2.1 2014/05/22 15:44:17 yamt Exp $	*/

/*
 * Copyright (c) 1985, 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	from: @(#)ftpcmd.y	5.20.1.1 (Berkeley) 3/2/89
 *	$NetBSD: ftp.y,v 1.4.2.1 2014/05/22 15:44:17 yamt Exp $
 */

/*
 * Grammar for FTP commands.
 * See RFC 959.
 */

%{

#ifndef lint
static char sccsid[] = "@(#)ftpcmd.y	5.20.1.1 (Berkeley) 3/2/89";
static char rcsid[] = "$NetBSD: ftp.y,v 1.4.2.1 2014/05/22 15:44:17 yamt Exp $";
#endif /* not lint */

#include <sys/param.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <arpa/ftp.h>

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <pwd.h>
#include <setjmp.h>
#include <syslog.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#ifdef YYBISON
int yylex(void);
static void yyerror(const char *);
#endif

extern	struct sockaddr_in data_dest;
extern	int logged_in;
extern	struct passwd *pw;
extern	int guest;
extern	int logging;
extern	int type;
extern	int form;
extern	int debug;
extern	int timeout;
extern	int maxtimeout;
extern  int pdata;
extern	char hostname[], remotehost[];
extern	char proctitle[];
extern	char *globerr;
extern	int usedefault;
extern  int transflag;
extern  char tmpline[];

extern char **glob(char *);
extern char *renamefrom(char *);
extern void cwd(const char *);

extern void dologout(int);
extern void fatal(const char *);
extern void makedir(const char *);
extern void nack(const char *);
extern void pass(const char *);
extern void passive(void);
extern void pwd(void);
extern void removedir(char *);
extern void renamecmd(char *, char *);
extern void retrieve(const char *, const char *);
extern void send_file_list(const char *);
extern void statcmd(void);
extern void statfilecmd(const char *);
extern void store(char *, const char *, int);
extern void user(const char *);

extern void perror_reply(int, const char *, ...);
extern void reply(int, const char *, ...);
extern void lreply(int, const char *, ...);

static	int cmd_type;
static	int cmd_form;
static	int cmd_bytesz;
char	cbuf[512];
char	*fromname;

struct tab {
	const char *name;
	short	token;
	short	state;
	short	implemented;	/* 1 if command is implemented */
	const char *help;
};

static char * copy(const char *);

#ifdef YYBISON
static void sizecmd(char *filename);
static void help(struct tab *ctab, char *s);
struct tab cmdtab[];
struct tab sitetab[];
#endif

static void
yyerror(const char *msg)
{
	perror(msg);
}
%}

%union
{
	int ival;
	char *sval;
}
%token <ival> NUMBER
%token <sval> STRING

%type <ival>
	byte_size
	check_login
	form_code
	mode_code
	octal_number
	struct_code

%type <sval>
	password
	pathname
	pathstring
	username

%token
	A	B	C	E	F	I
	L	N	P	R	S	T

	SP	CRLF	COMMA	STRING	NUMBER

	USER	PASS	ACCT	REIN	QUIT	PORT
	PASV	TYPE	STRU	MODE	RETR	STOR
	APPE	MLFL	MAIL	MSND	MSOM	MSAM
	MRSQ	MRCP	ALLO	REST	RNFR	RNTO
	ABOR	DELE	CWD	LIST	NLST	SITE
	STAT	HELP	NOOP	MKD	RMD	PWD
	CDUP	STOU	SMNT	SYST	SIZE	MDTM

	UMASK	IDLE	CHMOD

	LEXERR

%start	cmd_list

%%

cmd_list:	/* empty */
	|	cmd_list cmd
		{
			fromname = (char *) 0;
		}
	|	cmd_list rcmd
	;

cmd:		USER SP username CRLF
		{
			user($3);
			free($3);
		}
	|	PASS SP password CRLF
		{
			pass($3);
			free($3);
		}
	|	PORT SP host_port CRLF
		{
			usedefault = 0;
			if (pdata >= 0) {
				(void) close(pdata);
				pdata = -1;
			}
			reply(200, "PORT command successful.");
		}
	|	PASV CRLF
		{
			passive();
		}
	|	TYPE SP type_code CRLF
		{
			switch (cmd_type) {

			case TYPE_A:
				if (cmd_form == FORM_N) {
					reply(200, "Type set to A.");
					type = cmd_type;
					form = cmd_form;
				} else
					reply(504, "Form must be N.");
				break;

			case TYPE_E:
				reply(504, "Type E not implemented.");
				break;

			case TYPE_I:
				reply(200, "Type set to I.");
				type = cmd_type;
				break;

			case TYPE_L:
#if NBBY == 8
				if (cmd_bytesz == 8) {
					reply(200,
					    "Type set to L (byte size 8).");
					type = cmd_type;
				} else
					reply(504, "Byte size must be 8.");
#else /* NBBY == 8 */
				UNIMPLEMENTED for NBBY != 8
#endif /* NBBY == 8 */
			}
		}
	|	STRU SP struct_code CRLF
		{
			switch ($3) {

			case STRU_F:
				reply(200, "STRU F ok.");
				break;

			default:
				reply(504, "Unimplemented STRU type.");
			}
		}
	|	MODE SP mode_code CRLF
		{
			switch ($3) {

			case MODE_S:
				reply(200, "MODE S ok.");
				break;

			default:
				reply(502, "Unimplemented MODE type.");
			}
		}
	|	ALLO SP NUMBER CRLF
		{
			reply(202, "ALLO command ignored.");
		}
	|	ALLO SP NUMBER SP R SP NUMBER CRLF
		{
			reply(202, "ALLO command ignored.");
		}
	|	RETR check_login SP pathname CRLF
		{
			if ($2 && $4 != 0)
				retrieve((char *) 0, $4);
			if ($4 != 0)
				free($4);
		}
	|	STOR check_login SP pathname CRLF
		{
			if ($2 && $4 != 0)
				store($4, "w", 0);
			if ($4 != 0)
				free($4);
		}
	|	APPE check_login SP pathname CRLF
		{
			if ($2 && $4 != 0)
				store($4, "a", 0);
			if ($4 != 0)
				free($4);
		}
	|	NLST check_login CRLF
		{
			if ($2)
				send_file_list(".");
		}
	|	NLST check_login SP STRING CRLF
		{
			if ($2 && $4 != 0)
				send_file_list((char *) $4);
			if ($4 != 0)
				free((char *) $4);
		}
	|	LIST check_login CRLF
		{
			if ($2)
				retrieve("/bin/ls -lgA", "");
		}
	|	LIST check_login SP pathname CRLF
		{
			if ($2 && $4 != 0)
				retrieve("/bin/ls -lgA %s", $4);
			if ($4 != 0)
				free($4);
		}
	|	STAT check_login SP pathname CRLF
		{
			if ($2 && $4 != 0)
				statfilecmd($4);
			if ($4 != 0)
				free($4);
		}
	|	STAT CRLF
		{
			statcmd();
		}
	|	DELE check_login SP pathname CRLF
		{
			if ($2 && $4 != 0)
				remove((char *) $4);
			if ($4 != 0)
				free((char *) $4);
		}
	|	RNTO SP pathname CRLF
		{
			if (fromname) {
				renamecmd(fromname, (char *) $3);
				free(fromname);
				fromname = (char *) 0;
			} else {
				reply(503, "Bad sequence of commands.");
			}
			free((char *) $3);
		}
	|	ABOR CRLF
		{
			reply(225, "ABOR command successful.");
		}
	|	CWD check_login CRLF
		{
			if ($2)
				cwd(pw->pw_dir);
		}
	|	CWD check_login SP pathname CRLF
		{
			if ($2 && $4 != 0)
				cwd((char *) $4);
			if ($4 != 0)
				free((char *) $4);
		}
	|	HELP CRLF
		{
			help(cmdtab, (char *) 0);
		}
	|	HELP SP STRING CRLF
		{
			register char *cp = (char *)$3;

			if (strncasecmp(cp, "SITE", 4) == 0) {
				cp = (char *)$3 + 4;
				if (*cp == ' ')
					cp++;
				if (*cp)
					help(sitetab, cp);
				else
					help(sitetab, (char *) 0);
			} else
				help(cmdtab, (char *) $3);
		}
	|	NOOP CRLF
		{
			reply(200, "NOOP command successful.");
		}
	|	MKD check_login SP pathname CRLF
		{
			if ($2 && $4 != 0)
				makedir((char *) $4);
			if ($4 != 0)
				free((char *) $4);
		}
	|	RMD check_login SP pathname CRLF
		{
			if ($2 && $4 != 0)
				removedir((char *) $4);
			if ($4 != 0)
				free((char *) $4);
		}
	|	PWD check_login CRLF
		{
			if ($2)
				pwd();
		}
	|	CDUP check_login CRLF
		{
			if ($2)
				cwd("..");
		}
	|	SITE SP HELP CRLF
		{
			help(sitetab, (char *) 0);
		}
	|	SITE SP HELP SP STRING CRLF
		{
			help(sitetab, (char *) $5);
		}
	|	SITE SP UMASK check_login CRLF
		{
			int oldmask;

			if ($4) {
				oldmask = umask(0);
				(void) umask(oldmask);
				reply(200, "Current UMASK is %03o", oldmask);
			}
		}
	|	SITE SP UMASK check_login SP octal_number CRLF
		{
			int oldmask;

			if ($4) {
				if (($6 == -1) || ($6 > 0777)) {
					reply(501, "Bad UMASK value");
				} else {
					oldmask = umask($6);
					reply(200,
					    "UMASK set to %03o (was %03o)",
					    $6, oldmask);
				}
			}
		}
	|	SITE SP CHMOD check_login SP octal_number SP pathname CRLF
		{
			if ($4 && ($8 != 0)) {
				if ($6 > 0777)
					reply(501,
				"CHMOD: Mode value must be between 0 and 0777");
				else if (chmod((char *) $8, $6) < 0)
					perror_reply(550, (char *) $8);
				else
					reply(200, "CHMOD command successful.");
			}
			if ($8 != 0)
				free((char *) $8);
		}
	|	SITE SP IDLE CRLF
		{
			reply(200,
			    "Current IDLE time limit is %d seconds; max %d",
				timeout, maxtimeout);
		}
	|	SITE SP IDLE SP NUMBER CRLF
		{
			if ($5 < 30 || $5 > maxtimeout) {
				reply(501,
			"Maximum IDLE time must be between 30 and %d seconds",
				    maxtimeout);
			} else {
				timeout = $5;
				(void) alarm((unsigned) timeout);
				reply(200,
				    "Maximum IDLE time set to %d seconds",
				    timeout);
			}
		}
	|	STOU check_login SP pathname CRLF
		{
			if ($2 && $4 != 0)
				store((char *) $4, "w", 1);
			if ($4 != 0)
				free((char *) $4);
		}
	|	SYST CRLF
		{
#ifdef unix
#ifdef BSD
			reply(215, "UNIX Type: L%d Version: BSD-%d",
				NBBY, BSD);
#else /* BSD */
			reply(215, "UNIX Type: L%d", NBBY);
#endif /* BSD */
#else /* unix */
			reply(215, "UNKNOWN Type: L%d", NBBY);
#endif /* unix */
		}

		/*
		 * SIZE is not in RFC959, but Postel has blessed it and
		 * it will be in the updated RFC.
		 *
		 * Return size of file in a format suitable for
		 * using with RESTART (we just count bytes).
		 */
	|	SIZE check_login SP pathname CRLF
		{
			if ($2 && $4 != 0)
				sizecmd((char *) $4);
			if ($4 != 0)
				free((char *) $4);
		}

		/*
		 * MDTM is not in RFC959, but Postel has blessed it and
		 * it will be in the updated RFC.
		 *
		 * Return modification time of file as an ISO 3307
		 * style time. E.g. YYYYMMDDHHMMSS or YYYYMMDDHHMMSS.xxx
		 * where xxx is the fractional second (of any precision,
		 * not necessarily 3 digits)
		 */
	|	MDTM check_login SP pathname CRLF
		{
			if ($2 && $4 != 0) {
				struct stat stbuf;
				if (stat((char *) $4, &stbuf) < 0)
					perror_reply(550, "%s", (char *) $4);
				else if ((stbuf.st_mode&S_IFMT) != S_IFREG) {
					reply(550, "%s: not a plain file.",
						(char *) $4);
				} else {
					register struct tm *t;
					t = gmtime(&stbuf.st_mtime);
					reply(213,
					    "%04d%02d%02d%02d%02d%02d",
					    1900 + t->tm_year,
					    t->tm_mon+1, t->tm_mday,
					    t->tm_hour, t->tm_min, t->tm_sec);
				}
			}
			if ($4 != 0)
				free((char *) $4);
		}
	|	QUIT CRLF
		{
			reply(221, "Goodbye.");
			dologout(0);
		}
	|	error CRLF
		{
			yyerrok;
		}
	;
rcmd:		RNFR check_login SP pathname CRLF
		{
			if ($2 && $4) {
				fromname = renamefrom((char *) $4);
				if (fromname == (char *) 0 && $4) {
					free((char *) $4);
				}
			}
		}
	;

username:	STRING
	;

password:	/* empty */
		{
			*(const char **)(&($$)) = "";
		}
	|	STRING
	;

byte_size:	NUMBER
	;

host_port:	NUMBER COMMA NUMBER COMMA NUMBER COMMA NUMBER COMMA
		NUMBER COMMA NUMBER
		{
			register char *a, *p;

			a = (char *)&data_dest.sin_addr;
			a[0] = (char) $1;
			a[1] = (char) $3;
			a[2] = (char) $5;
			a[3] = (char) $7;
			p = (char *)&data_dest.sin_port;
			p[0] = (char) $9;
			p[1] = (char) $11;
			data_dest.sin_family = AF_INET;
		}
	;

form_code:	N
	{
		$$ = FORM_N;
	}
	|	T
	{
		$$ = FORM_T;
	}
	|	C
	{
		$$ = FORM_C;
	}
	;

type_code:	A
	{
		cmd_type = TYPE_A;
		cmd_form = FORM_N;
	}
	|	A SP form_code
	{
		cmd_type = TYPE_A;
		cmd_form = $3;
	}
	|	E
	{
		cmd_type = TYPE_E;
		cmd_form = FORM_N;
	}
	|	E SP form_code
	{
		cmd_type = TYPE_E;
		cmd_form = $3;
	}
	|	I
	{
		cmd_type = TYPE_I;
	}
	|	L
	{
		cmd_type = TYPE_L;
		cmd_bytesz = NBBY;
	}
	|	L SP byte_size
	{
		cmd_type = TYPE_L;
		cmd_bytesz = $3;
	}
	/* this is for a bug in the BBN ftp */
	|	L byte_size
	{
		cmd_type = TYPE_L;
		cmd_bytesz = $2;
	}
	;

struct_code:	F
	{
		$$ = STRU_F;
	}
	|	R
	{
		$$ = STRU_R;
	}
	|	P
	{
		$$ = STRU_P;
	}
	;

mode_code:	S
	{
		$$ = MODE_S;
	}
	|	B
	{
		$$ = MODE_B;
	}
	|	C
	{
		$$ = MODE_C;
	}
	;

pathname:	pathstring
	{
		/*
		 * Problem: this production is used for all pathname
		 * processing, but only gives a 550 error reply.
		 * This is a valid reply in some cases but not in others.
		 */
		if (logged_in && $1 && strncmp((char *) $1, "~", 1) == 0) {
			*(char **)&($$) = *glob((char *) $1);
			if (globerr != 0) {
				reply(550, globerr);
				$$ = 0;
			}
			free((char *) $1);
		} else
			$$ = $1;
	}
	;

pathstring:	STRING
	;

octal_number:	NUMBER
	{
		register int ret, dec, multby, digit;

		/*
		 * Convert a number that was read as decimal number
		 * to what it would be if it had been read as octal.
		 */
		dec = $1;
		multby = 1;
		ret = 0;
		while (dec) {
			digit = dec%10;
			if (digit > 7) {
				ret = -1;
				break;
			}
			ret += digit * multby;
			multby *= 8;
			dec /= 10;
		}
		$$ = ret;
	}
	;

check_login:	/* empty */
	{
		if (logged_in)
			$$ = 1;
		else {
			reply(530, "Please login with USER and PASS.");
			$$ = 0;
		}
	}
	;

%%

#ifdef YYBYACC
extern int YYLEX_DECL();
#endif

extern jmp_buf errcatch;

static void upper(char *);

#define	CMD	0	/* beginning of command */
#define	ARGS	1	/* expect miscellaneous arguments */
#define	STR1	2	/* expect SP followed by STRING */
#define	STR2	3	/* expect STRING */
#define	OSTR	4	/* optional SP then STRING */
#define	ZSTR1	5	/* SP then optional STRING */
#define	ZSTR2	6	/* optional STRING after SP */
#define	SITECMD	7	/* SITE command */
#define	NSTR	8	/* Number followed by a string */

struct tab cmdtab[] = {		/* In order defined in RFC 765 */
	{ "USER", USER, STR1, 1,	"<sp> username" },
	{ "PASS", PASS, ZSTR1, 1,	"<sp> password" },
	{ "ACCT", ACCT, STR1, 0,	"(specify account)" },
	{ "SMNT", SMNT, ARGS, 0,	"(structure mount)" },
	{ "REIN", REIN, ARGS, 0,	"(reinitialize server state)" },
	{ "QUIT", QUIT, ARGS, 1,	"(terminate service)", },
	{ "PORT", PORT, ARGS, 1,	"<sp> b0, b1, b2, b3, b4" },
	{ "PASV", PASV, ARGS, 1,	"(set server in passive mode)" },
	{ "TYPE", TYPE, ARGS, 1,	"<sp> [ A | E | I | L ]" },
	{ "STRU", STRU, ARGS, 1,	"(specify file structure)" },
	{ "MODE", MODE, ARGS, 1,	"(specify transfer mode)" },
	{ "RETR", RETR, STR1, 1,	"<sp> file-name" },
	{ "STOR", STOR, STR1, 1,	"<sp> file-name" },
	{ "APPE", APPE, STR1, 1,	"<sp> file-name" },
	{ "MLFL", MLFL, OSTR, 0,	"(mail file)" },
	{ "MAIL", MAIL, OSTR, 0,	"(mail to user)" },
	{ "MSND", MSND, OSTR, 0,	"(mail send to terminal)" },
	{ "MSOM", MSOM, OSTR, 0,	"(mail send to terminal or mailbox)" },
	{ "MSAM", MSAM, OSTR, 0,	"(mail send to terminal and mailbox)" },
	{ "MRSQ", MRSQ, OSTR, 0,	"(mail recipient scheme question)" },
	{ "MRCP", MRCP, STR1, 0,	"(mail recipient)" },
	{ "ALLO", ALLO, ARGS, 1,	"allocate storage (vacuously)" },
	{ "REST", REST, ARGS, 0,	"(restart command)" },
	{ "RNFR", RNFR, STR1, 1,	"<sp> file-name" },
	{ "RNTO", RNTO, STR1, 1,	"<sp> file-name" },
	{ "ABOR", ABOR, ARGS, 1,	"(abort operation)" },
	{ "DELE", DELE, STR1, 1,	"<sp> file-name" },
	{ "CWD",  CWD,  OSTR, 1,	"[ <sp> directory-name ]" },
	{ "XCWD", CWD,	OSTR, 1,	"[ <sp> directory-name ]" },
	{ "LIST", LIST, OSTR, 1,	"[ <sp> path-name ]" },
	{ "NLST", NLST, OSTR, 1,	"[ <sp> path-name ]" },
	{ "SITE", SITE, SITECMD, 1,	"site-cmd [ <sp> arguments ]" },
	{ "SYST", SYST, ARGS, 1,	"(get type of operating system)" },
	{ "STAT", STAT, OSTR, 1,	"[ <sp> path-name ]" },
	{ "HELP", HELP, OSTR, 1,	"[ <sp> <string> ]" },
	{ "NOOP", NOOP, ARGS, 1,	"" },
	{ "MKD",  MKD,  STR1, 1,	"<sp> path-name" },
	{ "XMKD", MKD,  STR1, 1,	"<sp> path-name" },
	{ "RMD",  RMD,  STR1, 1,	"<sp> path-name" },
	{ "XRMD", RMD,  STR1, 1,	"<sp> path-name" },
	{ "PWD",  PWD,  ARGS, 1,	"(return current directory)" },
	{ "XPWD", PWD,  ARGS, 1,	"(return current directory)" },
	{ "CDUP", CDUP, ARGS, 1,	"(change to parent directory)" },
	{ "XCUP", CDUP, ARGS, 1,	"(change to parent directory)" },
	{ "STOU", STOU, STR1, 1,	"<sp> file-name" },
	{ "SIZE", SIZE, OSTR, 1,	"<sp> path-name" },
	{ "MDTM", MDTM, OSTR, 1,	"<sp> path-name" },
	{ 0,   0,    0,    0,	0 }
};

struct tab sitetab[] = {
	{ "UMASK", UMASK, ARGS, 1,	"[ <sp> umask ]" },
	{ "IDLE", IDLE, ARGS, 1,	"[ <sp> maximum-idle-time ]" },
	{ "CHMOD", CHMOD, NSTR, 1,	"<sp> mode <sp> file-name" },
	{ "HELP", HELP, OSTR, 1,	"[ <sp> <string> ]" },
	{ 0,   0,    0,    0,	0 }
};

static struct tab *
lookup(struct tab *p, char *cmd)
{

	for (; p->name != 0; p++)
		if (strcmp(cmd, p->name) == 0)
			return (p);
	return (0);
}

#include <arpa/telnet.h>

/*
 * get_line - a hacked up version of fgets to ignore TELNET escape codes.
 */
static char *
get_line(char *s, int n, FILE *iop)
{
	register int c;
	register char *cs;

	cs = s;
/* tmpline may contain saved command from urgent mode interruption */
	for (c = 0; tmpline[c] != '\0' && --n > 0; ++c) {
		*cs++ = tmpline[c];
		if (tmpline[c] == '\n') {
			*cs = '\0';
			if (debug)
				syslog(LOG_DEBUG, "command: %s", s);
			tmpline[0] = '\0';
			return(s);
		}
		if (c == 0)
			tmpline[0] = '\0';
	}
	while ((c = getc(iop)) != EOF) {
		c &= 0377;
		if (c == IAC) {
		    if ((c = getc(iop)) != EOF) {
			c &= 0377;
			switch (c) {
			case WILL:
			case WONT:
				c = getc(iop);
				printf("%c%c%c", IAC, DONT, 0377&c);
				(void) fflush(stdout);
				continue;
			case DO:
			case DONT:
				c = getc(iop);
				printf("%c%c%c", IAC, WONT, 0377&c);
				(void) fflush(stdout);
				continue;
			case IAC:
				break;
			default:
				continue;	/* ignore command */
			}
		    }
		}
		*cs++ = (char) c;
		if (--n <= 0 || c == '\n')
			break;
	}
	if (c == EOF && cs == s)
		return (0);
	*cs = '\0';
	if (debug)
		syslog(LOG_DEBUG, "command: %s", s);
	return (s);
}

static void
toolong(int sig)
{
	time_t now;

	(void) sig;
	reply(421,
	  "Timeout (%d seconds): closing control connection.", timeout);
	(void) time(&now);
	if (logging) {
		syslog(LOG_INFO,
			"User %s timed out after %d seconds at %s",
			(pw ? pw -> pw_name : "unknown"), timeout, ctime(&now));
	}
	dologout(1);
}

int
yylex(void)
{
	static int cpos, state;
	register char *cp, *cp2;
	register struct tab *p;
	int n;
	char c;

	for (;;) {
		switch (state) {

		case CMD:
			(void) signal(SIGALRM, toolong);
			(void) alarm((unsigned) timeout);
			if (get_line(cbuf, sizeof(cbuf)-1, stdin) == 0) {
				reply(221, "You could at least say goodbye.");
				dologout(0);
			}
			(void) alarm(0);
#ifdef SETPROCTITLE
			if (strncasecmp(cbuf, "PASS", 4) != 0)
				setproctitle("%s: %s", proctitle, cbuf);
#endif /* SETPROCTITLE */
			if ((cp = strchr(cbuf, '\r'))) {
				*cp++ = '\n';
				*cp = '\0';
			}
			if ((cp = strpbrk(cbuf, " \n")))
				cpos = (int) (cp - cbuf);
			if (cpos == 0)
				cpos = 4;
			c = cbuf[cpos];
			cbuf[cpos] = '\0';
			upper(cbuf);
			p = lookup(cmdtab, cbuf);
			cbuf[cpos] = c;
			if (p != 0) {
				if (p->implemented == 0) {
					nack(p->name);
					longjmp(errcatch,0);
					/* NOTREACHED */
				}
				state = p->state;
				*(const char **)(&yylval) = p->name;
				return (p->token);
			}
			break;

		case SITECMD:
			if (cbuf[cpos] == ' ') {
				cpos++;
				return (SP);
			}
			cp = &cbuf[cpos];
			if ((cp2 = strpbrk(cp, " \n")))
				cpos = (int) (cp2 - cbuf);
			c = cbuf[cpos];
			cbuf[cpos] = '\0';
			upper(cp);
			p = lookup(sitetab, cp);
			cbuf[cpos] = c;
			if (p != 0) {
				if (p->implemented == 0) {
					state = CMD;
					nack(p->name);
					longjmp(errcatch,0);
					/* NOTREACHED */
				}
				state = p->state;
				*(const char **)(&yylval) = p->name;
				return (p->token);
			}
			state = CMD;
			break;

		case OSTR:
			if (cbuf[cpos] == '\n') {
				state = CMD;
				return (CRLF);
			}
			/* FALLTHROUGH */

		case STR1:
		case ZSTR1:
		dostr1:
			if (cbuf[cpos] == ' ') {
				cpos++;
				if (state == OSTR)
					state = STR2;
				else
					++state;
				return (SP);
			}
			break;

		case ZSTR2:
			if (cbuf[cpos] == '\n') {
				state = CMD;
				return (CRLF);
			}
			/* FALLTHROUGH */

		case STR2:
			cp = &cbuf[cpos];
			n = (int) strlen(cp);
			cpos += n - 1;
			/*
			 * Make sure the string is nonempty and \n terminated.
			 */
			if (n > 1 && cbuf[cpos] == '\n') {
				cbuf[cpos] = '\0';
				*(char **)&yylval = copy(cp);
				cbuf[cpos] = '\n';
				state = ARGS;
				return (STRING);
			}
			break;

		case NSTR:
			if (cbuf[cpos] == ' ') {
				cpos++;
				return (SP);
			}
			if (isdigit(cbuf[cpos])) {
				cp = &cbuf[cpos];
				while (isdigit(cbuf[++cpos]))
					;
				c = cbuf[cpos];
				cbuf[cpos] = '\0';
				yylval.ival = atoi(cp);
				cbuf[cpos] = c;
				state = STR1;
				return (NUMBER);
			}
			state = STR1;
			goto dostr1;

		case ARGS:
			if (isdigit(cbuf[cpos])) {
				cp = &cbuf[cpos];
				while (isdigit(cbuf[++cpos]))
					;
				c = cbuf[cpos];
				cbuf[cpos] = '\0';
				yylval.ival = atoi(cp);
				cbuf[cpos] = c;
				return (NUMBER);
			}
			switch (cbuf[cpos++]) {

			case '\n':
				state = CMD;
				return (CRLF);

			case ' ':
				return (SP);

			case ',':
				return (COMMA);

			case 'A':
			case 'a':
				return (A);

			case 'B':
			case 'b':
				return (B);

			case 'C':
			case 'c':
				return (C);

			case 'E':
			case 'e':
				return (E);

			case 'F':
			case 'f':
				return (F);

			case 'I':
			case 'i':
				return (I);

			case 'L':
			case 'l':
				return (L);

			case 'N':
			case 'n':
				return (N);

			case 'P':
			case 'p':
				return (P);

			case 'R':
			case 'r':
				return (R);

			case 'S':
			case 's':
				return (S);

			case 'T':
			case 't':
				return (T);

			}
			break;

		default:
			fatal("Unknown state in scanner.");
		}
		yyerror((char *) 0);
		state = CMD;
		longjmp(errcatch,0);
	}
}

static void
upper(char *s)
{
	while (*s != '\0') {
		if (islower(*s))
			*s = toupper(*s);
		s++;
	}
}

static char *
copy(const char *s)
{
	char *p;

	p = (char * )malloc(strlen(s) + 1);
	if (p == 0)
		fatal("Ran out of memory.");
	else
		(void) strcpy(p, s);
	return (p);
}

static void
help(struct tab *ctab, char *s)
{
	register struct tab *c;
	register int width, NCMDS;
	const char *help_type;

	if (ctab == sitetab)
		help_type = "SITE ";
	else
		help_type = "";
	width = 0, NCMDS = 0;
	for (c = ctab; c->name != 0; c++) {
		int len = (int) strlen(c->name);

		if (len > width)
			width = len;
		NCMDS++;
	}
	width = (width + 8) &~ 7;
	if (s == 0) {
		register int i, j, w;
		int columns, lines;

		lreply(214, "The following %scommands are recognized %s.",
		    help_type, "(* =>'s unimplemented)");
		columns = 76 / width;
		if (columns == 0)
			columns = 1;
		lines = (NCMDS + columns - 1) / columns;
		for (i = 0; i < lines; i++) {
			printf("   ");
			for (j = 0; j < columns; j++) {
				c = ctab + j * lines + i;
				assert(c->name != 0);
				printf("%s%c", c->name,
					c->implemented ? ' ' : '*');
				if (c + lines >= &ctab[NCMDS])
					break;
				w = (int) strlen(c->name) + 1;
				while (w < width) {
					putchar(' ');
					w++;
				}
			}
			printf("\r\n");
		}
		(void) fflush(stdout);
		reply(214, "Direct comments to ftp-bugs@%s.", hostname);
		return;
	}
	upper(s);
	c = lookup(ctab, s);
	if (c == (struct tab *)0) {
		reply(502, "Unknown command %s.", s);
		return;
	}
	if (c->implemented)
		reply(214, "Syntax: %s%s %s", help_type, c->name, c->help);
	else
		reply(214, "%s%-*s\t%s; unimplemented.", help_type, width,
		    c->name, c->help);
}

static void
sizecmd(char *filename)
{
	switch (type) {
	case TYPE_L:
	case TYPE_I: {
		struct stat stbuf;
		if (stat(filename, &stbuf) < 0 ||
		    (stbuf.st_mode&S_IFMT) != S_IFREG)
			reply(550, "%s: not a plain file.", filename);
		else
#ifdef HAVE_LONG_LONG
			reply(213, "%llu", (long long) stbuf.st_size);
#else
			reply(213, "%lu", stbuf.st_size);
#endif
		break;}
	case TYPE_A: {
		FILE *fin;
		register int c, count;
		struct stat stbuf;
		fin = fopen(filename, "r");
		if (fin == 0) {
			perror_reply(550, filename);
			return;
		}
		if (fstat(fileno(fin), &stbuf) < 0 ||
		    (stbuf.st_mode&S_IFMT) != S_IFREG) {
			reply(550, "%s: not a plain file.", filename);
			(void) fclose(fin);
			return;
		}

		count = 0;
		while((c=getc(fin)) != EOF) {
			if (c == '\n')	/* will get expanded to \r\n */
				count++;
			count++;
		}
		(void) fclose(fin);

		reply(213, "%ld", count);
		break;}
	default:
		reply(504, "SIZE not implemented for Type %c.", "?AEIL"[type]);
	}
}
