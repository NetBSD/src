# $Id: Makefile.am,v 1.1.1.1 2000/06/16 18:31:53 thorpej Exp $

include $(top_srcdir)/Makefile.am.common

INCLUDES += -I$(srcdir)/.. $(INCLUDE_krb4)

noinst_LIBRARIES = libtelnet.a

libtelnet_a_SOURCES = \
	auth-proto.h	\
	auth.c		\
	auth.h		\
	enc-proto.h	\
	enc_des.c	\
	encrypt.c	\
	encrypt.h	\
	genget.c	\
	kerberos.c	\
	kerberos5.c	\
	misc-proto.h	\
	misc.c		\
	misc.h

EXTRA_DIST = krb4encpwd.c rsaencpwd.c spx.c
