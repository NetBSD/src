/*	$KAME: oakley.h,v 1.16 2000/12/12 16:59:42 thorpej Exp $	*/

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

/* refer to RFC 2409 */

/* Attribute Classes */
#define OAKLEY_ATTR_ENC_ALG		1 /* B */
#define   OAKLEY_ATTR_ENC_ALG_DES		1
#define   OAKLEY_ATTR_ENC_ALG_IDEA		2
#define   OAKLEY_ATTR_ENC_ALG_BLOWFISH		3
#define   OAKLEY_ATTR_ENC_ALG_RC5		4
#define   OAKLEY_ATTR_ENC_ALG_3DES		5
#define   OAKLEY_ATTR_ENC_ALG_CAST		6
#define   OAKLEY_ATTR_ENC_ALG_RIJNDAEL		7
#define   OAKLEY_ATTR_ENC_ALG_AES		7
#define OAKLEY_ATTR_HASH_ALG		2 /* B */
#define   OAKLEY_ATTR_HASH_ALG_MD5		1
#define   OAKLEY_ATTR_HASH_ALG_SHA		2
#define   OAKLEY_ATTR_HASH_ALG_TIGER		3
#define   OAKLEY_ATTR_HASH_ALG_SHA2_256		4
#define   OAKLEY_ATTR_HASH_ALG_SHA2_384		5
#define   OAKLEY_ATTR_HASH_ALG_SHA2_512		6
#define OAKLEY_ATTR_AUTH_METHOD		3 /* B */
#define   OAKLEY_ATTR_AUTH_METHOD_PSKEY		1
#define   OAKLEY_ATTR_AUTH_METHOD_DSSSIG	2
#define   OAKLEY_ATTR_AUTH_METHOD_RSASIG	3
#define   OAKLEY_ATTR_AUTH_METHOD_RSAENC	4
#define   OAKLEY_ATTR_AUTH_METHOD_RSAREV	5
#define   OAKLEY_ATTR_AUTH_METHOD_EGENC		6
#define   OAKLEY_ATTR_AUTH_METHOD_EGREV		7
#define   OAKLEY_ATTR_AUTH_METHOD_GSSAPI_KRB	65001
#define OAKLEY_ATTR_GRP_DESC		4 /* B */
#define   OAKLEY_ATTR_GRP_DESC_MODP768		1
#define   OAKLEY_ATTR_GRP_DESC_MODP1024		2
#define   OAKLEY_ATTR_GRP_DESC_EC2N155		3
#define   OAKLEY_ATTR_GRP_DESC_EC2N185		4
#define   OAKLEY_ATTR_GRP_DESC_MODP1536		5
#define OAKLEY_ATTR_GRP_TYPE		5 /* B */
#define   OAKLEY_ATTR_GRP_TYPE_MODP		1
#define   OAKLEY_ATTR_GRP_TYPE_ECP		2
#define   OAKLEY_ATTR_GRP_TYPE_EC2N		3
#define OAKLEY_ATTR_GRP_PI		6 /* V */
#define OAKLEY_ATTR_GRP_GEN_ONE		7 /* V */
#define OAKLEY_ATTR_GRP_GEN_TWO		8 /* V */
#define OAKLEY_ATTR_GRP_CURVE_A		9 /* V */
#define OAKLEY_ATTR_GRP_CURVE_B		10 /* V */
#define OAKLEY_ATTR_SA_LD_TYPE		11 /* B */
#define   OAKLEY_ATTR_SA_LD_TYPE_DEFAULT	1
#define   OAKLEY_ATTR_SA_LD_TYPE_SEC		1
#define   OAKLEY_ATTR_SA_LD_TYPE_KB		2
#define   OAKLEY_ATTR_SA_LD_TYPE_MAX		3
#define OAKLEY_ATTR_SA_LD		12 /* V */
#define   OAKLEY_ATTR_SA_LD_SEC_DEFAULT		28800 /* 8 hours */
#define OAKLEY_ATTR_PRF			13 /* B */
#define OAKLEY_ATTR_KEY_LEN		14 /* B */
#define OAKLEY_ATTR_FIELD_SIZE		15 /* B */
#define OAKLEY_ATTR_GRP_ORDER		16 /* V */
#define OAKLEY_ATTR_BLOCK_SIZE		17 /* B */
#define OAKLEY_ATTR_GSS_ID		16384

#define OAKLEY_PRIME_MODP768 \
	"FFFFFFFF FFFFFFFF C90FDAA2 2168C234 C4C6628B 80DC1CD1" \
	"29024E08 8A67CC74 020BBEA6 3B139B22 514A0879 8E3404DD" \
	"EF9519B3 CD3A431B 302B0A6D F25F1437 4FE1356D 6D51C245" \
	"E485B576 625E7EC6 F44C42E9 A63A3620 FFFFFFFF FFFFFFFF"

#define OAKLEY_PRIME_MODP1024 \
	"FFFFFFFF FFFFFFFF C90FDAA2 2168C234 C4C6628B 80DC1CD1" \
	"29024E08 8A67CC74 020BBEA6 3B139B22 514A0879 8E3404DD" \
	"EF9519B3 CD3A431B 302B0A6D F25F1437 4FE1356D 6D51C245" \
	"E485B576 625E7EC6 F44C42E9 A637ED6B 0BFF5CB6 F406B7ED" \
	"EE386BFB 5A899FA5 AE9F2411 7C4B1FE6 49286651 ECE65381" \
	"FFFFFFFF FFFFFFFF"

#define OAKLEY_PRIME_MODP1536 \
	"FFFFFFFF FFFFFFFF C90FDAA2 2168C234 C4C6628B 80DC1CD1" \
	"29024E08 8A67CC74 020BBEA6 3B139B22 514A0879 8E3404DD" \
	"EF9519B3 CD3A431B 302B0A6D F25F1437 4FE1356D 6D51C245" \
	"E485B576 625E7EC6 F44C42E9 A637ED6B 0BFF5CB6 F406B7ED" \
	"EE386BFB 5A899FA5 AE9F2411 7C4B1FE6 49286651 ECE45B3D" \
	"C2007CB8 A163BF05 98DA4836 1C55D39A 69163FA8 FD24CF5F" \
	"83655D23 DCA3AD96 1C62F356 208552BB 9ED52907 7096966D" \
	"670C354E 4ABC9804 F1746C08 CA237327 FFFFFFFF FFFFFFFF"

#define MAXPADLWORD	20

struct cipher_algorithm {
	char *name;
	vchar_t *(*encrypt) __P((vchar_t *data, vchar_t *key, caddr_t iv));
	vchar_t *(*decrypt) __P((vchar_t *data, vchar_t *key, caddr_t iv));
	int (*weakkey) __P((vchar_t *key));
};

struct dhgroup {
	int type;
	vchar_t *prime;
	int gen1;
	int gen2;
	vchar_t *curve_a;
	vchar_t *curve_b;
	vchar_t *order;
};

#define MAXDHGROUP	10

extern struct dhgroup dhgroup[MAXDHGROUP];

/* certificate holder */
typedef struct cert_t_tag {
	u_int8_t type;		/* type of CERT, must be same to buf->v[0]*/
	vchar_t cert;		/* pointer to the CERT */
	vchar_t *pl;		/* CERT payload */
} cert_t;

struct ph1handle;
struct ph2handle;
struct isakmp_ivm;

extern int oakley_get_defaultlifetime __P((void));

extern void oakley_dhinit __P((void));
extern void oakley_dhgrp_free __P((struct dhgroup *));
extern int oakley_dh_compute __P((const struct dhgroup *,
	vchar_t *, vchar_t *, vchar_t *, vchar_t **));
extern int oakley_dh_generate __P((const struct dhgroup *,
	vchar_t **, vchar_t **));
extern int oakley_setdhgroup __P((int, struct dhgroup **));

extern vchar_t *oakley_prf __P((vchar_t *, vchar_t *, struct ph1handle *));
extern vchar_t *oakley_hash __P((vchar_t *, struct ph1handle *));

extern int oakley_compute_keymat __P((struct ph2handle *, int));

#if notyet
extern vchar_t *oakley_compute_hashx __P((void));
#endif
extern vchar_t *oakley_compute_hash3 __P((struct ph1handle *,
	u_int32_t, vchar_t *));
extern vchar_t *oakley_compute_hash1 __P((struct ph1handle *,
	u_int32_t, vchar_t *));
extern vchar_t *oakley_ph1hash_common __P((struct ph1handle *, int));
extern vchar_t *oakley_ph1hash_base_i __P((struct ph1handle *, int));
extern vchar_t *oakley_ph1hash_base_r __P((struct ph1handle *, int));

extern int oakley_validate_auth __P((struct ph1handle *));
#ifdef HAVE_SIGNING_C
extern int oakley_getmycert __P((struct ph1handle *));
extern int oakley_getsign __P((struct ph1handle *));
extern vchar_t *oakley_getcr __P((struct ph1handle *));
extern int oakley_checkcr __P((struct ph1handle *));
#endif
extern int oakley_needcr __P((int));
struct isakmp_gen;
extern int oakley_savecert __P((struct ph1handle *, struct isakmp_gen *));
extern int oakley_savecr __P((struct ph1handle *, struct isakmp_gen *));

extern int oakley_skeyid __P((struct ph1handle *));
extern int oakley_skeyid_dae __P((struct ph1handle *));

extern int oakley_compute_enckey __P((struct ph1handle *));
extern cert_t *oakley_newcert __P((void));
extern void oakley_delcert __P((cert_t *));
extern int oakley_newiv __P((struct ph1handle *));
extern struct isakmp_ivm *oakley_newiv2 __P((struct ph1handle *, u_int32_t));
extern void oakley_delivm __P((struct isakmp_ivm *));
extern vchar_t *oakley_do_decrypt __P((struct ph1handle *,
	vchar_t *, vchar_t *, vchar_t *));
extern vchar_t *oakley_do_encrypt __P((struct ph1handle *,
	vchar_t *, vchar_t *, vchar_t *));
extern int oakley_padlen __P((int));
