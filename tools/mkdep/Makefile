#	$NetBSD: Makefile,v 1.3 2001/11/14 22:13:41 tv Exp $

HOSTPROGNAME=	nbmkdep
HOST_SRCDIR=	usr.bin/mkdep

.include "${.CURDIR}/../Makefile.host"

# Don't run mkdep on mkdep.
MKDEP=		touch $@ || true

# This is the only program that comes before binstall.
INSTALL=	@true
${HOST_BINDIR}/${HOSTPROGNAME}:: mkdep
	-rm -f $@
	cp mkdep $@
