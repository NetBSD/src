/*	$NetBSD: octeon_cop2var.h,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $	*/

/*
 * TODO:
 *
 * - Utilize prefetch.
 *
 * - Implement loop in CBC operations.  Take an argument of the number of
 *   blocks.  Better if prefetch is used too.
 *
 * - In AES and DES buffer block loop, merge encrypt / decrypt.  Take a
 *   direction argument (int dir, 0 => encrypt, 1 => decrypt) then branch.
 */

#ifndef _CN30XXCOP2VAR_H_
#define _CN30XXCOP2VAR_H_

#ifdef __OCTEON_USEUN__
#define CNASM_ULD(r, o, b)		"uld	%["#r"], "#o"(%["#b"])	\n\t"
#define CNASM_USD(r, o, b)		"usd	%["#r"], "#o"(%["#b"])	\n\t"
#define CNASM_ULW(r, o, b)		"ulw	%["#r"], "#o"(%["#b"])	\n\t"
#define CNASM_USW(r, o, b)		"usw	%["#r"], "#o"(%["#b"])	\n\t"
#else
#define __CNASM_ULH(i, r, o, x, b)	i"	%["#r"], ("#o" + "#x")(%["#b"]) \n\t"
#define __CNASM_ULS(p, r, o, l, h, b)	__CNASM_ULH(#p"l", r, o, l, b) \
					__CNASM_ULH(#p"r", r, o, h, b)
#define CNASM_ULD(r, o, b)		__CNASM_ULS(ld, r, o, 0, 7, b)
#define CNASM_USD(r, o, b)		__CNASM_ULS(sd, r, o, 0, 7, b)
#define CNASM_ULW(r, o, b)		__CNASM_ULS(lw, r, o, 0, 3, b)
#define CNASM_USW(r, o, b)		__CNASM_ULS(sw, r, o, 0, 3, b)
#endif

#define CNASM_ALD(r, o, b)		"ld	%["#r"], "#o"(%["#b"])	\n\t"
#define CNASM_ASD(r, o, b)		"sd	%["#r"], "#o"(%["#b"])	\n\t"

#undef	__s
#define	__s(s)	#s	/* stringify */
#define CNASM_MT2(r, n, o)		"dmtc2	%["#r"], ("__s(n)" + "#o") \n\t"
#define CNASM_MF2(r, n, o)		"dmfc2	%["#r"], ("__s(n)" + "#o") \n\t"
#define CNASM_MT2ZERO(n, o)		"dmtc2	$0, ("__s(n)" + "#o")	\n\t"
#define CNASM_MT2ZERO(n, o)		"dmtc2	$0, ("__s(n)" + "#o")	\n\t"

#define CNASM_START()			".set	push			\n\t" \
					".set	mips64			\n\t" \
					".set	arch=octeon		\n\t" \
					".set	noreorder		\n\t"
#define CNASM_END()			".set	pop"

#define	__aligned_t	uint64_t
#define	__unaligned_t	uint8_t

/* -------------------------------------------------------------------------- */

/* AES */

#define	__octeon_cop2_aes_set_key_au_vaddr64(au, AU)		\
static inline void						\
octeon_cop2_aes_set_key_##au##_vaddr64(uint64_t key, uint32_t klen) \
{								\
	uint64_t tmp0, tmp1, tmp2, tmp3;			\
								\
	asm volatile (						\
		CNASM_START()					\
		/* %[cnt] is either 4 (256), 3 (192), or 2 (128) */ \
		/* Each operation set AESKEYLEN of cop2 also */ \
		/* >= 64 */					\
		CNASM_##AU##LD(tmp0, 0, key)			\
	"	subu	%[cnt], %[cnt], 1		\n"	\
	"	beqz	%[cnt], 1f			\n"	\
		 CNASM_MT2(tmp0, CVM_MT_AES_KEY, 0)	/* delay slot */ \
		/* >= 128 */					\
		CNASM_##AU##LD(tmp1, 8, key)			\
	"	subu	%[cnt], %[cnt], 1		\n"	\
	"	beqz	%[cnt], 1f			\n"	\
		 CNASM_MT2(tmp1, CVM_MT_AES_KEY, 1)	/* delay slot */ \
		/* >= 192 */					\
		CNASM_##AU##LD(tmp2, 16, key)			\
	"	subu	%[cnt], %[cnt], 1		\n"	\
	"	beqz	%[cnt], 1f			\n"	\
		 CNASM_MT2(tmp2, CVM_MT_AES_KEY, 2)	/* delay slot */ \
		/* >= 256 */					\
		CNASM_##AU##LD(tmp3, 24, key)			\
		CNASM_MT2(tmp3, CVM_MT_AES_KEY, 3)		\
		/* done */					\
	"1:						\n"	\
		CNASM_END()					\
		: [tmp0] "=&r" (tmp0),				\
		  [tmp1] "=&r" (tmp1),				\
		  [tmp2] "=&r" (tmp2),				\
		  [tmp3] "=&r" (tmp3)				\
		: [key] "d" (key),				\
		  [cnt] "d" (klen >> 6));			\
}

#define	__octeon_cop2_aes_set_key_au_ptr(au, AU, ptr)		\
static inline void						\
octeon_cop2_aes_set_key_##au(ptr key, uint32_t klen)		\
{								\
	octeon_cop2_aes_set_key_##au##_vaddr64((intptr_t)key, klen); \
}

#define	__octeon_cop2_aes_set_key_au(au, AU)			\
__octeon_cop2_aes_set_key_au_vaddr64(au, AU)			\
__octeon_cop2_aes_set_key_au_ptr(au, AU, __##au##_t *)

#define	__octeon_cop2_aes_set_key				\
__octeon_cop2_aes_set_key_au(aligned, A)				\
__octeon_cop2_aes_set_key_au(unaligned, U)

__octeon_cop2_aes_set_key

static inline void
octeon_cop2_aes_set_iv_unaligned_vaddr64(uint64_t iv)
{
	uint64_t tmp0, tmp1;

	asm volatile (
		CNASM_START()
		/* Store the IV to cop2 */
		CNASM_ULD(tmp0, 0, iv)
		CNASM_ULD(tmp1, 8, iv)
		CNASM_MT2(tmp0, CVM_MT_AES_IV, 0)
		CNASM_MT2(tmp1, CVM_MT_AES_IV, 1)
		CNASM_END()
		: [tmp0] "=&r" (tmp0),
		  [tmp1] "=&r" (tmp1)
		: [iv] "d" (iv));
}

static inline void
octeon_cop2_aes_set_iv_unaligned(uint8_t *iv)
{
	octeon_cop2_aes_set_iv_unaligned_vaddr64((intptr_t)iv);
}

#define	__octeon_cop2_aes_ed_16_au_vaddr64(ed, ED, au, AU)	\
static inline void						\
octeon_cop2_aes_##ed##_16_##au##_vaddr64(uint64_t d, uint64_t s) \
{								\
	uint64_t tmp0, tmp1;					\
								\
	asm volatile (						\
		CNASM_START()					\
		CNASM_##AU##LD(tmp0, 0, s)			\
		CNASM_##AU##LD(tmp1, 8, s)			\
		CNASM_MT2(tmp0, CVM_MT_AES_##ED##0, 0)		\
		CNASM_MT2(tmp1, CVM_MT_AES_##ED##1, 0)		\
		CNASM_MF2(tmp0, CVM_MF_AES_RESINP, 0)		\
		CNASM_MF2(tmp1, CVM_MF_AES_RESINP, 1)		\
		CNASM_##AU##SD(tmp0, 0, d)			\
		CNASM_##AU##SD(tmp1, 8, d)			\
		CNASM_END()					\
		: [tmp0] "=&r" (tmp0),				\
		  [tmp1] "=&r" (tmp1)				\
		: [d] "d" (d),					\
		  [s] "d" (s));					\
}

#define	__octeon_cop2_aes_ed_16_au_ptr(ed, ED, au, AU, ptr)	\
static inline void						\
octeon_cop2_aes_##ed##_16_##au(ptr d, ptr s)			\
{								\
	octeon_cop2_aes_##ed##_16_##au##_vaddr64((intptr_t)d, (intptr_t)s); \
}

#define	__octeon_cop2_aes_ed_16_au(ed, ED, au, AU)		\
__octeon_cop2_aes_ed_16_au_vaddr64(ed, ED, au, AU)		\
__octeon_cop2_aes_ed_16_au_ptr(ed, ED, au, AU, __##au##_t *)

#define	__octeon_cop2_aes_ed_16(ed, ED)				\
__octeon_cop2_aes_ed_16_au(ed, ED, aligned, A)			\
__octeon_cop2_aes_ed_16_au(ed, ED, unaligned, U)

#define	__octeon_cop2_aes_16					\
__octeon_cop2_aes_ed_16(encrypt, ENC)				\
__octeon_cop2_aes_ed_16(decrypt, DEC)				\
__octeon_cop2_aes_ed_16(cbc_encrypt, ENC_CBC)			\
__octeon_cop2_aes_ed_16(cbc_decrypt, DEC_CBC)

__octeon_cop2_aes_16

#define	__octeon_cop2_aes_ed_block_au_vaddr64(ed, ED, au, AU)	\
static inline void						\
octeon_cop2_aes_##ed##_block_##au##_vaddr64(uint64_t d, uint64_t s, int n) \
{								\
	uint64_t tmp0, tmp1;					\
	uint64_t x = d + 16 * n;				\
								\
	asm volatile (						\
		CNASM_START()					\
	"1:						\n"	\
		CNASM_##AU##LD(tmp0, 0, s)			\
		CNASM_##AU##LD(tmp1, 8, s)			\
		CNASM_MT2(tmp0, CVM_MT_AES_##ED##0, 0)		\
		CNASM_MT2(tmp1, CVM_MT_AES_##ED##1, 0)		\
		CNASM_MF2(tmp0, CVM_MF_AES_RESINP, 0)		\
		CNASM_MF2(tmp1, CVM_MF_AES_RESINP, 1)		\
		CNASM_##AU##SD(tmp0, 0, d)			\
		CNASM_##AU##SD(tmp1, 8, d)			\
	"	daddu	%[d], %[d], 16			\n"	\
	"	bne	%[d], %[x], 1b			\n"	\
	"	 daddu	%[s], %[s], 16			\n" /* delay slot */ \
		CNASM_END()					\
		: [d] "=d" (d),					\
		  [s] "=d" (s),					\
		  [tmp0] "=&r" (tmp0),				\
		  [tmp1] "=&r" (tmp1)				\
		: "0" (d),					\
		  "1" (s),					\
		  [x] "d" (x));					\
}

#define	__octeon_cop2_aes_ed_block_au_ptr(ed, ED, au, AU, ptr)	\
static inline void						\
octeon_cop2_aes_##ed##_block_##au(ptr d, ptr s, int n)		\
{								\
	octeon_cop2_aes_##ed##_block_##au##_vaddr64((intptr_t)d, (intptr_t)s, n); \
}

#define	__octeon_cop2_aes_ed_block_au(ed, ED, au, AU)		\
__octeon_cop2_aes_ed_block_au_vaddr64(ed, ED, au, AU)		\
__octeon_cop2_aes_ed_block_au_ptr(ed, ED, au, AU, __##au##_t *)

#define	__octeon_cop2_aes_ed_block(ed, ED)			\
__octeon_cop2_aes_ed_block_au(ed, ED, aligned, A)		\
__octeon_cop2_aes_ed_block_au(ed, ED, unaligned, U)

#define	__octeon_cop2_aes_block					\
/* __octeon_cop2_aes_ed_block(encrypt, ENC) */			\
/* __octeon_cop2_aes_ed_block(decrypt, DEC)	*/		\
__octeon_cop2_aes_ed_block(cbc_encrypt, ENC_CBC)			\
__octeon_cop2_aes_ed_block(cbc_decrypt, DEC_CBC)

__octeon_cop2_aes_block

#define	__octeon_cop2_aes_ed_64_au_vaddr64(ed, ED, au, AU)	\
static inline void						\
octeon_cop2_aes_##ed##_64_##au##_vaddr64(uint64_t d, uint64_t s) \
{								\
	uint64_t tmp0, tmp1, tmp2, tmp3;			\
								\
	asm volatile (						\
		CNASM_START()					\
		CNASM_##AU##LD(tmp0, 0, s)			\
		CNASM_##AU##LD(tmp1, 8, s)			\
		CNASM_MT2(tmp0, CVM_MT_AES_##ED##0, 0)		\
		CNASM_MT2(tmp1, CVM_MT_AES_##ED##1, 0)		\
		CNASM_##AU##LD(tmp2, 16, s)			\
		CNASM_##AU##LD(tmp3, 24, s)			\
		CNASM_MF2(tmp0, CVM_MF_AES_RESINP, 0)		\
		CNASM_MF2(tmp1, CVM_MF_AES_RESINP, 1)		\
		CNASM_MT2(tmp2, CVM_MT_AES_##ED##0, 0)		\
		CNASM_MT2(tmp3, CVM_MT_AES_##ED##1, 0)		\
		CNASM_##AU##SD(tmp0, 0, d)			\
		CNASM_##AU##SD(tmp1, 8, d)			\
		CNASM_MF2(tmp2, CVM_MF_AES_RESINP, 0)		\
		CNASM_MF2(tmp3, CVM_MF_AES_RESINP, 1)		\
		CNASM_##AU##SD(tmp2, 16, d)			\
		CNASM_##AU##SD(tmp3, 24, d)			\
		CNASM_##AU##LD(tmp0, 32, s)			\
		CNASM_##AU##LD(tmp1, 40, s)			\
		CNASM_MT2(tmp0, CVM_MT_AES_##ED##0, 0)		\
		CNASM_MT2(tmp1, CVM_MT_AES_##ED##1, 0)		\
		CNASM_##AU##LD(tmp2, 48, s)			\
		CNASM_##AU##LD(tmp3, 56, s)			\
		CNASM_MF2(tmp0, CVM_MF_AES_RESINP, 0)		\
		CNASM_MF2(tmp1, CVM_MF_AES_RESINP, 1)		\
		CNASM_MT2(tmp2, CVM_MT_AES_##ED##0, 0)		\
		CNASM_MT2(tmp3, CVM_MT_AES_##ED##1, 0)		\
		CNASM_##AU##SD(tmp0, 32, d)			\
		CNASM_##AU##SD(tmp1, 40, d)			\
		CNASM_MF2(tmp2, CVM_MF_AES_RESINP, 0)		\
		CNASM_MF2(tmp3, CVM_MF_AES_RESINP, 1)		\
		CNASM_##AU##SD(tmp2, 48, d)			\
		CNASM_##AU##SD(tmp3, 56, d)			\
		CNASM_END()					\
		: [tmp0] "=&r" (tmp0),				\
		  [tmp1] "=&r" (tmp1),				\
		  [tmp2] "=&r" (tmp2),				\
		  [tmp3] "=&r" (tmp3)				\
		: [d] "d" (d),					\
		  [s] "d" (s));					\
}

#define	__octeon_cop2_aes_ed_64_au_ptr(ed, ED, au, AU, ptr)	\
static inline void						\
octeon_cop2_aes_##ed##_64_##au(ptr d, ptr s)			\
{								\
	octeon_cop2_aes_##ed##_64_##au##_vaddr64((intptr_t)d, (intptr_t)s); \
}

#define	__octeon_cop2_aes_ed_64_au(ed, ED, au, AU)		\
__octeon_cop2_aes_ed_64_au_vaddr64(ed, ED, au, AU)		\
__octeon_cop2_aes_ed_64_au_ptr(ed, ED, au, AU, __##au##_t *)

#define	__octeon_cop2_aes_ed_64(ed, ED)				\
__octeon_cop2_aes_ed_64_au(ed, ED, aligned, A)			\
__octeon_cop2_aes_ed_64_au(ed, ED, unaligned, U)

#define	__octeon_cop2_aes_64					\
/* __octeon_cop2_aes_ed_64(encrypt, ENC) */			\
/* __octeon_cop2_aes_ed_64(decrypt, DEC)	*/			\
__octeon_cop2_aes_ed_64(cbc_encrypt, ENC_CBC)			\
__octeon_cop2_aes_ed_64(cbc_decrypt, DEC_CBC)

__octeon_cop2_aes_64

/* -------------------------------------------------------------------------- */

/* DES */

static inline void
octeon_cop2_des_set_key_unaligned_vaddr64(uint64_t k1, uint64_t k2, uint64_t k3)
{
	uint64_t tmp0, tmp1, tmp2;

	asm volatile (
		CNASM_START()
		/* Set key */
		CNASM_ULD(tmp0, 0, k1)
		CNASM_ULD(tmp1, 0, k2)
		CNASM_ULD(tmp2, 0, k3)
		CNASM_MT2(tmp0, CVM_MT_3DES_KEY, 0)
		CNASM_MT2(tmp1, CVM_MT_3DES_KEY, 1)
		CNASM_MT2(tmp2, CVM_MT_3DES_KEY, 2)
		CNASM_END()
		: [tmp0] "=&r" (tmp0),
		  [tmp1] "=&r" (tmp1),
		  [tmp2] "=&r" (tmp2)
		: [k1] "d" (k1),
		  [k2] "d" (k2),
		  [k3] "d" (k3));
}

static inline void
octeon_cop2_des_set_key_unaligned(uint64_t *k1, uint64_t *k2, uint64_t *k3)
{
	octeon_cop2_des_set_key_unaligned_vaddr64((intptr_t)k1, (intptr_t)k2, (intptr_t)k3);
}

static inline void
octeon_cop2_des_set_iv_unaligned_vaddr64(uint64_t iv)
{
	uint64_t tmp0;

	asm volatile (
		CNASM_START()
		/* Load IV to a register */
		CNASM_ULD(tmp0, 0, iv)
		/* Store the IV to cop2 */
		CNASM_MT2(tmp0, CVM_MT_3DES_IV, 0)
		CNASM_END()
		: [tmp0] "=&r" (tmp0)
		: [iv] "d" (iv));
}

static inline void
octeon_cop2_des_set_iv_unaligned(uint8_t *iv)
{
	octeon_cop2_des_set_iv_unaligned_vaddr64((intptr_t)iv);
}

#define	__octeon_cop2_des_ed_8_au_vaddr64(ed, ED, au, AU)	\
static inline void						\
octeon_cop2_des_##ed##_8_##au##_vaddr64(uint64_t d, uint64_t s)	\
{								\
	uint64_t tmp0;						\
								\
	asm volatile (						\
		CNASM_START()					\
		CNASM_##AU##LD(tmp0, 0, s)			\
		CNASM_MT2(tmp0, CVM_MT_3DES_##ED, 0)		\
		CNASM_MF2(tmp0, CVM_MF_3DES_RESULT, 0)		\
		CNASM_##AU##SD(tmp0, 0, s)			\
		CNASM_END()					\
		: [tmp0] "=&r" (tmp0)				\
		: [d] "d" (d),					\
		  [s] "d" (s));					\
}

#define	__octeon_cop2_des_ed_8_au_ptr(ed, ED, au, AU, ptr)	\
static inline void						\
octeon_cop2_des_##ed##_8_##au(ptr d, ptr s)			\
{								\
	octeon_cop2_des_##ed##_8_##au##_vaddr64((intptr_t)d, (intptr_t)s); \
}

#define	__octeon_cop2_des_ed_8_au(ed, ED, au, AU)		\
__octeon_cop2_des_ed_8_au_vaddr64(ed, ED, au, AU)		\
__octeon_cop2_des_ed_8_au_ptr(ed, ED, au, AU, __##au##_t *)

#define	__octeon_cop2_des_ed_8(ed, ED)				\
__octeon_cop2_des_ed_8_au(ed, ED, aligned, A)			\
__octeon_cop2_des_ed_8_au(ed, ED, unaligned, U)

#define	__octeon_cop2_des_8					\
__octeon_cop2_des_ed_8(encrypt, ENC)				\
__octeon_cop2_des_ed_8(decrypt, DEC)				\
__octeon_cop2_des_ed_8(cbc_encrypt, ENC_CBC)			\
__octeon_cop2_des_ed_8(cbc_decrypt, DEC_CBC)

__octeon_cop2_des_8

#define	__octeon_cop2_des_ed_block_au_vaddr64(ed, ED, au, AU)	\
static inline void						\
octeon_cop2_des_##ed##_block_##au##_vaddr64(uint64_t d, uint64_t s, int n) \
{								\
	uint64_t tmp0;						\
	uint64_t x = d + 8 * n;					\
								\
	asm volatile (						\
		CNASM_START()					\
	"1:						\n"	\
		CNASM_##AU##LD(tmp0, 0, s)			\
		CNASM_MT2(tmp0, CVM_MT_3DES_##ED, 0)		\
		CNASM_MF2(tmp0, CVM_MF_3DES_RESULT, 0)		\
		CNASM_##AU##SD(tmp0, 0, d)			\
	"	daddu	%[d], %[d], 8			\n"	\
	"	bne	%[d], %[x], 1b			\n"	\
	"	 daddu	%[s], %[s], 8			\n"	\
		CNASM_END()					\
		: [d] "=d" (d),					\
		  [s] "=d" (s),					\
		  [tmp0] "=&r" (tmp0)				\
		: "0" (d),					\
		  "1" (s),					\
		  [x] "d" (x));					\
}

#define	__octeon_cop2_des_ed_block_au_ptr(ed, ED, au, AU, ptr)	\
static inline void						\
octeon_cop2_des_##ed##_block_##au(ptr d, ptr s, int n)		\
{								\
	octeon_cop2_des_##ed##_block_##au##_vaddr64((intptr_t)d, (intptr_t)s, n); \
}

#define	__octeon_cop2_des_ed_block_au(ed, ED, au, AU)		\
__octeon_cop2_des_ed_block_au_vaddr64(ed, ED, au, AU)		\
__octeon_cop2_des_ed_block_au_ptr(ed, ED, au, AU, __##au##_t *)

#define	__octeon_cop2_des_ed_block(ed, ED)			\
__octeon_cop2_des_ed_block_au(ed, ED, aligned, A)		\
__octeon_cop2_des_ed_block_au(ed, ED, unaligned, U)

#define	__octeon_cop2_des_block					\
/* __octeon_cop2_des_ed_block(encrypt, ENC) */			\
/* __octeon_cop2_des_ed_block(decrypt, DEC) */			\
__octeon_cop2_des_ed_block(cbc_encrypt, ENC_CBC)			\
__octeon_cop2_des_ed_block(cbc_decrypt, DEC_CBC)

__octeon_cop2_des_block

#define	__octeon_cop2_des_ed_64_au_vaddr64(ed, ED, au, AU)	\
static inline void						\
octeon_cop2_des_##ed##_64_##au##_vaddr64(uint64_t d, uint64_t s) \
{								\
	uint64_t tmp0, tmp1, tmp2, tmp3;			\
								\
	asm volatile (						\
		CNASM_START()					\
		CNASM_##AU##LD(tmp0, 0, s)			\
		CNASM_##AU##LD(tmp1, 8, s)			\
		CNASM_MT2(tmp0, CVM_MT_3DES_##ED, 0)		\
		CNASM_##AU##LD(tmp2, 16, s)			\
		CNASM_MF2(tmp0, CVM_MF_3DES_RESULT, 0)		\
		CNASM_MT2(tmp1, CVM_MT_3DES_##ED, 0)		\
		CNASM_##AU##LD(tmp3, 24, s)			\
		CNASM_MF2(tmp1, CVM_MF_3DES_RESULT, 0)		\
		CNASM_MT2(tmp2, CVM_MT_3DES_##ED, 0)		\
		CNASM_##AU##SD(tmp0, 0, d)			\
		CNASM_MF2(tmp2, CVM_MF_3DES_RESULT, 0)		\
		CNASM_MT2(tmp3, CVM_MT_3DES_##ED, 0)		\
		CNASM_##AU##SD(tmp1, 8, d)			\
		CNASM_MF2(tmp3, CVM_MF_3DES_RESULT, 0)		\
		CNASM_##AU##SD(tmp2, 16, d)			\
		CNASM_##AU##SD(tmp3, 24, d)			\
		CNASM_##AU##LD(tmp0, 32, s)			\
		CNASM_##AU##LD(tmp1, 40, s)			\
		CNASM_MT2(tmp0, CVM_MT_3DES_##ED, 0)		\
		CNASM_##AU##LD(tmp2, 48, s)			\
		CNASM_MF2(tmp0, CVM_MF_3DES_RESULT, 0)		\
		CNASM_MT2(tmp1, CVM_MT_3DES_##ED, 0)		\
		CNASM_##AU##LD(tmp3, 56, s)			\
		CNASM_MF2(tmp1, CVM_MF_3DES_RESULT, 0)		\
		CNASM_MT2(tmp2, CVM_MT_3DES_##ED, 0)		\
		CNASM_##AU##SD(tmp0, 32, d)			\
		CNASM_MF2(tmp2, CVM_MF_3DES_RESULT, 0)		\
		CNASM_MT2(tmp3, CVM_MT_3DES_##ED, 0)		\
		CNASM_##AU##SD(tmp1, 40, d)			\
		CNASM_MF2(tmp3, CVM_MF_3DES_RESULT, 0)		\
		CNASM_##AU##SD(tmp2, 48, d)			\
		CNASM_##AU##SD(tmp3, 56, d)			\
		CNASM_END()					\
		: [tmp0] "=&r" (tmp0),				\
		  [tmp1] "=&r" (tmp1),				\
		  [tmp2] "=&r" (tmp2),				\
		  [tmp3] "=&r" (tmp3)				\
		: [d] "d" (d),					\
		  [s] "d" (s));					\
}

#define	__octeon_cop2_des_ed_64_au_ptr(ed, ED, au, AU, ptr)	\
static inline void						\
octeon_cop2_des_##ed##_64_##au(ptr d, ptr s)			\
{								\
	octeon_cop2_des_##ed##_64_##au##_vaddr64((intptr_t)d, (intptr_t)s); \
}

#define	__octeon_cop2_des_ed_64_au(ed, ED, au, AU)		\
__octeon_cop2_des_ed_64_au_vaddr64(ed, ED, au, AU)		\
__octeon_cop2_des_ed_64_au_ptr(ed, ED, au, AU, __##au##_t *)

#define	__octeon_cop2_des_ed_64(ed, ED)				\
__octeon_cop2_des_ed_64_au(ed, ED, aligned, A)			\
__octeon_cop2_des_ed_64_au(ed, ED, unaligned, U)

#define	__octeon_cop2_des_64					\
/* __octeon_cop2_des_ed_64(encrypt, ENC) */			\
/* __octeon_cop2_des_ed_64(decrypt, DEC) */			\
__octeon_cop2_des_ed_64(cbc_encrypt, ENC_CBC)			\
__octeon_cop2_des_ed_64(cbc_decrypt, DEC_CBC)

__octeon_cop2_des_64

/* -------------------------------------------------------------------------- */

/* MD5 */

static inline void
octeon_cop2_md5_set_iv_unaligned_vaddr64(uint64_t iv)
{
	uint64_t tmp0, tmp1;

	asm volatile (
		CNASM_START()
		/* Load IV from context */
		CNASM_ULD(tmp0, 0, iv)
		CNASM_ULD(tmp1, 8, iv)
		CNASM_MT2(tmp0, CVM_MT_HSH_IV, 0)
		CNASM_MT2(tmp1, CVM_MT_HSH_IV, 1)
		CNASM_MT2ZERO(  CVM_MT_HSH_IV, 2)
		CNASM_MT2ZERO(  CVM_MT_HSH_IV, 3)
		CNASM_END()
		: [tmp0] "=&r" (tmp0),
		  [tmp1] "=&r" (tmp1)
		: [iv] "d" (iv));
}

static inline void
octeon_cop2_md5_set_iv_unaligned(uint64_t *iv)
{
	octeon_cop2_md5_set_iv_unaligned_vaddr64((intptr_t)iv);
}

static inline void
octeon_cop2_md5_get_iv_unaligned_vaddr64(uint64_t iv)
{
	uint64_t tmp0, tmp1;

	asm volatile (
		CNASM_START()
		/* Store IV to context */
		CNASM_MF2(tmp0, CVM_MF_HSH_IV, 0)
		CNASM_MF2(tmp1, CVM_MF_HSH_IV, 1)
		CNASM_USD(tmp0, 0, iv)
		CNASM_USD(tmp1, 8, iv)
		CNASM_END()
		: [tmp0] "=&r" (tmp0),
		  [tmp1] "=&r" (tmp1)
		: [iv] "d" (iv));
}

static inline void
octeon_cop2_md5_get_iv_unaligned(uint64_t *iv)
{
	octeon_cop2_md5_get_iv_unaligned_vaddr64((intptr_t)iv);
}

static inline void
octeon_cop2_md5_update_unaligned_vaddr64(uint64_t src)
{
	uint64_t tmp0, tmp1, tmp2, tmp3;

	asm volatile (
		CNASM_START()
		/* Update HASH */
		CNASM_ULD(tmp0, 0, src)
		CNASM_ULD(tmp1, 8, src)
		CNASM_ULD(tmp2, 16, src)
		CNASM_ULD(tmp3, 24, src)
		CNASM_MT2(tmp0, CVM_MT_HSH_DAT, 0)
		CNASM_MT2(tmp1, CVM_MT_HSH_DAT, 1)
		CNASM_MT2(tmp2, CVM_MT_HSH_DAT, 2)
		CNASM_MT2(tmp3, CVM_MT_HSH_DAT, 3)
		CNASM_ULD(tmp0, 32, src)
		CNASM_ULD(tmp1, 40, src)
		CNASM_ULD(tmp2, 48, src)
		CNASM_ULD(tmp3, 56, src)
		CNASM_MT2(tmp0, CVM_MT_HSH_DAT, 4)
		CNASM_MT2(tmp1, CVM_MT_HSH_DAT, 5)
		CNASM_MT2(tmp2, CVM_MT_HSH_DAT, 6)
		CNASM_MT2(tmp3, CVM_MT_HSH_STANDARD5, 0)
		CNASM_END()
		: [tmp0] "=&r" (tmp0),
		  [tmp1] "=&r" (tmp1),
		  [tmp2] "=&r" (tmp2),
		  [tmp3] "=&r" (tmp3)
		: [src] "d" (src));
}

static inline void
octeon_cop2_md5_update_unaligned(uint64_t *src)
{
	octeon_cop2_md5_update_unaligned_vaddr64((intptr_t)src);
}

/* -------------------------------------------------------------------------- */

/* SHA1 */

static inline void
octeon_cop2_sha1_set_iv_unaligned_vaddr64(uint64_t iv)
{
	uint64_t tmp0, tmp1, tmp2;

	asm volatile (
		CNASM_START()
		/* Load IV from context */
		CNASM_ULD(tmp0, 0, iv)
		CNASM_ULD(tmp1, 8, iv)
		CNASM_ULW(tmp2, 16, iv)
		"dsll	%[tmp2], %[tmp2], 32	\n\t"
		CNASM_MT2(tmp0, CVM_MT_HSH_IV, 0)
		CNASM_MT2(tmp1, CVM_MT_HSH_IV, 1)
		CNASM_MT2(tmp2, CVM_MT_HSH_IV, 2)
		CNASM_MT2ZERO(  CVM_MT_HSH_IV, 3)
		CNASM_END()
	: [tmp0] "=&r" (tmp0),
	  [tmp1] "=&r" (tmp1),
	  [tmp2] "=&r" (tmp2)
	: [iv] "d" (iv));
}

static inline void
octeon_cop2_sha1_set_iv_unaligned(uint8_t *iv)
{
	octeon_cop2_sha1_set_iv_unaligned_vaddr64((intptr_t)iv);
}

static inline void
octeon_cop2_sha1_get_iv_unaligned_vaddr64(uint64_t iv)
{
	uint64_t tmp0, tmp1, tmp2;

	asm volatile (
		CNASM_START()
		/* Store IV to context */
		CNASM_MF2(tmp0, CVM_MF_HSH_IV, 0)
		CNASM_MF2(tmp1, CVM_MF_HSH_IV, 1)
		CNASM_MF2(tmp2, CVM_MF_HSH_IV, 2)
		CNASM_USD(tmp0, 0, iv)
		CNASM_USD(tmp1, 8, iv)
		"dsrl	%[tmp2], %[tmp2], 32	\n\t"
		CNASM_USW(tmp2, 16, iv)
		CNASM_END()
	: [tmp0] "=&r" (tmp0),
	  [tmp1] "=&r" (tmp1),
	  [tmp2] "=&r" (tmp2)
	: [iv] "d" (iv));
}

static inline void
octeon_cop2_sha1_get_iv_unaligned(uint8_t *iv)
{
	octeon_cop2_sha1_get_iv_unaligned_vaddr64((intptr_t)iv);
}

static inline void
octeon_cop2_sha1_update_unaligned_vaddr64(uint64_t src)
{
	uint64_t tmp0, tmp1, tmp2, tmp3;

	asm volatile (
		CNASM_START()
		/* Update HASH */
		CNASM_ULD(tmp0, 0, src)
		CNASM_ULD(tmp1, 8, src)
		CNASM_ULD(tmp2, 16, src)
		CNASM_ULD(tmp3, 24, src)
		CNASM_MT2(tmp0, CVM_MT_HSH_DAT, 0)
		CNASM_MT2(tmp1, CVM_MT_HSH_DAT, 1)
		CNASM_MT2(tmp2, CVM_MT_HSH_DAT, 2)
		CNASM_MT2(tmp3, CVM_MT_HSH_DAT, 3)
		CNASM_ULD(tmp0, 32, src)
		CNASM_ULD(tmp1, 40, src)
		CNASM_ULD(tmp2, 48, src)
		CNASM_ULD(tmp3, 56, src)
		CNASM_MT2(tmp0, CVM_MT_HSH_DAT, 4)
		CNASM_MT2(tmp1, CVM_MT_HSH_DAT, 5)
		CNASM_MT2(tmp2, CVM_MT_HSH_DAT, 6)
		CNASM_MT2(tmp3, CVM_MT_HSH_STARTSHA, 0)
		CNASM_END()
	: [tmp0] "=&r" (tmp0),
	  [tmp1] "=&r" (tmp1),
	  [tmp2] "=&r" (tmp2),
	  [tmp3] "=&r" (tmp3)
	: [src] "d" (src));
}

static inline void
octeon_cop2_sha1_update_unaligned(uint8_t *src)
{
	octeon_cop2_sha1_update_unaligned_vaddr64((intptr_t)src);
}

/* -------------------------------------------------------------------------- */

/* SHA256 */

static inline void
octeon_cop2_sha256_set_iv_unaligned_vaddr64(uint64_t iv)
{
	uint64_t tmp0, tmp1, tmp2, tmp3;

	asm volatile (
		CNASM_START()
		/* Load IV from context */
		CNASM_ULD(tmp0, 0, iv)
		CNASM_ULD(tmp1, 8, iv)
		CNASM_ULD(tmp2, 16, iv)
		CNASM_ULD(tmp3, 24, iv)
		CNASM_MT2(tmp0, CVM_MT_HSH_IV, 0)
		CNASM_MT2(tmp1, CVM_MT_HSH_IV, 1)
		CNASM_MT2(tmp2, CVM_MT_HSH_IV, 2)
		CNASM_MT2(tmp3, CVM_MT_HSH_IV, 3)
		CNASM_END()
		: [tmp0] "=&r" (tmp0),
		  [tmp1] "=&r" (tmp1),
		  [tmp2] "=&r" (tmp2),
		  [tmp3] "=&r" (tmp3)
		: [iv] "d" (iv));
}

static inline void
octeon_cop2_sha256_set_iv_unaligned(uint8_t *iv)
{
	octeon_cop2_sha256_set_iv_unaligned_vaddr64((intptr_t)iv);
}

static inline void
octeon_cop2_sha256_get_iv_unaligned_vaddr64(uint64_t iv)
{
	uint64_t tmp0, tmp1, tmp2, tmp3;

	asm volatile (
		CNASM_START()
		/* Store IV to context */
		CNASM_MF2(tmp0, CVM_MF_HSH_IV, 0)
		CNASM_MF2(tmp1, CVM_MF_HSH_IV, 1)
		CNASM_MF2(tmp2, CVM_MF_HSH_IV, 2)
		CNASM_MF2(tmp3, CVM_MF_HSH_IV, 3)
		CNASM_USD(tmp0, 0, iv)
		CNASM_USD(tmp1, 8, iv)
		CNASM_USD(tmp2, 16, iv)
		CNASM_USD(tmp3, 24, iv)
		CNASM_END()
		: [tmp0] "=&r" (tmp0),
		  [tmp1] "=&r" (tmp1),
		  [tmp2] "=&r" (tmp2),
		  [tmp3] "=&r" (tmp3)
		: [iv] "d" (iv));
}

static inline void
octeon_cop2_sha256_get_iv_unaligned(uint8_t *iv)
{
	octeon_cop2_sha256_get_iv_unaligned_vaddr64((intptr_t)iv);
}

static inline void
octeon_cop2_sha256_update_unaligned_vaddr64(uint64_t src)
{
	uint64_t tmp0, tmp1, tmp2, tmp3;

	asm volatile (
		CNASM_START()
		/* Update HASH */
		CNASM_ULD(tmp0, 0, src)
		CNASM_ULD(tmp1, 8, src)
		CNASM_ULD(tmp2, 16, src)
		CNASM_ULD(tmp3, 24, src)
		CNASM_MT2(tmp0, CVM_MT_HSH_DAT, 0)
		CNASM_MT2(tmp1, CVM_MT_HSH_DAT, 1)
		CNASM_MT2(tmp2, CVM_MT_HSH_DAT, 2)
		CNASM_MT2(tmp3, CVM_MT_HSH_DAT, 3)
		CNASM_ULD(tmp0, 32, src)
		CNASM_ULD(tmp1, 40, src)
		CNASM_ULD(tmp2, 48, src)
		CNASM_ULD(tmp3, 56, src)
		CNASM_MT2(tmp0, CVM_MT_HSH_DAT, 4)
		CNASM_MT2(tmp1, CVM_MT_HSH_DAT, 5)
		CNASM_MT2(tmp2, CVM_MT_HSH_DAT, 6)
		CNASM_MT2(tmp3, CVM_MT_HSH_STARTSHA256, 0)
		CNASM_END()
		: [tmp0] "=&r" (tmp0),
		  [tmp1] "=&r" (tmp1),
		  [tmp2] "=&r" (tmp2),
		  [tmp3] "=&r" (tmp3)
		: [src] "d" (src));
}

static inline void
octeon_cop2_sha256_update_unaligned(uint8_t *src)
{
	octeon_cop2_sha256_update_unaligned_vaddr64((intptr_t)src);
}

/* -------------------------------------------------------------------------- */

/* SHA512 */

static inline void
octeon_cop2_sha512_set_iv_unaligned_vaddr64(uint64_t iv)
{
	uint64_t tmp0, tmp1, tmp2, tmp3;

	asm volatile (
		CNASM_START()
		/* Load IV from context */
		CNASM_ULD(tmp0, 0, iv)
		CNASM_ULD(tmp1, 8, iv)
		CNASM_ULD(tmp2, 16, iv)
		CNASM_ULD(tmp3, 24, iv)
		CNASM_MT2(tmp0, CVM_MT_HSH_IVW, 0)
		CNASM_MT2(tmp1, CVM_MT_HSH_IVW, 1)
		CNASM_MT2(tmp2, CVM_MT_HSH_IVW, 2)
		CNASM_MT2(tmp3, CVM_MT_HSH_IVW, 3)
		CNASM_ULD(tmp0, 32, iv)
		CNASM_ULD(tmp1, 40, iv)
		CNASM_ULD(tmp2, 48, iv)
		CNASM_ULD(tmp3, 56, iv)
		CNASM_MT2(tmp0, CVM_MT_HSH_IVW, 4)
		CNASM_MT2(tmp1, CVM_MT_HSH_IVW, 5)
		CNASM_MT2(tmp2, CVM_MT_HSH_IVW, 6)
		CNASM_MT2(tmp3, CVM_MT_HSH_IVW, 7)
		CNASM_END()
		: [tmp0] "=&r" (tmp0),
		  [tmp1] "=&r" (tmp1),
		  [tmp2] "=&r" (tmp2),
		  [tmp3] "=&r" (tmp3)
		: [iv] "d" (iv));
}

static inline void
octeon_cop2_sha512_set_iv_unaligned(uint8_t *iv)
{
	octeon_cop2_sha512_set_iv_unaligned_vaddr64((intptr_t)iv);
}

static inline void
octeon_cop2_sha512_get_iv_unaligned_vaddr64(uint64_t iv)
{
	uint64_t tmp0, tmp1, tmp2, tmp3;

	asm volatile (
		CNASM_START()
		/* Store IV to context */
		CNASM_MF2(tmp0, CVM_MF_HSH_IVW, 0)
		CNASM_MF2(tmp1, CVM_MF_HSH_IVW, 1)
		CNASM_MF2(tmp2, CVM_MF_HSH_IVW, 2)
		CNASM_MF2(tmp3, CVM_MF_HSH_IVW, 3)
		CNASM_USD(tmp0, 0, iv)
		CNASM_USD(tmp1, 8, iv)
		CNASM_USD(tmp2, 16, iv)
		CNASM_USD(tmp3, 24, iv)
		CNASM_MF2(tmp0, CVM_MF_HSH_IVW, 4)
		CNASM_MF2(tmp1, CVM_MF_HSH_IVW, 5)
		CNASM_MF2(tmp2, CVM_MF_HSH_IVW, 6)
		CNASM_MF2(tmp3, CVM_MF_HSH_IVW, 7)
		CNASM_USD(tmp0, 32, iv)
		CNASM_USD(tmp1, 40, iv)
		CNASM_USD(tmp2, 48, iv)
		CNASM_USD(tmp3, 56, iv)
		CNASM_END()
		: [tmp0] "=&r" (tmp0),
		  [tmp1] "=&r" (tmp1),
		  [tmp2] "=&r" (tmp2),
		  [tmp3] "=&r" (tmp3)
		: [iv] "d" (iv));
}

static inline void
octeon_cop2_sha512_get_iv_unaligned(uint8_t *iv)
{
	octeon_cop2_sha512_get_iv_unaligned_vaddr64((intptr_t)iv);
}

static inline void
octeon_cop2_sha512_update_unaligned_vaddr64(uint64_t src)
{
	uint64_t tmp0, tmp1, tmp2, tmp3;

	asm volatile (
		CNASM_START()
		/* Update HASH */
		CNASM_ULD(tmp0, 0, src)
		CNASM_ULD(tmp1, 8, src)
		CNASM_ULD(tmp2, 16, src)
		CNASM_ULD(tmp3, 24, src)
		CNASM_MT2(tmp0, CVM_MT_HSH_DATW, 0)
		CNASM_MT2(tmp1, CVM_MT_HSH_DATW, 1)
		CNASM_MT2(tmp2, CVM_MT_HSH_DATW, 2)
		CNASM_MT2(tmp3, CVM_MT_HSH_DATW, 3)
		CNASM_ULD(tmp0, 32, src)
		CNASM_ULD(tmp1, 40, src)
		CNASM_ULD(tmp2, 48, src)
		CNASM_ULD(tmp3, 56, src)
		CNASM_MT2(tmp0, CVM_MT_HSH_DATW, 4)
		CNASM_MT2(tmp1, CVM_MT_HSH_DATW, 5)
		CNASM_MT2(tmp2, CVM_MT_HSH_DATW, 6)
		CNASM_MT2(tmp3, CVM_MT_HSH_DATW, 7)
		CNASM_ULD(tmp0, 64, src)
		CNASM_ULD(tmp1, 72, src)
		CNASM_ULD(tmp2, 80, src)
		CNASM_ULD(tmp3, 88, src)
		CNASM_MT2(tmp0, CVM_MT_HSH_DATW, 8)
		CNASM_MT2(tmp1, CVM_MT_HSH_DATW, 9)
		CNASM_MT2(tmp2, CVM_MT_HSH_DATW, 10)
		CNASM_MT2(tmp3, CVM_MT_HSH_DATW, 11)
		CNASM_ULD(tmp0, 96, src)
		CNASM_ULD(tmp1, 104, src)
		CNASM_ULD(tmp2, 112, src)
		CNASM_ULD(tmp3, 120, src)
		CNASM_MT2(tmp0, CVM_MT_HSH_DATW, 12)
		CNASM_MT2(tmp1, CVM_MT_HSH_DATW, 13)
		CNASM_MT2(tmp2, CVM_MT_HSH_DATW, 14)
		CNASM_MT2(tmp3, CVM_MT_HSH_STARTSHA512, 0)
		CNASM_END()
		: [tmp0] "=&r" (tmp0),
		  [tmp1] "=&r" (tmp1),
		  [tmp2] "=&r" (tmp2),
		  [tmp3] "=&r" (tmp3)
		: [src] "d" (src));
}

static inline void
octeon_cop2_sha512_update_unaligned(uint8_t *src)
{
	octeon_cop2_sha512_update_unaligned_vaddr64((intptr_t)src);
}

/* -------------------------------------------------------------------------- */

/* CRC */

/* XXX */

#ifdef notyet
static inline void
octeon_cop2_crc_polynomial(val)
{
	__asm __volatile (
		CNASM_START()
		"	dmtc2 %[val], 0x4200"
		CNASM_END()
		:
		: [val] "d" (val))
}

#define CVMX_MT_CRC_IV(val) \
            __asm __volatile (__PUSH "dmtc2 %0,0x0201" __POP :: "d"(val))
#define CVMX_MT_CRC_BYTE_REFLECT(val) \
            __asm __volatile (__PUSH "dmtc2 %0,0x0214" __POP :: "d"(val))
#define CVMX_MT_CRC_HALF_REFLECT(val) \
            __asm __volatile (__PUSH "dmtc2 %0,0x0215" __POP :: "d"(val))
#define CVMX_MT_CRC_DWORD_REFLECT(val) \
            __asm __volatile (__PUSH "dmtc2 %0,0x1217" __POP :: "d"(val))
#define CVMX_MF_CRC_IV_REFLECT(val) \
            __asm __volatile (__PUSH "dmfc2 %0,0x0203" __POP : "=d"(val))

static inline void
octeon_cop2_crc_reflect(XXX)
{
	__asm __volatile (
		CNASM_START()
	"	and	%[val], %[len], 15		\n"
	"	beq	%[val], %[len], 2f		\n"
	"	 subu	%[tmp], %[len], %[val]		\n"
	"	move	%[len], %[val]			\n"
	"	addu	%[tmp], %[buf]			\n"

	"	.align	3				\n"
	"1:						\n"
		CNASM_ULD(val, 0, buf)
	"	addu	%[buf], 16			\n"
		CNASM_MT2(val, CVM_MT_CRC_DWORD_REFLECT, 0)
		CNASM_ULD(val, -8, buf)
	"	bne	%[buf], %[tmp], 1b		\n"
		CNASM_MT2(val, CVM_MT_CRC_DWORD_REFLECT, 0)

	"	.align	3				\n"
	"2:	and	%[val], %[len], 1		\n"
	"	beq	%[val], %[len], 4f		\n"
	"	 subu	%[tmp], %[len], %[val]		\n"
	"	move	%[len], %[val]			\n"
	"	addu	%[tmp], %[buf]			\n"

	"	.align	3				\n"
	"3:	addu	%[buf], 2			\n"
	"	lhu	%[val], -2(%[buf])		\n"
	"	bne	%[buf], %[tmp], 3b		\n"
		CNASM_MT2(val, CVM_MT_CRC_HALF_REFLECT, 0)

	"	.align	3				\n"
	"4:	beqz	%[len], 5f			\n"
	"	 nop					\n"
	"	lbu	%[val], 0(%[buf])		\n"
		CNASM_MT2(val, CVM_MT_CRC_BYTE_REFLECT, 0)

	"	.align	3				\n"
	"5:						\n"
		CNASM_END()
		: [len] "=d" (len),
		  [buf] "=d" (buf),
		  [val] "=d" (val),
		  [tmp] "=d" (tmp)
		: "0" (len),
		  "1" (buf)
	);
#endif

/* -------------------------------------------------------------------------- */

/* GFM */

/* XXX */

#endif	/* _CN30XXCOP2VAR_H_ */
