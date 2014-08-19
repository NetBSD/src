# $NetBSD: Makefile,v 1.3.12.1 2014/08/19 23:45:54 tls Exp $

.include <bsd.init.mk>

.PATH:		${SRCDIR}/tools/sample

CONFIGFILES=	NetBSD.conf atf-run.hooks common.conf
FILESDIR=	/etc/atf
FILESMODE=	644

.include <bsd.prog.mk> # For clean and cleandir
.include <bsd.files.mk>
