/*	$NetBSD: vrledreg.h,v 1.3 2001/06/11 10:04:27 enami Exp $	*/

/*-
 * Copyright (c) 1999,2000 SATO Kazumi. All rights reserved.
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
 *	LED (LED CONTROL Unit) Registers definitions.
 *		start 0x0B000240 (??????)
 *		start 0x0F000180 (vr4122)
 */

#define LEDHTS_REG_W		0x0	/* LED H Time Set register */
#define		LEDHTS_SEC		0x10	/* 1 sec */
#define		LEDHTS_DIV2SEC		0x08	/* 0.5 sec */
#define		LEDHTS_DIV4SEC		0x04	/* 0.25 sec */
#define		LEDHTS_DIV8SEC		0x02	/* 0.125 sec */
#define		LEDHTS_DIV16SEC		0x01	/* 0.0625 sec */

#define LEDLTS_REG_W		0x2	/* LED L Time Set register */
#define		LEDLTS_4SEC		0x40	/* 4 sec */
#define		LEDLTS_2SEC		0x20	/* 2 sec */
#define		LEDLTS_SEC		0x10	/* 1 sec */
#define		LEDLTS_DIV2SEC		0x08	/* 0.5 sec */
#define		LEDLTS_DIV4SEC		0x04	/* 0.25 sec */
#define		LEDLTS_DIV8SEC		0x02	/* 0.125 sec */
#define		LEDLTS_DIV16SEC		0x01	/* 0.0625 sec */

#define LEDCNT_REG_W		0x8	/* LED Coltrol register */
#define		LEDCNT_AUTOSTOP		(1<<1)	/* Auto Stop */
#define		LEDCNT_NOAUTOSTOP	(0<<1)	/* Auto Stop Disable*/

#define		LEDCNT_BLINK		(1)	/* Start Blink  */
#define		LEDCNT_BLINK_DISABLE	(0)	/* Stop Blink */

#define LEDASTC_REG_W		0xa	/* LED Auto Stop Time register */

#define LEDINT_REG_W		0xc	/* LED Interrupt register */
#define 	LEDINT_AUTOSTOP		1	/* Auto Stop */
#define 	LEDINT_ALL		LEDINT_AUTOSTOP

/* END vrledreg.h */
