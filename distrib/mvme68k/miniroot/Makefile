#	$NetBSD: Makefile,v 1.1 2000/11/24 19:49:40 scw Exp $

all:

clean cleandir distclean:
	-rm -f a.out core *.core *.o

.if !defined(RELEASEDIR)
release:
	@echo setenv RELEASEDIR before doing that!
	@false
.else	# RELEASEDIR
release: 
	-mkdir -p ${RELEASEDIR}/installation/miniroot
	gzip < ${.CURDIR}/../../miniroot/miniroot \
	 > ${RELEASEDIR}/installation/miniroot/miniroot.gz
.endif	# RELEASEDIR

