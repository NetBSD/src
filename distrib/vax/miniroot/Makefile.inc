# $NetBSD: Makefile.inc,v 1.3 2001/04/23 22:19:29 wiz Exp $

PRELABEL=${DESTDIR}/sbin/disklabel -w -r ${VND_RDEV} miniroot
# To install the boot block...
BOOTINSTALL=${DESTDIR}/sbin/disklabel -B -b ${DESTDIR}/usr/mdec/xxboot ${VND_RDEV}	
