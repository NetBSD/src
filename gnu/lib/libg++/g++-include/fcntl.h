//	$Id: fcntl.h,v 1.2 1993/08/02 17:22:07 mycroft Exp $

#ifndef fcntl_h

extern "C" {

#ifdef __fcntl_h_recursive
#include_next <fcntl.h>
#else
#define fcntl __hide_fcntl
#define open  __hide_open
#define creat __hide_creat

#define __fcntl_h_recursive
#include <_G_config.h>
#include_next <fcntl.h>

#undef fcntl
#undef open
#undef creat

#define fcntl_h 1

int       fcntl(int, int, ...);
int	  creat _G_ARGS((const char*, unsigned short int));

int       open _G_ARGS((const char*, int, ...));

#endif
}
#endif
