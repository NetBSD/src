#	$NetBSD: Makefile,v 1.7 2004/09/05 08:38:23 manu Exp $

.include <bsd.own.mk>

# XXX Unsupported Kerberos options were removed from man page
# XXX Don't forget to update the man page if you fix Kerberos
PROG=	rcmd
SRCS=	rsh.c
CPPFLAGS+=-DIN_RCMD
BINOWN=	root
BINMODE=4555
.PATH:	${NETBSDSRCDIR}/usr.bin/rsh

.include <bsd.prog.mk>
