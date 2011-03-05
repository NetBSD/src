# $NetBSD: Makefile,v 1.3.2.2 2011/03/05 15:08:33 bouyer Exp $

.include <bsd.own.mk>

SUBDIR=mech

FILESDIR=/usr/share/examples/libsaslc
FILESOWN=root
FILESGRP=wheel
FILESMODE=444

FILES= saslc.conf
FILES+= README
CFG_MECH=
CFG_MECH+= ANONYMOUS.conf
CFG_MECH+= CRAM-MD5.conf
CFG_MECH+= DIGEST-MD5.conf
CFG_MECH+= EXTERNAL.conf
CFG_MECH+= GSSAPI.conf
CFG_MECH+= LOGIN.conf
CFG_MECH+= PLAIN.conf

.include <bsd.files.mk>
.include <bsd.subdir.mk>
