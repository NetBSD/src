/*	$NetBSD: EXTERN.h,v 1.3 1996/09/19 06:27:04 thorpej Exp $	*/

#ifdef EXT
#undef EXT
#endif
#define EXT extern

#ifdef INIT
#undef INIT
#endif
#define INIT(x)

#ifdef DOINIT
#undef DOINIT
#endif
