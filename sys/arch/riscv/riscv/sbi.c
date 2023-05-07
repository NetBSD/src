/*	$NetBSD: sbi.c,v 1.1 2023/05/07 12:41:49 skrll Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#include <sys/param.h>
#include <sys/types.h>

#include <sys/device.h>
#include <dev/cons.h>

#include <riscv/sbi.h>

struct sbiret
sbi_get_spec_version(void)
{
	return SBI_CALL0(SBI_EID_BASE, SBI_FID_BASE_GETSPECVERSION);
}

struct sbiret
sbi_get_impl_id(void)
{
	return SBI_CALL0(SBI_EID_BASE, SBI_FID_BASE_GETIMPLID);
}

struct sbiret
sbi_get_impl_version(void)
{
	return SBI_CALL0(SBI_EID_BASE, SBI_FID_BASE_GETIMPLVERSION);
}

struct sbiret
sbi_probe_extension(long extension_id)
{
	return SBI_CALL1(SBI_EID_BASE, SBI_FID_BASE_PROBEEXTENTION,
	    extension_id);
}

struct sbiret
sbi_get_mvendorid(void)
{
	return SBI_CALL0(SBI_EID_BASE, SBI_FID_BASE_GETMVENDORID);
}

struct sbiret
sbi_get_marchid(void)
{
	return SBI_CALL0(SBI_EID_BASE, SBI_FID_BASE_GETMARCHID);
}

struct sbiret
sbi_get_mimpid(void)
{
	return SBI_CALL0(SBI_EID_BASE, SBI_FID_BASE_GETMIMPID);
}


struct sbiret
sbi_set_timer(uint64_t stime_value)
{
#ifdef _LP64
	struct sbiret ret = SBI_CALL1(SBI_EID_TIMER, SBI_FID_TIMER_SET,
	    stime_value);
#else
	struct sbiret ret = SBI_CALL2(SBI_EID_TIMER, SBI_FID_TIMER_SET,
	    stime_value, stime_value >> 32);
#endif
	return ret;
}

struct sbiret
sbi_send_ipi(unsigned long hart_mask, unsigned long hart_mask_base)
{
	return SBI_CALL2(SBI_EID_IPI, SBI_FID_IPI_SEND,
	    hart_mask, hart_mask_base);
}


struct sbiret
sbi_remote_fence_i(unsigned long hart_mask, unsigned long hart_mask_base)
{
	return SBI_CALL2(SBI_EID_RFENCE, SBI_FID_RFENCE_FENCEI,
	    hart_mask, hart_mask_base);
}

struct sbiret
sbi_remote_sfence_vma(unsigned long hart_mask, unsigned long hart_mask_base,
    unsigned long start_addr, unsigned long size)
{
	return SBI_CALL4(SBI_EID_RFENCE, SBI_FID_RFENCE_SFENCEVMA,
	    hart_mask, hart_mask_base, start_addr, size);
}

struct sbiret
sbi_remote_sfence_vma_asid(unsigned long hart_mask,
    unsigned long hart_mask_base, unsigned long start_addr,
    unsigned long size, unsigned long asid)
{
	return SBI_CALL5(SBI_EID_RFENCE, SBI_FID_RFENCE_SFENCEVMAASID,
	    hart_mask, hart_mask_base, start_addr, size, asid);
}

struct sbiret
sbi_remote_hfence_gvma_vmid(unsigned long hart_mask,
    unsigned long hart_mask_base, unsigned long start_addr,
    unsigned long size, unsigned long vmid)
{
	return SBI_CALL5(SBI_EID_RFENCE, SBI_FID_RFENCE_HFENCEGVMAVMID,
	    hart_mask, hart_mask_base, start_addr, size, vmid);
}

struct sbiret
sbi_remote_hfence_gvma(unsigned long hart_mask,
    unsigned long hart_mask_base, unsigned long start_addr,
    unsigned long size)
{
	return SBI_CALL4(SBI_EID_RFENCE, SBI_FID_RFENCE_HFENCEGVMA,
	    hart_mask, hart_mask_base, start_addr, size);
}

struct sbiret
sbi_remote_hfence_vvma_asid(unsigned long hart_mask,
    unsigned long hart_mask_base, unsigned long start_addr,
    unsigned long size, unsigned long asid)
{
	return SBI_CALL5(SBI_EID_RFENCE, SBI_FID_RFENCE_HFENCEVVMAASID,
	    hart_mask, hart_mask_base, start_addr, size, asid);
}

struct sbiret
sbi_remote_hfence_vvma(unsigned long hart_mask, unsigned long hart_mask_base,
    unsigned long start_addr, unsigned long size)
{
	return SBI_CALL4(SBI_EID_RFENCE, SBI_FID_RFENCE_HFENCEVVMA,
	    hart_mask, hart_mask_base, start_addr, size);
}

struct sbiret
sbi_hart_start(unsigned long hartid, unsigned long start_addr,
    unsigned long opaque)
{
	return SBI_CALL3(SBI_EID_HSM, SBI_FID_HSM_START,
	    hartid, start_addr, opaque);
}

struct sbiret
sbi_hart_stop(void)
{
	return SBI_CALL0(SBI_EID_HSM, SBI_FID_HSM_STOP);
}

struct sbiret
sbi_hart_get_status(unsigned long hartid)
{
	return SBI_CALL1(SBI_EID_HSM, SBI_FID_HSM_GETSTATUS,
	    hartid);
}

struct sbiret
sbi_hart_suspend(uint32_t suspend_type, unsigned long resume_addr,
    unsigned long opaque)
{
	return SBI_CALL3(SBI_EID_HSM, SBI_FID_HSM_SUSPEND,
	    suspend_type, resume_addr, opaque);
}

struct sbiret
sbi_system_reset(uint32_t reset_type, uint32_t reset_reason)
{
	return SBI_CALL2(SBI_EID_SRST, SBI_FID_SRST_SYSTEMRESET,
	    reset_type, reset_reason);
}

struct sbiret
sbi_pmu_num_counters(void)
{
	return SBI_CALL0(SBI_EID_PMU, SBU_FID_PMU_GETCOUNTERS);
}

struct sbiret
sbi_pmu_counter_get_info(unsigned long counter_idx)
{
	return SBI_CALL1(SBI_EID_PMU, SBU_FID_PMU_COUNTERDETAILS,
	    counter_idx);
}


struct sbiret
sbi_pmu_counter_config_matching(unsigned long counter_idx_base,
    unsigned long counter_idx_mask, unsigned long config_flags,
    unsigned long event_idx, uint64_t event_data)
{
#ifdef _LP64
	return SBI_CALL5(SBI_EID_PMU, SBU_FID_PMU_CONFIGCOUNTER,
	    counter_idx_base, counter_idx_mask, config_flags, event_idx,
	    event_data);
#else
	return SBI_CALL6(SBI_EID_PMU, SBU_FID_PMU_CONFIGCOUNTER,
	    counter_idx_base, counter_idx_mask, config_flags, event_idx,
	    event_data, event_data >> 32);
#endif
}

struct sbiret
sbi_pmu_counter_start(unsigned long counter_idx_base,
    unsigned long counter_idx_mask, unsigned long start_flags,
    uint64_t initial_value)
{
#ifdef _LP64
	return SBI_CALL4(SBI_EID_PMU, SBU_FID_PMU_STARTCOUNTERS,
	    counter_idx_base, counter_idx_mask, start_flags,
	    initial_value);
#else
	return SBI_CALL5(SBI_EID_PMU, SBU_FID_PMU_STARTCOUNTERS,
	    counter_idx_base, counter_idx_mask, start_flags,
	    initial_value, initial_value >> 32);
#endif
}

struct sbiret
sbi_pmu_counter_stop(unsigned long counter_idx_base,
    unsigned long counter_idx_mask, unsigned long stop_flags)
{
	return SBI_CALL3(SBI_EID_PMU, SBU_FID_PMU_STOPCOUNTERS,
	    counter_idx_base, counter_idx_mask, stop_flags);
}

struct sbiret
sbi_pmu_counter_fw_read(unsigned long counter_idx)
{
	return SBI_CALL1(SBI_EID_PMU, SBU_FID_PMU_READCOUNTER,
	    counter_idx);
}
