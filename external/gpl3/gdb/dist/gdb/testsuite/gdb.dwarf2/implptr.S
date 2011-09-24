	.file	"implptr.c"
	.text
.Ltext0:
	.p2align 4,,15
	.globl	foo
	.type	foo, @function
foo:
.LFB1:
	.file 1 "implptr.c"
	.loc 1 45 0
	.cfi_startproc
.LVL0:
	.loc 1 45 0
	movl	4(%esp), %eax
.LVL1:
.LBB6:
.LBB7:
	.loc 1 36 0
	addl	$1, u
.LVL2:
	.loc 1 38 0
	addl	$1, u+4
.LVL3:
	.loc 1 40 0
	addl	$1, u+8
.LVL4:
.LBE7:
.LBE6:
.LBB8:
.LBB9:
	.loc 1 36 0
	addl	$1, u+12
.LVL5:
	.loc 1 38 0
	addl	$1, u+16
.LVL6:
	.loc 1 40 0
	addl	$1, u+20
.LBE9:
.LBE8:
	.loc 1 52 0
	leal	(%eax,%eax,4), %eax
	.loc 1 53 0
	ret
	.cfi_endproc
.LFE1:
	.size	foo, .-foo
	.p2align 4,,15
	.globl	bar
	.type	bar, @function
bar:
.LFB2:
	.loc 1 57 0
	.cfi_startproc
.LVL7:
	.loc 1 61 0
	movl	4(%esp), %eax
	addl	$1, %eax
	.loc 1 63 0
	ret
	.cfi_endproc
.LFE2:
	.size	bar, .-bar
	.p2align 4,,15
	.globl	main
	.type	main, @function
main:
.LFB3:
	.loc 1 66 0
	.cfi_startproc
	pushl	%ebx
	.cfi_def_cfa_offset 8
	.cfi_offset 3, -8
	subl	$4, %esp
	.cfi_def_cfa_offset 12
	.loc 1 67 0
	movl	$5, (%esp)
	call	bar
	movl	$23, (%esp)
	movl	%eax, %ebx
	call	foo
	.loc 1 68 0
	addl	$4, %esp
	.cfi_def_cfa_offset 8
	.loc 1 67 0
	addl	%ebx, %eax
	.loc 1 68 0
	popl	%ebx
	.cfi_def_cfa_offset 4
	.cfi_restore 3
	ret
	.cfi_endproc
.LFE3:
	.size	main, .-main
	.comm	u,24,4
.Letext0:
	.section	.debug_types,"G",@progbits,wt.ade46b36db7edab6,comdat
	.long	0x46
	.value	0x4
	.long	.Ldebug_abbrev0
	.byte	0x4
	.byte	0xad
	.byte	0xe4
	.byte	0x6b
	.byte	0x36
	.byte	0xdb
	.byte	0x7e
	.byte	0xda
	.byte	0xb6
	.long	0x1d
	.uleb128 0x1
	.byte	0x1
	.long	.Ldebug_line0
	.uleb128 0x2
	.string	"S"
	.byte	0x8
	.byte	0x1
	.byte	0x18
	.long	0x3c
	.uleb128 0x3
	.string	"x"
	.byte	0x1
	.byte	0x1a
	.long	0x3c
	.byte	0
	.uleb128 0x3
	.string	"y"
	.byte	0x1
	.byte	0x1a
	.long	0x42
	.byte	0x4
	.byte	0
	.uleb128 0x4
	.byte	0x4
	.long	0x42
	.uleb128 0x5
	.byte	0x4
	.byte	0x5
	.string	"int"
	.byte	0
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.long	0x1b6
	.value	0x4
	.long	.Ldebug_abbrev0
	.byte	0x4
	.uleb128 0x6
	.long	.LASF1
	.byte	0x1
	.long	.LASF2
	.long	.LASF3
	.long	.Ltext0
	.long	.Letext0
	.long	.Ldebug_line0
	.uleb128 0x4
	.byte	0x4
	.long	0x2b
	.uleb128 0x5
	.byte	0x4
	.byte	0x5
	.string	"int"
	.uleb128 0x7
	.string	"add"
	.byte	0x1
	.byte	0x20
	.byte	0x3
	.long	0x5a
	.uleb128 0x8
	.string	"a"
	.byte	0x1
	.byte	0x20
	.long	0x5a
	.uleb128 0x8
	.string	"b"
	.byte	0x1
	.byte	0x20
	.long	0x5a
	.uleb128 0x8
	.string	"c"
	.byte	0x1
	.byte	0x20
	.long	0x2b
	.byte	0
	.uleb128 0x9
	.byte	0x4
	.byte	0xad
	.byte	0xe4
	.byte	0x6b
	.byte	0x36
	.byte	0xdb
	.byte	0x7e
	.byte	0xda
	.byte	0xb6
	.uleb128 0xa
	.string	"foo"
	.byte	0x1
	.byte	0x2c
	.long	0x2b
	.long	.LFB1
	.long	.LFE1
	.uleb128 0x1
	.byte	0x9c
	.long	0xff
	.uleb128 0xb
	.string	"i"
	.byte	0x1
	.byte	0x2c
	.long	0x2b
	.long	.LLST0
	.uleb128 0xc
	.string	"j"
	.byte	0x1
	.byte	0x2e
	.long	0x2b
	.long	.LLST1
	.uleb128 0xc
	.string	"p"
	.byte	0x1
	.byte	0x2f
	.long	0xff
	.long	.LLST2
	.uleb128 0xd
	.long	0x32
	.long	.LBB6
	.long	.LBE6
	.byte	0x1
	.byte	0x30
	.long	0xd3
	.uleb128 0xe
	.long	0x50
	.byte	0
	.uleb128 0xf
	.long	0x47
	.uleb128 0x6
	.byte	0xf2
	.long	.Ldebug_info0+151
	.sleb128 8
	.uleb128 0x10
	.long	0x3e
	.long	.LLST3
	.byte	0
	.uleb128 0x11
	.long	0x32
	.long	.LBB8
	.long	.LBE8
	.byte	0x1
	.byte	0x33
	.uleb128 0xe
	.long	0x50
	.byte	0x3
	.uleb128 0xf
	.long	0x47
	.uleb128 0x6
	.byte	0xf2
	.long	.Ldebug_info0+151
	.sleb128 8
	.uleb128 0x10
	.long	0x3e
	.long	.LLST4
	.byte	0
	.byte	0
	.uleb128 0x12
	.byte	0xad
	.byte	0xe4
	.byte	0x6b
	.byte	0x36
	.byte	0xdb
	.byte	0x7e
	.byte	0xda
	.byte	0xb6
	.long	0x113
	.uleb128 0x13
	.long	0x113
	.byte	0x1
	.byte	0
	.uleb128 0x14
	.byte	0x4
	.byte	0x7
	.long	.LASF0
	.uleb128 0xa
	.string	"bar"
	.byte	0x1
	.byte	0x38
	.long	0x2b
	.long	.LFB2
	.long	.LFE2
	.uleb128 0x1
	.byte	0x9c
	.long	0x170
	.uleb128 0x15
	.string	"i"
	.byte	0x1
	.byte	0x38
	.long	0x2b
	.uleb128 0x2
	.byte	0x91
	.sleb128 0
	.uleb128 0x16
	.string	"j"
	.byte	0x1
	.byte	0x3a
	.long	0x25
	.uleb128 0x6
	.byte	0xf2
	.long	.Ldebug_info0+307
	.sleb128 0
	.uleb128 0x16
	.string	"k"
	.byte	0x1
	.byte	0x3b
	.long	0x170
	.uleb128 0x6
	.byte	0xf2
	.long	.Ldebug_info0+319
	.sleb128 0
	.uleb128 0x16
	.string	"l"
	.byte	0x1
	.byte	0x3c
	.long	0x176
	.uleb128 0x6
	.byte	0xf2
	.long	.Ldebug_info0+335
	.sleb128 0
	.byte	0
	.uleb128 0x4
	.byte	0x4
	.long	0x25
	.uleb128 0x4
	.byte	0x4
	.long	0x170
	.uleb128 0x17
	.long	.LASF4
	.byte	0x1
	.byte	0x41
	.long	0x2b
	.long	.LFB3
	.long	.LFE3
	.uleb128 0x1
	.byte	0x9c
	.uleb128 0x18
	.long	0x2b
	.long	0x1a1
	.uleb128 0x13
	.long	0x113
	.byte	0x5
	.byte	0
	.uleb128 0x19
	.string	"u"
	.byte	0x1
	.byte	0x1d
	.long	0x191
	.uleb128 0x1a
	.string	"u"
	.byte	0x1
	.byte	0x1d
	.long	0x191
	.uleb128 0x5
	.byte	0x3
	.long	u
	.byte	0
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1
	.uleb128 0x41
	.byte	0x1
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x10
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x2
	.uleb128 0x13
	.byte	0x1
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x3
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x4
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x5
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x8
	.byte	0
	.byte	0
	.uleb128 0x6
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1b
	.uleb128 0xe
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x10
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x7
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x20
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x8
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x9
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x20
	.byte	0
	.byte	0
	.uleb128 0xa
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xb
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0xc
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0xd
	.uleb128 0x1d
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x58
	.uleb128 0xb
	.uleb128 0x59
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xe
	.uleb128 0x5
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x1c
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0xf
	.uleb128 0x5
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x10
	.uleb128 0x5
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x11
	.uleb128 0x1d
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x58
	.uleb128 0xb
	.uleb128 0x59
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x12
	.uleb128 0x1
	.byte	0x1
	.uleb128 0x49
	.uleb128 0x20
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x13
	.uleb128 0x21
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2f
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x14
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.byte	0
	.byte	0
	.uleb128 0x15
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x16
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x17
	.uleb128 0x2e
	.byte	0
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x18
	.uleb128 0x1
	.byte	0x1
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x19
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x1a
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_loc,"",@progbits
.Ldebug_loc0:
.LLST0:
	.long	.LVL0-.Ltext0
	.long	.LVL1-.Ltext0
	.value	0x2
	.byte	0x91
	.sleb128 0
	.long	.LVL1-.Ltext0
	.long	.LFE1-.Ltext0
	.value	0x6
	.byte	0x91
	.sleb128 0
	.byte	0x6
	.byte	0x31
	.byte	0x24
	.byte	0x9f
	.long	0
	.long	0
.LLST1:
	.long	.LVL1-.Ltext0
	.long	.LVL4-.Ltext0
	.value	0x2
	.byte	0x91
	.sleb128 0
	.long	.LVL4-.Ltext0
	.long	.LFE1-.Ltext0
	.value	0x6
	.byte	0x91
	.sleb128 0
	.byte	0x6
	.byte	0x33
	.byte	0x1e
	.byte	0x9f
	.long	0
	.long	0
.LLST2:
	.long	.LVL1-.Ltext0
	.long	.LVL4-.Ltext0
	.value	0x20
	.byte	0xf2
	.long	.Ldebug_info0+125
	.sleb128 0
	.byte	0x93
	.uleb128 0x4
	.byte	0x91
	.sleb128 0
	.byte	0x6
	.byte	0x32
	.byte	0x24
	.byte	0x9f
	.byte	0x93
	.uleb128 0x4
	.byte	0xf2
	.long	.Ldebug_info0+138
	.sleb128 0
	.byte	0x93
	.uleb128 0x4
	.byte	0x91
	.sleb128 0
	.byte	0x6
	.byte	0x31
	.byte	0x24
	.byte	0x9f
	.byte	0x93
	.uleb128 0x4
	.long	.LVL4-.Ltext0
	.long	.LFE1-.Ltext0
	.value	0x20
	.byte	0xf2
	.long	.Ldebug_info0+138
	.sleb128 0
	.byte	0x93
	.uleb128 0x4
	.byte	0x91
	.sleb128 0
	.byte	0x6
	.byte	0x36
	.byte	0x1e
	.byte	0x9f
	.byte	0x93
	.uleb128 0x4
	.byte	0xf2
	.long	.Ldebug_info0+125
	.sleb128 0
	.byte	0x93
	.uleb128 0x4
	.byte	0x91
	.sleb128 0
	.byte	0x6
	.byte	0x31
	.byte	0x24
	.byte	0x9f
	.byte	0x93
	.uleb128 0x4
	.long	0
	.long	0
.LLST3:
	.long	.LVL1-.Ltext0
	.long	.LVL2-.Ltext0
	.value	0x6
	.byte	0xf2
	.long	.Ldebug_info0+151
	.sleb128 0
	.long	.LVL2-.Ltext0
	.long	.LVL3-.Ltext0
	.value	0x2
	.byte	0x30
	.byte	0x9f
	.long	.LVL3-.Ltext0
	.long	.LFE1-.Ltext0
	.value	0x6
	.byte	0xf2
	.long	.Ldebug_info0+151
	.sleb128 8
	.long	0
	.long	0
.LLST4:
	.long	.LVL4-.Ltext0
	.long	.LVL5-.Ltext0
	.value	0x6
	.byte	0xf2
	.long	.Ldebug_info0+151
	.sleb128 0
	.long	.LVL5-.Ltext0
	.long	.LVL6-.Ltext0
	.value	0x2
	.byte	0x30
	.byte	0x9f
	.long	.LVL6-.Ltext0
	.long	.LFE1-.Ltext0
	.value	0x6
	.byte	0xf2
	.long	.Ldebug_info0+151
	.sleb128 8
	.long	0
	.long	0
	.section	.debug_aranges,"",@progbits
	.long	0x1c
	.value	0x2
	.long	.Ldebug_info0
	.byte	0x4
	.byte	0
	.value	0
	.value	0
	.long	.Ltext0
	.long	.Letext0-.Ltext0
	.long	0
	.long	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF0:
	.string	"unsigned int"
.LASF3:
	.string	"/home/tromey/gnu/archer/archer/gdb/testsuite/gdb.dwarf2"
.LASF4:
	.string	"main"
.LASF2:
	.string	"implptr.c"
.LASF1:
	.string	"GNU C 4.6.0 20101116 (experimental) [trunk revision 166803]"
	.ident	"GCC: (GNU) 4.6.0 20101116 (experimental) [trunk revision 166803]"
	.section	.note.GNU-stack,"",@progbits
