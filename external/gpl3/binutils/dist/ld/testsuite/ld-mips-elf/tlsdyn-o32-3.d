
.*:     file format elf32-tradbigmips

Disassembly of section .text:

.* <other>:
  .*:	3c1c0fc0 	lui	gp,0xfc0
  .*:	279c7c10 	addiu	gp,gp,31760
  .*:	0399e021 	addu	gp,gp,t9
  .*:	27bdfff0 	addiu	sp,sp,-16
  .*:	afbe0008 	sw	s8,8\(sp\)
  .*:	03a0f021 	move	s8,sp
  .*:	afbc0000 	sw	gp,0\(sp\)
  .*:	8f998018 	lw	t9,-32744\(gp\)
  .*:	27848028 	addiu	a0,gp,-32728
  .*:	0320f809 	jalr	t9
  .*:	00000000 	nop
  .*:	8fdc0000 	lw	gp,0\(s8\)
  .*:	00000000 	nop
  .*:	8f998018 	lw	t9,-32744\(gp\)
  .*:	27848020 	addiu	a0,gp,-32736
  .*:	0320f809 	jalr	t9
  .*:	00000000 	nop
  .*:	8fdc0000 	lw	gp,0\(s8\)
  .*:	00000000 	nop
  .*:	8f998018 	lw	t9,-32744\(gp\)
  .*:	27848030 	addiu	a0,gp,-32720
  .*:	0320f809 	jalr	t9
  .*:	00000000 	nop
  .*:	8fdc0000 	lw	gp,0\(s8\)
  .*:	00401021 	move	v0,v0
  .*:	3c030000 	lui	v1,0x0
  .*:	24638000 	addiu	v1,v1,-32768
  .*:	00621821 	addu	v1,v1,v0
  .*:	7c02283b 	rdhwr	v0,\$5
  .*:	8f838038 	lw	v1,-32712\(gp\)
  .*:	00000000 	nop
  .*:	00621821 	addu	v1,v1,v0
  .*:	8f83801c 	lw	v1,-32740\(gp\)
  .*:	00000000 	nop
  .*:	00621821 	addu	v1,v1,v0
  .*:	7c02283b 	rdhwr	v0,\$5
  .*:	3c030000 	lui	v1,0x0
  .*:	24639004 	addiu	v1,v1,-28668
  .*:	00621821 	addu	v1,v1,v0
  .*:	03c0e821 	move	sp,s8
  .*:	8fbe0008 	lw	s8,8\(sp\)
  .*:	03e00008 	jr	ra
  .*:	27bd0010 	addiu	sp,sp,16
  .*:	00000000 	nop

.* <__start>:
  .*:	3c1c0fc0 	lui	gp,0xfc0
  .*:	279c7b60 	addiu	gp,gp,31584
  .*:	0399e021 	addu	gp,gp,t9
  .*:	27bdfff0 	addiu	sp,sp,-16
  .*:	afbe0008 	sw	s8,8\(sp\)
  .*:	03a0f021 	move	s8,sp
  .*:	afbc0000 	sw	gp,0\(sp\)
  .*:	8f998018 	lw	t9,-32744\(gp\)
  .*:	27848028 	addiu	a0,gp,-32728
  .*:	0320f809 	jalr	t9
  .*:	00000000 	nop
  .*:	8fdc0000 	lw	gp,0\(s8\)
  .*:	00000000 	nop
  .*:	8f998018 	lw	t9,-32744\(gp\)
  .*:	27848020 	addiu	a0,gp,-32736
  .*:	0320f809 	jalr	t9
  .*:	00000000 	nop
  .*:	8fdc0000 	lw	gp,0\(s8\)
  .*:	00000000 	nop
  .*:	8f998018 	lw	t9,-32744\(gp\)
  .*:	27848030 	addiu	a0,gp,-32720
  .*:	0320f809 	jalr	t9
  .*:	00000000 	nop
  .*:	8fdc0000 	lw	gp,0\(s8\)
  .*:	00401021 	move	v0,v0
  .*:	3c030000 	lui	v1,0x0
  .*:	24638000 	addiu	v1,v1,-32768
  .*:	00621821 	addu	v1,v1,v0
  .*:	7c02283b 	rdhwr	v0,\$5
  .*:	8f838038 	lw	v1,-32712\(gp\)
  .*:	00000000 	nop
  .*:	00621821 	addu	v1,v1,v0
  .*:	8f83801c 	lw	v1,-32740\(gp\)
  .*:	00000000 	nop
  .*:	00621821 	addu	v1,v1,v0
  .*:	7c02283b 	rdhwr	v0,\$5
  .*:	3c030000 	lui	v1,0x0
  .*:	24639004 	addiu	v1,v1,-28668
  .*:	00621821 	addu	v1,v1,v0
  .*:	03c0e821 	move	sp,s8
  .*:	8fbe0008 	lw	s8,8\(sp\)
  .*:	03e00008 	jr	ra
  .*:	27bd0010 	addiu	sp,sp,16

.* <__tls_get_addr>:
  .*:	03e00008 	jr	ra
  .*:	00000000 	nop
	...
