# $NetBSD commit to lose -kk$

PRELABEL=${DESTDIR}/sbin/disklabel -w -r ${VND_RDEV} miniroot
# To install the boot block...
BOOTINSTALL=${DESTDIR}/sbin/disklabel -B -b ${DESTDIR}/usr/mdec/xxboot ${VND_RDEV}	
