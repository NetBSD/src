/* $Id: memcpy.c,v 1.1.2.2 1999/01/18 20:18:26 drochner Exp $ */

#include <sys/types.h>

void *memcpy __P((void *, const void *, size_t));

#define WORD(x, y)	*(u_int32_t *)((char *)(x) + (y))
#define	FRACTION(x)	((0 - (int)(x)) & 3)
#define	CHUNKCNT(x, y)	((x) & -(y))
#define	GOODCASE(x, y)	(0 == (0x3 & ((unsigned)(x) ^ (unsigned)(y))))

void *
memcpy(dst, src, len)
	void *dst;
	const void *src;
	size_t len;
{
	void *v0;
	unsigned v1;

#ifdef MEMCPY_MISUSE
	__asm __volatile(".globl memmove; subu $3,$4,$5; bltu $3,$6,memmove")
#endif
	v0 = dst;
	if (len >= 8) {
		if (GOODCASE(dst, src)) {
			v1 = FRACTION(src);
			if (v1 > 0) {
				len -= v1;
				asm(".set noat\n\t"
				    "lwr $1,0(%0)\n\t"
				    "nop\n\t"
				    "swr $1,0(%1)\n\t"
				    ".set at"
				    :: "r"(src), "r"(dst) : "$1");
				dst = (char *)dst + v1;
				src = (char *)src + v1;
			}
			v1 = CHUNKCNT(len, 64);
			if (v1 > 0) {
				len -= v1;
				v1 = (unsigned)dst + v1;
				do {
					unsigned t0, t1, t2, t3, t4;
					unsigned t5, t6, t7, t8, t9;

					asm(".set noreorder");
					t0 = WORD(src, 0);
					t1 = WORD(src, 16);
					t2 = WORD(src, 32);
					t3 = WORD(src, 48);
					asm(".set reorder");

					t4 = WORD(src, 4);
					t5 = WORD(src, 8);
					t6 = WORD(src, 12);

					t7 = WORD(src, 20);
					t8 = WORD(src, 24);
					t9 = WORD(src, 28);

					WORD(dst, 0)  = t0;
					WORD(dst, 4)  = t4;
					WORD(dst, 8)  = t5;
					WORD(dst, 12) = t6;

					WORD(dst, 16) = t1;
					WORD(dst, 20) = t7;
					WORD(dst, 24) = t8;
					WORD(dst, 28) = t9;

					t4 = WORD(src, 36);
					t5 = WORD(src, 40);
					t6 = WORD(src, 44);

					t7 = WORD(src, 52);
					t8 = WORD(src, 56);
					t9 = WORD(src, 60);

					WORD(dst, 32) = t2;
					WORD(dst, 36) = t4;
					WORD(dst, 40) = t5;
					WORD(dst, 44) = t6;

					WORD(dst, 48) = t3;
					WORD(dst, 52) = t7;
					WORD(dst, 56) = t8;
					WORD(dst, 60) = t9;
					
					dst = (char *)dst + 64;
					src = (char *)src + 64;
				} while (dst != (void *)v1);
			}
			v1 = CHUNKCNT(len, 4);
			if (v1 > 0) {
				len -= v1;
				v1 = (unsigned)dst + v1;
				do {
					WORD(dst, 0) = WORD(src, 0);
					dst = (char *)dst + 4;
					src = (char *)src + 4;
				} while (dst != (void *)v1);
			}
		}
		else {
			v1 = FRACTION(dst);
			if (v1 > 0) {
				len -= v1;
				asm(".set noat\n\t"
				    "lwr $1,0(%0)\n\t"
				    "lwl $1,3(%0)\n\t"
				    "nop\n\t"
				    "swr $1,0(%1)\n\t"
				    ".set at"
				    :: "r"(src), "r"(dst) : "$1");
				dst = (char *)dst + v1;
				src = (char *)src + v1;
			}
			v1 = CHUNKCNT(len, 4);
			len -= v1;
			v1 = (unsigned)dst + v1;
			do {
				asm(".set noat\n\t"
				    "lwr $1,0(%0)\n\t"
				    "lwl $1,3(%0)\n\t"
				    "nop\n\t"
				    "sw	 $1,0(%1)\n\t"
				    ".set at"
				    :: "r"(src), "r"(dst) : "$1");
				dst = (char *)dst + 4;
				src = (char *)src + 4;
			} while (dst != (void *)v1);
		}
	}
	if (len > 0) {
		v1 = (unsigned)dst + len;
		do {
			*(u_int8_t *)dst = *(u_int8_t *)src;
			dst = (char *)dst + 1;
			src = (char *)src + 1;
		} while (dst != (void *)v1);
	}
	return v0;
}
