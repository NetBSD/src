#	$NetBSD: Makefile,v 1.2 2005/04/12 14:39:39 jwise Exp $

.include <bsd.own.mk>

.PATH:	${NETBSDSRCDIR}/dist/pf/share/man/man5

MAN+=	pf.conf.5
MAN+=	pf.os.5
MAN+=	pfspamd.conf.5

pfspamd.conf.5:	spamd.conf.5
	cp $> $@

.include <bsd.man.mk>
