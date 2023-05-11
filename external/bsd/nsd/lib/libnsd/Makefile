# $NetBSD: Makefile,v 1.4 2023/05/11 16:18:31 christos Exp $

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
ixfr.c \
ixfrcreate.c \
lookup3.c \
namedb.c \
nsec3.c \
options.c \
packet.c \
popen3.c \
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
udbradtree.c \
udbzone.c \
util.c \
verify.c

SRCS+=\
b64_ntop.c \
b64_pton.c

LIBDPLIBS+= ${DPLIBS}

.include <bsd.lib.mk>

COPTS.tsig-openssl.c+= -Wno-error=deprecated-declarations

# Use the generated files
.y.c:
