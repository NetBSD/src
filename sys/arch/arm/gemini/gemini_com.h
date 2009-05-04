/*	$NetBSD: gemini_com.h,v 1.1.14.2 2009/05/04 08:10:40 yamt Exp $	*/

#ifndef _ARM_GEMINI_COM_H_
#define _ARM_GEMINI_COM_H_

#if defined(SL3516)
# define GEMINI_COM_FREQ (3000000L * 16)
#else
# error unknown gemini chip
#endif

#endif	/* _ARM_GEMINI_COM_H_ */
