/* $NetBSD: qcomsmem.c,v 1.1 2024/12/30 12:31:10 jmcneill Exp $ */
/*	$OpenBSD: qcsmem.c,v 1.1 2023/05/19 21:13:49 patrick Exp $	*/
/*
 * Copyright (c) 2023 Patrick Wildt <patrick@blueri.se>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <dev/acpi/acpivar.h>
#include <dev/acpi/qcomsmem.h>

#define QCSMEM_ITEM_FIXED	8
#define QCSMEM_ITEM_COUNT	512
#define QCSMEM_HOST_COUNT	15

struct qcsmem_proc_comm {
	uint32_t command;
	uint32_t status;
	uint32_t params[2];
};

struct qcsmem_global_entry {
	uint32_t allocated;
	uint32_t offset;
	uint32_t size;
	uint32_t aux_base;
#define QCSMEM_GLOBAL_ENTRY_AUX_BASE_MASK	0xfffffffc
};

struct qcsmem_header {
	struct qcsmem_proc_comm proc_comm[4];
	uint32_t version[32];
#define QCSMEM_HEADER_VERSION_MASTER_SBL_IDX	7
#define QCSMEM_HEADER_VERSION_GLOBAL_HEAP	11
#define QCSMEM_HEADER_VERSION_GLOBAL_PART	12
	uint32_t initialized;
	uint32_t free_offset;
	uint32_t available;
	uint32_t reserved;
	struct qcsmem_global_entry toc[QCSMEM_ITEM_COUNT];
};

struct qcsmem_ptable_entry {
	uint32_t offset;
	uint32_t size;
	uint32_t flags;
	uint16_t host[2];
#define QCSMEM_LOCAL_HOST			0
#define QCSMEM_GLOBAL_HOST			0xfffe
	uint32_t cacheline;
	uint32_t reserved[7];
};

struct qcsmem_ptable {
	uint32_t magic;
#define QCSMEM_PTABLE_MAGIC	0x434f5424
	uint32_t version;
#define QCSMEM_PTABLE_VERSION	1
	uint32_t num_entries;
	uint32_t reserved[5];
	struct qcsmem_ptable_entry entry[];
};

struct qcsmem_partition_header {
	uint32_t magic;
#define QCSMEM_PART_HDR_MAGIC	0x54525024
	uint16_t host[2];
	uint32_t size;
	uint32_t offset_free_uncached;
	uint32_t offset_free_cached;
	uint32_t reserved[3];
};

struct qcsmem_partition {
	struct qcsmem_partition_header *phdr;
	size_t cacheline;
	size_t size;
};

struct qcsmem_private_entry {
	uint16_t canary;
#define QCSMEM_PRIV_ENTRY_CANARY	0xa5a5
	uint16_t item;
	uint32_t size;
	uint16_t padding_data;
	uint16_t padding_hdr;
	uint32_t reserved;
};

struct qcsmem_info {
	uint32_t magic;
#define QCSMEM_INFO_MAGIC	0x49494953
	uint32_t size;
	uint32_t base_addr;
	uint32_t reserved;
	uint32_t num_items;
};

struct qcsmem_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	void			*sc_smem;
	bus_space_handle_t	sc_mtx_ioh;

	bus_addr_t		sc_aux_base;
	bus_size_t		sc_aux_size;

	int			sc_item_count;
	struct qcsmem_partition	sc_global_partition;
	struct qcsmem_partition	sc_partitions[QCSMEM_HOST_COUNT];
};

#define QCMTX_OFF(idx)		((idx) * 0x1000)
#define QCMTX_NUM_LOCKS		32
#define QCMTX_APPS_PROC_ID	1

#define MTXREAD4(sc, reg)						\
	bus_space_read_4((sc)->sc_iot, (sc)->sc_mtx_ioh, (reg))
#define MTXWRITE4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_mtx_ioh, (reg), (val))

struct qcsmem_softc *qcsmem_sc;

#define QCSMEM_X1E_BASE		0xffe00000
#define QCSMEM_X1E_SIZE		0x200000

#define QCMTX_X1E_BASE		0x01f40000
#define QCMTX_X1E_SIZE		0x20000

#define QCSMEM_X1E_LOCK_IDX	3

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "QCOM0C84" },
	DEVICE_COMPAT_EOL
};

static int	qcsmem_match(device_t, cfdata_t, void *);
static void	qcsmem_attach(device_t, device_t, void *);
static int	qcmtx_lock(struct qcsmem_softc *, u_int, u_int);
static void	qcmtx_unlock(struct qcsmem_softc *, u_int);

CFATTACH_DECL_NEW(qcomsmem, sizeof(struct qcsmem_softc),
    qcsmem_match, qcsmem_attach, NULL, NULL);

static int
qcsmem_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	return acpi_compatible_match(aa, compat_data);
}

static void
qcsmem_attach(device_t parent, device_t self, void *aux)
{
	struct qcsmem_softc *sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	struct qcsmem_header *header;
	struct qcsmem_ptable *ptable;
	struct qcsmem_ptable_entry *pte;
	struct qcsmem_info *info;
	struct qcsmem_partition *part;
	struct qcsmem_partition_header *phdr;
	uintptr_t smem_va;
	uint32_t hdr_version;
	int i;

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_memt;
	sc->sc_smem = AcpiOsMapMemory(QCSMEM_X1E_BASE, QCSMEM_X1E_SIZE);
	KASSERT(sc->sc_smem != NULL);

	sc->sc_aux_base = QCSMEM_X1E_BASE;
	sc->sc_aux_size = QCSMEM_X1E_SIZE;

	if (bus_space_map(sc->sc_iot, QCMTX_X1E_BASE,
	    QCMTX_X1E_SIZE, 0, &sc->sc_mtx_ioh)) {
		aprint_error(": can't map mutex registers\n");
		return;
	}

	smem_va = (uintptr_t)sc->sc_smem;

	ptable = (void *)(smem_va + sc->sc_aux_size - PAGE_SIZE);
	if (ptable->magic != QCSMEM_PTABLE_MAGIC ||
	    ptable->version != QCSMEM_PTABLE_VERSION) {
		aprint_error(": unsupported ptable 0x%x/0x%x\n",
		    ptable->magic, ptable->version);
		return;
	}

	header = (void *)smem_va;
	hdr_version = header->version[QCSMEM_HEADER_VERSION_MASTER_SBL_IDX] >> 16;
	if (hdr_version != QCSMEM_HEADER_VERSION_GLOBAL_PART) {
		aprint_error(": unsupported header 0x%x\n", hdr_version);
		return;
	}

	for (i = 0; i < ptable->num_entries; i++) {
		pte = &ptable->entry[i];
		if (!pte->offset || !pte->size)
			continue;
		if (pte->host[0] == QCSMEM_GLOBAL_HOST &&
		    pte->host[1] == QCSMEM_GLOBAL_HOST)
			part = &sc->sc_global_partition;
		else if (pte->host[0] == QCSMEM_LOCAL_HOST &&
		    pte->host[1] < QCSMEM_HOST_COUNT)
			part = &sc->sc_partitions[pte->host[1]];
		else if (pte->host[1] == QCSMEM_LOCAL_HOST &&
		    pte->host[0] < QCSMEM_HOST_COUNT)
			part = &sc->sc_partitions[pte->host[0]];
		else
			continue;
		if (part->phdr != NULL)
			continue;
		phdr = (void *)(smem_va + pte->offset);
		if (phdr->magic != QCSMEM_PART_HDR_MAGIC) {
			aprint_error(": unsupported partition 0x%x\n",
			    phdr->magic);
			return;
		}
		if (pte->host[0] != phdr->host[0] ||
		    pte->host[1] != phdr->host[1]) {
			aprint_error(": bad hosts 0x%x/0x%x+0x%x/0x%x\n",
			    pte->host[0], phdr->host[0],
			    pte->host[1], phdr->host[1]);
			return;
		}
		if (pte->size != phdr->size) {
			aprint_error(": bad size 0x%x/0x%x\n",
			    pte->size, phdr->size);
			return;
		}
		if (phdr->offset_free_uncached > phdr->size) {
			aprint_error(": bad size 0x%x > 0x%x\n",
			    phdr->offset_free_uncached, phdr->size);
			return;
		}
		part->phdr = phdr;
		part->size = pte->size;
		part->cacheline = pte->cacheline;
	}
	if (sc->sc_global_partition.phdr == NULL) {
		aprint_error(": could not find global partition\n");
		return;
	}

	sc->sc_item_count = QCSMEM_ITEM_COUNT;
	info = (struct qcsmem_info *)&ptable->entry[ptable->num_entries];
	if (info->magic == QCSMEM_INFO_MAGIC)
		sc->sc_item_count = info->num_items;

	aprint_naive("\n");
	aprint_normal("\n");

	qcsmem_sc = sc;
}

static int
qcsmem_alloc_private(struct qcsmem_softc *sc, struct qcsmem_partition *part,
    int item, int size)
{
	struct qcsmem_private_entry *entry, *last;
	struct qcsmem_partition_header *phdr = part->phdr;
	uintptr_t phdr_va = (uintptr_t)phdr;

	entry = (void *)&phdr[1];
	last = (void *)(phdr_va + phdr->offset_free_uncached);

	if ((void *)last > (void *)(phdr_va + part->size))
		return EINVAL;

	while (entry < last) {
		if (entry->canary != QCSMEM_PRIV_ENTRY_CANARY) {
			device_printf(sc->sc_dev, "invalid canary\n");
			return EINVAL;
		}

		if (entry->item == item)
			return 0;

		entry = (void *)((uintptr_t)&entry[1] + entry->padding_hdr +
		    entry->size);
	}

	if ((void *)entry > (void *)(phdr_va + part->size))
		return EINVAL;

	if ((uintptr_t)&entry[1] + roundup(size, 8) >
	    phdr_va + phdr->offset_free_cached)
		return EINVAL;

	entry->canary = QCSMEM_PRIV_ENTRY_CANARY;
	entry->item = item;
	entry->size = roundup(size, 8);
	entry->padding_data = entry->size - size;
	entry->padding_hdr = 0;
	membar_producer();

	phdr->offset_free_uncached += sizeof(*entry) + entry->size;

	return 0;
}

static int
qcsmem_alloc_global(struct qcsmem_softc *sc, int item, int size)
{
	struct qcsmem_header *header;
	struct qcsmem_global_entry *entry;

	header = (void *)sc->sc_smem;
	entry = &header->toc[item];
	if (entry->allocated)
		return 0;

	size = roundup(size, 8);
	if (size > header->available)
		return EINVAL;

	entry->offset = header->free_offset;
	entry->size = size;
	membar_producer();
	entry->allocated = 1;

	header->free_offset += size;
	header->available -= size;

	return 0;
}

int
qcsmem_alloc(int host, int item, int size)
{
	struct qcsmem_softc *sc = qcsmem_sc;
	struct qcsmem_partition *part;
	int ret;

	if (sc == NULL)
		return ENXIO;

	if (item < QCSMEM_ITEM_FIXED)
		return EPERM;

	if (item >= sc->sc_item_count)
		return ENXIO;

	ret = qcmtx_lock(sc, QCSMEM_X1E_LOCK_IDX, 1000);
	if (ret)
		return ret;

	if (host < QCSMEM_HOST_COUNT &&
	    sc->sc_partitions[host].phdr != NULL) {
		part = &sc->sc_partitions[host];
		ret = qcsmem_alloc_private(sc, part, item, size);
	} else if (sc->sc_global_partition.phdr != NULL) {
		part = &sc->sc_global_partition;
		ret = qcsmem_alloc_private(sc, part, item, size);
	} else {
		ret = qcsmem_alloc_global(sc, item, size);
	}

	qcmtx_unlock(sc, QCSMEM_X1E_LOCK_IDX);

	return ret;
}

static void *
qcsmem_get_private(struct qcsmem_softc *sc, struct qcsmem_partition *part,
    int item, int *size)
{
	struct qcsmem_private_entry *entry, *last;
	struct qcsmem_partition_header *phdr = part->phdr;
	uintptr_t phdr_va = (uintptr_t)phdr;

	entry = (void *)&phdr[1];
	last = (void *)(phdr_va + phdr->offset_free_uncached);

	while (entry < last) {
		if (entry->canary != QCSMEM_PRIV_ENTRY_CANARY) {
			device_printf(sc->sc_dev, "invalid canary\n");
			return NULL;
		}

		if (entry->item == item) {
			if (size != NULL) {
				if (entry->size > part->size ||
				    entry->padding_data > entry->size)
					return NULL;
				*size = entry->size - entry->padding_data;
			}

			return (void *)((uintptr_t)&entry[1] + entry->padding_hdr);
		}

		entry = (void *)((uintptr_t)&entry[1] + entry->padding_hdr +
		    entry->size);
	}

	if ((uintptr_t)entry > phdr_va + part->size)
		return NULL;

	entry = (void *)(phdr_va + phdr->size -
	    roundup(sizeof(*entry), part->cacheline));
	last = (void *)(phdr_va + phdr->offset_free_cached);

	if ((uintptr_t)entry < phdr_va ||
	    (uintptr_t)last > phdr_va + part->size)
		return NULL;

	while (entry > last) {
		if (entry->canary != QCSMEM_PRIV_ENTRY_CANARY) {
			device_printf(sc->sc_dev, "invalid canary\n");
			return NULL;
		}

		if (entry->item == item) {
			if (size != NULL) {
				if (entry->size > part->size ||
				    entry->padding_data > entry->size)
					return NULL;
				*size = entry->size - entry->padding_data;
			}

			return (void *)((uintptr_t)entry - entry->size);
		}

		entry = (void *)((uintptr_t)entry - entry->size -
		    roundup(sizeof(*entry), part->cacheline));
	}

	if ((uintptr_t)entry < phdr_va)
		return NULL;

	return NULL;
}

static void *
qcsmem_get_global(struct qcsmem_softc *sc, int item, int *size)
{
	struct qcsmem_header *header;
	struct qcsmem_global_entry *entry;
	uint32_t aux_base;

	header = (void *)sc->sc_smem;
	entry = &header->toc[item];
	if (!entry->allocated)
		return NULL;

	aux_base = entry->aux_base & QCSMEM_GLOBAL_ENTRY_AUX_BASE_MASK;
	if (aux_base != 0 && aux_base != sc->sc_aux_base)
		return NULL;

	if (entry->size + entry->offset > sc->sc_aux_size)
		return NULL;

	if (size != NULL)
		*size = entry->size;

	return (void *)((uintptr_t)sc->sc_smem +
	    entry->offset);
}

void *
qcsmem_get(int host, int item, int *size)
{
	struct qcsmem_softc *sc = qcsmem_sc;
	struct qcsmem_partition *part;
	void *p = NULL;
	int ret;

	if (sc == NULL)
		return NULL;

	if (item >= sc->sc_item_count)
		return NULL;

	ret = qcmtx_lock(sc, QCSMEM_X1E_LOCK_IDX, 1000);
	if (ret)
		return NULL;

	if (host >= 0 &&
	    host < QCSMEM_HOST_COUNT &&
	    sc->sc_partitions[host].phdr != NULL) {
		part = &sc->sc_partitions[host];
		p = qcsmem_get_private(sc, part, item, size);
	} else if (sc->sc_global_partition.phdr != NULL) {
		part = &sc->sc_global_partition;
		p = qcsmem_get_private(sc, part, item, size);
	} else {
		p = qcsmem_get_global(sc, item, size);
	}

	qcmtx_unlock(sc, QCSMEM_X1E_LOCK_IDX);
	return p;
}

void
qcsmem_memset(void *ptr, uint8_t val, size_t len)
{
	if (len % 8 == 0 && val == 0) {
		volatile uint64_t *p = ptr;
		size_t n;

		for (n = 0; n < len; n += 8) {
			p[n] = val;
		}
	} else {
		volatile uint8_t *p = ptr;
		size_t n;

		for (n = 0; n < len; n++) {
			p[n] = val;
		}
	}
}

static int
qcmtx_dolockunlock(struct qcsmem_softc *sc, u_int idx, int lock)
{
	if (idx >= QCMTX_NUM_LOCKS)
		return ENXIO;

	if (lock) {
		MTXWRITE4(sc, QCMTX_OFF(idx), QCMTX_APPS_PROC_ID);
		if (MTXREAD4(sc, QCMTX_OFF(idx)) !=
		    QCMTX_APPS_PROC_ID)
			return EAGAIN;
		KASSERT(MTXREAD4(sc, QCMTX_OFF(idx)) == QCMTX_APPS_PROC_ID);
	} else {
		KASSERT(MTXREAD4(sc, QCMTX_OFF(idx)) == QCMTX_APPS_PROC_ID);
		MTXWRITE4(sc, QCMTX_OFF(idx), 0);
	}

	return 0;
}

static int
qcmtx_lock(struct qcsmem_softc *sc, u_int idx, u_int timeout_ms)
{
	int rv = EINVAL;
	u_int n;

	for (n = 0; n < timeout_ms; n++) {
		rv = qcmtx_dolockunlock(sc, idx, 1);
		if (rv != EAGAIN) {
			break;
		}
		delay(1000);
	}

	return rv;
}

static void
qcmtx_unlock(struct qcsmem_softc *sc, u_int idx)
{
	qcmtx_dolockunlock(sc, idx, 0);
}
