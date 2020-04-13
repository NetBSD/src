/*
 * $NetBSD: crypt.h,v 1.4.82.1 2020/04/13 08:03:12 martin Exp $
 */
char	*__md5crypt(const char *pw, const char *salt);	/* XXX */
char *__bcrypt(const char *, const char *);	/* XXX */
char *__crypt_sha1(const char *pw, const char *salt);
unsigned int __crypt_sha1_iterations (unsigned int hint);
void __hmac_sha1(const unsigned char *, size_t, const unsigned char *, size_t,
		 unsigned char *);
void __crypt_to64(char *s, u_int32_t v, int n);

#ifdef HAVE_ARGON2
char *__crypt_argon2(const char *pw, const char *salt);
int __gensalt_argon2id(char *salt, size_t saltsiz, const char *option);
int __gensalt_argon2i(char *salt, size_t saltsiz, const char *option);
int __gensalt_argon2d(char *salt, size_t saltsiz, const char *option);
#endif /* HAVE_ARGON2 */

int __gensalt_blowfish(char *salt, size_t saltlen, const char *option);
int __gensalt_old(char *salt, size_t saltsiz, const char *option);
int __gensalt_new(char *salt, size_t saltsiz, const char *option);
int __gensalt_md5(char *salt, size_t saltsiz, const char *option);
int __gensalt_sha1(char *salt, size_t saltsiz, const char *option);

#define SHA1_MAGIC "$sha1$"
#define SHA1_SIZE 20
