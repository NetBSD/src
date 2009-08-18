#as: -m750cl
#objdump: -dr -Mppcps
#name: PPC750CL paired single tests

.*: +file format elf(32)?(64)?-powerpc.*

Disassembly of section \.text:

0+0000000 <start>:
   0:	e0 03 d0 04 	psq_l   f0,4\(r3\),1,5
   4:	e4 22 30 08 	psq_lu  f1,8\(r2\),0,3
   8:	10 45 25 4c 	psq_lux f2,r5,r4,1,2
   c:	10 62 22 8c 	psq_lx  f3,r2,r4,0,5
  10:	f0 62 30 08 	psq_st  f3,8\(r2\),0,3
  14:	f4 62 70 08 	psq_stu f3,8\(r2\),0,7
  18:	10 43 22 ce 	psq_stux f2,r3,r4,0,5
  1c:	10 c7 46 0e 	psq_stx f6,r7,r8,1,4
  20:	10 a0 3a 10 	ps_abs  f5,f7
  24:	10 a0 3a 11 	ps_abs. f5,f7
  28:	10 22 18 2a 	ps_add  f1,f2,f3
  2c:	10 22 18 2b 	ps_add. f1,f2,f3
  30:	11 82 20 40 	ps_cmpo0 cr3,f2,f4
  34:	11 82 20 c0 	ps_cmpo1 cr3,f2,f4
  38:	11 82 20 00 	ps_cmpu0 cr3,f2,f4
  3c:	11 82 20 80 	ps_cmpu1 cr3,f2,f4
  40:	10 44 30 24 	ps_div  f2,f4,f6
  44:	10 44 30 25 	ps_div. f2,f4,f6
  48:	10 01 18 ba 	ps_madd f0,f1,f2,f3
  4c:	10 01 18 bb 	ps_madd. f0,f1,f2,f3
  50:	10 22 20 dc 	ps_madds0 f1,f2,f3,f4
  54:	10 22 20 dd 	ps_madds0. f1,f2,f3,f4
  58:	10 22 20 de 	ps_madds1 f1,f2,f3,f4
  5c:	10 22 20 df 	ps_madds1. f1,f2,f3,f4
  60:	10 44 34 20 	ps_merge00 f2,f4,f6
  64:	10 44 34 21 	ps_merge00. f2,f4,f6
  68:	10 44 34 60 	ps_merge01 f2,f4,f6
  6c:	10 44 34 61 	ps_merge01. f2,f4,f6
  70:	10 44 34 a0 	ps_merge10 f2,f4,f6
  74:	10 44 34 a1 	ps_merge10. f2,f4,f6
  78:	10 44 34 e0 	ps_merge11 f2,f4,f6
  7c:	10 44 34 e1 	ps_merge11. f2,f4,f6
  80:	10 60 28 90 	ps_mr   f3,f5
  84:	10 60 28 91 	ps_mr.  f3,f5
  88:	10 44 41 b8 	ps_msub f2,f4,f6,f8
  8c:	10 44 41 b9 	ps_msub. f2,f4,f6,f8
  90:	10 43 01 72 	ps_mul  f2,f3,f5
  94:	10 43 01 73 	ps_mul. f2,f3,f5
  98:	10 64 01 d8 	ps_muls0 f3,f4,f7
  9c:	10 64 01 d9 	ps_muls0. f3,f4,f7
  a0:	10 64 01 da 	ps_muls1 f3,f4,f7
  a4:	10 64 01 db 	ps_muls1. f3,f4,f7
  a8:	10 20 29 10 	ps_nabs f1,f5
  ac:	10 20 29 11 	ps_nabs. f1,f5
  b0:	10 20 28 50 	ps_neg  f1,f5
  b4:	10 20 28 51 	ps_neg. f1,f5
  b8:	10 23 39 7e 	ps_nmadd f1,f3,f5,f7
  bc:	10 23 39 7f 	ps_nmadd. f1,f3,f5,f7
  c0:	10 23 39 7c 	ps_nmsub f1,f3,f5,f7
  c4:	10 23 39 7d 	ps_nmsub. f1,f3,f5,f7
  c8:	11 20 18 30 	ps_res  f9,f3
  cc:	11 20 18 31 	ps_res. f9,f3
  d0:	11 20 18 34 	ps_rsqrte f9,f3
  d4:	11 20 18 35 	ps_rsqrte. f9,f3
  d8:	10 22 20 ee 	ps_sel  f1,f2,f3,f4
  dc:	10 22 20 ef 	ps_sel. f1,f2,f3,f4
  e0:	10 ab 10 28 	ps_sub  f5,f11,f2
  e4:	10 ab 10 29 	ps_sub. f5,f11,f2
  e8:	10 45 52 54 	ps_sum0 f2,f5,f9,f10
  ec:	10 45 52 55 	ps_sum0. f2,f5,f9,f10
  f0:	10 45 52 56 	ps_sum1 f2,f5,f9,f10
  f4:	10 45 52 57 	ps_sum1. f2,f5,f9,f10
  f8:	10 03 2f ec 	dcbz_l  r3,r5
