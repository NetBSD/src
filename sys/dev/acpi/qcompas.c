/* $NetBSD: qcompas.c,v 1.2 2026/02/03 08:45:53 skrll Exp $ */
/*	$OpenBSD: qcpas.c,v 1.8 2024/11/08 21:13:34 landry Exp $	*/
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
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/callout.h>
#include <sys/exec_elf.h>

#include <uvm/uvm_extern.h>

#include <dev/firmload.h>
#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_intr.h>
#include <dev/acpi/qcomipcc.h>
#include <dev/acpi/qcompep.h>
#include <dev/acpi/qcomscm.h>
#include <dev/acpi/qcomsmem.h>
#include <dev/acpi/qcomsmptp.h>

#define DRIVER_NAME		"qcompas"

#define MDT_TYPE_MASK				(7 << 24)
#define MDT_TYPE_HASH				(2 << 24)
#define MDT_RELOCATABLE				(1 << 27)

extern struct arm32_bus_dma_tag arm_generic_dma_tag;

enum qcpas_batt_sensor {
	/* Battery sensors (must be first) */
	QCPAS_DVOLTAGE,
	QCPAS_VOLTAGE,
	QCPAS_DCAPACITY,
	QCPAS_LFCCAPACITY,
	QCPAS_CAPACITY,
	QCPAS_CHARGERATE,
	QCPAS_DISCHARGERATE,
	QCPAS_CHARGING,
	QCPAS_CHARGE_STATE,
	QCPAS_DCYCLES,
	QCPAS_TEMPERATURE,
	/* AC adapter sensors */
	QCPAS_ACADAPTER,
	/* Total number of sensors */
	QCPAS_NUM_SENSORS
};

struct qcpas_dmamem {
	bus_dmamap_t		tdm_map;
	bus_dma_segment_t	tdm_seg;
	size_t			tdm_size;
	void			*tdm_kva;
};
#define QCPAS_DMA_MAP(_tdm)	((_tdm)->tdm_map)
#define QCPAS_DMA_LEN(_tdm)	((_tdm)->tdm_size)
#define QCPAS_DMA_DVA(_tdm)	((_tdm)->tdm_map->dm_segs[0].ds_addr)
#define QCPAS_DMA_KVA(_tdm)	((_tdm)->tdm_kva)

struct qcpas_softc {
	device_t		sc_dev;
	bus_dma_tag_t		sc_dmat;

	char			*sc_sub;

	void			*sc_ih[5];

	kmutex_t		sc_ready_lock;
	kcondvar_t		sc_ready_cv;
	bool			sc_ready;

	paddr_t			sc_mem_phys[2];
	size_t			sc_mem_size[2];
	uint8_t			*sc_mem_region[2];
	vaddr_t			sc_mem_reloc[2];

	const char		*sc_fwname;
	const char		*sc_dtb_fwname;
	uint32_t		sc_pas_id;
	uint32_t		sc_dtb_pas_id;
	uint32_t		sc_lite_pas_id;
	const char		*sc_load_state;
	uint32_t		sc_glink_remote_pid;
	uint32_t		sc_crash_reason;

	struct qcpas_dmamem	*sc_metadata[2];

	/* GLINK */
	volatile uint32_t	*sc_tx_tail;
	volatile uint32_t	*sc_tx_head;
	volatile uint32_t	*sc_rx_tail;
	volatile uint32_t	*sc_rx_head;

	uint32_t		sc_tx_off;
	uint32_t		sc_rx_off;

	uint8_t			*sc_tx_fifo;
	int			sc_tx_fifolen;
	uint8_t			*sc_rx_fifo;
	int			sc_rx_fifolen;
	void			*sc_glink_ih;

	void			*sc_ipcc;

	uint32_t		sc_glink_max_channel;
	TAILQ_HEAD(,qcpas_glink_channel) sc_glink_channels;

	uint32_t		sc_warning_capacity;
	uint32_t		sc_low_capacity;
	uint32_t		sc_power_state;
	struct sysmon_envsys	*sc_sme;
	envsys_data_t		sc_sens[QCPAS_NUM_SENSORS];
	struct sysmon_envsys	*sc_sme_acadapter;
	struct sysmon_pswitch	sc_smpsw_acadapter;
	callout_t		sc_rtr_refresh;
};

static int	qcpas_match(device_t, cfdata_t, void *);
static void	qcpas_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(qcompas, sizeof(struct qcpas_softc),
    qcpas_match, qcpas_attach, NULL, NULL);

static void	qcpas_mountroot(device_t);
static void	qcpas_firmload(void *);
static int	qcpas_map_memory(struct qcpas_softc *);
static int	qcpas_mdt_init(struct qcpas_softc *, int, u_char *, size_t);
static void	qcpas_glink_attach(struct qcpas_softc *);
static void	qcpas_glink_recv(void *);
static void	qcpas_get_limits(struct sysmon_envsys *, envsys_data_t *,
				 sysmon_envsys_lim_t *, uint32_t *);

static struct qcpas_dmamem *
		qcpas_dmamem_alloc(struct qcpas_softc *, bus_size_t, bus_size_t);
static void	qcpas_dmamem_free(struct qcpas_softc *, struct qcpas_dmamem *);

static int	qcpas_intr_wdog(void *);
static int	qcpas_intr_fatal(void *);
static int	qcpas_intr_ready(void *);
static int	qcpas_intr_handover(void *);
static int	qcpas_intr_stop_ack(void *);

struct qcpas_mem_region {
	bus_addr_t		start;
	bus_size_t		size;
};

struct qcpas_data {
	bus_addr_t		reg_addr;
	bus_size_t		reg_size;
	uint32_t		pas_id;
	uint32_t		dtb_pas_id;
	uint32_t		lite_pas_id;
	const char		*load_state;
	uint32_t		glink_remote_pid;
	struct qcpas_mem_region	mem_region[2];
	const char		*fwname;
	const char		*dtb_fwname;
	uint32_t		crash_reason;
};

static struct qcpas_data qcpas_x1e_data = {
	.reg_addr = 0x30000000,
	.reg_size = 0x100,
	.pas_id = 1,
	.dtb_pas_id = 36,
	.lite_pas_id = 31,
	.load_state = "adsp",
	.glink_remote_pid = 2,
	.mem_region = {
		[0] = { .start = 0x87e00000, .size = 0x3a00000 },
		[1] = { .start = 0x8b800000, .size = 0x80000 },
	},
	.fwname = "qcadsp8380.mbn",
	.dtb_fwname = "adsp_dtbs.elf",
	.crash_reason = 423,
};

#define IPCC_CLIENT_LPASS       	3
#define IPCC_MPROC_SIGNAL_GLINK_QMP	0

static const struct device_compatible_entry compat_data[] = {
        { .compat = "QCOM0C1B",         .data = &qcpas_x1e_data },
        DEVICE_COMPAT_EOL
};

static int
qcpas_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	return acpi_compatible_match(aa, compat_data);
}

static void
qcpas_attach(device_t parent, device_t self, void *aux)
{
	struct qcpas_softc *sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	const struct qcpas_data *data;
	struct acpi_resources res;
	ACPI_STATUS rv;
	int i;

	rv = acpi_resource_parse(self, aa->aa_node->ad_handle, "_CRS", &res,
	    &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv)) {
		return;
	}
	acpi_resource_cleanup(&res);

	data = acpi_compatible_lookup(aa, compat_data)->data;

	sc->sc_dev = self;
	sc->sc_dmat = &arm_generic_dma_tag;
	mutex_init(&sc->sc_ready_lock, MUTEX_DEFAULT, IPL_VM);
	cv_init(&sc->sc_ready_cv, "qcpasrdy");

	sc->sc_fwname = data->fwname;
	sc->sc_dtb_fwname = data->dtb_fwname;
	sc->sc_pas_id = data->pas_id;
	sc->sc_dtb_pas_id = data->dtb_pas_id;
	sc->sc_lite_pas_id = data->lite_pas_id;
	sc->sc_load_state = data->load_state;
	sc->sc_glink_remote_pid = data->glink_remote_pid;
	sc->sc_crash_reason = data->crash_reason;
	for (i = 0; i < __arraycount(sc->sc_mem_phys); i++) {
		sc->sc_mem_phys[i] = data->mem_region[i].start;
		KASSERT((sc->sc_mem_phys[i] & PAGE_MASK) == 0);
		sc->sc_mem_size[i] = data->mem_region[i].size;
		KASSERT((sc->sc_mem_size[i] & PAGE_MASK) == 0);
	}

	rv = acpi_eval_string(aa->aa_node->ad_handle, "_SUB", &sc->sc_sub);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(self, "failed to evaluate _SUB: %s\n",
		    AcpiFormatException(rv));
		return;
	}
	aprint_verbose_dev(self, "subsystem ID %s\n", sc->sc_sub);

	sc->sc_ih[0] = acpi_intr_establish(self,
	    (uint64_t)(uintptr_t)aa->aa_node->ad_handle,
	    IPL_VM, false, qcpas_intr_wdog, sc, device_xname(self));
	sc->sc_ih[1] =
	    qcsmptp_intr_establish(0, qcpas_intr_fatal, sc);
	sc->sc_ih[2] =
	    qcsmptp_intr_establish(1, qcpas_intr_ready, sc);
	sc->sc_ih[3] =
	    qcsmptp_intr_establish(2, qcpas_intr_handover, sc);
	sc->sc_ih[4] =
	    qcsmptp_intr_establish(3, qcpas_intr_stop_ack, sc);

	if (qcpas_map_memory(sc) != 0)
		return;

	config_mountroot(self, qcpas_mountroot);
}

static void
qcpas_firmload(void *arg)
{
	struct qcpas_softc *sc = arg;
	firmware_handle_t fwh = NULL, dtb_fwh = NULL;
	char fwname[128];
	size_t fwlen = 0, dtb_fwlen = 0;
	u_char *fw = NULL, *dtb_fw = NULL;
	int ret, error;

	snprintf(fwname, sizeof(fwname), "%s/%s", sc->sc_sub, sc->sc_fwname);
	error = firmware_open(DRIVER_NAME, fwname, &fwh);
	if (error == 0) {
		fwlen = firmware_get_size(fwh);
		fw = fwlen ? firmware_malloc(fwlen) : NULL;
		error = fw == NULL ? ENOMEM :
			firmware_read(fwh, 0, fw, fwlen);
	}
	if (error) {
		device_printf(sc->sc_dev, "failed to load %s/%s: %d\n",
		    DRIVER_NAME, fwname, error);
		goto cleanup;
	}
	aprint_normal_dev(sc->sc_dev, "loading %s/%s\n", DRIVER_NAME, fwname);

	if (sc->sc_lite_pas_id) {
		if (qcscm_pas_shutdown(sc->sc_lite_pas_id)) {
			device_printf(sc->sc_dev,
			    "failed to shutdown lite firmware\n");
		}
	}

	if (sc->sc_dtb_pas_id) {
		snprintf(fwname, sizeof(fwname), "%s/%s", sc->sc_sub,
		    sc->sc_dtb_fwname);
		error = firmware_open(DRIVER_NAME, fwname, &dtb_fwh);
		if (error == 0) {
			dtb_fwlen = firmware_get_size(dtb_fwh);
			dtb_fw = dtb_fwlen ? firmware_malloc(dtb_fwlen) : NULL;
			error = dtb_fw == NULL ? ENOMEM :
				firmware_read(dtb_fwh, 0, dtb_fw, dtb_fwlen);
		}
		if (error) {
			device_printf(sc->sc_dev, "failed to load %s/%s: %d\n",
			    DRIVER_NAME, fwname, error);
			goto cleanup;
		}
		aprint_normal_dev(sc->sc_dev, "loading %s/%s\n", DRIVER_NAME, fwname);
	}

	if (sc->sc_load_state) {
		char buf[64];
		snprintf(buf, sizeof(buf),
		    "{class: image, res: load_state, name: %s, val: on}",
		    sc->sc_load_state);
		ret = qcaoss_send(buf, sizeof(buf));
		if (ret != 0) {
			device_printf(sc->sc_dev, "failed to toggle load state\n");
			goto cleanup;
		}
	}

	if (sc->sc_dtb_pas_id) {
		qcpas_mdt_init(sc, sc->sc_dtb_pas_id, dtb_fw, dtb_fwlen);
	}

	ret = qcpas_mdt_init(sc, sc->sc_pas_id, fw, fwlen);
	if (ret != 0) {
		device_printf(sc->sc_dev, "failed to boot coprocessor\n");
		goto cleanup;
	}

	qcpas_glink_attach(sc);

	/* Battery sensors */
	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_name = "battery";
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_flags = SME_DISABLE_REFRESH;
	sc->sc_sme->sme_class = SME_CLASS_BATTERY;
	sc->sc_sme->sme_get_limits = qcpas_get_limits;

	/* AC adapter sensors */
	sc->sc_sme_acadapter = sysmon_envsys_create();
	sc->sc_sme_acadapter->sme_name = "charger";
	sc->sc_sme_acadapter->sme_cookie = sc;
	sc->sc_sme_acadapter->sme_flags = SME_DISABLE_REFRESH;
	sc->sc_sme_acadapter->sme_class = SME_CLASS_ACADAPTER;

#define INIT_SENSOR(sme, idx, unit, str)				\
	do {								\
		strlcpy(sc->sc_sens[idx].desc, str,			\
		    sizeof(sc->sc_sens[0].desc));			\
		sc->sc_sens[idx].units = unit;				\
		sc->sc_sens[idx].state = ENVSYS_SINVALID;		\
		sysmon_envsys_sensor_attach(sme,			\
		    &sc->sc_sens[idx]);					\
	} while (0)

	INIT_SENSOR(sc->sc_sme, QCPAS_DVOLTAGE, ENVSYS_SVOLTS_DC, "design voltage");
	INIT_SENSOR(sc->sc_sme, QCPAS_VOLTAGE, ENVSYS_SVOLTS_DC, "voltage");
	INIT_SENSOR(sc->sc_sme, QCPAS_DCAPACITY, ENVSYS_SWATTHOUR, "design cap");
	INIT_SENSOR(sc->sc_sme, QCPAS_LFCCAPACITY, ENVSYS_SWATTHOUR, "last full cap");
	INIT_SENSOR(sc->sc_sme, QCPAS_CAPACITY, ENVSYS_SWATTHOUR, "charge");
	INIT_SENSOR(sc->sc_sme, QCPAS_CHARGERATE, ENVSYS_SWATTS, "charge rate");
	INIT_SENSOR(sc->sc_sme, QCPAS_DISCHARGERATE, ENVSYS_SWATTS, "discharge rate");
	INIT_SENSOR(sc->sc_sme, QCPAS_CHARGING, ENVSYS_BATTERY_CHARGE, "charging");
	INIT_SENSOR(sc->sc_sme, QCPAS_CHARGE_STATE, ENVSYS_BATTERY_CAPACITY, "charge state");
	INIT_SENSOR(sc->sc_sme, QCPAS_DCYCLES, ENVSYS_INTEGER, "discharge cycles");
	INIT_SENSOR(sc->sc_sme, QCPAS_TEMPERATURE, ENVSYS_STEMP, "temperature");
	INIT_SENSOR(sc->sc_sme_acadapter, QCPAS_ACADAPTER, ENVSYS_INDICATOR, "connected");

#undef INIT_SENSOR

	sc->sc_sens[QCPAS_CHARGE_STATE].value_cur =
	    ENVSYS_BATTERY_CAPACITY_NORMAL;
	sc->sc_sens[QCPAS_CAPACITY].flags |=
	    ENVSYS_FPERCENT | ENVSYS_FVALID_MAX | ENVSYS_FMONLIMITS;
	sc->sc_sens[QCPAS_CHARGE_STATE].flags |=
	    ENVSYS_FMONSTCHANGED;

	sc->sc_sens[QCPAS_VOLTAGE].flags = ENVSYS_FMONNOTSUPP;
	sc->sc_sens[QCPAS_CHARGERATE].flags = ENVSYS_FMONNOTSUPP;
	sc->sc_sens[QCPAS_DISCHARGERATE].flags = ENVSYS_FMONNOTSUPP;
	sc->sc_sens[QCPAS_DCAPACITY].flags = ENVSYS_FMONNOTSUPP;
	sc->sc_sens[QCPAS_LFCCAPACITY].flags = ENVSYS_FMONNOTSUPP;
	sc->sc_sens[QCPAS_DVOLTAGE].flags = ENVSYS_FMONNOTSUPP;

	sc->sc_sens[QCPAS_CHARGERATE].flags |= ENVSYS_FHAS_ENTROPY;
	sc->sc_sens[QCPAS_DISCHARGERATE].flags |= ENVSYS_FHAS_ENTROPY;

	sysmon_envsys_register(sc->sc_sme);
	sysmon_envsys_register(sc->sc_sme_acadapter);

	sc->sc_smpsw_acadapter.smpsw_name = "acpiacad0";
	sc->sc_smpsw_acadapter.smpsw_type = PSWITCH_TYPE_ACADAPTER;
	sysmon_pswitch_register(&sc->sc_smpsw_acadapter);

cleanup:
	if (dtb_fw != NULL) {
		firmware_free(dtb_fw, dtb_fwlen);
	}
	if (fw != NULL) {
		firmware_free(fw, fwlen);
	}
	if (dtb_fwh != NULL) {
		firmware_close(dtb_fwh);
	}
	if (fwh != NULL) {
		firmware_close(fwh);
	}
}

static void
qcpas_mountroot(device_t self)
{
	struct qcpas_softc *sc = device_private(self);

	sysmon_task_queue_sched(0, qcpas_firmload, sc);
}

static int
qcpas_map_memory(struct qcpas_softc *sc)
{
	int i;

	for (i = 0; i < __arraycount(sc->sc_mem_phys); i++) {
		paddr_t pa, epa;
		vaddr_t va;

		if (sc->sc_mem_size[i] == 0)
			break;

		va = uvm_km_alloc(kernel_map, sc->sc_mem_size[i], 0, UVM_KMF_VAONLY);
		KASSERT(va != 0);
		sc->sc_mem_region[i] = (void *)va;

		for (pa = sc->sc_mem_phys[i], epa = sc->sc_mem_phys[i] + sc->sc_mem_size[i];
		     pa < epa;
		     pa += PAGE_SIZE, va += PAGE_SIZE) {
			pmap_kenter_pa(va, pa, VM_PROT_READ|VM_PROT_WRITE, PMAP_WRITE_COMBINE);
		}
		pmap_update(pmap_kernel());
	}

	return 0;
}

static int
qcpas_mdt_init(struct qcpas_softc *sc, int pas_id, u_char *fw, size_t fwlen)
{
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;
	paddr_t minpa = -1, maxpa = 0;
	int i, hashseg = 0, relocate = 0;
	uint8_t *metadata;
	int error;
	ssize_t off;
	int idx;

	if (pas_id == sc->sc_dtb_pas_id)
		idx = 1;
	else
		idx = 0;

	ehdr = (Elf32_Ehdr *)fw;
	phdr = (Elf32_Phdr *)&ehdr[1];

	if (ehdr->e_phnum < 2 || phdr[0].p_type == PT_LOAD)
		return EINVAL;

	for (i = 0; i < ehdr->e_phnum; i++) {
		if ((phdr[i].p_flags & MDT_TYPE_MASK) == MDT_TYPE_HASH) {
			if (i > 0 && !hashseg)
				hashseg = i;
			continue;
		}
		if (phdr[i].p_type != PT_LOAD || phdr[i].p_memsz == 0)
			continue;
		if (phdr[i].p_flags & MDT_RELOCATABLE)
			relocate = 1;
		if (phdr[i].p_paddr < minpa)
			minpa = phdr[i].p_paddr;
		if (phdr[i].p_paddr + phdr[i].p_memsz > maxpa)
			maxpa =
			    roundup(phdr[i].p_paddr + phdr[i].p_memsz,
			    PAGE_SIZE);
	}

	if (!hashseg)
		return EINVAL;

	if (sc->sc_metadata[idx] == NULL) {
		sc->sc_metadata[idx] = qcpas_dmamem_alloc(sc, phdr[0].p_filesz +
		    phdr[hashseg].p_filesz, PAGE_SIZE);
		if (sc->sc_metadata[idx] == NULL) {
			return EINVAL;
		}
	}

	metadata = QCPAS_DMA_KVA(sc->sc_metadata[idx]);

	memcpy(metadata, fw, phdr[0].p_filesz);
	if (phdr[0].p_filesz + phdr[hashseg].p_filesz == fwlen) {
		memcpy(metadata + phdr[0].p_filesz,
		    fw + phdr[0].p_filesz, phdr[hashseg].p_filesz);
	} else if (phdr[hashseg].p_offset + phdr[hashseg].p_filesz <= fwlen) {
		memcpy(metadata + phdr[0].p_filesz,
		    fw + phdr[hashseg].p_offset, phdr[hashseg].p_filesz);
	} else {
		device_printf(sc->sc_dev, "metadata split segment not supported\n");
		return EINVAL;
	}

	cpu_drain_writebuf();

	error = qcscm_pas_init_image(pas_id,
	    QCPAS_DMA_DVA(sc->sc_metadata[idx]));
	if (error != 0) {
		device_printf(sc->sc_dev, "init image failed: %d\n", error);
		qcpas_dmamem_free(sc, sc->sc_metadata[idx]);
		return error;
	}

	if (relocate) {
		if (qcscm_pas_mem_setup(pas_id,
		    sc->sc_mem_phys[idx], maxpa - minpa) != 0) {
			device_printf(sc->sc_dev, "mem setup failed\n");
			qcpas_dmamem_free(sc, sc->sc_metadata[idx]);
			return EINVAL;
		}
	}

	sc->sc_mem_reloc[idx] = relocate ? minpa : sc->sc_mem_phys[idx];

	for (i = 0; i < ehdr->e_phnum; i++) {
		if ((phdr[i].p_flags & MDT_TYPE_MASK) == MDT_TYPE_HASH ||
		    phdr[i].p_type != PT_LOAD || phdr[i].p_memsz == 0)
			continue;
		off = phdr[i].p_paddr - sc->sc_mem_reloc[idx];
		if (off < 0 || off + phdr[i].p_memsz > sc->sc_mem_size[0])
			return EINVAL;
		if (phdr[i].p_filesz > phdr[i].p_memsz)
			return EINVAL;

		if (phdr[i].p_filesz && phdr[i].p_offset < fwlen &&
		    phdr[i].p_offset + phdr[i].p_filesz <= fwlen) {
			memcpy(sc->sc_mem_region[idx] + off,
			    fw + phdr[i].p_offset, phdr[i].p_filesz);
		} else if (phdr[i].p_filesz) {
			device_printf(sc->sc_dev, "firmware split segment not supported\n");
			return EINVAL;
		}

		if (phdr[i].p_memsz > phdr[i].p_filesz)
			memset(sc->sc_mem_region[idx] + off + phdr[i].p_filesz,
			    0, phdr[i].p_memsz - phdr[i].p_filesz);
	}

	cpu_drain_writebuf();

	if (qcscm_pas_auth_and_reset(pas_id) != 0) {
		device_printf(sc->sc_dev, "auth and reset failed\n");
		qcpas_dmamem_free(sc, sc->sc_metadata[idx]);
		return EINVAL;
	}

	if (pas_id == sc->sc_dtb_pas_id)
		return 0;

	mutex_enter(&sc->sc_ready_lock);
	while (!sc->sc_ready) {
		error = cv_timedwait(&sc->sc_ready_cv, &sc->sc_ready_lock,
		    hz * 5);
		if (error == EWOULDBLOCK) {
			break;
		}
	}
	mutex_exit(&sc->sc_ready_lock);
	if (!sc->sc_ready) {
		device_printf(sc->sc_dev, "timeout waiting for ready signal\n");
		return ETIMEDOUT;
	}

	/* XXX: free metadata ? */

	return 0;
}

static struct qcpas_dmamem *
qcpas_dmamem_alloc(struct qcpas_softc *sc, bus_size_t size, bus_size_t align)
{
	struct qcpas_dmamem *tdm;
	int nsegs;

	tdm = kmem_zalloc(sizeof(*tdm), KM_SLEEP);
	tdm->tdm_size = size;

	if (bus_dmamap_create(sc->sc_dmat, size, 1, size, 0,
	    BUS_DMA_WAITOK | BUS_DMA_ALLOCNOW, &tdm->tdm_map) != 0)
		goto tdmfree;

	if (bus_dmamem_alloc(sc->sc_dmat, size, align, 0,
	    &tdm->tdm_seg, 1, &nsegs, BUS_DMA_WAITOK) != 0)
		goto destroy;

	if (bus_dmamem_map(sc->sc_dmat, &tdm->tdm_seg, nsegs, size,
	    &tdm->tdm_kva, BUS_DMA_WAITOK | BUS_DMA_PREFETCHABLE) != 0)
		goto free;

	if (bus_dmamap_load(sc->sc_dmat, tdm->tdm_map, tdm->tdm_kva, size,
	    NULL, BUS_DMA_WAITOK) != 0)
		goto unmap;

	memset(tdm->tdm_kva, 0, size);

	return (tdm);

unmap:
	bus_dmamem_unmap(sc->sc_dmat, tdm->tdm_kva, size);
free:
	bus_dmamem_free(sc->sc_dmat, &tdm->tdm_seg, 1);
destroy:
	bus_dmamap_destroy(sc->sc_dmat, tdm->tdm_map);
tdmfree:
	kmem_free(tdm, sizeof(*tdm));

	return (NULL);
}

static void
qcpas_dmamem_free(struct qcpas_softc *sc, struct qcpas_dmamem *tdm)
{
	bus_dmamem_unmap(sc->sc_dmat, tdm->tdm_kva, tdm->tdm_size);
	bus_dmamem_free(sc->sc_dmat, &tdm->tdm_seg, 1);
	bus_dmamap_destroy(sc->sc_dmat, tdm->tdm_map);
	kmem_free(tdm, sizeof(*tdm));
}

static void
qcpas_report_crash(struct qcpas_softc *sc, const char *source)
{
	char *msg;
	int size;

	msg = qcsmem_get(-1, sc->sc_crash_reason, &size);
	if (msg == NULL || size <= 0) {
		device_printf(sc->sc_dev, "%s\n", source);
	} else {
		device_printf(sc->sc_dev, "%s: \"%s\"\n", source, msg);
	}
}

static int
qcpas_intr_wdog(void *cookie)
{
	struct qcpas_softc *sc = cookie;

	qcpas_report_crash(sc, "watchdog");

	return 0;
}

static int
qcpas_intr_fatal(void *cookie)
{
	struct qcpas_softc *sc = cookie;

	qcpas_report_crash(sc, "fatal error");

	return 0;
}

static int
qcpas_intr_ready(void *cookie)
{
	struct qcpas_softc *sc = cookie;

	aprint_debug_dev(sc->sc_dev, "%s\n", __func__);

	mutex_enter(&sc->sc_ready_lock);
	sc->sc_ready = true;
	cv_broadcast(&sc->sc_ready_cv);
	mutex_exit(&sc->sc_ready_lock);

	return 0;
}

static int
qcpas_intr_handover(void *cookie)
{
	struct qcpas_softc *sc = cookie;

	aprint_debug_dev(sc->sc_dev, "%s\n", __func__);

	return 0;
}

static int
qcpas_intr_stop_ack(void *cookie)
{
	struct qcpas_softc *sc = cookie;

	aprint_debug_dev(sc->sc_dev, "%s\n", __func__);

	return 0;
}

/* GLINK */

#define SMEM_GLINK_NATIVE_XPRT_DESCRIPTOR	478
#define SMEM_GLINK_NATIVE_XPRT_FIFO_0		479
#define SMEM_GLINK_NATIVE_XPRT_FIFO_1		480

struct glink_msg {
	uint16_t cmd;
	uint16_t param1;
	uint32_t param2;
	uint8_t data[];
} __packed;

struct qcpas_glink_intent_pair {
	uint32_t size;
	uint32_t iid;
} __packed;

struct qcpas_glink_intent {
	TAILQ_ENTRY(qcpas_glink_intent) it_q;
	uint32_t it_id;
	uint32_t it_size;
	int it_inuse;
};

struct qcpas_glink_channel {
	TAILQ_ENTRY(qcpas_glink_channel) ch_q;
	struct qcpas_softc *ch_sc;
	struct qcpas_glink_protocol *ch_proto;
	uint32_t ch_rcid;
	uint32_t ch_lcid;
	uint32_t ch_max_intent;
	TAILQ_HEAD(,qcpas_glink_intent) ch_l_intents;
	TAILQ_HEAD(,qcpas_glink_intent) ch_r_intents;
};

#define GLINK_CMD_VERSION		0
#define GLINK_CMD_VERSION_ACK		1
#define  GLINK_VERSION				1
#define  GLINK_FEATURE_INTENT_REUSE		(1 << 0)
#define GLINK_CMD_OPEN			2
#define GLINK_CMD_CLOSE			3
#define GLINK_CMD_OPEN_ACK		4
#define GLINK_CMD_INTENT		5
#define GLINK_CMD_RX_DONE		6
#define GLINK_CMD_RX_INTENT_REQ		7
#define GLINK_CMD_RX_INTENT_REQ_ACK	8
#define GLINK_CMD_TX_DATA		9
#define GLINK_CMD_CLOSE_ACK		11
#define GLINK_CMD_TX_DATA_CONT		12
#define GLINK_CMD_READ_NOTIF		13
#define GLINK_CMD_RX_DONE_W_REUSE	14

static int	qcpas_glink_intr(void *);

static void	qcpas_glink_tx(struct qcpas_softc *, uint8_t *, int);
static void	qcpas_glink_tx_commit(struct qcpas_softc *);
static void	qcpas_glink_rx(struct qcpas_softc *, uint8_t *, int);
static void	qcpas_glink_rx_commit(struct qcpas_softc *);

static void	qcpas_glink_send(void *, void *, int);

static int	qcpas_pmic_rtr_init(void *);
static int	qcpas_pmic_rtr_recv(void *, uint8_t *, int);

struct qcpas_glink_protocol {
	const char *name;
	int (*init)(void *cookie);
	int (*recv)(void *cookie, uint8_t *buf, int len);
} qcpas_glink_protocols[] = {
	{ "PMIC_RTR_ADSP_APPS", qcpas_pmic_rtr_init , qcpas_pmic_rtr_recv },
};

static void
qcpas_glink_attach(struct qcpas_softc *sc)
{
	uint32_t remote = sc->sc_glink_remote_pid;
	uint32_t *descs;
	int size;

	if (qcsmem_alloc(remote, SMEM_GLINK_NATIVE_XPRT_DESCRIPTOR, 32) != 0 ||
	    qcsmem_alloc(remote, SMEM_GLINK_NATIVE_XPRT_FIFO_0, 16384) != 0)
		return;

	descs = qcsmem_get(remote, SMEM_GLINK_NATIVE_XPRT_DESCRIPTOR, &size);
	if (descs == NULL || size != 32)
		return;

	sc->sc_tx_tail = &descs[0];
	sc->sc_tx_head = &descs[1];
	sc->sc_rx_tail = &descs[2];
	sc->sc_rx_head = &descs[3];

	sc->sc_tx_fifo = qcsmem_get(remote, SMEM_GLINK_NATIVE_XPRT_FIFO_0,
	    &sc->sc_tx_fifolen);
	if (sc->sc_tx_fifo == NULL)
		return;
	sc->sc_rx_fifo = qcsmem_get(remote, SMEM_GLINK_NATIVE_XPRT_FIFO_1,
	    &sc->sc_rx_fifolen);
	if (sc->sc_rx_fifo == NULL)
		return;

	sc->sc_ipcc = qcipcc_channel(IPCC_CLIENT_LPASS,
	    IPCC_MPROC_SIGNAL_GLINK_QMP);
	if (sc->sc_ipcc == NULL)
		return;

	TAILQ_INIT(&sc->sc_glink_channels);

	sc->sc_glink_ih = qcipcc_intr_establish(IPCC_CLIENT_LPASS,
	    IPCC_MPROC_SIGNAL_GLINK_QMP, IPL_VM, qcpas_glink_intr, sc);
	if (sc->sc_glink_ih == NULL)
		return;

	/* Expect peer to send initial message */
}

static void
qcpas_glink_rx(struct qcpas_softc *sc, uint8_t *buf, int len)
{
	uint32_t head, tail;
	int avail;

	head = *sc->sc_rx_head;
	tail = *sc->sc_rx_tail + sc->sc_rx_off;
	if (tail >= sc->sc_rx_fifolen)
		tail -= sc->sc_rx_fifolen;

	/* Checked by caller */
	KASSERT(head != tail);

	if (head >= tail)
		avail = head - tail;
	else
		avail = (sc->sc_rx_fifolen - tail) + head;

	/* Dumb, but should do. */
	KASSERT(avail >= len);

	while (len > 0) {
		*buf = sc->sc_rx_fifo[tail];
		tail++;
		if (tail >= sc->sc_rx_fifolen)
			tail -= sc->sc_rx_fifolen;
		buf++;
		sc->sc_rx_off++;
		len--;
	}
}

static void
qcpas_glink_rx_commit(struct qcpas_softc *sc)
{
	uint32_t tail;

	tail = *sc->sc_rx_tail + roundup(sc->sc_rx_off, 8);
	if (tail >= sc->sc_rx_fifolen)
		tail -= sc->sc_rx_fifolen;

	membar_producer();
	*sc->sc_rx_tail = tail;
	sc->sc_rx_off = 0;
}

static void
qcpas_glink_tx(struct qcpas_softc *sc, uint8_t *buf, int len)
{
	uint32_t head, tail;
	int avail;

	head = *sc->sc_tx_head + sc->sc_tx_off;
	if (head >= sc->sc_tx_fifolen)
		head -= sc->sc_tx_fifolen;
	tail = *sc->sc_tx_tail;

	if (head < tail)
		avail = tail - head;
	else
		avail = (sc->sc_rx_fifolen - head) + tail;

	/* Dumb, but should do. */
	KASSERT(avail >= len);

	while (len > 0) {
		sc->sc_tx_fifo[head] = *buf;
		head++;
		if (head >= sc->sc_tx_fifolen)
			head -= sc->sc_tx_fifolen;
		buf++;
		sc->sc_tx_off++;
		len--;
	}
}

static void
qcpas_glink_tx_commit(struct qcpas_softc *sc)
{
	uint32_t head;

	head = *sc->sc_tx_head + roundup(sc->sc_tx_off, 8);
	if (head >= sc->sc_tx_fifolen)
		head -= sc->sc_tx_fifolen;

	membar_producer();
	*sc->sc_tx_head = head;
	sc->sc_tx_off = 0;
	qcipcc_send(sc->sc_ipcc);
}

static void
qcpas_glink_send(void *cookie, void *buf, int len)
{
	struct qcpas_glink_channel *ch = cookie;
	struct qcpas_softc *sc = ch->ch_sc;
	struct qcpas_glink_intent *it;
	struct glink_msg msg;
	uint32_t chunk_size, left_size;

	TAILQ_FOREACH(it, &ch->ch_r_intents, it_q) {
		if (!it->it_inuse)
			break;
		if (it->it_size < len)
			continue;
	}
	if (it == NULL) {
		device_printf(sc->sc_dev, "all intents in use\n");
		return;
	}
	it->it_inuse = 1;

	msg.cmd = GLINK_CMD_TX_DATA;
	msg.param1 = ch->ch_lcid;
	msg.param2 = it->it_id;

	chunk_size = len;
	left_size = 0;

	qcpas_glink_tx(sc, (char *)&msg, sizeof(msg));
	qcpas_glink_tx(sc, (char *)&chunk_size, sizeof(chunk_size));
	qcpas_glink_tx(sc, (char *)&left_size, sizeof(left_size));
	qcpas_glink_tx(sc, buf, len);
	qcpas_glink_tx_commit(sc);
}

static void
qcpas_glink_recv_version(struct qcpas_softc *sc, uint32_t ver,
    uint32_t features)
{
	struct glink_msg msg;

	if (ver != GLINK_VERSION) {
		device_printf(sc->sc_dev,
		    "unsupported glink version %u\n", ver);
		return;
	}

	msg.cmd = GLINK_CMD_VERSION_ACK;
	msg.param1 = GLINK_VERSION;
	msg.param2 = features & GLINK_FEATURE_INTENT_REUSE;

	qcpas_glink_tx(sc, (char *)&msg, sizeof(msg));
	qcpas_glink_tx_commit(sc);
}

static void
qcpas_glink_recv_open(struct qcpas_softc *sc, uint32_t rcid, uint32_t namelen)
{
	struct qcpas_glink_protocol *proto = NULL;
	struct qcpas_glink_channel *ch;
	struct glink_msg msg;
	char *name;
	int i, err;

	name = kmem_zalloc(namelen, KM_SLEEP);
	qcpas_glink_rx(sc, name, namelen);
	qcpas_glink_rx_commit(sc);

	TAILQ_FOREACH(ch, &sc->sc_glink_channels, ch_q) {
		if (ch->ch_rcid == rcid) {
			device_printf(sc->sc_dev, "duplicate open for %s\n",
			    name);
			kmem_free(name, namelen);
			return;
		}
	}

	for (i = 0; i < __arraycount(qcpas_glink_protocols); i++) {
		if (strcmp(qcpas_glink_protocols[i].name, name) != 0)
			continue;
		proto = &qcpas_glink_protocols[i];
		break;
	}
	if (proto == NULL) {
		kmem_free(name, namelen);
		return;
	}

	ch = kmem_zalloc(sizeof(*ch), KM_SLEEP);
	ch->ch_sc = sc;
	ch->ch_proto = proto;
	ch->ch_rcid = rcid;
	ch->ch_lcid = ++sc->sc_glink_max_channel;
	TAILQ_INIT(&ch->ch_l_intents);
	TAILQ_INIT(&ch->ch_r_intents);
	TAILQ_INSERT_TAIL(&sc->sc_glink_channels, ch, ch_q);

	/* Assume we can leave HW dangling if proto init fails */
	err = proto->init(ch);
	if (err) {
		TAILQ_REMOVE(&sc->sc_glink_channels, ch, ch_q);
		kmem_free(ch, sizeof(*ch));
		kmem_free(name, namelen);
		return;
	}

	msg.cmd = GLINK_CMD_OPEN_ACK;
	msg.param1 = ch->ch_rcid;
	msg.param2 = 0;

	qcpas_glink_tx(sc, (char *)&msg, sizeof(msg));
	qcpas_glink_tx_commit(sc);

	msg.cmd = GLINK_CMD_OPEN;
	msg.param1 = ch->ch_lcid;
	msg.param2 = strlen(name) + 1;

	qcpas_glink_tx(sc, (char *)&msg, sizeof(msg));
	qcpas_glink_tx(sc, name, strlen(name) + 1);
	qcpas_glink_tx_commit(sc);

	kmem_free(name, namelen);
}

static void
qcpas_glink_recv_open_ack(struct qcpas_softc *sc, uint32_t lcid)
{
	struct qcpas_glink_channel *ch;
	struct glink_msg msg;
	struct qcpas_glink_intent_pair intent;
	int i;

	TAILQ_FOREACH(ch, &sc->sc_glink_channels, ch_q) {
		if (ch->ch_lcid == lcid)
			break;
	}
	if (ch == NULL) {
		device_printf(sc->sc_dev, "unknown channel %u for OPEN_ACK\n",
		    lcid);
		return;
	}

	/* Respond with default intent now that channel is open */
	for (i = 0; i < 5; i++) {
		struct qcpas_glink_intent *it;

		it = kmem_zalloc(sizeof(*it), KM_SLEEP);
		it->it_id = ++ch->ch_max_intent;
		it->it_size = 1024;
		TAILQ_INSERT_TAIL(&ch->ch_l_intents, it, it_q);

		msg.cmd = GLINK_CMD_INTENT;
		msg.param1 = ch->ch_lcid;
		msg.param2 = 1;
		intent.size = it->it_size;
		intent.iid = it->it_id;
	}

	qcpas_glink_tx(sc, (char *)&msg, sizeof(msg));
	qcpas_glink_tx(sc, (char *)&intent, sizeof(intent));
	qcpas_glink_tx_commit(sc);
}

static void
qcpas_glink_recv_intent(struct qcpas_softc *sc, uint32_t rcid, uint32_t count)
{
	struct qcpas_glink_intent_pair *intents;
	struct qcpas_glink_channel *ch;
	struct qcpas_glink_intent *it;
	int i;

	intents = kmem_zalloc(sizeof(*intents) * count, KM_SLEEP);
	qcpas_glink_rx(sc, (char *)intents, sizeof(*intents) * count);
	qcpas_glink_rx_commit(sc);

	TAILQ_FOREACH(ch, &sc->sc_glink_channels, ch_q) {
		if (ch->ch_rcid == rcid)
			break;
	}
	if (ch == NULL) {
		device_printf(sc->sc_dev, "unknown channel %u for INTENT\n",
		    rcid);
		kmem_free(intents, sizeof(*intents) * count);
		return;
	}

	for (i = 0; i < count; i++) {
		it = kmem_zalloc(sizeof(*it), KM_SLEEP);
		it->it_id = intents[i].iid;
		it->it_size = intents[i].size;
		TAILQ_INSERT_TAIL(&ch->ch_r_intents, it, it_q);
	}

	kmem_free(intents, sizeof(*intents) * count);
}

static void
qcpas_glink_recv_tx_data(struct qcpas_softc *sc, uint32_t rcid, uint32_t liid)
{
	struct qcpas_glink_channel *ch;
	struct qcpas_glink_intent *it;
	struct glink_msg msg;
	uint32_t chunk_size, left_size;
	char *buf;

	qcpas_glink_rx(sc, (char *)&chunk_size, sizeof(chunk_size));
	qcpas_glink_rx(sc, (char *)&left_size, sizeof(left_size));
	qcpas_glink_rx_commit(sc);

	buf = kmem_zalloc(chunk_size, KM_SLEEP);
	qcpas_glink_rx(sc, buf, chunk_size);
	qcpas_glink_rx_commit(sc);

	TAILQ_FOREACH(ch, &sc->sc_glink_channels, ch_q) {
		if (ch->ch_rcid == rcid)
			break;
	}
	if (ch == NULL) {
		device_printf(sc->sc_dev, "unknown channel %u for TX_DATA\n",
		    rcid);
		kmem_free(buf, chunk_size);
		return;
	}

	TAILQ_FOREACH(it, &ch->ch_l_intents, it_q) {
		if (it->it_id == liid)
			break;
	}
	if (it == NULL) {
		device_printf(sc->sc_dev, "unknown intent %u for TX_DATA\n",
		    liid);
		kmem_free(buf, chunk_size);
		return;
	}

	/* FIXME: handle message chunking */
	KASSERT(left_size == 0);

	ch->ch_proto->recv(ch, buf, chunk_size);
	kmem_free(buf, chunk_size);

	if (!left_size) {
		msg.cmd = GLINK_CMD_RX_DONE_W_REUSE;
		msg.param1 = ch->ch_lcid;
		msg.param2 = it->it_id;

		qcpas_glink_tx(sc, (char *)&msg, sizeof(msg));
		qcpas_glink_tx_commit(sc);
	}
}

static void
qcpas_glink_recv_rx_done(struct qcpas_softc *sc, uint32_t rcid, uint32_t riid,
    int reuse)
{
	struct qcpas_glink_channel *ch;
	struct qcpas_glink_intent *it;

	TAILQ_FOREACH(ch, &sc->sc_glink_channels, ch_q) {
		if (ch->ch_rcid == rcid)
			break;
	}
	if (ch == NULL) {
		device_printf(sc->sc_dev, "unknown channel %u for RX_DONE\n",
		    rcid);
		return;
	}

	TAILQ_FOREACH(it, &ch->ch_r_intents, it_q) {
		if (it->it_id == riid)
			break;
	}
	if (it == NULL) {
		device_printf(sc->sc_dev, "unknown intent %u for RX_DONE\n",
		    riid);
		return;
	}

	/* FIXME: handle non-reuse */
	KASSERT(reuse);

	KASSERT(it->it_inuse);
	it->it_inuse = 0;
}

static void
qcpas_glink_recv(void *arg)
{
	struct qcpas_softc *sc = arg;
	struct glink_msg msg;

	while (*sc->sc_rx_tail != *sc->sc_rx_head) {
		membar_consumer();
		qcpas_glink_rx(sc, (uint8_t *)&msg, sizeof(msg));
		qcpas_glink_rx_commit(sc);

		switch (msg.cmd) {
		case GLINK_CMD_VERSION:
			qcpas_glink_recv_version(sc, msg.param1, msg.param2);
			break;
		case GLINK_CMD_OPEN:
			qcpas_glink_recv_open(sc, msg.param1, msg.param2);
			break;
		case GLINK_CMD_OPEN_ACK:
			qcpas_glink_recv_open_ack(sc, msg.param1);
			break;
		case GLINK_CMD_INTENT:
			qcpas_glink_recv_intent(sc, msg.param1, msg.param2);
			break;
		case GLINK_CMD_RX_INTENT_REQ:
			/* Nothing to do so far */
			break;
		case GLINK_CMD_TX_DATA:
			qcpas_glink_recv_tx_data(sc, msg.param1, msg.param2);
			break;
		case GLINK_CMD_RX_DONE:
			qcpas_glink_recv_rx_done(sc, msg.param1, msg.param2, 0);
			break;
		case GLINK_CMD_RX_DONE_W_REUSE:
			qcpas_glink_recv_rx_done(sc, msg.param1, msg.param2, 1);
			break;
		default:
			device_printf(sc->sc_dev, "unknown cmd %u\n", msg.cmd);
			return;
		}
	}
}

static int
qcpas_glink_intr(void *cookie)
{
	struct qcpas_softc *sc = cookie;

	sysmon_task_queue_sched(0, qcpas_glink_recv, sc);

	return 1;
}

/* GLINK PMIC Router */

struct pmic_glink_hdr {
	uint32_t owner;
#define PMIC_GLINK_OWNER_BATTMGR	32778
#define PMIC_GLINK_OWNER_USBC		32779
#define PMIC_GLINK_OWNER_USBC_PAN	32780
	uint32_t type;
#define PMIC_GLINK_TYPE_REQ_RESP	1
#define PMIC_GLINK_TYPE_NOTIFY		2
	uint32_t opcode;
};

#define BATTMGR_OPCODE_BAT_STATUS		0x1
#define BATTMGR_OPCODR_REQUEST_NOTIFICATION	0x4
#define BATTMGR_OPCODE_NOTIF			0x7
#define BATTMGR_OPCODE_BAT_INFO			0x9
#define BATTMGR_OPCODE_BAT_DISCHARGE_TIME	0xc
#define BATTMGR_OPCODE_BAT_CHARGE_TIME		0xd

#define BATTMGR_NOTIF_BAT_PROPERTY		0x30
#define BATTMGR_NOTIF_USB_PROPERTY		0x32
#define BATTMGR_NOTIF_WLS_PROPERTY		0x34
#define BATTMGR_NOTIF_BAT_STATUS		0x80
#define BATTMGR_NOTIF_BAT_INFO			0x81

#define BATTMGR_CHEMISTRY_LEN			4
#define BATTMGR_STRING_LEN			128

struct battmgr_bat_info {
	uint32_t power_unit;
	uint32_t design_capacity;
	uint32_t last_full_capacity;
	uint32_t battery_tech;
	uint32_t design_voltage;
	uint32_t capacity_low;
	uint32_t capacity_warning;
	uint32_t cycle_count;
	uint32_t accuracy;
	uint32_t max_sample_time_ms;
	uint32_t min_sample_time_ms;
	uint32_t max_average_interval_ms;
	uint32_t min_average_interval_ms;
	uint32_t capacity_granularity1;
	uint32_t capacity_granularity2;
	uint32_t swappable;
	uint32_t capabilities;
	char model_number[BATTMGR_STRING_LEN];
	char serial_number[BATTMGR_STRING_LEN];
	char battery_type[BATTMGR_STRING_LEN];
	char oem_info[BATTMGR_STRING_LEN];
	char battery_chemistry[BATTMGR_CHEMISTRY_LEN];
	char uid[BATTMGR_STRING_LEN];
	uint32_t critical_bias;
	uint8_t day;
	uint8_t month;
	uint16_t year;
	uint32_t battery_id;
};

struct battmgr_bat_status {
	uint32_t battery_state;
#define BATTMGR_BAT_STATE_DISCHARGE	(1 << 0)
#define BATTMGR_BAT_STATE_CHARGING	(1 << 1)
#define BATTMGR_BAT_STATE_CRITICAL_LOW	(1 << 2)
	uint32_t capacity;
	int32_t rate;
	uint32_t battery_voltage;
	uint32_t power_state;
#define BATTMGR_PWR_STATE_AC_ON			(1 << 0)
	uint32_t charging_source;
#define BATTMGR_CHARGING_SOURCE_AC		1
#define BATTMGR_CHARGING_SOURCE_USB		2
#define BATTMGR_CHARGING_SOURCE_WIRELESS	3
	uint32_t temperature;
};

static void	qcpas_pmic_rtr_refresh(void *);
static void	qcpas_pmic_rtr_bat_info(struct qcpas_softc *,
		    struct battmgr_bat_info *);
static void	qcpas_pmic_rtr_bat_status(struct qcpas_softc *,
		    struct battmgr_bat_status *);

static void
qcpas_pmic_rtr_battmgr_req_info(void *cookie)
{
	struct {
		struct pmic_glink_hdr hdr;
		uint32_t battery_id;
	} msg;

	msg.hdr.owner = PMIC_GLINK_OWNER_BATTMGR;
	msg.hdr.type = PMIC_GLINK_TYPE_REQ_RESP;
	msg.hdr.opcode = BATTMGR_OPCODE_BAT_INFO;
	msg.battery_id = 0;
	qcpas_glink_send(cookie, &msg, sizeof(msg));
}

static void
qcpas_pmic_rtr_battmgr_req_status(void *cookie)
{
	struct {
		struct pmic_glink_hdr hdr;
		uint32_t battery_id;
	} msg;

	msg.hdr.owner = PMIC_GLINK_OWNER_BATTMGR;
	msg.hdr.type = PMIC_GLINK_TYPE_REQ_RESP;
	msg.hdr.opcode = BATTMGR_OPCODE_BAT_STATUS;
	msg.battery_id = 0;
	qcpas_glink_send(cookie, &msg, sizeof(msg));
}

static int
qcpas_pmic_rtr_init(void *cookie)
{
	struct qcpas_glink_channel *ch = cookie;
	struct qcpas_softc *sc = ch->ch_sc;

	callout_init(&sc->sc_rtr_refresh, 0);
	callout_setfunc(&sc->sc_rtr_refresh, qcpas_pmic_rtr_refresh, ch);

	callout_schedule(&sc->sc_rtr_refresh, hz * 5);

	return 0;
}

static int
qcpas_pmic_rtr_recv(void *cookie, uint8_t *buf, int len)
{
	struct qcpas_glink_channel *ch = cookie;
	struct qcpas_softc *sc = ch->ch_sc;
	struct pmic_glink_hdr hdr;
	uint32_t notification;

	if (len < sizeof(hdr)) {
		device_printf(sc->sc_dev, "pmic glink message too small\n");
		return 0;
	}

	memcpy(&hdr, buf, sizeof(hdr));

	switch (hdr.owner) {
	case PMIC_GLINK_OWNER_BATTMGR:
		switch (hdr.opcode) {
		case BATTMGR_OPCODE_NOTIF:
			if (len - sizeof(hdr) != sizeof(uint32_t)) {
				device_printf(sc->sc_dev,
				    "invalid battgmr notification\n");
				return 0;
			}
			memcpy(&notification, buf + sizeof(hdr),
			    sizeof(uint32_t));
			switch (notification) {
			case BATTMGR_NOTIF_BAT_INFO:
				qcpas_pmic_rtr_battmgr_req_info(cookie);
				/* FALLTHROUGH */
			case BATTMGR_NOTIF_BAT_STATUS:
			case BATTMGR_NOTIF_BAT_PROPERTY:
				qcpas_pmic_rtr_battmgr_req_status(cookie);
				break;
			default:
				aprint_debug_dev(sc->sc_dev,
				    "unknown battmgr notification 0x%02x\n",
				    notification);
				break;
			}
			break;
		case BATTMGR_OPCODE_BAT_INFO: {
			struct battmgr_bat_info *bat;
			if (len - sizeof(hdr) < sizeof(*bat)) {
				device_printf(sc->sc_dev,
				    "invalid battgmr bat info\n");
				return 0;
			}
			bat = kmem_alloc(sizeof(*bat), KM_SLEEP);
			memcpy(bat, buf + sizeof(hdr), sizeof(*bat));
			qcpas_pmic_rtr_bat_info(sc, bat);
			kmem_free(bat, sizeof(*bat));
			break;
		}
		case BATTMGR_OPCODE_BAT_STATUS: {
			struct battmgr_bat_status *bat;
			if (len - sizeof(hdr) != sizeof(*bat)) {
				device_printf(sc->sc_dev,
				    "invalid battgmr bat status\n");
				return 0;
			}
			bat = kmem_alloc(sizeof(*bat), KM_SLEEP);
			memcpy(bat, buf + sizeof(hdr), sizeof(*bat));
			qcpas_pmic_rtr_bat_status(sc, bat);
			kmem_free(bat, sizeof(*bat));
			break;
		}
		default:
			device_printf(sc->sc_dev,
			    "unknown battmgr opcode 0x%02x\n",
			    hdr.opcode);
			break;
		}
		break;
	default:
		device_printf(sc->sc_dev,
		    "unknown pmic glink owner 0x%04x\n",
		    hdr.owner);
		break;
	}

	return 0;
}

static void
qcpas_pmic_rtr_refresh(void *arg)
{
	struct qcpas_glink_channel *ch = arg;
	struct qcpas_softc *sc = ch->ch_sc;

	qcpas_pmic_rtr_battmgr_req_status(ch);

	callout_schedule(&sc->sc_rtr_refresh, hz * 5);
}

static void
qcpas_pmic_rtr_bat_info(struct qcpas_softc *sc, struct battmgr_bat_info *bat)
{
	sc->sc_warning_capacity = bat->capacity_warning;
	sc->sc_low_capacity = bat->capacity_low;

	sc->sc_sens[QCPAS_DCAPACITY].value_cur =
	    bat->design_capacity * 1000;
	sc->sc_sens[QCPAS_DCAPACITY].state = ENVSYS_SVALID;

	sc->sc_sens[QCPAS_LFCCAPACITY].value_cur =
	    bat->last_full_capacity * 1000;
	sc->sc_sens[QCPAS_LFCCAPACITY].state = ENVSYS_SVALID;

	sc->sc_sens[QCPAS_DVOLTAGE].value_cur =
	    bat->design_voltage * 1000;
	sc->sc_sens[QCPAS_DVOLTAGE].state = ENVSYS_SVALID;

	sc->sc_sens[QCPAS_DCYCLES].value_cur =
	    bat->cycle_count;
	sc->sc_sens[QCPAS_DCYCLES].state = ENVSYS_SVALID;

	sc->sc_sens[QCPAS_CAPACITY].value_max =
	    bat->last_full_capacity * 1000;
	sysmon_envsys_update_limits(sc->sc_sme,
	    &sc->sc_sens[QCPAS_CAPACITY]);
}

void
qcpas_pmic_rtr_bat_status(struct qcpas_softc *sc,
    struct battmgr_bat_status *bat)
{
	sc->sc_sens[QCPAS_CHARGING].value_cur = 
	    (bat->battery_state & BATTMGR_BAT_STATE_CHARGING) != 0;
	sc->sc_sens[QCPAS_CHARGING].state = ENVSYS_SVALID;
	if ((bat->battery_state & BATTMGR_BAT_STATE_CHARGING) != 0) {
		sc->sc_sens[QCPAS_CHARGERATE].value_cur =
		    abs(bat->rate) * 1000;
		sc->sc_sens[QCPAS_CHARGERATE].state = ENVSYS_SVALID;
		sc->sc_sens[QCPAS_DISCHARGERATE].state = ENVSYS_SINVALID;
	} else if ((bat->battery_state & BATTMGR_BAT_STATE_DISCHARGE) != 0) {
		sc->sc_sens[QCPAS_CHARGERATE].state = ENVSYS_SINVALID;
		sc->sc_sens[QCPAS_DISCHARGERATE].value_cur =
		    abs(bat->rate) * 1000;
		sc->sc_sens[QCPAS_DISCHARGERATE].state = ENVSYS_SVALID;
	} else {
		sc->sc_sens[QCPAS_DISCHARGERATE].state = ENVSYS_SINVALID;
		sc->sc_sens[QCPAS_CHARGERATE].state = ENVSYS_SINVALID;
	}

	sc->sc_sens[QCPAS_VOLTAGE].value_cur =
	    bat->battery_voltage * 1000;
	sc->sc_sens[QCPAS_VOLTAGE].state = ENVSYS_SVALID;

	sc->sc_sens[QCPAS_TEMPERATURE].value_cur =
	    (bat->temperature * 10000) + 273150000;
	sc->sc_sens[QCPAS_TEMPERATURE].state = ENVSYS_SVALID;

	sc->sc_sens[QCPAS_CAPACITY].value_cur =
	    bat->capacity * 1000;
	sc->sc_sens[QCPAS_CAPACITY].state = ENVSYS_SVALID;

	sc->sc_sens[QCPAS_CHARGE_STATE].value_cur =
	    ENVSYS_BATTERY_CAPACITY_NORMAL;
	sc->sc_sens[QCPAS_CHARGE_STATE].state = ENVSYS_SVALID;

	if (bat->capacity < sc->sc_warning_capacity) {
		sc->sc_sens[QCPAS_CAPACITY].state = ENVSYS_SWARNUNDER;
		sc->sc_sens[QCPAS_CHARGE_STATE].value_cur =
		    ENVSYS_BATTERY_CAPACITY_WARNING;
	}

	if (bat->capacity < sc->sc_low_capacity) {
		sc->sc_sens[QCPAS_CAPACITY].state = ENVSYS_SCRITUNDER;
		sc->sc_sens[QCPAS_CHARGE_STATE].value_cur =
		    ENVSYS_BATTERY_CAPACITY_LOW;
	}

	if ((bat->battery_state & BATTMGR_BAT_STATE_CRITICAL_LOW) != 0) {
		sc->sc_sens[QCPAS_CAPACITY].state = ENVSYS_SCRITICAL;
		sc->sc_sens[QCPAS_CHARGE_STATE].value_cur =
		    ENVSYS_BATTERY_CAPACITY_CRITICAL;
	}

	if ((bat->power_state & BATTMGR_PWR_STATE_AC_ON) !=
	    (sc->sc_power_state & BATTMGR_PWR_STATE_AC_ON)) {
		sysmon_pswitch_event(&sc->sc_smpsw_acadapter,
		    (bat->power_state & BATTMGR_PWR_STATE_AC_ON) != 0 ?
		    PSWITCH_EVENT_PRESSED : PSWITCH_EVENT_RELEASED);

		aprint_debug_dev(sc->sc_dev, "AC adapter %sconnected\n",
		    (bat->power_state & BATTMGR_PWR_STATE_AC_ON) == 0 ?
		    "not " : "");
	}

	sc->sc_power_state = bat->power_state;
	sc->sc_sens[QCPAS_ACADAPTER].value_cur =
	    (bat->power_state & BATTMGR_PWR_STATE_AC_ON) != 0;
	sc->sc_sens[QCPAS_ACADAPTER].state = ENVSYS_SVALID;
}

static void
qcpas_get_limits(struct sysmon_envsys *sme, envsys_data_t *edata,
    sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct qcpas_softc *sc = sme->sme_cookie;

	if (edata->sensor != QCPAS_CAPACITY) {
		return;
	}

	limits->sel_critmin = sc->sc_low_capacity * 1000;
	limits->sel_warnmin = sc->sc_warning_capacity * 1000;

	*props |= PROP_BATTCAP | PROP_BATTWARN | PROP_DRIVER_LIMITS;
}
