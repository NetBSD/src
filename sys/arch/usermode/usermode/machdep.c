/* $NetBSD: machdep.c,v 1.54.14.1 2018/06/25 07:25:46 pgoyette Exp $ */

/*-
 * Copyright (c) 2011 Reinoud Zandijk <reinoud@netbsd.org>
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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
 * Note that this machdep.c uses the `dummy' mcontext_t defined for usermode.
 * This is basicly a blob of PAGE_SIZE big. We might want to switch over to
 * non-generic mcontext_t's one day, but will this break non-NetBSD hosts?
 */


#include "opt_memsize.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.54.14.1 2018/06/25 07:25:46 pgoyette Exp $");

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/exec.h>
#include <sys/buf.h>
#include <sys/boot_flag.h>
#include <sys/ucontext.h>
#include <sys/utsname.h>
#include <machine/pcb.h>
#include <machine/psl.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_page.h>

#include <dev/mm.h>
#include <machine/vmparam.h>
#include <machine/machdep.h>
#include <machine/mainbus.h>
#include <machine/thunk.h>

#ifndef MAX_DISK_IMAGES
#define MAX_DISK_IMAGES	4
#endif

#ifndef MAX_VDEVS
#define MAX_VDEVS 4
#endif

char machine[_SYS_NMLN] = "";
char machine_arch[_SYS_NMLN] = "";
char module_machine_usermode[_SYS_NMLN] = "";

struct vm_map *phys_map = NULL;

static char **saved_argv;

char *usermode_disk_image_path[MAX_DISK_IMAGES];
int usermode_disk_image_path_count = 0;

int   usermode_vdev_type[MAX_VDEVS];
char *usermode_vdev_path[MAX_VDEVS];
int usermode_vdev_count = 0;

static char usermode_tap_devicebuf[PATH_MAX] = "";
char *usermode_tap_device = NULL;
char *usermode_tap_eaddr = NULL;
static char usermode_audio_devicebuf[PATH_MAX] = "";
char *usermode_audio_device = NULL;
char *usermode_root_device = NULL;
int usermode_vnc_width = 0;
int usermode_vnc_height = 0;
int usermode_vnc_port = -1;

void	main(int argc, char *argv[]);
void	usermode_reboot(void);

static void
usage(const char *pn)
{
	thunk_printf("usage: %s [-acdqsvxz]"
	    " [net=<tapdev>,<eaddr>]"
	    " [audio=<audiodev>]"
	    " [disk=<diskimg> ...]"
	    " [root=<device>]"
	    " [vnc=<width>x<height>,<port>]"
	    " [vdev=atapi,device]\n",
	    pn);
	thunk_printf("       (ex. \"%s"
	    " net=tap0,00:00:be:ef:ca:fe"
	    " audio=audio0"
	    " disk=root.fs"
	    " root=ld0"
	    " vnc=640x480,5900"
	    " vdev=atapi,/dev/rcd0d\")\n", pn);
}


static int
vdev_type(const char *type)
{
	if (strcasecmp(type, "atapi")==0)
		return THUNKBUS_TYPE_VATAPI;
#if 0
	if (strcasecmp(type, "scsi")==0)
		return THUNKBUS_TYPE_VSCSI;
#endif
	return -1;
}


void
main(int argc, char *argv[])
{
	extern void ttycons_consinit(void);
	extern void pmap_bootstrap(void);
	extern void kernmain(void);
	int type, i, j, r, tmpopt = 0;

	saved_argv = argv;

	/* Get machine and machine_arch from host */
	thunk_getmachine(machine, sizeof(machine),
	    machine_arch, sizeof(machine_arch));
	/* Override module_machine to be ${machine}usermode */
	snprintf(module_machine_usermode, sizeof(module_machine_usermode),
	    "%susermode", machine);

	ttycons_consinit();

	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			if (strncmp(argv[i], "net=", strlen("net=")) == 0) {
				char *tap = argv[i] + strlen("net=");
				char *mac = strchr(tap, ',');
				char *p = usermode_tap_devicebuf;
				if (mac == NULL) {
					thunk_printf("bad net= format\n");
					return;
				}
				memset(usermode_tap_devicebuf, 0,
				    sizeof(usermode_tap_devicebuf));
				if (*tap != '/') {
					memcpy(p, "/dev/", strlen("/dev/"));
					p += strlen("/dev/");
				}
				for (; *tap != ','; p++, tap++)
					*p = *tap;
				usermode_tap_device = usermode_tap_devicebuf;
				usermode_tap_eaddr = mac + 1;
			} else if (strncmp(argv[i], "audio=",
			    strlen("audio=")) == 0) {
				char *audio = argv[i] + strlen("audio=");
				if (*audio != '/')
					snprintf(usermode_audio_devicebuf,
					    sizeof(usermode_audio_devicebuf),
					    "/dev/%s", audio);
				else
					snprintf(usermode_audio_devicebuf,
					    sizeof(usermode_audio_devicebuf),
					    "%s", audio);
				usermode_audio_device =
				    usermode_audio_devicebuf;
			} else if (strncmp(argv[i], "vnc=",
			    strlen("vnc=")) == 0) {
				char *vnc = argv[i] + strlen("vnc=");
				char *w, *h, *p;
				w = vnc;
				h = strchr(w, 'x');
				if (h == NULL) {
					thunk_printf("bad vnc= format\n");
					return;
				}
				*h++ = '\0';
				p = strchr(h, ',');
				if (p == NULL) {
					thunk_printf("bad vnc= format\n");
					return;
				}
				*p++ = '\0';
				usermode_vnc_width = strtoul(w, NULL, 10);
				usermode_vnc_height = strtoul(h, NULL, 10);
				usermode_vnc_port = strtoul(p, NULL, 10);
			} else if (strncmp(argv[i], "disk=",
			    strlen("disk=")) == 0) {
				if (usermode_disk_image_path_count ==
				    MAX_DISK_IMAGES) {
					thunk_printf("too many disk images "
					    "(increase MAX_DISK_IMAGES)\n");
					usage(argv[0]);
					return;
				}
				usermode_disk_image_path[
				    usermode_disk_image_path_count++] =
				    argv[i] + strlen("disk=");
			} else if (strncmp(argv[i], "vdev=",
			    strlen("vdev=")) == 0) {
				char *vdev = argv[i] + strlen("vdev=");
				char *t, *p;
				if (usermode_disk_image_path_count ==
				    MAX_VDEVS) {
					thunk_printf("too many vdevs "
					    "(increase MAX_VDEVS)\n");
					usage(argv[0]);
					return;
				}
				t = vdev;
				p = strchr(t, ',');
				if (p == NULL) {
					thunk_printf("bad vdev= format\n");
					return;
				}
				*p++ = '\0';
				type = vdev_type(t);
				if (type < 0) {
					thunk_printf("unknown vdev device type\n");
					return;
				}
				usermode_vdev_type[usermode_vdev_count] = type;
				usermode_vdev_path[usermode_vdev_count] = p;
				usermode_vdev_count++;
			} else if (strncmp(argv[i], "root=",
			    strlen("root=")) == 0) {
				usermode_root_device = argv[i] +
				    strlen("root=");
			} else {
				thunk_printf("%s: unknown parameter\n", argv[i]);
				usage(argv[0]);
				return;
			}
			continue;
		}
		for (j = 1; argv[i][j] != '\0'; j++) {
			r = 0;
			BOOT_FLAG(argv[i][j], r);
			if (r == 0) {
				thunk_printf("unknown kernel boot flag '%c'\n", argv[i][j]);
				usage(argv[0]);
				return;
			}
			tmpopt |= r;
		}
	}
	boothowto = tmpopt;

	uvm_md_init();
	uvmexp.ncolors = 2;

	pmap_bootstrap();

	splinit();
	splraise(IPL_HIGH);

	kernmain();
}

void
usermode_reboot(void)
{
	struct thunk_itimerval itimer;

	/* make sure the timer is turned off */
	memset(&itimer, 0, sizeof(itimer));
	thunk_setitimer(ITIMER_REAL, &itimer, NULL);

	if (thunk_execv(saved_argv[0], saved_argv) == -1)
		thunk_abort();
	/* NOTREACHED */
}

void
setstatclockrate(int arg)
{
}

void
consinit(void)
{
	printf("NetBSD/usermode startup\n");
}

int
mm_md_physacc(paddr_t pa, vm_prot_t prot)
{
	// printf("%s: pa = %p, acc %d\n", __func__, (void *) pa, prot);
	if (pa >= physmem * PAGE_SIZE)
		return EFAULT;
	return 0;
}


int
mm_md_kernacc(void *ptr, vm_prot_t prot, bool *handled)
{
	const vaddr_t va = (vaddr_t)ptr;
	extern void *end;

	// printf("%s: ptr %p, acc %d\n", __func__, ptr, prot);
	if (va < kmem_kvm_start)
		return EFAULT;
	if ((va >= kmem_kvm_cur_end) && (va < kmem_k_start))
		return EFAULT;
	if (va > (vaddr_t) end)
		return EFAULT;

	*handled = true;
	return 0;
}

