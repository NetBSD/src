#	$NetBSD: Makefile.inc,v 1.1 2003/03/24 15:04:04 atatat Exp $

CF_FEATURE_FILES= \
		accept_unqualified_senders.m4 \
		accept_unresolvable_domains.m4 access_db.m4 \
		allmasquerade.m4 always_add_domain.m4 authinfo.m4 \
		bestmx_is_local.m4 bitdomain.m4 blacklist_recipients.m4 \
		compat_check.m4 delay_checks.m4 dnsbl.m4 domaintable.m4 \
		enhdnsbl.m4 generics_entire_domain.m4 genericstable.m4 \
		ldap_routing.m4 limited_masquerade.m4 local_lmtp.m4 \
		local_no_masquerade.m4 local_procmail.m4 \
		lookupdotdomain.m4 loose_relay_check.m4 mailertable.m4 \
		masquerade_entire_domain.m4 masquerade_envelope.m4 msp.m4 \
		no_default_msa.m4 nocanonify.m4 nodns.m4 notsticky.m4 \
		nouucp.m4 nullclient.m4 preserve_local_plus_detail.m4 \
		preserve_luser_host.m4 promiscuous_relay.m4 queuegroup.m4 \
		redirect.m4 relay_based_on_MX.m4 relay_entire_domain.m4 \
		relay_hosts_only.m4 relay_local_from.m4 \
		relay_mail_from.m4 smrsh.m4 stickyhost.m4 use_ct_file.m4 \
		use_cw_file.m4 uucpdomain.m4 virtuser_entire_domain.m4 \
		virtusertable.m4
