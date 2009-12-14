## $NetBSD: Makefile.am,v 1.4 2009/12/14 23:29:56 agc Exp $

AM_CFLAGS		= $(WARNCFLAGS)

lib_LTLIBRARIES		= libnetpgp.la

libnetpgp_la_CPPFLAGS	= -I$(top_srcdir)/include

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

man3_MANS		= libnetpgp.3

dist_man_MANS		= libnetpgp.3
