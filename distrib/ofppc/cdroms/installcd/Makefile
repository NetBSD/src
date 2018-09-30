#	$NetBSD: Makefile,v 1.5.10.1 2018/09/30 01:45:03 pgoyette Exp $

CDBASE=		ofppccd		# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDRELEASE_NODEBUG=	true
CDKERNELS+=	netbsd-INSTALL.gz netbsd
CDINSTKERNEL=	../../instkernel

.include <bsd.kernobj.mk>	# For KERNSRCDIR
DISTRIBVERS!=	${HOST_SH} ${KERNSRCDIR}/conf/osrelease.sh

prepare_md_post:
	# install ofwboot as ofwboot.ofppc
	${HOST_INSTALL_FILE} -m ${BINMODE} ${DESTDIR}/usr/mdec/ofwboot \
	    ${.OBJDIR}/cdrom/ofwboot.ofppc
	${MKDIR} ${MKDIRPERM} ${.OBJDIR}/cdrom/nbsd
	${MKDIR} ${MKDIRPERM} ${.OBJDIR}/cdrom/ppc
	${HOST_INSTALL_FILE} -m ${BINMODE} ${DESTDIR}/usr/mdec/ofwboot \
	    ${.OBJDIR}/cdrom/nbsd/ofwboot
	${DESTDIR}/usr/mdec/mkbootinfo "NetBSD/${MACHINE} ${DISTRIBVERS}" \
	    "1,\nbsd\ofwboot" ${.OBJDIR}/cdrom/ppc/bootinfo.txt \
	    ${DESTDIR}/usr/mdec/netbsd.chrp

.include "${.CURDIR}/../../../common/Makefile.bootcd"
