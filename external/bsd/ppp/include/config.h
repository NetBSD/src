/* pppd/config.h.  Generated from config.h.in by configure.  */
/* pppd/config.h.in.  Generated from configure.ac by autoheader.  */

/* Define to 1 if you have the <asm/types.h> header file. */
/* #undef HAVE_ASM_TYPES_H */

/* Define to 1 if you have the <crypt.h> header file. */
/* #undef HAVE_CRYPT_H */

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <linux/if_ether.h> header file. */
/* #undef HAVE_LINUX_IF_ETHER_H */

/* Define to 1 if you have the <linux/if.h> header file. */
/* #undef HAVE_LINUX_IF_H */

/* Define to 1 if you have the <linux/if_packet.h> header file. */
/* #undef HAVE_LINUX_IF_PACKET_H */

/* System provides the logwtmp() function */
#define HAVE_LOGWTMP 1

/* Define to 1 if you have the 'mmap' function. */
#define HAVE_MMAP 1

/* Define to 1 if you have the <netinet/if_ether.h> header file. */
/* #undef HAVE_NETINET_IF_ETHER_H */

/* Define to 1 if you have the <netpacket/packet.h> header file. */
/* #undef HAVE_NETPACKET_PACKET_H */

/* Define to 1 if you have the <net/bpf.h> header file. */
/* #undef HAVE_NET_BPF_H */

/* Define to 1 if you have the <net/if_arp.h> header file. */
/* #undef HAVE_NET_IF_ARP_H */

/* Define to 1 if you have the <net/if.h> header file. */
/* #undef HAVE_NET_IF_H */

/* Define to 1 if you have the <net/if_types.h> header file. */
/* #undef HAVE_NET_IF_TYPES_H */

/* Define to 1 if you have the <paths.h> header file. */
#define HAVE_PATHS_H 1

/* Define to 1 if you have the <shadow.h> header file. */
/* #undef HAVE_SHADOW_H */

/* Define to 1 if you have the <stdarg.h> header file. */
#define HAVE_STDARG_H 1

/* Define to 1 if stdbool.h conforms to C99. */
#define HAVE_STDBOOL_H 1

/* Define to 1 if you have the <stddef.h> header file. */
#define HAVE_STDDEF_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the 'strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Struct sockaddr_ll is present on system */
/* #undef HAVE_STRUCT_SOCKADDR_LL */

/* Define to 1 if you have the <sys/dlpi.h> header file. */
/* #undef HAVE_SYS_DLPI_H */

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/uio.h> header file. */
#define HAVE_SYS_UIO_H 1

/* Define to 1 if you have the <time.h> header file. */
#define HAVE_TIME_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <utmp.h> header file. */
#define HAVE_UTMP_H 1

/* Define to 1 if the system has the type '_Bool'. */
#define HAVE__BOOL 1

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Use DES included with openssl */
#define OPENSSL_HAVE_DES 1

/* Use MD4 included with openssl */
#define OPENSSL_HAVE_MD4 1

/* Use MD5 included with openssl */
#define OPENSSL_HAVE_MD5 1

/* Use SHA included with openssl */
#define OPENSSL_HAVE_SHA 1

/* OpenSSL engine support */
/* #undef OPENSSL_NO_ENGINE */

/* Name of package */
#define PACKAGE "ppp"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "https://github.com/ppp-project/ppp"

/* Define to the full name of this package. */
#define PACKAGE_NAME "ppp"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "ppp 2.5.2"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "ppp"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "2.5.2"

/* Version of pppd */
#define PPPD_VERSION "2.5.2"

/* Have Callback Protocol support */
#define PPP_WITH_CBCP 1

/* Have Microsoft CHAP support */
#define PPP_WITH_CHAPMS 1

/* Have EAP-TLS authentication support */
#define PPP_WITH_EAPTLS 1

/* Have packet activity filter support */
#define PPP_WITH_FILTER 1

#if 0
/* from Makefile */
/* Have IPv6 Control Protocol */
#define PPP_WITH_IPV6CP 1
#endif

/* Have Microsoft MPPE support */
#define PPP_WITH_MPPE 1

/* Have Microsoft LAN Manager support */
#define PPP_WITH_MSLANMAN 1

/* Have multilink support */
#define PPP_WITH_MULTILINK 1

/* PPP is compiled with openssl support */
#define PPP_WITH_OPENSSL 1

/* Support for Pluggable Authentication Modules */
#ifdef USE_PAM
#define PPP_WITH_PAM 1
#else
#undef PPP_WITH_PAM
#endif

/* Have PEAP authentication support */
#define PPP_WITH_PEAP 1

/* Have support for loadable plugins */
#define PPP_WITH_PLUGINS 1

/* Support for libsrp authentication module */
/* #undef PPP_WITH_SRP */

/* Include TDB support */
#define PPP_WITH_TDB 1

/* The size of 'unsigned int', as computed by sizeof. */
#define SIZEOF_UNSIGNED_INT 4

/* The size of 'unsigned long', as computed by sizeof. */
#ifdef _LP64
#define SIZEOF_UNSIGNED_LONG 8
#else
#define SIZEOF_UNSIGNED_LONG 4
#endif

/* The size of 'unsigned short', as computed by sizeof. */
#define SIZEOF_UNSIGNED_SHORT 2

/* Define to 1 if all of the C89 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
#define STDC_HEADERS 1

/* Enable support for systemd notifications */
/* #undef SYSTEMD */

/* Version number of package */
#define VERSION "2.5.2"
