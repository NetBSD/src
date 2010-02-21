#	$NetBSD: Makefile,v 1.2 2010/02/21 01:46:36 darran Exp $

# $FreeBSD: src/cddl/usr.sbin/dtrace/Makefile,v 1.1.4.1 2009/08/03 08:13:06 kensmith Exp $

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

LDFLAGS+=	-pthread \
		-L${OSNETDIR}/lib/libdtrace \
		-L${OSNETDIR}/lib/libctf 

LDADD+=		-ldtrace -ly -ll -lctf -lelf -lz

#LDFLAGS+=	-pthread \
		-L${.OBJDIR}/../../lib/libdtrace \
		-L${.OBJDIR}/../../lib/libproc \
		-L${.OBJDIR}/../../lib/libctf \
		-L${.OBJDIR}/../../../lib/libelf

#LDADD+=		-ldtrace -ly -ll -lproc -lctf -lelf -lz

#DPADD+= ${LIBDTRACE} ${LIBCTF} ${LIBELF} ${LIBPTHREAD} ${LIBL} ${LIBY} ${LIBZ}

.include <bsd.prog.mk>
