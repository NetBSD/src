# $NetBSD: Makefile,v 1.4 2014/02/08 19:15:33 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tools/sample

CONFIGFILES=	NetBSD.conf atf-run.hooks common.conf
FILESDIR=	/etc/atf
FILESMODE=	644

.include <bsd.prog.mk> # For clean and cleandir
.include <bsd.files.mk>
