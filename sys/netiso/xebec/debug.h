/*	$NetBSD: debug.h,v 1.6.4.1 2007/02/26 09:12:02 yamt Exp $	*/

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

