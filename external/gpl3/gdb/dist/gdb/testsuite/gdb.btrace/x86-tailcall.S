/* This testcase is part of GDB, the GNU debugger.

   Copyright 2013-2015 Free Software Foundation, Inc.

   Contributed by Intel Corp. <markus.t.metzger@intel.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.


   This file has been generated using:
   gcc -S -O2 -dA -g x86-tailcall.c -o x86-tailcall.S  */

	.file	"x86-tailcall.c"
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.text
.Ltext0:
	.p2align 4,,15
	.type	bar, @function
bar:
.LFB0:
	.file 1 "x86-tailcall.c"
	# x86-tailcall.c:22
	.loc 1 22 0
	.cfi_startproc
	# basic block 2
	# x86-tailcall.c:24
	.loc 1 24 0
	movl	$42, %eax
	ret
	.cfi_endproc
.LFE0:
	.size	bar, .-bar
	.p2align 4,,15
	.type	foo, @function
foo:
.LFB1:
	# x86-tailcall.c:28
	.loc 1 28 0
	.cfi_startproc
	# basic block 2
	# x86-tailcall.c:29
	.loc 1 29 0
	jmp	bar
	.cfi_endproc
.LFE1:
	.size	foo, .-foo
	.p2align 4,,15
.globl main
	.type	main, @function
main:
.LFB2:
	# x86-tailcall.c:34
	.loc 1 34 0
	.cfi_startproc
	# basic block 2
	# x86-tailcall.c:37
	.loc 1 37 0
	call	foo
.LVL0:
	addl	$1, %eax
.LVL1:
	# x86-tailcall.c:39
	.loc 1 39 0
	ret
	.cfi_endproc
.LFE2:
	.size	main, .-main
.Letext0:
	.section	.debug_loc,"",@progbits
.Ldebug_loc0:
.LLST0:
	.quad	.LVL0-.Ltext0	# Location list begin address (*.LLST0)
	.quad	.LVL1-.Ltext0	# Location list end address (*.LLST0)
	.value	0x3	# Location expression size
	.byte	0x70	# DW_OP_breg0
	.sleb128 1
	.byte	0x9f	# DW_OP_stack_value
	.quad	.LVL1-.Ltext0	# Location list begin address (*.LLST0)
	.quad	.LFE2-.Ltext0	# Location list end address (*.LLST0)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	0x0	# Location list terminator begin (*.LLST0)
	.quad	0x0	# Location list terminator end (*.LLST0)
	.section	.debug_info
	.long	0x9c	# Length of Compilation Unit Info
	.value	0x3	# DWARF version number
	.long	.Ldebug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x8	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0xb) DW_TAG_compile_unit)
	.long	.LASF0	# DW_AT_producer: "GNU C 4.4.4 20100726 (Red Hat 4.4.4-13)"
	.byte	0x1	# DW_AT_language
	.long	.LASF1	# DW_AT_name: "x86-tailcall.c"
	.long	.LASF2	# DW_AT_comp_dir: ""
	.quad	.Ltext0	# DW_AT_low_pc
	.quad	.Letext0	# DW_AT_high_pc
	.long	.Ldebug_line0	# DW_AT_stmt_list
	.uleb128 0x2	# (DIE (0x2d) DW_TAG_subprogram)
	.ascii "bar\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (x86-tailcall.c)
	.byte	0x15	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x4b	# DW_AT_type
	.quad	.LFB0	# DW_AT_low_pc
	.quad	.LFE0	# DW_AT_high_pc
	.byte	0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
	.uleb128 0x3	# (DIE (0x4b) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.ascii "int\0"	# DW_AT_name
	.uleb128 0x2	# (DIE (0x52) DW_TAG_subprogram)
	.ascii "foo\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (x86-tailcall.c)
	.byte	0x1b	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x4b	# DW_AT_type
	.quad	.LFB1	# DW_AT_low_pc
	.quad	.LFE1	# DW_AT_high_pc
	.byte	0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
	.uleb128 0x4	# (DIE (0x70) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF3	# DW_AT_name: "main"
	.byte	0x1	# DW_AT_decl_file (x86-tailcall.c)
	.byte	0x21	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x4b	# DW_AT_type
	.quad	.LFB2	# DW_AT_low_pc
	.quad	.LFE2	# DW_AT_high_pc
	.byte	0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
	.uleb128 0x5	# (DIE (0x8f) DW_TAG_variable)
	.long	.LASF4	# DW_AT_name: "answer"
	.byte	0x1	# DW_AT_decl_file (x86-tailcall.c)
	.byte	0x23	# DW_AT_decl_line
	.long	0x4b	# DW_AT_type
	.long	.LLST0	# DW_AT_location
	.byte	0x0	# end of children of DIE 0x70
	.byte	0x0	# end of children of DIE 0xb
	.section	.debug_abbrev
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
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x10	# (DW_AT_stmt_list)
	.uleb128 0x6	# (DW_FORM_data4)
	.byte	0x0
	.byte	0x0
	.uleb128 0x2	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x27	# (DW_AT_prototyped)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0xa	# (DW_FORM_block1)
	.byte	0x0
	.byte	0x0
	.uleb128 0x3	# (abbrev code)
	.uleb128 0x24	# (TAG: DW_TAG_base_type)
	.byte	0x0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3e	# (DW_AT_encoding)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.byte	0x0
	.byte	0x0
	.uleb128 0x4	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x27	# (DW_AT_prototyped)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0xa	# (DW_FORM_block1)
	.byte	0x0
	.byte	0x0
	.uleb128 0x5	# (abbrev code)
	.uleb128 0x34	# (TAG: DW_TAG_variable)
	.byte	0x0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x6	# (DW_FORM_data4)
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.section	.debug_pubnames,"",@progbits
	.long	0x17	# Length of Public Names Info
	.value	0x2	# DWARF Version
	.long	.Ldebug_info0	# Offset of Compilation Unit Info
	.long	0xa0	# Compilation Unit Length
	.long	0x70	# DIE offset
	.ascii "main\0"	# external name
	.long	0x0
	.section	.debug_aranges,"",@progbits
	.long	0x2c	# Length of Address Ranges Info
	.value	0x2	# DWARF Version
	.long	.Ldebug_info0	# Offset of Compilation Unit Info
	.byte	0x8	# Size of Address
	.byte	0x0	# Size of Segment Descriptor
	.value	0x0	# Pad to 16 byte boundary
	.value	0x0
	.quad	.Ltext0	# Address
	.quad	.Letext0-.Ltext0	# Length
	.quad	0x0
	.quad	0x0
	.section	.debug_str,"MS",@progbits,1
.LASF0:
	.string	"GNU C 4.4.4 20100726 (Red Hat 4.4.4-13)"
.LASF3:
	.string	"main"
.LASF4:
	.string	"answer"
.LASF2:
	.string	""
.LASF1:
	.string	"x86-tailcall.c"
	.ident	"GCC: (GNU) 4.4.4 20100726 (Red Hat 4.4.4-13)"
	.section	.note.GNU-stack,"",@progbits
