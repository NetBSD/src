	.file	"unwind-on-each-insn-foo.c"
	.text
.Ltext0:
	.globl	foo
	.type	foo, @function
foo:
.LFB0:
	.file 1 "/home/vries/gdb_versions/devel/src/gdb/testsuite/gdb.base/unwind-on-each-insn-foo.c"
	# /home/vries/gdb_versions/devel/src/gdb/testsuite/gdb.base/unwind-on-each-insn-foo.c:20
	.loc 1 20 0
	.cfi_startproc
# BLOCK 2 seq:0
# PRED: ENTRY (FALLTHRU)
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movq	%rdi, -8(%rbp)
	# /home/vries/gdb_versions/devel/src/gdb/testsuite/gdb.base/unwind-on-each-insn-foo.c:22
	.loc 1 22 0
	nop
	popq	%rbp
	#.cfi_def_cfa 7, 8
# SUCC: EXIT [100.0%] 
	ret
	.cfi_endproc
.LFE0:
	.size	foo, .-foo
	.globl	bar
	.type	bar, @function
bar:
.LFB1:
	# /home/vries/gdb_versions/devel/src/gdb/testsuite/gdb.base/unwind-on-each-insn-foo.c:26
	.loc 1 26 0
	.cfi_startproc
# BLOCK 2 seq:0
# PRED: ENTRY (FALLTHRU)
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$8, %rsp
	movq	%rdi, -8(%rbp)
	# /home/vries/gdb_versions/devel/src/gdb/testsuite/gdb.base/unwind-on-each-insn-foo.c:27
	.loc 1 27 0
	movq	-8(%rbp), %rax
	movq	%rax, %rdi
	call	foo
	# /home/vries/gdb_versions/devel/src/gdb/testsuite/gdb.base/unwind-on-each-insn-foo.c:28
	.loc 1 28 0
	nop
	leave
	#.cfi_def_cfa 7, 8
# SUCC: EXIT [100.0%] 
	ret
	.cfi_endproc
.LFE1:
	.size	bar, .-bar
.Letext0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.long	0x8c	# Length of Compilation Unit Info
	.value	0x4	# DWARF version number
	.long	.Ldebug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x8	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0xb) DW_TAG_compile_unit)
	.long	.LASF0	# DW_AT_producer: "GNU C11 4.4.7 -mtune=generic -march=x86-64 -g -fno-stack-protector"
	.byte	0xc	# DW_AT_language
	.long	.LASF1	# DW_AT_name: "/home/vries/gdb_versions/devel/src/gdb/testsuite/gdb.base/unwind-on-each-insn-foo.c"
	.long	.LASF2	# DW_AT_comp_dir: "/home/vries/gdb_versions/devel/build/gdb/testsuite"
	.quad	.Ltext0	# DW_AT_low_pc
	.quad	.Letext0-.Ltext0	# DW_AT_high_pc
	.long	.Ldebug_line0	# DW_AT_stmt_list
	.uleb128 0x2	# (DIE (0x2d) DW_TAG_subprogram)
			# DW_AT_external
	.ascii "bar\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (/home/vries/gdb_versions/devel/src/gdb/testsuite/gdb.base/unwind-on-each-insn-foo.c)
	.byte	0x19	# DW_AT_decl_line
			# DW_AT_prototyped
	.quad	.LFB1	# DW_AT_low_pc
	.quad	.LFE1-.LFB1	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_tail_call_sites
	.long	0x57	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0x4a) DW_TAG_formal_parameter)
	.ascii "s\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (/home/vries/gdb_versions/devel/src/gdb/testsuite/gdb.base/unwind-on-each-insn-foo.c)
	.byte	0x19	# DW_AT_decl_line
	.long	0x57	# DW_AT_type
	.uleb128 0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -24
	.byte	0	# end of children of DIE 0x2d
	.uleb128 0x4	# (DIE (0x57) DW_TAG_pointer_type)
	.byte	0x8	# DW_AT_byte_size
	.long	0x64	# DW_AT_type
	.uleb128 0x5	# (DIE (0x5d) DW_TAG_base_type)
	.byte	0x1	# DW_AT_byte_size
	.byte	0x6	# DW_AT_encoding
	.long	.LASF3	# DW_AT_name: "char"
	.uleb128 0x6	# (DIE (0x64) DW_TAG_const_type)
	.long	0x5d	# DW_AT_type
	.uleb128 0x7	# (DIE (0x69) DW_TAG_subprogram)
			# DW_AT_external
	.ascii "foo\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (/home/vries/gdb_versions/devel/src/gdb/testsuite/gdb.base/unwind-on-each-insn-foo.c)
	.byte	0x13	# DW_AT_decl_line
			# DW_AT_prototyped
	.quad	.LFB0	# DW_AT_low_pc
	.quad	.LFE0-.LFB0	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_call_sites
	.uleb128 0x3	# (DIE (0x82) DW_TAG_formal_parameter)
	.ascii "s\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (/home/vries/gdb_versions/devel/src/gdb/testsuite/gdb.base/unwind-on-each-insn-foo.c)
	.byte	0x13	# DW_AT_decl_line
	.long	0x57	# DW_AT_type
	.uleb128 0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -24
	.byte	0	# end of children of DIE 0x69
	.byte	0	# end of children of DIE 0xb
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1	# (abbrev code)
	.uleb128 0x11	# (TAG: DW_TAG_compile_unit)
	.byte	0x1	# DW_children_yes
	.uleb128 0x25	# (DW_AT_producer)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x13	# (DW_AT_language)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x1b	# (DW_AT_comp_dir)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x7	# (DW_FORM_data8)
	.uleb128 0x10	# (DW_AT_stmt_list)
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.byte	0
	.byte	0
	.uleb128 0x2	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x27	# (DW_AT_prototyped)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x7	# (DW_FORM_data8)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.uleb128 0x2116	# (DW_AT_GNU_all_tail_call_sites)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x3	# (abbrev code)
	.uleb128 0x5	# (TAG: DW_TAG_formal_parameter)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.byte	0
	.byte	0
	.uleb128 0x4	# (abbrev code)
	.uleb128 0xf	# (TAG: DW_TAG_pointer_type)
	.byte	0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x5	# (abbrev code)
	.uleb128 0x24	# (TAG: DW_TAG_base_type)
	.byte	0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3e	# (DW_AT_encoding)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.byte	0
	.byte	0
	.uleb128 0x6	# (abbrev code)
	.uleb128 0x26	# (TAG: DW_TAG_const_type)
	.byte	0	# DW_children_no
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x7	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x27	# (DW_AT_prototyped)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x7	# (DW_FORM_data8)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.uleb128 0x2117	# (DW_AT_GNU_all_call_sites)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_aranges,"",@progbits
	.long	0x2c	# Length of Address Ranges Info
	.value	0x2	# DWARF Version
	.long	.Ldebug_info0	# Offset of Compilation Unit Info
	.byte	0x8	# Size of Address
	.byte	0	# Size of Segment Descriptor
	.value	0	# Pad to 16 byte boundary
	.value	0
	.quad	.Ltext0	# Address
	.quad	.Letext0-.Ltext0	# Length
	.quad	0
	.quad	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF0:
	.string	"GNU C11 4.4.7 -mtune=generic -march=x86-64 -g -fno-stack-protector"
.LASF1:
	.string	"/home/vries/gdb_versions/devel/src/gdb/testsuite/gdb.base/unwind-on-each-insn-foo.c"
.LASF2:
	.string	"/home/vries/gdb_versions/devel/build/gdb/testsuite"
.LASF3:
	.string	"char"
	.ident	"GCC: (SUSE Linux) 7.5.0"
	.section	.note.GNU-stack,"",@progbits
