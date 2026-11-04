# This testcase is part of GDB, the GNU debugger.

# Copyright 2022-2025 Free Software Foundation, Inc.

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

# Generate binaries imitating different ways source file paths can be passed to
# compilers.  Test printing macros from those binaries.
#
# The entry points for this test are in the various
# gdb.dwarf2/macro-source-path-*.exp files.

standard_testfile macro-source-path.c

lassign [function_range main $srcdir/$subdir/$srcfile] \
    main_start main_len

# Run one test.
#
#  - TEST_NAME is the name of the test, used to differentiate the binaries.
#  - LINES_VERSION is the version of the version of the .debug_line section to
#    generate.
#  - DW_AT_NAME is the string to put in the compilation unit's DW_AT_name
#    attribute.
#  - MAIN_FILE_IDX is the file index the .debug_line and .debug_macro sections
#    will use to refer to the main file.
#  - DIRECTORIES is a list of directories to put in the .debug_line section
#    header
#  - FILE_NAMES is a list of {name, directory index} pairs describing the files
#    names to put in the .debug_line section header.

proc do_test { test_name lines_version DW_AT_name main_file_idx directories
	       file_names } {
    with_test_prefix "test_name=$test_name" {
	foreach_with_prefix is_64 {true false} {
	    # So we can access them in Dwarf::assemble...
	    set ::lines_version $lines_version
	    set ::DW_AT_name $DW_AT_name
	    set ::main_file_idx $main_file_idx
	    set ::directories $directories
	    set ::file_names $file_names
	    set ::is_64 $is_64
	    set 32_or_64 [expr $is_64 ? 64 : 32]

	    set asm_file [standard_output_file ${::testfile}-${test_name}-${32_or_64}.S]
	    Dwarf::assemble $asm_file {
		declare_labels Llines cu_macros

		# DW_AT_comp_dir is always the current working directory
		# from which the compiler was invoked.  We pretend the compiler was
		# always launched from /tmp/cwd.
		set comp_dir "/tmp/cwd"

		cu {} {
		    DW_TAG_compile_unit {
			    {DW_AT_producer "My C Compiler"}
			    {DW_AT_language @DW_LANG_C11}
			    {DW_AT_name $::DW_AT_name}
			    {DW_AT_comp_dir $comp_dir}
			    {DW_AT_stmt_list $Llines DW_FORM_sec_offset}
			    {DW_AT_macros $cu_macros DW_FORM_sec_offset}
		    } {
			declare_labels int_type

			int_type: DW_TAG_base_type {
			    {DW_AT_byte_size 4 DW_FORM_sdata}
			    {DW_AT_encoding  @DW_ATE_signed}
			    {DW_AT_name int}
			}

			DW_TAG_subprogram {
			    {MACRO_AT_func {main}}
			    {type :$int_type}
			}
		    }
		}

		# Define the .debug_line section.
		lines [list version $::lines_version] "Llines" {
		    foreach directory $::directories {
			include_dir $directory
		    }

		    foreach file_name $::file_names {
			lassign $file_name name dir_index
			file_name $name $dir_index
		    }

		    # A line number program just good enough so that GDB can
		    # figure out we are stopped in main.
		    program {
			DW_LNS_set_file $::main_file_idx
			DW_LNE_set_address $::main_start
			line 10
			DW_LNS_copy

			DW_LNE_set_address "$::main_start + $::main_len"
			DW_LNE_end_sequence
		    }
		}

		# Define the .debug_macro section.
		macro {
		    cu_macros: unit {
			"debug-line-offset-label" $Llines
			"is-64" $::is_64
		    } {
			# A macro defined outside the main file, as if it was defined
			# on the command line with -D.
			#
			# Clang has this bug where it puts the macros defined on
			# the command-line after the main file portion (see
			# PR 29034).  We're not trying to replicate that here,
			# this is not in the scope of this test.
			define 0 "ONE 1"
			define_strp 0 "THREE 3"
			start_file 0 $::main_file_idx
			    # A macro defined at line 1 of the main file.
			    define 1 "TWO 2"
			end_file
		    }
		}
	    }

	    if { [prepare_for_testing "failed to prepare" ${::testfile}-${test_name}-${32_or_64} \
		      [list $::srcfile $asm_file] {nodebug}] } {
		return
	    }

	    with_complaints 5 {
		gdb_test_multiple "print main" "no complaints" {
		    -wrap -re "During symbol reading: .*" {
			fail $gdb_test_name
		    }
		    -wrap -re "" {
			pass $gdb_test_name
		    }
		}
	    }

	    if ![runto_main] {
		return
	    }

	    gdb_test "print ONE" " = 1"
	    gdb_test "print TWO" " = 2"
	    gdb_test "print THREE" " = 3"
	}
    }
}
