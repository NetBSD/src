/*	$NetBSD: md5.c,v 1.2 1997/10/17 11:37:09 lukem Exp $	*/

/*
 * MDDRIVER.C - test driver for MD2, MD4 and MD5
 */

/*
 *  Copyright (C) 1990-2, RSA Data Security, Inc. Created 1990. All
 *  rights reserved.
 *
 *  RSA Data Security, Inc. makes no representations concerning either
 *  the merchantability of this software or the suitability of this
 *  software for any particular purpose. It is provided "as is"
 *  without express or implied warranty of any kind.
 *
 *  These notices must be retained in any copies of any part of this
 *  documentation and/or software.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: md5.c,v 1.2 1997/10/17 11:37:09 lukem Exp $");
#endif /* not lint */

#include <sys/types.h>

#include <err.h>
#include <md5.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

void	MDFilter __P((int));
void	MDString __P((const char *));
void	MDTestSuite __P((void));
void	MDTimeTrial __P((void));

/*
 * Length of test block, number of test blocks.
 */
#define TEST_BLOCK_LEN 1000
#define TEST_BLOCK_COUNT 1000

/*
 * Digests a string and prints the result.
 */
void
MDString(string)
	const char *string;
{
	unsigned int len = strlen(string);
	char buf[33];

	printf("MD5 (\"%s\") = %s\n", string, MD5Data(string, len, buf));
}

/*
 * Measures the time to digest TEST_BLOCK_COUNT TEST_BLOCK_LEN-byte blocks.
 */
void
MDTimeTrial()
{
	MD5_CTX context;
	time_t endTime, startTime;
	unsigned char block[TEST_BLOCK_LEN];
	unsigned int i;
	char *p, buf[33];

	printf("MD5 time trial.  Digesting %d %d-byte blocks ...",
	    TEST_BLOCK_LEN, TEST_BLOCK_COUNT);
	fflush(stdout);

	/* Initialize block */
	for (i = 0; i < TEST_BLOCK_LEN; i++)
		block[i] = (unsigned char) (i & 0xff);

	/* Start timer */
	time(&startTime);

	/* Digest blocks */
	MD5Init(&context);
	for (i = 0; i < TEST_BLOCK_COUNT; i++)
		MD5Update(&context, block, TEST_BLOCK_LEN);
	p = MD5End(&context,buf);

	/* Stop timer */
	time(&endTime);

	printf(" done\n");
	printf("Digest = %s\n", p);
	printf("Time = %ld seconds\n", (long) (endTime - startTime));

	/*
	 * Be careful that endTime-startTime is not zero.
	 * (Bug fix from Ric * Anderson, ric@Artisoft.COM.)
	 */
	printf("Speed = %ld bytes/second\n",
	    (long) TEST_BLOCK_LEN * (long) TEST_BLOCK_COUNT /
	    ((endTime - startTime) != 0 ? (endTime - startTime) : 1));
}

/*
 * Digests a reference suite of strings and prints the results.
 */
void
MDTestSuite()
{
	printf("MD5 test suite:\n");

	MDString("");
	MDString("a");
	MDString("abc");
	MDString("message digest");
	MDString("abcdefghijklmnopqrstuvwxyz");
	MDString
	    ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
	MDString
	    ("1234567890123456789012345678901234567890\
1234567890123456789012345678901234567890");
}

/*
 * Digests the standard input and prints the result.
 */
void
MDFilter(pipe)
	int pipe;
{
	MD5_CTX context;
	int len;
	unsigned char buffer[BUFSIZ];
	char buf[33];

	MD5Init(&context);
	while ((len = fread(buffer, 1, BUFSIZ, stdin)) > 0) {
		if (pipe && (len != fwrite(buffer, 1, len, stdout)))
			err(1, "stdout");
		MD5Update(&context, buffer, len);
	}
	printf("%s\n", MD5End(&context,buf));
}
