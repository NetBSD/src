#	$NetBSD: Makefile,v 1.9 2004/03/19 08:07:28 jmc Exp $
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

# Annoying, but since they're buried under BtNetBSD and need to end up in tmp
# this is the simplest way to manage it. The other choice is moving all the 
# .uue files into this directory.
unixfs_res.uue:
	cp ${.CURDIR}/'BtNetBSD/!BtNetBSD/native/unixfs_res.uue' unixfs_res.uue
Sprite.uue:
	cp ${.CURDIR}/'BtNetBSD/!BtNetBSD/src/Banner/sprite.uue' Sprite.uue
Banner.uue:
	cp ${.CURDIR}/'BtNetBSD/!BtNetBSD/Banner.uue' Banner.uue
Sprites.uue:
	cp ${.CURDIR}/'BtNetBSD/!BtNetBSD/!Sprites.uue' Sprites.uue
Sprites22.uue:
	cp ${.CURDIR}/'BtNetBSD/!BtNetBSD/!Sprites22.uue' Sprites22.uue
MemFix.uue:
	cp ${.CURDIR}/'BtNetBSD/!BtNetBSD/MemFix.uue' MemFix.uue

CLEANFILES+=unixfs_res.uue Sprite.uue Banner.uue Sprites.uue Sprites22.uue \
	    MemFix.uue

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
