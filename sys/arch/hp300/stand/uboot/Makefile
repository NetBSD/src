#	$NetBSD: Makefile,v 1.2 1999/12/14 20:55:28 thorpej Exp $

PROGAOUT=	uboot
PROGSOURCE=	uboot.c tgets.c
NEWVERSWHAT=	"Primary Boot"

CPPFLAGS+=	-DSUPPORT_ETHERNET -DSUPPORT_TAPE -DSUPPORT_DISK

install:
	${INSTALL} -d -m 755 -o ${BINOWN} -g ${BINGRP} \
	    ${DESTDIR}${BINDIR}/rbootd
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 uboot.lif \
	    ${DESTDIR}${BINDIR}/uboot.lif
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

.include "../Makefile.buildboot"
