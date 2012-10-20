/* Paulo Barreto <pbarreto@unisys.com.br> 1996.01.17 */
/*
 *	idea.c - C source code for IDEA block cipher.
 *	IDEA (International Data Encryption Algorithm), formerly known as 
 *	IPES (Improved Proposed Encryption Standard).
 *	Algorithm developed by Xuejia Lai and James L. Massey, of ETH Zurich.
 *	This implementation modified and derived from original C code 
 *	developed by Xuejia Lai.  
 *	Zero-based indexing added, names changed from IPES to IDEA.
 *	CFB functions added.  Random number routines added.
 *
 *	Extensively optimized and restructured by Colin Plumb.
 *
 *	The IDEA(tm) block cipher is covered by patents held by ETH and a
 *	Swiss company called Ascom-Tech AG.  The Swiss patent number is
 *	PCT/CH91/00117, the European patent number is EP 0 482 154 B1, and
 *	the U.S. patent number is US005214703.  IDEA(tm) is a trademark of
 *	Ascom-Tech AG.  There is no license fee required for noncommercial
 *	use.  Commercial users may obtain licensing details from Dieter
 *	Profos, Ascom Tech AG, Solothurn Lab, Postfach 151, 4502 Solothurn,
 *	Switzerland, Tel +41 65 242885, Fax +41 65 235761.
 *
 *	The IDEA block cipher uses a 64-bit block size, and a 128-bit key 
 *	size.  It breaks the 64-bit cipher block into four 16-bit words
 *	because all of the primitive inner operations are done with 16-bit
 *	arithmetic.  It likewise breaks the 128-bit cipher key into eight 
 *	16-bit words.
 *
 *	For further information on the IDEA cipher, see the book:
 *	  Xuejia Lai, "On the Design and Security of Block Ciphers",
 *	  ETH Series on Information Processing (ed. J.L. Massey) Vol 1,
 *	  Hartung-Gorre Verlag, Konstanz, Switzerland, 1992.  ISBN
 *	  3-89191-573-X.
 *
 *	This code runs on arrays of bytes by taking pairs in big-endian
 *	order to make the 16-bit words that IDEA uses internally.  This
 *	produces the same result regardless of the byte order of the
 *	native CPU.
 */
#include <inttypes.h>
#include <string.h>

#include "idea.h"

#define OPTIMIZE 

#define LOW16(x) ((x) & 0xFFFF)

/*
 * Compute the multiplicative inverse of x, modulo 65537, using Euclid's
 * algorithm. It is unrolled twice to avoid swapping the registers each
 * iteration, and some subtracts of t have been changed to adds.
 */
static __inline uint16_t
mulInv(uint16_t x)     
{
	uint16_t t0, t1;
	uint16_t q, y;

	if (x <= 1)
		return x;	/* 0 and 1 are self-inverse */
	t1 = 0x10001 / x;	/* Since x >= 2, this fits into 16 bits */
	y = 0x10001 % x;
	if (y == 1)
		return ( uint16_t ) LOW16(1-t1);
	t0 = 1;
	do {
		q = x / y;
		x = x % y;
		t0 += q * t1;
		if (x == 1)
			return t0;
		q = y / x;
		y = y % x;
		t1 += q * t0;
	} while (y != 1);
	return ( uint16_t ) LOW16(1-t1);
}

/*
 * Expand a 128-bit user key to a working encryption key EK
 */
static void
ideaExpandKey(const uint8_t *userkey, uint16_t *EK)
{
	unsigned i,j;

	for (j=0; j<8; j++) {
		EK[j] = (userkey[0]<<8) + userkey[1];
		userkey += 2;
	}
	for (i=0; j < IDEA_KEYLEN; j++) {
		i++;
		EK[i+7] = (EK[i & 7] << 9) | (EK[(i + 1) & 7] >> 7);
		EK += i & 8;
		i &= 7;
	}
}

#define NEG(x) (- (int) (x))

/*
 * Compute IDEA decryption key DK from an expanded IDEA encryption key EK
 * Note that the input and output may be the same.  Thus, the key is
 * inverted into an internal buffer, and then copied to the output.
 */
static void
ideaInvertKey(const uint16_t *EK, uint16_t DK[IDEA_KEYLEN])
{
	uint16_t temp[IDEA_KEYLEN];
#ifdef OPTIMIZE
	register int k, p, r;

	p = IDEA_KEYLEN;
	temp [p-1] = mulInv (EK [3]);
	temp [p-2] = NEG (EK [2]);
	temp [p-3] = NEG (EK [1]);
	temp [p-4] = mulInv (EK [0]);

	k = 4; p -= 4;
	for (r = IDEA_ROUNDS - 1; r > 0; r--) {
		temp [p-1] = EK [k+1];
		temp [p-2] = EK [k];
		temp [p-3] = mulInv (EK [k+5]);
		temp [p-4] = NEG (EK [k+3]);
		temp [p-5] = NEG (EK [k+4]);
		temp [p-6] = mulInv (EK [k+2]);
		k += 6; p -= 6;
	}
	temp [p-1] = EK [k+1];
	temp [p-2] = EK [k];
	temp [p-3] = mulInv (EK [k+5]);
	temp [p-4] = NEG (EK [k+4]);
	temp [p-5] = NEG (EK [k+3]);
	temp [p-6] = mulInv (EK [k+2]);
#else  /* !OPTIMIZE */
	int i;
	uint16_t t1, t2, t3;
	uint16_t *p = temp + IDEA_KEYLEN;

	t1 = mulInv(*EK++);
	t2 = - ( int ) *EK++;
	t3 = - ( int ) *EK++;
	*--p = mulInv(*EK++);
	*--p = t3;
	*--p = t2;
	*--p = t1;

	for (i = 0; i < IDEA_ROUNDS-1; i++) {
		t1 = *EK++;
		*--p = *EK++;
		*--p = t1;

		t1 = mulInv(*EK++);
		t2 = - ( int ) *EK++;
		t3 = - ( int ) *EK++;
		*--p = mulInv(*EK++);
		*--p = t2;
		*--p = t3;
		*--p = t1;
	}
	t1 = *EK++;
	*--p = *EK++;
	*--p = t1;

	t1 = mulInv(*EK++);
	t2 = - ( int ) *EK++;
	t3 = - ( int ) *EK++;
	*--p = mulInv(*EK++);
	*--p = t3;
	*--p = t2;
	*--p = t1;
#endif /* ?OPTIMIZE */
/* Copy and destroy temp copy */
	memcpy(DK, temp, sizeof(temp));
	memset(temp, 0x0, sizeof(temp));
}

/*
 * MUL(x,y) computes x = x*y, modulo 0x10001.  Requires two temps, 
 * t16 and t32.  x is modified, and must me a side-effect-free lvalue.
 * y may be anything, but unlike x, must be strictly 16 bits even if
 * LOW16() is #defined.
 * All of these are equivalent - see which is faster on your machine
 */
#define MUL(x,y) \
	((t16 = (y)) ? \
		(x=LOW16(x)) ? \
			t32 = (uint32_t)x*t16, \
			x = LOW16(t32), \
			t16 = t32>>16, \
			x = (x-t16)+(x<t16) \
		: \
			(x = 1-t16) \
	: \
		(x = 1-x))

/*	IDEA encryption/decryption algorithm */
/* Note that in and out can be the same buffer */
static void
ideaCipher(const uint8_t inbuf[8], uint8_t outbuf[8], const uint16_t *key)
{
	register uint16_t x1, x2, x3, x4, s2, s3;
	const uint16_t *in;
	uint16_t *out;
	register uint16_t t16;	/* Temporaries needed by MUL macro */
	register uint32_t t32;
#ifndef OPTIMIZE
	int r = IDEA_ROUNDS;
#endif /* ?OPTIMIZE */
	int	indian = 1;

	in = (const uint16_t *)(const void *)inbuf;
#ifdef OPTIMIZE
	x1 = in[0];  x2 = in[1];
	x3 = in[2];  x4 = in[3];
#else  /* !OPTIMIZE */
	x1 = *in++;  x2 = *in++;
	x3 = *in++;  x4 = *in;
#endif /* ?OPTIMIZE */
	if (*(char *)(void *)&indian) {
		/* little endian */
		x1 = (x1>>8) | (x1<<8);
		x2 = (x2>>8) | (x2<<8);
		x3 = (x3>>8) | (x3<<8);
		x4 = (x4>>8) | (x4<<8);
	}

#ifdef OPTIMIZE
	/* round 1: */
	MUL(x1,key[0]);
	x2 += key[1];
	x3 += key[2];
	MUL(x4, key[3]);

	s3 = x3;
	x3 ^= x1;
	MUL(x3, key[4]);
	s2 = x2;
	x2 ^= x4;
	x2 += x3;
	MUL(x2, key[5]);
	x3 += x2;

	x1 ^= x2;  x4 ^= x3;
	x2 ^= s3;  x3 ^= s2;

	/* round 2: */
	MUL(x1,key[6]);
	x2 += key[7];
	x3 += key[8];
	MUL(x4, key[9]);

	s3 = x3;
	x3 ^= x1;
	MUL(x3, key[10]);
	s2 = x2;
	x2 ^= x4;
	x2 += x3;
	MUL(x2, key[11]);
	x3 += x2;

	x1 ^= x2;  x4 ^= x3;
	x2 ^= s3;  x3 ^= s2;

	/* round 3: */
	MUL(x1,key[12]);
	x2 += key[13];
	x3 += key[14];
	MUL(x4, key[15]);

	s3 = x3;
	x3 ^= x1;
	MUL(x3, key[16]);
	s2 = x2;
	x2 ^= x4;
	x2 += x3;
	MUL(x2, key[17]);
	x3 += x2;

	x1 ^= x2;  x4 ^= x3;
	x2 ^= s3;  x3 ^= s2;

	/* round 4: */
	MUL(x1,key[18]);
	x2 += key[19];
	x3 += key[20];
	MUL(x4, key[21]);

	s3 = x3;
	x3 ^= x1;
	MUL(x3, key[22]);
	s2 = x2;
	x2 ^= x4;
	x2 += x3;
	MUL(x2, key[23]);
	x3 += x2;

	x1 ^= x2;  x4 ^= x3;
	x2 ^= s3;  x3 ^= s2;

	/* round 5: */
	MUL(x1,key[24]);
	x2 += key[25];
	x3 += key[26];
	MUL(x4, key[27]);

	s3 = x3;
	x3 ^= x1;
	MUL(x3, key[28]);
	s2 = x2;
	x2 ^= x4;
	x2 += x3;
	MUL(x2, key[29]);
	x3 += x2;

	x1 ^= x2;  x4 ^= x3;
	x2 ^= s3;  x3 ^= s2;

	/* round 6: */
	MUL(x1,key[30]);
	x2 += key[31];
	x3 += key[32];
	MUL(x4, key[33]);

	s3 = x3;
	x3 ^= x1;
	MUL(x3, key[34]);
	s2 = x2;
	x2 ^= x4;
	x2 += x3;
	MUL(x2, key[35]);
	x3 += x2;

	x1 ^= x2;  x4 ^= x3;
	x2 ^= s3;  x3 ^= s2;

	/* round 7: */
	MUL(x1,key[36]);
	x2 += key[37];
	x3 += key[38];
	MUL(x4, key[39]);

	s3 = x3;
	x3 ^= x1;
	MUL(x3, key[40]);
	s2 = x2;
	x2 ^= x4;
	x2 += x3;
	MUL(x2, key[41]);
	x3 += x2;

	x1 ^= x2;  x4 ^= x3;
	x2 ^= s3;  x3 ^= s2;

	/* round 8: */
	MUL(x1,key[42]);
	x2 += key[43];
	x3 += key[44];
	MUL(x4, key[45]);

	s3 = x3;
	x3 ^= x1;
	MUL(x3, key[46]);
	s2 = x2;
	x2 ^= x4;
	x2 += x3;
	MUL(x2, key[47]);
	x3 += x2;

	x1 ^= x2;  x4 ^= x3;
	x2 ^= s3;  x3 ^= s2;

	/* final semiround: */
	MUL(x1, key[48]);
	x3 += key[49];
	x2 += key[50];
	MUL(x4, key[51]);

#else  /* !OPTIMIZE */
	do {
		MUL(x1,*key++);
		x2 += *key++;
		x3 += *key++;
		MUL(x4, *key++);

		s3 = x3;
		x3 ^= x1;
		MUL(x3, *key++);
		s2 = x2;
		x2 ^= x4;
		x2 += x3;
		MUL(x2, *key++);
		x3 += x2;

		x1 ^= x2;  x4 ^= x3;

		x2 ^= s3;  x3 ^= s2;
	} while (--r);
	MUL(x1, *key++);
	x3 += *key++;
	x2 += *key++;
	MUL(x4, *key);
#endif /* ?OPTIMIZE */

	out = (uint16_t *)outbuf;
	if (*(char *)(void *)&indian) {
		/* little endian */
#ifdef OPTIMIZE
		out[0] = (x1>>8) | (x1<<8);
		out[1] = (x3>>8) | (x3<<8);
		out[2] = (x2>>8) | (x2<<8);
		out[3] = (x4>>8) | (x4<<8);
#else  /* !OPTIMIZE */
		x1 = LOW16(x1);
		x2 = LOW16(x2);
		x3 = LOW16(x3);
		x4 = LOW16(x4);
		*out++ = (x1>>8) | (x1<<8);
		*out++ = (x3>>8) | (x3<<8);
		*out++ = (x2>>8) | (x2<<8);
		*out   = (x4>>8) | (x4<<8);
#endif /* ?OPTIMIZE */
	} else {
		/* big endian */
		*out++ = x1;
		*out++ = x3;
		*out++ = x2;
		*out = x4;
	}
}

int
idea_set_key(idea_context_t *ctx, uint8_t *key, unsigned keylen)
{
	ctx->have_decrypt_key = 0;
	ideaExpandKey(key, ctx->enckey);
	ideaInvertKey(ctx->enckey, ctx->deckey);
	return 0;
}

void
idea_encrypt_block(idea_context_t *ctx, uint8_t *out, uint8_t *in)
{
	ideaCipher(out, in, ctx->enckey);
}

void
idea_decrypt_block(idea_context_t *ctx, uint8_t *out, uint8_t *in)
{
	if (!ctx->have_decrypt_key) {
		ctx->have_decrypt_key = 1;
		ideaInvertKey(ctx->enckey, ctx->deckey);
	}
	ideaCipher(out, in, ctx->deckey);
}

/*-------------------------------------------------------------*/

#ifdef TEST

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
/*
 * This is the number of Kbytes of test data to encrypt.
 * It defaults to 1 MByte.
 */
#ifndef BLOCKS
  #ifndef KBYTES
	#define KBYTES 1024L
  #endif
  #define BLOCKS (64*KBYTES)
#endif

int
main(void)
{	/* Test driver for IDEA cipher */
	idea_context_t	ctx;
	int i, k;
	uint8_t userkey[16];
	uint8_t XX[8], YY[8], ZZ[8];
	clock_t start, end;
	long l;
	float secs;

	memset(&ctx, 0x0, sizeof(ctx));
	/* Make a sample user key for testing... */
	for(i=0; i<16; i++)
		userkey[i] = i+1;

	idea_set_key(&ctx, userkey, sizeof(userkey));

	/* Make a sample plaintext pattern for testing... */
	for (k=0; k<8; k++)
		XX[k] = k;

	printf("\nEncrypting %ld bytes (%ld blocks)...", BLOCKS*16, BLOCKS);
	fflush(stdout);
	start = clock();
	memcpy(YY, XX, sizeof(YY));
	for (l = 0; l < BLOCKS; l++)
		idea_encrypt_block(&ctx, YY, YY);	/* repeated encryption */
	memcpy(ZZ, YY, sizeof(ZZ));
	for (l = 0; l < BLOCKS; l++)
		idea_decrypt_block(&ctx, ZZ, ZZ);	/* repeated decryption */
	end = clock() - start;
	secs = (float) end / CLOCKS_PER_SEC;
	printf("%.2f sec = %.0f bytes/sec.\n",
		secs, (float) BLOCKS*16 / secs);

	printf("\nX %3u  %3u  %3u  %3u  %3u  %3u  %3u %3u\n",
	  XX[0], XX[1],  XX[2], XX[3], XX[4], XX[5],  XX[6], XX[7]);
	printf("\nY %3u  %3u  %3u  %3u  %3u  %3u  %3u %3u\n",
	  YY[0], YY[1],  YY[2], YY[3], YY[4], YY[5],  YY[6], YY[7]);
	printf("\nZ %3u  %3u  %3u  %3u  %3u  %3u  %3u %3u\n",    
	  ZZ[0], ZZ[1],  ZZ[2], ZZ[3], ZZ[4], ZZ[5],  ZZ[6], ZZ[7]);

	/* Now decrypted ZZ should be same as original XX */
	for (k=0; k<8; k++)
		if (XX[k] != ZZ[k]) {
			printf("\n\07Error!  Noninvertable encryption.\n");
			exit(1);	/* error exit */ 
		}
	printf("\nNormal exit.\n");
	return 0;	/* normal exit */
}

#endif /* TEST */