/*	$NetBSD: INTERN.h,v 1.3 1996/09/19 06:27:05 thorpej Exp $	*/

#ifdef EXT
#undef EXT
#endif
#define EXT

#ifdef INIT
#undef INIT
#endif
#define INIT(x) = x

#define DOINIT
