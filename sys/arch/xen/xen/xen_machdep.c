/*	$NetBSD: xen_machdep.c,v 1.27.4.1 2023/10/18 16:53:03 martin Exp $	*/

/*
 * Copyright (c) 2006 Manuel Bouyer.
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
 *
 */

/*
 *
 * Copyright (c) 2004 Christian Limpach.
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
__KERNEL_RCSID(0, "$NetBSD: xen_machdep.c,v 1.27.4.1 2023/10/18 16:53:03 martin Exp $");

#include "opt_xen.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/boot_flag.h>
#include <sys/conf.h>
#include <sys/disk.h>
#include <sys/device.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/timetc.h>
#include <sys/sysctl.h>
#include <sys/pmf.h>
#include <sys/xcall.h>

#include <dev/cons.h>

#include <xen/intr.h>
#include <xen/hypervisor.h>
#include <xen/shutdown_xenbus.h>
#include <xen/include/public/version.h>

#include <machine/pmap_private.h>

#define DPRINTK(x) printk x
#if 0
#define DPRINTK(x)
#endif

#ifdef DEBUG_GEOM
#define DPRINTF(a) printf a
#else
#define DPRINTF(a)
#endif


bool xen_suspend_allow;

void
xen_parse_cmdline(int what, union xen_cmdline_parseinfo *xcp)
{
	char _cmd_line[256], *cmd_line, *opt, *s;
	int b, i, ipidx = 0;
	uint32_t xi_ip[5];
	size_t len;

	len = strlcpy(_cmd_line, xen_start_info.cmd_line, sizeof(_cmd_line));
	if (len > sizeof(_cmd_line)) {
		printf("command line exceeded limit of 255 chars. Truncated.\n");
	}
	cmd_line = _cmd_line;

	switch (what) {
	case XEN_PARSE_BOOTDEV:
		xcp->xcp_bootdev[0] = 0;
		break;
	case XEN_PARSE_CONSOLE:
		xcp->xcp_console[0] = 0;
		break;
	}

	while (cmd_line && *cmd_line) {
		opt = cmd_line;
		cmd_line = strchr(opt, ' ');
		if (cmd_line)
			*cmd_line = 0;

		switch (what) {
		case XEN_PARSE_BOOTDEV:
			if (strncasecmp(opt, "bootdev=", 8) == 0) {
				strncpy(xcp->xcp_bootdev, opt + 8,
				    sizeof(xcp->xcp_bootdev));
				break;
			}
			if (strncasecmp(opt, "root=", 5) == 0) {
				strncpy(xcp->xcp_bootdev, opt + 5,
				    sizeof(xcp->xcp_bootdev));
				break;
			}
			break;

		case XEN_PARSE_NETINFO:
			if (xcp->xcp_netinfo.xi_root &&
			    strncasecmp(opt, "nfsroot=", 8) == 0)
				strncpy(xcp->xcp_netinfo.xi_root, opt + 8,
				    MNAMELEN);

			if (strncasecmp(opt, "ip=", 3) == 0) {
				memset(xi_ip, 0, sizeof(xi_ip));
				opt += 3;
				ipidx = 0;
				while (opt && *opt) {
					s = opt;
					opt = strchr(opt, ':');
					if (opt)
						*opt = 0;

					switch (ipidx) {
					case 0:	/* ip */
					case 1:	/* nfs server */
					case 2:	/* gw */
					case 3:	/* mask */
					case 4:	/* host */
						if (*s == 0)
							break;
						for (i = 0; i < 4; i++) {
							b = strtoul(s, &s, 10);
							xi_ip[ipidx] = b + 256
								* xi_ip[ipidx];
							if (*s != '.')
								break;
							s++;
						}
						if (i < 3)
							xi_ip[ipidx] = 0;
						break;
					case 5:	/* interface */
						if (!strncmp(s, "xennet", 6))
							s += 6;
						else if (!strncmp(s, "eth", 3))
							s += 3;
						else
							break;
						if (xcp->xcp_netinfo.xi_ifno
						    == strtoul(s, NULL, 10))
							memcpy(xcp->
							    xcp_netinfo.xi_ip,
							    xi_ip,
							    sizeof(xi_ip));
						break;
					}
					ipidx++;

					if (opt)
						*opt++ = ':';
				}
			}
			break;

		case XEN_PARSE_CONSOLE:
			if (strncasecmp(opt, "console=", 8) == 0)
				strncpy(xcp->xcp_console, opt + 8,
				    sizeof(xcp->xcp_console));
			break;

		case XEN_PARSE_BOOTFLAGS:
			if (*opt == '-') {
				opt++;
				while(*opt != '\0') {
					BOOT_FLAG(*opt, boothowto);
					opt++;
				}
			}
			break;
		case XEN_PARSE_PCIBACK:
			if (strncasecmp(opt, "pciback.hide=", 13) == 0)
				strncpy(xcp->xcp_pcidevs, opt + 13,
				    sizeof(xcp->xcp_pcidevs));
			break;
		}

		if (cmd_line)
			*cmd_line++ = ' ';
	}
}

#ifdef XENPV

static int sysctl_xen_suspend(SYSCTLFN_ARGS);
static void xen_suspend_domain(void);
static void xen_prepare_suspend(void);
static void xen_prepare_resume(void);

/*
 * this function sets up the machdep.xen.suspend sysctl(7) that
 * controls domain suspend/save.
 */
void
sysctl_xen_suspend_setup(void)
{
	const struct sysctlnode *node = NULL;

	/*
	 * dom0 implements sleep support through ACPI. It should not call
	 * this function to register a suspend interface.
	 */
	KASSERT(!(xendomain_is_dom0()));

	sysctl_createv(NULL, 0, NULL, &node,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0,
	    CTL_MACHDEP, CTL_EOL);

	sysctl_createv(NULL, 0, &node, &node,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "xen",
	    SYSCTL_DESCR("Xen top level node"),
	    NULL, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);

	sysctl_createv(NULL, 0, &node, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE | CTLFLAG_IMMEDIATE,
	    CTLTYPE_INT, "suspend",
	    SYSCTL_DESCR("Suspend/save current Xen domain"),
	    sysctl_xen_suspend, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);
}

static int
sysctl_xen_suspend(SYSCTLFN_ARGS)
{
	int error;
	struct sysctlnode node;

	node = *rnode;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (error || newp == NULL)
		return error;

	/* only allow domain to suspend when dom0 instructed to do so */
	if (xen_suspend_allow == false)
		return EAGAIN;

	xen_suspend_domain();

	return 0;

}

static void xen_suspendclocks_xc(void *, void*);
static void xen_resumeclocks_xc(void *, void*);

/*
 * Last operations before suspending domain
 */
static void
xen_prepare_suspend(void)
{

	kpreempt_disable();

	pmap_xen_suspend();
	xc_wait(xc_broadcast(0, &xen_suspendclocks_xc, NULL, NULL));

	/*
	 * save/restore code does not translate these MFNs to their
	 * associated PFNs, so we must do it
	 */
	xen_start_info.store_mfn =
	    atop(xpmap_mtop(ptoa(xen_start_info.store_mfn)));
	xen_start_info.console_mfn =
	    atop(xpmap_mtop(ptoa(xen_start_info.console_mfn)));

	DPRINTK(("suspending domain\n"));
	aprint_verbose("suspending domain\n");

	/* invalidate the shared_info page */
	if (HYPERVISOR_update_va_mapping((vaddr_t)HYPERVISOR_shared_info,
	    0, UVMF_INVLPG)) {
		DPRINTK(("HYPERVISOR_shared_info page invalidation failed"));
		HYPERVISOR_crash();
	}

}

static void
xen_suspendclocks_xc(void *a, void *b)
{

	kpreempt_disable();
	xen_suspendclocks(curcpu());
	kpreempt_enable();
}

/*
 * First operations before restoring domain context
 */
static void
xen_prepare_resume(void)
{
	/* map the new shared_info page */
	if (HYPERVISOR_update_va_mapping((vaddr_t)HYPERVISOR_shared_info,
	    xen_start_info.shared_info | PTE_W | PTE_P,
	    UVMF_INVLPG)) {
		DPRINTK(("could not map new shared info page"));
		HYPERVISOR_crash();
	}

	pmap_xen_resume();

	if (xen_start_info.nr_pages != physmem) {
		/*
		 * XXX JYM for now, we crash - fix it with memory
		 * hotplug when supported
		 */
		DPRINTK(("xen_start_info.nr_pages != physmem"));
		HYPERVISOR_crash();
	}

	DPRINTK(("preparing domain resume\n"));
	aprint_verbose("preparing domain resume\n");

	xen_suspend_allow = false;

	xc_wait(xc_broadcast(0, xen_resumeclocks_xc, NULL, NULL));

	kpreempt_enable();

}

static void
xen_resumeclocks_xc(void *a, void *b)
{

	kpreempt_disable();
	xen_resumeclocks(curcpu());
	kpreempt_enable();
}

static void
xen_suspend_domain(void)
{
	paddr_t mfn;
	int s = splvm(); /* XXXSMP */

	/*
	 * console becomes unavailable when suspended, so
	 * direct communications to domain are hampered from there on.
	 * We can only rely on low level primitives like printk(), until
	 * console is fully restored
	 */
	if (!pmf_system_suspend(PMF_Q_NONE)) {
		DPRINTK(("devices suspend failed"));
		HYPERVISOR_crash();
	}

	/*
	 * obtain the MFN of the start_info page now, as we will not be
	 * able to do it once pmap is locked
	 */
	pmap_extract_ma(pmap_kernel(), (vaddr_t)&xen_start_info, &mfn);
	mfn >>= PAGE_SHIFT;

	xen_prepare_suspend();

	DPRINTK(("calling HYPERVISOR_suspend()\n"));
	if (HYPERVISOR_suspend(mfn) != 0) {
	/* XXX JYM: implement checkpoint/snapshot (ret == 1) */
		DPRINTK(("HYPERVISOR_suspend() failed"));
		HYPERVISOR_crash();
	}

	DPRINTK(("left HYPERVISOR_suspend()\n"));

	xen_prepare_resume();

	DPRINTK(("resuming devices\n"));
	if (!pmf_system_resume(PMF_Q_NONE)) {
		DPRINTK(("devices resume failed\n"));
		HYPERVISOR_crash();
	}

	splx(s);

	/* xencons is back online, we can print to console */
	aprint_verbose("domain resumed\n");

}
#endif /* XENPV */

#define PRINTK_BUFSIZE 1024
void
printk(const char *fmt, ...)
{
	va_list ap;
	int ret;
	static char buf[PRINTK_BUFSIZE];

	va_start(ap, fmt);
	ret = vsnprintf(buf, PRINTK_BUFSIZE - 1, fmt, ap);
	va_end(ap);
	buf[ret] = 0;
	(void)HYPERVISOR_console_io(CONSOLEIO_write, ret, buf);
}

static int early_xenconscn_getc(dev_t);
static void early_xenconscn_putc(dev_t, int);
static void early_xenconscn_pollc(dev_t, int);

static struct consdev early_xencons = {
	NULL, NULL,
	early_xenconscn_getc, early_xenconscn_putc, early_xenconscn_pollc,
	NULL, NULL, NULL, NODEV, CN_NORMAL
};

void
xen_early_console(void)
{
	cn_tab = &early_xencons; /* fallback console */
}

static int
early_xenconscn_getc(dev_t dev)
{
	while(1)
		;
	return -1;
}

static void 
early_xenconscn_putc(dev_t dev, int c)
{
	printk("%c", c);
}

static void
early_xenconscn_pollc(dev_t dev, int on)
{
	return;
}
bool xen_feature_tables[XENFEAT_NR_SUBMAPS * 32];

void
xen_init_features(void)
{
	xen_feature_info_t features;

	for (int sm = 0; sm < XENFEAT_NR_SUBMAPS; sm++) {
		features.submap_idx = sm;
		if (HYPERVISOR_xen_version(XENVER_get_features, &features) < 0)
			break;
		for (int f = 0; f < 32; f++) {
			xen_feature_tables[sm * 32 + f] =
			    (features.submap & (1 << f)) ? 1 : 0;
		}
	}
}

/*
 * Attempt to find the device from which we were booted.
 */

static int
is_valid_disk(device_t dv)
{
	if (device_class(dv) != DV_DISK)
		return (0);

	return (device_is_a(dv, "dk") ||
		device_is_a(dv, "sd") ||
		device_is_a(dv, "wd") ||
		device_is_a(dv, "ld") ||
		device_is_a(dv, "ed") ||
		device_is_a(dv, "xbd"));
}

void
xen_bootconf(void)
{
	device_t dv;
	deviter_t di;
	union xen_cmdline_parseinfo xcp;
	static char bootspecbuf[sizeof(xcp.xcp_bootdev)];

	if (booted_device) {
		DPRINTF(("%s: preset booted_device: %s\n", __func__, device_xname(booted_device)));
		return;
	}

	xen_parse_cmdline(XEN_PARSE_BOOTDEV, &xcp);

	for (dv = deviter_first(&di, DEVITER_F_ROOT_FIRST);
	     dv != NULL;
	     dv = deviter_next(&di)) {
		bool is_ifnet, is_disk;
		const char *devname;

		is_ifnet = (device_class(dv) == DV_IFNET);
		is_disk = is_valid_disk(dv);
		devname = device_xname(dv);

		if (!is_ifnet && !is_disk)
			continue;

		if (is_disk && xcp.xcp_bootdev[0] == 0) {
			booted_device = dv;
			break;
		}

		if (strncmp(xcp.xcp_bootdev, devname, strlen(devname)))
			continue;

		if (is_disk && strlen(xcp.xcp_bootdev) > strlen(devname)) {
			/* XXX check device_cfdata as in x86_autoconf.c? */
			booted_partition = toupper(
				xcp.xcp_bootdev[strlen(devname)]) - 'A';
			DPRINTF(("%s: booted_partition: %d\n", __func__, booted_partition));
		}

		booted_device = dv;
		booted_method = "bootinfo/bootdev";
		break;
	}
	deviter_release(&di);

	if (booted_device) {
		DPRINTF(("%s: booted_device: %s\n", __func__, device_xname(booted_device)));
		return;
	}

	/*
	 * not a boot device name, pass through to MI code
	 */
	if (xcp.xcp_bootdev[0] != '\0') {
		strlcpy(bootspecbuf, xcp.xcp_bootdev, sizeof(bootspecbuf));
		bootspec = bootspecbuf;
		booted_method = "bootinfo/bootspec";
		DPRINTF(("%s: bootspec: %s\n", __func__, bootspec));
		return;
	}
}
