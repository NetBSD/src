## $Id: Makefile.am,v 1.1 2009/05/26 04:27:29 lukem Exp $

AM_CFLAGS		= $(WARNCFLAGS)

lib_LTLIBRARIES		= libnetpgp.la

libnetpgp_la_LDFLAGS	= -static

libnetpgp_la_CPPFLAGS	= -I$(top_srcdir)/include

libnetpgp_la_SOURCES	= \
	compress.c \
	create.c \
	crypto.c \
	keyring.c \
	misc.c \
	netpgp.c \
	openssl_crypto.c \
	packet-parse.c \
	packet-print.c \
	packet-show.c \
	reader.c \
	signature.c \
	symmetric.c \
	validate.c \
	writer.c

