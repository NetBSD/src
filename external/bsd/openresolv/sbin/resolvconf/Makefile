# $NetBSD: Makefile,v 1.7.16.1 2020/04/13 07:56:27 martin Exp $
#

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/external/bsd/openresolv/dist
.PATH:		${DIST} 

SCRIPTS=	resolvconf
SCRIPTSDIR=	/sbin

FILES=		libc dnsmasq named pdns_recursor pdnsd unbound
FILESDIR=	/libexec/resolvconf
# We should not have to do this ...
.for f in ${FILES}
FILESBUILD_$f=	yes
.endfor

MAN=		resolvconf.conf.5 resolvconf.8

CLEANFILES=	${SCRIPTS} ${FILES} ${MAN}

.for f in ${SCRIPTS} ${FILES} ${MAN}
${f}:	Makefile ${f}.in
	${TOOL_SED} -e 's:@PREFIX@::g' \
		-e 's:@SBINDIR@:/sbin:g' \
		-e 's:@RCDIR@:/etc/rc.d:g' \
		-e 's:@SYSCONFDIR@:/etc:g' \
		-e 's:@LIBEXECDIR@:/libexec/resolvconf:g' \
		-e 's:@VARDIR@:/var/run/resolvconf:g' \
		-e 's:@RESTARTCMD@::g' \
		-e 's:@STATUSARG@::g' \
		${DIST}/${f}.in > $@
.endfor

.include <bsd.prog.mk>
