#	$NetBSD: Makefile,v 1.34.2.1 2026/11/04 22:37:09 perseant Exp $

CDBASE=		sparc64cd		# gives ${CDBASE}.iso
CDRELEASE_NODEBUG=	true
CDRELEASE_NOCOMPAT=	true
CDRELEASE_NOHTML=	true
CDRELEASE_NOTESTS=	true

# miniroot is for non-CD installations and takes up 10MB.
CDRELEASE_EXCLUDE+=	-s ',./installation/miniroot/.*,,gp'

CDMAKEFSEXTRAOPTS=	-m 700m

.include "${.CURDIR}/../../../common/Makefile.minibase"
.include "${.CURDIR}/../Makefile.cdrom"
