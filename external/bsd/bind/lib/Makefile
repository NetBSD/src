#	$NetBSD: Makefile,v 1.1.16.2 2017/04/25 22:02:04 snj Exp $

# XXX These bind9/ISC libraries should depend on each other but the
# XXX dependency ordering requires considerable investigation.
# XXX Please preserve the order below so we know the order ISC links
# XXX them in in their Makefiles.
SUBDIR+= libisc liblwres .WAIT libdns libisccc .WAIT libisccfg .WAIT libbind9 \
	libirs

.include <bsd.subdir.mk>
