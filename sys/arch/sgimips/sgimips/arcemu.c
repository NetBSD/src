/*	$NetBSD: arcemu.c,v 1.21.12.1 2014/08/20 00:03:23 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: arcemu.c,v 1.21.12.1 2014/08/20 00:03:23 tls Exp $");

#ifndef _LP64

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/machtype.h>
#include <sys/bus.h>

#include <dev/cons.h>

#include <dev/arcbios/arcbios.h>
#include <dev/arcbios/arcbiosvar.h>

#include <dev/ic/wd33c93reg.h>

#define _ARCEMU_PRIVATE
#include <sgimips/sgimips/arcemu.h>
#include <sgimips/dev/picreg.h>

static struct consdev arcemu_cn = {
	NULL,			/* probe */
	NULL,			/* init */
	NULL,			/* getc */ /* XXX: this would be nice */
	arcemu_prom_putc,	/* putc */
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
	.Load =				ARCEMU_UNIMPL,
	.Invoke =			ARCEMU_UNIMPL,
	.Execute = 			ARCEMU_UNIMPL,
	.Halt =				ARCEMU_UNIMPL,
	.PowerDown =			ARCEMU_UNIMPL,
	.Restart = 			ARCEMU_UNIMPL,
	.Reboot =			ARCEMU_UNIMPL,
	.EnterInteractiveMode =		ARCEMU_UNIMPL,
	.ReturnFromMain =		0,
	.GetPeer =			ARCEMU_UNIMPL,
	.GetChild =			ARCEMU_UNIMPL,
	.GetParent =			ARCEMU_UNIMPL,
	.GetConfigurationData =		ARCEMU_UNIMPL,
	.AddChild =			ARCEMU_UNIMPL,
	.DeleteComponent =		ARCEMU_UNIMPL,
	.GetComponent =			ARCEMU_UNIMPL,
	.SaveConfiguration =		ARCEMU_UNIMPL,
	.GetSystemId =			ARCEMU_UNIMPL,
	.GetMemoryDescriptor =		ARCEMU_UNIMPL,
	.Signal =			0,
	.GetTime =			ARCEMU_UNIMPL,
	.GetRelativeTime =		ARCEMU_UNIMPL,
	.GetDirectoryEntry =		ARCEMU_UNIMPL,
	.Open =				ARCEMU_UNIMPL,
	.Close =			ARCEMU_UNIMPL,
	.Read =				ARCEMU_UNIMPL,
	.GetReadStatus =		ARCEMU_UNIMPL,
	.Write =			ARCEMU_UNIMPL,
	.Seek =				ARCEMU_UNIMPL,
	.Mount =			ARCEMU_UNIMPL,
	.GetEnvironmentVariable =	ARCEMU_UNIMPL,
	.SetEnvironmentVariable =	ARCEMU_UNIMPL,
	.GetFileInformation =		ARCEMU_UNIMPL,
	.SetFileInformation =		ARCEMU_UNIMPL,
	.FlushAllCaches =		ARCEMU_UNIMPL
};

/*
 * Establish our emulated ARCBIOS vector or return ARCBIOS failure.
 */
int
arcemu_init(const char **env)
{
	switch ((mach_type = arcemu_identify())) {
	case MACH_SGI_IP6 | MACH_SGI_IP10:
	case MACH_SGI_IP12:
		arcemu_ipN_init(ARCEMU_ENVOK(env) ? env : NULL);
		break;

	default:
		return (1);	
	}

	ARCBIOS = &arcemu_v;

	return (0);
}

/*
 * Attempt to identify the SGI IP%d platform. This is extra ugly since
 * we don't yet have badaddr to fall back on.
 */
static int
arcemu_identify(void)
{
	int mach;

	/*
	 * Try to write a value to one of IP12's pic(4) graphics DMA registers.
	 * This is at the same location as four byte parity strobes on IP6,
	 * which appear to always read 0.
	 */
	*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(0x1faa0000) = 0xdeadbeef;
	DELAY(1000);
	if (*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(0x1faa0000) == 0xdeadbeef)
		mach = MACH_SGI_IP12;
	else
		mach = MACH_SGI_IP6;
	*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(0x1faa0000) = 0;
	(void)*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(0x1faa0000);

	return (mach);
}

static boolean_t
extractenv(const char **env, const char *key, char *dest, int len)
{
	int i;

	if (env == NULL)
		return (false);

	for (i = 0; env[i] != NULL; i++) {
		if (strncasecmp(env[i], key, strlen(key)) == 0 &&
		    env[i][strlen(key)] == '=') {
			strlcpy(dest, strchr(env[i], '=') + 1, len);
			return (true);
		}
	}

	return (false);
}

/* Prom Vectors */
static void   (*sgi_prom_reset)(void) = (void *)MIPS_PHYS_TO_KSEG1(0x1fc00000);
static void   (*sgi_prom_reinit)(void) =(void *)MIPS_PHYS_TO_KSEG1(0x1fc00018);
static int    (*sgi_prom_printf)(const char *, ...) =
					 (void *)MIPS_PHYS_TO_KSEG1(0x1fc00080);

/*
 * The following matches IP6's and IP12's NVRAM memory layout
 */
static struct arcemu_nvramdata {
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
} nvram;

static char enaddr[18];

static struct arcemu_sgienv {
	char dbaud[5];
	char rbaud[5];
	char bootmode;
	char console;
	char diskless;
	char volume[4];
	char cpufreq[3];
	char gfx[32];
	char netaddr[32];
	char dlserver[32];
	char osloadoptions[32];
} sgienv;

/*
 * EEPROM reading routines. IP6's wiring is sufficiently ugly and the routine
 * sufficiently small that we just roll our own, rather than contorting the MD
 * driver.
 */
static void
eeprom_read(uint8_t *eeprom_buf, size_t len, int is_cs56,
    void (*set_pre)(int), void (*set_cs)(int), void (*set_sk)(int),
    int (*get_do)(void), void (*set_di)(int))
{
	int i, j;

	for (i = 0; i < (len / 2); i++) {
		uint16_t instr = 0xc000 | (i << ((is_cs56) ? 5 : 7));
		uint16_t bitword = 0;

		set_di(0);
		set_sk(0);
		set_pre(0);
		set_cs(0);
		set_cs(1);
		set_sk(1);

		for (j = 0; j < ((is_cs56) ? 11 : 9); j++) {
			set_di(instr & 0x8000);
			set_sk(0);
			set_sk(1);
			instr <<= 1;
		}

		set_di(0);

		for (j = 0; j < 17; j++) {
			bitword = (bitword << 1) | get_do();
			set_sk(0);
			set_sk(1);
		}

		eeprom_buf[i * 2 + 0] = bitword >> 8;
		eeprom_buf[i * 2 + 1] = bitword & 0xff;
	
		set_sk(0);
		set_cs(0);
	}
}

/*
 * Read the EEPROM. It's not clear which machines have which parts, and
 * there's a difference in instruction length between the two. We'll try
 * both and see which doesn't give us garbage.
 */
static void
arcemu_eeprom_read(void)
{
	int i;

	/* try long instruction length first (the only one I've seen) */
	for (i = 1; i >= 0; i--) {
		if (mach_type == (MACH_SGI_IP6 | MACH_SGI_IP10)) {
			eeprom_read((uint8_t *)&nvram, sizeof(nvram), i,
			    ip6_set_pre, ip6_set_cs, ip6_set_sk,
			    ip6_get_do,  ip6_set_di);
		} else {
			eeprom_read((uint8_t *)&nvram, sizeof(nvram), i,
			    ip12_set_pre, ip12_set_cs, ip12_set_sk,
			    ip12_get_do,  ip12_set_di);
		}

		if (nvram.enaddr[0] == 0x08 && nvram.enaddr[1] == 0x00 &&
		    nvram.enaddr[2] == 0x69)
			break;

		if (memcmp(nvram.lbaud, "9600\x0", 5) == 0)
			break;

		if (memcmp(nvram.bootfile, "dksc(", 5) == 0 ||
		    memcmp(nvram.bootfile, "bootp(", 6) == 0)
			break;
	}

	/* cache enaddr string */
	snprintf(enaddr, sizeof(enaddr), "%02x:%02x:%02x:%02x:%02x:%02x",
	    nvram.enaddr[0],
	    nvram.enaddr[1],
	    nvram.enaddr[2],
	    nvram.enaddr[3],
	    nvram.enaddr[4],
	    nvram.enaddr[5]);
}

static void
arcemu_ipN_init(const char **env)
{

	arcemu_v.GetPeer =		  (intptr_t)arcemu_GetPeer;
	arcemu_v.GetChild =		  (intptr_t)arcemu_GetChild;
	arcemu_v.GetEnvironmentVariable = (intptr_t)arcemu_GetEnvironmentVariable;

	if (mach_type == MACH_SGI_IP6 || mach_type == MACH_SGI_IP10)
		arcemu_v.GetMemoryDescriptor = (intptr_t)arcemu_ip6_GetMemoryDescriptor;
	else if (mach_type == MACH_SGI_IP12)
		arcemu_v.GetMemoryDescriptor = (intptr_t)arcemu_ip12_GetMemoryDescriptor;

	arcemu_v.Reboot =                 (intptr_t)sgi_prom_reset; 
	arcemu_v.PowerDown =		  (intptr_t)sgi_prom_reinit; 
	arcemu_v.EnterInteractiveMode =   (intptr_t)sgi_prom_reinit;	

	cn_tab = &arcemu_cn;

	arcemu_eeprom_read();

	memset(&sgienv, 0, sizeof(sgienv));
	extractenv(env, "dbaud", sgienv.dbaud, sizeof(sgienv.dbaud));
	extractenv(env, "rbaud", sgienv.rbaud, sizeof(sgienv.rbaud));
	extractenv(env, "bootmode",&sgienv.bootmode, sizeof(sgienv.bootmode));
	extractenv(env, "console", &sgienv.console, sizeof(sgienv.console));
	extractenv(env, "diskless",&sgienv.diskless, sizeof(sgienv.diskless));
	extractenv(env, "volume", sgienv.volume, sizeof(sgienv.volume));
	extractenv(env, "cpufreq", sgienv.cpufreq, sizeof(sgienv.cpufreq));
	extractenv(env, "gfx", sgienv.gfx, sizeof(sgienv.gfx));
	extractenv(env, "netaddr", sgienv.netaddr, sizeof(sgienv.netaddr));
	extractenv(env, "dlserver", sgienv.dlserver, sizeof(sgienv.dlserver));
	extractenv(env, "osloadoptions", sgienv.osloadoptions,
	    sizeof(sgienv.osloadoptions));

	strcpy(arcbios_sysid_vendor, "SGI");
	if (mach_type == MACH_SGI_IP6 || mach_type == MACH_SGI_IP10) {
		strcpy(arcbios_system_identifier, "SGI-IP6");
		strcpy(arcbios_sysid_product, "IP6");
	} else if (mach_type == MACH_SGI_IP12) {
		strcpy(arcbios_system_identifier, "SGI-IP12");
		strcpy(arcbios_sysid_product, "IP12");
	}
}

static void *
arcemu_GetPeer(void *node)
{
	int i;

	if (node == NULL)
		return (NULL);

	for (i = 0; arcemu_component_tree[i].Class != -1; i++) {
		if (&arcemu_component_tree[i] == node &&
		     arcemu_component_tree[i+1].Class != -1)
			return (&arcemu_component_tree[i+1]);
	}

	return (NULL);
}

static void *
arcemu_GetChild(void *node)
{

	/*
	 * ARCBIOS just walks the entire tree, so we'll represent our
	 * emulated tree as a single level and avoid messy hierarchies. 
	 */
	if (node == NULL)
		return (&arcemu_component_tree[0]);

	return (NULL);
}

static const char *
arcemu_GetEnvironmentVariable(const char *var)
{

	/* 'd'ebug (serial), 'g'raphics, 'G'raphics w/ logo */

	/* XXX This does not indicate the actual current console */
	if (strcasecmp("ConsoleOut", var) == 0) {
		/* if no keyboard is attached, we should default to serial */
		if (strstr(sgienv.gfx, "dead") != NULL)
			return "serial(0)";

		switch (nvram.console) {
		case 'd':
		case 'D':
		case 's':
		case 'S':
			return "serial(0)";
		case 'g':
		case 'G':
			return "video()";
		default:
			printf("arcemu: unknown console \"%c\", using serial\n",
			    nvram.console);
			return "serial(0)";
		}
	}

	if (strcasecmp("cpufreq", var) == 0) {
		if (sgienv.cpufreq[0] != '\0')
			return (sgienv.cpufreq);

		/* IP6 is 12, IP10 is 20 */
		if (mach_type == MACH_SGI_IP6 || mach_type == MACH_SGI_IP10)
			return ("16");
			
		/* IP12 is 30, 33 or 36 */
		return ("33");
	}

	if (strcasecmp("dbaud", var) == 0)
		return (nvram.lbaud);

	if (strcasecmp("eaddr", var) == 0)
		return (enaddr);

	if (strcasecmp("gfx", var) == 0) {
		if (sgienv.gfx[0] != '\0')
			return (sgienv.gfx);
	}

	/*
	 * Ugly Kludge Alert!
	 *
	 * Since we don't yet have an ip12 bootloader, we can only squish
	 * a kernel into the volume header. However, this makes the bootfile
	 * something like 'dksc(0,1,8)', which translates into 'sd0i'. Ick.
	 * Munge what we return to always map to 'sd0a'. Lord have mercy.
	 *
	 * makebootdev() can handle "dksc(a,b,c)/netbsd", etc already
	 */
	if (strcasecmp("OSLoadPartition", var) == 0) {
		char *hack;

		hack = strstr(nvram.bootfile, ",8)");
		if (hack != NULL)
			hack[1] = '0';
		return (nvram.bootfile);
	}

	/* pull filename from e.g.: "dksc(0,1,0)netbsd" */
	if (strcasecmp("OSLoadFilename", var) == 0) {
		char *file;

		if ((file = strrchr(nvram.bootfile, ')')) != NULL)
			return (file + 1);
		else	
			return (NULL);
	}

	/*
	 * As far as I can tell, old systems had no analogue of OSLoadOptions.
	 * So, to allow forcing of single user mode, we accomodate the
	 * user setting the ARCBIOSy environment variable "OSLoadOptions" to
	 * something other than "auto".
	 */
	if (strcasecmp("OSLoadOptions", var) == 0) {
		if (sgienv.osloadoptions[0] == '\0')
			return ("auto");
		else
			return (sgienv.osloadoptions);
	}

	return (NULL);
}

static void *
arcemu_ip6_GetMemoryDescriptor(void *mem)
{
	static struct arcbios_mem am;
	static int invoc;

	unsigned int pages;
	u_int8_t memcfg;

	if (mem == NULL) {
		/*
		 * We know pages 0, 1 are occupied, emulate the reserved space.
		 */
		am.Type = ARCBIOS_MEM_ExceptionBlock;
		am.BasePage = 0;
		am.PageCount = 2;

		invoc = 0;
		return (&am);
	}

	memcfg = *(volatile uint8_t *)MIPS_PHYS_TO_KSEG1(0x1f800000) & 0x1f;
	pages = (memcfg & 0x0f) + 1;

	/* 4MB or 1MB units? */
	if (memcfg & 0x10) {
		pages *= 4096;

#if 0 // may cause an an exception and bring us down in flames; disable until tested
		/* check for aliasing and adjust page count if necessary */
		volatile uint8_t *tp1, *tp2;
		uint8_t tmp;

		tp1 = (volatile uint8_t *)MIPS_PHYS_TO_KSEG1((pages - 4096) << 12);
		tp2 = tp1 + (4 * 1024 * 1024);

		tmp = *tp1;
		*tp2 = ~tmp;
		if (*tp1 != tmp)
			pages -= (3 * 1024);
#endif
	} else {
		pages *= 1024;
	}

	/*
	 * It appears that the PROM's stack is at 0x400000 in physical memory.
	 * Don't destroy it, and assume (based on IP12 specs), that the prom bss
	 * is below it at 0x380000. This is probably overly conservative.
	 *
	 * Also note that we preserve the first two pages.
	 */
	switch (invoc) {
	case 0:
		/* free: pages [2, 896) */
		am.BasePage = 2;
		am.PageCount = 894;
		am.Type = ARCBIOS_MEM_FreeContiguous; 
		break;

	case 1:
		/* prom bss/stack: pages [896, 1023) */
		am.BasePage = 896;
		am.PageCount = 128;
		am.Type = ARCBIOS_MEM_FirmwareTemporary;
		break;

	case 2:
		/* free: pages [1024, ...) */
		am.BasePage = 1024;
		if (pages < 1024)
			am.PageCount = 0;
		else
			am.PageCount = pages - 1024;
		am.Type = ARCBIOS_MEM_FreeContiguous; 
		break;

	default:
		return (NULL);
	}

	invoc++;
	return (&am);
}

static void *
arcemu_ip12_GetMemoryDescriptor(void *mem)
{
	static int bank;
	u_int32_t memcfg;
	static struct arcbios_mem am;

	if (mem == NULL) {
		/*
		 * We know pages 0, 1 are occupied, emulate the reserved space.
		 */
		am.Type = ARCBIOS_MEM_ExceptionBlock;
		am.BasePage = 0;
		am.PageCount = 2;

		bank = 0;
		return (&am);
	}

	if (bank > 3)
		return (NULL);

	switch (bank) {
	case 0:
		memcfg = *(u_int32_t *)
		    MIPS_PHYS_TO_KSEG1(PIC_MEMCFG0_PHYSADDR) >> 16;
		break;

	case 1:
		memcfg = *(u_int32_t *)
		    MIPS_PHYS_TO_KSEG1(PIC_MEMCFG0_PHYSADDR) & 0xffff;
		break;

	case 2:
		memcfg = *(u_int32_t *)
		    MIPS_PHYS_TO_KSEG1(PIC_MEMCFG1_PHYSADDR) >> 16;
		break;

	case 3:
		memcfg = *(u_int32_t *)
		    MIPS_PHYS_TO_KSEG1(PIC_MEMCFG1_PHYSADDR) & 0xffff;
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

		/* pages 0, 1 are occupied (if clause before switch), compensate */
		if (am.BasePage == 0) {
			am.BasePage = 2;
			am.PageCount -= 2;	/* won't overflow */
		}
	}

	bank++;
	return (&am);
}

/*
 * If this breaks.. well.. then it breaks.
 */
static void
arcemu_prom_putc(dev_t dummy, int c)
{
	sgi_prom_printf("%c", c);
}

/* Unimplemented Vector */
static void
arcemu_unimpl(void)
{

	panic("arcemu vector not established on IP%d", mach_type);
}
#endif /* !_LP64 */
