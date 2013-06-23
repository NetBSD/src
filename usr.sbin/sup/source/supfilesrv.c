/*	$NetBSD: supfilesrv.c,v 1.47.8.1 2013/06/23 06:29:06 tls Exp $	*/

/*
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 *
 */
/*
 * supfilesrv -- SUP File Server
 *
 * Usage:  supfilesrv [-d] [-l] [-P] [-N] [-R] [-S]
 *	-d	"debug" -- don't fork daemon
 *	-l	"log" -- print successull connects (when compiled with libwrap)
 *	-P	"debug ports" -- use debugging network ports
 *	-N	"debug network" -- print debugging messages for network i/o
 *	-R	"RCS mode" -- if file is an rcs file, use co to get contents
 *	-S	"Operate silently" -- Only print error messages
 *
 **********************************************************************
 * HISTORY
 * 2-Aug-99   Manuel Bouyer at LIP6
 *	Added libwrap support
 *
 * 13-Sep-92  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Changed name of sup program in xpatch from /usr/cs/bin/sup to
 *	/usr/bin/sup for exported version of sup.
 *
 * 7-July-93  Nate Williams at Montana State University
 *	Modified SUP to use gzip based compression when sending files
 *	across the network to save BandWidth
 *
 * Revision 1.20  92/09/09  22:05:00  mrt
 * 	Added Brad's change to make send_file take a va_list.
 * 	Added support in login to accept an non-encrypted login
 * 	message if no user or password is being sent. This supports
 * 	a non-crypting version of sup. Also fixed to skip leading
 * 	white space from crypts in host files.
 * 	[92/09/01            mrt]
 *
 * Revision 1.19  92/08/11  12:07:59  mrt
 * 		Made maxchildren a patchable variable, which can be set by the
 * 		command line switch -C or else defaults to the MAXCHILDREN
 * 		defined in sup.h. Added most of Brad's STUMP changes.
 * 	Increased PGMVERSION to 12 to reflect substantial changes.
 * 	[92/07/28            mrt]
 *
 * Revision 1.18  90/12/25  15:15:39  ern
 * 	Yet another rewrite of the logging code. Make up the text we will write
 * 	   and then get in, write it and get out.
 * 	Also set error on write-to-full-disk if the logging is for recording
 * 	   server is busy.
 * 	[90/12/25  15:15:15  ern]
 *
 * Revision 1.17  90/05/07  09:31:13  dlc
 * 	Sigh, some more fixes to the new "crypt" file handling code.  First,
 * 	just because the "crypt" file is in a local file system does not mean
 * 	it can be trusted.  We have to check for hard links to root owned
 * 	files whose contents could be interpretted as a crypt key.  For
 * 	checking this fact, the new routine stat_info_ok() was added.  This
 * 	routine also makes other sanity checks, such as owner only permission,
 * 	the file is a regular file, etc.  Also, even if the uid/gid of th
 * 	"crypt" file is not going to be used, still use its contents in order
 * 	to cause fewer surprises to people supping out of a shared file system
 * 	such as AFS.
 * 	[90/05/07            dlc]
 *
 * Revision 1.16  90/04/29  04:21:08  dlc
 * 	Fixed logic bug in docrypt() which would not get the stat information
 * 	from the crypt file if the crypt key had already been set from a
 * 	"host" file.
 * 	[90/04/29            dlc]
 *
 * Revision 1.15  90/04/18  19:51:27  dlc
 * 	Added the new routines local_file(), link_nofollow() for use in
 * 	dectecting whether a file is located in a local file system.  These
 * 	routines probably should have been in another module, but only
 * 	supfilesrv needs to do the check and none of its other modules seemed
 * 	appropriate.  Note, the implementation should be changed once we have
 * 	direct kernel support, for example the fstatvfs(2) system call, for
 * 	detecting the type of file system a file resides.  Also, I changed
 * 	the routines which read the crosspatch crypt file or collection crypt
 * 	file to save the uid and gid from the stat information obtained via
 * 	the local_file() call (when the file is local) at the same time the
 * 	crypt key is read.  This change disallows non-local files for the
 * 	crypt key to plug a security hole involving the usage of the uid/gid
 * 	of the crypt file to define who the file server should run as.  If
 * 	the saved uid/gid are both valid, then the server will set its uid/gid
 * 	to these values.
 * 	[90/04/18            dlc]
 *
 * Revision 1.14  89/08/23  14:56:15  gm0w
 * 	Changed msgf routines to msg routines.
 * 	[89/08/23            gm0w]
 *
 * Revision 1.13  89/08/03  19:57:33  mja
 * 	Remove setaid() call.
 *
 * Revision 1.12  89/08/03  19:49:24  mja
 * 	Updated to use v*printf() in place of _doprnt().
 * 	[89/04/19            mja]
 *
 * 11-Sep-88  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code to record release name in logfile.
 *
 * 18-Mar-88  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added host=<hostfile> support to releases file. [V7.12]
 *
 * 27-Dec-87  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added crosspatch support.  Created docrypt() routine for crypt
 *	test message.
 *
 * 09-Sep-87  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Removed common information logging code, the quiet switch, and
 *	moved samehost() check to after device/inode check.
 *
 * 28-Jun-87  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code for "release" support. [V5.11]
 *
 * 26-May-87  Doug Philips (dwp) at Carnegie-Mellon University
 *	Added code to record final status of client in logfile. [V5.10]
 *
 * 22-May-87  Chriss Stephens (chriss) at Carnegie Mellon University
 *	Mergered divergent CS and ECE versions. [V5.9a]
 *
 * 20-May-87  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Removed support for version 3 of SUP protocol.  Added changes
 *	to make lint happy.  Added calls to new logging routines. [V5.9]
 *
 * 31-Mar-87  Dan Nydick (dan) at Carnegie-Mellon University
 *	Fixed so no password check is done when crypts are used.
 *
 * 25-Nov-86  Rudy Nedved (ern) at Carnegie-Mellon University
 *	Set F_APPEND fcntl in logging to increase the chance
 *	that the log entry from this incarnation of the file
 *	server will not be lost by another incarnation. [V5.8]
 *
 * 20-Oct-86  Dan Nydick (dan) at Carnegie-Mellon University
 *	Changed not to call okmumbles when not compiled with CMUCS.
 *
 * 04-Aug-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code to increment scmdebug as more -N flags are
 *	added. [V5.7]
 *
 * 25-May-86  Jonathan J. Chew (jjc) at Carnegie-Mellon University
 *	Renamed local variable in main program from "sigmask" to
 *	"signalmask" to avoid name conflict with 4.3BSD identifier.
 *	Conditionally compile in calls to CMU routines, "setaid" and
 *	"logaccess". [V5.6]
 *
 * 21-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed supfilesrv to use the crypt file owner and group for
 *	access purposes, rather than the directory containing the crypt
 *	file. [V5.5]
 *
 * 07-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code to keep logfiles in repository collection directory.
 *	Added code for locking collections. [V5.4]
 *
 * 05-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code to support new FSETUPBUSY return.  Now accepts all
 *	connections and tells any clients after the 8th that the
 *	fileserver is busy.  New clients will retry again later. [V5.3]
 *
 * 29-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Major rewrite for protocol version 4. [V4.2]
 *
 * 12-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Fixed close of crypt file to use file pointer as argument
 *	instead of string pointer.
 *
 * 24-Nov-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Allow "!hostname" lines and comments in collection "host" file.
 *
 * 13-Nov-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Don't use access() on symbolic links since they may not point to
 *	an existing file.
 *
 * 22-Oct-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code to restrict file server availability to when it has
 *	less than or equal to eight children.
 *
 * 22-Sep-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Merged 4.1 and 4.2 versions together.
 *
 * 04-Jun-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Created for 4.2 BSD.
 *
 **********************************************************************
 */

#ifdef AFS
#include <afs/param.h>
#undef MAXNAMLEN
#endif
#include <sys/param.h>
#include <signal.h>
#include <errno.h>
#include <setjmp.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/socket.h>
#ifndef HAS_POSIX_DIR
#include <sys/dir.h>
#else
#include <dirent.h>
#endif
#if	MACH
#include <sys/ioctl.h>
#endif
#if	CMUCS
#include <acc.h>
#include <sys/ttyloc.h>
#include <access.h>
#include <sys/viceioctl.h>
#else				/* CMUCS */
#define ACCESS_CODE_OK		0
#define ACCESS_CODE_BADPASSWORD (-2)
#endif				/* CMUCS */

#ifdef __SVR4
#include <sys/mkdev.h>
#include <sys/statvfs.h>
#endif
#ifdef LIBWRAP
#include <tcpd.h>
#endif

#include "supcdefs.h"
#include "supextern.h"
#define MSGFILE
#include "supmsg.h"
#include "libc.h"
#include "c.h"

extern char *crypt(const char *, const char *);

int maxchildren;

/*
 * These are used to save the stat information from the crosspatch crypt
 * file or collection crypt file at the time it is opened for the crypt
 * key and it is verified to be a local file.
 */
int runas_uid = -1;
int runas_gid = -1;

#define PGMVERSION 13

/*************************
 ***    M A C R O S    ***
 *************************/

#define HASHBITS 8
#define HASHSIZE (1<<HASHBITS)
#define HASHMASK (HASHSIZE-1)
#define HASHFUNC(x,y) ((x)&HASHMASK)

/*******************************************
 ***    D A T A   S T R U C T U R E S    ***
 *******************************************/

struct hashstruct {		/* hash table for number lists */
	int Hnum1;		/* numeric keys */
	int Hnum2;
	char *Hname;		/* string value */
	TREE *Htree;		/* TREE value */
	struct hashstruct *Hnext;
};
typedef struct hashstruct HASH;

/*********************************************
 ***    G L O B A L   V A R I A B L E S    ***
 *********************************************/

char program[] = "supfilesrv";	/* program name for SCM messages */
int progpid = -1;		/* and process id */

jmp_buf sjbuf;			/* jump location for network errors */
TREELIST *listTL;		/* list of trees to upgrade */

int silent;			/* -S flag */
#ifdef LIBWRAP
int sup_clog;			/* -l flag */
#endif
int live;			/* -d flag */
int dbgportsq;			/* -P flag */
extern int scmdebug;		/* -N flag */
extern int netfile;
#ifdef RCS
int candorcs;			/* -R flag */
int dorcs = FALSE;
#endif

int nchildren;			/* number of children that exist */
char *prefix;			/* collection pathname prefix */
char *release;			/* collection release name */
char *cryptkey;			/* encryption key if non-null */
#ifdef CVS
char *cvs_root;			/* RCS root */
#endif
char *rcs_branch;		/* RCS branch name */
int lockfd;			/* descriptor of lock file */

/* global variables for scan functions */
int trace = FALSE;		/* directory scan trace */
int cancompress = FALSE;	/* Can we compress files */
int docompress = FALSE;		/* Do we compress files */

HASH *uidH[HASHSIZE];		/* for uid and gid lookup */
HASH *gidH[HASHSIZE];
HASH *inodeH[HASHSIZE];		/* for inode lookup for linked file check */


/* supfilesrv.c */
int main(int, char **);
void chldsig(int);
void usage(void);
void init(int, char **);
void answer(void);
void srvsignon(void);
void srvsetup(void);
void docrypt(void);
void srvlogin(void);
void listfiles(void);
int denyone(TREE *, void *);
void send_files(void);
int send_one(TREE *, void *);
int send_dir(TREE *, void *);
int send_file(TREE *, va_list);
void srvfinishup(time_t);
void Hfree(HASH **);
HASH *Hlookup(HASH **, int, int);
void Hinsert(HASH **, int, int, char *, TREE *);
TREE *linkcheck(TREE *, int, int);
char *uconvert(int);
char *gconvert(int);
char *changeuid(char *, char *, int, int);
void goaway(const char *, ...);
char *fmttime(time_t);
int local_file(int, struct stat *);
int stat_info_ok(struct stat *, struct stat *);
int link_nofollow(int);
int link_nofollow(int);

struct hostpid {
	char name[MAXHOSTNAMELEN];
	pid_t pid;
} *hp;

static void
addchild(pid_t pid)
{
	size_t i;
	for (i = 0; i < maxchildren; i++)
		if (hp[i].pid == 0) {
			hp[i].pid = pid;
			strcpy(hp[i].name, remotehost());
			nchildren++;
			return;
		}
	logerr("Out of space adding child %s", remotehost());
}

static void
removechild(pid_t pid)
{
	size_t i;
	for (i = 0; i < maxchildren; i++)
		if (hp[i].pid == pid) {
			hp[i].pid = 0;
			nchildren--;
			return;
		}
	logerr("Child with pid %jd not found", (intmax_t)pid);
}

static int
checkchild(void)
{
	const char *h = remotehost();
	size_t i;
	for (i = 0; i < maxchildren; i++)
		if (hp[i].pid && strcmp(hp[i].name, h) == 0) {
			logerr("Ignoring connection frm %s", h);
			return 0;
		}
	return 1;
}

/*************************************
 ***    M A I N   R O U T I N E    ***
 *************************************/

int
main(int argc, char **argv)
{
	int x, pid;
	sigset_t nset, oset;
	struct sigaction chld, ign;
	time_t tloc;
#ifdef LIBWRAP
	struct request_info req;
#endif

	/* initialize global variables */
	pgmversion = PGMVERSION;/* export version number */
	isserver = TRUE;	/* export that we're not a server */
	collname = NULL;	/* no current collection yet */
	maxchildren = MAXCHILDREN;	/* defined in sup.h */

	init(argc, argv);	/* process arguments */

#ifdef HAS_DAEMON
	if (!live)		/* if not debugging, turn into daemon */
		if (daemon(0, 0) == -1)
		    goaway("Daemon failed (%s)", strerror(errno));
#endif
	hp = malloc(sizeof(*hp) * maxchildren);
	if (hp == NULL)
		goaway("Cannot allocate memory");

	logopen("supfile");
	tloc = time(NULL);
	loginfo("SUP File Server Version %d.%d (%s) starting at %s",
	    PROTOVERSION, PGMVERSION, scmversion, fmttime(tloc));
	if (live) {
		x = service();

		if (x != SCMOK)
			logquit(1, "Can't connect to network");
#ifdef LIBWRAP
		request_init(&req, RQ_DAEMON, "supfilesrv", RQ_FILE, netfile,
		    NULL);
		fromhost(&req);
		if (hosts_access(&req) == 0) {
			logdeny("refused connection from %.500s",
			    eval_client(&req));
			servicekill();
			exit(1);
		}
		if (sup_clog) {
			logallow("connection from %.500s", eval_client(&req));
		}
#endif
		answer();
		(void) serviceend();
		exit(0);
	}
	ign.sa_handler = SIG_IGN;
	sigemptyset(&ign.sa_mask);
	ign.sa_flags = 0;
	(void) sigaction(SIGHUP, &ign, NULL);
	(void) sigaction(SIGINT, &ign, NULL);
	(void) sigaction(SIGPIPE, &ign, NULL);
	chld.sa_handler = chldsig;
	sigemptyset(&chld.sa_mask);
	chld.sa_flags = 0;
	(void) sigaction(SIGCHLD, &chld, NULL);
	nchildren = 0;
	for (;;) {
		x = service();
		if (x != SCMOK) {
			logerr("Error in establishing network connection");
			(void) servicekill();
			continue;
		}
		/*
		 * If we are being bombarded, don't even spend time forking
		 * or conversing
		 */
		if (nchildren > maxchildren) {
			(void) servicekill();
			continue;
		}
		sigemptyset(&nset);
		sigaddset(&nset, SIGCHLD);
		sigprocmask(SIG_BLOCK, &nset, &oset);
		if ((pid = fork()) == 0) {	/* server process */
#ifdef LIBWRAP
			request_init(&req, RQ_DAEMON, "supfilesrv", RQ_FILE,
			    netfile, NULL);
			fromhost(&req);
			if (hosts_access(&req) == 0) {
				logdeny("refused connection from %.500s",
				    eval_client(&req));
				servicekill();
				exit(1);
			}
			if (sup_clog) {
				logallow("connection from %.500s",
				    eval_client(&req));
			}
#endif
			(void) serviceprep();
			answer();
			(void) serviceend();
			exit(0);
		}
		(void) servicekill();	/* parent */
		if (pid > 0) {
			addchild(pid);
			setproctitle("Master [%d/%d]", nchildren, maxchildren);
		}
		(void) sigprocmask(SIG_SETMASK, &oset, NULL);
	}
}
/*
 * Child status signal handler
 */

void
chldsig(int snum __unused)
{
	int w;
	pid_t pid;

	while ((pid = waitpid(-1, &w, WNOHANG)) > 0) {
		if (kill(pid, 0) == -1)
			switch (errno) {
			case ESRCH:
				if (nchildren == 0) {
					logerr("no children but pid %jd\n",
					    (intmax_t)pid);
					break;
				}
				removechild(pid);
				break;
			default:
				logerr("killing pid %jd: (%s)\n", (intmax_t)
				    pid, strerror(errno));
				break;
			}

	}
}
/*****************************************
 ***    I N I T I A L I Z A T I O N    ***
 *****************************************/

void
usage(void)
{
#ifdef LIBWRAP
	quit(1, "Usage: supfilesrv [ -4 | -6 | -l | -d | -P | -N | -C <max children> | -H <host> <user> <cryptfile> <supargs> ]\n");
#else
	quit(1, "Usage: supfilesrv [ -4 | -6 | -d | -P | -N | -C <max children> | -H <host> <user> <cryptfile> <supargs> ]\n");
#endif
}

void
init(int argc, char **argv)
{
	int i;
	int x;
	char *clienthost, *clientuser;
	char *p, *q;
	char buf[STRINGLENGTH];
	int maxsleep;
	FILE *f;
	int af = AF_INET;

#ifdef RCS
	candorcs = FALSE;
#endif
	live = FALSE;
#ifdef LIBWRAP
	sup_clog = FALSE;
#endif
	dbgportsq = FALSE;
	scmdebug = 0;
	clienthost = NULL;
	clientuser = NULL;
	maxsleep = 5;
	if (--argc < 0)
		usage();
	argv++;
	while (clienthost == NULL && argc > 0 && argv[0][0] == '-') {
		switch (argv[0][1]) {
		case 'S':
			silent = TRUE;
			break;
#ifdef LIBWRAP
		case 'l':
			sup_clog = TRUE;
			break;
#endif
		case 'd':
			live = TRUE;
			break;
		case 'P':
			dbgportsq = TRUE;
			break;
		case 'N':
			scmdebug++;
			break;
		case 'C':
			if (--argc < 1)
				quit(1, "Missing arg to -C\n");
			argv++;
			maxchildren = atoi(argv[0]);
			break;
		case 'H':
			if (--argc < 3)
				quit(1, "Missing args to -H\n");
			argv++;
			clienthost = argv[0];
			clientuser = argv[1];
			cryptkey = argv[2];
			argc -= 2;
			argv += 2;
			break;
#ifdef RCS
		case 'R':
			candorcs = TRUE;
			break;
#endif
		case '4':
			af = AF_INET;
			break;
#ifdef AF_INET6
		case '6':
			af = AF_INET6;
			break;
#endif
		default:
			fprintf(stderr, "Unknown flag %s ignored\n", argv[0]);
			break;
		}
		--argc;
		argv++;
	}
	if (clienthost == NULL) {
		if (argc != 0)
			usage();
		x = servicesetup(dbgportsq ? DEBUGFPORT : FILEPORT, af);
		if (x != SCMOK)
			quit(1, "Error in network setup");
		for (i = 0; i < HASHSIZE; i++)
			uidH[i] = gidH[i] = inodeH[i] = NULL;
		return;
	}
	isserver = FALSE;
	if (argc < 1)
		usage();
	f = fopen(cryptkey, "r");
	if (f == NULL)
		quit(1, "Unable to open cryptfile %s\n", cryptkey);
	if ((p = fgets(buf, STRINGLENGTH, f)) != NULL) {
		if ((q = strchr(p, '\n')) != NULL)
			*q = '\0';
		if (*p == '\0')
			quit(1, "No cryptkey found in %s\n", cryptkey);
		cryptkey = estrdup(buf);
	}
	(void) fclose(f);
	x = request(dbgportsq ? DEBUGFPORT : FILEPORT, clienthost, &maxsleep);
	if (x != SCMOK)
		quit(1, "Unable to connect to host %s\n", clienthost);
	x = msgsignon();
	if (x != SCMOK)
		quit(1, "Error sending signon request to fileserver\n");
	x = msgsignonack();
	if (x != SCMOK)
		quit(1, "Error reading signon reply from fileserver\n");
	printf("SUP Fileserver %d.%d (%s) %d on %s\n",
	    protver, pgmver, scmver, fspid, remotehost());
	free(scmver);
	scmver = NULL;
	if (protver < 7)
		quit(1, "Remote fileserver does not implement reverse sup\n");
	xpatch = TRUE;
	xuser = clientuser;
	x = msgsetup();
	if (x != SCMOK)
		quit(1, "Error sending setup request to fileserver\n");
	x = msgsetupack();
	if (x != SCMOK)
		quit(1, "Error reading setup reply from fileserver\n");
	switch (setupack) {
	case FSETUPOK:
		break;
	case FSETUPSAME:
		quit(1, "User %s not found on remote client\n", xuser);
	case FSETUPHOST:
		quit(1, "This host has no permission to reverse sup\n");
	default:
		quit(1, "Unrecognized file server setup status %d\n", setupack);
	}
	if (netcrypt(cryptkey) != SCMOK)
		quit(1, "Running non-crypting fileserver\n");
	crypttest = CRYPTTEST;
	x = msgcrypt();
	if (x != SCMOK)
		quit(1, "Error sending encryption test request\n");
	x = msgcryptok();
	if (x == SCMEOF)
		quit(1, "Data encryption test failed\n");
	if (x != SCMOK)
		quit(1, "Error reading encryption test reply\n");
	logcrypt = CRYPTTEST;
	loguser = NULL;
	logpswd = NULL;
	if (netcrypt(PSWDCRYPT) != SCMOK)	/* encrypt password data */
		quit(1, "Running non-crypting fileserver\n");
	x = msglogin();
	(void) netcrypt(NULL);	/* turn off encryption */
	if (x != SCMOK)
		quit(1, "Error sending login request to file server\n");
	x = msglogack();
	if (x != SCMOK)
		quit(1, "Error reading login reply from file server\n");
	if (logack == FLOGNG)
		quit(1, "%s\nImproper login to %s account\n", logerror, xuser);
	xargc = argc;
	xargv = argv;
	x = msgxpatch();
	if (x != SCMOK)
		quit(1, "Error sending crosspatch request\n");
	crosspatch();
	exit(0);
}
/*****************************************
 ***    A N S W E R   R E Q U E S T    ***
 *****************************************/

void
answer(void)
{
	time_t starttime;
	int x;

	progpid = fspid = getpid();
	collname = NULL;
	basedir = NULL;
	prefix = NULL;
	release = NULL;
	rcs_branch = NULL;
#ifdef CVS
	cvs_root = NULL;
#endif
	goawayreason = NULL;
	donereason = NULL;
	lockfd = -1;
	starttime = time(NULL);
	if (!setjmp(sjbuf)) {
		srvsignon();
		srvsetup();
		docrypt();
		srvlogin();
		if (xpatch) {
			int fd;

			x = msgxpatch();
			if (x != SCMOK)
				exit(0);
			xargv[0] = "sup";
			xargv[1] = "-X";
			xargv[xargc] = NULL;
			(void) dup2(netfile, 0);
			(void) dup2(netfile, 1);
			(void) dup2(netfile, 2);
			fd = getdtablesize();
			while (--fd > 2)
				(void) close(fd);
			execvp(xargv[0], xargv);
			exit(0);
		}
		listfiles();
		send_files();
	}
	srvfinishup(starttime);
	if (collname)
		free(collname);
	if (basedir)
		free(basedir);
	if (prefix)
		free(prefix);
	if (release)
		free(release);
	if (rcs_branch)
		free(rcs_branch);
#ifdef CVS
	if (cvs_root)
		free(cvs_root);
#endif
	if (goawayreason) {
		if (donereason == goawayreason)
			donereason = NULL;
		free(goawayreason);
	}
	if (donereason)
		free(donereason);
	if (lockfd >= 0)
		(void) close(lockfd);
	endpwent();
	(void) endgrent();
#if	CMUCS
	endacent();
#endif				/* CMUCS */
	Hfree(uidH);
	Hfree(gidH);
	Hfree(inodeH);
}
/*****************************************
 ***    S I G N   O N   C L I E N T    ***
 *****************************************/

void
srvsignon(void)
{
	int x;

	xpatch = FALSE;
	x = msgsignon();
	if (x != SCMOK)
		goaway("Error reading signon request from client");
	x = msgsignonack();
	if (x != SCMOK)
		goaway("Error sending signon reply to client");
	free(scmver);
	scmver = NULL;
}
/*****************************************************************
 ***    E X C H A N G E   S E T U P   I N F O R M A T I O N    ***
 *****************************************************************/

void
srvsetup(void)
{
	int x;
	char *p, *q;
	char buf[STRINGLENGTH], filename[MAXPATHLEN];
	FILE *f;
	struct stat sbuf;
	TREELIST *tl;

	if (protver > 7) {
		cancompress = TRUE;
	}
	x = msgsetup();
	if (x != SCMOK)
		goaway("Error reading setup request from client");
	if (protver < 4) {
		setupack = FSETUPOLD;
		(void) msgsetupack();
		if (protver >= 6)
			longjmp(sjbuf, TRUE);
		goaway("Sup client using obsolete version of protocol");
	}
	if (xpatch) {
		struct passwd *pw;

		if ((pw = getpwnam(xuser)) == NULL) {
			setupack = FSETUPSAME;
			(void) msgsetupack();
			if (protver >= 6)
				longjmp(sjbuf, TRUE);
			goaway("User `%s' not found", xuser);
		}
		(void) free(xuser);
		xuser = estrdup(pw->pw_dir);

		/* check crosspatch host access file */
		cryptkey = NULL;
		(void) sprintf(buf, FILEXPATCH, xuser);

		/* Turn off link following */
		if (link_nofollow(1) != -1) {
			int hostok = FALSE;
			/* get stat info before open */
			if (stat(buf, &sbuf) == -1)
				(void) bzero(&sbuf, sizeof(sbuf));

			if ((f = fopen(buf, "r")) != NULL) {
				struct stat fsbuf;

				while ((p = fgets(buf, STRINGLENGTH, f)) != NULL) {
					q = strchr(p, '\n');
					if (q)
						*q = 0;
					if (strchr("#;:", *p))
						continue;
					q = nxtarg(&p, " \t");
					if (*p == '\0')
						continue;
					if (!matchhost(q))
						continue;

					cryptkey = estrdup(p);
					hostok = TRUE;
					if (local_file(fileno(f), &fsbuf) > 0
					    && stat_info_ok(&sbuf, &fsbuf)) {
						runas_uid = sbuf.st_uid;
						runas_gid = sbuf.st_gid;
					}
					break;
				}
				(void) fclose(f);
			}
			/* Restore link following */
			if (link_nofollow(0) == -1)
				goaway("Restore link following");

			if (!hostok) {
				setupack = FSETUPHOST;
				(void) msgsetupack();
				if (protver >= 6)
					longjmp(sjbuf, TRUE);
				goaway("Host not on access list");
			}
		}
		setupack = FSETUPOK;
		x = msgsetupack();
		if (x != SCMOK)
			goaway("Error sending setup reply to client");
		return;
	}
#ifdef RCS
	if (candorcs && release != NULL &&
	    (strncmp(release, "RCS.", 4) == 0)) {
		rcs_branch = estrdup(&release[4]);
		free(release);
		release = estrdup("RCS");
		dorcs = TRUE;
	}
#endif
	if (release == NULL)
		release = estrdup(DEFRELEASE);
	if (basedir == NULL || *basedir == '\0') {
		basedir = NULL;
		(void) sprintf(filename, FILEDIRS, DEFDIR);
		f = fopen(filename, "r");
		if (f) {
			while ((p = fgets(buf, STRINGLENGTH, f)) != NULL) {
				q = strchr(p, '\n');
				if (q)
					*q = 0;
				if (strchr("#;:", *p))
					continue;
				q = nxtarg(&p, " \t=");
				if (strcmp(q, collname) == 0) {
					basedir = skipover(p, " \t=");
					basedir = estrdup(basedir);
					break;
				}
			}
			(void) fclose(f);
		}
		if (basedir == NULL) {
			(void) sprintf(buf, FILEBASEDEFAULT, collname);
			basedir = estrdup(buf);
		}
	}
	if (chdir(basedir) < 0)
		goaway("Can't chdir to base directory %s (%s)", basedir,
		    strerror(errno));
	(void) sprintf(filename, FILEPREFIX, collname);
	f = fopen(filename, "r");
	if (f) {
		while ((p = fgets(buf, STRINGLENGTH, f)) != NULL) {
			q = strchr(p, '\n');
			if (q)
				*q = 0;
			if (strchr("#;:", *p))
				continue;
			prefix = estrdup(p);
			if (chdir(prefix) < 0)
				goaway("%s: Can't chdir to %s from base "
				    "directory %s (%s)", filename, prefix,
				    basedir, strerror(errno));
			break;
		}
		(void) fclose(f);
	}
	x = stat(".", &sbuf);
	if (prefix) {
		int serrno = errno;
		if (chdir(basedir) < 0)
			goaway("Can't chdir to %s (%s)", basedir,
			    strerror(errno));
		errno = serrno;
	}
	if (x < 0)
		goaway("Can't stat base/prefix directory (%s)",
		    strerror(errno));
	if (nchildren >= maxchildren || !checkchild()) {
		setupack = FSETUPBUSY;
		(void) msgsetupack();
		if (protver >= 6)
			longjmp(sjbuf, TRUE);
		goaway("Sup client told to try again later");
	}
	if (sbuf.st_dev == basedev && sbuf.st_ino == baseino && samehost()) {
		setupack = FSETUPSAME;
		(void) msgsetupack();
		if (protver >= 6)
			longjmp(sjbuf, TRUE);
		goaway("Attempt to upgrade to same directory on same host");
	}
	/* obtain release information */
	if (!getrelease(release)) {
		setupack = FSETUPRELEASE;
		(void) msgsetupack();
		if (protver >= 6)
			longjmp(sjbuf, TRUE);
		goaway("Invalid release information");
	}
	/* check host access file */
	cryptkey = NULL;
	for (tl = listTL; tl != NULL; tl = tl->TLnext) {
		char *h;
		if ((h = tl->TLhost) == NULL)
			h = FILEHOSTDEF;
		(void) sprintf(buf, FILEHOST, collname, h);
		f = fopen(buf, "r");
		if (f) {
			int hostok = FALSE;
			while ((p = fgets(buf, STRINGLENGTH, f)) != NULL) {
				int not;
				q = strchr(p, '\n');
				if (q)
					*q = 0;
				if (strchr("#;:", *p))
					continue;
				q = nxtarg(&p, " \t");
				if ((not = (*q == '!')) && *++q == '\0')
					q = nxtarg(&p, " \t");
				hostok = (not == (matchhost(q) == 0));
				if (hostok) {
					while ((*p == ' ') || (*p == '\t'))
						p++;
					if (*p)
						cryptkey = estrdup(p);
					break;
				}
			}
			(void) fclose(f);
			if (!hostok) {
				setupack = FSETUPHOST;
				(void) msgsetupack();
				if (protver >= 6)
					longjmp(sjbuf, TRUE);
				goaway("Host not on access list for %s",
				    collname);
			}
		}
	}
	/* try to lock collection */
	(void) sprintf(buf, FILELOCK, collname);
#ifdef LOCK_SH
	x = open(buf, O_RDONLY, 0);
	if (x >= 0) {
		if (flock(x, (LOCK_SH | LOCK_NB)) < 0) {
			(void) close(x);
			if (errno != EWOULDBLOCK)
				goaway("Can't lock collection %s", collname);
			setupack = FSETUPBUSY;
			(void) msgsetupack();
			if (protver >= 6)
				longjmp(sjbuf, TRUE);
			goaway("Sup client told to wait for lock");
		}
		lockfd = x;
	}
#endif
	setupack = FSETUPOK;
	x = msgsetupack();
	if (x != SCMOK)
		goaway("Error sending setup reply to client");
}

void
/** Test data encryption **/
docrypt(void)
{
	int x;
	char *p, *q;
	char buf[STRINGLENGTH];
	FILE *f;
	struct stat sbuf;

	if (!xpatch) {
		(void) sprintf(buf, FILECRYPT, collname);

		/* Turn off link following */
		if (link_nofollow(1) != -1) {
			/* get stat info before open */
			if (stat(buf, &sbuf) == -1)
				(void) bzero(&sbuf, sizeof(sbuf));

			if ((f = fopen(buf, "r")) != NULL) {
				struct stat fsbuf;

				if (cryptkey == NULL &&
				    (p = fgets(buf, STRINGLENGTH, f))) {
					if ((q = strchr(p, '\n')) != NULL)
						*q = '\0';
					if (*p)
						cryptkey = estrdup(buf);
				}
				if (local_file(fileno(f), &fsbuf) > 0
				    && stat_info_ok(&sbuf, &fsbuf)) {
					runas_uid = sbuf.st_uid;
					runas_gid = sbuf.st_gid;
				}
				(void) fclose(f);
			}
			/* Restore link following */
			if (link_nofollow(0) == -1)
				goaway("Restore link following");
		}
	}
	if (netcrypt(cryptkey) != SCMOK)
		goaway("Runing non-crypting supfilesrv");
	x = msgcrypt();
	if (x != SCMOK)
		goaway("Error reading encryption test request from client");
	(void) netcrypt(NULL);
	if (strcmp(crypttest, CRYPTTEST) != 0)
		goaway("Client not encrypting data properly");
	free(crypttest);
	crypttest = NULL;
	x = msgcryptok();
	if (x != SCMOK)
		goaway("Error sending encryption test reply to client");
}
/***************************************************************
 ***    C O N N E C T   T O   P R O P E R   A C C O U N T    ***
 ***************************************************************/

void
srvlogin(void)
{
	int x, fileuid = -1, filegid = -1;

	(void) netcrypt(PSWDCRYPT);	/* encrypt acct name and password */
	x = msglogin();
	(void) netcrypt(NULL);		/* turn off encryption */
	if (x != SCMOK)
		goaway("Error reading login request from client");
	if (logcrypt) {
		if (strcmp(logcrypt, CRYPTTEST) != 0) {
			logack = FLOGNG;
			logerror = "Improper login encryption";
			(void) msglogack();
			goaway("Client not encrypting login information properly");
		}
		free(logcrypt);
		logcrypt = NULL;
	}
	if (loguser == NULL) {
		if (cryptkey) {
			if (runas_uid >= 0 && runas_gid >= 0) {
				fileuid = runas_uid;
				filegid = runas_gid;
				loguser = NULL;
			} else
				loguser = estrdup(DEFUSER);
		} else
			loguser = estrdup(DEFUSER);
	}
	if ((logerror = changeuid(loguser, logpswd, fileuid, filegid)) != NULL) {
		logack = FLOGNG;
		(void) msglogack();
		if (protver >= 6)
			longjmp(sjbuf, TRUE);
		goaway("Client denied login access");
	}
	if (loguser)
		free(loguser);
	if (logpswd)
		free(logpswd);
	logack = FLOGOK;
	x = msglogack();
	if (x != SCMOK)
		goaway("Error sending login reply to client");
	if (!xpatch)		/* restore desired encryption */
		if (netcrypt(cryptkey) != SCMOK)
			goaway("Running non-crypting supfilesrv");
	free(cryptkey);
	cryptkey = NULL;
}
/*****************************************
 ***    M A K E   N A M E   L I S T    ***
 *****************************************/

void
listfiles(void)
{
	int x;

	refuseT = NULL;
	x = msgrefuse();
	if (x != SCMOK)
		goaway("Error reading refuse list from client");
	getscanlists();
	Tfree(&refuseT);
	x = msglist();
	if (x != SCMOK)
		goaway("Error sending file list to client");
	Tfree(&listT);
	listT = NULL;
	needT = NULL;
	x = msgneed();
	if (x != SCMOK)
		goaway("Error reading needed files list from client");
	denyT = NULL;
	(void) Tprocess(needT, denyone, NULL);
	Tfree(&needT);
	x = msgdeny();
	if (x != SCMOK)
		goaway("Error sending denied files list to client");
	Tfree(&denyT);
}


int
denyone(TREE * t, void *v __unused)
{
	TREELIST *tl;
	char *name = t->Tname;
	int update = (t->Tflags & FUPDATE) != 0;
	struct stat sbuf;
	TREE *tlink;
	char slinkname[STRINGLENGTH];
	int x;

	for (tl = listTL; tl != NULL; tl = tl->TLnext)
		if ((t = Tsearch(tl->TLtree, name)) != NULL)
			break;
	if (t == NULL) {
		(void) Tinsert(&denyT, name, FALSE);
		return (SCMOK);
	}
	cdprefix(tl->TLprefix);
	if (S_ISLNK(t->Tmode))
		x = lstat(name, &sbuf);
	else
		x = stat(name, &sbuf);
	if (x < 0 || (sbuf.st_mode & S_IFMT) != (t->Tmode & S_IFMT)) {
		(void) Tinsert(&denyT, name, FALSE);
		return (SCMOK);
	}
	switch (t->Tmode & S_IFMT) {
	case S_IFLNK:
		if ((x = readlink(name, slinkname, STRINGLENGTH - 1)) <= 0) {
			(void) Tinsert(&denyT, name, FALSE);
			return (SCMOK);
		}
		slinkname[x] = '\0';
		(void) Tinsert(&t->Tlink, slinkname, FALSE);
		break;
	case S_IFREG:
		if (sbuf.st_nlink > 1 &&
		    (tlink = linkcheck(t, (int) sbuf.st_dev, (int) sbuf.st_ino))) {
			(void) Tinsert(&tlink->Tlink, name, FALSE);
			return (SCMOK);
		}
		if (update)
			t->Tflags |= FUPDATE;
	case S_IFDIR:
		t->Tuid = sbuf.st_uid;
		t->Tgid = sbuf.st_gid;
		break;
	default:
		(void) Tinsert(&denyT, name, FALSE);
		return (SCMOK);
	}
	t->Tflags |= FNEEDED;
	return (SCMOK);
}
/*********************************
 ***    S E N D   F I L E S    ***
 *********************************/

void
send_files(void)
{
	TREELIST *tl;
	int x;

	/* Does the protocol support compression */
	if (cancompress) {
		/* Check for compression on sending files */
		x = msgcompress();
		if (x != SCMOK)
			goaway("Error sending compression check to server");
	}
	/* send all files */
	for (tl = listTL; tl != NULL; tl = tl->TLnext) {
		cdprefix(tl->TLprefix);
#ifdef CVS
		if (candorcs) {
			cvs_root = getcwd(NULL, 256);
			if (access("CVSROOT", F_OK) < 0)
				dorcs = FALSE;
			else {
				loginfo("is a CVSROOT \"%s\"\n", cvs_root);
				dorcs = TRUE;
			}
		}
#endif
		(void) Tprocess(tl->TLtree, send_one, NULL);
	}
	/* send directories in reverse order */
	for (tl = listTL; tl != NULL; tl = tl->TLnext) {
		cdprefix(tl->TLprefix);
		(void) Trprocess(tl->TLtree, send_dir, NULL);
	}
	x = msgsend();
	if (x != SCMOK)
		goaway("Error reading receive file request from client");
	upgradeT = NULL;
	x = msgrecv(send_file, 0);
	if (x != SCMOK)
		goaway("Error sending file to client");
}

int
send_one(TREE * t, void *v __unused)
{
	int x, fd;
	char temp_file[STRINGLENGTH];
	char *av[50];		/* More than enough */

	if ((t->Tflags & FNEEDED) == 0)	/* only send needed files */
		return (SCMOK);
	if (S_ISDIR(t->Tmode))	/* send no directories this pass */
		return (SCMOK);
	x = msgsend();
	if (x != SCMOK)
		goaway("Error reading receive file request from client");
	upgradeT = t;		/* upgrade file pointer */
	fd = -1;		/* no open file */
	if (S_ISREG(t->Tmode)) {
		if (!listonly && (t->Tflags & FUPDATE) == 0) {
#ifdef RCS
			if (dorcs) {
				char rcs_release[STRINGLENGTH];

				tmpnam(rcs_file);
				fd = open(rcs_file, (O_WRONLY | O_CREAT | O_TRUNC | O_EXCL), 0600);
				if (fd < 0)
					goaway("We died trying to create temp file");
				close(fd);
				fd = -1;
				if (strcmp(&t->Tname[strlen(t->Tname) - 2], ",v") == 0) {
					t->Tname[strlen(t->Tname) - 2] = '\0';
					ac = 0;
#ifdef CVS
					av[ac++] = "cvs";
					av[ac++] = "-d";
					av[ac++] = cvs_root;
					av[ac++] = "-r";
					av[ac++] = "-l";
					av[ac++] = "-Q";
					av[ac++] = "co";
					av[ac++] = "-p";
					if (rcs_branch != NULL) {
						av[ac++] = "-r";
						av[ac++] = rcs_branch;
					}
#else
					av[ac++] = "co";
					av[ac++] = "-q";
					av[ac++] = "-p";
					if (rcs_branch != NULL) {
						sprintf(rcs_release, "-r%s",
						    rcs_branch);
						av[ac++] = rcs_release;
					}
#endif
					av[ac++] = t->Tname;
					av[ac++] = NULL;
					status = runio(av, NULL, rcs_file,
					    "/dev/null");
					/* loginfo("using rcs mode \n"); */
					if (status < 0 || WEXITSTATUS(status)) {
						/* Just in case */
						unlink(rcs_file);
						if (status < 0) {
							goaway("We died trying to run cvs or rcs on %s", rcs_file);
							t->Tmode = 0;
						} else {
#if 0
							logerr("rcs command failed = %d\n",
							    WEXITSTATUS(status));
#endif
							t->Tflags |= FUPDATE;
						}
					} else if (docompress) {
						tmpnam(temp_file);
						av[0] = "gzip";
						av[1] = "-cf";
						av[2] = NULL;
						if (runio(av, rcs_file, temp_file, NULL) != 0) {
							/* Just in case */
							unlink(temp_file);
							unlink(rcs_file);
							goaway("We died trying to gzip %s", rcs_file);
							t->Tmode = 0;
						}
						fd = open(temp_file, O_RDONLY, 0);
					} else
						fd = open(rcs_file, O_RDONLY, 0);
				}
			}
#endif
			if (fd == -1) {
				if (docompress) {
					snprintf(temp_file, sizeof(temp_file),
					     "%s/supfilesrv.XXXXXX", P_tmpdir);
					fd = mkstemp(temp_file);
					if (fd < 0)
						goaway("We died trying to create temp file");
					close(fd);
					fd = -1;
					av[0] = "gzip";
					av[1] = "-cf";
					av[2] = NULL;
					if (runio(av, t->Tname, temp_file, NULL) != 0) {
						/* Just in case */
						unlink(temp_file);
						goaway("We died trying to gzip %s", t->Tname);
						t->Tmode = 0;
					}
					fd = open(temp_file, O_RDONLY, 0);
				} else
					fd = open(t->Tname, O_RDONLY, 0);
			}
			if (fd < 0 && (t->Tflags & FUPDATE) == 0)
				t->Tmode = 0;
		}
		if (t->Tmode) {
			t->Tuser = estrdup(uconvert(t->Tuid));
			t->Tgroup = estrdup(gconvert(t->Tgid));
		}
	}
	x = msgrecv(send_file, fd);
	if (docompress)
		unlink(temp_file);
#ifdef RCS
	if (dorcs)
		unlink(rcs_file);
#endif
	if (x != SCMOK)
		goaway("Error sending file %s to client", t->Tname);
	return (SCMOK);
}

int
send_dir(TREE * t, void *v __unused)
{
	int x;

	if ((t->Tflags & FNEEDED) == 0)	/* only send needed files */
		return (SCMOK);
	if (!S_ISDIR(t->Tmode))	/* send only directories this pass */
		return (SCMOK);
	x = msgsend();
	if (x != SCMOK)
		goaway("Error reading receive file request from client");
	upgradeT = t;		/* upgrade file pointer */
	t->Tuser = estrdup(uconvert(t->Tuid));
	t->Tgroup = estrdup(gconvert(t->Tgid));
	x = msgrecv(send_file, 0);
	if (x != SCMOK)
		goaway("Error sending file %s to client", t->Tname);
	return (SCMOK);
}

int
send_file(TREE * t, va_list ap)
{
	int x, fd;

	fd = va_arg(ap, int);
	if (!S_ISREG(t->Tmode) || listonly || (t->Tflags & FUPDATE))
		return (SCMOK);
	x = writefile(fd);
	if (x != SCMOK)
		goaway("Error sending file %s to client", t->Tname);
	(void) close(fd);
	return (SCMOK);
}
/*****************************************
 ***    E N D   C O N N E C T I O N    ***
 *****************************************/

void
srvfinishup(time_t starttime)
{
	int x = SCMOK;
	char tmpbuf[BUFSIZ], *p, lognam[STRINGLENGTH];
	int logfd;
	time_t finishtime;
	char *releasename;

	(void) netcrypt(NULL);
	if (protver < 6) {
		if (goawayreason != NULL)
			free(goawayreason);
		goawayreason = NULL;
		x = msggoaway();
		doneack = FDONESUCCESS;
		donereason = estrdup("Unknown");
	} else if (goawayreason == NULL)
		x = msgdone();
	else {
		doneack = FDONEGOAWAY;
		donereason = goawayreason;
	}
	if (x == SCMEOF || x == SCMERR) {
		doneack = FDONEUSRERROR;
		donereason = estrdup("Premature EOF on network");
	} else if (x != SCMOK) {
		doneack = FDONESRVERROR;
		donereason = estrdup("Unknown SCM code");
	}
	if (doneack == FDONEDONTLOG)
		return;
	if (donereason == NULL)
		donereason = estrdup("No reason");
	if (doneack == FDONESRVERROR || doneack == FDONEUSRERROR)
		logerr("%s: %s", remotehost(), donereason);
	else if (doneack == FDONEGOAWAY)
		logerr("GOAWAY: %s: %s", remotehost(), donereason);
	else if (doneack != FDONESUCCESS)
		logerr("%s: Reason %d: %s", remotehost(), doneack, donereason);
	goawayreason = donereason;
	cdprefix(NULL);
	if (collname == NULL) {
		logerr("%s: NULL collection in svrfinishup", remotehost());
		return;
	}
	(void) sprintf(lognam, FILELOGFILE, collname);
	if ((logfd = open(lognam, O_APPEND | O_WRONLY, 0644)) < 0)
		return;		/* can not open file up...error */
	finishtime = time(NULL);
	p = tmpbuf;
	(void) sprintf(p, "%s ", fmttime(lasttime));
	p += strlen(p);
	(void) sprintf(p, "%s ", fmttime(starttime));
	p += strlen(p);
	(void) sprintf(p, "%s ", fmttime(finishtime));
	p += strlen(p);
	if ((releasename = release) == NULL)
		releasename = "UNKNOWN";
	(void) sprintf(p, "%s %s %d %s\n", remotehost(), releasename,
	    FDONESUCCESS - doneack, donereason);
	p += strlen(p);
#if	MACH
	/* if we are busy dont get stuck updating the disk if full */
	if (setupack == FSETUPBUSY) {
		long l = FIOCNOSPC_ERROR;
		ioctl(logfd, FIOCNOSPC, &l);
	}
#endif				/* MACH */
	if (write(logfd, tmpbuf, (p - tmpbuf)) == -1)
		logerr("%s: write failed (%s)", remotehost(), strerror(errno));
	(void) close(logfd);
}
/***************************************************
 ***    H A S H   T A B L E   R O U T I N E S    ***
 ***************************************************/

void
Hfree(HASH ** table)
{
	HASH *h;
	int i;
	for (i = 0; i < HASHSIZE; i++)
		while ((h = table[i]) != NULL) {
			table[i] = h->Hnext;
			if (h->Hname)
				free(h->Hname);
			free(h);
		}
}

HASH *
Hlookup(HASH ** table, int num1, int num2)
{
	HASH *h;
	int hno;
	hno = HASHFUNC(num1, num2);
	for (h = table[hno]; h && (h->Hnum1 != num1 || h->Hnum2 != num2); h = h->Hnext);
	return (h);
}

void
Hinsert(HASH ** table, int num1, int num2, char *name, TREE * tree)
{
	HASH *h;
	int hno;
	hno = HASHFUNC(num1, num2);
	h = malloc(sizeof(*h));
	if (h == NULL)
		goaway("Cannot allocate memory");
	h->Hnum1 = num1;
	h->Hnum2 = num2;
	h->Hname = name;
	h->Htree = tree;
	h->Hnext = table[hno];
	table[hno] = h;
}
/*********************************************
 ***    U T I L I T Y   R O U T I N E S    ***
 *********************************************/

TREE *
linkcheck(TREE * t, int d, int i)
 /* inode # and device # */
{
	HASH *h;
	h = Hlookup(inodeH, i, d);
	if (h)
		return (h->Htree);
	Hinsert(inodeH, i, d, NULL, t);
	return (NULL);
}

char *
uconvert(int uid)
{
	struct passwd *pw;
	char *p;
	HASH *u;
	u = Hlookup(uidH, uid, 0);
	if (u)
		return (u->Hname);
	pw = getpwuid(uid);
	if (pw == NULL)
		return ("");
	p = estrdup(pw->pw_name);
	Hinsert(uidH, uid, 0, p, NULL);
	return (p);
}

char *
gconvert(int gid)
{
	struct group *gr;
	char *p;
	HASH *g;
	g = Hlookup(gidH, gid, 0);
	if (g)
		return (g->Hname);
	gr = getgrgid(gid);
	if (gr == NULL)
		return ("");
	p = estrdup(gr->gr_name);
	Hinsert(gidH, gid, 0, p, NULL);
	return (p);
}

char *
changeuid(char *namep, char *passwordp, int fileuid, int filegid)
{
	char *group, *account, *pswdp;
	struct passwd *pwd;
	struct group *grp;
#if	CMUCS
	struct account *acc;
	struct ttyloc tlc;
#endif				/* CMUCS */
	int status = ACCESS_CODE_OK;
	char nbuf[STRINGLENGTH];
	static char errbuf[STRINGLENGTH];
#if	CMUCS
	int *grps;
#endif				/* CMUCS */
	char *p = NULL;

	if (namep == NULL) {
		pwd = getpwuid(fileuid);
		if (pwd == NULL) {
			(void) sprintf(errbuf, "Reason:  Unknown user id %d",
			    fileuid);
			return (errbuf);
		}
		grp = getgrgid(filegid);
		if (grp)
			group = strcpy(nbuf, grp->gr_name);
		else
			group = NULL;
		account = NULL;
		pswdp = NULL;
	} else {
		(void) strcpy(nbuf, namep);
		account = group = strchr(nbuf, ',');
		if (group != NULL) {
			*group++ = '\0';
			account = strchr(group, ',');
			if (account != NULL) {
				*account++ = '\0';
				if (*account == '\0')
					account = NULL;
			}
			if (*group == '\0')
				group = NULL;
		}
		pwd = getpwnam(nbuf);
		if (pwd == NULL) {
			(void) sprintf(errbuf, "Reason:  Unknown user %s",
			    nbuf);
			return (errbuf);
		}
		if (strcmp(nbuf, DEFUSER) == 0)
			pswdp = NULL;
		else
			pswdp = passwordp ? passwordp : "";
#ifdef AFS
		if (strcmp(nbuf, DEFUSER) != 0) {
			char *reason;
			setpag();	/* set a pag */
			if (ka_UserAuthenticate(pwd->pw_name, "", 0,
				pswdp, 1, &reason)) {
				(void) sprintf(errbuf, "AFS authentication failed, %s",
				    reason);
				logerr("Attempt by %s; %s",
				    nbuf, errbuf);
				return (errbuf);
			}
		}
#endif
	}
	if (getuid() != 0) {
		if (getuid() == pwd->pw_uid)
			return (NULL);
		if (strcmp(pwd->pw_name, DEFUSER) == 0)
			return (NULL);
		logerr("Fileserver not superuser");
		return ("Reason:  fileserver is not running privileged");
	}
#if	CMUCS
	tlc.tlc_hostid = TLC_UNKHOST;
	tlc.tlc_ttyid = TLC_UNKTTY;
	if (okaccess(pwd->pw_name, ACCESS_TYPE_SU, 0, -1, tlc) != 1)
		status = ACCESS_CODE_DENIED;
	else {
		grp = NULL;
		acc = NULL;
		status = oklogin(pwd->pw_name, group, &account, pswdp, &pwd, &grp, &acc, &grps);
		if (status == ACCESS_CODE_OK) {
			if ((p = okpassword(pswdp, pwd->pw_name, pwd->pw_gecos)) != NULL)
				status = ACCESS_CODE_INSECUREPWD;
		}
	}
#else				/* CMUCS */
	status = ACCESS_CODE_OK;
	if (namep && strcmp(pwd->pw_name, DEFUSER) != 0)
		if (pswdp == NULL || strcmp(pwd->pw_passwd, crypt(pswdp, pwd->pw_passwd)))
			status = ACCESS_CODE_BADPASSWORD;
#endif				/* CMUCS */
	switch (status) {
	case ACCESS_CODE_OK:
		break;
	case ACCESS_CODE_BADPASSWORD:
		p = "Reason:  Invalid password";
		break;
#if	CMUCS
	case ACCESS_CODE_INSECUREPWD:
		(void) sprintf(errbuf, "Reason:  %s", p);
		p = errbuf;
		break;
	case ACCESS_CODE_DENIED:
		p = "Reason:  Access denied";
		break;
	case ACCESS_CODE_NOUSER:
		p = errbuf;
		break;
	case ACCESS_CODE_ACCEXPIRED:
		p = "Reason:  Account expired";
		break;
	case ACCESS_CODE_GRPEXPIRED:
		p = "Reason:  Group expired";
		break;
	case ACCESS_CODE_ACCNOTVALID:
		p = "Reason:  Invalid account";
		break;
	case ACCESS_CODE_MANYDEFACC:
		p = "Reason:  User has more than one default account";
		break;
	case ACCESS_CODE_NOACCFORGRP:
		p = "Reason:  No account for group";
		break;
	case ACCESS_CODE_NOGRPFORACC:
		p = "Reason:  No group for account";
		break;
	case ACCESS_CODE_NOGRPDEFACC:
		p = "Reason:  No group for default account";
		break;
	case ACCESS_CODE_NOTGRPMEMB:
		p = "Reason:  Not member of group";
		break;
	case ACCESS_CODE_NOTDEFMEMB:
		p = "Reason:  Not member of default group";
		break;
	case ACCESS_CODE_OOPS:
		p = "Reason:  Internal error";
		break;
#endif				/* CMUCS */
	default:
		(void) sprintf(p = errbuf, "Reason:  Status %d", status);
		break;
	}
	if (status != ACCESS_CODE_OK) {
		logerr("Login failure for %s", pwd->pw_name);
		logerr("%s", p);
#if	CMUCS
		logaccess(pwd->pw_name, ACCESS_TYPE_SUP, status, 0, -1, tlc);
#endif				/* CMUCS */
		return (p);
	}
#if	CMUCS
	if (setgroups(grps[0], &grps[1]) < 0)
		logerr("setgroups: %%m");
	if (setgid((gid_t) grp->gr_gid) < 0)
		logerr("setgid: %%m");
	if (setuid((uid_t) pwd->pw_uid) < 0)
		logerr("setuid: %%m");
#else				/* CMUCS */
	if (initgroups(pwd->pw_name, pwd->pw_gid) < 0)
		return ("Error setting group list");
	if (setgid(pwd->pw_gid) < 0)
		logerr("setgid: %%m");
	if (setuid(pwd->pw_uid) < 0)
		logerr("setuid: %%m");
#endif				/* CMUCS */
	return (NULL);
}

void
goaway(const char *fmt, ...)
{
	char buf[STRINGLENGTH];
	va_list ap;

	va_start(ap, fmt);
	(void) netcrypt(NULL);

	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	goawayreason = estrdup(buf);
	(void) msggoaway();
	logerr("%s: %s", remotehost(), buf);
	longjmp(sjbuf, TRUE);
}

char *
fmttime(time_t time)
{
	static char buf[STRINGLENGTH];
	unsigned int len;

	(void) strcpy(buf, ctime(&time) + 4);
	len = strlen(buf);
	if (len > 2)
		buf[len - 2] = '\0';
	return (buf);
}
/*
 * Determine whether the file referenced by the file descriptor 'handle' can
 * be trusted, namely is it a file resident in the local file system.
 *
 * The main method of operation is to perform operations on the file
 * descriptor so that an attempt to spoof the checks should fail, for
 * example renamimg the file from underneath us and/or changing where the
 * file lives from underneath us.
 *
 * returns: -1 for error, indicating that we can not tell
 *	     0 for file is definately not local, or it is an RFS link
 *	     1 for file is local and can be trusted
 *
 * Side effect: copies the stat information into the supplied buffer,
 * regardless of the type of file system the file resides.
 *
 * Currently, the cases that we try to distinguish are RFS, AFS, NFS and
 * UFS, where the latter is considered a trusted file.  We assume that the
 * caller has disabled link following and will detect an attempt to access
 * a file through an RFS link, except in the case the last component is
 * an RFS link.  With link following disabled, the last component itself is
 * interpreted as a regular file if it is really an RFS link, so we
 * disallow the RFS link identified by group "symlink" and mode "IEXEC by
 * owner only". An AFS file is
 * detected by trying the VIOCIGETCELL ioctl, which is one of the few AFS
 * ioctls which operate on a file descriptor.  Note, this AFS ioctl is
 * implemented in the cache manager, so the decision does not involve a
 * query with the AFS file server.  An NFS file is detected by looking at
 * the major device number and seeing if it matches the known values for
 * MACH NSF/Sun OS 3.x or Sun OS 4.x.
 *
 * Having the fstatvfs() system call would make this routine easier and
 * more reliable.
 *
 * Note, in order to make the checks simpler, the file referenced by the
 * file descriptor can not be a BSD style symlink.  Even with symlink
 * following of the last path component disabled, the attempt to open a
 * file which is a symlink will succeed, so we check for the BSD symlink
 * file type here.  Also, the link following on/off and RFS file types
 * are only relevant in a MACH environment.
 */
#ifdef	AFS
#include <sys/viceioctl.h>
#endif

#define SYMLINK_GRP 64

int
local_file(int handle, struct stat * sinfo)
{
	struct stat sb;
#ifdef	VIOCIGETCELL
	/*
	 * dummies for the AFS ioctl
	 */
	struct ViceIoctl vdata;
	char cellname[512];
#endif				/* VIOCIGETCELL */

	if (fstat(handle, &sb) < 0)
		return (-1);
	if (sinfo != NULL)
		*sinfo = sb;

#if	CMUCS
	/*
	 * If the following test succeeds, then the file referenced by
	 * 'handle' is actually an RFS link, so we will not trust it.
	 * See <sys/inode.h>.
	 */
	if (sb.st_gid == SYMLINK_GRP
	    && (sb.st_mode & (S_IFMT | S_IEXEC | (S_IEXEC >> 3) | (S_IEXEC >> 6)))
	    == (S_IFREG | S_IEXEC))
		return (0);
#endif				/* CMUCS */

	/*
	 * Do not trust BSD style symlinks either.
	 */
	if (S_ISLNK(sb.st_mode))
		return (0);

#ifdef	VIOCIGETCELL
	/*
	 * This is the VIOCIGETCELL ioctl, which takes an fd, not
	 * a path name.  If it succeeds, then the file is in AFS.
	 *
	 * On failure, ENOTTY indicates that the file was not in
	 * AFS; all other errors are pessimistically assumed to be
	 * a temporary AFS error.
	 */
	vdata.in_size = 0;
	vdata.out_size = sizeof(cellname);
	vdata.out = cellname;
	if (ioctl(handle, VIOCIGETCELL, (char *) &vdata) != -1)
		return (0);
	if (errno != ENOTTY)
		return (-1);
#endif				/* VIOCIGETCELL */

	/*
	 * Verify the file is not in NFS.
	 *
	 * Our current implementation and Sun OS 3.x use major device
	 * 255 for NFS files; Sun OS 4.x seems to use 130 (I have only
	 * determined this empirically -- DLC).  Without a fstatvfs()
	 * system call, this will have to do for now.
	 */
#if defined(__SVR4) || __NetBSD_Version__ > 299000900
	{
		struct statvfs sf;

		if (fstatvfs(handle, &sf) == -1)
			return (-1);
#ifdef __SVR4
		return strncmp(sf.f_basetype, "nfs", 3) != 0;
#else
		return strncmp(sf.f_fstypename, "nfs", 3) != 0;
#endif
	}
#elif defined(__NetBSD__)
	{
		struct statfs sf;
		if (fstatfs(handle, &sf) == -1)
			return (-1);
		return strncmp(sf.f_fstypename, "nfs", 3) != 0;
	}
#else
	if (major(sb.st_dev) == 255 || major(sb.st_dev) == 130)
		return (0);
	else
		return (1);
#endif

}
/*
 * Companion routine for ensuring that a local file can be trusted.  Compare
 * various pieces of the stat information to make sure that the file can be
 * trusted.  Returns true for stat information which meets the criteria
 * for being trustworthy.  The main paranoia is to prevent a hard link to
 * a root owned file.  Since the link could be removed after the file is
 * opened, a simply fstat() can not be relied upon.  The two stat buffers
 * for comparison should come from a stat() on the file name and a following
 * fstat() on the open file.  Some of the following checks are also an
 * additional level of paranoia.  Also, this test will fail (correctly) if
 * either or both of the stat structures have all fields zeroed; typically
 * due to a stat() failure.
 */


int
stat_info_ok(struct stat * sb1, struct stat * sb2)
{
	return (sb1->st_ino == sb2->st_ino &&	/* Still the same file */
	    sb1->st_dev == sb2->st_dev &&	/* On the same device */
	    sb1->st_mode == sb2->st_mode &&	/* Perms (and type) same */
	    S_ISREG(sb1->st_mode) &&	/* Only allow reg files */
	    (sb1->st_mode & 077) == 0 &&	/* Owner only perms */
	    sb1->st_nlink == sb2->st_nlink &&	/* # hard links same... */
	    sb1->st_nlink == 1 &&	/* and only 1 */
	    sb1->st_uid == sb2->st_uid &&	/* owner and ... */
	    sb1->st_gid == sb2->st_gid &&	/* group unchanged */
	    sb1->st_mtime == sb2->st_mtime &&	/* Unmodified between stats */
	    sb1->st_ctime == sb2->st_ctime);	/* Inode unchanged.  Hopefully
						 * a catch-all paranoid test */
}
#if MACH
/*
 * Twiddle symbolic/RFS link following on/off.  This is a no-op in a non
 * CMUCS/MACH environment.  Also, the setmodes/getmodes interface is used
 * mainly because it is simpler than using table(2) directly.
 */
#include <sys/table.h>

int
link_nofollow(int on)
{
	static int modes = -1;

	if (modes == -1 && (modes = getmodes()) == -1)
		return (-1);
	if (on)
		return (setmodes(modes | UMODE_NOFOLLOW));
	return (setmodes(modes));
}
#else				/* MACH */
/*ARGSUSED*/
int
link_nofollow(int on __unused)
{
	return (0);
}
#endif				/* MACH */
