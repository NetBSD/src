#	$NetBSD: Makefile,v 1.9 2003/10/26 07:25:38 lukem Exp $

TIMESTAMP=	${TOOLDIR}/bin/${_TOOL_PREFIX}host-mkdep

CLEANFILES+=	config.cache config.log config.status host-mkdep

.include <bsd.hostprog.mk>

realall: host-mkdep
host-mkdep: configure host-mkdep.in
	-rm -f $@
	CC=${HOST_CC:Q} ${HOST_SH} ${.CURDIR}/configure --cache-file=config.cache
	chmod +x $@

# This is the only program that comes before binstall.
install: ${TIMESTAMP}
${TIMESTAMP}: host-mkdep
	mkdir -p ${TOOLDIR}/bin
	cp host-mkdep $@
	chmod +x $@

# Run by hand, then "configure" script committed:
regen:
	cd ${.CURDIR} && ${TOOLDIR}/bin/${_TOOL_PREFIX}autoconf
