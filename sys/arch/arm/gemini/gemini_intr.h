/*	$NetBSD: gemini_intr.h,v 1.4 2022/06/25 13:24:34 jmcneill Exp $	*/

#ifndef _ARM_GEMINI_INTR_H_
#define _ARM_GEMINI_INTR_H_

#define	ARM_IRQ_HANDLER	_C_LABEL(gemini_irq_handler)

#ifndef _LOCORE
void	gemini_irq_handler(void *);

#include <arm/pic/picvar.h>
#endif	/* _LOCORE */

#endif	/* _ARM_GEMINI_INTR_H_ */
