# $NetBSD: Makefile,v 1.2 2009/12/03 10:28:37 roy Exp $
#

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/external/bsd/openresolv/dist
.PATH:		${DIST} 

SCRIPTS=	resolvconf
SCRIPTS+=	libc dnsmasq named unbound

SCRIPTSDIR_resolvconf= /sbin
SCRIPTSDIR=	/libexec/resolvconf

MAN=		resolvconf.conf.5 resolvconf.8

CLEANFILES=	${SCRIPTS} ${MAN}

CMD1=		\1 status >/dev/null 2>\&1
CMD2=		\1 restart
RESTARTCMD=	/etc/rc.d/${CMD1} \&\& /etc/rc.d/${CMD2}
.for f in ${SCRIPTS} ${MAN}
${f}:	${f}.in
	${TOOL_SED} -e 's:@PREFIX@::g' \
		-e 's:@SYSCONFDIR@:/etc:g' \
		-e 's:@LIBEXECDIR@:/libexec/resolvconf:g' \
		-e 's:@VARDIR@:/var/run/resolvconf:g' \
		-e 's:@RESTARTCMD \(.*\)@:${RESTARTCMD}:g' \
		${DIST}/${f}.in > $@
.endfor

.include <bsd.prog.mk>
