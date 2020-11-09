/* $Header: /cvsroot/src/games/warp/EXTERN.h,v 1.1 2020/11/09 23:37:05 kamil Exp $
 *
 * $Log: EXTERN.h,v $
 * Revision 1.1  2020/11/09 23:37:05  kamil
 * Add Warp Kit, Version 7.0 by Larry Wall
 *
 * Warp is a real-time space war game that doesn't get boring very quickly.
 * Read warp.doc and the manual page for more information.
 *
 * games/warp originally distributed with 4.3BSD-Reno, is back to the BSD
 * world via NetBSD. Its remnants were still mentioned in games/Makefile.
 *
 * Larry Wall, the original author and the copyright holder, generously
 * donated the game and copyright to The NetBSD Foundation, Inc.
 *
 * Import the game sources as-is from 4.3BSD-Reno, with the cession
 * of the copyright and license to BSD-2-clause NetBSD-style.
 *
 * Signed-off-by: Larry Wall <larry@wall.org>
 * Signed-off-by: Kamil Rytarowski <kamil@netbsd.org>
 *
 * Revision 7.0.1.1  86/12/12  16:46:50  lwall
 * Guarded the undefs.
 * 
 * Revision 7.0  86/10/08  15:11:31  lwall
 * Split into separate files.  Added amoebas and pirates.
 * 
 */

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
