#	$NetBSD: Makefile,v 1.2 1999/11/20 19:47:10 veego Exp $

SUBDIR=	lib dig dnskeygen dnsquery host named named-bootconf named-xfer \
	ndc nslookup nsupdate reload restart

all-dig all-dnskeygen all-dnsquery all-host all-named all-named-xfer \
	all-ndc all-nslookup all-nsupdate: all-lib
depend-dig depend-dnskeygen depend-dnsquery depend-host depend-named \
	depend-named-xfer depend-ndc depend-nslookup depend-nsupdate: depend-lib

# depend on named/pathname.h
depend-named-xfer depend-ndc: depend-named

VER= 8.2.2-P5

.include <bsd.subdir.mk>
