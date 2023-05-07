/*	$NetBSD: sbi.h,v 1.1 2023/05/07 12:41:48 skrll Exp $	*/

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

#ifndef _RISCV_SBI_H_
#define _RISCV_SBI_H_

#include <sys/types.h>

struct sbiret {
	long error;
	long value;
};

#define	SBI_SUCCESS			 0
#define	SBI_ERR_FAILED			-1
#define	SBI_ERR_NOT_SUPPORTED		-2
#define	SBI_ERR_INVALID_PARAM		-3
#define	SBI_ERR_DENIED			-4
#define	SBI_ERR_INVALID_ADDRESS		-5
#define	SBI_ERR_ALREADY_AVAILABLE	-6
#define	SBI_ERR_ALREADY_STARTED		-7
#define	SBI_ERR_ALREADY_STOPPED		-8

#define	SBI_EID_BASE		      0x10
#define	SBI_EID_TIMER		0x54494D45	// "TIME"
#define	SBI_EID_IPI		0x00735049	//  "sPI"
#define	SBI_EID_RFENCE		0x52464E43	// "RFNC"
#define	SBI_EID_HSM		0x0048534D	//  "HSM"
#define	SBI_EID_SRST		0x53525354	// "SRST"
#define	SBI_EID_PMU		0x00504D55	//  "PMU"

/*
| Function Name            | SBI Version | FID | EID
| sbi_get_sbi_spec_version | 0.2         |   0 | 0x10
| sbi_get_sbi_impl_id      | 0.2         |   1 | 0x10
| sbi_get_sbi_impl_version | 0.2         |   2 | 0x10
| sbi_probe_extension      | 0.2         |   3 | 0x10
| sbi_get_mvendorid        | 0.2         |   4 | 0x10
| sbi_get_marchid          | 0.2         |   5 | 0x10
| sbi_get_mimpid           | 0.2         |   6 | 0x10
*/

#define SBI_FID_BASE_GETSPECVERSION	0
#define SBI_FID_BASE_GETIMPLID		1
#define SBI_FID_BASE_GETIMPLVERSION	2
#define SBI_FID_BASE_PROBEEXTENTION	3
#define SBI_FID_BASE_GETMVENDORID	4
#define SBI_FID_BASE_GETMARCHID		5
#define SBI_FID_BASE_GETMIMPID		6

struct sbiret sbi_get_spec_version(void);
struct sbiret sbi_get_impl_id(void);
struct sbiret sbi_get_impl_version(void);
struct sbiret sbi_probe_extension(long extension_id);
struct sbiret sbi_get_mvendorid(void);
struct sbiret sbi_get_marchid(void);
struct sbiret sbi_get_mimpid(void);


/*
| Implementation ID | Name
| 0                 | Berkeley Boot Loader (BBL)
| 1                 | OpenSBI
| 2                 | Xvisor
| 3                 | KVM
| 4                 | RustSBI
| 5                 | Diosix
*/

#define	SBI_IMPLID_BERKELEY		0
#define	SBI_IMPLID_OPENSBI		1
#define	SBI_IMPLID_XVISOR		2
#define	SBI_IMPLID_KVM			3
#define	SBI_IMPLID_RUSTSBI		4
#define	SBI_IMPLID_DIOSIX		5


#define SBI_FID_TIMER_SET		0
struct sbiret sbi_set_timer(uint64_t stime_value);

#define SBI_FID_IPI_SEND		0
struct sbiret sbi_send_ipi(unsigned long hart_mask,
                           unsigned long hart_mask_base);

#define SBI_FID_RFENCE_FENCEI		0
#define SBI_FID_RFENCE_SFENCEVMA	1
#define	SBI_FID_RFENCE_SFENCEVMAASID	2
#define	SBI_FID_RFENCE_HFENCEGVMAVMID	3
#define	SBI_FID_RFENCE_HFENCEGVMA	4
#define	SBI_FID_RFENCE_HFENCEVVMAASID	5
#define	SBI_FID_RFENCE_HFENCEVVMA	6


struct sbiret sbi_remote_fence_i(unsigned long hart_mask,
                                 unsigned long hart_mask_base);
struct sbiret sbi_remote_sfence_vma(unsigned long hart_mask,
                                    unsigned long hart_mask_base,
                                    unsigned long start_addr,
                                    unsigned long size);
struct sbiret sbi_remote_sfence_vma_asid(unsigned long hart_mask,
                                         unsigned long hart_mask_base,
                                         unsigned long start_addr,
                                         unsigned long size,
                                         unsigned long asid);
struct sbiret sbi_remote_hfence_gvma_vmid(unsigned long hart_mask,
                                          unsigned long hart_mask_base,
                                          unsigned long start_addr,
                                          unsigned long size,
                                          unsigned long vmid);
struct sbiret sbi_remote_hfence_gvma(unsigned long hart_mask,
                                     unsigned long hart_mask_base,
                                     unsigned long start_addr,
                                     unsigned long size);
struct sbiret sbi_remote_hfence_vvma_asid(unsigned long hart_mask,
                                          unsigned long hart_mask_base,
                                          unsigned long start_addr,
                                          unsigned long size,
                                          unsigned long asid);
struct sbiret sbi_remote_hfence_vvma(unsigned long hart_mask,
                                     unsigned long hart_mask_base,
                                     unsigned long start_addr,
                                     unsigned long size);

#define	SBI_FID_HSM_START		0
#define	SBI_FID_HSM_STOP		1
#define	SBI_FID_HSM_GETSTATUS		2
#define	SBI_FID_HSM_SUSPEND		3

struct sbiret sbi_hart_start(unsigned long hartid,
                             unsigned long start_addr,
                             unsigned long opaque);
struct sbiret sbi_hart_stop(void);
struct sbiret sbi_hart_get_status(unsigned long hartid);
struct sbiret sbi_hart_suspend(uint32_t suspend_type,
                               unsigned long resume_addr,
                               unsigned long opaque);

#define SBI_HART_STARTED		0
#define SBI_HART_STOPPED		1
#define SBI_HART_STARTPENDING		2
#define SBI_HART_STOPPENDING		3
#define SBI_HART_SUSPENDED		4
#define SBI_HART_SUSPENDPENDING		5
#define SBI_HART_RESUMEPENDING		6


#define SBI_FID_SRST_SYSTEMRESET	0
struct sbiret sbi_system_reset(uint32_t reset_type, uint32_t reset_reason);

#define SBI_RESET_TYPE_SHUTDOWN		0
#define SBI_RESET_TYPE_COLDREBOOT	1
#define SBI_RESET_TYPE_WARNREBOOT	2

#define SBI_RESET_REASON_NONE		0
#define SBI_RESET_REASON_FAILURE	1



#define SBU_FID_PMU_GETCOUNTERS		0
#define SBU_FID_PMU_COUNTERDETAILS	1
#define SBU_FID_PMU_CONFIGCOUNTER	2
#define SBU_FID_PMU_STARTCOUNTERS	3
#define SBU_FID_PMU_STOPCOUNTERS	4
#define SBU_FID_PMU_READCOUNTER		5

#define	SBI_PMU_HW_NO_EVENT                 0	// Unused event because
						// ...`event_idx` cannot be zero
#define	SBI_PMU_HW_CPU_CYCLES               1	// Event for each CPU cycle
#define	SBI_PMU_HW_INSTRUCTIONS             2	// Event for each completed
						// ... instruction
#define	SBI_PMU_HW_CACHE_REFERENCES         3	// Event for cache hit
#define	SBI_PMU_HW_CACHE_MISSES             4	// Event for cache miss
#define	SBI_PMU_HW_BRANCH_INSTRUCTIONS      5	// Event for a branch instruction
#define	SBI_PMU_HW_BRANCH_MISSES            6	// Event for a branch misprediction
#define	SBI_PMU_HW_BUS_CYCLES               7	// Event for each BUS cycle
#define	SBI_PMU_HW_STALLED_CYCLES_FRONTEND  8	// Event for a stalled cycle in
						// ... microarchitecture frontend
#define	SBI_PMU_HW_STALLED_CYCLES_BACKEND   9	// Event for a stalled cycle in
						// ... microarchitecture backend
#define	SBI_PMU_HW_REF_CPU_CYCLES          10	// Event for each reference
						// ... CPU cycle

#define	SBI_PMU_HW_CACHE_L1D   0		// Level1 data cache event
#define	SBI_PMU_HW_CACHE_L1I   1		// Level1 instruction cache event
#define	SBI_PMU_HW_CACHE_LL    2		// Last level cache event
#define	SBI_PMU_HW_CACHE_DTLB  3		// Data TLB event
#define	SBI_PMU_HW_CACHE_ITLB  4		// Instruction TLB event
#define	SBI_PMU_HW_CACHE_BPU   5		// Branch predictor unit event
#define	SBI_PMU_HW_CACHE_NODE  6		// NUMA node cache event

#define	SBI_PMU_HW_CACHE_OP_READ      0		// Read cache line
#define	SBI_PMU_HW_CACHE_OP_WRITE     1		// Write cache line
#define	SBI_PMU_HW_CACHE_OP_PREFETCH  2		// Prefetch cache line

#define	SBI_PMU_HW_CACHE_RESULT_ACCESS 0	// Cache access
#define	SBI_PMU_HW_CACHE_RESULT_MISS   1	// Cache miss

#define	SBI_PMU_FW_MISALIGNED_LOAD            0	// Misaligned load trap event
#define	SBI_PMU_FW_MISALIGNED_STORE           1	// Misaligned store trap event
#define	SBI_PMU_FW_ACCESS_LOAD                2	// Load access trap event
#define	SBI_PMU_FW_ACCESS_STORE               3	// Store access trap event
#define	SBI_PMU_FW_ILLEGAL_INSN               4	// Illegal instruction trap event
#define	SBI_PMU_FW_SET_TIMER                  5	// Set timer event
#define	SBI_PMU_FW_IPI_SENT                   6	// Sent IPI to other HART event
#define	SBI_PMU_FW_IPI_RECEIVED               7	// Received IPI from other
                                                // ... HART event
#define	SBI_PMU_FW_FENCE_I_SENT               8	// Sent FENCE.I request to
                                                // ... other HART event
#define	SBI_PMU_FW_FENCE_I_RECEIVED           9	// Received FENCE.I request
                                                // ... from other HART event
#define	SBI_PMU_FW_SFENCE_VMA_SENT           10	// Sent SFENCE.VMA request
                                                // ... to other HART event
#define	SBI_PMU_FW_SFENCE_VMA_RECEIVED       11	// Received SFENCE.VMA request
                                                // ... from other HART event
#define	SBI_PMU_FW_SFENCE_VMA_ASID_SENT      12	// Sent SFENCE.VMA with ASID
                                                // ... request to other HART event
#define	SBI_PMU_FW_SFENCE_VMA_ASID_RECEIVED  13	// Received SFENCE.VMA with ASID
                                                // ... request from other HART event
#define	SBI_PMU_FW_HFENCE_GVMA_SENT          14	// Sent HFENCE.GVMA request to
                                                // ... other HART event
#define	SBI_PMU_FW_HFENCE_GVMA_RECEIVED      15	// Received HFENCE.GVMA request
                                                // ... from other HART event
#define	SBI_PMU_FW_HFENCE_GVMA_VMID_SENT     16	// Sent HFENCE.GVMA with VMID
                                                // ... request to other HART event
#define	SBI_PMU_FW_HFENCE_GVMA_VMID_RECEIVED 17	// Received HFENCE.GVMA with VMID
                                                // ... request from other HART event
#define	SBI_PMU_FW_HFENCE_VVMA_SENT          18	// Sent HFENCE.VVMA request to
                                                // ... other HART event
#define	SBI_PMU_FW_HFENCE_VVMA_RECEIVED      19	// Received HFENCE.VVMA request
                                                // ... from other HART event
#define	SBI_PMU_FW_HFENCE_VVMA_ASID_SENT     20	// Sent HFENCE.VVMA with ASID
                                                // ... request to other HART event
#define	SBI_PMU_FW_HFENCE_VVMA_ASID_RECEIVED 21	// Received HFENCE.VVMA with ASID
                                                // ... request from other HART event


struct sbiret sbi_pmu_num_counters(void);
struct sbiret sbi_pmu_counter_get_info(unsigned long counter_idx);

struct sbiret sbi_pmu_counter_config_matching(unsigned long counter_idx_base,
					      unsigned long counter_idx_mask,
					      unsigned long config_flags,
					      unsigned long event_idx,
					      uint64_t event_data);
struct sbiret sbi_pmu_counter_start(unsigned long counter_idx_base,
				    unsigned long counter_idx_mask,
				    unsigned long start_flags,
				    uint64_t initial_value);
struct sbiret sbi_pmu_counter_stop(unsigned long counter_idx_base,
				    unsigned long counter_idx_mask,
				    unsigned long stop_flags);
struct sbiret sbi_pmu_counter_fw_read(unsigned long counter_idx);



static inline struct sbiret
sbi_call(int eid, int fid,
    unsigned long arg0, unsigned long arg1, unsigned long arg2,
    unsigned long arg3, unsigned long arg4, unsigned long arg5)
{
	struct sbiret ret;

	register register_t _a7 __asm ("a7") = eid;
	register register_t _a6 __asm ("a6") = fid;

	register register_t _a0 __asm ("a0") = arg0;
	register register_t _a1 __asm ("a1") = arg1;
	register register_t _a2 __asm ("a2") = arg2;
	register register_t _a3 __asm ("a3") = arg3;
	register register_t _a4 __asm ("a4") = arg4;
	register register_t _a5 __asm ("a5") = arg5;

	__asm __volatile (
		"ecall"
		: "+r" (_a0), "+r" (_a1)
		: "r" (_a2), "r" (_a3), "r" (_a4), "r" (_a5), "r" (_a6), "r" (_a7)
		: "memory");
	ret.error = _a0;
	ret.value = _a1;

	return ret;
}


#define SBI_CALL0(eid, fid)                         sbi_call(eid, fid,  0,  0,  0,  0,  0,   0)
#define SBI_CALL1(eid, fid, a0)                     sbi_call(eid, fid, a0,  0,  0,  0,  0,   0)
#define SBI_CALL2(eid, fid, a0, a1)                 sbi_call(eid, fid, a0, a1,  0,  0,  0,   0)
#define SBI_CALL3(eid, fid, a0, a1, a2)             sbi_call(eid, fid, a0, a1, a2,  0,  0,   0)
#define SBI_CALL4(eid, fid, a0, a1, a2, a3)         sbi_call(eid, fid, a0, a1, a2, a3,  0,   0)
#define SBI_CALL5(eid, fid, a0, a1, a2, a3, a4)     sbi_call(eid, fid, a0, a1, a2, a3, a4,   0)
#define SBI_CALL6(eid, fid, a0, a1, a2, a3, a4, a5) sbi_call(eid, fid, a0, a1, a2, a3, a4,  a5)



#if defined(__RVSBI_LEGACY)

#define SBI_LEGACY_SET_TIMER              0
#define SBI_LEGACY_CONSOLE_PUTCHAR        1
#define SBI_LEGACY_CONSOLE_GETCHAR        2
#define SBI_LEGACY_CLEAR_IPI              3
#define SBI_LEGACY_SEND_IPI               4
#define SBI_LEGACY_REMOTE_FENCE_I         5
#define SBI_LEGACY_REMOTE_SFENCE_VMA      6
#define SBI_LEGACY_REMOTE_SFENCE_VMA_ASID 7
#define SBI_LEGACY_SHUTDOWN               8

#define SBI_LEGACY_CALL0(eid)                         sbi_call(eid, 0,  0,  0,  0,  0,  0,  0)
#define SBI_LEGACY_CALL1(eid, a0)                     sbi_call(eid, 0, a0,  0,  0,  0,  0,  0)
#define SBI_LEGACY_CALL2(eid, a0, a1)                 sbi_call(eid, 0, a0, a1,  0,  0,  0,  0)
#define SBI_LEGACY_CALL3(eid, a0, a1, a2)             sbi_call(eid, 0, a0, a1, a2,  0,  0,  0)
#define SBI_LEGACY_CALL4(eid, a0, a1, a2, a3)         sbi_call(eid, 0, a0, a1, a2, a3,  0,  0)
#define SBI_LEGACY_CALL5(eid, a0, a1, a2, a3, a4)     sbi_call(eid, 0, a0, a1, a2, a3, a4,  0)


/*
 * void sbi_set_timer(uint64_t stime_value)
 */

static inline long
sbi_legacy_set_timer(uint64_t stime_value)
{
#ifdef _LP64
	struct sbiret ret = SBI_LEGACY_CALL1(SBI_LEGACY_SET_TIMER, stime_value);
#else
	struct sbiret ret = SBI_LEGACY_CALL2(SBI_LEGACY_SET_TIMER,
	    stime_value, stime_value >> 32);
#endif
	return ret.error;
}

/*
 * void sbi_console_putchar(int ch)
 */

static inline long
sbi_legacy_console_putchar(int c) {
	struct sbiret ret = SBI_LEGACY_CALL1(SBI_LEGACY_CONSOLE_PUTCHAR, c);

	return ret.error;
}

/*
 * long sbi_console_getchar(void)
 */
static inline long
sbi_legacy_console_getchar(void) {
	struct sbiret ret = SBI_LEGACY_CALL0(SBI_LEGACY_CONSOLE_GETCHAR);

	return ret.error;
}

/*
 * long sbi_clear_ipi(void)
 */
static inline long
sbi_legacy_clear_ipi(void) {
	struct sbiret ret = SBI_LEGACY_CALL0(SBI_LEGACY_CLEAR_IPI);

	return ret.error;
}


/*
 * hart_mask is a virtual address that points to a bit-vector of harts. The
 * bit vector is represented as a sequence of unsigned longs whose length
 * equals the number of harts in the system divided by the number of bits
 * in an unsigned long, rounded up to the next integer.
*/

/*
 * void sbi_send_ipi(const unsigned long *hart_mask)
 */
static inline long
sbi_legacy_send_ipi(const unsigned long *hart_mask) {
	struct sbiret ret = SBI_LEGACY_CALL1(SBI_LEGACY_SEND_IPI,
	    (unsigned long)hart_mask);

	return ret.error;
}

/*
 * long sbi_remote_fence_i(const unsigned long *hart_mask)
 */
static inline long
sbi_legacy_remote_fence_i(const unsigned long *hart_mask) {
	struct sbiret ret = SBI_LEGACY_CALL1(SBI_LEGACY_REMOTE_FENCE_I,
	    (unsigned long)hart_mask);

	return ret.error;
}

/*
 * long sbi_remote_sfence_vma(const unsigned long *hart_mask,
 *                            unsigned long start,
 *                            unsigned long size)
 */
static inline long
sbi_legacy_remote_sfence_vma(const unsigned long *hart_mask,
    unsigned long start, unsigned long size)
{
	struct sbiret ret = SBI_LEGACY_CALL3(SBI_LEGACY_REMOTE_SFENCE_VMA,
	    (unsigned long)hart_mask, start, size);

	return ret.error;
}

/*
 * long sbi_remote_sfence_vma_asid(const unsigned long *hart_mask,
 *                                 unsigned long start,
 *                                 unsigned long size,
 *                                 unsigned long asid)
 */
static inline long
sbi_legacy_remote_sfence_vma_asid(const unsigned long *hart_mask,
    unsigned long start, unsigned long size, unsigned long asid)
{
	struct sbiret ret = SBI_LEGACY_CALL4(SBI_LEGACY_REMOTE_SFENCE_VMA_ASID,
	    (unsigned long)hart_mask, start, size, asid);

	return ret.error;
}

/*
 * void sbi_shutdown(void)
 */
static inline void
sbi_legacy_shutdown(void) {
	SBI_LEGACY_CALL0(SBI_LEGACY_SHUTDOWN);
}

#endif


#endif /* _RISCV_SBI_H_ */
