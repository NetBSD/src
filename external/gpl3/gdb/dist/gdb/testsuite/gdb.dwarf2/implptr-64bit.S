/* Copyright 2010-2016 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

	.section	.debug_info
d:
	/* Length of Compilation Unit Info */
#if OFFSET_SIZE == 4
# define OFFSET .4byte
# define HEADER_LINE1
# define HEADER_LINE2(END) .4byte	END - 1f
#elif OFFSET_SIZE == 8
# define OFFSET .8byte
# define HEADER_LINE1 .4byte	0xffffffff
# define HEADER_LINE2(END) .8byte	END - 1f
#else
# error
#endif
#if ADDR_SIZE == 4
# define ADDR .4byte
#elif ADDR_SIZE == 8
# define ADDR .8byte
#else
# error
#endif
#if REF_ADDR_SIZE == 4
# define REF_ADDR .4byte
#elif REF_ADDR_SIZE == 8
# define REF_ADDR .8byte
#else
# error
#endif
	
#if TWO_CU
# define END1 .Lcu_end_1
#else
# define END1 debug_end
#endif

	HEADER_LINE1
	HEADER_LINE2(END1)

1:
	.2byte	DWARF_VERSION	/* DWARF version number */
	OFFSET	.Ldebug_abbrev0	/* Offset Into Abbrev. Section */
	.byte	ADDR_SIZE	/* Pointer Size (in bytes) */

	.uleb128 0x1	/* (DIE (0xb) DW_TAG_compile_unit) */
	.ascii "GNU C 4.4.3\0"	/* DW_AT_producer */
	.byte	0x1	/* DW_AT_language */
	.ascii "1.c\0"	/* DW_AT_name */

.Ltype_int:
	.uleb128 0x7	/* DW_TAG_base_type */
	.byte	0x4	/* DW_AT_byte_size */
	.byte	0x5	/* DW_AT_encoding */
	.ascii "int\0"	/* DW_AT_name */

.Ltype_struct:
	.uleb128 0x2	/* DW_TAG_structure_type */
	.ascii "s\0"	/* DW_AT_name */
	.byte	4	/* DW_AT_byte_size */

	.uleb128 0x3	/* DW_TAG_member */
	.ascii "f\0"	/* DW_AT_name */
	.4byte	.Ltype_int - d	/* DW_AT_type */
	.byte	0	/* DW_AT_data_member_location */

	.byte	0x0	/* end of children of DW_TAG_structure_type */

.Ltype_structptr:
	.uleb128 0x5	/* DW_TAG_pointer_type */
	.byte	ADDR_SIZE	/* DW_AT_byte_size */
	.4byte	.Ltype_struct - d	/* DW_AT_type */

.Lvar_out:
	.uleb128 0x4	/* (DW_TAG_variable) */
	.ascii "v\0"	/* DW_AT_name */
	.byte	2f - 1f	/* DW_AT_location: DW_FORM_block1 */
1:
	.byte	0x9e	/* DW_OP_implicit_value */
	.uleb128  2f - 3f
3:
	.byte	1, 1, 1, 1
2:
	REF_ADDR	.Ltype_struct - d	/* DW_AT_type */

#if TWO_CU
	.byte	0x0	/* end of children of CU */
.Lcu_end_1:

	HEADER_LINE1
	HEADER_LINE2 (debug_end)

1:
	.2byte	DWARF_VERSION	/* DWARF version number */
	OFFSET	.Ldebug_abbrev0	/* Offset Into Abbrev. Section */
	.byte	ADDR_SIZE	/* Pointer Size (in bytes) */

	.uleb128 0x1	/* (DIE (0xb) DW_TAG_compile_unit) */
	.ascii "GNU C 4.4.3\0"	/* DW_AT_producer */
	.byte	0x1	/* DW_AT_language */
	.ascii "1.c\0"	/* DW_AT_name */
#endif

	.uleb128	6			/* Abbrev: DW_TAG_subprogram */
	.ascii		"main\0"		/* DW_AT_name */
	ADDR		main			/* DW_AT_low_pc */
	ADDR		main + 0x100		/* DW_AT_high_pc */
	REF_ADDR	.Ltype_int - d		/* DW_AT_type */
	.byte		1			/* DW_AT_external */

	.uleb128 0x4	/* (DW_TAG_variable) */
	.ascii "p\0"	/* DW_AT_name */
	.byte	2f - 1f	/* DW_AT_location: DW_FORM_block1 */
1:
	.byte	0xf2	/* DW_OP_GNU_implicit_pointer */
	REF_ADDR	.Lvar_out - d	/* referenced DIE */
	.sleb128	0	/* offset */
2:
	REF_ADDR	.Ltype_structptr - d	/* DW_AT_type */

	.byte	0x0	/* end of children of main */

	.byte	0x0	/* end of children of CU */
debug_end:

	.section	.debug_abbrev
.Ldebug_abbrev0:

	.uleb128 0x1	/* (abbrev code) */
	.uleb128 0x11	/* (TAG: DW_TAG_compile_unit) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x25	/* (DW_AT_producer) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x13	/* (DW_AT_language) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x2	/* (abbrev code) */
	.uleb128 0x13	/* (TAG: DW_TAG_structure_type) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0xb	/* (DW_AT_byte_size) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.byte	0
	.byte	0

	.uleb128 0x3	/* (abbrev code) */
	.uleb128 0xd	/* (TAG: DW_TAG_member) */
	.byte	0	/* DW_children_no */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x38	/* (DW_AT_data_member_location) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.byte	0
	.byte	0

	.uleb128 0x4	/* (abbrev code) */
	.uleb128 0x34	/* (TAG: DW_TAG_variable) */
	.byte	0x0	/* DW_children_yes */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x02	/* (DW_AT_location) */
	.uleb128 0xa	/* (DW_FORM_block1) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x10	/* (DW_FORM_ref_addr) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x5	/* (abbrev code) */
	.uleb128 0xf	/* (TAG: DW_TAG_pointer_type) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0xb	/* (DW_AT_byte_size) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0x0
	.byte	0x0

	.uleb128	6			/* Abbrev code */
	.uleb128	0x2e			/* DW_TAG_subprogram */
	.byte		1			/* has_children */
	.uleb128	0x3			/* DW_AT_name */
	.uleb128	0x8			/* DW_FORM_string */
	.uleb128	0x11			/* DW_AT_low_pc */
	.uleb128	0x1			/* DW_FORM_addr */
	.uleb128	0x12			/* DW_AT_high_pc */
	.uleb128	0x1			/* DW_FORM_addr */
	.uleb128	0x49			/* DW_AT_type */
	.uleb128	0x10			/* DW_FORM_ref_addr */
	.uleb128	0x3f			/* DW_AT_external */
	.uleb128	0xc			/* DW_FORM_flag */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.uleb128 0x7	/* (abbrev code) */
	.uleb128 0x24	/* (TAG: DW_TAG_base_type) */
	.byte	0	/* DW_children_no */
	.uleb128 0xb	/* (DW_AT_byte_size) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3e	/* (DW_AT_encoding) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.byte	0
	.byte	0

	.byte	0x0
