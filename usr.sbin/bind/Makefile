#	$NetBSD: Makefile,v 1.4 2000/04/28 06:21:20 itojun Exp $

SUBDIR=	lib dig dnskeygen dnsquery host named named-bootconf named-xfer \
	ndc nslookup nsupdate reload restart doc

all-dig all-dnskeygen all-dnsquery all-host all-named all-named-xfer \
	all-ndc all-nslookup all-nsupdate: all-lib
depend-dig depend-dnskeygen depend-dnsquery depend-host depend-named \
	depend-named-xfer depend-ndc depend-nslookup depend-nsupdate: depend-lib

# depend on named/pathname.h
depend-named-xfer depend-ndc: depend-named
all-named-xfer all-ndc: all-named

VER= 8.2.2-P5

.include <bsd.subdir.mk>
