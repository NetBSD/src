#	$NetBSD: Makefile,v 1.40 2023/04/30 22:32:51 nikita Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

FILES=	NetBSD.el acronyms acronyms-o acronyms.comp airport ascii \
	birthtoken bsd-family-tree \
	country domains operator \
	eqnchar flowers indent.pro inter.phone language man.template \
	mdoc.template na.phone na.postal style zipcodes
FILESDIR=${BINDIR}/misc

# TO AVOID INSTALLING THE POTENTIALLY OFFENSIVE FORTUNES, RUN 'make' with
# "INSTALL_OFFENSIVE_FORTUNES=NO", or set "INSTALL_OFFENSIVE_FORTUNES=NO"
# IN bsd.own.mk OR /etc/mk.conf.

INSTALL_OFFENSIVE_FORTUNES?=    YES
.if (${INSTALL_OFFENSIVE_FORTUNES} == "YES")
TYPE=	real
.else
TYPE=	fake
.endif
CLEANFILES+=acronyms-o
FILESBUILD_acronyms-o=yes

# According to iana@ISI.EDU, the URL below is the authoritative list
# distributed by the ISO 3166 Maintenance Agency.
DOMAIN_URL=	https://www.iana.org/domains/root/db 
update-domains:
	( \
	  echo '# $$''NetBSD''$$' ;  \
	  echo '# ISO 3166 country codes, from ${DOMAIN_URL}' ; \
	  echo '# Last update:' `date` ; \
	  echo '#' ; \
	  ftp -o - ${DOMAIN_URL} \
	  | ${TOOL_SED} -f ${.CURDIR}/nanpa.sed \
	  | ${TOOL_SED} '/^\..*/!d' \
	  | ${TOOL_SED} 's/&quot;/\"/g' \
	  | ${TOOL_AWK} 'BEGIN{FS=":"} {print $$1 " " $$3}' \
	 ) > domains ; \

update-na.phone:
	< /dev/null ${TOOL_AWK} -f nanpa.awk > na.phone

update-country:
	${.CURDIR}/make.country > country


.include <bsd.prog.mk>

acronyms-o: acronyms-o.${TYPE}
	@cp ${.ALLSRC} ${.TARGET}
