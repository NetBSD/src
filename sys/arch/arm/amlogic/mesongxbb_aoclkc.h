/* $NetBSD: mesongxbb_aoclkc.h,v 1.1 2019/02/25 19:30:17 jmcneill Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _MESONGXBB_AOCLKC_H
#define _MESONGXBB_AOCLKC_H

#define	MESONGXBB_RESET_AO_REMOTE	0
#define	MESONGXBB_RESET_AO_I2C_MASTER	1
#define	MESONGXBB_RESET_AO_I2C_SLAVE	2
#define	MESONGXBB_RESET_AO_UART1	3
#define	MESONGXBB_RESET_AO_UART2	4
#define	MESONGXBB_RESET_AO_IR_BLASTER	5

#define	MESONGXBB_CLOCK_AO_REMOTE	0
#define	MESONGXBB_CLOCK_AO_I2C_MASTER	1
#define	MESONGXBB_CLOCK_AO_I2C_SLAVE	2
#define	MESONGXBB_CLOCK_AO_UART1	3
#define	MESONGXBB_CLOCK_AO_UART2	4
#define	MESONGXBB_CLOCK_AO_IR_BLASTER	5
#define	MESONGXBB_CLOCK_CEC_32K		6

#endif /* _MESONGXBB_AOCLKC_H */
