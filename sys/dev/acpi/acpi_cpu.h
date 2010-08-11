/* $NetBSD: acpi_cpu.h,v 1.13.2.2 2010/08/11 22:53:15 yamt Exp $ */

/*-
 * Copyright (c) 2010 Jukka Ruohonen <jruohonen@iki.fi>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
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

#ifndef _SYS_DEV_ACPI_ACPI_CPU_H
#define _SYS_DEV_ACPI_ACPI_CPU_H

/*
 * The following _PDC values are based on:
 *
 * 	Intel Corporation: Intel Processor-Specific ACPI
 *	Interface Specification, September 2006, Revision 005.
 *
 *	http://download.intel.com/technology/IAPC/acpi/downloads/30222305.pdf
 *
 * For other relevant reading, see for instance:
 *
 *	Advanced Micro Devices: Using ACPI to Report APML P-State
 *	Limit Changes to Operating Systems and VMM's. August 7, 2009.
 *
 *	http://developer.amd.com/Assets/ACPI-APML-PState-rev12.pdf
 */
#define ACPICPU_PDC_REVID         0x1
#define ACPICPU_PDC_SMP           0xA
#define ACPICPU_PDC_MSR           0x1

#define ACPICPU_PDC_P_FFH         __BIT(0)	/* SpeedStep MSRs            */
#define ACPICPU_PDC_C_C1_HALT     __BIT(1)	/* C1 "I/O then halt"        */
#define ACPICPU_PDC_T_FFH         __BIT(2)	/* OnDemand throttling MSRs  */
#define ACPICPU_PDC_C_C1PT        __BIT(3)	/* SMP C1, Px, and Tx (same) */
#define ACPICPU_PDC_C_C2C3        __BIT(4)	/* SMP C2 and C3 (same)      */
#define ACPICPU_PDC_P_SW          __BIT(5)	/* SMP Px (different)        */
#define ACPICPU_PDC_C_SW          __BIT(6)	/* SMP Cx (different)        */
#define ACPICPU_PDC_T_SW          __BIT(7)	/* SMP Tx (different)        */
#define ACPICPU_PDC_C_C1_FFH      __BIT(8)	/* SMP C1 native beyond halt */
#define ACPICPU_PDC_C_C2C3_FFH    __BIT(9)	/* SMP C2 and C2 native      */
#define ACPICPU_PDC_P_HW          __BIT(11)	/* Px hardware coordination  */

#define ACPICPU_PDC_GAS_HW	  __BIT(0)	/* HW-coordinated state      */
#define ACPICPU_PDC_GAS_BM	  __BIT(1)	/* Bus master check required */

/*
 * Notify values.
 */
#define ACPICPU_P_NOTIFY	 0x80		/* _PPC */
#define ACPICPU_C_NOTIFY	 0x81		/* _CST */
#define ACPICPU_T_NOTIFY	 0x82		/* _TPC */

/*
 * C-states.
 */
#define ACPICPU_C_C2_LATENCY_MAX 100		/* us */
#define ACPICPU_C_C3_LATENCY_MAX 1000		/* us */

#define ACPICPU_C_STATE_HALT	 0x01
#define ACPICPU_C_STATE_FFH	 0x02
#define ACPICPU_C_STATE_SYSIO	 0x03

/*
 * P-states.
 */
#define ACPICPU_P_STATE_RETRY	 100
#define ACPICPU_P_STATE_UNKNOWN	 0x0

/*
 * Flags.
 */
#define ACPICPU_FLAG_C		 __BIT(0)	/* C-states supported        */
#define ACPICPU_FLAG_P		 __BIT(1)	/* P-states supported        */
#define ACPICPU_FLAG_T		 __BIT(2)	/* T-states supported        */

#define ACPICPU_FLAG_C_CST	 __BIT(3)	/* C-states with _CST	     */
#define ACPICPU_FLAG_C_FADT	 __BIT(4)	/* C-states with FADT        */
#define ACPICPU_FLAG_C_BM	 __BIT(5)	/* Bus master control        */
#define ACPICPU_FLAG_C_BM_STS	 __BIT(6)	/* Bus master check required */
#define ACPICPU_FLAG_C_ARB	 __BIT(7)	/* Bus master arbitration    */
#define ACPICPU_FLAG_C_NOC3	 __BIT(8)	/* C3 disabled (quirk)       */
#define ACPICPU_FLAG_C_FFH	 __BIT(9)	/* MONITOR/MWAIT supported   */
#define ACPICPU_FLAG_C_C1E	 __BIT(10)	/* AMD C1E detected	     */

#define ACPICPU_FLAG_P_PPC	 __BIT(11)	/* Dynamic freq. with _PPC   */
#define ACPICPU_FLAG_P_FFH	 __BIT(12)	/* EST etc. supported        */

/*
 * This is AML_RESOURCE_GENERIC_REGISTER,
 * included here separately for convenience.
 */
struct acpicpu_reg {
	uint8_t			 reg_desc;
	uint16_t		 reg_reslen;
	uint8_t			 reg_spaceid;
	uint8_t			 reg_bitwidth;
	uint8_t			 reg_bitoffset;
	uint8_t			 reg_accesssize;
	uint64_t		 reg_addr;
} __packed;

struct acpicpu_cstate {
	struct evcnt		 cs_evcnt;
	char			 cs_name[EVCNT_STRING_MAX];
	uint64_t		 cs_addr;
	uint32_t		 cs_power;	/* mW */
	uint32_t		 cs_latency;	/* us */
	int			 cs_method;
	int			 cs_flags;
};

struct acpicpu_pstate {
	struct evcnt		 ps_evcnt;
	char			 ps_name[EVCNT_STRING_MAX];
	uint32_t		 ps_freq;	/* MHz */
	uint32_t		 ps_power;	/* mW */
	uint32_t		 ps_latency;	/* us */
	uint32_t		 ps_latency_bm; /* us */
	uint32_t		 ps_control;
	uint32_t		 ps_status;
};

struct acpicpu_object {
	uint32_t		 ao_procid;
	uint32_t		 ao_pblklen;
	uint32_t		 ao_pblkaddr;
};

struct acpicpu_softc {
	device_t		 sc_dev;
	struct acpi_devnode	*sc_node;
	struct acpicpu_object	 sc_object;

	struct acpicpu_cstate	 sc_cstate[ACPI_C_STATE_COUNT];
	uint32_t		 sc_cstate_sleep;

	struct acpicpu_pstate	*sc_pstate;
	struct acpicpu_reg	 sc_pstate_control;
	struct acpicpu_reg	 sc_pstate_status;
	uint32_t		 sc_pstate_current;
	uint32_t		 sc_pstate_count;
	uint32_t		 sc_pstate_max;

	kmutex_t		 sc_mtx;
	bus_space_tag_t		 sc_iot;
	bus_space_handle_t	 sc_ioh;

	uint32_t		 sc_cap;
	uint32_t		 sc_flags;
	cpuid_t			 sc_cpuid;
	bool			 sc_cold;
	bool			 sc_mapped;
};

void		acpicpu_cstate_attach(device_t);
int		acpicpu_cstate_detach(device_t);
int		acpicpu_cstate_start(device_t);
bool		acpicpu_cstate_suspend(device_t);
bool		acpicpu_cstate_resume(device_t);
void		acpicpu_cstate_callback(void *);
void		acpicpu_cstate_idle(void);

void		acpicpu_pstate_attach(device_t);
int		acpicpu_pstate_detach(device_t);
int		acpicpu_pstate_start(device_t);
bool		acpicpu_pstate_suspend(device_t);
bool		acpicpu_pstate_resume(device_t);
void		acpicpu_pstate_callback(void *);
int		acpicpu_pstate_get(struct acpicpu_softc *, uint32_t *);
int		acpicpu_pstate_set(struct acpicpu_softc *, uint32_t);

uint32_t	acpicpu_md_cap(void);
uint32_t	acpicpu_md_quirks(void);
uint32_t	acpicpu_md_cpus_running(void);
int		acpicpu_md_idle_start(void);
int		acpicpu_md_idle_stop(void);
void		acpicpu_md_idle_enter(int, int);
int		acpicpu_md_pstate_start(void);
int		acpicpu_md_pstate_stop(void);
int		acpicpu_md_pstate_get(struct acpicpu_softc *, uint32_t *);
int		acpicpu_md_pstate_set(struct acpicpu_pstate *);

#endif	/* !_SYS_DEV_ACPI_ACPI_CPU_H */
