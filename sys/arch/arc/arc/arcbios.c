/*	$NetBSD: arcbios.c,v 1.4 2000/03/03 12:50:19 soda Exp $	*/
/*	$OpenBSD: arcbios.c,v 1.3 1998/06/06 06:33:33 mickey Exp $	*/

/*-
 * Copyright (c) 1996 M. Warner Losh.  All rights reserved.
 * Copyright (c) 1996, 1997, 1998 Per Fogelstrom.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/kcore.h>
#include <vm/vm.h>
#include <dev/cons.h>
#include <machine/cpu.h>
#include <arc/arc/arcbios.h>
#include <arc/arc/arctype.h>

int Bios_Read __P((int, char *, int, int *));
int Bios_Write __P((int, char *, int, int *));
int Bios_Open __P((char *, int, u_int *));
int Bios_Close __P((u_int));
arc_mem_t *Bios_GetMemoryDescriptor __P((arc_mem_t *));
arc_sid_t *Bios_GetSystemId __P((void));
arc_config_t *Bios_GetChild __P((arc_config_t *));
arc_dsp_stat_t *Bios_GetDisplayStatus __P((int));

char vendor[sizeof(((arc_sid_t *)0)->vendor) + 1];
char prodid[sizeof(((arc_sid_t *)0)->prodid) + 1];

arc_dsp_stat_t	displayinfo;		/* Save area for display status info. */
static struct systypes {
	char *sys_vend;		/* Vendor ID if name is ambigous */
	char *sys_name;		/* May be left NULL if name is sufficient */
	int  sys_type;
} sys_types[] = {
#ifdef arc
    { NULL,		"PICA-61",			ACER_PICA_61 },
    { NULL,		"NEC-R94",			ACER_PICA_61 },
    { NULL,		"NEC-RD94",			NEC_RD94 },
    { NULL,		"DESKTECH-TYNE",		DESKSTATION_TYNE }, 
    { NULL,		"DESKTECH-ARCStation I",	DESKSTATION_RPC44 },
    { NULL,		"Microsoft-Jazz",		MAGNUM },
    { NULL,		"RM200PCI",			SNI_RM200 },
#endif
#ifdef sgi
    { NULL,		"SGI-IP17",			SGI_CRIMSON },
    { NULL,		"SGI-IP19",			SGI_ONYX },
    { NULL,		"SGI-IP20",			SGI_INDIGO },
    { NULL,		"SGI-IP21",			SGI_POWER },
    { NULL,		"SGI-IP22",			SGI_INDY },
    { NULL,		"SGI-IP25",			SGI_POWER10 },
    { NULL,		"SGI-IP26",			SGI_POWERI },
    { NULL,		"SGI-IP32",			SGI_O2 },
#endif
};

#define KNOWNSYSTEMS (sizeof(sys_types) / sizeof(struct systypes))

/*
 *	ARC Bios trampoline code.
 */
#define ARC_Call(Name,Offset)	\
__asm__("\n"			\
"	.text\n"		\
"	.ent	" #Name "\n"	\
"	.align	3\n"		\
"	.set	noreorder\n"	\
"	.globl	" #Name "\n" 	\
#Name":\n"			\
"	lw	$2, 0x80001020\n"\
"	lw	$2," #Offset "($2)\n"\
"	jr	$2\n"		\
"	nop\n"			\
"	.end	" #Name "\n"	);

ARC_Call(Bios_Load,			0x00);
ARC_Call(Bios_Invoke,			0x04);
ARC_Call(Bios_Execute,			0x08);
ARC_Call(Bios_Halt,			0x0c);
ARC_Call(Bios_PowerDown,		0x10);
ARC_Call(Bios_Restart,			0x14);
ARC_Call(Bios_Reboot,			0x18);
ARC_Call(Bios_EnterInteractiveMode,	0x1c);
ARC_Call(Bios_Unused1,			0x20);	/* return_from_main? */
ARC_Call(Bios_GetPeer,			0x24);
ARC_Call(Bios_GetChild,			0x28);
ARC_Call(Bios_GetParent,		0x2c);
ARC_Call(Bios_GetConfigurationData,	0x30);
ARC_Call(Bios_AddChild,			0x34);
ARC_Call(Bios_DeleteComponent,		0x38);
ARC_Call(Bios_GetComponent,		0x3c);
ARC_Call(Bios_SaveConfiguration,	0x40);
ARC_Call(Bios_GetSystemId,		0x44);
ARC_Call(Bios_GetMemoryDescriptor,	0x48);
ARC_Call(Bios_Unused2,			0x4c);	/* signal??? */
ARC_Call(Bios_GetTime,			0x50);
ARC_Call(Bios_GetRelativeTime,		0x54);
ARC_Call(Bios_GetDirectoryEntry,	0x58);
ARC_Call(Bios_Open,			0x5c);
ARC_Call(Bios_Close,			0x60);
ARC_Call(Bios_Read,			0x64);
ARC_Call(Bios_GetReadStatus,		0x68);
ARC_Call(Bios_Write,			0x6c);
ARC_Call(Bios_Seek,			0x70);
ARC_Call(Bios_Mount,			0x74);
ARC_Call(Bios_GetEnvironmentVariable,	0x78);
ARC_Call(Bios_SetEnvironmentVariable,	0x7c);
ARC_Call(Bios_GetFileInformation,	0x80);
ARC_Call(Bios_SetFileInformation,	0x84);
ARC_Call(Bios_FlushAllCaches,		0x88);
/* note: the followings don't exist on SGI */
#ifdef arc
ARC_Call(Bios_TestUnicodeCharacter,	0x8c);
ARC_Call(Bios_GetDisplayStatus,		0x90);
#endif

/*
 *	BIOS based console, for early stage.
 */

int  biosgetc __P((dev_t));
void biosputc __P((dev_t, int));

/* this is to fake out the console routines, while booting. */
struct consdev bioscons = {
	NULL, NULL, biosgetc, biosputc, nullcnpollc, NODEV, CN_DEAD
};

int
biosgetc(dev)
	dev_t dev;
{
	int cnt;
	char buf;

	if (Bios_Read(0, &buf, 1, &cnt) != arc_ESUCCESS)
		return (-1);
	return (buf & 255);
}

void
biosputc(dev, ch)
	dev_t dev;
	int ch;
{
	int cnt;
	char buf;

	buf = ch;
	Bios_Write(1, &buf, 1, &cnt);
}

void
bios_init_console()
{
	static int initialized = 0;

	if (!initialized) {
		initialized = 1;
		/* fake out the console routines, for now */
		cn_tab = &bioscons;
	}
}

/*
 * Get memory descriptor for the memory configuration and
 * create a layout database used by pmap init to set up
 * the memory system.
 *
 * Concatenate obvious adjecent segments.
 */
int
bios_configure_memory(mem_reserved, mem_clusters, mem_cluster_cnt_return)
	int *mem_reserved;
	phys_ram_seg_t *mem_clusters;
	int *mem_cluster_cnt_return;
{
	int physmem = 0;		/* Total physical memory size */
	int mem_cluster_cnt = 0;

	arc_mem_t *descr = NULL;
	paddr_t seg_start, seg_end;
	int i, reserved;

	while ((descr = Bios_GetMemoryDescriptor(descr)) != NULL) {
		seg_start = descr->BasePage * 4096;
		seg_end = seg_start + descr->PageCount * 4096;

#ifdef BIOS_MEMORY_DEBUG
		printf("memory type:%d, 0x%8lx..%8lx, size:%8ld bytes\n",
		    descr->Type, seg_start, seg_end, seg_end - seg_start);
#endif

		switch (descr->Type) {
		case BadMemory:		/* Have no use for these */
			break;

		case ExeceptionBlock:
		case SystemParameterBlock:
		case FirmwarePermanent:
			reserved = 1;
			goto account_it;

		case FreeMemory:
		case LoadedProgram:	/* This is the loaded kernel */
		case FirmwareTemporary:
		case FreeContigous:
			reserved = 0;
account_it:
			physmem += descr->PageCount * 4096;

			for (i = 0; i < mem_cluster_cnt; ) {
				if (mem_reserved[i] == reserved &&
				    mem_clusters[i].start == seg_end)
					seg_end += mem_clusters[i].size;
				else if (mem_reserved[i] == reserved &&
				    mem_clusters[i].start +
				    mem_clusters[i].size == seg_start)
					seg_start = mem_clusters[i].start;
				else { /* do not merge the cluster */
					i++;
					continue;
				}
				--mem_cluster_cnt;
				mem_reserved[i] = mem_reserved[mem_cluster_cnt];
				mem_clusters[i] = mem_clusters[mem_cluster_cnt];
			}
			/* assert(i == mem_cluster_cnt); */
			if (mem_cluster_cnt >= VM_PHYSSEG_MAX) {
				printf("VM_PHYSSEG_MAX too small\n");
				for (;;)
					;
			}
			mem_reserved[i] = reserved;
			mem_clusters[i].start =	seg_start;
			mem_clusters[i].size = seg_end - seg_start;
			mem_cluster_cnt++;
			break;

		default:		/* Unknown type, leave it alone... */
			break;
		}
	}

#ifdef MEMORY_DEBUG
	for (i = 0; i < mem_cluster_cnt; i++)
		printf("mem_clusters[%d] = %d:{ 0x%8lx, 0x%8lx }\n", i,
		    mem_reserved[i],
		    (long)mem_clusters[i].start,
		    (long)mem_clusters[i].size);
	printf("physmem = %d\n", physmem);
#endif

	*mem_cluster_cnt_return = mem_cluster_cnt;
	return (physmem);
}

/*
 * Find out system type.
 */
int
bios_ident()
{
	arc_config_t	*cf;
	arc_sid_t	*sid;
	int		i;

	if ((ArcBiosBase->magic != ARC_PARAM_BLK_MAGIC) &&
	    (ArcBiosBase->magic != ARC_PARAM_BLK_MAGIC_BUG)) {
		return (-1);	/* This is not an ARC system */
	}

#ifdef BIOS_IDENT_DEBUG
	bios_init_console();
#endif
	sid = Bios_GetSystemId();
	if (sid) {
		bcopy(sid->vendor, vendor, sizeof(sid->vendor));
		vendor[sizeof(vendor) - 1] = 0;
		bcopy(sid->prodid, prodid, sizeof(sid->prodid));
		prodid[sizeof(prodid) - 1] = 0;
#ifdef BIOS_IDENT_DEBUG
		printf("BIOS Vendor  ID [%8.8s]\n", sid->vendor);
		printf("BIOS Product ID [%02x", sid->prodid[0]);
		for (i = 1; i < sizeof(sid->prodid); i++)
			printf(":%02x", sid->prodid[i]);
		printf("]\n");
#endif
	} else {
		strcpy(vendor, "N/A");
		strcpy(prodid, "N/A");
	}
	cf = Bios_GetChild(NULL);
	if (cf) {
#ifdef BIOS_IDENT_DEBUG
		printf("BIOS System ID [%s]\n", cf->id);
#endif
		for (i = 0; i < KNOWNSYSTEMS; i++) {
			if (strcmp(sys_types[i].sys_name, cf->id) != 0)
				continue;
			if (sys_types[i].sys_vend &&
			    strncmp(sys_types[i].sys_vend, sid->vendor,
			    sizeof(sid->vendor)) != 0)
				continue;
			return (sys_types[i].sys_type);	/* Found it. */
		}
	}

	bios_init_console();
	printf("UNIDENTIFIED ARC SYSTEM [%s] VENDOR [%8.8s] PRODID [%02x",
	       cf ? cf->id : "N/A", vendor, prodid[0]);
	for (i = 1; i < sizeof(sid->prodid); i++)
		printf(":%02x", prodid[i]);
	printf("]\n");
	printf("Please contact NetBSD (mailto: port-arc@netbsd.org).\n");
	for (;;)
		;
}

/*
 * save various information of BIOS for future use.
 */
void
bios_save_info()
{
#ifdef arc
	displayinfo = *Bios_GetDisplayStatus(1);
#endif
}

#ifdef arc
/*
 * Return geometry of the display. Used by pccons.c to set up the
 * display configuration.
 */
void
bios_display_info(xpos, ypos, xsize, ysize)
	int *xpos;
	int *ypos;
	int *xsize;
	int *ysize;
{
	*xpos = displayinfo.CursorXPosition;
	*ypos = displayinfo.CursorYPosition;
	*xsize = displayinfo.CursorMaxXPosition;
	*ysize = displayinfo.CursorMaxYPosition;
}
#endif
