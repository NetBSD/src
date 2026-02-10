/* Copyright (C) 2024-2025 Free Software Foundation, Inc.
   Contributed by Oracle.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#ifndef _HWC_ARM_NEOVERSEN1_H_
#define _HWC_ARM_NEOVERSEN1_H_

#define I(nm, event, mtr) INIT_HWC(nm, mtr, (event), PERF_TYPE_RAW)

static Hwcentry arm_neoverse_n1_list[] = {
  HWC_GENERIC
/* bus: */
  { I("bus_access", 0x19,
    STXT("Attributable Bus access. Unit: armv8_pmuv3_0")) },
  { I("bus_access_rd", 0x60, STXT("Bus access read")) },
  { I("bus_access_wr", 0x61, STXT("Bus access write")) },
  { I("bus_cycles", 0x1d, STXT("Bus cycle. Unit: armv8_pmuv3_0")) },
/* exception: */
  { I("exc_dabort", 0x84, STXT("Exception taken, Data Abort and SError")) },
  { I("exc_fiq", 0x87, STXT("Exception taken, FIQ")) },
  { I("exc_hvc", 0x8a, STXT("Exception taken, Hypervisor Call")) },
  { I("exc_irq", 0x86, STXT("Exception taken, IRQ")) },
  { I("exc_pabort", 0x83, STXT("Exception taken, Instruction Abort")) },
  { I("exc_return", 0xa,
    STXT("Instruction architecturally executed, condition check pass, exception"
      " return. Unit: armv8_pmuv3_0")) },
  { I("exc_smc", 0x88, STXT("Exception taken, Secure Monitor Call")) },
  { I("exc_svc", 0x82, STXT("Exception taken, Supervisor Call")) },
  { I("exc_taken", 0x9, STXT("Exception taken. Unit: armv8_pmuv3_0")) },
  { I("exc_trap_dabort", 0x8c,
    STXT("Exception taken, Data Abort or SError not taken locally")) },
  { I("exc_trap_fiq", 0x8f, STXT("Exception taken, FIQ not taken locally")) },
  { I("exc_trap_irq", 0x8e, STXT("Exception taken, IRQ not taken locally")) },
  { I("exc_trap_other", 0x8d,
    STXT("Exception taken, Other traps not taken locally")) },
  { I("exc_trap_pabort", 0x8b,
    STXT("Exception taken, Instruction Abort not taken locally")) },
  { I("exc_undef", 0x81, STXT("Exception taken, Other synchronous")) },
/* general: */
  { I("cpu_cycles", 0x11, STXT("Cycle. Unit: armv8_pmuv3_0")) },
/* l1d_cache: */
  { I("l1d_cache", 0x4,
    STXT("Level 1 data cache access. Unit: armv8_pmuv3_0")) },
  { I("l1d_cache_inval", 0x48, STXT("L1D cache invalidate")) },
  { I("l1d_cache_rd", 0x40, STXT("L1D cache access, read")) },
  { I("l1d_cache_refill", 0x3,
    STXT("Level 1 data cache refill. Unit: armv8_pmuv3_0")) },
  { I("l1d_cache_refill_inner", 0x44, STXT("L1D cache refill, inner")) },
  { I("l1d_cache_refill_outer", 0x45, STXT("L1D cache refill, outer")) },
  { I("l1d_cache_refill_rd", 0x42, STXT("L1D cache refill, read")) },
  { I("l1d_cache_refill_wr", 0x43, STXT("L1D cache refill, write")) },
  { I("l1d_cache_wb", 0x15,
    STXT("Attributable Level 1 data cache write-back. Unit: armv8_pmuv3_0")) },
  { I("l1d_cache_wb_clean", 0x47,
    STXT("L1D cache Write-Back, cleaning and coherency")) },
  { I("l1d_cache_wb_victim", 0x46, STXT("L1D cache Write-Back, victim")) },
  { I("l1d_cache_wr", 0x41, STXT("L1D cache access, write")) },
/* l1i_cache: */
  { I("l1i_cache", 0x14,
    STXT("Attributable Level 1 instruction cache access. Unit: armv8_pmuv3_0")) },
  { I("l1i_cache_refill", 0x1,
    STXT("Level 1 instruction cache refill. Unit: armv8_pmuv3_0")) },
/* l2_cache: */
  { I("l2d_cache", 0x16,
    STXT("Level 2 data cache access. Unit: armv8_pmuv3_0")) },
  { I("l2d_cache_allocate", 0x20,
    STXT("Attributable Level 2 data cache allocation without refill. Unit:"
      " armv8_pmuv3_0")) },
  { I("l2d_cache_inval", 0x58, STXT("L2D cache invalidate")) },
  { I("l2d_cache_rd", 0x50, STXT("L2D cache access, read")) },
  { I("l2d_cache_refill", 0x17,
    STXT("Level 2 data refill. Unit: armv8_pmuv3_0")) },
  { I("l2d_cache_refill_rd", 0x52, STXT("L2D cache refill, read")) },
  { I("l2d_cache_refill_wr", 0x53, STXT("L2D cache refill, write")) },
  { I("l2d_cache_wb", 0x18,
    STXT("Attributable Level 2 data cache write-back. Unit: armv8_pmuv3_0")) },
  { I("l2d_cache_wb_clean", 0x57,
    STXT("L2D cache Write-Back, cleaning and coherency")) },
  { I("l2d_cache_wb_victim", 0x56, STXT("L2D cache Write-Back, victim")) },
  { I("l2d_cache_wr", 0x51, STXT("L2D cache access, write")) },
/* l3_cache: */
  { I("l3d_cache", 0x2b,
    STXT("Attributable Level 3 data cache access. Unit: armv8_pmuv3_0")) },
  { I("l3d_cache_allocate", 0x29,
    STXT("Attributable Level 3 data cache allocation without refill. Unit:"
      " armv8_pmuv3_0")) },
  { I("l3d_cache_rd", 0xa0,
    STXT("Attributable Level 3 data or unified cache access, read")) },
  { I("l3d_cache_refill", 0x2a,
    STXT("Attributable Level 3 data cache refill. Unit: armv8_pmuv3_0")) },
/* ll_cache: */
  { I("ll_cache_miss_rd", 0x37, STXT("Last level cache miss, read")) },
  { I("ll_cache_rd", 0x36,
    STXT("Attributable Last level cache memory read")) },
/* memory: */
  { I("mem_access", 0x13, STXT("Data memory access. Unit: armv8_pmuv3_0")) },
  { I("mem_access_rd", 0x66, STXT("Data memory access, read")) },
  { I("mem_access_wr", 0x67, STXT("Data memory access, write")) },
  { I("memory_error", 0x1a, STXT("Local memory error. Unit: armv8_pmuv3_0")) },
  { I("remote_access", 0x31,
    STXT("Access to another socket in a multi-socket system")) },
/* retired: */
  { I("br_mis_pred_retired", 0x22,
    STXT("Instruction architecturally executed, mispredicted branch. Unit:"
      " armv8_pmuv3_0")) },
  { I("br_retired", 0x21,
    STXT("Instruction architecturally executed, branch. Unit: armv8_pmuv3_0")) },
  { I("cid_write_retired", 0xb,
    STXT("Instruction architecturally executed, condition code check pass, write"
      " to CONTEXTIDR. Unit: armv8_pmuv3_0")) },
  { I("inst_retired", 0x8,
    STXT("Instruction architecturally executed. Unit: armv8_pmuv3_0")) },
  { I("sw_incr", 0,
    STXT("Instruction architecturally executed, Condition code check pass,"
      " software increment. Unit: armv8_pmuv3_0")) },
  { I("ttbr_write_retired", 0x1c,
    STXT("Instruction architecturally executed, Condition code check pass, write"
      " to TTBR. Unit: armv8_pmuv3_0")) },
/* spe: */
  { I("sample_collision", 0x4003,
    STXT("Sample collided with previous sample. Unit: armv8_pmuv3_0")) },
  { I("sample_feed", 0x4001, STXT("Sample Taken. Unit: armv8_pmuv3_0")) },
  { I("sample_filtrate", 0x4002,
    STXT("Sample Taken and not removed by filtering. Unit: armv8_pmuv3_0")) },
  { I("sample_pop", 0x4000, STXT("Sample Population. Unit: armv8_pmuv3_0")) },
/* spec_operation: */
  { I("ase_spec", 0x74,
    STXT("Operation speculatively executed, Advanced SIMD instruction")) },
  { I("br_immed_spec", 0x78,
    STXT("Branch speculatively executed, immediate branch")) },
  { I("br_indirect_spec", 0x7a,
    STXT("Branch speculatively executed, indirect branch")) },
  { I("br_mis_pred", 0x10,
    STXT("Mispredicted or not predicted branch speculatively executed. Unit:"
      " armv8_pmuv3_0")) },
  { I("br_pred", 0x12,
    STXT("Predictable branch speculatively executed. Unit: armv8_pmuv3_0")) },
  { I("br_return_spec", 0x79,
    STXT("Branch speculatively executed, procedure return")) },
  { I("crypto_spec", 0x77,
    STXT("Operation speculatively executed, Cryptographic instruction")) },
  { I("dmb_spec", 0x7e, STXT("Barrier speculatively executed, DMB")) },
  { I("dp_spec", 0x73,
    STXT("Operation speculatively executed, integer data processing")) },
  { I("dsb_spec", 0x7d, STXT("Barrier speculatively executed, DSB")) },
  { I("inst_spec", 0x1b,
    STXT("Operation speculatively executed. Unit: armv8_pmuv3_0")) },
  { I("isb_spec", 0x7c, STXT("Barrier speculatively executed, ISB")) },
  { I("ld_spec", 0x70, STXT("Operation speculatively executed, load")) },
  { I("ldrex_spec", 0x6c,
    STXT("Exclusive operation speculatively executed, LDREX or LDX")) },
  { I("pc_write_spec", 0x76,
    STXT("Operation speculatively executed, software change of the PC")) },
  { I("rc_ld_spec", 0x90,
    STXT("Release consistency operation speculatively executed, Load-Acquire")) },
  { I("rc_st_spec", 0x91,
    STXT("Release consistency operation speculatively executed, Store-Release")) },
  { I("st_spec", 0x71, STXT("Operation speculatively executed, store")) },
  { I("strex_fail_spec", 0x6e,
    STXT("Exclusive operation speculatively executed, STREX or STX fail")) },
  { I("strex_pass_spec", 0x6d,
    STXT("Exclusive operation speculatively executed, STREX or STX pass")) },
  { I("strex_spec", 0x6f,
    STXT("Exclusive operation speculatively executed, STREX or STX")) },
  { I("unaligned_ld_spec", 0x68, STXT("Unaligned access, read")) },
  { I("unaligned_ldst_spec", 0x6a, STXT("Unaligned access")) },
  { I("unaligned_st_spec", 0x69, STXT("Unaligned access, write")) },
  { I("vfp_spec", 0x75,
    STXT("Operation speculatively executed, floating-point instruction")) },
/* stall: */
  { I("stall_backend", 0x24,
    STXT("No operation issued due to the backend. Unit: armv8_pmuv3_0")) },
  { I("stall_frontend", 0x23,
    STXT("No operation issued because of the frontend. Unit: armv8_pmuv3_0")) },
/* tlb: */
  { I("dtlb_walk", 0x34,
    STXT("Access to data TLB causes a translation table walk")) },
  { I("itlb_walk", 0x35,
    STXT("Access to instruction TLB that causes a translation table walk")) },
  { I("l1d_tlb", 0x25,
    STXT("Attributable Level 1 data or unified TLB access. Unit: armv8_pmuv3_0")) },
  { I("l1d_tlb_rd", 0x4e, STXT("L1D tlb access, read")) },
  { I("l1d_tlb_refill", 0x5,
    STXT("Attributable Level 1 data TLB refill. Unit: armv8_pmuv3_0")) },
  { I("l1d_tlb_refill_rd", 0x4c, STXT("L1D tlb refill, read")) },
  { I("l1d_tlb_refill_wr", 0x4d, STXT("L1D tlb refill, write")) },
  { I("l1d_tlb_wr", 0x4f, STXT("L1D tlb access, write")) },
  { I("l1i_tlb", 0x26,
    STXT("Attributable Level 1 instruction TLB access. Unit: armv8_pmuv3_0")) },
  { I("l1i_tlb_refill", 0x2,
    STXT("Attributable Level 1 instruction TLB refill. Unit: armv8_pmuv3_0")) },
  { I("l2d_tlb", 0x2f,
    STXT("Attributable Level 2 data or unified TLB access. Unit: armv8_pmuv3_0")) },
  { I("l2d_tlb_rd", 0x5e, STXT("L2D cache access, read")) },
  { I("l2d_tlb_refill", 0x2d,
    STXT("Attributable Level 2 data TLB refill. Unit: armv8_pmuv3_0")) },
  { I("l2d_tlb_refill_rd", 0x5c, STXT("L2D cache refill, read")) },
  { I("l2d_tlb_refill_wr", 0x5d, STXT("L2D cache refill, write")) },
  { I("l2d_tlb_wr", 0x5f, STXT("L2D cache access, write")) },
  { NULL, NULL, 0, NULL }
};

#undef I
#endif
