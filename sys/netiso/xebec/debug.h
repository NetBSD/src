/*	$NetBSD: debug.h,v 1.7 2005/12/11 12:25:15 christos Exp $	*/

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

