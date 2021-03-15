# $NetBSD: Makefile,v 1.2 2021/03/15 19:48:51 christos Exp $

LIBISPRIVATE=yes
.include <bsd.init.mk>

.PATH: ${NSD}/compat

CPPFLAGS+=-I${NSD} -I${NSD}/../include

LIB=nsd
SRCS=\
answer.c \
axfr.c \
bitset.c \
buffer.c \
configlexer.c \
configparser.c \
dname.c \
dns.c \
edns.c \
iterated_hash.c \
lookup3.c \
namedb.c \
nsec3.c \
options.c \
packet.c \
query.c \
radtree.c \
radtree.c \
rbtree.c \
rdata.c \
region-allocator.c \
rrl.c \
tsig-openssl.c \
tsig.c \
udb.c \
udbradtree.c \
udbzone.c \
util.c

SRCS+=\
b64_ntop.c \
b64_pton.c

LIBDPLIBS+= ${DPLIBS}

.include <bsd.lib.mk>

# Use the generated files
.y.c:
