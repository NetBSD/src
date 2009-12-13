/*	$NetBSD: ntp_stdlib.h,v 1.1.1.1 2009/12/13 16:54:53 kardel Exp $	*/

/*
 * ntp_stdlib.h - Prototypes for NTP lib.
 */
#ifndef NTP_STDLIB_H
#define NTP_STDLIB_H

#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "l_stdlib.h"
#include "ntp_rfc2553.h"
#include "ntp_types.h"
#include "ntp_string.h"
#include "ntp_net.h"
#include "ntp_syslog.h"


/*
 * Handle gcc __attribute__ if available.
 */
#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5) || (defined(__STRICT_ANSI__))
#  define __attribute__(Spec) /* empty */
# endif
/* The __-protected variants of `format' and `printf' attributes
   are accepted by gcc versions 2.6.4 (effectively 2.7) and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 7)
#  define __format__ format
#  define __printf__ printf
# endif
#endif

extern	void	msyslog		(int, const char *, ...)
				__attribute__((__format__(__printf__, 2, 3)));

/*
 * When building without OpenSSL, use a few macros of theirs to
 * minimize source differences in NTP.
 */
#ifndef OPENSSL
#define NID_md5	4	/* from openssl/objects.h */
/* from openssl/evp.h */
#define EVP_MAX_MD_SIZE	64	/* longest known is SHA512 */
#endif

extern	void	auth_delkeys	(void);
extern	int	auth_havekey	(keyid_t);
extern	int	authdecrypt	(keyid_t, u_int32 *, int, int);
extern	int	authencrypt	(keyid_t, u_int32 *, int);
extern	int	authhavekey	(keyid_t);
extern	int	authistrusted	(keyid_t);
extern	int	authreadkeys	(const char *);
extern	void	authtrust	(keyid_t, u_long);
extern	int	authusekey	(keyid_t, int, const u_char *);

extern	u_long	calyearstart	(u_long);
extern	const char *clockname	(int);
extern	int	clocktime	(int, int, int, int, int, u_long, u_long *, u_int32 *);
#if !defined(_MSC_VER) || !defined(_DEBUG)
extern	void *	emalloc		(size_t);
extern	void *	erealloc	(void *, size_t);
extern	char *	estrdup		(const char *);
#else
extern	void *	debug_erealloc	(void *, size_t, const char *, int);
#define		emalloc(c)	debug_erealloc(NULL, (c), __FILE__, __LINE__)
#define		erealloc(p, c)	debug_erealloc((p), (c), __FILE__, __LINE__)
extern	char *	debug_estrdup	(const char *, const char *, int);
#define		estrdup(s)	debug_estrdup((s), __FILE__, __LINE__)
#endif
extern	int	ntp_getopt	(int, char **, const char *);
extern	void	init_auth	(void);
extern	void	init_lib	(void);
extern	struct savekey *auth_findkey (keyid_t);
extern	int	auth_moremem	(void);
extern	int	ymd2yd		(int, int, int);

/* a_md5encrypt.c */
extern	int	MD5authdecrypt	(int, u_char *, u_int32 *, int, int);
extern	int	MD5authencrypt	(int, u_char *, u_int32 *, int);
extern	void	MD5auth_setkey	(keyid_t, int, const u_char *, const int);
extern	u_int32	addr2refid	(sockaddr_u *);


extern	int	atoint		(const char *, long *);
extern	int	atouint		(const char *, u_long *);
extern	int	hextoint	(const char *, u_long *);
extern	char *	humanlogtime	(void);
extern	char *	inttoa		(long);
extern	char *	mfptoa		(u_long, u_long, short);
extern	char *	mfptoms		(u_long, u_long, short);
extern	const char * modetoa	(int);
extern  const char * eventstr	(int);
extern  const char * ceventstr	(int);
extern	char *	statustoa	(int, int);
extern  const char * sysstatstr (int);
extern  const char * peerstatstr (int);
extern  const char * clockstatstr (int);
extern	sockaddr_u * netof	(sockaddr_u *);
extern	char *	numtoa		(u_int32);
extern	char *	numtohost	(u_int32);
extern	char *	socktoa		(sockaddr_u *);
extern	char *	socktohost	(sockaddr_u *);
extern	int	octtoint	(const char *, u_long *);
extern	u_long	ranp2		(int);
extern	char *	refnumtoa	(sockaddr_u *);
extern	int	tsftomsu	(u_long, int);
extern	char *	uinttoa		(u_long);

extern	int	decodenetnum	(const char *, sockaddr_u *);

extern	const char * FindConfig	(const char *);

extern	void	signal_no_reset (int, RETSIGTYPE (*func)(int));

extern	void	getauthkeys 	(const char *);
extern	void	auth_agekeys	(void);
extern	void	rereadkeys	(void);

/*
 * Variable declarations for libntp.
 */

/*
 * Defined by any program.
 */
extern volatile int debug;		/* debugging flag */

/* authkeys.c */
extern u_long	authkeynotfound;	/* keys not found */
extern u_long	authkeylookups;		/* calls to lookup keys */
extern u_long	authnumkeys;		/* number of active keys */
extern u_long	authkeyexpired;		/* key lifetime expirations */
extern u_long	authkeyuncached;	/* cache misses */
extern u_long	authencryptions;	/* calls to encrypt */
extern u_long	authdecryptions;	/* calls to decrypt */

extern int	authnumfreekeys;

/*
 * The key cache. We cache the last key we looked at here.
 */
extern keyid_t	cache_keyid;		/* key identifier */
extern u_char *	cache_key;		/* key pointer */
extern u_int	cache_keylen;		/* key length */

/* getopt.c */
extern char *	ntp_optarg;		/* global argument pointer */
extern int	ntp_optind;		/* global argv index */

/* lib_strbuf.c */
extern int	ipv4_works;
extern int	ipv6_works;

/* machines.c */
typedef void (*pset_tod_using)(const char *);
extern pset_tod_using	set_tod_using;

/* ssl_init.c */
#ifdef OPENSSL
extern	void	ssl_init		(void);
extern	void	ssl_check_version	(void);
extern	int	ssl_init_done;
#define	INIT_SSL()				\
	do {					\
		if (!ssl_init_done)		\
			ssl_init();		\
	} while (0)
#else	/* !OPENSSL follows */
#define	INIT_SSL()		do {} while (0)
#endif
extern	int	keytype_from_text	(const char *,	size_t *);
extern	const char *keytype_name	(int);


/* lib/isc/win32/strerror.c
 *
 * To minimize Windows-specific changes to the rest of the NTP code,
 * particularly reference clocks, we hijack calls to strerror() to deal
 * with our mixture of error codes from the  C runtime (open, write)
 * and Windows (sockets, serial ports).  This is an ugly hack because
 * both use the lowest values differently, but particularly for ntpd,
 * it's not a problem.
 */
#ifdef NTP_REDEFINE_STRERROR
#define	strerror(e)	ntp_strerror(e)
extern char *	ntp_strerror	(int e);
#endif

/* systime.c */
extern double	sys_tick;		/* adjtime() resolution */

/* version.c */
extern const char *Version;		/* version declaration */

#endif	/* NTP_STDLIB_H */
