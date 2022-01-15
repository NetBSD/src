/*	$NetBSD: gcc_attribute_aligned.c,v 1.2 2022/01/15 14:22:03 rillig Exp $	*/
# 3 "gcc_attribute_aligned.c"

/*
 * Test size computations on aligned and packed structs.
 */

typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

/* from sys/arch/x86/include/cpu_extended_state.h */

union fp_addr {
	uint64_t fa_64;
	struct {
		uint32_t fa_off;
		uint16_t fa_seg;
		uint16_t fa_opcode;
	} fa_32;
} __attribute__((packed)) __attribute__((aligned(4)));

struct fpacc87 {
	uint64_t f87_mantissa;
	uint16_t f87_exp_sign;
} __attribute__((packed)) __attribute__((aligned(2)));

struct save87 {
	uint16_t s87_cw __attribute__((aligned(4)));
	uint16_t s87_sw __attribute__((aligned(4)));
	uint16_t s87_tw __attribute__((aligned(4)));
	union fp_addr s87_ip;
	union fp_addr s87_dp;
	struct fpacc87 s87_ac[8];
};

struct {
	unsigned int sizeof_fp_addr: sizeof(union fp_addr) == 8 ? 1 : -1;

	unsigned int sizeof_fpacc87: sizeof(struct fpacc87) == 10 ? 1 : -1;

	/* FIXME: @4 2 + @4 2 + @4 2 + @4 8 + @4 8 + @2 (8 * 10) == 108 */
	/* expect+1: illegal bit-field size: 255 */
	unsigned int sizeof_save87: sizeof(struct save87) == 108 ? 1 : -1;
};
