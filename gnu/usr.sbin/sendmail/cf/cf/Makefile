#	$NetBSD: Makefile,v 1.6.2.5 2002/02/21 00:24:15 he Exp $
#	install these sources in /usr/share/sendmail/cf

DIST=		${.CURDIR}/../../../../dist
.PATH: ${DIST}/sendmail/cf/cf

FILESDIR=	/usr/share/sendmail/cf
FILES=		boat-anchor.mc chimera.mc ihack.net.mc minbar.mc \
		netbsd-proto.mc sun-lamp.mc zhadum.mc Makefile

.if (${.CURDIR} == ${FILESDIR})
CFDIR=		${.CURDIR}/..
.else
CFDIR?=		${DIST}/sendmail/cf
.endif
CHMOD=		chmod
M4?=		m4
ROMODE=		444
RM=		rm -f

ALL=		netbsd-proto.cf

OTHERS=		${FILES:S/Makefile//:C/netbsd-proto.*//:S/.mc/.cf/g}

.SUFFIXES: .mc .cf

.mc.cf:
	${RM} $@
	(cd ${CFDIR}/cf && \
	    ${M4} ${CFDIR}/m4/cf.m4 ${.CURDIR}/${@:R}.mc > ${.OBJDIR}/$@)
	${CHMOD} ${ROMODE} $@

realall: ${ALL}

afterinstall:
	${CHMOD} ${ROMODE} ${DESTDIR}${FILESDIR}/netbsd-proto.mc

# XXX not friendly with "make obj"
others: ${OTHERS}

clean cleandir distclean:
	${RM} ${ALL} core

realdepend:

distribution: netbsd-proto.cf
	${INSTALL} ${INSTPRIV} ${COPY} -o root -g wheel -m 444 \
	    netbsd-proto.cf ${DESTDIR}/etc/mail/sendmail.cf

# this is overkill, but....
M4FILES=	${CFDIR}/domain/Berkeley.EDU.m4 \
		${CFDIR}/domain/CS.Berkeley.EDU.m4 \
		${CFDIR}/domain/EECS.Berkeley.EDU.m4 \
                ${CFDIR}/domain/S2K.Berkeley.EDU.m4 \
		${CFDIR}/domain/berkeley-only.m4 \
		${CFDIR}/domain/generic.m4 \
		${.CURDIR}/../domain/NetBSD.ORG.m4 \
		${.CURDIR}/../domain/ihack.net.m4 \
		${CFDIR}/feature/accept_unqualified_senders.m4 \
		${CFDIR}/feature/accept_unresolvable_domains.m4 \
		${CFDIR}/feature/access_db.m4 \
		${CFDIR}/feature/allmasquerade.m4 \
		${CFDIR}/feature/always_add_domain.m4 \
		${CFDIR}/feature/bestmx_is_local.m4 \
		${CFDIR}/feature/bitdomain.m4 \
		${CFDIR}/feature/blacklist_recipients.m4 \
		${CFDIR}/feature/dnsbl.m4 \
		${CFDIR}/feature/domaintable.m4 \
		${CFDIR}/feature/generics_entire_domain.m4 \
		${CFDIR}/feature/genericstable.m4 \
		${CFDIR}/feature/ldap_routing.m4 \
		${CFDIR}/feature/limited_masquerade.m4 \
		${CFDIR}/feature/local_lmtp.m4 \
		${CFDIR}/feature/local_procmail.m4 \
		${CFDIR}/feature/loose_relay_check.m4 \
		${CFDIR}/feature/mailertable.m4 \
		${CFDIR}/feature/masquerade_entire_domain.m4 \
		${CFDIR}/feature/masquerade_envelope.m4 \
		${CFDIR}/feature/no_default_msa.m4 \
		${CFDIR}/feature/nocanonify.m4 \
		${CFDIR}/feature/nodns.m4 \
		${CFDIR}/feature/notsticky.m4 \
		${CFDIR}/feature/nouucp.m4 \
		${CFDIR}/feature/nullclient.m4 \
		${CFDIR}/feature/promiscuous_relay.m4 \
		${CFDIR}/feature/rbl.m4 \
		${CFDIR}/feature/redirect.m4 \
		${CFDIR}/feature/relay_based_on_MX.m4 \
		${CFDIR}/feature/relay_entire_domain.m4 \
		${CFDIR}/feature/relay_hosts_only.m4 \
		${CFDIR}/feature/relay_local_from.m4 \
		${CFDIR}/feature/relay_mail_from.m4 \
		${CFDIR}/feature/smrsh.m4 \
		${CFDIR}/feature/stickyhost.m4 \
		${CFDIR}/feature/use_ct_file.m4 \
		${CFDIR}/feature/use_cw_file.m4 \
		${CFDIR}/feature/uucpdomain.m4 \
		${CFDIR}/feature/virtuser_entire_domain.m4 \
		${CFDIR}/feature/virtusertable.m4 \
		${CFDIR}/hack/cssubdomain.m4 \
		${CFDIR}/m4/cf.m4 \
		${CFDIR}/m4/cfhead.m4 \
		${CFDIR}/m4/proto.m4 \
		${CFDIR}/m4/version.m4 \
		${CFDIR}/mailer/cyrus.m4 \
		${CFDIR}/mailer/fax.m4 \
		${CFDIR}/mailer/local.m4 \
		${CFDIR}/mailer/mail11.m4 \
		${CFDIR}/mailer/phquery.m4\
		${CFDIR}/mailer/pop.m4 \
		${CFDIR}/mailer/procmail.m4 \
		${CFDIR}/mailer/qpage.m4 \
		${CFDIR}/mailer/smtp.m4 \
		${CFDIR}/mailer/usenet.m4 \
		${CFDIR}/mailer/uucp.m4 \
		${CFDIR}/ostype/bsd4.4.m4 \
		${CFDIR}/siteconfig/uucp.cogsci.m4 \
		${CFDIR}/siteconfig/uucp.old.arpa.m4 \
		${CFDIR}/siteconfig/uucp.ucbarpa.m4 \
		${CFDIR}/siteconfig/uucp.ucbvax.m4

${ALL}: ${M4FILES}

.include <bsd.prog.mk>
.include <bsd.files.mk>
