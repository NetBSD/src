#	$NetBSD: Makefile,v 1.6 2004/04/27 05:33:55 heas Exp $

.include <bsd.own.mk>

.if ${MKDOC} != "no"						# {

BINDIR=	${PFIX_HTMLDIR}

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/html
.PATH:	${DIST}

PFX_DAEMONS =  bounce.8.html cleanup.8.html error.8.html local.8.html \
	lmtp.8.html master.8.html pickup.8.html pipe.8.html qmgr.8.html \
	showq.8.html smtp.8.html smtpd.8.html trivial-rewrite.8.html \
	oqmgr.8.html spawn.8.html flush.8.html virtual.8.html qmqpd.8.html \
	verify.8.html proxymap.8.html
PFX_COMMANDS= postalias.1.html postcat.1.html \
	postconf.1.html postfix.1.html postkick.1.html postlock.1.html \
	postlog.1.html postdrop.1.html postmap.1.html sendmail.1.html \
	postqueue.1.html postsuper.1.html smtp-source.1.html \
	smtp-sink.1.html qmqp-source.1.html qmqp-sink.1.html \
	qshape.1.html
PFX_CONFIG  = access.5.html aliases.5.html canonical.5.html relocated.5.html \
	transport.5.html virtual.5.html pcre_table.5.html regexp_table.5.html \
	cidr_table.5.html header_checks.5.html \
	ldap_table.5.html mysql_table.5.html pgsql_table.5.html

FILES=	$(PFX_DAEMONS) $(PFX_COMMANDS) $(PFX_CONFIG)

SYMLINKS= bounce.8.html ${BINDIR}/defer.8.html
SYMLINKS+=bounce.8.html ${BINDIR}/trace.8.html
SYMLINKS+=sendmail.1.html ${BINDIR}/mailq.1.html
SYMLINKS+=sendmail.1.html ${BINDIR}/newaliases.1.html

# not installed:
# qmqpd.8.html

.endif								# }

.include <bsd.prog.mk>
