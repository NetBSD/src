#	$NetBSD: Makefile,v 1.2 2002/05/11 15:56:03 jdolecek Exp $
#

# we use compressed tar, SparkPlug doesn't handle gzipped tar
ARCHIVE=BtNetBSD.tar.Z

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
