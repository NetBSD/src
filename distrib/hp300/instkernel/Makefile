#	$NetBSD: Makefile,v 1.1 2002/02/10 01:22:55 gmcgarry Exp $

.include "../../../Makefile.inc"
.include <bsd.kernobj.mk>
.include <bsd.own.mk>

DISKBINDIR=	/installation/diskimage
KERNBINDIR=	/binary/kernel

KERN=		${KERNOBJDIR}/RAMDISK/netbsd
RAMDISK!=	cd ${.CURDIR}/../ramdisk; ${MAKE} echoimage
CLEANFILES=	netbsd netbsd.gz

MDSETIMAGE?=	mdsetimage

all:	netbsd.gz

netbsd.gz: ${KERN} ${RAMDISK}
	cp ${KERN} netbsd
	${MDSETIMAGE} -v netbsd ${RAMDISK}
	rm -f netbsd.gz
	gzip -9 netbsd

realinstall:
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -o root -g wheel \
		-m ${BINMODE} netbsd.gz ${RELEASEDIR}${KERNBINDIR}/install.gz

clean cleandir distclean:
	rm -f ${CLEANFILES}

# XXX!
depend:
dependall: depend all

.include <bsd.obj.mk>
.include "../Makefile.inc"
