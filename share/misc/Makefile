#	$NetBSD: Makefile,v 1.13 1999/01/05 01:53:09 hubertf Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

FILES=	airport ascii birthtoken domains eqnchar flowers getopt \
	inter.phone man.template mdoc.template na.phone operator style \
	zipcodes
FILESDIR=${BINDIR}/misc
NOOBJ=	noobj


# According to iana@ISI.EDU, the URL below is the authoritative list
# distributed by the ISO 3166 Maintenance Agency.
update-domains:
	ftp -o /tmp/domains.$$$$ http://www.iana.org/country-codes.txt ; \
	( \
	echo '# $$NetBSD: Makefile,v 1.13 1999/01/05 01:53:09 hubertf Exp $$' ;  \
	echo '# ISO 3166 country codes, from http://www.iana.org/country-codes.txt' ;  \
	echo '# '`grep ^Latest /tmp/domains.$$$$` ;  \
	echo '#' ;  \
	awk </tmp/domains.$$$$ '{ if ( $$2 ~ /[A-Z][A-Z]/ && $$4 ~ /[0-9][0-9][0-9]/ ) { x=tolower($$2 " "$$1); print x } };' \
	) >domains ; \
	rm /tmp/domains.$$$$
	

.include <bsd.prog.mk>
