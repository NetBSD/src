/*	$NetBSD: arcemu.c,v 1.4 2004/04/11 11:34:13 pooka Exp $	*/

/*
 * Copyright (c) 2004 Steve Rumble 
 * Copyright (c) 2004 Antti Kantee
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: arcemu.c,v 1.4 2004/04/11 11:34:13 pooka Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/machtype.h>
#include <machine/bus.h>

#include <dev/cons.h>

#include <dev/arcbios/arcbios.h>
#include <dev/arcbios/arcbiosvar.h>

#include <dev/ic/smc93cx6var.h>

#define _ARCEMU_PRIVATE
#include <sgimips/sgimips/arcemu.h>
#include <sgimips/dev/picreg.h>

static struct consdev arcemu_ip12_cn = {
	NULL,			/* probe */
	NULL,			/* init */
	NULL,			/* getc */ /* XXX: this would be nice */
	arcemu_ip12_putc,	/* putc */
	nullcnpollc,		/* pollc */
	NULL,			/* bell */
	NULL,
	NULL,
	NODEV,
	CN_NORMAL,
};

/*
 * Emulate various ARCBIOS functions on pre-ARCS sgimips
 * machines (<= IP17).
 */

static struct arcbios_fv arcemu_v = {
	.Load =				arcemu_unimpl_Load,
	.Invoke =			arcemu_unimpl_Invoke,
	.Execute = 			arcemu_unimpl_Execute,
	.Halt =				arcemu_unimpl_void_void_noret,
	.PowerDown =			arcemu_unimpl_void_void_noret,
	.Restart = 			arcemu_unimpl_void_void_noret,
	.Reboot =			arcemu_unimpl_void_void_noret,
	.EnterInteractiveMode =		arcemu_unimpl_void_void_noret,
	.reserved0 =			NULL,
	.GetPeer =			arcemu_unimpl_voidptr_voidptr,
	.GetChild =			arcemu_unimpl_voidptr_voidptr,
	.GetParent =			arcemu_unimpl_voidptr_voidptr,
	.GetConfigurationData =		arcemu_unimpl_GetConfigurationData,
	.AddChild =			arcemu_unimpl_AddChild,
	.DeleteComponent =		arcemu_unimpl_DeleteComponent,
	.GetComponent =			arcemu_unimpl_GetComponent,
	.SaveConfiguration =		arcemu_unimpl_SaveConfiguration,
	.GetSystemId =			arcemu_unimpl_voidptr_void,
	.GetMemoryDescriptor =		arcemu_unimpl_GetMemoryDescriptor,
	.reserved1 =			NULL,
	.GetTime =			arcemu_unimpl_voidptr_void,
	.GetRelativeTime =		arcemu_unimpl_GetRelativeTime,
	.GetDirectoryEntry =		arcemu_unimpl_GetDirectoryEntry,
	.Open =				arcemu_unimpl_Open,
	.Close =			arcemu_unimpl_Close,
	.Read =				arcemu_unimpl_GetDirectoryEntry,
	.GetReadStatus =		arcemu_unimpl_GetReadStatus,
	.Write =			arcemu_unimpl_GetDirectoryEntry,
	.Seek =				arcemu_unimpl_Seek,
	.Mount =			arcemu_unimpl_Mount,
	.GetEnvironmentVariable =	arcemu_unimpl_GetEnvironmentVariable,
	.SetEnvironmentVariable =	arcemu_unimpl_SetEnvironmentVariable,
	.GetFileInformation =		arcemu_unimpl_GetFileInformation,
	.SetFileInformation =		arcemu_unimpl_SetFileInformation,
	.FlushAllCaches =		arcemu_unimpl_void_void
};

/*
 * Establish our emulated ARCBIOS vector or return ARCBIOS failure.
 */
int
arcemu_init()
{
	switch (arcemu_identify()) {
	case MACH_SGI_IP12:
		arcemu_ip12_init();
		break;

	default:
		return (1);	
	}

	ARCBIOS = &arcemu_v;

	return (0);
}

/* Attempt to identify the SGI IP%d platform. */
static int
arcemu_identify()
{

	/*
	 * XXX - identifying an HPC should be sufficient for IP12
	 *       since it's the only non-ARCS offering with one.
	 */
	return (MACH_SGI_IP12); /* boy, that was easy! */
}

/*
 * IP12 specific
 */

/*
 * The following matches IP12 NVRAM memory layout
 */
static struct arcemu_ip12_nvramdata {
	char bootmode;
	char state;
	char netaddr[16];
	char lbaud[5];
	char rbaud[5];
	char console;
	char sccolor[6];
	char pgcolor[6];
	char lgcolor[6];
	char keydb;
	char pad0;
	char checksum;
	char diskless;
	char nokdb;
	char bootfile[50];
	char passwd[17];
	char volume[3];
	uint8_t enaddr[6];
} ip12nvram;
static char ip12enaddr[18];

static void
arcemu_ip12_eeprom_read()
{
	struct seeprom_descriptor sd;
	bus_space_handle_t bsh;
	bus_space_tag_t tag;
	uint32_t reg;

	tag = SGIMIPS_BUS_SPACE_NORMAL;
	bus_space_map(tag, 0x1fa00000 + 0x1801bf, 1, 0, &bsh);

	/*
	 * 4D/3x and VIP12 sport a smaller EEPROM than Indigo HP1/HPLC.
	 * We're only interested in the first 64 half-words in either
	 * case, but the seeprom driver has to know how many addressing
	 * bits to feed the chip.
	 */
	reg = *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fbd0000);
	if ((reg & 0x8000) == 0)
		sd.sd_chip = C46;
	else
		sd.sd_chip = C56_66;

	sd.sd_tag = tag;
	sd.sd_bsh = bsh;
	sd.sd_regsize = 1;
	sd.sd_control_offset = 0;
	sd.sd_status_offset = 0;
	sd.sd_dataout_offset = 0;
	sd.sd_DI = 0x10;	/* EEPROM -> CPU */
	sd.sd_DO = 0x08;	/* CPU -> EEPROM */
	sd.sd_CK = 0x04;
	sd.sd_CS = 0x02;
	sd.sd_MS = 0;
	sd.sd_RDY = 0;

	if (read_seeprom(&sd, (uint16_t *)&ip12nvram, 0, 64) != 1)
		panic("arcemu: NVRAM read failed");

	bus_space_unmap(tag, bsh, 1); /* play technically nice */

	/* cache enaddr string */
	sprintf(ip12enaddr, "%02x:%02x:%02x:%02x:%02x:%02x",
	    ip12nvram.enaddr[0],
	    ip12nvram.enaddr[1],
	    ip12nvram.enaddr[2],
	    ip12nvram.enaddr[3],
	    ip12nvram.enaddr[4],
	    ip12nvram.enaddr[5]);
}

static void
arcemu_ip12_init()
{

	arcemu_v.GetPeer =		  arcemu_ip12_GetPeer;
	arcemu_v.GetChild =		  arcemu_ip12_GetChild;
	arcemu_v.GetEnvironmentVariable = arcemu_ip12_GetEnvironmentVariable;
	arcemu_v.GetMemoryDescriptor =    arcemu_ip12_GetMemoryDescriptor;
	arcemu_v.Reboot =                 IP12_PROM_REBOOT; 
	arcemu_v.PowerDown =              IP12_PROM_POWER_DOWN;
	arcemu_v.EnterInteractiveMode =   IP12_PROM_INTERACTIVE_MODE;

	cn_tab = &arcemu_ip12_cn;
	arcemu_ip12_eeprom_read();

	strcpy(arcbios_system_identifier, "SGI-IP12");
	strcpy(arcbios_sysid_vendor, "SGI");
	strcpy(arcbios_sysid_product, "IP12");
}

static void *
arcemu_ip12_GetPeer(void *node)
{
	int i;

	if (node == NULL)
		return (NULL);

	for (i = 0; ip12_tree[i].Class != -1; i++) {
		if (&ip12_tree[i] == node && ip12_tree[i+1].Class != -1)
			return (&ip12_tree[i+1]);
	}

	return (NULL);
}

static void *
arcemu_ip12_GetChild(void *node)
{

	/*
	 * ARCBIOS just walks the entire tree, so we'll represent our
	 * emulated tree as a single level and avoid messy hierarchies. 
	 */
	if (node == NULL)
		return (&ip12_tree[0]);

	return (NULL);
}

static char *
arcemu_ip12_GetEnvironmentVariable(char *var)
{

	/* 'd'ebug (serial), 'g'raphics, 'G'raphics w/ logo */
	if (strcasecmp("ConsoleOut", var) == 0) {
		switch (ip12nvram.console) {
		case 'd':
		case 'D':
		case 's':
		case 'S':
			return "serial(0)";
		case 'g':
		case 'G':
			return "graphics(0)";
		default:
			printf("arcemu: unknown console type %c\n",
			    ip12nvram.console);
		}
	}

	/* Not super-important.  My IP12 is 33MHz ;p */ 
	if (strcasecmp("cpufreq", var) == 0)
		return ("33");

	if (strcasecmp("dbaud", var) == 0)
		return (ip12nvram.lbaud);

	if (strcasecmp("eaddr", var) == 0)
		return (ip12enaddr);

	/* makebootdev() can handle "dksc(a,b,c)/netbsd", etc already */
	if (strcasecmp("OSLoadPartition", var) == 0)
		return (ip12nvram.bootfile);

	/* pull filename from e.g.: "dksc(0,1,0)netbsd" */
	if (strcasecmp("OSLoadFilename", var) == 0) {
		char *file;

		if ((file = strrchr(ip12nvram.bootfile, ')')) != NULL)
			return (file + 1);
		else	
			return (NULL);
	}

	return (NULL);
}

static void *
arcemu_ip12_GetMemoryDescriptor(void *mem)
{
	static int bank;
	u_int32_t memcfg;
	static struct arcbios_mem am;

	if (mem == NULL) {
		/*
		 * We know page 0 is occupied, emulate the reserved space.
		 */
		am.Type = ARCBIOS_MEM_ExceptionBlock;
		am.BasePage = 0;
		am.PageCount = 1;

		bank = 0;
		return (&am);
	} else if (bank > 3)
		return (NULL);

	switch (bank) {
	case 0:
		memcfg = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(PIC_MEMCFG0_PHYSADDR)
		    >>16;
		break;

	case 1:
		memcfg = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(PIC_MEMCFG0_PHYSADDR)
		    & 0xffff;
		break;

	case 2:
		memcfg = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(PIC_MEMCFG1_PHYSADDR)
		    >> 16;
		break;

	case 3:
		memcfg = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(PIC_MEMCFG1_PHYSADDR)
		    & 0xffff;
		break;

	default:
		memcfg = PIC_MEMCFG_BADADDR;
	}	

	if (memcfg == PIC_MEMCFG_BADADDR) {
		am.Type = ARCBIOS_MEM_BadMemory;
		am.BasePage =
		    PIC_MEMCFG_ADDR(PIC_MEMCFG_BADADDR) / ARCBIOS_PAGESIZE;
		am.PageCount = 0;
	} else {
		am.Type = ARCBIOS_MEM_FreeContiguous; 
		am.BasePage = PIC_MEMCFG_ADDR(memcfg) / ARCBIOS_PAGESIZE;
		am.PageCount = PIC_MEMCFG_SIZ(memcfg) / ARCBIOS_PAGESIZE;

		/* page 0 is occupied (if clause before switch), compensate */
		if (am.BasePage == 0) {
			am.BasePage = 1;
			am.PageCount--;	/* won't overflow */
		}
	}

	bank++;
	return (&am);
}

/*
 * If this breaks.. well.. then it breaks.
 */
static void
arcemu_ip12_putc(dev_t dummy, int c)
{
	static void (*ip12write)(char *, int, int, int) = IP12_PROM_PRINT;
	char t[2];

	t[0] = c;
	t[1] = '\0';

	ip12write(t, 0, 0, 0);
}

/*
 * Unimplemented Vectors 
 */

static void
arcemu_unimpl()
{
	panic("arcemu vector not established on IP%d.\n", mach_type);
}

static void
arcemu_unimpl_void_void_noret()
{
	panic("arcemu vector not established on IP%d.\n", mach_type);
}

static void
arcemu_unimpl_void_void()
{
	arcemu_unimpl();
}

static void *
arcemu_unimpl_voidptr_void()
{
	arcemu_unimpl();	

	return (NULL);
}

static void *
arcemu_unimpl_voidptr_voidptr(void *a)
{
	arcemu_unimpl();	

	return (NULL);
}

static uint32_t
arcemu_unimpl_Load(char *a, uint32_t b, uint32_t c, uint32_t *d)
{
	arcemu_unimpl();

	return (0);
}

static uint32_t
arcemu_unimpl_Invoke(uint32_t a, uint32_t b, uint32_t c, char **d, char **e)
{
	arcemu_unimpl();	

	return (0);
}

static uint32_t
arcemu_unimpl_Execute(char *a, uint32_t b, char **c, char **d)
{
	arcemu_unimpl();	

	return (0);
}

static uint32_t
arcemu_unimpl_GetConfigurationData(void *a, void *b)
{
	arcemu_unimpl();	

	return (0);
}

static void *
arcemu_unimpl_AddChild(void *a, void *b)
{
	arcemu_unimpl();

	return (NULL);
}

static uint32_t
arcemu_unimpl_DeleteComponent(void *a)
{
	arcemu_unimpl();

	return (0);
}

static uint32_t
arcemu_unimpl_GetComponent(char *a)
{
	arcemu_unimpl();

	return (0);
}

static uint32_t
arcemu_unimpl_SaveConfiguration()
{
	arcemu_unimpl();

	return (0);
}

static void *
arcemu_unimpl_GetMemoryDescriptor(void *a)
{
	arcemu_unimpl();

	return (NULL);
}

static uint32_t
arcemu_unimpl_GetRelativeTime()
{
	arcemu_unimpl();

	return (0);
}

static uint32_t
arcemu_unimpl_GetDirectoryEntry(uint32_t a, void *b, uint32_t c, uint32_t *d)
{
	arcemu_unimpl();

	return (0);
}

static uint32_t
arcemu_unimpl_Open(char *a, uint32_t b, uint32_t *c)
{
	arcemu_unimpl();

	return (0);
}

static uint32_t
arcemu_unimpl_Close(uint32_t a)
{
	arcemu_unimpl();

	return (0);
}

static uint32_t
arcemu_unimpl_GetReadStatus(uint32_t a)
{
	arcemu_unimpl();

	return (0);
}

static uint32_t
arcemu_unimpl_Seek(uint32_t a, int64_t *b, uint32_t c)
{
	arcemu_unimpl();

	return (0);
}

static uint32_t
arcemu_unimpl_Mount(char *a, uint32_t b)
{
	arcemu_unimpl();

	return (0);
}

static char *
arcemu_unimpl_GetEnvironmentVariable(char *a)
{
	arcemu_unimpl();

	return (NULL);
}

static uint32_t
arcemu_unimpl_SetEnvironmentVariable(char *a, char *b)
{
	arcemu_unimpl();

	return (0);
}

static uint32_t
arcemu_unimpl_GetFileInformation(uint32_t a, void *b)
{
	arcemu_unimpl();

	return (0);
}

static uint32_t
arcemu_unimpl_SetFileInformation(uint32_t a, uint32_t b, uint32_t c)
{
	arcemu_unimpl();

	return (0);
}
