/*	$NetBSD: debug.h,v 1.7.20.1 2007/02/01 08:48:45 ad Exp $	*/

#define OUT stdout

extern int	debug[128];

#ifdef DEBUG
extern int column;

#define IFDEBUG(letter) \
	if(debug['letter']) {
#define ENDDEBUG  ; (void) fflush(stdout);}

#else

#define STAR *
#define IFDEBUG(letter)	 {
#define ENDDEBUG	 ; }

#endif /* DEBUG */

