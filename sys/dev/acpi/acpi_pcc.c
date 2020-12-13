/* $NetBSD: acpi_pcc.c,v 1.1 2020/12/13 20:27:53 jmcneill Exp $ */

/*-
 * Copyright (c) 2020 Jared McNeill <jmcneill@invisible.ca>
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
 * Platform Communications Channel (PCC) device driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_pcc.c,v 1.1 2020/12/13 20:27:53 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_pcc.h>

#define	PCC_MEMORY_BARRIER()	membar_sync()
#if defined(__aarch64__)
#define	PCC_DMA_BARRIER()	__asm __volatile("dsb sy" ::: "memory")
#else
#error PCC_BARRIER not implemented on this platform
#endif

#define	PCC_SIGNATURE(space_id)		(0x50434300 | (space_id))
#define	PCC_STATUS_COMMAND_COMPLETE	__BIT(0)
#define	PCC_STATUS_COMMAND_ERROR	__BIT(2)

struct pcc_register {
	ACPI_GENERIC_ADDRESS	reg_addr;
	uint64_t		reg_preserve;
	uint64_t		reg_set;
};

struct pcc_subspace {
	uint8_t			ss_id;
	uint8_t			ss_type;
	void *			ss_data;
	size_t			ss_len;
	struct pcc_register	ss_doorbell_reg;
	uint32_t		ss_latency;
	uint32_t		ss_turnaround;
	kmutex_t		ss_lock;
};

struct pcc_softc {
	device_t		sc_dev;
	struct pcc_subspace *	sc_ss;
	u_int			sc_nss;
};

typedef void (*pcc_subspace_callback)(ACPI_SUBTABLE_HEADER *, uint8_t, void *);

static int	pcc_match(device_t, cfdata_t, void *);
static void	pcc_attach(device_t, device_t, void *);

static uint8_t	pcc_subspace_foreach(ACPI_TABLE_PCCT *,
				     pcc_subspace_callback, void *);
static void	pcc_subspace_attach(ACPI_SUBTABLE_HEADER *, uint8_t, void *);

static struct pcc_softc *pcc_softc = NULL;

CFATTACH_DECL_NEW(acpipcc, sizeof(struct pcc_softc),
    pcc_match, pcc_attach, NULL, NULL);

static int
pcc_match(device_t parent, cfdata_t cf, void *aux)
{
	ACPI_TABLE_HEADER *hdrp = aux;

	return memcmp(hdrp->Signature, ACPI_SIG_PCCT, ACPI_NAMESEG_SIZE) == 0;
}

static void
pcc_attach(device_t parent, device_t self, void *aux)
{
	struct pcc_softc * const sc = device_private(self);
	ACPI_TABLE_PCCT *pcct = aux;
	u_int count;

	if (pcc_softc != NULL) {
		aprint_error(": Already attached!\n");
		return;
	}

	count = pcc_subspace_foreach(pcct, NULL, NULL);
	if (count == 0) {
		aprint_error(": No subspaces found!\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Platform Communications Channel, %u subspace%s\n",
	    count, count == 1 ? "" : "s");

	sc->sc_dev = self;
	sc->sc_nss = count;
	sc->sc_ss = kmem_zalloc(count * sizeof(*sc->sc_ss), KM_SLEEP);
	pcc_subspace_foreach(pcct, pcc_subspace_attach, sc);

	pcc_softc = sc;
}

/*
 * pcc_subspace_foreach --
 *
 *	Find all subspaces defined in the PCCT table. If a 'func' callback
 *	is provided, the callback is invoked for each subspace with the
 *	table pointer, subspace ID, and 'arg' as arguments. Returns the
 *	number of subspaces found.
 */
static uint8_t
pcc_subspace_foreach(ACPI_TABLE_PCCT *pcct, pcc_subspace_callback func,
    void *arg)
{
	ACPI_SUBTABLE_HEADER *header;
	char *ptr, *end;
	uint8_t count;

	end = (char *)pcct + pcct->Header.Length;
	ptr = (char *)pcct + sizeof(*pcct);
	count = 0;

	while (ptr < end && count < 255) {
		header = (ACPI_SUBTABLE_HEADER *)ptr;
		if (header->Length == 0 || ptr + header->Length > end) {
			break;
		}
		if (func != NULL) {
			func(header, count, arg);
		}
		++count;
		ptr += header->Length;
	}

	return count;
}

/*
 * pcc_subspace_attach --
 *
 *	Allocate resources for a PCC subspace.
 */
static void
pcc_subspace_attach(ACPI_SUBTABLE_HEADER *header, uint8_t id, void *arg)
{
	struct pcc_softc * const sc = arg;
	struct pcc_subspace * const ss = &sc->sc_ss[id];
	ACPI_PCCT_SUBSPACE *generic = (ACPI_PCCT_SUBSPACE *)header;

	ss->ss_id = id;
	ss->ss_type = header->Type;
	ss->ss_data = AcpiOsMapMemory(generic->BaseAddress, generic->Length);
	if (ss->ss_data == NULL) {
		panic("%s: couldn't map subspace 0x%02x at 0x%016" PRIx64,
		    device_xname(sc->sc_dev), id, generic->BaseAddress);
	}
	ss->ss_len = generic->Length;
	ss->ss_doorbell_reg.reg_addr = generic->DoorbellRegister;
	ss->ss_doorbell_reg.reg_preserve = generic->PreserveMask;
	ss->ss_doorbell_reg.reg_set = generic->WriteMask;
	mutex_init(&ss->ss_lock, MUTEX_DEFAULT, IPL_NONE);
}

/*
 * pcc_wait_command --
 *
 *	Wait for command complete to be set on a PCC subspace, with timeout.
 */
static ACPI_STATUS
pcc_wait_command(struct pcc_subspace *ss, bool first)
{
	volatile ACPI_PCCT_SHARED_MEMORY *shmem;
	int retry;

	switch (ss->ss_type) {
	case ACPI_PCCT_TYPE_GENERIC_SUBSPACE:
		/*
		 * OSPM does not have to wait for command complete before
		 * sending the first command.
		 */
		if (first) {
			return AE_OK;
		}
		/* FALLTHROUGH */

	case ACPI_PCCT_TYPE_HW_REDUCED_SUBSPACE:
	case ACPI_PCCT_TYPE_HW_REDUCED_SUBSPACE_TYPE2:
		/*
		 * Wait for the command complete bit to be set in the status
		 * register.
		 */
		shmem = ss->ss_data;
		retry = imax(1000, ss->ss_turnaround + ss->ss_latency * 100);
		while (retry > 0) {
			PCC_MEMORY_BARRIER();
			if ((shmem->Status & PCC_STATUS_COMMAND_COMPLETE) != 0)
				return AE_OK;
			delay(10);
			retry -= 10;
		}
		return AE_TIME;

	default:
		return AE_NOT_IMPLEMENTED;
	}
}

/*
 * pcc_doorbell --
 *
 *	Ring the door bell by writing to the doorbell register.
 */
static ACPI_STATUS
pcc_doorbell(struct pcc_register *reg)
{
	ACPI_STATUS rv;
	UINT64 val;

	rv = AcpiRead(&val, &reg->reg_addr);
	if (ACPI_FAILURE(rv)) {
		return rv;
	}
	val &= reg->reg_preserve;
	val |= reg->reg_set;
	return AcpiWrite(val, &reg->reg_addr);
}

/*
 * pcc_send_command --
 *
 *	Write a command into PCC subspace and ring the doorbell.
 */
static ACPI_STATUS
pcc_send_command(struct pcc_subspace *ss, ACPI_GENERIC_ADDRESS *reg,
    uint32_t command, int flags, ACPI_INTEGER val)
{
	volatile ACPI_PCCT_SHARED_MEMORY *shmem = ss->ss_data;
	uint8_t *data = __UNVOLATILE(shmem + 1);
	UINT64 tmp, mask;

	KASSERT(ss->ss_type == ACPI_PCCT_TYPE_GENERIC_SUBSPACE ||
	        ss->ss_type == ACPI_PCCT_TYPE_HW_REDUCED_SUBSPACE ||
		ss->ss_type == ACPI_PCCT_TYPE_HW_REDUCED_SUBSPACE_TYPE2);

	shmem->Signature = PCC_SIGNATURE(ss->ss_id);
	shmem->Command = command & 0xff;
	if ((flags & PCC_WRITE) != 0) {
		mask = __BITS(reg->BitOffset + reg->BitWidth - 1,
			      reg->BitOffset);
		ACPI_MOVE_64_TO_64(&tmp, data + reg->Address);
		tmp &= mask;
		tmp |= __SHIFTIN(val, mask);
		ACPI_MOVE_64_TO_64(data + reg->Address, &tmp);
	}
	PCC_MEMORY_BARRIER();
	shmem->Status &= ~(PCC_STATUS_COMMAND_COMPLETE |
			   PCC_STATUS_COMMAND_ERROR);
	PCC_DMA_BARRIER();

	return pcc_doorbell(&ss->ss_doorbell_reg);
}

/*
 * pcc_receive_response --
 *
 *	Process a command response in PCC subspace.
 */
static ACPI_STATUS
pcc_receive_response(struct pcc_subspace *ss, ACPI_GENERIC_ADDRESS *reg,
    int flags, ACPI_INTEGER *val)
{
	volatile ACPI_PCCT_SHARED_MEMORY *shmem = ss->ss_data;
	const uint8_t *data = __UNVOLATILE(shmem + 1);
	UINT64 tmp, mask;

	KASSERT(ss->ss_type == ACPI_PCCT_TYPE_GENERIC_SUBSPACE ||
	        ss->ss_type == ACPI_PCCT_TYPE_HW_REDUCED_SUBSPACE ||
		ss->ss_type == ACPI_PCCT_TYPE_HW_REDUCED_SUBSPACE_TYPE2);

	KASSERT((shmem->Status & PCC_STATUS_COMMAND_COMPLETE) != 0);

	if ((shmem->Status & PCC_STATUS_COMMAND_ERROR) != 0) {
		return AE_ERROR;
	}

	if ((flags & PCC_READ) != 0) {
		mask = __BITS(reg->BitOffset + reg->BitWidth - 1,
			      reg->BitOffset);
		ACPI_MOVE_64_TO_64(&tmp, data + reg->Address);
		*val = __SHIFTOUT(tmp, mask);
	}

	return AE_OK;
}

/*
 * pcc_message --
 *
 *	Send or receive a command over a PCC subspace.
 */
ACPI_STATUS
pcc_message(ACPI_GENERIC_ADDRESS *reg, uint32_t command, int flags,
    ACPI_INTEGER *val)
{
	struct pcc_softc * const sc = pcc_softc;
	struct pcc_subspace *ss;
	uint8_t ss_id;
	ACPI_STATUS rv;

	/* The "Access Width" in the PCC GAS is the Subspace ID */
	ss_id = reg->AccessWidth;

	if (sc == NULL) {
		return AE_NOT_CONFIGURED;
	}
	if (ss_id >= sc->sc_nss) {
		return AE_NOT_FOUND;
	}

	ss = &sc->sc_ss[ss_id];
	switch (ss->ss_type) {
	case ACPI_PCCT_TYPE_GENERIC_SUBSPACE:
	case ACPI_PCCT_TYPE_HW_REDUCED_SUBSPACE:
	case ACPI_PCCT_TYPE_HW_REDUCED_SUBSPACE_TYPE2:
		break;
	default:
		return AE_NOT_IMPLEMENTED;
	}

	mutex_enter(&ss->ss_lock);

	/* Wait for any previous commands to finish. */
	rv = pcc_wait_command(ss, true);
	if (ACPI_FAILURE(rv)) {
		goto unlock;
	}

	/* Place command into subspace and ring the doorbell */
	rv = pcc_send_command(ss, reg, command, flags, *val);
	if (ACPI_FAILURE(rv)) {
		goto unlock;
	}

	/* Wait for command to complete. */
	rv = pcc_wait_command(ss, false);
	if (ACPI_FAILURE(rv)) {
		goto unlock;
	}

	/* Process the command response */
	rv = pcc_receive_response(ss, reg, flags, val);

	if (ss->ss_turnaround != 0)
		delay(ss->ss_turnaround);

unlock:
	mutex_exit(&ss->ss_lock);

	return rv;
}
