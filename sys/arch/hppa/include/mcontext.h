/*	$NetBSD: mcontext.h,v 1.1 2003/08/31 01:28:58 chs Exp $	*/

#ifndef _HPPA_MCONTEXT_H_
#define _HPPA_MCONTEXT_H_

typedef struct mcontext {
	int placeholder;
} mcontext_t;

#define _UC_MACHINE_SP(uc) 0

#endif /* _HPPA_MCONTEXT_H_ */
