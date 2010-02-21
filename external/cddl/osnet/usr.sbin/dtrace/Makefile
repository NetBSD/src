#	$NetBSD: Makefile,v 1.3 2010/02/21 11:00:02 darran Exp $

# $FreeBSD: src/cddl/usr.sbin/dtrace/Makefile,v 1.1.4.1 2009/08/03 08:13:06 kensmith Exp $

.include <bsd.own.mk>
.include	"../../Makefile.inc"

PROG=		dtrace

BINDIR?=	/usr/sbin

SRCS=		dtrace.c

#WARNS=		1

CFLAGS+=	-I${OSNETDIR}/sys \
		-I${OSNETDIR}/include \
		-I${OPENSOLARIS_USR_DISTDIR}/head \
		-I${OPENSOLARIS_USR_DISTDIR}/lib/libdtrace/common \
		-I${OPENSOLARIS_USR_DISTDIR}/lib/libproc/common \
		-I${OPENSOLARIS_SYS_DISTDIR}/uts/common \
		-I${OPENSOLARIS_SYS_DISTDIR}/compat \

.PATH:		${OPENSOLARIS_USR_DISTDIR}/cmd/dtrace

# Optional debugging stuff...
#CFLAGS+=	-DNEED_ERRLOC
#YFLAGS+=	-d


LDFLAGS+=	-pthread

LIBCTF_OBJDIR!=	cd ${.CURDIR}/../../lib/libctf && ${PRINTOBJDIR}
LDFLAGS+=	-L${LIBDTRACE_OBJDIR} -ldtrace

LIBDTRACE_OBJDIR!= cd ${.CURDIR}/../../lib/libdtrace && ${PRINTOBJDIR}
LDFLAGS+=	-L${LIBCTF_OBJDIR} -lctf

LDADD+=		-ly -ll -lelf -lz

.include <bsd.prog.mk>
