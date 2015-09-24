#	$NetBSD: Makefile,v 1.6 2015/09/24 14:36:54 christos Exp $

# $FreeBSD: src/cddl/usr.sbin/dtrace/Makefile,v 1.1.4.1 2009/08/03 08:13:06 kensmith Exp $

.include <bsd.own.mk>
.include	"../../Makefile.inc"

PROG=		dtrace
MAN=		dtrace.1

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

COPTS.dtrace.c += -Wno-stack-protector
COPTS.dtrace.c += -Wno-stack-protector -Wno-format-extra-args

LDFLAGS+=	-pthread

PROGDPLIBS+=	ctf ${.CURDIR}/../../lib/libctf
PROGDPLIBS+=	dtrace ${.CURDIR}/../../lib/libdtrace
PROGDPLIBS+=	elf ${NETBSDSRCDIR}/external/bsd/elftoolchain/lib/libelf
PROGDPLIBS+=	y ${NETBSDSRCDIR}/lib/liby
PROGDPLIBS+=	l ${NETBSDSRCDIR}/external/bsd/flex/lib
PROGDPLIBS+=	z ${NETBSDSRCDIR}/lib/libz

.include <bsd.prog.mk>
