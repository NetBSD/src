#	$Id: Makefile,v 1.5 1994/01/15 19:31:32 cgd Exp $

LIB=	compat
CFLAGS+=-DLIBC_SCCS -DSYSLIBC_SCCS
AINC=	-I${.CURDIR}/../libc/${MACHINE_ARCH}
NOPIC=	nopic

.PATH:  ${.CURDIR}/4.1/${MACHINE_ARCH} ${.CURDIR}/4.1 \
	${.CURDIR}/4.3/${MACHINE_ARCH} ${.CURDIR}/4.3 \
	${.CURDIR}/regexp

# compat 4.1 sources
SRCS+=	ftime.c

# compat 4.3 sources
SRCS+=	insque.c regex.c remque.c

MAN3+=	insque.0

MLINKS+=insque.3 remque.3

# regexp sources
SRCS+=	regerror.c regexp.c regsub.c

MAN3+=	regexp.0

MLINKS+=regexp.3 regcomp.3 regexp.3 regexec.3 regexp.3 regsub.3 \
	regexp.3 regerror.3

.include <bsd.lib.mk>
