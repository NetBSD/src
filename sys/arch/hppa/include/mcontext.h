/*	$NetBSD: mcontext.h,v 1.3.2.2 2004/08/03 10:35:37 skrll Exp $	*/

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
