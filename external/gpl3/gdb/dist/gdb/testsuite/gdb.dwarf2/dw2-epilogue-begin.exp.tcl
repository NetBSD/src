# Copyright 2022-2024 Free Software Foundation, Inc.

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

# Check that GDB can honor the epilogue_begin flag the compiler can place
# in the line-table data.
# We test 2 things: 1. that a software watchpoint triggered in an epilogue
# is correctly ignored
# 2. that GDB can mark the same line as both prologue and epilogue

load_lib dwarf.exp

# This test can only be run on targets which support DWARF-2 and use gas.
require dwarf2_support
# restricted to x86 to make it simpler to follow a variable
require is_x86_64_m64_target

set trivial_line [gdb_get_line_number "trivial function"]
set main_prologue [gdb_get_line_number "main prologue"]
set main_epilogue [gdb_get_line_number "main end"]
set watch_start_line [gdb_get_line_number "watch start"]

set asm_file [standard_output_file $srcfile2]

# The producer will be set to clang because at the time of writing
# we only care about epilogues if the producer is clang.  When the
# producer is GCC, variables use CFA locations, so watchpoints can
# continue working even on epilogues.
Dwarf::assemble $asm_file {
    global srcdir subdir srcfile srcfile2
    global trivial_line main_prologue main_epilogue watch_start_line
    declare_labels lines_label

    get_func_info main
    get_func_info trivial
    get_func_info watch

    if { $::version == 1 } {
	set switch_file {}
    } elseif { $::version == 2 } {
	set switch_file { set f $f2 }
    } else {
	error "Unhandled version: $::version"
    }

    cu {} {
	compile_unit {
	    {language @DW_LANG_C}
	    {name dw2-prologue-end.c}
	    {stmt_list ${lines_label} DW_FORM_sec_offset}
	    {producer "clang version 17.0.1"}
	} {
	    declare_labels char_label

	    char_label: base_type {
		{name char}
		{encoding @DW_ATE_signed}
		{byte_size 1 DW_FORM_sdata}
	    }

	    subprogram {
		{external 1 flag}
		{name trivial}
		{low_pc $trivial_start addr}
		{high_pc "$trivial_start + $trivial_len" addr}
	    }
	    subprogram {
		{external 1 flag}
		{name watch}
		{low_pc $watch_start addr}
		{high_pc "$watch_start + $watch_len" addr}
	    } {
		DW_TAG_variable {
		    {name local}
		    {type :$char_label}
		    {DW_AT_location {DW_OP_reg0} SPECIAL_expr}
		}
	    }
	    subprogram {
		{external 1 flag}
		{name main}
		{low_pc $main_start addr}
		{high_pc "$main_start + $main_len" addr}
	    }
	}
    }

    lines {version 5} lines_label {
	set diridx [include_dir "${srcdir}/${subdir}"]
	set f1 [file_name "$srcfile" $diridx]
	set f2 [file_name "$srcfile.inc" $diridx]

	set f $f1
	program {
	    DW_LNS_set_file $f

	    DW_LNE_set_address $trivial_start
	    line $trivial_line
	    DW_LNS_set_prologue_end
	    DW_LNS_set_epilogue_begin
	    DW_LNS_copy

	    DW_LNE_set_address $trivial_end
	    DW_LNE_end_sequence


	    DW_LNS_set_file $f

	    DW_LNE_set_address $watch_start
	    line $watch_start_line
	    DW_LNS_copy

	    DW_LNE_set_address watch_start
	    line [gdb_get_line_number "watch assign"]
	    DW_LNS_set_prologue_end
	    DW_LNS_copy

	    eval $switch_file
	    DW_LNS_set_file $f

	    DW_LNE_set_address watch_reassign
	    line [gdb_get_line_number "watch reassign"]
	    DW_LNS_set_epilogue_begin
	    DW_LNS_copy

	    DW_LNE_set_address watch_end
	    line [gdb_get_line_number "watch end"]
	    DW_LNS_copy

	    DW_LNE_set_address $watch_end
	    DW_LNE_end_sequence


	    DW_LNS_set_file $f

	    DW_LNE_set_address $main_start
	    line $main_prologue
	    DW_LNS_set_prologue_end
	    DW_LNS_copy

	    DW_LNE_set_address main_fun_call
	    line [gdb_get_line_number "main function call"]
	    DW_LNS_copy

	    DW_LNE_set_address main_epilogue
	    line $main_epilogue
	    DW_LNS_set_epilogue_begin
	    DW_LNS_copy

	    DW_LNE_set_address $main_end
	    DW_LNE_end_sequence
	}
    }
}

if { [prepare_for_testing "failed to prepare" ${testfile} \
	  [list $srcfile $asm_file] {nodebug}] } {
    return -1
}

if ![runto_main] {
    return -1
}

# Moving to the scope with a local variable.

gdb_breakpoint $srcfile:$watch_start_line
gdb_continue_to_breakpoint "continuing to function" ".*"
gdb_test "next" "local = 2.*" "stepping to epilogue"

# Forcing software watchpoints because hardware ones don't care if we
# are in the epilogue or not.
gdb_test_no_output "set can-use-hw-watchpoints 0"

# Test that the software watchpoint will not trigger in this case
gdb_test "watch local" "\[W|w\]atchpoint .: local" "set watchpoint"
gdb_test "continue" ".*\[W|w\]atchpoint . deleted.*" \
    "confirm watchpoint doesn't trigger"

# First we test that the trivial function has a line with both a prologue
# and an epilogue. Do this by finding a line that has 3 Y columns
set sep "\[ \t\]"
set hex_number "0x\[0-9a-f\]+"
gdb_test_multiple "maint info line-table" "test epilogue in linetable" -lbl {
    -re "\[0-9\]$sep+$trivial_line$sep+$hex_number$sep+$hex_number$sep+Y$sep+Y$sep+Y" {
	pass $gdb_test_name
    }
}
