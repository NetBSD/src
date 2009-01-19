# $NetBSD: Makefile,v 1.1 2009/01/19 07:13:03 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/data

CONFIGFILES=	NetBSD.conf atf-run.hooks
FILESDIR=	/etc/atf
FILESMODE=	644

.include <bsd.prog.mk> # For clean and cleandir
.include <bsd.files.mk>
