/*	$NetBSD: tzone.h,v 1.3 1998/03/14 04:39:55 lukem Exp $	*/

/* tzone.h */

extern int32 secondswest;

#ifdef	__STDC__
#define P(args) args
#else
#define P(args) ()
#endif

extern void tzone_init P((void));

#undef P
