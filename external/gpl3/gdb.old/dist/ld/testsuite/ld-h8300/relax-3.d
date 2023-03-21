# name: H8300 Relaxation Test 3
# ld: --relax
# objdump: -d

.*:     file format .*

Disassembly of section .text:

00000100 <_start>:
#
# Relaxation of aa:16
#
.*:	6a 08 00 00 	mov.b	@0x0:16,r0l
.*:	6a 08 7f ff 	mov.b	@0x7fff:16,r0l
.*:	6a 08 80 00 	mov.b	@0x8000:16,r0l
.*:	6a 08 fe ff 	mov.b	@0xfeff:16,r0l
.*:	28 00 *	mov.b	@0x0:8,r0l
.*:	28 ff *	mov.b	@0xff:8,r0l
#
# Relaxation of aa:32
#
.*:	6a 08 00 00 	mov.b	@0x0:16,r0l
.*:	6a 08 7f ff 	mov.b	@0x7fff:16,r0l
.*:	6a 28 00 00 	mov.b	@0x8000:32,r0l
.*:	80 00 
.*:	6a 28 00 00 	mov.b	@0xff00:32,r0l
.*:	ff 00 
.*:	6a 28 00 ff 	mov.b	@0xffff00:32,r0l
.*:	ff 00 
.*:	6a 28 ff ff 	mov.b	@0xffff7fff:32,r0l
.*:	7f ff 
.*:	6a 08 80 00 	mov.b	@0x8000:16,r0l
.*:	6a 08 fe ff 	mov.b	@0xfeff:16,r0l
.*:	28 00 *	mov.b	@0x0:8,r0l
.*:	28 ff *	mov.b	@0xff:8,r0l
