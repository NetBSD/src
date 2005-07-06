#	$NetBSD: Makefile,v 1.28 2005/07/06 14:43:24 drochner Exp $

.include <bsd.own.mk>

LIB=	compat
CPPFLAGS+=-DLIBC_SCCS -DSYSLIBC_SCCS -D__LIBCOMPAT_SOURCE__
AFLAGS+=-I${NETBSDSRCDIR}/lib/libc/arch/${MACHINE_ARCH}
.if defined(DESTDIR)
AFLAGS+=-nostdinc -idirafter ${DESTDIR}/usr/include
.endif

.PATH:	${.CURDIR}/4.1/${MACHINE_ARCH} ${.CURDIR}/4.1 \
	${.CURDIR}/4.3/${MACHINE_ARCH} ${.CURDIR}/4.3 \
	${.CURDIR}/4.4/${MACHINE_ARCH} ${.CURDIR}/4.4 \
	${.CURDIR}/regexp

# compat 4.1 sources
# missing: getpw.c tell.c vlimit.c vtimes.c
SRCS=	gtty.c ftime.c stty.c

.if (${MACHINE} == "vax")
# missing: (for vax) reset.s
.endif

MAN+=	ftime.3 stty.3

MLINKS+=stty.3 gtty.3

# compat 4.3 sources
# missing: ecvt.c gcvt.c sibuf.c sobuf.c strout.c
SRCS+=	cfree.c regex.c rexec.c
SRCS+=	ruserpass.c

# missing: ecvt.0
MAN+=	re_comp.3 rexec.3

#MLINKS+=ecvt.3 fcvt.3 ecvt.3 gcvt.3
MLINKS+=re_comp.3 re_exec.3


# compat 4.4 sources
SRCS+=	cuserid.c
MAN+=	cuserid.3


# regexp sources
SRCS+=	regerror.c regexp.c regsub.c
MAN+=	regexp.3

.include <bsd.lib.mk>
