/*	$NetBSD: tx39var.h,v 1.9.2.1 2002/02/11 20:08:11 jdolecek Exp $ */

/*-
 * Copyright (c) 1999-2002 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

enum tx_chipset {
	__TX391X,
	__TX392X,
};

/* attach priority of TX-internal modules. */
enum tx_attach_order {
	ATTACH_FIRST	= 3,
	ATTACH_NORMAL	= 2,
	ATTACH_LAST	= 1,
};

/* unified I/O manager */
enum txio_group {
	MFIO = 0,
	IO,
	BETTY,
	SNOWBALL,
	PLUM,
	NTXIO_GROUP
};

struct hpcio_chip;

struct tx_chipset_tag {
	enum tx_chipset tc_chipset;
	void *tc_intrt;  /* interrupt tag */
	void *tc_powert; /* power tag */
	void *tc_clockt; /* clock/timer tag */
	void *tc_soundt; /* sound tag */
	struct hpcio_chip *tc_iochip[NTXIO_GROUP];
	void *tc_videot; /* video chip tag */
};

typedef struct tx_chipset_tag* tx_chipset_tag_t __attribute__((__unused__));
typedef u_int32_t txreg_t; 
extern struct tx_chipset_tag tx_chipset;
#define tx_conf_get_tag() (&tx_chipset)

#if defined TX391X && defined TX392X
#define IS_TX391X(t)	((t)->tc_chipset == __TX391X)
#define IS_TX392X(t)	((t)->tc_chipset == __TX392X)
#elif defined TX391X
#define IS_TX391X(t)	(1)
#define IS_TX392X(t)	(0)
#elif defined TX392X
#define IS_TX391X(t)	(0)
#define IS_TX392X(t)	(1)
#endif

void	tx_conf_register_intr(tx_chipset_tag_t, void *);
void	tx_conf_register_power(tx_chipset_tag_t, void *);
void	tx_conf_register_clock(tx_chipset_tag_t, void *);
void	tx_conf_register_sound(tx_chipset_tag_t, void *);
void	tx_conf_register_ioman(tx_chipset_tag_t, struct hpcio_chip *);
void	tx_conf_register_video(tx_chipset_tag_t, void *);

/*
 *	TX39 Internal Function Register access
 */
#define TX39_SYSADDR_CONFIG_REG_KSEG1	0xb0c00000
#define tx_conf_read(t, reg) (						\
	(*((volatile txreg_t *)(TX39_SYSADDR_CONFIG_REG_KSEG1 + (reg)))))
#define tx_conf_write(t, reg, val) (					\
	(*((volatile txreg_t *)(TX39_SYSADDR_CONFIG_REG_KSEG1 + (reg))) \
	= (val)))

/*
 *	txsim attach arguments. (txsim ... TX System Internal Module)
 */
struct txsimbus_attach_args {
	char *tba_busname;
};

/* 
 *	txsim module attach arguments. 
 */
struct txsim_attach_args {
	tx_chipset_tag_t ta_tc; /* Chipset tag */
};

/*
 *	Interrupt staff
 */
#define MAKEINTR(s, b)	((s) * 32 + (ffs(b) - 1))
void*	tx_intr_establish(tx_chipset_tag_t, int, int, int, int (*)(void *),
			  void *);
void	tx_intr_disestablish(tx_chipset_tag_t, void *);

#ifdef USE_POLL
void*	tx39_poll_establish(tx_chipset_tag_t, int, int, int (*)(void *),
			    void *);
void	tx39_poll_disestablish(tx_chipset_tag_t, void *);
#define POLL_CONT	0
#define POLL_END	1
#endif /* USE_POLL */

u_int32_t tx_intr_status(tx_chipset_tag_t, int);
extern u_int32_t tx39intrvec;

/*
 *	Power management staff
 */
void tx39power_suspend_cpu(void);

/*
 *	Debug print configration.
 */
#define USE_HPC_DPRINTF
#define __DPRINTF_EXT
