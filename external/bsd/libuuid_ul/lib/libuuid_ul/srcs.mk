#	$NetBSD: srcs.mk,v 1.1 2019/03/08 09:58:23 mrg Exp $

# Sources and flags for libuuid_ul.

UUID_UL_SRCS=	\
	compare.c \
	copy.c \
	gen_uuid.c \
	pack.c \
	parse.c \
	unpack.c \
	unparse.c

UUID_UL_CPPFLAGS+= \
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

UUID_UL_CPPFLAGS+= \
	-I. \
	-I${NETBSDSRCDIR}/external/bsd/libuuid_ul/lib/libuuid_ul \
	-DUL_CLOEXECSTR=\"e\" \
	-DUUIDD_OP_BULK_TIME_UUID=0

.for _s in ${UUID_UL_SRCS}
CPPFLAGS.${_s}+=	${UUID_UL_CPPFLAGS}
.endfor

.PATH: ${X11SRCDIR}/external/bsd/libuuid_ul/dist
