#	$NetBSD: Makefile,v 1.14 2003/07/10 10:34:15 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

IMAGE=		miniroot.fs
IMAGESIZE=	12m
MAKEFS_FLAGS=	-o minfree=0,optimization=space,density=8k

KERNEL=		${KERNOBJDIR}/INSTALL/netbsd

WARNS=		1
DBG=		-Os

LISTS=		${.CURDIR}/list
MTREECONF=	${.CURDIR}/mtree.conf
IMAGEENDIAN=	be
PARSELISTENV=	KERNEL=${KERNEL}
MAKEDEVTARGETS=	std md0 cd0 st0 st1 sd0 sd1 sd2 xd0 xy0 fd0
IMAGEDEPENDS=	${KERNEL} \
		fstab install.md profile termcap \
		${DESTDIR}/bin/cat ${DESTDIR}/bin/chmod ${DESTDIR}/bin/cp \
		${DESTDIR}/bin/date ${DESTDIR}/bin/dd ${DESTDIR}/bin/df \
		${DESTDIR}/bin/ed ${DESTDIR}/bin/expr ${DESTDIR}/bin/hostname \
		${DESTDIR}/bin/kill ${DESTDIR}/bin/ln ${DESTDIR}/bin/ls \
		${DESTDIR}/bin/mkdir ${DESTDIR}/bin/mt ${DESTDIR}/bin/mv \
		${DESTDIR}/bin/pax ${DESTDIR}/bin/ps ${DESTDIR}/bin/pwd \
		${DESTDIR}/bin/rm ${DESTDIR}/bin/rmdir ${DESTDIR}/bin/sh \
		${DESTDIR}/bin/sleep ${DESTDIR}/bin/stty ${DESTDIR}/bin/sync \
		${DESTDIR}/bin/test  \
		${DESTDIR}/dev/MAKEDEV ${DESTDIR}/dev/MAKEDEV.local \
		${DESTDIR}/etc/disktab ${DESTDIR}/etc/group \
		${DESTDIR}/etc/master.passwd ${DESTDIR}/etc/netconfig \
		${DESTDIR}/etc/passwd ${DESTDIR}/etc/protocols \
		${DESTDIR}/etc/pwd.db ${DESTDIR}/etc/services \
		${DESTDIR}/etc/spwd.db \
		${DESTDIR}/sbin/dmesg ${DESTDIR}/sbin/edlabel \
		${DESTDIR}/sbin/fsck ${DESTDIR}/sbin/fsck_ffs \
		${DESTDIR}/sbin/ifconfig ${DESTDIR}/sbin/init \
		${DESTDIR}/sbin/mknod ${DESTDIR}/sbin/mount \
		${DESTDIR}/sbin/mount_cd9660 ${DESTDIR}/sbin/mount_ffs \
		${DESTDIR}/sbin/mount_kernfs ${DESTDIR}/sbin/mount_nfs \
		${DESTDIR}/sbin/newfs ${DESTDIR}/sbin/ping \
		${DESTDIR}/sbin/reboot ${DESTDIR}/sbin/restore \
		${DESTDIR}/sbin/route ${DESTDIR}/sbin/shutdown \
		${DESTDIR}/sbin/slattach ${DESTDIR}/sbin/swapctl \
		${DESTDIR}/sbin/sysctl ${DESTDIR}/sbin/umount \
		${DESTDIR}/usr/bin/chflags ${DESTDIR}/usr/bin/cksum \
		${DESTDIR}/usr/bin/cmp ${DESTDIR}/usr/bin/false \
		${DESTDIR}/usr/bin/ftp ${DESTDIR}/usr/bin/gzip \
		${DESTDIR}/usr/bin/less ${DESTDIR}/usr/bin/rsh \
		${DESTDIR}/usr/bin/sed ${DESTDIR}/usr/bin/sort \
		${DESTDIR}/usr/bin/tip ${DESTDIR}/usr/bin/true \
		${DESTDIR}/usr/bin/vi \
		${DESTDIR}/usr/mdec/bootxx ${DESTDIR}/usr/mdec/bootyy \
		${DESTDIR}/usr/mdec/netboot ${DESTDIR}/usr/mdec/tapeboot \
		${DESTDIR}/usr/mdec/ufsboot \
		${DESTDIR}/usr/sbin/chown ${DESTDIR}/usr/sbin/installboot \
		${DESTDIR}/usr/sbin/rdate \
		${DISTRIBDIR}/miniroot/install.sh \
		${DISTRIBDIR}/miniroot/upgrade.sh \
		install.sub

IMAGEPOSTBUILD=		${TOOL_INSTALLBOOT} -v -m ${MACHINE} ${IMAGE} \
		    	    ${DESTDIR}/usr/mdec/bootxx /ufsboot
IMAGE_RELEASEDIR=	installation/miniroot

install.sub: ${DISTRIBDIR}/miniroot/install.sub
	sed -e "/^VERSION=/s/=.*/=${DISTRIBREV}/" < $? > $@

CLEANFILES+= install.sub

.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
