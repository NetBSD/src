/*	$NetBSD: skeysubr.c,v 1.16 2000/07/06 22:30:19 mjl Exp $	*/

/* S/KEY v1.1b (skeysubr.c)
 *
 * Authors:
 *          Neil M. Haller <nmh@thumper.bellcore.com>
 *          Philip R. Karn <karn@chicago.qualcomm.com>
 *          John S. Walden <jsw@thumper.bellcore.com>
 *
 * Modifications:
 *          Scott Chasin <chasin@crimelab.com>
 *          Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * S/KEY misc routines.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <termios.h>

#include <md4.h>
#include <md5.h>
#include <rmd160.h>
#include <sha1.h>

#include "skey.h"

/* Default hash function to use (index into skey_hash_types array) */
#ifndef SKEY_HASH_DEFAULT
#define SKEY_HASH_DEFAULT  	 0		/* MD4 */
#endif

static void f_md4 __P((char *));
static void f_md5 __P((char *));
static void f_sha1 __P((char *));
/* static void f_rmd160 __P((char *x)); */
static int keycrunch_md4 __P((char *, const char *, const char *));
static int keycrunch_md5 __P((char *, const char *, const char *));
static int keycrunch_sha1 __P((char *, const char *, const char *));
/* static int keycrunch_rmd160 __P((char *, const char *, const char *)); */
static void lowcase __P((char *));
static void skey_echo __P((int));
static void trapped __P((int));
static char *mkSeedPassword(const char *, const char *, size_t *);

/* Current hash type (index into skey_hash_types array) */
static int skey_hash_type = SKEY_HASH_DEFAULT;

/*
 * Hash types we support.
 * Each has an associated keycrunch() and f() function.
 */

struct skey_algorithm_table {
	const char *name;
	int (*keycrunch) __P((char *, const char *, const char *));
	void (*f) __P((char *));
};
static struct skey_algorithm_table skey_algorithm_table[] = {
	{ "md4", keycrunch_md4, f_md4 },
	{ "md5", keycrunch_md5, f_md5 },
	{ "sha1", keycrunch_sha1, f_sha1 },
#if 0
	{ "rmd160", keycrunch_rmd160, f_rmd160 },
#endif
	{ NULL }
};

/*
 * Crunch a key:
 * concatenate the (lower cased) seed and the password, run through
 * the hash algorithm and collapse to 64 bits.
 * This is defined as the user's starting key.
 */
int keycrunch(char *result, 	    /* SKEY_BINKEY_SIZE result */
	      const char *seed,     /* Seed, any length */
	      const char *passwd)   /* Password, any length */
{
    return(skey_algorithm_table[skey_hash_type].keycrunch(result, seed, passwd));
}

static char *mkSeedPassword(const char *seed, const char *passwd,
			    size_t *buflen)
{
	char *buf;

	*buflen = strlen(seed) + strlen(passwd);
	if ((buf = (char *) malloc(*buflen + 1)) == NULL)
		return NULL;
	strcpy(buf, seed);
	lowcase(buf);
	strcat(buf, passwd);
	sevenbit(buf);
	
	return buf;
}

static int keycrunch_md4(char *result,       /* SKEY_BINKEY_SIZE result */
			 const char *seed,   /* Seed, any length */
			 const char *passwd) /* Password, any length */
{
	char *buf;
	MD4_CTX md;
	unsigned int buflen;
	u_int32_t results[4];

	if ((buf = mkSeedPassword(seed, passwd, &buflen)) == NULL)
		return -1;

	/* Crunch the key through MD4 */
	MD4Init(&md);
	MD4Update(&md, (unsigned char *) buf, buflen);
	MD4Final((unsigned char *) (void *) results, &md);
	free(buf);

	/* Fold result from 128 to 64 bits */
	results[0] ^= results[2];
	results[1] ^= results[3];

	(void)memcpy(result, results, SKEY_BINKEY_SIZE);

	return 0;
}

static int keycrunch_md5(char *result,		/* SKEY_BINKEY_SIZE result */
			 const char *seed,	/* Seed, any length */
			 const char *passwd)	/* Password, any length */
{
	char *buf;
	MD5_CTX md;
	u_int32_t results[4];
	unsigned int buflen;

	if ((buf = mkSeedPassword(seed, passwd, &buflen)) == NULL)
		return -1;

	/* Crunch the key through MD5 */
	MD5Init(&md);
	MD5Update(&md, (unsigned char *)buf, buflen);
	MD5Final((unsigned char *) (void *)results, &md);
	free(buf);

	/* Fold result from 128 to 64 bits */
	results[0] ^= results[2];
	results[1] ^= results[3];

	(void)memcpy((void *)result, (void *)results, SKEY_BINKEY_SIZE);

	return(0);
}

static int keycrunch_sha1(char *result,		/* SKEY_BINKEY_SIZE result */
			  const char *seed,	/* Seed, any length */
			  const char *passwd)	/* Password, any length */
{
	char *buf;
	SHA1_CTX sha;
	u_int32_t results[5];
	unsigned int buflen;
	int i, j;

	if ((buf = mkSeedPassword(seed, passwd, &buflen)) == NULL)
		return -1;

	/* Crunch the key through SHA1 */
	SHA1Init(&sha);
	SHA1Update(&sha, (unsigned char *)buf, buflen);
	SHA1Final((unsigned char *) (void *)results, &sha);
	free(buf);

	/* Fold 160 to 64 bits */
	results[0] ^= results[2];
	results[1] ^= results[3];
	results[0] ^= results[4];

	/*
	 * Even though my readings of rfc2289 indicate that this is
	 * wrong (it converts stuff to big endian), it is needed to
	 * make the output match up the regression tests in said rfc.
	 * Something is wrong here? --mjl
	 */
		
	for(i=j=0; j<8; i++, j+=4)
		{
		result[j+3] = (unsigned char)(results[i] & 0xff);
		result[j+2] = (unsigned char)((results[i] >> 8) & 0xff);
		result[j+1] = (unsigned char)((results[i] >> 16) & 0xff);
		result[j+0] = (unsigned char)((results[i] >> 24) & 0xff);
		}

	return(0);
}

#if 0
static int keycrunch_rmd160(char *result,	/* SKEY_BINKEY_SIZE result */
			    const char *seed,	/* Seed, any length */
			    const char *passwd)	/* Password, any length */
{
	char *buf;
	RMD160_CTX rmd;
	u_int32_t results[5];
	unsigned int buflen;

	if ((buf = mkSeedPassword(seed, passwd, &buflen)) == NULL)
		return -1;

	/* Crunch the key through RMD-160 */
	RMD160Init(&rmd);
	RMD160Update(&rmd, (unsigned char *)buf, buflen);
	RMD160Final((unsigned char *)(void *)results, &rmd);
	free(buf);

	/* Fold 160 to 64 bits */
	results[0] ^= results[2];
	results[1] ^= results[3];
	results[0] ^= results[4];

	(void)memcpy((void *)result, (void *)results, SKEY_BINKEY_SIZE);

	return(0);
}
#endif

/* The one-way function f(). Takes 8 bytes and returns 8 bytes in place */
void f(char *x)
{
	skey_algorithm_table[skey_hash_type].f(x);
}

static void f_md4(char *x)
{
	MD4_CTX md;
	u_int32_t results[4];

	MD4Init(&md);
	MD4Update(&md, (unsigned char *) x, SKEY_BINKEY_SIZE);
	MD4Final((unsigned char *) (void *) results, &md);

	/* Fold 128 to 64 bits */
	results[0] ^= results[2];
	results[1] ^= results[3];

	(void)memcpy(x, results, SKEY_BINKEY_SIZE);
}

static void f_md5(char *x)
{
	MD5_CTX md;
	u_int32_t results[4];

	MD5Init(&md);
	MD5Update(&md, (unsigned char *)x, SKEY_BINKEY_SIZE);
	MD5Final((unsigned char *) (void *)results, &md);

	/* Fold 128 to 64 bits */
	results[0] ^= results[2];
	results[1] ^= results[3];

	(void)memcpy((void *)x, (void *)results, SKEY_BINKEY_SIZE);
}

static void f_sha1(char *x)
{
	SHA1_CTX sha;
	u_int32_t results[5];
	int i, j;
	
	SHA1Init(&sha);
	SHA1Update(&sha, (unsigned char *)x, SKEY_BINKEY_SIZE);
	SHA1Final((unsigned char *) (void *)results, &sha);

	/* Fold 160 to 64 bits */
	results[0] ^= results[2];
	results[1] ^= results[3];
	results[0] ^= results[4];

	for(i=j=0; j<8; i++, j+=4)
		{
		x[j+3] = (unsigned char)(results[i] & 0xff);
		x[j+2] = (unsigned char)((results[i] >> 8) & 0xff);
		x[j+1] = (unsigned char)((results[i] >> 16) & 0xff);
		x[j+0] = (unsigned char)((results[i] >> 24) & 0xff);
		}
}

#if 0
static void f_rmd160(char *x)
{
	RMD160_CTX rmd;
	u_int32_t results[5];

	RMD160Init(&rmd);
	RMD160Update(&rmd, (unsigned char *)x, SKEY_BINKEY_SIZE);
	RMD160Final((unsigned char *)(void *)results, &rmd);

	/* Fold 160 to 64 bits */
	results[0] ^= results[2];
	results[1] ^= results[3];
	results[0] ^= results[4];

	(void)memcpy((void *)x, (void *)results, SKEY_BINKEY_SIZE);
}
#endif

/* Strip trailing cr/lf from a line of text */
void rip(char *buf)
{
	buf += strcspn(buf, "\r\n");

	if (*buf)
		*buf = '\0';
}

/* Read in secret password (turns off echo) */
char *readpass(char *buf, int n)
{
	void *old_handler;

	/* Turn off echoing */
	skey_echo(0);

	/* Catch SIGINT and save old signal handler */
	old_handler = signal(SIGINT, trapped);

	fgets(buf, n, stdin);
	rip(buf);

	putc('\n', stderr);
	fflush(stderr);

	/* Restore signal handler and turn echo back on */
	if (old_handler != SIG_ERR)
		(void)signal(SIGINT, old_handler);
	skey_echo(1);

	sevenbit(buf);

	return buf;
}

/* Read in an s/key OTP (does not turn off echo) */
char *readskey(char *buf, int n)
{
	fgets(buf, n, stdin);

	rip(buf);

	sevenbit (buf);

	return buf;
}

/* Signal handler for trapping ^C */
/*ARGSUSED*/
static void trapped(int sig)
{
	fputs("^C\n", stderr);
	fflush(stderr);

	/* Turn on echo if necesary */
	skey_echo(1);

	exit(-1);
}

/*
 * Convert 8-byte hex-ascii string to binary array
 * Returns 0 on success, -1 on error
 */
int atob8(char *out, const char *in)
{
	int i;
	int val;

	if (in == NULL || out == NULL)
		return -1;

	for (i=0; i<8; i++) {
		if ((in = skipspace(in)) == NULL)
			return -1;
		if ((val = htoi(*in++)) == -1)
			return -1;
		*out = val << 4;

		if ((in = skipspace(in)) == NULL)
			return -1;
		if ((val = htoi(*in++)) == -1)
			return -1;
		*out++ |= val;
	}
	return 0;
}

/* Convert 8-byte binary array to hex-ascii string */
int btoa8(char *out, const char *in)
{
	int i;

	if (in == NULL || out == NULL)
		return -1;

	for (i=0;i<8;i++) {
		sprintf(out, "%02x", *in++ & 0xff);
		out += 2;
	}
	return 0;
}


/* Convert hex digit to binary integer */
int htoi(int c)
{
	if ('0' <= c && c <= '9')
		return c - '0';
	if ('a' <= c && c <= 'f')
		return 10 + c - 'a';
	if ('A' <= c && c <= 'F')
		return 10 + c - 'A';
	return -1;
}

/* Skip leading spaces from the string */
const char *skipspace(const char *cp)
{
	while (*cp == ' ' || *cp == '\t')
		cp++;

	if (*cp == '\0')
		return NULL;
	else
		return cp;
}

/* Remove backspaced over charaters from the string */
void backspace(char *buf)
{
	char bs = 0x8;
	char *cp = buf;
	char *out = buf;

	while (*cp) {
		if (*cp == bs) {
			if (out == buf) {
				cp++;
				continue;
			} else {
			  cp++;
			  out--;
			}
		} else {
			*out++ = *cp++;
		}

	}
	*out = '\0';
}

/* Make sure line is all seven bits */
void sevenbit(char *s)
{
	while (*s)
		*s++ &= 0x7f;
}

/* Set hash algorithm type */
const char *skey_set_algorithm(const char *new)
{
	int i;

	for (i = 0; skey_algorithm_table[i].name; i++) {
		if (strcmp(new, skey_algorithm_table[i].name) == 0) {
			skey_hash_type = i;
			return(new);
		}
	}

	return(NULL);
}

/* Get current hash type */
const char *skey_get_algorithm()
{
	return(skey_algorithm_table[skey_hash_type].name);
}

/* Turn echo on/off */
static void skey_echo(int action)
{
	static struct termios term;
	static int echo = 0;

	if (action == 0) {
		/* Turn echo off */
		(void) tcgetattr(fileno(stdin), &term);
		if ((echo = (term.c_lflag & ECHO))) {
			term.c_lflag &= ~ECHO;
			(void) tcsetattr(fileno(stdin), TCSAFLUSH|TCSASOFT, &term);
		}
	} else if (action && echo) {
		/* Turn echo on */
		term.c_lflag |= ECHO;
		(void) tcsetattr(fileno(stdin), TCSAFLUSH|TCSASOFT, &term);
		echo = 0;
	}
}

/* Convert string to lower case */
static void lowcase(char *s)
{
	char *p;

	for (p = s; *p; p++)
		if (isupper(*p))
			*p = tolower(*p);
}
