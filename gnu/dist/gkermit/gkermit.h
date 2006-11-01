/* G K E R M I T . H  --  GNU Kermit header  */
/*
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

#ifndef _GKERMIT_H
#define _GKERMIT_H

#include <stdio.h>

/* Kermit protocol definitions */

#define MAXTRY 10			/* Packet retry limit */

#ifndef SMALL				/* Define small systems here */
#ifdef pdp11				/* PDP-11 (64K address space) */
#define SMALL
#endif /* pdp11 */
#endif /* SMALL */

#ifdef SMALL				/* Buffer sizes for small systems */

#define MAXSP 4000
#define MAXRP 4000
#define DEFRP 1000
#define MAXPATHLEN 255
#define DEVNAMLEN 64
#define MAXRECORD 255

#else  /* 32-bit and 64-bit platforms... */

#ifndef MAXSP
#define MAXSP 9000			/* Maximum send packet size */
#endif /* MAXSP */

#ifndef MAXRP				/* Maximum receive packet size */
#define MAXRP 9000
#endif /* MAXRP */

#ifndef DEFRP				/* Default receive packet size */
#define DEFRP 4000
#endif /* DEFRP */

#endif /* SMALL */

#define CTTNAM  "/dev/tty"		/* Default terminal name */

/* Sizes for file/device-name and file i/o buffers */

#ifndef MAXPATHLEN			/* Maximum file specification length */
#define MAXPATHLEN 1024
#endif /* MAXPATHLEN */

#ifndef MAXNAMLEN			/* Maximum file name length */
#define MAXNAMLEN 256
#endif /* MAXNAMLEN */

#ifndef DEVNAMLEN			/* Maximum device name length */
#define DEVNAMLEN 1024
#endif /* DEVNAMLEN */

#ifndef MAXRECORD			/* Text file input buffer length */
#define MAXRECORD 4080
#endif /* MAXRECORD */

#ifdef __STDC__
#define VOID void
#else
#define VOID int
#endif /* __STDC__ */

/* SVORPOSIX = System V or POSIX */

#ifndef SVORPOSIX
#ifdef POSIX
#define SVORPOSIX
#else
#ifdef SYSV
#define SVORPOSIX
#endif /* SYSV */
#endif /* POSIX */
#endif /* SVORPOSIX */

/* Signal type */

#ifndef SIG_V
#ifndef SIG_I
#ifdef SVORPOSIX
#define SIG_V
#else
#define SIG_I
#endif /* SVORPOSIX */
#endif /* SIG_I */
#endif /* SIG_V */

#ifdef SIG_I
#define SIGRETURN return(0)
#ifndef SIGTYP
#define SIGTYP int
#endif /* SIGTYP */
#else
#ifdef SIG_V
#define SIGRETURN return
#ifndef SIGTYP
#define SIGTYP void
#endif /* SIGTYP */
#endif /* SIG_V */
#endif /* SIG_I */

/* WHATAMI bit definitions */

#define WMI_FMODE   2			/* File transfer mode */
#define WMI_FNAME   4			/* File name conversion */
#define WMI_STREAM  8			/* I have a reliable transport */
#define WMI_CLEAR  16			/* I have a clear channel */
#define WMI_FLAG   32			/* Flag that WHATAMI field is valid */
#define WMI2_XMODE  1			/* Transfer mode auto(0)/manual(1) */
#define WMI2_RECU   2			/* Transfer is recursive */
#define WMI2_FLAG  32			/* Flag that WHATAMI2 field is valid */

/* Data types */

#ifndef CHAR
#define CHAR unsigned char
#endif /* CHAR */

#ifndef ULONG
#define ULONG unsigned long
#endif /* ULONG */

/* Pointer and character constants */

#ifndef NULL
#define NULL 0				/* NULL pointer */
#endif /* NULL */

#define NUL '\000'			/* ASCII NUL character */
#define LF  '\012'			/* ASCII Linefeed */
#define CR  '\015'			/* ASCII Carriage Return */
#define SP  '\040'			/* ASCII space character */
#define DEL 127				/* ASCII DEL */

/* Macros */

#define tochar(ch)  ((ch) + SP )	/* Number to character */
#define xunchar(ch) ((ch) - SP )	/* Character to number */
#define ctl(ch)     ((ch) ^ 64 )	/* Controllify/Uncontrollify */
#define zgetc(a) (((--zincnt)>=0) ? ((int)(*zinptr++) & 0xff) : zfillbuf(a))

/* Function prototype macro works for both ANSI and K&R C */

#ifdef __STDC__
#define _MYPROTOTYPE( func, parms ) func parms
#else
#define _MYPROTOTYPE( func, parms ) func()
#endif /* __STDC__ */

/* Function prototypes */

_MYPROTOTYPE( SIGTYP doexit, (int) );
_MYPROTOTYPE( VOID sysinit, (void) );
_MYPROTOTYPE( char dopar, (char) );
_MYPROTOTYPE( VOID tmsg, (char *) );
_MYPROTOTYPE( VOID tmsgl, (char *) );
_MYPROTOTYPE( int ttopen, (char *) );
_MYPROTOTYPE( int ttpkt, (int) );
_MYPROTOTYPE( int ttres, (void) );
_MYPROTOTYPE( int ttinl, (char *, int, int, char, char, int) );
_MYPROTOTYPE( int ttol, (char *, int) );
_MYPROTOTYPE( int ttchk, (void) );
_MYPROTOTYPE( int ttflui, (void) );
_MYPROTOTYPE( long zchki, (char *) );
_MYPROTOTYPE( int zchko, (char *) );
_MYPROTOTYPE( int zopeni, (char *) );
_MYPROTOTYPE( int zopeno, (char *) );
_MYPROTOTYPE( int zclosi, (void) );
_MYPROTOTYPE( int zcloso, (int) );
_MYPROTOTYPE( int zfillbuf, (int) );
_MYPROTOTYPE( VOID zltor, (char *, char *, int) );
_MYPROTOTYPE( int zrtol, (char *, char *, int, int) );
_MYPROTOTYPE( int zbackup, (char *) );

_MYPROTOTYPE( int input, (void) );	/* Input to state machine (like lex) */
_MYPROTOTYPE( VOID nxtpkt, (void) );	/* Increment packet number */
_MYPROTOTYPE( int ack, (void) );	/* Send empty Acknowledgment */
_MYPROTOTYPE( int ack1, (char *) );	/* Send data-bearing Acknowledgment */
_MYPROTOTYPE( int nak, (void) );	/* Send Negative acknowledgement */
_MYPROTOTYPE( VOID tinit, (void) );	/* Transaction initialization */
_MYPROTOTYPE( VOID errpkt, (char *) );	/* Send error packet */
_MYPROTOTYPE( int sinit, (char) );	/* Send S packet */
_MYPROTOTYPE( int sfile, (void) );	/* Send File header packet */
_MYPROTOTYPE( int sdata, (void) );	/* Send Data packet */
_MYPROTOTYPE( int seof, (char *) );	/* Send EOF packet */
_MYPROTOTYPE( int seot, (void) );	/* Send EOT packet */
_MYPROTOTYPE( int resend, (void) );	/* Resend a packet */
_MYPROTOTYPE( int decode, (int) );	/* Decode packet data field */
_MYPROTOTYPE( int encstr, (char *) );	/* Encode a memory string */
_MYPROTOTYPE( int gattr, (char *) );	/* Get incoming file attributes */
_MYPROTOTYPE( int sattr, (void) );	/* Send file attributes */
_MYPROTOTYPE( VOID ginit, (void) );	/* Transaction initialization */
_MYPROTOTYPE( int scmd, (char, char *) ); /* Send command to Kermit server */
_MYPROTOTYPE( VOID rinit, (void) );	/* Receive initialization */
_MYPROTOTYPE( int gnfile, (void) );	/* Get next filename */
_MYPROTOTYPE( int rcvfil, (void) );	/* Receive incoming file */
_MYPROTOTYPE( VOID spar, (char *) );	/* Set parameters from other Kermit */
_MYPROTOTYPE( char *rpar, (void) );	/* Tell parameters to other Kermit */
_MYPROTOTYPE( VOID usage, (void) );	/* Give usage message */
_MYPROTOTYPE( int gwart, (void) );	/* State table switcher */

/* Externs */

#ifdef ERRNO_H
#include <errno.h>
#else
extern int errno;
#endif /* ERRNO_H */
#ifndef _GKERMIT_C
extern int debug;
#endif /* _GKERMIT_C */

#endif /* _GKERMIT_H */

/* End gkermit.h */
