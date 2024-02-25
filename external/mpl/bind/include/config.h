/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* dnsrps $librpz_name */
#define DNSRPS_LIBRPZ_PATH "librpz.so"

/* 0=no DNSRPS 1=static link 2=dlopen() */
#define DNSRPS_LIB_OPEN 0

/* Define to enable "rrset-order fixed" syntax. */
#define DNS_RDATASET_FIXED 1

/* Define to enable American Fuzzy Lop test harness */
/* #undef ENABLE_AFL */

/* define if you want TCP_FASTOPEN enabled if available */
#define ENABLE_TCP_FASTOPEN 1

/* define if the ARM yield instruction is available */
/* #undef HAVE_ARM_YIELD */

/* Define to 1 if you have the `backtrace_symbols' function. */
#define HAVE_BACKTRACE_SYMBOLS 1

/* Define to 1 if you have the `BIO_read_ex' function. */
#define HAVE_BIO_READ_EX 1

/* Define to 1 if you have the `BIO_write_ex' function. */
#define HAVE_BIO_WRITE_EX 1

/* Define to 1 if you have the `BN_GENCB_new' function. */
#define HAVE_BN_GENCB_NEW 1

/* Define to 1 if the compiler supports __builtin_clz. */
#define HAVE_BUILTIN_CLZ 1

/* define if the compiler supports __builtin_*_overflow(). */
#define HAVE_BUILTIN_OVERFLOW 1

/* define if the compiler supports __builtin_unreachable(). */
#define HAVE_BUILTIN_UNREACHABLE 1

/* Define to 1 if you have the `chroot' function. */
#define HAVE_CHROOT 1

/* Define to 1 if you have the `clock_gettime' function. */
#define HAVE_CLOCK_GETTIME 1

/* Use CMocka */
/* #undef HAVE_CMOCKA */

/* Define to 1 if you have the `CRYPTO_zalloc' function. */
#define HAVE_CRYPTO_ZALLOC 1

/* Define to 1 if you have the declaration of `UV_UDP_MMSG_CHUNK', and to 0 if
   you don't. */
#define HAVE_DECL_UV_UDP_MMSG_CHUNK 1

/* Define to 1 if you have the declaration of `UV_UDP_MMSG_FREE', and to 0 if
   you don't. */
#define HAVE_DECL_UV_UDP_MMSG_FREE 1

/* Define to 1 if you have the declaration of `UV_UDP_RECVMMSG', and to 0 if
   you don't. */
#define HAVE_DECL_UV_UDP_RECVMMSG 1

/* Define to 1 if you have the `DH_get0_key' function. */
#define HAVE_DH_GET0_KEY 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 to enable dnstap support */
/* #undef HAVE_DNSTAP */

/* Define to 1 if you have the `ECDSA_SIG_get0' function. */
#define HAVE_ECDSA_SIG_GET0 1

/* Define to 1 if you have the `ERR_get_error_all' function. */
#define HAVE_ERR_GET_ERROR_ALL 1

/* Define to 1 if you have the `EVP_aes_128_ecb' function. */
#define HAVE_EVP_AES_128_ECB 1

/* Define to 1 if you have the `EVP_aes_192_ecb' function. */
#define HAVE_EVP_AES_192_ECB 1

/* Define to 1 if you have the `EVP_aes_256_ecb' function. */
#define HAVE_EVP_AES_256_ECB 1

/* Define to 1 if you have the `EVP_CIPHER_CTX_free' function. */
#define HAVE_EVP_CIPHER_CTX_FREE 1

/* Define to 1 if you have the `EVP_CIPHER_CTX_new' function. */
#define HAVE_EVP_CIPHER_CTX_NEW 1

/* Define to 1 if you have the `EVP_DigestSignInit' function. */
#define HAVE_EVP_DIGESTSIGNINIT 1

/* Define to 1 if you have the `EVP_DigestVerifyInit' function. */
#define HAVE_EVP_DIGESTVERIFYINIT 1

/* Define to 1 if you have the `EVP_MD_CTX_free' function. */
#define HAVE_EVP_MD_CTX_FREE 1

/* Define to 1 if you have the `EVP_MD_CTX_get0_md' function. */
#define HAVE_EVP_MD_CTX_GET0_MD 1

/* Define to 1 if you have the `EVP_MD_CTX_new' function. */
#define HAVE_EVP_MD_CTX_NEW 1

/* Define to 1 if you have the `EVP_MD_CTX_reset' function. */
#define HAVE_EVP_MD_CTX_RESET 1

/* Define to 1 if you have the `EVP_PKEY_eq' function. */
#define HAVE_EVP_PKEY_EQ 1

/* Define to 1 if you have the `EVP_PKEY_get0_EC_KEY' function. */
#define HAVE_EVP_PKEY_GET0_EC_KEY 1

/* Define to 1 if you have the `EVP_PKEY_new_raw_private_key' function. */
#define HAVE_EVP_PKEY_NEW_RAW_PRIVATE_KEY 1

/* Define to 1 if you have the `EVP_sha1' function. */
#define HAVE_EVP_SHA1 1

/* Define to 1 if you have the `EVP_sha224' function. */
#define HAVE_EVP_SHA224 1

/* Define to 1 if you have the `EVP_sha256' function. */
#define HAVE_EVP_SHA256 1

/* Define to 1 if you have the `EVP_sha384' function. */
#define HAVE_EVP_SHA384 1

/* Define to 1 if you have the `EVP_sha512' function. */
#define HAVE_EVP_SHA512 1

/* Define to 1 if you have the <execinfo.h> header file. */
#define HAVE_EXECINFO_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `FIPS_mode' function. */
/* #undef HAVE_FIPS_MODE */

/* Define to 1 if you have the `flockfile' function. */
#define HAVE_FLOCKFILE 1

/* Define to 1 if fseeko (and presumably ftello) exists and is declared. */
#define HAVE_FSEEKO 1

/* Define to 1 if the system has the `constructor' function attribute */
#define HAVE_FUNC_ATTRIBUTE_CONSTRUCTOR 1

/* Define to 1 if the system has the `destructor' function attribute */
#define HAVE_FUNC_ATTRIBUTE_DESTRUCTOR 1

/* Define to 1 if the system has the `malloc' function attribute */
#define HAVE_FUNC_ATTRIBUTE_MALLOC 1

/* Define to 1 if the system has the `noreturn' function attribute */
#define HAVE_FUNC_ATTRIBUTE_NORETURN 1

/* Define to 1 if the system has the `returns_nonnull' function attribute */
#define HAVE_FUNC_ATTRIBUTE_RETURNS_NONNULL 1

/* Build with GeoIP2 support */
/* #undef HAVE_GEOIP2 */

/* Define to 1 if you have the `getc_unlocked' function. */
#define HAVE_GETC_UNLOCKED 1

/* Define to 1 if you have the <glob.h> header file. */
#define HAVE_GLOB_H 1

/* Define to 1 if you have the Kerberos Framework available */
#define HAVE_GSSAPI 1

/* Define to 1 if you have the <gssapi/gssapi.h> header file. */
#define HAVE_GSSAPI_GSSAPI_H 1

/* Define to 1 if you have the <gssapi/gssapi_krb5.h> header file. */
#define HAVE_GSSAPI_GSSAPI_KRB5_H 1

/* Define to 1 if you have the <gssapi.h> header file. */
/* #undef HAVE_GSSAPI_H */

/* Define to 1 if you have the <gssapi_krb5.h> header file. */
/* #undef HAVE_GSSAPI_KRB5_H */

/* Define to 1 if you have the `gss_acquire_cred' function. */
#define HAVE_GSS_ACQUIRE_CRED 1

/* Define to 1 if you have the <idn2.h> header file. */
/* #undef HAVE_IDN2_H */

/* Define to 1 if you have the `if_nametoindex' function. */
#define HAVE_IF_NAMETOINDEX 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if jemalloc is available */
/* #undef HAVE_JEMALLOC */

/* Define to 1 if you have the <jemalloc/jemalloc.h> header file. */
/* #undef HAVE_JEMALLOC_JEMALLOC_H */

/* Use json-c library */
/* #undef HAVE_JSON_C */

/* Define to 1 if you have the <krb5.h> header file. */
/* #undef HAVE_KRB5_H */

/* Define to 1 if you have the `krb5_init_context' function. */
#define HAVE_KRB5_INIT_CONTEXT 1

/* Define to 1 if you have the <krb5/krb5.h> header file. */
#define HAVE_KRB5_KRB5_H 1

/* Define if libidn2 was found */
/* #undef HAVE_LIBIDN2 */

/* Build with DNS-over-HTTPS support */
/* #undef HAVE_LIBNGHTTP2 */

/* Use libxml2 library */
/* #undef HAVE_LIBXML2 */

/* Define to 1 if you have the <linux/netlink.h> header file. */
/* #undef HAVE_LINUX_NETLINK_H */

/* Define to 1 if you have the <linux/rtnetlink.h> header file. */
/* #undef HAVE_LINUX_RTNETLINK_H */

/* Use lmdb library */
/* #undef HAVE_LMDB */

/* define if extended attributes for malloc are available */
/* #undef HAVE_MALLOC_EXT_ATTR */

/* Define to 1 if you have the <malloc_np.h> header file. */
/* #undef HAVE_MALLOC_NP_H */

/* Define to 1 if you have the `malloc_size' function. */
/* #undef HAVE_MALLOC_SIZE */

/* Define to 1 if you have the `malloc_usable_size' function. */
/* #undef HAVE_MALLOC_USABLE_SIZE */

/* Define to 1 if you have the <minix/config.h> header file. */
/* #undef HAVE_MINIX_CONFIG_H */

/* Define to 1 if you have the <net/if6.h> header file. */
/* #undef HAVE_NET_IF6_H */

/* Define to 1 if you have the <net/route.h> header file. */
#define HAVE_NET_ROUTE_H 1

/* Define to 1 if you have the `OPENSSL_cleanup' function. */
#define HAVE_OPENSSL_CLEANUP 1

/* define if OpenSSL supports Ed25519 */
#define HAVE_OPENSSL_ED25519 1

/* define if OpenSSL supports Ed448 */
#define HAVE_OPENSSL_ED448 1

/* Define to 1 if you have the `OPENSSL_init_crypto' function. */
#define HAVE_OPENSSL_INIT_CRYPTO 1

/* Define to 1 if you have the `OPENSSL_init_ssl' function. */
#define HAVE_OPENSSL_INIT_SSL 1

/* Define if you have POSIX threads libraries and header files. */
#define HAVE_PTHREAD 1

/* Define to 1 if you have the `pthread_attr_getstacksize' function. */
#define HAVE_PTHREAD_ATTR_GETSTACKSIZE 1

/* Define to 1 if you have the `pthread_attr_setstacksize' function. */
#define HAVE_PTHREAD_ATTR_SETSTACKSIZE 1

/* Define to 1 if you have the `pthread_barrier_init' function. */
#define HAVE_PTHREAD_BARRIER_INIT 1

/* Support for PTHREAD_MUTEX_ADAPTIVE_NP */
/* #undef HAVE_PTHREAD_MUTEX_ADAPTIVE_NP */

/* Define to 1 if you have the <pthread_np.h> header file. */
/* #undef HAVE_PTHREAD_NP_H */

/* Have PTHREAD_PRIO_INHERIT. */
#define HAVE_PTHREAD_PRIO_INHERIT 1

/* Define to 1 if you have the `pthread_rwlock_rdlock' function. */
/* #undef HAVE_PTHREAD_RWLOCK_RDLOCK */

/* Define to 1 if you have the `pthread_setname_np' function. */
#define HAVE_PTHREAD_SETNAME_NP 1

/* Define to 1 if you have the `pthread_set_name_np' function. */
/* #undef HAVE_PTHREAD_SET_NAME_NP */

/* Define to 1 if you have the `pthread_yield' function. */
/* #undef HAVE_PTHREAD_YIELD */

/* Define to 1 if you have the `pthread_yield_np' function. */
/* #undef HAVE_PTHREAD_YIELD_NP */

/* Build with editline support */
/* #undef HAVE_READLINE_EDITLINE */

/* Build with libedit support */
/* #undef HAVE_READLINE_LIBEDIT */

/* Build with readline support */
#define HAVE_READLINE_READLINE 1

/* Define to 1 if you have the <regex.h> header file. */
#define HAVE_REGEX_H 1

/* Define to 1 if you have the `RSA_set0_key' function. */
#define HAVE_RSA_SET0_KEY 1

/* Define to 1 if you have the <sched.h> header file. */
#define HAVE_SCHED_H 1

/* Define to 1 if you have the `sched_yield' function. */
#define HAVE_SCHED_YIELD 1

/* Define to 1 if you have the `setegid' function. */
#define HAVE_SETEGID 1

/* Define to 1 if you have the `seteuid' function. */
#define HAVE_SETEUID 1

/* Define to 1 if you have the `setresgid' function. */
/* #undef HAVE_SETRESGID */

/* Define to 1 if you have the `setresuid' function. */
/* #undef HAVE_SETRESUID */

/* define if the SPARC pause instruction is available */
/* #undef HAVE_SPARC_PAUSE */

/* Define to 1 if you have the `SSL_CTX_set1_cert_store' function. */
#define HAVE_SSL_CTX_SET1_CERT_STORE 1

/* Define to 1 if you have the `SSL_CTX_set_keylog_callback' function. */
#define HAVE_SSL_CTX_SET_KEYLOG_CALLBACK 1

/* Define to 1 if you have the `SSL_CTX_set_min_proto_version' function. */
/* #undef HAVE_SSL_CTX_SET_MIN_PROTO_VERSION */

/* Define to 1 if you have the `SSL_CTX_up_ref' function. */
#define HAVE_SSL_CTX_UP_REF 1

/* Define to 1 if you have the `SSL_peek_ex' function. */
#define HAVE_SSL_PEEK_EX 1

/* Define to 1 if you have the `SSL_read_ex' function. */
#define HAVE_SSL_READ_EX 1

/* Define to 1 if you have the `SSL_SESSION_is_resumable' function. */
#define HAVE_SSL_SESSION_IS_RESUMABLE 1

/* Define to 1 if you have the `SSL_write_ex' function. */
#define HAVE_SSL_WRITE_EX 1

/* define if struct stat has st_mtim.tv_nsec field */
#define HAVE_STAT_NSEC 1

/* Define to 1 if you have the <stdalign.h> header file. */
#define HAVE_STDALIGN_H 1

/* Define to 1 if you have the <stdatomic.h> header file. */
//#ifndef __lint__
/* Gcc provides its own */
#define HAVE_STDATOMIC_H 1
//#endif

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <stdnoreturn.h> header file. */
#define HAVE_STDNORETURN_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strlcat' function. */
#define HAVE_STRLCAT 1

/* Define to 1 if you have the `strlcpy' function. */
#define HAVE_STRLCPY 1

/* Define to 1 if you have the `strnstr' function. */
#define HAVE_STRNSTR 1

/* Define to 1 if you have the `sysconf' function. */
#define HAVE_SYSCONF 1

/* Define to 1 if you have the `sysctlbyname' function. */
#define HAVE_SYSCTLBYNAME 1

/* Define to 1 if you have the <sys/capability.h> header file. */
/* #undef HAVE_SYS_CAPABILITY_H */

/* Define to 1 if you have the <sys/mman.h> header file. */
#define HAVE_SYS_MMAN_H 1

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/select.h> header file. */
#define HAVE_SYS_SELECT_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/sockio.h> header file. */
#define HAVE_SYS_SOCKIO_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/sysctl.h> header file. */
#define HAVE_SYS_SYSCTL_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <threads.h> header file. */
#define HAVE_THREADS_H 1

/* Define to 1 if you have the `TLS_client_method' function. */
#define HAVE_TLS_CLIENT_METHOD 1

/* Define to 1 if you have the `TLS_server_method' function. */
#define HAVE_TLS_SERVER_METHOD 1

/* Define to 1 if you have the `tzset' function. */
#define HAVE_TZSET 1

/* Define to 1 if you have the <uchar.h> header file. */
/* #undef HAVE_UCHAR_H */

/* Define to 1 if the system has the type `uintptr_t'. */
#define HAVE_UINTPTR_T 1

/* define if uname is available */
#define HAVE_UNAME 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <wchar.h> header file. */
#define HAVE_WCHAR_H 1

/* Define to 1 if you have the `X509_STORE_up_ref' function. */
#define HAVE_X509_STORE_UP_REF 1

/* Use zlib library */
#define HAVE_ZLIB 1

/* define if __atomic builtins are not available */
/* #undef HAVE___ATOMIC */

/* have __attribute__s used in librpz.h */
#define LIBRPZ_HAVE_ATTR 1

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Defined if extern char *optarg is not declared. */
/* #undef NEED_OPTARG */

/* Define if connect does not honour the permission on the UNIX domain socket.
   */
/* #undef NEED_SECURE_DIRECTORY */

/* Name of package */
#define PACKAGE "bind"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "https://gitlab.isc.org/isc-projects/bind9/-/issues/new?issuable_template=Bug"

/* make or Visual Studio */
#define PACKAGE_BUILDER "make"

/* Either 'defaults' or used ./configure options */
#define PACKAGE_CONFIGARGS "default"

/* An extra string to print after PACKAGE_STRING */
#define PACKAGE_DESCRIPTION " (Extended Support Version)"

/* Define to the full name of this package. */
#define PACKAGE_NAME "BIND"

/* A short hash from git */
#define PACKAGE_SRCID "6d7674f"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "BIND 9.18.24"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "bind"

/* Define to the home page for this package. */
#define PACKAGE_URL "https://www.isc.org/downloads/"

/* Define to the version of this package. */
#define PACKAGE_VERSION "9.18.24"

/* BIND 9 Extra part of the version */
#define PACKAGE_VERSION_EXTRA ""

/* BIND 9 Major part of the version */
#define PACKAGE_VERSION_MAJOR "9"

/* BIND 9 Minor part of the version */
#define PACKAGE_VERSION_MINOR "18"

/* BIND 9 Patch part of the version */
#define PACKAGE_VERSION_PATCH "24"

/* Define to necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* Define to 1 if all of the C90 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
#define STDC_HEADERS 1

/* If the compiler supports a TLS storage class, define it to that here */
#define TLS _Thread_local

/* Define to use default system tuning. */
#define TUNE_LARGE 1

/* Enable DNS Response Policy Service API */
/* #undef USE_DNSRPS */

/* Define if you want to use pthread rwlock implementation */
/* #undef USE_PTHREAD_RWLOCK */

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable general extensions on macOS.  */
#ifndef _DARWIN_C_SOURCE
# define _DARWIN_C_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable X/Open compliant socket functions that do not require linking
   with -lxnet on HP-UX 11.11.  */
#ifndef _HPUX_ALT_XOPEN_SOCKET_API
# define _HPUX_ALT_XOPEN_SOCKET_API 1
#endif
/* Identify the host operating system as Minix.
   This macro does not affect the system headers' behavior.
   A future release of Autoconf may stop defining this macro.  */
#ifndef _MINIX
/* # undef _MINIX */
#endif
/* Enable general extensions on NetBSD.
   Enable NetBSD compatibility extensions on Minix.  */
#ifndef _NETBSD_SOURCE
# define _NETBSD_SOURCE 1
#endif
/* Enable OpenBSD compatibility extensions on NetBSD.
   Oddly enough, this does nothing on OpenBSD.  */
#ifndef _OPENBSD_SOURCE
# define _OPENBSD_SOURCE 1
#endif
/* Define to 1 if needed for POSIX-compatible behavior.  */
#ifndef _POSIX_SOURCE
/* # undef _POSIX_SOURCE */
#endif
/* Define to 2 if needed for POSIX-compatible behavior.  */
#ifndef _POSIX_1_SOURCE
/* # undef _POSIX_1_SOURCE */
#endif
/* Enable POSIX-compatible threading on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-5:2014.  */
#ifndef __STDC_WANT_IEC_60559_ATTRIBS_EXT__
# define __STDC_WANT_IEC_60559_ATTRIBS_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-1:2014.  */
#ifndef __STDC_WANT_IEC_60559_BFP_EXT__
# define __STDC_WANT_IEC_60559_BFP_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-2:2015.  */
#ifndef __STDC_WANT_IEC_60559_DFP_EXT__
# define __STDC_WANT_IEC_60559_DFP_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-4:2015.  */
#ifndef __STDC_WANT_IEC_60559_FUNCS_EXT__
# define __STDC_WANT_IEC_60559_FUNCS_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-3:2015.  */
#ifndef __STDC_WANT_IEC_60559_TYPES_EXT__
# define __STDC_WANT_IEC_60559_TYPES_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TR 24731-2:2010.  */
#ifndef __STDC_WANT_LIB_EXT2__
# define __STDC_WANT_LIB_EXT2__ 1
#endif
/* Enable extensions specified by ISO/IEC 24747:2009.  */
#ifndef __STDC_WANT_MATH_SPEC_FUNCS__
# define __STDC_WANT_MATH_SPEC_FUNCS__ 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable X/Open extensions.  Define to 500 only if necessary
   to make mbstate_t available.  */
#ifndef _XOPEN_SOURCE
/* # undef _XOPEN_SOURCE */
#endif


/* the default value of dnssec-validation option */
#define VALIDATION_DEFAULT "auto"

/* Version number of package */
#define VERSION "9.18.24"

/* Define to enable very verbose query trace logging. */
#define WANT_QUERYTRACE 1

/* Define to enable single-query tracing. */
/* #undef WANT_SINGLETRACE */

#ifndef __NetBSD__
/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif
#endif

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define to 1 to make fseeko visible on some hosts (e.g. glibc 2.2). */
/* #undef _LARGEFILE_SOURCE */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Select RFC3542 IPv6 API on macOS */
#define __APPLE_USE_RFC_3542 1

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef ssize_t */

/* Define if the compiler uses a different keyword than thread_local for TLS
   variables */
#define thread_local _Thread_local

/* Define to the type of an unsigned integer type wide enough to hold a
   pointer, if such a type exists, and if the system does not define it. */
/* #undef uintptr_t */

/* Define to empty if the keyword `volatile' does not work. Warning: valid
   code using `volatile' can become incorrect without. Disable with care. */
/* #undef volatile */
