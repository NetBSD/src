	.section	.gnu.linkonce.t.foo, "a", %progbits
1:
	.globl	symfoo
symfoo:
	.long	0

	.section	.gnu.linkonce.t.bar, "a", %progbits
2:
	.globl	symbar
symbar:
	.long	0

	.section	.gnu.linkonce.r.foo, "a", %progbits
	.long	1b
	.long	symfoo
/* ld currently incorrectly silently discards this relocation.  Just such
   relocations are never produced by g++-3.4 so this suppressed error message
   is not a problem:
   #error: `.gnu.linkonce.t.bar' referenced in section `.gnu.linkonce.r.foo' of tmpdir/dump1.o: defined in discarded section `.gnu.linkonce.t.bar' of tmpdir/dump1.o
   */
	.long	2b
	.long	symbar
