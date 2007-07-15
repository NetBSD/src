/*	$NetBSD: sa11xx_pcicvar.h,v 1.2.82.1 2007/07/15 13:15:39 ad Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

struct sapcic_socket {
	struct sapcic_softc *sc;
	int socket;			/* socket number */
	struct device *pcmcia;
	struct sapcic_tag *pcictag;

	struct lwp *event_thread;
	int event;
	int laststatus;
	int shutdown;

	int power_capability;

	void *pcictag_cookie;	/* opaque data for pcictag functions */
};

struct sapcic_tag {
	int (*read)(struct sapcic_socket *, int);
	void (*write)(struct sapcic_socket *, int, int);
	void (*set_power)(struct sapcic_socket *, int);
	void (*clear_intr)(int);
	void *(*intr_establish)(struct sapcic_socket *, int,
			       int (*)(void *), void *);
	void (*intr_disestablish)(struct sapcic_socket *, void *);
};

/* registers and their values */
#define SAPCIC_STATUS_CARD		0
#define SAPCIC_CARD_VALID		1
#define SAPCIC_CARD_INVALID		0
#define SAPCIC_STATUS_VS1		1
#define SAPCIC_STATUS_VS2		2
#define SAPCIC_STATUS_READY		3

#define SAPCIC_CONTROL_RESET		8	/* assert RESET */
#define SAPCIC_CONTROL_LINEENABLE	9	/* enable control lines */
#define SAPCIC_CONTROL_WAITENABLE	10	/* enable nWAIT signal */
#define SAPCIC_CONTROL_POWERSELECT	11	/* select card voltage */

/* Voltage selection */
#define SAPCIC_POWER_OFF		0
#define SAPCIC_POWER_3V			1
#define SAPCIC_POWER_5V			2

/* common part for sacpcic_softc and sagpcic_softc */
struct sapcic_softc {
	struct device sc_dev;
	bus_space_tag_t sc_iot;

	struct lock sc_lock;
};

int	sapcic_intr(void *);
void	sapcic_kthread_create(void *);

extern struct pcmcia_chip_functions sa11x0_pcmcia_functions;
