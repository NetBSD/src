#	$NetBSD: Makefile.inc,v 1.20 2002/03/06 12:27:08 lukem Exp $

# TOP is assumed to be defined by Makefile including this one.

COMMONDIR=	${TOP}/bootfloppy-common

.include <bsd.kernobj.mk>

IMAGE?=		xxx-${DISTRIBREV}.fs
.ifndef INSTKERNEL
INSTKERNNAME?=	netbsd.xxx.gz
INSTKERNODIR!=	cd ${INSTKERNDIR} ; ${MAKE} print-objdir
INSTKERNEL=	${INSTKERNODIR}/${INSTKERNNAME}
.endif
BOOTNAME?=	USTAR.volsize.4540
BASENAME?=	boot
IMAGE1?=	${BASENAME}1.fs

VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
MDEC=		${DESTDIR}/usr/mdec
FSTMP=		ustar.fs

DISKTYPE?=	floppy5
DISKSIZE?=	2400
BLOCK8K?=	149


CLEANFILES+=	netbsd ${BOOTNAME} ${FSTMP}

realall: netbsd ${BOOTNAME}
	tar cvf ${FSTMP} ${BOOTNAME} netbsd
	@echo
	@echo Making disk number one
	-mv -f ${IMAGE1} ${IMAGE1}.tmp
	dd if=/dev/zero of=${IMAGE1}.tmp count=${DISKSIZE}
	vnconfig -t ${DISKTYPE} -v -c ${VND} ${IMAGE1}.tmp
	disklabel -rw ${VND} ${DISKTYPE}
	dd bs=8k seek=1 count=${BLOCK8K} if=${FSTMP} of=${VND_RDEV}
	@echo "installing new bootblocks"
	${MDEC}/installboot.new -v ${MDEC}/fdboot_ustar ${VND_RDEV}
	vnconfig -u ${VND}
	@ls -l ${FSTMP} | (read mode links uid gid size junk; \
		dksize=$$((${DISKSIZE} * 512 - 8 * 1024)); \
		disks=$$(($$size / $$dksize + 1)); \
		if test $$size -gt $$dksize; then \
			d=2; \
			while test $$d -le $$disks; do \
				echo; \
				echo Making disk number $$d.; \
				IMAGE=${BASENAME}$${d}.fs; \
				echo USTARFS $$d > $${IMAGE}; \
				skip=$$((($$d - 1) * ${BLOCK8K})); \
				if test $$d -eq $$disks; then \
					dd bs=8k seek=1 skip=$${skip} \
						conv=sync \
						if=${FSTMP} of=$${IMAGE}; \
				else \
					dd bs=8k seek=1 skip=$${skip} \
						conv=sync count=${BLOCK8K} \
						if=${FSTMP} of=$${IMAGE}; \
				fi; \
				d=$$(($$d + 1)); \
			done; \
		else \
			dd seek=$$(($${size} / 512 + 15)) count=1 \
				if=/dev/zero of=${IMAGE1}; \
		fi; \
		echo; \
                bytes=$$(($$dksize * $$disks - $$size));    \
                echo "There are $$bytes ($$(($$bytes / 1024))K) bytes free\
                        on disk $$disks."; \
	)
	mv -f ${IMAGE1}.tmp ${IMAGE1}

unconfig:
	-vnconfig -u ${VND}

${BOOTNAME}:
	cp -fp ${MDEC}/boot ${BOOTNAME}

netbsd:
	cp -fp ${INSTKERNEL} netbsd

depend:

release:
	-mkdir -p ${RELEASEDIR}/installation/floppy
	cp -p ${BASENAME}*.fs ${RELEASEDIR}/installation/floppy
	for f in ${RELEASEDIR}/installation/floppy/${BASENAME}*.fs; do \
		gzip <$$f >$$f.gz; \
	done

clean cleandir distclean:
	/bin/rm -f *.core ${IMAGE1}.tmp ${BASENAME}?.fs ${CLEANFILES}

includes:

.include <bsd.own.mk>
.include <bsd.obj.mk>
