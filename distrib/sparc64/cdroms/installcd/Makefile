#	$NetBSD: Makefile,v 1.36 2026/01/06 09:29:21 nia Exp $

CDBASE=		sparc64cd		# gives ${CDBASE}.iso
CDRELEASE_NODEBUG=	true
CDRELEASE_NOCOMPAT=	true
CDRELEASE_NOHTML=	true
CDRELEASE_NOTESTS=	true

# miniroot is for non-CD installations and takes up 10MB.
CDRELEASE_EXCLUDE+=	-s ',./installation/miniroot/.*,,gp'

.include "${.CURDIR}/../../../common/Makefile.minibase"
.include "${.CURDIR}/../Makefile.cdrom"
