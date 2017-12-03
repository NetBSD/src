/*	$NetBSD: openfirm.h,v 1.7.22.1 2017/12/03 11:36:43 jdolecek Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Prototypes for additional OpenFirmware Interface Routines
 */

#ifndef _SPARC_OPENFIRM_H_
#define _SPARC_OPENFIRM_H_

#include <dev/ofw/openfirm.h>

#ifdef __sparc_v9__
/* All cells are 8 byte slots */
typedef uint64_t cell_t;
#ifdef __arch64__
#define HDL2CELL(x)	(cell_t)(u_int)(int)(x)
#define ADR2CELL(x)	(cell_t)(x)
#else
#define HDL2CELL(x)	(cell_t)(u_int)(int)(x)
#define ADR2CELL(x)	(cell_t)(u_int)(int)(x)
#endif
#define HDQ2CELL_HI(x)	(cell_t)(0)
#define HDQ2CELL_LO(x)	(cell_t)(x)
#define CELL2HDQ(hi,lo)	(lo)
#else /* __sparc_v9__ */
/* All cells are 4 byte slots */
typedef uint32_t cell_t;
#define HDL2CELL(x)	(cell_t)(x)
#define ADR2CELL(x)	(cell_t)(x)
#define HDQ2CELL_HI(x)	(cell_t)((x) >> 32)
#define HDQ2CELL_LO(x)	(cell_t)(x)
#endif /* __sparc_v9__ */

void	OF_init(void);
int	OF_test(const char *);
int	OF_test_method(int, const char *);
void	OF_set_symbol_lookup(void (*)(void *), void (*)(void *));
int	OF_searchprop (int, const char *, void *, int);
int	OF_mapintr(int, int *, int, int);

void	OF_poweroff(void) __attribute__((__noreturn__));
int	OF_interpret(const char *, int, int, ...);
void	OF_sym2val(void *);
void	OF_val2sym(void *);

#endif /* _SPARC_OPENFIRM_H_ */
