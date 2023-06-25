/* nbtool_config.h.  Generated from nbtool_config.h.in by configure.  */
/* nbtool_config.h.in.  Generated from configure.ac by autoheader.  */

/*      $NetBSD: nbtool_config.h.in,v 1.53 2021/02/25 13:41:58 christos Exp $    */
 
#ifndef __NETBSD_NBTOOL_CONFIG_H__
#define __NETBSD_NBTOOL_CONFIG_H__

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define to 1 if your `fparseln' function is broken. */
#define BROKEN_FPARSELN 1

/* Define to one of `_getb67', `GETB67', `getb67' for Cray-2 and Cray-YMP
   systems. This function is required for `alloca.c' support on those systems.
   */
/* #undef CRAY_STACKSEG_END */

/* Define to 1 if using `alloca.c'. */
/* #undef C_ALLOCA */

/* Define to 1 if you have `alloca', as a function or macro. */
#define HAVE_ALLOCA 1

/* Define to 1 if you have <alloca.h> and it should be used (not on Ultrix).
   */
#define HAVE_ALLOCA_H 1

/* Define to 1 if you have the <arpa/nameser.h> header file. */
#define HAVE_ARPA_NAMESER_H 1

/* Define to 1 if you have the `asnprintf' function. */
/* #undef HAVE_ASNPRINTF */

/* Define to 1 if you have the `asprintf' function. */
#define HAVE_ASPRINTF 1

/* Define to 1 if you have the <assert.h> header file. */
#define HAVE_ASSERT_H 1

/* Define to 1 if you have the `atoll' function. */
#define HAVE_ATOLL 1

/* Define to 1 if you have the `basename' function. */
#define HAVE_BASENAME 1

/* Define to 1 if you have the <ctype.h> header file. */
#define HAVE_CTYPE_H 1

/* Define to 1 if you have the declaration of `asnprintf', and to 0 if you
   don't. */
#define HAVE_DECL_ASNPRINTF 0

/* Define to 1 if you have the declaration of `asprintf', and to 0 if you
   don't. */
#define HAVE_DECL_ASPRINTF 1

/* Define to 1 if you have the declaration of `atoll', and to 0 if you don't.
   */
#define HAVE_DECL_ATOLL 1

/* Define to 1 if you have the declaration of `basename', and to 0 if you
   don't. */
#define HAVE_DECL_BASENAME 1

/* Define to 1 if you have the declaration of `be16dec', and to 0 if you
   don't. */
#define HAVE_DECL_BE16DEC 0

/* Define to 1 if you have the declaration of `be16enc', and to 0 if you
   don't. */
#define HAVE_DECL_BE16ENC 0

/* Define to 1 if you have the declaration of `be16toh', and to 0 if you
   don't. */
#define HAVE_DECL_BE16TOH 0

/* Define to 1 if you have the declaration of `be32dec', and to 0 if you
   don't. */
#define HAVE_DECL_BE32DEC 0

/* Define to 1 if you have the declaration of `be32enc', and to 0 if you
   don't. */
#define HAVE_DECL_BE32ENC 0

/* Define to 1 if you have the declaration of `be32toh', and to 0 if you
   don't. */
#define HAVE_DECL_BE32TOH 0

/* Define to 1 if you have the declaration of `be64dec', and to 0 if you
   don't. */
#define HAVE_DECL_BE64DEC 0

/* Define to 1 if you have the declaration of `be64enc', and to 0 if you
   don't. */
#define HAVE_DECL_BE64ENC 0

/* Define to 1 if you have the declaration of `be64toh', and to 0 if you
   don't. */
#define HAVE_DECL_BE64TOH 0

/* Define to 1 if you have the declaration of `bswap16', and to 0 if you
   don't. */
#define HAVE_DECL_BSWAP16 0

/* Define to 1 if you have the declaration of `bswap32', and to 0 if you
   don't. */
#define HAVE_DECL_BSWAP32 0

/* Define to 1 if you have the declaration of `bswap64', and to 0 if you
   don't. */
#define HAVE_DECL_BSWAP64 0

/* Define to 1 if you have the declaration of `dirname', and to 0 if you
   don't. */
#define HAVE_DECL_DIRNAME 1

/* Define to 1 if you have the declaration of `err', and to 0 if you don't. */
#define HAVE_DECL_ERR 1

/* Define to 1 if you have the declaration of `errc', and to 0 if you don't.
   */
#define HAVE_DECL_ERRC 1

/* Define to 1 if you have the declaration of `errx', and to 0 if you don't.
   */
#define HAVE_DECL_ERRX 1

/* Define to 1 if you have the declaration of `fgetln', and to 0 if you don't.
   */
#define HAVE_DECL_FGETLN 1

/* Define to 1 if you have the declaration of `fparseln', and to 0 if you
   don't. */
#define HAVE_DECL_FPARSELN 0

/* Define to 1 if you have the declaration of `fpurge', and to 0 if you don't.
   */
#define HAVE_DECL_FPURGE 1

/* Define to 1 if you have the declaration of `fstatvfs', and to 0 if you
   don't. */
#define HAVE_DECL_FSTATVFS 1

/* Define to 1 if you have the declaration of `getdelim', and to 0 if you
   don't. */
#define HAVE_DECL_GETDELIM 1

/* Define to 1 if you have the declaration of `getline', and to 0 if you
   don't. */
#define HAVE_DECL_GETLINE 1

/* Define to 1 if you have the declaration of `getprogname', and to 0 if you
   don't. */
#define HAVE_DECL_GETPROGNAME 1

/* Define to 1 if you have the declaration of `getsubopt', and to 0 if you
   don't. */
#define HAVE_DECL_GETSUBOPT 1

/* Define to 1 if you have the declaration of `gid_from_group', and to 0 if
   you don't. */
#define HAVE_DECL_GID_FROM_GROUP 0

/* Define to 1 if you have the declaration of `group_from_gid', and to 0 if
   you don't. */
#define HAVE_DECL_GROUP_FROM_GID 1

/* Define to 1 if you have the declaration of `heapsort', and to 0 if you
   don't. */
#define HAVE_DECL_HEAPSORT 1

/* Define to 1 if you have the declaration of `htobe16', and to 0 if you
   don't. */
#define HAVE_DECL_HTOBE16 0

/* Define to 1 if you have the declaration of `htobe32', and to 0 if you
   don't. */
#define HAVE_DECL_HTOBE32 0

/* Define to 1 if you have the declaration of `htobe64', and to 0 if you
   don't. */
#define HAVE_DECL_HTOBE64 0

/* Define to 1 if you have the declaration of `htole16', and to 0 if you
   don't. */
#define HAVE_DECL_HTOLE16 0

/* Define to 1 if you have the declaration of `htole32', and to 0 if you
   don't. */
#define HAVE_DECL_HTOLE32 0

/* Define to 1 if you have the declaration of `htole64', and to 0 if you
   don't. */
#define HAVE_DECL_HTOLE64 0

/* Define to 1 if you have the declaration of `isblank', and to 0 if you
   don't. */
#define HAVE_DECL_ISBLANK 1

/* Define to 1 if you have the declaration of `issetugid', and to 0 if you
   don't. */
#define HAVE_DECL_ISSETUGID 1

/* Define to 1 if you have the declaration of `lchflags', and to 0 if you
   don't. */
#define HAVE_DECL_LCHFLAGS 1

/* Define to 1 if you have the declaration of `lchmod', and to 0 if you don't.
   */
#define HAVE_DECL_LCHMOD 1

/* Define to 1 if you have the declaration of `lchown', and to 0 if you don't.
   */
#define HAVE_DECL_LCHOWN 1

/* Define to 1 if you have the declaration of `le16dec', and to 0 if you
   don't. */
#define HAVE_DECL_LE16DEC 0

/* Define to 1 if you have the declaration of `le16enc', and to 0 if you
   don't. */
#define HAVE_DECL_LE16ENC 0

/* Define to 1 if you have the declaration of `le16toh', and to 0 if you
   don't. */
#define HAVE_DECL_LE16TOH 0

/* Define to 1 if you have the declaration of `le32dec', and to 0 if you
   don't. */
#define HAVE_DECL_LE32DEC 0

/* Define to 1 if you have the declaration of `le32enc', and to 0 if you
   don't. */
#define HAVE_DECL_LE32ENC 0

/* Define to 1 if you have the declaration of `le32toh', and to 0 if you
   don't. */
#define HAVE_DECL_LE32TOH 0

/* Define to 1 if you have the declaration of `le64dec', and to 0 if you
   don't. */
#define HAVE_DECL_LE64DEC 0

/* Define to 1 if you have the declaration of `le64enc', and to 0 if you
   don't. */
#define HAVE_DECL_LE64ENC 0

/* Define to 1 if you have the declaration of `le64toh', and to 0 if you
   don't. */
#define HAVE_DECL_LE64TOH 0

/* Define to 1 if you have the declaration of `mi_vector_hash', and to 0 if
   you don't. */
#define HAVE_DECL_MI_VECTOR_HASH 0

/* Define to 1 if you have the declaration of `mkdtemp', and to 0 if you
   don't. */
#define HAVE_DECL_MKDTEMP 0

/* Define to 1 if you have the declaration of `mkstemp', and to 0 if you
   don't. */
#define HAVE_DECL_MKSTEMP 1

/* Define to 1 if you have the declaration of `optind', and to 0 if you don't.
   */
#define HAVE_DECL_OPTIND 1

/* Define to 1 if you have the declaration of `optreset', and to 0 if you
   don't. */
#define HAVE_DECL_OPTRESET 1

/* Define to 1 if you have the declaration of `pread', and to 0 if you don't.
   */
#define HAVE_DECL_PREAD 1

/* Define to 1 if you have the declaration of `pwcache_groupdb', and to 0 if
   you don't. */
#define HAVE_DECL_PWCACHE_GROUPDB 0

/* Define to 1 if you have the declaration of `pwcache_userdb', and to 0 if
   you don't. */
#define HAVE_DECL_PWCACHE_USERDB 0

/* Define to 1 if you have the declaration of `pwrite', and to 0 if you don't.
   */
#define HAVE_DECL_PWRITE 1

/* Define to 1 if you have the declaration of `raise_default_signal', and to 0
   if you don't. */
#define HAVE_DECL_RAISE_DEFAULT_SIGNAL 0

/* Define to 1 if you have the declaration of `reallocarr', and to 0 if you
   don't. */
#define HAVE_DECL_REALLOCARR 0

/* Define to 1 if you have the declaration of `reallocarray', and to 0 if you
   don't. */
#define HAVE_DECL_REALLOCARRAY 0

/* Define to 1 if you have the declaration of `setenv', and to 0 if you don't.
   */
#define HAVE_DECL_SETENV 1

/* Define to 1 if you have the declaration of `setgroupent', and to 0 if you
   don't. */
#define HAVE_DECL_SETGROUPENT 1

/* Define to 1 if you have the declaration of `setpassent', and to 0 if you
   don't. */
#define HAVE_DECL_SETPASSENT 1

/* Define to 1 if you have the declaration of `setprogname', and to 0 if you
   don't. */
#define HAVE_DECL_SETPROGNAME 1

/* Define to 1 if you have the declaration of `snprintf', and to 0 if you
   don't. */
#define HAVE_DECL_SNPRINTF 1

/* Define to 1 if you have the declaration of `strcasecmp', and to 0 if you
   don't. */
#define HAVE_DECL_STRCASECMP 1

/* Define to 1 if you have the declaration of `strlcat', and to 0 if you
   don't. */
#define HAVE_DECL_STRLCAT 1

/* Define to 1 if you have the declaration of `strlcpy', and to 0 if you
   don't. */
#define HAVE_DECL_STRLCPY 1

/* Define to 1 if you have the declaration of `strmode', and to 0 if you
   don't. */
#define HAVE_DECL_STRMODE 1

/* Define to 1 if you have the declaration of `strncasecmp', and to 0 if you
   don't. */
#define HAVE_DECL_STRNCASECMP 1

/* Define to 1 if you have the declaration of `strndup', and to 0 if you
   don't. */
#define HAVE_DECL_STRNDUP 1

/* Define to 1 if you have the declaration of `strnlen', and to 0 if you
   don't. */
#define HAVE_DECL_STRNLEN 1

/* Define to 1 if you have the declaration of `strsep', and to 0 if you don't.
   */
#define HAVE_DECL_STRSEP 1

/* Define to 1 if you have the declaration of `strsuftoll', and to 0 if you
   don't. */
#define HAVE_DECL_STRSUFTOLL 0

/* Define to 1 if you have the declaration of `strtoi', and to 0 if you don't.
   */
#define HAVE_DECL_STRTOI 0

/* Define to 1 if you have the declaration of `strtoll', and to 0 if you
   don't. */
#define HAVE_DECL_STRTOLL 1

/* Define to 1 if you have the declaration of `strtou', and to 0 if you don't.
   */
#define HAVE_DECL_STRTOU 0

/* Define to 1 if you have the declaration of `sys_signame', and to 0 if you
   don't. */
#define HAVE_DECL_SYS_SIGNAME 1

/* Define to 1 if you have the declaration of `uid_from_user', and to 0 if you
   don't. */
#define HAVE_DECL_UID_FROM_USER 0

/* Define to 1 if you have the declaration of `user_from_uid', and to 0 if you
   don't. */
#define HAVE_DECL_USER_FROM_UID 1

/* Define to 1 if you have the declaration of `vasnprintf', and to 0 if you
   don't. */
#define HAVE_DECL_VASNPRINTF 0

/* Define to 1 if you have the declaration of `vasprintf', and to 0 if you
   don't. */
#define HAVE_DECL_VASPRINTF 1

/* Define to 1 if you have the declaration of `verrc', and to 0 if you don't.
   */
#define HAVE_DECL_VERRC 1

/* Define to 1 if you have the declaration of `verrx', and to 0 if you don't.
   */
#define HAVE_DECL_VERRX 1

/* Define to 1 if you have the declaration of `vsnprintf', and to 0 if you
   don't. */
#define HAVE_DECL_VSNPRINTF 1

/* Define to 1 if you have the declaration of `vwarnc', and to 0 if you don't.
   */
#define HAVE_DECL_VWARNC 1

/* Define to 1 if you have the declaration of `vwarnx', and to 0 if you don't.
   */
#define HAVE_DECL_VWARNX 1

/* Define to 1 if you have the declaration of `warn', and to 0 if you don't.
   */
#define HAVE_DECL_WARN 1

/* Define to 1 if you have the declaration of `warnc', and to 0 if you don't.
   */
#define HAVE_DECL_WARNC 1

/* Define to 1 if you have the declaration of `warnx', and to 0 if you don't.
   */
#define HAVE_DECL_WARNX 1

/* Define to 1 if you have the `devname' function. */
#define HAVE_DEVNAME 1

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'.
   */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the `dirfd' function. */
#define HAVE_DIRFD 1

/* Define to 1 if you have the `dirname' function. */
#define HAVE_DIRNAME 1

/* Define to 1 if `dd_fd' is a member of `DIR'. */
/* #undef HAVE_DIR_DD_FD */

/* Define to 1 if `__dd_fd' is a member of `DIR'. */
#define HAVE_DIR___DD_FD 1

/* Define to 1 if you have the `dprintf' function. */
#define HAVE_DPRINTF 1

/* Define if you have the enum uio_rw type. */
#define HAVE_ENUM_UIO_RW 1

/* Define if you have the enum uio_seg type. */
/* #undef HAVE_ENUM_UIO_SEG */

/* Define to 1 if you have the <errno.h> header file. */
#define HAVE_ERRNO_H 1

/* Define to 1 if you have the <err.h> header file. */
#define HAVE_ERR_H 1

/* Define to 1 if you have the `esetfunc' function. */
/* #undef HAVE_ESETFUNC */

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the <features.h> header file. */
/* #undef HAVE_FEATURES_H */

/* Define to 1 if you have the `fgetln' function. */
#define HAVE_FGETLN 1

/* Define to 1 if you have the `flock' function. */
#define HAVE_FLOCK 1

/* Define to 1 if you have the `fparseln' function. */
#define HAVE_FPARSELN 1

/* Define to 1 if you have the `fpurge' function. */
#define HAVE_FPURGE 1

/* Define to 1 if you have the `futimes' function. */
#define HAVE_FUTIMES 1

/* Define to 1 if you have the `getline' function. */
#define HAVE_GETLINE 1

/* Define to 1 if you have the `getopt' function. */
#define HAVE_GETOPT 1

/* Define to 1 if you have the <getopt.h> header file. */
#define HAVE_GETOPT_H 1

/* Define to 1 if you have the `getopt_long' function. */
#define HAVE_GETOPT_LONG 1

/* Define to 1 if you have the `gid_from_group' function. */
/* #undef HAVE_GID_FROM_GROUP */

/* Define to 1 if you have the `group_from_gid' function. */
#define HAVE_GROUP_FROM_GID 1

/* Define to 1 if you have the <grp.h> header file. */
#define HAVE_GRP_H 1

/* Define to 1 if you have the `heapsort' function. */
#define HAVE_HEAPSORT 1

/* Define to 1 if the system has the type `id_t'. */
#define HAVE_ID_T 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `isblank' function. */
#define HAVE_ISBLANK 1

/* Define to 1 if you have the `issetugid' function. */
#define HAVE_ISSETUGID 1

/* Define to 1 if you have the `lchflags' function. */
#define HAVE_LCHFLAGS 1

/* Define to 1 if you have the `lchmod' function. */
#define HAVE_LCHMOD 1

/* Define to 1 if you have the `lchown' function. */
#define HAVE_LCHOWN 1

/* Define to 1 if you have the <libgen.h> header file. */
#define HAVE_LIBGEN_H 1

/* Define to 1 if you have the `regex' library (-lregex). */
/* #undef HAVE_LIBREGEX */

/* Define to 1 if you have the `rt' library (-lrt). */
/* #undef HAVE_LIBRT */

/* Define to 1 if you have the `z' library (-lz). */
#define HAVE_LIBZ 1

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1

/* Define to 1 if the system has the type `long long'. */
#define HAVE_LONG_LONG 1

/* Define to 1 if you have the `lutimes' function. */
#define HAVE_LUTIMES 1

/* Define to 1 if you have the <machine/bswap.h> header file. */
/* #undef HAVE_MACHINE_BSWAP_H */

/* Define to 1 if you have the <machine/endian.h> header file. */
#define HAVE_MACHINE_ENDIAN_H 1

/* Define to 1 if you have the <malloc.h> header file. */
/* #undef HAVE_MALLOC_H */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mkdtemp' function. */
#define HAVE_MKDTEMP 1

/* Define to 1 if you have the `mkstemp' function. */
#define HAVE_MKSTEMP 1

/* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
/* #undef HAVE_NDIR_H */

/* Define to 1 if you have the <netconfig.h> header file. */
/* #undef HAVE_NETCONFIG_H */

/* Define to 1 if you have the <netdb.h> header file. */
#define HAVE_NETDB_H 1

/* Define to 1 if you have the <paths.h> header file. */
#define HAVE_PATHS_H 1

/* Define to 1 if you have the `poll' function. */
#define HAVE_POLL 1

/* Define to 1 if you have the `pread' function. */
#define HAVE_PREAD 1

/* Define to 1 if you have the <pthread.h> header file. */
#define HAVE_PTHREAD_H 1

/* Define to 1 if you have the `putc_unlocked' function. */
#define HAVE_PUTC_UNLOCKED 1

/* Define to 1 if you have the `pwcache_groupdb' function. */
/* #undef HAVE_PWCACHE_GROUPDB */

/* Define to 1 if you have the `pwcache_userdb' function. */
/* #undef HAVE_PWCACHE_USERDB */

/* Define to 1 if you have the <pwd.h> header file. */
#define HAVE_PWD_H 1

/* Define to 1 if you have the `pwrite' function. */
#define HAVE_PWRITE 1

/* Define to 1 if you have the `raise_default_signal' function. */
/* #undef HAVE_RAISE_DEFAULT_SIGNAL */

/* Define to 1 if you have the `random' function. */
#define HAVE_RANDOM 1

/* Define to 1 if you have the `reallocarr' function. */
/* #undef HAVE_REALLOCARR */

/* Define to 1 if you have the `reallocarray' function. */
/* #undef HAVE_REALLOCARRAY */

/* Define to 1 if you have the <resolv.h> header file. */
#define HAVE_RESOLV_H 1

/* Define to 1 if you have the <rpc/types.h> header file. */
#define HAVE_RPC_TYPES_H 1

/* Define to 1 if you have the `setenv' function. */
#define HAVE_SETENV 1

/* Define to 1 if you have the `setgroupent' function. */
#define HAVE_SETGROUPENT 1

/* Define to 1 if you have the `setpassent' function. */
#define HAVE_SETPASSENT 1

/* Define to 1 if you have the `setprogname' function. */
#define HAVE_SETPROGNAME 1

/* Define to 1 if you have the <signal.h> header file. */
#define HAVE_SIGNAL_H 1

/* Define to 1 if you have the `snprintb_m' function. */
/* #undef HAVE_SNPRINTB_M */

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1

/* Define if you have the socklen_t type. */
#define HAVE_SOCKLEN_T 1

/* Define to 1 if you have the <stdarg.h> header file. */
#define HAVE_STDARG_H 1

/* Define to 1 if you have the <stddef.h> header file. */
#define HAVE_STDDEF_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio_ext.h> header file. */
/* #undef HAVE_STDIO_EXT_H */

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#define HAVE_STRCASECMP 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strlcat' function. */
#define HAVE_STRLCAT 1

/* Define to 1 if you have the `strlcpy' function. */
#define HAVE_STRLCPY 1

/* Define to 1 if you have the `strmode' function. */
#define HAVE_STRMODE 1

/* Define to 1 if you have the `strncasecmp' function. */
#define HAVE_STRNCASECMP 1

/* Define to 1 if you have the `strndup' function. */
#define HAVE_STRNDUP 1

/* Define to 1 if you have the `strnlen' function. */
#define HAVE_STRNLEN 1

/* Define to 1 if you have the `strsep' function. */
#define HAVE_STRSEP 1

/* Define to 1 if you have the `strsuftoll' function. */
/* #undef HAVE_STRSUFTOLL */

/* Define to 1 if you have the `strtoi' function. */
/* #undef HAVE_STRTOI */

/* Define to 1 if you have the `strtoll' function. */
#define HAVE_STRTOLL 1

/* Define to 1 if you have the `strtou' function. */
/* #undef HAVE_STRTOU */

/* Define to 1 if `d_namlen' is a member of `struct dirent'. */
#define HAVE_STRUCT_DIRENT_D_NAMLEN 1

/* Define to 1 if `f_iosize' is a member of `struct statvfs'. */
/* #undef HAVE_STRUCT_STATVFS_F_IOSIZE */

/* Define to 1 if `st_atim' is a member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT_ST_ATIM */

/* Define to 1 if `st_birthtime' is a member of `struct stat'. */
#define HAVE_STRUCT_STAT_ST_BIRTHTIME 1

/* Define to 1 if `st_birthtimensec' is a member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT_ST_BIRTHTIMENSEC */

/* Define to 1 if `st_flags' is a member of `struct stat'. */
#define HAVE_STRUCT_STAT_ST_FLAGS 1

/* Define to 1 if `st_gen' is a member of `struct stat'. */
#define HAVE_STRUCT_STAT_ST_GEN 1

/* Define to 1 if `st_mtimensec' is a member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT_ST_MTIMENSEC */

/* Define to 1 if `tm_gmtoff' is a member of `struct tm'. */
#define HAVE_STRUCT_TM_TM_GMTOFF 1

/* Define to 1 if you have the <sys/bswap.h> header file. */
/* #undef HAVE_SYS_BSWAP_H */

/* Define to 1 if you have the <sys/cdefs.h> header file. */
#define HAVE_SYS_CDEFS_H 1

/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_DIR_H */

/* Define to 1 if you have the <sys/endian.h> header file. */
/* #undef HAVE_SYS_ENDIAN_H */

/* Define to 1 if you have the <sys/featuretest.h> header file. */
/* #undef HAVE_SYS_FEATURETEST_H */

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/mman.h> header file. */
#define HAVE_SYS_MMAN_H 1

/* Define to 1 if you have the <sys/mtio.h> header file. */
/* #undef HAVE_SYS_MTIO_H */

/* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_NDIR_H */

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/poll.h> header file. */
#define HAVE_SYS_POLL_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/syslimits.h> header file. */
#define HAVE_SYS_SYSLIMITS_H 1

/* Define to 1 if you have the <sys/sysmacros.h> header file. */
/* #undef HAVE_SYS_SYSMACROS_H */

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/uio.h> header file. */
#define HAVE_SYS_UIO_H 1

/* Define to 1 if you have the <sys/utsname.h> header file. */
#define HAVE_SYS_UTSNAME_H 1

/* Define to 1 if you have the <sys/wait.h> header file. */
#define HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the <termios.h> header file. */
#define HAVE_TERMIOS_H 1

/* Define to 1 if the system has the type `uchar_t'. */
/* #undef HAVE_UCHAR_T */

/* Define to 1 if you have the `uid_from_user' function. */
/* #undef HAVE_UID_FROM_USER */

/* Define to 1 if the system has the type `uint_t'. */
/* #undef HAVE_UINT_T */

/* Define to 1 if the system has the type `ulong_t'. */
/* #undef HAVE_ULONG_T */

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `user_from_uid' function. */
#define HAVE_USER_FROM_UID 1

/* Define to 1 if the system has the type `ushort_t'. */
/* #undef HAVE_USHORT_T */

/* Define to 1 if you have the <util.h> header file. */
#define HAVE_UTIL_H 1

/* Define to 1 if the system has the type `u_char'. */
#define HAVE_U_CHAR 1

/* Define to 1 if the system has the type `u_int'. */
#define HAVE_U_INT 1

/* Define to 1 if the system has the type `u_long'. */
#define HAVE_U_LONG 1

/* Define to 1 if the system has the type `u_quad_t'. */
#define HAVE_U_QUAD_T 1

/* Define to 1 if the system has the type `u_short'. */
#define HAVE_U_SHORT 1

/* Define to 1 if you have the `vasnprintf' function. */
/* #undef HAVE_VASNPRINTF */

/* Define to 1 if you have the `vasprintf' function. */
#define HAVE_VASPRINTF 1

/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

/* Define to 1 if you have the `__fpurge' function. */
/* #undef HAVE___FPURGE */

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "lib-bug-people@NetBSD.org"

/* Define to the full name of this package. */
#define PACKAGE_NAME "libnbcompat"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "libnbcompat noversion"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libnbcompat"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "noversion"

/* Path to sh(1). */
#define PATH_BSHELL "/bin/sh"

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at runtime.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown */
/* #undef STACK_DIRECTION */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

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

/* Define for NetBSD headers. */
/* #undef _POSIX_C_SOURCE */

/* Define for NetBSD headers. */
/* #undef _POSIX_SOURCE */

/* Define for NetBSD headers. */
/* #undef _XOPEN_SOURCE */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define if you have uint16_t, but not u_int16_t. */
/* #undef u_int16_t */

/* Define if you have uint32_t, but not u_int32_t. */
/* #undef u_int32_t */

/* Define if you have uint64_t, but not u_int64_t. */
/* #undef u_int64_t */

/* Define if you have uint8_t, but not u_int8_t. */
/* #undef u_int8_t */

/* Define if you have u_int16_t, but not uint16_t. */
/* #undef uint16_t */

/* Define if you have u_int32_t, but not uint32_t. */
/* #undef uint32_t */

/* Define if you have u_int64_t, but not uint64_t. */
/* #undef uint64_t */

/* Define if you have u_int8_t, but not uint8_t. */
/* #undef uint8_t */

#include "compat_defs.h"
#endif /* !__NETBSD_NBTOOL_CONFIG_H__ */
