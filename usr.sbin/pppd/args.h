/*
 * neat macro from ka9q to "do the right thing" with ansi prototypes
 * $Id: args.h,v 1.2 1993/11/10 01:33:54 paulus Exp $
 */

#ifndef __ARGS
#ifdef __STDC__
#define __ARGS(x)       x
#else
#define __ARGS(x)       ()
#endif
#endif
