/*	$KAME: eaytest.c,v 1.19 2000/12/17 23:04:19 sakane Exp $	*/

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

#include <sys/types.h>

#include <netinet/in.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "var.h"
#include "vmbuf.h"
#include "misc.h"
#include "debug.h"
#include "str2val.h"

#include "oakley.h"
#include "crypto_openssl.h"

#define PVDUMP(var) hexdump((var)->v, (var)->l)

u_int32_t loglevel = 4;

char *capath = "/usr/local/openssl/certs";
char *certs[] = {
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
};

/* prototype */

void certtest __P((void));
void ciphertest __P((void));
void hmactest __P((void));
void sha1test __P((void));
void md5test __P((void));
void dhtest __P((int));
void bntest __P((void));

/* test */

#include <sys/stat.h>
#include <unistd.h>
void
certtest()
{
	vchar_t c;
	char *str;
	vchar_t *vstr;
	int type;
	int error;
	int i;

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
	if (asn1dn == NULL || asn1dn->l != asn1dn0.l) {
		printf("asn1dn length mismatched.\n");
		exit(1);
	}
	/*
	 * NOTE: The value pointed by "<==" above is different from the
	 * return of eay_str2asn1dn().  but eay_cmp_asn1dn() can distinguish
	 * both of the names are same name.
	 */
	if (eay_cmp_asn1dn(&asn1dn0,  asn1dn)) {
		printf("asn1dn mismatched.\n");
		exit(1);
	}
	vfree(asn1dn);

	printf("succeed.\n");
    }

	printf("\nCAUTION: These certificates may be invalid on your "
		"environment because it was signed by SSH test CA and you "
		"may not own their issuer's certificates.\n\n");

	eay_init_error();

	for (i = 0; i < sizeof(certs)/sizeof(certs[0]); i++) {

		printf("CERT[%d]===\n", i);

		c.v = certs[i];
		c.l = strlen(certs[i]);

		/* print text */
		str = eay_get_x509text(&c);
		printf("%s", str);
		free(str);

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
			free(str);
		}
	    }

	    {
		struct stat sb;

		stat(capath, &sb);
		if (!(sb.st_mode & S_IFDIR)) {
			printf("ERROR: %s is not directory.\n", capath);
			return;
		}
	    }

		error = eay_check_x509cert(&c, capath);
		printf("cert is %s\n", error ? "invalid" : "valid");
		printf("\n");
	}
}

void
ciphertest()
{
	vchar_t data;
	vchar_t key;
	vchar_t *res1, *res2;
	char iv[8];

	printf("\n**Test for CIPHER.**\n");

	data.v = str2val("a7c3a855 a328a6d4 b1bd9c06 c5bd5c17 b8c5f657 bd8ea245 2a6726d0 ce3689f5", 16, &data.l);
	key.v = str2val("fadc3844 61d6114e fadc3844 61d6114e fadc3844 61d6114e", 16, &key.l);

	/* des */
	printf("DES\n");
	printf("data:\n");
	PVDUMP(&data);

	memset(iv, 0, sizeof(iv));
	res1 = eay_des_encrypt(&data, &key, (caddr_t)iv);
	printf("encrypto:\n");
	PVDUMP(res1);

	memset(iv, 0, sizeof(iv));
	res2 = eay_des_decrypt(res1, &key, (caddr_t)iv);
	printf("decrypto:\n");
	PVDUMP(res2);

	vfree(res1);
	vfree(res2);

#ifdef HAVE_OPENSSL_IDEA_H
	/* idea */
	printf("IDEA\n");
	printf("data:\n");
	PVDUMP(&data);

	memset(iv, 0, sizeof(iv));
	res1 = eay_idea_encrypt(&data, &key, (caddr_t)iv);
	printf("encrypto:\n");
	PVDUMP(res1);

	memset(iv, 0, sizeof(iv));
	res2 = eay_idea_decrypt(res1, &key, (caddr_t)iv);
	printf("decrypto:\n");
	PVDUMP(res2);

	vfree(res1);
	vfree(res2);
#endif

	/* blowfish */
	printf("BLOWFISH\n");
	printf("data:\n");
	PVDUMP(&data);

	memset(iv, 0, sizeof(iv));
	res1 = eay_bf_encrypt(&data, &key, (caddr_t)iv);
	printf("encrypto:\n");
	PVDUMP(res1);

	memset(iv, 0, sizeof(iv));
	res2 = eay_bf_decrypt(res1, &key, (caddr_t)iv);
	printf("decrypto:\n");
	PVDUMP(res2);

	vfree(res1);
	vfree(res2);

#ifdef HAVE_OPENSSL_RC5_H
	/* rc5 */
	printf("RC5\n");
	printf("data:\n");
	PVDUMP(&data);

	memset(iv, 0, sizeof(iv));
	res1 = eay_bf_encrypt(&data, &key, (caddr_t)iv);
	printf("encrypto:\n");
	PVDUMP(res1);

	memset(iv, 0, sizeof(iv));
	res2 = eay_bf_decrypt(res1, &key, (caddr_t)iv);
	printf("decrypto:\n");
	PVDUMP(res2);

	vfree(res1);
	vfree(res2);
#endif

	/* 3des */
	printf("3DES\n");
	printf("data:\n");
	PVDUMP(&data);

	memset(iv, 0, sizeof(iv));
	res1 = eay_3des_encrypt(&data, &key, (caddr_t)iv);
	printf("encrypto:\n");
	PVDUMP(res1);

	memset(iv, 0, sizeof(iv));
	res2 = eay_3des_decrypt(res1, &key, (caddr_t)iv);
	printf("decrypto:\n");
	PVDUMP(res2);

	vfree(res1);
	vfree(res2);

	/* cast */
	printf("CAST\n");
	printf("data:\n");
	PVDUMP(&data);

	memset(iv, 0, sizeof(iv));
	res1 = eay_cast_encrypt(&data, &key, (caddr_t)iv);
	printf("encrypto:\n");
	PVDUMP(res1);

	memset(iv, 0, sizeof(iv));
	res2 = eay_cast_decrypt(res1, &key, (caddr_t)iv);
	printf("decrypto:\n");
	PVDUMP(res2);

	vfree(res1);
	vfree(res2);
}

void
hmactest()
{
	u_char *keyword = "hehehe test secret!";
	vchar_t kir, ki, kr;
	vchar_t *key, *data, *res, *data2;

	printf("\n**Test for HMAC MD5 & SHA1.**\n");

	kir.v = str2val("d7e6a6c1876ef0488bb74958b9fee94efdb563d4e18de4ec03a4a1842d432985", 16, &kir.l);
	ki.v = str2val("d7e6a6c1876ef0488bb74958b9fee94e", 16, &ki.l);
	kr.v = str2val("fdb563d4e18de4ec03a4a1842d432985", 16, &kr.l);

	key = vmalloc(strlen(keyword));
	memcpy(key->v, keyword, key->l);

	data = vmalloc(kir.l);
	memcpy(data->v, kir.v, kir.l);

	/* HMAC MD5 */
	printf("HMAC MD5\n");
	res = eay_hmacmd5_one(key, data);
	PVDUMP(res);
	vfree(res);

	/* HMAC SHA1 */
	printf("HMAC SHA1\n");
	res = eay_hmacsha1_one(key, data);
	PVDUMP(res);
	vfree(res);

	vfree(data);

	/* HMAC SHA1 */
	data = vmalloc(ki.l);
	memcpy(data->v, ki.v, ki.l);
	data2 = vmalloc(kr.l);
	memcpy(data2->v, kr.v, kr.l);

	printf("HMAC SHA1\n");
	res = (vchar_t *)eay_hmacsha1_oneX(key, data, data2);
	PVDUMP(res);
	vfree(res);

	vfree(key);
}

void
sha1test()
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
md5test()
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
dhtest(f)
	int f;
{
	vchar_t p1, p2, *pub1, *priv1, *pub2, *priv2, *key;

	printf("\n**Test for DH.**\n");

	switch (f) {
	case 0:
		p1.v = str2val(OAKLEY_PRIME_MODP768, 16, &p1.l);
		p2.v = str2val(OAKLEY_PRIME_MODP768, 16, &p2.l);
		break;
	case 1:
		p1.v = str2val(OAKLEY_PRIME_MODP1024, 16, &p1.l);
		p2.v = str2val(OAKLEY_PRIME_MODP1024, 16, &p2.l);
		break;
	case 2:
	default:
		p1.v = str2val(OAKLEY_PRIME_MODP1536, 16, &p1.l);
		p2.v = str2val(OAKLEY_PRIME_MODP1536, 16, &p2.l);
		break;
	}
	printf("prime number = \n"); PVDUMP(&p1);

	key = vmalloc(p1.l);

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
	memset(key->v, 0, key->l);
	eay_dh_compute(&p1, 2, pub1, priv1, pub2, &key);
	printf("sharing key of user 1 = \n"); PVDUMP(key);

	/* process to generate key for user 2 */
	memset(key->v, 0, key->l);
	eay_dh_compute(&p2, 2, pub2, priv2, pub1, &key);
	printf("sharing key of user 2 = \n"); PVDUMP(key);

	vfree(pub1);
	vfree(priv1);
	vfree(priv2);
	vfree(key);

	return;
}

void
bntest()
{
	vchar_t *rn;

	printf("\n**Test for generate a random number.**\n");

	rn = eay_set_random((u_int32_t)96);
	PVDUMP(rn);
	vfree(rn);
}

int
main(ac, av)
	int ac;
	char **av;
{
	if (strcmp(*av, "-h") == 0) {
		printf("Usage: eaytest [dh|md5|sha1|hmac|cipher|cert]\n");
		exit(0);
	}

	if (ac == 1) {
		bntest();
		dhtest(0);
		md5test();
		sha1test();
		hmactest();
		ciphertest();
		certtest();
	} else {
		for (av++; *av != '\0'; av++) {
			if (strcmp(*av, "random") == 0)
				bntest();
			else if (strcmp(*av, "dh") == 0)
				dhtest(0);
			else if (strcmp(*av, "md5") == 0)
				md5test();
			else if (strcmp(*av, "sha1") == 0)
				sha1test();
			else if (strcmp(*av, "hmac") == 0)
				hmactest();
			else if (strcmp(*av, "cipher") == 0)
				ciphertest();
			else if (strcmp(*av, "cert") == 0)
				certtest();
		}
	}

	exit(0);
}
