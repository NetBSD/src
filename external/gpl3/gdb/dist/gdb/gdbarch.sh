#!/bin/sh -u

# Architecture commands for GDB, the GNU debugger.
#
# Copyright (C) 1998-2013 Free Software Foundation, Inc.
#
# This file is part of GDB.
#
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

# Make certain that the script is not running in an internationalized
# environment.
LANG=C ; export LANG
LC_ALL=C ; export LC_ALL


compare_new ()
{
    file=$1
    if test ! -r ${file}
    then
	echo "${file} missing? cp new-${file} ${file}" 1>&2
    elif diff -u ${file} new-${file}
    then
	echo "${file} unchanged" 1>&2
    else
	echo "${file} has changed? cp new-${file} ${file}" 1>&2
    fi
}


# Format of the input table
read="class returntype function formal actual staticdefault predefault postdefault invalid_p print garbage_at_eol"

do_read ()
{
    comment=""
    class=""
    # On some SH's, 'read' trims leading and trailing whitespace by
    # default (e.g., bash), while on others (e.g., dash), it doesn't.
    # Set IFS to empty to disable the trimming everywhere.
    while IFS='' read line
    do
	if test "${line}" = ""
	then
	    continue
	elif test "${line}" = "#" -a "${comment}" = ""
	then
	    continue
	elif expr "${line}" : "#" > /dev/null
	then
	    comment="${comment}
${line}"
	else

	    # The semantics of IFS varies between different SH's.  Some
	    # treat ``::' as three fields while some treat it as just too.
	    # Work around this by eliminating ``::'' ....
	    line="`echo "${line}" | sed -e 's/::/: :/g' -e 's/::/: :/g'`"

	    OFS="${IFS}" ; IFS="[:]"
	    eval read ${read} <<EOF
${line}
EOF
	    IFS="${OFS}"

	    if test -n "${garbage_at_eol}"
	    then
		echo "Garbage at end-of-line in ${line}" 1>&2
		kill $$
		exit 1
	    fi

	    # .... and then going back through each field and strip out those
	    # that ended up with just that space character.
	    for r in ${read}
	    do
		if eval test \"\${${r}}\" = \"\ \"
		then
		    eval ${r}=""
		fi
	    done

	    case "${class}" in
		m ) staticdefault="${predefault}" ;;
		M ) staticdefault="0" ;;
		* ) test "${staticdefault}" || staticdefault=0 ;;
	    esac

	    case "${class}" in
	    F | V | M )
		case "${invalid_p}" in
		"" )
		    if test -n "${predefault}"
		    then
			#invalid_p="gdbarch->${function} == ${predefault}"
			predicate="gdbarch->${function} != ${predefault}"
		    elif class_is_variable_p
		    then
			predicate="gdbarch->${function} != 0"
		    elif class_is_function_p
		    then
			predicate="gdbarch->${function} != NULL"
		    fi
		    ;;
		* )
		    echo "Predicate function ${function} with invalid_p." 1>&2
		    kill $$
		    exit 1
		    ;;
		esac
	    esac

	    # PREDEFAULT is a valid fallback definition of MEMBER when
	    # multi-arch is not enabled.  This ensures that the
	    # default value, when multi-arch is the same as the
	    # default value when not multi-arch.  POSTDEFAULT is
	    # always a valid definition of MEMBER as this again
	    # ensures consistency.

	    if [ -n "${postdefault}" ]
	    then
		fallbackdefault="${postdefault}"
	    elif [ -n "${predefault}" ]
	    then
		fallbackdefault="${predefault}"
	    else
		fallbackdefault="0"
	    fi

	    #NOT YET: See gdbarch.log for basic verification of
	    # database

	    break
	fi
    done
    if [ -n "${class}" ]
    then
	true
    else
	false
    fi
}


fallback_default_p ()
{
    [ -n "${postdefault}" -a "x${invalid_p}" != "x0" ] \
	|| [ -n "${predefault}" -a "x${invalid_p}" = "x0" ]
}

class_is_variable_p ()
{
    case "${class}" in
	*v* | *V* ) true ;;
	* ) false ;;
    esac
}

class_is_function_p ()
{
    case "${class}" in
	*f* | *F* | *m* | *M* ) true ;;
	* ) false ;;
    esac
}

class_is_multiarch_p ()
{
    case "${class}" in
	*m* | *M* ) true ;;
	* ) false ;;
    esac
}

class_is_predicate_p ()
{
    case "${class}" in
	*F* | *V* | *M* ) true ;;
	* ) false ;;
    esac
}

class_is_info_p ()
{
    case "${class}" in
	*i* ) true ;;
	* ) false ;;
    esac
}


# dump out/verify the doco
for field in ${read}
do
  case ${field} in

    class ) : ;;

	# # -> line disable
	# f -> function
	#   hiding a function
	# F -> function + predicate
	#   hiding a function + predicate to test function validity
	# v -> variable
	#   hiding a variable
	# V -> variable + predicate
	#   hiding a variable + predicate to test variables validity
	# i -> set from info
	#   hiding something from the ``struct info'' object
	# m -> multi-arch function
	#   hiding a multi-arch function (parameterised with the architecture)
        # M -> multi-arch function + predicate
	#   hiding a multi-arch function + predicate to test function validity

    returntype ) : ;;

	# For functions, the return type; for variables, the data type

    function ) : ;;

	# For functions, the member function name; for variables, the
	# variable name.  Member function names are always prefixed with
	# ``gdbarch_'' for name-space purity.

    formal ) : ;;

	# The formal argument list.  It is assumed that the formal
	# argument list includes the actual name of each list element.
	# A function with no arguments shall have ``void'' as the
	# formal argument list.

    actual ) : ;;

	# The list of actual arguments.  The arguments specified shall
	# match the FORMAL list given above.  Functions with out
	# arguments leave this blank.

    staticdefault ) : ;;

	# To help with the GDB startup a static gdbarch object is
	# created.  STATICDEFAULT is the value to insert into that
	# static gdbarch object.  Since this a static object only
	# simple expressions can be used.

	# If STATICDEFAULT is empty, zero is used.

    predefault ) : ;;

	# An initial value to assign to MEMBER of the freshly
	# malloc()ed gdbarch object.  After initialization, the
	# freshly malloc()ed object is passed to the target
	# architecture code for further updates.

	# If PREDEFAULT is empty, zero is used.

	# A non-empty PREDEFAULT, an empty POSTDEFAULT and a zero
	# INVALID_P are specified, PREDEFAULT will be used as the
	# default for the non- multi-arch target.

	# A zero PREDEFAULT function will force the fallback to call
	# internal_error().

	# Variable declarations can refer to ``gdbarch'' which will
	# contain the current architecture.  Care should be taken.

    postdefault ) : ;;

	# A value to assign to MEMBER of the new gdbarch object should
	# the target architecture code fail to change the PREDEFAULT
	# value.

	# If POSTDEFAULT is empty, no post update is performed.

	# If both INVALID_P and POSTDEFAULT are non-empty then
	# INVALID_P will be used to determine if MEMBER should be
	# changed to POSTDEFAULT.

	# If a non-empty POSTDEFAULT and a zero INVALID_P are
	# specified, POSTDEFAULT will be used as the default for the
	# non- multi-arch target (regardless of the value of
	# PREDEFAULT).

	# You cannot specify both a zero INVALID_P and a POSTDEFAULT.

	# Variable declarations can refer to ``gdbarch'' which
	# will contain the current architecture.  Care should be
	# taken.

    invalid_p ) : ;;

	# A predicate equation that validates MEMBER.  Non-zero is
	# returned if the code creating the new architecture failed to
	# initialize MEMBER or the initialized the member is invalid.
	# If POSTDEFAULT is non-empty then MEMBER will be updated to
	# that value.  If POSTDEFAULT is empty then internal_error()
	# is called.

	# If INVALID_P is empty, a check that MEMBER is no longer
	# equal to PREDEFAULT is used.

	# The expression ``0'' disables the INVALID_P check making
	# PREDEFAULT a legitimate value.

	# See also PREDEFAULT and POSTDEFAULT.

    print ) : ;;

	# An optional expression that convers MEMBER to a value
	# suitable for formatting using %s.

	# If PRINT is empty, core_addr_to_string_nz (for CORE_ADDR)
	# or plongest (anything else) is used.

    garbage_at_eol ) : ;;

	# Catches stray fields.

    *)
	echo "Bad field ${field}"
	exit 1;;
  esac
done


function_list ()
{
  # See below (DOCO) for description of each field
  cat <<EOF
i:const struct bfd_arch_info *:bfd_arch_info:::&bfd_default_arch_struct::::gdbarch_bfd_arch_info (gdbarch)->printable_name
#
i:int:byte_order:::BFD_ENDIAN_BIG
i:int:byte_order_for_code:::BFD_ENDIAN_BIG
#
i:enum gdb_osabi:osabi:::GDB_OSABI_UNKNOWN
#
i:const struct target_desc *:target_desc:::::::host_address_to_string (gdbarch->target_desc)

# The bit byte-order has to do just with numbering of bits in debugging symbols
# and such.  Conceptually, it's quite separate from byte/word byte order.
v:int:bits_big_endian:::1:(gdbarch->byte_order == BFD_ENDIAN_BIG)::0

# Number of bits in a char or unsigned char for the target machine.
# Just like CHAR_BIT in <limits.h> but describes the target machine.
# v:TARGET_CHAR_BIT:int:char_bit::::8 * sizeof (char):8::0:
#
# Number of bits in a short or unsigned short for the target machine.
v:int:short_bit:::8 * sizeof (short):2*TARGET_CHAR_BIT::0
# Number of bits in an int or unsigned int for the target machine.
v:int:int_bit:::8 * sizeof (int):4*TARGET_CHAR_BIT::0
# Number of bits in a long or unsigned long for the target machine.
v:int:long_bit:::8 * sizeof (long):4*TARGET_CHAR_BIT::0
# Number of bits in a long long or unsigned long long for the target
# machine.
v:int:long_long_bit:::8 * sizeof (LONGEST):2*gdbarch->long_bit::0
# Alignment of a long long or unsigned long long for the target
# machine.
v:int:long_long_align_bit:::8 * sizeof (LONGEST):2*gdbarch->long_bit::0

# The ABI default bit-size and format for "half", "float", "double", and
# "long double".  These bit/format pairs should eventually be combined
# into a single object.  For the moment, just initialize them as a pair.
# Each format describes both the big and little endian layouts (if
# useful).

v:int:half_bit:::16:2*TARGET_CHAR_BIT::0
v:const struct floatformat **:half_format:::::floatformats_ieee_half::pformat (gdbarch->half_format)
v:int:float_bit:::8 * sizeof (float):4*TARGET_CHAR_BIT::0
v:const struct floatformat **:float_format:::::floatformats_ieee_single::pformat (gdbarch->float_format)
v:int:double_bit:::8 * sizeof (double):8*TARGET_CHAR_BIT::0
v:const struct floatformat **:double_format:::::floatformats_ieee_double::pformat (gdbarch->double_format)
v:int:long_double_bit:::8 * sizeof (long double):8*TARGET_CHAR_BIT::0
v:const struct floatformat **:long_double_format:::::floatformats_ieee_double::pformat (gdbarch->long_double_format)

# For most targets, a pointer on the target and its representation as an
# address in GDB have the same size and "look the same".  For such a
# target, you need only set gdbarch_ptr_bit and gdbarch_addr_bit
# / addr_bit will be set from it.
#
# If gdbarch_ptr_bit and gdbarch_addr_bit are different, you'll probably
# also need to set gdbarch_dwarf2_addr_size, gdbarch_pointer_to_address and
# gdbarch_address_to_pointer as well.
#
# ptr_bit is the size of a pointer on the target
v:int:ptr_bit:::8 * sizeof (void*):gdbarch->int_bit::0
# addr_bit is the size of a target address as represented in gdb
v:int:addr_bit:::8 * sizeof (void*):0:gdbarch_ptr_bit (gdbarch):
#
# dwarf2_addr_size is the target address size as used in the Dwarf debug
# info.  For .debug_frame FDEs, this is supposed to be the target address
# size from the associated CU header, and which is equivalent to the
# DWARF2_ADDR_SIZE as defined by the target specific GCC back-end.
# Unfortunately there is no good way to determine this value.  Therefore
# dwarf2_addr_size simply defaults to the target pointer size.
#
# dwarf2_addr_size is not used for .eh_frame FDEs, which are generally
# defined using the target's pointer size so far.
#
# Note that dwarf2_addr_size only needs to be redefined by a target if the
# GCC back-end defines a DWARF2_ADDR_SIZE other than the target pointer size,
# and if Dwarf versions < 4 need to be supported.
v:int:dwarf2_addr_size:::sizeof (void*):0:gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT:
#
# One if \`char' acts like \`signed char', zero if \`unsigned char'.
v:int:char_signed:::1:-1:1
#
F:CORE_ADDR:read_pc:struct regcache *regcache:regcache
F:void:write_pc:struct regcache *regcache, CORE_ADDR val:regcache, val
# Function for getting target's idea of a frame pointer.  FIXME: GDB's
# whole scheme for dealing with "frames" and "frame pointers" needs a
# serious shakedown.
m:void:virtual_frame_pointer:CORE_ADDR pc, int *frame_regnum, LONGEST *frame_offset:pc, frame_regnum, frame_offset:0:legacy_virtual_frame_pointer::0
#
M:enum register_status:pseudo_register_read:struct regcache *regcache, int cookednum, gdb_byte *buf:regcache, cookednum, buf
# Read a register into a new struct value.  If the register is wholly
# or partly unavailable, this should call mark_value_bytes_unavailable
# as appropriate.  If this is defined, then pseudo_register_read will
# never be called.
M:struct value *:pseudo_register_read_value:struct regcache *regcache, int cookednum:regcache, cookednum
M:void:pseudo_register_write:struct regcache *regcache, int cookednum, const gdb_byte *buf:regcache, cookednum, buf
#
v:int:num_regs:::0:-1
# This macro gives the number of pseudo-registers that live in the
# register namespace but do not get fetched or stored on the target.
# These pseudo-registers may be aliases for other registers,
# combinations of other registers, or they may be computed by GDB.
v:int:num_pseudo_regs:::0:0::0

# Assemble agent expression bytecode to collect pseudo-register REG.
# Return -1 if something goes wrong, 0 otherwise.
M:int:ax_pseudo_register_collect:struct agent_expr *ax, int reg:ax, reg

# Assemble agent expression bytecode to push the value of pseudo-register
# REG on the interpreter stack.
# Return -1 if something goes wrong, 0 otherwise.
M:int:ax_pseudo_register_push_stack:struct agent_expr *ax, int reg:ax, reg

# GDB's standard (or well known) register numbers.  These can map onto
# a real register or a pseudo (computed) register or not be defined at
# all (-1).
# gdbarch_sp_regnum will hopefully be replaced by UNWIND_SP.
v:int:sp_regnum:::-1:-1::0
v:int:pc_regnum:::-1:-1::0
v:int:ps_regnum:::-1:-1::0
v:int:fp0_regnum:::0:-1::0
# Convert stab register number (from \`r\' declaration) to a gdb REGNUM.
m:int:stab_reg_to_regnum:int stab_regnr:stab_regnr::no_op_reg_to_regnum::0
# Provide a default mapping from a ecoff register number to a gdb REGNUM.
m:int:ecoff_reg_to_regnum:int ecoff_regnr:ecoff_regnr::no_op_reg_to_regnum::0
# Convert from an sdb register number to an internal gdb register number.
m:int:sdb_reg_to_regnum:int sdb_regnr:sdb_regnr::no_op_reg_to_regnum::0
# Provide a default mapping from a DWARF2 register number to a gdb REGNUM.
m:int:dwarf2_reg_to_regnum:int dwarf2_regnr:dwarf2_regnr::no_op_reg_to_regnum::0
m:const char *:register_name:int regnr:regnr::0

# Return the type of a register specified by the architecture.  Only
# the register cache should call this function directly; others should
# use "register_type".
M:struct type *:register_type:int reg_nr:reg_nr

# See gdbint.texinfo, and PUSH_DUMMY_CALL.
M:struct frame_id:dummy_id:struct frame_info *this_frame:this_frame
# Implement DUMMY_ID and PUSH_DUMMY_CALL, then delete
# deprecated_fp_regnum.
v:int:deprecated_fp_regnum:::-1:-1::0

# See gdbint.texinfo.  See infcall.c.
M:CORE_ADDR:push_dummy_call:struct value *function, struct regcache *regcache, CORE_ADDR bp_addr, int nargs, struct value **args, CORE_ADDR sp, int struct_return, CORE_ADDR struct_addr:function, regcache, bp_addr, nargs, args, sp, struct_return, struct_addr
v:int:call_dummy_location::::AT_ENTRY_POINT::0
M:CORE_ADDR:push_dummy_code:CORE_ADDR sp, CORE_ADDR funaddr, struct value **args, int nargs, struct type *value_type, CORE_ADDR *real_pc, CORE_ADDR *bp_addr, struct regcache *regcache:sp, funaddr, args, nargs, value_type, real_pc, bp_addr, regcache

m:void:print_registers_info:struct ui_file *file, struct frame_info *frame, int regnum, int all:file, frame, regnum, all::default_print_registers_info::0
M:void:print_float_info:struct ui_file *file, struct frame_info *frame, const char *args:file, frame, args
M:void:print_vector_info:struct ui_file *file, struct frame_info *frame, const char *args:file, frame, args
# MAP a GDB RAW register number onto a simulator register number.  See
# also include/...-sim.h.
m:int:register_sim_regno:int reg_nr:reg_nr::legacy_register_sim_regno::0
m:int:cannot_fetch_register:int regnum:regnum::cannot_register_not::0
m:int:cannot_store_register:int regnum:regnum::cannot_register_not::0
# setjmp/longjmp support.
F:int:get_longjmp_target:struct frame_info *frame, CORE_ADDR *pc:frame, pc
#
v:int:believe_pcc_promotion:::::::
#
m:int:convert_register_p:int regnum, struct type *type:regnum, type:0:generic_convert_register_p::0
f:int:register_to_value:struct frame_info *frame, int regnum, struct type *type, gdb_byte *buf, int *optimizedp, int *unavailablep:frame, regnum, type, buf, optimizedp, unavailablep:0
f:void:value_to_register:struct frame_info *frame, int regnum, struct type *type, const gdb_byte *buf:frame, regnum, type, buf:0
# Construct a value representing the contents of register REGNUM in
# frame FRAME, interpreted as type TYPE.  The routine needs to
# allocate and return a struct value with all value attributes
# (but not the value contents) filled in.
f:struct value *:value_from_register:struct type *type, int regnum, struct frame_info *frame:type, regnum, frame::default_value_from_register::0
#
m:CORE_ADDR:pointer_to_address:struct type *type, const gdb_byte *buf:type, buf::unsigned_pointer_to_address::0
m:void:address_to_pointer:struct type *type, gdb_byte *buf, CORE_ADDR addr:type, buf, addr::unsigned_address_to_pointer::0
M:CORE_ADDR:integer_to_address:struct type *type, const gdb_byte *buf:type, buf

# Return the return-value convention that will be used by FUNCTION
# to return a value of type VALTYPE.  FUNCTION may be NULL in which
# case the return convention is computed based only on VALTYPE.
#
# If READBUF is not NULL, extract the return value and save it in this buffer.
#
# If WRITEBUF is not NULL, it contains a return value which will be
# stored into the appropriate register.  This can be used when we want
# to force the value returned by a function (see the "return" command
# for instance).
M:enum return_value_convention:return_value:struct value *function, struct type *valtype, struct regcache *regcache, gdb_byte *readbuf, const gdb_byte *writebuf:function, valtype, regcache, readbuf, writebuf

# Return true if the return value of function is stored in the first hidden
# parameter.  In theory, this feature should be language-dependent, specified
# by language and its ABI, such as C++.  Unfortunately, compiler may
# implement it to a target-dependent feature.  So that we need such hook here
# to be aware of this in GDB.
m:int:return_in_first_hidden_param_p:struct type *type:type::default_return_in_first_hidden_param_p::0

m:CORE_ADDR:skip_prologue:CORE_ADDR ip:ip:0:0
M:CORE_ADDR:skip_main_prologue:CORE_ADDR ip:ip
f:int:inner_than:CORE_ADDR lhs, CORE_ADDR rhs:lhs, rhs:0:0
m:const gdb_byte *:breakpoint_from_pc:CORE_ADDR *pcptr, int *lenptr:pcptr, lenptr::0:
# Return the adjusted address and kind to use for Z0/Z1 packets.
# KIND is usually the memory length of the breakpoint, but may have a
# different target-specific meaning.
m:void:remote_breakpoint_from_pc:CORE_ADDR *pcptr, int *kindptr:pcptr, kindptr:0:default_remote_breakpoint_from_pc::0
M:CORE_ADDR:adjust_breakpoint_address:CORE_ADDR bpaddr:bpaddr
m:int:memory_insert_breakpoint:struct bp_target_info *bp_tgt:bp_tgt:0:default_memory_insert_breakpoint::0
m:int:memory_remove_breakpoint:struct bp_target_info *bp_tgt:bp_tgt:0:default_memory_remove_breakpoint::0
v:CORE_ADDR:decr_pc_after_break:::0:::0

# A function can be addressed by either it's "pointer" (possibly a
# descriptor address) or "entry point" (first executable instruction).
# The method "convert_from_func_ptr_addr" converting the former to the
# latter.  gdbarch_deprecated_function_start_offset is being used to implement
# a simplified subset of that functionality - the function's address
# corresponds to the "function pointer" and the function's start
# corresponds to the "function entry point" - and hence is redundant.

v:CORE_ADDR:deprecated_function_start_offset:::0:::0

# Return the remote protocol register number associated with this
# register.  Normally the identity mapping.
m:int:remote_register_number:int regno:regno::default_remote_register_number::0

# Fetch the target specific address used to represent a load module.
F:CORE_ADDR:fetch_tls_load_module_address:struct objfile *objfile:objfile
#
v:CORE_ADDR:frame_args_skip:::0:::0
M:CORE_ADDR:unwind_pc:struct frame_info *next_frame:next_frame
M:CORE_ADDR:unwind_sp:struct frame_info *next_frame:next_frame
# DEPRECATED_FRAME_LOCALS_ADDRESS as been replaced by the per-frame
# frame-base.  Enable frame-base before frame-unwind.
F:int:frame_num_args:struct frame_info *frame:frame
#
M:CORE_ADDR:frame_align:CORE_ADDR address:address
m:int:stabs_argument_has_addr:struct type *type:type::default_stabs_argument_has_addr::0
v:int:frame_red_zone_size
#
m:CORE_ADDR:convert_from_func_ptr_addr:CORE_ADDR addr, struct target_ops *targ:addr, targ::convert_from_func_ptr_addr_identity::0
# On some machines there are bits in addresses which are not really
# part of the address, but are used by the kernel, the hardware, etc.
# for special purposes.  gdbarch_addr_bits_remove takes out any such bits so
# we get a "real" address such as one would find in a symbol table.
# This is used only for addresses of instructions, and even then I'm
# not sure it's used in all contexts.  It exists to deal with there
# being a few stray bits in the PC which would mislead us, not as some
# sort of generic thing to handle alignment or segmentation (it's
# possible it should be in TARGET_READ_PC instead).
m:CORE_ADDR:addr_bits_remove:CORE_ADDR addr:addr::core_addr_identity::0

# FIXME/cagney/2001-01-18: This should be split in two.  A target method that
# indicates if the target needs software single step.  An ISA method to
# implement it.
#
# FIXME/cagney/2001-01-18: This should be replaced with something that inserts
# breakpoints using the breakpoint system instead of blatting memory directly
# (as with rs6000).
#
# FIXME/cagney/2001-01-18: The logic is backwards.  It should be asking if the
# target can single step.  If not, then implement single step using breakpoints.
#
# A return value of 1 means that the software_single_step breakpoints
# were inserted; 0 means they were not.
F:int:software_single_step:struct frame_info *frame:frame

# Return non-zero if the processor is executing a delay slot and a
# further single-step is needed before the instruction finishes.
M:int:single_step_through_delay:struct frame_info *frame:frame
# FIXME: cagney/2003-08-28: Need to find a better way of selecting the
# disassembler.  Perhaps objdump can handle it?
f:int:print_insn:bfd_vma vma, struct disassemble_info *info:vma, info::0:
f:CORE_ADDR:skip_trampoline_code:struct frame_info *frame, CORE_ADDR pc:frame, pc::generic_skip_trampoline_code::0


# If in_solib_dynsym_resolve_code() returns true, and SKIP_SOLIB_RESOLVER
# evaluates non-zero, this is the address where the debugger will place
# a step-resume breakpoint to get us past the dynamic linker.
m:CORE_ADDR:skip_solib_resolver:CORE_ADDR pc:pc::generic_skip_solib_resolver::0
# Some systems also have trampoline code for returning from shared libs.
m:int:in_solib_return_trampoline:CORE_ADDR pc, const char *name:pc, name::generic_in_solib_return_trampoline::0

# A target might have problems with watchpoints as soon as the stack
# frame of the current function has been destroyed.  This mostly happens
# as the first action in a funtion's epilogue.  in_function_epilogue_p()
# is defined to return a non-zero value if either the given addr is one
# instruction after the stack destroying instruction up to the trailing
# return instruction or if we can figure out that the stack frame has
# already been invalidated regardless of the value of addr.  Targets
# which don't suffer from that problem could just let this functionality
# untouched.
m:int:in_function_epilogue_p:CORE_ADDR addr:addr:0:generic_in_function_epilogue_p::0
f:void:elf_make_msymbol_special:asymbol *sym, struct minimal_symbol *msym:sym, msym::default_elf_make_msymbol_special::0
f:void:coff_make_msymbol_special:int val, struct minimal_symbol *msym:val, msym::default_coff_make_msymbol_special::0
v:int:cannot_step_breakpoint:::0:0::0
v:int:have_nonsteppable_watchpoint:::0:0::0
F:int:address_class_type_flags:int byte_size, int dwarf2_addr_class:byte_size, dwarf2_addr_class
M:const char *:address_class_type_flags_to_name:int type_flags:type_flags
M:int:address_class_name_to_type_flags:const char *name, int *type_flags_ptr:name, type_flags_ptr
# Is a register in a group
m:int:register_reggroup_p:int regnum, struct reggroup *reggroup:regnum, reggroup::default_register_reggroup_p::0
# Fetch the pointer to the ith function argument.
F:CORE_ADDR:fetch_pointer_argument:struct frame_info *frame, int argi, struct type *type:frame, argi, type

# Return the appropriate register set for a core file section with
# name SECT_NAME and size SECT_SIZE.
M:const struct regset *:regset_from_core_section:const char *sect_name, size_t sect_size:sect_name, sect_size

# Supported register notes in a core file.
v:struct core_regset_section *:core_regset_sections:const char *name, int len::::::host_address_to_string (gdbarch->core_regset_sections)

# Create core file notes
M:char *:make_corefile_notes:bfd *obfd, int *note_size:obfd, note_size

# The elfcore writer hook to use to write Linux prpsinfo notes to core
# files.  Most Linux architectures use the same prpsinfo32 or
# prpsinfo64 layouts, and so won't need to provide this hook, as we
# call the Linux generic routines in bfd to write prpsinfo notes by
# default.
F:char *:elfcore_write_linux_prpsinfo:bfd *obfd, char *note_data, int *note_size, const struct elf_internal_linux_prpsinfo *info:obfd, note_data, note_size, info

# Find core file memory regions
M:int:find_memory_regions:find_memory_region_ftype func, void *data:func, data

# Read offset OFFSET of TARGET_OBJECT_LIBRARIES formatted shared libraries list from
# core file into buffer READBUF with length LEN.
M:LONGEST:core_xfer_shared_libraries:gdb_byte *readbuf, ULONGEST offset, LONGEST len:readbuf, offset, len

# How the core target converts a PTID from a core file to a string.
M:char *:core_pid_to_str:ptid_t ptid:ptid

# BFD target to use when generating a core file.
V:const char *:gcore_bfd_target:::0:0:::pstring (gdbarch->gcore_bfd_target)

# If the elements of C++ vtables are in-place function descriptors rather
# than normal function pointers (which may point to code or a descriptor),
# set this to one.
v:int:vtable_function_descriptors:::0:0::0

# Set if the least significant bit of the delta is used instead of the least
# significant bit of the pfn for pointers to virtual member functions.
v:int:vbit_in_delta:::0:0::0

# Advance PC to next instruction in order to skip a permanent breakpoint.
F:void:skip_permanent_breakpoint:struct regcache *regcache:regcache

# The maximum length of an instruction on this architecture in bytes.
V:ULONGEST:max_insn_length:::0:0

# Copy the instruction at FROM to TO, and make any adjustments
# necessary to single-step it at that address.
#
# REGS holds the state the thread's registers will have before
# executing the copied instruction; the PC in REGS will refer to FROM,
# not the copy at TO.  The caller should update it to point at TO later.
#
# Return a pointer to data of the architecture's choice to be passed
# to gdbarch_displaced_step_fixup.  Or, return NULL to indicate that
# the instruction's effects have been completely simulated, with the
# resulting state written back to REGS.
#
# For a general explanation of displaced stepping and how GDB uses it,
# see the comments in infrun.c.
#
# The TO area is only guaranteed to have space for
# gdbarch_max_insn_length (arch) bytes, so this function must not
# write more bytes than that to that area.
#
# If you do not provide this function, GDB assumes that the
# architecture does not support displaced stepping.
#
# If your architecture doesn't need to adjust instructions before
# single-stepping them, consider using simple_displaced_step_copy_insn
# here.
M:struct displaced_step_closure *:displaced_step_copy_insn:CORE_ADDR from, CORE_ADDR to, struct regcache *regs:from, to, regs

# Return true if GDB should use hardware single-stepping to execute
# the displaced instruction identified by CLOSURE.  If false,
# GDB will simply restart execution at the displaced instruction
# location, and it is up to the target to ensure GDB will receive
# control again (e.g. by placing a software breakpoint instruction
# into the displaced instruction buffer).
#
# The default implementation returns false on all targets that
# provide a gdbarch_software_single_step routine, and true otherwise.
m:int:displaced_step_hw_singlestep:struct displaced_step_closure *closure:closure::default_displaced_step_hw_singlestep::0

# Fix up the state resulting from successfully single-stepping a
# displaced instruction, to give the result we would have gotten from
# stepping the instruction in its original location.
#
# REGS is the register state resulting from single-stepping the
# displaced instruction.
#
# CLOSURE is the result from the matching call to
# gdbarch_displaced_step_copy_insn.
#
# If you provide gdbarch_displaced_step_copy_insn.but not this
# function, then GDB assumes that no fixup is needed after
# single-stepping the instruction.
#
# For a general explanation of displaced stepping and how GDB uses it,
# see the comments in infrun.c.
M:void:displaced_step_fixup:struct displaced_step_closure *closure, CORE_ADDR from, CORE_ADDR to, struct regcache *regs:closure, from, to, regs::NULL

# Free a closure returned by gdbarch_displaced_step_copy_insn.
#
# If you provide gdbarch_displaced_step_copy_insn, you must provide
# this function as well.
#
# If your architecture uses closures that don't need to be freed, then
# you can use simple_displaced_step_free_closure here.
#
# For a general explanation of displaced stepping and how GDB uses it,
# see the comments in infrun.c.
m:void:displaced_step_free_closure:struct displaced_step_closure *closure:closure::NULL::(! gdbarch->displaced_step_free_closure) != (! gdbarch->displaced_step_copy_insn)

# Return the address of an appropriate place to put displaced
# instructions while we step over them.  There need only be one such
# place, since we're only stepping one thread over a breakpoint at a
# time.
#
# For a general explanation of displaced stepping and how GDB uses it,
# see the comments in infrun.c.
m:CORE_ADDR:displaced_step_location:void:::NULL::(! gdbarch->displaced_step_location) != (! gdbarch->displaced_step_copy_insn)

# Relocate an instruction to execute at a different address.  OLDLOC
# is the address in the inferior memory where the instruction to
# relocate is currently at.  On input, TO points to the destination
# where we want the instruction to be copied (and possibly adjusted)
# to.  On output, it points to one past the end of the resulting
# instruction(s).  The effect of executing the instruction at TO shall
# be the same as if executing it at FROM.  For example, call
# instructions that implicitly push the return address on the stack
# should be adjusted to return to the instruction after OLDLOC;
# relative branches, and other PC-relative instructions need the
# offset adjusted; etc.
M:void:relocate_instruction:CORE_ADDR *to, CORE_ADDR from:to, from::NULL

# Refresh overlay mapped state for section OSECT.
F:void:overlay_update:struct obj_section *osect:osect

M:const struct target_desc *:core_read_description:struct target_ops *target, bfd *abfd:target, abfd

# Handle special encoding of static variables in stabs debug info.
F:const char *:static_transform_name:const char *name:name
# Set if the address in N_SO or N_FUN stabs may be zero.
v:int:sofun_address_maybe_missing:::0:0::0

# Parse the instruction at ADDR storing in the record execution log
# the registers REGCACHE and memory ranges that will be affected when
# the instruction executes, along with their current values.
# Return -1 if something goes wrong, 0 otherwise.
M:int:process_record:struct regcache *regcache, CORE_ADDR addr:regcache, addr

# Save process state after a signal.
# Return -1 if something goes wrong, 0 otherwise.
M:int:process_record_signal:struct regcache *regcache, enum gdb_signal signal:regcache, signal

# Signal translation: translate inferior's signal (target's) number
# into GDB's representation.  The implementation of this method must
# be host independent.  IOW, don't rely on symbols of the NAT_FILE
# header (the nm-*.h files), the host <signal.h> header, or similar
# headers.  This is mainly used when cross-debugging core files ---
# "Live" targets hide the translation behind the target interface
# (target_wait, target_resume, etc.).
M:enum gdb_signal:gdb_signal_from_target:int signo:signo

# Extra signal info inspection.
#
# Return a type suitable to inspect extra signal information.
M:struct type *:get_siginfo_type:void:

# Record architecture-specific information from the symbol table.
M:void:record_special_symbol:struct objfile *objfile, asymbol *sym:objfile, sym

# Function for the 'catch syscall' feature.

# Get architecture-specific system calls information from registers.
M:LONGEST:get_syscall_number:ptid_t ptid:ptid

# SystemTap related fields and functions.

# Prefix used to mark an integer constant on the architecture's assembly
# For example, on x86 integer constants are written as:
#
#  \$10 ;; integer constant 10
#
# in this case, this prefix would be the character \`\$\'.
v:const char *:stap_integer_prefix:::0:0::0:pstring (gdbarch->stap_integer_prefix)

# Suffix used to mark an integer constant on the architecture's assembly.
v:const char *:stap_integer_suffix:::0:0::0:pstring (gdbarch->stap_integer_suffix)

# Prefix used to mark a register name on the architecture's assembly.
# For example, on x86 the register name is written as:
#
#  \%eax ;; register eax
#
# in this case, this prefix would be the character \`\%\'.
v:const char *:stap_register_prefix:::0:0::0:pstring (gdbarch->stap_register_prefix)

# Suffix used to mark a register name on the architecture's assembly
v:const char *:stap_register_suffix:::0:0::0:pstring (gdbarch->stap_register_suffix)

# Prefix used to mark a register indirection on the architecture's assembly.
# For example, on x86 the register indirection is written as:
#
#  \(\%eax\) ;; indirecting eax
#
# in this case, this prefix would be the charater \`\(\'.
#
# Please note that we use the indirection prefix also for register
# displacement, e.g., \`4\(\%eax\)\' on x86.
v:const char *:stap_register_indirection_prefix:::0:0::0:pstring (gdbarch->stap_register_indirection_prefix)

# Suffix used to mark a register indirection on the architecture's assembly.
# For example, on x86 the register indirection is written as:
#
#  \(\%eax\) ;; indirecting eax
#
# in this case, this prefix would be the charater \`\)\'.
#
# Please note that we use the indirection suffix also for register
# displacement, e.g., \`4\(\%eax\)\' on x86.
v:const char *:stap_register_indirection_suffix:::0:0::0:pstring (gdbarch->stap_register_indirection_suffix)

# Prefix used to name a register using GDB's nomenclature.
#
# For example, on PPC a register is represented by a number in the assembly
# language (e.g., \`10\' is the 10th general-purpose register).  However,
# inside GDB this same register has an \`r\' appended to its name, so the 10th
# register would be represented as \`r10\' internally.
v:const char *:stap_gdb_register_prefix:::0:0::0:pstring (gdbarch->stap_gdb_register_prefix)

# Suffix used to name a register using GDB's nomenclature.
v:const char *:stap_gdb_register_suffix:::0:0::0:pstring (gdbarch->stap_gdb_register_suffix)

# Check if S is a single operand.
#
# Single operands can be:
#  \- Literal integers, e.g. \`\$10\' on x86
#  \- Register access, e.g. \`\%eax\' on x86
#  \- Register indirection, e.g. \`\(\%eax\)\' on x86
#  \- Register displacement, e.g. \`4\(\%eax\)\' on x86
#
# This function should check for these patterns on the string
# and return 1 if some were found, or zero otherwise.  Please try to match
# as much info as you can from the string, i.e., if you have to match
# something like \`\(\%\', do not match just the \`\(\'.
M:int:stap_is_single_operand:const char *s:s

# Function used to handle a "special case" in the parser.
#
# A "special case" is considered to be an unknown token, i.e., a token
# that the parser does not know how to parse.  A good example of special
# case would be ARM's register displacement syntax:
#
#  [R0, #4]  ;; displacing R0 by 4
#
# Since the parser assumes that a register displacement is of the form:
#
#  <number> <indirection_prefix> <register_name> <indirection_suffix>
#
# it means that it will not be able to recognize and parse this odd syntax.
# Therefore, we should add a special case function that will handle this token.
#
# This function should generate the proper expression form of the expression
# using GDB\'s internal expression mechanism (e.g., \`write_exp_elt_opcode\'
# and so on).  It should also return 1 if the parsing was successful, or zero
# if the token was not recognized as a special token (in this case, returning
# zero means that the special parser is deferring the parsing to the generic
# parser), and should advance the buffer pointer (p->arg).
M:int:stap_parse_special_token:struct stap_parse_info *p:p


# True if the list of shared libraries is one and only for all
# processes, as opposed to a list of shared libraries per inferior.
# This usually means that all processes, although may or may not share
# an address space, will see the same set of symbols at the same
# addresses.
v:int:has_global_solist:::0:0::0

# On some targets, even though each inferior has its own private
# address space, the debug interface takes care of making breakpoints
# visible to all address spaces automatically.  For such cases,
# this property should be set to true.
v:int:has_global_breakpoints:::0:0::0

# True if inferiors share an address space (e.g., uClinux).
m:int:has_shared_address_space:void:::default_has_shared_address_space::0

# True if a fast tracepoint can be set at an address.
m:int:fast_tracepoint_valid_at:CORE_ADDR addr, int *isize, char **msg:addr, isize, msg::default_fast_tracepoint_valid_at::0

# Return the "auto" target charset.
f:const char *:auto_charset:void::default_auto_charset:default_auto_charset::0
# Return the "auto" target wide charset.
f:const char *:auto_wide_charset:void::default_auto_wide_charset:default_auto_wide_charset::0

# If non-empty, this is a file extension that will be opened in place
# of the file extension reported by the shared library list.
#
# This is most useful for toolchains that use a post-linker tool,
# where the names of the files run on the target differ in extension
# compared to the names of the files GDB should load for debug info.
v:const char *:solib_symbols_extension:::::::pstring (gdbarch->solib_symbols_extension)

# If true, the target OS has DOS-based file system semantics.  That
# is, absolute paths include a drive name, and the backslash is
# considered a directory separator.
v:int:has_dos_based_file_system:::0:0::0

# Generate bytecodes to collect the return address in a frame.
# Since the bytecodes run on the target, possibly with GDB not even
# connected, the full unwinding machinery is not available, and
# typically this function will issue bytecodes for one or more likely
# places that the return address may be found.
m:void:gen_return_address:struct agent_expr *ax, struct axs_value *value, CORE_ADDR scope:ax, value, scope::default_gen_return_address::0

# Implement the "info proc" command.
M:void:info_proc:char *args, enum info_proc_what what:args, what

# Implement the "info proc" command for core files.  Noe that there
# are two "info_proc"-like methods on gdbarch -- one for core files,
# one for live targets.
M:void:core_info_proc:char *args, enum info_proc_what what:args, what

# Iterate over all objfiles in the order that makes the most sense
# for the architecture to make global symbol searches.
#
# CB is a callback function where OBJFILE is the objfile to be searched,
# and CB_DATA a pointer to user-defined data (the same data that is passed
# when calling this gdbarch method).  The iteration stops if this function
# returns nonzero.
#
# CB_DATA is a pointer to some user-defined data to be passed to
# the callback.
#
# If not NULL, CURRENT_OBJFILE corresponds to the objfile being
# inspected when the symbol search was requested.
m:void:iterate_over_objfiles_in_search_order:iterate_over_objfiles_in_search_order_cb_ftype *cb, void *cb_data, struct objfile *current_objfile:cb, cb_data, current_objfile:0:default_iterate_over_objfiles_in_search_order::0

# Ravenscar arch-dependent ops.
v:struct ravenscar_arch_ops *:ravenscar_ops:::NULL:NULL::0:host_address_to_string (gdbarch->ravenscar_ops)
EOF
}

#
# The .log file
#
exec > new-gdbarch.log
function_list | while do_read
do
    cat <<EOF
${class} ${returntype} ${function} ($formal)
EOF
    for r in ${read}
    do
	eval echo \"\ \ \ \ ${r}=\${${r}}\"
    done
    if class_is_predicate_p && fallback_default_p
    then
	echo "Error: predicate function ${function} can not have a non- multi-arch default" 1>&2
	kill $$
	exit 1
    fi
    if [ "x${invalid_p}" = "x0" -a -n "${postdefault}" ]
    then
	echo "Error: postdefault is useless when invalid_p=0" 1>&2
	kill $$
	exit 1
    fi
    if class_is_multiarch_p
    then
	if class_is_predicate_p ; then :
	elif test "x${predefault}" = "x"
	then
	    echo "Error: pure multi-arch function ${function} must have a predefault" 1>&2
	    kill $$
	    exit 1
	fi
    fi
    echo ""
done

exec 1>&2
compare_new gdbarch.log


copyright ()
{
cat <<EOF
/* *INDENT-OFF* */ /* THIS FILE IS GENERATED -*- buffer-read-only: t -*- */
/* vi:set ro: */

/* Dynamic architecture support for GDB, the GNU debugger.

   Copyright (C) 1998-2013 Free Software Foundation, Inc.

   This file is part of GDB.

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

/* This file was created with the aid of \`\`gdbarch.sh''.

   The Bourne shell script \`\`gdbarch.sh'' creates the files
   \`\`new-gdbarch.c'' and \`\`new-gdbarch.h and then compares them
   against the existing \`\`gdbarch.[hc]''.  Any differences found
   being reported.

   If editing this file, please also run gdbarch.sh and merge any
   changes into that script. Conversely, when making sweeping changes
   to this file, modifying gdbarch.sh and using its output may prove
   easier.  */

EOF
}

#
# The .h file
#

exec > new-gdbarch.h
copyright
cat <<EOF
#ifndef GDBARCH_H
#define GDBARCH_H

struct floatformat;
struct ui_file;
struct frame_info;
struct value;
struct objfile;
struct obj_section;
struct minimal_symbol;
struct regcache;
struct reggroup;
struct regset;
struct disassemble_info;
struct target_ops;
struct obstack;
struct bp_target_info;
struct target_desc;
struct displaced_step_closure;
struct core_regset_section;
struct syscall;
struct agent_expr;
struct axs_value;
struct stap_parse_info;
struct ravenscar_arch_ops;
struct elf_internal_linux_prpsinfo;

/* The architecture associated with the inferior through the
   connection to the target.

   The architecture vector provides some information that is really a
   property of the inferior, accessed through a particular target:
   ptrace operations; the layout of certain RSP packets; the solib_ops
   vector; etc.  To differentiate architecture accesses to
   per-inferior/target properties from
   per-thread/per-frame/per-objfile properties, accesses to
   per-inferior/target properties should be made through this
   gdbarch.  */

/* This is a convenience wrapper for 'current_inferior ()->gdbarch'.  */
extern struct gdbarch *target_gdbarch (void);

/* The initial, default architecture.  It uses host values (for want of a better
   choice).  */
extern struct gdbarch startup_gdbarch;


/* Callback type for the 'iterate_over_objfiles_in_search_order'
   gdbarch  method.  */

typedef int (iterate_over_objfiles_in_search_order_cb_ftype)
  (struct objfile *objfile, void *cb_data);
EOF

# function typedef's
printf "\n"
printf "\n"
printf "/* The following are pre-initialized by GDBARCH.  */\n"
function_list | while do_read
do
    if class_is_info_p
    then
	printf "\n"
	printf "extern ${returntype} gdbarch_${function} (struct gdbarch *gdbarch);\n"
	printf "/* set_gdbarch_${function}() - not applicable - pre-initialized.  */\n"
    fi
done

# function typedef's
printf "\n"
printf "\n"
printf "/* The following are initialized by the target dependent code.  */\n"
function_list | while do_read
do
    if [ -n "${comment}" ]
    then
	echo "${comment}" | sed \
	    -e '2 s,#,/*,' \
	    -e '3,$ s,#,  ,' \
	    -e '$ s,$, */,'
    fi

    if class_is_predicate_p
    then
	printf "\n"
	printf "extern int gdbarch_${function}_p (struct gdbarch *gdbarch);\n"
    fi
    if class_is_variable_p
    then
	printf "\n"
	printf "extern ${returntype} gdbarch_${function} (struct gdbarch *gdbarch);\n"
	printf "extern void set_gdbarch_${function} (struct gdbarch *gdbarch, ${returntype} ${function});\n"
    fi
    if class_is_function_p
    then
	printf "\n"
	if [ "x${formal}" = "xvoid" ] && class_is_multiarch_p
	then
	    printf "typedef ${returntype} (gdbarch_${function}_ftype) (struct gdbarch *gdbarch);\n"
	elif class_is_multiarch_p
	then
	    printf "typedef ${returntype} (gdbarch_${function}_ftype) (struct gdbarch *gdbarch, ${formal});\n"
	else
	    printf "typedef ${returntype} (gdbarch_${function}_ftype) (${formal});\n"
	fi
	if [ "x${formal}" = "xvoid" ]
	then
	  printf "extern ${returntype} gdbarch_${function} (struct gdbarch *gdbarch);\n"
	else
	  printf "extern ${returntype} gdbarch_${function} (struct gdbarch *gdbarch, ${formal});\n"
	fi
	printf "extern void set_gdbarch_${function} (struct gdbarch *gdbarch, gdbarch_${function}_ftype *${function});\n"
    fi
done

# close it off
cat <<EOF

/* Definition for an unknown syscall, used basically in error-cases.  */
#define UNKNOWN_SYSCALL (-1)

extern struct gdbarch_tdep *gdbarch_tdep (struct gdbarch *gdbarch);


/* Mechanism for co-ordinating the selection of a specific
   architecture.

   GDB targets (*-tdep.c) can register an interest in a specific
   architecture.  Other GDB components can register a need to maintain
   per-architecture data.

   The mechanisms below ensures that there is only a loose connection
   between the set-architecture command and the various GDB
   components.  Each component can independently register their need
   to maintain architecture specific data with gdbarch.

   Pragmatics:

   Previously, a single TARGET_ARCHITECTURE_HOOK was provided.  It
   didn't scale.

   The more traditional mega-struct containing architecture specific
   data for all the various GDB components was also considered.  Since
   GDB is built from a variable number of (fairly independent)
   components it was determined that the global aproach was not
   applicable.  */


/* Register a new architectural family with GDB.

   Register support for the specified ARCHITECTURE with GDB.  When
   gdbarch determines that the specified architecture has been
   selected, the corresponding INIT function is called.

   --

   The INIT function takes two parameters: INFO which contains the
   information available to gdbarch about the (possibly new)
   architecture; ARCHES which is a list of the previously created
   \`\`struct gdbarch'' for this architecture.

   The INFO parameter is, as far as possible, be pre-initialized with
   information obtained from INFO.ABFD or the global defaults.

   The ARCHES parameter is a linked list (sorted most recently used)
   of all the previously created architures for this architecture
   family.  The (possibly NULL) ARCHES->gdbarch can used to access
   values from the previously selected architecture for this
   architecture family.

   The INIT function shall return any of: NULL - indicating that it
   doesn't recognize the selected architecture; an existing \`\`struct
   gdbarch'' from the ARCHES list - indicating that the new
   architecture is just a synonym for an earlier architecture (see
   gdbarch_list_lookup_by_info()); a newly created \`\`struct gdbarch''
   - that describes the selected architecture (see gdbarch_alloc()).

   The DUMP_TDEP function shall print out all target specific values.
   Care should be taken to ensure that the function works in both the
   multi-arch and non- multi-arch cases.  */

struct gdbarch_list
{
  struct gdbarch *gdbarch;
  struct gdbarch_list *next;
};

struct gdbarch_info
{
  /* Use default: NULL (ZERO).  */
  const struct bfd_arch_info *bfd_arch_info;

  /* Use default: BFD_ENDIAN_UNKNOWN (NB: is not ZERO).  */
  int byte_order;

  int byte_order_for_code;

  /* Use default: NULL (ZERO).  */
  bfd *abfd;

  /* Use default: NULL (ZERO).  */
  struct gdbarch_tdep_info *tdep_info;

  /* Use default: GDB_OSABI_UNINITIALIZED (-1).  */
  enum gdb_osabi osabi;

  /* Use default: NULL (ZERO).  */
  const struct target_desc *target_desc;
};

typedef struct gdbarch *(gdbarch_init_ftype) (struct gdbarch_info info, struct gdbarch_list *arches);
typedef void (gdbarch_dump_tdep_ftype) (struct gdbarch *gdbarch, struct ui_file *file);

/* DEPRECATED - use gdbarch_register() */
extern void register_gdbarch_init (enum bfd_architecture architecture, gdbarch_init_ftype *);

extern void gdbarch_register (enum bfd_architecture architecture,
                              gdbarch_init_ftype *,
                              gdbarch_dump_tdep_ftype *);


/* Return a freshly allocated, NULL terminated, array of the valid
   architecture names.  Since architectures are registered during the
   _initialize phase this function only returns useful information
   once initialization has been completed.  */

extern const char **gdbarch_printable_names (void);


/* Helper function.  Search the list of ARCHES for a GDBARCH that
   matches the information provided by INFO.  */

extern struct gdbarch_list *gdbarch_list_lookup_by_info (struct gdbarch_list *arches, const struct gdbarch_info *info);


/* Helper function.  Create a preliminary \`\`struct gdbarch''.  Perform
   basic initialization using values obtained from the INFO and TDEP
   parameters.  set_gdbarch_*() functions are called to complete the
   initialization of the object.  */

extern struct gdbarch *gdbarch_alloc (const struct gdbarch_info *info, struct gdbarch_tdep *tdep);


/* Helper function.  Free a partially-constructed \`\`struct gdbarch''.
   It is assumed that the caller freeds the \`\`struct
   gdbarch_tdep''.  */

extern void gdbarch_free (struct gdbarch *);


/* Helper function.  Allocate memory from the \`\`struct gdbarch''
   obstack.  The memory is freed when the corresponding architecture
   is also freed.  */

extern void *gdbarch_obstack_zalloc (struct gdbarch *gdbarch, long size);
#define GDBARCH_OBSTACK_CALLOC(GDBARCH, NR, TYPE) ((TYPE *) gdbarch_obstack_zalloc ((GDBARCH), (NR) * sizeof (TYPE)))
#define GDBARCH_OBSTACK_ZALLOC(GDBARCH, TYPE) ((TYPE *) gdbarch_obstack_zalloc ((GDBARCH), sizeof (TYPE)))


/* Helper function.  Force an update of the current architecture.

   The actual architecture selected is determined by INFO, \`\`(gdb) set
   architecture'' et.al., the existing architecture and BFD's default
   architecture.  INFO should be initialized to zero and then selected
   fields should be updated.

   Returns non-zero if the update succeeds.  */

extern int gdbarch_update_p (struct gdbarch_info info);


/* Helper function.  Find an architecture matching info.

   INFO should be initialized using gdbarch_info_init, relevant fields
   set, and then finished using gdbarch_info_fill.

   Returns the corresponding architecture, or NULL if no matching
   architecture was found.  */

extern struct gdbarch *gdbarch_find_by_info (struct gdbarch_info info);


/* Helper function.  Set the target gdbarch to "gdbarch".  */

extern void set_target_gdbarch (struct gdbarch *gdbarch);


/* Register per-architecture data-pointer.

   Reserve space for a per-architecture data-pointer.  An identifier
   for the reserved data-pointer is returned.  That identifer should
   be saved in a local static variable.

   Memory for the per-architecture data shall be allocated using
   gdbarch_obstack_zalloc.  That memory will be deleted when the
   corresponding architecture object is deleted.

   When a previously created architecture is re-selected, the
   per-architecture data-pointer for that previous architecture is
   restored.  INIT() is not re-called.

   Multiple registrarants for any architecture are allowed (and
   strongly encouraged).  */

struct gdbarch_data;

typedef void *(gdbarch_data_pre_init_ftype) (struct obstack *obstack);
extern struct gdbarch_data *gdbarch_data_register_pre_init (gdbarch_data_pre_init_ftype *init);
typedef void *(gdbarch_data_post_init_ftype) (struct gdbarch *gdbarch);
extern struct gdbarch_data *gdbarch_data_register_post_init (gdbarch_data_post_init_ftype *init);
extern void deprecated_set_gdbarch_data (struct gdbarch *gdbarch,
                                         struct gdbarch_data *data,
			                 void *pointer);

extern void *gdbarch_data (struct gdbarch *gdbarch, struct gdbarch_data *);


/* Set the dynamic target-system-dependent parameters (architecture,
   byte-order, ...) using information found in the BFD.  */

extern void set_gdbarch_from_file (bfd *);


/* Initialize the current architecture to the "first" one we find on
   our list.  */

extern void initialize_current_architecture (void);

/* gdbarch trace variable */
extern unsigned int gdbarch_debug;

extern void gdbarch_dump (struct gdbarch *gdbarch, struct ui_file *file);

#endif
EOF
exec 1>&2
#../move-if-change new-gdbarch.h gdbarch.h
compare_new gdbarch.h


#
# C file
#

exec > new-gdbarch.c
copyright
cat <<EOF

#include "defs.h"
#include "arch-utils.h"

#include "gdbcmd.h"
#include "inferior.h" 
#include "symcat.h"

#include "floatformat.h"

#include "gdb_assert.h"
#include "gdb_string.h"
#include "reggroups.h"
#include "osabi.h"
#include "gdb_obstack.h"
#include "observer.h"
#include "regcache.h"
#include "objfiles.h"

/* Static function declarations */

static void alloc_gdbarch_data (struct gdbarch *);

/* Non-zero if we want to trace architecture code.  */

#ifndef GDBARCH_DEBUG
#define GDBARCH_DEBUG 0
#endif
unsigned int gdbarch_debug = GDBARCH_DEBUG;
static void
show_gdbarch_debug (struct ui_file *file, int from_tty,
                    struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Architecture debugging is %s.\\n"), value);
}

static const char *
pformat (const struct floatformat **format)
{
  if (format == NULL)
    return "(null)";
  else
    /* Just print out one of them - this is only for diagnostics.  */
    return format[0]->name;
}

static const char *
pstring (const char *string)
{
  if (string == NULL)
    return "(null)";
  return string;
}

EOF

# gdbarch open the gdbarch object
printf "\n"
printf "/* Maintain the struct gdbarch object.  */\n"
printf "\n"
printf "struct gdbarch\n"
printf "{\n"
printf "  /* Has this architecture been fully initialized?  */\n"
printf "  int initialized_p;\n"
printf "\n"
printf "  /* An obstack bound to the lifetime of the architecture.  */\n"
printf "  struct obstack *obstack;\n"
printf "\n"
printf "  /* basic architectural information.  */\n"
function_list | while do_read
do
    if class_is_info_p
    then
	printf "  ${returntype} ${function};\n"
    fi
done
printf "\n"
printf "  /* target specific vector.  */\n"
printf "  struct gdbarch_tdep *tdep;\n"
printf "  gdbarch_dump_tdep_ftype *dump_tdep;\n"
printf "\n"
printf "  /* per-architecture data-pointers.  */\n"
printf "  unsigned nr_data;\n"
printf "  void **data;\n"
printf "\n"
cat <<EOF
  /* Multi-arch values.

     When extending this structure you must:

     Add the field below.

     Declare set/get functions and define the corresponding
     macro in gdbarch.h.

     gdbarch_alloc(): If zero/NULL is not a suitable default,
     initialize the new field.

     verify_gdbarch(): Confirm that the target updated the field
     correctly.

     gdbarch_dump(): Add a fprintf_unfiltered call so that the new
     field is dumped out

     \`\`startup_gdbarch()'': Append an initial value to the static
     variable (base values on the host's c-type system).

     get_gdbarch(): Implement the set/get functions (probably using
     the macro's as shortcuts).

     */

EOF
function_list | while do_read
do
    if class_is_variable_p
    then
	printf "  ${returntype} ${function};\n"
    elif class_is_function_p
    then
	printf "  gdbarch_${function}_ftype *${function};\n"
    fi
done
printf "};\n"

# A pre-initialized vector
printf "\n"
printf "\n"
cat <<EOF
/* The default architecture uses host values (for want of a better
   choice).  */
EOF
printf "\n"
printf "extern const struct bfd_arch_info bfd_default_arch_struct;\n"
printf "\n"
printf "struct gdbarch startup_gdbarch =\n"
printf "{\n"
printf "  1, /* Always initialized.  */\n"
printf "  NULL, /* The obstack.  */\n"
printf "  /* basic architecture information.  */\n"
function_list | while do_read
do
    if class_is_info_p
    then
	printf "  ${staticdefault},  /* ${function} */\n"
    fi
done
cat <<EOF
  /* target specific vector and its dump routine.  */
  NULL, NULL,
  /*per-architecture data-pointers.  */
  0, NULL,
  /* Multi-arch values */
EOF
function_list | while do_read
do
    if class_is_function_p || class_is_variable_p
    then
	printf "  ${staticdefault},  /* ${function} */\n"
    fi
done
cat <<EOF
  /* startup_gdbarch() */
};

EOF

# Create a new gdbarch struct
cat <<EOF

/* Create a new \`\`struct gdbarch'' based on information provided by
   \`\`struct gdbarch_info''.  */
EOF
printf "\n"
cat <<EOF
struct gdbarch *
gdbarch_alloc (const struct gdbarch_info *info,
               struct gdbarch_tdep *tdep)
{
  struct gdbarch *gdbarch;

  /* Create an obstack for allocating all the per-architecture memory,
     then use that to allocate the architecture vector.  */
  struct obstack *obstack = XMALLOC (struct obstack);
  obstack_init (obstack);
  gdbarch = obstack_alloc (obstack, sizeof (*gdbarch));
  memset (gdbarch, 0, sizeof (*gdbarch));
  gdbarch->obstack = obstack;

  alloc_gdbarch_data (gdbarch);

  gdbarch->tdep = tdep;
EOF
printf "\n"
function_list | while do_read
do
    if class_is_info_p
    then
	printf "  gdbarch->${function} = info->${function};\n"
    fi
done
printf "\n"
printf "  /* Force the explicit initialization of these.  */\n"
function_list | while do_read
do
    if class_is_function_p || class_is_variable_p
    then
	if [ -n "${predefault}" -a "x${predefault}" != "x0" ]
	then
	  printf "  gdbarch->${function} = ${predefault};\n"
	fi
    fi
done
cat <<EOF
  /* gdbarch_alloc() */

  return gdbarch;
}
EOF

# Free a gdbarch struct.
printf "\n"
printf "\n"
cat <<EOF
/* Allocate extra space using the per-architecture obstack.  */

void *
gdbarch_obstack_zalloc (struct gdbarch *arch, long size)
{
  void *data = obstack_alloc (arch->obstack, size);

  memset (data, 0, size);
  return data;
}


/* Free a gdbarch struct.  This should never happen in normal
   operation --- once you've created a gdbarch, you keep it around.
   However, if an architecture's init function encounters an error
   building the structure, it may need to clean up a partially
   constructed gdbarch.  */

void
gdbarch_free (struct gdbarch *arch)
{
  struct obstack *obstack;

  gdb_assert (arch != NULL);
  gdb_assert (!arch->initialized_p);
  obstack = arch->obstack;
  obstack_free (obstack, 0); /* Includes the ARCH.  */
  xfree (obstack);
}
EOF

# verify a new architecture
cat <<EOF


/* Ensure that all values in a GDBARCH are reasonable.  */

static void
verify_gdbarch (struct gdbarch *gdbarch)
{
  struct ui_file *log;
  struct cleanup *cleanups;
  long length;
  char *buf;

  log = mem_fileopen ();
  cleanups = make_cleanup_ui_file_delete (log);
  /* fundamental */
  if (gdbarch->byte_order == BFD_ENDIAN_UNKNOWN)
    fprintf_unfiltered (log, "\n\tbyte-order");
  if (gdbarch->bfd_arch_info == NULL)
    fprintf_unfiltered (log, "\n\tbfd_arch_info");
  /* Check those that need to be defined for the given multi-arch level.  */
EOF
function_list | while do_read
do
    if class_is_function_p || class_is_variable_p
    then
	if [ "x${invalid_p}" = "x0" ]
	then
	    printf "  /* Skip verify of ${function}, invalid_p == 0 */\n"
	elif class_is_predicate_p
	then
	    printf "  /* Skip verify of ${function}, has predicate.  */\n"
	# FIXME: See do_read for potential simplification
 	elif [ -n "${invalid_p}" -a -n "${postdefault}" ]
	then
	    printf "  if (${invalid_p})\n"
	    printf "    gdbarch->${function} = ${postdefault};\n"
	elif [ -n "${predefault}" -a -n "${postdefault}" ]
	then
	    printf "  if (gdbarch->${function} == ${predefault})\n"
	    printf "    gdbarch->${function} = ${postdefault};\n"
	elif [ -n "${postdefault}" ]
	then
	    printf "  if (gdbarch->${function} == 0)\n"
	    printf "    gdbarch->${function} = ${postdefault};\n"
	elif [ -n "${invalid_p}" ]
	then
	    printf "  if (${invalid_p})\n"
	    printf "    fprintf_unfiltered (log, \"\\\\n\\\\t${function}\");\n"
	elif [ -n "${predefault}" ]
	then
	    printf "  if (gdbarch->${function} == ${predefault})\n"
	    printf "    fprintf_unfiltered (log, \"\\\\n\\\\t${function}\");\n"
	fi
    fi
done
cat <<EOF
  buf = ui_file_xstrdup (log, &length);
  make_cleanup (xfree, buf);
  if (length > 0)
    internal_error (__FILE__, __LINE__,
                    _("verify_gdbarch: the following are invalid ...%s"),
                    buf);
  do_cleanups (cleanups);
}
EOF

# dump the structure
printf "\n"
printf "\n"
cat <<EOF
/* Print out the details of the current architecture.  */

void
gdbarch_dump (struct gdbarch *gdbarch, struct ui_file *file)
{
  const char *gdb_nm_file = "<not-defined>";

#if defined (GDB_NM_FILE)
  gdb_nm_file = GDB_NM_FILE;
#endif
  fprintf_unfiltered (file,
                      "gdbarch_dump: GDB_NM_FILE = %s\\n",
                      gdb_nm_file);
EOF
function_list | sort -t: -k 3 | while do_read
do
    # First the predicate
    if class_is_predicate_p
    then
	printf "  fprintf_unfiltered (file,\n"
	printf "                      \"gdbarch_dump: gdbarch_${function}_p() = %%d\\\\n\",\n"
	printf "                      gdbarch_${function}_p (gdbarch));\n"
    fi
    # Print the corresponding value.
    if class_is_function_p
    then
	printf "  fprintf_unfiltered (file,\n"
	printf "                      \"gdbarch_dump: ${function} = <%%s>\\\\n\",\n"
	printf "                      host_address_to_string (gdbarch->${function}));\n"
    else
	# It is a variable
	case "${print}:${returntype}" in
	    :CORE_ADDR )
		fmt="%s"
		print="core_addr_to_string_nz (gdbarch->${function})"
		;;
	    :* )
	        fmt="%s"
		print="plongest (gdbarch->${function})"
		;;
	    * )
	        fmt="%s"
		;;
        esac
	printf "  fprintf_unfiltered (file,\n"
	printf "                      \"gdbarch_dump: ${function} = %s\\\\n\",\n" "${fmt}"
	printf "                      ${print});\n"
    fi
done
cat <<EOF
  if (gdbarch->dump_tdep != NULL)
    gdbarch->dump_tdep (gdbarch, file);
}
EOF


# GET/SET
printf "\n"
cat <<EOF
struct gdbarch_tdep *
gdbarch_tdep (struct gdbarch *gdbarch)
{
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_tdep called\\n");
  return gdbarch->tdep;
}
EOF
printf "\n"
function_list | while do_read
do
    if class_is_predicate_p
    then
	printf "\n"
	printf "int\n"
	printf "gdbarch_${function}_p (struct gdbarch *gdbarch)\n"
	printf "{\n"
        printf "  gdb_assert (gdbarch != NULL);\n"
	printf "  return ${predicate};\n"
	printf "}\n"
    fi
    if class_is_function_p
    then
	printf "\n"
	printf "${returntype}\n"
	if [ "x${formal}" = "xvoid" ]
	then
	  printf "gdbarch_${function} (struct gdbarch *gdbarch)\n"
	else
	  printf "gdbarch_${function} (struct gdbarch *gdbarch, ${formal})\n"
	fi
	printf "{\n"
        printf "  gdb_assert (gdbarch != NULL);\n"
	printf "  gdb_assert (gdbarch->${function} != NULL);\n"
	if class_is_predicate_p && test -n "${predefault}"
	then
	    # Allow a call to a function with a predicate.
	    printf "  /* Do not check predicate: ${predicate}, allow call.  */\n"
	fi
	printf "  if (gdbarch_debug >= 2)\n"
	printf "    fprintf_unfiltered (gdb_stdlog, \"gdbarch_${function} called\\\\n\");\n"
	if [ "x${actual}" = "x-" -o "x${actual}" = "x" ]
	then
	    if class_is_multiarch_p
	    then
		params="gdbarch"
	    else
		params=""
	    fi
	else
	    if class_is_multiarch_p
	    then
		params="gdbarch, ${actual}"
	    else
		params="${actual}"
	    fi
        fi
       	if [ "x${returntype}" = "xvoid" ]
	then
	  printf "  gdbarch->${function} (${params});\n"
	else
	  printf "  return gdbarch->${function} (${params});\n"
	fi
	printf "}\n"
	printf "\n"
	printf "void\n"
	printf "set_gdbarch_${function} (struct gdbarch *gdbarch,\n"
        printf "            `echo ${function} | sed -e 's/./ /g'`  gdbarch_${function}_ftype ${function})\n"
	printf "{\n"
	printf "  gdbarch->${function} = ${function};\n"
	printf "}\n"
    elif class_is_variable_p
    then
	printf "\n"
	printf "${returntype}\n"
	printf "gdbarch_${function} (struct gdbarch *gdbarch)\n"
	printf "{\n"
        printf "  gdb_assert (gdbarch != NULL);\n"
	if [ "x${invalid_p}" = "x0" ]
	then
	    printf "  /* Skip verify of ${function}, invalid_p == 0 */\n"
	elif [ -n "${invalid_p}" ]
	then
	    printf "  /* Check variable is valid.  */\n"
	    printf "  gdb_assert (!(${invalid_p}));\n"
	elif [ -n "${predefault}" ]
	then
	    printf "  /* Check variable changed from pre-default.  */\n"
	    printf "  gdb_assert (gdbarch->${function} != ${predefault});\n"
	fi
	printf "  if (gdbarch_debug >= 2)\n"
	printf "    fprintf_unfiltered (gdb_stdlog, \"gdbarch_${function} called\\\\n\");\n"
	printf "  return gdbarch->${function};\n"
	printf "}\n"
	printf "\n"
	printf "void\n"
	printf "set_gdbarch_${function} (struct gdbarch *gdbarch,\n"
        printf "            `echo ${function} | sed -e 's/./ /g'`  ${returntype} ${function})\n"
	printf "{\n"
	printf "  gdbarch->${function} = ${function};\n"
	printf "}\n"
    elif class_is_info_p
    then
	printf "\n"
	printf "${returntype}\n"
	printf "gdbarch_${function} (struct gdbarch *gdbarch)\n"
	printf "{\n"
        printf "  gdb_assert (gdbarch != NULL);\n"
	printf "  if (gdbarch_debug >= 2)\n"
	printf "    fprintf_unfiltered (gdb_stdlog, \"gdbarch_${function} called\\\\n\");\n"
	printf "  return gdbarch->${function};\n"
	printf "}\n"
    fi
done

# All the trailing guff
cat <<EOF


/* Keep a registry of per-architecture data-pointers required by GDB
   modules.  */

struct gdbarch_data
{
  unsigned index;
  int init_p;
  gdbarch_data_pre_init_ftype *pre_init;
  gdbarch_data_post_init_ftype *post_init;
};

struct gdbarch_data_registration
{
  struct gdbarch_data *data;
  struct gdbarch_data_registration *next;
};

struct gdbarch_data_registry
{
  unsigned nr;
  struct gdbarch_data_registration *registrations;
};

struct gdbarch_data_registry gdbarch_data_registry =
{
  0, NULL,
};

static struct gdbarch_data *
gdbarch_data_register (gdbarch_data_pre_init_ftype *pre_init,
		       gdbarch_data_post_init_ftype *post_init)
{
  struct gdbarch_data_registration **curr;

  /* Append the new registration.  */
  for (curr = &gdbarch_data_registry.registrations;
       (*curr) != NULL;
       curr = &(*curr)->next);
  (*curr) = XMALLOC (struct gdbarch_data_registration);
  (*curr)->next = NULL;
  (*curr)->data = XMALLOC (struct gdbarch_data);
  (*curr)->data->index = gdbarch_data_registry.nr++;
  (*curr)->data->pre_init = pre_init;
  (*curr)->data->post_init = post_init;
  (*curr)->data->init_p = 1;
  return (*curr)->data;
}

struct gdbarch_data *
gdbarch_data_register_pre_init (gdbarch_data_pre_init_ftype *pre_init)
{
  return gdbarch_data_register (pre_init, NULL);
}

struct gdbarch_data *
gdbarch_data_register_post_init (gdbarch_data_post_init_ftype *post_init)
{
  return gdbarch_data_register (NULL, post_init);
}

/* Create/delete the gdbarch data vector.  */

static void
alloc_gdbarch_data (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch->data == NULL);
  gdbarch->nr_data = gdbarch_data_registry.nr;
  gdbarch->data = GDBARCH_OBSTACK_CALLOC (gdbarch, gdbarch->nr_data, void *);
}

/* Initialize the current value of the specified per-architecture
   data-pointer.  */

void
deprecated_set_gdbarch_data (struct gdbarch *gdbarch,
			     struct gdbarch_data *data,
			     void *pointer)
{
  gdb_assert (data->index < gdbarch->nr_data);
  gdb_assert (gdbarch->data[data->index] == NULL);
  gdb_assert (data->pre_init == NULL);
  gdbarch->data[data->index] = pointer;
}

/* Return the current value of the specified per-architecture
   data-pointer.  */

void *
gdbarch_data (struct gdbarch *gdbarch, struct gdbarch_data *data)
{
  gdb_assert (data->index < gdbarch->nr_data);
  if (gdbarch->data[data->index] == NULL)
    {
      /* The data-pointer isn't initialized, call init() to get a
	 value.  */
      if (data->pre_init != NULL)
	/* Mid architecture creation: pass just the obstack, and not
	   the entire architecture, as that way it isn't possible for
	   pre-init code to refer to undefined architecture
	   fields.  */
	gdbarch->data[data->index] = data->pre_init (gdbarch->obstack);
      else if (gdbarch->initialized_p
	       && data->post_init != NULL)
	/* Post architecture creation: pass the entire architecture
	   (as all fields are valid), but be careful to also detect
	   recursive references.  */
	{
	  gdb_assert (data->init_p);
	  data->init_p = 0;
	  gdbarch->data[data->index] = data->post_init (gdbarch);
	  data->init_p = 1;
	}
      else
	/* The architecture initialization hasn't completed - punt -
	 hope that the caller knows what they are doing.  Once
	 deprecated_set_gdbarch_data has been initialized, this can be
	 changed to an internal error.  */
	return NULL;
      gdb_assert (gdbarch->data[data->index] != NULL);
    }
  return gdbarch->data[data->index];
}


/* Keep a registry of the architectures known by GDB.  */

struct gdbarch_registration
{
  enum bfd_architecture bfd_architecture;
  gdbarch_init_ftype *init;
  gdbarch_dump_tdep_ftype *dump_tdep;
  struct gdbarch_list *arches;
  struct gdbarch_registration *next;
};

static struct gdbarch_registration *gdbarch_registry = NULL;

static void
append_name (const char ***buf, int *nr, const char *name)
{
  *buf = xrealloc (*buf, sizeof (char**) * (*nr + 1));
  (*buf)[*nr] = name;
  *nr += 1;
}

const char **
gdbarch_printable_names (void)
{
  /* Accumulate a list of names based on the registed list of
     architectures.  */
  int nr_arches = 0;
  const char **arches = NULL;
  struct gdbarch_registration *rego;

  for (rego = gdbarch_registry;
       rego != NULL;
       rego = rego->next)
    {
      const struct bfd_arch_info *ap;
      ap = bfd_lookup_arch (rego->bfd_architecture, 0);
      if (ap == NULL)
        internal_error (__FILE__, __LINE__,
                        _("gdbarch_architecture_names: multi-arch unknown"));
      do
        {
          append_name (&arches, &nr_arches, ap->printable_name);
          ap = ap->next;
        }
      while (ap != NULL);
    }
  append_name (&arches, &nr_arches, NULL);
  return arches;
}


void
gdbarch_register (enum bfd_architecture bfd_architecture,
                  gdbarch_init_ftype *init,
		  gdbarch_dump_tdep_ftype *dump_tdep)
{
  struct gdbarch_registration **curr;
  const struct bfd_arch_info *bfd_arch_info;

  /* Check that BFD recognizes this architecture */
  bfd_arch_info = bfd_lookup_arch (bfd_architecture, 0);
  if (bfd_arch_info == NULL)
    {
      internal_error (__FILE__, __LINE__,
                      _("gdbarch: Attempt to register "
			"unknown architecture (%d)"),
                      bfd_architecture);
    }
  /* Check that we haven't seen this architecture before.  */
  for (curr = &gdbarch_registry;
       (*curr) != NULL;
       curr = &(*curr)->next)
    {
      if (bfd_architecture == (*curr)->bfd_architecture)
	internal_error (__FILE__, __LINE__,
                        _("gdbarch: Duplicate registration "
			  "of architecture (%s)"),
	                bfd_arch_info->printable_name);
    }
  /* log it */
  if (gdbarch_debug)
    fprintf_unfiltered (gdb_stdlog, "register_gdbarch_init (%s, %s)\n",
			bfd_arch_info->printable_name,
			host_address_to_string (init));
  /* Append it */
  (*curr) = XMALLOC (struct gdbarch_registration);
  (*curr)->bfd_architecture = bfd_architecture;
  (*curr)->init = init;
  (*curr)->dump_tdep = dump_tdep;
  (*curr)->arches = NULL;
  (*curr)->next = NULL;
}

void
register_gdbarch_init (enum bfd_architecture bfd_architecture,
		       gdbarch_init_ftype *init)
{
  gdbarch_register (bfd_architecture, init, NULL);
}


/* Look for an architecture using gdbarch_info.  */

struct gdbarch_list *
gdbarch_list_lookup_by_info (struct gdbarch_list *arches,
                             const struct gdbarch_info *info)
{
  for (; arches != NULL; arches = arches->next)
    {
      if (info->bfd_arch_info != arches->gdbarch->bfd_arch_info)
	continue;
      if (info->byte_order != arches->gdbarch->byte_order)
	continue;
      if (info->osabi != arches->gdbarch->osabi)
	continue;
      if (info->target_desc != arches->gdbarch->target_desc)
	continue;
      return arches;
    }
  return NULL;
}


/* Find an architecture that matches the specified INFO.  Create a new
   architecture if needed.  Return that new architecture.  */

struct gdbarch *
gdbarch_find_by_info (struct gdbarch_info info)
{
  struct gdbarch *new_gdbarch;
  struct gdbarch_registration *rego;

  /* Fill in missing parts of the INFO struct using a number of
     sources: "set ..."; INFOabfd supplied; and the global
     defaults.  */
  gdbarch_info_fill (&info);

  /* Must have found some sort of architecture.  */
  gdb_assert (info.bfd_arch_info != NULL);

  if (gdbarch_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "gdbarch_find_by_info: info.bfd_arch_info %s\n",
			  (info.bfd_arch_info != NULL
			   ? info.bfd_arch_info->printable_name
			   : "(null)"));
      fprintf_unfiltered (gdb_stdlog,
			  "gdbarch_find_by_info: info.byte_order %d (%s)\n",
			  info.byte_order,
			  (info.byte_order == BFD_ENDIAN_BIG ? "big"
			   : info.byte_order == BFD_ENDIAN_LITTLE ? "little"
			   : "default"));
      fprintf_unfiltered (gdb_stdlog,
			  "gdbarch_find_by_info: info.osabi %d (%s)\n",
			  info.osabi, gdbarch_osabi_name (info.osabi));
      fprintf_unfiltered (gdb_stdlog,
			  "gdbarch_find_by_info: info.abfd %s\n",
			  host_address_to_string (info.abfd));
      fprintf_unfiltered (gdb_stdlog,
			  "gdbarch_find_by_info: info.tdep_info %s\n",
			  host_address_to_string (info.tdep_info));
    }

  /* Find the tdep code that knows about this architecture.  */
  for (rego = gdbarch_registry;
       rego != NULL;
       rego = rego->next)
    if (rego->bfd_architecture == info.bfd_arch_info->arch)
      break;
  if (rego == NULL)
    {
      if (gdbarch_debug)
	fprintf_unfiltered (gdb_stdlog, "gdbarch_find_by_info: "
			    "No matching architecture\n");
      return 0;
    }

  /* Ask the tdep code for an architecture that matches "info".  */
  new_gdbarch = rego->init (info, rego->arches);

  /* Did the tdep code like it?  No.  Reject the change and revert to
     the old architecture.  */
  if (new_gdbarch == NULL)
    {
      if (gdbarch_debug)
	fprintf_unfiltered (gdb_stdlog, "gdbarch_find_by_info: "
			    "Target rejected architecture\n");
      return NULL;
    }

  /* Is this a pre-existing architecture (as determined by already
     being initialized)?  Move it to the front of the architecture
     list (keeping the list sorted Most Recently Used).  */
  if (new_gdbarch->initialized_p)
    {
      struct gdbarch_list **list;
      struct gdbarch_list *this;
      if (gdbarch_debug)
	fprintf_unfiltered (gdb_stdlog, "gdbarch_find_by_info: "
			    "Previous architecture %s (%s) selected\n",
			    host_address_to_string (new_gdbarch),
			    new_gdbarch->bfd_arch_info->printable_name);
      /* Find the existing arch in the list.  */
      for (list = &rego->arches;
	   (*list) != NULL && (*list)->gdbarch != new_gdbarch;
	   list = &(*list)->next);
      /* It had better be in the list of architectures.  */
      gdb_assert ((*list) != NULL && (*list)->gdbarch == new_gdbarch);
      /* Unlink THIS.  */
      this = (*list);
      (*list) = this->next;
      /* Insert THIS at the front.  */
      this->next = rego->arches;
      rego->arches = this;
      /* Return it.  */
      return new_gdbarch;
    }

  /* It's a new architecture.  */
  if (gdbarch_debug)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_find_by_info: "
			"New architecture %s (%s) selected\n",
			host_address_to_string (new_gdbarch),
			new_gdbarch->bfd_arch_info->printable_name);
  
  /* Insert the new architecture into the front of the architecture
     list (keep the list sorted Most Recently Used).  */
  {
    struct gdbarch_list *this = XMALLOC (struct gdbarch_list);
    this->next = rego->arches;
    this->gdbarch = new_gdbarch;
    rego->arches = this;
  }    

  /* Check that the newly installed architecture is valid.  Plug in
     any post init values.  */
  new_gdbarch->dump_tdep = rego->dump_tdep;
  verify_gdbarch (new_gdbarch);
  new_gdbarch->initialized_p = 1;

  if (gdbarch_debug)
    gdbarch_dump (new_gdbarch, gdb_stdlog);

  return new_gdbarch;
}

/* Make the specified architecture current.  */

void
set_target_gdbarch (struct gdbarch *new_gdbarch)
{
  gdb_assert (new_gdbarch != NULL);
  gdb_assert (new_gdbarch->initialized_p);
  current_inferior ()->gdbarch = new_gdbarch;
  observer_notify_architecture_changed (new_gdbarch);
  registers_changed ();
}

/* Return the current inferior's arch.  */

struct gdbarch *
target_gdbarch (void)
{
  return current_inferior ()->gdbarch;
}

extern void _initialize_gdbarch (void);

void
_initialize_gdbarch (void)
{
  add_setshow_zuinteger_cmd ("arch", class_maintenance, &gdbarch_debug, _("\\
Set architecture debugging."), _("\\
Show architecture debugging."), _("\\
When non-zero, architecture debugging is enabled."),
                            NULL,
                            show_gdbarch_debug,
                            &setdebuglist, &showdebuglist);
}
EOF

# close things off
exec 1>&2
#../move-if-change new-gdbarch.c gdbarch.c
compare_new gdbarch.c
