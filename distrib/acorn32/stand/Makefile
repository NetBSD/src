#	$NetBSD: Makefile,v 1.2.2.2 2002/07/20 02:59:56 lukem Exp $
#

# we use compressed tar, SparkPlug doesn't handle gzipped tar
ARCHIVE=BtNetBSD.tar.Z

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}

all:

release: check_RELEASEDIR .WAIT ${ARCHIVE}
	mv -f ${ARCHIVE} ${RELEASEDIR}/installation/misc/

${ARCHIVE}: tmp/BtNetBSD
	cd tmp && tar cZf ${.OBJDIR}/${ARCHIVE} BtNetBSD

.PHONY: tmp/BtNetBSD
tmp/BtNetBSD:
	rm -rf tmp
	mkdir tmp
	cp -R ${.CURDIR}/BtNetBSD tmp/
	cp ${KERNOBJDIR}/INSTALL/netbsd tmp/BtNetBSD/InstKern
	${MDSETIMAGE} tmp/BtNetBSD/InstKern ${RAMDISKDIR}/ramdisk.fs
	find tmp -path '*/CVS/*' -type f -exec rm -rf {} \;
	find tmp -name CVS -type d | xargs rmdir
	find tmp -name '*.uue' | while read filename; do \
		( cd "`dirname $$filename`" && uudecode "`basename $$filename`" ); \
		rm "$$filename"; \
	done;
	# unixfs is copied into the !BtNetBSD dir at install time
	cd tmp/BtNetBSD && cp -R '!BtNetBSD/native' unixfs

clean:
	@rm -f BtNetBSD.tar.Z
	@if [ -d tmp ]; then rm -rf tmp; fi

.include <bsd.subdir.mk>

.include <bsd.obj.mk>
.include <bsd.kernobj.mk>

