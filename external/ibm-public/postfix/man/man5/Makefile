#	$NetBSD: Makefile,v 1.1.10.2 2013/01/23 00:05:18 yamt Exp $

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/man/man5
.PATH:	${DIST}

MAN=	access.5 aliases.5 bounce.5 canonical.5 relocated.5 \
	transport.5 virtual.5 sqlite_table.5 regexp_table.5 \
	cidr_table.5 header_checks.5 ldap_table.5 master.5  \
	nisplus_table.5 generic.5 memcache_table.5
MAN+=	postconf.5 postfix-wrapper.5

MLINKS=	header_checks.5 body_checks.5

# XXX notyet:
# The base install does include support for these:
# mysql_table.5 pgsql_table.5

.include <bsd.man.mk>
