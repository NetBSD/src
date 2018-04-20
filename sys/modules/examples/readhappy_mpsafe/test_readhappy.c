/*	$NetBSD: test_readhappy.c,v 1.1 2018/04/20 00:06:45 kamil Exp $	*/

/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Compile this program with -pthread and run it to test the readhappy_mpsafe
 * module. This program should print out happy numbers read from /dev/happy_mpsafe.
 * Insert the module and make the /dev/happy_mpsafe node before executing this file.
 */


static void* read_happy(void *);

static FILE *fp;

/*
 * thread function used to read /dev/happy_mpsafe and print them out.
 */
static void *
read_happy(void *unused)
{
	int line;

	for (;;) {
		fscanf(fp, "%d", &line);
		printf("%d\n", line);
		fflush(stdout);
	}
	/* NOTREACHED */
	return NULL;
}

/*
 *  main function : opens /dev/happy_mpsafe and sets the global file descriptor
 *  creates 100 threads which try to use the file descriptor to read
 *  from /dev/happy_mpsafe.
 *  This function does not return anything and will run in an infinite loop.
 */
int
main(int argc, const char *argv[])
{
	pthread_t thr[100];

	fp = fopen("/dev/happy_mpsafe","r");
	if (fp == NULL)
		err(EXIT_FAILURE, "open");
	for (int i = 0; i < 100; i++)
		pthread_create(&thr[i], NULL, read_happy, NULL);
	(void)read_happy(NULL);
	/* NOTREACHED */
	return EXIT_SUCCESS;
}
