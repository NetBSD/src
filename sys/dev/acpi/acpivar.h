/*	$NetBSD: acpivar.h,v 1.2 2001/09/29 05:34:00 thorpej Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This file defines the ACPI interface provided to the rest of the
 * kernel, as well as the autoconfiguration structures for ACPI
 * support.
 */

#include <machine/bus.h>
#include <dev/pci/pcivar.h>

#include <dev/acpi/acpica.h>

/*
 * acpibus_attach_args:
 *
 *	This structure is used to attach the ACPI "bus".
 */
struct acpibus_attach_args {
	const char *aa_busname;		/* XXX should be common */
	bus_space_tag_t aa_iot;		/* PCI I/O space tag */
	bus_space_tag_t aa_memt;	/* PCI MEM space tag */
	pci_chipset_tag_t aa_pc;	/* PCI chipset */
	int aa_pciflags;		/* PCI bus flags */
};

/*
 * Types of switches that ACPI understands.
 */
#define	ACPI_SWITCH_POWERBUTTON		0
#define	ACPI_SWITCH_SLEEPBUTTON		1
#define	ACPI_SWITCH_LID			2
#define	ACPI_NSWITCHES			3

/*
 * acpi_devnode:
 *
 *	An ACPI device node.
 */
struct acpi_devnode {
	TAILQ_ENTRY(acpi_devnode) ad_list;
	ACPI_HANDLE	ad_handle;	/* our ACPI handle */
	u_int32_t	ad_level;	/* ACPI level */
	u_int32_t	ad_type;	/* ACPI object type */
	ACPI_DEVICE_INFO ad_devinfo;	/* our ACPI device info */
	struct acpi_scope *ad_scope;	/* backpointer to scope */
	struct device	*ad_device;	/* pointer to configured device */
};

/*
 * acpi_scope:
 *
 *	Description of an ACPI scope.
 */
struct acpi_scope {
	TAILQ_ENTRY(acpi_scope) as_list;
	const char *as_name;		/* scope name */
	/*
	 * Device nodes we manage.
	 */
	TAILQ_HEAD(, acpi_devnode) as_devnodes;
};

/*
 * acpi_softc:
 *
 *	Software state of the ACPI subsystem.
 */
struct acpi_softc {
	struct device sc_dev;		/* base device info */
	bus_space_tag_t sc_iot;		/* PCI I/O space tag */
	bus_space_tag_t sc_memt;	/* PCI MEM space tag */
	pci_chipset_tag_t sc_pc;	/* PCI chipset tag */
	int sc_pciflags;		/* PCI bus flags */

	void *sc_sdhook;		/* shutdown hook */

	/*
	 * Sleep state to transition to when a given
	 * switch is activated.
	 */
	int sc_switch_sleep[ACPI_NSWITCHES];

	int sc_sleepstate;		/* current sleep state */

	/*
	 * Scopes we manage.
	 */
	TAILQ_HEAD(, acpi_scope) sc_scopes;
};

/*
 * acpi_attach_args:
 *
 *	Used to attach a device instance to the acpi "bus".
 */
struct acpi_attach_args {
	struct acpi_devnode *aa_node;	/* ACPI device node */
	bus_space_tag_t aa_iot;		/* PCI I/O space tag */
	bus_space_tag_t aa_memt;	/* PCI MEM space tag */
	pci_chipset_tag_t aa_pc;	/* PCI chipset tag */
	int aa_pciflags;		/* PCI bus flags */
};

/*
 * ACPI resources:
 *
 *	acpi_io		I/O ports
 *	acpi_iorange	I/O port range
 *	acpi_mem	memory region
 *	acpi_memrange	memory range
 *	acpi_irq	Interrupt Request
 *	acpi_drq	DMA request
 */

struct acpi_io {
	SIMPLEQ_ENTRY(acpi_io) ar_list;
	int		ar_index;
	uint32_t	ar_base;
	uint32_t	ar_length;
};

struct acpi_iorange {
	SIMPLEQ_ENTRY(acpi_iorange) ar_list;
	int		ar_index;
	uint32_t	ar_low;
	uint32_t	ar_high;
	uint32_t	ar_length;
	uint32_t	ar_align;
};

struct acpi_mem {
	SIMPLEQ_ENTRY(acpi_mem) ar_list;
	int		ar_index;
	uint32_t	ar_base;
	uint32_t	ar_length;
};

struct acpi_memrange {
	SIMPLEQ_ENTRY(acpi_memrange) ar_list;
	int		ar_index;
	uint32_t	ar_low;
	uint32_t	ar_high;
	uint32_t	ar_length;
	uint32_t	ar_align;
};

struct acpi_irq {
	SIMPLEQ_ENTRY(acpi_irq) ar_list;
	int		ar_index;
	uint32_t	ar_irq;
};

struct acpi_drq {
	SIMPLEQ_ENTRY(acpi_drq) ar_list;
	int		ar_index;
	uint32_t	ar_drq;
};

struct acpi_resources {
	SIMPLEQ_HEAD(, acpi_io) ar_io;
	int ar_nio;

	SIMPLEQ_HEAD(, acpi_iorange) ar_iorange;
	int ar_niorange;

	SIMPLEQ_HEAD(, acpi_mem) ar_mem;
	int ar_nmem;

	SIMPLEQ_HEAD(, acpi_memrange) ar_memrange;
	int ar_nmemrange;

	SIMPLEQ_HEAD(, acpi_irq) ar_irq;
	int ar_nirq;

	SIMPLEQ_HEAD(, acpi_drq) ar_drq;
	int ar_ndrq;
};

/*
 * acpi_resource_parse_ops:
 *
 *	The client of ACPI resources specifies these operations
 *	when the resources are parsed.
 */
struct acpi_resource_parse_ops {
	void	(*init)(struct device *, void *, void **);
	void	(*fini)(struct device *, void *);

	void	(*ioport)(struct device *, void *, uint32_t, uint32_t);
	void	(*iorange)(struct device *, void *, uint32_t, uint32_t,
		    uint32_t, uint32_t);

	void	(*memory)(struct device *, void *, uint32_t, uint32_t);
	void	(*memrange)(struct device *, void *, uint32_t, uint32_t,
		    uint32_t, uint32_t);

	void	(*irq)(struct device *, void *, uint32_t);
	void	(*drq)(struct device *, void *, uint32_t);

	void	(*start_dep)(struct device *, void *, int);
	void	(*end_dep)(struct device *, void *);
};

extern struct acpi_softc *acpi_softc;
extern int acpi_active;

extern const struct acpi_resource_parse_ops acpi_resource_parse_ops_default;

int		acpi_probe(void);

ACPI_STATUS	acpi_eval_integer(ACPI_HANDLE, char *, int *);

ACPI_STATUS	acpi_get(ACPI_HANDLE, ACPI_BUFFER *,
		    ACPI_STATUS (*)(ACPI_HANDLE, ACPI_BUFFER *));

ACPI_STATUS	acpi_resource_parse(struct device *, struct acpi_devnode *,
		    void *, const struct acpi_resource_parse_ops *);
void		acpi_resource_print(struct device *, struct acpi_resources *);

struct acpi_io		*acpi_res_io(struct acpi_resources *, int);
struct acpi_iorange	*acpi_res_iorange(struct acpi_resources *, int);
struct acpi_mem		*acpi_res_mem(struct acpi_resources *, int);
struct acpi_memrange	*acpi_res_memrange(struct acpi_resources *, int);
struct acpi_irq		*acpi_res_irq(struct acpi_resources *, int);
struct acpi_drq		*acpi_res_drq(struct acpi_resources *, int);
