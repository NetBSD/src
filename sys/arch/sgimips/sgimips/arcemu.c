/*	$NetBSD: arcemu.c,v 1.14 2007/05/10 17:27:05 rumble Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: arcemu.c,v 1.14 2007/05/10 17:27:05 rumble Exp $");

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
	.Load =				ARCEMU_UNIMPL,
	.Invoke =			ARCEMU_UNIMPL,
	.Execute = 			ARCEMU_UNIMPL,
	.Halt =				ARCEMU_UNIMPL,
	.PowerDown =			ARCEMU_UNIMPL,
	.Restart = 			ARCEMU_UNIMPL,
	.Reboot =			ARCEMU_UNIMPL,
	.EnterInteractiveMode =		ARCEMU_UNIMPL,
	.reserved0 =			NULL,
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
	.reserved1 =			NULL,
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
	switch (arcemu_identify()) {
	case MACH_SGI_IP12:
		arcemu_ip12_init(env);
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

/*
 * IP12 specific
 */

/* Prom Vectors */
static void   (*ip12_prom_reset)(void) = (void *)MIPS_PHYS_TO_KSEG1(0x1fc00000);
static void   (*ip12_prom_reinit)(void) =(void *)MIPS_PHYS_TO_KSEG1(0x1fc00018);
static int    (*ip12_prom_printf)(const char *, ...) =
					 (void *)MIPS_PHYS_TO_KSEG1(0x1fc00080);

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

static struct arcemu_ip12env {
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
} ip12env;

static void
arcemu_ip12_eeprom_read()
{
	struct seeprom_descriptor sd;
	bus_space_handle_t bsh;
	bus_space_tag_t tag;
	u_int32_t reg;

	tag = SGIMIPS_BUS_SPACE_NORMAL;
	bus_space_map(tag, 0x1fa00000 + 0x1801bf, 1, 0, &bsh);

	/*
	 * 4D/3x and VIP12 sport a smaller EEPROM than Indigo HP1/HPLC.
	 * We're only interested in the first 64 half-words in either
	 * case, but the seeprom driver has to know how many addressing
	 * bits to feed the chip.
	 */

	/* 
	 * This appears to not be the case on my 4D/35.  We'll assume that
	 * we use eight-bit addressing mode for all IP12 variants.
	 */

	reg = *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fbd0000);

#if 0
	if ((reg & 0x8000) == 0)
		sd.sd_chip = C46;
	else
		sd.sd_chip = C56_66;
#endif

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
arcemu_ip12_init(const char **env)
{

	arcemu_v.GetPeer =		  arcemu_ip12_GetPeer;
	arcemu_v.GetChild =		  arcemu_ip12_GetChild;
	arcemu_v.GetEnvironmentVariable = arcemu_ip12_GetEnvironmentVariable;
	arcemu_v.GetMemoryDescriptor =    arcemu_ip12_GetMemoryDescriptor;
	arcemu_v.Reboot =                 (void *)ip12_prom_reset; 
	arcemu_v.PowerDown =		  (void *)ip12_prom_reinit; 
	arcemu_v.EnterInteractiveMode =   (void *)ip12_prom_reinit;	

	cn_tab = &arcemu_ip12_cn;
	arcemu_ip12_eeprom_read();

	memset(&ip12env, 0, sizeof(ip12env));
	extractenv(env, "dbaud", ip12env.dbaud, sizeof(ip12env.dbaud));
	extractenv(env, "rbaud", ip12env.rbaud, sizeof(ip12env.rbaud));
	extractenv(env, "bootmode",&ip12env.bootmode, sizeof(ip12env.bootmode));
	extractenv(env, "console", &ip12env.console, sizeof(ip12env.console));
	extractenv(env, "diskless",&ip12env.diskless, sizeof(ip12env.diskless));
	extractenv(env, "volume", ip12env.volume, sizeof(ip12env.volume));
	extractenv(env, "cpufreq", ip12env.cpufreq, sizeof(ip12env.cpufreq));
	extractenv(env, "gfx", ip12env.gfx, sizeof(ip12env.gfx));
	extractenv(env, "netaddr", ip12env.netaddr, sizeof(ip12env.netaddr));
	extractenv(env, "dlserver", ip12env.dlserver, sizeof(ip12env.dlserver));

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

static const char *
arcemu_ip12_GetEnvironmentVariable(const char *var)
{

	/* 'd'ebug (serial), 'g'raphics, 'G'raphics w/ logo */

	/* XXX This does not indicate the actual current console */
	if (strcasecmp("ConsoleOut", var) == 0) {
		/* if no keyboard is attached, we should default to serial */
		if (strstr(ip12env.gfx, "dead") != NULL)
			return "serial(0)";

		switch (ip12nvram.console) {
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
			    ip12nvram.console);
			return "serial(0)";
		}
	}

	if (strcasecmp("cpufreq", var) == 0) {
		if (ip12env.cpufreq[0] != '\0')
			return (ip12env.cpufreq);
		else
			return ("33");
	}

	if (strcasecmp("dbaud", var) == 0)
		return (ip12nvram.lbaud);

	if (strcasecmp("eaddr", var) == 0)
		return (ip12enaddr);

	if (strcasecmp("gfx", var) == 0) {
		if (ip12env.gfx[0] != '\0')
			return (ip12env.gfx);
	}

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
	ip12_prom_printf("%c", c);
}

/* Unimplemented Vector */
static void
arcemu_unimpl()
{

	panic("arcemu vector not established on IP%d", mach_type);
}
