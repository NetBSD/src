#	$NetBSD: Makefile,v 1.4 1995/09/29 07:22:43 cgd Exp $

# Make a distribution for the alpha, on a spare disk.
# This creates a large, gzipped disk image in ${.OBJDIR}

.if !defined(DESTDIR) || !defined(DESTDISK)
all:
	@echo both 'DESTDIR' and 'DESTDISK' must be defined.
	@false
.else

SECPERCYL=	558
CYLS=		1476
TARGET=		alphadist-rz25.gz

all: ${TARGET}

${TARGET}:
	disklabel -W ${DESTDISK}
	-dd if=/dev/zero of=/dev/r${DESTDISK}c bs=`expr ${SECPERCYL} \* 512` \
	    count=${CYLS}
	disklabel -w -r -B -b /usr/mdec/sdboot -s /usr/mdec/bootsd \
	    ${DESTDISK} rz25
	disklabel -W ${DESTDISK}
	newfs -O /dev/r${DESTDISK}a
	newfs -O /dev/r${DESTDISK}d
	mount /dev/${DESTDISK}a ${DESTDIR}
	mkdir ${DESTDIR}/usr
	mount /dev/${DESTDISK}d ${DESTDIR}/usr
	cd ${.CURDIR}/../../../etc && make distribution
	cd ${.CURDIR}/../../../cygnus && \
	    make prefix=${DESTDIR}/usr/local install
	cd ${DESTDIR}/dev && sh MAKEDEV all
	cp ${.CURDIR}/../../../sys/arch/alpha/compile/GENERIC/netbsd ${DESTDIR}
	ln -s gcc ${DESTDIR}/usr/local/bin/cc
	ln -s \
	../local/lib/gcc-lib/alpha-unknown-netbsd1.0A/2.7-95q4/cpp \
	    ${DESTDIR}/usr/libexec
	ln -s ../local/bin/ar ${DESTDIR}/usr/bin
	ln -s ../local/bin/as ${DESTDIR}/usr/bin
	ln -s ../local/bin/cc ${DESTDIR}/usr/bin
	ln -s ../local/bin/gcc ${DESTDIR}/usr/bin
	ln -s ../local/bin/ld ${DESTDIR}/usr/bin
	ln -s ../local/bin/nm ${DESTDIR}/usr/bin
	ln -s ../local/bin/ranlib ${DESTDIR}/usr/bin
	ln -s ../local/bin/size ${DESTDIR}/usr/bin
	ln -s ../local/bin/strip ${DESTDIR}/usr/bin
	cp -p ${DESTDIR}/usr/mdec/boot ${DESTDIR}/
	umount ${DESTDIR}/usr ${DESTDIR}
	fsck /dev/r${DESTDISK}a /dev/r${DESTDISK}d
	/bin/rm -f $@
	dd if=/dev/r${DESTDISK}c bs=`expr ${SECPERCYL} \* 512` \
	    count=${CYLS} | gzip -9 > $@
.endif

_SUBDIRUSE:

.include <bsd.own.mk>
.include <bsd.obj.mk>
