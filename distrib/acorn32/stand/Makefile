#	$NetBSD: Makefile,v 1.10.2.5 2004/06/17 08:19:22 tron Exp $
#

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>


# we use spark, so that we get filetypes.
ARCHIVE=BtNetBSD.spk

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}

BOOTLOADER=	${DESTDIR}/usr/mdec/boot32,ffa

all: ${ARCHIVE}

release: check_RELEASEDIR .WAIT ${ARCHIVE}
	${RELEASE_INSTALL} ${ARCHIVE} \
	    ${RELEASEDIR}/${MACHINE}/installation/misc/

${ARCHIVE}: tmp/BtNetBSD
	export TOOL_SPARKCRC=${TOOL_SPARKCRC}; \
	cd tmp/BtNetBSD && ${.CURDIR}/../mksparkive.sh * > ${.OBJDIR}/${ARCHIVE}

UUDECODE_FILES=unixfs_res Sprite banner pling.Sprites pling.Sprites22 MemFix

UUDECODE_FILES_RENAME_unixfs_res=tmp/BtNetBSD/!BtNetBSD/native/unixfs_res,ffa
UUDECODE_FILES_RENAME_Sprite=tmp/BtNetBSD/!BtNetBSD/src/Banner/Sprite,ff9
UUDECODE_FILES_RENAME_banner=tmp/BtNetBSD/!BtNetBSD/Banner,ffa
UUDECODE_FILES_RENAME_pling.Sprites=tmp/BtNetBSD/!BtNetBSD/!Sprites,ff9
UUDECODE_FILES_RENAME_pling.Sprites22=tmp/BtNetBSD/!BtNetBSD/!Sprites22,ff9
UUDECODE_FILES_RENAME_MemFix=tmp/BtNetBSD/!BtNetBSD/MemFix,ffa

SETTYPE_FILES=!BtNetBSD/!Boot !BtNetBSD/!Edit !BtNetBSD/!Run !BtNetBSD/BtNetBSD
SETTYPE_FILES+=!BtNetBSD/PreBoot !BtNetBSD/TestBoot !BtNetBSD/checkro403

SETTYPE_FILE_!BtNetBSD/!Boot=feb
SETTYPE_FILE_!BtNetBSD/!Edit=feb
SETTYPE_FILE_!BtNetBSD/!Run=feb
SETTYPE_FILE_!BtNetBSD/BtNetBSD=ffb
SETTYPE_FILE_!BtNetBSD/PreBoot=feb
SETTYPE_FILE_!BtNetBSD/TestBoot=ffb
SETTYPE_FILE_!BtNetBSD/checkro403=ffb

SETTYPE_FILES+=!BtNetBSD/native/MountUFS !BtNetBSD/native/KillUnixfs
SETTYPE_FILES+=!BtNetBSD/native/OpenRoot !BtNetBSD/native/wd0a
SETTYPE_FILES+=!BtNetBSD/native/wd0e !BtNetBSD/native/wd0f
SETTYPE_FILES+=!BtNetBSD/native/wd0g !BtNetBSD/native/wd1a
SETTYPE_FILES+=!BtNetBSD/native/wd1e

SETTYPE_FILE_!BtNetBSD/native/KillUnixfs=feb
SETTYPE_FILE_!BtNetBSD/native/MountUFS=ffb
SETTYPE_FILE_!BtNetBSD/native/OpenRoot=feb
SETTYPE_FILE_!BtNetBSD/native/wd0a=feb
SETTYPE_FILE_!BtNetBSD/native/wd0e=feb
SETTYPE_FILE_!BtNetBSD/native/wd0f=feb
SETTYPE_FILE_!BtNetBSD/native/wd0g=feb
SETTYPE_FILE_!BtNetBSD/native/wd1a=feb
SETTYPE_FILE_!BtNetBSD/native/wd1e=feb

SETTYPE_FILES+=!BtNetBSD/src/Banner/!Path !BtNetBSD/src/Banner/Banner
SETTYPE_FILES+=!BtNetBSD/src/Banner/resutil

SETTYPE_FILE_!BtNetBSD/src/Banner/!Path=feb
SETTYPE_FILE_!BtNetBSD/src/Banner/Banner=ffb
SETTYPE_FILE_!BtNetBSD/src/Banner/resutil=ffb

SETTYPE_FILES+=misc/bb_netbsd

SETTYPE_FILE_misc/bb_netbsd=ffb

SETTYPE_FILES+=misc/Mkrel/MkRelease misc/Mkrel/MkRelScr

SETTYPE_FILE_misc/Mkrel/MkRelease=feb
SETTYPE_FILE_misc/Mkrel/MkRelScr=feb

.PATH: ${.CURDIR}/BtNetBSD/!BtNetBSD
.PATH: ${.CURDIR}/BtNetBSD/!BtNetBSD/native 
.PATH: ${.CURDIR}/BtNetBSD/!BtNetBSD/src/Banner

${UUDECODE_FILES}: setup_tmp

.PHONY: setup_tmp
setup_tmp:
	rm -rf tmp
	mkdir tmp
	cp -R ${.CURDIR}/BtNetBSD tmp/
	find tmp -path '*/CVS/*' -type f -exec rm -rf {} \;
	find tmp -name CVS -type d | xargs rmdir
	find tmp -name '*.uue' -type f -exec rm -f {} \;

.PHONY: settypes
settypes: setup_tmp
.for file in ${SETTYPE_FILES}
	mv tmp/BtNetBSD/${file} tmp/BtNetBSD/${file},${SETTYPE_FILE_${file}}
.endfor

.PHONY: tmp/BtNetBSD
tmp/BtNetBSD: setup_tmp settypes
	cp ${KERNOBJDIR}/INSTALL/netbsd tmp/BtNetBSD/InstKern,1a9
	cp ${BOOTLOADER} 'tmp/BtNetBSD/!BtNetBSD/boot32,ffa'
	${TOOL_MDSETIMAGE} tmp/BtNetBSD/InstKern,1a9 ${RAMDISKDIR}/ramdisk.fs
	# unixfs is copied into the !BtNetBSD dir at install time
	cd tmp/BtNetBSD && cp -R '!BtNetBSD/native' unixfs

clean: localclean

localclean:
	rm -f ${ARCHIVE}
	if [ -d tmp ]; then rm -rf tmp; fi

.include <bsd.files.mk>
.include <bsd.prog.mk>
