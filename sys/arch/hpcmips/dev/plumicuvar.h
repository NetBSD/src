/*	$NetBSD: plumicuvar.h,v 1.1 1999/11/21 06:50:26 uch Exp $ */

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/* 
 * 1	EXT -> IO5(INT0, INT1, INT2), IO3(INT0, INT1)
 * 2	SM
 * 3	USBWAKE
 * 4	USB
 * 5	DISP
 * 6	C2SC
 * 7	C1SC
 * 8	PCC -> C1IO, C1RI, C2IO, C2RI
 */

/* Logical interrupt line # */
#define PLUM_INT_C1IO		0
#define PLUM_INT_C1RI		1
#define PLUM_INT_C1SC		2
#define PLUM_INT_C2IO		3
#define PLUM_INT_C2RI		4
#define PLUM_INT_C2SC		5
#define PLUM_INT_DISP		6
#define PLUM_INT_USB		7
#define PLUM_INT_USBWAKE	8
#define PLUM_INT_SM		9
#define PLUM_INT_EXT5IO0	10
#define PLUM_INT_EXT5IO1	11
#define PLUM_INT_EXT5IO2	12
#define PLUM_INT_EXT5IO3	13
#define PLUM_INT_EXT3IO0	14
#define PLUM_INT_EXT3IO1	15

#define PLUM_INTR_MAX		16
#define LEGAL_PRUM_INTR(x) (((x) >= 0) && ((x) < PLUM_INTR_MAX))

void*	plum_intr_establish __P((plum_chipset_tag_t, int, int, int, int (*) __P((void*)),	void*));
void	plum_intr_disestablish __P((plum_chipset_tag_t, void*));
