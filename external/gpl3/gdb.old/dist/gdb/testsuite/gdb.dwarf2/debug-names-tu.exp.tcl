# Copyright 2022-2023 Free Software Foundation, Inc.

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
if {![dwarf2_support]} {
    return 0
}

standard_testfile _start.c debug-names.S

set func_info_vars \
    [get_func_info _start [list debug additional_flags=-nostartfiles]]

# Create the DWARF.
set asm_file [standard_output_file $srcfile2]
Dwarf::assemble {
    filename $asm_file
    add_dummy_cus 0
} {
    global dwarf_version
    global func_info_vars
    foreach var $func_info_vars {
	global $var
    }

    cu { label cu_label version $dwarf_version } {
	compile_unit {{language @DW_LANG_C}} {
	    subprogram {
		{DW_AT_name _start}
		{DW_AT_low_pc $_start_start DW_FORM_addr}
		{DW_AT_high_pc $_start_end DW_FORM_addr}
	    }
	}
    }

    tu { label tu_label version $dwarf_version } 0x8ece66f4224fddb3 "" {
	type_unit {} {
	    declare_labels int_type

	    structure_type {
		{name struct_with_int_member}
		{byte_size 4 sdata}
	    } {
		member {
		    {name member}
		    {type :$int_type}
		}
	    }
	    int_type: base_type {
		{name int}
		{encoding @DW_ATE_signed}
		{byte_size 4 sdata}
	    }
	}
    }

    debug_names {} {
	cu cu_label
	tu tu_label
	name _start subprogram cu_label 0xEDDB6232
	name struct_with_int_member structure_type tu_label 0x53A2AE86
    }
}

if [prepare_for_testing "failed to prepare" $testfile "${asm_file} ${srcfile}" \
	[list additional_flags=-nostartfiles]] {
    return -1
}

# Verify that .debug_names section is not ignored.
set index [have_index $binfile]
gdb_assert { [string equal $index "debug_names"] } ".debug_names used"

# Verify that we can find the type in the type unit.
set re \
    [multi_line \
	 "type = struct struct_with_int_member {" \
	 "    int member;" \
	 "}"]
gdb_test "ptype struct struct_with_int_member" $re
