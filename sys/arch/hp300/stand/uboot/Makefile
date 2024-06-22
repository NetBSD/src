#	$NetBSD: Makefile,v 1.10.2.2 2024/06/22 10:57:11 martin Exp $

PROGELF=	uboot
PROGSOURCE=	uboot.c tgets.c
NEWVERSWHAT=	"Primary Boot"

CPPFLAGS+=	-DSUPPORT_ETHERNET -DSUPPORT_TAPE -DSUPPORT_DISK
CPPFLAGS+=	-DSUPPORT_UFS2
CPPFLAGS+=	-DSUPPORT_CD

LINKS=	${BINDIR}/${PROG} ${BINDIR}/rdboot
LINKS+=	${BINDIR}/${PROG} ${BINDIR}/bootrd
LINKS+=	${BINDIR}/${PROG} ${BINDIR}/sdboot
LINKS+=	${BINDIR}/${PROG} ${BINDIR}/bootsd
LINKS+=	${BINDIR}/${PROG} ${BINDIR}/ctboot
LINKS+=	${BINDIR}/${PROG} ${BINDIR}/bootct
LINKS+=	${BINDIR}/${PROG} ${BINDIR}/rbootd/SYS_UBOOT

.include "../Makefile.buildboot"

beforeinstall:
	${INSTALL_DIR} -o ${BINOWN} -g ${BINGRP} -m 755 \
	    ${DESTDIR}${BINDIR}/rbootd

release: check_RELEASEDIR .WAIT ${PROG}
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${PROG} \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc/SYS_UBOOT
