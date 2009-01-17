/*	$NetBSD: gemini_com.h,v 1.1.8.2 2009/01/17 13:27:52 mjf Exp $	*/

#ifndef _ARM_GEMINI_COM_H_
#define _ARM_GEMINI_COM_H_

#if defined(SL3516)
# define GEMINI_COM_FREQ (3000000L * 16)
#else
# error unknown gemini chip
#endif

#endif	/* _ARM_GEMINI_COM_H_ */
