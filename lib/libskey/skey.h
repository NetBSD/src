/*	$NetBSD: skey.h,v 1.8 2000/07/28 16:35:11 thorpej Exp $	*/

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
 *          Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Main client header
 */

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

/* Maximum sequence number we allow */
#ifndef SKEY_MAX_SEQ
#define SKEY_MAX_SEQ           10000
#endif

/* Minimum secret password length (rfc2289) */
#ifndef SKEY_MIN_PW_LEN
#define SKEY_MIN_PW_LEN                10
#endif

/* Max secret password length (rfc2289 says 63 but allows more) */
#ifndef SKEY_MAX_PW_LEN
#define SKEY_MAX_PW_LEN                255
#endif

/* Max length of an S/Key seed (rfc2289) */
#ifndef SKEY_MAX_SEED_LEN
#define SKEY_MAX_SEED_LEN  	 16
#endif

/* Max length of S/Key challenge (otp-???? 9999 seed) */
#ifndef SKEY_MAX_CHALLENGE
#define SKEY_MAX_CHALLENGE 	 (11 + SKEY_MAX_HASHNAME_LEN + SKEY_MAX_SEED_LEN)
#endif

/* Max length of hash algorithm name (md4/md5/sha1/rmd160) */
#define SKEY_MAX_HASHNAME_LEN  6

/* Size of a binary key (not NULL-terminated) */
#define SKEY_BINKEY_SIZE		 8

/* Location of random file for bogus challenges */
#define _SKEY_RAND_FILE_PATH_  "/var/db/host.random"

/* Prototypes */
void f __P ((char *));
int keycrunch __P ((char *, const char *, const char *));
char *btoe __P ((char *, const char *));
char *put8 __P ((char *, const char *));
int etob __P ((char *, const char *));
void rip __P ((char *));
int skeychallenge __P ((struct skey *, const char *, char *, size_t));
int skeylookup __P ((struct skey *, const char *));
int skeyverify __P ((struct skey *, char *));
void sevenbit __P ((char *));
void backspace __P ((char *));
const char *skipspace __P ((const char *));
char *readpass __P ((char *, int));
char *readskey __P ((char *, int));
int skey_authenticate __P ((const char *));
int skey_passcheck __P ((const char *, char *));
const char *skey_keyinfo __P ((const char *));
int skey_haskey __P ((const char *));
int getskeyprompt __P ((struct skey *, char *, char *));
int atob8 __P((char *, const char *));
int btoa8 __P((char *, const char *));
int htoi __P((int));
const char *skey_get_algorithm __P((void));
const char *skey_set_algorithm __P((const char *));
int skeygetnext __P((struct skey *));
int skeyzero __P((struct skey *, char *));
