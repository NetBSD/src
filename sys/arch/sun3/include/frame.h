/*
 *	$Id: frame.h,v 1.3 1994/01/08 19:08:49 cgd Exp $
 */

/* use the common m68k definition */
#include <m68k/frame.h>

/* XXX -- stack frame for making boot rom calls. should be elsewere */
struct funcall_frame {
	struct funcall_frame *fr_savfp;
	int fr_savpc;
	int fr_arg[1];
};
