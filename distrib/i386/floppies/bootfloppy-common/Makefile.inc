#	$NetBSD: Makefile.inc,v 1.34 2002/02/03 14:56:00 lukem Exp $

# TOP is assumed to be defined by Makefile including this one.

VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
IMAGE?=		boot-xxx.fs
FSTMP=		ustar.tmp
MDEC=		${DESTDIR}/usr/mdec
BOOTCODE?=	${MDEC}/biosboot.sym
#BOOTCODE=	${MDEC}/biosboot_com0.sym	# Serial output
STRIP?=		strip

DISKTYPE?=	floppy3
DISKSIZE?=	2880
BLOCK8K?=	179

CLEANFILES+=	netbsd boot ustar.tmp ${METAFILE}

realall:	netbsd boot
	if test "${METAFILE}" != ""; then \
		touch ${METAFILE}; \
	fi
	tar cvf ${FSTMP} boot ${METAFILE} netbsd
	@echo
	@echo Making disk number one
	rm -f ${IMAGE}.tmp
	dd if=/dev/zero of=${IMAGE}.tmp count=${DISKSIZE}
	vnconfig -t ${DISKTYPE} -v -c ${VND} ${IMAGE}.tmp
	disklabel -rw ${VND} ${DISKTYPE}
	dd bs=8k seek=1 count=${BLOCK8K} if=${FSTMP} of=${VND_RDEV}
	@echo "installing new bootblocks"
	${MDEC}/installboot -b 17 -v -f ${BOOTCODE} ${VND_RDEV}
	vnconfig -u ${VND}
	@ls -l ${FSTMP} | (read mode links uid gid size junk; \
		dksize=$$((${DISKSIZE} * 512 - 8 * 1024)); \
		disks=$$(($$size / $$dksize + 1)); \
		if test $$size -gt $$dksize; then \
                	bytes=$$(($$size - $$dksize));    \
			echo "Image is $$bytes ($$(($$bytes / 1024))K) too big\
				to fit on one disk."; \
			exit 1; \
		else \
			dd seek=$$(($${size} / 512 + 15)) count=1 \
				if=/dev/zero of=${IMAGE}.tmp; \
		fi; \
		echo; \
                bytes=$$(($$dksize * $$disks - $$size));    \
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

boot: ${BOOTCODE}
	rm -f boot
	cp ${BOOTCODE} boot
	strip boot

release:
	-mkdir -p ${RELEASEDIR}/installation/floppy
	cp -p ${IMAGE} ${RELEASEDIR}/installation/floppy
	
clean cleandir distclean:
	/bin/rm -f *.core ${CLEANFILES} ${IMAGE} ${IMAGE}.tmp

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
