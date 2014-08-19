# $NetBSD: Makefile,v 1.2.12.1 2014/08/19 23:45:22 tls Exp $

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILESDIR=/usr/share/examples/libsaslc/mech
FILESOWN=root
FILESGRP=wheel
FILESMODE=444

FILES=
FILES+= ANONYMOUS.conf
FILES+= CRAM-MD5.conf
FILES+= DIGEST-MD5.conf
FILES+= EXTERNAL.conf
FILES+= GSSAPI.conf
FILES+= LOGIN.conf
FILES+= PLAIN.conf
.endif

.include <bsd.prog.mk>
