/*	$NetBSD: xen_machdep.c,v 1.4.12.3 2009/05/31 20:15:37 jym Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christian Limpach.
 * 4. The name of the author may not be used to endorse or promote products
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
__KERNEL_RCSID(0, "$NetBSD: xen_machdep.c,v 1.4.12.3 2009/05/31 20:15:37 jym Exp $");

#include "opt_xen.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/boot_flag.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/timetc.h>
#include <sys/sysctl.h>
#include <sys/pmf.h>

#include <xen/hypervisor.h>

#define DPRINTK(x) printk x
#if 0
#define DPRINTK(x)
#endif

u_int	tsc_get_timecount(struct timecounter *);

uint64_t tsc_freq;	/* XXX */

#ifdef XEN3
static int sysctl_xen_sleepstate(SYSCTLFN_ARGS);
void xen_suspend_domain(void);
static void xen_prepare_suspend(void);
static void xen_prepare_resume(void);
#endif

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

u_int
tsc_get_timecount(struct timecounter *tc)
{

	panic("xen: tsc_get_timecount");
}

#ifdef XEN3
/*
 * this function sets up the machdep.sleep_state sysctl equivalent
 * for guest domains with no ACPI support
 * This sysctl mimics the ACPI one, except it should be used only for 
 * Xen's save/restore functionalities of guest domains
 */
void
sysctl_xen_sleepstate_setup(void) {

	int ret;

	/*
	 * dom0 implements sleep_state support through ACPI
	 * it should not call this function to register
	 * machdep.sleep_state sysctl
	 */
	KASSERT( !(xendomain_is_dom0()) );

	ret = sysctl_createv(NULL, 0, NULL, NULL, CTLFLAG_READWRITE,
	     CTLTYPE_INT, "sleep_state", NULL, sysctl_xen_sleepstate, 0,
	     NULL, 0, CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	
	if (ret)
		aprint_error("sysctl_createv failed: %d\n", ret);
}

static int
sysctl_xen_sleepstate(SYSCTLFN_ARGS) {

	int error, t;
	struct sysctlnode node;

	node = *rnode;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (error || newp == NULL)
		return error;

	xen_suspend_domain();

	return 0;

}

/*
 * Last operations before suspending domain
 */
static void
xen_prepare_suspend(void) {

	kpreempt_disable();

	xen_suspendclocks();

	xen_acquire_writer_ptom_lock();

	/*
	 * Xen lazy evaluation of recursive mappings requires
	 * to flush the APDP entries
	 */
	pmap_unmap_all_apdp_pdes();

	/*
	 * save/restore code does not translate these MFNs to their
	 * associated PFNs, so we must do it
	 */
	xen_start_info.store_mfn = mfn_to_pfn(xen_start_info.store_mfn);
	xen_start_info.console_mfn = mfn_to_pfn(xen_start_info.console_mfn);

	DPRINTK(("suspending domain\n"));
	aprint_verbose("suspending domain\n");

	/* invalidate the shared_info page */
	if (HYPERVISOR_update_va_mapping((vaddr_t)HYPERVISOR_shared_info,
	    0, UVMF_INVLPG)) {
		DPRINTK(("HYPERVISOR_shared_info page invalidation failed"));
		HYPERVISOR_crash();
	}

}

/*
 * First operations before restoring domain context
 */
static void
xen_prepare_resume(void) {

	/* map the new shared_info page */
	if (HYPERVISOR_update_va_mapping((vaddr_t)HYPERVISOR_shared_info,
	    xen_start_info.shared_info | PG_RW | PG_V,
	    UVMF_INVLPG)) {
		DPRINTK(("could not map new shared info page"));
		HYPERVISOR_crash();
	}

	if (xen_start_info.nr_pages != physmem) {
		/*
		 * XXX JYM for now, we crash - fix it with balloon when
		 * supported
		 */
		DPRINTK(("xen_start_info.nr_pages != physmem"));
		HYPERVISOR_crash();
	}

	xen_release_ptom_lock();

	DPRINTK(("preparing domain resume\n"));
	aprint_verbose("preparing domain resume\n");

	xen_initclocks();

	kpreempt_enable();

}

void
xen_suspend_domain(void) {

	paddr_t mfn;
	int s = splvm();

	/*
	 * console becomes unavailable when suspended, so
	 * direct communications to domain are hampered from there on.
	 * We can only rely on low level primitives like printk(), until
	 * console is fully restored
	 */
	if (!pmf_system_suspend(PMF_F_NONE)) {
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
	if (!pmf_system_resume(PMF_F_NONE)) {
		DPRINTK(("devices resume failed\n"));
		HYPERVISOR_crash();
	}

	splx(s);

	/* xencons is back online, we can print to console */
	aprint_verbose("domain resumed\n");

}
#endif /* XEN3 */
