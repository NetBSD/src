//	$Id: select.h,v 1.2 1993/08/02 17:21:43 mycroft Exp $

/* Needed by Interviews for AIX. */
#ifndef __libgxx_sys_select_h
extern "C"
{
#include_next <sys/select.h>
#define __libgxx_sys_select_h 1
}
#endif
