#	$NetBSD: Makefile,v 1.1.1.1 1996/03/09 00:01:31 phil Exp $

LIB=	iostream

SRCS=	PlotFile.cc SFile.cc builtinbuf.cc editbuf.cc filebuf.cc fstream.cc \
indstream.cc ioassign.cc ioextend.cc iomanip.cc iostream.cc isgetline.cc \
isgetsb.cc isscan.cc osform.cc parsestream.cc pfstream.cc procbuf.cc \
sbform.cc sbgetline.cc sbscan.cc stdiostream.cc stdstrbufs.cc stdstreams.cc \
stream.cc streambuf.cc strstream.cc cleanup.c filedoalloc.c fileops.c \
floatconv.c genops.c iofclose.c iofgetpos.c iofread.c iofscanf.c \
iofsetpos.c iogetdelim.c iogetline.c ioignore.c iopadn.c iopopen.c \
ioprims.c ioprintf.c ioseekoff.c ioseekpos.c iostrerror.c ioungetc.c \
iovfprintf.c iovfscanf.c outfloat.c strops.c 

CXXFLAGS+=	-nostdinc++ -I$(.CURDIR)/../include \
		  -I$(.CURDIR)/../libio 
CFLAGS+=	-I$(.CURDIR) -I$(.CURDIR)/../libio
NOMAN=		noman

.PATH: $(.CURDIR)/../libio

.include <bsd.lib.mk>
