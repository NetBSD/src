#	$NetBSD: Makefile,v 1.5.2.1 2003/08/12 18:27:29 tron Exp $

TIMESTAMP=	${TOOLDIR}/bin/nbhost-mkdep

CLEANFILES+=	config.cache config.log config.status host-mkdep

.include <bsd.hostprog.mk>

realall: host-mkdep
host-mkdep: configure host-mkdep.in
	-rm -f $@
	CC=${HOST_CC:Q} CFLAGS=${HOST_CFLAGS:Q} LDFLAGS=${HOST_LDFLAGS:Q} \
		sh ${.CURDIR}/configure --cache-file=config.cache

# This is the only program that comes before binstall.
install: ${TIMESTAMP}
${TIMESTAMP}: host-mkdep
	mkdir -p ${TOOLDIR}/bin
	cp host-mkdep $@
	chmod +x $@

# Run by hand, then "configure" script committed:
regen:
	cd ${.CURDIR} && ${TOOLDIR}/bin/nbautoconf
