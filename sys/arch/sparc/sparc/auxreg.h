/*	$NetBSD: auxreg.h,v 1.13.12.1 2012/10/30 17:20:22 yamt Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)auxreg.h	8.1 (Berkeley) 6/11/93
 */

/*
 * Sun-4c Auxiliary I/O register.  This register talks to the floppy
 * (if it exists) and the front-panel LED.
 */

#define	AUXIO4C_MB1	0xf0		/* must be set on write */
#define	AUXIO4C_FHD	0x20		/* floppy: high density (unreliable?)*/
#define	AUXIO4C_FDC	0x10		/* floppy: diskette was changed */
#define	AUXIO4C_FDS	0x08		/* floppy: drive select */
#define	AUXIO4C_FTC	0x04		/* floppy: drives Terminal Count pin */
#define	AUXIO4C_FEJ	0x02		/* floppy: eject disk */
#define	AUXIO4C_LED	0x01		/* front panel LED */

#define	AUXIO4M_MB1	0xc0		/* must be set on write? */
#define	AUXIO4M_FHD	0x20		/* floppy: high density (unreliable?)*/
#define	AUXIO4M_LTE	0x08		/* link-test enable */
					/* power up modem in SPARCbook 3GX */

#define	AUXIO4M_MMX	0x04		/* Monitor/Mouse MUX; what is it? */
					/* power up DBRI in SPARCbook 3GX */

#define	AUXIO4M_FTC	0x02		/* floppy: drives Terminal Count pin */
#define	AUXIO4M_LED	0x01		/* front panel LED */

/*
 * We use a fixed virtual address for the register because we use it for
 * timing short sections of code (via external hardware attached to the LED).
 */
#define	AUXIO4C_REG	((volatile u_char *)(AUXREG_VA + 3))
#define	AUXIO4M_REG	((volatile u_char *)(AUXREG_VA))

#define LED_ON		do {						\
	if (CPU_ISSUN4M) {						\
		auxio_regval |= AUXIO4M_LED;				\
		*AUXIO4M_REG = auxio_regval;				\
	} else {							\
		auxio_regval |= AUXIO4C_LED;				\
		*AUXIO4C_REG = auxio_regval;				\
	}								\
} while(0)

#define LED_OFF		do {						\
	if (CPU_ISSUN4M) {						\
		auxio_regval &= ~AUXIO4M_LED;				\
		*AUXIO4M_REG = auxio_regval;				\
	} else {							\
		auxio_regval &= ~AUXIO4C_LED;				\
		*AUXIO4C_REG = auxio_regval;				\
	}								\
} while(0)

#define LED_FLIP	do {						\
	if (CPU_ISSUN4M) {						\
		auxio_regval ^= AUXIO4M_LED;				\
		*AUXIO4M_REG = auxio_regval;				\
	} else {							\
		auxio_regval ^= AUXIO4C_LED;				\
		*AUXIO4C_REG = auxio_regval;				\
	}								\
} while(0)

#define FTC_FLIP	do {						\
	if (CPU_ISSUN4M) {						\
		/* AUXIO4M_FTC bit is auto-clear */			\
		*AUXIO4M_REG = auxio_regval | AUXIO4M_FTC;		\
		/* XXX we need to clear it on hyperSPARC SS20 */	\
		DELAY(10);						\
		*AUXIO4M_REG = auxio_regval;				\
	} else {							\
		auxio_regval |= AUXIO4C_FTC;				\
		*AUXIO4C_REG = auxio_regval;				\
		DELAY(10);						\
		auxio_regval &= ~AUXIO4C_FTC;				\
		*AUXIO4C_REG = auxio_regval;				\
	}								\
} while(0)

#define	AUXIO_BITS	(						\
	CPU_ISSUN4M							\
		? "\20\6FHD\4LTE\3MMX\2FTC\1LED"			\
		: "\20\6FHD\5FDC\4FDS\3FTC\2FEJ\1LED"			\
)

#ifndef _LOCORE
extern volatile u_char *auxio_reg;	/* Copy of AUXIO_REG */
extern u_char auxio_regval;
unsigned int auxregbisc(int, int);
#endif
