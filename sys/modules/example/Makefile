#	$NetBSD: Makefile,v 1.2.10.2 2008/03/23 02:05:04 matt Exp $

KMOD=   example

SRCS=   	real-example.c
# XXX A hack to workaround the fact that the final module is named
# example.o.  To be removed once we change the extension of modules.
BUILDSYMLINKS=	${.CURDIR}/example.c real-example.c

NOMAN=	# defined

.include <bsd.kmod.mk>
