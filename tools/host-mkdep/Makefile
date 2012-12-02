#	$NetBSD: Makefile,v 1.11 2012/12/02 12:21:51 apb Exp $

TIMESTAMP=	${TOOLDIR}/bin/${_TOOL_PREFIX}host-mkdep

CLEANFILES+=	config.cache config.log config.status host-mkdep

.include <bsd.hostprog.mk>

# XXX: The configure script needs to use awk, but we can't pass
# AWK=${TOOL_AWK:Q} in CONFIGURE_ENV, because TOOL_AWK is not yet
# available at the time that host-mkdep is built.  The configure script
# will try to find some other version of awk.
#
CONFIGURE_ENV=	CC=${HOST_CC:Q}

realall: host-mkdep
host-mkdep: configure host-mkdep.in
	-rm -f $@
	${CONFIGURE_ENV} \
	    ${HOST_SH} ${.CURDIR}/configure --cache-file=config.cache
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
