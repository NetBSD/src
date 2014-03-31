/*	$NetBSD: spdvar.h,v 1.5 2014/03/31 11:25:49 martin Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

enum spd_slot {
	SPD_HDD,
	SPD_NIC
};

struct spd_attach_args {
	enum spd_slot spa_slot;
	const char *spa_product_name;
};

/* interrupt */
void *spd_intr_establish(enum spd_slot, int (*)(void *), void *);
void spd_intr_disestablish(void *);

/* EEPROM */
void spd_eeprom_read(int, u_int16_t *, int);

/* HDD LED */
#define SPD_LED_OFF()							\
{									\
		*(volatile u_int8_t *)SPD_IO_DIR_REG8 = SPD_IO_LED;	\
		*(volatile u_int8_t *)SPD_IO_DATA_REG8 = SPD_IO_LED;\
}
#define SPD_LED_ON()							\
{									\
		*(volatile u_int8_t *)SPD_IO_DIR_REG8 = SPD_IO_LED;	\
		*(volatile u_int8_t *)SPD_IO_DATA_REG8 = 0;		\
}
