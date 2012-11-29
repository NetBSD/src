/*	$NetBSD: crypto_openssl.c,v 1.22 2012/11/29 15:31:24 vanhu Exp $	*/

/* Id: crypto_openssl.c,v 1.47 2006/05/06 20:42:09 manubsd Exp */

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

#include "config.h"

#include <sys/types.h>
#include <sys/param.h>

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

/* get openssl/ssleay version number */
#include <openssl/opensslv.h>

#if !defined(OPENSSL_VERSION_NUMBER) || (OPENSSL_VERSION_NUMBER < 0x0090602fL)
#error OpenSSL version 0.9.6 or later required.
#endif

#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/x509_vfy.h>
#include <openssl/bn.h>
#include <openssl/dh.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/des.h>
#include <openssl/crypto.h>
#ifdef HAVE_OPENSSL_ENGINE_H
#include <openssl/engine.h>
#endif
#include <openssl/blowfish.h>
#include <openssl/cast.h>
#include <openssl/err.h>
#ifdef HAVE_OPENSSL_RC5_H
#include <openssl/rc5.h>
#endif
#ifdef HAVE_OPENSSL_IDEA_H
#include <openssl/idea.h>
#endif
#if defined(HAVE_OPENSSL_AES_H)
#include <openssl/aes.h>
#elif defined(HAVE_OPENSSL_RIJNDAEL_H)
#include <openssl/rijndael.h>
#else
#include "crypto/rijndael/rijndael-api-fst.h"
#endif
#if defined(HAVE_OPENSSL_CAMELLIA_H)
#include <openssl/camellia.h>
#endif
#ifdef WITH_SHA2
#ifdef HAVE_OPENSSL_SHA2_H
#include <openssl/sha2.h>
#else
#include "crypto/sha2/sha2.h"
#endif
#endif
#include "plog.h"

/* 0.9.7 stuff? */
#if OPENSSL_VERSION_NUMBER < 0x0090700fL
typedef STACK_OF(GENERAL_NAME) GENERAL_NAMES;
#else
#define USE_NEW_DES_API
#endif

#define OpenSSL_BUG()	do { plog(LLV_ERROR, LOCATION, NULL, "OpenSSL function failed\n"); } while(0)

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "crypto_openssl.h"
#include "debug.h"
#include "gcmalloc.h"
#include "isakmp.h"

/*
 * I hate to cast every parameter to des_xx into void *, but it is
 * necessary for SSLeay/OpenSSL portability.  It sucks.
 */

static int cb_check_cert_local __P((int, X509_STORE_CTX *));
static int cb_check_cert_remote __P((int, X509_STORE_CTX *));
static X509 *mem2x509 __P((vchar_t *));

static caddr_t eay_hmac_init __P((vchar_t *, const EVP_MD *));

/* X509 Certificate */
/*
 * convert the string of the subject name into DER
 * e.g. str = "C=JP, ST=Kanagawa";
 */
vchar_t *
eay_str2asn1dn(str, len)
	const char *str;
	int len;
{
	X509_NAME *name;
	char *buf, *dst;
	char *field, *value;
	int i;
	vchar_t *ret = NULL;
	caddr_t p;

	if (len == -1)
		len = strlen(str);

	buf = racoon_malloc(len + 1);
	if (!buf) {
		plog(LLV_WARNING, LOCATION, NULL,"failed to allocate buffer\n");
		return NULL;
	}
	memcpy(buf, str, len);

	name = X509_NAME_new();

	dst = field = &buf[0];
	value = NULL;
	for (i = 0; i < len; i++) {
		if (buf[i] == '\\') {
			/* Escape characters specified in RFC 2253 */
			if (i < len - 1 &&
			    strchr("\\,=+<>#;", buf[i+1]) != NULL) {
				*dst++ = buf[++i];
				continue;
			} else if (i < len - 2) {
				/* RFC 2253 hexpair character escape */
				long u;
				char esc_str[3];
				char *endptr;

				esc_str[0] = buf[++i];
				esc_str[1] = buf[++i];
				esc_str[2] = '\0';
				u = strtol(esc_str, &endptr, 16);
				if (*endptr != '\0' || u < 0 || u > 255)
					goto err;
				*dst++ = u;
				continue;
			} else
				goto err;
		}
		if (!value && buf[i] == '=') {
			*dst = '\0';
			dst = value = &buf[i + 1];
			continue;
		} else if (buf[i] == ',' || buf[i] == '/') {
			*dst = '\0';

			plog(LLV_DEBUG, LOCATION, NULL, "DN: %s=%s\n",
			     field, value);

			if (!value) goto err;
			if (!X509_NAME_add_entry_by_txt(name, field,
					(value[0] == '*' && value[1] == 0) ? 
						V_ASN1_PRINTABLESTRING : MBSTRING_ASC,
					(unsigned char *) value, -1, -1, 0)) {
				plog(LLV_ERROR, LOCATION, NULL, 
				     "Invalid DN field: %s=%s\n",
				     field, value);
				plog(LLV_ERROR, LOCATION, NULL, 
				     "%s\n", eay_strerror());
				goto err;
			}

			while (i + 1 < len && buf[i + 1] == ' ') i++;
			dst = field = &buf[i + 1];
			value = NULL;
			continue;
		} else {
			*dst++  = buf[i];
		}
	}
	*dst = '\0';

	plog(LLV_DEBUG, LOCATION, NULL, "DN: %s=%s\n",
	     field, value);

	if (!value) goto err;
	if (!X509_NAME_add_entry_by_txt(name, field,
			(value[0] == '*' && value[1] == 0) ? 
				V_ASN1_PRINTABLESTRING : MBSTRING_ASC,
			(unsigned char *) value, -1, -1, 0)) {
		plog(LLV_ERROR, LOCATION, NULL, 
		     "Invalid DN field: %s=%s\n",
		     field, value);
		plog(LLV_ERROR, LOCATION, NULL, 
		     "%s\n", eay_strerror());
		goto err;
	}

	i = i2d_X509_NAME(name, NULL);
	if (!i)
		goto err;
	ret = vmalloc(i);
	if (!ret)
		goto err;
	p = ret->v;
	i = i2d_X509_NAME(name, (void *)&p);
	if (!i)
		goto err;

	return ret;

    err:
	if (buf)
		racoon_free(buf);
	if (name)
		X509_NAME_free(name);
	if (ret)
		vfree(ret);
	return NULL;
}

/*
 * convert the hex string of the subject name into DER
 */
vchar_t *
eay_hex2asn1dn(const char *hex, int len)
{
	BIGNUM *bn = BN_new();
	char *binbuf;
	size_t binlen;
	vchar_t *ret = NULL;
	
	if (len == -1)
		len = strlen(hex);

	if (BN_hex2bn(&bn, hex) != len) {
		plog(LLV_ERROR, LOCATION, NULL, 
		     "conversion of Hex-encoded ASN1 string to binary failed: %s\n",
		     eay_strerror());
		goto out;
	}
	
	binlen = BN_num_bytes(bn);
	ret = vmalloc(binlen);
	if (!ret) {
		plog(LLV_WARNING, LOCATION, NULL,"failed to allocate buffer\n");
		return NULL;
	}
	binbuf = ret->v;

	BN_bn2bin(bn, (unsigned char *) binbuf);

out:
	BN_free(bn);

	return ret;
}

/*
 * The following are derived from code in crypto/x509/x509_cmp.c
 * in OpenSSL0.9.7c:
 * X509_NAME_wildcmp() adds wildcard matching to the original
 * X509_NAME_cmp(), nocase_cmp() and nocase_spacenorm_cmp() are as is.
 */
#include <ctype.h>
/* Case insensitive string comparision */
static int nocase_cmp(const ASN1_STRING *a, const ASN1_STRING *b)
{
	int i;

	if (a->length != b->length)
		return (a->length - b->length);

	for (i=0; i<a->length; i++)
	{
		int ca, cb;

		ca = tolower(a->data[i]);
		cb = tolower(b->data[i]);

		if (ca != cb)
			return(ca-cb);
	}
	return 0;
}

/* Case insensitive string comparision with space normalization 
 * Space normalization - ignore leading, trailing spaces, 
 *       multiple spaces between characters are replaced by single space  
 */
static int nocase_spacenorm_cmp(const ASN1_STRING *a, const ASN1_STRING *b)
{
	unsigned char *pa = NULL, *pb = NULL;
	int la, lb;
	
	la = a->length;
	lb = b->length;
	pa = a->data;
	pb = b->data;

	/* skip leading spaces */
	while (la > 0 && isspace(*pa))
	{
		la--;
		pa++;
	}
	while (lb > 0 && isspace(*pb))
	{
		lb--;
		pb++;
	}

	/* skip trailing spaces */
	while (la > 0 && isspace(pa[la-1]))
		la--;
	while (lb > 0 && isspace(pb[lb-1]))
		lb--;

	/* compare strings with space normalization */
	while (la > 0 && lb > 0)
	{
		int ca, cb;

		/* compare character */
		ca = tolower(*pa);
		cb = tolower(*pb);
		if (ca != cb)
			return (ca - cb);

		pa++; pb++;
		la--; lb--;

		if (la <= 0 || lb <= 0)
			break;

		/* is white space next character ? */
		if (isspace(*pa) && isspace(*pb))
		{
			/* skip remaining white spaces */
			while (la > 0 && isspace(*pa))
			{
				la--;
				pa++;
			}
			while (lb > 0 && isspace(*pb))
			{
				lb--;
				pb++;
			}
		}
	}
	if (la > 0 || lb > 0)
		return la - lb;

	return 0;
}

static int X509_NAME_wildcmp(const X509_NAME *a, const X509_NAME *b)
{
    int i,j;
    X509_NAME_ENTRY *na,*nb;

    if (sk_X509_NAME_ENTRY_num(a->entries)
	!= sk_X509_NAME_ENTRY_num(b->entries))
	    return sk_X509_NAME_ENTRY_num(a->entries)
	      -sk_X509_NAME_ENTRY_num(b->entries);
    for (i=sk_X509_NAME_ENTRY_num(a->entries)-1; i>=0; i--)
    {
	    na=sk_X509_NAME_ENTRY_value(a->entries,i);
	    nb=sk_X509_NAME_ENTRY_value(b->entries,i);
	    j=OBJ_cmp(na->object,nb->object);
	    if (j) return(j);
	    if ((na->value->length == 1 && na->value->data[0] == '*')
	     || (nb->value->length == 1 && nb->value->data[0] == '*'))
		    continue;
	    j=na->value->type-nb->value->type;
	    if (j) return(j);
	    if (na->value->type == V_ASN1_PRINTABLESTRING)
		    j=nocase_spacenorm_cmp(na->value, nb->value);
	    else if (na->value->type == V_ASN1_IA5STRING
		    && OBJ_obj2nid(na->object) == NID_pkcs9_emailAddress)
		    j=nocase_cmp(na->value, nb->value);
	    else
		    {
		    j=na->value->length-nb->value->length;
		    if (j) return(j);
		    j=memcmp(na->value->data,nb->value->data,
			    na->value->length);
		    }
	    if (j) return(j);
	    j=na->set-nb->set;
	    if (j) return(j);
    }

    return(0);
}

/*
 * compare two subjectNames.
 * OUT:        0: equal
 *	positive:
 *	      -1: other error.
 */
int
eay_cmp_asn1dn(n1, n2)
	vchar_t *n1, *n2;
{
	X509_NAME *a = NULL, *b = NULL;
	caddr_t p;
	int i = -1;

	p = n1->v;
	if (!d2i_X509_NAME(&a, (void *)&p, n1->l))
		goto end;
	p = n2->v;
	if (!d2i_X509_NAME(&b, (void *)&p, n2->l))
		goto end;

	i = X509_NAME_wildcmp(a, b);

    end:
	if (a)
		X509_NAME_free(a);
	if (b)
		X509_NAME_free(b);
	return i;
}

/*
 * this functions is derived from apps/verify.c in OpenSSL0.9.5
 */
int
eay_check_x509cert(cert, CApath, CAfile, local)
	vchar_t *cert;
	char *CApath;
	char *CAfile;
	int local;
{
	X509_STORE *cert_ctx = NULL;
	X509_LOOKUP *lookup = NULL;
	X509 *x509 = NULL;
	X509_STORE_CTX *csc;
	int error = -1;

	cert_ctx = X509_STORE_new();
	if (cert_ctx == NULL)
		goto end;

	if (local)
		X509_STORE_set_verify_cb_func(cert_ctx, cb_check_cert_local);
	else 
		X509_STORE_set_verify_cb_func(cert_ctx, cb_check_cert_remote);

	lookup = X509_STORE_add_lookup(cert_ctx, X509_LOOKUP_file());
	if (lookup == NULL)
		goto end;

	X509_LOOKUP_load_file(lookup, CAfile, 
	    (CAfile == NULL) ? X509_FILETYPE_DEFAULT : X509_FILETYPE_PEM);

	lookup = X509_STORE_add_lookup(cert_ctx, X509_LOOKUP_hash_dir());
	if (lookup == NULL)
		goto end;
	error = X509_LOOKUP_add_dir(lookup, CApath, X509_FILETYPE_PEM);
	if(!error) {
		error = -1;
		goto end;
	}
	error = -1;	/* initialized */

	/* read the certificate to be verified */
	x509 = mem2x509(cert);
	if (x509 == NULL)
		goto end;

	csc = X509_STORE_CTX_new();
	if (csc == NULL)
		goto end;
	X509_STORE_CTX_init(csc, cert_ctx, x509, NULL);
#if OPENSSL_VERSION_NUMBER >= 0x00907000L
	X509_STORE_CTX_set_flags (csc, X509_V_FLAG_CRL_CHECK);
	X509_STORE_CTX_set_flags (csc, X509_V_FLAG_CRL_CHECK_ALL);
#endif
	error = X509_verify_cert(csc);
	X509_STORE_CTX_free(csc);

	/*
	 * if x509_verify_cert() is successful then the value of error is
	 * set non-zero.
	 */
	error = error ? 0 : -1;

end:
	if (error)
		plog(LLV_WARNING, LOCATION, NULL,"%s\n", eay_strerror());
	if (cert_ctx != NULL)
		X509_STORE_free(cert_ctx);
	if (x509 != NULL)
		X509_free(x509);

	return(error);
}

/*
 * callback function for verifing certificate.
 * this function is derived from cb() in openssl/apps/s_server.c
 */
static int
cb_check_cert_local(ok, ctx)
	int ok;
	X509_STORE_CTX *ctx;
{
	char buf[256];
	int log_tag;

	if (!ok) {
		X509_NAME_oneline(
				X509_get_subject_name(ctx->current_cert),
				buf,
				256);
		/*
		 * since we are just checking the certificates, it is
		 * ok if they are self signed. But we should still warn
		 * the user.
 		 */
		switch (ctx->error) {
		case X509_V_ERR_CERT_HAS_EXPIRED:
		case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
		case X509_V_ERR_INVALID_CA:
		case X509_V_ERR_PATH_LENGTH_EXCEEDED:
		case X509_V_ERR_INVALID_PURPOSE:
		case X509_V_ERR_UNABLE_TO_GET_CRL:
			ok = 1;
			log_tag = LLV_WARNING;
			break;
		default:
			log_tag = LLV_ERROR;
		}
		plog(log_tag, LOCATION, NULL,
			"%s(%d) at depth:%d SubjectName:%s\n",
			X509_verify_cert_error_string(ctx->error),
			ctx->error,
			ctx->error_depth,
			buf);
	}
	ERR_clear_error();

	return ok;
}

/*
 * callback function for verifing remote certificates.
 * this function is derived from cb() in openssl/apps/s_server.c
 */
static int
cb_check_cert_remote(ok, ctx)
	int ok;
	X509_STORE_CTX *ctx;
{
	char buf[256];
	int log_tag;

	if (!ok) {
		X509_NAME_oneline(
				X509_get_subject_name(ctx->current_cert),
				buf,
				256);
		switch (ctx->error) {
		case X509_V_ERR_UNABLE_TO_GET_CRL:
			ok = 1;
			log_tag = LLV_WARNING;
			break;
		default:
			log_tag = LLV_ERROR;
		}
		plog(log_tag, LOCATION, NULL,
			"%s(%d) at depth:%d SubjectName:%s\n",
			X509_verify_cert_error_string(ctx->error),
			ctx->error,
			ctx->error_depth,
			buf);
	}
	ERR_clear_error();

	return ok;
}

/*
 * get a subjectName from X509 certificate.
 */
vchar_t *
eay_get_x509asn1subjectname(cert)
	vchar_t *cert;
{
	X509 *x509 = NULL;
	u_char *bp;
	vchar_t *name = NULL;
	int len;

	x509 = mem2x509(cert);
	if (x509 == NULL)
		goto error;

	/* get the length of the name */
	len = i2d_X509_NAME(x509->cert_info->subject, NULL);
	name = vmalloc(len);
	if (!name)
		goto error;
	/* get the name */
	bp = (unsigned char *) name->v;
	len = i2d_X509_NAME(x509->cert_info->subject, &bp);

	X509_free(x509);

	return name;

error:
	plog(LLV_ERROR, LOCATION, NULL, "%s\n", eay_strerror());

	if (name != NULL) 
		vfree(name);

	if (x509 != NULL)
		X509_free(x509);

	return NULL;
}

/*
 * get the subjectAltName from X509 certificate.
 * the name must be terminated by '\0'.
 */
int
eay_get_x509subjectaltname(cert, altname, type, pos)
	vchar_t *cert;
	char **altname;
	int *type;
	int pos;
{
	X509 *x509 = NULL;
	GENERAL_NAMES *gens = NULL;
	GENERAL_NAME *gen;
	int len;
	int error = -1;

	*altname = NULL;
	*type = GENT_OTHERNAME;

	x509 = mem2x509(cert);
	if (x509 == NULL)
		goto end;

	gens = X509_get_ext_d2i(x509, NID_subject_alt_name, NULL, NULL);
	if (gens == NULL)
		goto end;

	/* there is no data at "pos" */
	if (pos > sk_GENERAL_NAME_num(gens))
		goto end;

	gen = sk_GENERAL_NAME_value(gens, pos - 1);

	/* read DNSName / Email */
	if (gen->type == GEN_DNS	||
		gen->type == GEN_EMAIL	||
		gen->type == GEN_URI )
	{
		/* make sure if the data is terminated by '\0'. */
		if (gen->d.ia5->data[gen->d.ia5->length] != '\0')
		{
			plog(LLV_ERROR, LOCATION, NULL,
				 "data is not terminated by NUL.");
			racoon_hexdump(gen->d.ia5->data, gen->d.ia5->length + 1);
			goto end;
		}
		
		len = gen->d.ia5->length + 1;
		*altname = racoon_malloc(len);
		if (!*altname)
			goto end;
		
		strlcpy(*altname, (char *) gen->d.ia5->data, len);
		*type = gen->type;
		error = 0;
	}
	/* read IP address */
	else if (gen->type == GEN_IPADD)
	{
		unsigned char p[5], *ip;
		ip = p;
		
		/* only support IPv4 */
		if (gen->d.ip->length != 4)
			goto end;
		
		/* convert Octet String to String
		 * XXX ???????
		 */
		/*i2d_ASN1_OCTET_STRING(gen->d.ip,&ip);*/
		ip = gen->d.ip->data;

		/* XXX Magic, enough for an IPv4 address
		 */
		*altname = racoon_malloc(20);
		if (!*altname)
			goto end;
		
		sprintf(*altname, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
		*type = gen->type;
		error = 0;
	}
	/* XXX other possible types ?
	 * For now, error will be -1 if unsupported type
	 */

end:
	if (error) {
		if (*altname) {
			racoon_free(*altname);
			*altname = NULL;
		}
		plog(LLV_ERROR, LOCATION, NULL, "%s\n", eay_strerror());
	}
	if (x509)
		X509_free(x509);
	if (gens)
		/* free the whole stack. */
		sk_GENERAL_NAME_pop_free(gens, GENERAL_NAME_free);

	return error;
}

/*
 * get a issuerName from X509 certificate.
 */
vchar_t *
eay_get_x509asn1issuername(cert)
	vchar_t *cert;
{
	X509 *x509 = NULL;
	u_char *bp;
	vchar_t *name = NULL;
	int len;

	x509 = mem2x509(cert);
	if (x509 == NULL)
		goto error;

	/* get the length of the name */
	len = i2d_X509_NAME(x509->cert_info->issuer, NULL);
	name = vmalloc(len);
	if (name == NULL)
		goto error;

	/* get the name */
	bp = (unsigned char *) name->v;
	len = i2d_X509_NAME(x509->cert_info->issuer, &bp);

	X509_free(x509);

	return name;

error:
	plog(LLV_ERROR, LOCATION, NULL, "%s\n", eay_strerror());

	if (name != NULL)
		vfree(name);
	if (x509 != NULL)
		X509_free(x509);

	return NULL;
}

/*
 * decode a X509 certificate and make a readable text terminated '\n'.
 * return the buffer allocated, so must free it later.
 */
char *
eay_get_x509text(cert)
	vchar_t *cert;
{
	X509 *x509 = NULL;
	BIO *bio = NULL;
	char *text = NULL;
	u_char *bp = NULL;
	int len = 0;
	int error = -1;

	x509 = mem2x509(cert);
	if (x509 == NULL)
		goto end;

	bio = BIO_new(BIO_s_mem());
	if (bio == NULL)
		goto end;

	error = X509_print(bio, x509);
	if (error != 1) {
		error = -1;
		goto end;
	}

	len = BIO_get_mem_data(bio, &bp);
	text = racoon_malloc(len + 1);
	if (text == NULL)
		goto end;
	memcpy(text, bp, len);
	text[len] = '\0';

	error = 0;

    end:
	if (error) {
		if (text) {
			racoon_free(text);
			text = NULL;
		}
		plog(LLV_ERROR, LOCATION, NULL, "%s\n", eay_strerror());
	}
	if (bio)
		BIO_free(bio);
	if (x509)
		X509_free(x509);

	return text;
}

/* get X509 structure from buffer. */
static X509 *
mem2x509(cert)
	vchar_t *cert;
{
	X509 *x509;

#ifndef EAYDEBUG
    {
	u_char *bp;

	bp = (unsigned char *) cert->v + 1;

	x509 = d2i_X509(NULL, (void *)&bp, cert->l - 1);
    }
#else
    {
	BIO *bio;
	int len;

	bio = BIO_new(BIO_s_mem());
	if (bio == NULL)
		return NULL;
	len = BIO_write(bio, cert->v + 1, cert->l - 1);
	if (len == -1)
		return NULL;
	x509 = PEM_read_bio_X509(bio, NULL, NULL, NULL);
	BIO_free(bio);
    }
#endif
	return x509;
}

/*
 * get a X509 certificate from local file.
 * a certificate must be PEM format.
 * Input:
 *	path to a certificate.
 * Output:
 *	NULL if error occured
 *	other is the cert.
 */
vchar_t *
eay_get_x509cert(path)
	char *path;
{
	FILE *fp;
	X509 *x509;
	vchar_t *cert;
	u_char *bp;
	int len;
	int error;

	/* Read private key */
	fp = fopen(path, "r");
	if (fp == NULL)
		return NULL;
	x509 = PEM_read_X509(fp, NULL, NULL, NULL);
	fclose (fp);

	if (x509 == NULL)
		return NULL;

	len = i2d_X509(x509, NULL);
	cert = vmalloc(len + 1);
	if (cert == NULL) {
		X509_free(x509);
		return NULL;
	}
	cert->v[0] = ISAKMP_CERT_X509SIGN;
	bp = (unsigned char *) &cert->v[1];
	error = i2d_X509(x509, &bp);
	X509_free(x509);

	if (error == 0) {
		vfree(cert);
		return NULL;
	}

	return cert;
}

/*
 * check a X509 signature
 *	XXX: to be get hash type from my cert ?
 *		to be handled EVP_dss().
 * OUT: return -1 when error.
 *	0
 */
int
eay_check_x509sign(source, sig, cert)
	vchar_t *source;
	vchar_t *sig;
	vchar_t *cert;
{
	X509 *x509;
	EVP_PKEY *evp;
	int res;

	x509 = mem2x509(cert);
	if (x509 == NULL)
		return -1;

	evp = X509_get_pubkey(x509);
	if (! evp) {
		plog(LLV_ERROR, LOCATION, NULL, "X509_get_pubkey(): %s\n", eay_strerror());
		X509_free(x509);
		return -1;
	}

	res = eay_rsa_verify(source, sig, evp->pkey.rsa);

	EVP_PKEY_free(evp);
	X509_free(x509);

	return res;
}

/*
 * check RSA signature
 * OUT: return -1 when error.
 *	0 on success
 */
int
eay_check_rsasign(source, sig, rsa)
	vchar_t *source;
	vchar_t *sig;
	RSA *rsa;
{
	return eay_rsa_verify(source, sig, rsa);
}

/*
 * get PKCS#1 Private Key of PEM format from local file.
 */
vchar_t *
eay_get_pkcs1privkey(path)
	char *path;
{
	FILE *fp;
	EVP_PKEY *evp = NULL;
	vchar_t *pkey = NULL;
	u_char *bp;
	int pkeylen;
	int error = -1;

	/* Read private key */
	fp = fopen(path, "r");
	if (fp == NULL)
		return NULL;

	evp = PEM_read_PrivateKey(fp, NULL, NULL, NULL);

	fclose (fp);

	if (evp == NULL)
		return NULL;

	pkeylen = i2d_PrivateKey(evp, NULL);
	if (pkeylen == 0)
		goto end;
	pkey = vmalloc(pkeylen);
	if (pkey == NULL)
		goto end;
	bp = (unsigned char *) pkey->v;
	pkeylen = i2d_PrivateKey(evp, &bp);
	if (pkeylen == 0)
		goto end;

	error = 0;

end:
	if (evp != NULL)
		EVP_PKEY_free(evp);
	if (error != 0 && pkey != NULL) {
		vfree(pkey);
		pkey = NULL;
	}

	return pkey;
}

/*
 * get PKCS#1 Public Key of PEM format from local file.
 */
vchar_t *
eay_get_pkcs1pubkey(path)
	char *path;
{
	FILE *fp;
	EVP_PKEY *evp = NULL;
	vchar_t *pkey = NULL;
	X509 *x509 = NULL;
	u_char *bp;
	int pkeylen;
	int error = -1;

	/* Read private key */
	fp = fopen(path, "r");
	if (fp == NULL)
		return NULL;

	x509 = PEM_read_X509(fp, NULL, NULL, NULL);

	fclose (fp);

	if (x509 == NULL)
		return NULL;
  
	/* Get public key - eay */
	evp = X509_get_pubkey(x509);
	if (evp == NULL)
		return NULL;

	pkeylen = i2d_PublicKey(evp, NULL);
	if (pkeylen == 0)
		goto end;
	pkey = vmalloc(pkeylen);
	if (pkey == NULL)
		goto end;
	bp = (unsigned char *) pkey->v;
	pkeylen = i2d_PublicKey(evp, &bp);
	if (pkeylen == 0)
		goto end;

	error = 0;
end:
	if (evp != NULL)
		EVP_PKEY_free(evp);
	if (error != 0 && pkey != NULL) {
		vfree(pkey);
		pkey = NULL;
	}

	return pkey;
}

vchar_t *
eay_get_x509sign(src, privkey)
	vchar_t *src, *privkey;
{
	EVP_PKEY *evp;
	u_char *bp = (unsigned char *) privkey->v;
	vchar_t *sig = NULL;
	int len;
	int pad = RSA_PKCS1_PADDING;

	/* XXX to be handled EVP_PKEY_DSA */
	evp = d2i_PrivateKey(EVP_PKEY_RSA, NULL, (void *)&bp, privkey->l);
	if (evp == NULL)
		return NULL;

	sig = eay_rsa_sign(src, evp->pkey.rsa);

	EVP_PKEY_free(evp);

	return sig;
}

vchar_t *
eay_get_rsasign(src, rsa)
	vchar_t *src;
	RSA *rsa;
{
	return eay_rsa_sign(src, rsa);
}

vchar_t *
eay_rsa_sign(vchar_t *src, RSA *rsa)
{
	int len;
	vchar_t *sig = NULL;
	int pad = RSA_PKCS1_PADDING;

	len = RSA_size(rsa);

	sig = vmalloc(len);
	if (sig == NULL)
		return NULL;

	len = RSA_private_encrypt(src->l, (unsigned char *) src->v, 
			(unsigned char *) sig->v, rsa, pad);

	if (len == 0 || len != sig->l) {
		vfree(sig);
		sig = NULL;
	}

	return sig;
}

int
eay_rsa_verify(src, sig, rsa)
	vchar_t *src, *sig;
	RSA *rsa;
{
	vchar_t *xbuf = NULL;
	int pad = RSA_PKCS1_PADDING;
	int len = 0;
	int error;

	len = RSA_size(rsa);
	xbuf = vmalloc(len);
	if (xbuf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, "%s\n", eay_strerror());
		return -1;
	}

	len = RSA_public_decrypt(sig->l, (unsigned char *) sig->v, 
			(unsigned char *) xbuf->v, rsa, pad);
	if (len == 0 || len != src->l) {
		plog(LLV_ERROR, LOCATION, NULL, "%s\n", eay_strerror());
		vfree(xbuf);
		return -1;
	}

	error = memcmp(src->v, xbuf->v, src->l);
	vfree(xbuf);
	if (error != 0)
		return -1;

	return 0;
}

/*
 * get error string
 * MUST load ERR_load_crypto_strings() first.
 */
char *
eay_strerror()
{
	static char ebuf[512];
	int len = 0, n;
	unsigned long l;
	char buf[200];
	const char *file, *data;
	int line, flags;
	unsigned long es;

	es = CRYPTO_thread_id();

	while ((l = ERR_get_error_line_data(&file, &line, &data, &flags)) != 0){
		n = snprintf(ebuf + len, sizeof(ebuf) - len,
				"%lu:%s:%s:%d:%s ",
				es, ERR_error_string(l, buf), file, line,
				(flags & ERR_TXT_STRING) ? data : "");
		if (n < 0 || n >= sizeof(ebuf) - len)
			break;
		len += n;
		if (sizeof(ebuf) < len)
			break;
	}

	return ebuf;
}

vchar_t *
evp_crypt(vchar_t *data, vchar_t *key, vchar_t *iv, const EVP_CIPHER *e, int enc)
{
	vchar_t *res;
	EVP_CIPHER_CTX ctx;

	if (!e)
		return NULL;

	if (data->l % EVP_CIPHER_block_size(e))
		return NULL;

	if ((res = vmalloc(data->l)) == NULL)
		return NULL;

	EVP_CIPHER_CTX_init(&ctx);

	switch(EVP_CIPHER_nid(e)){
	case NID_bf_cbc:
	case NID_bf_ecb:
	case NID_bf_cfb64:
	case NID_bf_ofb64:
	case NID_cast5_cbc:
	case NID_cast5_ecb:
	case NID_cast5_cfb64:
	case NID_cast5_ofb64:
		/* XXX: can we do that also for algos with a fixed key size ?
		 */
		/* init context without key/iv
         */
        if (!EVP_CipherInit(&ctx, e, NULL, NULL, enc))
        {
            OpenSSL_BUG();
            vfree(res);
            return NULL;
        }
		
        /* update key size
         */
        if (!EVP_CIPHER_CTX_set_key_length(&ctx, key->l))
        {
            OpenSSL_BUG();
            vfree(res);
            return NULL;
        }

        /* finalize context init with desired key size
         */
        if (!EVP_CipherInit(&ctx, NULL, (u_char *) key->v,
							(u_char *) iv->v, enc))
        {
            OpenSSL_BUG();
            vfree(res);
            return NULL;
		}
		break;
	default:
		if (!EVP_CipherInit(&ctx, e, (u_char *) key->v, 
							(u_char *) iv->v, enc)) {
			OpenSSL_BUG();
			vfree(res);
			return NULL;
		}
	}

	/* disable openssl padding */
	EVP_CIPHER_CTX_set_padding(&ctx, 0); 
	
	if (!EVP_Cipher(&ctx, (u_char *) res->v, (u_char *) data->v, data->l)) {
		OpenSSL_BUG();
		vfree(res);
		return NULL;
	}

	EVP_CIPHER_CTX_cleanup(&ctx);

	return res;
}

int
evp_weakkey(vchar_t *key, const EVP_CIPHER *e)
{
	return 0;
}

int
evp_keylen(int len, const EVP_CIPHER *e)
{
	if (!e)
		return -1;
	/* EVP functions return lengths in bytes, ipsec-tools
	 * uses lengths in bits, therefore conversion is required. --AK
	 */
	if (len != 0 && len != (EVP_CIPHER_key_length(e) << 3))
		return -1;
	
	return EVP_CIPHER_key_length(e) << 3;
}

/*
 * DES-CBC
 */
vchar_t *
eay_des_encrypt(data, key, iv)
	vchar_t *data, *key, *iv;
{
	return evp_crypt(data, key, iv, EVP_des_cbc(), 1);
}

vchar_t *
eay_des_decrypt(data, key, iv)
	vchar_t *data, *key, *iv;
{
	return evp_crypt(data, key, iv, EVP_des_cbc(), 0);
}

int
eay_des_weakkey(key)
	vchar_t *key;
{
#ifdef USE_NEW_DES_API
	return DES_is_weak_key((void *)key->v);
#else
	return des_is_weak_key((void *)key->v);
#endif
}

int
eay_des_keylen(len)
	int len;
{
	return evp_keylen(len, EVP_des_cbc());
}

#ifdef HAVE_OPENSSL_IDEA_H
/*
 * IDEA-CBC
 */
vchar_t *
eay_idea_encrypt(data, key, iv)
	vchar_t *data, *key, *iv;
{
	vchar_t *res;
	IDEA_KEY_SCHEDULE ks;

	idea_set_encrypt_key((unsigned char *)key->v, &ks);

	/* allocate buffer for result */
	if ((res = vmalloc(data->l)) == NULL)
		return NULL;

	/* decryption data */
	idea_cbc_encrypt((unsigned char *)data->v, (unsigned char *)res->v, data->l,
			&ks, (unsigned char *)iv->v, IDEA_ENCRYPT);

	return res;
}

vchar_t *
eay_idea_decrypt(data, key, iv)
	vchar_t *data, *key, *iv;
{
	vchar_t *res;
	IDEA_KEY_SCHEDULE ks, dks;

	idea_set_encrypt_key((unsigned char *)key->v, &ks);
	idea_set_decrypt_key(&ks, &dks);

	/* allocate buffer for result */
	if ((res = vmalloc(data->l)) == NULL)
		return NULL;

	/* decryption data */
	idea_cbc_encrypt((unsigned char *)data->v, (unsigned char *)res->v, data->l,
			&dks, (unsigned char *)iv->v, IDEA_DECRYPT);

	return res;
}

int
eay_idea_weakkey(key)
	vchar_t *key;
{
	return 0;       /* XXX */
}

int
eay_idea_keylen(len)
	int len;
{
	if (len != 0 && len != 128)
		return -1;
	return 128;
}
#endif

/*
 * BLOWFISH-CBC
 */
vchar_t *
eay_bf_encrypt(data, key, iv)
	vchar_t *data, *key, *iv;
{
	return evp_crypt(data, key, iv, EVP_bf_cbc(), 1);
}

vchar_t *
eay_bf_decrypt(data, key, iv)
	vchar_t *data, *key, *iv;
{
	return evp_crypt(data, key, iv, EVP_bf_cbc(), 0);
}

int
eay_bf_weakkey(key)
	vchar_t *key;
{
	return 0;	/* XXX to be done. refer to RFC 2451 */
}

int
eay_bf_keylen(len)
	int len;
{
	if (len == 0)
		return 448;
	if (len < 40 || len > 448)
		return -1;
	return len;
}

#ifdef HAVE_OPENSSL_RC5_H
/*
 * RC5-CBC
 */
vchar_t *
eay_rc5_encrypt(data, key, iv)
	vchar_t *data, *key, *iv;
{
	vchar_t *res;
	RC5_32_KEY ks;

	/* in RFC 2451, there is information about the number of round. */
	RC5_32_set_key(&ks, key->l, (unsigned char *)key->v, 16);

	/* allocate buffer for result */
	if ((res = vmalloc(data->l)) == NULL)
		return NULL;

	/* decryption data */
	RC5_32_cbc_encrypt((unsigned char *)data->v, (unsigned char *)res->v, data->l,
		&ks, (unsigned char *)iv->v, RC5_ENCRYPT);

	return res;
}

vchar_t *
eay_rc5_decrypt(data, key, iv)
	vchar_t *data, *key, *iv;
{
	vchar_t *res;
	RC5_32_KEY ks;

	/* in RFC 2451, there is information about the number of round. */
	RC5_32_set_key(&ks, key->l, (unsigned char *)key->v, 16);

	/* allocate buffer for result */
	if ((res = vmalloc(data->l)) == NULL)
		return NULL;

	/* decryption data */
	RC5_32_cbc_encrypt((unsigned char *)data->v, (unsigned char *)res->v, data->l,
		&ks, (unsigned char *)iv->v, RC5_DECRYPT);

	return res;
}

int
eay_rc5_weakkey(key)
	vchar_t *key;
{
	return 0;       /* No known weak keys when used with 16 rounds. */

}

int
eay_rc5_keylen(len)
	int len;
{
	if (len == 0)
		return 128;
	if (len < 40 || len > 2040)
		return -1;
	return len;
}
#endif

/*
 * 3DES-CBC
 */
vchar_t *
eay_3des_encrypt(data, key, iv)
	vchar_t *data, *key, *iv;
{
	return evp_crypt(data, key, iv, EVP_des_ede3_cbc(), 1);
}

vchar_t *
eay_3des_decrypt(data, key, iv)
	vchar_t *data, *key, *iv;
{
	return evp_crypt(data, key, iv, EVP_des_ede3_cbc(), 0);
}

int
eay_3des_weakkey(key)
	vchar_t *key;
{
#ifdef USE_NEW_DES_API
	return (DES_is_weak_key((void *)key->v) ||
	    DES_is_weak_key((void *)(key->v + 8)) ||
	    DES_is_weak_key((void *)(key->v + 16)));
#else
	if (key->l < 24)
		return 0;

	return (des_is_weak_key((void *)key->v) ||
	    des_is_weak_key((void *)(key->v + 8)) ||
	    des_is_weak_key((void *)(key->v + 16)));
#endif
}

int
eay_3des_keylen(len)
	int len;
{
	if (len != 0 && len != 192)
		return -1;
	return 192;
}

/*
 * CAST-CBC
 */
vchar_t *
eay_cast_encrypt(data, key, iv)
	vchar_t *data, *key, *iv;
{
	return evp_crypt(data, key, iv, EVP_cast5_cbc(), 1);
}

vchar_t *
eay_cast_decrypt(data, key, iv)
	vchar_t *data, *key, *iv;
{
	return evp_crypt(data, key, iv, EVP_cast5_cbc(), 0);
}

int
eay_cast_weakkey(key)
	vchar_t *key;
{
	return 0;	/* No known weak keys. */
}

int
eay_cast_keylen(len)
	int len;
{
	if (len == 0)
		return 128;
	if (len < 40 || len > 128)
		return -1;
	return len;
}

/*
 * AES(RIJNDAEL)-CBC
 */
#ifndef HAVE_OPENSSL_AES_H
vchar_t *
eay_aes_encrypt(data, key, iv)
	vchar_t *data, *key, *iv;
{
	vchar_t *res;
	keyInstance k;
	cipherInstance c;

	memset(&k, 0, sizeof(k));
	if (rijndael_makeKey(&k, DIR_ENCRYPT, key->l << 3, key->v) < 0)
		return NULL;

	/* allocate buffer for result */
	if ((res = vmalloc(data->l)) == NULL)
		return NULL;

	/* encryption data */
	memset(&c, 0, sizeof(c));
	if (rijndael_cipherInit(&c, MODE_CBC, iv->v) < 0){
		vfree(res);
		return NULL;
	}
	if (rijndael_blockEncrypt(&c, &k, data->v, data->l << 3, res->v) < 0){
		vfree(res);
		return NULL;
	}

	return res;
}

vchar_t *
eay_aes_decrypt(data, key, iv)
	vchar_t *data, *key, *iv;
{
	vchar_t *res;
	keyInstance k;
	cipherInstance c;

	memset(&k, 0, sizeof(k));
	if (rijndael_makeKey(&k, DIR_DECRYPT, key->l << 3, key->v) < 0)
		return NULL;

	/* allocate buffer for result */
	if ((res = vmalloc(data->l)) == NULL)
		return NULL;

	/* decryption data */
	memset(&c, 0, sizeof(c));
	if (rijndael_cipherInit(&c, MODE_CBC, iv->v) < 0){
		vfree(res);
		return NULL;
	}
	if (rijndael_blockDecrypt(&c, &k, data->v, data->l << 3, res->v) < 0){
		vfree(res);
		return NULL;
	}

	return res;
}
#else
static inline const EVP_CIPHER *
aes_evp_by_keylen(int keylen)
{
	switch(keylen) {
		case 16:
		case 128:
			return EVP_aes_128_cbc();
		case 24:
		case 192:
			return EVP_aes_192_cbc();
		case 32:
		case 256:
			return EVP_aes_256_cbc();
		default:
			return NULL;
	}
}

vchar_t *
eay_aes_encrypt(data, key, iv)
       vchar_t *data, *key, *iv;
{
	return evp_crypt(data, key, iv, aes_evp_by_keylen(key->l), 1);
}

vchar_t *
eay_aes_decrypt(data, key, iv)
       vchar_t *data, *key, *iv;
{
	return evp_crypt(data, key, iv, aes_evp_by_keylen(key->l), 0);
}
#endif

int
eay_aes_weakkey(key)
	vchar_t *key;
{
	return 0;
}

int
eay_aes_keylen(len)
	int len;
{
	if (len == 0)
		return 128;
	if (len != 128 && len != 192 && len != 256)
		return -1;
	return len;
}

int
eay_aesgcm_keylen(len)
	int len;
{
	/* RFC 4106:
	 * The size of the KEYMAT for the AES-GCM-ESP MUST be four octets longer
	 * than is needed for the associated AES key.  The keying material is
	 * used as follows:
	 *
	 * AES-GCM-ESP with a 128 bit key
	 * The KEYMAT requested for each AES-GCM key is 20 octets.  The first
	 * 16 octets are the 128-bit AES key, and the remaining four octets
	 * are used as the salt value in the nonce.
	 *
	 * AES-GCM-ESP with a 192 bit key
	 * The KEYMAT requested for each AES-GCM key is 28 octets.  The first
	 * 24 octets are the 192-bit AES key, and the remaining four octets
	 * are used as the salt value in the nonce.
	 *
	 * AES-GCM-ESP with a 256 bit key
	 * The KEYMAT requested for each AES GCM key is 36 octets.  The first
	 * 32 octets are the 256-bit AES key, and the remaining four octets
	 * are used as the salt value in the nonce.
	 */
	if (len == 0)
		len = 128;

	if (len != 128 && len != 192 && len != 256)
		return -1;

	return len + 32;
}

#if defined(HAVE_OPENSSL_CAMELLIA_H)
/*
 * CAMELLIA-CBC
 */
static inline const EVP_CIPHER *
camellia_evp_by_keylen(int keylen)
{
	switch(keylen) {
		case 16:
		case 128:
			return EVP_camellia_128_cbc();
		case 24:
		case 192:
			return EVP_camellia_192_cbc();
		case 32:
		case 256:
			return EVP_camellia_256_cbc();
		default:
			return NULL;
	}
}

vchar_t *
eay_camellia_encrypt(data, key, iv)
       vchar_t *data, *key, *iv;
{
	return evp_crypt(data, key, iv, camellia_evp_by_keylen(key->l), 1);
}

vchar_t *
eay_camellia_decrypt(data, key, iv)
       vchar_t *data, *key, *iv;
{
	return evp_crypt(data, key, iv, camellia_evp_by_keylen(key->l), 0);
}

int
eay_camellia_weakkey(key)
	vchar_t *key;
{
	return 0;
}

int
eay_camellia_keylen(len)
	int len;
{
	if (len == 0)
		return 128;
	if (len != 128 && len != 192 && len != 256)
		return -1;
	return len;
}

#endif

/* for ipsec part */
int
eay_null_hashlen()
{
	return 0;
}

int
eay_kpdk_hashlen()
{
	return 0;
}

int
eay_twofish_keylen(len)
	int len;
{
	if (len < 0 || len > 256)
		return -1;
	return len;
}

int
eay_null_keylen(len)
	int len;
{
	return 0;
}

/*
 * HMAC functions
 */
static caddr_t
eay_hmac_init(key, md)
	vchar_t *key;
	const EVP_MD *md;
{
	HMAC_CTX *c = racoon_malloc(sizeof(*c));

	HMAC_Init(c, key->v, key->l, md);

	return (caddr_t)c;
}

static vchar_t *eay_hmac_one(key, data, type)
	vchar_t *key, *data;
	const EVP_MD *type;
{
	vchar_t *res;

	if ((res = vmalloc(EVP_MD_size(type))) == 0)
		return NULL;

	if (!HMAC(type, (void *) key->v, key->l,
		  (void *) data->v, data->l, (void *) res->v, NULL)) {
		vfree(res);
		return NULL;
	}

	return res;
}

static vchar_t *eay_digest_one(data, type)
	vchar_t *data;
	const EVP_MD *type;
{
	vchar_t *res;

	if ((res = vmalloc(EVP_MD_size(type))) == 0)
		return NULL;

	if (!EVP_Digest((void *) data->v, data->l,
			(void *) res->v, NULL, type, NULL)) {
		vfree(res);
		return NULL;
	}

	return res;
}

#ifdef WITH_SHA2
/*
 * HMAC SHA2-512
 */
vchar_t *
eay_hmacsha2_512_one(key, data)
	vchar_t *key, *data;
{
	return eay_hmac_one(key, data, EVP_sha2_512());
}

caddr_t
eay_hmacsha2_512_init(key)
	vchar_t *key;
{
	return eay_hmac_init(key, EVP_sha2_512());
}

void
eay_hmacsha2_512_update(c, data)
	caddr_t c;
	vchar_t *data;
{
	HMAC_Update((HMAC_CTX *)c, (unsigned char *) data->v, data->l);
}

vchar_t *
eay_hmacsha2_512_final(c)
	caddr_t c;
{
	vchar_t *res;
	unsigned int l;

	if ((res = vmalloc(SHA512_DIGEST_LENGTH)) == 0)
		return NULL;

	HMAC_Final((HMAC_CTX *)c, (unsigned char *) res->v, &l);
	res->l = l;
	HMAC_cleanup((HMAC_CTX *)c);
	(void)racoon_free(c);

	if (SHA512_DIGEST_LENGTH != res->l) {
		plog(LLV_ERROR, LOCATION, NULL,
			"hmac sha2_512 length mismatch %zd.\n", res->l);
		vfree(res);
		return NULL;
	}

	return(res);
}

/*
 * HMAC SHA2-384
 */
vchar_t *
eay_hmacsha2_384_one(key, data)
	vchar_t *key, *data;
{
	return eay_hmac_one(key, data, EVP_sha2_384());
}

caddr_t
eay_hmacsha2_384_init(key)
	vchar_t *key;
{
	return eay_hmac_init(key, EVP_sha2_384());
}

void
eay_hmacsha2_384_update(c, data)
	caddr_t c;
	vchar_t *data;
{
	HMAC_Update((HMAC_CTX *)c, (unsigned char *) data->v, data->l);
}

vchar_t *
eay_hmacsha2_384_final(c)
	caddr_t c;
{
	vchar_t *res;
	unsigned int l;

	if ((res = vmalloc(SHA384_DIGEST_LENGTH)) == 0)
		return NULL;

	HMAC_Final((HMAC_CTX *)c, (unsigned char *) res->v, &l);
	res->l = l;
	HMAC_cleanup((HMAC_CTX *)c);
	(void)racoon_free(c);

	if (SHA384_DIGEST_LENGTH != res->l) {
		plog(LLV_ERROR, LOCATION, NULL,
			"hmac sha2_384 length mismatch %zd.\n", res->l);
		vfree(res);
		return NULL;
	}

	return(res);
}

/*
 * HMAC SHA2-256
 */
vchar_t *
eay_hmacsha2_256_one(key, data)
	vchar_t *key, *data;
{
	return eay_hmac_one(key, data, EVP_sha2_256());
}

caddr_t
eay_hmacsha2_256_init(key)
	vchar_t *key;
{
	return eay_hmac_init(key, EVP_sha2_256());
}

void
eay_hmacsha2_256_update(c, data)
	caddr_t c;
	vchar_t *data;
{
	HMAC_Update((HMAC_CTX *)c, (unsigned char *) data->v, data->l);
}

vchar_t *
eay_hmacsha2_256_final(c)
	caddr_t c;
{
	vchar_t *res;
	unsigned int l;

	if ((res = vmalloc(SHA256_DIGEST_LENGTH)) == 0)
		return NULL;

	HMAC_Final((HMAC_CTX *)c, (unsigned char *) res->v, &l);
	res->l = l;
	HMAC_cleanup((HMAC_CTX *)c);
	(void)racoon_free(c);

	if (SHA256_DIGEST_LENGTH != res->l) {
		plog(LLV_ERROR, LOCATION, NULL,
			"hmac sha2_256 length mismatch %zd.\n", res->l);
		vfree(res);
		return NULL;
	}

	return(res);
}
#endif	/* WITH_SHA2 */

/*
 * HMAC SHA1
 */
vchar_t *
eay_hmacsha1_one(key, data)
	vchar_t *key, *data;
{
	return eay_hmac_one(key, data, EVP_sha1());
}

caddr_t
eay_hmacsha1_init(key)
	vchar_t *key;
{
	return eay_hmac_init(key, EVP_sha1());
}

void
eay_hmacsha1_update(c, data)
	caddr_t c;
	vchar_t *data;
{
	HMAC_Update((HMAC_CTX *)c, (unsigned char *) data->v, data->l);
}

vchar_t *
eay_hmacsha1_final(c)
	caddr_t c;
{
	vchar_t *res;
	unsigned int l;

	if ((res = vmalloc(SHA_DIGEST_LENGTH)) == 0)
		return NULL;

	HMAC_Final((HMAC_CTX *)c, (unsigned char *) res->v, &l);
	res->l = l;
	HMAC_cleanup((HMAC_CTX *)c);
	(void)racoon_free(c);

	if (SHA_DIGEST_LENGTH != res->l) {
		plog(LLV_ERROR, LOCATION, NULL,
			"hmac sha1 length mismatch %zd.\n", res->l);
		vfree(res);
		return NULL;
	}

	return(res);
}

/*
 * HMAC MD5
 */
vchar_t *
eay_hmacmd5_one(key, data)
	vchar_t *key, *data;
{
	return eay_hmac_one(key, data, EVP_md5());
}

caddr_t
eay_hmacmd5_init(key)
	vchar_t *key;
{
	return eay_hmac_init(key, EVP_md5());
}

void
eay_hmacmd5_update(c, data)
	caddr_t c;
	vchar_t *data;
{
	HMAC_Update((HMAC_CTX *)c, (unsigned char *) data->v, data->l);
}

vchar_t *
eay_hmacmd5_final(c)
	caddr_t c;
{
	vchar_t *res;
	unsigned int l;

	if ((res = vmalloc(MD5_DIGEST_LENGTH)) == 0)
		return NULL;

	HMAC_Final((HMAC_CTX *)c, (unsigned char *) res->v, &l);
	res->l = l;
	HMAC_cleanup((HMAC_CTX *)c);
	(void)racoon_free(c);

	if (MD5_DIGEST_LENGTH != res->l) {
		plog(LLV_ERROR, LOCATION, NULL,
			"hmac md5 length mismatch %zd.\n", res->l);
		vfree(res);
		return NULL;
	}

	return(res);
}

#ifdef WITH_SHA2
/*
 * SHA2-512 functions
 */
caddr_t
eay_sha2_512_init()
{
	SHA512_CTX *c = racoon_malloc(sizeof(*c));

	SHA512_Init(c);

	return((caddr_t)c);
}

void
eay_sha2_512_update(c, data)
	caddr_t c;
	vchar_t *data;
{
	SHA512_Update((SHA512_CTX *)c, (unsigned char *) data->v, data->l);

	return;
}

vchar_t *
eay_sha2_512_final(c)
	caddr_t c;
{
	vchar_t *res;

	if ((res = vmalloc(SHA512_DIGEST_LENGTH)) == 0)
		return(0);

	SHA512_Final((unsigned char *) res->v, (SHA512_CTX *)c);
	(void)racoon_free(c);

	return(res);
}

vchar_t *
eay_sha2_512_one(data)
	vchar_t *data;
{
	return eay_digest_one(data, EVP_sha512());
}

int
eay_sha2_512_hashlen()
{
	return SHA512_DIGEST_LENGTH << 3;
}
#endif

#ifdef WITH_SHA2
/*
 * SHA2-384 functions
 */
caddr_t
eay_sha2_384_init()
{
	SHA384_CTX *c = racoon_malloc(sizeof(*c));

	SHA384_Init(c);

	return((caddr_t)c);
}

void
eay_sha2_384_update(c, data)
	caddr_t c;
	vchar_t *data;
{
	SHA384_Update((SHA384_CTX *)c, (unsigned char *) data->v, data->l);

	return;
}

vchar_t *
eay_sha2_384_final(c)
	caddr_t c;
{
	vchar_t *res;

	if ((res = vmalloc(SHA384_DIGEST_LENGTH)) == 0)
		return(0);

	SHA384_Final((unsigned char *) res->v, (SHA384_CTX *)c);
	(void)racoon_free(c);

	return(res);
}

vchar_t *
eay_sha2_384_one(data)
	vchar_t *data;
{
	return eay_digest_one(data, EVP_sha2_384());
}

int
eay_sha2_384_hashlen()
{
	return SHA384_DIGEST_LENGTH << 3;
}
#endif

#ifdef WITH_SHA2
/*
 * SHA2-256 functions
 */
caddr_t
eay_sha2_256_init()
{
	SHA256_CTX *c = racoon_malloc(sizeof(*c));

	SHA256_Init(c);

	return((caddr_t)c);
}

void
eay_sha2_256_update(c, data)
	caddr_t c;
	vchar_t *data;
{
	SHA256_Update((SHA256_CTX *)c, (unsigned char *) data->v, data->l);

	return;
}

vchar_t *
eay_sha2_256_final(c)
	caddr_t c;
{
	vchar_t *res;

	if ((res = vmalloc(SHA256_DIGEST_LENGTH)) == 0)
		return(0);

	SHA256_Final((unsigned char *) res->v, (SHA256_CTX *)c);
	(void)racoon_free(c);

	return(res);
}

vchar_t *
eay_sha2_256_one(data)
	vchar_t *data;
{
	return eay_digest_one(data, EVP_sha2_256());
}

int
eay_sha2_256_hashlen()
{
	return SHA256_DIGEST_LENGTH << 3;
}
#endif

/*
 * SHA functions
 */
caddr_t
eay_sha1_init()
{
	SHA_CTX *c = racoon_malloc(sizeof(*c));

	SHA1_Init(c);

	return((caddr_t)c);
}

void
eay_sha1_update(c, data)
	caddr_t c;
	vchar_t *data;
{
	SHA1_Update((SHA_CTX *)c, data->v, data->l);

	return;
}

vchar_t *
eay_sha1_final(c)
	caddr_t c;
{
	vchar_t *res;

	if ((res = vmalloc(SHA_DIGEST_LENGTH)) == 0)
		return(0);

	SHA1_Final((unsigned char *) res->v, (SHA_CTX *)c);
	(void)racoon_free(c);

	return(res);
}

vchar_t *
eay_sha1_one(data)
	vchar_t *data;
{
	return eay_digest_one(data, EVP_sha1());
}

int
eay_sha1_hashlen()
{
	return SHA_DIGEST_LENGTH << 3;
}

/*
 * MD5 functions
 */
caddr_t
eay_md5_init()
{
	MD5_CTX *c = racoon_malloc(sizeof(*c));

	MD5_Init(c);

	return((caddr_t)c);
}

void
eay_md5_update(c, data)
	caddr_t c;
	vchar_t *data;
{
	MD5_Update((MD5_CTX *)c, data->v, data->l);

	return;
}

vchar_t *
eay_md5_final(c)
	caddr_t c;
{
	vchar_t *res;

	if ((res = vmalloc(MD5_DIGEST_LENGTH)) == 0)
		return(0);

	MD5_Final((unsigned char *) res->v, (MD5_CTX *)c);
	(void)racoon_free(c);

	return(res);
}

vchar_t *
eay_md5_one(data)
	vchar_t *data;
{
	return eay_digest_one(data, EVP_md5());
}

int
eay_md5_hashlen()
{
	return MD5_DIGEST_LENGTH << 3;
}

/*
 * eay_set_random
 *   size: number of bytes.
 */
vchar_t *
eay_set_random(size)
	u_int32_t size;
{
	BIGNUM *r = NULL;
	vchar_t *res = 0;

	if ((r = BN_new()) == NULL)
		goto end;
	BN_rand(r, size * 8, 0, 0);
	eay_bn2v(&res, r);

end:
	if (r)
		BN_free(r);
	return(res);
}

/* DH */
int
eay_dh_generate(prime, g, publen, pub, priv)
	vchar_t *prime, **pub, **priv;
	u_int publen;
	u_int32_t g;
{
	BIGNUM *p = NULL;
	DH *dh = NULL;
	int error = -1;

	/* initialize */
	/* pre-process to generate number */
	if (eay_v2bn(&p, prime) < 0)
		goto end;

	if ((dh = DH_new()) == NULL)
		goto end;
	dh->p = p;
	p = NULL;	/* p is now part of dh structure */
	dh->g = NULL;
	if ((dh->g = BN_new()) == NULL)
		goto end;
	if (!BN_set_word(dh->g, g))
		goto end;

	if (publen != 0)
		dh->length = publen;

	/* generate public and private number */
	if (!DH_generate_key(dh))
		goto end;

	/* copy results to buffers */
	if (eay_bn2v(pub, dh->pub_key) < 0)
		goto end;
	if (eay_bn2v(priv, dh->priv_key) < 0) {
		vfree(*pub);
		goto end;
	}

	error = 0;

end:
	if (dh != NULL)
		DH_free(dh);
	if (p != 0)
		BN_free(p);
	return(error);
}

int
eay_dh_compute(prime, g, pub, priv, pub2, key)
	vchar_t *prime, *pub, *priv, *pub2, **key;
	u_int32_t g;
{
	BIGNUM *dh_pub = NULL;
	DH *dh = NULL;
	int l;
	unsigned char *v = NULL;
	int error = -1;

	/* make public number to compute */
	if (eay_v2bn(&dh_pub, pub2) < 0)
		goto end;

	/* make DH structure */
	if ((dh = DH_new()) == NULL)
		goto end;
	if (eay_v2bn(&dh->p, prime) < 0)
		goto end;
	if (eay_v2bn(&dh->pub_key, pub) < 0)
		goto end;
	if (eay_v2bn(&dh->priv_key, priv) < 0)
		goto end;
	dh->length = pub2->l * 8;

	dh->g = NULL;
	if ((dh->g = BN_new()) == NULL)
		goto end;
	if (!BN_set_word(dh->g, g))
		goto end;

	if ((v = racoon_calloc(prime->l, sizeof(u_char))) == NULL)
		goto end;
	if ((l = DH_compute_key(v, dh_pub, dh)) == -1)
		goto end;
	memcpy((*key)->v + (prime->l - l), v, l);

	error = 0;

end:
	if (dh_pub != NULL)
		BN_free(dh_pub);
	if (dh != NULL)
		DH_free(dh);
	if (v != NULL)
		racoon_free(v);
	return(error);
}

/*
 * convert vchar_t <-> BIGNUM.
 *
 * vchar_t: unit is u_char, network endian, most significant byte first.
 * BIGNUM: unit is BN_ULONG, each of BN_ULONG is in host endian,
 *	least significant BN_ULONG must come first.
 *
 * hex value of "0x3ffe050104" is represented as follows:
 *	vchar_t: 3f fe 05 01 04
 *	BIGNUM (BN_ULONG = u_int8_t): 04 01 05 fe 3f
 *	BIGNUM (BN_ULONG = u_int16_t): 0x0104 0xfe05 0x003f
 *	BIGNUM (BN_ULONG = u_int32_t_t): 0xfe050104 0x0000003f
 */
int
eay_v2bn(bn, var)
	BIGNUM **bn;
	vchar_t *var;
{
	if ((*bn = BN_bin2bn((unsigned char *) var->v, var->l, NULL)) == NULL)
		return -1;

	return 0;
}

int
eay_bn2v(var, bn)
	vchar_t **var;
	BIGNUM *bn;
{
	*var = vmalloc(BN_num_bytes(bn));
	if (*var == NULL)
		return(-1);

	(*var)->l = BN_bn2bin(bn, (unsigned char *) (*var)->v);

	return 0;
}

void
eay_init()
{
	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();
#ifdef HAVE_OPENSSL_ENGINE_H
	ENGINE_load_builtin_engines();
	ENGINE_register_all_complete();
#endif
}

vchar_t *
base64_decode(char *in, long inlen)
{
	BIO *bio=NULL, *b64=NULL;
	vchar_t *res = NULL;
	char *outb;
	long outlen;

	outb = malloc(inlen * 2);
	if (outb == NULL)
		goto out;
	bio = BIO_new_mem_buf(in, inlen);
	b64 = BIO_new(BIO_f_base64());
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	bio = BIO_push(b64, bio);

	outlen = BIO_read(bio, outb, inlen * 2);
	if (outlen <= 0) {
		plog(LLV_ERROR, LOCATION, NULL, "%s\n", eay_strerror());
		goto out;
	}

	res = vmalloc(outlen);
	if (!res)
		goto out;

	memcpy(res->v, outb, outlen);

out:
	if (outb)
		free(outb);
	if (bio)
		BIO_free_all(bio);

	return res;
}

vchar_t *
base64_encode(char *in, long inlen)
{
	BIO *bio=NULL, *b64=NULL;
	char *ptr;
	long plen = -1;
	vchar_t *res = NULL;

	bio = BIO_new(BIO_s_mem());
	b64 = BIO_new(BIO_f_base64());
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	bio = BIO_push(b64, bio);

	BIO_write(bio, in, inlen);
	BIO_flush(bio);

	plen = BIO_get_mem_data(bio, &ptr);
	res = vmalloc(plen+1);
	if (!res)
		goto out;
	
	memcpy (res->v, ptr, plen);
	res->v[plen] = '\0';

out:	
	if (bio)
		BIO_free_all(bio);

	return res;
}

static RSA *
binbuf_pubkey2rsa(vchar_t *binbuf)
{
	BIGNUM *exp, *mod;
	RSA *rsa_pub = NULL;

	if (binbuf->v[0] > binbuf->l - 1) {
		plog(LLV_ERROR, LOCATION, NULL, "Plain RSA pubkey format error: decoded string doesn't make sense.\n");
		goto out;
	}

	exp = BN_bin2bn((unsigned char *) (binbuf->v + 1), binbuf->v[0], NULL);
	mod = BN_bin2bn((unsigned char *) (binbuf->v + binbuf->v[0] + 1), 
			binbuf->l - binbuf->v[0] - 1, NULL);
	rsa_pub = RSA_new();

	if (!exp || !mod || !rsa_pub) {
		plog(LLV_ERROR, LOCATION, NULL, "Plain RSA pubkey parsing error: %s\n", eay_strerror());
		if (exp)
			BN_free(exp);
		if (mod)
			BN_free(exp);
		if (rsa_pub)
			RSA_free(rsa_pub);
		rsa_pub = NULL;
		goto out;
	}
	
	rsa_pub->n = mod;
	rsa_pub->e = exp;

out:
	return rsa_pub;
}

RSA *
base64_pubkey2rsa(char *in)
{
	BIGNUM *exp, *mod;
	RSA *rsa_pub = NULL;
	vchar_t *binbuf;

	if (strncmp(in, "0s", 2) != 0) {
		plog(LLV_ERROR, LOCATION, NULL, "Plain RSA pubkey format error: doesn't start with '0s'\n");
		return NULL;
	}

	binbuf = base64_decode(in + 2, strlen(in + 2));
	if (!binbuf) {
		plog(LLV_ERROR, LOCATION, NULL, "Plain RSA pubkey format error: Base64 decoding failed.\n");
		return NULL;
	}
	
	if (binbuf->v[0] > binbuf->l - 1) {
		plog(LLV_ERROR, LOCATION, NULL, "Plain RSA pubkey format error: decoded string doesn't make sense.\n");
		goto out;
	}

	rsa_pub = binbuf_pubkey2rsa(binbuf);

out:
	if (binbuf)
		vfree(binbuf);

	return rsa_pub;
}

RSA *
bignum_pubkey2rsa(BIGNUM *in)
{
	RSA *rsa_pub = NULL;
	vchar_t *binbuf;

	binbuf = vmalloc(BN_num_bytes(in));
	if (!binbuf) {
		plog(LLV_ERROR, LOCATION, NULL, "Plain RSA pubkey conversion: memory allocation failed..\n");
		return NULL;
	}
	
	BN_bn2bin(in, (unsigned char *) binbuf->v);

	rsa_pub = binbuf_pubkey2rsa(binbuf);

out:
	if (binbuf)
		vfree(binbuf);

	return rsa_pub;
}

u_int32_t
eay_random()
{
	u_int32_t result;
	vchar_t *vrand;

	vrand = eay_set_random(sizeof(result));
	memcpy(&result, vrand->v, sizeof(result));
	vfree(vrand);

	return result;
}

const char *
eay_version()
{
	return SSLeay_version(SSLEAY_VERSION);
}
