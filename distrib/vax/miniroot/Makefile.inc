# $NetBSD: Makefile.inc,v 1.1.6.2 2000/11/04 19:42:34 he Exp $

PRELABEL=${DESTDIR}/sbin/disklabel -w -r ${VND_RDEV} miniroot
# To install the boot block...
BOOTINSTALL=${DESTDIR}/sbin/disklabel -B -b ${DESTDIR}/usr/mdec/xxboot ${VND_RDEV}	
