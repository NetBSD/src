#	$Id: Makefile,v 1.11 1994/07/06 04:08:28 mycroft Exp $

LIB=	compat
CFLAGS+=-DLIBC_SCCS -DSYSLIBC_SCCS
AINC=	-I${.CURDIR}/../libc/arch/${MACHINE_ARCH}
.if defined(DESTDIR)
AINC+=	-nostdinc -idirafter ${DESTDIR}/usr/include
.endif
NOPIC=	nopic

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

MAN3+=	ftime.0 getpw.0 stty.0 vlimit.0 vtimes.0

MLINKS+=stty.3 gtty.3

# compat 4.3 sources
# missing: ecvt.c gcvt.c sibuf.c sobuf.c strout.c
SRCS+=	cfree.c lsearch.c regex.c rexec.c

.if	(${MACHINE_ARCH} == "m68k")
SRCS+=	insque.s remque.s
.else
SRCS+=	insque.c remque.c
.endif

# missing: ecvt.0
MAN3+=	insque.0 lsearch.0 re_comp.0 rexec.0

#MLINKS+=ecvt.3 fcvt.3 ecvt.3 gcvt.3
MLINKS+=insque.3 remque.3
MLINKS+=lsearch.3 lfind.3
MLINKS+=re_comp.3 re_exec.3


# compat 4.4 sources
SRCS+=	cuserid.c
MAN3+=	cuserid.0


# regexp sources
SRCS+=	regerror.c regexp.c regsub.c
MAN3+=	regexp.0

.include <bsd.lib.mk>
