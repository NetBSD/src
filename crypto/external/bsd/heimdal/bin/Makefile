# $NetBSD: Makefile,v 1.1.4.1 2014/05/22 13:21:23 yamt Exp $

.include <bsd.own.mk>

HEIMBASE=${.CURDIR}/..

.if (${MKKERBEROS} != "no")
SUBDIR =		\
	gsstool		\
	hxtool		\
	kcc		\
	kdestroy	\
	kgetcred	\
	kinit		\
	kpasswd		\
	krb5-config	\
	kvno		\
	string2key	\
	verify_krb5_conf

.endif	# MKKERBEROS != no

.include <bsd.subdir.mk>
