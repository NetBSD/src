#	$NetBSD: Makefile,v 1.3.12.1 2014/08/19 23:52:23 tls Exp $

# $FreeBSD: src/cddl/lib/libctf/Makefile,v 1.2.2.1 2009/08/03 08:13:06 kensmith Exp $

.include	"../../Makefile.inc"

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

.PATH:		${.CURDIR}/../../dist/common/ctf
.PATH:		${.CURDIR}/../../dist/lib/libctf/common

CPPFLAGS+=	-DCTF_OLD_VERSIONS

CPPFLAGS+=	-I${OSNETDIR}/sys \
		-I${OSNETDIR}/include \
		-I${OPENSOLARIS_USR_DISTDIR}/head \
		-I${OPENSOLARIS_USR_DISTDIR}/common/ctf \
		-I${OPENSOLARIS_USR_DISTDIR}/lib/libctf/common \
		-I${OPENSOLARIS_SYS_DISTDIR}/uts/common

.include <bsd.lib.mk>

