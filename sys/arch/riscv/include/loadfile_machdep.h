/*	$NetBSD: loadfile_machdep.h,v 1.1.4.2 2021/05/13 00:47:27 thorpej Exp $	*/

#ifdef _LP64
#define BOOT_ELF64
#else
#define BOOT_ELF32
#endif

#define LOAD_KERNEL	(LOAD_ALL & ~LOAD_TEXTA)
#define COUNT_KERNEL	(COUNT_ALL & ~COUNT_TEXTA)

#define LOADADDR(a)		(((u_long)(a)))
#define ALIGNENTRY(a)		((u_long)(a))
#define READ(f, b, c)		read((f), (void*)LOADADDR(b), (c))
#define BCOPY(s, d, c)		memmove((void*)LOADADDR(d), (void*)(s), (c))
#define BZERO(d, c)		memset((void*)LOADADDR(d), 0, (c))
#define	WARN(a)			do { \
					(void)printf a; \
					if (errno) \
						(void)printf(": %s\n", \
						             strerror(errno)); \
					else \
						(void)printf("\n"); \
				} while(/* CONSTCOND */0)
#ifdef PROGRESS_FN
void PROGRESS_FN(const char *, ...) __printflike(1, 2);
#define PROGRESS(a)		PROGRESS_FN a
#else
#define PROGRESS(a)		(void)printf a
#endif
#define ALLOC(a)		alloc(a)
#define DEALLOC(a, b)		dealloc(a, b)
#define OKMAGIC(a)		((a) == ZMAGIC)
