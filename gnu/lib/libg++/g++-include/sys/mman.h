//	$Id: mman.h,v 1.2 1993/08/02 17:21:38 mycroft Exp $

#ifndef __libgxx_sys_mman_h

extern "C" {
#ifdef __sys_mman_h_recursive
#include_next <sys/mman.h>
#else
#define __sys_mman_h_recursive
#include_next <sys/mman.h>

#define __libgxx_sys_mman_h 1
#endif
}

#endif
