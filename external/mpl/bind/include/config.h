/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define if you cannot bind() before connect() for TCP sockets. */
/* #undef BROKEN_TCP_BIND_BEFORE_CONNECT */

/* dnsrps $librpz_name */
#define DNSRPS_LIBRPZ_PATH "librpz.so"

/* 0=no DNSRPS 1=static link 2=dlopen() */
#define DNSRPS_LIB_OPEN 2

/* Define to enable "rrset-order fixed" syntax. */
#define DNS_RDATASET_FIXED 1

/* Define to enable American Fuzzy Lop test harness */
/* #undef ENABLE_AFL */

/* define if you want TCP_FASTOPEN enabled if available */
#define ENABLE_TCP_FASTOPEN 1

/* Solaris hack to get select_large_fdset. */
/* #undef FD_SETSIZE */

/* Define to nothing if C supports flexible array members, and to 1 if it does
   not. That way, with a declaration like `struct s { int n; double
   d[FLEXIBLE_ARRAY_MEMBER]; };', the struct hack can be used with pre-C99
   compilers. When computing the size of such an object, don't use 'sizeof
   (struct s)' as it overestimates the size. Use 'offsetof (struct s, d)'
   instead. Don't use 'offsetof (struct s, d[0])', as this doesn't work with
   MSVC and with C++ compilers. */
#define FLEXIBLE_ARRAY_MEMBER /**/

/* Define to 1 if you have the `arc4random' function. */
#define HAVE_ARC4RANDOM 1

/* Define to 1 if you have the `arc4random_buf' function. */
#define HAVE_ARC4RANDOM_BUF 1

/* Define to 1 if you have the `arc4random_uniform' function. */
#define HAVE_ARC4RANDOM_UNIFORM 1

/* define if the ARM yield instruction is available */
/* #undef HAVE_ARM_YIELD */

/* Define to 1 if you have the `BIO_read_ex' function. */
#define HAVE_BIO_READ_EX 1

/* Define to 1 if you have the `BIO_write_ex' function. */
#define HAVE_BIO_WRITE_EX 1

/* Define to 1 if you have the `BN_GENCB_new' function. */
#define HAVE_BN_GENCB_NEW 1

/* Define to 1 if the compiler supports __builtin_clz. */
#define HAVE_BUILTIN_CLZ 1

/* Define to 1 if the compiler supports __builtin_expect. */
#define HAVE_BUILTIN_EXPECT 1

/* define if the compiler supports __builtin_unreachable(). */
#define HAVE_BUILTIN_UNREACHABLE 1

/* Define to 1 if you have the `chroot' function. */
#define HAVE_CHROOT 1

/* Define if clock_gettime is available. */
#define HAVE_CLOCK_GETTIME 1

/* Use CMocka */
/* #undef HAVE_CMOCKA */

/* Define to 1 if you have the `cpuset_setaffinity' function. */
/* #undef HAVE_CPUSET_SETAFFINITY */

/* Define to 1 if you have the `cpuset_setaffinity' function. */
/* #undef HAVE_CPUSET_SETAFFINITY */

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

/* Define to 1 if you have the <devpoll.h> header file. */
/* #undef HAVE_DEVPOLL_H */

/* Define to 1 if you have the `DH_get0_key' function. */
#define HAVE_DH_GET0_KEY 1

/* Define to 1 if you have the `dlclose' function. */
#define HAVE_DLCLOSE 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the `dlopen' function. */
#define HAVE_DLOPEN 1

/* Define to 1 if you have the `dlsym' function. */
#define HAVE_DLSYM 1

/* Define to 1 to enable dnstap support */
/* #undef HAVE_DNSTAP */

/* Define to 1 if you have the `ECDSA_sign' function. */
#define HAVE_ECDSA_SIGN 1

/* Define to 1 if you have the `ECDSA_SIG_get0' function. */
#define HAVE_ECDSA_SIG_GET0 1

/* Define to 1 if you have the `ECDSA_verify' function. */
#define HAVE_ECDSA_VERIFY 1

/* Define to 1 if you have the <editline/readline.h> header file. */
/* #undef HAVE_EDITLINE_READLINE_H */

/* Define to 1 if you have the <edit/readline/history.h> header file. */
/* #undef HAVE_EDIT_READLINE_HISTORY_H */

/* Define to 1 if you have the <edit/readline/readline.h> header file. */
/* #undef HAVE_EDIT_READLINE_READLINE_H */

/* Define to 1 if you have the `epoll_create1' function. */
/* #undef HAVE_EPOLL_CREATE1 */

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

/* Define to 1 if you have the `EVP_MD_CTX_free' function. */
#define HAVE_EVP_MD_CTX_FREE 1

/* Define to 1 if you have the `EVP_MD_CTX_new' function. */
#define HAVE_EVP_MD_CTX_NEW 1

/* Define to 1 if you have the `EVP_MD_CTX_reset' function. */
#define HAVE_EVP_MD_CTX_RESET 1

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

/* Define to 1 if the system has the `returns_nonnull' function attribute */
#define HAVE_FUNC_ATTRIBUTE_RETURNS_NONNULL 1

/* Build with GeoIP2 support */
/* #undef HAVE_GEOIP2 */

/* Define to 1 if you have the `getc_unlocked' function. */
#define HAVE_GETC_UNLOCKED 1

/* Define to 1 if you have the `getpassphrase' function. */
/* #undef HAVE_GETPASSPHRASE */

/* Define to 1 if you have the `getrandom' function. */
#define HAVE_GETRANDOM 1

/* Define to use gperftools CPU profiler. */
/* #undef HAVE_GPERFTOOLS_PROFILER */

/* Define to 1 if you have the <gssapi/gssapi.h> header file. */
#define HAVE_GSSAPI_GSSAPI_H 1

/* Define to 1 if you have the <gssapi/gssapi_krb5.h> header file. */
#define HAVE_GSSAPI_GSSAPI_KRB5_H 1

/* Define to 1 if you have the <gssapi.h> header file. */
#define HAVE_GSSAPI_H 1

/* Define to 1 if you have the <gssapi_krb5.h> header file. */
/* #undef HAVE_GSSAPI_KRB5_H */

/* Define to 1 if you have the `HMAC_CTX_free' function. */
#define HAVE_HMAC_CTX_FREE 1

/* Define to 1 if you have the `HMAC_CTX_get_md' function. */
#define HAVE_HMAC_CTX_GET_MD 1

/* Define to 1 if you have the `HMAC_CTX_new' function. */
#define HAVE_HMAC_CTX_NEW 1

/* Define to 1 if you have the `HMAC_CTX_reset' function. */
#define HAVE_HMAC_CTX_RESET 1

/* Define to 1 if you have the <idn2.h> header file. */
/* #undef HAVE_IDN2_H */

/* Define to 1 if you have the `if_nametoindex' function. */
#define HAVE_IF_NAMETOINDEX 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Use json-c library */
/* #undef HAVE_JSON_C */

/* Define to 1 if you have the <kerberosv5/krb5.h> header file. */
/* #undef HAVE_KERBEROSV5_KRB5_H */

/* Define to 1 if you have the `kqueue' function. */
#define HAVE_KQUEUE 1

/* Define to 1 if you have the <krb5.h> header file. */
#define HAVE_KRB5_H 1

/* Define to 1 if you have the <krb5/krb5.h> header file. */
#define HAVE_KRB5_KRB5_H 1

/* define if system have backtrace function */
#define HAVE_LIBCTRACE 1

/* Define if libidn2 was found */
/* #undef HAVE_LIBIDN2 */

/* Define to 1 if you have the `nsl' library (-lnsl). */
/* #undef HAVE_LIBNSL */

/* Define to 1 if you have the `scf' library (-lscf). */
/* #undef HAVE_LIBSCF */

/* Define to 1 if you have the `socket' library (-lsocket). */
/* #undef HAVE_LIBSOCKET */

/* Use libxml2 library */
/* #undef HAVE_LIBXML2 */

/* Define to 1 if you have the <linux/netlink.h> header file. */
/* #undef HAVE_LINUX_NETLINK_H */

/* Define to 1 if you have the <linux/rtnetlink.h> header file. */
/* #undef HAVE_LINUX_RTNETLINK_H */

/* Define if lmdb was found */
/* #undef HAVE_LMDB */

/* Define to 1 if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1

/* define if extended attributes for malloc are available */
/* #undef HAVE_MALLOC_EXT_ATTR */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mmap' function. */
#define HAVE_MMAP 1

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

/* Define to 1 if you have the `readline' function. */
#define HAVE_READLINE 1

/* Define to 1 if you have the <readline/history.h> header file. */
#define HAVE_READLINE_HISTORY_H 1

/* Define to 1 if you have the <readline/readline.h> header file. */
#define HAVE_READLINE_READLINE_H 1

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

/* Define to 1 if you have the `setlocale' function. */
#define HAVE_SETLOCALE 1

/* Define to 1 if you have the `setresgid' function. */
/* #undef HAVE_SETRESGID */

/* Define to 1 if you have the `setresuid' function. */
/* #undef HAVE_SETRESUID */

/* define if the SPARC pause instruction is available */
/* #undef HAVE_SPARC_PAUSE */

/* Define to 1 if you have the `SSL_CTX_set_min_proto_version' function. */
/* #undef HAVE_SSL_CTX_SET_MIN_PROTO_VERSION */

/* Define to 1 if you have the `SSL_CTX_up_ref' function. */
#define HAVE_SSL_CTX_UP_REF 1

/* Define to 1 if you have the `SSL_peek_ex' function. */
#define HAVE_SSL_PEEK_EX 1

/* Define to 1 if you have the `SSL_read_ex' function. */
#define HAVE_SSL_READ_EX 1

/* Define to 1 if you have the `SSL_write_ex' function. */
#define HAVE_SSL_WRITE_EX 1

/* define if struct stat has st_mtim.tv_nsec field */
/* #undef HAVE_STAT_NSEC */

/* Define to 1 if you have the <stdalign.h> header file. */
#define HAVE_STDALIGN_H 1

/* Define to 1 if you have the <stdatomic.h> header file. */
#ifndef __lint__
/* Gcc provides its own */
#define HAVE_STDATOMIC_H 1
#endif

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

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

/* Define to 1 if you have the <sys/devpoll.h> header file. */
/* #undef HAVE_SYS_DEVPOLL_H */

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

/* Define to 1 if you have the <sys/un.h> header file. */
#define HAVE_SYS_UN_H 1

/* Define to 1 if you have the <threads.h> header file. */
#define HAVE_THREADS_H 1

/* Define if thread_local keyword is available */
#define HAVE_THREAD_LOCAL 1

/* Define if Thread-Local Storage is available */
#define HAVE_TLS 1

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

/* define if the compiler supports _Unwind_Backtrace() */
#define HAVE_UNWIND_BACKTRACE 1

/* Use zlib library */
#define HAVE_ZLIB 1

/* define if __atomic builtins are not available */
/* #undef HAVE___ATOMIC */

/* Define if __thread keyword is available */
/* #undef HAVE___THREAD */

/* Define if you want to use inline buffers */
#define ISC_BUFFER_USEINLINE 1

/* Define to allow building of objects for dlopen(). */
#define ISC_DLZ_DLOPEN 1

/* define if the linker supports --wrap option */
#define LD_WRAP 1

/* have __attribute__s used in librpz.h */
#define LIBRPZ_HAVE_ATTR 1

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Defined if extern char *optarg is not declared. */
/* #undef NEED_OPTARG */

/* Define if connect does not honour the permission on the UNIX domain socket.
   */
/* #undef NEED_SECURE_DIRECTORY */

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "https://gitlab.isc.org/isc-projects/bind9/-/issues/new?issuable_template=Bug"

/* Define to the full name of this package. */
#define PACKAGE_NAME "BIND"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "BIND 9.16"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "bind"

/* Define to the home page for this package. */
#define PACKAGE_URL "https://www.isc.org/downloads/"

/* Define to the version of this package. */
#define PACKAGE_VERSION "9.16"

/* define the default PKCS11 library path */
#define PK11_LIB_LOCATION "undefined"

/* Sets which flag to pass to open/fcntl to make non-blocking
   (O_NDELAY/O_NONBLOCK). */
#define PORT_NONBLOCK O_NONBLOCK

/* Define to necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* Exit code for skipped tests */
#define SKIPPED_TEST_EXIT_CODE 0

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Define to use default system tuning. */
#define TUNE_LARGE 1

/* define if we can use backtrace */
#define USE_BACKTRACE 1

/* Enable DNS Response Policy Service API */
#define USE_DNSRPS 1

/* Defined if you need to use ioctl(FIONBIO) instead a fcntl call to make
   non-blocking. */
/* #undef USE_FIONBIO_IOCTL */

/* Define if libtool is used for compilation */
/* #undef USE_LIBTOOL */

/* define if OpenSSL is used for Public-Key Cryptography */
#define USE_OPENSSL 1

/* define if PKCS11 is used for Public-Key Cryptography */
/* #undef USE_PKCS11 */

/* Define if you want to use pthread rwlock implementation */
/* #undef USE_PTHREAD_RWLOCK */

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif


/* the default value of dnssec-validation option */
#define VALIDATION_DEFAULT "auto"

/* Define to enable very verbose query trace logging. */
#define WANT_QUERYTRACE 1

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

/* Enable large inode numbers on Mac OS X 10.5.  */
#ifndef _DARWIN_USE_64_BIT_INODE
# define _DARWIN_USE_64_BIT_INODE 1
#endif

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define to 1 to make fseeko visible on some hosts (e.g. glibc 2.2). */
/* #undef _LARGEFILE_SOURCE */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to 1 if on MINIX. */
/* #undef _MINIX */

/* Define to 2 if the system does not provide POSIX.1 features except with
   this defined. */
/* #undef _POSIX_1_SOURCE */

/* Define to 1 if you need to in order for `stat' and other things to work. */
/* #undef _POSIX_SOURCE */

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

/* Define to the type of an unsigned integer type wide enough to hold a
   pointer, if such a type exists, and if the system does not define it. */
/* #undef uintptr_t */

/* Define to empty if the keyword `volatile' does not work. Warning: valid
   code using `volatile' can become incorrect without. Disable with care. */
/* #undef volatile */
