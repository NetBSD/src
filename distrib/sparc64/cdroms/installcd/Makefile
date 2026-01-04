#	$NetBSD: Makefile,v 1.34.4.1 2026/01/04 12:09:39 martin Exp $

CDBASE=		sparc64cd		# gives ${CDBASE}.iso
CDRELEASE_NODEBUG=	true
CDRELEASE_NOCOMPAT=	true
CDRELEASE_NOHTML=	true
CDRELEASE_NOTESTS=	true

# miniroot is for non-CD installations and takes up 10MB.
CDRELEASE_EXCLUDE+=	-s ',./installation/miniroot/.*,,gp'

.include "${.CURDIR}/../Makefile.cdrom"
