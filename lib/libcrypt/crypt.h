/*
 * $NetBSD: crypt.h,v 1.1 2004/07/02 00:05:23 sjg Exp $
 */
char	*__md5crypt(const char *pw, const char *salt);	/* XXX */
char *__bcrypt(const char *, const char *);	/* XXX */
char *__crypt_sha1(const char *pw, const char *salt);
unsigned int __crypt_sha1_iterations (unsigned int hint);
void __hmac_sha1(unsigned char *, size_t, unsigned char *, size_t, unsigned char *);
void __crypt_to64(char *s, u_int32_t v, int n);

#define SHA1_MAGIC "$sha1$"
#define SHA1_SIZE 20

#ifdef __GNUC__
#define UNCONST(ptr)	({ 		\
    union __unconst {			\
	const void *__cp;		\
	void *__p;			\
    } __d;				\
    __d.__cp = ptr, __d.__p; })
#else
#define UNCONST(ptr)	(void *)(ptr)
#endif
