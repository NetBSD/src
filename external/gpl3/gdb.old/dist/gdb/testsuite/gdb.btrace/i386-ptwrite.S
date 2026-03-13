/* This testcase is part of GDB, the GNU debugger.

   Copyright 2024 Free Software Foundation, Inc.

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


   This file has been generated using gcc version 10.3.1 20210422
   (Red Hat 10.3.1-1):
   gcc -S -dA -g -m32 -mptwrite ptwrite.c -o i386-ptwrite.S.  */


	.file	"ptwrite.c"
	.text
.Ltext0:
	.globl	ptwrite1
	.type	ptwrite1, @function
ptwrite1:
.LFB4021:
	.file 1 "ptwrite.c"
	# ptwrite.c:22:1
	.loc 1 22 1
	.cfi_startproc
# BLOCK 2 seq:0
# PRED: ENTRY (FALLTHRU)
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	subl	$16, %esp
	# ptwrite.c:23:3
	.loc 1 23 3
	movl	8(%ebp), %eax
	movl	%eax, -4(%ebp)
.LBB6:
.LBB7:
	.file 2 "/usr/lib/gcc/x86_64-redhat-linux/10/include/immintrin.h"
	# /usr/lib/gcc/x86_64-redhat-linux/10/include/immintrin.h:307:3
	.loc 2 307 3
	movl	-4(%ebp), %eax
	ptwrite	%eax
	# /usr/lib/gcc/x86_64-redhat-linux/10/include/immintrin.h:308:1
	.loc 2 308 1
	nop
.LBE7:
.LBE6:
	# ptwrite.c:24:1
	.loc 1 24 1
	nop
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
# SUCC: EXIT [always]
	ret
	.cfi_endproc
.LFE4021:
	.size	ptwrite1, .-ptwrite1
	.globl	ptwrite2
	.type	ptwrite2, @function
ptwrite2:
.LFB4022:
	# ptwrite.c:28:1
	.loc 1 28 1
	.cfi_startproc
# BLOCK 2 seq:0
# PRED: ENTRY (FALLTHRU)
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	subl	$16, %esp
	# ptwrite.c:29:3
	.loc 1 29 3
	movl	8(%ebp), %eax
	movl	%eax, -4(%ebp)
.LBB8:
.LBB9:
	# /usr/lib/gcc/x86_64-redhat-linux/10/include/immintrin.h:307:3
	.loc 2 307 3
	movl	-4(%ebp), %eax
	ptwrite	%eax
	# /usr/lib/gcc/x86_64-redhat-linux/10/include/immintrin.h:308:1
	.loc 2 308 1
	nop
.LBE9:
.LBE8:
	# ptwrite.c:30:1
	.loc 1 30 1
	nop
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
# SUCC: EXIT [always]
	ret
	.cfi_endproc
.LFE4022:
	.size	ptwrite2, .-ptwrite2
	.globl	main
	.type	main, @function
main:
.LFB4023:
	# ptwrite.c:34:1
	.loc 1 34 1
	.cfi_startproc
# BLOCK 2 seq:0
# PRED: ENTRY (FALLTHRU)
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	# ptwrite.c:35:3
	.loc 1 35 3
	pushl	$66
	call	ptwrite1
	addl	$4, %esp
	# ptwrite.c:36:3
	.loc 1 36 3
	pushl	$67
	call	ptwrite2
	addl	$4, %esp
	# ptwrite.c:38:10
	.loc 1 38 10
	movl	$0, %eax
	# ptwrite.c:39:1
	.loc 1 39 1
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
# SUCC: EXIT [always]
	ret
	.cfi_endproc
.LFE4023:
	.size	main, .-main
.Letext0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.long	0x129	# Length of Compilation Unit Info
	.value	0x4	# DWARF version number
	.long	.Ldebug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x4	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0xb) DW_TAG_compile_unit)
	.long	.LASF15	# DW_AT_producer: "GNU C17 10.3.1 20210422 (Red Hat 10.3.1-1) -m32 -mptwrite -mtune=generic -march=i686 -g"
	.byte	0xc	# DW_AT_language
	.long	.LASF16	# DW_AT_name: "ptwrite.c"
	.long	.LASF17	# DW_AT_comp_dir: "gdb/gdb/testsuite/gdb.btrace"
	.long	.Ltext0	# DW_AT_low_pc
	.long	.Letext0-.Ltext0	# DW_AT_high_pc
	.long	.Ldebug_line0	# DW_AT_stmt_list
	.uleb128 0x2	# (DIE (0x25) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.ascii "int\0"	# DW_AT_name
	.uleb128 0x3	# (DIE (0x2c) DW_TAG_base_type)
	.byte	0x2	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.long	.LASF0	# DW_AT_name: "short int"
	.uleb128 0x3	# (DIE (0x33) DW_TAG_base_type)
	.byte	0x1	# DW_AT_byte_size
	.byte	0x6	# DW_AT_encoding
	.long	.LASF1	# DW_AT_name: "char"
	.uleb128 0x3	# (DIE (0x3a) DW_TAG_base_type)
	.byte	0x8	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.long	.LASF2	# DW_AT_name: "long long int"
	.uleb128 0x3	# (DIE (0x41) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x4	# DW_AT_encoding
	.long	.LASF3	# DW_AT_name: "float"
	.uleb128 0x3	# (DIE (0x48) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x7	# DW_AT_encoding
	.long	.LASF4	# DW_AT_name: "unsigned int"
	.uleb128 0x3	# (DIE (0x4f) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.long	.LASF5	# DW_AT_name: "long int"
	.uleb128 0x3	# (DIE (0x56) DW_TAG_base_type)
	.byte	0x1	# DW_AT_byte_size
	.byte	0x8	# DW_AT_encoding
	.long	.LASF6	# DW_AT_name: "unsigned char"
	.uleb128 0x3	# (DIE (0x5d) DW_TAG_base_type)
	.byte	0x2	# DW_AT_byte_size
	.byte	0x7	# DW_AT_encoding
	.long	.LASF7	# DW_AT_name: "short unsigned int"
	.uleb128 0x3	# (DIE (0x64) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x7	# DW_AT_encoding
	.long	.LASF8	# DW_AT_name: "long unsigned int"
	.uleb128 0x3	# (DIE (0x6b) DW_TAG_base_type)
	.byte	0x1	# DW_AT_byte_size
	.byte	0x6	# DW_AT_encoding
	.long	.LASF9	# DW_AT_name: "signed char"
	.uleb128 0x3	# (DIE (0x72) DW_TAG_base_type)
	.byte	0x8	# DW_AT_byte_size
	.byte	0x7	# DW_AT_encoding
	.long	.LASF10	# DW_AT_name: "long long unsigned int"
	.uleb128 0x3	# (DIE (0x79) DW_TAG_base_type)
	.byte	0x8	# DW_AT_byte_size
	.byte	0x4	# DW_AT_encoding
	.long	.LASF11	# DW_AT_name: "double"
	.uleb128 0x4	# (DIE (0x80) DW_TAG_subprogram)
			# DW_AT_external
	.long	.LASF18	# DW_AT_name: "main"
	.byte	0x1	# DW_AT_decl_file (ptwrite.c)
	.byte	0x21	# DW_AT_decl_line
	.byte	0x1	# DW_AT_decl_column
			# DW_AT_prototyped
	.long	0x25	# DW_AT_type
	.long	.LFB4023	# DW_AT_low_pc
	.long	.LFE4023-.LFB4023	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_tail_call_sites
	.uleb128 0x5	# (DIE (0x96) DW_TAG_subprogram)
			# DW_AT_external
	.long	.LASF12	# DW_AT_name: "ptwrite2"
	.byte	0x1	# DW_AT_decl_file (ptwrite.c)
	.byte	0x1b	# DW_AT_decl_line
	.byte	0x1	# DW_AT_decl_column
			# DW_AT_prototyped
	.long	.LFB4022	# DW_AT_low_pc
	.long	.LFE4022-.LFB4022	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_call_sites
	.long	0xd5	# DW_AT_sibling
	.uleb128 0x6	# (DIE (0xac) DW_TAG_formal_parameter)
	.long	.LASF14	# DW_AT_name: "value"
	.byte	0x1	# DW_AT_decl_file (ptwrite.c)
	.byte	0x1b	# DW_AT_decl_line
	.byte	0xf	# DW_AT_decl_column
	.long	0x25	# DW_AT_type
	.uleb128 0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.uleb128 0x7	# (DIE (0xbb) DW_TAG_inlined_subroutine)
	.long	0x114	# DW_AT_abstract_origin
	.long	.LBB8	# DW_AT_low_pc
	.long	.LBE8-.LBB8	# DW_AT_high_pc
	.byte	0x1	# DW_AT_call_file (ptwrite.c)
	.byte	0x1d	# DW_AT_call_line
	.byte	0x3	# DW_AT_call_column
	.uleb128 0x8	# (DIE (0xcb) DW_TAG_formal_parameter)
	.long	0x11e	# DW_AT_abstract_origin
	.uleb128 0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -12
	.byte	0	# end of children of DIE 0xbb
	.byte	0	# end of children of DIE 0x96
	.uleb128 0x5	# (DIE (0xd5) DW_TAG_subprogram)
			# DW_AT_external
	.long	.LASF13	# DW_AT_name: "ptwrite1"
	.byte	0x1	# DW_AT_decl_file (ptwrite.c)
	.byte	0x15	# DW_AT_decl_line
	.byte	0x1	# DW_AT_decl_column
			# DW_AT_prototyped
	.long	.LFB4021	# DW_AT_low_pc
	.long	.LFE4021-.LFB4021	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_call_sites
	.long	0x114	# DW_AT_sibling
	.uleb128 0x6	# (DIE (0xeb) DW_TAG_formal_parameter)
	.long	.LASF14	# DW_AT_name: "value"
	.byte	0x1	# DW_AT_decl_file (ptwrite.c)
	.byte	0x15	# DW_AT_decl_line
	.byte	0xf	# DW_AT_decl_column
	.long	0x25	# DW_AT_type
	.uleb128 0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.uleb128 0x7	# (DIE (0xfa) DW_TAG_inlined_subroutine)
	.long	0x114	# DW_AT_abstract_origin
	.long	.LBB6	# DW_AT_low_pc
	.long	.LBE6-.LBB6	# DW_AT_high_pc
	.byte	0x1	# DW_AT_call_file (ptwrite.c)
	.byte	0x17	# DW_AT_call_line
	.byte	0x3	# DW_AT_call_column
	.uleb128 0x8	# (DIE (0x10a) DW_TAG_formal_parameter)
	.long	0x11e	# DW_AT_abstract_origin
	.uleb128 0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -12
	.byte	0	# end of children of DIE 0xfa
	.byte	0	# end of children of DIE 0xd5
	.uleb128 0x9	# (DIE (0x114) DW_TAG_subprogram)
			# DW_AT_external
	.long	.LASF19	# DW_AT_name: "_ptwrite32"
	.byte	0x2	# DW_AT_decl_file (/usr/lib/gcc/x86_64-redhat-linux/10/include/immintrin.h)
	.value	0x131	# DW_AT_decl_line
	.byte	0x1	# DW_AT_decl_column
			# DW_AT_prototyped
	.byte	0x3	# DW_AT_inline
			# DW_AT_artificial
	.uleb128 0xa	# (DIE (0x11e) DW_TAG_formal_parameter)
	.ascii "__B\0"	# DW_AT_name
	.byte	0x2	# DW_AT_decl_file (/usr/lib/gcc/x86_64-redhat-linux/10/include/immintrin.h)
	.value	0x131	# DW_AT_decl_line
	.byte	0x16	# DW_AT_decl_column
	.long	0x48	# DW_AT_type
	.byte	0	# end of children of DIE 0x114
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
	.uleb128 0x6	# (DW_FORM_data4)
	.uleb128 0x10	# (DW_AT_stmt_list)
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.byte	0
	.byte	0
	.uleb128 0x2	# (abbrev code)
	.uleb128 0x24	# (TAG: DW_TAG_base_type)
	.byte	0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3e	# (DW_AT_encoding)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.byte	0
	.byte	0
	.uleb128 0x3	# (abbrev code)
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
	.uleb128 0x4	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0	# DW_children_no
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x39	# (DW_AT_decl_column)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x27	# (DW_AT_prototyped)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x6	# (DW_FORM_data4)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.uleb128 0x2116	# (DW_AT_GNU_all_tail_call_sites)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.byte	0
	.byte	0
	.uleb128 0x5	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x39	# (DW_AT_decl_column)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x27	# (DW_AT_prototyped)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x6	# (DW_FORM_data4)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.uleb128 0x2117	# (DW_AT_GNU_all_call_sites)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x6	# (abbrev code)
	.uleb128 0x5	# (TAG: DW_TAG_formal_parameter)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x39	# (DW_AT_decl_column)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.byte	0
	.byte	0
	.uleb128 0x7	# (abbrev code)
	.uleb128 0x1d	# (TAG: DW_TAG_inlined_subroutine)
	.byte	0x1	# DW_children_yes
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x6	# (DW_FORM_data4)
	.uleb128 0x58	# (DW_AT_call_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x59	# (DW_AT_call_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x57	# (DW_AT_call_column)
	.uleb128 0xb	# (DW_FORM_data1)
	.byte	0
	.byte	0
	.uleb128 0x8	# (abbrev code)
	.uleb128 0x5	# (TAG: DW_TAG_formal_parameter)
	.byte	0	# DW_children_no
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.byte	0
	.byte	0
	.uleb128 0x9	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0x5	# (DW_FORM_data2)
	.uleb128 0x39	# (DW_AT_decl_column)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x27	# (DW_AT_prototyped)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x20	# (DW_AT_inline)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x34	# (DW_AT_artificial)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.byte	0
	.byte	0
	.uleb128 0xa	# (abbrev code)
	.uleb128 0x5	# (TAG: DW_TAG_formal_parameter)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0x5	# (DW_FORM_data2)
	.uleb128 0x39	# (DW_AT_decl_column)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_aranges,"",@progbits
	.long	0x1c	# Length of Address Ranges Info
	.value	0x2	# DWARF aranges version
	.long	.Ldebug_info0	# Offset of Compilation Unit Info
	.byte	0x4	# Size of Address
	.byte	0	# Size of Segment Descriptor
	.value	0	# Pad to 8 byte boundary
	.value	0
	.long	.Ltext0	# Address
	.long	.Letext0-.Ltext0	# Length
	.long	0
	.long	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF2:
	.string	"long long int"
.LASF14:
	.string	"value"
.LASF19:
	.string	"_ptwrite32"
.LASF4:
	.string	"unsigned int"
.LASF17:
	.string	"gdb/gdb/testsuite/gdb.btrace"
.LASF18:
	.string	"main"
.LASF16:
	.string	"ptwrite.c"
.LASF8:
	.string	"long unsigned int"
.LASF10:
	.string	"long long unsigned int"
.LASF13:
	.string	"ptwrite1"
.LASF12:
	.string	"ptwrite2"
.LASF6:
	.string	"unsigned char"
.LASF1:
	.string	"char"
.LASF5:
	.string	"long int"
.LASF15:
	.string	"GNU C17 10.3.1 20210422 (Red Hat 10.3.1-1) -m32 -mptwrite -mtune=generic -march=i686 -g"
.LASF11:
	.string	"double"
.LASF7:
	.string	"short unsigned int"
.LASF9:
	.string	"signed char"
.LASF3:
	.string	"float"
.LASF0:
	.string	"short int"
	.ident	"GCC: (GNU) 10.3.1 20210422 (Red Hat 10.3.1-1)"
	.section	.note.GNU-stack,"",@progbits
