//	$Id: stddef.h,v 1.2 1993/08/02 17:22:16 mycroft Exp $

#ifndef __libgxx_stddef_h

extern "C" {
#ifdef __stddef_h_recursive
#include_next <stddef.h>
#else
#include_next <stddef.h>

#define __libgxx_stddef_h 1
#endif
}
#endif
