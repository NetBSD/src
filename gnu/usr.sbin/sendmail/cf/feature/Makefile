#	$NetBSD: Makefile,v 1.3 2002/04/24 08:18:48 lukem Exp $
#	install these sources in /usr/share/sendmail/feature

DIST=		${.CURDIR}/../../../../dist
.PATH: ${DIST}/sendmail/cf/feature

FILESDIR=	/usr/share/sendmail/feature
FILES=		accept_unqualified_senders.m4 accept_unresolvable_domains.m4 \
		access_db.m4 allmasquerade.m4 always_add_domain.m4 \
		bestmx_is_local.m4 bitdomain.m4 blacklist_recipients.m4 \
		delay_checks.m4 dnsbl.m4 domaintable.m4 \
		generics_entire_domain.m4 genericstable.m4 \
		ldap_routing.m4 limited_masquerade.m4 local_lmtp.m4 \
		local_procmail.m4 loose_relay_check.m4 mailertable.m4 \
		masquerade_entire_domain.m4 masquerade_envelope.m4 \
		no_default_msa.m4 nocanonify.m4 nodns.m4 notsticky.m4 \
		nouucp.m4 nullclient.m4 promiscuous_relay.m4 rbl.m4 \
		redirect.m4 relay_based_on_MX.m4 relay_entire_domain.m4 \
		relay_hosts_only.m4 relay_local_from.m4 relay_mail_from.m4 \
		smrsh.m4 stickyhost.m4 use_ct_file.m4 use_cw_file.m4 \
		uucpdomain.m4 virtuser_entire_domain.m4 virtusertable.m4

all:

.include <bsd.prog.mk>
