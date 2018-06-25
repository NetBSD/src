#	$NetBSD: Makefile,v 1.9.14.1 2018/06/25 07:25:27 pgoyette Exp $

# $FreeBSD: head/cddl/usr.sbin/dtrace/Makefile 314654 2017-03-04 11:30:04Z ngie $

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
COPTS.dtrace.c += -Wno-format-extra-args

COPTS.dtrace.c += -Wno-shadow
COPTS.dtrace.c += ${${ACTIVE_CC} == "gcc" :? -Wno-discarded-qualifiers :}
COPTS.dtrace.c += ${${ACTIVE_CC} == "gcc" :? -Wno-unused-but-set-variable :}
COPTS.dtrace.c += -Wno-missing-field-initializers
COPTS.dtrace.c += -Wno-format

LDFLAGS+=	-pthread

PROGDPLIBS+=	ctf ${.CURDIR}/../../lib/libctf
PROGDPLIBS+=	dtrace ${.CURDIR}/../../lib/libdtrace
PROGDPLIBS+=	elf ${NETBSDSRCDIR}/external/bsd/elftoolchain/lib/libelf
PROGDPLIBS+=	y ${NETBSDSRCDIR}/lib/liby
PROGDPLIBS+=	l ${NETBSDSRCDIR}/external/bsd/flex/lib
PROGDPLIBS+=	z ${NETBSDSRCDIR}/lib/libz

.include <bsd.prog.mk>
