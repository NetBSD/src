/*	$KAME: algorithm.h,v 1.12 2001/03/21 22:38:29 sakane Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* algorithm strength */
enum {
	algstrength_ehigh,
	algstrength_high,
	algstrength_normal,
#define MAXALGSTRENGTH	3
};

struct algorithm_strength {
	/*
	 * algorithm type N is mapped to 1 << (N - 1).
	 * N of 0 is not encoded, that is, it means nothing.
	 */
	u_int32_t algtype[MAXALGSTRENGTH];
};

/* algorithm class */
enum {
	algclass_ipsec_enc,
	algclass_ipsec_auth,
	algclass_ipsec_comp,
	algclass_isakmp_enc,
	algclass_isakmp_hash,
	algclass_isakmp_dh,
	algclass_isakmp_ameth,	/* authentication method. */
#define MAXALGCLASS	7
};

#define ALG_DEFAULT_KEYLEN	64

#define ALGTYPE_NOTHING		0

/* algorithm type */
enum {
	/* enc */
	algtype_nothing = 0,
	algtype_des_iv64, algtype_des, algtype_3des,
	algtype_rc5, algtype_idea, algtype_cast128, algtype_blowfish,
	algtype_3idea, algtype_des_iv32, algtype_rc4, algtype_null_enc,
	algtype_rijndael, algtype_twofish,
};

enum {
	/* ipsec auth */
	/* 0 is defined as algtype_nothing above. */
	algtype_hmac_md5 = 1, algtype_hmac_sha1, algtype_des_mac, algtype_kpdk,
	algtype_non_auth,
};

enum {
	/* ipcomp */
	/* 0 is defined as algtype_nothing above. */
	algtype_oui = 1, algtype_deflate, algtype_lzs,
};

enum {
	/* hash */
	/* 0 is defined as algtype_nothing above. */
	algtype_md5 = 1, algtype_sha1, algtype_tiger,
};

enum {
	/* dh_group */
	/* 0 is defined as algtype_nothing above. */
	algtype_modp768 = 1, algtype_modp1024,
	algtype_ec2n155, algtype_ec2n185,
	algtype_modp1536,
};

enum {
	/* authentication method. */
	/* 0 is defined as algtype_nothing above. */
	algtype_psk = 1, algtype_dsssig, algtype_rsasig,
	algtype_rsaenc, algtype_rsarev, algtype_gssapikrb
};

extern int default_keylen __P((int, int));
extern int check_keylen __P((int, int, int));
extern int algtype2doi __P((int, int));
extern int algclass2doi __P((int));
extern struct algorithm_strength **initalgstrength __P((void));
extern void flushalgstrength __P((struct algorithm_strength **));
