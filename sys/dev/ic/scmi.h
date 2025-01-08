/* $NetBSD: scmi.h,v 1.1 2025/01/08 22:55:35 jmcneill Exp $ */

/*
 * Copyright (c) 2023 Mark Kettenis <kettenis@openbsd.org>
 * Copyright (c) 2024 Tobias Heider <tobhe@openbsd.org>
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

#pragma once

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/mutex.h>

struct scmi_shmem {
	uint32_t reserved1;
	uint32_t channel_status;
#define SCMI_CHANNEL_ERROR		(1 << 1)
#define SCMI_CHANNEL_FREE		(1 << 0)
	uint32_t reserved2;
	uint32_t reserved3;
	uint32_t channel_flags;
	uint32_t length;
	uint32_t message_header;
	uint32_t message_payload[];
};

struct scmi_softc;

struct scmi_perf_level {
	uint32_t	pl_perf;
	uint32_t	pl_cost;
	uint32_t	pl_ifreq;
};

struct scmi_perf_domain {
	struct scmi_softc		*pd_sc;
	uint32_t			pd_domain_id;
	size_t				pd_nlevels;
	struct scmi_perf_level		*pd_levels;
	int				pd_curlevel;
	char				pd_name[16];
	bool				pd_can_level_set;
	bool				pd_level_index_mode;
	uint32_t			pd_rate_limit;
	uint32_t			pd_sustained_perf;

	struct cpu_info			*pd_ci;	/* first CPU in domain */
	u_int				pd_busy;
	char				*pd_freq_available;
	u_int				pd_freq_target;
	int				pd_node_target;
	int				pd_node_current;
	int				pd_node_available;
};

struct scmi_perf_domain_map {
	uint32_t			pm_domain;
	struct cpu_info			*pm_ci;	/* first CPU in domain */
};

struct scmi_softc {
	device_t			sc_dev;
	bus_space_tag_t			sc_iot;
	volatile struct scmi_shmem	*sc_shmem_tx;
	volatile struct scmi_shmem	*sc_shmem_rx;
	kmutex_t			sc_shmem_tx_lock;
	kmutex_t			sc_shmem_rx_lock;

	uint32_t			sc_smc_id;

	void				*sc_mbox_tx;
	int				(*sc_mbox_tx_send)(void *);
	void				*sc_mbox_rx;
	int				(*sc_mbox_rx_send)(void *);

	size_t				sc_perf_ndmap;
	struct scmi_perf_domain_map	*sc_perf_dmap;

	uint16_t			sc_ver_major;
	uint16_t			sc_ver_minor;

#if notyet
	/* SCMI_CLOCK */
	struct clock_device		sc_cd;
#endif

	/* SCMI_PERF */
	int				sc_perf_power_unit;
#define SCMI_POWER_UNIT_UW	0x2
#define SCMI_POWER_UNIT_MW	0x1
#define SCMI_POWER_UNIT_NONE	0x0
	size_t				sc_perf_ndomains;
	struct scmi_perf_domain		*sc_perf_domains;

	int32_t				(*sc_command)(struct scmi_softc *);
};

int	scmi_init_smc(struct scmi_softc *);
int	scmi_init_mbox(struct scmi_softc *);

void    scmi_attach_perf(struct scmi_softc *);
