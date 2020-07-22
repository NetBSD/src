# $NetBSD: Makefile.inc,v 1.1 2020/07/22 19:00:19 roy Exp $
#

DIST=		${NETBSDSRCDIR}/external/bsd/openresolv/dist
.PATH:		${DIST} 

RESOLVCONF_SED=	-e 's:@PREFIX@::g' \
		-e 's:@SBINDIR@:/sbin:g' \
		-e 's:@RCDIR@:/etc/rc.d:g' \
		-e 's:@SYSCONFDIR@:/etc:g' \
		-e 's:@LIBEXECDIR@:/libexec/resolvconf:g' \
		-e 's:@VARDIR@:/var/run/resolvconf:g' \
		-e 's:@RESTARTCMD@::g' \
		-e 's:@STATUSARG@::g'
