/* This testcase is part of GDB, the GNU debugger.

   Copyright 2012-2020 Free Software Foundation, Inc.

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

   This file was created by doing:

   clang fission-multi-cu*.c -g -fno-split-dwarf-inlining -emit-llvm -S -c
   llvm-link fission-multi-cu*.ll -S -o fission-multi-cu.ll
   clang-tot fission-multi-cu.ll -gsplit-dwarf -S

   and then massaging the output.
*/
	.text
	.file	"llvm-link"
	.globl	func
	.p2align	4, 0x90
	.type	func,@function
func:                                   # @func
.Lfunc_begin0:
	.file	1 "fission-multi-cu1.c"
	.loc	1 20 0                  # fission-multi-cu1.c:20:0
	.cfi_startproc
# BB#0:                                 # %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	%edi, -4(%rbp)
	.loc	1 21 10 prologue_end    # fission-multi-cu1.c:21:10
	movl	-4(%rbp), %edi
	.loc	1 21 14 is_stmt 0       # fission-multi-cu1.c:21:14
	addl	$1, %edi
	.loc	1 21 3                  # fission-multi-cu1.c:21:3
	movl	%edi, %eax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	func, .Lfunc_end0-func
	.cfi_endproc

	.globl	main
	.p2align	4, 0x90
	.type	main,@function
main:                                   # @main
.Lfunc_begin1:
	.file	2 "fission-multi-cu2.c"
	.loc	2 23 0 is_stmt 1        # fission-multi-cu2.c:23:0
	.cfi_startproc
# BB#0:                                 # %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	$4294967295, %edi       # imm = 0xFFFFFFFF
	.loc	2 24 3 prologue_end     # fission-multi-cu2.c:24:3
	movb	$0, %al
	callq	func
	xorl	%eax, %eax
	.loc	2 25 1                  # fission-multi-cu2.c:25:1
	popq	%rbp
	retq
.Lfunc_end1:
	.size	main, .Lfunc_end1-main
	.cfi_endproc

	.section	.debug_str,"MS",@progbits,1
.Lskel_string0:
	.asciz	"fission-multi-cu.dwo"  # string offset=0
.Lskel_string1:
	.asciz	"/tmp/src/gdb/testsuite" # string offset=21
	.section	.debug_loc.dwo,"",@progbits
	.section	.debug_abbrev,"",@progbits
	.byte	1                       # Abbreviation Code
	.byte	17                      # DW_TAG_compile_unit
	.byte	0                       # DW_CHILDREN_no
	.byte	16                      # DW_AT_stmt_list
	.byte	23                      # DW_FORM_sec_offset
	.ascii	"\260B"                 # DW_AT_GNU_dwo_name
	.byte	14                      # DW_FORM_strp
	.byte	27                      # DW_AT_comp_dir
	.byte	14                      # DW_FORM_strp
	.ascii	"\261B"                 # DW_AT_GNU_dwo_id
	.byte	7                       # DW_FORM_data8
	.ascii	"\263B"                 # DW_AT_GNU_addr_base
	.byte	23                      # DW_FORM_sec_offset
	.byte	17                      # DW_AT_low_pc
	.byte	1                       # DW_FORM_addr
	.byte	18                      # DW_AT_high_pc
	.byte	6                       # DW_FORM_data4
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	0                       # EOM(3)
	.section	.debug_info,"",@progbits
.Lcu_begin0:
	.long	44                      # Length of Unit
	.short	4                       # DWARF version number
	.long	.debug_abbrev           # Offset Into Abbrev. Section
	.byte	8                       # Address Size (in bytes)
	.byte	1                       # Abbrev [1] 0xb:0x25 DW_TAG_compile_unit
	.long	.Lline_table_start0     # DW_AT_stmt_list
	.long	.Lskel_string0          # DW_AT_GNU_dwo_name
	.long	.Lskel_string1          # DW_AT_comp_dir
	.quad	7615852067747431413     # DW_AT_GNU_dwo_id
	.long	.debug_addr             # DW_AT_GNU_addr_base
	.quad	.Lfunc_begin0           # DW_AT_low_pc
	.long	.Lfunc_end0-.Lfunc_begin0 # DW_AT_high_pc
.Lcu_begin1:
	.long	44                      # Length of Unit
	.short	4                       # DWARF version number
	.long	.debug_abbrev           # Offset Into Abbrev. Section
	.byte	8                       # Address Size (in bytes)
	.byte	1                       # Abbrev [1] 0xb:0x25 DW_TAG_compile_unit
	.long	.Lline_table_start0     # DW_AT_stmt_list
	.long	.Lskel_string0          # DW_AT_GNU_dwo_name
	.long	.Lskel_string1          # DW_AT_comp_dir
	.quad	2037650261599692324     # DW_AT_GNU_dwo_id
	.long	.debug_addr             # DW_AT_GNU_addr_base
	.quad	.Lfunc_begin1           # DW_AT_low_pc
	.long	.Lfunc_end1-.Lfunc_begin1 # DW_AT_high_pc
	.section	.debug_ranges,"",@progbits
	.section	.debug_macinfo,"",@progbits
.Lcu_macro_begin1:
.Lcu_macro_begin3:
	.byte	0                       # End Of Macro List Mark
	.section	.debug_str.dwo,"MS",@progbits,1
.Linfo_string0:
	.asciz	"fission-multi-cu.dwo"  # string offset=0
.Linfo_string1:
	.asciz	"clang version 5.0.0 (trunk 302855) (llvm/trunk 302853)" # string offset=21
.Linfo_string2:
	.asciz	"fission-multi-cu1.c" # string offset=76
.Linfo_string3:
	.asciz	"fission-multi-cu2.c" # string offset=96
.Linfo_string4:
	.asciz	"func"                  # string offset=116
.Linfo_string5:
	.asciz	"int"                   # string offset=121
.Linfo_string6:
	.asciz	"main"                  # string offset=125
.Linfo_string7:
	.asciz	"arg"                   # string offset=130
	.section	.debug_str_offsets.dwo,"",@progbits
	.long	0
	.long	21
	.long	76
	.long	96
	.long	116
	.long	121
	.long	125
	.long	130
	.section	.debug_info.dwo,"",@progbits
	.long	53                      # Length of Unit
	.short	4                       # DWARF version number
	.long	0                       # Offset Into Abbrev. Section
	.byte	8                       # Address Size (in bytes)
	.byte	1                       # Abbrev [1] 0xb:0x2e DW_TAG_compile_unit
	.byte	0                       # DW_AT_GNU_dwo_name
	.byte	1                       # DW_AT_producer
	.short	12                      # DW_AT_language
	.byte	2                       # DW_AT_name
	.quad	7615852067747431413     # DW_AT_GNU_dwo_id
	.byte	2                       # Abbrev [2] 0x19:0x1b DW_TAG_subprogram
	.byte	0                       # DW_AT_low_pc
	.long	.Lfunc_end0-.Lfunc_begin0 # DW_AT_high_pc
	.byte	1                       # DW_AT_frame_base
	.byte	86
	.byte	4                       # DW_AT_name
	.byte	1                       # DW_AT_decl_file
	.byte	19                      # DW_AT_decl_line
                                        # DW_AT_prototyped
	.long	52                      # DW_AT_type
                                        # DW_AT_external
	.byte	3                       # Abbrev [3] 0x28:0xb DW_TAG_formal_parameter
	.byte	2                       # DW_AT_location
	.byte	145
	.byte	124
	.byte	7                       # DW_AT_name
	.byte	1                       # DW_AT_decl_file
	.byte	19                      # DW_AT_decl_line
	.long	52                      # DW_AT_type
	.byte	0                       # End Of Children Mark
	.byte	4                       # Abbrev [4] 0x34:0x4 DW_TAG_base_type
	.byte	5                       # DW_AT_name
	.byte	5                       # DW_AT_encoding
	.byte	4                       # DW_AT_byte_size
	.byte	0                       # End Of Children Mark
	.long	41                      # Length of Unit
	.short	4                       # DWARF version number
	.long	0                       # Offset Into Abbrev. Section
	.byte	8                       # Address Size (in bytes)
	.byte	1                       # Abbrev [1] 0xb:0x22 DW_TAG_compile_unit
	.byte	0                       # DW_AT_GNU_dwo_name
	.byte	1                       # DW_AT_producer
	.short	12                      # DW_AT_language
	.byte	3                       # DW_AT_name
	.quad	2037650261599692324     # DW_AT_GNU_dwo_id
	.byte	5                       # Abbrev [5] 0x19:0xf DW_TAG_subprogram
	.byte	1                       # DW_AT_low_pc
	.long	.Lfunc_end1-.Lfunc_begin1 # DW_AT_high_pc
	.byte	1                       # DW_AT_frame_base
	.byte	86
	.byte	6                       # DW_AT_name
	.byte	2                       # DW_AT_decl_file
	.byte	22                      # DW_AT_decl_line
	.long	40                      # DW_AT_type
                                        # DW_AT_external
	.byte	4                       # Abbrev [4] 0x28:0x4 DW_TAG_base_type
	.byte	5                       # DW_AT_name
	.byte	5                       # DW_AT_encoding
	.byte	4                       # DW_AT_byte_size
	.byte	0                       # End Of Children Mark
	.section	.debug_abbrev.dwo,"",@progbits
	.byte	1                       # Abbreviation Code
	.byte	17                      # DW_TAG_compile_unit
	.byte	1                       # DW_CHILDREN_yes
	.ascii	"\260B"                 # DW_AT_GNU_dwo_name
	.ascii	"\202>"                 # DW_FORM_GNU_str_index
	.byte	37                      # DW_AT_producer
	.ascii	"\202>"                 # DW_FORM_GNU_str_index
	.byte	19                      # DW_AT_language
	.byte	5                       # DW_FORM_data2
	.byte	3                       # DW_AT_name
	.ascii	"\202>"                 # DW_FORM_GNU_str_index
	.ascii	"\261B"                 # DW_AT_GNU_dwo_id
	.byte	7                       # DW_FORM_data8
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	2                       # Abbreviation Code
	.byte	46                      # DW_TAG_subprogram
	.byte	1                       # DW_CHILDREN_yes
	.byte	17                      # DW_AT_low_pc
	.ascii	"\201>"                 # DW_FORM_GNU_addr_index
	.byte	18                      # DW_AT_high_pc
	.byte	6                       # DW_FORM_data4
	.byte	64                      # DW_AT_frame_base
	.byte	24                      # DW_FORM_exprloc
	.byte	3                       # DW_AT_name
	.ascii	"\202>"                 # DW_FORM_GNU_str_index
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	39                      # DW_AT_prototyped
	.byte	25                      # DW_FORM_flag_present
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	63                      # DW_AT_external
	.byte	25                      # DW_FORM_flag_present
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	3                       # Abbreviation Code
	.byte	5                       # DW_TAG_formal_parameter
	.byte	0                       # DW_CHILDREN_no
	.byte	2                       # DW_AT_location
	.byte	24                      # DW_FORM_exprloc
	.byte	3                       # DW_AT_name
	.ascii	"\202>"                 # DW_FORM_GNU_str_index
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	4                       # Abbreviation Code
	.byte	36                      # DW_TAG_base_type
	.byte	0                       # DW_CHILDREN_no
	.byte	3                       # DW_AT_name
	.ascii	"\202>"                 # DW_FORM_GNU_str_index
	.byte	62                      # DW_AT_encoding
	.byte	11                      # DW_FORM_data1
	.byte	11                      # DW_AT_byte_size
	.byte	11                      # DW_FORM_data1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	5                       # Abbreviation Code
	.byte	46                      # DW_TAG_subprogram
	.byte	0                       # DW_CHILDREN_no
	.byte	17                      # DW_AT_low_pc
	.ascii	"\201>"                 # DW_FORM_GNU_addr_index
	.byte	18                      # DW_AT_high_pc
	.byte	6                       # DW_FORM_data4
	.byte	64                      # DW_AT_frame_base
	.byte	24                      # DW_FORM_exprloc
	.byte	3                       # DW_AT_name
	.ascii	"\202>"                 # DW_FORM_GNU_str_index
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	63                      # DW_AT_external
	.byte	25                      # DW_FORM_flag_present
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	0                       # EOM(3)
	.section	.debug_line.dwo,"",@progbits
.Ltmp4:
	.long	(.Ltmp5-.Ltmp4)-4
	.short	2
	.long	(.Ltmp6-.Ltmp4)-10
	.byte	1
	.byte	1
	.byte	-5
	.byte	14
	.byte	1
	.byte	0
	.byte	0
.Ltmp6:
.Ltmp5:
	.section	.debug_addr,"",@progbits
	.quad	.Lfunc_begin0
	.quad	.Lfunc_begin1
	.section	.debug_pubnames,"",@progbits
	.long	.LpubNames_end0-.LpubNames_begin0 # Length of Public Names Info
.LpubNames_begin0:
	.short	2                       # DWARF Version
	.long	.Lcu_begin0             # Offset of Compilation Unit Info
	.long	48                      # Compilation Unit Length
	.long	25                      # DIE offset
	.asciz	"func"                  # External Name
	.long	0                       # End Mark
.LpubNames_end0:
	.long	.LpubNames_end1-.LpubNames_begin1 # Length of Public Names Info
.LpubNames_begin1:
	.short	2                       # DWARF Version
	.long	.Lcu_begin1             # Offset of Compilation Unit Info
	.long	48                      # Compilation Unit Length
	.long	25                      # DIE offset
	.asciz	"main"                  # External Name
	.long	0                       # End Mark
.LpubNames_end1:
	.section	.debug_pubtypes,"",@progbits
	.long	.LpubTypes_end0-.LpubTypes_begin0 # Length of Public Types Info
.LpubTypes_begin0:
	.short	2                       # DWARF Version
	.long	.Lcu_begin0             # Offset of Compilation Unit Info
	.long	48                      # Compilation Unit Length
	.long	52                      # DIE offset
	.asciz	"int"                   # External Name
	.long	0                       # End Mark
.LpubTypes_end0:
	.long	.LpubTypes_end1-.LpubTypes_begin1 # Length of Public Types Info
.LpubTypes_begin1:
	.short	2                       # DWARF Version
	.long	.Lcu_begin1             # Offset of Compilation Unit Info
	.long	48                      # Compilation Unit Length
	.long	40                      # DIE offset
	.asciz	"int"                   # External Name
	.long	0                       # End Mark
.LpubTypes_end1:

	.ident	"clang version 5.0.0 (trunk 302855) (llvm/trunk 302853)"
	.ident	"clang version 5.0.0 (trunk 302855) (llvm/trunk 302853)"
	.section	".note.GNU-stack","",@progbits
	.section	.debug_line,"",@progbits
.Lline_table_start0:
