#	$NetBSD: Makefile,v 1.10 2022/12/11 07:41:37 tsutsui Exp $

PROGAOUT=	uboot
PROGSOURCE=	uboot.c tgets.c
NEWVERSWHAT=	"Primary Boot"

CPPFLAGS+=	-DSUPPORT_ETHERNET -DSUPPORT_TAPE -DSUPPORT_DISK
CPPFLAGS+=	-DSUPPORT_UFS2

.include "../Makefile.buildboot"

install:
	${INSTALL_DIR} -o ${BINOWN} -g ${BINGRP} -m 755 \
	    ${DESTDIR}${BINDIR}/rbootd
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 444 \
	    uboot.lif ${DESTDIR}${BINDIR}/uboot.lif
	rm -f ${DESTDIR}${BINDIR}/rdboot
	ln ${DESTDIR}${BINDIR}/uboot.lif ${DESTDIR}${BINDIR}/rdboot
	rm -f ${DESTDIR}${BINDIR}/bootrd
	ln ${DESTDIR}${BINDIR}/uboot.lif ${DESTDIR}${BINDIR}/bootrd
	rm -f ${DESTDIR}${BINDIR}/sdboot
	ln ${DESTDIR}${BINDIR}/uboot.lif ${DESTDIR}${BINDIR}/sdboot
	rm -f ${DESTDIR}${BINDIR}/bootsd
	ln ${DESTDIR}${BINDIR}/uboot.lif ${DESTDIR}${BINDIR}/bootsd
	rm -f ${DESTDIR}${BINDIR}/ctboot
	ln ${DESTDIR}${BINDIR}/uboot.lif ${DESTDIR}${BINDIR}/ctboot
	rm -f ${DESTDIR}${BINDIR}/bootct
	ln ${DESTDIR}${BINDIR}/uboot.lif ${DESTDIR}${BINDIR}/bootct
	rm -f ${DESTDIR}${BINDIR}/rbootd/SYS_UBOOT
	ln ${DESTDIR}${BINDIR}/uboot.lif ${DESTDIR}${BINDIR}/rbootd/SYS_UBOOT

release: check_RELEASEDIR .WAIT uboot.lif
	${HOST_INSTALL_FILE} -m ${NONBINMODE} uboot.lif \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc/SYS_UBOOT
