/*	$NetBSD: debug.h,v 1.5.16.1 2005/04/29 11:29:36 kent Exp $	*/

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

#endif /* DEBUG */

