/* $Header: /cvsroot/src/sys/netiso/xebec/Attic/debug.h,v 1.1 1993/04/09 12:02:05 cgd Exp $ */
/* $Source: /cvsroot/src/sys/netiso/xebec/Attic/debug.h,v $ */

#define OUT stdout

extern int	debug[128];

#ifdef DEBUG
extern int column;

#define IFDEBUG(letter) \
	if(debug['letter']) { 
#define ENDDEBUG  ; (void) fflush(stdout);}

#else 

#define STAR *
#define IFDEBUG(letter)	 //*beginning of comment*/STAR
#define ENDDEBUG	 STAR/*end of comment*//

#endif DEBUG

