#objdump: -dw
#name: i386 SIB

.*: +file format .*

Disassembly of section .text:

0+ <foo>:
[ 	]*[a-f0-9]+:	8b 1d e2 ff ff ff    	mov    0xffffffe2,%ebx
[ 	]*[a-f0-9]+:	8b 1c 25 e2 ff ff ff 	mov    -0x1e\(,%eiz,1\),%ebx
[ 	]*[a-f0-9]+:	8b 04 25 e2 ff ff ff 	mov    -0x1e\(,%eiz,1\),%eax
[ 	]*[a-f0-9]+:	8b 04 65 e2 ff ff ff 	mov    -0x1e\(,%eiz,2\),%eax
[ 	]*[a-f0-9]+:	8b 04 a5 e2 ff ff ff 	mov    -0x1e\(,%eiz,4\),%eax
[ 	]*[a-f0-9]+:	8b 04 e5 e2 ff ff ff 	mov    -0x1e\(,%eiz,8\),%eax
[ 	]*[a-f0-9]+:	a1 1e 00 00 00       	mov    0x1e,%eax
[ 	]*[a-f0-9]+:	8b 04 25 1e 00 00 00 	mov    0x1e\(,%eiz,1\),%eax
[ 	]*[a-f0-9]+:	8b 04 25 1e 00 00 00 	mov    0x1e\(,%eiz,1\),%eax
[ 	]*[a-f0-9]+:	8b 04 65 1e 00 00 00 	mov    0x1e\(,%eiz,2\),%eax
[ 	]*[a-f0-9]+:	8b 04 a5 1e 00 00 00 	mov    0x1e\(,%eiz,4\),%eax
[ 	]*[a-f0-9]+:	8b 04 e5 1e 00 00 00 	mov    0x1e\(,%eiz,8\),%eax
[ 	]*[a-f0-9]+:	8b 03                	mov    \(%ebx\),%eax
[ 	]*[a-f0-9]+:	8b 04 23             	mov    \(%ebx,%eiz,1\),%eax
[ 	]*[a-f0-9]+:	8b 04 23             	mov    \(%ebx,%eiz,1\),%eax
[ 	]*[a-f0-9]+:	8b 04 63             	mov    \(%ebx,%eiz,2\),%eax
[ 	]*[a-f0-9]+:	8b 04 a3             	mov    \(%ebx,%eiz,4\),%eax
[ 	]*[a-f0-9]+:	8b 04 e3             	mov    \(%ebx,%eiz,8\),%eax
[ 	]*[a-f0-9]+:	8b 04 24             	mov    \(%esp\),%eax
[ 	]*[a-f0-9]+:	8b 04 24             	mov    \(%esp\),%eax
[ 	]*[a-f0-9]+:	8b 04 64             	mov    \(%esp,%eiz,2\),%eax
[ 	]*[a-f0-9]+:	8b 04 a4             	mov    \(%esp,%eiz,4\),%eax
[ 	]*[a-f0-9]+:	8b 04 e4             	mov    \(%esp,%eiz,8\),%eax
[ 	]*[a-f0-9]+:	8b 04 25 e2 ff ff ff 	mov    -0x1e\(,%eiz,1\),%eax
[ 	]*[a-f0-9]+:	8b 04 65 e2 ff ff ff 	mov    -0x1e\(,%eiz,2\),%eax
[ 	]*[a-f0-9]+:	8b 04 a5 e2 ff ff ff 	mov    -0x1e\(,%eiz,4\),%eax
[ 	]*[a-f0-9]+:	8b 04 e5 e2 ff ff ff 	mov    -0x1e\(,%eiz,8\),%eax
[ 	]*[a-f0-9]+:	8b 04 25 1e 00 00 00 	mov    0x1e\(,%eiz,1\),%eax
[ 	]*[a-f0-9]+:	8b 04 65 1e 00 00 00 	mov    0x1e\(,%eiz,2\),%eax
[ 	]*[a-f0-9]+:	8b 04 a5 1e 00 00 00 	mov    0x1e\(,%eiz,4\),%eax
[ 	]*[a-f0-9]+:	8b 04 e5 1e 00 00 00 	mov    0x1e\(,%eiz,8\),%eax
[ 	]*[a-f0-9]+:	8b 04 23             	mov    \(%ebx,%eiz,1\),%eax
[ 	]*[a-f0-9]+:	8b 04 23             	mov    \(%ebx,%eiz,1\),%eax
[ 	]*[a-f0-9]+:	8b 04 63             	mov    \(%ebx,%eiz,2\),%eax
[ 	]*[a-f0-9]+:	8b 04 a3             	mov    \(%ebx,%eiz,4\),%eax
[ 	]*[a-f0-9]+:	8b 04 e3             	mov    \(%ebx,%eiz,8\),%eax
[ 	]*[a-f0-9]+:	8b 04 24             	mov    \(%esp\),%eax
[ 	]*[a-f0-9]+:	8b 04 24             	mov    \(%esp\),%eax
[ 	]*[a-f0-9]+:	8b 04 24             	mov    \(%esp\),%eax
[ 	]*[a-f0-9]+:	8b 04 64             	mov    \(%esp,%eiz,2\),%eax
[ 	]*[a-f0-9]+:	8b 04 a4             	mov    \(%esp,%eiz,4\),%eax
[ 	]*[a-f0-9]+:	8b 04 e4             	mov    \(%esp,%eiz,8\),%eax
#pass
