/* $Header: /cvsroot/src/usr.bin/patch/Attic/INTERN.h,v 1.1 1993/04/09 11:33:52 cgd Exp $
 *
 * $Log: INTERN.h,v $
 * Revision 1.1  1993/04/09 11:33:52  cgd
 * patch 2.0.12u8, from prep.ai.mit.edu.  this is not under the GPL.
 *
 * Revision 2.0  86/09/17  15:35:58  lwall
 * Baseline for netwide release.
 * 
 */

#ifdef EXT
#undef EXT
#endif
#define EXT

#ifdef INIT
#undef INIT
#endif
#define INIT(x) = x

#define DOINIT
