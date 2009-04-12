#	$NetBSD: Makefile,v 1.1 2009/04/12 03:46:10 christos Exp $

# XXX These bind9/ISC libraries should depend on each other but the
# XXX dependency ordering requires considerable investigation.
# XXX Please preserve the order below so we know the order ISC links
# XXX them in in their Makefiles.
SUBDIR+= libbind9 libdns liblwres libisccfg libisccc libisc

.include <bsd.subdir.mk>
