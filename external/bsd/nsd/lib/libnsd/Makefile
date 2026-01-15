# $NetBSD: Makefile,v 1.7 2026/01/15 21:35:19 christos Exp $

LIBISPRIVATE=yes
.include <bsd.init.mk>

SIMDZONE=${NSD}/simdzone/src
.PATH: ${NSD}/compat ${NSD}/util ${SIMDZONE} ${SIMDZONE}/fallback
CPPFLAGS+=-I${SIMDZONE}

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
ixfr.c \
ixfrcreate.c \
lookup3.c \
namedb.c \
nsec3.c \
options.c \
packet.c \
parser.c \
popen3.c \
proxy_protocol.c \
query.c \
radtree.c \
radtree.c \
rbtree.c \
rdata.c \
region-allocator.c \
rrl.c \
siphash.c \
tsig-openssl.c \
tsig.c \
udb.c \
util.c \
verify.c \
zone.c

SRCS+=\
b64_ntop.c \
b64_pton.c

LIBDPLIBS+= ${DPLIBS}

.include <bsd.lib.mk>

COPTS.tsig-openssl.c+= -Wno-error=deprecated-declarations
COPTS.iterated_hash.c+= -Wno-error=deprecated-declarations


# Use the generated files
.y.c:
