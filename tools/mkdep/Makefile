#	$NetBSD: Makefile,v 1.2 2001/11/12 23:16:31 tv Exp $

HOST_SRCDIR=	usr.bin/mkdep

.include "${.CURDIR}/../Makefile.host"

# Don't run mkdep on mkdep.
MKDEP=		touch $@ || true

# This is the only program that comes before binstall.
INSTALL=	@true
${HOST_BINDIR}/${HOSTPROGNAME}:: mkdep
	-rm -f $@
	cp mkdep $@
