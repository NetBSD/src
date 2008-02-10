#	$NetBSD: Makefile,v 1.1 2008/02/10 12:40:10 jmmv Exp $

KMOD=   	k_helper
KMODDIR=	${TESTSBASE}/modules

SRCS=   	real-k_helper.c
# XXX A hack to workaround the fact that the final module is named
# k_helper.o.  To be removed once we change the extension of modules.
BUILDSYMLINKS=	${.CURDIR}/k_helper.c real-k_helper.c

NOATFFILE=	# defined
NOMAN=		# defined

.include <bsd.test.mk>
.include <bsd.kmod.mk>
