#	$NetBSD: Makefile,v 1.20.46.1 2014/08/10 06:53:13 tls Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 5/10/94

.include <bsd.own.mk>

# The following modules do not build/install:
# 10.named, 13.amd

# Missing from 4.4BSD-Lite:
# 14.uucpimpl 15.uucpnet 16.security

SUBDIR=	01.setup config 04.quotas 05.fastfs 06.nfs 17.password 18.net
.if exists(03.fsck_ffs)
SUBDIR+= 03.fsck_ffs
.endif
.if exists(07.lpd)
SUBDIR+= 07.lpd
.endif
.if exists(08.sendmailop)
SUBDIR+= 08.sendmailop
.endif
.if exists(09.sendmail)
SUBDIR+= 09.sendmail
.endif
#SUBDIR=	01.setup 02.config 03.fsck 04.quotas 05.fastfs 06.nfs 07.lpd \
#	08.sendmailop 09.sendmail 14.uucpimpl \
#	15.uucpnet 16.security 17.password 18.net 19.perl

SECTION=smm
SUBARTICLES=title contents
SRCS.title=Title
SRCS.contents=00.contents
MACROS=-ms

.include <bsd.doc.mk>
.include <bsd.subdir.mk>
