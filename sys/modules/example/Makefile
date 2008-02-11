#	$NetBSD: Makefile,v 1.1.4.3 2008/02/11 14:59:59 yamt Exp $

KMOD=   example

SRCS=   	real-example.c
# XXX A hack to workaround the fact that the final module is named
# example.o.  To be removed once we change the extension of modules.
BUILDSYMLINKS=	${.CURDIR}/example.c real-example.c

NOMAN=	# defined

.include <bsd.kmod.mk>
