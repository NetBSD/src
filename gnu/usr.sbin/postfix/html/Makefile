#	$NetBSD: Makefile,v 1.4 2003/03/08 19:12:17 perry Exp $

.include <bsd.own.mk>

BINDIR=	${PFIX_HTMLDIR}

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/html
.PATH:	${DIST}

FILES=	access.5.html aliases.5.html architecture.html \
	backstage.html basic.html big-picture.gif big-picture.html \
	bounce.8.html canonical.5.html cleanup.8.html \
	commands.html delivering.html error.8.html \
	faq.html flush.8.html goals.html inbound.gif index.html \
	lmtp.8.html local.8.html master.8.html \
	motivation.html nqmgr.8.html \
	outbound.gif pcre_table.5.html pickup.8.html \
	pipe.8.html postalias.1.html postcat.1.html \
	postconf.1.html postdrop.1.html postfix.1.html \
	postkick.1.html postlock.1.html postlog.1.html \
	postmap.1.html postqueue.1.html postsuper.1.html \
	proxymap.8.html \
	qmgr.8.html queuing.html rate.html \
	receiving.html regexp_table.5.html relocated.5.html \
	resource.html rewrite.html security.html sendmail.1.html \
	showq.8.html small-picture.gif smtp.8.html smtpd.8.html \
	spawn.8.html transport.5.html trivial-rewrite.8.html \
	uce.html virtual.5.html virtual.8.html

SYMLINKS= bounce.8.html ${BINDIR}/defer.8.html
SYMLINKS+=sendmail.1.html ${BINDIR}/mailq.1.html
SYMLINKS+=sendmail.1.html ${BINDIR}/newaliases.1.html

# not installed:
# qmqpd.8.html

.include <bsd.prog.mk>
