#	$NetBSD: Makefile,v 1.3 2005/04/11 22:34:18 jwise Exp $
#	$OpenBSD: Makefile,v 1.7 2004/02/26 07:28:55 beck Exp $

PROG=	pfspamd
SRCS=	spamd.c sdl.c grey.c
MAN=	pfspamd.8

BINDIR=	/usr/libexec

pfspamd.8:	spamd.8
	cp $> $@

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/libexec/spamd
