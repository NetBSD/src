/*	$NetBSD: skey.h,v 1.6 2000/01/23 02:11:02 mycroft Exp $	*/

/*
 * S/KEY v1.1b (skey.h)
 *
 * Authors:
 *          Neil M. Haller <nmh@thumper.bellcore.com>
 *          Philip R. Karn <karn@chicago.qualcomm.com>
 *          John S. Walden <jsw@thumper.bellcore.com>
 *
 * Modifications:
 *          Scott Chasin <chasin@crimelab.com>
 *
 * Main client header
 */

#if	defined(__TURBOC__) || defined(__STDC__) || defined(LATTICE)
#define	ANSIPROTO	1
#endif

#ifndef	__ARGS
#ifdef	ANSIPROTO
#define	__ARGS(x)	x
#else
#define	__ARGS(x)	()
#endif
#endif

#ifdef SOLARIS
#define setpriority(x,y,z)      z
#endif

/* Server-side data structure for reading keys file during login */
struct skey
{
  FILE *keyfile;
  char buf[256];
  char *logname;
  int n;
  char *seed;
  char *val;
  long recstart;		/* needed so reread of buffer is efficient */


};

/* Client-side structure for scanning data stream for challenge */
struct mc
{
  char buf[256];
  int skip;
  int cnt;
};

void f __ARGS ((char *x));
int keycrunch __ARGS ((char *result, const char *seed, const char *passwd));
char *btoe __ARGS ((char *engout, const char *c));
char *put8 __ARGS ((char *out, const char *s));
int etob __ARGS ((char *out, const char *e));
void rip __ARGS ((char *buf));
int skeychallenge __ARGS ((struct skey * mp, const char *name, char *ss, int sslen));
int skeylookup __ARGS ((struct skey * mp, const char *name));
int skeyverify __ARGS ((struct skey * mp, char *response));
void sevenbit __ARGS ((char *s));
void backspace __ARGS ((char *s));
const char *skipspace __ARGS ((const char *s));
char *readpass __ARGS ((char *buf, int n));
char *readskey __ARGS ((char *buf, int n));
int skey_authenticate __ARGS ((const char *));
int skey_passcheck __ARGS ((const char *, char *));
char *skey_keyinfo __ARGS ((const char *));
int skey_haskey __ARGS ((const char *));
int getskeyprompt __ARGS ((struct skey *, char *, char *));
int atob8 __ARGS((char *, const char *));
int btoa8 __ARGS((char *, const char *));
int htoi __ARGS((int));

