#objdump: -drw
#name: i386 naked reg

.*: +file format .*i386.*

Disassembly of section .text:

0+000 <foo>:
   0:	26 66 ff 23 [ 	]*jmpw   \*%es:\(%ebx\)
   4:	8a 25 50 00 00 00 [ 	]*mov    0x50,%ah
   a:	b2 20 [ 	]*mov    \$0x20,%dl
   c:	bb 00 00 00 00 [ 	]*mov    \$0x0,%ebx	d: (R_386_)?(dir)?32	.text
  11:	d9 c9 [ 	]*fxch   %st\(1\)
  13:	36 8c a4 81 d2 04 00 00 [ 	]*mov    %fs,%ss:0x4d2\(%ecx,%eax,4\)
  1b:	8c 2c ed 00 00 00 00 [ 	]*mov    %gs,0x0\(,%ebp,8\)
  22:	26 88 25 00 00 00 00 [ 	]*mov    %ah,%es:0x0
  29:	2e 8b 74 14 80 [ 	]*mov    %cs:-0x80\(%esp,%edx,1\),%esi
  2e:	65 f3 a5 [ 	]*rep movsl %gs:\(%esi\),%es:\(%edi\)
  31:	ec [ 	]*in     \(%dx\),%al
  32:	66 ef [ 	]*out    %ax,\(%dx\)
  34:	67 d2 14[ 	]*rclb[ 	]+%cl,\(%si\)
  37:	0f 20 d0 [ 	]*mov    %cr2,%eax
  3a:	0f 72 d0 04 [ 	]*psrld  \$0x4,%mm0
  3e:	66 47 [ 	]*inc    %di
  40:	66 51 [ 	]*push   %cx
  42:	66 58 [ 	]*pop    %ax
  44:	66 87 dd [ 	]*xchg   %bx,%bp
  47:	6a 02 [ 	]*push   \$0x2
  49:	00 00 [ 	]*add    %al,\(%eax\)
  4b:	00 00 [ 	]*add    %al,\(%eax\)
  4d:	00 00 [ 	]*add    %al,\(%eax\)
	...
