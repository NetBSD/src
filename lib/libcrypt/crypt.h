/*
 * $NetBSD: crypt.h,v 1.8 2021/10/16 10:53:33 nia Exp $
 */

#define crypt_private     __attribute__((__visibility__("hidden")))

crypt_private char *__md5crypt(const char *, const char *);	/* XXX */
crypt_private char *__bcrypt(const char *, const char *);	/* XXX */
crypt_private char *__crypt_sha1(const char *, const char *);
crypt_private unsigned int __crypt_sha1_iterations (unsigned int);
crypt_private void __hmac_sha1(const unsigned char *, size_t,
    const unsigned char *, size_t, unsigned char *);

#ifdef HAVE_ARGON2
crypt_private char *__crypt_argon2(const char *, const char *);
crypt_private int __gensalt_argon2id(char *, size_t, const char *);
crypt_private int __gensalt_argon2i(char *, size_t, const char *);
crypt_private int __gensalt_argon2d(char *, size_t, const char *);
#endif /* HAVE_ARGON2 */

crypt_private int __gensalt_blowfish(char *, size_t, const char *);
crypt_private int __gensalt_old(char *, size_t, const char *);
crypt_private int __gensalt_new(char *, size_t, const char *);
crypt_private int __gensalt_md5(char *, size_t, const char *);
crypt_private int __gensalt_sha1(char *, size_t, const char *);

crypt_private int getnum(const char *, size_t *);
crypt_private void __crypt_to64(char *, uint32_t, int);
crypt_private void __crypt_tobase64(char *, uint32_t, int);

#define SHA1_MAGIC "$sha1$"
#define SHA1_SIZE 20
