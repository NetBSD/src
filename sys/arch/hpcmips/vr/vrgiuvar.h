/*	$NetBSD: vrgiuvar.h,v 1.2 1999/12/23 06:26:10 takemura Exp $	*/

/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
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
 */
typedef void *vrgiu_chipset_tag_t;

struct vrgiu_function_tag {
	int (*gf_portread) __P((vrgiu_chipset_tag_t, int));
	int (*gf_portwrite) __P((vrgiu_chipset_tag_t, int, int));
	u_int32_t (*gf_regread_4) __P((vrgiu_chipset_tag_t, bus_addr_t));
	void (*gf_regwrite_4) __P((vrgiu_chipset_tag_t, bus_addr_t, u_int32_t));
	void *(*gf_intr_establish) __P((vrgiu_chipset_tag_t, int, int, int, int (*)(void *), void*));
	void (*gf_intr_disestablish) __P((vrgiu_chipset_tag_t, void*));
};
typedef struct vrgiu_function_tag *vrgiu_function_tag_t;

struct gpbus_attach_args {
	char *gpa_busname;
	vrgiu_chipset_tag_t gpa_gc;
	vrgiu_function_tag_t gpa_gf;
};

#define MAX_GPIO_OUT 50    /* port 32:49 are output only port */
#define MAX_GPIO_INOUT 32  /* input/output port(0:31) */

struct vrgiu_intr_entry {
	int ih_port;
	int (*ih_fun) __P((void*));
	void *ih_arg;
	TAILQ_ENTRY(vrgiu_intr_entry) ih_link;
};

struct vrgiu_softc {
	struct	device sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	/* Interrupt */
	vrip_chipset_tag_t sc_vc;
	void *sc_ih;
	u_int32_t sc_intr_mask;
	u_int32_t sc_intr_mode[MAX_GPIO_INOUT];
	TAILQ_HEAD(, vrgiu_intr_entry) sc_intr_head[MAX_GPIO_INOUT]; 
	/* Platform dependent port mapping XXX Should not be in here? */
	int sc_gpio_map[MAX_GPIO_OUT];
};

/* Interrupt options. */
#define VRGIU_INTR_EDGE		(1<<2)
#define VRGIU_INTR_LEVEL	(0<<2)
#define VRGIU_INTR_HIGH		(1<<1)
#define VRGIU_INTR_LOW		(0<<1)
#define VRGIU_INTR_HOLD		(1<<0)
#define VRGIU_INTR_THROUGH	(0<<0)

#define VRGIU_INTR_LEVEL_HIGH_HOLD	\
		(VRGIU_INTR_LEVEL|VRGIU_INTR_HIGH|VRGIU_INTR_HOLD)
#define VRGIU_INTR_LEVEL_HIGH_THROUGH	\
		(VRGIU_INTR_LEVEL|VRGIU_INTR_HIGH|VRGIU_INTR_THROUGH)
#define VRGIU_INTR_LEVEL_LOW_HOLD	\
		(VRGIU_INTR_LEVEL|VRGIU_INTR_LOW|VRGIU_INTR_HOLD)
#define VRGIU_INTR_LEVEL_LOW_THROUGH	\
		(VRGIU_INTR_LEVEL|VRGIU_INTR_LOW|VRGIU_INTR_THROUGH)
#define VRGIU_INTR_EDGE_HOLD	\
		(VRGIU_INTR_EDGE|VRGIU_INTR_HOLD)
#define VRGIU_INTR_EDGE_THROUGH	\
		(VRGIU_INTR_EDGE|VRGIU_INTR_THROUGH)

/* Before autoconfiguration */
void __vrgiu_out __P((int, int));
