/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2014 Roy Marples <roy@marples.name>
 * All rights reserved

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdint.h>

#include "../crypt/crypt.h"
#include "test.h"

/* RFC2202 MD5 implementation */

static void
print_hmac(uint8_t *hmac)
{
	int i;

	printf("digest = 0x");
	for (i = 0; i < 16; i++)
		printf("%02x", *hmac++);
	printf("\n");
}

static void
hmac_md5_test1(void)
{
	uint8_t hmac[16];
	const uint8_t text[] = "Hi There";
	uint8_t key[16];
	int i;

	printf ("HMAC MD5 Test 1:\t\t");
	for (i = 0; i < 16; i++)
		key[i] = 0x0b;
	hmac_md5(text, 8, key, 16, hmac);
	print_hmac(hmac);
	printf("\t\texpected result:\t 0x9294727a3638bb1c13f48ef8158bfc9d\n");
}

static void
hmac_md5_test2(void)
{
	uint8_t hmac[16];
	const uint8_t text[] = "what do ya want for nothing?";
	const uint8_t key[] = "Jefe";

	printf("HMAC MD5 Test 2:\t\t");
	hmac_md5(text, 28, key, 4, hmac);
	print_hmac(hmac);
	printf("\t\texpected result:\t 0x750c783e6ab0b503eaa863e10a5db738\n");
}

static void
hmac_md5_test3(void)
{
	uint8_t hmac[16];
	uint8_t text[50];
	uint8_t key[16];
	int i;

	printf ("HMAC MD5 Test 3:\t\t");
	for (i = 0; i < 50; i++)
		text[i] = 0xdd;
	for (i = 0; i < 16; i++)
		key[i] = 0xaa;
	hmac_md5(text, 50, key, 16, hmac);
	print_hmac(hmac);
	printf("\t\texpected result:\t 0x56be34521d144c88dbb8c733f0e8b3f6\n");
}

static void
hmac_md5_test4(void)
{
	uint8_t hmac[16];
	uint8_t text[50];
	uint8_t key[25];
	uint8_t i;

	printf ("HMAC MD5 Test 4:\t\t");
	for (i = 0; i < 50; i++)
		text[i] = 0xcd;
	for (i = 0; i < 25; i++)
		key[i] = (uint8_t)(i + 1);
	hmac_md5(text, 50, key, 25, hmac);
	print_hmac(hmac);
	printf("\t\texpected result:\t 0x697eaf0aca3a3aea3a75164746ffaa79\n");
}

static void
hmac_md5_test5(void)
{
	uint8_t hmac[16];
	const uint8_t text[] = "Test With Truncation";
	uint8_t key[16];
	int i;

	printf ("HMAC MD5 Test 5:\t\t");
	for (i = 0; i < 16; i++)
		key[i] = 0x0c;
	hmac_md5(text, 20, key, 16, hmac);
	print_hmac(hmac);
	printf("\t\texpected result:\t 0x56461ef2342edc00f9bab995690efd4c\n");
}

static void
hmac_md5_test6(void)
{
	uint8_t hmac[16];
	const uint8_t text[] = "Test Using Larger Than Block-Size Key - Hash Key First";
	uint8_t key[80];
	int i;

	printf ("HMAC MD5 Test 6:\t\t");
	for (i = 0; i < 80; i++)
		key[i] = 0xaa;
	hmac_md5(text, 54, key, 80, hmac);
	print_hmac(hmac);
	printf("\t\texpected result:\t 0x6b1ab7fe4bd7bf8f0b62e6ce61b9d0cd\n");
}

static void
hmac_md5_test7(void)
{
	uint8_t hmac[16];
	const uint8_t text[] = "Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data";
	uint8_t key[80];
	int i;

	printf ("HMAC MD5 Test 7:\t\t");
	for (i = 0; i < 80; i++)
		key[i] = 0xaa;
	hmac_md5(text, 73, key, 80, hmac);
	print_hmac(hmac);
	printf("\t\texpected result:\t 0x6f630fad67cda0ee1fb1f562db3aa53e\n");
}

int test_hmac_md5(void)
{

	printf ("Starting HMAC MD5 tests...\n\n");
	hmac_md5_test1();
	hmac_md5_test2();
	hmac_md5_test3();
	hmac_md5_test4();
	hmac_md5_test5();
	hmac_md5_test6();
	hmac_md5_test7();
	printf("\nConfirm above results visually against RFC 2202.\n");
	return 0;
}
