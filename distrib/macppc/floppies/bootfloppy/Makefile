#	$NetBSD: Makefile,v 1.16 2002/03/30 07:45:33 tsutsui Exp $

.include <bsd.own.mk>
.include <bsd.kernobj.mk>

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=		boot.fs
DISKSIZE=	2880
FSTMP=		ustar.tmp
MDEC=		${DESTDIR}/usr/mdec
BOOT=		ofwboot

KERNDIR!=	cd ${.CURDIR}/../md-kernel && ${PRINTOBJDIR}
KERN=		${KERNDIR}/netbsd.INSTALL.gz

CLEANFILES+=	netbsd ${BOOT} ${FSTMP}


DISKTYPE=	floppy

realall: netbsd ${BOOT}
	${PAX} -wvf ${FSTMP} ${BOOT} netbsd
	@echo
	@echo Making disk number one
	rm -rf empty
	mkdir -m 755 empty
	${MAKEFS} -s ${DISKSIZE}b ${IMAGE}.tmp empty
	dd bs=8k seek=1 count=$$((${DISKSIZE} / 16 - 1)) \
	    if=${FSTMP} of=${IMAGE}.tmp
	@echo "installing new bootblocks"
	${MDEC}/installboot -v -b 17 ${BOOT} ${MDEC}/bootxx ${IMAGE}.tmp
	@ls -l ${FSTMP} | (read mode links uid gid size junk; \
		dksize=$$((${DISKSIZE} * 512 - 8 * 1024)); \
		disks=$$(($$size / $$dksize + 1)); \
		if test $$size -gt $$dksize; then \
			bytes=$$(($$size - $$dksize)); \
			echo "Image is $$bytes ($$(($$bytes / 1024))K) too big\
				to fit on one disk."; \
			exit 1; \
		else \
			dd seek=$$(($${size} / 512 + 15)) count=1 \
				if=/dev/zero of=${IMAGE}.tmp; \
		fi; \
		echo; \
		bytes=$$(($$dksize * $$disks - $$size)); \
		if test "${PAD}" = "yes"; then \
			size=$$(($$size + 8 * 1024)); \
			echo "Padding image with $$bytes bytes from $$size"; \
			sectors=$$(($$bytes / 512)); \
			ssize=$$(($$size / 512)); \
			dd if=/dev/zero bs=512 seek=$${ssize} of=${IMAGE}.tmp \
				count=$${sectors}; \
		else \
			echo "There are $$bytes ($$(($$bytes / 1024))K) bytes\
				free on disk $$disks."; \
		fi; \
	)
	mv -f ${IMAGE}.tmp ${IMAGE}

# Let the kernel on the diskette be called "netbsd" although
# it is compressed.  This is because the boot code will search
# for "netbsd" first, and calling it "netbsd" instead of "netbsd.gz"
# we avoid scanning all the diskettes before reading the file.

netbsd: ${KERN}
	rm -f netbsd
	cp ${KERN} netbsd

# This is "cheating", just so that we have the available
# space at the start of the tar file.  Installboot will overwrite
# the start of the bootcode with the tail end of the "real", aligned
# second-stage boot loader when it is run (which is quite a bit shorter
# than the entire ${BOOTCODE} file, even when symbols are removed.
# (the first 15*512 bytes are put into the first 8K filesystem block,
# around the disklabel)

${BOOT}: ${MDEC}/${BOOT}
	rm -f ${BOOT}
	cp ${MDEC}/${BOOT} ${BOOT}

release: check_RELEASEDIR
	-mkdir -p ${RELEASEDIR}/installation/floppy
	cp -p ${IMAGE} ${RELEASEDIR}/installation/floppy

clean cleandir distclean:
	rm -f *.core ${CLEANFILES} ${IMAGE} ${IMAGE}.tmp
	rm -rf empty

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
