#	$NetBSD: Makefile,v 1.10 2004/03/20 02:55:21 jmc Exp $
#

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>


# we use compressed tar, SparkPlug doesn't handle gzipped tar
ARCHIVE=BtNetBSD.tar.Z

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}

all: ${ARCHIVE}

release: check_RELEASEDIR .WAIT ${ARCHIVE}
	${RELEASE_INSTALL} ${ARCHIVE} \
	    ${RELEASEDIR}/${MACHINE}/installation/misc/

${ARCHIVE}: tmp/BtNetBSD
	cd tmp && tar cZf ${.OBJDIR}/${ARCHIVE} BtNetBSD

UUDECODE_FILES=unixfs_res Sprite Banner Sprites Sprites22 MemFix

UUDECODE_FILES_RENAME_unixfs_res=tmp/BtNetBSD/!BtNetBSD/native/unixfs_res
UUDECODE_FILES_RENAME_Sprite=tmp/BtNetBSD/!BtNetBSD/src/Banner/Sprite
UUDECODE_FILES_RENAME_Banner=tmp/BtNetBSD/!BtNetBSD/Banner
UUDECODE_FILES_RENAME_Sprites=tmp/BtNetBSD/!BtNetBSD/!Sprites
UUDECODE_FILES_RENAME_Sprites22=tmp/BtNetBSD/!BtNetBSD/!Sprites22
UUDECODE_FILES_RENAME_MemFix=tmp/BtNetBSD/!BtNetBSD/MemFix

.PATH: ${.CURDIR}/BtNetBSD/!BtNetBSD/native 
.PATH: ${.CURDIR}/BtNetBSD/!BtNetBSD/src/Banner
.PATH: ${.CURDIR}/BtNetBSD/!BtNetBSD

${UUDECODE_FILES}: setup_tmp

.PHONY: setup_tmp
setup_tmp:
	rm -rf tmp
	mkdir tmp
	cp -R ${.CURDIR}/BtNetBSD tmp/
	find tmp -path '*/CVS/*' -type f -exec rm -rf {} \;
	find tmp -name CVS -type d | xargs rmdir

.PHONY: tmp/BtNetBSD
tmp/BtNetBSD: setup_tmp
	cp ${KERNOBJDIR}/INSTALL/netbsd tmp/BtNetBSD/InstKern
	${TOOL_MDSETIMAGE} tmp/BtNetBSD/InstKern ${RAMDISKDIR}/ramdisk.fs
	# unixfs is copied into the !BtNetBSD dir at install time
	cd tmp/BtNetBSD && cp -R '!BtNetBSD/native' unixfs

clean: localclean

localclean:
	rm -f ${ARCHIVE}
	if [ -d tmp ]; then rm -rf tmp; fi

.include <bsd.files.mk>
.include <bsd.prog.mk>
