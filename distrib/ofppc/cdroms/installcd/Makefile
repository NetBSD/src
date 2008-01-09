#	$NetBSD: Makefile,v 1.1.6.1 2008/01/09 01:28:07 matt Exp $

CDBASE=		ofppccd		# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDKERNELS+=	netbsd-INSTALL.gz netbsd.ofppc
CDINSTKERNEL=	../../instkernel

prepare_md_post:
	# install ofwboot as ofwboot.ofppc
	${HOST_INSTALL_FILE} -m ${BINMODE} ${DESTDIR}/usr/mdec/ofwboot \
	    ${.OBJDIR}/cdrom/ofwboot.ofppc

.include "${.CURDIR}/../../../common/Makefile.bootcd"
