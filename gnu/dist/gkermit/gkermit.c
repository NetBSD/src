/* G K E R M I T  --  GNU Kermit  */

/*
  A free implementation of the Kermit file transfer protocol for UNIX.
  If you change this software, please either (a) send changes back to the
  Kermit Project to be considered for the base version; or (b) change the
  strings below to show a new version number and date and to reflect the
  person or organization responsible for the new version.

  Sat Dec 25 19:22:54 1999
*/

char *versio = "G-Kermit CU-1.00, Columbia University, 1999-12-25";
char *url =    "http://www.columbia.edu/kermit/";
char *email =  "kermit@columbia.edu";
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
#ifdef POSIX
char *build =  "POSIX";			/* Identify which build */
#else
#ifdef SYSV
char *build =  "SYSV";
#else
#ifdef BSD
char *build =  "BSD";
#else
char *build =  "stty";
#endif /* BSD */
#endif /* SYSV */
#endif /* POSIX */

#define _GKERMIT_C
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "gkermit.h"

/* Forward declarations of functions used within this module... */

_MYPROTOTYPE( int cmdlin, (void) );	/* Command-line parser */
_MYPROTOTYPE( int spacket, (char,int,int,char *) ); /* Make & send a packet */
_MYPROTOTYPE( int rpacket, (void) );	/* Read a packet */
_MYPROTOTYPE( unsigned int chk1, (char *,int) ); /* 1-byte block check */
_MYPROTOTYPE( unsigned int chk3, (char *,int) ); /* 3-byte CRC */
_MYPROTOTYPE( unsigned int chksum, (char *,int) ); /* Checksum */
_MYPROTOTYPE( int getpkt, (int) );	/* Fill packet data field */
_MYPROTOTYPE( VOID encode, (int,int) ); /* Encode a character */
_MYPROTOTYPE( VOID decstr, (char *) );	/* Decode a memory string */

/* Externs */

extern int zincnt;			/* For buffered file i/o */
extern char * zinptr;
extern FILE * ofp;			/* Output file pointer */

/* Global variables declared here */

char *cmarg, *cmarg2 = NULL;		/* Send filename pointers */
char **cmlist = NULL;			/* Pointer to file list in argv */

FILE * db = NULL;			/* Debug log file pointer */
int debug = 0;				/* Debugging on */
int failure = 0;			/* Return status */
int retries = 0;			/* Packet retry counter */
int sendtype = 0;			/* Type of last packet sent */
int recvtype = 0;			/* Type of last packet received */
int datalen = 0;			/* Length of received data field */

/* Protocol parameters */

int spsiz = 90,				/* Default send-packet size */
    rpsiz = 94,				/* Default short receive-packet size */
    urpsiz = DEFRP,			/* Default long receive-packet size */
    maxrp = MAXRP,			/* Maximum  long receive-packet size */
    timint = 9,				/* Timeout interval I use */
    rtimo = 7,				/* Timeout I want you to use */
    rpadn = 0,				/* How much padding to send */
    spadn = 0,				/* How much padding to ask for */
    bctr = 3,				/* Block check type requested */
    bctu = 1,				/* Block check type used */
    ebq =  '&',				/* 8th bit prefix */
    ebqflg = 0,				/* 8th-bit quoting flag */
    rqf = -1,				/* Flag used in 8bq negotiation */
    rq = 0,				/* Received 8bq bid */
    sq = 'Y',				/* Sent 8bq bid */
    rpt = 0,				/* Repeat count */
    rptq = '~',				/* Repeat prefix */
    rptflg = 0,				/* Repeat processing flag */
    backup = 1,				/* Back up existing files */
    seq = 0,				/* Current packet number */
    size,				/* Current size of output pkt data */
    osize,				/* Previous output packet data size */
    maxsiz,				/* Max size for building data field */
    rln,				/* Received packet length */
    rsn,				/* Received packet sequence number */
    sndpkl;				/* Length of packet being sent */

char spadc = 0,				/* Padding character to send */
    rpadc = 0,				/* Padding character to ask for */
    seol = CR,				/* End-Of-Line character to send */
    reol = CR,				/* End-Of-Line character to look for */
    rctlq = '#',			/* Control prefix in incoming data */
    sctlq = '#',			/* Outbound control character prefix */
    sndpkt[MAXSP+16],			/* Entire packet being sent */
    rcvpkt[MAXRP+16], 			/* Packet most recently received */
    *rdatap,				/* Pointer to data field of rcvpkt */
    xxdata[MAXRP+16],			/* Packet data buffer */
    *isp = NUL,				/* Input string pointer */
    *osp = NUL,				/* Output string pointer */
    smark = '\1',			/* Outbound packet-start character */
    rmark = '\1';			/* Incoming packet-start character */

char * xdata = xxdata;			/* Packet data field pointer */

/* File-related variables */

char filnam[MAXPATHLEN+1];		/* Name of current file. */
char ttname[DEVNAMLEN+1];		/* Name of communication device. */

int nfils;				/* Number of files to send */
int cx = 0, cz = 0;			/* File and batch interruption flags */
int quiet = 0;				/* Suppress messages */
int keep = 0;				/* Keep incompletely received files */
int streamok = 0;			/* Streaming protocol negotiated */
int streaming = 0;			/* Streaming active now */
int nomodes = 0;			/* Don't mess with tty modes */
int xonxoff = 0;			/* Force Xon/Xoff */
int noxonxoff = 0;			/* Don't force Xon/Xoff */
int manual = 0;				/* Transfer mode manual, not auto */
int parity = 0;				/* Parity specified, 0,'e','o',etc */
int delay = 1;				/* Initial delay before sending */
int text = 0;				/* Flag for text/binary mode */
int first = 0;				/* Flag for first input from file */
int longpackets = 0;			/* Long packets negotiated */
int attributes = 0;			/* Attribute packets negotiated */
int literal = 0;			/* Literal filenames */
char start  = 0;			/* Starting state for automaton */

char **xargv;				/* Global copies of argv */
int  xargc;				/* and argc */
char *dftty = CTTNAM;			/* Default controlling terminal name */

char *
#ifdef __STDC__
mystrchr(char * s, char c)
#else
mystrchr(s,c) char *s, c;
#endif /* __STDC__ */
{
    if (!s)				/* strchr() replacement */
      return(NULL);			/* because strchr not portable */
    while (*s && *s != c)
      s++;
    return((*s == c) ? s : NULL);
}

int					/* Main Program */
main(argc,argv) int argc; char **argv; {
    xargc = argc;			/* Make global copies of argc */
    xargv = argv;			/* ...and argv. */
    start = 0;				/* No default start state. */
    seq = 0;				/* Packet sequence number. */
    parity = 0;				/* Initial parity. */
    strncpy(ttname,dftty,DEVNAMLEN);	/* Default device name. */

    if (argc < 2) {			/* If no args */
	usage();			/* give usage message */
	doexit(1);			/* and quit. */
    }
    sysinit();				/* Set up interrupt handlers, etc */
    if (urpsiz > MAXRP)			/* In case MAXRP < DEFRP... */
      urpsiz = MAXRP;
    if (maxrp > 9020)
      maxrp = 9020;
    start = cmdlin();			/* Parse args */
    if (start == 0 && !debug) {		/* Nothing to do? */
	fprintf(stderr,"No start state\n");
	doexit(1);
    }
    if (ttopen(ttname) < 0) {		/* "Open" the communication device */
	fprintf(stderr,"Can't open terminal\n");
	doexit(1);
    }
    if (ttpkt(parity) < 0) {		/* Open OK, put it in packet mode */
	printf("Can't set packet mode\n");
	doexit(1);
    }					/* OK, do requested actions. */
    if (start) {			/* Start state */
	if (start == 's' || start == 'r') /* If sending or getting*/
	  ttflui();			/* flush comm line input buffer */
	gwart();			/* Do the protocol */
    }
    doexit(failure);			/* Done, exit. */
    return(failure);			/* To shut up picky compilers. */
}

/* Functions */

int					/* Called implicitly by protocol */
input() {				/* to get the next packet. */
    int x, type;
    if (start != 0) {			/* Start state in effect? */
	type = start;			/* Yes, call it a packet type, */
	start = 0;			/* nullify the start state, */
	if (debug) fprintf(db,"input[%c]\n",(char)type);
	return(type);			/* and return the type. */
    }
    if (streaming && (sendtype == 'D')) { /* If streaming and sending data */
	x = ttchk();			/* Look for return traffic */
	if (debug)
	  fprintf(db,"input streaming ttchk %d\n",x);
	if (x < 0)
	  errpkt("Fatal i/o error");
	else if (x < 5)
	  return('Y');			/* If none, pretend we got an ACK */
	timint = 4;
	type = rpacket();		/* Something came in, read it */
	timint = 0;
	if (mystrchr("TQN",type))
	  errpkt("Transmission error while streaming");
    } else {
	type = rpacket();		/* No start state, read a packet. */
	while (rsn != seq ||		/* Sequence number doesn't match, or */
	       mystrchr("TQN",type)	/* Timeout, Checksum error, or NAK */
	       ) {
	    if (streaming)
	      errpkt("Transmission error while streaming");
	    resend();			/* Resend previous packet. */
	    type = rpacket();		/* Try to read response. */
	}
    }
    if (!streaming)			/* Got a good packet */
      ttflui();				/* Flush buffer if not streaming */
    if (debug)
      fprintf(db,"input[%c]\n",(char)type);
    return(type);			/* Return its type */
}

VOID
nxtpkt() {				/* Next packet */
    retries = 0;			/* Reset per-packet retry count */
    seq = (seq + 1) & 63;		/* Next packet number, mod 64 */
}

int
ack() {					/* Acknowledge */
    int x = 0;				/* Empty acknowledgement */
    if (!streaming || recvtype != 'D')
      x = spacket('Y',seq,0,"");	/* Send the packet */
    nxtpkt();				/* Increment packet number */
    return(x);
}

int
ack1(s) char *s; {			/* Acknowledgement with data */
    int x;
    x = spacket('Y',seq,strlen(s),s);	/* Send the packet */
    nxtpkt();				/* Increment packet number */
    return(x);
}

int
nak() {					/* Negative acknowledgement */
    int x;
    retries++;
    if (debug)
      fprintf(db,"nak #%d (%d/%d)\n",seq,retries,MAXTRY);
    if (retries > MAXTRY) {		/* Check retry limit */
	errpkt("Too many retries");
	doexit(1);
    }
    x = spacket('N',seq,0,"");		/* A NAK never has data */
    return(x);
}

VOID
tinit() {				/* Transaction Initialization */
    osp = NULL;				/* Reset output string pointer */
    cx = cz = 0;			/* File interruption flags off */
    bctu = 1;				/* Block check back to 1 */
    ebqflg = rptflg = 0;		/* 8th bit & repeat quoting off */
    sq = 'Y';				/* Normal 8th bit quote bid */
    rqf = -1;				/* Flag that no 8-b-q bid seen yet */
    seq = 0;				/* Start off with packet zero */
    *filnam = *sndpkt = *rcvpkt = NUL;	/* Clear out string buffers */
}

VOID
errpkt(s) char *s; {			/* Fatal error */
    if (start == 'v' || start == 'r')	/* Receiving a file? */
      sleep(1);				/* Time to soak up incoming junk. */
    spacket('E',seq,(int)strlen(s),s); 	/* Send Error packet. */
    doexit(1);				/* Exit with failure status. */
}

int
#ifdef __STDC__
sinit(char c)
#else
sinit(c) char c;
#endif /* __STDC__ */
{					/* Begin send */
    char *s;
    s = rpar();				/* Get our protocol parameters */
    if (c == 'S') {			/* Sending a file... */
	tmsgl(versio);
	tmsgl("Escape back to your local Kermit and give a RECEIVE command.");
	tmsgl("");
	tmsgl("KERMIT READY TO SEND...");
	sleep(delay);
    }
    return(spacket(c,seq,strlen(s),s));	/* Send S or I packet */
}

int
sfile() {				/* Send file header packet */
    int x;
    char pktnam[MAXPATHLEN];
    if (debug)
      fprintf(db,"sfile filnam [%s] max = %d\n",filnam,MAXPATHLEN);
    if (zopeni(filnam) < 0) {		/* Open the input file */
	if (debug)
	  fprintf(db,"sfile error %d",errno);
	errpkt("Failure to open file");
    }
    if (cmarg2)				/* User-supplied name - use as-is */
      strncpy(pktnam,cmarg2,MAXPATHLEN);
    else				/* Actual filename - convert */
      zltor(filnam,pktnam,MAXPATHLEN);
    cmarg2 = NULL;			/* Only works for one file */
    if (debug)
      fprintf(db,"sfile pktnam %s\n",pktnam);
    x = encstr(pktnam);			/* Encode name */
    if (debug)
      fprintf(db,"sfile encstr[%s] x=%d\n",xdata,x);
    first = 1;				/* Starting condition for file */
    maxsiz = spsiz - (bctr + 3);	/* Maximum packet data field length */
    if (spsiz > 94) maxsiz -= 4;	/* Long packet has more fields */
    nxtpkt();				/* Get next packet number */
    return(spacket('F',seq,x,xdata));	/* Send filename */
}

int
sattr() {				/* Build and send A packet */
    int i, aln;
    int binary;
    extern long filelength;

    nxtpkt();				/* Get next packet sequence number */
    if (debug) fprintf(db,"sattr seq %d\n",seq);
    i = 0;				/* i = Data field character number */
    binary = !text;			/* (for notational convenience) */

    xdata[i++] = '"';
    if (binary) {			/* Binary */
	xdata[i++] = tochar(2);		/*  Two characters */
	xdata[i++] = 'B';		/*  B for Binary */
	xdata[i++] = '8';		/*  8-bit bytes (note assumption...) */
    } else {				/* Text */
	xdata[i++] = tochar(3);		/*  Three characters */
	xdata[i++] = 'A';		/*  A = (extended) ASCII with CRLFs */
	xdata[i++] = 'M';		/*  M for carriage return */
	xdata[i++] = 'J';		/*  J for linefeed */
	xdata[i++] = '*';		/* Encoding */
	xdata[i++] = tochar(1);		/* Length of value is 1 */
	xdata[i++] = 'A';		/* A for ASCII */
    }
    if (filelength > -1L) {
	xdata[i] = '1';			/* File length in bytes */
	sprintf((char *) &xdata[i+2],"%ld",filelength);
	aln = (int)strlen((char *)(xdata+i+2));
	xdata[i+1] = tochar(aln);
	i += aln + 2;
    }
    xdata[i] = NUL;			/* Terminate last good field */
    aln = (int)strlen((char *)xdata);	/* Get overall length of attributes */
    if (debug) fprintf(db,"sattr data %s\n",xdata);
    return(spacket('A',seq,aln,xdata)); /* Send it */
}
#define FTBUFL 10			/* File type buffer */

int
gattr(s) char *s; {			/* Get attributes from other Kermit */
    char c;
    int aln, i;
    static char ftbuf[FTBUFL+1];
    int retcode;			/* Return code */

    retcode = 0;			/* Initialize return code. */
    while ((c = *s++)) {		/* Get attribute tag */
	aln = xunchar(*s++);		/* Length of attribute string */
	switch (c) {
	  case '"':			/* File type (text, binary, ...) */
	    for (i = 0; (i < aln) && (i < FTBUFL); i++)
	      ftbuf[i] = *s++;		/* Copy it into a static string */
	    ftbuf[i] = '\0';
	    if (i < aln) s += (aln - i);
	    if ((*ftbuf != 'A' && *ftbuf != 'B' && *ftbuf != 'I')) {
		retcode = -1;	/* Reject the file */
		break;
	    }
	    if (*ftbuf == 'A') {	/* Check received attributes. */
		text = 1;		/* Set current type to Text. */
	    } else if (*ftbuf == 'B') {
		text = 0;
	    }
	    break;

	  default:			/* Unknown attribute */
	    s += aln;			/* Just skip past it */
	    break;
	}
    }
    return(retcode);
}


int
sdata() {				/* Send a data packet */
    int x;
    if (cx || cz) {			/* Interrupted... */
	if (debug)
	  fprintf(db,"sdata interrupted cx = %d cz = %d\n",cx,cz);
	failure = 1;
	return(0);
    }
    x = getpkt(maxsiz);			/* Fill data field from input file */
    if (x < 1) {
	if (debug)
	  fprintf(db,"sdata getpkt = %d\n",x);
	return(0);
    }
    nxtpkt();				/* Get next packet number */
    return(spacket('D',seq,x,xdata));	/* Send the packet */
}

int
seof(s) char *s; {			/* Send end-of-file packet */
    if (debug) fprintf(db,"seof [%s]\n",s);
    if (zclosi() < 0) {			/* Close the file */
	if (debug)
	  fprintf(db,"seof zclosi failure %d\n",errno);
	errpkt("Failure to close input file");
    } else {
	int x;
        nxtpkt();			/* Get next packet number */
	x = spacket('Z',seq,strlen(s),s); /* Send EOF packet */
	if (debug)
	  fprintf(db,"seof spacket %d\n",x);
        return(x);
    }
    return(0);				/* To shut up picky compilers */
}

int
seot() {				/* Send end-of-transmission packet */
    nxtpkt();
    return(spacket('B',seq,0,""));
}

int
closof() {				/* Close output file */
    if (zcloso(cx|cz) < 0)
      errpkt("Failure to close output file");
    return(0);
}

int					/* Frame and send a packet */
#ifdef __STDC__
spacket(char type,int n,int len,char *d)
#else
spacket(type,n,len,d) char type, *d; int n, len;
#endif /* __STDC__ */
{
    register int i = 0;
    int j, k, m, x;
    char * p = sndpkt;

    for (i = 0; i < spadn; i++)		/* Do requested padding */
      p[i] = spadc;

    p[i++] = smark;			/* Send-packet mark */
    k = i++;				/* Remember this place */
    p[i++] = tochar(n);			/* Sequence number */
    p[i++] = type;			/* Packet type */
    j = len + bctu;
    if ((len + bctu + 2) > 94) {	/* If long packet */
	x = j / 95;			/* Use long-packet format */
	p[k] = tochar(0);
	p[i++] = tochar(x);
	x = j % 95;
	p[i++] = tochar(x);
        p[i] = NUL;			/* Terminate for header checksum */
        p[i++] = tochar(chk1(&p[k],5));	/* Insert header checksum */
    } else				/* Short packet */
      p[k] = tochar(j+2);
    for (j = len; j > 0; j--) {		/* Data */
        p[i++] = *d++;
    }
    p[i] = NUL;				/* Null-terminate */
    m = i - k;
    switch (bctu) {			/* Block Check Type Used? */
      case 1:				/* Type 1 - 6 bit checksum */
	p[i++] = tochar(chk1(p+k,m));
	break;
      case 2:				/* Type 2 - 12 bit checksum */
	j = chksum(p+1,m);
	p[i++] = tochar((j >> 6) & 077);
	p[i++] = tochar(j & 077);
	break;
      case 3:				/* Type 3 - 16 bit CRC-CCITT */
	j = chk3(p+1,m);
	p[i++] = tochar((j >> 12) & 017);
	p[i++] = tochar((j >> 6) & 077);
	p[i++] = tochar(j & 077);
	break;
    }
    p[i++] = seol;			/* End of line */
    p[i] = NUL;				/* Null string-terminator */
    sndpkl = i;				/* Remember length. */
    x = ttol(p,sndpkl);			/* Send the packet. */
    if (x > -1)
      sendtype = type;			/* Remember its type */
    return(x);
}

int
resend() {				/* Resend a packet */
    int x;
    if (*sndpkt) {
	retries++;
	if (debug) fprintf(db,"resend #%d (%d/%d)\n",seq,retries,MAXTRY);
	if (retries > MAXTRY) {		/* Check retry limit */
	    errpkt("Too many retries");
	    doexit(1);
	}
	x = ttol(sndpkt,sndpkl);   	/* Send previous packet */
    } else {
	x = nak();			/* or NAK if nothing sent yet. */
    }
    return(x);
}

int
rpacket() {				/* Read a packet */
    register int i = 0;
    int j, x, type, rlnpos, rpktl;	/* Local variables */
    unsigned int c1, c2;
    char * p = rcvpkt;
    char pbc[5];			/* Packet block check */

    rsn = rln = -1;			/* In case of failure. */

    *p = NUL;				/* Initialize the receive buffer. */
    j = ttinl(p,maxrp,timint,reol,rmark,0); /* Read a raw packet */
    if (j < 5) {			/* Error */
	if (j == -1) return('T');	/* Timed out. */
	if (j < -1) doexit(1);		/* I/O error, hangup, etc. */
	if (debug) fprintf(db,"rpacket runt");
	return('Q');			/* Runt */
    }
    rpktl = j;				/* Remember length. */
    rlnpos = ++i;			/* Length position. */
    if ((j = xunchar(p[i++])) == 0) {	/* Get packet length.  */
        if ((j = rlnpos+5) > rpktl) {	/* Long packet */
	    if (debug) fprintf(db,"rpacket bad length");
	    return('Q');		/* Too long */
	}
	x = p[j];			/* Header checksum. */
	p[j] = NUL;			/* Calculate & compare. */
	if (xunchar(x) != chk1(p+rlnpos,5)) {
	    if (debug)
	      fprintf(db,"rpacket header checksum: %u\n",x);
	    return('Q');
	}
	p[j] = x;			/* Checksum ok, put it back. */
	rln = xunchar(p[j-2]) * 95 + xunchar(p[j-1]) - bctu;
	j = 3;				/* Data offset. */
    } else if (j < 3) {			/* Bad length */
	if (debug) fprintf(db,"rpacket bad xlength");
	return('Q');
    } else {				/* Regular packet */
	rln = j - bctu - 2;		/* No extended header */
	j = 0;
    }
    rsn = xunchar(p[i++]);		/* Sequence number */
    type = p[i++];			/* Packet type */
    if (debug)
      fprintf(db,"rpacket type=%c, seq=%02d, len=%d\n",type,rsn,rln);
    i += j;
    rdatap = p+i;			/* The data itself */
    j = rln + i;			/* Position of block check */
    for (x = 0; x < bctu; x++)		/* Copy the block check. */
      pbc[x] = p[j+x];
    pbc[x] = NUL;
    p[j] = NUL;				/* Null-terminate the data */
    datalen = j - i;			/* Remember data length */

    switch (bctu) {			/* Check the block check */
      case 1:
	c1 = (unsigned)xunchar((unsigned)((unsigned)(*pbc) & 0xff));
	c2 = chk1(p+rlnpos,j-rlnpos);
	if (c1 != c2) {
	    if (debug) fprintf(db,"rpacket chk1 (0x%x:0x%x)\n",c1,c2);
	    return('Q');
	}
	break;
      case 2:
	c1 = (unsigned)xunchar(*pbc) << 6 | (unsigned)xunchar(pbc[1]);
	c2 = chksum(p+rlnpos,j-rlnpos);
	if (c1 != c2) {
	    if (debug) fprintf(db,"rpacket chk2 (0x%x:0x%x)\n",c1,c2);
	    return('Q');
	}
	break;
      case 3:
	c1 = ((unsigned)(xunchar((unsigned)((unsigned)*pbc & 0xff)) << 12) |
	      (unsigned)(xunchar((unsigned)((unsigned)pbc[1] & 0xff)) << 6) |
	      (unsigned)(xunchar((unsigned)((unsigned)pbc[2] & 0xff)))) &
		0xffff;
	c2 = chk3(p+rlnpos,j-rlnpos);
	if (c1 != c2) {
	    if (debug) fprintf(db,"rpacket crc (0x%x:0x%x)\n",c1,c2);
	    return('Q');
	}
	break;
      default:
	errpkt("BLOCKCHECK");
    }
    recvtype = type;			/* Remember type. */
    return(type);			/* Good packet, return type. */
}

unsigned int
chksum(p,len) char *p; int len; {	/* Generate arithmetic checksum */
    register unsigned int k, m;
    m = (parity) ? 0177 : 0377;
    for (k = 0; len-- > 0; *p++)
      k += (unsigned)*p & m;
    return(k);
}

unsigned int
chk1(packet,len) char *packet; int len; { /* Generate Type 1 block check */
    unsigned int x;
    x = chksum(packet,len);
    if (debug) fprintf(db,"chksum=%u\n",x);
    return((((x & 192) >> 6) + x) & 63);
}

unsigned int
chk3(s,len) char *s; int len; {		/* Tableless 16-bit CRC generator */
    register unsigned int c;
    register ULONG q, crc = 0L;
    while (len-- > 0) {
	c = (unsigned)(*s++);
	if (parity) c &= 0177;
	q = (crc ^ c) & 017;
	crc = (crc >> 4) ^ (q * 010201);
	q = (crc ^ (c >> 4)) & 017;
	crc = (crc >> 4) ^ (q * 010201);
    }
    if (debug) fprintf(db,"crc=%u\n",(unsigned int)(crc & 0xffff));
    return((unsigned int)(crc & 0xffff));
}

int
getpkt(maxlen) int maxlen; {		/* Fill a packet from file */
    int i, next;
    static int c;
    static char remain[6] = { NUL, NUL, NUL, NUL, NUL, NUL };

    if (first == 1) {			/* If first time thru...  */
	first = 0;			/* don't do this next time, */
	*remain = NUL;			/* discard any old leftovers. */
	if (isp) {			/* Get first byte. */
	    c = *isp++;			/* Of memory string... */
	    if (!c) c = -1;
	} else {			/* or file... */
	    c = zgetc(text);
	}
	if (c < 0) {			/* Watch out for empty file. */
	    first = -1;
	    return(size = 0);
	}
    } else if (first == -1 && !*remain) { /* EOF from last time? */
        return(size = 0);
    }
    for (size = 0; (xdata[size] = remain[size]) != NUL; size++) ;
    *remain = NUL;
    if (first == -1)
      return(size);

    rpt = 0;				/* Initialize repeat counter. */
    while (first > -1) {		/* Until end of file or string... */
	if (isp) {
	    next = *isp++;
	    if (!next) next = -1;
	} else {
	    next = zgetc(text);
	}
	if (next < 0) first = -1;	/* If none, we're at EOF. */
        osize = size;			/* Remember current size. */
        encode(c,next);			/* Encode the character. */
	c = next;			/* Old next char is now current. */

        if (size == maxlen) return(size); /* If just at end, done. */

        if (size > maxlen) {		/* Past end, must save some. */

            for (i = 0; (remain[i] = xdata[osize+i]) != NUL; i++) ;
            size = osize;
            xdata[size] = NUL;
            return(size);		/* Return size. */
        }
    }
    return(size);			/* EOF, return size. */
}

int
encstr(s) char *s; { 			/* Fill a packet from string s. */
    first = 1;				/* Start lookahead. */
    isp = s;				/* Set input string pointer */
    getpkt(spsiz);			/* Fill a packet */
    isp = NULL;				/* Reset input string pointer */
    return(size);			/* Return data field length */
}

VOID
decstr(s) char *s; {			/* Decode packet data into a string */
    osp = s;				/* Set output string pointer  */
    decode(datalen);			/* Decode the string */
    *osp = NUL;				/* Terminate with null */
    osp = NULL;				/* Reset output string pointer */
}

VOID
encode(a,next) int a, next; {		/* Encode character a into packet */
    int a7, b8;

    if (rptflg) {			/* Doing run-length encoding? */
	if (a == next) {		/* Yes, got a run? */
	    if (++rpt < 94) {		/* Yes, count. */
		return;
	    } else if (rpt == 94) {	/* If at maximum */
	    	xdata[size++] = rptq;	/* Emit prefix, */
		xdata[size++] = tochar(rpt);	/* and count, */
		rpt = 0;			/* and reset counter. */
	    }
	} else if (rpt == 1) {		/* Run broken, only two? */
	    rpt = 0;			/* Yes, do the character twice */
	    encode(a,-1);		/* by calling self recursively. */
	    if (size <= maxsiz) osize = size; /* Watch out for boundary. */
	    rpt = 0;			/* Call self second time. */
	    encode(a,-1);
	    return;
	} else if (rpt > 1) {		/* Run broken, more than two? */
	    xdata[size++] = rptq;	/* Yes, emit prefix and count */
	    xdata[size++] = tochar(++rpt);
	    rpt = 0;			/* and reset counter. */
	}
    }
    a7 = a & 127;			/* Get low 7 bits of character */
    b8 = a & 128;			/* And "parity" bit */

    if (ebqflg && b8) {			/* If doing 8th bit prefixing */
	xdata[size++] = ebq;		/* and 8th bit on, insert prefix */
	a = a7;				/* and clear the 8th bit. */
    }
    if (a7 < 32 || a7 == 127) {		/* If in control range */
        xdata[size++] = sctlq;		/* insert control prefix */
        a = ctl(a);			/* and make character printable. */
    } else if (a7 == sctlq)		/* If data is control prefix, */
      xdata[size++] = sctlq;		/* prefix it. */
    else if (ebqflg && a7 == ebq)	/* If doing 8th-bit prefixing, */
      xdata[size++] = sctlq;		/* ditto for 8th-bit prefix. */
    else if (rptflg && a7 == rptq)	/* If doing run-length encoding, */
      xdata[size++] = sctlq;		/* ditto for repeat prefix. */

    xdata[size++] = a;			/* Finally, emit the character. */
    xdata[size] = NUL;			/* Terminate string with null. */
}

int
decode(len) int len; {			/* Decode packet data field */
    unsigned int a, a7, b8;		/* Local variables */

    rpt = 0;				/* Initialize repeat count. */
    while (len-- > 0) {
	a = *rdatap++;			/* For each character, a, do... */
	if (rptflg) {			/* Repeat processing? */
	    if (a == rptq) {		/* Yes, have repeat prefix? */
		if (len-- < 1) {
		    if (debug) fprintf(db,"decode rpt error A\n");
		    return(-1);
		}
		rpt = xunchar(*rdatap++); /* Yes, get count */
		if (len-- < 1) {
		    if (debug) fprintf(db,"decode rpt error B\n");
		    return(-1);
		}
		a = *rdatap++;		/* and the following character. */
	    }
	}
	b8 = 0;				/* Assume 8th bit not on. */
	if (ebqflg) {			/* Doing 8th-bit prefixing? */
	    if (a == ebq) {		/* Yes, have 8th-bit prefix? */
		b8 = 128;		/* Yes, remember bit 8 on */
		if (len-- < 1) {
		    if (debug) fprintf(db,"decode ebq error\n");
		    return(-1);
		}
		a = *rdatap++;		/* and get following character. */
	    }
	}
        if (a == rctlq) {		/* Control quote? */
	    if (len-- < 1) {
		if (debug) fprintf(db,"decode ctl error\n");
		return(-1);
	    }
            a = *rdatap++;		/* Yes, get next character */
            a7 = a & 127;		/* and its low 7 bits */
            if (a7 > 62 && a7 < 96)	/* Encoded control character? */
                a = ctl(a);		/* Yes, controllify. */
        }
        if (rpt == 0)			/* Rationalize the repeat count. */
	  rpt = 1;
	a |= b8;			/* OR in the 8th bit. */
	a &= 0xff;			/* Undo any sign extension. */

	/* The following is UNIX-specific but so is this program */

	if (text && a == CR)		/* Don't output carriage returns. */
	  continue;
	for (; rpt > 0; rpt--) {	/* Output the character. */
	    if (osp) {			/* As many copies as indicated by */
		*osp++ = a;		/* the repeat count. */
	    } else {
		putc((char)a,ofp);	/* Use putc macro here to avoid */
		if (ferror(ofp)) {	/* function-call overhead. */
		    if (debug)
		      fprintf(db,"decode putc a=%u errno=%u\n",a,errno);
		    failure = 1;
		    return(-1);
		}
	    }
	}
    }
    return(0);				/* Return successfully when done. */
}

VOID
spar(s) char *s; {			/* Set protocol parameters. */
    int k, x;

    s--;				/* Line up with field numbers. */

    x = (rln >= 1) ? xunchar(s[1]) : 80;/* Limit on size of inbound packets */
    spsiz = (x < 10) ? 80 : x;

    x = (rln >= 2) ? xunchar(s[2]) : 5;	/* Timeout on inbound packets */
    if (timint > 0)			/* "-b 0" overrides */
      timint = (x < 0) ? 5 : x;

    spadn = 0; spadc = NUL;		/* Outbound Padding */
    if (rln >= 3) {
	spadn = xunchar(s[3]);
	if (rln >= 4) spadc = ctl(s[4]); else spadc = 0;
    }
    seol = (rln >= 5) ? xunchar(s[5]) : '\r'; /* Outbound Packet Terminator */
    if ((seol < 2) || (seol > 037)) seol = '\r';

    x = (rln >= 6) ? s[6] : '#';	/* Control prefix */
    rctlq = ((x > 32 && x < 63) || (x > 95 && x < 127)) ? x : '#';

    rq = (rln >= 7) ? s[7] : 0; 	/* 8th-bit prefix */
    if (rq == 'Y') rqf = 1;
    else if ((rq > 32 && rq < 63) || (rq > 95 && rq < 127)) rqf = 2;
    else rqf = 0;
    switch (rqf) {
	case 0: ebqflg = 0; break;
	case 1: if (parity) { ebqflg = 1; ebq = '&'; } break;
	case 2: if ((ebqflg = (ebq == sq || sq == 'Y'))) ebq = rq;
    }
    x = 1; 				/* Block check */
    if (rln >= 8) {
	x = s[8] - '0';
	if ((x < 1) || (x > 3)) x = 1;
    }
    if (bctr > 1 && bctr != x) x = 1;
    bctr = x;
    if (rln >= 9) { 			/* Repeat prefix */
	rptq = s[9];
	rptflg = ((rptq > 32 && rptq < 63) || (rptq > 95 && rptq < 127));
    } else
      rptflg = 0;
    k = rln;
    if (rln >= 10) {			/* Capas */
	x = xunchar(s[10]);
	if (x & 8) attributes = 1;
	if (x & 2) longpackets = 1;
	for (k = 10; (rln >= k) && (xunchar(s[k]) & 1); k++)
	  ;
	if (debug) fprintf(db,"spar attributes=%d longpackets=%d\n",
			   attributes, longpackets);
    }
    if (rln > k+1) {			/* Long packets */
	if (longpackets) {
	    x = xunchar(s[k+2]) * 95 + xunchar(s[k+3]);
	    spsiz = (x > MAXSP) ? MAXSP : x;
	    if (spsiz < 10) spsiz = 80;
	}
    }
#ifndef NOSTREAMING
    if (rln > k+7) {
	x = xunchar(s[k+8]);		/* Other Kermit's WHATAMI info */
	if (debug) fprintf(db,"spar whatami = 0x%x\n",x);
	if (streamok > -1)
	  if (x & WMI_FLAG)
	    if (x & WMI_STREAM)
	      streamok = 1;
	if (debug) fprintf(db,"spar streamok = %d\n",streamok);
    }
#endif /* NOSTREAMING */
    if (rln > k+8) {			/* Other Kermit's system ID */
	char buf[16];
	int z;
	x = xunchar(s[k+9]);
	z = k;
	k += (9 + x);
	if (x > 0 && x < 16 && rln >= k) {
	    strncpy(buf,(char *)s+z+10,x);
	    buf[x] = NUL;
	    if (debug) fprintf(db,"spar sysid [%s]\n",buf);
	    if (buf[0] && !manual) {	/* If transfer mode is automatic... */
		if (!strcmp(buf,"U1")) { /* If UNIX, same as me... */
		    text = 0;		/* Switch to binary mode */
  		    literal = 1;	/* and literal filenames */
		    if (debug) fprintf(db,"spar recognizes peer\n");
		}
	    }
	}
    }
}

char *					/* Send our protocol parameters */
rpar() {
    int x;
    if (rpsiz > 94)
      xdata[1] = tochar(94);		/* Biggest packet I can receive */
    else
      xdata[1] = tochar(rpsiz);
    xdata[2] = tochar(rtimo);		/* When I want to be timed out */
    xdata[3] = tochar(rpadn);		/* How much padding I need (none) */
    xdata[4] = ctl(rpadc);		/* Padding character I want */
    xdata[5] = tochar(reol);		/* End-Of-Line character I want */
    xdata[6] = '#';			/* Control-Quote character I send */

    switch (rqf) {			/* 8th-bit prefix */
      case -1:
      case  1: if (parity) ebq = sq = '&'; break;
      case  0:
      case  2: break;
    }
    xdata[7] = sq;

    xdata[8] = bctr + '0';		/* Block check type */
    if (rptflg)				/* Run length encoding */
      xdata[9] = rptq;			/* If receiving, agree. */
    else
      xdata[9] = '~';
    xdata[10] = tochar(2+8);		/* Capabilities (LP+Attributes) */
    xdata[11] = tochar(1);		/* Window size */
    if (urpsiz > 94) {
	xdata[12] = (char) tochar(urpsiz / 95); /* Long packet size */
	xdata[13] = (char) tochar(urpsiz % 95);
    }
    xdata[14] = '0';			/* No checkpointing */
    xdata[15] = '+';
    xdata[16] = '+';
    xdata[17] = '+';
    x = WMI_FLAG;			/* WHATAMI info */
    if (literal) x |= WMI_FNAME;	/* Filenames literal */
    if (!text) x |= WMI_FMODE;		/* Text or binary mode */
#ifndef NOSTREAMING
    if (streamok > -1)			/* If streaming not disabled, */
      x |= WMI_STREAM;			/* offer to stream. */
    if (debug)
      fprintf(db,"rpar streamok=%d whatami=%d\n",streamok,x);
#endif /* NOSTREAMING */
    xdata[18] = tochar(x);
    xdata[19] = tochar(2);		/* System ID lengh is 2 */
    xdata[20] = 'U';			/* UNIX system ID is "U1" */
    xdata[21] = '1';
    x = WMI2_FLAG;			/* WHATAMI2 info */
    if (manual) x |= WMI2_XMODE;	/* Transfer-mode is manual */
    xdata[22] = tochar(x);
    return(xdata+1);			/* Return pointer length field */
}

VOID
rinit() {				/* RECEIVE initialization */
    if (quiet) return;
    tmsgl(versio);
    tmsgl("Escape back to your local Kermit and give a SEND command.");
    tmsgl("");
    tmsgl("KERMIT READY TO RECEIVE...");
}

VOID
ginit() {				/* GET initialization */
    if (quiet) return;
    tmsgl(versio);
    tmsgl("Escape back to your local Kermit and give a SERVER command.");
    tmsgl("");
    tmsgl("KERMIT READY TO GET...");
}

int
gnfile() {				/* Get name of next file to send */
    if (cz)				/* Batch was canceled */
      return(0);
    while (nfils-- > 0) {		/* Otherwise continue thru list */
	strncpy(filnam,*cmlist++,MAXPATHLEN);
	filnam[MAXPATHLEN-1] = NUL;
	if (zchki(filnam) > -1)		/* Skip files that can't be read */
	  return(1);
    }
    return(0);				/* None left */
}

int
rcvfil() {				/* Receive a file */
    char myname[MAXPATHLEN+1];		/* Local name buffer */
    cx = 0;				/* Reset per-file cancellation flag */
    if (cmarg2) {			/* User gave as-name? */
	if (debug) fprintf(db,"rcvfil as-name [%s]\n",cmarg2);
	strncpy(myname,cmarg2,MAXPATHLEN); /* Use it */
	myname[MAXPATHLEN] = NUL;
	if (backup) zbackup(myname);	/* Handle collisions */
	cmarg2 = NULL;			/* Undo as-name */
    } else {
	decstr(filnam);			/* Decode name string */
	if (zrtol(filnam,myname,backup,MAXPATHLEN) < 0)
	  errpkt("Backup file creation failure");
	strncpy(filnam,myname,MAXPATHLEN);
	filnam[MAXPATHLEN] = NUL;
	if (debug) fprintf(db,"rcvfil filename [%s]\n",filnam);
    }
    if (zchko(myname) < 0)		/* Check writability */
      errpkt("Write access denied");
    if (zopeno(myname) < 0)		/* Open the output file */
      errpkt("Error opening output file");
    return(0);
}

int					/* Send a command packet */
#ifdef __STDC__
scmd(char t, char *s)
#else
scmd(t,s) char t, *s;
#endif /* __STDC__ */
{
    encstr(s);				/* Encode the command string */
    return(spacket(t,seq,size,xdata));	/* Send the packet */
}
