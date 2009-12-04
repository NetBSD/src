# $NetBSD: Makefile,v 1.3 2009/12/04 14:44:57 roy Exp $
#

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/external/bsd/openresolv/dist
.PATH:		${DIST} 

SCRIPTS=	resolvconf
SCRIPTSDIR=	/sbin

FILES=		libc dnsmasq named unbound
FILESDIR=	/libexec/resolvconf
# We should not have to do this ...
.for f in ${FILES}
FILESBUILD_$f=	yes
.endfor

MAN=		resolvconf.conf.5 resolvconf.8

CLEANFILES=	${SCRIPTS} ${FILES} ${MAN}

CMD1=		\1 status >/dev/null 2>\&1
CMD2=		\1 restart
RESTARTCMD=	/etc/rc.d/${CMD1} \&\& /etc/rc.d/${CMD2}
.for f in ${SCRIPTS} ${FILES} ${MAN}
${f}:	${f}.in
	${TOOL_SED} -e 's:@PREFIX@::g' \
		-e 's:@SYSCONFDIR@:/etc:g' \
		-e 's:@LIBEXECDIR@:/libexec/resolvconf:g' \
		-e 's:@VARDIR@:/var/run/resolvconf:g' \
		-e 's:@RESTARTCMD \(.*\)@:${RESTARTCMD}:g' \
		${DIST}/${f}.in > $@
.endfor

.include <bsd.prog.mk>
