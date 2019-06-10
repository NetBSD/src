/* $NetBSD: trap.h,v 1.1.6.2 2019/06/10 22:06:50 christos Exp $ */

/*
 * Handcrafted redirect to prevent problems with i386 and x86_64 sharing x86
 */
#ifndef _USERMODE_TRAP_H
#define _USERMODE_TRAP_H

#if defined(__i386__)
#include "../../x86/include/trap.h"
#elif defined(__x86_64__)
#include "../../x86/include/trap.h"
#elif defined(__arm__)
#include "../../arm/include/trap.h"
#else
#error port me
#endif

#endif
