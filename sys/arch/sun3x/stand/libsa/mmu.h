/*	$NetBSD: mmu.h,v 1.2 1998/01/05 07:03:35 perry Exp $	*/

u_int32_t get_pte(u_int32_t);
void set_pte(u_int32_t va, u_int32_t pa);
void mmu_atc_flush (u_int32_t va);
