#	$NetBSD: Makefile,v 1.3 2022/07/12 02:43:46 mrg Exp $

# This makefile is used to generate the libxcb xml/python generated files.
# For now it is broken, and these files were copied from a normal libxcb
# build.

.include <bsd.own.mk>
.include "Makefile.xmlsrcs"

PYTHON?=	/usr/pkg/bin/python3.9
PYTHONLIB?=	/usr/pkg/lib/python3.9
C_CLIENT?=	${X11SRCDIR.xcb}/src/c_client.py

.SUFFIXES: .c .xml

# Set this to a r/w location if using r/o sources.
NEWCONFIGDIR?=  ${.CURDIR}

# this doesn't work yet - needs properly install "xcbgen".
# these files are copied from a normal build.
.xml.c:
	( \
	cd ${NEWCONFIGDIR}; \
	env PYTHONPATH=${X11SRCDIR.xcb-proto} ${PYTHON} ${C_CLIENT} \
		-c "libxcb 1.15" \
		-l "X Version 11" \
		-s 3 \
		-p ${PYTHONLIB}/site-packages ${.IMPSRC} \
	)

generate_from_xml: ${XMLSRCS}

all: generate_from_xml

.include <bsd.subdir.mk>

.PATH: ${X11SRCDIR.xcb-proto}/src
