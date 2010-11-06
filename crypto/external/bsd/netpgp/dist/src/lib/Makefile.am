## $NetBSD: Makefile.am,v 1.8 2010/11/06 00:03:32 agc Exp $

AM_CFLAGS		= $(WARNCFLAGS)

lib_LTLIBRARIES		= libnetpgp.la

libnetpgp_la_CPPFLAGS	= -I$(top_srcdir)/include -I../libmj

libnetpgp_la_SOURCES	= \
	bufgap.c \
	compress.c \
	create.c \
	crypto.c \
	fastctype.c \
	keyring.c \
	misc.c \
	netpgp.c \
	openssl_crypto.c \
	packet-parse.c \
	packet-print.c \
	packet-show.c \
	reader.c \
	signature.c \
	ssh2pgp.c \
	symmetric.c \
	validate.c \
	writer.c

LDADD			= ../libmj/libmj.a

dist_man_MANS		= libnetpgp.3
