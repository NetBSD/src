#objdump: -dr
#as: --defsym DIRECTIVE=1
#source: rdma.s

.*:     file format .*


Disassembly of section \.text:

0000000000000000 <.*>:
   0:	2e428420 	sqrdmlah	v0\.4h, v1\.4h, v2\.4h
   4:	6e428420 	sqrdmlah	v0\.8h, v1\.8h, v2\.8h
   8:	2e828420 	sqrdmlah	v0\.2s, v1\.2s, v2\.2s
   c:	6e828420 	sqrdmlah	v0\.4s, v1\.4s, v2\.4s
  10:	2e428c20 	sqrdmlsh	v0\.4h, v1\.4h, v2\.4h
  14:	6e428c20 	sqrdmlsh	v0\.8h, v1\.8h, v2\.8h
  18:	2e828c20 	sqrdmlsh	v0\.2s, v1\.2s, v2\.2s
  1c:	6e828c20 	sqrdmlsh	v0\.4s, v1\.4s, v2\.4s
  20:	7e828420 	sqrdmlah	s0, s1, s2
  24:	7e428420 	sqrdmlah	h0, h1, h2
  28:	7e828c20 	sqrdmlsh	s0, s1, s2
  2c:	7e428c20 	sqrdmlsh	h0, h1, h2
  30:	2f42d020 	sqrdmlah	v0\.4h, v1\.4h, v2\.h\[0\]
  34:	2f52d020 	sqrdmlah	v0\.4h, v1\.4h, v2\.h\[1\]
  38:	2f62d020 	sqrdmlah	v0\.4h, v1\.4h, v2\.h\[2\]
  3c:	2f72d020 	sqrdmlah	v0\.4h, v1\.4h, v2\.h\[3\]
  40:	6f42d020 	sqrdmlah	v0\.8h, v1\.8h, v2\.h\[0\]
  44:	6f52d020 	sqrdmlah	v0\.8h, v1\.8h, v2\.h\[1\]
  48:	6f62d020 	sqrdmlah	v0\.8h, v1\.8h, v2\.h\[2\]
  4c:	6f72d020 	sqrdmlah	v0\.8h, v1\.8h, v2\.h\[3\]
  50:	2f82d020 	sqrdmlah	v0\.2s, v1\.2s, v2\.s\[0\]
  54:	2fa2d020 	sqrdmlah	v0\.2s, v1\.2s, v2\.s\[1\]
  58:	2f82d820 	sqrdmlah	v0\.2s, v1\.2s, v2\.s\[2\]
  5c:	2fa2d820 	sqrdmlah	v0\.2s, v1\.2s, v2\.s\[3\]
  60:	6f82d020 	sqrdmlah	v0\.4s, v1\.4s, v2\.s\[0\]
  64:	6fa2d020 	sqrdmlah	v0\.4s, v1\.4s, v2\.s\[1\]
  68:	6f82d820 	sqrdmlah	v0\.4s, v1\.4s, v2\.s\[2\]
  6c:	6fa2d820 	sqrdmlah	v0\.4s, v1\.4s, v2\.s\[3\]
  70:	2f42f020 	sqrdmlsh	v0\.4h, v1\.4h, v2\.h\[0\]
  74:	2f52f020 	sqrdmlsh	v0\.4h, v1\.4h, v2\.h\[1\]
  78:	2f62f020 	sqrdmlsh	v0\.4h, v1\.4h, v2\.h\[2\]
  7c:	2f72f020 	sqrdmlsh	v0\.4h, v1\.4h, v2\.h\[3\]
  80:	6f42f020 	sqrdmlsh	v0\.8h, v1\.8h, v2\.h\[0\]
  84:	6f52f020 	sqrdmlsh	v0\.8h, v1\.8h, v2\.h\[1\]
  88:	6f62f020 	sqrdmlsh	v0\.8h, v1\.8h, v2\.h\[2\]
  8c:	6f72f020 	sqrdmlsh	v0\.8h, v1\.8h, v2\.h\[3\]
  90:	2f82f020 	sqrdmlsh	v0\.2s, v1\.2s, v2\.s\[0\]
  94:	2fa2f020 	sqrdmlsh	v0\.2s, v1\.2s, v2\.s\[1\]
  98:	2f82f820 	sqrdmlsh	v0\.2s, v1\.2s, v2\.s\[2\]
  9c:	2fa2f820 	sqrdmlsh	v0\.2s, v1\.2s, v2\.s\[3\]
  a0:	6f82f020 	sqrdmlsh	v0\.4s, v1\.4s, v2\.s\[0\]
  a4:	6fa2f020 	sqrdmlsh	v0\.4s, v1\.4s, v2\.s\[1\]
  a8:	6f82f820 	sqrdmlsh	v0\.4s, v1\.4s, v2\.s\[2\]
  ac:	6fa2f820 	sqrdmlsh	v0\.4s, v1\.4s, v2\.s\[3\]
  b0:	7f42d020 	sqrdmlah	h0, h1, v2\.h\[0\]
  b4:	7f52d020 	sqrdmlah	h0, h1, v2\.h\[1\]
  b8:	7f62d020 	sqrdmlah	h0, h1, v2\.h\[2\]
  bc:	7f72d020 	sqrdmlah	h0, h1, v2\.h\[3\]
  c0:	7f82d020 	sqrdmlah	s0, s1, v2\.s\[0\]
  c4:	7fa2d020 	sqrdmlah	s0, s1, v2\.s\[1\]
  c8:	7f82d820 	sqrdmlah	s0, s1, v2\.s\[2\]
  cc:	7fa2d820 	sqrdmlah	s0, s1, v2\.s\[3\]
  d0:	7f42f020 	sqrdmlsh	h0, h1, v2\.h\[0\]
  d4:	7f52f020 	sqrdmlsh	h0, h1, v2\.h\[1\]
  d8:	7f62f020 	sqrdmlsh	h0, h1, v2\.h\[2\]
  dc:	7f72f020 	sqrdmlsh	h0, h1, v2\.h\[3\]
  e0:	7f82f020 	sqrdmlsh	s0, s1, v2\.s\[0\]
  e4:	7fa2f020 	sqrdmlsh	s0, s1, v2\.s\[1\]
  e8:	7f82f820 	sqrdmlsh	s0, s1, v2\.s\[2\]
  ec:	7fa2f820 	sqrdmlsh	s0, s1, v2\.s\[3\]
