#	$NetBSD: Makefile,v 1.21 2003/03/25 11:51:29 tron Exp $
#	install these sources in /usr/share/sendmail/cf

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist
.PATH: ${DIST}/sendmail/cf/cf

FILESDIR=	/usr/share/sendmail/cf
FILES=		boat-anchor.mc chimera.mc ihack.net.mc minbar.mc \
		netbsd-proto.mc netbsd-msp.mc submit.mc sun-lamp.mc zhadum.mc \
		Makefile

.if (${.CURDIR} == ${FILESDIR})
CFDIR=		${.CURDIR}/..
.else
CFDIR?=		${DIST}/sendmail/cf
.endif
CHMOD=		chmod
M4?=		m4
ROMODE=		444
RM=		rm -f

ALL=		netbsd-proto.cf netbsd-msp.cf

OTHERS=		${FILES:S/Makefile//:C/netbsd-(proto|msp).*//:S/.mc/.cf/g}

.SUFFIXES: .mc .cf

.mc.cf:
	${RM} $@
	(cd ${CFDIR}/cf && \
	    ${M4} ${CFDIR}/m4/cf.m4 ${.CURDIR}/${@:R}.mc > ${.OBJDIR}/$@)
	${CHMOD} ${ROMODE} $@

realall: ${ALL}

afterinstall:
	${CHMOD} ${ROMODE} ${DESTDIR}${FILESDIR}/netbsd-proto.mc
	${CHMOD} ${ROMODE} ${DESTDIR}${FILESDIR}/netbsd-msp.mc

# XXX not friendly with "make obj"
others: ${OTHERS}

clean cleandir distclean:
	${RM} ${ALL} core

realdepend:

distribution: netbsd-proto.cf netbsd-msp.cf
	${INSTALL_FILE} -o root -g wheel -m 444 \
	    netbsd-proto.cf ${DESTDIR}/etc/mail/sendmail.cf
	${INSTALL_FILE} -o root -g wheel -m 444 \
	    netbsd-msp.cf ${DESTDIR}/etc/mail/submit.cf

# this is overkill, but....
.include <../domain/Makefile.inc>
.include <../feature/Makefile.inc>
.include <../hack/Makefile.inc>
.include <../m4/Makefile.inc>
.include <../mailer/Makefile.inc>
.include <../ostype/Makefile.inc>
.include <../siteconfig/Makefile.inc>
M4FILES=	${CF_DOMAIN_FILES:@.cf.@${CFDIR}/domain/${.cf.}@} \
		${.CURDIR}/../domain/NetBSD.ORG.m4 \
		${.CURDIR}/../domain/ihack.net.m4 \
		${CF_FEATURE_FILES:@.cf.@${CFDIR}/feature/${.cf.}@} \
		${CF_HACK_FILES:@.cf.@${CFDIR}/hack/${.cf.}@} \
		${CF_M4_FILES:@.cf.@${CFDIR}/m4/${.cf.}@} \
		${CF_MAILER_FILES:@.cf.@${CFDIR}/mailer/${.cf.}@} \
		${CF_OSTYPE_FILES:@.cf.@${CFDIR}/ostype/${.cf.}@} \
		${CF_SITECONFIG_FILES:@.cf.@${CFDIR}/siteconfig/${.cf.}@}

${ALL}: ${M4FILES}

.include <bsd.prog.mk>
