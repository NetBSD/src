//	$Id: fcntl.h,v 1.2 1993/08/02 17:21:36 mycroft Exp $

#ifndef __libgxx_sys_fcntl_h

extern "C"
{
#ifdef __sys_fcntl_h_recursive
#include_next <sys/fcntl.h>
#else
#define __sys_fcntl_h_recursive
#include_next <sys/fcntl.h>
#define __libgxx_sys_fcntl_h 1
#endif
}


#endif
