# $NetBSD: Makefile,v 1.3.6.1 2014/05/22 13:21:31 yamt Exp $

.include <bsd.own.mk>

SUBDIR=mech

.if ${MKSHARE} != "no"
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
.endif

.include <bsd.files.mk>
.include <bsd.subdir.mk>
