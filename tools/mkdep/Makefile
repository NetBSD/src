#	$NetBSD: Makefile,v 1.6 2002/01/03 22:45:23 tv Exp $

HOSTPROGNAME=	nbmkdep
HOST_SRCDIR=	usr.bin/mkdep
NOCOMPATLIB=	# defined

.PATH:		${.CURDIR}/../compat
SRCS=		mkdep.c setprogname.c

.include "${.CURDIR}/../Makefile.host"

CLEANFILES+=	config.cache config.log config.status host-mkdep

# Ths host mkdep lives here until it is installed.
MKDEP=		sh ./host-mkdep
.depend mkdep:	host-mkdep
host-mkdep:	configure host-mkdep.in
	-rm -f $@
	CC="${HOST_CC}" ${.CURDIR}/configure --cache-file=config.cache

# This is the only program that comes before binstall.
INSTALL=	true
${HOST_BINDIR}/nbmkdep:: mkdep
	-rm -f ${HOST_BINDIR}/nbmkdep ${HOST_BINDIR}/nbhost-mkdep
	cp mkdep ${HOST_BINDIR}/nbmkdep
	cp host-mkdep ${HOST_BINDIR}/nbhost-mkdep
	chmod +x ${HOST_BINDIR}/nbhost-mkdep

# Run by hand, then "configure" script committed:
regen:
	cd ${.CURDIR} && ${TOOLDIR}/bin/nbautoconf
