#	$NetBSD: Makefile,v 1.24 2003/03/02 20:12:16 jhawk Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

NOOBJ=	# defined

FILES=	acronyms airport ascii birthtoken bsd-family-tree country domains \
	eqnchar flowers indent.pro inter.phone language man.template \
	mdoc.template na.phone operator style zipcodes
FILESDIR=${BINDIR}/misc


# According to iana@ISI.EDU, the URL below is the authoritative list
# distributed by the ISO 3166 Maintenance Agency.
update-domains:
	( \
	  mkdir /tmp/domains.$$$$; cd /tmp/domains.$$$$ ; \
	  ftp -o - http://www.iana.org/cctld/cctld-whois.htm | \
	  tr -d \\015 | \
	  sed -f ${.CURDIR}/domains.sed > list ;\
	  echo '# $$''NetBSD: $$' ;  \
	  echo '# ISO 3166 country codes, from http://www.iana.org/cctld/cctld-whois.htm' ; \
	  cat top ; echo '#' ; cat list ; \
	 ) > domains ; \
	rm -r /tmp/domains.$$$$

.include <bsd.prog.mk>
