#	$NetBSD: Makefile,v 1.3 2002/02/12 19:54:16 jmc Exp $

TIMESTAMP=	${TOOLDIR}/bin/nbhost-mkdep

CLEANFILES+=	config.cache config.log config.status host-mkdep

.include <bsd.hostprog.mk>

realall: host-mkdep
host-mkdep: configure host-mkdep.in
	-rm -f $@
	CC=${HOST_CC:Q} CFLAGS=${HOST_CFLAGS:Q} LDFLAGS=${HOST_LDFLAGS:Q} \
		${.CURDIR}/configure --cache-file=config.cache

# This is the only program that comes before binstall.
install: ${TIMESTAMP}
${TIMESTAMP}: host-mkdep
	if [ ! -d ${TOOLDIR}/bin ]; then \
		mkdir ${TOOLDIR}; \
		mkdir ${TOOLDIR}/bin; \
	fi; cp host-mkdep $@
	chmod +x $@

# Run by hand, then "configure" script committed:
regen:
	cd ${.CURDIR} && ${TOOLDIR}/bin/nbautoconf
