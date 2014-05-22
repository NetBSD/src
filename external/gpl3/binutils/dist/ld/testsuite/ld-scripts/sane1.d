# source: data.s
# ld: -T sane1.t
# nm: -B
# notarget: mmix-* pdp11-* rs6000-*-aix* tic30-*-aout
# mmix symbol sections are wrong, pdp sign extends 16-bit addresses
# rs6000-aix and tic30 don't like empty .text

#...
0+8004 D d1
0+8024 D d2
0+0020 A diff
0+0100 A e1
0+0080 A e2
0+8000 A e3
0+0090 A prod
0+8002 D s1
0+8001 D s2
0+8007 D s3
0+8002 A s4
0+0004 A s5
0+19a0 A s6
0+8020 D s_diff
0+8090 D s_prod
0+8028 D s_sum
0+8020 D s_sum_neg
0+0028 A sum
0+0020 A sum_neg
0+8002 D x1
0+8001 D x2
0+8007 D x3
0+8002 A x4
0+0004 A x5
0+19a0 A x6
