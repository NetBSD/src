/*	$NetBSD: empbreg.h,v 1.4.2.1 2013/02/25 00:28:22 tls Exp $ */

/*-
 * Copyright (c) 2012, 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

/*
 * Elbox Mediator registers. This information was obtained using reverse 
 * engineering methods by Frank Wille and Radoslaw Kujawa (without access to
 * official documentation). 
 */

#ifndef _AMIGA_EMPBREG_H_

/* Zorro IDs */
#define ZORRO_MANID_ELBOX	2206    
#define ZORRO_PRODID_MEDZIV	31
#define ZORRO_PRODID_MED1K2	32
#define ZORRO_PRODID_MED4K	33      
#define ZORRO_PRODID_MED1K2SX	40
#define ZORRO_PRODID_MED1K2LT2	48
#define ZORRO_PRODID_MED1K2LT4	49
#define ZORRO_PRODID_MED1K2TX	60
#define ZORRO_PRODID_MED4KMKII	63  

#define ZORRO_PRODID_MEDZIV_MEM		ZORRO_PRODID_MEDZIV+0x80
#define ZORRO_PRODID_MED1K2_MEM		ZORRO_PRODID_MED1K2+0x80
#define ZORRO_PRODID_MED4K_MEM		ZORRO_PRODID_MED4K+0x80
#define ZORRO_PRODID_MED1K2SX_MEM	ZORRO_PRODID_MED1K2SX+0x80
#define ZORRO_PRODID_MED1K2LT2_MEM	ZORRO_PRODID_MED1K2LT2+0x80
#define ZORRO_PRODID_MED1K2LT4_MEM	ZORRO_PRODID_MED1K2LT4+0x80
#define ZORRO_PRODID_MED1K2TX_MEM	ZORRO_PRODID_MED1K2TX+0x80
#define ZORRO_PRODID_MED4KMKII_MEM	ZORRO_PRODID_MED4KMKII+0x80

/*
 * Mediator 1200 consists of two boards. First board lives in Z2 I/O
 * space and is internally divided into two 64kB spaces. Second board, used
 * as a window into PCI memory space is configured somewhere within 24-bit Fast
 * RAM space (its size depends on a WINDOW jumper setting).
 */
#define EMPB_SETUP_OFF		0x00000000
#define EMPB_SETUP_SIZE		0x30

#define EMPB_SETUP_WINDOW_OFF	0x2	/* set memory window position */
#define EMPB_SETUP_BRIDGE_OFF	0x7	/* select between conf or I/O */
#define EMPB_SETUP_INTR_OFF	0xB	/* interrupt setup */

#define BRIDGE_CONF		0xA0	/* switch into configuration space */
#define BRIDGE_IO		0x20	/* switch into I/O space */

#define EMPB_BRIDGE_OFF		0x00010000
#define EMPB_BRIDGE_SIZE	0xFFFF

#define	EMPB_CONF_DEV_STRIDE	0x800	/* offset between PCI devices */
#define EMPB_CONF_FUNC_STRIDE	0x100 	/* XXX: offset between PCI funcs */ 

#define EMPB_INTR_ENABLE	0xFF	/* enable all interrupts */

#define EMPB_WINDOW_SHIFT	0x10	
#define EMPB_WINDOW_MASK_8M	0xFF80
#define EMPB_WINDOW_MASK_4M	0xFFC0

#define EMPB_MEM_BASE		0x80000000
#define EMPB_MEM_END		0xA0000000

#define EMPB_PM_OFF		0x40	/* power management register */
#define EMPB_PM_PSU_SHUTDOWN	0x0

/* All PCI interrupt lines are wired to INT2. */
#define EMPB_INT		2

/*
 * Elbox Mediator 4000.
 *
 * Similar design to Mediator 1200, consists of two Zorro III boards.
 * First (with lower ID, 16MB) is used to access bridge setup, configuration 
 * and I/O spaces. The second board (256MB or 512MB) is a window into PCI 
 * memory space.
 */
#define EM4K_SETUP_OFF		0x0
#define EM4K_SETUP_SIZE		0x1F

#define EM4K_CONF_OFF		0x00800000
#define EM4K_CONF_SIZE		0x003FFFFF

#define EM4K_IO_OFF		0x00C00000
#define EM4K_IO_SIZE		0x000FFFFF

#define EM4K_SETUP_WINDOW_OFF	0x0	/* window position register */
#define EM4K_SETUP_INTR_OFF	0x4	/* interrupt setup */

#define EM4K_WINDOW_SHIFT	0x18
#define EM4K_WINDOW_MASK_512M	0xE0
#define EM4K_WINDOW_MASK_256M	0xF0

#endif /* _AMIGA_EMPBREG_H_ */

