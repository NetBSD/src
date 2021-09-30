/*	$NetBSD: gemini_intr.h,v 1.3 2021/09/30 07:49:09 skrll Exp $	*/

#ifndef _ARM_GEMINI_INTR_H_
#define _ARM_GEMINI_INTR_H_

#define	ARM_IRQ_HANDLER	_C_LABEL(gemini_irq_handler)

#ifndef _LOCORE
void	gemini_irq_handler(void *);

#define	_splraise	pic_splraise
#define	_spllower	pic_spllower
#define	splx		pic_splx

#include <arm/pic/picvar.h>
#endif	/* _LOCORE */

#endif	/* _ARM_GEMINI_INTR_H_ */
