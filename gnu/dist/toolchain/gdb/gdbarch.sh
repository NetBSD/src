#!/usr/local/bin/bash

# Architecture commands for GDB, the GNU debugger.
# Copyright 1998-2000 Free Software Foundation, Inc.
#
# This file is part of GDB.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

compare_new ()
{
    file=$1
    if ! test -r ${file}
    then
	echo "${file} missing? cp new-${file} ${file}" 1>&2
    elif diff -c ${file} new-${file}
    then
	echo "${file} unchanged" 1>&2
    else
	echo "${file} has changed? cp new-${file} ${file}" 1>&2
    fi
}


# DEFAULT is a valid fallback definition of a MACRO when
# multi-arch is not enabled.
default_is_fallback_p ()
{
    [ "${default}" != "" -a "${invalid_p}" = "0" ]
    # FIXME: cagney - not until after 5.0
    false
}

# Format of the input table
read="class level macro returntype function formal actual attrib startup default invalid_p fmt print print_p description"

do_read ()
{
    if eval read $read
    then
	test "${startup}" || startup=0
	test "${fmt}" || fmt="%ld"
	test "${print}" || print="(long) ${macro}"
	#test "${default}" || default=0
	:
    else
	false
    fi
}


# dump out/verify the doco
for field in ${read}
do
  case ${field} in

    class ) : ;;

	# # -> line disable
	# f -> function
	#   hiding a function
	# v -> variable
	#   hiding a variable
	# i -> set from info
	#   hiding something from the ``struct info'' object

    level ) : ;;

	# See GDB_MULTI_ARCH description.  Having GDB_MULTI_ARCH >=
	# LEVEL is a predicate on checking that a given method is
	# initialized (using INVALID_P).

    macro ) : ;;

	# The name of the MACRO that this method is to be accessed by.

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

    attrib ) : ;;

	# Any GCC attributes that should be attached to the function
	# declaration.  At present this field is unused.

    startup ) : ;;

	# To help with the GDB startup a static gdbarch object is
	# created.  STARTUP is the value to insert into that static
	# gdbarch object.

	# By default ``0'' is used.

    default ) : ;;

	# Any initial value to assign to a new gdbarch object after it
	# as been malloc()ed.  Zero is used by default.

	# Specify a non-empty DEFAULT and a zero INVALID_P to create a
	# fallback value or function for when multi-arch is disabled.
	# Specify a zero DEFAULT function to make that fallback
	# illegal to call.

    invalid_p ) : ;;

	# A predicate equation that validates MEMBER. Non-zero is
	# returned if the code creating the new architecture failed to
	# initialize the MEMBER or initialized the member to something
	# invalid. By default, a check that the value is no longer
	# equal to DEFAULT ips performed.  The equation ``0'' disables
	# the invalid_p check.

    fmt ) : ;;

	# printf style format string that can be used to print out the
	# MEMBER.  Sometimes "%s" is useful.  For functions, this is
	# ignored and the function address is printed.

	# By default ```%ld'' is used.  

    print ) : ;;

	# An optional equation that casts MEMBER to a value suitable
	# for formatting by FMT.

	# By default ``(long)'' is used.

    print_p ) : ;;

	# An optional indicator for any predicte to wrap around the
	# print member code.

	#   # -> Wrap print up in ``#ifdef MACRO''
	#   exp -> Wrap print up in ``if (${print_p}) ...
	#   ``'' -> No predicate

    description ) : ;;

      # Currently unused.

    *) exit 1;;
  esac
done

IFS=:

function_list ()
{
  # See below (DOCO) for description of each field
  cat <<EOF |
i:2:TARGET_ARCHITECTURE:const struct bfd_arch_info *:bfd_arch_info::::&bfd_default_arch_struct:::%s:TARGET_ARCHITECTURE->printable_name:TARGET_ARCHITECTURE != NULL
#
i:2:TARGET_BYTE_ORDER:int:byte_order::::BIG_ENDIAN
#
v:1:TARGET_BFD_VMA_BIT:int:bfd_vma_bit::::8 * sizeof (void*):TARGET_ARCHITECTURE->bits_per_address:0
v:1:TARGET_PTR_BIT:int:ptr_bit::::8 * sizeof (void*):0
#v:1:TARGET_CHAR_BIT:int:char_bit::::8 * sizeof (char):0
v:1:TARGET_SHORT_BIT:int:short_bit::::8 * sizeof (short):0
v:1:TARGET_INT_BIT:int:int_bit::::8 * sizeof (int):0
v:1:TARGET_LONG_BIT:int:long_bit::::8 * sizeof (long):0
v:1:TARGET_LONG_LONG_BIT:int:long_long_bit::::8 * sizeof (LONGEST):0
v:1:TARGET_FLOAT_BIT:int:float_bit::::8 * sizeof (float):0
v:1:TARGET_DOUBLE_BIT:int:double_bit::::8 * sizeof (double):0
v:1:TARGET_LONG_DOUBLE_BIT:int:long_double_bit::::8 * sizeof (long double):0
#
f:1:TARGET_READ_PC:CORE_ADDR:read_pc:int pid:pid::0:0
f:1:TARGET_WRITE_PC:void:write_pc:CORE_ADDR val, int pid:val, pid::0:0
f:1:TARGET_READ_FP:CORE_ADDR:read_fp:void:::0:0
f:1:TARGET_WRITE_FP:void:write_fp:CORE_ADDR val:val::0:0
f:1:TARGET_READ_SP:CORE_ADDR:read_sp:void:::0:0
f:1:TARGET_WRITE_SP:void:write_sp:CORE_ADDR val:val::0:0
#
v:2:NUM_REGS:int:num_regs::::0:-1
v:2:SP_REGNUM:int:sp_regnum::::0:-1
v:2:FP_REGNUM:int:fp_regnum::::0:-1
v:2:PC_REGNUM:int:pc_regnum::::0:-1
f:2:REGISTER_NAME:char *:register_name:int regnr:regnr:::legacy_register_name:0
v:2:REGISTER_SIZE:int:register_size::::0:-1
v:2:REGISTER_BYTES:int:register_bytes::::0:-1
f:2:REGISTER_BYTE:int:register_byte:int reg_nr:reg_nr::0:0
f:2:REGISTER_RAW_SIZE:int:register_raw_size:int reg_nr:reg_nr::0:0
v:2:MAX_REGISTER_RAW_SIZE:int:max_register_raw_size::::0:-1
f:2:REGISTER_VIRTUAL_SIZE:int:register_virtual_size:int reg_nr:reg_nr::0:0
v:2:MAX_REGISTER_VIRTUAL_SIZE:int:max_register_virtual_size::::0:-1
f:2:REGISTER_VIRTUAL_TYPE:struct type *:register_virtual_type:int reg_nr:reg_nr::0:0
#
v:1:USE_GENERIC_DUMMY_FRAMES:int:use_generic_dummy_frames::::0:-1
v:2:CALL_DUMMY_LOCATION:int:call_dummy_location::::0:0
f:2:CALL_DUMMY_ADDRESS:CORE_ADDR:call_dummy_address:void:::0:0:gdbarch->call_dummy_location == AT_ENTRY_POINT && gdbarch->call_dummy_address == 0:
v:2:CALL_DUMMY_START_OFFSET:CORE_ADDR:call_dummy_start_offset::::0:-1::0x%08lx
v:2:CALL_DUMMY_BREAKPOINT_OFFSET:CORE_ADDR:call_dummy_breakpoint_offset::::0:-1::0x%08lx
v:1:CALL_DUMMY_BREAKPOINT_OFFSET_P:int:call_dummy_breakpoint_offset_p::::0:-1
v:2:CALL_DUMMY_LENGTH:int:call_dummy_length::::0:-1::::CALL_DUMMY_LOCATION == BEFORE_TEXT_END || CALL_DUMMY_LOCATION == AFTER_TEXT_END
f:2:PC_IN_CALL_DUMMY:int:pc_in_call_dummy:CORE_ADDR pc, CORE_ADDR sp, CORE_ADDR frame_address:pc, sp, frame_address::0:0
v:1:CALL_DUMMY_P:int:call_dummy_p::::0:-1
v:2:CALL_DUMMY_WORDS:LONGEST *:call_dummy_words::::0:legacy_call_dummy_words:0:0x%08lx
v:2:SIZEOF_CALL_DUMMY_WORDS:int:sizeof_call_dummy_words::::0:legacy_sizeof_call_dummy_words:0:0x%08lx
v:1:CALL_DUMMY_STACK_ADJUST_P:int:call_dummy_stack_adjust_p::::0:-1::0x%08lx
v:2:CALL_DUMMY_STACK_ADJUST:int:call_dummy_stack_adjust::::0::gdbarch->call_dummy_stack_adjust_p && gdbarch->call_dummy_stack_adjust == 0:0x%08lx::CALL_DUMMY_STACK_ADJUST_P
f:2:FIX_CALL_DUMMY:void:fix_call_dummy:char *dummy, CORE_ADDR pc, CORE_ADDR fun, int nargs, struct value **args, struct type *type, int gcc_p:dummy, pc, fun, nargs, args, type, gcc_p::0:0
#
v:2:BELIEVE_PCC_PROMOTION:int:believe_pcc_promotion::::0:::::#
v:2:BELIEVE_PCC_PROMOTION_TYPE:int:believe_pcc_promotion_type::::0:::::#
f:2:COERCE_FLOAT_TO_DOUBLE:int:coerce_float_to_double:struct type *formal, struct type *actual:formal, actual:::default_coerce_float_to_double:0
f:1:GET_SAVED_REGISTER:void:get_saved_register:char *raw_buffer, int *optimized, CORE_ADDR *addrp, struct frame_info *frame, int regnum, enum lval_type *lval:raw_buffer, optimized, addrp, frame, regnum, lval::generic_get_saved_register:0
#
f:1:REGISTER_CONVERTIBLE:int:register_convertible:int nr:nr:::generic_register_convertible_not:0
f:2:REGISTER_CONVERT_TO_VIRTUAL:void:register_convert_to_virtual:int regnum, struct type *type, char *from, char *to:regnum, type, from, to:::0:0
f:2:REGISTER_CONVERT_TO_RAW:void:register_convert_to_raw:struct type *type, int regnum, char *from, char *to:type, regnum, from, to:::0:0
#
f:2:EXTRACT_RETURN_VALUE:void:extract_return_value:struct type *type, char *regbuf, char *valbuf:type, regbuf, valbuf::0:0
f:1:PUSH_ARGUMENTS:CORE_ADDR:push_arguments:int nargs, struct value **args, CORE_ADDR sp, int struct_return, CORE_ADDR struct_addr:nargs, args, sp, struct_return, struct_addr::0:0
f:2:PUSH_DUMMY_FRAME:void:push_dummy_frame:void:-:::0
f:1:PUSH_RETURN_ADDRESS:CORE_ADDR:push_return_address:CORE_ADDR pc, CORE_ADDR sp:pc, sp:::0
f:2:POP_FRAME:void:pop_frame:void:-:::0
#
# I wish that these would just go away....
f:2:D10V_MAKE_DADDR:CORE_ADDR:d10v_make_daddr:CORE_ADDR x:x:::0:0
f:2:D10V_MAKE_IADDR:CORE_ADDR:d10v_make_iaddr:CORE_ADDR x:x:::0:0
f:2:D10V_DADDR_P:int:d10v_daddr_p:CORE_ADDR x:x:::0
f:2:D10V_IADDR_P:int:d10v_iaddr_p:CORE_ADDR x:x:::0
f:2:D10V_CONVERT_DADDR_TO_RAW:CORE_ADDR:d10v_convert_daddr_to_raw:CORE_ADDR x:x:::0
f:2:D10V_CONVERT_IADDR_TO_RAW:CORE_ADDR:d10v_convert_iaddr_to_raw:CORE_ADDR x:x:::0
#
f:2:STORE_STRUCT_RETURN:void:store_struct_return:CORE_ADDR addr, CORE_ADDR sp:addr, sp:::0
f:2:STORE_RETURN_VALUE:void:store_return_value:struct type *type, char *valbuf:type, valbuf:::0
f:2:EXTRACT_STRUCT_VALUE_ADDRESS:CORE_ADDR:extract_struct_value_address:char *regbuf:regbuf:::0
f:2:USE_STRUCT_CONVENTION:int:use_struct_convention:int gcc_p, struct type *value_type:gcc_p, value_type:::0
#
f:2:FRAME_INIT_SAVED_REGS:void:frame_init_saved_regs:struct frame_info *frame:frame::0:0
f:2:INIT_EXTRA_FRAME_INFO:void:init_extra_frame_info:int fromleaf, struct frame_info *frame:fromleaf, frame:::0
#
f:2:SKIP_PROLOGUE:CORE_ADDR:skip_prologue:CORE_ADDR ip:ip::0:0
f:2:INNER_THAN:int:inner_than:CORE_ADDR lhs, CORE_ADDR rhs:lhs, rhs::0:0
f:2:BREAKPOINT_FROM_PC:unsigned char *:breakpoint_from_pc:CORE_ADDR *pcptr, int *lenptr:pcptr, lenptr:::legacy_breakpoint_from_pc:0
f:2:MEMORY_INSERT_BREAKPOINT:int:memory_insert_breakpoint:CORE_ADDR addr, char *contents_cache:addr, contents_cache::0:default_memory_insert_breakpoint:0
f:2:MEMORY_REMOVE_BREAKPOINT:int:memory_remove_breakpoint:CORE_ADDR addr, char *contents_cache:addr, contents_cache::0:default_memory_remove_breakpoint:0
v:2:DECR_PC_AFTER_BREAK:CORE_ADDR:decr_pc_after_break::::0:-1
v:2:FUNCTION_START_OFFSET:CORE_ADDR:function_start_offset::::0:-1
#
f:2:REMOTE_TRANSLATE_XFER_ADDRESS:void:remote_translate_xfer_address:CORE_ADDR gdb_addr, int gdb_len, CORE_ADDR *rem_addr, int *rem_len:gdb_addr, gdb_len, rem_addr, rem_len:::generic_remote_translate_xfer_address:0
#
v:2:FRAME_ARGS_SKIP:CORE_ADDR:frame_args_skip::::0:-1
f:2:FRAMELESS_FUNCTION_INVOCATION:int:frameless_function_invocation:struct frame_info *fi:fi:::generic_frameless_function_invocation_not:0
f:2:FRAME_CHAIN:CORE_ADDR:frame_chain:struct frame_info *frame:frame::0:0
f:1:FRAME_CHAIN_VALID:int:frame_chain_valid:CORE_ADDR chain, struct frame_info *thisframe:chain, thisframe::0:0
f:2:FRAME_SAVED_PC:CORE_ADDR:frame_saved_pc:struct frame_info *fi:fi::0:0
f:2:FRAME_ARGS_ADDRESS:CORE_ADDR:frame_args_address:struct frame_info *fi:fi::0:0
f:2:FRAME_LOCALS_ADDRESS:CORE_ADDR:frame_locals_address:struct frame_info *fi:fi::0:0
f:2:SAVED_PC_AFTER_CALL:CORE_ADDR:saved_pc_after_call:struct frame_info *frame:frame::0:0
f:2:FRAME_NUM_ARGS:int:frame_num_args:struct frame_info *frame:frame::0:0
#
EOF
  grep -v '^#'
}


# dump it out
if true
then
  exec > new-gdbarch
  function_list | while do_read # eval read $read
  do
    cat <<EOF
${class} ${macro}(${actual})
  ${returntype} ${function} ($formal)${attrib}
    level=${level}
    startup=${startup}
    default=${default}
    invalid_p=${invalid_p}
    fmt=${fmt}
    print=${print}
    print_p=${print_p}
    description=${description}
EOF
  done
  exec 1>&2
fi

copyright ()
{
cat <<EOF
/* *INDENT-OFF* */ /* THIS FILE IS GENERATED */

/* Dynamic architecture support for GDB, the GNU debugger.
   Copyright 1998-1999, Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* This file was created with the aid of \`\`gdbarch.sh''.

   The bourn shell script \`\`gdbarch.sh'' creates the files
   \`\`new-gdbarch.c'' and \`\`new-gdbarch.h and then compares them
   against the existing \`\`gdbarch.[hc]''.  Any differences found
   being reported.

   If editing this file, please also run gdbarch.sh and merge any
   changes into that script. Conversely, when makeing sweeping changes
   to this file, modifying gdbarch.sh and using its output may prove
   easier. */

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

struct frame_info;
struct value;


#ifndef GDB_MULTI_ARCH
#define GDB_MULTI_ARCH 0
#endif

extern struct gdbarch *current_gdbarch;


/* See gdb/doc/gdbint.texi for a discussion of the GDB_MULTI_ARCH
   macro */


/* If any of the following are defined, the target wasn't correctly
   converted. */

#if GDB_MULTI_ARCH
#if defined (CALL_DUMMY)
#error "CALL_DUMMY: replaced by CALL_DUMMY_WORDS/SIZEOF_CALL_DUMMY_WORDS"
#endif
#endif

#if GDB_MULTI_ARCH
#if defined (REGISTER_NAMES)
#error "REGISTER_NAMES: replaced by REGISTER_NAME"
#endif
#endif

#if GDB_MULTI_ARCH
#if defined (EXTRA_FRAME_INFO)
#error "EXTRA_FRAME_INFO: replaced by struct frame_extra_info"
#endif
#endif

#if GDB_MULTI_ARCH
#if defined (FRAME_FIND_SAVED_REGS)
#error "FRAME_FIND_SAVED_REGS: replaced by FRAME_INIT_SAVED_REGS"
#endif
#endif
EOF

# function typedef's
echo ""
echo ""
echo "/* The following are pre-initialized by GDBARCH. */"
function_list | while do_read # eval read $read
do
  case "${class}" in
    "i" )
	echo ""
	echo "extern ${returntype} gdbarch_${function} (struct gdbarch *gdbarch);"
	echo "/* set_gdbarch_${function}() - not applicable - pre-initialized. */"
	echo "#if GDB_MULTI_ARCH"
	echo "#if (GDB_MULTI_ARCH > 1) || !defined (${macro})"
	echo "#define ${macro} (gdbarch_${function} (current_gdbarch))"
	echo "#endif"
	echo "#endif"
	;;
  esac
done

# function typedef's
echo ""
echo ""
echo "/* The following are initialized by the target dependant code. */"
function_list | while do_read # eval read $read
do
  case "${class}" in
    "v" )
	echo ""
	echo "extern ${returntype} gdbarch_${function} (struct gdbarch *gdbarch);"
	echo "extern void set_gdbarch_${function} (struct gdbarch *gdbarch, ${returntype} ${function});"
	echo "#if GDB_MULTI_ARCH"
	echo "#if (GDB_MULTI_ARCH > 1) || !defined (${macro})"
	echo "#define ${macro} (gdbarch_${function} (current_gdbarch))"
	echo "#endif"
	echo "#endif"
	;;
    "f" )
	echo ""
	echo "typedef ${returntype} (gdbarch_${function}_ftype) (${formal});"
	if [ "${formal}" = "void" ]
	then
	  echo "extern ${returntype} gdbarch_${function} (struct gdbarch *gdbarch);"
	else
	  echo "extern ${returntype} gdbarch_${function} (struct gdbarch *gdbarch, ${formal});"
	fi
	echo "extern void set_gdbarch_${function} (struct gdbarch *gdbarch, gdbarch_${function}_ftype *${function});"
	if ! default_is_fallback_p
	then
	    echo "#if GDB_MULTI_ARCH"
	fi
	echo "#if (GDB_MULTI_ARCH > 1) || !defined (${macro})"
	if [ "${actual}" = "" ]
	then
	  echo "#define ${macro}() (gdbarch_${function} (current_gdbarch))"
	elif [ "${actual}" = "-" ]
	then
	  echo "#define ${macro} (gdbarch_${function} (current_gdbarch))"
	else
	  echo "#define ${macro}(${actual}) (gdbarch_${function} (current_gdbarch, ${actual}))"
	fi
	echo "#endif"
	if ! default_is_fallback_p
	then
	    echo "#endif"
	fi
	;;
  esac
done

# close it off
cat <<EOF

extern struct gdbarch_tdep *gdbarch_tdep (struct gdbarch *gdbarch);


/* Mechanism for co-ordinating the selection of a specific
   architecture.

   GDB targets (*-tdep.c) can register an interest in a specific
   architecture.  Other GDB components can register a need to maintain
   per-architecture data.

   The mechanisms below ensures that there is only a loose connection
   between the set-architecture command and the various GDB
   components.  Each component can independantly register their need
   to maintain architecture specific data with gdbarch.

   Pragmatics:

   Previously, a single TARGET_ARCHITECTURE_HOOK was provided.  It
   didn't scale.

   The more traditional mega-struct containing architecture specific
   data for all the various GDB components was also considered.  Since
   GDB is built from a variable number of (fairly independant)
   components it was determined that the global aproach was not
   applicable. */


/* Register a new architectural family with GDB.

   Register support for the specified ARCHITECTURE with GDB.  When
   gdbarch determines that the specified architecture has been
   selected, the corresponding INIT function is called.

   --

   The INIT function takes two parameters: INFO which contains the
   information available to gdbarch about the (possibly new)
   architecture; ARCHES which is a list of the previously created
   \`\`struct gdbarch'' for this architecture.

   The INIT function parameter INFO shall, as far as possible, be
   pre-initialized with information obtained from INFO.ABFD or
   previously selected architecture (if similar).  INIT shall ensure
   that the INFO.BYTE_ORDER is non-zero.

   The INIT function shall return any of: NULL - indicating that it
   doesn't reconize the selected architecture; an existing \`\`struct
   gdbarch'' from the ARCHES list - indicating that the new
   architecture is just a synonym for an earlier architecture (see
   gdbarch_list_lookup_by_info()); a newly created \`\`struct gdbarch''
   - that describes the selected architecture (see
   gdbarch_alloc()). */

struct gdbarch_list
{
  struct gdbarch *gdbarch;
  struct gdbarch_list *next;
};

struct gdbarch_info
{
  /* Use default: bfd_arch_unknown (ZERO). */
  enum bfd_architecture bfd_architecture;

  /* Use default: NULL (ZERO). */
  const struct bfd_arch_info *bfd_arch_info;

  /* Use default: 0 (ZERO). */
  int byte_order;

  /* Use default: NULL (ZERO). */
  bfd *abfd;

  /* Use default: NULL (ZERO). */
  struct gdbarch_tdep_info *tdep_info;
};

typedef struct gdbarch *(gdbarch_init_ftype) (struct gdbarch_info info, struct gdbarch_list *arches);

extern void register_gdbarch_init (enum bfd_architecture architecture, gdbarch_init_ftype *);


/* Helper function.  Search the list of ARCHES for a GDBARCH that
   matches the information provided by INFO. */

extern struct gdbarch_list *gdbarch_list_lookup_by_info (struct gdbarch_list *arches,  const struct gdbarch_info *info);


/* Helper function.  Create a preliminary \`\`struct gdbarch''.  Perform
   basic initialization using values obtained from the INFO andTDEP
   parameters.  set_gdbarch_*() functions are called to complete the
   initialization of the object. */

extern struct gdbarch *gdbarch_alloc (const struct gdbarch_info *info, struct gdbarch_tdep *tdep);


/* Helper function.  Free a partially-constructed \`\`struct gdbarch''.  */
extern void gdbarch_free (struct gdbarch *);


/* Helper function. Force an update of the current architecture.  Used
   by legacy targets that have added their own target specific
   architecture manipulation commands.

   The INFO parameter shall be fully initialized (\`\`memset (&INFO,
   sizeof (info), 0)'' set relevant fields) before gdbarch_update() is
   called.  gdbarch_update() shall initialize any \`\`default'' fields
   using information obtained from the previous architecture or
   INFO.ABFD (if specified) before calling the corresponding
   architectures INIT function. */

extern int gdbarch_update (struct gdbarch_info info);



/* Register per-architecture data-pointer.

   Reserve space for a per-architecture data-pointer.  An identifier
   for the reserved data-pointer is returned.  That identifer should
   be saved in a local static.

   When a new architecture is selected, INIT() is called.  When a
   previous architecture is re-selected, the per-architecture
   data-pointer for that previous architecture is restored (INIT() is
   not called).

   INIT() shall return the initial value for the per-architecture
   data-pointer for the current architecture.

   Multiple registrarants for any architecture are allowed (and
   strongly encouraged).  */

typedef void *(gdbarch_data_ftype) (void);
extern struct gdbarch_data *register_gdbarch_data (gdbarch_data_ftype *init);

/* Return the value of the per-architecture data-pointer for the
   current architecture. */

extern void *gdbarch_data (struct gdbarch_data*);



/* Register per-architecture memory region.

   Provide a memory-region swap mechanism.  Per-architecture memory
   region are created.  These memory regions are swapped whenever the
   architecture is changed.  For a new architecture, the memory region
   is initialized with zero (0) and the INIT function is called.

   Memory regions are swapped / initialized in the order that they are
   registered.  NULL DATA and/or INIT values can be specified.

   New code should use register_gdbarch_data(). */

typedef void (gdbarch_swap_ftype) (void);
extern void register_gdbarch_swap (void *data, unsigned long size, gdbarch_swap_ftype *init);
#define REGISTER_GDBARCH_SWAP(VAR) register_gdbarch_swap (&(VAR), sizeof ((VAR)), NULL)



/* The target-system-dependant byte order is dynamic */

/* TARGET_BYTE_ORDER_SELECTABLE_P determines if the target endianness
   is selectable at runtime.  The user can use the \`\`set endian''
   command to change it.  TARGET_BYTE_ORDER_AUTO is nonzero when
   target_byte_order should be auto-detected (from the program image
   say). */

#if GDB_MULTI_ARCH
/* Multi-arch GDB is always bi-endian. */
#define TARGET_BYTE_ORDER_SELECTABLE_P 1
#endif

#ifndef TARGET_BYTE_ORDER_SELECTABLE_P
/* compat - Catch old targets that define TARGET_BYTE_ORDER_SLECTABLE
   when they should have defined TARGET_BYTE_ORDER_SELECTABLE_P 1 */
#ifdef TARGET_BYTE_ORDER_SELECTABLE
#define TARGET_BYTE_ORDER_SELECTABLE_P 1
#else
#define TARGET_BYTE_ORDER_SELECTABLE_P 0
#endif
#endif

extern int target_byte_order;
#ifdef TARGET_BYTE_ORDER_SELECTABLE
/* compat - Catch old targets that define TARGET_BYTE_ORDER_SELECTABLE
   and expect defs.h to re-define TARGET_BYTE_ORDER. */
#undef TARGET_BYTE_ORDER
#endif
#ifndef TARGET_BYTE_ORDER
#define TARGET_BYTE_ORDER (target_byte_order + 0)
#endif

extern int target_byte_order_auto;
#ifndef TARGET_BYTE_ORDER_AUTO
#define TARGET_BYTE_ORDER_AUTO (target_byte_order_auto + 0)
#endif



/* The target-system-dependant BFD architecture is dynamic */

extern int target_architecture_auto;
#ifndef TARGET_ARCHITECTURE_AUTO
#define TARGET_ARCHITECTURE_AUTO (target_architecture_auto + 0)
#endif

extern const struct bfd_arch_info *target_architecture;
#ifndef TARGET_ARCHITECTURE
#define TARGET_ARCHITECTURE (target_architecture + 0)
#endif

/* Notify the target dependant backend of a change to the selected
   architecture. A zero return status indicates that the target did
   not like the change. */

extern int (*target_architecture_hook) (const struct bfd_arch_info *);



/* The target-system-dependant disassembler is semi-dynamic */

#include "dis-asm.h"		/* Get defs for disassemble_info */

extern int dis_asm_read_memory (bfd_vma memaddr, bfd_byte *myaddr,
				unsigned int len, disassemble_info *info);

extern void dis_asm_memory_error (int status, bfd_vma memaddr,
				  disassemble_info *info);

extern void dis_asm_print_address (bfd_vma addr,
				   disassemble_info *info);

extern int (*tm_print_insn) (bfd_vma, disassemble_info*);
extern disassemble_info tm_print_insn_info;
#ifndef TARGET_PRINT_INSN
#define TARGET_PRINT_INSN(vma, info) (*tm_print_insn) (vma, info)
#endif
#ifndef TARGET_PRINT_INSN_INFO
#define TARGET_PRINT_INSN_INFO (&tm_print_insn_info)
#endif



/* Explicit test for D10V architecture.
   USE of these macro's is *STRONGLY* discouraged. */

#define GDB_TARGET_IS_D10V (TARGET_ARCHITECTURE->arch == bfd_arch_d10v)
#ifndef D10V_MAKE_DADDR
#define D10V_MAKE_DADDR(X) (internal_error ("gdbarch: D10V_MAKE_DADDR"), 0)
#endif
#ifndef D10V_MAKE_IADDR
#define D10V_MAKE_IADDR(X) (internal_error ("gdbarch: D10V_MAKE_IADDR"), 0)
#endif


/* Fallback definition of FRAMELESS_FUNCTION_INVOCATION */
#ifndef FRAMELESS_FUNCTION_INVOCATION
#define FRAMELESS_FUNCTION_INVOCATION(FI) (0)
#endif


/* Fallback definition of REGISTER_CONVERTIBLE etc */
extern int generic_register_convertible_not (int reg_nr);
#ifndef REGISTER_CONVERTIBLE
#define REGISTER_CONVERTIBLE(x) (0)
#endif
#ifndef REGISTER_CONVERT_TO_VIRTUAL
#define REGISTER_CONVERT_TO_VIRTUAL(x, y, z, a)
#endif
#ifndef REGISTER_CONVERT_TO_RAW
#define REGISTER_CONVERT_TO_RAW(x, y, z, a)
#endif


/* Fallback definition for EXTRACT_STRUCT_VALUE_ADDRESS */
#ifndef EXTRACT_STRUCT_VALUE_ADDRESS
#define EXTRACT_STRUCT_VALUE_ADDRESS_P (0)
#define EXTRACT_STRUCT_VALUE_ADDRESS(X) (internal_error ("gdbarch: EXTRACT_STRUCT_VALUE_ADDRESS"), 0)
#else
#ifndef EXTRACT_STRUCT_VALUE_ADDRESS_P
#define EXTRACT_STRUCT_VALUE_ADDRESS_P (1)
#endif
#endif


/* Fallback definition for REGISTER_NAME for systems still defining
   REGISTER_NAMES. */
#ifndef REGISTER_NAME
extern char *gdb_register_names[];
#define REGISTER_NAME(i) gdb_register_names[i]
#endif


/* Set the dynamic target-system-dependant parameters (architecture,
   byte-order, ...) using information found in the BFD */

extern void set_gdbarch_from_file (bfd *);


/* Explicitly set the dynamic target-system-dependant parameters based
   on bfd_architecture and machine. */

extern void set_architecture_from_arch_mach (enum bfd_architecture, unsigned long);


/* Initialize the current architecture to the "first" one we find on
   our list.  */

extern void initialize_current_architecture (void);

/* Helper function for targets that don't know how my arguments are
   being passed */

extern int frame_num_args_unknown (struct frame_info *fi);


/* gdbarch trace variable */
extern int gdbarch_debug;

extern void gdbarch_dump (void);

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
#include "gdbarch-utils.h"

#if GDB_MULTI_ARCH
#include "gdbcmd.h"
#include "inferior.h" /* enum CALL_DUMMY_LOCATION et.al. */
#else
/* Just include everything in sight so that the every old definition
   of macro is visible. */
#include "gdb_string.h"
#include <ctype.h>
#include "symtab.h"
#include "frame.h"
#include "inferior.h"
#include "breakpoint.h"
#include "gdb_wait.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "target.h"
#include "gdbthread.h"
#include "annotate.h"
#include "symfile.h"		/* for overlay functions */
#endif
#include "symcat.h"


/* Static function declarations */

static void verify_gdbarch (struct gdbarch *gdbarch);
static void init_gdbarch_data (struct gdbarch *);
static void init_gdbarch_swap (struct gdbarch *);
static void swapout_gdbarch_swap (struct gdbarch *);
static void swapin_gdbarch_swap (struct gdbarch *);

/* Convenience macro for allocting typesafe memory. */

#ifndef XMALLOC
#define XMALLOC(TYPE) (TYPE*) xmalloc (sizeof (TYPE))
#endif


/* Non-zero if we want to trace architecture code.  */

#ifndef GDBARCH_DEBUG
#define GDBARCH_DEBUG 0
#endif
int gdbarch_debug = GDBARCH_DEBUG;

EOF

# gdbarch open the gdbarch object
echo ""
echo "/* Maintain the struct gdbarch object */"
echo ""
echo "struct gdbarch"
echo "{"
echo "  /* basic architectural information */"
function_list | while do_read # eval read $read
do
  case "${class}" in
    "i" ) echo "  ${returntype} ${function};" ;;
  esac
done
echo ""
echo "  /* target specific vector. */"
echo "  struct gdbarch_tdep *tdep;"
echo ""
echo "  /* per-architecture data-pointers */"
echo "  int nr_data;"
echo "  void **data;"
echo ""
echo "  /* per-architecture swap-regions */"
echo "  struct gdbarch_swap *swap;"
echo ""
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

     gdbarch_dump(): Add a fprintf_unfiltered call to so that the new
     field is dumped out

     \`\`startup_gdbarch()'': Append an initial value to the static
     variable (base values on the host's c-type system).

     get_gdbarch(): Implement the set/get functions (probably using
     the macro's as shortcuts).

     */

EOF
function_list | while do_read # eval read $read
do
  case "${class}" in
    "v" ) echo "  ${returntype} ${function};" ;;
    "f" ) echo "  gdbarch_${function}_ftype *${function}${attrib};" ;;
  esac
done
echo "};"

# A pre-initialized vector
echo ""
echo ""
cat <<EOF
/* The default architecture uses host values (for want of a better
   choice). */
EOF
echo ""
echo "extern const struct bfd_arch_info bfd_default_arch_struct;"
echo ""
echo "struct gdbarch startup_gdbarch = {"
echo "  /* basic architecture information */"
function_list | while do_read # eval read $read
do
  case "${class}" in
    "i" ) 
      echo "  ${startup},"
    ;;
  esac
done
cat <<EOF
  /* target specific vector */
  NULL,
  /*per-architecture data-pointers and swap regions */
  0, NULL, NULL,
  /* Multi-arch values */
EOF
function_list | while do_read # eval read $read
do
  case "${class}" in
    "f" | "v" )
      echo "  ${startup},"
    ;;
  esac
done
cat <<EOF
  /* startup_gdbarch() */
};
struct gdbarch *current_gdbarch = &startup_gdbarch;
EOF

# Create a new gdbarch struct
echo ""
echo ""
cat <<EOF
/* Create a new \`\`struct gdbarch'' based in information provided by
   \`\`struct gdbarch_info''. */
EOF
echo ""
cat <<EOF
struct gdbarch *
gdbarch_alloc (const struct gdbarch_info *info,
               struct gdbarch_tdep *tdep)
{
  struct gdbarch *gdbarch = XMALLOC (struct gdbarch);
  memset (gdbarch, 0, sizeof (*gdbarch));

  gdbarch->tdep = tdep;
EOF
echo ""
function_list | while do_read # eval read $read
do
  case "${class}" in
    "i" ) echo "  gdbarch->${function} = info->${function};"
  esac
done
echo ""
echo "  /* Force the explicit initialization of these. */"
function_list | while do_read # eval read $read
do
  case "${class}" in
    "f" | "v" )
	if [ "${default}" != "" -a "${default}" != "0" ]
	then
	  echo "  gdbarch->${function} = ${default};"
	fi
	;;
  esac
done
cat <<EOF
  /* gdbarch_alloc() */

  return gdbarch;
}
EOF

# Free a gdbarch struct.
echo ""
echo ""
cat <<EOF
/* Free a gdbarch struct.  This should never happen in normal
   operation --- once you've created a gdbarch, you keep it around.
   However, if an architecture's init function encounters an error
   building the structure, it may need to clean up a partially
   constructed gdbarch.  */
void
gdbarch_free (struct gdbarch *arch)
{
  /* At the moment, this is trivial.  */
  free (arch);
}
EOF

# verify a new architecture
echo ""
echo ""
echo "/* Ensure that all values in a GDBARCH are reasonable. */"
echo ""
cat <<EOF
static void
verify_gdbarch (struct gdbarch *gdbarch)
{
  /* Only perform sanity checks on a multi-arch target. */
  if (GDB_MULTI_ARCH <= 0)
    return;
  /* fundamental */
  if (gdbarch->byte_order == 0)
    internal_error ("verify_gdbarch: byte-order unset");
  if (gdbarch->bfd_arch_info == NULL)
    internal_error ("verify_gdbarch: bfd_arch_info unset");
  /* Check those that need to be defined for the given multi-arch level. */
EOF
function_list | while do_read # eval read $read
do
  case "${class}" in
    "f" | "v" )
	if [ "${invalid_p}" = "0" ]
	then
	    echo "  /* Skip verify of ${function}, invalid_p == 0 */"
 	elif [ "${invalid_p}" ]
	then
	    echo "  if ((GDB_MULTI_ARCH >= ${level})"
	    echo "      && (${invalid_p}))"
	    echo "    internal_error (\"gdbarch: verify_gdbarch: ${function} invalid\");"
	elif [ "${default}" ]
	then
	    echo "  if ((GDB_MULTI_ARCH >= ${level})"
	    echo "      && (gdbarch->${function} == ${default}))"
	    echo "    internal_error (\"gdbarch: verify_gdbarch: ${function} invalid\");"
	fi
	;;
  esac
done
cat <<EOF
}
EOF

# dump the structure
echo ""
echo ""
echo "/* Print out the details of the current architecture. */"
echo ""
cat <<EOF
void
gdbarch_dump (void)
{
EOF
function_list | while do_read # eval read $read
do
  case "${class}" in
    "f" )
	echo "  fprintf_unfiltered (gdb_stdlog,"
	echo "                      \"gdbarch_update: ${macro} = 0x%08lx\\n\","
	echo "                      (long) current_gdbarch->${function}"
	echo "                      /*${macro} ()*/);"
	;;
    * )
	if [ "${print_p}" = "#" ]
	then
	  echo "#ifdef ${macro}"
	  echo "  fprintf_unfiltered (gdb_stdlog,"
	  echo "                      \"gdbarch_update: ${macro} = ${fmt}\\n\","
	  echo "                      ${print});"
	  echo "#endif"
	elif [ "${print_p}" ]
	then
	  echo "  if (${print_p})"
	  echo "    fprintf_unfiltered (gdb_stdlog,"
	  echo "                        \"gdbarch_update: ${macro} = ${fmt}\\n\","
	  echo "                        ${print});"
	else
	  echo "  fprintf_unfiltered (gdb_stdlog,"
	  echo "                      \"gdbarch_update: ${macro} = ${fmt}\\n\","
	  echo "                      ${print});"
	fi
	;;
  esac
done
echo "}"


# GET/SET
echo ""
cat <<EOF
struct gdbarch_tdep *
gdbarch_tdep (struct gdbarch *gdbarch)
{
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_tdep called\n");
  return gdbarch->tdep;
}
EOF
echo ""
function_list | while do_read # eval read $read
do
  case "${class}" in
    "f" )
	echo ""
	echo "${returntype}"
	if [ "${formal}" = "void" ]
	then
	  echo "gdbarch_${function} (struct gdbarch *gdbarch)"
	else
	  echo "gdbarch_${function} (struct gdbarch *gdbarch, ${formal})"
	fi
	echo "{"
	if default_is_fallback_p && [ "${default}" != "0" ]
	then
	    echo "  if (GDB_MULTI_ARCH == 0)"
	    if [ "${returntype}" = "void" ]
	    then
		echo "    {"
		echo "      ${default} (${actual});"
		echo "      return;"
		echo "    }"
	    else
		echo "    return ${default} (${actual});"
	    fi
	fi
        echo "  if (gdbarch->${function} == 0)"
        echo "    internal_error (\"gdbarch: gdbarch_${function} invalid\");"
	echo "  if (gdbarch_debug >= 2)"
	echo "    fprintf_unfiltered (gdb_stdlog, \"gdbarch_${function} called\n\");"
        test "${actual}" = "-" && actual=""
       	if [ "${returntype}" = "void" ]
	then
	  echo "  gdbarch->${function} (${actual});"
	else
	  echo "  return gdbarch->${function} (${actual});"
	fi
	echo "}"
	echo ""
	echo "void"
	echo "set_gdbarch_${function} (struct gdbarch *gdbarch,"
        echo "            `echo ${function} | sed -e 's/./ /g'`  gdbarch_${function}_ftype ${function})"
	echo "{"
	echo "  gdbarch->${function} = ${function};"
	echo "}"
	;;
    "v" )
	echo ""
	echo "${returntype}"
	echo "gdbarch_${function} (struct gdbarch *gdbarch)"
	echo "{"
	if [ "${invalid_p}" = "0" ]
	then
	    echo "  /* Skip verify of ${function}, invalid_p == 0 */"
	elif [ "${invalid_p}" ]
	then
	  echo "  if (${invalid_p})"
	  echo "    internal_error (\"gdbarch: gdbarch_${function} invalid\");"
	elif [ "${default}" ]
	then
	  echo "  if (gdbarch->${function} == ${default})"
	  echo "    internal_error (\"gdbarch: gdbarch_${function} invalid\");"
	fi
	echo "  if (gdbarch_debug >= 2)"
	echo "    fprintf_unfiltered (gdb_stdlog, \"gdbarch_${function} called\n\");"
	echo "  return gdbarch->${function};"
	echo "}"
	echo ""
	echo "void"
	echo "set_gdbarch_${function} (struct gdbarch *gdbarch,"
        echo "            `echo ${function} | sed -e 's/./ /g'`  ${returntype} ${function})"
	echo "{"
	echo "  gdbarch->${function} = ${function};"
	echo "}"
	;;
    "i" )
	echo ""
	echo "${returntype}"
	echo "gdbarch_${function} (struct gdbarch *gdbarch)"
	echo "{"
	echo "  if (gdbarch_debug >= 2)"
	echo "    fprintf_unfiltered (gdb_stdlog, \"gdbarch_${function} called\n\");"
	echo "  return gdbarch->${function};"
	echo "}"
	;;
  esac
done

# All the trailing guff
cat <<EOF


/* Keep a registrary of per-architecture data-pointers required by GDB
   modules. */

struct gdbarch_data
{
  int index;
};

struct gdbarch_data_registration
{
  gdbarch_data_ftype *init;
  struct gdbarch_data *data;
  struct gdbarch_data_registration *next;
};

struct gdbarch_data_registrary
{
  int nr;
  struct gdbarch_data_registration *registrations;
};

struct gdbarch_data_registrary gdbarch_data_registrary =
{
  0, NULL,
};

struct gdbarch_data *
register_gdbarch_data (gdbarch_data_ftype *init)
{
  struct gdbarch_data_registration **curr;
  for (curr = &gdbarch_data_registrary.registrations;
       (*curr) != NULL;
       curr = &(*curr)->next);
  (*curr) = XMALLOC (struct gdbarch_data_registration);
  (*curr)->next = NULL;
  (*curr)->init = init;
  (*curr)->data = XMALLOC (struct gdbarch_data);
  (*curr)->data->index = gdbarch_data_registrary.nr++;
  return (*curr)->data;
}


/* Walk through all the registered users initializing each in turn. */

static void
init_gdbarch_data (struct gdbarch *gdbarch)
{
  struct gdbarch_data_registration *rego;
  gdbarch->nr_data = gdbarch_data_registrary.nr + 1;
  gdbarch->data = xmalloc (sizeof (void*) * gdbarch->nr_data);
  for (rego = gdbarch_data_registrary.registrations;
       rego != NULL;
       rego = rego->next)
    {
      if (rego->data->index < gdbarch->nr_data)
	gdbarch->data[rego->data->index] = rego->init ();
    }
}


/* Return the current value of the specified per-architecture
   data-pointer. */

void *
gdbarch_data (data)
     struct gdbarch_data *data;
{
  if (data->index >= current_gdbarch->nr_data)
    internal_error ("gdbarch_data: request for non-existant data.");
  return current_gdbarch->data[data->index];
}



/* Keep a registrary of swaped data required by GDB modules. */

struct gdbarch_swap
{
  void *swap;
  struct gdbarch_swap_registration *source;
  struct gdbarch_swap *next;
};

struct gdbarch_swap_registration
{
  void *data;
  unsigned long sizeof_data;
  gdbarch_swap_ftype *init;
  struct gdbarch_swap_registration *next;
};

struct gdbarch_swap_registrary
{
  int nr;
  struct gdbarch_swap_registration *registrations;
};

struct gdbarch_swap_registrary gdbarch_swap_registrary = 
{
  0, NULL,
};

void
register_gdbarch_swap (void *data,
		       unsigned long sizeof_data,
		       gdbarch_swap_ftype *init)
{
  struct gdbarch_swap_registration **rego;
  for (rego = &gdbarch_swap_registrary.registrations;
       (*rego) != NULL;
       rego = &(*rego)->next);
  (*rego) = XMALLOC (struct gdbarch_swap_registration);
  (*rego)->next = NULL;
  (*rego)->init = init;
  (*rego)->data = data;
  (*rego)->sizeof_data = sizeof_data;
}


static void
init_gdbarch_swap (struct gdbarch *gdbarch)
{
  struct gdbarch_swap_registration *rego;
  struct gdbarch_swap **curr = &gdbarch->swap;
  for (rego = gdbarch_swap_registrary.registrations;
       rego != NULL;
       rego = rego->next)
    {
      if (rego->data != NULL)
	{
	  (*curr) = XMALLOC (struct gdbarch_swap);
	  (*curr)->source = rego;
	  (*curr)->swap = xmalloc (rego->sizeof_data);
	  (*curr)->next = NULL;
	  memset (rego->data, 0, rego->sizeof_data);
	  curr = &(*curr)->next;
	}
      if (rego->init != NULL)
	rego->init ();
    }
}

static void
swapout_gdbarch_swap (struct gdbarch *gdbarch)
{
  struct gdbarch_swap *curr;
  for (curr = gdbarch->swap;
       curr != NULL;
       curr = curr->next)
    memcpy (curr->swap, curr->source->data, curr->source->sizeof_data);
}

static void
swapin_gdbarch_swap (struct gdbarch *gdbarch)
{
  struct gdbarch_swap *curr;
  for (curr = gdbarch->swap;
       curr != NULL;
       curr = curr->next)
    memcpy (curr->source->data, curr->swap, curr->source->sizeof_data);
}


/* Keep a registrary of the architectures known by GDB. */

struct gdbarch_init_registration
{
  enum bfd_architecture bfd_architecture;
  gdbarch_init_ftype *init;
  struct gdbarch_list *arches;
  struct gdbarch_init_registration *next;
};

static struct gdbarch_init_registration *gdbarch_init_registrary = NULL;

void
register_gdbarch_init (enum bfd_architecture bfd_architecture,
                       gdbarch_init_ftype *init)
{
  struct gdbarch_init_registration **curr;
  const struct bfd_arch_info *bfd_arch_info;
  /* Check that BFD reconizes this architecture */
  bfd_arch_info = bfd_lookup_arch (bfd_architecture, 0);
  if (bfd_arch_info == NULL)
    {
      internal_error ("gdbarch: Attempt to register unknown architecture (%d)", bfd_architecture);
    }
  /* Check that we haven't seen this architecture before */
  for (curr = &gdbarch_init_registrary;
       (*curr) != NULL;
       curr = &(*curr)->next)
    {
      if (bfd_architecture == (*curr)->bfd_architecture)
	internal_error ("gdbarch: Duplicate registraration of architecture (%s)",
	       bfd_arch_info->printable_name);
    }
  /* log it */
  if (gdbarch_debug)
    fprintf_unfiltered (gdb_stdlog, "register_gdbarch_init (%s, 0x%08lx)\n",
			bfd_arch_info->printable_name,
			(long) init);
  /* Append it */
  (*curr) = XMALLOC (struct gdbarch_init_registration);
  (*curr)->bfd_architecture = bfd_architecture;
  (*curr)->init = init;
  (*curr)->arches = NULL;
  (*curr)->next = NULL;
}
  


/* Look for an architecture using gdbarch_info.  Base search on only
   BFD_ARCH_INFO and BYTE_ORDER. */

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
      return arches;
    }
  return NULL;
}


/* Update the current architecture. Return ZERO if the update request
   failed. */

int
gdbarch_update (struct gdbarch_info info)
{
  struct gdbarch *new_gdbarch;
  struct gdbarch_list **list;
  struct gdbarch_init_registration *rego;

  /* Fill in any missing bits. Most important is the bfd_architecture
     which is used to select the target architecture. */
  if (info.bfd_architecture == bfd_arch_unknown)
    {
      if (info.bfd_arch_info != NULL)
	info.bfd_architecture = info.bfd_arch_info->arch;
      else if (info.abfd != NULL)
	info.bfd_architecture = bfd_get_arch (info.abfd);
      /* FIXME - should query BFD for its default architecture. */
      else
	info.bfd_architecture = current_gdbarch->bfd_arch_info->arch;
    }
  if (info.bfd_arch_info == NULL)
    {
      if (target_architecture_auto && info.abfd != NULL)
	info.bfd_arch_info = bfd_get_arch_info (info.abfd);
      else
	info.bfd_arch_info = current_gdbarch->bfd_arch_info;
    }
  if (info.byte_order == 0)
    {
      if (target_byte_order_auto && info.abfd != NULL)
	info.byte_order = (bfd_big_endian (info.abfd) ? BIG_ENDIAN
			   : bfd_little_endian (info.abfd) ? LITTLE_ENDIAN
			   : 0);
      else
	info.byte_order = current_gdbarch->byte_order;
      /* FIXME - should query BFD for its default byte-order. */
    }
  /* A default for abfd? */

  /* Find the target that knows about this architecture. */
  for (rego = gdbarch_init_registrary;
       rego != NULL && rego->bfd_architecture != info.bfd_architecture;
       rego = rego->next);
  if (rego == NULL)
    {
      if (gdbarch_debug)
	fprintf_unfiltered (gdb_stdlog, "gdbarch_update: No matching architecture\n");
      return 0;
    }

  if (gdbarch_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "gdbarch_update: info.bfd_architecture %d (%s)\n",
			  info.bfd_architecture,
			  bfd_lookup_arch (info.bfd_architecture, 0)->printable_name);
      fprintf_unfiltered (gdb_stdlog,
			  "gdbarch_update: info.bfd_arch_info %s\n",
			  (info.bfd_arch_info != NULL
			   ? info.bfd_arch_info->printable_name
			   : "(null)"));
      fprintf_unfiltered (gdb_stdlog,
			  "gdbarch_update: info.byte_order %d (%s)\n",
			  info.byte_order,
			  (info.byte_order == BIG_ENDIAN ? "big"
			   : info.byte_order == LITTLE_ENDIAN ? "little"
			   : "default"));
      fprintf_unfiltered (gdb_stdlog,
			  "gdbarch_update: info.abfd 0x%lx\n",
			  (long) info.abfd);
      fprintf_unfiltered (gdb_stdlog,
			  "gdbarch_update: info.tdep_info 0x%lx\n",
			  (long) info.tdep_info);
    }

  /* Ask the target for a replacement architecture. */
  new_gdbarch = rego->init (info, rego->arches);

  /* Did the target like it?  No. Reject the change. */
  if (new_gdbarch == NULL)
    {
      if (gdbarch_debug)
	fprintf_unfiltered (gdb_stdlog, "gdbarch_update: Target rejected architecture\n");
      return 0;
    }

  /* Did the architecture change?  No. Do nothing. */
  if (current_gdbarch == new_gdbarch)
    {
      if (gdbarch_debug)
	fprintf_unfiltered (gdb_stdlog, "gdbarch_update: Architecture 0x%08lx (%s) unchanged\n",
			    (long) new_gdbarch,
			    new_gdbarch->bfd_arch_info->printable_name);
      return 1;
    }

  /* Swap all data belonging to the old target out */
  swapout_gdbarch_swap (current_gdbarch);

  /* Is this a pre-existing architecture?  Yes. Swap it in.  */
  for (list = &rego->arches;
       (*list) != NULL;
       list = &(*list)->next)
    {
      if ((*list)->gdbarch == new_gdbarch)
	{
	  if (gdbarch_debug)
	    fprintf_unfiltered (gdb_stdlog, "gdbarch_update: Previous architecture 0x%08lx (%s) selected\n",
				(long) new_gdbarch,
				new_gdbarch->bfd_arch_info->printable_name);
	  current_gdbarch = new_gdbarch;
	  swapin_gdbarch_swap (new_gdbarch);
	  return 1;
	}
    }
    
  /* Append this new architecture to this targets list. */
  (*list) = XMALLOC (struct gdbarch_list);
  (*list)->next = NULL;
  (*list)->gdbarch = new_gdbarch;

  /* Switch to this new architecture.  Dump it out. */
  current_gdbarch = new_gdbarch;
  if (gdbarch_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "gdbarch_update: New architecture 0x%08lx (%s) selected\n",
			  (long) new_gdbarch,
			  new_gdbarch->bfd_arch_info->printable_name);
      gdbarch_dump ();
    }
  
  /* Check that the newly installed architecture is valid.  */
  verify_gdbarch (new_gdbarch);

  /* Initialize the per-architecture memory (swap) areas.
     CURRENT_GDBARCH must be update before these modules are
     called. */
  init_gdbarch_swap (new_gdbarch);
  
  /* Initialize the per-architecture data-pointer of all parties that
     registered an interest in this architecture.  CURRENT_GDBARCH
     must be updated before these modules are called. */
  init_gdbarch_data (new_gdbarch);
  
  return 1;
}



/* Functions to manipulate the endianness of the target.  */

#ifdef TARGET_BYTE_ORDER_SELECTABLE
/* compat - Catch old targets that expect a selectable byte-order to
   default to BIG_ENDIAN */
#ifndef TARGET_BYTE_ORDER_DEFAULT
#define TARGET_BYTE_ORDER_DEFAULT BIG_ENDIAN
#endif
#endif
#if !TARGET_BYTE_ORDER_SELECTABLE_P
#ifndef TARGET_BYTE_ORDER_DEFAULT
/* compat - Catch old non byte-order selectable targets that do not
   define TARGET_BYTE_ORDER_DEFAULT and instead expect
   TARGET_BYTE_ORDER to be used as the default.  For targets that
   defined neither TARGET_BYTE_ORDER nor TARGET_BYTE_ORDER_DEFAULT the
   below will get a strange compiler warning. */
#define TARGET_BYTE_ORDER_DEFAULT TARGET_BYTE_ORDER
#endif
#endif
#ifndef TARGET_BYTE_ORDER_DEFAULT
#define TARGET_BYTE_ORDER_DEFAULT BIG_ENDIAN /* arbitrary */
#endif
int target_byte_order = TARGET_BYTE_ORDER_DEFAULT;
int target_byte_order_auto = 1;

/* Chain containing the \"set endian\" commands.  */
static struct cmd_list_element *endianlist = NULL;

/* Called by \`\`show endian''.  */
static void
show_endian (char *args, int from_tty)
{
  char *msg =
    (TARGET_BYTE_ORDER_AUTO
     ? "The target endianness is set automatically (currently %s endian)\n"
     : "The target is assumed to be %s endian\n");
  printf_unfiltered (msg, (TARGET_BYTE_ORDER == BIG_ENDIAN ? "big" : "little"));
}

/* Called if the user enters \`\`set endian'' without an argument.  */
static void
set_endian (char *args, int from_tty)
{
  printf_unfiltered ("\"set endian\" must be followed by \"auto\", \"big\" or \"little\".\n");
  show_endian (args, from_tty);
}

/* Called by \`\`set endian big''.  */
static void
set_endian_big (char *args, int from_tty)
{
  if (TARGET_BYTE_ORDER_SELECTABLE_P)
    {
      target_byte_order = BIG_ENDIAN;
      target_byte_order_auto = 0;
      if (GDB_MULTI_ARCH)
	{
	  struct gdbarch_info info;
	  memset (&info, 0, sizeof info);
	  info.byte_order = BIG_ENDIAN;
	  gdbarch_update (info);
	}
    }
  else
    {
      printf_unfiltered ("Byte order is not selectable.");
      show_endian (args, from_tty);
    }
}

/* Called by \`\`set endian little''.  */
static void
set_endian_little (char *args, int from_tty)
{
  if (TARGET_BYTE_ORDER_SELECTABLE_P)
    {
      target_byte_order = LITTLE_ENDIAN;
      target_byte_order_auto = 0;
      if (GDB_MULTI_ARCH)
	{
	  struct gdbarch_info info;
	  memset (&info, 0, sizeof info);
	  info.byte_order = LITTLE_ENDIAN;
	  gdbarch_update (info);
	}
    }
  else
    {
      printf_unfiltered ("Byte order is not selectable.");
      show_endian (args, from_tty);
    }
}

/* Called by \`\`set endian auto''.  */
static void
set_endian_auto (char *args, int from_tty)
{
  if (TARGET_BYTE_ORDER_SELECTABLE_P)
    {
      target_byte_order_auto = 1;
    }
  else
    {
      printf_unfiltered ("Byte order is not selectable.");
      show_endian (args, from_tty);
    }
}

/* Set the endianness from a BFD.  */
static void
set_endian_from_file (bfd *abfd)
{
  if (TARGET_BYTE_ORDER_SELECTABLE_P)
    {
      int want;
      
      if (bfd_big_endian (abfd))
	want = BIG_ENDIAN;
      else
	want = LITTLE_ENDIAN;
      if (TARGET_BYTE_ORDER_AUTO)
	target_byte_order = want;
      else if (TARGET_BYTE_ORDER != want)
	warning ("%s endian file does not match %s endian target.",
		 want == BIG_ENDIAN ? "big" : "little",
		 TARGET_BYTE_ORDER == BIG_ENDIAN ? "big" : "little");
    }
  else
    {
      if (bfd_big_endian (abfd)
	  ? TARGET_BYTE_ORDER != BIG_ENDIAN
	  : TARGET_BYTE_ORDER == BIG_ENDIAN)
	warning ("%s endian file does not match %s endian target.",
		 bfd_big_endian (abfd) ? "big" : "little",
		 TARGET_BYTE_ORDER == BIG_ENDIAN ? "big" : "little");
    }
}



/* Functions to manipulate the architecture of the target */

enum set_arch { set_arch_auto, set_arch_manual };

int target_architecture_auto = 1;
extern const struct bfd_arch_info bfd_default_arch_struct;
const struct bfd_arch_info *target_architecture = &bfd_default_arch_struct;
int (*target_architecture_hook) (const struct bfd_arch_info *ap);

static void show_endian (char *, int);
static void set_endian (char *, int);
static void set_endian_big (char *, int);
static void set_endian_little (char *, int);
static void set_endian_auto (char *, int);
static void set_endian_from_file (bfd *);
static int arch_ok (const struct bfd_arch_info *arch);
static void set_arch (const struct bfd_arch_info *arch, enum set_arch type);
static void show_architecture (char *, int);
static void set_architecture (char *, int);
static void info_architecture (char *, int);
static void set_architecture_from_file (bfd *);

/* Do the real work of changing the current architecture */

static int
arch_ok (const struct bfd_arch_info *arch)
{
  /* Should be performing the more basic check that the binary is
     compatible with GDB. */
  /* Check with the target that the architecture is valid. */
  return (target_architecture_hook == NULL
	  || target_architecture_hook (arch));
}

static void
set_arch (const struct bfd_arch_info *arch,
          enum set_arch type)
{
  switch (type)
    {
    case set_arch_auto:
      if (!arch_ok (arch))
	warning ("Target may not support %s architecture",
		 arch->printable_name);
      target_architecture = arch;
      break;
    case set_arch_manual:
      if (!arch_ok (arch))
	{
	  printf_unfiltered ("Target does not support \`%s' architecture.\n",
			     arch->printable_name);
	}
      else
	{
	  target_architecture_auto = 0;
	  target_architecture = arch;
	}
      break;
    }
  if (gdbarch_debug)
    gdbarch_dump ();
}

/* Called if the user enters \`\`show architecture'' without an argument. */
static void
show_architecture (char *args, int from_tty)
{
  const char *arch;
  arch = TARGET_ARCHITECTURE->printable_name;
  if (target_architecture_auto)
    printf_filtered ("The target architecture is set automatically (currently %s)\n", arch);
  else
    printf_filtered ("The target architecture is assumed to be %s\n", arch);
}

/* Called if the user enters \`\`set architecture'' with or without an
   argument. */
static void
set_architecture (char *args, int from_tty)
{
  if (args == NULL)
    {
      printf_unfiltered ("\"set architecture\" must be followed by \"auto\" or an architecture name.\n");
    }
  else if (strcmp (args, "auto") == 0)
    {
      target_architecture_auto = 1;
    }
  else if (GDB_MULTI_ARCH)
    {
      const struct bfd_arch_info *arch = bfd_scan_arch (args);
      if (arch == NULL)
	printf_unfiltered ("Architecture \`%s' not reconized.\n", args);
      else
	{
	  struct gdbarch_info info;
	  memset (&info, 0, sizeof info);
	  info.bfd_arch_info = arch;
	  if (gdbarch_update (info))
	    target_architecture_auto = 0;
	  else
	    printf_unfiltered ("Architecture \`%s' not reconized.\n", args);
	}
    }
  else
    {
      const struct bfd_arch_info *arch = bfd_scan_arch (args);
      if (arch != NULL)
	set_arch (arch, set_arch_manual);
      else
	printf_unfiltered ("Architecture \`%s' not reconized.\n", args);
    }
}

/* Called if the user enters \`\`info architecture'' without an argument. */
static void
info_architecture (char *args, int from_tty)
{
  enum bfd_architecture a;
  if (GDB_MULTI_ARCH)
    {
      if (gdbarch_init_registrary != NULL)
	{
	  struct gdbarch_init_registration *rego;
	  printf_filtered ("Available architectures are:\n");
	  for (rego = gdbarch_init_registrary;
	       rego != NULL;
	       rego = rego->next)
	    {
	      const struct bfd_arch_info *ap;
	      ap = bfd_lookup_arch (rego->bfd_architecture, 0);
	      if (ap != NULL)
		{
		  do
		    {
		      printf_filtered (" %s", ap->printable_name);
		      ap = ap->next;
		    }
		  while (ap != NULL);
		  printf_filtered ("\n");
		}
	    }
	}
      else
	{
	  printf_filtered ("There are no available architectures.\n");
	}
      return;
    }
  printf_filtered ("Available architectures are:\n");
  for (a = bfd_arch_obscure + 1; a < bfd_arch_last; a++)
    {
      const struct bfd_arch_info *ap = bfd_lookup_arch (a, 0);
      if (ap != NULL)
	{
	  do
	    {
	      printf_filtered (" %s", ap->printable_name);
	      ap = ap->next;
	    }
	  while (ap != NULL);
	  printf_filtered ("\n");
	}
    }
}

/* Set the architecture from arch/machine */
void
set_architecture_from_arch_mach (arch, mach)
     enum bfd_architecture arch;
     unsigned long mach;
{
  const struct bfd_arch_info *wanted = bfd_lookup_arch (arch, mach);
  if (wanted != NULL)
    set_arch (wanted, set_arch_manual);
  else
    internal_error ("gdbarch: hardwired architecture/machine not reconized");
}

/* Set the architecture from a BFD */
static void
set_architecture_from_file (bfd *abfd)
{
  const struct bfd_arch_info *wanted = bfd_get_arch_info (abfd);
  if (target_architecture_auto)
    {
      set_arch (wanted, set_arch_auto);
    }
  else if (wanted != target_architecture)
    {
      warning ("%s architecture file may be incompatible with %s target.",
	       wanted->printable_name,
	       target_architecture->printable_name);
    }
}


/* Misc helper functions for targets. */

int
frame_num_args_unknown (fi)
     struct frame_info *fi;
{
  return -1;
}


int
generic_register_convertible_not (num)
     int num;
{
  return 0;
}
  
/* Disassembler */

/* Pointer to the target-dependent disassembly function.  */
int (*tm_print_insn) (bfd_vma, disassemble_info *);
disassemble_info tm_print_insn_info;



/* Set the dynamic target-system-dependant parameters (architecture,
   byte-order) using information found in the BFD */

void
set_gdbarch_from_file (abfd)
     bfd *abfd;
{
  if (GDB_MULTI_ARCH)
    {
      struct gdbarch_info info;
      memset (&info, 0, sizeof info);
      info.abfd = abfd;
      gdbarch_update (info);
      return;
    }
  set_architecture_from_file (abfd);
  set_endian_from_file (abfd);
}


#if defined (CALL_DUMMY)
/* FIXME - this should go away */
LONGEST call_dummy_words[] = CALL_DUMMY;
int sizeof_call_dummy_words = sizeof (call_dummy_words);
#endif


/* Initialize the current architecture.  */
void
initialize_current_architecture ()
{
  if (GDB_MULTI_ARCH)
    {
      struct gdbarch_init_registration *rego;
      const struct bfd_arch_info *chosen = NULL;
      for (rego = gdbarch_init_registrary; rego != NULL; rego = rego->next)
	{
	  const struct bfd_arch_info *ap
	    = bfd_lookup_arch (rego->bfd_architecture, 0);

	  /* Choose the first architecture alphabetically.  */
	  if (chosen == NULL
	      || strcmp (ap->printable_name, chosen->printable_name) < 0)
	    chosen = ap;
	}

      if (chosen != NULL)
	{
	  struct gdbarch_info info;
	  memset (&info, 0, sizeof info);
	  info.bfd_arch_info = chosen;
	  gdbarch_update (info);
	}
    }
}

extern void _initialize_gdbarch (void);
void
_initialize_gdbarch ()
{
  struct cmd_list_element *c;

  add_prefix_cmd ("endian", class_support, set_endian,
		  "Set endianness of target.",
		  &endianlist, "set endian ", 0, &setlist);
  add_cmd ("big", class_support, set_endian_big,
	   "Set target as being big endian.", &endianlist);
  add_cmd ("little", class_support, set_endian_little,
	   "Set target as being little endian.", &endianlist);
  add_cmd ("auto", class_support, set_endian_auto,
	   "Select target endianness automatically.", &endianlist);
  add_cmd ("endian", class_support, show_endian,
	   "Show endianness of target.", &showlist);

  add_cmd ("architecture", class_support, set_architecture,
	   "Set architecture of target.", &setlist);
  add_alias_cmd ("processor", "architecture", class_support, 1, &setlist);
  add_cmd ("architecture", class_support, show_architecture,
	   "Show architecture of target.", &showlist);
  add_cmd ("architecture", class_support, info_architecture,
	   "List supported target architectures", &infolist);

  INIT_DISASSEMBLE_INFO_NO_ARCH (tm_print_insn_info, gdb_stdout, (fprintf_ftype)fprintf_filtered);
  tm_print_insn_info.flavour = bfd_target_unknown_flavour;
  tm_print_insn_info.read_memory_func = dis_asm_read_memory;
  tm_print_insn_info.memory_error_func = dis_asm_memory_error;
  tm_print_insn_info.print_address_func = dis_asm_print_address;

  add_show_from_set (add_set_cmd ("arch",
				  class_maintenance,
				  var_zinteger,
				  (char *)&gdbarch_debug,
				  "Set architecture debugging.\n\\
When non-zero, architecture debugging is enabled.", &setdebuglist),
		     &showdebuglist);
  c = add_set_cmd ("archdebug",
		   class_maintenance,
		   var_zinteger,
		   (char *)&gdbarch_debug,
		   "Set architecture debugging.\n\\
When non-zero, architecture debugging is enabled.", &setlist);

  deprecate_cmd (c, "set debug arch");
  deprecate_cmd (add_show_from_set (c, &showlist), "show debug arch");
}
EOF

# close things off
exec 1>&2
#../move-if-change new-gdbarch.c gdbarch.c
compare_new gdbarch.c
