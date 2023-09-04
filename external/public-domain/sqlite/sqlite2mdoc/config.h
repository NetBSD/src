#ifndef OCONFIGURE_CONFIG_H
#define OCONFIGURE_CONFIG_H

#ifdef __cplusplus
# error "Do not use C++: this is a C application."
#endif
#if !defined(__GNUC__) || (__GNUC__ < 4)
# define __attribute__(x)
#endif
#if defined(__linux__) || defined(__MINT__) || defined(__wasi__)
# define _GNU_SOURCE /* memmem, memrchr, setresuid... */
# define _DEFAULT_SOURCE /* le32toh, crypt, ... */
#endif
#if defined(__NetBSD__)
# define _OPENBSD_SOURCE /* reallocarray, etc. */
#endif
#if defined(__sun)
# ifndef _XOPEN_SOURCE /* SunOS already defines */
#  define _XOPEN_SOURCE /* XPGx */
# endif
# define _XOPEN_SOURCE_EXTENDED 1 /* XPG4v2 */
# ifndef __EXTENSIONS__ /* SunOS already defines */
#  define __EXTENSIONS__ /* reallocarray, etc. */
# endif
#endif
#if !defined(__BEGIN_DECLS)
# define __BEGIN_DECLS
#endif
#if !defined(__END_DECLS)
# define __END_DECLS
#endif

#include <sys/types.h> /* size_t, mode_t, dev_t */ 

#include <stdint.h> /* C99 [u]int[nn]_t types */

/*
 * Results of configuration feature-testing.
 */
#define HAVE_ARC4RANDOM 1
#define HAVE_B64_NTOP 1
#define HAVE_CAPSICUM 0
#define HAVE_CRYPT 1
#define HAVE_CRYPT_NEWHASH 0
#define HAVE_ENDIAN_H 1
#define HAVE_ERR 1
#define HAVE_EXPLICIT_BZERO 0
#define HAVE_FTS 1
#define HAVE_GETEXECNAME 0
#define HAVE_GETPROGNAME 1
#define HAVE_INFTIM 1
#define HAVE_LANDLOCK 0
#define HAVE_MD5 1
#define HAVE_MEMMEM 1
#define HAVE_MEMRCHR 1
#define HAVE_MEMSET_S 0
#define HAVE_MKFIFOAT 1
#define HAVE_MKNODAT 1
#define HAVE_OSBYTEORDER_H 0
#define HAVE_PATH_MAX 1
#define HAVE_PLEDGE 0
#define HAVE_PROGRAM_INVOCATION_SHORT_NAME 0
#define HAVE_READPASSPHRASE 0
#define HAVE_REALLOCARRAY 1
#define HAVE_RECALLOCARRAY 0
#define HAVE_SANDBOX_INIT 0
#define HAVE_SCAN_SCALED 0
#define HAVE_SECCOMP_FILTER 0
#define HAVE_SETRESGID 0
#define HAVE_SETRESUID 0
#define HAVE_SHA2 0
#define HAVE_SHA2_H 0
#define HAVE_SOCK_NONBLOCK 1
#define HAVE_STRLCAT 1
#define HAVE_STRLCPY 1
#define HAVE_STRNDUP 1
#define HAVE_STRNLEN 1
#define HAVE_STRTONUM 1
#define HAVE_SYS_BYTEORDER_H 0
#define HAVE_SYS_ENDIAN_H 1
#define HAVE_SYS_MKDEV_H 0
#define HAVE_SYS_QUEUE 1
#define HAVE_SYS_SYSMACROS_H 0
#define HAVE_SYS_TREE 1
#define HAVE_SYSTRACE 0
#define HAVE_UNVEIL 0
#define HAVE_TERMIOS 1
#define HAVE_WAIT_ANY 1
#define HAVE___PROGNAME 1

/*
 * Handle the various major()/minor() header files.
 * Use sys/mkdev.h before sys/sysmacros.h because SunOS
 * has both, where only the former works properly.
 */
#if HAVE_SYS_MKDEV_H
# define COMPAT_MAJOR_MINOR_H <sys/mkdev.h>
#elif HAVE_SYS_SYSMACROS_H
# define COMPAT_MAJOR_MINOR_H <sys/sysmacros.h>
#else
# define COMPAT_MAJOR_MINOR_H <sys/types.h>
#endif

/*
 * Make it easier to include endian.h forms.
 */
#if HAVE_ENDIAN_H
# define COMPAT_ENDIAN_H <endian.h>
#elif HAVE_SYS_ENDIAN_H
# define COMPAT_ENDIAN_H <sys/endian.h>
#elif HAVE_OSBYTEORDER_H
# define COMPAT_ENDIAN_H <libkern/OSByteOrder.h>
#elif HAVE_SYS_BYTEORDER_H
# define COMPAT_ENDIAN_H <sys/byteorder.h>
#else
# warning No suitable endian.h could be found.
# warning Please e-mail the maintainers with your OS.
# define COMPAT_ENDIAN_H <endian.h>
#endif

/*
 * Compatibility for sha2(3).
 */

/*** SHA-256/384/512 Various Length Definitions ***********************/
#define SHA256_BLOCK_LENGTH		64
#define SHA256_DIGEST_LENGTH		32
#define SHA256_DIGEST_STRING_LENGTH	(SHA256_DIGEST_LENGTH * 2 + 1)
#define SHA384_BLOCK_LENGTH		128
#define SHA384_DIGEST_LENGTH		48
#define SHA384_DIGEST_STRING_LENGTH	(SHA384_DIGEST_LENGTH * 2 + 1)
#define SHA512_BLOCK_LENGTH		128
#define SHA512_DIGEST_LENGTH		64
#define SHA512_DIGEST_STRING_LENGTH	(SHA512_DIGEST_LENGTH * 2 + 1)
#define SHA512_256_BLOCK_LENGTH		128
#define SHA512_256_DIGEST_LENGTH	32
#define SHA512_256_DIGEST_STRING_LENGTH	(SHA512_256_DIGEST_LENGTH * 2 + 1)

/*** SHA-224/256/384/512 Context Structure *******************************/
typedef struct _SHA2_CTX {
	union {
		uint32_t	st32[8];
		uint64_t	st64[8];
	} state;
	uint64_t	bitcount[2];
	uint8_t		buffer[SHA512_BLOCK_LENGTH];
} SHA2_CTX;

void SHA256Init(SHA2_CTX *);
void SHA256Transform(uint32_t state[8], const uint8_t [SHA256_BLOCK_LENGTH]);
void SHA256Update(SHA2_CTX *, const uint8_t *, size_t);
void SHA256Pad(SHA2_CTX *);
void SHA256Final(uint8_t [SHA256_DIGEST_LENGTH], SHA2_CTX *);
char *SHA256End(SHA2_CTX *, char *);
char *SHA256File(const char *, char *);
char *SHA256FileChunk(const char *, char *, off_t, off_t);
char *SHA256Data(const uint8_t *, size_t, char *);

void SHA384Init(SHA2_CTX *);
void SHA384Transform(uint64_t state[8], const uint8_t [SHA384_BLOCK_LENGTH]);
void SHA384Update(SHA2_CTX *, const uint8_t *, size_t);
void SHA384Pad(SHA2_CTX *);
void SHA384Final(uint8_t [SHA384_DIGEST_LENGTH], SHA2_CTX *);
char *SHA384End(SHA2_CTX *, char *);
char *SHA384File(const char *, char *);
char *SHA384FileChunk(const char *, char *, off_t, off_t);
char *SHA384Data(const uint8_t *, size_t, char *);

void SHA512Init(SHA2_CTX *);
void SHA512Transform(uint64_t state[8], const uint8_t [SHA512_BLOCK_LENGTH]);
void SHA512Update(SHA2_CTX *, const uint8_t *, size_t);
void SHA512Pad(SHA2_CTX *);
void SHA512Final(uint8_t [SHA512_DIGEST_LENGTH], SHA2_CTX *);
char *SHA512End(SHA2_CTX *, char *);
char *SHA512File(const char *, char *);
char *SHA512FileChunk(const char *, char *, off_t, off_t);
char *SHA512Data(const uint8_t *, size_t, char *);

#define	FMT_SCALED_STRSIZE	7 /* minus sign, 4 digits, suffix, null byte */
int	fmt_scaled(long long, char *);
int	scan_scaled(char *, long long *);
/*
 * Compatibility for explicit_bzero(3).
 */
extern void explicit_bzero(void *, size_t);

/*
 * Macros and function required for readpassphrase(3).
 */
#define RPP_ECHO_OFF 0x00
#define RPP_ECHO_ON 0x01
#define RPP_REQUIRE_TTY 0x02
#define RPP_FORCELOWER 0x04
#define RPP_FORCEUPPER 0x08
#define RPP_SEVENBIT 0x10
#define RPP_STDIN 0x20
char *readpassphrase(const char *, char *, size_t, int);

/*
 * Compatibility for recallocarray(3).
 */
extern void *recallocarray(void *, size_t, size_t, size_t);

/*
 * Compatibility for setresgid(2).
 */
int setresgid(gid_t rgid, gid_t egid, gid_t sgid);

/*
 * Compatibility for setresuid(2).
 */
int setresuid(uid_t ruid, uid_t euid, uid_t suid);

#endif /*!OCONFIGURE_CONFIG_H*/
