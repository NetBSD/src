/*	$NetBSD: debug.h,v 1.8 2007/01/18 12:43:38 cbiere Exp $	*/

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

