#	$NetBSD: Makefile,v 1.4.4.2 2002/03/16 15:57:38 jdolecek Exp $

PROGAOUT=	uboot
PROGSOURCE=	uboot.c tgets.c
NEWVERSWHAT=	"Primary Boot"

CPPFLAGS+=	-DSUPPORT_ETHERNET -DSUPPORT_TAPE -DSUPPORT_DISK

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

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
release: uboot.lif
	mkdir -p ${RELEASEDIR}/installation/misc
	rm -f ${RELEASEDIR}/installation/misc/SYS_UBOOT
	${INSTALL} ${COPY} -m 444 uboot.lif \
	    ${RELEASEDIR}/installation/misc/SYS_UBOOT
.endif # RELEASEDIR check
