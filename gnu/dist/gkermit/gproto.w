/* G P R O T O  --  Protocol module for gkermit  */             /* -*-C-*- */

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
#include <stdio.h>
#include "gkermit.h"

_MYPROTOTYPE( int closof, (void) );	/* Close output file */
_MYPROTOTYPE( VOID errpkt, (char *) );	/* Send Error packet */

extern char * xdata, *rdatap, **cmlist, *cmarg, *rpar(), strbuf[], filnam[];
extern int start, bctu, bctr, delay, cx, cz, failure, attributes, datalen;
extern int streamok, streaming, timint;
extern FILE * db;

static int x;

static VOID
streamon() {				/* Start streaming if negotiated */
    x = 0;
    if (streamok > 0) {
	streaming = 1;
	timint = 0;
    }
}

/* Declare gwart states (like lex) */

%states ssini ssfil ssdat ssatt sseot sseof sipkt
%states srini srfil srdat sratt

/* Packets are read by the input() function, which returns the packet type */
/* that serves as the input to the state machine, which follows... */

%%
/* Sending states... */

s {					/* Start state for Send. */
    tinit();				/* Initialize transaction. */
    if (sinit('S') < 0) { errpkt("sinit"); } /* Build and send the S packet. */
    else BEGIN ssini;
}
<ssini>Y {				/* Receive ACK to I packet */
    spar(rdatap);			/* Set parameters from it */
    bctu = bctr;			/* Switch to negotiated block check */
    if (gnfile() > 0) {			/* Is there a file to send? */
        if (sfile() < 0) {		/* Yes, open it, send F packet, */
	    errpkt("sfile");
	} else {			/* No error */
	    streamon();
	    BEGIN ssfil;
	}
    } else {				/* No files to send, */
        if (seot() < 0) { errpkt("seot"); } /* so send EOT packet. */
        else BEGIN sseot;
    }
}
<ssfil>Y {				/* Receive ACK to File header packet */
    if (attributes) {			/* If attributes negotiated */
	if (sattr() < 0) {		/* Send file attributes */
	    errpkt("sattr");
	} else
	  BEGIN ssatt;
    } else if ((x = sdata()) == 0) {	/* Otherwise send first Data packet */
	if (seof("") < 0) {		/* Empty file - send EOF */
	    errpkt("seof");
	} else {
	    BEGIN sseof;
	}
    } else if (x < 0) {			/* Error */
	errpkt("sdata");
    } else {				/* OK - switch to Data state */
	BEGIN ssdat;
    }
}
<ssatt>Y {				/* Receive ACK to Attribute packet */
    if (*rdatap == 'N') {		/* Check for refusal */
	seof("D");
	BEGIN sseof;
    } else if ((x = sdata()) == 0) {	/* Otherwise send first Data packet */
	if (seof("") < 0) {		/* Empty file - send EOF */
	    errpkt("seof");
	} else {
	    BEGIN sseof;
	}
    } else if (x < 0) {			/* Error */
	errpkt("sdata");
    } else {				/* OK - switch to Data state */
	BEGIN ssdat;
    }
}
<ssdat>Y {				/* Receive ACK to Data packet */
    if (*rdatap == 'X')			/* Check for file cancellation */
      cx = 1;
    else if (*rdatap == 'Z')		/* Check for batch cancellation */
      cz = 1;
    if ((x = sdata()) == 0) {		/* Send data packet if data left. */
	if (seof((cx | cz) ? "D" : "") < 0) { /* If not, send Z packet */
	    errpkt("seof");
	} else {
	    BEGIN sseof;
	}
    } else if (x < 0)			/* Fatal error sending data */
      errpkt("sdata");
}
<sseof>Y {				/* Receive ACK to EOF */
    if (gnfile() > 0) {			/* Get next file from list */
	if (sfile() > 0)
	  BEGIN ssfil;
	else { errpkt("sfile"); }
    } else {				/* No more files */
	seot();				/* Send EOT */
	BEGIN sseot;
    }
}
<sseot>Y { return(failure); }		/* Send ACK to EOT - done */

/* Receiving states... */

v { tinit(); rinit(); BEGIN srini; }	/* Start-state for Receive */

<srini>S {				/* Receive S packet */
    spar(rdatap);			/* Set parameters from it */
    ack1(rpar());			/* ACK with our parameters */
    bctu = bctr;			/* Switch to negotiated block check */
    streamon();
    BEGIN srfil;			/* Wait for file or EOT */
}

<srfil>B { ack(); return(failure); }	/* Receive EOT packet */

<srfil>F {				/* Receive File header packet */
    if (rcvfil() < 0) {
	errpkt("rcvfil");
    } else {
	encstr(filnam);
	ack1(xdata);
	streamon();
	BEGIN sratt;
    }
}
<sratt>A {				/* Receive Attribute packet */
    if (gattr(rdatap) == 0) {
	ack();
    } else {
	ack1("\"");
    }
}
<sratt>D {				/* Receive first Data packet */
    if (decode(datalen) < 0) {
	errpkt("Packet decoding error");
    } else {
	ack();
	BEGIN srdat;
    }
}
<sratt>Z {				/* Empty file */
    if (*rdatap == 'D')			/* Check for Discard directive */
      cx = 1;
    if (closof() < 0) {			/* Close the output file */
	errpkt("closof");
    } else {
	ack();				/* Send ACK */
	BEGIN srfil;			/* Wait for another file or EOT */
    }
}
<srdat>D {				/* Receive Data packet */
    if (decode(datalen) < 0)
      errpkt("Packet decoding error");
    else
      ack();
}
<srdat>Z {				/* Receive EOF packet */
    if (*rdatap == 'D')			/* Check for Discard directive */
      cx = 1;
    if (closof() < 0) {			/* Close the output file */
	errpkt("closof");
    } else {
	ack();				/* Send ACK */
	BEGIN srfil;			/* Wait for another file or EOT */
    }
}

/* GET files from a Kermit server... */

r {					/* Start state for Get */
    tinit();				/* Initialize transaction */
    ginit();				/* Initialize Get */
    sinit('I');				/* Send I packet */
    BEGIN sipkt;
}
<sipkt>Y {				/* Receive ACK for I packet */
    spar(rdatap);			/* Set parameters from it */
    if (scmd('R',cmarg) < 0)		/* Send GET packet file filespec */
      errpkt("scmd");
    else
      BEGIN srini;			/* Wait for S packet */
}

E { return(failure = 1); }		/* Receive an Error packet */

. { errpkt("Unknown packet type"); }	/* Handle unwanted packet types. */
%%
