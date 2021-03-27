#	$NetBSD: Makefile,v 1.15 2021/03/27 02:58:39 simonb Exp $

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
COPTS.dtrace.c += ${${ACTIVE_CC} == "clang" :? -Wno-incompatible-pointer-types-discards-qualifiers :}
COPTS.dtrace.c += -Wno-missing-field-initializers
COPTS.dtrace.c += -Wno-format

LDFLAGS+=	-pthread
LDADD+=		   -lctf     -ldtrace     -lelf     -lproc     -ly     -ll     -lz
DPADD+=		${LIBCTF} ${LIBDTRACE} ${LIBELF} ${LIBPROC} ${LIBY} ${LIBL} ${LIBZ}

.include "../../compat/exec.mk"

.include <bsd.prog.mk>
