#	$NetBSD: Makefile,v 1.3.12.1 2013/02/25 00:26:47 tls Exp $

# $FreeBSD: src/cddl/usr.bin/ctfdump/Makefile,v 1.1.4.1 2009/08/03 08:13:06 kensmith Exp $

.include	"../../Makefile.inc"

PROG=		ctfdump

SRCS=		dump.c \
		symbol.c \
		utils.c

CFLAGS+=	-I${OSNETDIR}/sys \
		-I${OSNETDIR}/include \
		-I${OPENSOLARIS_USR_DISTDIR} \
		-I${OPENSOLARIS_SYS_DISTDIR} \
		-I${OPENSOLARIS_USR_DISTDIR}/head \
		-I${OPENSOLARIS_USR_DISTDIR}/cmd/mdb/tools/common \
		-I${OPENSOLARIS_USR_DISTDIR}/tools/ctf/common \
		-I${OPENSOLARIS_SYS_DISTDIR}/uts/common \
		-I${NETBSDSRCDIR}/external/bsd/libelf/dist

LDFLAGS+=	-pthread

LDADD+=		-L${NETBSDSRCDIR}/external/bsd/libelf/lib
LDADD+=		-lelf -lz

.PATH:		${.CURDIR}/../../dist/tools/ctf/common
.PATH:		${.CURDIR}/../../dist/tools/ctf/dump

.include <bsd.prog.mk>
