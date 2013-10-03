/* Copyright (C) 2012-2013 Free Software Foundation, Inc.

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

   This was created using clang -g -S dw2-restrict.c.

  */

	.file	"dw2-restrict.c"
	.section	.debug_frame,"",@progbits
.Lsection_debug_frame:
	.section	.debug_info,"",@progbits
.Lsection_info:
	.section	.debug_abbrev,"",@progbits
.Lsection_abbrev:
	.section	.debug_aranges,"",@progbits
	.section	.debug_macinfo,"",@progbits
	.section	.debug_line,"",@progbits
.Lsection_line:
	.section	.debug_loc,"",@progbits
	.section	.debug_pubnames,"",@progbits
	.section	.debug_pubtypes,"",@progbits
	.section	.debug_str,"",@progbits
.Lsection_str:
	.section	.debug_ranges,"",@progbits
.Ldebug_range:
	.section	.debug_loc,"",@progbits
.Lsection_debug_loc:
	.text
.Ltext_begin:
	.data
	.file	1 "dw2-restrict.c"
	.text
	.globl	f
	.align	16, 0x90
	.type	f,@function
f:                                      # @f
.Leh_func_begin0:
.Lfunc_begin0:
	.loc	1 19 0
.Ltmp0:
# BB#0:
	movq	%rdi, -8(%rsp)
	.loc	1 20 3
.Ltmp1:
	movq	-8(%rsp), %rdi          # dw2-restrict.c:20:3
	movsbl	(%rdi), %eax            # dw2-restrict.c:20:3
	ret                             # dw2-restrict.c:20:3
.Ltmp2:
.Ltmp3:
	.size	f, .Ltmp3-f
.Lfunc_end0:
.Leh_func_end0:

	.globl	main
	.align	16, 0x90
	.type	main,@function
main:                                   # @main
.Leh_func_begin1:
.Lfunc_begin1:
	.loc	1 24 0
.Ltmp7:
# BB#0:
	pushq	%rbp
.Ltmp4:
	movq	%rsp, %rbp
.Ltmp5:
	subq	$16, %rsp
.Ltmp6:
	leaq	.L.str, %rdi
	movl	$0, -4(%rbp)
	.loc	1 25 3
.Ltmp8:
	callq	f                       # dw2-restrict.c:25:3
	addq	$16, %rsp               # dw2-restrict.c:25:3
	popq	%rbp                    # dw2-restrict.c:25:3
	ret                             # dw2-restrict.c:25:3
.Ltmp9:
.Ltmp10:
	.size	main, .Ltmp10-main
.Lfunc_end1:
.Leh_func_end1:

	.type	.L.str,@object          # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	 "hi bob"
	.size	.L.str, 7

	.section	.eh_frame,"a",@progbits
.LEH_frame0:
.Lsection_eh_frame0:
.Leh_frame_common0:
.Lset0 = .Leh_frame_common_end0-.Leh_frame_common_begin0 # Length of Common Information Entry
	.long	.Lset0
.Leh_frame_common_begin0:
	.long	0                       # CIE Identifier Tag
	.byte	1                       # DW_CIE_VERSION
	.asciz	 "zR"                   # CIE Augmentation
	.byte	1                       # CIE Code Alignment Factor
	.byte	120                     # CIE Data Alignment Factor
	.byte	16                      # CIE Return Address Column
	.byte	1                       # Augmentation Size
	.byte	3                       # FDE Encoding = udata4
	.byte	12                      # DW_CFA_def_cfa
	.byte	7                       # Register
	.byte	8                       # Offset
	.byte	144                     # DW_CFA_offset + Reg (16)
	.byte	1                       # Offset
	.align	8
.Leh_frame_common_end0:
.Lf.eh:
.Lset1 = .Leh_frame_end0-.Leh_frame_begin0 # Length of Frame Information Entry
	.long	.Lset1
.Leh_frame_begin0:
.Lset2 = .Leh_frame_begin0-.Leh_frame_common0 # FDE CIE offset
	.long	.Lset2
	.long	.Leh_func_begin0        # FDE initial location
.Lset3 = .Leh_func_end0-.Leh_func_begin0 # FDE address range
	.long	.Lset3
	.byte	0                       # Augmentation size
	.align	8
.Leh_frame_end0:

.Lmain.eh:
.Lset4 = .Leh_frame_end1-.Leh_frame_begin1 # Length of Frame Information Entry
	.long	.Lset4
.Leh_frame_begin1:
.Lset5 = .Leh_frame_begin1-.Leh_frame_common0 # FDE CIE offset
	.long	.Lset5
	.long	.Leh_func_begin1        # FDE initial location
.Lset6 = .Leh_func_end1-.Leh_func_begin1 # FDE address range
	.long	.Lset6
	.byte	0                       # Augmentation size
	.byte	4                       # DW_CFA_advance_loc4
.Lset7 = .Ltmp4-.Leh_func_begin1
	.long	.Lset7
	.byte	14                      # DW_CFA_def_cfa_offset
	.byte	16                      # Offset
	.byte	134                     # DW_CFA_offset + Reg (6)
	.byte	2                       # Offset
	.byte	4                       # DW_CFA_advance_loc4
.Lset8 = .Ltmp5-.Ltmp4
	.long	.Lset8
	.byte	13                      # DW_CFA_def_cfa_register
	.byte	6                       # Register
	.align	8
.Leh_frame_end1:

	.text
.Ltext_end:
	.data
.Ldata_end:
	.text
.Lsection_end1:
	.section	.debug_frame,"",@progbits
.Ldebug_frame_common:
.Lset9 = .Ldebug_frame_common_end-.Ldebug_frame_common_begin # Length of Common Information Entry
	.long	.Lset9
.Ldebug_frame_common_begin:
	.long	-1                      # CIE Identifier Tag
	.byte	1                       # CIE Version
	.byte	0                       # CIE Augmentation
	.byte	1                       # CIE Code Alignment Factor
	.byte	120                     # CIE Data Alignment Factor
	.byte	16                      # CIE RA Column
	.byte	12                      # DW_CFA_def_cfa
	.byte	7                       # Register
	.byte	8                       # Offset
	.byte	144                     # DW_CFA_offset + Reg (16)
	.byte	1                       # Offset
	.align	4
.Ldebug_frame_common_end:
.Lset10 = .Ldebug_frame_end0-.Ldebug_frame_begin0 # Length of Frame Information Entry
	.long	.Lset10
.Ldebug_frame_begin0:
	.long	.Ldebug_frame_common    # FDE CIE offset
	.quad	.Lfunc_begin0           # FDE initial location
.Lset11 = .Lfunc_end0-.Lfunc_begin0     # FDE address range
	.quad	.Lset11
	.align	4
.Ldebug_frame_end0:
.Lset12 = .Ldebug_frame_end1-.Ldebug_frame_begin1 # Length of Frame Information Entry
	.long	.Lset12
.Ldebug_frame_begin1:
	.long	.Ldebug_frame_common    # FDE CIE offset
	.quad	.Lfunc_begin1           # FDE initial location
.Lset13 = .Lfunc_end1-.Lfunc_begin1     # FDE address range
	.quad	.Lset13
	.byte	4                       # DW_CFA_advance_loc4
.Lset14 = .Ltmp4-.Lfunc_begin1
	.long	.Lset14
	.byte	14                      # DW_CFA_def_cfa_offset
	.byte	16                      # Offset
	.byte	134                     # DW_CFA_offset + Reg (6)
	.byte	2                       # Offset
	.byte	4                       # DW_CFA_advance_loc4
.Lset15 = .Ltmp5-.Ltmp4
	.long	.Lset15
	.byte	13                      # DW_CFA_def_cfa_register
	.byte	6                       # Register
	.align	4
.Ldebug_frame_end1:
	.section	.debug_info,"",@progbits
.Linfo_begin1:
	.long	197                     # Length of Compilation Unit Info
	.short	2                       # DWARF version number
	.long	.Labbrev_begin          # Offset Into Abbrev. Section
	.byte	8                       # Address Size (in bytes)
	.byte	1                       # Abbrev [1] 0xb:0xba DW_TAG_compile_unit
	.ascii	 "clang version 2.9 (tags/RELEASE_29/final)" # DW_AT_producer
	.byte	0
	.short	12                      # DW_AT_language
	.ascii	 "dw2-restrict.c"       # DW_AT_name
	.byte	0
	.quad	0                       # DW_AT_entry_pc
	.quad	.Lsection_line          # DW_AT_stmt_list
	.ascii	 "/tmp"                 # DW_AT_comp_dir
	.byte	0
	.byte	2                       # Abbrev [2] 0x5c:0x7 DW_TAG_base_type
	.byte	5                       # DW_AT_encoding
	.ascii	 "int"                  # DW_AT_name
	.byte	0
	.byte	4                       # DW_AT_byte_size
	.byte	3                       # Abbrev [3] 0x63:0x2f DW_TAG_subprogram
	.long	146                     # DW_AT_sibling
	.byte	102                     # DW_AT_name
	.byte	0
	.byte	1                       # DW_AT_decl_file
	.byte	19                      # DW_AT_decl_line
	.byte	1                       # DW_AT_prototyped
	.long	92                      # DW_AT_type
	.byte	1                       # DW_AT_external
	.quad	.Lfunc_begin0           # DW_AT_low_pc
	.quad	.Lfunc_end0             # DW_AT_high_pc
	.byte	1                       # DW_AT_frame_base
	.byte	87
	.byte	1                       # DW_AT_APPLE_omit_frame_ptr
	.byte	4                       # Abbrev [4] 0x85:0xc DW_TAG_formal_parameter
	.byte	120                     # DW_AT_name
	.byte	0
	.byte	1                       # DW_AT_decl_file
	.byte	18                      # DW_AT_decl_line
	.long	191                     # DW_AT_type
	.byte	2                       # DW_AT_location
	.byte	145
	.byte	120
	.byte	0                       # End Of Children Mark
	.byte	5                       # Abbrev [5] 0x92:0x1f DW_TAG_subprogram
	.ascii	 "main"                 # DW_AT_name
	.byte	0
	.byte	1                       # DW_AT_decl_file
	.byte	24                      # DW_AT_decl_line
	.long	92                      # DW_AT_type
	.byte	1                       # DW_AT_external
	.quad	.Lfunc_begin1           # DW_AT_low_pc
	.quad	.Lfunc_end1             # DW_AT_high_pc
	.byte	1                       # DW_AT_frame_base
	.byte	86
	.byte	2                       # Abbrev [2] 0xb1:0x8 DW_TAG_base_type
	.byte	6                       # DW_AT_encoding
	.ascii	 "char"                 # DW_AT_name
	.byte	0
	.byte	1                       # DW_AT_byte_size
	.byte	6                       # Abbrev [6] 0xb9:0x6 DW_TAG_pointer_type
	.long	177                     # DW_AT_type
	.byte	8                       # DW_AT_byte_size
	.byte	7                       # Abbrev [7] 0xbf:0x5 DW_TAG_restrict_type
	.long	185                     # DW_AT_type
	.byte	0                       # End Of Children Mark
	.byte	0                       # 4 extra padding bytes for GDB
	.byte	0
	.byte	0
	.byte	0
.Linfo_end1:
	.section	.debug_abbrev,"",@progbits
.Labbrev_begin:
	.byte	1                       # Abbreviation Code
	.byte	17                      # DW_TAG_compile_unit
	.byte	1                       # DW_CHILDREN_yes
	.byte	37                      # DW_AT_producer
	.byte	8                       # DW_FORM_string
	.byte	19                      # DW_AT_language
	.byte	5                       # DW_FORM_data2
	.byte	3                       # DW_AT_name
	.byte	8                       # DW_FORM_string
	.byte	82                      # DW_AT_entry_pc
	.byte	1                       # DW_FORM_addr
	.byte	16                      # DW_AT_stmt_list
	.byte	1                       # DW_FORM_addr
	.byte	27                      # DW_AT_comp_dir
	.byte	8                       # DW_FORM_string
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	2                       # Abbreviation Code
	.byte	36                      # DW_TAG_base_type
	.byte	0                       # DW_CHILDREN_no
	.byte	62                      # DW_AT_encoding
	.byte	11                      # DW_FORM_data1
	.byte	3                       # DW_AT_name
	.byte	8                       # DW_FORM_string
	.byte	11                      # DW_AT_byte_size
	.byte	11                      # DW_FORM_data1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	3                       # Abbreviation Code
	.byte	46                      # DW_TAG_subprogram
	.byte	1                       # DW_CHILDREN_yes
	.byte	1                       # DW_AT_sibling
	.byte	19                      # DW_FORM_ref4
	.byte	3                       # DW_AT_name
	.byte	8                       # DW_FORM_string
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	39                      # DW_AT_prototyped
	.byte	12                      # DW_FORM_flag
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	63                      # DW_AT_external
	.byte	12                      # DW_FORM_flag
	.byte	17                      # DW_AT_low_pc
	.byte	1                       # DW_FORM_addr
	.byte	18                      # DW_AT_high_pc
	.byte	1                       # DW_FORM_addr
	.byte	64                      # DW_AT_frame_base
	.byte	10                      # DW_FORM_block1
	.ascii	 "\347\177"             # DW_AT_APPLE_omit_frame_ptr
	.byte	12                      # DW_FORM_flag
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	4                       # Abbreviation Code
	.byte	5                       # DW_TAG_formal_parameter
	.byte	0                       # DW_CHILDREN_no
	.byte	3                       # DW_AT_name
	.byte	8                       # DW_FORM_string
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	2                       # DW_AT_location
	.byte	10                      # DW_FORM_block1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	5                       # Abbreviation Code
	.byte	46                      # DW_TAG_subprogram
	.byte	0                       # DW_CHILDREN_no
	.byte	3                       # DW_AT_name
	.byte	8                       # DW_FORM_string
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	63                      # DW_AT_external
	.byte	12                      # DW_FORM_flag
	.byte	17                      # DW_AT_low_pc
	.byte	1                       # DW_FORM_addr
	.byte	18                      # DW_AT_high_pc
	.byte	1                       # DW_FORM_addr
	.byte	64                      # DW_AT_frame_base
	.byte	10                      # DW_FORM_block1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	6                       # Abbreviation Code
	.byte	15                      # DW_TAG_pointer_type
	.byte	0                       # DW_CHILDREN_no
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	11                      # DW_AT_byte_size
	.byte	11                      # DW_FORM_data1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	7                       # Abbreviation Code
	.byte	55                      # DW_TAG_restrict_type
	.byte	0                       # DW_CHILDREN_no
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	0                       # EOM(3)
.Labbrev_end:
	.section	.debug_pubnames,"",@progbits
.Lset16 = .Lpubnames_end1-.Lpubnames_begin1 # Length of Public Names Info
	.long	.Lset16
.Lpubnames_begin1:
	.short	2                       # DWARF Version
	.long	.Linfo_begin1           # Offset of Compilation Unit Info
.Lset17 = .Linfo_end1-.Linfo_begin1     # Compilation Unit Length
	.long	.Lset17
	.long	146                     # DIE offset
	.asciz	 "main"                 # External Name
	.long	99                      # DIE offset
	.asciz	 "f"                    # External Name
	.long	0                       # End Mark
.Lpubnames_end1:
	.section	.debug_pubtypes,"",@progbits
.Lset18 = .Lpubtypes_end1-.Lpubtypes_begin1 # Length of Public Types Info
	.long	.Lset18
.Lpubtypes_begin1:
	.short	2                       # DWARF Version
	.long	.Linfo_begin1           # Offset of Compilation Unit Info
.Lset19 = .Linfo_end1-.Linfo_begin1     # Compilation Unit Length
	.long	.Lset19
	.long	0                       # End Mark
.Lpubtypes_end1:
	.section	.debug_aranges,"",@progbits
	.section	.debug_ranges,"",@progbits
	.section	.debug_macinfo,"",@progbits

	.section	".note.GNU-stack","",@progbits
