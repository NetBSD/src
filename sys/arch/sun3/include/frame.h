/*
 *	$Id: frame.h,v 1.4 1994/02/04 08:19:57 glass Exp $
 */

/* use the common m68k definition */
#include <m68k/frame.h>

/* hack for tracing call stack.  used by tracedump() */
struct funcall_frame {
	struct funcall_frame *fr_savfp;
	int fr_savpc;
	int fr_arg[1];
};
