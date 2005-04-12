#	$NetBSD: Makefile,v 1.5 2005/04/12 14:23:26 jwise Exp $
#	$OpenBSD: Makefile,v 1.7 2004/02/26 07:28:55 beck Exp $

PROG=	pfspamd
SRCS=	spamd.c sdl.c grey.c
MAN=	pfspamd.8

BINDIR=	/usr/libexec

CPPFLAGS+=	-DSPAMD_CHROOT=\"/var/chroot/pfspamd\"

pfspamd.8:	spamd.8
	cp $> $@

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/libexec/spamd
