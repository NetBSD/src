/*	$NetBSD: arcbios.c,v 1.11 2003/03/21 04:35:02 tsutsui Exp $	*/
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
#include <uvm/uvm_extern.h>
#include <dev/cons.h>
#include <machine/cpu.h>
#include <arc/arc/arcbios.h>

int Bios_Read __P((int, char *, int, int *));
int Bios_Write __P((int, char *, int, int *));
int Bios_Open __P((char *, int, u_int *));
int Bios_Close __P((u_int));
arc_mem_t *Bios_GetMemoryDescriptor __P((arc_mem_t *));
arc_sid_t *Bios_GetSystemId __P((void));
arc_config_t *Bios_GetChild __P((arc_config_t *));
arc_config_t *Bios_GetPeer __P((arc_config_t *));
arc_dsp_stat_t *Bios_GetDisplayStatus __P((int));

static void bios_config_id_copy __P((arc_config_t *, char *, size_t));
static void bios_config_component __P((arc_config_t *));
static void bios_config_subtree __P((arc_config_t *));

char arc_vendor_id[sizeof(((arc_sid_t *)0)->vendor) + 1];
unsigned char arc_product_id[sizeof(((arc_sid_t *)0)->prodid)];

char arc_id[64 + 1];

char arc_displayc_id[64 + 1];		/* DisplayController id */
arc_dsp_stat_t	arc_displayinfo;	/* Save area for display status info. */

int arc_cpu_l2cache_size = 0;

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
	NULL, NULL, biosgetc, biosputc, nullcnpollc, NULL, NULL,
	    NULL, NODEV, CN_DEAD
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
		    descr->Type, (u_long)seg_start, (u_long)seg_end,
		    (u_long)(seg_end - seg_start));
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

#ifdef BIOS_MEMORY_DEBUG
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
 * ARC Firmware present?
 */
int
bios_ident()
{
	return (
	    (ArcBiosBase->magic == ARC_PARAM_BLK_MAGIC) ||
	    (ArcBiosBase->magic == ARC_PARAM_BLK_MAGIC_BUG));
}

/*
 * save various information of BIOS for future use.
 */

static void
bios_config_id_copy(cf, string, size)
	arc_config_t *cf;
	char *string;
	size_t size;
{
	size--;
	if (size > cf->id_len)
		size = cf->id_len;
	memcpy(string, cf->id, size);
	string[size] = '\0';
}

static void
bios_config_component(cf)
	arc_config_t *cf;
{
	switch (cf->class) {
	case arc_SystemClass:
		if (cf->type == arc_System)
			bios_config_id_copy(cf, arc_id, sizeof(arc_id));
		break;
	case arc_CacheClass:
		if (cf->type == arc_SecondaryDcache)
			arc_cpu_l2cache_size = 4096 << (cf->key & 0xffff);
		break;
	case arc_ControllerClass:
		if (cf->type == arc_DisplayController &&
		    arc_displayc_id[0] == '\0' /* first found one. XXX */)
			bios_config_id_copy(cf, arc_displayc_id,
			    sizeof(arc_displayc_id));
		break;
	default:
		break;
	}
}

static
void bios_config_subtree(cf)
	arc_config_t *cf;
{
	for (cf = Bios_GetChild(cf); cf != NULL; cf = Bios_GetPeer(cf)) {
		bios_config_component(cf);
		bios_config_subtree(cf);
	}
}

void
bios_save_info()
{
	arc_sid_t *sid;

	sid = Bios_GetSystemId();
	if (sid) {
		memcpy(arc_vendor_id, sid->vendor, sizeof(arc_vendor_id) - 1);
		arc_vendor_id[sizeof(arc_vendor_id) - 1] = 0;
		memcpy(arc_product_id, sid->prodid, sizeof(arc_product_id));
	}

	bios_config_subtree(NULL);

#ifdef arc
	arc_displayinfo = *Bios_GetDisplayStatus(1);
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
	*xpos = arc_displayinfo.CursorXPosition;
	*ypos = arc_displayinfo.CursorYPosition;
	*xsize = arc_displayinfo.CursorMaxXPosition;
	*ysize = arc_displayinfo.CursorMaxYPosition;
}
#endif
