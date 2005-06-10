/*	$NetBSD: sha1.h,v 1.5.2.1 2005/06/10 14:38:35 tron Exp $	*/

/*
 * SHA-1 in C
 * By Steve Reid <steve@edmweb.com>
 * 100% Public Domain
 */

#ifndef _SYS_SHA1_H_
#define	_SYS_SHA1_H_

#include <sys/types.h>

#define SHA1_DIGEST_LENGTH 20

typedef struct {
	u_int32_t state[5];
	u_int32_t count[2];
	u_char buffer[64];
} SHA1_CTX;

void	SHA1Transform(u_int32_t[5], const u_char[64]);
void	SHA1Init(SHA1_CTX *);
void	SHA1Update(SHA1_CTX *, const u_char *, u_int);
void	SHA1Final(u_char[SHA1_DIGEST_LENGTH], SHA1_CTX *);
#ifndef _KERNEL
char	*SHA1End(SHA1_CTX *, char *);
char	*SHA1File(char *, char *);
char	*SHA1Data(const u_char *, size_t, char *);
#endif /* _KERNEL */

#endif /* _SYS_SHA1_H_ */
