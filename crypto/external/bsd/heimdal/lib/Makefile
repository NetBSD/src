# $NetBSD: Makefile,v 1.1 2011/04/13 19:16:47 elric Exp $

.include "bsd.own.mk"

HEIMBASE=${.CURDIR}/..

.if (${MKKERBEROS} != "no")
SUBDIR = \
	libheimbase	\
	libcom_err	\
	libroken	\
	libvers		\
	.WAIT		\
	libsl		\
	.WAIT		\
	libasn1		\
	libwind		\
	.WAIT		\
	libhx509	\
	libipc		\
	.WAIT		\
	libkrb5		\
	.WAIT		\
	libhdb		\
	libkafs		\
	libheimntlm	\
	.WAIT		\
	libkdc		\
	libgssapi	\
	libkadm5srv	\
	libkadm5clnt

.endif	# MKKERBEROS != no

.include <bsd.subdir.mk>
