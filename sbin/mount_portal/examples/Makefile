# $NetBSD: Makefile,v 1.5 2002/04/24 08:18:52 lukem Exp $

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/examples/mount_portal

FILES=	overview advanced.1 advanced.1.conf fing.c rfilter.1 \
	rfilter.2 rfilter.2.conf tcp.1 tcp.1.conf fs.1 fs.1.conf \
	cvs.1 cvs.conf

SCRIPTSDIR=	${FILESDIR}

# The scripts have been renamed to have a suffix of .sh.sh, since the
# SCRIPTS rule strips off one suffix.
SCRIPTS=	sysctlfs.sh.sh cvs.sh.sh
.endif

.include <bsd.prog.mk>
