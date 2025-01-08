/* $NetBSD: scmi.c,v 1.1 2025/01/08 22:55:35 jmcneill Exp $ */
/*	$OpenBSD: scmi.c,v 1.2 2024/11/25 22:12:18 tobhe Exp $	*/

/*
 * Copyright (c) 2023 Mark Kettenis <kettenis@openbsd.org>
 * Copyright (c) 2024 Tobias Heider <tobhe@openbsd.org>
 * Copyright (c) 2025 Jared McNeill <jmcneill@invisible.ca>
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
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/sysctl.h>
#include <sys/cpu.h>

#include <arm/arm/smccc.h>
#include <dev/ic/scmi.h>

#define SCMI_SUCCESS		0
#define SCMI_NOT_SUPPORTED	-1
#define SCMI_DENIED		-3
#define SCMI_BUSY		-6
#define SCMI_COMMS_ERROR	-7

/* Protocols */
#define SCMI_BASE		0x10
#define SCMI_PERF		0x13
#define SCMI_CLOCK		0x14

/* Common messages */
#define SCMI_PROTOCOL_VERSION			0x0
#define SCMI_PROTOCOL_ATTRIBUTES		0x1
#define SCMI_PROTOCOL_MESSAGE_ATTRIBUTES	0x2

/* Clock management messages */
#define SCMI_CLOCK_ATTRIBUTES			0x3
#define SCMI_CLOCK_DESCRIBE_RATES		0x4
#define SCMI_CLOCK_RATE_SET			0x5
#define SCMI_CLOCK_RATE_GET			0x6
#define SCMI_CLOCK_CONFIG_SET			0x7
#define  SCMI_CLOCK_CONFIG_SET_ENABLE		(1U << 0)

/* Performance management messages */
#define SCMI_PERF_DOMAIN_ATTRIBUTES		0x3
#define SCMI_PERF_DESCRIBE_LEVELS		0x4
#define SCMI_PERF_LIMITS_GET			0x6
#define SCMI_PERF_LEVEL_SET			0x7
#define SCMI_PERF_LEVEL_GET			0x8

struct scmi_resp_perf_domain_attributes_40 {
	uint32_t pa_attrs;
#define SCMI_PERF_ATTR_CAN_LEVEL_SET		(1U << 30)
#define SCMI_PERF_ATTR_LEVEL_INDEX_MODE		(1U << 25)
	uint32_t pa_ratelimit;
	uint32_t pa_sustifreq;
	uint32_t pa_sustperf;
	char 	 pa_name[16];
};

struct scmi_resp_perf_describe_levels_40 {
	uint16_t pl_nret;
	uint16_t pl_nrem;
	struct {
		uint32_t	pe_perf;
		uint32_t	pe_cost;
		uint16_t	pe_latency;
		uint16_t	pe_reserved;
		uint32_t	pe_ifreq;
		uint32_t	pe_lindex;
	} pl_entry[];
};

static void scmi_cpufreq_init_sysctl(struct scmi_softc *, uint32_t);

static inline void
scmi_message_header(volatile struct scmi_shmem *shmem,
    uint32_t protocol_id, uint32_t message_id)
{
	shmem->message_header = (protocol_id << 10) | (message_id << 0);
}

int32_t	scmi_smc_command(struct scmi_softc *);
int32_t	scmi_mbox_command(struct scmi_softc *);

int
scmi_init_smc(struct scmi_softc *sc)
{
	volatile struct scmi_shmem *shmem;
	int32_t status;
	uint32_t vers;

	if (sc->sc_smc_id == 0) {
		aprint_error_dev(sc->sc_dev, "no SMC id\n");
		return -1;
	}

	shmem = sc->sc_shmem_tx;

	sc->sc_command = scmi_smc_command;

	if ((shmem->channel_status & SCMI_CHANNEL_FREE) == 0) {
		aprint_error_dev(sc->sc_dev, "channel busy\n");
		return -1;
	}

	scmi_message_header(shmem, SCMI_BASE, SCMI_PROTOCOL_VERSION);
	shmem->length = sizeof(uint32_t);
	status = sc->sc_command(sc);
	if (status != SCMI_SUCCESS) {
		aprint_error_dev(sc->sc_dev, "protocol version command failed\n");
		return -1;
	}

	vers = shmem->message_payload[1];
	sc->sc_ver_major = vers >> 16;
	sc->sc_ver_minor = vers & 0xfffff;
	aprint_normal_dev(sc->sc_dev, "SCMI %d.%d\n",
	    sc->sc_ver_major, sc->sc_ver_minor);

	mutex_init(&sc->sc_shmem_tx_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_shmem_rx_lock, MUTEX_DEFAULT, IPL_NONE);

	return 0;
}

int
scmi_init_mbox(struct scmi_softc *sc)
{
	int32_t status;
	uint32_t vers;

	if (sc->sc_mbox_tx == NULL) {
		aprint_error_dev(sc->sc_dev, "no tx mbox\n");
		return -1;
	}
	if (sc->sc_mbox_rx == NULL) {
		aprint_error_dev(sc->sc_dev, "no rx mbox\n");
		return -1;
	}

	sc->sc_command = scmi_mbox_command;

	scmi_message_header(sc->sc_shmem_tx, SCMI_BASE, SCMI_PROTOCOL_VERSION);
	sc->sc_shmem_tx->length = sizeof(uint32_t);
	status = sc->sc_command(sc);
	if (status != SCMI_SUCCESS) {
		aprint_error_dev(sc->sc_dev,
		    "protocol version command failed\n");
		return -1;
	}

	vers = sc->sc_shmem_tx->message_payload[1];
	sc->sc_ver_major = vers >> 16;
	sc->sc_ver_minor = vers & 0xfffff;
	aprint_normal_dev(sc->sc_dev, "SCMI %d.%d\n",
	    sc->sc_ver_major, sc->sc_ver_minor);

	mutex_init(&sc->sc_shmem_tx_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_shmem_rx_lock, MUTEX_DEFAULT, IPL_NONE);

	return 0;
}

int32_t
scmi_smc_command(struct scmi_softc *sc)
{
	volatile struct scmi_shmem *shmem = sc->sc_shmem_tx;
	int32_t status;

	shmem->channel_status = 0;
	status = smccc_call(sc->sc_smc_id, 0, 0, 0, 0,
			    NULL, NULL, NULL, NULL);
	if (status != SMCCC_SUCCESS)
		return SCMI_NOT_SUPPORTED;
	if ((shmem->channel_status & SCMI_CHANNEL_ERROR))
		return SCMI_COMMS_ERROR;
	if ((shmem->channel_status & SCMI_CHANNEL_FREE) == 0)
		return SCMI_BUSY;
	return shmem->message_payload[0];
}

int32_t
scmi_mbox_command(struct scmi_softc *sc)
{
	volatile struct scmi_shmem *shmem = sc->sc_shmem_tx;
	int ret;
	int i;

	shmem->channel_status = 0;
	ret = sc->sc_mbox_tx_send(sc->sc_mbox_tx);
	if (ret != 0)
		return SCMI_NOT_SUPPORTED; 

	/* XXX: poll for now */
	for (i = 0; i < 20; i++) {
		if (shmem->channel_status & SCMI_CHANNEL_FREE)
			break;
		delay(10);
	}
	if ((shmem->channel_status & SCMI_CHANNEL_ERROR))
		return SCMI_COMMS_ERROR;
	if ((shmem->channel_status & SCMI_CHANNEL_FREE) == 0)
		return SCMI_BUSY;

	return shmem->message_payload[0];
}

#if notyet
/* Clock management. */

void	scmi_clock_enable(void *, uint32_t *, int);
uint32_t scmi_clock_get_frequency(void *, uint32_t *);
int	scmi_clock_set_frequency(void *, uint32_t *, uint32_t);

void
scmi_attach_clock(struct scmi_softc *sc, int node)
{
	volatile struct scmi_shmem *shmem = sc->sc_shmem_tx;
	int32_t status;
	int nclocks;

	scmi_message_header(shmem, SCMI_CLOCK, SCMI_PROTOCOL_ATTRIBUTES);
	shmem->length = sizeof(uint32_t);
	status = sc->sc_command(sc);
	if (status != SCMI_SUCCESS)
		return;

	nclocks = shmem->message_payload[1] & 0xffff;
	if (nclocks == 0)
		return;

	sc->sc_cd.cd_node = node;
	sc->sc_cd.cd_cookie = sc;
	sc->sc_cd.cd_enable = scmi_clock_enable;
	sc->sc_cd.cd_get_frequency = scmi_clock_get_frequency;
	sc->sc_cd.cd_set_frequency = scmi_clock_set_frequency;
	clock_register(&sc->sc_cd);
}

void
scmi_clock_enable(void *cookie, uint32_t *cells, int on)
{
	struct scmi_softc *sc = cookie;
	volatile struct scmi_shmem *shmem = sc->sc_shmem_tx;
	uint32_t idx = cells[0];

	scmi_message_header(shmem, SCMI_CLOCK, SCMI_CLOCK_CONFIG_SET);
	shmem->length = 3 * sizeof(uint32_t);
	shmem->message_payload[0] = idx;
	shmem->message_payload[1] = on ? SCMI_CLOCK_CONFIG_SET_ENABLE : 0;
	sc->sc_command(sc);
}

uint32_t
scmi_clock_get_frequency(void *cookie, uint32_t *cells)
{
	struct scmi_softc *sc = cookie;
	volatile struct scmi_shmem *shmem = sc->sc_shmem_tx;
	uint32_t idx = cells[0];
	int32_t status;

	scmi_message_header(shmem, SCMI_CLOCK, SCMI_CLOCK_RATE_GET);
	shmem->length = 2 * sizeof(uint32_t);
	shmem->message_payload[0] = idx;
	status = sc->sc_command(sc);
	if (status != SCMI_SUCCESS)
		return 0;
	if (shmem->message_payload[2] != 0)
		return 0;

	return shmem->message_payload[1];
}

int
scmi_clock_set_frequency(void *cookie, uint32_t *cells, uint32_t freq)
{
	struct scmi_softc *sc = cookie;
	volatile struct scmi_shmem *shmem = sc->sc_shmem_tx;
	uint32_t idx = cells[0];
	int32_t status;

	scmi_message_header(shmem, SCMI_CLOCK, SCMI_CLOCK_RATE_SET);
	shmem->length = 5 * sizeof(uint32_t);
	shmem->message_payload[0] = 0;
	shmem->message_payload[1] = idx;
	shmem->message_payload[2] = freq;
	shmem->message_payload[3] = 0;
	status = sc->sc_command(sc);
	if (status != SCMI_SUCCESS)
		return -1;

	return 0;
}
#endif

/* Performance management */
void	scmi_perf_descr_levels(struct scmi_softc *, int);

void
scmi_attach_perf(struct scmi_softc *sc)
{
	volatile struct scmi_shmem *shmem = sc->sc_shmem_tx;
	int32_t status;
	uint32_t vers;
	int i;

	scmi_message_header(sc->sc_shmem_tx, SCMI_PERF, SCMI_PROTOCOL_VERSION);
	sc->sc_shmem_tx->length = sizeof(uint32_t);
	status = sc->sc_command(sc);
	if (status != SCMI_SUCCESS) {
		aprint_error_dev(sc->sc_dev,
		    "SCMI_PROTOCOL_VERSION failed\n");
		return;
	}

	vers = shmem->message_payload[1];
	if (vers != 0x40000) {
		aprint_error_dev(sc->sc_dev,
		    "invalid perf protocol version (0x%x != 0x4000)", vers);
		return;
	}

	scmi_message_header(shmem, SCMI_PERF, SCMI_PROTOCOL_ATTRIBUTES);
	shmem->length = sizeof(uint32_t);
	status = sc->sc_command(sc);
	if (status != SCMI_SUCCESS) {
		aprint_error_dev(sc->sc_dev,
		    "SCMI_PROTOCOL_ATTRIBUTES failed\n");
		return;
	}

	sc->sc_perf_ndomains = shmem->message_payload[1] & 0xffff;
	sc->sc_perf_domains = kmem_zalloc(sc->sc_perf_ndomains *
	    sizeof(struct scmi_perf_domain), KM_SLEEP);
	sc->sc_perf_power_unit = (shmem->message_payload[1] >> 16) & 0x3;

	/* Add one frequency sensor per perf domain */
	for (i = 0; i < sc->sc_perf_ndomains; i++) {
		volatile struct scmi_resp_perf_domain_attributes_40 *pa;

		scmi_message_header(shmem, SCMI_PERF,
		    SCMI_PERF_DOMAIN_ATTRIBUTES);
		shmem->length = 2 * sizeof(uint32_t);
		shmem->message_payload[0] = i;
		status = sc->sc_command(sc);
		if (status != SCMI_SUCCESS) {
			aprint_error_dev(sc->sc_dev,
			    "SCMI_PERF_DOMAIN_ATTRIBUTES failed\n");
			return;
		}

		pa = (volatile struct scmi_resp_perf_domain_attributes_40 *)
		    &shmem->message_payload[1];
		aprint_debug_dev(sc->sc_dev,
		    "dom %u attr %#x rate_limit %u sfreq %u sperf %u "
		    "name \"%s\"\n",
		    i, pa->pa_attrs, pa->pa_ratelimit, pa->pa_sustifreq,
		    pa->pa_sustperf, pa->pa_name);

		sc->sc_perf_domains[i].pd_domain_id = i;
		sc->sc_perf_domains[i].pd_sc = sc;
		for (int map = 0; map < sc->sc_perf_ndmap; map++) {
			if (sc->sc_perf_dmap[map].pm_domain == i) {
				sc->sc_perf_domains[i].pd_ci =
				    sc->sc_perf_dmap[map].pm_ci;
				break;
			}
		}
		snprintf(sc->sc_perf_domains[i].pd_name,
		    sizeof(sc->sc_perf_domains[i].pd_name), "%s", pa->pa_name);
		sc->sc_perf_domains[i].pd_can_level_set =
		    (pa->pa_attrs & SCMI_PERF_ATTR_CAN_LEVEL_SET) != 0;
		sc->sc_perf_domains[i].pd_level_index_mode =
		    (pa->pa_attrs & SCMI_PERF_ATTR_LEVEL_INDEX_MODE) != 0;
		sc->sc_perf_domains[i].pd_rate_limit = pa->pa_ratelimit;
		sc->sc_perf_domains[i].pd_sustained_perf = pa->pa_sustperf;

		scmi_perf_descr_levels(sc, i);

		if (sc->sc_perf_domains[i].pd_can_level_set &&
		    sc->sc_perf_domains[i].pd_nlevels > 0 &&
		    sc->sc_perf_domains[i].pd_levels[0].pl_ifreq != 0) {
			scmi_cpufreq_init_sysctl(sc, i);
		}
	}
	return;
}

void
scmi_perf_descr_levels(struct scmi_softc *sc, int domain)
{
	volatile struct scmi_shmem *shmem = sc->sc_shmem_tx;
	volatile struct scmi_resp_perf_describe_levels_40 *pl;
	struct scmi_perf_domain *pd = &sc->sc_perf_domains[domain];
	int status, i, idx;

	idx = 0;
	do {
		scmi_message_header(shmem, SCMI_PERF,
		    SCMI_PERF_DESCRIBE_LEVELS);
		shmem->length = sizeof(uint32_t) * 3;
		shmem->message_payload[0] = domain;
		shmem->message_payload[1] = idx;
		status = sc->sc_command(sc);
		if (status != SCMI_SUCCESS) {
			aprint_error_dev(sc->sc_dev,
			    "SCMI_PERF_DESCRIBE_LEVELS failed\n");
			return;
		}

		pl = (volatile struct scmi_resp_perf_describe_levels_40 *)
		    &shmem->message_payload[1];

		if (pd->pd_levels == NULL) {
			pd->pd_nlevels = pl->pl_nret + pl->pl_nrem;
			pd->pd_levels = kmem_zalloc(pd->pd_nlevels *
			    sizeof(struct scmi_perf_level),
			    KM_SLEEP);
		}

		for (i = 0; i < pl->pl_nret; i++) {
			pd->pd_levels[idx + i].pl_cost =
			    pl->pl_entry[i].pe_cost;
			pd->pd_levels[idx + i].pl_perf =
			    pl->pl_entry[i].pe_perf;
			pd->pd_levels[idx + i].pl_ifreq =
			    pl->pl_entry[i].pe_ifreq;
			aprint_debug_dev(sc->sc_dev,
			    "dom %u pl %u cost %u perf %i ifreq %u\n",
			    domain, idx + i,
			    pl->pl_entry[i].pe_cost,
			    pl->pl_entry[i].pe_perf,
			    pl->pl_entry[i].pe_ifreq);
		}
		idx += pl->pl_nret;
	} while (pl->pl_nrem);
}

static int32_t
scmi_perf_limits_get(struct scmi_perf_domain *pd, uint32_t *max_level,
    uint32_t *min_level)
{
	struct scmi_softc *sc = pd->pd_sc;
	volatile struct scmi_shmem *shmem = sc->sc_shmem_tx;
	int32_t status;

	if (pd->pd_levels == NULL) {
		return SCMI_NOT_SUPPORTED;
	}

	mutex_enter(&sc->sc_shmem_tx_lock);
	scmi_message_header(shmem, SCMI_PERF, SCMI_PERF_LIMITS_GET);
	shmem->length = sizeof(uint32_t) * 2;
	shmem->message_payload[0] = pd->pd_domain_id;
	status = sc->sc_command(sc);
	if (status == SCMI_SUCCESS) {
		*max_level = shmem->message_payload[1];
		*min_level = shmem->message_payload[2];
	}
	mutex_exit(&sc->sc_shmem_tx_lock);

	return status;
}

static int32_t
scmi_perf_level_get(struct scmi_perf_domain *pd, uint32_t *perf_level)
{
	struct scmi_softc *sc = pd->pd_sc;
	volatile struct scmi_shmem *shmem = sc->sc_shmem_tx;
	int32_t status;

	if (pd->pd_levels == NULL) {
		return SCMI_NOT_SUPPORTED;
	}

	mutex_enter(&sc->sc_shmem_tx_lock);
	scmi_message_header(shmem, SCMI_PERF, SCMI_PERF_LEVEL_GET);
	shmem->length = sizeof(uint32_t) * 2;
	shmem->message_payload[0] = pd->pd_domain_id;
	status = sc->sc_command(sc);
	if (status == SCMI_SUCCESS) {
		*perf_level = shmem->message_payload[1];
	}
	mutex_exit(&sc->sc_shmem_tx_lock);

	return status;
}

static int32_t
scmi_perf_level_set(struct scmi_perf_domain *pd, uint32_t perf_level)
{
	struct scmi_softc *sc = pd->pd_sc;
	volatile struct scmi_shmem *shmem = sc->sc_shmem_tx;
	int32_t status;

	if (pd->pd_levels == NULL) {
		return SCMI_NOT_SUPPORTED;
	}

	mutex_enter(&sc->sc_shmem_tx_lock);
	scmi_message_header(shmem, SCMI_PERF, SCMI_PERF_LEVEL_SET);
	shmem->length = sizeof(uint32_t) * 3;
	shmem->message_payload[0] = pd->pd_domain_id;
	shmem->message_payload[1] = perf_level;
	status = sc->sc_command(sc);
	mutex_exit(&sc->sc_shmem_tx_lock);

	return status;
}

static u_int
scmi_cpufreq_level_to_mhz(struct scmi_perf_domain *pd, uint32_t level)
{
	ssize_t n;

	if (pd->pd_level_index_mode) {
		if (level < pd->pd_nlevels) {
			return pd->pd_levels[level].pl_ifreq / 1000;
		}
	} else {
		for (n = 0; n < pd->pd_nlevels; n++) {
			if (pd->pd_levels[n].pl_perf == level) {
				return pd->pd_levels[n].pl_ifreq / 1000;
			}
		}
	}

	return 0;
}

static int
scmi_cpufreq_set_rate(struct scmi_softc *sc, struct scmi_perf_domain *pd,
    u_int freq_mhz)
{
	uint32_t perf_level = -1;
	int32_t status;
	ssize_t n;

	for (n = 0; n < pd->pd_nlevels; n++) {
		if (pd->pd_levels[n].pl_ifreq / 1000 == freq_mhz) {
			perf_level = pd->pd_level_index_mode ?
			    n : pd->pd_levels[n].pl_perf;
			break;
		}
	}
	if (n == pd->pd_nlevels)
		return EINVAL;

	status = scmi_perf_level_set(pd, perf_level);
	if (status != SCMI_SUCCESS) {
		return EIO;
	}

	if (pd->pd_rate_limit > 0)
		delay(pd->pd_rate_limit);

	return 0;
}

static int
scmi_cpufreq_sysctl_helper(SYSCTLFN_ARGS)
{
	struct scmi_perf_domain * const pd = rnode->sysctl_data;
	struct scmi_softc * const sc = pd->pd_sc;
	struct sysctlnode node;
	u_int fq, oldfq = 0, old_target;
	uint32_t level;
	int32_t status;
	int error;

	node = *rnode;
	node.sysctl_data = &fq;

	if (rnode->sysctl_num == pd->pd_node_target) {
		if (pd->pd_freq_target == 0) {
			status = scmi_perf_level_get(pd, &level);
			if (status != SCMI_SUCCESS) {
				return EIO;
			}
			pd->pd_freq_target =
			    scmi_cpufreq_level_to_mhz(pd, level);
		}
		fq = pd->pd_freq_target;
	} else {
		status = scmi_perf_level_get(pd, &level);
		if (status != SCMI_SUCCESS) {
			return EIO;
		}
		fq = scmi_cpufreq_level_to_mhz(pd, level);
	}

	if (rnode->sysctl_num == pd->pd_node_target)
		oldfq = fq;

	if (pd->pd_freq_target == 0)
		pd->pd_freq_target = fq;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (fq == oldfq || rnode->sysctl_num != pd->pd_node_target)
		return 0;

	if (atomic_cas_uint(&pd->pd_busy, 0, 1) != 0)
		return EBUSY;

	old_target = pd->pd_freq_target;
	pd->pd_freq_target = fq;

	error = scmi_cpufreq_set_rate(sc, pd, fq);
	if (error != 0) {
		pd->pd_freq_target = old_target;
	}

	atomic_dec_uint(&pd->pd_busy);

	return error;
}

static void
scmi_cpufreq_init_sysctl(struct scmi_softc *sc, uint32_t domain_id)
{
	const struct sysctlnode *node, *cpunode;
	struct scmi_perf_domain *pd = &sc->sc_perf_domains[domain_id];
	struct cpu_info *ci = pd->pd_ci;
	struct sysctllog *cpufreq_log = NULL;
	uint32_t max_level, min_level;
	int32_t status;
	int error, i;

	if (ci == NULL)
		return;

	status = scmi_perf_limits_get(pd, &max_level, &min_level);
	if (status != SCMI_SUCCESS) {
		/*
		 * Not supposed to happen, but at least one implementation
		 * returns DENIED here. Assume that there are no limits.
		 */
		min_level = 0;
		max_level = UINT32_MAX;
	}
	aprint_debug_dev(sc->sc_dev, "dom %u limits max %u min %u\n",
	    domain_id, max_level, min_level);

	pd->pd_freq_available = kmem_zalloc(strlen("XXXX ") *
	    pd->pd_nlevels, KM_SLEEP);
	for (i = 0; i < pd->pd_nlevels; i++) {
		char buf[6];
		uint32_t level = pd->pd_level_index_mode ?
				 i : pd->pd_levels[i].pl_perf;

		if (level < min_level) {
			continue;
		} else if (level > max_level) {
			break;
		}

		snprintf(buf, sizeof(buf), i ? " %u" : "%u",
		    pd->pd_levels[i].pl_ifreq / 1000);
		strcat(pd->pd_freq_available, buf);
		if (level == pd->pd_sustained_perf) {
			break;
		}
	}

	error = sysctl_createv(&cpufreq_log, 0, NULL, &node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL);
	if (error)
		goto sysctl_failed;
	error = sysctl_createv(&cpufreq_log, 0, &node, &node,
	    0, CTLTYPE_NODE, "cpufreq", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;
	error = sysctl_createv(&cpufreq_log, 0, &node, &cpunode,
	    0, CTLTYPE_NODE, cpu_name(ci), NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;

	error = sysctl_createv(&cpufreq_log, 0, &cpunode, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "target", NULL,
	    scmi_cpufreq_sysctl_helper, 0, (void *)pd, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;
	pd->pd_node_target = node->sysctl_num;

	error = sysctl_createv(&cpufreq_log, 0, &cpunode, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "current", NULL,
	    scmi_cpufreq_sysctl_helper, 0, (void *)pd, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;
	pd->pd_node_current = node->sysctl_num;

	error = sysctl_createv(&cpufreq_log, 0, &cpunode, &node,
	    0, CTLTYPE_STRING, "available", NULL,
	    NULL, 0, pd->pd_freq_available, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;
	pd->pd_node_available = node->sysctl_num;

	return;

sysctl_failed:
	aprint_error_dev(sc->sc_dev, "couldn't create sysctl nodes: %d\n",
	    error);
	sysctl_teardown(&cpufreq_log);
}
