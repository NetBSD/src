#	$NetBSD: Makefile,v 1.21.32.1 2019/08/19 16:00:03 martin Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>
#DBG=-g

RUMPPRG=sysctl
MAN=	sysctl.8

SRCS=	sysctl.c

.PATH:	${.CURDIR}/../../lib/libc/gen
CPPFLAGS+= -DRUMP_ACTION
RUMPSRCS+= sysctlbyname.c sysctlgetmibinfo.c

SANITIZER_RENAME_CLASSES+=	rump
SANITIZER_RENAME_FILES.rump+=	${PROG}_rumpops.c ${RUMPSRCS}
SANITIZER_RENAME_SYMBOL.rump+=	sysctlbyname sysctlgetmibinfo

.include <bsd.prog.mk>
