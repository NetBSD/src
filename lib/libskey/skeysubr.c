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
 *
 * $Id: skeysubr.c,v 1.1 1994/05/21 05:46:19 deraadt Exp $
 */

#include <stdio.h>

#ifdef HAS_STD_LIB
#include <stdlib.h>
#else
#include <sys/types.h>
#endif

#include <string.h>
#include <signal.h>

#ifdef	__MSDOS__
#include <dos.h>
#endif

#ifdef stty
# undef stty
#endif
 
#ifdef gtty
# undef gtty
#endif

#ifndef SYSV
# include <sgtty.h>
# define TTYSTRUCT sgttyb
#ifndef __NetBSD__
# define stty(fd,buf) ioctl((fd),TIOCSETN,(buf))
# define gtty(fd,buf) ioctl((fd),TIOCGETP,(buf))
#endif
#else
# include <termio.h>
# define TTYSTRUCT termio
# define stty(fd,buf) ioctl((fd),TCSETA,(buf))
# define gtty(fd,buf) ioctl((fd),TCGETA,(buf))
#endif

#ifdef SYSV
    struct termio newtty;
    struct termio oldtty;
#else
    struct sgttyb newtty;
    struct sgttyb oldtty;
    struct tchars chars;
#endif

#ifdef SIGVOID
#define SIGTYPE void
#else
#define SIGTYPE void
#endif

SIGTYPE trapped();

#include "md4.h"
#include "skey.h"

#if (defined(__MSDOS__) || defined(MPU8086) || defined(MPU8080) \
    || defined(vax) || defined (MIPSEL))
#define	LITTLE_ENDIAN
#endif

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
	MDstruct md;
	unsigned int buflen;
#ifndef	LITTLE_ENDIAN
	int i;
	register long tmp;
#endif
	
	buflen = strlen(seed) + strlen(passwd);
	if ((buf = (char *)malloc(buflen+1)) == NULL)
		return -1;
	strcpy(buf,seed);
	strcat(buf,passwd);

	/* Crunch the key through MD4 */
	sevenbit(buf);
	MDbegin(&md);
	MDupdate(&md,(unsigned char *)buf,8*buflen);

	free(buf);

	/* Fold result from 128 to 64 bits */
	md.buffer[0] ^= md.buffer[2];
	md.buffer[1] ^= md.buffer[3];

#ifdef	LITTLE_ENDIAN
	/* Only works on byte-addressed little-endian machines!! */
	memcpy(result,(char *)md.buffer,8);
#else
	/* Default (but slow) code that will convert to
	 * little-endian byte ordering on any machine
	 */
	for (i=0; i<2; i++) {
		tmp = md.buffer[i];
		*result++ = tmp;
		tmp >>= 8;
		*result++ = tmp;
		tmp >>= 8;
		*result++ = tmp;
		tmp >>= 8;
		*result++ = tmp;
	}
#endif

	return 0;
}

/* The one-way function f(). Takes 8 bytes and returns 8 bytes in place */
void f (x)
char *x;
{
	MDstruct md;
#ifndef	LITTLE_ENDIAN
	register long tmp;
#endif

	MDbegin(&md);
	MDupdate(&md,(unsigned char *)x,64);

	/* Fold 128 to 64 bits */
	md.buffer[0] ^= md.buffer[2];
	md.buffer[1] ^= md.buffer[3];

#ifdef	LITTLE_ENDIAN
	/* Only works on byte-addressed little-endian machines!! */
	memcpy(x,(char *)md.buffer,8);

#else
	/* Default (but slow) code that will convert to
	 * little-endian byte ordering on any machine
	 */
	tmp = md.buffer[0];
	*x++ = tmp;
	tmp >>= 8;
	*x++ = tmp;
	tmp >>= 8;
	*x++ = tmp;
	tmp >>= 8;
	*x++ = tmp;

	tmp = md.buffer[1];
	*x++ = tmp;
	tmp >>= 8;
	*x++ = tmp;
	tmp >>= 8;
	*x++ = tmp;
	tmp >>= 8;
	*x = tmp;
#endif
}

/* Strip trailing cr/lf from a line of text */
void rip (buf)
char *buf;
{
	char *cp;

	if((cp = strchr(buf,'\r')) != NULL)
		*cp = '\0';

	if((cp = strchr(buf,'\n')) != NULL)
		*cp = '\0';
}

#ifdef	__MSDOS__
char *readpass(buf,n)
char *buf;
int n;
{
  int i;
  char *cp;

  for (cp=buf,i = 0; i < n ; i++)
       if ((*cp++ = bdos(7,0,0)) == '\r')
          break;
   *cp = '\0';
   putchar('\n');
   rip(buf);
   return buf;
}
#else

char *readpass (buf,n)
char *buf;
int n;
{

#ifndef USE_ECHO
    set_term ();
    echo_off ();
#endif

    fgets (buf, n, stdin);

    rip (buf);

    printf ("\n\n");
    sevenbit (buf);

#ifndef USE_ECHO
    unset_term ();
#endif
    return buf;
}

set_term () 
{
    gtty (fileno(stdin), &newtty);
    gtty (fileno(stdin), &oldtty);
 
    signal (SIGINT, trapped);
}

echo_off ()
{

#ifdef SYSV
    newtty.c_lflag &= ~(ICANON | ECHO | ECHONL);
#else
    newtty.sg_flags |= CBREAK;
    newtty.sg_flags &= ~ECHO;
#endif

#ifdef SYSV
    newtty.c_cc[VMIN] = 1;
    newtty.c_cc[VTIME] = 0;
    newtty.c_cc[VINTR] = 3;
#else
    ioctl(fileno(stdin), TIOCGETC, &chars);
    chars.t_intrc = 3;
    ioctl(fileno(stdin), TIOCSETC, &chars);
#endif

    stty (fileno (stdin), &newtty);
}

unset_term ()
{
    stty (fileno (stdin), &oldtty);
 
#ifndef SYSV
    ioctl(fileno(stdin), TIOCSETC, &chars);
#endif
}

void trapped()
 {
  signal (SIGINT, trapped);
  printf ("^C\n");
  unset_term ();
  exit (-1);
 }

#endif

/* removebackspaced over charaters from the string */
backspace(buf)
char *buf;
{
	char bs = 0x8;
	char *cp = buf;
	char *out = buf;

	while(*cp){
		if( *cp == bs ) {
			if(out == buf){
				cp++;
				continue;
			}
			else {
			  cp++;
			  out--;
			}
		}
		else {
			*out++ = *cp++;
		}

	}
	*out = '\0';
	
}

/* sevenbit ()
 *
 * Make sure line is all seven bits.
 */
 
sevenbit (s)
char *s;
{
   while (*s) {
     *s = 0x7f & ( *s);
     s++;
   }
}
