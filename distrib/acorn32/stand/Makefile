#	$NetBSD: Makefile,v 1.8 2003/07/10 10:34:00 lukem Exp $
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

.PHONY: tmp/BtNetBSD
tmp/BtNetBSD:
	rm -rf tmp
	mkdir tmp
	cp -R ${.CURDIR}/BtNetBSD tmp/
	cp ${KERNOBJDIR}/INSTALL/netbsd tmp/BtNetBSD/InstKern
	${TOOL_MDSETIMAGE} tmp/BtNetBSD/InstKern ${RAMDISKDIR}/ramdisk.fs
	find tmp -path '*/CVS/*' -type f -exec rm -rf {} \;
	find tmp -name CVS -type d | xargs rmdir
	find tmp -name '*.uue' | while read filename; do \
		( cd "`dirname $$filename`" && uudecode "`basename $$filename`" ); \
		rm "$$filename"; \
	done;
	# unixfs is copied into the !BtNetBSD dir at install time
	cd tmp/BtNetBSD && cp -R '!BtNetBSD/native' unixfs

clean:
	@rm -f ${ARCHIVE}
	@if [ -d tmp ]; then rm -rf tmp; fi

.include <bsd.prog.mk>
