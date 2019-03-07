#	$NetBSD: Makefile,v 1.1 2019/03/07 03:53:51 mrg Exp $

# Makefile to build minimal util-linux libuuid locally, with special
# prefix on all functions to avoid conflicts with libc uuid_compare(3).
#

LIB=	uuid_ul
SRCS=	compare.c copy.c \
	gen_uuid.c \
	pack.c parse.c \
	unpack.c unparse.c

CPPFLAGS+= \
	-DHAVE_UNISTD_H \
	-DHAVE_STDLIB_H \
	-DHAVE_SYS_TIME_H \
	-DHAVE_SYS_FILE_H \
	-DHAVE_SYS_IOCTL_H \
	-DHAVE_SYS_SOCKET_H \
	-DHAVE_SYS_SOCKIO_H \
	-DHAVE_SYS_SOCKIO_H \
	-DHAVE_NETINET_IN_H \
	-DHAVE_SYS_UN_H \
	-DHAVE_TLS \
	-DHAVE_NET_IF_H \
	-DHAVE_NET_IF_DL_H \
	-DHAVE_SA_LEN

CPPFLAGS+= \
	-I${.CURDIR} \
	-DUL_CLOEXECSTR=\"e\" \
	-DUUIDD_OP_BULK_TIME_UUID=0

.include "libuuid_ul-rename.mk"

LIBISPRIVATE=	yes

.include <bsd.lib.mk>

.PATH: ${X11SRCDIR}/external/bsd/libuuid_ul/dist
