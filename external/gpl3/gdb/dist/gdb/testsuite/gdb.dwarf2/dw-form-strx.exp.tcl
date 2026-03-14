# Copyright 2025 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

load_lib dwarf.exp

# This test can only be run on targets which support DWARF-2 and use gas.
require dwarf2_support

standard_testfile main.c -dw.S

set asm_file [standard_output_file $srcfile2]

# Debug info in the main file.
Dwarf::assemble $asm_file {
    declare_labels base_offset_cu1

    debug_str_offsets { base_offset base_offset_cu1 } int

    cu {
	version 5
    } {
	DW_TAG_compile_unit {
	    {DW_AT_str_offsets_base $base_offset_cu1 sec_offset}
	} {
	    declare_labels int4_type

	    int4_type: DW_TAG_base_type {
		{DW_AT_byte_size 4 DW_FORM_sdata}
		{DW_AT_encoding  @DW_ATE_signed}
		{DW_AT_name      $::int_str_idx DW_FORM_strx_id}
	    }

	    DW_TAG_variable {
		{DW_AT_name global_var}
		{DW_AT_type :$int4_type}
		{DW_AT_location {
		    DW_OP_const1u 12
		    DW_OP_stack_value
		} SPECIAL_expr}
	    }
	}
    }
}

if { [prepare_for_testing "failed to prepare" ${testfile} \
	  [list $srcfile $asm_file] {nodebug}] } {
    return
}

# Let includers know prepare_for_testing was done, without having to check
# source return status.
set prepare_for_testing_done 1
