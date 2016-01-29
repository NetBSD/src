#objdump: -dr --prefix-addresses --show-raw-insn
#name: UDF
#error-output: udf.l

.*: +file format .*arm.*

Disassembly of section \.text:

0+0 <arm>:
\s*0:\s+e7f000f0\s+udf	#0
\s*4:\s+e7fabcfd\s+udf	#43981	; 0xabcd

0+0 <thumb>:
\s*8:\s+deab\s+udf	#171	; 0xab
\s*a:\s+decd\s+udf	#205	; 0xcd
\s*c:\s+de00\s+udf	#0
\s*e:\s+46c0\s+nop.*
\s*10:\s+f7f0 a000\s+udf\.w	#0
\s*14:\s+f7f1 a234\s+udf\.w	#4660	; 0x1234
\s*18:\s+f7fc acdd\s+udf\.w	#52445	; 0xccdd
\s*1c:\s+bf08\s+it	eq
\s*1e:\s+de12\s+udfeq	#18
\s*20:\s+de23\s+udf	#35	; 0x23
\s*22:\s+de34\s+udf	#52	; 0x34
\s*24:\s+de56\s+udf	#86	; 0x56
\s*26:\s+bf18\s+it	ne
\s*28:\s+f7f1 a234\s+udfne\.w	#4660	; 0x1234
\s*2c:\s+f7f2 a345\s+udf\.w	#9029	; 0x2345
\s*30:\s+f7f3 a456\s+udf\.w	#13398	; 0x3456
\s*34:\s+f7f5 a678\s+udf\.w	#22136	; 0x5678
