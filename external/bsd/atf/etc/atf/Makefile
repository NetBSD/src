# $NetBSD: Makefile,v 1.2 2010/06/04 08:33:40 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/atf-run/sample

CONFIGFILES=	NetBSD.conf atf-run.hooks
FILESDIR=	/etc/atf
FILESMODE=	644

.include <bsd.prog.mk> # For clean and cleandir
.include <bsd.files.mk>
