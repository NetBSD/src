/* $NetBSD: int_const.h,v 1.12 2018/05/16 12:40:43 reinoud Exp $ */

/*
 * Automatically generated by ./genheaders.sh on Wed May 16 14:39:02 CEST 2018
 * Do not modify directly!
 */
#ifndef _USERMODE_INT_CONST_H
#define _USERMODE_INT_CONST_H

#if defined(__i386__)
#include "../../i386/include/int_const.h"
#elif defined(__x86_64__)
#include "../../amd64/include/int_const.h"
#elif defined(__arm__)
#include "../../arm/include/int_const.h"
#else
#error port me
#endif

#endif
