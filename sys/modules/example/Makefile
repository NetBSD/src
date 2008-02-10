#	$NetBSD: Makefile,v 1.2 2008/02/10 10:51:18 jmmv Exp $

KMOD=   example

SRCS=   	real-example.c
# XXX A hack to workaround the fact that the final module is named
# example.o.  To be removed once we change the extension of modules.
BUILDSYMLINKS=	${.CURDIR}/example.c real-example.c

NOMAN=	# defined

.include <bsd.kmod.mk>
