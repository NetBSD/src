/*	$KAME: eaytest.c,v 1.43 2004/04/08 09:15:10 sakane Exp $	*/

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: eaytest.c,v 1.5 2004/04/12 03:34:06 itojun Exp $");

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>

#include <openssl/bio.h>
#include <openssl/pem.h>

#include "var.h"
#include "vmbuf.h"
#include "misc.h"
#include "debug.h"
#include "str2val.h"

#include "oakley.h"
#include "dhgroup.h"
#include "crypto_openssl.h"

#define PVDUMP(var) hexdump((var)->v, (var)->l)

u_int32_t loglevel = 4;

/* prototype */

void rsatest __P((int, char **));
#if 0
static vchar_t *pem_read_buf __P((char *));
#endif
void certtest __P((int, char **));
static char **getcerts __P((char *));
void ciphertest __P((int, char **));
void hmactest __P((int, char **));
#ifdef WITH_SHA2
void sha2test __P((int, char **));
#endif
void sha1test __P((int, char **));
void md5test __P((int, char **));
void dhtest __P((int, char **));
void bntest __P((int, char **));
void Usage __P((void));

/* test */

void
rsatest(ac, av)
	int ac;
	char **av;
{
#if 0
	char *text = "this is test.";
	vchar_t src;
	vchar_t *priv, *pub, *sig;
	int error;

	char *pkcs1 =
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIICXQIBAAKBgQChe5/Fzk9SA0vCKBOcu9jBcLb9oLv50PeuEfQojhakY+OH8A3Q\n"
"M8A0qIDG6uhTNGPvzCWb/+mKeOB48n5HJpLxlDFyP3kyd2yXHIZ/MN8g1nh4FsB0\n"
"iTkk8QUCJkkan6FCOBrIeLEsGA5AdodzuR+khnCMt8vO+NFHZYKAQeynyQIDAQAB\n"
"AoGAOfDcnCHxjhDGrwyoNNWl6Yqi7hAtQm67YAbrH14UO7nnmxAENM9MyNgpFLaW\n"
"07v5m8IZQIcradcDXAJOUwNBN8E06UflwEYCaScIwndvr5UpVlN3e2NC6Wyg2yC7\n"
"GarxQput3zj35XNR5bK42UneU0H6zDxpHWqI1SwE+ToAHu0CQQDNl9gUJTpg0L09\n"
"HkbE5jeb8bA5I20nKqBOBP0v5tnzpwu41umQwk9I7Ru0ucD7j+DW4k8otadW+FnI\n"
"G1M1MpSjAkEAyRMt4bN8otfpOpsOQWzw4jQtouohOxRFCrQTntHhU20PrQnQLZWs\n"
"pOVzqCjRytYtkPEUA1z8QK5gGcVPcOQsowJBALmt2rwPB1NrEo5Bat7noO+Zb3Ob\n"
"WDiYWeE8xkHd95gDlSWiC53ur9aINo6ZeP556jGIgL+el/yHHecJLrQL84sCQH48\n"
"zUxq/C/cb++8UzneJGlPqusiJNTLiAENR1gpmlZfHT1c8Nb9phMsfu0vG29GAfuC\n"
"bzchVLljALCNQK+2gRMCQQCNIgN+R9mRWZhFAcC1sq++YnuSBlw4VwdL/fd1Yg9e\n"
"Ul+U98yPl/NXt8Rs4TRBFcOZjkFI8xv0hQtevTgTmgz+\n"
"-----END RSA PRIVATE KEY-----\n\n";
	char *pubkey =
"-----BEGIN PUBLIC KEY-----\n"
"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQChe5/Fzk9SA0vCKBOcu9jBcLb9\n"
"oLv50PeuEfQojhakY+OH8A3QM8A0qIDG6uhTNGPvzCWb/+mKeOB48n5HJpLxlDFy\n"
"P3kyd2yXHIZ/MN8g1nh4FsB0iTkk8QUCJkkan6FCOBrIeLEsGA5AdodzuR+khnCM\n"
"t8vO+NFHZYKAQeynyQIDAQAB\n"
"-----END PUBLIC KEY-----\n\n";

	priv = pem_read_buf(pkcs1);

	src.v = text;
	src.l = strlen(text);

	/* sign */
	sig = eay_rsa_sign(&src, priv);
	if (sig == NULL)
		printf("sign failed. %s\n", eay_strerror());
	printf("RSA signed data.\n");
	PVDUMP(sig);

	/* verify */
	pub = pem_read_buf(pubkey);
	error = eay_rsa_verify(&src, sig, pub);
	if (error)
		printf("verifying failed.\n");
	else
		printf("verified.\n");
#endif
	return;
}

#if 0
static vchar_t *
pem_read_buf(buf)
	char *buf;
{
	BIO *bio;
	char *nm = NULL, *header = NULL;
	unsigned char *data = NULL;
	long len;
	vchar_t *ret;
	int error;

	bio = BIO_new_mem_buf(buf, strlen(buf));
	error = PEM_read_bio(bio, &nm, &header, &data, &len);
	if (error == 0)
		errx(1, "%s", eay_strerror());
	ret = vmalloc(len);
	if (ret == NULL)
		err(1, "vmalloc");
	memcpy(ret->v, data, len);

	return ret;
}
#endif

void
certtest(ac, av)
	int ac;
	char **av;
{
	char *certpath;
	char **certs;
	int type;
	int error;

	printf("\n**Test for Certificate.**\n");

    {
	char dnstr[] = "C=JP, ST=Kanagawa, L=Fujisawa, O=WIDE Project, OU=KAME Project, CN=Shoichi Sakane/Email=sakane@kame.net";
	vchar_t *asn1dn = NULL, asn1dn0;
	char dn0[] = {
		0x30,0x81,0x9a,0x31,0x0b,0x30,0x09,0x06,
		0x03,0x55,0x04,0x06,0x13,0x02,0x4a,0x50,
		0x31,0x11,0x30,0x0f,0x06,0x03,0x55,0x04,
		0x08,0x13,0x08,0x4b,0x61,0x6e,0x61,0x67,
		0x61,0x77,0x61,0x31,0x11,0x30,0x0f,0x06,
		0x03,0x55,0x04,0x07,0x13,0x08,0x46,0x75,
		0x6a,0x69,0x73,0x61,0x77,0x61,0x31,0x15,
		0x30,0x13,0x06,0x03,0x55,0x04,0x0a,0x13,
		0x0c,0x57,0x49,0x44,0x45,0x20,0x50,0x72,
		0x6f,0x6a,0x65,0x63,0x74,0x31,0x15,0x30,
		0x13,0x06,0x03,0x55,0x04,0x0b,0x13,0x0c,
		0x4b,0x41,0x4d,0x45,0x20,0x50,0x72,0x6f,
		0x6a,0x65,0x63,0x74,0x31,0x17,0x30,0x15,
		0x06,0x03,0x55,0x04,0x03,0x13,0x0e,0x53,
		0x68,0x6f,0x69,0x63,0x68,0x69,0x20,0x53,
		0x61,0x6b,0x61,0x6e,0x65,0x31,0x1e,0x30,
		0x1c,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,
		0x0d,0x01,0x09,0x01,
		0x0c,	/* <== XXX */
		0x0f,0x73,0x61,
		0x6b,0x61,0x6e,0x65,0x40,0x6b,0x61,0x6d,
		0x65,0x2e,0x6e,0x65,0x74,
	};

	printf("check to convert the string into subjectName.\n");
	printf("%s\n", dnstr);

	asn1dn0.v = dn0;
	asn1dn0.l = sizeof(dn0);

	asn1dn = eay_str2asn1dn(dnstr, sizeof(dnstr));
	if (asn1dn == NULL || asn1dn->l != asn1dn0.l)
		errx(1, "asn1dn length mismatched.");

	/*
	 * NOTE: The value pointed by "<==" above is different from the
	 * return of eay_str2asn1dn().  but eay_cmp_asn1dn() can distinguish
	 * both of the names are same name.
	 */
	if (eay_cmp_asn1dn(&asn1dn0,  asn1dn))
		errx(1, "asn1dn mismatched.");
	vfree(asn1dn);

	printf("succeed.\n");
    }

	eay_init_error();

	/* get certs */
	if (ac > 1) {
		certpath = *(av + 1);
		certs = getcerts(certpath);
	} else {
		printf("\nCAUTION: These certificates are probably invalid "
			"on your environment because you don't have their "
			"issuer's certs in your environment.\n\n");

		certpath = "/usr/local/openssl/certs";
		certs = getcerts(NULL);
	}

	while (*certs != NULL) {

		vchar_t c;
		char *str;
		vchar_t *vstr;

		printf("===CERT===\n");

		c.v = *certs;
		c.l = strlen(*certs);

		/* print text */
		str = eay_get_x509text(&c);
		printf("%s", str);
		racoon_free(str);

		/* print ASN.1 of subject name */
		vstr = eay_get_x509asn1subjectname(&c);
		if (!vstr)
			return;
		PVDUMP(vstr);
		printf("\n");
		vfree(vstr);

		/* print subject alt name */
	    {
		int pos;
		for (pos = 1; ; pos++) {
			error = eay_get_x509subjectaltname(&c, &str, &type, pos);
			if (error) {
				printf("no subjectaltname found.\n");
				break;
			}
			if (!str)
				break;
			printf("SubjectAltName: %d: %s\n", type, str);
			racoon_free(str);
		}
	    }

		error = eay_check_x509cert(&c, certpath);
		if (error)
			printf("ERROR: cert is invalid.\n");
		printf("\n");

		certs++;
	}
}

static char **
getcerts(path)
	char *path;
{
	char **certs = NULL, **p;
	DIR *dirp;
	struct dirent *dp;
	struct stat sb;
	char buf[512];
	int len;
	int n;
	int fd;

	static char *samplecerts[] = {
/* self signed */
"-----BEGIN CERTIFICATE-----\n"
"MIICpTCCAg4CAQAwDQYJKoZIhvcNAQEEBQAwgZoxCzAJBgNVBAYTAkpQMREwDwYD\n"
"VQQIEwhLYW5hZ2F3YTERMA8GA1UEBxMIRnVqaXNhd2ExFTATBgNVBAoTDFdJREUg\n"
"UHJvamVjdDEVMBMGA1UECxMMS0FNRSBQcm9qZWN0MRcwFQYDVQQDEw5TaG9pY2hp\n"
"IFNha2FuZTEeMBwGCSqGSIb3DQEJARYPc2FrYW5lQGthbWUubmV0MB4XDTAwMDgy\n"
"NDAxMzc0NFoXDTAwMDkyMzAxMzc0NFowgZoxCzAJBgNVBAYTAkpQMREwDwYDVQQI\n"
"EwhLYW5hZ2F3YTERMA8GA1UEBxMIRnVqaXNhd2ExFTATBgNVBAoTDFdJREUgUHJv\n"
"amVjdDEVMBMGA1UECxMMS0FNRSBQcm9qZWN0MRcwFQYDVQQDEw5TaG9pY2hpIFNh\n"
"a2FuZTEeMBwGCSqGSIb3DQEJARYPc2FrYW5lQGthbWUubmV0MIGfMA0GCSqGSIb3\n"
"DQEBAQUAA4GNADCBiQKBgQCpIQG/H3zn4czAmPBcbkDrYxE1A9vcpghpib3Of0Op\n"
"SsiWIBOyIMiVAzK/I/JotWp3Vdn5fzGp/7DGAbWXAALas2xHkNmTMPpu6qhmNQ57\n"
"kJHZHal24mgc1hwbrI9fb5olvIexx9a1riNPnKMRVHzXYizsyMbf+lJJmZ8QFhWN\n"
"twIDAQABMA0GCSqGSIb3DQEBBAUAA4GBACKs6X/BYycuHI3iop403R3XWMHHnNBN\n"
"5XTHVWiWgR1cMWkq/dp51gn+nPftpdAaYGpqGkiHGhZcXLoBaX9uON3p+7av+sQN\n"
"plXwnvUf2Zsgu+fojskS0gKcDlYiq1O8TOaBgJouFZgr1q6PiYjVEJGogAP28+HN\n"
"M4o+GBFbFoqK\n"
"-----END CERTIFICATE-----\n\n",
/* signed by SSH testing CA + CA1 + CA2 */
"-----BEGIN X509 CERTIFICATE-----\n"
"MIICtTCCAj+gAwIBAgIEOaR8NjANBgkqhkiG9w0BAQUFADBjMQswCQYDVQQGEwJG\n"
"STEkMCIGA1UEChMbU1NIIENvbW11bmljYXRpb25zIFNlY3VyaXR5MREwDwYDVQQL\n"
"EwhXZWIgdGVzdDEbMBkGA1UEAxMSVGVzdCBDQSAxIHN1YiBjYSAyMB4XDTAwMDgy\n"
"NDAwMDAwMFoXDTAwMTAwMTAwMDAwMFowgZoxCzAJBgNVBAYTAkpQMREwDwYDVQQI\n"
"EwhLYW5hZ2F3YTERMA8GA1UEBxMIRnVqaXNhd2ExFTATBgNVBAoTDFdJREUgUHJv\n"
"amVjdDEVMBMGA1UECxMMS0FNRSBQcm9qZWN0MRcwFQYDVQQDEw5TaG9pY2hpIFNh\n"
"a2FuZTEeMBwGCSqGSIb3DQEJAQwPc2FrYW5lQGthbWUubmV0MIGfMA0GCSqGSIb3\n"
"DQEBAQUAA4GNADCBiQKBgQCpIQG/H3zn4czAmPBcbkDrYxE1A9vcpghpib3Of0Op\n"
"SsiWIBOyIMiVAzK/I/JotWp3Vdn5fzGp/7DGAbWXAALas2xHkNmTMPpu6qhmNQ57\n"
"kJHZHal24mgc1hwbrI9fb5olvIexx9a1riNPnKMRVHzXYizsyMbf+lJJmZ8QFhWN\n"
"twIDAQABo18wXTALBgNVHQ8EBAMCBaAwGgYDVR0RBBMwEYEPc2FrYW5lQGthbWUu\n"
"bmV0MDIGA1UdHwQrMCkwJ6AloCOGIWh0dHA6Ly9sZGFwLnNzaC5maS9jcmxzL2Nh\n"
"MS0yLmNybDANBgkqhkiG9w0BAQUFAANhADtaqual41OWshF/rwCTuR6zySBJysGp\n"
"+qjkp5efCiYKhAu1L4WXlMsV/SNdzspui5tHasPBvUw8gzFsU/VW/B2zuQZkimf1\n"
"u6ZPjUb/vt8vLOPScP5MeH7xrTk9iigsqQ==\n"
"-----END X509 CERTIFICATE-----\n\n",
/* VP100 */
"-----BEGIN CERTIFICATE-----\n"
"MIICXzCCAcigAwIBAgIEOXGBIzANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJG\n"
"STEkMCIGA1UEChMbU1NIIENvbW11bmljYXRpb25zIFNlY3VyaXR5MREwDwYDVQQL\n"
"EwhXZWIgdGVzdDESMBAGA1UEAxMJVGVzdCBDQSAxMB4XDTAwMDcxNjAwMDAwMFoX\n"
"DTAwMDkwMTAwMDAwMFowNTELMAkGA1UEBhMCanAxETAPBgNVBAoTCHRhaGl0ZXN0\n"
"MRMwEQYDVQQDEwpmdXJ1a2F3YS0xMIGdMA0GCSqGSIb3DQEBAQUAA4GLADCBhwKB\n"
"gQDUmI2RaAuoLvtRDbASwRhbkj/Oq0BBIKgAqbFknc/EanJSQwZQu82gD88nf7gG\n"
"VEioWmKPLDuEjz5JCuM+k5f7HYHI1wWmz1KFr7UA+avZm4Kp6YKnhuH7soZp7kBL\n"
"hTiZEpL0jdmCWLW3ZXoro55rmPrBsCd+bt8VU6tRZm5dUwIBKaNZMFcwCwYDVR0P\n"
"BAQDAgWgMBYGA1UdEQQPMA2CBVZQMTAwhwQKFIaFMDAGA1UdHwQpMCcwJaAjoCGG\n"
"H2h0dHA6Ly9sZGFwLnNzaC5maS9jcmxzL2NhMS5jcmwwDQYJKoZIhvcNAQEFBQAD\n"
"gYEAKJ/2Co/KYW65mwpGG3CBvsoRL8xyUMHGt6gQpFLHiiHuAdix1ADTL6uoFuYi\n"
"4sE5omQm1wKVv2ZhS03zDtUfKoVEv0HZ7IY3AU/FZT/M5gQvbt43Dki/ma3ock2I\n"
"PPhbLsvXm+GCVh3jvkYGk1zr7VERVeTPtmT+hW63lcxfFp4=\n"
"-----END CERTIFICATE-----\n\n",
/* IKED */
"-----BEGIN CERTIFICATE-----\n"
"MIIEFTCCA7+gAwIBAgIKYU5X6AAAAAAACTANBgkqhkiG9w0BAQUFADCBljEpMCcG\n"
"CSqGSIb3DQEJARYaeS13YXRhbmFAc2RsLmhpdGFjaGkuY28uanAxCzAJBgNVBAYT\n"
"AkpQMREwDwYDVQQIEwhLQU5BR0FXQTERMA8GA1UEBxMIWW9rb2hhbWExEDAOBgNV\n"
"BAoTB0hJVEFDSEkxDDAKBgNVBAsTA1NETDEWMBQGA1UEAxMNSVBzZWMgVGVzdCBD\n"
"QTAeFw0wMDA3MTUwMjUxNDdaFw0wMTA3MTUwMzAxNDdaMEUxCzAJBgNVBAYTAkpQ\n"
"MREwDwYDVQQIEwhLQU5BR0FXQTEQMA4GA1UEChMHSElUQUNISTERMA8GA1UEAxMI\n"
"V0FUQU5BQkUwXDANBgkqhkiG9w0BAQEFAANLADBIAkEA6Wja5A7Ldzrtx+rMWHEB\n"
"Cyt+/ZoG0qdFQbuuUiU1vOSq+1f+ZSCYAdTq13Lrr6Xfz3jDVFEZLPID9PSTFwq+\n"
"yQIDAQABo4ICPTCCAjkwDgYDVR0PAQH/BAQDAgTwMBMGA1UdJQQMMAoGCCsGAQUF\n"
"CAICMB0GA1UdDgQWBBTkv7/MH5Ra+S1zBAmnUIH5w8ZTUTCB0gYDVR0jBIHKMIHH\n"
"gBQsF2qoaTl5F3GFLKrttaxPJ8j4faGBnKSBmTCBljEpMCcGCSqGSIb3DQEJARYa\n"
"eS13YXRhbmFAc2RsLmhpdGFjaGkuY28uanAxCzAJBgNVBAYTAkpQMREwDwYDVQQI\n"
"EwhLQU5BR0FXQTERMA8GA1UEBxMIWW9rb2hhbWExEDAOBgNVBAoTB0hJVEFDSEkx\n"
"DDAKBgNVBAsTA1NETDEWMBQGA1UEAxMNSVBzZWMgVGVzdCBDQYIQeccIf4GYDIBA\n"
"rS6HSUt8XjB7BgNVHR8EdDByMDagNKAyhjBodHRwOi8vZmxvcmEyMjAvQ2VydEVu\n"
"cm9sbC9JUHNlYyUyMFRlc3QlMjBDQS5jcmwwOKA2oDSGMmZpbGU6Ly9cXGZsb3Jh\n"
"MjIwXENlcnRFbnJvbGxcSVBzZWMlMjBUZXN0JTIwQ0EuY3JsMIGgBggrBgEFBQcB\n"
"AQSBkzCBkDBFBggrBgEFBQcwAoY5aHR0cDovL2Zsb3JhMjIwL0NlcnRFbnJvbGwv\n"
"ZmxvcmEyMjBfSVBzZWMlMjBUZXN0JTIwQ0EuY3J0MEcGCCsGAQUFBzAChjtmaWxl\n"
"Oi8vXFxmbG9yYTIyMFxDZXJ0RW5yb2xsXGZsb3JhMjIwX0lQc2VjJTIwVGVzdCUy\n"
"MENBLmNydDANBgkqhkiG9w0BAQUFAANBAG8yZAWHb6g3zba453Hw5loojVDZO6fD\n"
"9lCsyaxeo9/+7x1JEEcdZ6qL7KKqe7ZBwza+hIN0ITkp2WEWo22gTz4=\n"
"-----END CERTIFICATE-----\n\n",
/* From Entrust */
"-----BEGIN CERTIFICATE-----\n"
"MIIDXTCCAsagAwIBAgIEOb6khTANBgkqhkiG9w0BAQUFADA4MQswCQYDVQQGEwJV\n"
"UzEQMA4GA1UEChMHRW50cnVzdDEXMBUGA1UECxMOVlBOIEludGVyb3AgUk8wHhcN\n"
"MDAwOTE4MjMwMDM3WhcNMDMwOTE4MjMzMDM3WjBTMQswCQYDVQQGEwJVUzEQMA4G\n"
"A1UEChMHRW50cnVzdDEXMBUGA1UECxMOVlBOIEludGVyb3AgUk8xGTAXBgNVBAMT\n"
"EFNob2ljaGkgU2FrYW5lIDIwgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBAKj3\n"
"eXSt1qXxFXzpa265B/NQYk5BZN7pNJg0tlTKBTVV3UgpQ92Bx5DoNfZh11oIv0Sw\n"
"6YnG5p9F9ma36U9HDoD3hVTjAvQKy4ssCsnU1y6v5XOU1QvYQo6UTzgsXUTaIau4\n"
"Lrccl+nyoiNzy3lG51tLR8CxuA+3OOAK9xPjszClAgMBAAGjggFXMIIBUzBABgNV\n"
"HREEOTA3gQ9zYWthbmVAa2FtZS5uZXSHBM6vIHWCHjIwNi0xNzUtMzItMTE3LnZw\n"
"bndvcmtzaG9wLmNvbTATBgNVHSUEDDAKBggrBgEFBQgCAjALBgNVHQ8EBAMCAKAw\n"
"KwYDVR0QBCQwIoAPMjAwMDA5MTgyMzAwMzdagQ8yMDAyMTAyNTExMzAzN1owWgYD\n"
"VR0fBFMwUTBPoE2gS6RJMEcxCzAJBgNVBAYTAlVTMRAwDgYDVQQKEwdFbnRydXN0\n"
"MRcwFQYDVQQLEw5WUE4gSW50ZXJvcCBSTzENMAsGA1UEAxMEQ1JMMTAfBgNVHSME\n"
"GDAWgBTzVmhu0tBoWKwkZE5mXpooE9630DAdBgNVHQ4EFgQUEgBHPtXggJqei5Xz\n"
"92CrWXTJxfAwCQYDVR0TBAIwADAZBgkqhkiG9n0HQQAEDDAKGwRWNS4wAwIEsDAN\n"
"BgkqhkiG9w0BAQUFAAOBgQCIFriNGMUE8GH5LuDrTJfA8uGx8vLy2seljuo694TR\n"
"et/ojp9QnfOJ1PF9iAdGaEaSLfkwhY4fZNZzxic5HBoHLeo9BXLP7i7FByXjvOZC\n"
"Y8++0dC8NVvendIILcJBM5nbDq1TqIbb8K3SP80XhO5JLVJkoZiQftAMjo0peZPO\n"
"EQ==\n"
"-----END CERTIFICATE-----\n\n",
	NULL,
	};

	if (path == NULL)
		return (char **)&samplecerts;

	stat(path, &sb);
	if (!(sb.st_mode & S_IFDIR)) {
		printf("ERROR: %s is not directory.\n", path);
		exit(0);
	}

	dirp = opendir(path);
	if (dirp == NULL) {
		printf("opendir failed.\n");
		exit(0);
	}

	n = 0;
	while ((dp = readdir(dirp)) != NULL) {
		if (dp->d_type != DT_REG)
			continue;
		if (strcmp(dp->d_name + dp->d_namlen - 4, "cert"))
			continue;
		snprintf(buf, sizeof(buf), "%s/%s", path, dp->d_name);
		stat(buf, &sb);

		p = (char **)realloc(certs, (n + 1) * sizeof(certs));
		if (p == NULL)
			err(1, "realloc");
		certs = p;

		certs[n] = malloc(sb.st_size + 1);
		if (certs[n] == NULL)
			err(1, "malloc");

		fd = open(buf, O_RDONLY);
		if (fd == -1)
			err(1, "open");
		len = read(fd, certs[n], sb.st_size);
		if (len == -1)
			err(1, "read");
		if (len != sb.st_size)
			errx(1, "read: length mismatch");
		certs[n][sb.st_size] = '\0';
		close(fd);

		printf("%s: %d\n", dp->d_name, (int)sb.st_size);

		n++;
	}

	p = (char **)realloc(certs, (n + 1) * sizeof(certs));
	if (p == NULL)
		err(1, "realloc");
	certs = p;
	certs[n] = NULL;

	return certs;
}

void
ciphertest(ac, av)
	int ac;
	char **av;
{
	vchar_t data;
	vchar_t key;
	vchar_t iv0;
	vchar_t *res1, *res2, *iv;

	printf("\n**Test for CIPHER.**\n");

	data.v = str2val("\
06000017 03000000 73616b61 6e65406b 616d652e 6e657409 0002c104 308202b8 \
04f05a90 \
	", 16, &data.l);
	key.v = str2val("f59bd70f 81b9b9cc 2a32c7fd 229a4b37", 16, &key.l);
	iv0.v = str2val("26b68c90 9467b4ab 7ec29fa0 0b696b55", 16, &iv0.l);

	iv = vmalloc(8);

	/* des */
	printf("DES\n");
	printf("data:\n");
	PVDUMP(&data);

	memcpy(iv->v, iv0.v, 8);
	res1 = eay_des_encrypt(&data, &key, iv);
	if (res1 == NULL)
		errx(1, "length must be 8");
	printf("encrypto:\n");
	PVDUMP(res1);

	memcpy(iv->v, iv0.v, 8);
	res2 = eay_des_decrypt(res1, &key, iv);
	printf("decrypto:\n");
	PVDUMP(res2);

	if (memcmp(data.v, res2->v, data.l))
		printf("XXX NG XXX\n");
	vfree(res1);
	vfree(res2);

#ifdef HAVE_OPENSSL_IDEA_H
	/* idea */
	printf("IDEA\n");
	printf("data:\n");
	PVDUMP(&data);

	memcpy(iv->v, iv0.v, 8);
	res1 = eay_idea_encrypt(&data, &key, iv);
	printf("encrypto:\n");
	PVDUMP(res1);

	memcpy(iv->v, iv0.v, 8);
	res2 = eay_idea_decrypt(res1, &key, iv);
	printf("decrypto:\n");
	PVDUMP(res2);

	if (memcmp(data.v, res2->v, data.l))
		printf("XXX NG XXX\n");
	vfree(res1);
	vfree(res2);
#endif

	/* blowfish */
	printf("BLOWFISH\n");
	printf("data:\n");
	PVDUMP(&data);

	memcpy(iv->v, iv0.v, 8);
	res1 = eay_bf_encrypt(&data, &key, iv);
	printf("encrypto:\n");
	PVDUMP(res1);

	memcpy(iv->v, iv0.v, 8);
	res2 = eay_bf_decrypt(res1, &key, iv);
	printf("decrypto:\n");
	PVDUMP(res2);

	if (memcmp(data.v, res2->v, data.l))
		printf("XXX NG XXX\n");
	vfree(res1);
	vfree(res2);

#ifdef HAVE_OPENSSL_RC5_H
	/* rc5 */
	printf("RC5\n");
	printf("data:\n");
	PVDUMP(&data);

	memcpy(iv->v, iv0.v, 8);
	res1 = eay_bf_encrypt(&data, &key, iv);
	printf("encrypto:\n");
	PVDUMP(res1);

	memcpy(iv->v, iv0.v, 8);
	res2 = eay_bf_decrypt(res1, &key, iv);
	printf("decrypto:\n");
	PVDUMP(res2);

	if (memcmp(data.v, res2->v, data.l))
		printf("XXX NG XXX\n");
	vfree(res1);
	vfree(res2);
#endif

	/* 3des */
	printf("3DES\n");
	printf("data:\n");
	PVDUMP(&data);

	memcpy(iv->v, iv0.v, 8);
	res1 = eay_3des_encrypt(&data, &key, iv);
	printf("encrypto:\n");
	if (res1)
		PVDUMP(res1);

	memcpy(iv->v, iv0.v, 8);
	res2 = eay_3des_decrypt(res1, &key, iv);
	printf("decrypto:\n");
	if (res1)
		PVDUMP(res2);

	if (res2 && memcmp(data.v, res2->v, data.l))
		printf("XXX NG XXX\n");
	vfree(res1);
	vfree(res2);

	/* cast */
	printf("CAST\n");
	printf("data:\n");
	PVDUMP(&data);

	memcpy(iv->v, iv0.v, 8);
	res1 = eay_cast_encrypt(&data, &key, iv);
	printf("encrypto:\n");
	PVDUMP(res1);

	memcpy(iv->v, iv0.v, 8);
	res2 = eay_cast_decrypt(res1, &key, iv);
	printf("decrypto:\n");
	PVDUMP(res2);

	if (memcmp(data.v, res2->v, data.l))
		printf("XXX NG XXX\n");
	vfree(res1);
	vfree(res2);

	/* aes */
	iv = vrealloc(iv, 16);

	printf("AES\n");
	printf("data:\n");
	PVDUMP(&data);

    {
	vchar_t *buf;
	int padlen = 16 - data.l % 16;
	buf = vmalloc(data.l + padlen);
	memcpy(buf->v, data.v, data.l);

	memcpy(iv->v, iv0.v, 16);
	res1 = eay_aes_encrypt(buf, &key, iv);
	printf("encrypto:\n");
	PVDUMP(res1);

	memcpy(iv->v, iv0.v, 16);
	res2 = eay_aes_decrypt(res1, &key, iv);
	printf("decrypto:\n");
	PVDUMP(res2);

	if (memcmp(data.v, res2->v, data.l))
		printf("XXX NG XXX\n");
	vfree(res1);
	vfree(res2);
    }
}

void
hmactest(ac, av)
	int ac;
	char **av;
{
	char *keyword = "hehehe test secret!";
	char *object  = "d7e6a6c1876ef0488bb74958b9fee94e";
	char *object1 = "d7e6a6c1876ef048";
	char *object2 =                 "8bb74958b9fee94e";
	char *r_hmd5  = "5702d7d1 fd1bfc7e 210fc9fa cda7d02c";
	char *r_hsha1 = "309999aa 9779a43e ebdea839 1b4e7ee1 d8646874";
#ifdef WITH_SHA2
	char *r_hsha2 = "d47262d8 a5b6f39d d8686939 411b3e79 ed2e27f9 2c4ea89f dd0a06ae 0c0aa396";
#endif
	vchar_t *key, *data, *data1, *data2, *res;
	vchar_t mod;
	caddr_t ctx;

	printf("\n**Test for HMAC MD5 & SHA1.**\n");

	key = vmalloc(strlen(keyword));
	memcpy(key->v, keyword, key->l);

	data = vmalloc(strlen(object));
	data1 = vmalloc(strlen(object1));
	data2 = vmalloc(strlen(object2));
	memcpy(data->v, object, data->l);
	memcpy(data1->v, object1, data1->l);
	memcpy(data2->v, object2, data2->l);

	/* HMAC MD5 */
	printf("HMAC MD5 by eay_hmacmd5_one()\n");
	res = eay_hmacmd5_one(key, data);
	PVDUMP(res);
	mod.v = str2val(r_hmd5, 16, &mod.l);
	if (memcmp(res->v, mod.v, mod.l))
		printf(" XXX NG XXX\n");
	free(mod.v);
	vfree(res);

	/* HMAC MD5 */
	printf("HMAC MD5 by eay_hmacmd5_xxx()\n");
	ctx = eay_hmacmd5_init(key);
	eay_hmacmd5_update(ctx, data1);
	eay_hmacmd5_update(ctx, data2);
	res = eay_hmacmd5_final(ctx);
	PVDUMP(res);
	mod.v = str2val(r_hmd5, 16, &mod.l);
	if (memcmp(res->v, mod.v, mod.l))
		printf(" XXX NG XXX\n");
	free(mod.v);
	vfree(res);

#ifdef WITH_SHA2
	/* HMAC SHA2 */
	printf("HMAC SHA2 by eay_hmacsha2_256_one()\n");
	res = eay_hmacsha2_256_one(key, data);
	PVDUMP(res);
	mod.v = str2val(r_hsha2, 16, &mod.l);
	if (memcmp(res->v, mod.v, mod.l))
		printf(" XXX NG XXX\n");
	free(mod.v);
	vfree(res);
#endif

	/* HMAC SHA1 */
	printf("HMAC SHA1 by eay_hmacsha1_one()\n");
	res = eay_hmacsha1_one(key, data);
	PVDUMP(res);
	mod.v = str2val(r_hsha1, 16, &mod.l);
	if (memcmp(res->v, mod.v, mod.l))
		printf(" XXX NG XXX\n");
	free(mod.v);
	vfree(res);

	/* HMAC MD5 */
	printf("HMAC SHA1 by eay_hmacsha1_xxx()\n");
	ctx = eay_hmacsha1_init(key);
	eay_hmacsha1_update(ctx, data1);
	eay_hmacsha1_update(ctx, data2);
	res = eay_hmacsha1_final(ctx);
	PVDUMP(res);
	mod.v = str2val(r_hsha1, 16, &mod.l);
	if (memcmp(res->v, mod.v, mod.l))
		printf(" XXX NG XXX\n");
	free(mod.v);
	vfree(res);

	vfree(data);
	vfree(data1);
	vfree(data2);
	vfree(key);
}

void
sha1test(ac, av)
	int ac;
	char **av;
{
	char *word1 = "1234567890", *word2 = "12345678901234567890";
	caddr_t ctx;
	vchar_t *buf, *res;

	printf("\n**Test for SHA1.**\n");

	ctx = eay_sha1_init();
	buf = vmalloc(strlen(word1));
	memcpy(buf->v, word1, buf->l);
	eay_sha1_update(ctx, buf);
	eay_sha1_update(ctx, buf);
	res = eay_sha1_final(ctx);
	PVDUMP(res);
	vfree(res);
	vfree(buf);

	ctx = eay_sha1_init();
	buf = vmalloc(strlen(word2));
	memcpy(buf->v, word2, buf->l);
	eay_sha1_update(ctx, buf);
	res = eay_sha1_final(ctx);
	PVDUMP(res);
	vfree(res);

	res = eay_sha1_one(buf);
	PVDUMP(res);
	vfree(res);
	vfree(buf);
}

void
md5test(ac, av)
	int ac;
	char **av;
{
	char *word1 = "1234567890", *word2 = "12345678901234567890";
	caddr_t ctx;
	vchar_t *buf, *res;

	printf("\n**Test for MD5.**\n");

	ctx = eay_md5_init();
	buf = vmalloc(strlen(word1));
	memcpy(buf->v, word1, buf->l);
	eay_md5_update(ctx, buf);
	eay_md5_update(ctx, buf);
	res = eay_md5_final(ctx);
	PVDUMP(res);
	vfree(res);
	vfree(buf);

	ctx = eay_md5_init();
	buf = vmalloc(strlen(word2));
	memcpy(buf->v, word2, buf->l);
	eay_md5_update(ctx, buf);
	res = eay_md5_final(ctx);
	PVDUMP(res);
	vfree(res);

	res = eay_md5_one(buf);
	PVDUMP(res);
	vfree(res);
	vfree(buf);
}

void
dhtest(ac, av)
	int ac;
	char **av;
{
	static struct {
		char *name;
		char *p;
	} px[] = {
		{ "modp768",	OAKLEY_PRIME_MODP768, },
		{ "modp1024",	OAKLEY_PRIME_MODP1024, },
		{ "modp1536",	OAKLEY_PRIME_MODP1536, },
		{ "modp2048",	OAKLEY_PRIME_MODP2048, },
		{ "modp3072",	OAKLEY_PRIME_MODP3072, },
		{ "modp4096",	OAKLEY_PRIME_MODP4096, },
		{ "modp6144",	OAKLEY_PRIME_MODP6144, },
		{ "modp8192",	OAKLEY_PRIME_MODP8192, },
	};
	vchar_t p1, *pub1, *priv1, *gxy1;
	vchar_t p2, *pub2, *priv2, *gxy2;
	int i;

	printf("\n**Test for DH.**\n");

	for (i = 0; i < sizeof(px)/sizeof(px[0]); i++) {
		printf("\n**Test for DH %s.**\n", px[i].name);

		p1.v = str2val(px[i].p, 16, &p1.l);
		p2.v = str2val(px[i].p, 16, &p2.l);
		printf("prime number = \n"); PVDUMP(&p1);

		if (eay_dh_generate(&p1, 2, 96, &pub1, &priv1) < 0) {
			printf("error\n");
			return;
		}
		printf("private key for user 1 = \n"); PVDUMP(priv1);
		printf("public key for user 1  = \n"); PVDUMP(pub1);

		if (eay_dh_generate(&p2, 2, 96, &pub2, &priv2) < 0) {
			printf("error\n");
			return;
		}
		printf("private key for user 2 = \n"); PVDUMP(priv2);
		printf("public key for user 2  = \n"); PVDUMP(pub2);

		/* process to generate key for user 1 */
		gxy1 = vmalloc(p1.l);
		memset(gxy1->v, 0, gxy1->l);
		eay_dh_compute(&p1, 2, pub1, priv1, pub2, &gxy1);
		printf("sharing gxy1 of user 1 = \n"); PVDUMP(gxy1);

		/* process to generate key for user 2 */
		gxy2 = vmalloc(p1.l);
		memset(gxy2->v, 0, gxy2->l);
		eay_dh_compute(&p2, 2, pub2, priv2, pub1, &gxy2);
		printf("sharing gxy2 of user 2 = \n"); PVDUMP(gxy2);

		if (memcmp(gxy1->v, gxy2->v, gxy1->l))
			printf("ERROR: sharing gxy mismatched.\n");

		vfree(pub1);
		vfree(pub2);
		vfree(priv1);
		vfree(priv2);
		vfree(gxy1);
		vfree(gxy2);
	}

	return;
}

void
bntest(ac, av)
	int ac;
	char **av;
{
	vchar_t *rn;

	printf("\n**Test for generate a random number.**\n");

	rn = eay_set_random((u_int32_t)96);
	PVDUMP(rn);
	vfree(rn);
}

struct {
	char *name;
	void (*func) __P((int, char **));
} func[] = {
	{ "random", bntest, },
	{ "dh", dhtest, },
	{ "md5", md5test, },
	{ "sha1", sha1test, },
	{ "hmac", hmactest, },
	{ "cipher", ciphertest, },
	{ "cert", certtest, },
	{ "rsa", rsatest, },
};

int
main(ac, av)
	int ac;
	char **av;
{
	int i;
	int len = sizeof(func)/sizeof(func[0]);

	if (strcmp(*av, "-h") == 0) {
		printf("Usage: eaytest [");
		for (i = 0; i < len; i++) {
			printf("%s", func[i].name);
			if (i != len)
				printf("|");
		}
		printf("]\n");
		Usage();
	}

	ac--;
	av++;

	if (ac == 0) {
		for (i = 0; i < len; i++)
			(func[i].func)(ac, av);
	} else {
		for (i = 0; i < len; i++) {
			if (strcmp(*av, func[i].name) == 0) {
				(func[i].func)(ac, av);
				break;
			}
		}
		if (i == len)
			Usage();
	}

	exit(0);
}

void
Usage()
{
	printf("Usage: eaytest [dh|md5|sha1|hmac|cipher]\n");
	printf("       eaytest cert [cert_directory]\n");
	exit(0);
}
