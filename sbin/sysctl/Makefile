#	$NetBSD: Makefile,v 1.9 2000/01/17 02:36:32 itojun Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	sysctl
MAN=	sysctl.8

CPPFLAGS+=-DINET6 -DIPSEC
#CPPFLAGS+=-DTCP6

afterinstall:
	(cd ${DESTDIR}/usr/sbin; ln -sf ../../sbin/sysctl .)

.include <bsd.prog.mk>
