/*	$NetBSD: mcontext.h,v 1.2 2003/10/08 22:43:01 thorpej Exp $	*/

#ifndef _HPPA_MCONTEXT_H_
#define _HPPA_MCONTEXT_H_

typedef struct mcontext {
	int placeholder;
} mcontext_t;

#define _UC_MACHINE_SP(uc) 0
#define _UC_MACHINE_PC(uc) 0
#define _UC_MACHINE_INTRV(uc) 0

#define	_UC_MACHINE_SET_PC(uc, pc)	/* XXX */

#endif /* _HPPA_MCONTEXT_H_ */
