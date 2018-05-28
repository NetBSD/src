#	$NetBSD: Makefile,v 1.10 2018/05/28 21:05:09 chs Exp $

# $FreeBSD: head/cddl/lib/libctf/Makefile 314654 2017-03-04 11:30:04Z ngie $

.include <bsd.init.mk>

LIB=		ctf
SRCS=		ctf_create.c \
		ctf_decl.c \
		ctf_error.c \
		ctf_hash.c \
		ctf_labels.c \
		ctf_lib.c \
		ctf_lookup.c \
		ctf_open.c \
		ctf_subr.c \
		ctf_types.c \
		ctf_util.c
MAN=		ctf.5

.PATH:		${.CURDIR}/../../dist/common/ctf
.PATH:		${.CURDIR}/../../dist/lib/libctf/common

CPPFLAGS+=	-DCTF_OLD_VERSIONS

CPPFLAGS+=	-I${OSNETDIR}/sys \
		-I${OSNETDIR}/include \
		-I${OPENSOLARIS_USR_DISTDIR}/head \
		-I${OPENSOLARIS_USR_DISTDIR}/common/ctf \
		-I${OPENSOLARIS_USR_DISTDIR}/lib/libctf/common \
		-I${OPENSOLARIS_SYS_DISTDIR}/uts/common

LIBDPLIBS+=	z	${NETBSDSRCDIR}/lib/libz

.include <bsd.lib.mk>

