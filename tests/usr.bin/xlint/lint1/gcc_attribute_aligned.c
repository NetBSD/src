/*	$NetBSD: gcc_attribute_aligned.c,v 1.6 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "gcc_attribute_aligned.c"

/*
 * Test size computations on aligned and packed structs.
 */

/* lint1-extra-flags: -X 351 */

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

/* Each variant of the union has size 8. */
/* expect+1: error: negative array dimension (-8) [20] */
typedef int sizeof_fp_addr[-(int)sizeof(union fp_addr)];

struct fpacc87 {
	uint64_t f87_mantissa;
	uint16_t f87_exp_sign;
} __attribute__((packed)) __attribute__((aligned(2)));

/*
 * Due to the 'packed', the uint64_t does not need to be aligned on an 8-byte
 * boundary, which allows the struct to have the minimal required size of 10.
 */
/* expect+1: error: negative array dimension (-10) [20] */
typedef int sizeof_fpacc87[-(int)sizeof(struct fpacc87)];

struct save87 {
	uint16_t s87_cw __attribute__((aligned(4)));
	uint16_t s87_sw __attribute__((aligned(4)));
	uint16_t s87_tw __attribute__((aligned(4)));
	union fp_addr s87_ip;
	union fp_addr s87_dp;
	struct fpacc87 s87_ac[8];
};

/* FIXME: @4 2 + @4 2 + @4 2 + @4 8 + @4 8 + @2 (8 * 10) == 108 */
/* expect+1: error: negative array dimension (-104) [20] */
typedef int sizeof_save87[-(int)sizeof(struct save87)];


void
aligned_struct_member(void)
{
	struct aligned {
		int first;
		int second __attribute__((__aligned__(16)));
	};

	/*
	 * Aligning 'second' to a 16-bytes boundary not only aligns the member
	 * inside the structure, it also affects the alignment requirement of
	 * the whole structure.  Due to this struct alignment, the size of the
	 * structure gets rounded up to 32 instead of using the minimally
	 * necessary storage of 20.
	 *
	 * https://gcc.gnu.org/onlinedocs/gcc/Common-Type-Attributes.html
	 */
	/* TODO: should be -32 instead of -8. */
	/* expect+1: error: negative array dimension (-8) [20] */
	typedef int ctassert[-(int)sizeof(struct aligned)];
}
