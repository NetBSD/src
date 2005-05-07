#	$NetBSD: Makefile,v 1.1.2.2 2005/05/07 14:52:32 riz Exp $

.include <bsd.own.mk>

.PATH:	${NETBSDSRCDIR}/dist/pf/share/man/man5

MAN+=	pf.conf.5
MAN+=	pf.os.5
MAN+=	pfspamd.conf.5

pfspamd.conf.5:	spamd.conf.5
	cp $> $@

CLEANFILES+=	pfspamd.conf.5

.include <bsd.man.mk>
