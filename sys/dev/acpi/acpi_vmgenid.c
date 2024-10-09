/*	$NetBSD: acpi_vmgenid.c,v 1.3.2.2 2024/10/09 13:25:12 martin Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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
 * Virtual Machine Generation ID
 *
 *	The VMGENID is an 8-byte cookie shared between a VM host and VM
 *	guest.  Whenever the host clones a VM, it changes the VMGENID
 *	and sends an ACPI notification to the guest.
 *
 * References:
 *
 *	`Virtual Machine Generation ID', Microsoft, 2012-08-01.
 *	http://go.microsoft.com/fwlink/?LinkId=260709
 *
 *	`Virtual Machine Generation ID Device', The QEMU Project
 *	Developers.
 *	https://www.qemu.org/docs/master/specs/vmgenid.html
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_vmgenid.c,v 1.3.2.2 2024/10/09 13:25:12 martin Exp $");

#include <sys/device.h>
#include <sys/entropy.h>
#include <sys/module.h>
#include <sys/rndsource.h>
#include <sys/sysctl.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#define	_COMPONENT	ACPI_RESOURCE_COMPONENT
ACPI_MODULE_NAME	("acpi_vmgenid")

struct acpivmgenid {
	uint8_t		id[16];
} __aligned(8);

struct acpivmgenid_softc {
	device_t			sc_dev;
	struct acpi_devnode		*sc_node;
	uint64_t			sc_paddr;
	struct acpivmgenid		*sc_vaddr;
	struct krndsource		sc_rndsource;
	struct sysctllog		*sc_sysctllog;
	const struct sysctlnode		*sc_sysctlroot;
};

static int acpivmgenid_match(device_t, cfdata_t, void *);
static void acpivmgenid_attach(device_t, device_t, void *);
static int acpivmgenid_detach(device_t, int);
static void acpivmgenid_set(struct acpivmgenid_softc *, const char *);
static void acpivmgenid_notify(ACPI_HANDLE, uint32_t, void *);
static void acpivmgenid_reset(void *);
static int acpivmgenid_sysctl(SYSCTLFN_ARGS);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "VM_Gen_Counter" },		/* from the Microsoft spec */
	{ .compat = "VM_GEN_COUNTER" },		/* used by qemu */
	{ .compat = "VMGENCTR" },		/* recognized by Linux */
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(acpivmgenid, sizeof(struct acpivmgenid_softc),
    acpivmgenid_match, acpivmgenid_attach, acpivmgenid_detach, NULL);

static int
acpivmgenid_match(device_t parent, cfdata_t match, void *aux)
{
	const struct acpi_attach_args *const aa = aux;

	return acpi_compatible_match(aa, compat_data);
}

static void
acpivmgenid_attach(device_t parent, device_t self, void *aux)
{
	struct acpivmgenid_softc *const sc = device_private(self);
	const struct acpi_attach_args *const aa = aux;
	ACPI_BUFFER addrbuf = {
		.Pointer = NULL,
		.Length = ACPI_ALLOCATE_BUFFER,
	};
	ACPI_OBJECT *addrobj, *addrarr;
	ACPI_STATUS rv;
	int error;

	aprint_naive(": ACPI VM Generation ID\n");
	aprint_normal(": ACPI VM Generation ID\n");

	sc->sc_dev = self;
	sc->sc_node = aa->aa_node;

	/*
	 * Get the address from the ADDR object, which is a package of
	 * two 32-bit integers representing the low and high halves of
	 * a 64-bit physical address.
	 */
	rv = AcpiEvaluateObjectTyped(sc->sc_node->ad_handle, "ADDR", NULL,
	    &addrbuf, ACPI_TYPE_PACKAGE);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(self, "failed to get ADDR: %s\n",
		    AcpiFormatException(rv));
		goto out;
	}
	addrobj = addrbuf.Pointer;
	if (addrobj->Type != ACPI_TYPE_PACKAGE ||
	    addrobj->Package.Count != 2) {
		aprint_error_dev(self, "invalid ADDR\n");
		goto out;
	}
	addrarr = addrobj->Package.Elements;
	if (addrarr[0].Type != ACPI_TYPE_INTEGER ||
	    addrarr[1].Type != ACPI_TYPE_INTEGER ||
	    addrarr[0].Integer.Value > UINT32_MAX ||
	    addrarr[1].Integer.Value > UINT32_MAX) {
		aprint_error_dev(self, "invalid ADDR\n");
		goto out;
	}
	sc->sc_paddr = (ACPI_PHYSICAL_ADDRESS)addrarr[0].Integer.Value;
	sc->sc_paddr |= (ACPI_PHYSICAL_ADDRESS)addrarr[1].Integer.Value << 32;
	aprint_normal_dev(self, "paddr=0x%"PRIx64"\n", (uint64_t)sc->sc_paddr);

	/*
	 * Map the physical address into virtual address space.
	 */
	sc->sc_vaddr = AcpiOsMapMemory(sc->sc_paddr, sizeof(*sc->sc_vaddr));
	if (sc->sc_vaddr == NULL) {
		aprint_error_dev(self, "failed to map address\n");
		goto out;
	}

	/*
	 * Register a random source so we can attribute samples.
	 */
	rnd_attach_source(&sc->sc_rndsource, device_xname(self),
	    RND_TYPE_UNKNOWN, RND_FLAG_COLLECT_TIME|RND_FLAG_COLLECT_VALUE);

	/*
	 * Register an ACPI notifier so that we can detect changes.
	 */
	(void)acpi_register_notify(sc->sc_node, acpivmgenid_notify);

	/*
	 * Now that we have registered a random source and a notifier,
	 * read out the first value.
	 */
	acpivmgenid_set(sc, "initial");

	/*
	 * Attach a sysctl tree, rooted at hw.acpivmgenidN.
	 */
	error = sysctl_createv(&sc->sc_sysctllog, 0, NULL, &sc->sc_sysctlroot,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, device_xname(self),
	    SYSCTL_DESCR("Virtual Machine Generation ID device"),
	    NULL, 0, NULL, 0,
	    CTL_HW, CTL_CREATE, CTL_EOL);
	if (error) {
		aprint_error_dev(self, "failed to create sysctl hw.%s: %d\n",
		    device_xname(self), error);
		goto out;
	}

	/*
	 * hw.acpivmgenidN.id (`struct', 16-byte array)
	 */
	error = sysctl_createv(&sc->sc_sysctllog, 0, &sc->sc_sysctlroot, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY|CTLFLAG_PRIVATE, CTLTYPE_STRUCT,
	    "id", SYSCTL_DESCR("Virtual Machine Generation ID"),
	    &acpivmgenid_sysctl, 0, sc, sizeof(struct acpivmgenid),
	    CTL_CREATE, CTL_EOL);
	if (error) {
		aprint_error_dev(self,
		    "failed to create sysctl hw.%s.id: %d\n",
		    device_xname(self), error);
		goto out;
	}

	/*
	 * hw.acpivmgenidN.paddr (64-bit integer)
	 */
	__CTASSERT(sizeof(ACPI_PHYSICAL_ADDRESS) == sizeof(quad_t));
	error = sysctl_createv(&sc->sc_sysctllog, 0, &sc->sc_sysctlroot, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY|CTLFLAG_PRIVATE, CTLTYPE_QUAD,
	    "paddr", SYSCTL_DESCR("Physical address of VM Generation ID"),
	    NULL, 0, &sc->sc_paddr, sizeof(sc->sc_paddr),
	    CTL_CREATE, CTL_EOL);
	if (error) {
		aprint_error_dev(self,
		    "failed to create sysctl hw.%s.paddr: %d\n",
		    device_xname(self), error);
		goto out;
	}

out:	ACPI_FREE(addrbuf.Pointer);
}

static int
acpivmgenid_detach(device_t self, int flags)
{
	struct acpivmgenid_softc *const sc = device_private(self);
	int error;

	error = config_detach_children(self, flags);
	if (error)
		return error;

	sysctl_teardown(&sc->sc_sysctllog);
	acpi_deregister_notify(sc->sc_node);
	rnd_detach_source(&sc->sc_rndsource);
	if (sc->sc_vaddr) {
		AcpiOsUnmapMemory(sc->sc_vaddr, sizeof(*sc->sc_vaddr));
		sc->sc_vaddr = NULL;	/* paranoia */
	}
	sc->sc_paddr = 0;	/* paranoia */

	return 0;
}

static void
acpivmgenid_set(struct acpivmgenid_softc *sc, const char *prefix)
{
	struct acpivmgenid vmgenid;
	char vmgenidstr[2*__arraycount(vmgenid.id) + 1];
	unsigned i;

	/*
	 * Grab the current VM generation ID.  No obvious way to make
	 * this atomic, so let's hope if it changes in the middle we'll
	 * get another notification.
	 */
	memcpy(&vmgenid, sc->sc_vaddr, sizeof(vmgenid));

	/*
	 * Print the VM generation ID to the console for posterity.
	 */
	for (i = 0; i < __arraycount(vmgenid.id); i++) {
		vmgenidstr[2*i] = "0123456789abcdef"[vmgenid.id[i] >> 4];
		vmgenidstr[2*i + 1] = "0123456789abcdef"[vmgenid.id[i] & 0xf];
	}
	vmgenidstr[2*sizeof(vmgenid)] = '\0';
	aprint_verbose_dev(sc->sc_dev, "%s: %s\n", prefix, vmgenidstr);

	/*
	 * Enter the new VM generation ID into the entropy pool.
	 */
	rnd_add_data(&sc->sc_rndsource, &vmgenid, sizeof(vmgenid), 0);
}

static void
acpivmgenid_notify(ACPI_HANDLE hdl, uint32_t notify, void *opaque)
{
	const device_t self = opaque;
	struct acpivmgenid_softc *const sc = device_private(self);

	if (notify != 0x80) {
		aprint_debug_dev(self, "unknown notify 0x%02x\n", notify);
		return;
	}

	(void)AcpiOsExecute(OSL_NOTIFY_HANDLER, &acpivmgenid_reset, sc);
}

static void
acpivmgenid_reset(void *cookie)
{
	struct acpivmgenid_softc *const sc = cookie;

	/*
	 * Reset the system entropy pool's measure of entropy (not the
	 * data, just the system's assessment of whether it has
	 * entropy), and gather more entropy from any synchronous
	 * sources we have available like CPU RNG instructions.  We
	 * can't be interrupted by a signal so ignore return value.
	 */
	entropy_reset();
	(void)entropy_gather();

	/*
	 * Grab the current VM generation ID to put it into the entropy
	 * pool; then force consolidation so it affects all subsequent
	 * draws from the entropy pool and the entropy epoch advances.
	 */
	acpivmgenid_set(sc, "cloned");
	entropy_consolidate();
}

static int
acpivmgenid_sysctl(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct acpivmgenid_softc *const sc = node.sysctl_data;

	node.sysctl_data = sc->sc_vaddr;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

MODULE(MODULE_CLASS_DRIVER, acpivmgenid, NULL);

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
acpivmgenid_modcmd(modcmd_t cmd, void *opaque)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_acpivmgenid,
		    cfattach_ioconf_acpivmgenid, cfdata_ioconf_acpivmgenid);
#endif
		return error;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_acpivmgenid,
		    cfattach_ioconf_acpivmgenid, cfdata_ioconf_acpivmgenid);
#endif
		return error;
	default:
		return ENOTTY;
	}
}
