#	$Id: Makefile.inc,v 1.1.1.1 1995/08/27 06:36:35 phil Exp $

# TOP is assumed to be defined by Makefile including this one.

CBIN=		instbin
COMMONDIR=	${TOP}/inst-common

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}c
VND_RDEV=	/dev/r${VND}c
IMAGE?=		xxx-${REV}.fs
MDEC=		${DESTDIR}/usr/mdec

LISTS=		${COMMONDIR}/list ${.CURDIR}/list
CRUNCHCONF=	${COMMONDIR}/${CBIN}.conf
MTREE=		${COMMONDIR}/mtree.conf
ADIR=		${.CURDIR}/../../../../sys/arch/pc532

all: ${CBIN} netbsd
	dd if=/dev/zero of=${IMAGE} bs=18k count=80
	disklabel -r -w ${.OBJDIR}/${IMAGE} boot3
	-yes|bim -c init -c "add ${MDEC}/boot boot" -c "default 0" -c "exit" ${.OBJDIR}/${IMAGE}
	vnconfig -v -c ${VND_DEV} ${IMAGE}
	newfs -O -m 0 -o space -i 7168 -c 80 ${VND_RDEV} boot3
	mount ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${MTREE} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/runlist.sh ${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_DEV}
	cat /*bin/* > /dev/null

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}
	-/bin/rm -f ${IMAGE}

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	crunchgen -D ${TOP}/../../.. -L ${DESTDIR}/usr/lib ${CRUNCHCONF}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	make -f ${CBIN}.mk all

cloak: ${COMMONDIR}/cloak.c

netbsd.o: cloak
	cp ${ADIR}/compile/INSTALL/netbsd /tmp/$$$$; \
		strip /tmp/$$$$; \
		gzip -9 </tmp/$$$$ | ./cloak >netbsd.o; \
		rm /tmp/$$$$

netbsd: netbsd.o
	ld -s -z -T 380000 -e begin ${MDEC}/zboot.o netbsd.o -o netbsd

clean cleandir:
	/bin/rm -f core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache *.o *.lo *.c

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
