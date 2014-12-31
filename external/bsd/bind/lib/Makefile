#	$NetBSD: Makefile,v 1.1.16.1 2014/12/31 11:59:14 msaitoh Exp $

# XXX These bind9/ISC libraries should depend on each other but the
# XXX dependency ordering requires considerable investigation.
# XXX Please preserve the order below so we know the order ISC links
# XXX them in in their Makefiles.
SUBDIR+= libisc liblwres .WAIT libdns libisccc .WAIT libisccfg .WAIT libbind9 \
	libirs

.include <bsd.subdir.mk>
