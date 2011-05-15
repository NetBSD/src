/* G U N I X I O  --  UNIX i/o module for gkermit */

/*
  UNIX i/o functions for gkermit.

  Author:
    Frank da Cruz
    The Kermit Project
    Columbia University
    612 West 115th Street
    New York NY 10025-7799  USA
    http://www.columbia.edu/kermit/
    kermit@columbia.edu

  Copyright (C) 1999,
  The Trustees of Columbia University in the City of New York.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 CONTENTS...

 Console Output:
   tmsg   - Type a message
   tmsgl  - Type a line

 Communication Device:
   ttopen - Open
   ttpkt  - Put in packet mode
   ttres  - Restore normal mode
   ttinl  - Input a raw packet
   ttol   - Send a packet
   ttchk  - Check if input ready
   ttflui - Flush input buffer

 File:
   zchki  - See if file can be opened for input
   zopeni - Open input file
   zopeno - Open output file
   zclosi - Close input file
   zcloso - Close output file
   zrtol  - Remote-to-Local filename conversion
   zltor  - Local-to-Remote filename conversion
   zgetc  - Get character from input file
*/

#include <stdio.h>			/* Standard input/output */
#include <string.h>
#include <stdlib.h>

#ifdef POSIX
#include <termios.h>			/* Terminal modes */
#else
#ifdef SYSV
#include <termio.h>
#else
#include <sgtty.h>
#endif /* SYSV */
#endif /* POSIX */

#include <ctype.h>			/* Character types */
#include <sys/types.h>			/* Needed e.g. by <stat.h> */
#include <signal.h>			/* Interrupts */
#include <setjmp.h>			/* Longjumps */
#include <sys/stat.h>			/* File exist, file size */
#include <errno.h>			/* Error symbols */
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "gkermit.h"			/* gkermit definitions */

/* All versions of HP-UX need Xon/Xoff */

#ifdef hpux				/* HP-UX Pre-7.00 */
#ifndef __hpux
#define __hpux
#endif /* __hpux */
#endif /* hpux */

#ifdef __hpux				/* HP-UX 7.00 and later */
#ifndef SETXONXOFF
#define SETXONXOFF
#endif /* SETXONXOFF */
#endif /*  __hpux */

#ifdef NOXONXOFF			/* -DNOXONXOFF overrides */
#ifdef SETXONXOFF
#undef SETXONXOFF
#endif /* SETXONXOFF */
#endif /* NOXONXOFF */

#ifndef TINBUFSIZ			/* read() inpbut buffer */
#ifdef USE_GETCHAR
#define TINBUFSIZ 0			/* getchar() has its own */
#else
#ifdef SMALL
#define TINBUFSIZ 240
#else
#define TINBUFSIZ 4080
#endif /* SMALL */
#endif /* USE_GETCHAR */
#endif /* TINBUFSIZ */

#ifndef DUMBIO
#ifndef USE_GETCHAR
#ifndef NOFCNTL_H			/* For nonblocking buffered read() */
#ifdef SYS_FCNTL_H
#include <sys/fcntl.h>
#else
#include <fcntl.h>
#ifndef O_NDELAY
#ifdef O_NONBLOCK
#define O_NDELAY O_NONBLOCK
#endif /* O_NONBLOCK */
#endif /* O_NDELAY */
#endif /* SYS_FCNTL_H */
#endif /* NOFCNTL_H */
#endif /* USE_GETCHAR */
#endif /* DUMBIO */

#ifdef O_NDELAY				/* For System V R3 and earlier */
#ifndef EWOULDBLOCK
#ifdef EAGAIN
#define EWOULDBLOCK EAGAIN
#endif /* EAGAIN */
#endif /* EWOULDBLOCK */
#endif /* O_NDELAY */

#ifndef DUMBIO				/* To force single-char read/write */
#ifndef USE_GETCHAR
#ifndef O_NDELAY
#define DUMBIO
#endif /* O_NDELAY */
#endif /* USE_GETCHAR */
#endif /* DUMBIO */

/* Header file deficiencies section... */

#ifndef R_OK
#define R_OK 4
#endif /* R_OK */

#ifndef W_OK
#define W_OK 2
#endif /* W_OK */

#ifndef _IFMT
#ifdef S_IFMT
#define _IFMT S_IFMT
#else
#define _IFMT 0170000
#endif /* S_IFMT */
#endif /* _IFMT */

#ifndef S_ISREG
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif /* S_ISREG */

/* External variables */

extern int literal;			/* Literal filenames */
extern int quiet;			/* No messages */
extern int keep;			/* Keep incomplete files */
extern int streamok;			/* OK to offer streaming */
extern int nomodes;			/* Don't get/set tty modes */
extern int xonxoff;			/* Set Xon/Xoff */
extern int noxonxoff;			/* Don't set Xon/Xoff */
extern FILE * db;			/* Debug log file */

/* Exported variables */

FILE *ifp, *ofp;			/* Input and output file pointers */
char zinbuf[MAXRECORD+1];		/* File input buffer */
int zincnt = 0;				/* count */
char * zinptr = NULL;			/* and pointer. */

/* Private global variables */

static int havemodes = 0;		/* Have obtained terminal modes */
static int ttflags = -1;		/* Terminal flags */
static int nonblock = 0;		/* Nonblocking i/o enabled */
static char tinbuf[TINBUFSIZ+16];	/* Communications input buffer */
static char * tinptr = NULL;		/* Pointer to current item */
static int tincnt = 0;			/* Buffer count */
static int tlast = 0;			/* Last item in buffer */
static int xparity = 0;			/* Parity in use, 0 = none */
static int raw = 0;			/* Terminal rawmode flag */
static char work[MAXPATHLEN+1];		/* Filename conversion buffer */

/* Terminal mode structs */

#ifdef POSIX				/* POSIX */
static struct termios ttold, ttraw;
#else
#ifdef SYSV				/* System V */
static struct termio ttold = {0};
static struct termio ttraw = {0};
#else
#ifdef BSD				/* 4.2 BSD or UNIX V7 */
static struct sgttyb ttold, ttraw;
#endif /* BSD */
#endif /* SYSV */
#endif /* POSIX */

static jmp_buf jbuf;			/* Longjump buffer for timeouts */

/* Functions... */

SIGTYP
doexit(x) int x; {			/* Exit routine */
    if (x)				/* If failure */
      ttflui();				/* flush pending junk we won't read */
    ttres();				/* Reset the communication device */
#ifdef F_SETFL
    if (ttflags > -1)			/* Restore its flags */
      fcntl(0,F_SETFL,ttflags);
#endif /* F_SETFL */
    if (debug) {
	fprintf(db,"exit %d\n",x);
	fclose(db);
    }
    exit(x);
}

VOID
sysinit() {				/* To be run first thing */
#ifdef F_SETFL
    ttflags = fcntl(0,F_GETFL,0);	/* Get and save stdin flags */
#endif /* F_SETFL */
#ifdef SIGINT
    signal(SIGINT,SIG_IGN);		/* Ignore interrupts */
#endif /* SIGINT */
#ifdef SIGTSTP
    signal(SIGTSTP,SIG_IGN);
#endif /* SIGTSTP */
#ifdef SIGQUIT
    signal(SIGQUIT,SIG_IGN);
#endif /* SIGQUIT */
    signal(SIGHUP,doexit);		/* Go here on hangup */
}

/* Console Functions */

#ifdef COMMENT				/* (not used) */
VOID
tmsg(s) char *s; {			/* tmsg() */
    if (!quiet)
      fprintf(stderr,"%s",s);		/* Type message on the screen. */
}
#endif /* COMMENT */

VOID
tmsgl(s) char *s; {			/* tmsgl() */
    if (!quiet) {
	if (raw)
	  fprintf(stderr,"%s\r\n",s);	/* Type message with CRLF */
	else
	  fprintf(stderr,"%s\n",s);
    }
}

/* Debugging functions */

VOID
logerr(s) char * s; {			/* Log text and errno */
    if (!s) s = "";
    if (!debug) return;
    if (db) fprintf(db,"%s: errno = %d\n",s,errno);
}

/* Parity function */

char
#ifdef __STDC__
dopar(char ch)
#else
dopar(ch) char ch;
#endif /* __STDC__ */
{					/* Do parity */
    unsigned int a;
    if (!xparity) return(ch); else ch &= 0177;
    switch (xparity) {
      case 'm':  return(ch | 128);	/* Mark */
      case 's':  return(ch & 127);	/* Space */
      case 'o':				/* Odd (fall thru) */
      case 'e':				/* Even */
	a = (ch & 15) ^ ((ch >> 4) & 15);
	a = (a & 3) ^ ((a >> 2) & 3);
	a = (a & 1) ^ ((a >> 1) & 1);
	if (xparity == 'o') a = 1 - a;	/* Switch sense for odd */
	return(ch | (a << 7));
      default: return(ch);
    }
}

/* Communication functions */

int
ttopen(ttname) char *ttname; {		/* "Open" the communication device */
    if (debug) {			/* Vital statistics for debug log */
#ifdef __STDC__
	fprintf(db,"ttopen __STDC__\n");
#endif /* __STDC__ */
#ifdef SIG_V
	fprintf(db,"ttopen SIG_V\n");
#else
#ifdef SIG_I
	fprintf(db,"ttopen SIG_I\n");
#endif /* SIG_I */
#endif /* SIG_V */
#ifdef USE_GETCHAR
	fprintf(db,"ttopen getchar/putchar\n");
#ifdef BUFSIZ
	fprintf(db,"ttopen BUFSIZ = %d\n", BUFSIZ);
#endif /* BUFSIZ */
#else
#ifdef DUMBIO
	fprintf(db,"ttopen single-byte read/write\n");
#else
	fprintf(db,"ttopen nonblocking read/write\n");
#endif /* DUMBIO */
#endif /* USE_GETCHAR */
	fprintf(db,"ttopen TINBUFSIZ = %d\n", TINBUFSIZ);
#ifdef __hpux
	fprintf(db,"ttopen __hpux\n");
#endif /* __hpux */
#ifdef pdp11
	fprintf(db,"ttopen pdp11\n");
#endif /* pdp11 */
#ifdef SETXONXOFF
	fprintf(db,"ttopen SETXONXOFF\n");
#endif /* SETXONXOFF */
	fprintf(db,"ttopen xonxoff = %d\n",xonxoff);
	fprintf(db,"ttopen noxonxoff = %d\n",noxonxoff);
	fprintf(db,"ttopen ttflags %d\n",ttflags);
	fprintf(db,"ttopen nomodes %d\n",nomodes);
    }
    if (nomodes) {			/* If external protocol */
#ifdef SIGINT				/* exit on interrupts */
	signal(SIGINT,doexit);
#endif /* SIGINT */
#ifdef SIGTSTP
	signal(SIGTSTP,doexit);
#endif /* SIGTSTP */
#ifdef SIGQUIT
	signal(SIGQUIT,doexit);
#endif /* SIGQUIT */
	return(0);
    }

#ifndef DUMBIO
#ifndef USE_GETCHAR
#ifdef O_NDELAY
#ifdef F_SETFL
    if (ttflags != -1) {		/* Set nonbocking i/o on stdin */
	errno = 0;
	if (fcntl(0, F_SETFL,ttflags|O_NDELAY) == -1)
	  logerr("ttopen fcntl(0,F_SETFL,O_NDELAY)");
	else
	  nonblock = 1;
    }
#endif /* F_SETFL */
#endif /* O_NDELAY */
#endif /* USE_GETCHAR */
#endif /* DUMBIO */
    if (!nonblock)			/* No streaming without */
      streamok = -1;			/* nonblocking reads */

    if (debug)
      fprintf(db,"ttopen nonblock = %d\n", nonblock);
#ifdef POSIX
    tcgetattr(0,&ttold);		/* Get stdin device attributes */
    tcgetattr(0,&ttraw);
#else
#ifdef SYSV
    ioctl(0,TCGETA,&ttold);
    ioctl(0,TCGETA,&ttraw);
#else
#ifdef BSD
    gtty(0,&ttold);
    gtty(0,&ttraw);
#endif /* BSD */
#endif /* SYSV */
#endif /* POSIX */
    havemodes++;
    return(0);
}

int
ttpkt(parity) int parity; {		/* Put comm device in packet mode */
#ifdef BSD
    int x;
#endif /* BSD */
    xparity = parity;			/* Make local copy of parity */
    if (nomodes)
      return(0);

#ifdef SVORPOSIX			/* System V or POSIX section... */
    ttraw.c_iflag |= IGNPAR;
    ttraw.c_lflag &= ~(ICANON|ECHO);
    ttraw.c_lflag &= ~ISIG;
    ttraw.c_lflag |= NOFLSH;
#ifdef SETXONXOFF
    if (!noxonxoff) {
	ttraw.c_iflag |= (IXON|IXOFF);
	if (debug) fprintf(db,"ttpkt SVORPOSIX Xon/Xoff\n");
    }
#else
    if (xonxoff) {
	if (debug) fprintf(db,"ttpkt SVORPOSIX Xon/Xoff\n");
	ttraw.c_iflag |= (IXON|IXOFF);
    }
#endif /* SETXONXOFF */
#ifdef IEXTEN
    ttraw.c_lflag &= ~IEXTEN;
#endif /* IEXTEN */
#ifdef POSIX
    ttraw.c_iflag &= ~(IGNBRK|INLCR|IGNCR|ICRNL|INPCK|ISTRIP);
#else
    ttraw.c_iflag &= ~(IGNBRK|INLCR|IGNCR|ICRNL|INPCK|ISTRIP|IXANY);
#endif /* POSIX */
    ttraw.c_oflag &= ~OPOST;
    ttraw.c_cflag &= ~(CSIZE);
    ttraw.c_cflag |= (CS8|CREAD|HUPCL);
    ttraw.c_cflag &= ~(PARENB);

#ifndef VEOF
    ttraw.c_cc[4] = 1;
#else
#ifdef VMIN
    ttraw.c_cc[VMIN] = 1;
#endif /* VMIN */
#endif /* VEOF */

#ifndef VEOL
    ttraw.c_cc[5] = 0;
#else
#ifdef VTIME
    ttraw.c_cc[VTIME] = 0;
#endif /* VTIME */
#endif /* VEOL */

#ifdef VINTR
    ttraw.c_cc[VINTR] = 0;
#endif /* VINTR */

#ifdef POSIX
    if (tcsetattr(0,TCSADRAIN,&ttraw) < 0)
      return(-1);
#else
    if (ioctl(0,TCSETAW,&ttraw) < 0)
      return(-1);
#ifdef SYSV
#endif /* SYSV */
#endif /* POSIX */

#else  /* Not SVORPOSIX */

#ifdef BSD
    ttraw.sg_flags |= RAW;		/* BSD/V7 raw (binary) mode */
#ifdef SETXONXOFF
    if (!noxonxoff) {
	ttraw.sg_flags |= TANDEM;
	if (debug) fprintf(db,"ttpkt BSD Xon/Xoff\n");
    }
#else
    if (xonxoff) {
	ttraw.sg_flags |= TANDEM;
	if (debug) fprintf(db,"ttpkt BSD Xon/Xoff\n");
    }
#endif /* SETXONXOFF */
    ttraw.sg_flags &= ~(ECHO|CRMOD);    /* No echo, etc */
    if (stty(0,&ttraw) < 0) return(-1); /* Set modes */
#else
    system("stty raw -echo");
#endif /* BSD */
#endif /* SVORPOSIX */
    raw = 1;				/* Flag we're now in raw mode */
    return(0);
}

int
ttres() {				/* Reset terminal */
    int x = 0;
    if (havemodes) {			/* Restore old modes */
#ifdef POSIX
	x = tcsetattr(0,TCSADRAIN,&ttold);
#else
#ifdef SYSV
	sleep(1);			/* Let output finish */
	x = ioctl(0,TCSETAW,&ttold);
#else
#ifdef BSD
	sleep(1);			/* Let output finish */
	x = stty(0,&ttold);
#else
	x = system("stty -raw echo");
#endif /* BSD */
#endif /* SYSV */
#endif /* POSIX */
    }
    write(1,"\015\012",2);
    raw = 0;
    return(x);
}

int
ttchk() {				/* Check if input ready */
    int x = 0;
    if (nonblock) {			/* Try to read */
	errno = 0;
	x = read(0,&tinbuf[tlast],TINBUFSIZ-tlast);
#ifdef EXTRADEBUG
	fprintf(db,"ttchk read %d errno = %d\n",x,errno);
#endif /* EXTRADEBUG */
#ifdef EWOULDBLOCK
	if (x < 0 && errno == EWOULDBLOCK) /* Nothing to read */
	  x = 0;
#endif /* EWOULDBLOCK */
	if (x < 0)			/* Fatal i/o error */
	  return(-1);
    }
    tincnt += x;			/* Buffer bookkeeping */
    tlast  += x;
    return(x + tincnt);			/* How much is waiting to be read */
}

int
ttflui() {				/* Flush comm device input buffer */
#ifdef BSD
    long n = 1;				/* Specify read queue */
#endif /* BSD */
    int x;
    tincnt = 0;				/* Our own buffer */
    tlast = 0;
    tinptr = tinbuf;
    errno = 0;
#ifdef POSIX
    x = tcflush(0,TCIFLUSH);		/* kernel/driver buffers */
#else
#ifdef SYSV
    x = ioctl(0,TCFLSH,0);
#else
#ifdef BSD
    x = ioctl(0,TIOCFLUSH,&n);
#endif /* BSD */
#endif /* SYSV */
#endif /* POSIX */
    if (debug) fprintf(db,"ttflui = %d, errno = %d\n",x,errno);
    return(x);
}

SIGTYP
timerh(dummy) int dummy; {		/* Timeout handler */
    longjmp(jbuf,1);
    SIGRETURN;
}

/*
 ttinl() - Read a raw packet.

 Call with:
   dest - where to put it
   max  - maximum length
   timo - timeout (seconds, 0 = none)
   eol  - packet terminator
   turn - half-duplex line turnaround character to wait for, 0 = none

 Returns length obtained, or -1 if error or timeout, -2 on disconnection.
*/
#ifndef DEBUGWRAP
#define DEBUGWRAP 48
#endif /* DEBUGWRAP */

int
#ifdef __STDC__
ttinl(char * dest, int max, int timo, char eol, char soh, int turn)
#else
ttinl(dest,max,timo,eol,soh,turn) int max, timo, turn; char eol, soh, *dest;
#endif /* __STDC__ */
{
    int n = 0, x = 0, flag = 0, rc = 0, ccn = 0; /* Local variables */
    char c = NUL;
    int havelen = 0, pktlen = 0, lplen = 0;

#ifdef USE_GETCHAR
    if (debug) fprintf(db,"ttinl getchar timo = %d\n",timo);
#else
    if (debug) fprintf(db,"ttinl read timo = %d\n",timo);
#endif /* USE_GETCHAR */
    *dest = NUL;			/* Clear destination buffer */
    if (timo) {
	signal(SIGALRM,timerh);		/* Enable timer interrupt */
	alarm(timo);			/* Set it. */
    }
    if (setjmp(jbuf)) {			/* Timer went off? */
	if (debug) fprintf(db,"ttinl timeout\n");
        rc = -1;			/* Yes, set this return code. */
    } else {				/* Otherwise... */
	while (1) {			/* Read until we have a packet */
#ifdef DUMBIO
	    x = read(0,&c,1);		/* Dumb blocking read byte loop */
	    if (x < 0) {
		logerr("ttinl XX read 1");
		rc = -2;
	    }
#else
#ifdef USE_GETCHAR
	    errno = 0;
	    x = getchar();		/* Buffered read with getchar() */
	    if (x == EOF) {
		if (errno == EINTR)
		  continue;
		logerr("ttinl getchar");
		rc = -2;
	    }
	    c = x;
#else  /* USE_GETCHAR */
#ifdef O_NDELAY
	    if (nonblock) {		/* Buffered nonblocking read() */
		int x;
		if (tincnt < 1) {	/* Need to fill our buffer */
		    errno = 0;
		    tincnt = read(0,tinbuf,TINBUFSIZ);
		    if (tincnt > -1) tlast = tincnt;
		    if (debug)
		      fprintf(db,"ttinl nonblock tincnt=%d errno=%d\n",
			      tincnt,errno);
		    if (tincnt == 0 || errno == EWOULDBLOCK) {
#ifdef F_SETFL
			/* Go back to blocking and wait for 1 char */
			if (ttflags != -1) {
			    errno = 0;
			    x = fcntl(0, F_SETFL, ttflags  & ~O_NDELAY);
			    if (x == -1 || errno)
			      logerr("ttinl fcntl O_NDELAY off");
			    errno = 0;
			    tincnt = read(0,tinbuf,1);
			    if (tincnt < 1 || errno)
			      logerr("ttinl BL read");
			    errno = 0;
			    fcntl(0, F_SETFL, ttflags | O_NDELAY);
			    if (x == -1 || errno)
			      logerr("ttinl fcntl O_NDELAY on");
			}
			if (tincnt == 0) { /* Check results */
			    continue;
			}
			if (tincnt < 0) { /* I/O error */
			    rc = -2;
			    goto xttinl;
			}
			if (debug)
			  fprintf(db,"ttinl blocking read %d\n",tincnt);
#else
			/* No other form of sleeping is portable */
			sleep(1);
			continue;
#endif /* F_SETFL */
		    } else if (tincnt < 0) {
			rc = -2;
			goto xttinl;
		    }
		    tinptr = tinbuf;
		}
		c = *tinptr++;
		tincnt--;
	    } else {
#endif /* O_NDELAY */
		x = read(0,&c,1);	/* Dumb read byte loop */
		if (x < 0) {
		    logerr("ttinl XX read 1");
		    rc = -2;
		}
#ifdef O_NDELAY
	    }
#endif /* O_NDELAY */
#endif /* USE_GETCHAR */
#endif /* DUMBIO */
	    if (rc < 0)
	      break;
	    if (xparity)		/* Strip parity */
	      c &= 0x7f;
#ifdef COMMENT
	    /* Only uncomment in emergencies */
	    if (debug)
	      fprintf(db,"ttinl char=%c flag=%d tincnt=%d\n",c,flag,tincnt);
#endif /* COMMENT */
	    if (c == '\03') {		/* Got ^C, count it. */
	    	if (++ccn > 2) {	/* If more than 2, let them out */
		    fprintf(stderr,"^C...");
		    ttres();
		    if (debug) fprintf(db,"ttinl interrupted\n");
		    dest[n = 0] = NUL;
		    rc = -9;
		    goto xttinl;
		}
	    } else			/* Not ^C so reset counter*/
	      ccn = 0;

	    if (!flag && (c != soh))	/* Look for SOH */
	      continue;			/* Skip stuff between packets */
	    flag = 1;			/* Have SOH */

	    if (n >= max) {
		if (debug) fprintf(db,"ttinl overflow\n");
		rc = -2;
		goto xttinl;
	    }
	    dest[n++] = c;		/* Store the character */
#ifdef USE_EOL
	    /* Use EOL to determine end of packet */
	    if (c == eol) {
		dest[n] = NUL;
		break;
	    }
#else
	    /* Use length field for framing */
	    if (!havelen) {
		if (n == 2) {
		    pktlen = xunchar(dest[1] & 0x7f);
		    if (pktlen > 1) {
			if (debug) fprintf(db,"ttinl length = %d\n",pktlen);
			havelen = 1;
		    }
		} else if (n == 5 && pktlen == 0) {
		    lplen = xunchar(dest[4] & 0x7f);
		} else if (n == 6 && pktlen == 0) {
		    pktlen = lplen * 95 + xunchar(dest[5] & 0x7f) + 5;
		    if (debug) fprintf(db,"ttinl length = %d\n",pktlen);
		    havelen = 1;
		}
	    }
	    if (havelen && (n > pktlen+1)) {
		if (turn && c != turn)	/* Wait for turnaround char */
		  continue;
		dest[n] = NUL;		/* Null-terminate whatever we got */
		break;
	    }
#endif /* USE_EOL */
	}
    }
  xttinl:				/* Common exit point */
    if (timo) {
	alarm(0);			/* Turn off the alarm */
	signal(SIGALRM,SIG_IGN);	/* and associated interrupt */
    }
    if (debug && n > 0) {		/* Log packet */
#ifndef FULLPACKETS
	if (n > DEBUGWRAP) {		/* Truncate if it would wrap */
	    dest[n] = NUL;		/* in case of interruption */
	    c = dest[DEBUGWRAP];
	    dest[DEBUGWRAP] = NUL;
	    fprintf(db,"PKT<-[^A%s...](%d) rc=%d\n",&dest[1],n,rc);
	    dest[DEBUGWRAP] = c;

	} else
#endif /* FULLPACKETS */
	    fprintf(db,"PKT<-[^A%s](%d) rc=%d\n",&dest[1],n,rc);
    }
    if (rc == -9)			/* Interrupted by user */
      doexit(1);
    else if (rc > -1)
      rc = n;
    return(rc);				/* Return length, or failure. */
}

int
ttol(s,len) int len; char *s; {		/* Output string s of given length  */
    register int i = 0, n = 0, m = 0;
    int partial = 0;

    n = len;
    if (n < 0) {
	if (debug) fprintf(db,"ttol len = %d\n",n);
	return(-1);
    }
    if (xparity) {			/* Add parity if requested */
	for (i = 0; i < n; i++)
	  s[i] = dopar(s[i]);
    }
    if (debug) {			/* Log the packet if requested */
	char c;
#ifndef FULLPACKETS
	if (n > DEBUGWRAP) {
	    c = s[DEBUGWRAP];
	    s[DEBUGWRAP] = NUL;
	    fprintf(db,"PKT->[^A%s...](%d)\n",&s[1],n);
	    s[DEBUGWRAP] = c;
	} else {
#endif /* FULLPACKETS */
	    c = s[n-1];
	    s[n-1] = NUL;
	    fprintf(db,"PKT->[^A%s](%d)\n",&s[1],n);
	    s[n-1] = c;
#ifndef FULLPACKETS
	}
#endif /* FULLPACKETS */
    }
#ifdef USE_GETCHAR
    {					/* Send the packet with putchar() */
	register CHAR c; register int i;
	for (i = 0; i < n; i++) {
	    c = *s++;
	    if (putchar(c) == EOF) {
		logerr("ttol putchar");
		return(-1);
	    }
	}
    }
    fflush(stdout);			/* Push it out */
    return(n);
#else
    while (n > 0) {			/* Send the packet with write() */
	i = write(1,&s[m],n);		/* Allowing for partial results */
	if (i < 0) {
	    if (errno == EWOULDBLOCK)	/* and even no results at all.. */
	      continue;
	    logerr("ttol write");
	    return(-1);
	}
	if (i == n)
	  break;
	partial++;
	m += i;
	if (debug) fprintf(db,"ttol partial write %d (%d/%d)\n",i,m,len);
	n -= i;
    }
    if (partial) {
	m += i;
	if (debug) fprintf(db,"ttol partial write %d (%d/%d)\n",i,m,len);
	if (m != len) {
	    if (debug) fprintf(db,"ttol foulup %d != %d\n",m,len);
	    return(-1);
	}
    }
    return(len);
#endif /* USE_GETCHAR */
}

/* File Functions */

char ofile[MAXPATHLEN];			/* Output filename */
long filelength = -1L;

long
zchki(fn) char * fn; {			/* Check if file is readable */
    struct stat buf;
    if (!fn) return(-1);
    if (stat(fn,&buf) < 0)
      return(-1);
    errno = 0;
    if (access(fn,R_OK) < 0) {
	if (debug)
	  fprintf(db,"zchki access %s errno = %d\n",fn,errno);
	return(-1);
    }
    if (!S_ISREG(buf.st_mode)) {
	if (debug)
	  fprintf(db,"zchki %s is a directory",fn);
	return(-2);
    }
    return(buf.st_size);
}

int
zchko(fn) char *fn; {			/* Check write access */
    int i, x;
    char * s;

    if (!fn)				/* Defend against empty name */
      fn = "";
    if (!*fn)
      return(-1);
    if (!strcmp(fn,"/dev/null"))	/* Null device is OK. */
      return(0);
    if ((x = zchki(fn)) == -2)		/* An existing directory? */
      return(-1);
    s = fn;
    if (x < 0) {			/* If file does not exist */
	strncpy(work,fn,MAXPATHLEN);
	work[MAXPATHLEN] = NUL;
	s = work;
	for (i = (int)strlen(s); i > 0; i--) { /* Strip filename from right */
	    if (s[i-1] == '/') {	/* and check its directory */
		s[i-1] = NUL;
		break;
	    }
	}
	if (i == 0)
	  s = ".";
    }
    errno = 0;
    x = access(s,W_OK);			/* Check access of path. */
    if (debug) fprintf(db,"zchko(%s) x = %d errno = %d\n",s,x,errno);
    return((x < 0) ? -1 : 0);           /* and return. */
}

int
zopeni(name) char *name; {		/* Open existing file for input */
    ifp = fopen(name,"r");
    if (debug) fprintf(db,"zopeni %s: %d\n",name, ifp ? 0 : errno);
    filelength = zchki(name);
    if (filelength < 0)
      return((int)filelength);
    zincnt = 0;
    zinptr = zinbuf;
    return((ifp == NULL) ? -1 : 0);
}

int
zopeno(name) char *name; {		/* Open new file for output */
    errno = 0;
    ofp = fopen(name,"w");
    if (debug) fprintf(db,"zopeno %s: %d\n",name, ofp ? 0 : errno);
    if (ofp) {
	strncpy(ofile,name,MAXPATHLEN);
	ofile[MAXPATHLEN-1] = NUL;
	return(0);
    } else
      return(-1);
}

VOID					/* Local to remote file name */
zltor(lclnam,pktnam,maxlen) char *lclnam, *pktnam; int maxlen; {
    char *p, *np = NULL, *cp, *pp, c;
    char *dotp = NULL;
    char *dirp = NULL;
    int n = 0;

    if (debug)
      fprintf(db,"zltor %s: maxlen = %d, literal = %d\n",
	      lclnam,maxlen,literal);
    if (literal) {
	p = lclnam;
	dirp = p;
	while (*p) {
	    if (*p == '/') dirp = p+1;
	    p++;
	}
	strncpy(pktnam,dirp,maxlen);
    } else {
	for (p = lclnam; *p; p++) {	/* Point to name part */
	    if (*p == '/')
	      np = p;
	}
	if (np) {
	    np++;
	    if (!*np) np = lclnam;
	} else
	  np = lclnam;

	if (debug)
	  fprintf(db,"zltor np %s\n",np);

	pp = work;			/* Output buffer */
	for (cp = np, n = 0; *cp && n < maxlen; cp++,n++) {
	    c = *cp;
	    if (islower(c))		/* Uppercase letters */
	      *pp++ = toupper(c);	/* Change tilde to hyphen */
	    else if (c == '~')
	      *pp++ = '-';
	    else if (c == '#')		/* Change number sign to 'X' */
	      *pp++ = 'X';
	    else if (c == '*' || c == '?') /* Change wildcard chars to 'X' */
	      *pp++ = 'X';
	    else if (c == ' ')		/* Change space to underscore */
	      *pp++ = '_';
	    else if (c < ' ')		/* Change space and controls to 'X' */
	      *pp++ = 'X';
	    else if (c == '.') {	/* Change dot to underscore */
		dotp = pp;		/* Remember where we last did this */
		*pp++ = '_';
	    } else {
		if (c == '/')
		  dirp = pp;
		*pp++ = c;
	    }
	}
	*pp = NUL;			/* Tie it off. */
	if (dotp > dirp) *dotp = '.';	/* Restore last dot in file name */
	cp = pktnam;			/* If nothing before dot, */
	if (*work == '.') *cp++ = 'X';	/* insert 'X' */
	strncpy(cp,work,maxlen);
	cp[maxlen-1] = NUL;
    }
    if (debug)
      fprintf(db,"zltor result: %s\n",pktnam);
}

int
zbackup(fn) char * fn; {		/* Back up existing file */
    struct stat buf;
    int i, j, k, x, state, flag;
    char *p, newname[MAXPATHLEN+12];

    if (!fn)				/* Watch out for null pointers. */
      return(-1);
    if (!*fn)				/* And empty names. */
      return(-1);
    if (stat(fn,&buf) < 0)		/* If file doesn't exist */
      return(0);			/* no need to back it up. */

    i = strlen(fn);			/* Get length */
    if (i > MAXPATHLEN)			/* Guard buffer */
      i = MAXPATHLEN;
    if (debug)
      fprintf(db,"zbackup A %s: %d\n", fn, i);

    strncpy(work,fn,MAXPATHLEN);	/* Make pokeable copy of name */
    work[MAXPATHLEN] = NUL;
    p = work;				/* Strip any backup prefix */

    i--;
    for (flag = state = 0; (!flag && (i > 0)); i--) {
	switch (state) {
	  case 0:			/* State 0 - final char */
	    if (p[i] == '~')		/* Is tilde */
	      state = 1;		/* Switch to next state */
	    else			/* Otherwise */
	      flag = 1;			/* Quit - no backup suffix. */
	    break;
	  case 1:			/* State 1 - digits */
	    if (p[i] == '~'  && p[i-1] == '.') { /* Have suffix */
		p[i-1] = NUL;		/* Trim it */
		flag = 1;		/* done */
	    } else if (p[i] >= '0' && p[i] <= '9') { /* In number part */
		continue;		/* Keep going */
	    } else {			/* Something else */
		flag = 1;		/* Not a backup suffix - quit. */
	    }
	    break;
	}
    }
    if (debug)
      fprintf(db,"zbackup B %s\n", p);
    if (!p[0])
      p = fn;
    j = strlen(p);
    strncpy(newname,p,MAXPATHLEN);
    for (i = 1; i < 1000; i++) {	/* Search from 1 to 999 */
	if (i < 10)			/* Length of numeric part of suffix */
	  k = 1;
	else if (i < 100)
	  k = 2;
	else
	  k = 3;
	x = j;				/* Where to write suffix */
	if ((x + k + 3) > MAXPATHLEN)
	  x = MAXPATHLEN - k - 3;
	sprintf(&newname[x],".~%d~",i); /* Make a backup name */
	if (stat(newname,&buf) < 0) {	/* If it doesn't exist */
	    errno = 0;
	    if (link(fn,newname) <  0) { /* Rename old file to backup name */
		if (debug)
		  fprintf(db,"zbackup failed: link(%s): %d\n",newname,errno);
		return(-1);
	    } else if (unlink(fn) < 0) {
		if (debug)
		  fprintf(db,"zbackup failed: unlink(%s): %d\n",fn,errno);
		return(-1);
	    } else {
		if (debug)
		  fprintf(db,"zbackup %s: OK\n",newname);
		return(0);
	    }
	}
    }
    if (debug)
      fprintf(db,"zbackup failed: all numbers used\n");
    return(-1);
}

int					/* Remote to local filename */
zrtol(pktnam,lclnam,warn,maxlen) char *pktnam, *lclnam; int warn, maxlen; {
    int acase = 0, flag = 0, n = 0;
    char * p;

    if (literal) {
	strncpy(lclnam,pktnam,maxlen);
    } else {
	for (p = lclnam; *pktnam != '\0' && n < maxlen; pktnam++) {
	    if (*pktnam > SP) flag = 1;	/* Strip leading blanks and controls */
	    if (flag == 0 && *pktnam < '!')
	      continue;
	    if (isupper(*pktnam))	/* Check for mixed case */
	      acase |= 1;
	    else if (islower(*pktnam))
	      acase |= 2;
	    *p++ = *pktnam;
	    n++;
	}
	*p-- = NUL;			/* Terminate */
	while (*p < '!' && p > lclnam)	/* Strip trailing blanks & controls */
	  *p-- = '\0';

	if (!*lclnam) {			/* Nothing left? */
	    strncpy(lclnam,"NONAME",maxlen); /* do this... */
	} else if (acase == 1) {	/* All uppercase? */
	    p = lclnam;			/* So convert all letters to lower */
	    while (*p) {
		if (isupper(*p))
		  *p = tolower(*p);
		p++;
	    }
	}
    }
    if (warn) {
	if (zbackup(lclnam) < 0)
	  return(-1);
    }
    return(0);
}

int
zclosi() {				/* Close input file */
    int rc;
    rc = (fclose(ifp) == EOF) ? -1 : 0;
    ifp = NULL;
    return(rc);
}

int
zcloso(cx) int cx; {			/* Close output file */
    int rc;
    rc = (fclose(ofp) == EOF) ? -1 : 0;
    if (debug) fprintf(db,"zcloso(%s) cx = %d keep = %d\n", ofile, cx, keep);
    if (cx && !keep ) unlink(ofile);	/* Delete if incomplete */
    ofp = NULL;
    return(rc);
}

int
zfillbuf(text) int text; {		/* Refill input file buffer */
    if (zincnt < 1) {			/* Nothing in buffer - must refill */
	if (text) {			/* Text mode needs LF/CRLF handling */
	    int c;			/* Current character */
	    for (zincnt = 0;		/* Read a line */
		 zincnt < MAXRECORD - 1 && (c = getc(ifp)) != EOF && c != '\n';
		 zincnt++
		 ) {
		zinbuf[zincnt] = c;
	    }
	    if (c == '\n') {		/* Have newline. */
		zinbuf[zincnt++] = '\r'; /* Substitute CRLF */
		zinbuf[zincnt++] = c;
	    }
	} else {			/* Binary - just read raw buffers */
	    zincnt = fread(zinbuf, sizeof(char), MAXRECORD, ifp);
	}
	zinbuf[zincnt] = NUL;		/* Terminate. */
	if (zincnt == 0)		/* Check for EOF */
	  return(-1);
	zinptr = zinbuf;		/* Not EOF - reset pointer */
    }
#ifdef EXTRADEBUG				/* Voluminous debugging */
    if (debug) fprintf(db,"zfillbuf (%s) zincnt = %d\n",
		       text ? "text" : "binary",
		       zincnt
		       );
#endif /* EXTRADEBUG */
    zincnt--;				/* Return first byte. */
    return(*zinptr++ & 0xff);
}
