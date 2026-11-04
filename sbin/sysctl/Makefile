#	$NetBSD: Makefile,v 1.22.10.1 2026/11/04 23:19:44 perseant Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>
#DBG=-g

RUMPPRG=sysctl
MAN=	sysctl.8

SRCS=	sysctl.c

.PATH:	${.CURDIR}/../../lib/libc/gen
CPPFLAGS+= -DRUMP_ACTION
RUMPSRCS+= sysctlbyname.c sysctlgetmibinfo.c

LINTFLAGS.sysctl.c+=	-X 351	# missing 'extern' header declaration

SANITIZER_RENAME_CLASSES+=	rump
SANITIZER_RENAME_FILES.rump+=	${PROG}_rumpops.c ${RUMPSRCS}
SANITIZER_RENAME_SYMBOL.rump+=	sysctlbyname sysctlgetmibinfo

.include <bsd.prog.mk>
