/*	$NetBSD: skey.h,v 1.9 2005/02/04 16:12:13 perry Exp $	*/

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
struct skey {
  FILE *keyfile;
  char buf[256];
  char *logname;
  int n;
  char *seed;
  char *val;
  long recstart;		/* needed so reread of buffer is efficient */
};

/* Client-side structure for scanning data stream for challenge */
struct mc {
  char buf[256];
  int skip;
  int cnt;
};

/* Maximum sequence number we allow */
#ifndef SKEY_MAX_SEQ
#define SKEY_MAX_SEQ		10000
#endif

/* Minimum secret password length (rfc2289) */
#ifndef SKEY_MIN_PW_LEN
#define SKEY_MIN_PW_LEN		10
#endif

/* Max secret password length (rfc2289 says 63 but allows more) */
#ifndef SKEY_MAX_PW_LEN
#define SKEY_MAX_PW_LEN		255
#endif

/* Max length of an S/Key seed (rfc2289) */
#ifndef SKEY_MAX_SEED_LEN
#define SKEY_MAX_SEED_LEN	16
#endif

/* Max length of S/Key challenge (otp-???? 9999 seed) */
#ifndef SKEY_MAX_CHALLENGE
#define SKEY_MAX_CHALLENGE     (11 + SKEY_MAX_HASHNAME_LEN + SKEY_MAX_SEED_LEN)
#endif

/* Max length of hash algorithm name (md4/md5/sha1/rmd160) */
#define SKEY_MAX_HASHNAME_LEN	6

/* Size of a binary key (not NULL-terminated) */
#define SKEY_BINKEY_SIZE	8

/* Location of random file for bogus challenges */
#define _SKEY_RAND_FILE_PATH_	"/var/db/host.random"

/* Prototypes */
void f(char *);
int keycrunch(char *, const char *, const char *);
char *btoe(char *, const char *);
char *put8(char *, const char *);
int etob(char *, const char *);
void rip(char *);
int skeychallenge(struct skey *, const char *, char *, size_t);
int skeylookup(struct skey *, const char *);
int skeyverify(struct skey *, char *);
void sevenbit(char *);
void backspace(char *);
const char *skipspace(const char *);
char *readpass(char *, int);
char *readskey(char *, int);
int skey_authenticate(const char *);
int skey_passcheck(const char *, char *);
const char *skey_keyinfo(const char *);
int skey_haskey(const char *);
int getskeyprompt(struct skey *, char *, char *);
int atob8(char *, const char *);
int btoa8(char *, const char *);
int htoi(int);
const char *skey_get_algorithm(void);
const char *skey_set_algorithm(const char *);
int skeygetnext(struct skey *);
int skeyzero(struct skey *, char *);
