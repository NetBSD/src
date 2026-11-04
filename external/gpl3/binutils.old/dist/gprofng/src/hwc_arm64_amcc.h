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

#ifndef _HWC_ARM64_AMCC_H
#define _HWC_ARM64_AMCC_H

#define I(nm, event, mtr) INIT_HWC(nm, mtr, (event), PERF_TYPE_RAW)

static Hwcentry arm64_amcc_list[] = {
  HWC_GENERIC
/* branch: */
  { I("br_immed_spec", 0x78,
    STXT("Branch speculatively executed, immediate branch")) },
  { I("br_indirect_spec", 0x7a,
    STXT("Branch speculatively executed, indirect branch")) },
  { I("br_mis_pred", 0x10, STXT("Branch mispredicted")) },
  { I("br_pred", 0x12, STXT("Predictable branch")) },
  { I("br_return_spec", 0x79,
    STXT("Branch speculatively executed, procedure return")) },
/* bus: */
  { I("bus_access", 0x19, STXT("Attributable Bus access")) },
  { I("bus_access_normal", 0x64, STXT("Bus access, Normal")) },
  { I("bus_access_not_shared", 0x63,
    STXT("Bus access, not Normal, Cacheable, Shareable")) },
  { I("bus_access_periph", 0x65, STXT("Bus access, peripheral")) },
  { I("bus_access_shared", 0x62,
    STXT("Bus access, Normal, Cacheable, Shareable")) },
/* cache: */
  { I("btb_mis_pred", 0x102, STXT("BTB misprediction")) },
  { I("dtb_miss", 0x104, STXT("DTB miss")) },
  { I("itb_miss", 0x103, STXT("ITB miss")) },
  { I("l1_stage2_tlb_refill", 0x111, STXT("L1 stage 2 TLB refill")) },
  { I("l1d_cache", 0x4, STXT("Level 1 data cache access")) },
  { I("l1d_cache_inval", 0x48, STXT("L1D cache invalidate")) },
  { I("l1d_cache_late_miss", 0x105, STXT("L1D cache late miss")) },
  { I("l1d_cache_prefetch", 0x106, STXT("L1D cache prefetch")) },
  { I("l1d_cache_refill", 0x3, STXT("Level 1 data cache refill")) },
  { I("l1d_tlb", 0x25, STXT("L1D TLB access")) },
  { I("l1d_tlb_refill", 0x5, STXT("Attributable Level 1 data TLB refill")) },
  { I("l1i_cache", 0x14,
    STXT("Attributable Level 1 instruction cache access")) },
  { I("l1i_cache_refill", 0x1, STXT("Level 1 instruction cache refill")) },
  { I("l1i_tlb", 0x26, STXT("Attributable Level 1 instruction TLB access")) },
  { I("l1i_tlb_refill", 0x2,
    STXT("Attributable Level 1 instruction TLB refill")) },
  { I("l2d_cache", 0x16, STXT("Level 2 data cache access")) },
  { I("l2d_cache_inval", 0x58, STXT("L2D cache invalidate")) },
  { I("l2d_cache_prefetch", 0x107, STXT("L2D cache prefetch")) },
  { I("l2d_cache_rd", 0x50, STXT("L2D cache access, read")) },
  { I("l2d_cache_refill", 0x17, STXT("Level 2 data refill")) },
  { I("l2d_cache_refill_rd", 0x52, STXT("L2D cache refill, read")) },
  { I("l2d_cache_refill_wr", 0x53, STXT("L2D cache refill, write")) },
  { I("l2d_cache_wb", 0x18,
    STXT("Attributable Level 2 data cache write-back")) },
  { I("l2d_cache_wb_clean", 0x57,
    STXT("L2D cache Write-Back, cleaning and coherency")) },
  { I("l2d_cache_wb_victim", 0x56, STXT("L2D cache Write-Back, victim")) },
  { I("l2d_cache_wr", 0x51, STXT("L2D cache access, write")) },
  { I("l2d_tlb_access", 0x34, STXT("L2D TLB access")) },
  { I("l2i_tlb_access", 0x35, STXT("L2I TLB access")) },
  { I("page_walk_l0_stage1_hit", 0x112, STXT("Page walk, L0 stage-1 hit")) },
  { I("page_walk_l1_stage1_hit", 0x113, STXT("Page walk, L1 stage-1 hit")) },
  { I("page_walk_l1_stage2_hit", 0x115, STXT("Page walk, L1 stage-2 hit")) },
  { I("page_walk_l2_stage1_hit", 0x114, STXT("Page walk, L2 stage-1 hit")) },
  { I("page_walk_l2_stage2_hit", 0x116, STXT("Page walk, L2 stage-2 hit")) },
/* clock: */
  { I("cpu_cycles", 0x11, STXT("Cycle")) },
  { I("fsu_clock_off_cycles", 0x101, STXT("FSU clocking gated off cycle")) },
  { I("wait_cycles", 0x110, STXT("Wait state cycle")) },
/* core imp def: */
  { I("bus_access_rd", 0x60, STXT("Bus access read")) },
  { I("bus_access_wr", 0x61, STXT("Bus access write")) },
  { I("l1d_cache_rd", 0x40, STXT("L1D cache access, read")) },
  { I("l1d_cache_refill_rd", 0x42, STXT("L1D cache refill, read")) },
  { I("l1d_cache_refill_wr", 0x43, STXT("L1D cache refill, write")) },
  { I("l1d_cache_wr", 0x41, STXT("L1D cache access, write")) },
  { I("l1d_tlb_rd", 0x4e, STXT("L1D tlb access, read")) },
  { I("l1d_tlb_refill_rd", 0x4c, STXT("L1D tlb refill, read")) },
  { I("l1d_tlb_refill_wr", 0x4d, STXT("L1D tlb refill, write")) },
  { I("l1d_tlb_wr", 0x4f, STXT("L1D tlb access, write")) },
/* exception: */
  { I("exc_dabort", 0x84, STXT("Exception taken, Data Abort and SError")) },
  { I("exc_fiq", 0x87, STXT("Exception taken, FIQ")) },
  { I("exc_hvc", 0x8a, STXT("Exception taken, Hypervisor Call")) },
  { I("exc_irq", 0x86, STXT("Exception taken, IRQ")) },
  { I("exc_pabort", 0x83, STXT("Exception taken, Instruction Abort")) },
  { I("exc_return", 0xa,
    STXT("Instruction architecturally executed, condition check pass, exception"
      " return")) },
  { I("exc_svc", 0x82, STXT("Exception taken, Supervisor Call")) },
  { I("exc_taken", 0x9, STXT("Exception taken")) },
  { I("exc_trap_dabort", 0x8c,
    STXT("Exception taken, Data Abort or SError not taken locally")) },
  { I("exc_trap_fiq", 0x8f, STXT("Exception taken, FIQ not taken locally")) },
  { I("exc_trap_irq", 0x8e, STXT("Exception taken, IRQ not taken locally")) },
  { I("exc_trap_other", 0x8d,
    STXT("Exception taken, Other traps not taken locally")) },
  { I("exc_trap_pabort", 0x8b,
    STXT("Exception taken, Instruction Abort not taken locally")) },
  { I("exc_undef", 0x81, STXT("Exception taken, Other synchronous")) },
/* instruction: */
  { I("ase_spec", 0x74,
    STXT("Operation speculatively executed, Advanced SIMD instruction")) },
  { I("br_mis_pred_retired", 0x22,
    STXT("Instruction architecturally executed, mispredicted branch")) },
  { I("br_retired", 0x21,
    STXT("Instruction architecturally executed, branch")) },
  { I("cid_write_retired", 0xb, STXT("Write to CONTEXTIDR")) },
  { I("crypto_spec", 0x77,
    STXT("Operation speculatively executed, Cryptographic instruction")) },
  { I("dmb_spec", 0x7e, STXT("Barrier speculatively executed, DMB")) },
  { I("dp_spec", 0x73,
    STXT("Operation speculatively executed, integer data processing")) },
  { I("dsb_spec", 0x7d, STXT("Barrier speculatively executed, DSB")) },
  { I("inst_retired", 0x8, STXT("Instruction architecturally executed")) },
  { I("inst_spec", 0x1b, STXT("Operation speculatively executed")) },
  { I("isb_spec", 0x7c, STXT("Barrier speculatively executed, ISB")) },
  { I("ld_spec", 0x70, STXT("Operation speculatively executed, load")) },
  { I("ldst_spec", 0x72,
    STXT("Operation speculatively executed, load or store")) },
  { I("nop_spec", 0x100, STXT("Speculatively executed, NOP")) },
  { I("pc_write_spec", 0x76,
    STXT("Operation speculatively executed, software change of the PC")) },
  { I("rc_ld_spec", 0x90,
    STXT("Release consistency operation speculatively executed, Load-Acquire")) },
  { I("rc_st_spec", 0x91,
    STXT("Release consistency operation speculatively executed, Store-Release")) },
  { I("st_spec", 0x71, STXT("Operation speculatively executed, store")) },
  { I("sw_incr", 0, STXT("Software increment")) },
  { I("ttbr_write_retired", 0x1c,
    STXT("Instruction architecturally executed, Condition code check pass, write"
      " to TTBR")) },
  { I("vfp_spec", 0x75,
    STXT("Operation speculatively executed, floating-point instruction")) },
/* intrinsic: */
  { I("ldrex_spec", 0x6c,
    STXT("Exclusive operation speculatively executed, LDREX or LDX")) },
  { I("strex_fail_spec", 0x6e,
    STXT("Exclusive operation speculatively executed, STREX or STX fail")) },
  { I("strex_pass_spec", 0x6d,
    STXT("Exclusive operation speculatively executed, STREX or STX pass")) },
  { I("strex_spec", 0x6f,
    STXT("Exclusive operation speculatively executed, STREX or STX")) },
/* memory: */
  { I("mem_access", 0x13, STXT("Data memory access")) },
  { I("mem_access_rd", 0x66, STXT("Data memory access, read")) },
  { I("mem_access_wr", 0x67, STXT("Data memory access, write")) },
  { I("memory_error", 0x1a, STXT("Local memory error")) },
  { I("unaligned_ld_spec", 0x68, STXT("Unaligned access, read")) },
  { I("unaligned_ldst_spec", 0x6a, STXT("Unaligned access")) },
  { I("unaligned_st_spec", 0x69, STXT("Unaligned access, write")) },
/* pipeline: */
  { I("bx_stall", 0x10c, STXT("BX stalled")) },
  { I("decode_stall", 0x108, STXT("Decode starved")) },
  { I("dispatch_stall", 0x109, STXT("Dispatch stalled")) },
  { I("fx_stall", 0x10f, STXT("FX stalled")) },
  { I("ixa_stall", 0x10a, STXT("IXA stalled")) },
  { I("ixb_stall", 0x10b, STXT("IXB stalled")) },
  { I("lx_stall", 0x10d, STXT("LX stalled")) },
  { I("sx_stall", 0x10e, STXT("SX stalled")) },
  { NULL, NULL, 0, NULL }
};

#undef I
#endif
