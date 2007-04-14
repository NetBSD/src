#	$NetBSD: Makefile,v 1.13 2007/04/14 20:50:48 martin Exp $
#
# (C)opyright 1993-1996 by Darren Reed.
#
# See the IPFILTER.LICENCE file for details on licencing.
#
.include <bsd.prog.mk>

regress all: results tests

results:
	mkdir -p results

tests: ipf nat logtests ipv6 pools

ipf: ftests ptests

nat: ntests nitests intests

first:
	-mkdir -p results

# Filtering tests
ftests: f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f12 f13 f14 f15 f16 f17 f18 f19 f20

# Rule parsing tests
ptests: i1 i2 i3 i4 i5 i6 i7 i8 i9 i10 i11 i12 i13 i14 i15 i16 i17 i18 i19 \
	i20 i21

ntests: n1 n2 n3 n4 n5 n6 n7 n8 n9 n10 n11 n12 n13 n14

nitests: ni1 ni2 ni3 ni4 ni5 ni6 ni7 ni8 ni9 ni10 ni11 ni12 ni13 ni14 ni15 \
	ni16 ni19 ni20 ni21

intests: in1 in2 in3 in4 in5 in6

logtests: l1

pools: p1 p2 p3 p5 ip1 ip2

ipv6: ipv6.1 ipv6.2 ipv6.3

bpf: bpf1 bpf-f1

f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f12 f13 f14 f19:
	@${HOST_SH} ${.CURDIR}/dotest ${.CURDIR} \
		`${AWK} "/^$@ / { print; } " ${.CURDIR}/test.format`

f15 f16 f17 f18 f20:
	@${HOST_SH} ${.CURDIR}/mtest ${.CURDIR} \
		`${AWK} "/^$@ / { print; } " ${.CURDIR}/test.format`

i1 i2 i3 i4 i5 i6 i7 i8 i9 i10 i11 i12 i13 i14 i15 i16 i17 i18 i19 i20 i21 bpf1:
	@${HOST_SH} ${.CURDIR}/itest ${.CURDIR} \
		`${AWK} "/^$@ / { print; } " ${.CURDIR}/test.format`

n1 n2 n3 n4 n5 n6 n7 n8 n9 n10 n11 n12 n13 n14:
	@${HOST_SH} ${.CURDIR}/nattest ${.CURDIR} \
		`${AWK} "/^$@ / { print; } " ${.CURDIR}/test.format`

ni2 ni3 ni4 ni5 ni7 ni8 ni9 ni10 ni11 ni12 ni13 ni14 ni15 ni16 ni19 ni20:
	@${HOST_SH} ${.CURDIR}/natipftest ${.CURDIR} single \
		`${AWK} "/^$@ / { print; } " ${.CURDIR}/test.format`

ni1 ni6 ni21 ni23:
	@${HOST_SH} ${.CURDIR}/natipftest ${.CURDIR} multi \
		`${AWK} "/^$@ / { print; } " ${.CURDIR}/test.format`

in1 in2 in3 in4 in5 in6:
	@${HOST_SH} ${.CURDIR}/intest ${.CURDIR} \
		`${AWK} "/^$@ / { print; } " ${.CURDIR}/test.format`

l1:
	@${HOST_SH} ${.CURDIR}/logtest ${.CURDIR} \
		`${AWK} "/^$@ / { print; } " ${.CURDIR}/test.format`

ipv6.1 ipv6.2 ipv6.3:
	@${HOST_SH} ${.CURDIR}/dotest6 ${.CURDIR} \
		`${AWK} "/^$@ / { print; } " ${.CURDIR}/test.format`

p1 p2 p3 p5:
	@${HOST_SH} ${.CURDIR}/ptest ${.CURDIR} \
		`${AWK} "/^$@ / { print; } " ${.CURDIR}/test.format`

ip1 ip2:
	@${HOST_SH} ${.CURDIR}/iptest ${.CURDIR} \
		`${AWK} "/^$@ / { print; } " ${.CURDIR}/test.format`

bpf-f1:
	@${HOST_SH} ${.CURDIR}/bpftest ${.CURDIR} \
		`${AWK} "/^$@ / { print; } " ${.CURDIR}/test.format`

clean:
	/bin/rm -f f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f12 f13 f14 f15 f16 f17 f18 f19 f20
	/bin/rm -f i1 i2 i3 i4 i5 i6 i7 i8 i9 i10 i11 i12 i13 i14 i15 i16 i17 i18 i19 i20 i21
	/bin/rm -f n1 n2 n3 n4 n5 n6 n7 n8 n9 n10 n11 n12 n13 n14
	/bin/rm -f ni1 ni2 ni3 ni4 ni5 ni6 ni7 ni8 ni9
	/bin/rm -f ni10 ni11 ni12 ni13 ni14 ni15 ni16 ni19 ni20 ni21 ni23
	/bin/rm -f in1 in2 in3 in4 in5 in6
	/bin/rm -f p1 p2 p3 p5 ip1 ip2
	/bin/rm -f l1
	/bin/rm -f ipv6.1 ipv6.2 ipv6.3
	/bin/rm -f bpf1 bpf-f1
	/bin/rm -f results/* logout
	/bin/rm -rf results

diffs:
	-cd expected; for i in *; do if [ -f $$i -a ! -f ../$$i -a -f ../results/$$i ] ; then  diff -c $$i ../results/$$i >> ../diff.out; fi done
