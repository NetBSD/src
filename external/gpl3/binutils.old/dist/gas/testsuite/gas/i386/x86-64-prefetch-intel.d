#objdump: -dw -Mintel
#name: x86-64 prefetch (Intel disassembly)
#source: prefetch.s

.*: +file format .*

Disassembly of section .text:

0+ <amd_prefetch>:
\s*[a-f0-9]+:	0f 0d 00             	prefetch BYTE PTR \[rax\]
\s*[a-f0-9]+:	0f 0d 08             	prefetchw BYTE PTR \[rax\]
\s*[a-f0-9]+:	0f 0d 10             	prefetchwt1 BYTE PTR \[rax\]
\s*[a-f0-9]+:	0f 0d 18             	prefetch BYTE PTR \[rax\]
\s*[a-f0-9]+:	0f 0d 20             	prefetch BYTE PTR \[rax\]
\s*[a-f0-9]+:	0f 0d 28             	prefetch BYTE PTR \[rax\]
\s*[a-f0-9]+:	0f 0d 30             	prefetch BYTE PTR \[rax\]
\s*[a-f0-9]+:	0f 0d 38             	prefetch BYTE PTR \[rax\]

0+[0-9a-f]+ <intel_prefetch>:
\s*[a-f0-9]+:	0f 18 00             	prefetchnta BYTE PTR \[rax\]
\s*[a-f0-9]+:	0f 18 08             	prefetcht0 BYTE PTR \[rax\]
\s*[a-f0-9]+:	0f 18 10             	prefetcht1 BYTE PTR \[rax\]
\s*[a-f0-9]+:	0f 18 18             	prefetcht2 BYTE PTR \[rax\]
\s*[a-f0-9]+:	0f 18 20             	nop/reserved BYTE PTR \[rax\]
\s*[a-f0-9]+:	0f 18 28             	nop/reserved BYTE PTR \[rax\]
\s*[a-f0-9]+:	0f 18 30             	nop/reserved BYTE PTR \[rax\]
\s*[a-f0-9]+:	0f 18 38             	nop/reserved BYTE PTR \[rax\]
