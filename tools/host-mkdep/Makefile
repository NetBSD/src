#	$NetBSD: Makefile,v 1.1 2002/01/10 17:04:21 tv Exp $

TIMESTAMP=	${TOOLDIR}/bin/nbhost-mkdep

CLEANFILES+=	config.cache config.log config.status host-mkdep

.include <bsd.hostprog.mk>

realall: host-mkdep
host-mkdep: configure host-mkdep.in
	-rm -f $@
	CC="${HOST_CC}" ${.CURDIR}/configure --cache-file=config.cache

# This is the only program that comes before binstall.
install: ${TIMESTAMP}
${TIMESTAMP}: host-mkdep
	cp host-mkdep $@
	chmod +x $@

# Run by hand, then "configure" script committed:
regen:
	cd ${.CURDIR} && ${TOOLDIR}/bin/nbautoconf
