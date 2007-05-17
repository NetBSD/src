/*	$NetBSD: arcemu.h,v 1.6.26.2 2007/05/17 13:41:00 yamt Exp $	*/

/*
 * Copyright (c) 2004 Steve Rumble 
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _ARCEMU_H_

#include <sys/param.h>

#include <mips/cpuregs.h>

#include <dev/arcbios/arcbios.h>
#include <dev/arcbios/arcbiosvar.h>

int arcemu_init(const char **env);

#ifdef _ARCEMU_PRIVATE

/* Platform identification */
static int	arcemu_identify(void);

/* Helper functions */
static boolean_t extractenv(const char **, const char *, char *, int);

/*
 * IP12 Emulation 
 */

/* Prom Emulators */
static void	arcemu_ip12_init(const char **);
static void *	arcemu_ip12_GetPeer(void *);
static void *	arcemu_ip12_GetChild(void *);
static const char *	arcemu_ip12_GetEnvironmentVariable(const char *); 
static void *	arcemu_ip12_GetMemoryDescriptor(void *mem);

static void	arcemu_ip12_eeprom_read(void);
static void	arcemu_ip12_putc(dev_t, int);

#define ARCEMU_IP12_ENVOK(_x) 			\
    (MIPS_PHYS_TO_KSEG1((_x)) >= 0xa0380000 &&	\
     MIPS_PHYS_TO_KSEG1((_x)) <  0xa0400000)

/* ARCBIOS Component Tree. Represented in linear fashion. */
static struct arcbios_component ip12_tree[] = {
	{	COMPONENT_CLASS_ProcessorClass,	COMPONENT_TYPE_CPU,
		-1, -1, -1, -1, -1, -1, -1, NULL			},

	/* end of list */
	{	-1, -1, -1, -1, -1, -1, -1, -1, -1, NULL		}
};

/* Unimplmented Vector */
#define ARCEMU_UNIMPL ((void *)arcemu_unimpl)
static void	arcemu_unimpl(void);

#endif /* _ARCEMU_PRIVATE */

#endif /* _ARCEMU_H_ */
