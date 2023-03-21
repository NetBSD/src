#objdump: -dw
#name: x86-64 segment overrides

.*: +file format .*

Disassembly of section .text:

0+ <segovr>:
[ 	]*[a-f0-9]+:	8b 00[ 	]+mov[ 	]+\(%rax\),%eax
[ 	]*[a-f0-9]+:	8b 01[ 	]+mov[ 	]+\(%rcx\),%eax
[ 	]*[a-f0-9]+:	8b 02[ 	]+mov[ 	]+\(%rdx\),%eax
[ 	]*[a-f0-9]+:	8b 03[ 	]+mov[ 	]+\(%rbx\),%eax
[ 	]*[a-f0-9]+:	3e 8b 04 24[ 	]+mov[ 	]+%ds:\(%rsp\),%eax
[ 	]*[a-f0-9]+:	3e 8b 45 00[ 	]+mov[ 	]+%ds:((0x)?0)?\(%rbp\),%eax
[ 	]*[a-f0-9]+:	8b 06[ 	]+mov[ 	]+\(%rsi\),%eax
[ 	]*[a-f0-9]+:	8b 07[ 	]+mov[ 	]+\(%rdi\),%eax
[ 	]*[a-f0-9]+:	41 8b 00[ 	]+mov[ 	]+\(%r8\),%eax
[ 	]*[a-f0-9]+:	41 8b 01[ 	]+mov[ 	]+\(%r9\),%eax
[ 	]*[a-f0-9]+:	41 8b 02[ 	]+mov[ 	]+\(%r10\),%eax
[ 	]*[a-f0-9]+:	41 8b 03[ 	]+mov[ 	]+\(%r11\),%eax
[ 	]*[a-f0-9]+:	41 8b 04 24[ 	]+mov[ 	]+\(%r12\),%eax
[ 	]*[a-f0-9]+:	41 8b 45 00[ 	]+mov[ 	]+((0x)?0)?\(%r13\),%eax
[ 	]*[a-f0-9]+:	41 8b 06[ 	]+mov[ 	]+\(%r14\),%eax
[ 	]*[a-f0-9]+:	41 8b 07[ 	]+mov[ 	]+\(%r15\),%eax
[ 	]*[a-f0-9]+:	36 8b 00[ 	]+mov[ 	]+%ss:\(%rax\),%eax
[ 	]*[a-f0-9]+:	36 8b 01[ 	]+mov[ 	]+%ss:\(%rcx\),%eax
[ 	]*[a-f0-9]+:	36 8b 02[ 	]+mov[ 	]+%ss:\(%rdx\),%eax
[ 	]*[a-f0-9]+:	36 8b 03[ 	]+mov[ 	]+%ss:\(%rbx\),%eax
[ 	]*[a-f0-9]+:	8b 04 24[ 	]+mov[ 	]+\(%rsp\),%eax
[ 	]*[a-f0-9]+:	8b 45 00[ 	]+mov[ 	]+((0x)?0)?\(%rbp\),%eax
[ 	]*[a-f0-9]+:	36 8b 06[ 	]+mov[ 	]+%ss:\(%rsi\),%eax
[ 	]*[a-f0-9]+:	36 8b 07[ 	]+mov[ 	]+%ss:\(%rdi\),%eax
[ 	]*[a-f0-9]+:	36 41 8b 00[ 	]+mov[ 	]+%ss:\(%r8\),%eax
[ 	]*[a-f0-9]+:	36 41 8b 01[ 	]+mov[ 	]+%ss:\(%r9\),%eax
[ 	]*[a-f0-9]+:	36 41 8b 02[ 	]+mov[ 	]+%ss:\(%r10\),%eax
[ 	]*[a-f0-9]+:	36 41 8b 03[ 	]+mov[ 	]+%ss:\(%r11\),%eax
[ 	]*[a-f0-9]+:	36 41 8b 04 24[ 	]+mov[ 	]+%ss:\(%r12\),%eax
[ 	]*[a-f0-9]+:	36 41 8b 45 00[ 	]+mov[ 	]+%ss:((0x)?0)?\(%r13\),%eax
[ 	]*[a-f0-9]+:	36 41 8b 06[ 	]+mov[ 	]+%ss:\(%r14\),%eax
[ 	]*[a-f0-9]+:	36 41 8b 07[ 	]+mov[ 	]+%ss:\(%r15\),%eax
#pass
