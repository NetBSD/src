/*	$NetBSD: arcemu.h,v 1.1 2004/04/10 19:53:48 pooka Exp $	*/

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

int arcemu_init(void);

#ifdef _ARCEMU_PRIVATE

/* Platform identification */
static int	arcemu_identify(void);

/*
 * IP12 Emulation 
 */

/* Prom Emulators */
static void	arcemu_ip12_init(void);
static void *	arcemu_ip12_GetPeer(void *);
static void *	arcemu_ip12_GetChild(void *);
static char *	arcemu_ip12_GetEnvironmentVariable(char *); 
static void *	arcemu_ip12_GetMemoryDescriptor(void *mem);

static void	arcemu_ip12_eeprom_read(void);
static void	arcemu_ip12_putc(dev_t, int);

/* Prom Vectors */ 
#define IP12_PROM_REBOOT		(void *)MIPS_PHYS_TO_KSEG1(0x1fc00000)
#define IP12_PROM_POWER_DOWN		(void *)MIPS_PHYS_TO_KSEG1(0x1fc00018)
#define IP12_PROM_INTERACTIVE_MODE	(void *)MIPS_PHYS_TO_KSEG1(0x1fc00018)
#define IP12_PROM_PRINT			(void *)MIPS_PHYS_TO_KSEG1(0x1fc00080)

/* ARCBIOS Component Tree. Represented in linear fashion. */
static struct arcbios_component ip12_tree[] = {
	{	COMPONENT_CLASS_ProcessorClass,	COMPONENT_TYPE_CPU,
		-1, -1, -1, -1, -1, -1, -1, NULL			},

	/* end of list */
	{	-1, -1, -1, -1, -1, -1, -1, -1, -1, NULL		}
};

/*
 * Unimplmented Vectors
 */

static void	arcemu_unimpl(void);
static void	arcemu_unimpl_void_void_noret(void)
				__attribute__((__noreturn__));
static void	arcemu_unimpl_void_void(void);
static void    *arcemu_unimpl_voidptr_void(void);
static void    *arcemu_unimpl_voidptr_voidptr(void *);
static uint32_t	arcemu_unimpl_Load(char *, uint32_t, uint32_t, uint32_t *);
static uint32_t	arcemu_unimpl_Invoke(uint32_t, uint32_t, uint32_t, char **,
								   char **);
static uint32_t	arcemu_unimpl_Execute(char *, uint32_t, char **, char **);
static uint32_t	arcemu_unimpl_GetConfigurationData(void *, void *);
static void    *arcemu_unimpl_AddChild(void *, void *);
static uint32_t	arcemu_unimpl_DeleteComponent(void *);
static uint32_t	arcemu_unimpl_GetComponent(char *);
static uint32_t	arcemu_unimpl_SaveConfiguration(void);
static void    *arcemu_unimpl_GetMemoryDescriptor(void *);
static uint32_t	arcemu_unimpl_GetRelativeTime(void);
static uint32_t	arcemu_unimpl_GetDirectoryEntry(uint32_t, void *, uint32_t,
								  uint32_t *);
static uint32_t arcemu_unimpl_Open(char *, uint32_t, uint32_t *);
static uint32_t arcemu_unimpl_Close(uint32_t);
static uint32_t arcemu_unimpl_GetReadStatus(uint32_t);
static uint32_t arcemu_unimpl_Seek(uint32_t, int64_t *, uint32_t);
static uint32_t	arcemu_unimpl_Mount(char *, uint32_t); 
static char    *arcemu_unimpl_GetEnvironmentVariable(char *);
static uint32_t	arcemu_unimpl_SetEnvironmentVariable(char *, char *);
static uint32_t	arcemu_unimpl_GetFileInformation(uint32_t, void *);
static uint32_t	arcemu_unimpl_SetFileInformation(uint32_t, uint32_t, uint32_t);

#endif /* _ARCEMU_PRIVATE */

#endif /* _ARCEMU_H_ */
