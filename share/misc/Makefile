#	$NetBSD: Makefile,v 1.15 1999/01/26 04:35:19 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

FILES=	airport ascii birthtoken domains eqnchar flowers getopt \
	indent.pro inter.phone man.template mdoc.template na.phone \
	operator style zipcodes
FILESDIR=${BINDIR}/misc
NOOBJ=	noobj


# According to iana@ISI.EDU, the URL below is the authoritative list
# distributed by the ISO 3166 Maintenance Agency.
update-domains:
	ftp -o /tmp/domains.$$$$ http://www.iana.org/country-codes.txt ; \
	( \
	echo '# $$NetBSD: Makefile,v 1.15 1999/01/26 04:35:19 thorpej Exp $$' ;  \
	echo '# ISO 3166 country codes, from http://www.iana.org/country-codes.txt' ;  \
	echo '# '`grep ^Latest /tmp/domains.$$$$` ;  \
	echo '#' ;  \
	perl -ane 'print lc(join(" ", $$F[$$#F-2], @F[0..$$#F-3])),"\n" if /^[A-Z]{2,}\s*.*\d{3}\s*$$/;' </tmp/domains.$$$$ \
	) >domains ; \
	rm /tmp/domains.$$$$
	

.include <bsd.prog.mk>
