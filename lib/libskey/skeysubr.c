/*	$NetBSD: skeysubr.c,v 1.10 1997/06/28 01:12:19 christos Exp $	*/

/* S/KEY v1.1b (skeysubr.c)
 *
 * Authors:
 *          Neil M. Haller <nmh@thumper.bellcore.com>
 *          Philip R. Karn <karn@chicago.qualcomm.com>
 *          John S. Walden <jsw@thumper.bellcore.com>
 *
 * Modifications: 
 *          Scott Chasin <chasin@crimelab.com>
 *
 * S/KEY misc routines.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <md4.h>
#include "skey.h"

struct termios newtty;
struct termios oldtty;

static void trapped __ARGS((int sig));
static void set_term __ARGS((void));
static void unset_term __ARGS((void));
static void echo_off __ARGS((void));

/* Crunch a key:
 * concatenate the seed and the password, run through MD4 and
 * collapse to 64 bits. This is defined as the user's starting key.
 */
int
keycrunch(result,seed,passwd)
char *result;	/* 8-byte result */
char *seed;	/* Seed, any length */
char *passwd;	/* Password, any length */
{
	char *buf;
	MD4_CTX md;
	unsigned int buflen;
	int i;
	register int tmp;
	u_int32_t hash[4];
	
	buflen = strlen(seed) + strlen(passwd);
	if ((buf = (char *)malloc(buflen+1)) == NULL)
		return -1;
	strcpy(buf, seed);	/* XXX strcpy is safe */
	strcat(buf, passwd);	/* XXX strcat is safe */

	/* Crunch the key through MD4 */
	sevenbit(buf);
	MD4Init(&md);
	MD4Update(&md, (unsigned char *) buf, buflen);
	MD4Final((unsigned char *) hash, &md);

	free(buf);

	/* Fold result from 128 to 64 bits */
	hash[0] ^= hash[2];
	hash[1] ^= hash[3];

	/* Default (but slow) code that will convert to
	 * little-endian byte ordering on any machine
	 */
	for (i=0; i<2; i++) {
		tmp = hash[i];
		*result++ = tmp;
		tmp >>= 8;
		*result++ = tmp;
		tmp >>= 8;
		*result++ = tmp;
		tmp >>= 8;
		*result++ = tmp;
	}

	return 0;
}

/* The one-way function f(). Takes 8 bytes and returns 8 bytes in place */
void
f(x)
	char *x;
{
	MD4_CTX md;
	register int tmp;
	u_int32_t hash[4];

	MD4Init(&md);
	MD4Update(&md, (unsigned char *) x, 8);
	MD4Final((unsigned char *) hash, &md);

	/* Fold 128 to 64 bits */
	hash[0] ^= hash[2];
	hash[1] ^= hash[3];

	/* Default (but slow) code that will convert to
	 * little-endian byte ordering on any machine
	 */
	tmp = hash[0];
	*x++ = tmp;
	tmp >>= 8;
	*x++ = tmp;
	tmp >>= 8;
	*x++ = tmp;
	tmp >>= 8;
	*x++ = tmp;

	tmp = hash[1];
	*x++ = tmp;
	tmp >>= 8;
	*x++ = tmp;
	tmp >>= 8;
	*x++ = tmp;
	tmp >>= 8;
	*x = tmp;
}

/* Strip trailing cr/lf from a line of text */
void
rip(buf)
	char *buf;
{
	char *cp;

	if ((cp = strchr(buf,'\r')) != NULL)
		*cp = '\0';

	if ((cp = strchr(buf,'\n')) != NULL)
		*cp = '\0';
}

char *
readpass (buf,n)
	char *buf;
	int n;
{
	set_term();
	echo_off();

	fgets(buf, n, stdin);

	rip(buf);
	printf("\n");

	sevenbit(buf);

	unset_term();
	return buf;
}

char *
readskey(buf, n)
	char *buf;
	int n;
{
	fgets (buf, n, stdin);

	rip(buf);
	printf ("\n");

	sevenbit (buf);

	return buf;
}

static void
set_term() 
{
	fflush(stdout);
	tcgetattr(fileno(stdin), &newtty);
	tcgetattr(fileno(stdin), &oldtty);
 
	signal (SIGINT, trapped);
}

static void
echo_off()
{
	newtty.c_lflag &= ~(ICANON | ECHO | ECHONL);
	newtty.c_cc[VMIN] = 1;
	newtty.c_cc[VTIME] = 0;
	newtty.c_cc[VINTR] = 3;

	tcsetattr(fileno(stdin), TCSADRAIN, &newtty);
}

static void
unset_term()
{
	tcsetattr(fileno(stdin), TCSADRAIN, &oldtty);
}

static void
trapped(sig)
	int sig;
{
	signal(SIGINT, trapped);
	printf("^C\n");
	unset_term();
	exit(-1);
}

/* Convert 8-byte hex-ascii string to binary array
 * Returns 0 on success, -1 on error
 */
int
atob8(out, in)
	register char *out, *in;
{
	register int i;
	register int val;

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
int
btoa8(out, in)
	register char *out, *in;
{
	register int i;

	if (in == NULL || out == NULL)
		return -1;

	for (i=0;i<8;i++) {
		sprintf(out,"%02x",*in++ & 0xff);	/* XXX: sprintf() (btoa8() appears to be unused */
		out += 2;
	}
	return 0;
}


/* Convert hex digit to binary integer */
int
htoi(c)
	register char c;
{
	if ('0' <= c && c <= '9')
		return c - '0';
	if ('a' <= c && c <= 'f')
		return 10 + c - 'a';
	if ('A' <= c && c <= 'F')
		return 10 + c - 'A';
	return -1;
}

char *
skipspace(cp)
	register char *cp;
{
	while (*cp == ' ' || *cp == '\t')
		cp++;

	if (*cp == '\0')
		return NULL;
	else
		return cp;
}

/* removebackspaced over charaters from the string */
void
backspace(buf)
char *buf;
{
	char bs = 0x8;
	char *cp = buf;
	char *out = buf;

	while (*cp) {
		if (*cp == bs) {
			if (out == buf) {
				cp++;
				continue;
			}
			else {
			  cp++;
			  out--;
			}
		} else {
			*out++ = *cp++;
		}

	}
	*out = '\0';
}

/* sevenbit ()
 *
 * Make sure line is all seven bits.
 */
 
void
sevenbit(s)
	char *s;
{
	while (*s)
		*s++ &= 0x7f;
}
