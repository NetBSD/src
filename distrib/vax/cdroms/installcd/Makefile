#	$NetBSD: Makefile,v 1.19 2019/12/20 19:46:51 martin Exp $
CDBASE=		vaxcd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$RELEASEMACHINEDIR
CDDEV_POPULATE=	true			# populate /dev on the CD
CDRELEASE_NODEBUG=	true
CDKERNELS=	${RELEASEDIR}/${RELEASEMACHINEDIR}/binary/kernel/netbsd-GENERIC.gz	netbsd.gz
CDRELEASE_NOISOS=true
CD_SETS=	base etc	# no modules for VAX

# for PRINTOBJDIR
.include <bsd.own.mk>
SYSINSTDIR!= cd ${.CURDIR}/../../../../usr.sbin/sysinst/arch/${MACHINE} && ${PRINTOBJDIR}

# prepare mutliuser install environment
image_md_pre:
	${RM} -f cdrom/boot
	${INSTALL} ${COPY} -m 0444 ${DESTDIR}/usr/mdec/boot cdrom/boot
	${RM} -f bootxx.${MACHINE}
	${INSTALL} ${COPY} -m 0444 ${DESTDIR}/usr/mdec/xxboot bootxx.${MACHINE}
	${RM} -f cdrom/etc/gettytab cdrom/etc/ttys cdrom/etc/rc
	${HOST_LN} -fs /tmp/gettytab cdrom/etc/gettytab
	${INSTALL} ${COPY} ${.CURDIR}/etc.ttys cdrom/etc/ttys
	${INSTALL} ${COPY} ${.CURDIR}/etc.rc cdrom/etc/rc
	${INSTALL} ${COPY} -m 0555 ${.CURDIR}/install.sh cdrom/install.sh
	${MKDIR} ${MKDIRPERM} cdrom/mnt2 cdrom/targetroot
	${INSTALL} ${COPY} ${SYSINSTDIR}/sysinstmsgs.?? cdrom/usr/share/sysinst/catalog/
	${TOOL_GZIP} -d -f cdrom/netbsd.gz

# make the CD bootable
image_md_post:
	${TOOL_INSTALLBOOT} -m${MACHINE} ${CDIMAGE} bootxx.${MACHINE}

.include "${.CURDIR}/../../../common/Makefile.bootcd"
