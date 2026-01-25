#	$NetBSD: Makefile,v 1.2.2.2 2026/01/25 16:31:06 martin Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-luna68k-live	# gives ${IMGBASE}.img

.include <bsd.own.mk>

LIVEIMAGEMB?=	1536	# < 2GB is preferred for SCSI emulators that use FAT32
MDBOOTPARTMB?=	8	# as PART_BOOT in sysinst/arch/luna68k/md.h

USE_MBR=	no
USE_GPT=	no
USE_MDBOOTPART=	yes
USE_SUNLABEL=	yes

DISKPROTO_IN=	${.CURDIR}/diskproto.in
FSTAB_IN=	${.CURDIR}/fstab.in
SPEC_EXTRA=	${.CURDIR}/spec.in

.include "${.CURDIR}/../../common/bootimage/Makefile.liveimage"

MDBOOTPARTFILES=	${WORKDIR}/usr/mdec/boot
MDBOOTPARTMAKEFSOPTIONS= -t ffs -o version=0,bsize=4096,fsize=512,density=2048
SPEC_BOOT=		spec.boot
CLEANFILES+=		${SPEC_BOOT}

${MDBOOTPARTFILES}: ${TARGETFS}
	# ${MDBOOTPARTFILES} are assumed extracted during to ${WORKDIR}
	# from binary sets during ${TARGETFS} image build

${WORKMDBOOTPART}: ${TARGETFS} ${MDBOOTPARTFILES}
	@echo create MD boot partition for bootstrap files...
	@${MKDIR} ${MKDIRPERM} ${WORKMDBOOTPARTDIR}
	rm -f ${SPEC_BOOT}
	cp ${.CURDIR}/spec.boot.in ${SPEC_BOOT}
	@echo Copying files for MD boot partition...
.for f in ${MDBOOTPARTFILES}
	@if [ ! -f ${f} ]; then 					\
		echo "${f} in MDBOOTPARTFILES not found, aborting";	\
		false; 							\
	fi
	${INSTALL} ${COPY} -m 0644 ${f} ${WORKMDBOOTPARTDIR}
	echo "./$$(basename ${f}) type=file uname=root gname=wheel mode=0644" \
	    >> ${SPEC_BOOT}
.endfor
	${RM} -f ${WORKMDBOOTPART}
	${TOOL_MAKEFS} -M ${MDBOOTPARTMB}m -m ${MDBOOTPARTMB}m		\
	    -xx -F ${SPEC_BOOT}	-N ${WORKDIR}/etc			\
	    -B ${TARGET_ENDIANNESS}					\
	    ${MAKEFS_TIMESTAMP}						\
	    ${MDBOOTPARTMAKEFSOPTIONS}					\
	    ${WORKMDBOOTPART} ${WORKMDBOOTPARTDIR}
