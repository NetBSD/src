/*	$NetBSD: acpi_resource.c,v 1.15 2004/04/12 15:11:04 kochi Exp $	*/

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

/*-
 * Copyright (c) 2000 Michael Smith
 * Copyright (c) 2000 BSDi
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * ACPI resource parsing.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_resource.c,v 1.15 2004/04/12 15:11:04 kochi Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#define	_COMPONENT	ACPI_RESOURCE_COMPONENT
ACPI_MODULE_NAME("RESOURCE")

static ACPI_STATUS acpi_resource_parse_callback(ACPI_RESOURCE *, void *);

struct resource_parse_callback_arg {
	const struct acpi_resource_parse_ops *ops;
	struct device *dev;
	void *context;
};

static ACPI_STATUS
acpi_resource_parse_callback(ACPI_RESOURCE *res, void *context)
{
	struct resource_parse_callback_arg *arg = context;
	const struct acpi_resource_parse_ops *ops;
	int i;

	ACPI_FUNCTION_TRACE(__FUNCTION__);

	ops = arg->ops;

	switch (res->Id) {
	case ACPI_RSTYPE_FIXED_IO:
		ACPI_DEBUG_PRINT((ACPI_DB_RESOURCES,
				     "FixedIo 0x%x/%d\n",
				     res->Data.FixedIo.BaseAddress,
				     res->Data.FixedIo.RangeLength));
		if (ops->ioport)
			(*ops->ioport)(arg->dev, arg->context,
			    res->Data.FixedIo.BaseAddress,
			    res->Data.FixedIo.RangeLength);
		break;

	case ACPI_RSTYPE_IO:
		if (res->Data.Io.MinBaseAddress ==
			    res->Data.Io.MaxBaseAddress) {
			ACPI_DEBUG_PRINT((ACPI_DB_RESOURCES,
					     "Io 0x%x/%d\n",
					     res->Data.Io.MinBaseAddress,
					     res->Data.Io.RangeLength));
			if (ops->ioport)
				(*ops->ioport)(arg->dev, arg->context,
				    res->Data.Io.MinBaseAddress,
				    res->Data.Io.RangeLength);
		} else {
			ACPI_DEBUG_PRINT((ACPI_DB_RESOURCES,
					     "Io 0x%x-0x%x/%d\n",
					     res->Data.Io.MinBaseAddress,
					     res->Data.Io.MaxBaseAddress,
					     res->Data.Io.RangeLength));
			if (ops->ioport)
				(*ops->iorange)(arg->dev, arg->context,
				    res->Data.Io.MinBaseAddress,
				    res->Data.Io.MaxBaseAddress,
				    res->Data.Io.RangeLength,
				    res->Data.Io.Alignment);
		}
		break;

	case ACPI_RSTYPE_FIXED_MEM32:
		ACPI_DEBUG_PRINT((ACPI_DB_RESOURCES,
				     "FixedMemory32 0x%x/%d\n",
				     res->Data.FixedMemory32.RangeBaseAddress,
				     res->Data.FixedMemory32.RangeLength));
		if (ops->memory)
			(*ops->memory)(arg->dev, arg->context,
			    res->Data.FixedMemory32.RangeBaseAddress,
			    res->Data.FixedMemory32.RangeLength);
		break;

	case ACPI_RSTYPE_MEM32:
		if (res->Data.Memory32.MinBaseAddress ==
		    res->Data.Memory32.MaxBaseAddress) {
			ACPI_DEBUG_PRINT((ACPI_DB_RESOURCES,
					     "Memory32 0x%x/%d\n",
					     res->Data.Memory32.MinBaseAddress,
					     res->Data.Memory32.RangeLength));
			if (ops->memory)
				(*ops->memory)(arg->dev, arg->context,
				    res->Data.Memory32.MinBaseAddress,
				    res->Data.Memory32.RangeLength);
		} else {
			ACPI_DEBUG_PRINT((ACPI_DB_RESOURCES,
					     "Memory32 0x%x-0x%x/%d\n",
					     res->Data.Memory32.MinBaseAddress,
					     res->Data.Memory32.MaxBaseAddress,
					     res->Data.Memory32.RangeLength));
			if (ops->memrange)
				(*ops->memrange)(arg->dev, arg->context,
				    res->Data.Memory32.MinBaseAddress,
				    res->Data.Memory32.MaxBaseAddress,
				    res->Data.Memory32.RangeLength,
				    res->Data.Memory32.Alignment);
		}
		break;

	case ACPI_RSTYPE_MEM24:
		if (res->Data.Memory24.MinBaseAddress ==
		    res->Data.Memory24.MaxBaseAddress) {
			ACPI_DEBUG_PRINT((ACPI_DB_RESOURCES,
					     "Memory24 0x%x/%d\n",
					     res->Data.Memory24.MinBaseAddress,
					     res->Data.Memory24.RangeLength));
			if (ops->memory)
				(*ops->memory)(arg->dev, arg->context,
				    res->Data.Memory24.MinBaseAddress,
				    res->Data.Memory24.RangeLength);
		} else {
			ACPI_DEBUG_PRINT((ACPI_DB_RESOURCES,
					     "Memory24 0x%x-0x%x/%d\n",
					     res->Data.Memory24.MinBaseAddress,
					     res->Data.Memory24.MaxBaseAddress,
					     res->Data.Memory24.RangeLength));
			if (ops->memrange)
				(*ops->memrange)(arg->dev, arg->context,
				    res->Data.Memory24.MinBaseAddress,
				    res->Data.Memory24.MaxBaseAddress,
				    res->Data.Memory24.RangeLength,
				    res->Data.Memory24.Alignment);
		}
		break;

	case ACPI_RSTYPE_IRQ:
		for (i = 0; i < res->Data.Irq.NumberOfInterrupts; i++) {
			ACPI_DEBUG_PRINT((ACPI_DB_RESOURCES,
					     "IRQ %d\n",
					     res->Data.Irq.Interrupts[i]));
			if (ops->irq)
				(*ops->irq)(arg->dev, arg->context,
				    res->Data.Irq.Interrupts[i],
				    res->Data.Irq.EdgeLevel);
		}
		break;

	case ACPI_RSTYPE_DMA:
		for (i = 0; i < res->Data.Dma.NumberOfChannels; i++) {
			ACPI_DEBUG_PRINT((ACPI_DB_RESOURCES,
					     "DRQ %d\n",
					     res->Data.Dma.Channels[i]));
			if (ops->drq)
				(*ops->drq)(arg->dev, arg->context,
				    res->Data.Dma.Channels[i]);
		}
		break;

	case ACPI_RSTYPE_START_DPF:
		ACPI_DEBUG_PRINT((ACPI_DB_RESOURCES,
				     "Start dependant functions: %d\n",
				     res->Data.StartDpf.CompatibilityPriority));
		if (ops->start_dep)
			(*ops->start_dep)(arg->dev, arg->context,
			    res->Data.StartDpf.CompatibilityPriority);
		break;

	case ACPI_RSTYPE_END_DPF:
		ACPI_DEBUG_PRINT((ACPI_DB_RESOURCES,
				     "End dependant functions\n"));
		if (ops->end_dep)
			(*ops->end_dep)(arg->dev, arg->context);

	case ACPI_RSTYPE_ADDRESS32:
		ACPI_DEBUG_PRINT((ACPI_DB_RESOURCES,
				     "Address32 unimplemented\n"));
		break;

	case ACPI_RSTYPE_ADDRESS16:
		ACPI_DEBUG_PRINT((ACPI_DB_RESOURCES,
				     "Address16 unimplemented\n"));
		break;

	case ACPI_RSTYPE_EXT_IRQ:
		ACPI_DEBUG_PRINT((ACPI_DB_RESOURCES,
				     "ExtendedIrq unimplemented\n"));
		break;

	case ACPI_RSTYPE_VENDOR:
		ACPI_DEBUG_PRINT((ACPI_DB_RESOURCES,
				     "VendorSpecific unimplemented\n"));
		break;

	default:
		ACPI_DEBUG_PRINT((ACPI_DB_RESOURCES,
				     "Unknown resource type: %d\n", res->Id));
		break;
	}

	return_ACPI_STATUS(AE_OK);
}


/*
 * acpi_resource_parse:
 *
 *	Parse a device node's resources and fill them in for the
 *	client.
 *
 *	This API supports _CRS (current resources) and
 *	_PRS (possible resources).
 *
 *	Note that it might be nice to also locate ACPI-specific resource
 *	items, such as GPE bits.
 */
ACPI_STATUS
acpi_resource_parse(struct device *dev, ACPI_HANDLE handle, char *path,
    void *arg, const struct acpi_resource_parse_ops *ops)
{
	struct resource_parse_callback_arg cbarg;
	ACPI_STATUS rv;

	ACPI_FUNCTION_TRACE(__FUNCTION__);

	if (ops->init)
		(*ops->init)(dev, arg, &cbarg.context);
	else
		cbarg.context = arg;
	cbarg.ops = ops;
	cbarg.dev = dev;

	rv =AcpiWalkResources(handle, path, acpi_resource_parse_callback,
	    &cbarg);
	if (ACPI_FAILURE(rv)) {
		printf("%s: ACPI: unable to get %s resources: %s\n",
		    dev->dv_xname, path, AcpiFormatException(rv));
		return_ACPI_STATUS(rv);
	}

	if (ops->fini)
		(*ops->fini)(dev, cbarg.context);

	return_ACPI_STATUS(AE_OK);
}

/*
 * acpi_resource_print:
 *
 *	Print the resources assigned to a device.
 */
void
acpi_resource_print(struct device *dev, struct acpi_resources *res)
{
	const char *sep;

	if (SIMPLEQ_EMPTY(&res->ar_io) &&
	    SIMPLEQ_EMPTY(&res->ar_iorange) &&
	    SIMPLEQ_EMPTY(&res->ar_mem) &&
	    SIMPLEQ_EMPTY(&res->ar_memrange) &&
	    SIMPLEQ_EMPTY(&res->ar_irq) &&
	    SIMPLEQ_EMPTY(&res->ar_drq))
		return;

	printf("%s:", dev->dv_xname);

	if (SIMPLEQ_EMPTY(&res->ar_io) == 0) {
		struct acpi_io *ar;

		sep = "";
		printf(" io ");
		SIMPLEQ_FOREACH(ar, &res->ar_io, ar_list) {
			printf("%s0x%x", sep, ar->ar_base);
			if (ar->ar_length > 1)
				printf("-0x%x", ar->ar_base +
				    ar->ar_length - 1);
			sep = ",";
		}
	}

	/* XXX iorange */

	if (SIMPLEQ_EMPTY(&res->ar_mem) == 0) {
		struct acpi_mem *ar;

		sep = "";
		printf(" mem ");
		SIMPLEQ_FOREACH(ar, &res->ar_mem, ar_list) {
			printf("%s0x%x", sep, ar->ar_base);
			if (ar->ar_length > 1)
				printf("-0x%x", ar->ar_base +
				    ar->ar_length - 1);
			sep = ",";
		}
	}

	/* XXX memrange */

	if (SIMPLEQ_EMPTY(&res->ar_irq) == 0) {
		struct acpi_irq *ar;

		sep = "";
		printf(" irq ");
		SIMPLEQ_FOREACH(ar, &res->ar_irq, ar_list) {
			printf("%s%d", sep, ar->ar_irq);
			sep = ",";
		}
	}

	if (SIMPLEQ_EMPTY(&res->ar_drq) == 0) {
		struct acpi_drq *ar;

		sep = "";
		printf(" drq ");
		SIMPLEQ_FOREACH(ar, &res->ar_drq, ar_list) {
			printf("%s%d", sep, ar->ar_drq);
			sep = ",";
		}
	}

	printf("\n");
}

/*
 * acpi_resource_cleanup:
 *
 *	Free all allocated buffers
 */
void
acpi_resource_cleanup(struct acpi_resources *res)
{
	while (!SIMPLEQ_EMPTY(&res->ar_io)) {
		struct acpi_io *ar;
		ar = SIMPLEQ_FIRST(&res->ar_io);
		SIMPLEQ_REMOVE_HEAD(&res->ar_io, ar_list);
		AcpiOsFree(ar);
	}

	while (!SIMPLEQ_EMPTY(&res->ar_iorange)) {
		struct acpi_iorange *ar;
		ar = SIMPLEQ_FIRST(&res->ar_iorange);
		SIMPLEQ_REMOVE_HEAD(&res->ar_iorange, ar_list);
		AcpiOsFree(ar);
	}

	while (!SIMPLEQ_EMPTY(&res->ar_mem)) {
		struct acpi_mem *ar;
		ar = SIMPLEQ_FIRST(&res->ar_mem);
		SIMPLEQ_REMOVE_HEAD(&res->ar_mem, ar_list);
		AcpiOsFree(ar);
	}

	while (!SIMPLEQ_EMPTY(&res->ar_memrange)) {
		struct acpi_memrange *ar;
		ar = SIMPLEQ_FIRST(&res->ar_memrange);
		SIMPLEQ_REMOVE_HEAD(&res->ar_memrange, ar_list);
		AcpiOsFree(ar);
	}

	while (!SIMPLEQ_EMPTY(&res->ar_irq)) {
		struct acpi_irq *ar;
		ar = SIMPLEQ_FIRST(&res->ar_irq);
		SIMPLEQ_REMOVE_HEAD(&res->ar_irq, ar_list);
		AcpiOsFree(ar);
	}

	while (!SIMPLEQ_EMPTY(&res->ar_drq)) {
		struct acpi_drq *ar;
		ar = SIMPLEQ_FIRST(&res->ar_drq);
		SIMPLEQ_REMOVE_HEAD(&res->ar_drq, ar_list);
		AcpiOsFree(ar);
	}

	res->ar_nio = res->ar_niorange = res->ar_nmem = 
	    res->ar_nmemrange = res->ar_nirq = res->ar_ndrq = 0;
}

struct acpi_io *
acpi_res_io(struct acpi_resources *res, int idx)
{
	struct acpi_io *ar;

	SIMPLEQ_FOREACH(ar, &res->ar_io, ar_list) {
		if (ar->ar_index == idx)
			return ar;
	}
	return NULL;
}

struct acpi_iorange *
acpi_res_iorange(struct acpi_resources *res, int idx)
{
	struct acpi_iorange *ar;

	SIMPLEQ_FOREACH(ar, &res->ar_iorange, ar_list) {
		if (ar->ar_index == idx)
			return ar;
	}
	return NULL;
}

struct acpi_mem *
acpi_res_mem(struct acpi_resources *res, int idx)
{
	struct acpi_mem *ar;

	SIMPLEQ_FOREACH(ar, &res->ar_mem, ar_list) {
		if (ar->ar_index == idx)
			return ar;
	}
	return NULL;
}

struct acpi_memrange *
acpi_res_memrange(struct acpi_resources *res, int idx)
{
	struct acpi_memrange *ar;

	SIMPLEQ_FOREACH(ar, &res->ar_memrange, ar_list) {
		if (ar->ar_index == idx)
			return ar;
	}
	return NULL;
}

struct acpi_irq *
acpi_res_irq(struct acpi_resources *res, int idx)
{
	struct acpi_irq *ar;

	SIMPLEQ_FOREACH(ar, &res->ar_irq, ar_list) {
		if (ar->ar_index == idx)
			return ar;
	}
	return NULL;
}

struct acpi_drq *
acpi_res_drq(struct acpi_resources *res, int idx)
{
	struct acpi_drq *ar;

	SIMPLEQ_FOREACH(ar, &res->ar_drq, ar_list) {
		if (ar->ar_index == idx)
			return ar;
	}
	return NULL;
}

/*****************************************************************************
 * Default ACPI resource parse operations.
 *****************************************************************************/

static void	acpi_res_parse_init(struct device *, void *, void **);
static void	acpi_res_parse_fini(struct device *, void *);

static void	acpi_res_parse_ioport(struct device *, void *, uint32_t,
		    uint32_t);
static void	acpi_res_parse_iorange(struct device *, void *, uint32_t,
		    uint32_t, uint32_t, uint32_t);

static void	acpi_res_parse_memory(struct device *, void *, uint32_t,
		    uint32_t);
static void	acpi_res_parse_memrange(struct device *, void *, uint32_t,
		    uint32_t, uint32_t, uint32_t);

static void	acpi_res_parse_irq(struct device *, void *, uint32_t, uint32_t);
static void	acpi_res_parse_drq(struct device *, void *, uint32_t);

static void	acpi_res_parse_start_dep(struct device *, void *, int);
static void	acpi_res_parse_end_dep(struct device *, void *);

const struct acpi_resource_parse_ops acpi_resource_parse_ops_default = {
	.init = acpi_res_parse_init,
	.fini = acpi_res_parse_fini,

	.ioport = acpi_res_parse_ioport,
	.iorange = acpi_res_parse_iorange,

	.memory = acpi_res_parse_memory,
	.memrange = acpi_res_parse_memrange,

	.irq = acpi_res_parse_irq,
	.drq = acpi_res_parse_drq,

	.start_dep = acpi_res_parse_start_dep,
	.end_dep = acpi_res_parse_end_dep,
};

static void
acpi_res_parse_init(struct device *dev, void *arg, void **contextp)
{
	struct acpi_resources *res = arg;

	SIMPLEQ_INIT(&res->ar_io);
	res->ar_nio = 0;

	SIMPLEQ_INIT(&res->ar_iorange);
	res->ar_niorange = 0;

	SIMPLEQ_INIT(&res->ar_mem);
	res->ar_nmem = 0;

	SIMPLEQ_INIT(&res->ar_memrange);
	res->ar_nmemrange = 0;

	SIMPLEQ_INIT(&res->ar_irq);
	res->ar_nirq = 0;

	SIMPLEQ_INIT(&res->ar_drq);
	res->ar_ndrq = 0;

	*contextp = res;
}

static void
acpi_res_parse_fini(struct device *dev, void *context)
{
	struct acpi_resources *res = context;

	/* Print the resources we're using. */
	acpi_resource_print(dev, res);
}

static void
acpi_res_parse_ioport(struct device *dev, void *context, uint32_t base,
    uint32_t length)
{
	struct acpi_resources *res = context;
	struct acpi_io *ar;

	/*
	 * Check if there is another I/O port directly below/under
	 * this one.
	 */
	SIMPLEQ_FOREACH(ar, &res->ar_io, ar_list) {
		if (ar->ar_base == base + length ) {
			/*
			 * Entry just below existing entry - adjust
			 * the entry and return.
			 */
			ar->ar_base = base;
			ar->ar_length += length;
			return;
		} else if (ar->ar_base + ar->ar_length == base) {
			/*
			 * Entry just above existing entry - adjust
			 * the entry and return.
			 */
			ar->ar_length += length;
			return;
		}
	}

	ar = AcpiOsAllocate(sizeof(*ar));
	if (ar == NULL) {
		printf("%s: ACPI: unable to allocate I/O resource %d\n",
		    dev->dv_xname, res->ar_nio);
		res->ar_nio++;
		return;
	}

	ar->ar_index = res->ar_nio++;
	ar->ar_base = base;
	ar->ar_length = length;

	SIMPLEQ_INSERT_TAIL(&res->ar_io, ar, ar_list);
}

static void
acpi_res_parse_iorange(struct device *dev, void *context, uint32_t low,
    uint32_t high, uint32_t length, uint32_t align)
{
	struct acpi_resources *res = context;
	struct acpi_iorange *ar;

	ar = AcpiOsAllocate(sizeof(*ar));
	if (ar == NULL) {
		printf("%s: ACPI: unable to allocate I/O range resource %d\n",
		    dev->dv_xname, res->ar_niorange);
		res->ar_niorange++;
		return;
	}

	ar->ar_index = res->ar_niorange++;
	ar->ar_low = low;
	ar->ar_high = high;
	ar->ar_length = length;
	ar->ar_align = align;

	SIMPLEQ_INSERT_TAIL(&res->ar_iorange, ar, ar_list);
}

static void
acpi_res_parse_memory(struct device *dev, void *context, uint32_t base,
    uint32_t length)
{
	struct acpi_resources *res = context;
	struct acpi_mem *ar;

	ar = AcpiOsAllocate(sizeof(*ar));
	if (ar == NULL) {
		printf("%s: ACPI: unable to allocate Memory resource %d\n",
		    dev->dv_xname, res->ar_nmem);
		res->ar_nmem++;
		return;
	}

	ar->ar_index = res->ar_nmem++;
	ar->ar_base = base;
	ar->ar_length = length;

	SIMPLEQ_INSERT_TAIL(&res->ar_mem, ar, ar_list);
}

static void
acpi_res_parse_memrange(struct device *dev, void *context, uint32_t low,
    uint32_t high, uint32_t length, uint32_t align)
{
	struct acpi_resources *res = context;
	struct acpi_memrange *ar;

	ar = AcpiOsAllocate(sizeof(*ar));
	if (ar == NULL) {
		printf("%s: ACPI: unable to allocate Memory range resource "
		    "%d\n", dev->dv_xname, res->ar_nmemrange);
		res->ar_nmemrange++;
		return;
	}

	ar->ar_index = res->ar_nmemrange++;
	ar->ar_low = low;
	ar->ar_high = high;
	ar->ar_length = length;
	ar->ar_align = align;

	SIMPLEQ_INSERT_TAIL(&res->ar_memrange, ar, ar_list);
}

static void
acpi_res_parse_irq(struct device *dev, void *context, uint32_t irq, uint32_t type)
{
	struct acpi_resources *res = context;
	struct acpi_irq *ar;

	ar = AcpiOsAllocate(sizeof(*ar));
	if (ar == NULL) {
		printf("%s: ACPI: unable to allocate IRQ resource %d\n",
		    dev->dv_xname, res->ar_nirq);
		res->ar_nirq++;
		return;
	}

	ar->ar_index = res->ar_nirq++;
	ar->ar_irq = irq;
	ar->ar_type = type;

	SIMPLEQ_INSERT_TAIL(&res->ar_irq, ar, ar_list);
}

static void
acpi_res_parse_drq(struct device *dev, void *context, uint32_t drq)
{
	struct acpi_resources *res = context;
	struct acpi_drq *ar;

	ar = AcpiOsAllocate(sizeof(*ar));
	if (ar == NULL) {
		printf("%s: ACPI: unable to allocate DRQ resource %d\n",
		    dev->dv_xname, res->ar_ndrq);
		res->ar_ndrq++;
		return;
	}

	ar->ar_index = res->ar_ndrq++;
	ar->ar_drq = drq;

	SIMPLEQ_INSERT_TAIL(&res->ar_drq, ar, ar_list);
}

static void
acpi_res_parse_start_dep(struct device *dev, void *context, int preference)
{

	printf("%s: ACPI: dependant functions not supported\n",
	    dev->dv_xname);
}

static void
acpi_res_parse_end_dep(struct device *dev, void *context)
{

	/* Nothing to do. */
}
