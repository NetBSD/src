#       $NetBSD: Makefile,v 1.6 2009/09/19 07:09:53 snj Exp $
#

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

UUDECODE_FILES=BSD_Mac68k_Booter.bin Mkfs.sea.hqx Booter-HTML-manual-12.tar
UUDECODE_FILES+=Mkfs_1.47.sea.bin Booter2.0.0-src.sea Mkfs_1.47.sea.hqx 
UUDECODE_FILES+=Booter2.0.0.sea Mkfs_1.47src.sea.hqx BooterManual.stxt.bin 
UUDECODE_FILES+=NetBSD_ROM.sit.hqx Installer.sea.hqx README.Booter 
UUDECODE_FILES+=Installer_1.1g.sea.bin README.Installer Installer_1.1g.sea.hqx 
UUDECODE_FILES+=README Installer_1.1g.src.sea.hqx mode32-7.5.bin 
UUDECODE_FILES+=Installer_1.1h.sea.bin mode32-7.5.hqx Installer_1.1h.sea.hqx 
UUDECODE_FILES+=pdisk.sea.hqx Installer_1.1h.src.sea.hqx res-edit-and-others.hqx

release: ${UUDECODE_FILES}
	${INSTALL} -d ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc
	${RELEASE_INSTALL} ${DISTRIBDIR}/mac68k/stand/extensions.map \
	  ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc
	rm -f \
	  ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc/{MD5,SHA512}
	for i in BSD_Mac68k_Booter.bin Mkfs.sea.hqx Booter-HTML-manual-12.tar \
	  Mkfs_1.47.sea.bin Mkfs_1.47.sea.hqx Booter2.0.0.sea \
	  BooterManual.stxt.bin NetBSD_ROM.sit.hqx Installer.sea.hqx \
	  README.Booter Installer_1.1g.sea.bin README.Installer \
	  Installer_1.1g.sea.hqx README mode32-7.5.bin Installer_1.1h.sea.bin \
	  mode32-7.5.hqx Installer_1.1h.sea.hqx pdisk.sea.hqx \
	  res-edit-and-others.hqx \
	  ${.CURDIR}/BSD_Mac68k_Booter.bin.uue \
	  ${.CURDIR}/BooterManual.stxt.bin.uue ; \
	do \
	  ${RELEASE_INSTALL} $$i ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc ; \
	  (cd ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc ; \
	    ${TOOL_CKSUM} -a MD5 `basename $$i` >> MD5) ; \
	  (cd ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc ; \
	    ${TOOL_CKSUM} -a SHA512 `basename $$i` >> SHA512) ; \
	done
	${INSTALL} -d ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc/src
	rm -f \
	  ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc/src/{MD5,SHA512}
	for i in Booter2.0.0-src.sea Mkfs_1.47src.sea.hqx \
	  Installer_1.1g.src.sea.hqx Installer_1.1h.src.sea.hqx ; \
	do \
	  ${RELEASE_INSTALL} $$i \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc/src ; \
	  (cd ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc/src ; \
	    ${TOOL_CKSUM} -a MD5 `basename $$i` >> MD5) ; \
	  (cd ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc/src ; \
	    ${TOOL_CKSUM} -a SHA512 `basename $$i` >> SHA512) ; \
	done

.include <bsd.files.mk>
.include <bsd.obj.mk>
