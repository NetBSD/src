/*	$NetBSD: frame.h,v 1.5 1994/10/26 09:10:27 cgd Exp $	*/

/* use the common m68k definition */
#include <m68k/frame.h>

/* hack for tracing call stack.  used by tracedump() */
struct funcall_frame {
	struct funcall_frame *fr_savfp;
	int fr_savpc;
	int fr_arg[1];
};
