# $NetBSD: Makefile,v 1.2.26.2 2020/04/21 19:37:55 martin Exp $

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
