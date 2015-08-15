Tue Jan 23 15:49:47 1990  Jim Kingdon  (kingdon at pogo.ai.mit.edu)

	* dbxread.c (define_symbol): Deal with deftype 'X'.

	* convex-dep.c (wait): Make it pid_t.

	* convex-dep.c (comm_registers_info): accept decimal comm register
	specification, as "i comm 32768".

	* dbxread.c (process_one_symbol): Make VARIABLES_INSIDE_BLOCK
	macro say by itself where variables are.  Pass it desc.
	m-convex.h (VARIABLES_INSIDE_BLOCK): Nonzero for native compiler.

	* m-convex.h (SET_STACK_LIMIT_HUGE): Define.
	(IGNORE_SYMBOL): Take out #ifdef N_MONPT and put in 0xc4.

Fri Jan 19 20:04:15 1990  Jim Kingdon  (kingdon at albert.ai.mit.edu)

	* printcmd.c (print_frame_args): Always set highest_offset to
	current_offset when former is -1.

	* dbxread.c (read_struct_type): Print nice error message
	when encountering multiple inheritance.

Thu Jan 18 13:43:30 1990  Jim Kingdon  (kingdon at mole.ai.mit.edu)

	* dbxread.c (read_dbx_symtab): Always treat N_FN as a potential
	source for a x.o or -lx symbol, ignoring OFILE_FN_FLAGGED.

	* printcmd.c (print_frame_args): Cast -1 to (CORE_ADDR).

	* hp300bsd-dep.c (_initialize_hp300_dep): Get kernel_u_addr.
	m-hp300bsd.h (KERNEL_U_ADDR): Use kernel_u_addr.

	* infcmd.c (run_command): #if 0 out call to
	breakpoint_clear_ignore_counts.

Thu Jan 11 12:58:12 1990  Jim Kingdon  (kingdon at mole)

	* printcmd.c (print_frame_args) [STRUCT_ARG_SYM_GARBAGE]:
	Try looking up name of var before giving up & printing '?'.

Wed Jan 10 14:00:14 1990  Jim Kingdon  (kingdon at pogo)

	* many files: Move stdio.h before param.h.

	* sun3-dep.c (store_inferior_registers): Only try to write FP
	regs #ifdef FP0_REGNUM.

Mon Jan  8 17:56:15 1990  Jim Kingdon  (kingdon at pogo)

	* symtab.c: #if 0 out "info methods" code.

Sat Jan  6 12:33:04 1990  Jim Kingdon  (kingdon at pogo)

	* dbxread.c (read_struct_type): Set TYPE_NFN_FIELDS_TOTAL
	from all baseclasses; remove vestigial variable baseclass.

	* findvar.c (read_var_value): Check REG_STRUCT_HAS_ADDR.
	printcmd.c (print_frame_args):  Check STRUCT_ARG_SYM_GARBAGE.
	m-sparc.h: Define REG_STRUCT_HAS_ADDR and STRUCT_ARG_SYM_GARBAGE.

	* blockframe.c (get_frame_block): Subtract one from pc if not
	innermost frame.

Fri Dec 29 15:26:33 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* printcmd.c (print_frame_args): check highest_offset != -1, not i.

Thu Dec 28 16:21:02 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* valops.c (value_struct_elt): Clean up error msg.

	* breakpoint.c (describe_other_breakpoints):
	Delete extra space before "also set at" and add period at end.

Tue Dec 19 10:28:42 1989  Jim Kingdon  (kingdon at pogo)

	* source.c (print_source_lines): Tell user which line number
	was out of range when printing error message.

Sun Dec 17 14:14:09 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* blockframe.c (find_pc_partial_function): Use
	BLOCK_START (SYMBOL_BLOCK_VALUE (f)) instead of
	SYMBOL_VALUE (f) to get start of function.

	* dbxread.c: Make xxmalloc just a #define for xmalloc.

Thu Dec 14 16:13:16 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* m68k-opcode.h (fseq & following fp instructions):
	Change @ to $.

Fri Dec  8 19:06:44 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* breakpoint.c (breakpoint_clear_ignore_counts): New function.
	infcmd.c (run_command): Call it.

Wed Dec  6 15:03:38 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* valprint.c: Change it so "array-max 0" means there is
	no limit.

	* expread.y (yylex): Change error message "invalid token in
	expression" to "invalid character '%c' in expression".

Mon Dec  4 16:12:54 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* blockframe.c (find_pc_partial_function): Always return 1
	for success, 0 for failure, and set *NAME and *ADDRESS to
	match the return value.

	* dbxread.c (symbol_file_command): Use perror_with_name on
	error from stat.
	(psymtab_to_symtab, add_file_command),
	core.c (validate_files), source.c (find_source_lines),
	default-dep.c (exec_file_command): Check for errors from stat,
	fstat, and myread.

Fri Dec  1 05:16:42 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* valops.c (check_field): When following pointers, just get
	their types; don't call value_ind.
	
Thu Nov 30 14:45:29 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* config.gdb (pyr): New machine.
	core.c [REG_STACK_SEGMENT]: New code.
	dbxread.c (process_one_symbol): Cast return from copy_pending
	to long before casting to enum namespace.
	infrun.c: Split registers_info into DO_REGISTERS_INFO
	and registers_info.
	m-pyr.h, pyr-{dep.c,opcode.h,pinsn.c}: New files.

	* hp300bsd-dep.c: Stay in sync with default-dep.c.

	* m-hp300bsd.h (IN_SIGTRAMP): Define.

Mon Nov 27 23:48:21 1989  Jim Kingdon  (kingdon at apple-gunkies.ai.mit.edu)

	* m-sparc.h (EXTRACT_RETURN_VALUE, STORE_RETURN_VALUE):
	Return floating point values in %f0.

Tue Nov 21 00:34:46 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* dbxread.c (read_type): #if 0 out code which skips to
	comma following x-ref.

Sat Nov 18 20:10:54 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* valprint.c (val_print): Undo changes of Nov 11 & 16.
	(print_string): Add parameter force_ellipses.
	(val_print): Pass force_ellipses true when we stop fetching string
	before we get to the end, else pass false.

Thu Nov 16 11:59:50 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* infrun.c (restore_inferior_status): Don't try to restore
	selected frame if the inferior no longer exists.

	* valprint.c (val_print): Rewrite string printing code not to
	call print_string.

	* Makefile.dist (clean): Remove xgdb and xgdb.o.

Tue Nov 14 12:41:47 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* Makefile.dist (XGDB, bindir, xbindir, install, all): New stuff.

Sat Nov 11 15:29:38 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* valprint.c (val_print): chars_to_get: New variable.

Thu Nov  9 12:31:47 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* main.c (main): Process "-help" as a switch that doesn't
	take an argument.

Wed Nov  8 13:07:02 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* Makefile.dist (gdb.tar.Z): Add "else true".

Tue Nov  7 12:25:14 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* infrun.c (restore_inferior_status): Don't dereference fid if NULL.

	* config.gdb (sun3, sun4): Accept "sun3" and "sun4".

Mon Nov  6 09:49:23 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* Makefile.dist (Makefile): Move comments after commands.

	* *-dep.c [READ_COFF_SYMTAB]: Pass optional header size to
	read_section_hdr().

	* inflow.c: Include <fcntl.h> regardless of USG.

	* coffread.c (read_section_hdr): Add optional_header_size.
	(symbol_file_command): Pass optional header size to
	read_section_hdr().
	(read_coff_symtab): Initialize filestring.

	* version.c: Change version to 3.4.xxx.

	* GDB 3.4 released.

Sun Nov  5 11:39:01 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* version.c: Change version to 3.4.

	* symtab.c (decode_line_1): Only skip past "struct" if it
	is there.

	* valops.c (value_ind), eval.c (evaluate_subexp, case UNOP_IND):
	Have "*" <int-valued-exp> return an int, not a LONGEST.

	* utils.c (fprintf_filtered): Pass arg{4,5,6} to sprintf.

	* printcmd.c (x_command): Use variable itself rather
	than treating it as a pointer only if it is a function.
	(See comment "this makes x/i main work").

	* coffread.c (symbol_file_command): Use error for
	"%s does not have a symbol-table.\n".

Wed Nov  1 19:56:18 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* dbxread.c [BELIEVE_PCC_PROMOTION_TYPE]: New code.
	m-sparc.h: Define BELIEVE_PCC_PROMOTION_TYPE.

Thu Oct 26 12:45:00 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* infrun.c: Include <sys/dir.h>.

	* dbxread.c (read_dbx_symtab, case N_LSYM, case 'T'):
	Check for enum types and put constants in psymtab.

Mon Oct 23 15:02:25 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* dbxread.c (define_symbol, read_dbx_symtab): Handle enum
	constants (e.g.	"b:c=e6,0").

Thu Oct 19 14:57:26 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* stack.c (frame_info): Use FRAME_ARGS_ADDRESS_CORRECT
	m-vax.h (FRAME_ARGS_ADDRESS_CORRECT): New macro.
	(FRAME_ARGS_ADDRESS): Restore old meaning.

	* frame.h (Frame_unknown): New macro.
	stack.c (frame_info): Check for Frame_unknown return from
	FRAME_ARGS_ADDRESS.
	m-vax.h (FRAME_ARGS_ADDRESS): Sometimes return Frame_unknown.

	* utils.c (fatal_dump_core): Add "internal error" to message.

	* infrun.c (IN_SIGTRAMP): New macro.
	(wait_for_inferior): Use IN_SIGTRAMP.
	m-vax.h (IN_SIGTRAMP): New macro.

Wed Oct 18 15:09:22 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* config.gdb, Makefile.dist: Shorten m-i386-sv32.h.

	* coffread.c (symbol_file_command): Pass 0 to select_source_symtab.

Tue Oct 17 12:24:41 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* i386-dep.c (i386_frame_num_args): Take function from m-i386.h
	file.  Check for pfi null.
	m-i386.h (FRAME_NUM_ARGS): Use i386_frame_num_args.

	* infrun.c (wait_for_inferior): set stop_func_name to 0
	before calling find_pc_partial_function.

Thu Oct 12 01:08:50 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* breakpoint.c (_initialize_breakpoint): Add "disa".

	* Makefile.dist: Add GLOBAL_CFLAGS and pass to readline.

	* config.gdb (various): "$machine =" -> "machine =".

Wed Oct 11 11:54:31 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* inflow.c (try_writing_regs): #if 0 out this function.

	* main.c (main): Add "-help" option.

	* dbxread.c (read_dbx_symtab): Merge code for N_FUN with
	N_STSYM, etc.

Mon Oct  9 14:21:55 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* inflow.c (try_writing_regs_command): Don't write past end
	of struct user.

	* dbxread.c (read_struct_type): #if 0 out code which checks for
	bitpos and bitsize 0.

	* config.gdb: Accept sequent-i386 (not seq386).
	(symmetry): Set depfile and paramfile.

	* m-convex.h (IGNORE_SYMBOL): Check for N_MONPT if defined.

Thu Oct  5 10:14:26 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* default-dep.c (read_inferior_memory): Put #if 0'd out comment
	within /* */.

Wed Oct  4 18:44:41 1989  Jim Kingdon  (kingdon at apple-gunkies.ai.mit.edu)

	* config.gdb: Change /dev/null to m-i386.h for various
	386 machine "opcodefile" entries.

	* config.gdb: Accept seq386 for sequent symmetry.

Mon Oct  2 09:59:50 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* hp300bsd-dep.c:  Fix copyright notice.

Sun Oct  1 16:25:30 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* Makefile.dist (DEPFILES): Add isi-dep.c.

	* default-dep.c (read_inferior_memory): Move #endif after else.

Sat Sep 30 12:50:16 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* version.c: Change version number to 3.3.xxx.

	* GDB 3.3 released.

	* version.c: Change version number to 3.3.

	* Makefile.dist (READLINE): Add vi_mode.c

	* config.gdb (i386): Change /dev/null to m-i386.h

	* config.gdb: Add ';;' before 'esac'.

	* Makefile.dist (gdb.tar.Z): Move comment above dependency.

	* dbxread.c (read_ofile_symtab): Check symbol before start
	of source file for GCC_COMPILED_FLAG_SYMBOL.
	(start_symtab): Don't clear processing_gcc_compilation.

Thu Sep 28 22:30:23 1989  Roland McGrath  (roland at hobbes.ai.mit.edu)

	* valprint.c (print_string): If LENGTH is zero, print "".

Wed Sep 27 10:15:10 1989  Jim Kingdon  (kingdon at apple-gunkies.ai.mit.edu)

	* config.gdb: "rm tmp.c" -> "rm -f tmp.c".

Tue Sep 26 13:02:10 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* utils.c (_initialize_utils): Use termcap to set lines_per_page
	and chars_per_line.

Mon Sep 25 10:06:43 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* dbxread.c (read_dbx_symtab, N_SOL): Do not add the same file
	more than once.

Thu Sep 21 12:43:18 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* infcmd.c (unset_environment_command): Delete all variables
	if called with no arg.

	* remote.c, inferior.h (remote_{read,write}_inferior_memory):
	New functions.
	core.c ({read,write}_memory): Use remote_{read,write}_inferior_memory.

	* valops.c (call_function): When reserving stack space for
	arguments, call value_arg_coerce.

	* m-hp9k320.h: define BROKEN_LARGE_ALLOCA.

	* breakpoint.c (delete_command): Ask for confirmation only
	when there are breakpoints.

	* dbxread.c (read_struct_type): If lookup_basetype_type has
	copied a stub type, call add_undefined_type.

	* sparc_pinsn.c (compare_opcodes): Check for "1+i" anywhere
	in args.

	* val_print.c (type_print_base): Print stub types as
	"<incomplete type>".

Wed Sep 20 07:32:00 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* sparc-opcode.h (swapa): Remove i bit from match.
	(all alternate space instructions): Delete surplus "foo rs1+0"
	patterns.

	* Makefile.dist (LDFLAGS): Set to $(CFLAGS).

	* remote-multi.shar (remote_utils.c, putpkt): Change csum to unsigned.

Tue Sep 19 14:15:16 1989  Jim Kingdon  (kingdon at apple-gunkies.ai.mit.edu)

	* sparc-opcode.h: Set i bit in lose for many instructions which
	aren't immediate.

	* stack.c (print_frame_info): add "func = 0".

Mon Sep 18 16:19:48 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* sparc-opcode.h (mov): Add mov to/from %tbr, %psr, %wim.

	* sparc-opcode.h (rett): Fix notation to use suggested assembler
	syntax from architecture manual.

	* symmetry-dep.c (I386_REGNO_TO_SYMMETRY): New macro.
	(i386_frame_find_saved_regs): Use I386_REGNO_TO_SYMMETRY.

Sat Sep 16 22:21:17 1989  Jim Kingdon  (kingdon at spiff)

	* remote.c (remote_close): Set remote_desc to -1.

	* gdb.texinfo (Output): Fix description of echo to match
	reality and ANSI C.

Fri Sep 15 14:28:59 1989  Jim Kingdon  (kingdon at apple-gunkies.ai.mit.edu)

	* symtab.c (lookup_symbol): Add comment about "asm".

	* sparc-pinsn.c: Use NUMOPCODES.

	* sparc-opcode.h (NUMOPCODES): Use sparc_opcodes[0] not *sparc_opcodes.

Thu Sep 14 15:25:20 1989  Jim Kingdon  (kingdon at apple-gunkies.ai.mit.edu)

	* dbxread.c (xxmalloc): Print error message before calling abort().

	* infrun.c (wait_for_inferior): Check for {stop,prev}_func_name
	null before passing to strcmp.

Wed Sep 13 12:34:15 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* sparc-opcode.h: New field delayed.
	sparc-pinsn.c (is_delayed_branch): New function.
	(print_insn): Check for delayed branches.

	* stack.c (print_frame_info): Use misc_function_vector in
	case where ar truncates file names.

Tue Sep 12 00:16:14 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* convex-dep.c (psw_info): Move "struct pswbit *p" with declarations.

Mon Sep 11 14:59:57 1989  Jim Kingdon  (kingdon at spiff)

	* convex-dep.c (core_file_command): Delete redundant printing
	of "Program %s".

	* m-convex.h (ENTRY_POINT): New macro.

	* m-convex.h (FRAME_CHAIN_VALID): Change outside_first_object_file
	to outside_startup_file

	* main.c: #if 0 out catch_termination and related code.

	* command.c (lookup_cmd_1): Consider underscores part of
	command names.

Sun Sep 10 09:20:12 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* printcmd.c: Change asdump_command to disassemble_command
	(_initialize_printcmd): Change asdump to diassemble.

	* main.c (main): Exit with code 0 if we hit the end of a batch
	file.

	* Makefile.dist (libreadline.a): Fix syntax of "CC=${CC}".

Sat Sep  9 01:07:18 1989  Jim Kingdon  (kingdon at apple-gunkies.ai.mit.edu)

	* values.c (history_info): Renamed to value_history_info.
	Command renamed to "info value" (with "info history" still
	accepted).

	* sparc-pinsn.c (print_insn): Extend symbolic address printing
	to cover "sethi" following by an insn which uses 1+i.

Fri Sep  8 14:24:01 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* m-hp9k320.h, m-hp300bsd.h, m-altos.h, m-sparc.h, m-sun3.h
	(READ_GDB_SYMSEGS): Remove.
	dbxread.c [READ_GDB_SYMSEGS]: Remove code to read symsegs.

	* sparc-pinsn.c (print_insn): Detect "sethi-or" pairs and
	print symbolic address.

	* sparc-opcode.h (sethi, set): Change lose from 0xc0000000 to
	0xc0c00000000. 

	* remote.c (remote_desc): Initialize to -1.

	* Makefile.dist (libreadline.a): Pass CC='${CC}' to readline makefile.

Thu Sep  7 00:07:17 1989  Jim Kingdon  (kingdon at apple-gunkies.ai.mit.edu)

	* dbxread.c (read_struct_type): Check for static member functions.
	values.c, eval.c, valarith.c, valprint.c, valops.c: Merge changes
	from Tiemann for static member functions.

	* sparc-opcode.h (tst): Fix all 3 patterns.

	* Makefile.dist (gdb1): New rule.

	* sparc-opcode.h: Change comment about what the disassembler
	does with the order of the opcodes.

	* sparc-pinsn.c (compare_opcodes): Put 1+i before i+1.
	Also fix mistaken comment about preserving order of original table.

	* sparc-opcode.h (clr, mov): Fix incorrect lose entries.

	* m-symmetry.h (FRAME_NUM_ARGS): Add check to deal with code that
	GCC sometimes generates.

	* config.gdb: Change all occurances of "skip" to "/dev/null".

	* README (about languages other than C): Update comments about
	Pascal and FORTRAN.

	* sparc-opcode.h (nop): Change lose from 0xae3fffff to 0xfe3fffff.

	* values.c (value_virtual_fn_field): #if 0-out assignment to
	VALUE_TYPE(vtbl).

Wed Sep  6 12:19:22 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* utils.c (fatal_dump_core): New function.
	Makefile.dist (MALLOC_FLAGS): use -Dbotch=fatal_dump_core

Tue Sep  5 15:47:18 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* breakpoint.c (enable_command): With no arg, enable all bkpts.

	* Makefile.dist (Makefile): Remove \"'s around $(MD).

	* Makefile.dist: In "cd readline; make . . ." change first
	SYSV_DEFINE to SYSV.

	* m68k-pinsn.c (_initialize_pinsn): Use alternate assembler
	syntax #ifdef HPUX_ASM

Sat Sep  2 23:24:43 1989  Jim Kingdon  (kingdon at apple-gunkies.ai.mit.edu)

	* values.c (history_info): Don't check num_exp[0] if num_exp
	is nil (just like recent editing_info change).

Fri Sep  1 19:19:01 1989  Jim Kingdon  (kingdon at apple-gunkies.ai.mit.edu)

	* gdb.texinfo (inc-history, inc-readline): Copy in the inc-* files
	because people might not have makeinfo.

	* README (xgdb): Strengthen nasty comments.

	* gdb.texinfo: Change @setfilename to "gdb.info".

Thu Aug 31 17:23:50 1989  Jim Kingdon  (kingdon at apple-gunkies.ai.mit.edu)

	* main.c (editing_info): Don't check arg[0] if arg is null.

	* m-vax.h: Add comment about known sigtramp bug.

	* sun3-dep.c, sparc-dep.c (IS_OBJECT_FILE, exec_file_command):
	Get right text & data addresses for .o files.

Wed Aug 30 13:54:19 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* utils.c (tilde_expand): Remove function (it's in readline).

	* sparc-opcode.h (call): Change "8" to "9" in first two
	patterns (%g7->%o7).

Tue Aug 29 16:44:41 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* printcmd.c (whatis_command): Change 4th arg to type_print
	from 1 to -1.

Mon Aug 28 12:22:41 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* dbxread.c (psymtab_to_symtab_1): In "and %s ..." change
	pst->filename to pst->dependencies[i]->filename.

	* blockframe.c (FRAMELESS_LOOK_FOR_PROLOGUE): New macro
	made from FRAMELESS_FUNCTION_INVOCATION from m-sun3.h except
	that it checks for zero return from get_pc_function_start.
	m-hp9k320.h, m-hp300bsd.h, m-i386.h, m-isi.h, m-altos.h,
	m-news.h, m-sparc.h, m-sun2.h, m-sun3.h, m-symmetry.h
	(FRAMELESS_FUNCTION_INVOCATION): Use FRAMELESS_LOOK_FOR_PROLOGUE.

	* dbxread.c (read_struct_type): Give warning and ignore field
	if bitpos and bitsize are zero.

Sun Aug 27 04:55:20 1989  Jim Kingdon  (kingdon at apple-gunkies.ai.mit.edu)

	* dbxread.c (psymtab_to_symtab{,_1}): Print message about
	reading in symbols before reading stringtab, not after.

Sat Aug 26 02:01:53 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* dbxread.c (IS_OBJECT_FILE, ADDR_OF_TEXT_SEGMENT): New macros.
	(read_dbx_symtab): Use text_addr & text_size to set end_of_text_addr.
	(symbol_file_command): pass text_addr & text_size to read_dbx_symtab.

Fri Aug 25 23:08:13 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* valprint.c (value_print): Try to give the name of function
	pointed to when printing a function pointer.

Thu Aug 24 23:18:40 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* core.c (xfer_core_file): In cases where MEMADDR is above the
	largest address that makes sense, set i to len.

Thu Aug 24 16:04:17 1989  Roland McGrath  (roland at hobbes.ai.mit.edu)

	* valprint.c (print_string): New function to print a character
	string, doing array-max limiting and repeat count processing.
	(val_print, value_print): Use print_string.
	(REPEAT_COUNT_THRESHOLD): New #define, the max number of elts to print
	without using a repeat count.  Set to ten.
	(value_print, val_print): Use REPEAT_COUNT_THRESHOLD.

	* utils.c (printchar): Use {fputs,fprintf}_filtered.

	* valprint.c (val_print): Pass the repeat count arg to the
	fprintf_filtered call for "<repeats N times>" messages.

Wed Aug 23 22:53:47 1989  Roland McGrath  (roland at hobbes.ai.mit.edu)

	* utils.c: Include <pwd.h>.

	* main.c: Declare free.

Wed Aug 23 05:05:59 1989  Jim Kingdon  (kingdon at apple-gunkies.ai.mit.edu)

	* utils.c, defs.h: Add tilde_expand.
	source.c (directory_command),
	main.c (cd_command),
	main.c (set_history_filename),
	dbxread.c (symbol_file_command),
	coffread.c (symbol_file_command),
	dbxread.c (add_file_command),
	symmisc.c (print_symtabs),
	*-dep.c (exec_file_command, core_file_command),
	main.c (source_command): Use tilde_expand.

	* dbxread.c (read_type): When we get a cross-reference, resolve
	it immediately if possible, only calling add_undefined_type if
	necessary.

	* gdb.texinfo: Uncomment @includes and put comment at start
	of file telling people to use makeinfo.

	* valprint.c (type_print_base): Print the right thing for
	bitfields.

	* config.gdb (sun3os3): Set paramfile and depfile.

Tue Aug 22 05:38:36 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* dbxread.c (symbol_file_command):  Pass string table size to
	read_dbx_symtab().
	(read_dbx_symtab): Before indexing into string table, check
	string table index for reasonableness.
	(psymtab_to_symtab{,_1}, read_ofile_symtab): Same.

Tue Aug 22 04:04:39 1989  Roland McGrath  (roland at hobbes.ai.mit.edu)

	* m68k-pinsn.c: Replaced many calls to fprintf and fputs with
	calls to fprintf_filtered and fputs_filtered.
	(print_insn_arg): Use normal MIT 68k syntax for postincrement,
	predecrement, and register indirect addressing modes.

Mon Aug 21 10:08:02 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* main.c (initialize_signals): Set signal handler for SIGQUIT
	and SIGHUP to do_nothing.

	* ns32k-opcode.h (ord): Change 1D1D to 1D2D.

	* ns32k-pinsn.c (print_insn_arg, print_insn): Handle index
	bytes correctly.

	* ns32k-opcode.h: Add comments.

	* dbxread.c (read_type): Put enum fields in type.fields in order
	that they were found in the debugging symbols (not reverse order).

Sun Aug 20 21:17:13 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* main.c (source_command): Read .gdbinit if run without argument.

	* source.c (directory_command): Only print "foo already in path"
	if from_tty.

	* version.c: Change version number to 3.2.xxx

Sat Aug 19 00:24:08 1989  Jim Kingdon  (kingdon at apple-gunkies.ai.mit.edu)

	* m-news.h: Define HAVE_WAIT_STRUCT.

	* m-isi.h, isi-dep.c: Replace with new version from Adam de Boor.
	config.gdb: Remove isibsd43.

	* main.c (catch_termination): Don't say we have written
	.gdb_history until after we really have.

	* convex-dep.c (attach): Add "sleep (1)".
	(write_vector_register): Use "LL" with long long constant.
	(wait): Close comment.
	(wait): Change "unix 7.1 bug" to "unix 7.1 feature" & related
	changes in comment.
	(scan_stack): And fp with 0x80000000 in while loop test.
	(core_file_command): Move code to set COREFILE.
	(many places): Change printf to printf_filtered.
	(psw_info): Allow argument giving value to print as a psw.
	(_initialize_convex_dep): Update docstrings.

	* m-convex.h (WORDS_BIG_ENDIAN): Correct typo ("WRODS")
	define NO_SIGINTERRUPT.
	define SET_STACK_LIMIT_HUGE.
	add "undef BUILTIN_TYPE_LONGEST" before defining it.
	Use "LL" after constants in CALL_DUMMY.

	* dbxread.c: In the 3 places it says error "ridiculous string
	table size"... delete extra parameter to error.

	* dbxread.c (scan_file_globals): Check for FORTRAN common block.
	Allow multiple references for the sake of common blocks.

	* main.c (initialize_main): Set history_filename to include
	current directory.

	* valprint.c (decode_format): Don't return a defaulted size
	field if osize is zero.

	* gdb.texinfo (Compilation): Update information on -gg symbols.
	Document problem with ar.

Fri Aug 18 19:45:20 1989  Jim Kingdon  (kingdon at apple-gunkies.ai.mit.edu)

	* valprint.c (val_print, value_print): Add "<repeats %d times>" code.
	Also put "..." outside quotes for strings.

	* main.c (initialize_main): Add comment about history output file
	being different from history input file.

	* m-newsos3.h: Undefine NO_SIGINTERRUPT.  Rearrange a few comments.

	* m-newsos3.h (REGISTER_U_ADDR): Use new version from Hikichi.

	* sparc-opcode.h: Add comment clarifying meaning of the order of
	the entries in sparc_opcodes.

	* eval.c (evaluate_subexp, case UNOP_IND): Deal with deferencing
	things that are not pointers.

	* valops.c (value_ind): Make dereferencing an int give a LONGEST.

	* expprint.c (print_subexp): Add (int) cast in OP_LAST case.

	* dbxread.c (read_array_type): Set lower and upper if adjustable.

	* symtab.c (lookup_symbol): Don't abort if symbol found in psymtab
	but not in symtab.

Thu Aug 17 15:51:20 1989  Randy Smith  (randy at hobbes.ai.mit.edu)

	* config.gdb: Changed "Makefile.c" to "Makefile.dist".

Thu Aug 17 01:58:04 1989  Roland McGrath  (roland at apple-gunkies.ai.mit.edu)

	* sparc-opcode.h (or): Removed incorrect lose bit 0x08000000.
	[many]: Changed many `lose' entries to have the 0x10 bit set, so
	they don't think %l0 is %g0.

Wed Aug 16 00:30:44 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* m-symmetry.h (STORE_STRUCT_RETURN): Also write reg 0.
	(EXTRACT_RETURN_VALUE): Call symmetry_extract_return_value.
	symmetry-dep.c (symmetry_extract_return_value): New fn.

	* main.c (symbol_completion_function): Deal with changed
	result_list from lookup_cmd_1 for ambiguous return.
	command.c (lookup_cmd): Same.

	* inflow.c [TIOCGETC]: Move #include "param.h" back before
	system #includes.  Change all #ifdef TIOCGETC to
	#if defined(TIOCGETC) && !defined(TIOCGETC_BROKEN)
	m-i386-sysv3.2.h, m-i386gas-sysv3.2.h: Remove "#undef TIOCGETC"
	and add "#define TIOCGETC_BROKEN".

	* command.c (lookup_cmd_1): Give the correct result_list in the
	case of an ambiguous return where there is a partial match
	(e.g. "info a").  Add comment clarifying what is the correct
	result_list.

	* gdb.texinfo (GDB History): Document the two changes below.

	* main.c (command_line_input): Make history expansion not
	just occur at the beginning of a line.

	* main.c (initialize_main): Make history expansion off by default.

	* inflow.c: Move #include "param.h" after system #includes.

	* i386-dep.c (i386_float_info): Use U_FPSTATE macro.

	* m-i386-sysv3.2.h, m-i386gas-sysv3.2.h: New files.
	Makefile.dist, config.gdb: Know about these new files.

Tue Aug 15 21:36:11 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* symtab.c (lookup_struct_elt_type): Use type_print rather
	than assuming type has a name.

Tue Aug 15 02:25:43 1989  Roland McGrath  (roland at apple-gunkies.ai.mit.edu)

	* sparc-opcode.h (mov): Removed bogus "or i,0,d" pattern.

	* sparc-opcode.h (mov, or): Fixed incorrect `lose' members.

	* sparc-dep.c: Don't include "sparc-opcode.h".
	(skip_prologue, isanulled): Declare special types to recognize
	instructions, and use them.

	* sparc-pinsn.c (print_insn): Sign-extend 13-bit immediate args.
	If they are less than +9, print them in signed decimal instead
	of unsigned hex.

	* sparc-opcode.h, sparc-pinsn.c: Completely rewritten to share an
	opcode table with gas, and thus produce disassembly that looks
	like what the assembler accepts.

Tue Aug 15 16:20:52 1989  Jim Kingdon  (kingdon at apple-gunkies.ai.mit.edu)

	* symtab.c (find_pc_psymbol): Move best_pc=psymtab->textlow-1
	after test for psymtab null.

	* main.c (editing_info): Remove variable retval.

	* config.gdb (sun3, isi): Comment out obsolete message about telling
	it whether you have an FPU (now that it detects it).

	* config.gdb (sun3): Accept sun3os3.

	* m68k-insn.h: Include <signal.h>.

	* m68k-pinsn.h (convert_{to,from}_68881): Add have_fpu code

	* m-newsos3.h: Undefine USE_PCB.  That code didn't seem to work.

	* sparc-dep.c: Put in insn_fmt and other stuff from the old
	sparc-opcode.h.

	* sparc-opcode.h, sparc-pinsn.c: Correct copyright notice.

	* sparc-opcode.h, sparc-pinsn.c: Replace the old ones with the new
	ones by roland.

Tue Aug 15 02:25:43 1989  Roland McGrath  (roland at apple-gunkies.ai.mit.edu)

	* Makefile.dist: Don't define CC at all.

	* Makefile.dist (Makefile): Remove tmp.c after preprocessing.
	Use $(MD) instead of M_MAKEDEFINE in the cc command.

	* Makefile.dist: Don't define RL_LIB as
	"${READLINE}/libreadline.a", since READLINE is a list of files.

Mon Aug 14 23:49:29 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* main.c (print_version): Change 1988 to 1989.

	* main.c (copying_info, initialize_main): Remove #if 0'd code.

Tue Aug  1 14:44:56 1989  Hikichi  (hikichi at sran203)

	* m-newsos3.h
	    (NO_SIGINTERRUPT): have SIGINTERRUPT on NEWS os 3.

	* m-news.h(FRAME_FIND_SAVED_REGS): use the sun3's instead of old
	one.

Mon Aug 14 15:27:01 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* m-news.h, m-newsos3.h, news-dep.c: Merge additional changes
	by Hikichi (ChangeLog entries above).

	* Makefile.dist (READLINE): List readline files individually
	so we don't accidently get random files from the readline
	directory.

	* m-news.h (STORE_RETURN_VALUE, EXTRACT_RETURN_VALUE):
	Expect floating point returns to be in fp0.

	* gdb.texinfo (Format options): New node.

	* gdb.texinfo: Comment out "@include"s until bfox fixes the
	readline & history docs.

	* dbxread.c (read_addl_syms): Set startup_file_* if necessary at
	the end (as well as when we hit ".o").

	* printcmd.c (decode_format): Set val.format & val.size to '?' at
	start and set defaults at end.

	* symtab.c (decode_line_1): Check for class_name null.

	* valops.c: Each place where it compares against field names,
	check for null field names.  (new t_field_name variables).

	* utils.c (fputs_filtered): Check for linebuffer null before
	checking whether to call fputs.  Remove later check for linebuffer
	null.  

Sun Aug 13 15:56:50 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* m-isi.h, m-sun3.h ({PUSH,POP}_FP_REGS):  New macros.
	m-sun3.h (NUM_REGS): Conditionalize on FPU.
	config.gdb (sun3, isi): Add message about support for machines
	without FPU.

	* main.c (catch_termination, initialize_signals): new functions.

	* main.c (editing_info): Add "info editing n" and "info editing +".
	Rewrite much of this function.
	gdb.texinfo (GDB Readline): Document it.

	* values.c (history_info): Add "info history +".  Also add code to
	do "info history +" when command is repeated.
	gdb.texinfo (Value History): Document "info history +".

	* expprint.c (print_subexp): Add OP_THIS to case stmt.

	* config.gdb (sun4os4): Put quotes around make define.

	* config.gdb: Canonicalize machine name at beginning.

Sat Aug 12 00:50:59 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* config.gdb: define M_MAKEDEFINE
	Makefile (Makefile, MD): Be able to re-make Makefile.

	* main.c (command_line_input): Add comments to
	the command history.

	* Makefile.dist (Makefile): Add /bin/false.

Fri Aug 11 14:35:33 1989  Jim Kingdon  (kingdon at spiff)

	* Makefile.dist: Comment out .c.o rule and add TARGET_ARCH.

	* m-altos.h: Include sys/page.h & sys/net.h

	* m-altos.h (FRAME_CHAIN{,_VALID}):  Use outside_startup_file.

	* config.gdb (altos, altosgas): Add M_SYSV & M_BSD_NM and remove
	M_ALLOCA=alloca.o from makedefine.

	* coffread.c (complete_symtab): Change a_entry to entry.

	* m-altosgas.h: New file.

	* m-symmetry (REGISTER_BYTE): Fix dumb mistake.

Fri Aug 11 06:39:49 1989  Roland McGrath  (roland at hobbes.ai.mit.edu)

	* utils.c (set_screensize_command): Check for ARG being nil, since
	that's what execute_command will pass if there's no argument.

	* expread.y (yylex): Recognize "0x" or "0X" as the beginning of a
	number.

Thu Aug 10 15:43:12 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* config.gdb, Makefile.dist: Rename Makefile.c to Makefile.dist.

	* m-altos.h: Add comment about porting to USGR2.

	* config.gdb (sparc): Add -Usparc.

Wed Aug  9 14:20:39 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* m-sun3os4.h: Define BROKEN_LARGE_ALLOCA.

	* values.c (modify_field): Check for value too large to fit in
	bitfield. 

	* utils.c (fputs_filtered): Allow LINEBUFFER to be NULL.

	* breakpoint.c (condition_command): Check for attempt to specify
	non-numeric breakpoint number.

	* config.gdb, Makefile, m-altos.h, altos-dep.c: Merge Altos
	port.

	* README: Change message about editing Makefile.

	* config.gdb: Edit Makefile.
	Copied Makefile to Makefile.c and changed to let config.gdb
	run us through the C preprocessor.

	* expread.y (yylex): Test correctly for definition of number.

Wed Aug  9 11:56:05 1989  Randy Smith  (randy at hobbes.ai.mit.edu)

	* dbxread.c (read_dbx_symtab): Put bracketing of entry point in
	test case for .o symbols so that it will be correct even without
	debugging symbols.
	(end_psymtab): Took bracketing out.

	* blockframe.c (outside_startup_file): Reverse the sense of the
	return value to make the functionality implied by the name
	correct. 

Tue Aug  8 11:48:38 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* coffread.c (symbol_file_command): Do  not assume presence of a.out
	header. 

	* blockframe.c: Replace first_object_file_end with
	startup_file_{start,end}
	(outside_startup_file): New function.
	dbxread.c (read_addl_syms, read_dbx_symtab, end_psymbol): set
	startup_file_*.  Delete first_object_file_end code.
	Add entry_point and ENTRY_POINT
	coffread.c (complete_symtab): Set startup_file_*.
	(first_object_file_end): Add as static.
	m-*.h (FRAME_CHAIN, FRAME_CHAIN_VALID): Call outside_startup_file
	instead of comparing with first_object_file_end.

	* breakpoint.c (breakpoint_1): Change -1 to (CORE_ADDR)-1.

	* config.gdb (i386, i386gas): Add missing quotes at end of "echo"

	* source.c (directory_command): Add dont_repeat ();

Mon Aug  7 18:03:51 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* dbxread.c (read_addl_syms): Change strcmp to strncmp and put 3rd
	arg back.

	* command.h (struct cmd_list_element): Add comment clarifying
	purpose of abbrev_flag.

Mon Aug  7 12:51:03 1989  Randy Smith  (randy at hobbes.ai.mit.edu)

	* printcmd.c (_initialize_printcmd): Changed "undisplay" not to
	have abbrev flag set; it isn't an abbreviation of "delete
	display", it's an alias.

Mon Aug  7 00:25:15 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* symtab.c (lookup_symtab_1): Remove filematch (never used).

	* expread.y [type]: Add second argument to 2 calls to
	lookup_member_type which were missing them.

	* dbxread.c (symbol_file_command): Add from_tty arg.
	Check it before calling query.

	* infcmd.c (tty_command): Add from_tty arg.

	* eval.c (evaluate_subexp): Remove 3rd argument from
	calls to value_x_unop.

	* dbxread.c (read_addl_syms): Remove 3rd argument from
	call to strcmp.

	* gdb.texinfo (Command editing): @include inc-readline.texinfo
	and inc-history.texinfo and reorganize GDB-specific stuff.

	* Makefile: Add line MAKE=make.

	* README (second paragraph): Fix trivial errors.

	* dbxread.c (read_struct_type): Make sure p is initialized.

	* main.c (symbol_completion_function): Complete correctly
	on the empty string.

Sun Aug  6 21:01:59 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* symmetry-dep.c: Remove "long" from definition of i386_follow_jump.

	* gdb.texinfo (Backtrace): Document "where" and "info stack".

	* dbxread.c (cleanup_undefined_types): Strip off "struct "
	or "union " from type names before doing comparison

Sat Aug  5 02:05:36 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* config.gdb (i386, i386gas): Improve makefile editing instructions.

	* Makefile: Fix typo in CLIBS for SYSV.

	* dbxread.c (read_dbx_symtab): Deal with N_GSYM typedefs.

	* dbxread.c (add_file_command): Do not free name.  We didn't
	allocate it; it just points into arg_string.

	* Makefile, m-*.h: Change LACK_VPRINTF to HAVE_VPRINTF.

Fri Jul 28 00:07:48 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* valprint.c (val_print): Made sure that all returns returned a
	value (usually 0, indicating no memory printed).

	* core.c (read_memory): Changed "return" to "return 0".

	* expread.y (parse_number): Handle scientific notation when the
	string does not contain a '.'.

Thu Jul 27 15:14:03 1989  Randy Smith  (randy at hobbes.ai.mit.edu)

	* infrun.c (signals_info): Error if signal number passed is out of
	bounds. 

	* defs.h: Define alloca to be __builtin_alloca if compiling with
	gcc and localized inclusion of alloca.h on the sparc with the
	other alloca stuff.
	* command.c: Doesn't need to include alloca.h on the sparc; defs.h
	does it for you.

	* printcmd.c (print_frame_args): Changed test for call to
	print_frame_nameless_args to check i to tell if any args had been
	printed.

Thu Jul 27 04:40:56 1989  Roland McGrath  (roland at hobbes.ai.mit.edu)

	* blockframe.c (find_pc_partial_function): Always check that NAME
	and/or ADDRESS are not nil before storing into them.

Wed Jul 26 23:41:21 1989  Roland McGrath  (roland at hobbes.ai.mit.edu)

	* m-newsos3.h: Define BROKEN_LARGE_ALLOCA.
	* dbxread.c (symbol_file_command, psymtab_to_symtab):
	Use xmalloc #ifdef BROKEN_LARGE_ALLOCA.

Tue Jul 25 16:28:18 1989  Jay Fenlason  (hack at apple-gunkies.ai.mit.edu)

	* m68k-opcode.h: moved some of the fmovem entries so they're
	all consecutive.  This way the assembler doesn't bomb.

Mon Jul 24 22:45:54 1989  Randy Smith  (randy at hobbes.ai.mit.edu)

	* symtab.c (lookup_symbol): Changed error to an informational (if
	not very comforting) message about internal problems.  This will
	get a null symbol returned to decode_line_1, which should force
	things to be looked up in the misc function vector.

Wed Jul 19 13:47:34 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* symtab.c (lookup_symbol): Changed "fatal" to "error" in
	external symbol not found in symtab in which it was supposed to be
	found.  This can be reached because of a bug in ar.

Tue Jul 18 22:57:43 1989  Randy Smith  (roland at hobbes.ai.mit.edu)

	* m-news.h [REGISTER_U_ADDR]: Decreased the assumed offset of fp0
	by 4 to bring it into (apparently) appropriate alignment with
	reality. 

Tue Jul 18 18:14:42 1989  Randy Smith  (randy at hobbes.ai.mit.edu)

	* Makefile: pinsn.o should depend on opcode.h

	* m68k-opcode.h: Moved fmovemx with register lists to before other
	fmovemx. 

Tue Jul 18 11:21:42 1989  Jim Kingdon  (kingdon at susie)

	* Makefile, m*.h: Only #define vprintf (to _doprnt or printf,
	depends on the system) if the library lacks it (controlled by
	LACK_VPRINTF_DEFINE in makefile).  Unpleasant, but necessary to
	make this work with the GNU C library.

Mon Jul 17 15:17:48 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* breakpoint.c (breakpoint_1): Change addr-b->address to
	b->address-addr.

Sun Jul 16 16:23:39 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* eval.c (evaluate_subexp): Change error message printed when
	right operand of '@' is not an integer to English.

	* infcmd.c (registers_info): Fix call to print_spaces_filtered
	to specify right # of arguments.

	* gdb.texinfo (Command Editing): Document info editing command.

	* coffread.c (read_file_hdr): Add MC68MAGIC.

	* source.c (select_source_symtab): Change MAX to max.

Fri Jul 14 21:19:11 1989  Jim Kingdon  (kingdon at apple-gunkies.ai.mit.edu)

	* infcmd.c (registers_info): Clean up display to look good with long
	register names, to say "register" instead of "reg", and to put the
	"relative to selected stack frame" bit at the top.

Fri Jul 14 18:23:09 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* dbxread.c (record_misc_function): Put parens around | to force
	correct evaluation.

Wed Jul 12 12:25:53 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* m-newsos3, m-news, infrun.c, Makefile, config.gdb, news-dep.c:
	Merge in Hikichi's changes for Sony/News-OS 3 support.

Tue Jul 11 21:41:32 1989  Jim Kingdon  (kingdon at apple-gunkies.ai.mit.edu)

	* utils.c (fputs_filtered): Don't do any filtering if output is
	not to stdout, or if stdout is not a tty.
	(fprintf_filtered): Rely on fputs_filtered's check for whether to
	do filtering.

Tue Jul 11 00:33:58 1989  Randy Smith  (randy at hobbes.ai.mit.edu)

	* GDB 3.2 Released.

	* valprint.h: Deleted.

	* utils.c (fputs_filtered): Don't do any filtering if filtering is
	disabled (lines_per_page == 0).

Mon Jul 10 22:27:53 1989  Randy Smith  (roland at hobbes.ai.mit.edu)

	* expread.y [typebase]: Added "unsigned long int" and "unsigned
	short int" to specs.

Mon Jul 10 21:44:55 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* main.c (main): Make -cd use cd_command to avoid
	current_directory with non-absolute pathname.

Mon Jul 10 00:34:29 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* dbxread.c (symbol_file_command): Catch errors from stat (even
	though they should never happen).

	* source.c (openp): If the path is null, use the current
	directory. 

	* dbxread.c (read_dbx_symtab): Put N_SETV symbols into the misc
	function vector ...
	(record_misc_function): ... as data symbols.

	* utils.c (fprintf_filtered): Return after printing if we aren't
	going to do filtering.

	* Makefile: Added several things for make clean to take care of.

	* expread.y: Lowered "@" in precedence below +,-,*,/,%.

	* eval.c (evaluate_subexp): Return an error if the rhs of "@"
	isn't integral.

	* Makefile: Added removal of core and gdb[0-9] files to clean
	target. 

	* Makefile: Made a new target "distclean", which cleans things up
	correctly for making a distribution.

Sun Jul  9 23:21:27 1989  Randy Smith  (randy at hobbes.ai.mit.edu)

	* dbxread.c: Surrounded define of gnu symbols with an #ifndef
	NO_GNU_STABS in case you don't want them on some machines.
	* m-npl.h, m-pn.h: Defined NO_GNU_STABS.

Sun Jul  9 19:25:22 1989  Jim Kingdon  (kingdon at apple-gunkies.ai.mit.edu)

	* utils.c (fputs_filtered): New function.
	(fprintf_filtered): Use fputs_filtered.
	utils.c (print_spaces_filtered),
	command.c (help_cmd,help_cmd_list),
	printcmd.c (print_frame_args),
	stack.c (print_block_frame_locals, print_frame_arg_vars),
	valprint.c (many functions): Use fputs_filtered instead of
	fprintf_filtered to avoid arbitrary limit.

	* utils.c (fprintf_filtered): Fix incorrect comment.

Sat Jul  8 18:12:01 1989  Randy Smith  (randy at hobbes.ai.mit.edu)

	* valprint.c (val_print): Changed assignment of pretty to use
	prettyprint as a conditional rather than rely on values of the
	enum. 

	* Projects: Cleaned up a little for release.

	* main.c (initialize_main): Initialize
	rl_completion_entry_function instead of completion_entry_function. 

	* Makefile: Modified to use the new readline library setup.

	* breakpoint.c (break_command_1, delete_breakpoint,
	enable_breakpoint, disable_breakpoint): Put in new printouts for
	xgdb usage triggered off of xgdb_verbose.
	* main.c (main): Added check for flag to set xgdb_verbose.
	* stack.c (frame_command): Set frame_changed when frame command
	used. 

Fri Jul  7 16:20:58 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* Remove valprint.h and move contents to value.h (more logical).

Fri Jul  7 02:28:06 1989  Randall Smith  (randy at rice-chex)

	* m68k-pinsn.c (print_insn): Included a check for register list;
	if there is one, make sure to start p after it.

	* breakpoint.c (break_command_1, delete_breakpoint,
	enable_breakpoint, disable_breakpoint): #ifdef'd out changes
	below; they produce unwanted output in gdb mode in gnu-emacs.

	* gdb.texinfo: Spelled.  Also removed index references from
	command editing section; the relevance/volume ratio was too low.
	Removed all references to the function index.

	* ns32k-opcode.h, ns32k-pinsn.c: Backed out changes of June 24th;
	haven't yet received legal papers.

	* .gdbinit: Included message telling the user what it is doing.

	* symmetry-dep.c: Added static decls for i386_get_frame_setup,
	i386_follow_jump.
	* values.c (unpack_double): Added a return (double)0 at the end to
	silence a compiler warning.

	* printcmd.c (containing_function_bounds, asdump_command): Created
	to dump the assembly code of a function (support for xgdb and a
	useful hack).
	(_initialize_printcmd): Added this to command list.
	* gdb.texinfo [Memory]: Added documentation for the asdump
	command.
	* breakpoint.c (break_command_1, delete_breakpoint,
	enable_breakpoint, disable_breakpoint): Added extra verbosity for
	xgdb conditionalized on the new external frame_full_file_name.
	* source.c (identify_source_line): Increase verbosity of fullname
	prointout to include pc value.
	* stack.c: Added a new variable; "frame_changed" to indicate when
	a frame has been changed so that gdb can print out a frame change
	message when the frame only changes implicitly.
	(print_frame_info): Check the new variable in determining when to
	print out a new message and set it to zero when done.
	(up_command): Increment it.
	(down_command): Decrement it.

	* m68k-pinsn.c (print_insn_arg [lL]): Modified cases for register
	lists to reset the point to point to after the word from which the
	list is grabbed *if* that would cause point to point farther than
	it currently is.

Thu Jul  6 14:28:11 1989  Jim Kingdon  (kingdon at apple-gunkies.ai.mit.edu)

	* valprint.c (val_print, value_print): Add parameter to control
	prettyprinting.
	valprint.h: New file containing constants used for passing
	prettyprinting parameter to val{,ue}_print.
	expprint.c, infcmd.c, printcmd.c, valprint.c, values.c:
	Change all calls to val{,ue}_print to use new parameter.
	
Mon Jul  3 22:38:11 1989  Randy Smith  (randy at apple-gunkies.ai.mit.edu)

	* dbxread.c (,process_one_symbol): Moved extern declaration for
	index out of function to beginning of file.

Mon Jul  3 18:40:14 1989  Jim Kingdon  (kingdon at hobbes.ai.mit.edu)

	* gdb.texinfo (Registers): Add "ps" to list of standard registers.

Sun Jul  2 23:13:03 1989  Jim Kingdon  (kingdon at apple-gunkies.ai.mit.edu)

	* printcmd.c (enable_display): Change d->next to d = d->next so
	that "enable display" without args works.

Fri Jun 30 23:42:04 1989  Jim Kingdon  (kingdon at apple-gunkies.ai.mit.edu)

	* source.c (list_command):  Made error message given when no
	symtab is loaded clearer.

	* valops.c (value_assign): Make it so that when assigning to an
	internal variable, the type of the assignment exp is the type of
	the value being assigned.

Fri Jun 30 12:12:43 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* main.c (verbose_info): Created.
	(initialize_main): Put "info verbose" into command list.

	* utils.c (screensize_info): Created.
	(_initialize_utils): Defined "info screensize" as a normal command.

	* valprint.c (format_info): Added information about maximum number
	of array elements to function.

	* blockframe.c (find_pc_partial_function): Again.

	* blockframe.c (find_pc_partial_function): Replaced a "shouldn't
	happen" (which does) with a zero return.

	* main.c (dont_repeat): Moved ahead of first use.

Thu Jun 29 19:15:08 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* vax-opcode.h: Made minor modifications (moved an instruction and
	removed a typo) to bring this into accord with gas' table; also
	changed copyright to reflect it being part of both gdb and gas.

	* m68k-opcode.h: Added whole scads and bunches of new stuff for
	the m68851 and changed the coptyrightto recognize that the file
	was shared between gdb and gas.

	* main.c (stop_sig): Use "dont_repeat ()" instead of *line = 0;

	* core.c (read_memory): Don't do anything if length is 0.

	* Makefile: Added readline.c to the list of files screwed by
	having the ansi ioctl.h compilation with gcc.

	* config.gdb: Added sun4os3 & sun4-os3 as availible options.

Wed Jun 28 02:01:26 1989  Jim Kingdon  (kingdon at apple-gunkies.ai.mit.edu)

	* command.c (lookup_cmd): Add ignore_help_classes argument.
	(lookup_cmd_1): Add ignore_help_classes argument.
	command.c, main.c: Change callers of lookup_cmd{,_1} to supply
        value for ignore_help_classes.

Tue Jun 27 18:01:31 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* utils.c (print_spaces_filtered): Made more efficient.
	* defs.h: Declaration.
	* valprint.c (val_print): Used in a couple of new places.

Mon Jun 26 18:27:28 1989  Randall Smith  (randy at gluteus.ai.mit.edu)

	* m68k-pinsn.c (print_insn_arg ['#', '^']): Combined them into one
	case which always gets the argument from the word immediately
	following the instruction.
	(print_insn_arg ["[lL]w"]): Make sure to always get the register
	mask from the word immediately following the instruction.

Sun Jun 25 19:14:56 1989  Randall Smith  (randy at galapas.ai.mit.edu)

	* Makefile: Added hp-include back in as something to distribute.

	* stack.c (print_block_frame_locals): Return value changed from
	void to int; return 1 if values printed.  Use _filtered.
	(print_frame_local_vars): Use return value from
	print_block_frame_locals to mention if nothing printed; mention
	lack of symbol table, use _filtered.
	(print_frame_arg_vars): Tell the user if no symbol table
	or no values printed.  Use fprintf_filtered instead of fprintf.
	* blockframe.c (get_prev_frame_info): Check for no inferior or
	core file before crashing.

	* inflow.c (inferior_died): Set current frame to zero to keep from
	looking like we're in start.

Sat Jun 24 15:50:53 1989  Randall Smith  (randy at gluteus.ai.mit.edu)

	* stack.c (frame_command): Added a check to make sure that there
	was an inferior or a core file.

	* expread.y (yylex): Allow floating point numbers of the form ".5"
	to be parsed.

        Changes by David Taylor at TMC:
	* ns32k-pinsn.c: Added define for ?floating point coprocessor? and
	tables for register names to be used for each of the possibilities.
	(list_search): Created; searches a list of options for a specific
	value.
	(print_insn_arg): Added 'Q', 'b', 'M', 'P', 'g', and 'G' options
	to the value location switch.
	* ns32k-opcode.h: Added several new location flags.
	[addr, enter, exit, ext[bwd], exts[bwd], lmr, lpr[bwd], restore,
	rett, spr[bwd], smr]: Improved insn format output.

	* symtab.c (list_symbols): Rearrange printing to produce readable
	output for "info types".

	* eval.c (evaluate_subexp_for_address): Fixed typo.

	* dbxread.c (read_type): Don't output an error message when
	there isn't a ',' after a cross-reference.

	* dbxread.c (read_dbx_symtab): #if'd out N_FN case in
	read_dbx_symtab if it has the EXT bit set (otherwise multiple
	cases with the same value).

Fri Jun 23 13:12:08 1989  Randall Smith  (randy at plantaris.ai.mit.edu)

	* symmisc.c: Changed decl of print_spaces from static to extern
	(since it's defined in utils.c).

	* remote.c (remote_open): Close remote_desc if it's already been
	opened. 

	* Remote_Makefile, remote_gutils.c, remote_inflow.c,
	remote_server.c, remote_utils.c: Combined into remote-multi.shar.
	* remote-multi.shar: Created (Vikram Koka's remote stub).
	* remote-sa.m68k.shar: Created (Glenn Engel's remcom.c).
	* README: Updated to reflect new organization of remote stubs.

	* dbxread.c (read_dbx_symtab): Put an N_FN in with N_FN | N_EXT to
	account for those machines which don't use the external bit here.
	Sigh. 

	* m-symmetry.h: Defined NO_SIGINTERRUPT.

Thu Jun 22 12:51:37 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* printcmd.c (decode_format): Make sure characters are printed
	using a byte size.

	* utils.c (error): Added a terminal_ours here.

	* stack.c (locals_info): Added check for selected frame.

	* dbxread.c (read_type): Checked to make sure that a "," was
	actually found in the symbol to end a cross reference.

Wed Jun 21 10:30:01 1989  Randy Smith  (randy at tartarus.uchicago.edu)

	* expread.y (parse_number, [exp]): Allowed for the return of a
	number marked as unsigned; this will allow inclusion of unsigned
	constants. 

	* symtab.h: Put in default definitions for BUILTIN_TYPE_LONGEST
	and BUILTIN_TYPE_UNSIGNED_LONGEST.

	* expread.y (parse_number): Will now accept integers suffixed with
	a 'u' (though does nothing special with it).

	* valarith.c (value_binop): Added cases to deal with unsigned
	arithmetic correctly.

Tue Jun 20 14:25:54 1989  Randy Smith  (randy at tartarus.uchicago.edu)

	* dbxread.c (psymtab_to_symtab_1): Changed reading in info message
	to go through printf_filtered.

	* symtab.c (list_symbols): Placed header message after all calls
	to psymtab_to_symtab. 

	* symtab.c (smash_to_{function, reference, pointer}_type): Carried
	attribute of permanence for the type being smashed over the bzero
	and allowed any type to point at this one if it is permanent.

	* symtab.c (smash_to_{function, reference, pointer}_type): Fix
	typo: check flags of to_type instead of type.

	* m-hp9k320.h: Changed check on __GNU__ predefine to __GNUC__.

	* Makefile: Made MUNCH_DEFINE seperate and based on SYSV_DEFINE;
	they aren't the same on hp's.

Mon Jun 19 17:10:16 1989  Randy Smith  (randy at tartarus.uchicago.edu)

	* Makefile: Fixed typo.

	* valops.c (call_function): Error if the inferior has not been
	started. 

	* ns32k-opcode.h [check[wc], cmpm[bwd], movm[bwd], skpsb]: Fixed
	typos. 

Fri Jun  9 16:23:04 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* m-news.h [NO_SIGINTERRUPT]: Defined.

	* dbxread.c (read_type): Start copy of undefined structure name
	past [sue] defining type of cross ref.

	* dbxread.c (process_one_symbol): Changed strchr to index.

	* ns32k-opcode.h, ns32k-pinsn.c: More changes to number of
	operands, addition of all of the set condition opcodes, addition
	of several flag letters, all patterned after the gas code.

	* ns32k-opcode.h [mov{su,us}[bwd], or[bwd]]: Changed number of
	operands from 1 to 2.

Wed Jun  7 15:04:24 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* symseg.h [TYPE_FLAG_STUB]: Created.
	* dbxread.c (read_type): Set flag bit if type is stub.
	(cleanup_undefined_types): Don't mark it as a stub if it's been
	defined since we first learned about it.
	* valprint.c (val_print): Print out a message to that effect if
	this type is encountered.

	* symseg.h, symtab.h: Moved the definition of TYPE_FLAG_PERM over
	to symseg.h so that all such definitions would be in the same place.

	* valprint.c (val_print): Print out <No data fields> for a
	structure if there aren't any.

	* dbxread.c (read_type): Set type name of a cross reference type
	to "struct whatever" or something.

Tue Jun  6 19:40:52 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* breakpoint.c (breakpoint_1): Print out symbolic location of
	breakpoints for which there are no debugging symbols.

Mon Jun  5 15:14:51 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* command.c (help_cmd_list): Made line_size static.

Sat Jun  3 17:33:45 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* Makefile: Don't include the binutils hp-include directory in the
	distribution anymore; refer the users to the binutils distribution.

Thu Jun  1 16:33:07 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* printcmd.c (disable_display_command): Fixed loop iteration for
	no arg case.

	* printcmd.c (disable_display_command): Added from_tty parameter
	to function.

	* valops.c (value_of_variable): Call read_var_value with 0 cast to
	FRAME instead of CORE_ADDR.

	* eval.c (evaluate_subexp): Corrected number of args passed to
	value_subscript (to 2).

	* infrun.c (wait_for_inferior), symtab.c (decode_line_1),
	m-convex.h: Changed name of FIRSTLINE_DEBUG_BROKEN to
	PROLOGUE_FIRSTLINE_OVERLAP. 

	* m-merlin.h: Fixed typo.
	* ns32k-opcode.h: Added ns32381 opcodes and "cinv" insn, and fixed
	errors in movm[wd], rett, and sfsr.

	* eval.c (evaluate_subexp, evaluate_subexp_for_address), valops.c
	(value_zero): Change value_zero over to taking two arguments
	instead of three.

	* eval.c (evaluate_subexp)
	  [OP_VAR_VALUE]: Get correct lval type for AVOID_SIDE_EFFECTS for
	  all types of symbols.
	  [BINOP_DIV]: Don't divide if avoiding side effects; just return
	  an object of the correct type.
	  [BINOP_REPEAT]: Don't call value_repeat, just allocate a
	  repeated value.
	(evaluete_subexp_for_address) [OP_VAR_VALUE]: Just return a thing
	of the right type (after checking to make sure that we are allowed
	to take the address of whatever variable has been passed).

Mon May 29 11:01:02 1989  Randall Smith  (randy at galapas.ai.mit.edu)

	* breakpoint.c (until_break_command): Set the breakpoint with a
	frame specification so that it won't trip in inferior calls to the
	function.  Also set things up so that it works based on selected
	frame, not current one.

Sun May 28 15:05:33 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* eval.c (evalue_subexp): Change subscript case to use value_zero
	in EVAL_AVOID_SIDE_EFFECTS case.

Fri May 26 12:03:56 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* dbxread.c (read_addl_syms, psymtab_to_symtab): Removed
	cleanup_undefined_types; this needs to be done on a symtab basis.
	(end_symtab): Called cleanup_undefined_types from here.
	(cleanup_undefined_types): No longer uses lookup_symbol (brain
	dead idea; oh, well), now it searches through file_symbols.

Wed May 24 15:52:43 1989  Randall Smith  (randy at galapas)

	* source.c (select_source_symtab): Only run through
	partial_symtab_list if it exists.

	* coffread.c (read_coff_symtab): Don't unrecord a misc function
	when a function symbol is seen for it.

	* expread.y [variable]: Make sure to write a type for memvals if
	you don't get a mft you recognize.

Tue May 23 12:15:57 1989  Randall Smith  (randy at plantaris.ai.mit.edu)

	* dbxread.c (read_ofile_symtab, psymtab_to_symtab): Moved cleanup
	of undefined types to psymtab_to_symtab.  That way it will be
	called once for all readins (which will, among other things,
	help reduce infinite loops).

	* symtab.h [misc_function_type]: Forced mf_unknown to 0.
	* dbxread.c (record_misc_function): Cast enum to unsigned char (to
	fit).
	* expread.y [variable]: Cast unsigned char back to enum to test.

Mon May 22 13:08:25 1989  Randall Smith  (randy at gluteus.ai.mit.edu)

        Patches by John Gilmore for dealing well with floating point:
	* findvar.c (value_from_register, locate_var_value): Used
	BYTES_BIG_ENDIAN instead of an inline test.
	* m-sparc.h [IEEE_FLOAT]: Created to indicate that the sparc is
	IEEE compatible.
	* printcmd.c (print_scalar_formatted): Use BYTES_BIG_ENDIAN and
	the stream argument for printing; also modify default type for
	'f'.  Change handling of invalid floats; changed call syntax for
	is_nan.
	(print_command): Don't print out anything indicating that
	something was recorded on the history list if it wasn't.
	* valprint.c (val_print): Fixed to deal properley with new format
	of is_nan and unpacking doubles without errors occuring.
	(is_nan): Changed argument list and how it figures big endianness
	(uses macros).
	* values.c (record_latest_value): Return -1 and don't record if
	it's an invalid float.
	(value_as_double): Changed to use new unpack_double calling
	convention.
	(unpack_double): Changed not to call error if the float was
	invalid; simply to set invp and return.  Changed calling syntax.
	(unpack_field_as_long, modify_field): Changed to use
	BITS_BIG_ENDIAN to determine correct action.

	* m-hp9k320.h [HP_OS_BUG]: Created; deals with problem where a
	trap happens after a continue.
	* infrun.c (wait_for_inferior): Used.

	* m-convex.h [FIRSTLINE_DEBUG_BROKEN]: Defined a flag to indicate
	that the debugging symbols output by the compiler for the first
	line of a function were broken.
	* infrun.c (wait_for_inferior), symtab.c (decode_line_1): Used.
	
	* gdb.texinfo [Data, Memory]: Minor cleanups of phrasing.

Fri May 19 00:16:59 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* dbxread.c (add_undefined_type, cleanup_undefined_types): Created
	to keep a list of cross references to as yet undefined types.
	(read_type): Call add_undefined_type when we run into such a case.
	(read_addl_syms, read_ofile_symtab): Call cleanup_undefined_types
	when we're done.

	* dbxread.c (psymtab_to_symtab, psymtab_to_symtab_1): Broke
	psymtab_to_symtab out into two routines; made sure the string
	table was only readin once and the globals were only scanned once,
	for any number of dependencies.

Thu May 18 19:59:18 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* m-*.h: Defined (or not, as appropriate per machine)
	BITS_BIG_ENDIAN, BYTES_BIG_ENDIAN, and WORDS_BIG_ENDIAN.

Wed May 17 13:37:45 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* main.c (symbol_completion_function): Always complete on result
	command list, even if exact match found.  If it's really an exact
	match, it'll find it again; if there's something longer than it,
	it'll get the right result.

	* symtab.c (make_symbol_completion_function): Fixed typo; strcmp
	==> strncmp.

	* dbxread.c (read_dbx_symtab): Change 'G' case to mark symbols as
	LOC_EXTERNAL. 

	* expread.y [variables]: Changed default type of text symbols to
	function returning int so that one can use, eg. strcmp.

	* infrun.c (wait_for_inferior): Include a special flag indicating
	that one shouldn't insert the breakpoints on the next step for
	returning from a sigtramp and forcing at least one move forward. 

	* infrun.c (wait_for_inferior): Change test for nexting into a
	function to check for current stack pointer inner than previous
	stack pointer.

	* infrun.c (wait_for_inferior): Check for step resume break
	address before dealing with normal breakpoints.

	* infrun.c (wait_for_inferior): Added a case to deal with taking
	and passing along a signal when single stepping past breakpoints
	before inserting breakpoints.

	* infrun.c (wait_for_inferior): Inserted special case to keep
	going after taking a signal we are supposed to be taking.  

Tue May 16 12:49:55 1989  Randall Smith  (randy at gluteus.ai.mit.edu)

	* inflow.c (terminal_ours_1): Cast result of signal to (int
	(*)()). 

	* gdb.texinfo: Made sure that references to the program were in
	upper case.  Modify description of the "set prompt" command.
	[Running]: Cleaned up introduction.
	[Attach]: Cleaned up.
	[Stepping]: Change "Proceed" to "Continue running" or "Execute".
	Minor cleanup.
	[Source Path]: Cleaned up intro.  Cleared up distinction between
	the executable search path and the source path.  Restated effect
	of the "directory" command with no arguments.
	[Data]: Fixed typos and trivial details.
	[Stepping]: Fixed up explanation of "until".

	* source.c (print_source_lines): Print through filter.

	* printcmd.c (x_command): If the format with which to print is
	"i", use the address of anything that isn't a pointer instead of
	the value.  This is for, eg. "x/10i main".

	* gdb.texinfo: Updated last modification date on manual.

Mon May 15 12:11:33 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* symtab.c (lookup_symtab): Fixed typo (name ==> copy) in call to
	lookup_symtab_1. 

	* gdb.texinfo: Added documentation for "break [+-]n" and for new
	actions of "directory" command (taking multiple directory names at
	the same time).

	* m68k-opcode.h: Replaced the version in gdb with an up-to-date
	version from the assembler directory.
	* m68k-pinsn.c (print_insn_arg): Added cases 'l' & 'L' to switch
	to print register lists for movem instructions.

	* dbxread.c, m-convex.h: Moved convex dependent include files over
	from dbxread.c to m-convex.h.

	* printcmd.c (disable_display, disable_display_command): Changed
	name of first to second, and created first which takes an int as
	arg rather than a char pointer.  Changed second to use first.
	(_initialize_printcmd): Changed to use second as command to call.
	(delete_current_display, disable_current_display): Changed name of
	first to second, and changed functionality to match.
	* infrun.c (normal_stop), main.c (return_to_top_level): Changed to 
	call disable_current_display.

	* dbxread.c (process_one_symbol, read_dbx_symtab): Changed N_FN to
	be N_FN | N_EXT to deal with new Berkeley define; this works with
	either the old or the new.

	* Remote_Makefile, remote_gutils.c, remote_inflow.c,
	remote_server.c, remote_utils.c: Created.
	* Makefile: Included in tag and tar files.
	* README: Included a note about them.

	* printcmd.c (print_address): Use find_pc_partial_function to
	remove need to readin symtabs for symbolic addresses.  

	* source.c (directory_command): Replaced function with new one
	that can accept lists of directories seperated by spaces or :'s.

	* inflow.c (new_tty): Replaced calls to dup2 with calls to dup.

Sun May 14 12:33:16 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* stack.c (args_info): Make sure that you have an inferior or core
	file before taking action.

	* ns32k-opcode.h [deiw, deid]: Fixed machine code values for these
	opcodes.

	* dbxread.c (scan_file_globals): Modified to use misc function
	vector instead of file itself.  Killed all arguments to the
	funciton; no longer needed.
	(psymtab_to_symtab): Changed call for above to reflect new (void)
	argument list.

	* dbxread.c (read_dbx_symtab, ): Moved HASH_OFFSET define out of
	read_dbx_symtab. 

	* expread.y [variable]: Changed default type of misc function in
	text space to be (void ()).

	* Makefile: Modified for proper number of s/r conflicts (order is
	confusing; the mod that necessitated this change was on May 12th,
	not today). 

	* expread.y (yylex): Added SIGNED, LONG, SHORT, and INT keywords.
	[typename]: Created.
	[typebase]: Added rules for LONG, LONG INT, SHORT, SHORT INT,
	SIGNED name, and UNSIGNED name (a good approximation of ansi
	standard). 	

	* Makefile: Included .c.o rule to avoid sun's make from throwing
	any curves at us.

	* blockframe.c: Included <obstack.h>

	* command.c (lookup_cmd): Clear out trailing whitespace.

	* command.c (lookup_cmd_1): Changed malloc to alloca.  
	
Fri May 12 12:13:12 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* printcmd.c (print_frame_args): Only print nameless args when you
	know how many args there are supposed to be and when you've
	printed fewer than them.  Don't print nameless args between
	printed args.

	* symtab.c (make_symbol_completion_function): Fixed typo (= ==>
	==). 

	* remote.c (remote_open): ifdef'd out siginterrupt call by #ifndef
	NO_SIGINTERRUPT.
	* m-umax.h: Defined NO_SIGINTERRUPT.

	* expread.y [ptype, array_mod, func_mod, direct_abs_decl,
	abs_decl]:  Added rules for parsing and creating arbitrarily
	strange types for casts and sizeofs.

	* symtab.c, symtab.h (create_array_type): Created.  Some minor
	misfeatures; see comments for details (main one being that you
	might end up creating two arrays when you only needed one).

Thu May 11 13:11:49 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* valops.c (value_zero): Add an argument for type of lval.
	* eval.c (evaluate_subexp_for_address): Take address properly in
	the avoid side affects case (ie. keep track of whether we have an
	lval in memory and we can take the address).
	(evaluate_subexp): Set the lval type of expressions created with
	value_zero properley.

	* valops.c, value.h (value_zero): Created--will return a value of
	any type with contents filled with zero.
	* symtab.c, symtab.h (lookup_struct_elt_type): Created.
	* eval.c (evaluate_subexp): Modified to not read memory when
	called with EVAL_AVOID_SIDE_EFFECTS.

	* Makefile: Moved dbxread.c ahead of coffread.c in the list of
	source files. 

Wed May 10 11:29:19 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* munch: Make sure that sysv version substitutes for the whole
	line. 

	* symtab.h: Created an enum misc_function_type to hold the type of
	the misc function being recorded.
	* dbxread.c (record_misc_function): Branched on dbx symbols to
	decide which type to assign to a misc function.
	* coffread.c (record_misc_function): Always assign type unknown.
	* expread.y [variable]: Now tests based on new values.

Tue May  9 13:03:54 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* symtab.c: Changed inclusion of <strings.h> (doesn't work on
	SYSV) to declaration of index.

	* Makefile: Changed last couple of READLINE_FLAGS SYSV_DEFINE

	* source.c ({forward, reverse}_search_command): Made a default
	search file similar to for the list command.

Mon May  8 18:07:51 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* printcmd.c (print_frame_args): If we don't know how many
	arguments there are to this function, don't print the nameless
	arguments.  We don't know enough to find them.

	* printcmd.c (print_frame_args): Call print_frame_nameless_args
	with proper arguments (start & end as offsets from addr).

	* dbxread.c (read_addl_syms): Removed cases to deal with global
	symbols; this should all be done in scan_global_symbols.

Sun May  7 11:36:23 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* Makefile: Added copying.awk to ${OTHERS}.

Fri May  5 16:49:01 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* valprint.c (type_print_varspec_prefix): Don't pass
	passed_a_pointer onto children.

	* valprint.c (type_print_varspec_suffix): Print "array of" with
	whatever the "of" is after tha array brackets.

	* valprint.c (type_print_varspec_{prefix,suffix}): Arrange to
	parenthesisze pointers to arrays as well as pointers to other
	objects. 

	* valprint.c (type_print_varspec_suffix): Make sure to print
	subscripts of multi-dimensional arrays in the right order.

	* infcmd.c (run_command): Fixed improper usages of variables
	within remote debugging branch.

	* Makefile: Added Convex.notes to the list of extra files to carry
	around. 

	* dbxread.c (symbol_file_command): Made use of alloca or malloc
	dependent on macro define.

Thu May  4 15:47:04 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* Makefile: Changed READLINE_FLAGS to SYSV_DEFINE and called munch
	with it also.
	* munch: Check first argument for -DSYSV and be looser about
	picking up init routines if you find it.

	* coffread.c: Made fclose be of type int.

	* breakpoint.c (_initialize_breakpoint): Put "unset" into class
	alias. 

Wed May  3 14:09:12 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* m-sparc.h [STACK_END_ADDR]: Parameterized off of
	machine/vmparam.h (as per John Gilmore's suggestion).

	* blockframe.c (get_prev_frame_info): Changed this function back
	to checking frameless invocation first before checking frame
	chain.  This means that a backtrace up from start will produce the
	wrong value, but that a backtrace from a frameless function called
	in main will show up correctly.

	* breakpoint.c (_initialize_breakpoint): Added entry in help for
	delete that indicates that unset is an alias for it.

	* main.c (symbol_completion_function): Modified recognition of
	being within a single command.

Tue May  2 15:13:45 1989  Randy Smith  (randy at gnu)

	* expread.y [variable]: Add some parens to get checking of the
	misc function vector right.

Mon May  1 13:07:03 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* default-dep.c (core_file_command): Made reg_offset unsigned.

	* default-dep.c (core_file_command): Improved error messages for
	reading in registers.

	* expread.y: Allowed a BLOCKNAME to be ok for a variable name (as
	per C syntax). 

	* dbxread.c (psymtab_to_symtab): Flushed stdout after printing
	starting message about reading in symbols.

	* printcmd.c (print_frame_args): Switched starting place for
	printing of frameless args to be sizeof int above last real arg
	printed. 

	* printcmd.c (print_frame_args): Modified final call to
	print_nameless_args to not use frame slots used array if none had
	been used. 

	* infrun.c (wait_for_inferior):  Take FUNCTION_START_OFFSET into
	account when dealing with comparison of pc values to function
	addresses. 

	* Makefile: Added note about compiling gdb on a Vax running 4.3.

Sun Apr 30 12:59:46 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* command.c (lookup_cmd): Got correct error message on bad
	command.  

	* m-sun3.h [ABOUT_TO_RETURN]: Modified to allow any of the return
	instructions, including trapv and return from interupt.

	* command.c (lookup_cmd): If a command is found, use it's values
	for error reporting and determination of needed subcommands.

	* command.c (lookup_cmd): Use null string for error if cmdtype is
	null; pass *line to error instead of **.

	* command.c (lookup_cmd_1): End of command marked by anything but
	alpha numeric or '-'.  Included ctype.h.

Fri Apr 28 18:30:49 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* source.c (select_source_symtab): Kept line number from ever
	being less than 1 in main decode.

Wed Apr 26 13:03:20 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* default-dep.c (core_file_command): Fixed typo.

	* utils.c (fprintf_filtered): Don't use return value from
	numchars. 

	* main.c, command.c (complete_on_cmdlist): Moved function to
	command.c. 

	* command.c (lookup_cmd): Modified to use my new routine.  Old
	version is still there, ifdef'd out.

	* command.c, command.h (lookup_cmd_1): Added a routine to do all
	of the work of lookup_cmd with no error reporting and full return
	of information garnered in search.

Tue Apr 25 12:37:54 1989  Randall Smith  (randy at gluteus.ai.mit.edu)

	* breakpoint.c (_initialize_breakpoint): Change "delete
	breakpionts" to be in class alias and not have the abbrev flag
	set. 

	* main.c (symbol_completion_function): Fix to correctly complete
	things that correspond to multiword aliases.

	* main.c (complete_on_cmdlist): Don't complete on something if it
	isn't a command or prefix (ie. if it's just a help topic).

	* main.c (symbol_completion_function): Set list index to be 0 if
	creating a list with just one element.

	* main.c (complete_on_cmdlist): Don't allow things with
	abbrev_flag set to be completion values.
	(symbol_completion_function): Don't accept an exact match if the
	abbrev flag is set.

	* dbxread.c (read_type): Fixed typo in comparision to check if
	type number existed.

	* dbxread.c (read_type): Made sure to only call dbx_lookup_type on
	typenums if typenums were not -1.

Mon Apr 24 17:52:12 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* symtab.c: Added strings.h as an include file.

Fri Apr 21 15:28:38 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* symtab.c (lookup_partial_symtab): Changed to only return a match
	if the name match is exact (which is what I want in all cases in
	which this is currently used.

Thu Apr 20 11:12:34 1989  Randall Smith  (randy at gluteus.ai.mit.edu)

	* m-isi.h [REGISTER_U_ADDR]: Installed new version from net.
	* default-dep.c: Deleted inclusion of fcntl.h; apparently not
	necessary.
	* Makefile: Added comment about compiling on isi under 4.3.

	* breakpoint.c (break_command_1): Only give decode_line_1 the
	default_breakpoint_defaults if there's nothing better (ie. make
	the default be off of the current_source notes if at all
	possible). 

	* blockframe.c (get_prev_frame_info): Clean up comments and
	delete code ifdefed out around FRAMELESS_FUNCTION_INVOCATION test. 

	* remote.c: Added a "?" message to protocol.
	(remote_open): Used at startup.
	(putpkt): Read whatever garbage comes over the line until we see a
	'+' (ie. don't treat garbage as a timeout).

	* valops.c (call_function): Eliminated no longer appropriate
	comment.

	* infrun.c (wait_for_inferior): Changed several convex conditional
	compilations to be conditional on CANNOT_EXECUTE_STACK.

Wed Apr 19 10:18:17 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* printcmd.c (print_frame_args): Added code to attempt to deal
	with arguments that are bigger than an int.

	Continuation of Convex/Fortran changes:
	* printcmd.c (print_scalar_formatted): Added leading zeros to
	printing of large integers.
	(address_info, print_frame_args): Added code to deal with
	LOC_REF_ARG.
	(print_nameless_args): Allow param file to specify a routine with
	which to print typeless integers.
	(printf_command): Deal with long long values well.
	* stack.c (print_frame_arg_vars): Change to deal with LOC_REF_ARG.
	* symmisc.c (print_symbol): Change to deal with LOC_REF_ARG.
	* symseg.h: Added LOC_REF_ARG to enum address_class.
	* symtab.c (lookup_block_symbol): Changed to deal with
	LOC_REF_ARG.
	* valarith.c (value_subscripted_rvalue): Created.
	(value_subscript): Used above when app.
	(value_less, value_equal): Change to cast to (char *) before doing
	comparison, for machines where that casting does something.
	* valops.c (call_function): Setup to deal with machines where you
	cannot execute code on the stack segment.
	* valprint.c (val_print): Make sure that array element size isn't
	zero before printing.  Set address of default array to address of
	first element.  Put in a couple of int cast.  Removed some convex
	specific code. Added check for endianness of machine in case of a
	packed structure.  Added code for printing typeless integers and
	for LONG LONG's.
	(set_maximum_command): Change to use parse_and_eval_address to get
	argument (so can use expressions there).
	* values.c (value_of_internalvar, set_internalvar_component,
	set_internalvar, convenience_info): Add in hooks for trapped
	internal vars.
	(unpack_long): Deal with LONG_LONG.
	(value_field): Remove LONGEST cast.
	(using_struct_return): Fixed typo ENUM ==> UNION.
	* xgdb.c (_initialize_xgdb): Make sure that specify_exec_file_hook
	is not called unless we are setting up a windowing environ.

Tue Apr 18 13:43:37 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	Various changes involved in 1) getting gdb to work on the convex,
	and 2) Getting gdb to work with fortran (due to convex!csmith):
	* convex-dep.c, convex-opcode.h, m-convex.h, convex-pinsn.c:
	Created (or replaced with new files).
	* Makefile: Add convex dependent files.  Changed default flags to
	gnu malloc to be CFLAGS.
	* config.gdb: Added convex to list of machines.
	* core.c (files_info): Added a FILES_INFO_HOOK to be used if
	defined.
	(xfer_core_file): Conditionalized compilation of xfer_core_file on
	the macro XFER_CORE_FILE.
	* coffread.c (record_misc_function): Made sure it zerod type field
	(which is now being used; see next).
	* dbxread.c: Included some convex dependent include files.
	(copy_pending, fix_common_blocks): Created.
	[STAB_REG_REGNUM, BELIEVE_PCC_PROMOTION]: Created default values;
	may be overridden in m-*.h.
	Included data structures for keeping track of common blocks.
	(dbx_alloc_type): Modified; if called with negative 1's will
	create a type without putting it into the type vector.
	(read_dbx_symtab, read_addl_syms): Modified calls to
	record_misc_function to include the new information.
	(symbol_file_command, psymtab_to_symtab, add_file_command):
	Modified reading in of string table to adapt to machines which
	*don't* store the size of the string table in the first four bytes
	of the string table.
	(read_dbx_symtab, scan_file_globals, read_ofile_symtab,
	read_addl_syms): Modified assignment of namestring to accept null
	index into symtab as ok.
	(read_addl_syms): Modified readin of a new object file to fiddle
	with common blocks correctly.
	(process_one_symbol): Fixed incorrect comment about convex.  Get
	symbols local to a lexical context from correct spot on a per
	machine basis.  Catch a bug in pcc which occaisionally puts an SO
	where there should be an SOL.  Seperate sections for N_BCOMM &
	N_ECOMM.
	(define_symbol): Ignore symbols with no ":".  Use
	STAB_REG_TO_REGNUM.  Added support for function args calling by
	reference.
	(read_type): Only read type number if one is there.  Remove old
	(#if 0'd out) array code.
	(read_array_type): Added code for dealing with adjustable (by
	parameter) arrays half-heartedly.
	(read_enum_type): Allow a ',' to end a list of values.
	(read_range_type): Added code to check for long long.
	* expread.y: Modified to use LONGEST instead of long where
	necessary.  Modified to use a default type of int for objects that
	weren't in text space.
	* findvar.c (locate_var_value, read_var_value): Modified to deal
	with args passed by reference.
	* inflow.c (create_inferior): Used CREATE_INFERIOR_HOOK if it
	exists.
	* infrun.c (attach_program): Run terminal inferior when attaching.
	(wait_for_inferior): Removed several convex dependencies.
	* main.c (float_handler): Created.
	Made whatever signal indicates a stop configurable (via macro
	STOP_SIGNAL).
	(main): Setup use of above as a signal handler.  Added check for
	"-nw" in args already processed.
	(command_line_input): SIGTSTP ==>STOP_SIGNAL.

	* expread.y: Added token BLOCKNAME to remove reduce/reduce
	conflict.
	* Makefile: Change message to reflect new grammar.

Mon Apr 17 13:24:59 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* printcmd.c (compare_ints): Created.
	(print_frame_args): Modified to always print arguments in the
	order in which they were found in the symbol table.  Figure out
	what apots are missing on the fly.

	* stack.c (up_command): Error if no inferior or core file.

	* m-i386.h, m-symmetry.h [FRAMELESS_FUNCTION_INVOCATION]: Created;
	same as m68k.

	* dbxread.c (define_symbol): Changed "desc==0" test to
	"processing_gcc_compilation", which is the correct way to do it.

Sat Apr 15 17:18:38 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* expread.y: Added precedence rules for arglists, ?:, and sizeof
	to eliminate some shift-reduce conflicts.
	* Makefile: Modified "Expect" message to conform to new results.

Thu Apr 13 12:29:26 1989  Randall Smith  (randy at plantaris.ai.mit.edu)

	* inflow.c (terminal_init_inferior): Fixed typo in recent diff
	installation; TIOGETC ==> TIOCGETC.

	* m-vax.h, m-sun2.h, m-sun3.h, m-sparc.h, m-hp*.h, m-isi.h,
	m-news.h [FRAMELESS_FUNCTION_INVOCATION]: Created macro with
	appropriate definition.

Wed Apr 12 15:30:29 1989  Randall Smith  (randy at plantaris.ai.mit.edu)

	* blockframe.c (get_prev_frame_info): Added in a macro to specify
	when a "frame" is called without a frame pointer being setup.

	* Makefile [clean]: Made sure to delete gnu malloc if it was being
	used. 

Mon Apr 10 12:43:49 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* dbxread.c (process_one_symbol): Reset within_function to 0 after
	last RBRAC of a function.

	* dbxread.c (read_struct_type): Changed check for filling in of
	TYPE_MAIN_VARIANT of type.

	* inflow.c (create_inferior): Conditionalized fork so that it
	would be used if USG was defined and HAVE_VFORK was not defined.

	* defs.h: Added comment about enum command_class element
	class_alias. 

	* dbxread.c (process_one_symbol): Fixed a typo with interesting
	implications for associative processing in the brain (':' ==> 'c').

	* sparc-dep.c (isabranch): Changed name to isannulled, modified to
	deal with coprocessor branches, and improved comment.
	(single_step): Changed to trap at npc + 4 instead of pc +8 on
	annulled branches.  Changed name in call to isabranch as above.

	* m-sun4os4.h (STACK_END_ADDRESS): Changed it to 0xf8000000 under
	os 4.0.

Sat Apr  8 17:04:07 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* dbxread.c (process_one_symbol): In the case N_FUN or N_FNAME the
	value being refered to is sometimes just a text segment variable.
	Catch this case.

	* infrun.c (wait_for_inferior), breakpoint.c
	(breakpoint_stop_status): Move the selection of the frame to
	inside breakpoint_stop_status so that the frame only gets selected
	(and the symbols potentially read in) if the symbols are needed.

	* symtab.c (find_pc_psymbol): Fixed minor misthough (pc >=
	fucntion start, not >).

	* breakpoint.c (_initialize_breakpoint): Change "delete" internal
	help entry to simply refer to it being a prefix command (since the
	list of subcommands is right there on a "help delete").

Fri Apr  7 15:22:18 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* blockframe.c (find_pc_partial_function): Created; figures out
	what function pc is in (name and address) without reading in any
	new symbols.
	* symtab.h: Added decl for above.
	* infrun.c (wait_for_inferior): Used instead of
	find_pc_function_start.
	* stack.c (print_frame_info): Used instead of hand coding for same
	thing. 

	* dbxread.c (psymtab_to_symtab): No longer patch readin pst's out
	of the partial_symtab_list; need them there for some checks.
	* blockframe.c (block_for_pc), source.c (select_source_symtab),
	symtab.c (lookup_symbol, find_pc_symtab, list_symbols): Made extra
	sure not to call psymtab_to_symtab with ->readin == 1, since these
	psymtab now stay on the list.
	* symtab.c (sources_info): Now distinguishes between psymtabs with
	readin set and those with it not set.

	* symtab.c (lookup_symtab): Added check through partial symtabs
	for name with .c appended.

	* source.c (select_source_symtab): Changed semantics a little so
	that the argument means something.
	* source.c (list_command), symtab.c (decode_line_1): Changed call
	to select_source_symtab to match new conventions.

	* dbxread.c (add_file_command): This command no longer selects a
	symbol table to list from.

	* infrun.c (wait_for_inferior): Only call find_pc_function (to
	find out if we have debugging symbols for a function and hence if
	we should step over or into it) if we are doing a "step".

Thu Apr  6 12:42:28 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* main.c (command_line_input): Added a local buffer and only
	copied information into the global main.c buffer when it is
	appropriate for it to be saved (and repeated).
	(dont_repeat): Only nail line when we are reading from stdin
	(otherwise null lines won't repeat and what's in line needs to be
	saved).
	(read_command_lines): Fixed typo; you don't what to repeat when
	reading command lines from the input stream unless it's standard
	input. 

        John Gilmore's (gnu@toad.com) mods for USG gdb:
	* inflow.c: Removed inclusion of sys/user.h; no longer necessary.
	(, terminal_init_inferior, terminal_inferior, terminal_ours_1,
	term_status_command, _initialize_inflow) Seperated out declaration
	and usage of terminal mode structures based on the existence of
	the individual ioctls.
	* utils.c (request_quit): Restore signal handler under USG.  If
	running under USG initialize sys_siglist at run time (too much
	variation between systems).

Wed Apr  5 13:47:24 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

        John Gilmore's (gnu@toad.com) mods for USG gdb:
	* default-dep.c: Moved include of sys/user.h to after include of
	a.out.h.
	(store_inferior_registers): Fixed error message.
	(core_file_command): Improved error messages from reading in of
	u area in core file.  Changed calculation of offset of registers
	to account for some machines putting it in as an offset rather
	than an absolute address.  Changed error messages for reading of
	registers from core file.

	* coffread.c (read_file_hdr): Added final check for BADMAG macro
	to use if couldn't recognize magic number.
	* Makefile: Added explicit directions for alloca addition.
	Included alloca.c in list of possible library files. Cleaned up
	possible library usage.  Included additional information on gcc
	and include files.  

	* source.c, remote.c, inflow.c, dbxread.c, core.c, coffread.c:
	Changed include of sys/fcntl.h to an include of fcntl.h (as per
	posix; presumably this will break fewer machines.  I hopw).
	* README: Added a pointer to comments at top of Makefile.
	* Makefile: Added a comment about machines which need fcntl.h in
	sys. 

Tue Apr  4 11:29:04 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* valprint.c (set_prettyprint_command, set_unionprint_command,
	format_info): Created.
	(_initialize_valprint): Added to lists of commands.

	* gdb.texinfo [Backtrace]: Added a section describing the format
	if symbols have not yet been read in.

	* valprint.c (val_print): Added code to prettyprint structures if
	"prettyprint" is set and only to print unions below the top level
	if "unionprint" is set.

	* infcmd.c (registers_info), valprint.c (value_print, val_print):
	Added argument to call to val_print indicating deptch of recursion.

	* symtab.[ch] (find_pc_psymbol): Created; finds static function
	psymbol with value nearest to but under value passed.
	* stack.c (print_frame_info): Used above to make sure I have best
	fit to pc value.

	* symseg.h (struct partial_symbol): Added value field.
	* dbxread.c (read_dbx_symtab): Set value field for partial symbols
	saved (so that we can lookup static symbols).

	* symtab.[ch] (find_pc_symtab): Changed to external.
	* stack.c (select_frame): Call above to make sure that symbols for
	a selected frame is readin.

Mon Apr  3 12:48:16 1989  Randall Smith  (randy at plantaris.ai.mit.edu)

	* stack.c (print_frame_info): Modified to only print out full
	stack frame info on symbols whose tables have been read in.
	* symtab.c, symtab.h (find_pc_psymtab): Made function external;
	above needed it.

	* main.c (,set_verbose_command, initialize_main): Created a
	variable "info_verbose" which says to talk it up in various and
	sundry places.  Added command to set this variable.
	* gdb.texinfo (GDB Output): Added documentation on "set verbose"
	and changed the name of the "Screen Output" section to "GDB
	Output".
	* dbxread.c (psymtab_to_symtab): Added information message about
	symbol readin.  Conditionalized on above.

	* dbxread.c (define_symbol): Made an "i" constant be of class
	LOC_CONST and an "r" constant be of class LOC_CONST_BYTES.

	* README: Made a note about modifications which may be necessary
	to the manual for this version of gdb.

	* blockframe.c (get_prev_frame_info): Now we get saved address and
	check for validity before we check for leafism.  This means that
	we will catch the fact that we are in start, but we will miss any
	fns that start calls without an fp.  This should be fine.

	* m-*.h (FRAME_CHAIN): Modified to return 0 if we are in start.
	This is usually a test for within the first object file.
	* m-sparc.h (FRAME_CHAIN): The test here is simply if the fp saved
	off the the start sp is 0.

	* blockframe.c (get_prev_frame_info): Removed check to see if we
	were in start.  Screws up sparc.  

	* m-sparc.h (FRAME_FIND_SAVED_REGISTERS): Changed test for dummy
	frame to not need frame to be innermost.

	* gdb.texinfo: Added section on frameless invocations of functions
	and when gdb can and can't deal with this.

	* stack.c (frame_info): Disallowed call if no inferior or core
	file; fails gracefully if truely bad stack specfication has been
	given (ie. parse_frame_specification returns 0).

Fri Mar 31 13:59:33 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* infrun.c (normal_stop): Changed references to "unset-env" to
	"delete env".

	* infcmd.c (_initialize_infcmd): Change reference to set-args in
	help run to "set args".

	* remote.c (getpkt): Allow immediate quit when reading from
	device; it could be hung.

	* coffread.c (process_coff_symbol): Modify handling of REG
	parameter symbols.

Thu Mar 30 15:27:23 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* dbxread.c (symbol_file_command): Use malloc to allocate the
	space for the string table in symbol_file_command (and setup a
	cleanup for this).  This allows a more graceful error failure if
	there isn't any memory availible (and probably allows more memory
	to be avail, depending on the machine).

	Additional mods for handling GNU C++ (from Tiemann):
	* dbxread.c (read_type): Added case for '#' type (method type, I
	believe).
	(read_struct_type): If type code is undefined, make the main
	variant for the type be itself.  Allow recognition of bad format
	in reading of structure fields.
	* eval.c (evaluate_subexp): Modify evaluation of a member of a
	structure and pointer to same to make sure that the syntax is
	being used correctly and that the member is being accessed correctly.
	* symseg.h: Added TYPE_CODE_METHOD to enum type_code.  Add a
	pointer to an array of argument types to the type structure.
	* symtab.c (lookout_method_type, smash_to_method_type): Created.
	* symtab.h (TYPE_ARG_TYPES): Created.
	* valops.c (call_function): Modified handling of methods to be the
	same as handling of functions; no longer check for members.
	* valprint.c (val_print, type_print_varspec_{prefix,suffix},
	type_print_base): Added code to print method args correctly.
	* values.c (value_virtual_fn_field): Modify access to virtual
	function table.

Wed Mar 29 13:19:34 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* findvar.c: Special cases for REGISTER_WINDOWS: 1) Return 0 if we
	are the innermost frame, and 2) return the next frame in's value
	if the SP is being looked for.

	* blockframe.c (get_next_frame): Created; returns the next (inner)
	frame of the called frame.
	* frame.h: Extern delcaration for above.

	* main.c (command_line_input): Stick null at end before doing
	history expansion.

Tue Mar 28 17:35:50 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* dbxread.c (read_dbx_symtab): Added namestring assignment to
	N_DATA/BSS/ABS case.  Sigh.

Sat Mar 25 17:49:07 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* expread.y: Defined YYDEBUG.

Fri Mar 24 20:46:55 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* symtab.c (make_symbol_completion_list): Completely rewrote to
	never call psymtab_to_symtab, to do a correct search (no
	duplicates) through the visible symbols, and to include structure
	and union fields in the things that it can match.

Thu Mar 23 15:27:44 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* dbxread.c (dbx_create_type): Created; allocates and inits space
	for a type without putting it on the type vector lists.
	(dbx_alloc_type): Uses above.

	* Makefile: xgdb.o now produced by default rules for .o.c.

Fri Mar 17 14:27:50 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* infrun.c: Fixed up inclusion of aouthdr.h on UMAX_PTRACE.

	* Makefile, config.gdb: Added hp300bsd to potential
	configurations.
	* hp300bsd-dep.c, m-hp300bsd.h: Created.

	* infrun.c (wait_for_inferior): Rewrote to do no access to
	inferior until we make sure it's still there.

	* inflow.c (inferior_died): Added a select to force the selected
	frame to null when inferior dies.

	* dbxread.c (symbol_file_command): free and zero symfile when
	discarding symbols.

	* core.c (xfer_core_file): Extended and cleaned up logic in
	interpeting memory address.

	* core.c (xfer_core_file): Extended opening comment.

Thu Mar 16 15:39:42 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* coffread.c (symbol_file_command): Free symfile name when freeing
	contents.

	* blockframe.c (get_prev_frame_info): Added to fatal error message
	to indicate that it should never happen.

	* stack.c (frame_info): Printed out value of "saved" sp seperately
	to call attention to the fact that it isn't stored in memory
	anywhere; the actual previous frames address is printed.

	* m-sparc.h (FRAME_FIND_SAVED_REGS): Set address of sp saved in
	frame to value of fp (rather than value of sp in current frame).

	* expread.y: Allow "unsigned" as a type itself, as well as a type
	modifier. 

	* coffread.c: Added declaration for fclose

Fri Mar 10 17:22:31 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* main.c (command_line_input): Checked for -1 return from
	readline; indicates EOF.

Fri Mar  3 00:31:27 1989  Randall Smith  (randy at gluteus.ai.mit.edu)

	* remote.c (remote_open): Cast return from signal to (void (*)) to
	avoid problems on machines where the return type of signal is (int
	(*)). 

	* Makefile: Removed deletion of version control from it (users
	will need it for their changes).

Thu Mar  2 15:32:21 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* symmetry-dep.c (print_1167_regs): Print out effective doubles on
	even number regs.
	(fetch_inferior_registers): Get the floating point regs also.

	* xgdb.c (do_command): Copied command before calling execute
	command (so that execute_command wouldn't write into text space).

	* copying.awk: Created (will produce copying.c as output when
	given COPYING as input).
	* Makefile: Used above to create copying.c.
	* main.c: Took out info_warranty and info_copying.

	* *.*: Changed copyright notice to use new GNU General Public
	License (includes necessary changes to manual).

	* xgdb.c (create_text_widget): Created text_widget before I create
	the source and sink.
	(print_prompt): Added fflush (stdout).

	* Makefile: Added -lXmu to the compilation line for xgdb.  Left
	the old one there incase people still had R2.

	* README: Added note about -gg format.

	* remote.c (getpkt): Fixed typo; && ==> &.

	* Makefile: Added new variable READLINE_FLAGS so that I could
	force compilation of readline.c and history.c with -DSYSV on
	system V machines.  Mentioned in Makefile comments at top.

Wed Mar  1 17:01:01 1989  Randall Smith  (randy at gluteus.ai.mit.edu)

	* hp9k320-dep.c (store_inferior_registers): Fixed typo.

Fri Feb 24 14:58:45 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* hp9k320-dep.c (store_inferior_registers,
	fetch_inferior_registers): Added support for remote debugging. 

	* remote.c (remote_timer): Created.
	(remote_open, readchar): Setup to timeout reads if they take
	longer than "timeout".  This allows one to debug how long such
	things take.
	(putpkt): Modified to print a debugging message (if such things
	are enabled) each time it resends a packet.
	(getpkt): Modified to make the variable CSUM unsigned and read it
	CSUM with an & 0xff (presumably to deal with poor sign extension
	on some machines).  Also made c1 and c2 unsigned.
	(remote_wait): Changed buffer to unsigned status.
	(remote_store_registers, remote_write_bytes): Puts a null byte at
	the end of the control string.

	* infcmd.c (attach_command, detach_command, _initialize_infcmd):
	Made attach_command and detach_command always availible, but
	modified them to only allow device file attaches if ATTACH_DETACH
	is not defined.

	* gdb.texinfo: Added cross reference from attach command to remote
	debugging. 

Thu Feb 23 12:37:59 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* remote.c (remote_close): Created to close the remote connection
	and set the remote_debugging flag to 0.
	* infcmd.c (detach_command): Now calls the above when appropriate. 

	* gdb.texinfo: Removed references to the ``Distribution'' section
	in the copyright.

	* main.c, utils.c (ISATTY): Created default defintions of this
	macro which use isatty and fileno.
	* utils.c (fprintf_filtered, print_spaces_filtered), main.c
	(command_loop, command_line_input): Used this macro.
	* m-news.h: Created a definition to override this one.

	* utils.c (fprintf_filtered): Made line_size static (clueless).

	* utils.c (fprintf_filtered): Changed max length of line printed
	to be 255 chars or twice the format length.

	* symmetry-dep.c, m-symmetry: Fixed typo (^L ==> ).

	* printcmd.c (do_examine): Fixed typo (\n ==> \t).

Wed Feb 22 16:00:33 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

        Contributed by Jay Vosburgh (jay@mentor.cc.purdue.edu)
	* m-symmetry.h, symmetry-dep.c: Created.
	* Makefile: Added above in appropriate lists.
	* config.gdb: Added "symmetry" target.

	* utils.c (prompt_for_continue): Zero'd chars_printed also.

	* utils.c (fprintf_filtered): Call prompt for continue instead of
	doing it yourself.

	* dbxread.c (read_dbx_symtab): Added code to conditionalize what
	symbol type holds to "x.o" or "-lx" symbol that indicates the
	beginning of a new file.

Tue Feb 21 16:22:13 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* gdb.texinfo: Deleted @ignore block at end of file.

	* findvar.c, stack.c: Changed comments that refered to "frame
	address" to "frame id".

	* findvar.c (locate_var_value): Modified so that taking the
	address of an array generates an object whose type is a pointer to
	the elements of the array.

Sat Feb 18 16:35:14 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* gdb.texinfo: Removed reference to "!" as a shell escape
	character.  Added a section on controling screen output
	(pagination); changing "Input" section to "User Interface"
	section.  Changed many inappropriate subsubsection nodes into
	subsections nodes (in the readline and history expansion
	sections). 

Fri Feb 17 11:10:54 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* utils.c (set_screensize_command): Created.
	(_initialize_utils): Added above to setlist.

	* main.c (main): Added check to see if ~/.gdbinit and .gdbinit
	were the same file; only one gets read if so.  Had to include
	sys/stat.h for this.

	* valprint.c (type_print_base): Changed calls to print_spaces to
	print_spaces_filtered. 

	* main.c (command_line_input): Chaned test for command line
	editing to check for stdin and isatty.

	* main.c (command_loop): Call reinitialize_more_filter before each
	command (if reading from stdin and it's a tty).
	utils.c (initialize_more_filter): Changed name to
	reinitialize_more_filter; killed arguments.
	utils.c (_initialize_utils): Created; initialized lines_per_page
	and chars_per_line here.

	* utils.c (fprintf_filtered): Removed printing of "\\\n" after
	printing linesize - 1 chars; assume that the screen display will
	take care of that.  Still watching that overflow.

	* main.c: Created the global variables linesize and pagesize to
	describe the number of chars per line and lines per page.

Thu Feb 16 17:27:43 1989  Randall Smith  (randy at gluteus.ai.mit.edu)

	* printcmd.c (do_examine, print_scalar_formatted, print_address,
	whatis_command, do_one_display, ptype_command), valprint.c
	(value_print, val_print, type_print_method_args, type_print_1,
	type_print_derivation_info, type_print_varspec_suffix,
	type_print_base), breakpoint.c (breakpoints_info, breakpoint_1),
	values.c (history_info), main.c (editing_info, warranty_info,
	copying_info), infcmd.c (registers_info), inflow.c
	(term_status_command), infrun.c (signals_info), stack.c
	(backtrace_command, print_frame_info), symtab.c (list_symbols,
	output_source_filename), command.c (help_cmd, help_list,
	help_command_list): Replaced calls to printf, fprintf, and putc
	with calls to [f]printf_filtered to handle more processing.
	Killed local more emulations where I noticed them.

Wed Feb 15 15:27:36 1989  Randall Smith  (randy at gluteus.ai.mit.edu)

	* defs.h, utils.c (initialize_more_filter, fprintf_filtered,
	printf_filtered): Created a printf that will also act as a more
	filter, prompting the user for a <return> whenever the page length
	is overflowed.

	* symtab.c (list_symbols): Elminated some code inside of an #if 0. 

Tue Feb 14 11:11:24 1989  Randall Smith  (randy at gluteus.ai.mit.edu)

	* Makefile: Turned off backup versions for this file; it changes
	too often.

	* command.c (lookup_cmd, _initialize_command): Changed '!' so that
	it was no longer a shell escape.  "sh" must be used.

	* main.c (command_line_input, set_history_expansion,
	initialize_main): Turned history expansion on, made it the
	default, and only execute it if the first character in the line is
	a '!'.

	* version.c, gdb.texinfo: Moved version to 3.2 (as usual, jumping
	the gun some time before release).

	* gdb.texinfo: Added sections (adapted from Brian's notes) on
	command line editing and history expansion.

	* main.c (set_command_editing, initialize_main): Modified name to
	set_editing and modified command to "set editing".

	* Makefile: Put in dependencies for READLINEOBJS.

	* main.c (history_info, command_info): Combined into new command
	info; deleted history_info.
	(initialize_main): Deleted "info history" command; it was
	interfering with the value history.

	* coffread.c (enter_linenos): Modified to do bit copy instead of
	pointer dereference, since the clipper machine can't handle having
	longs on short boundaries.
	(read_file_hdr): Added code to get number of syms for clipper.

	* stack.c (return_command): Fixed method for checking when all of
	the necessary frames had been popped.

	* dbxread.c (read_dbx_symtab (ADD_PSYMBOL_TO_LIST)): Fixed typo in
	allocation length.

Mon Feb 13 10:03:27 1989  Randall Smith  (randy at gluteus.ai.mit.edu)

	* dbxread.c (read_dbx_symtab): Split assignment to namestring into
	several different assignments (so that it wouldn't be done except
	when it had to be).  Shortened switches and duplicated code to
	produce the lowest possible execution time.  Commented (at top of
	switch) which code I duplicated.

	* dbxread.c (read_dbx_symtab): Modified which variables were
	register and deleted several variables which weren't used.  Also
	eliminated 'F' choice from subswitch, broke out strcmp's, reversed
	compare on line 1986, and elminated test for !namestring[0]; it is
	caught by following test for null index of ':'.

Sun Feb 12 12:57:56 1989  Randall Smith  (randy at plantaris.ai.mit.edu)

	* main.c (gdb_completer_word_break_characters): Turned \~ into ~.

Sat Feb 11 15:39:06 1989  Randall Smith  (randy at plantaris.ai.mit.edu)

	* symtab.c (find_pc_psymtab): Created; checks all psymtab's till
	it finds pc.
	(find_pc_symtab): Used; fatal error if psymtab found is readin
	(should have been caught in symtab loop).
	(lookup_symbol): Added check before scan through partial symtab
	list for symbol name to be on the misc function vector (only if in
	VAR_NAMESPACE).  Also made sure that psymtab's weren't fooled with
	if they had already been read in.
	(list_symbols): Checked through misc_function_vector for matching
	names if we were looking for functions.
	(make_symbol_completion_list): Checked through
	misc_function_vector for matching names.
	* dbxread.c (read_dbx_symtab): Don't bother to do processing on
	global function types; this will be taken care of by the
	misc_function hack.

	* symtab.h: Modified comment on misc_function structure.

Fri Feb 10 18:09:33 1989  Randall Smith  (randy at plantaris.ai.mit.edu)

	* symseg.h, dbxread.c (read_dbx_symtab, init_psymbol_list,
	start_psymtab, end_psymtab), coffread.c (_initialize_coff),
	symtab.c (lookup_partial_symbol, list_symbols,
	make_symbol_completion_list): Changed separate variables for
	description of partial symbol allocation into a specific kind of
	structure.

	(read_dbx_symtab, process_symbol_for_psymtab): Moved most of
	process_symbol_for_psymtab up into read_dbx_symtab, moved a couple
	of symbol types down to the ingore section, streamlined (I hope)
	code some, modularized access to psymbol lists.

Thu Feb  9 13:21:19 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* main.c (command_line_input): Made sure that it could recognize
	newlines as indications to repeat the last line.

	* symtab.c (_initialize_symtab): Changed size of builtin_type_void
	to be 1 for compatibility with gcc.

	* main.c (initialize_main): Made history_expansion the default
	when gdb is compiled with HISTORY_EXPANSION.

	* readline.c, readline.h, history.c, history.h, general.h,
	emacs_keymap.c, vi_keymap.c, keymaps.c, funmap.c: Made all of
	these links to /gp/gnu/bash/* to keep them updated.
	* main.c (initialize_main): Made default be command editing on. 

Wed Feb  8 13:32:04 1989  & Smith  (randy at hobbes)

	* dbxread.c (read_dbx_symtab): Ignore N_BSLINE on first
	readthrough. 

	* Makefile: Removed convex-dep.c from list of distribution files.

Tue Feb  7 14:06:25 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* main.c: Added command lists sethistlist and unsethistlist to
	accesible command lists.
	(parse_binary_operation): Created to parse a on/1/yes vs. off/0/no
	spec.
	(set_command_edit, set_history, set_history_expansion,
	set_history_write, set_history_size, set_history_filename,
	command_info, history_info): Created to allow users to control
	various aspects of command line editing.

	* main.c (symbol_creation_function): Created.
	(command_line_input, initialize_main): Added rest of stuff
	necessary for calling bfox' command editing routines under
	run-time control.
	* Makefile: Included readline and history source files for command
	editing; also made arrangements to make sure that the termcap
	library was available.
	* symtab.c (make_symbol_completion_list): Created.

Mon Feb  6 16:25:25 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* main.c: Invented variables to control command editing.
	command_editing_p, history_expansion_p, history_size,
	write_history_p, history_filename.  Initialized them to default
	values in initialize_main.

	* infcmd.c (registers_info), infrun.c (signals_info), 
	* main.c (gdb_read_line): Changed name to command_line_input.
	(readline): Changed name to gdb_readline; added second argument
	indicating that the read value shouldn't be saved (via malloc).
	* infcmd.c (registers_info), infrun.c (signals_info), main.c
	(copying_info), symtab.c (output_source_filename, MORE,
	list_symbols): Converted to use gdb_readline in place of
	gdb_read_line. 


Sun Feb  5 17:34:38 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* blockframe.c (get_frame_saved_regs): Removed macro expansion
	that had accidentally been left in the code.

Sat Feb  4 17:54:14 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* main.c (gdb_read_line, readline): Added function readline and
	converted gdb_read_line to use it.  This was a conversion to the
	line at a time style of input, in preparation for full command
	editing. 

Fri Feb  3 12:39:03 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* dbxread.c (read_dbx_symtab): Call end_psymtab at the end of
	read_dbx_symtab if any psymtab still needs to be completed.

	* config.gdb, sun3-dep.c: Brought these into accord with the
	actual sun2 status (no floating point period; sun3-dep.c unless
	has os > 3.0).
	* m-sun2os2.h: Deleted; not needed.

	* config.gdb: Added a couple of aliases for machines in the
	script. 

	* infrun.c: Added inclusion of aouthdr.h inside of #ifdef UMAX
	because ptrace needs to know about the a.out header.

	* Makefile: Made dep.o depend on dep.c and config.status only.

	* expread.y: Added declarations of all of the new write_exp_elt
	functions at the include section in the top.

	* Makefile: Added a YACC definition so that people can use bison
	if they wish.

	* Makefile: Added rms' XGDB-README to the distribution.

	* Makefile: Added removal of init.o on a "make clean".

Thu Feb  2 16:27:06 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* *-dep.c: Deleted definition of COFF_FORMAT if AOUTHDR was
	defined since 1) We *may* (recent mail message) want to define
	AOUTHDR under a basically BSD system, and 2) AOUTHDR is sometimes
	a typedef in coff encapsulation setups.  Also removed #define's of
	AOUTHDR if AOUTHDR is already defined (inside of coff format).  
	* core.c, dbxread.c: Removed #define's of AOUTHDR if AOUTHDR is
	already defined (inside of coff format).

Tue Jan 31 12:56:01 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* GDB 3.1 released.

	* values.c (modify_field): Changed test for endianness to assign
	to integer and reference character (so that all bits would be
	defined). 

Mon Jan 30 11:41:21 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* news-dep.c: Deleted inclusion of fcntl.h; just duplicates stuff
	found in sys/file.h.

	* i386-dep.c: Included default definition of N_SET_MAGIC for
	COFF_FORMAT.

	* config.gdb: Added checks for several different operating
	systems. 

	* coffread.c (read_struct_type): Put in a flag variable so that
	one could tell when you got to the end of a structure.

	* sun3-dep.c (core_file_command): Changed #ifdef based on SUNOS4
	to ifdef based on FPU.

	* infrun.c (restore_inferior_status): Changed error message to
	"unable to restore previously selected frame".

	* dbxread.c (read_dbx_symtab): Used intermediate variable in error
	message reporting a bad symbol type.  (scan_file_globals,
	read_ofile_symtab, read_addl_syms): Data type of "type" changed to
	unsigned char (which is what it is).
	* i386-dep.c: Removed define of COFF_FORMAT if AOUTHDR is defined.
	Removed define of a_magic to magic (taken care of by N_MAGIC).
	(core_file_command): Zero'd core_aouthdr instead of setting magic
	to zero.
	* i386-pinsn.c: Changed jcxz == jCcxz in jump table.
	(putop): Added a case for 'C'.
	(OP_J): Added code to handle possible masking of PC value on
	certain kinds of data.
	m-i386gas.h: Moved COFF_ENCAPSULATE to before inclusion of
	m-i386.h and defined NAMES_HAVE_UNDERSCORE.

	* coffread.c (unrecrod_misc_function, read_coff_symtab): Added
	symbol number on which error occured to error output.

Fri Jan 27 11:55:04 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* Makefile: Removed init.c in make clean.  Removed it without -f
	and with leading - in make ?gdb.

Thu Jan 26 15:08:03 1989  Randall Smith  (randy at gluteus.ai.mit.edu)

	Changes to get it to work on gould NP1.
	* dbxread.c (read_dbx_symtab): Included cases for N_NBDATA and
	N_NBBSS.
	(psymtab_to_symtab): Changed declaration of hdr to
	DECLARE_FILE_HEADERS.  Changed access to use STRING_TABLE_SIZE and
	SYMBOL_TABLE_SIZE.
	* gld-pinsn.c (findframe): Added declaration of framechain() as
	FRAME_ADDR. 

	* coffread.c (read_coff_symtab): Avoided treating typedefs as
	external symbol definitions.

Wed Jan 25 14:45:43 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* Makefile: Removed reference to alloca.c.  If they need it, they
	can pull alloca.o from the gnu-emacs directory.

	* version.c, gdb.texinfo: Updated version to 3.1 (jumping the gun
	a bit so that I won't forget when I release).

	* m-sun2.h, m-sun2os2.h, m-sun3os4.h, config.gdb: Modified code so
	that default includes new sun core, ptrace, and attach-detach.
	Added defaults for sun 2 os 2.

	Modifications to reset stack limit back to what it used to be just
	before exec.  All mods inside of #ifdef SET_STACK_LIMIT_HUGE.
	* main.c: Added global variable original_stack_limit.
	(main): Set original_stack_limit to original stack limit.
	* inflow.c: Added inclusion of necessary files and external
	reference to original_stack_limit.
	(create_inferior): Reset stack limit to original_stack_limit.

	* dbxread.c (read_dbx_symtab): Killed PROFILE_SYMBOLS ifdef.

	* sparc-dep.c (isabranch): Multiplied offset by 4 before adding it
	to addr to get target.

	* Makefile: Added definition of SHELL to Makefile.

	* m-sun2os4.h: Added code to define NEW_SUN_PTRACE, NEW_SUN_CORE,
	and ATTACH_DETACH.
	* sun3-dep.c: Added code to avoid fp regs if we are on a sun2.

Tue Jan 24 17:59:14 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* dbxread.c (read_array_type): Added function.
	(read_type): Added call to above instead of inline code.

	* Makefile: Added ${GNU_MALLOC} to the list of dependencies for
	the executables.

Mon Jan 23 15:08:51 1989  Randall Smith  (randy at plantaris.ai.mit.edu)

	* gdb.texinfo: Added paragraph to summary describing languages
	with which gdb can be run.  Also added descriptions of the
	"info-methods" and "add-file" commands.

	* symseg.h: Commented a range type as having TYPE_TARGET_TYPE
	pointing at the containing type for the range (often int).
	* dbxread.c (read_range_type): Added code to do actual range types
	if they are defined.  Assumed that the length of a range type is
	the length of the target type; this is a lie, but will do until
	somebody gets back to me as to what these silly dbx symbols mean.
	
	* dbxread.c (read_range_type): Added code to be more picky about
	recognizing builtins as range types, to treat types defined as
	subranges of themselves to be subranges of int, and to recognize
	the char type idiom from dbx as a special case.

Sun Jan 22 01:00:13 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* m-vax.h: Removed definition of FUNCTION_HAS_FRAME_POINTER.
	* blockframe.c (get_prev_frame_info): Removed default definition
	and use of above.  Instead conditionalized checking for leaf nodes
	on FUNCTION_START_OFFSET (see comment in code).

Sat Jan 21 16:59:19 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* dbxread.c (read_range_type): Fixed assumption that integer was
	always type 1.

	* gdb.texinfo: Fixed spelling mistake and added a note in the
	running section making it clear that users may invoke subroutines
	directly from gdb.

	* blockframe.c: Setup a default definition for the macro
	FUNCTION_HAS_FRAME_POINTER.
	(get_prev_frame_info): Used this macro instead of checking
	SKIP_PROLOGUE directly.
	* m-vax.h: Overroad definition; all functions on the vax have
	frame pointers.

Fri Jan 20 12:25:35 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* core.c: Added default definition of N_MAGIC for COFF_FORMAT.

	* xgdb.c: Installed a fix to keep the thing from dying when there
	isn't any frame selected.

	* core.c: Made a change for the UMAX system; needs a different
	file included if using that core format. 

	* Makefile: Deleted duplicate obstack.h in dbxread.c dependency.

	* munch: Modified (much simpler) to cover (I hope) all cases.

	* utils.c (save_cleanups, restore_cleanups): Added functions to
	allow you to push and pop the chain of cleanups to be done.
	* defs.h: Declared the new functions.
	* main.c (catch_errors): Made sure that the only cleanups which
	would be done were the ones put on the chain *after* the current
	location. 

	* m-*.h (FRAME_CHAIN_VALID): Removed check on pc in the current
	frame being valid.
	* blockframe.c (get_prev_frame_info): Made the assumption that if
	a frame's pc value was within the first object file (presumed to
	be /lib/crt0.o), that we shouldn't go any higher.

	* infrun.c (wait_for_inferior): Do *not* execute check for stop pc
	at step_resume_break if we are proceeding over a breakpoint (ie.
	if trap_expected != 0).

	* Makefile: Added -g to LDFLAGS.

	* m-news.h (POP_FRAME) Fixed typo.

	* printcmd.c (print_frame_args): Modified to print out register
	params in order by .stabs entry, not by register number.

	* sparc-opcode.h: Changed declaration of (struct
	arith_imm_fmt).simm to be signed (as per architecture manual).
	* sparc-pinsn.c (fprint_addr1, print_insn): Forced a cast to an
	int, so that we really would get signed behaivior (default for sun
	cc is unsigned).

	* i386-dep.c (i386_get_frame_setup): Replace function with new
	function provided by pace to fix bug in recognizing prologue.

Thu Jan 19 11:01:22 1989  Randall Smith  (randy at plantaris.ai.mit.edu)

	* infcmd.c (run_command): Changed error message to "Program not
	restarted." 

	* value.h: Changed "frame" field in value structure to be a
	FRAME_ADDR (actually CORE_ADDR) so that it could survive across
	calls. 

	* m-sun.h (FRAME_FIND_SAVED_REGS): Fixed a typo.

	* value.h: Added lval: "lval_reg_frame_relative" to indicate a
	register that must be interpeted relative to a frame.  Added
	single entry to value structure: "frame", used to indicate which
	frame a relative regnum is relative to.
	* findvar.c (value_from_register): Modified to correctly setup
	these fields when needed.  Deleted section to fiddle with last
	register copied on little endian machine; multi register
	structures will always occupy an integral number of registers.
	(find_saved_register): Made extern.
	* values.c (allocate_value, allocate_repeat_value): Zero frame
	field on creation.
	* valops.c (value_assign): Added case for lval_reg_frame_relative;
	copy value out, modify it, and copy it back.  Desclared
	find_saved_register as being external.
	* value.h: Removed addition of kludgy structure; thoroughly
	commented file.
	* values.c (free_value, free_all_values, clear_value_history,
	set_internalvar, clear_internavars): Killed free_value.

Wed Jan 18 20:09:39 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* value.h: Deleted struct partial_storage; left over from
	yesterday. 

	* findvar.c (value_from_register): Added code to create a value of
	type lval_reg_partsaved if a value is in seperate registers and
	saved in different places.

Tue Jan 17 13:50:18 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* value.h: Added lval_reg_partsaved to enum lval_type and
	commented enum lval_type.  Commented value structure.
	Added "struct partial_register_saved" to value struct; added
	macros to deal with structure to value.h.
	* values.c (free_value): Created; special cases lval_reg_partsaved
	(which has a pointer to an array which also needs to be free).
	(free_all_values, clear_value_history, set_internalvar,
	clear_internalvars): Modified to use free_values.

	* m-sunos4.h: Changed name to sun3os4.h.
	* m-sun2os4.h, m-sun4os4.h: Created.
	* config.gdb: Added configuration entries for each of the above.
	* Makefile: Added into correct lists.

	* Makefile: Added dependencies on a.out.encap.h.  Made
	a.out.encap.h dependent on a.out.gnu.h and dbxread.c dependent on
	stab.gnu.h. 

	* infrun.c, remote.c: Removed inclusion of any a.out.h files in
	these files; they aren't needed.

	* README: Added comment about bug reporting and comment about
	xgdb. 

	* Makefile: Added note to HPUX dependent section warning about
	problems if compiled with gcc and mentioning the need to add
	-Ihp-include to CFLAGS if you compile on those systems.  Added a
	note about needing the GNU nm with compilers *of gdb* that use the
	coff encapsulate feature also.  * hp-include: Made symbolic link
	over to /gp/gnu/binutils.

	* Makefile: Added TSOBS NTSOBS OBSTACK and REGEX to list of things
	to delete in "make clean".  Also changed "squeakyclean" target as
	"realclean". 

	* findvar.c (value_from_register): Added assignment of VALUE_LVAL
	to be lval_memory when that is appropriate (original code didn't
	bother because it assumed that it was working with a pre lval
	memoried value).

	* expread.y (yylex): Changed to only return type THIS if the
	symbol "$this" is defined in some block superior or equal to the
	current expression context block.

Mon Jan 16 13:56:44 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* m-*.h (FRAME_CHAIN_VALID): On machines which check the relation
	of FRAME_SAVED_PC (thisframe) to first_object_file_end (all except
	gould), make sure that the pc of the current frame also passes (in
	case someone stops in _start).

	* findvar.c (value_of_register): Changed error message in case of
	no inferior or core file.

	* infcmd.c (registers_info): Added a check for inferior or core
	file; error message if not.

	* main.c (gdb_read_line): Modified to take prompt as argument and
	output it to stdout.
	* infcmd.c (registers_info, signals_info), main.c (command_loop,
	read_command_lines, copying_info), symtab.c (decode_line_2,
	output_source_filename, MORE, list_symbols): Changed calling
	convention used to call gdb_read_line.

	* infcmd.c, infrun.c, main.c, symtab.c: Changed the name of the
	function "read_line" to "gdb_read_line".
	* breakpoint.c: Deleted external referenced to function
	"read_line" (not needed by code).

Fri Jan 13 12:22:05 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* i386-dep.c: Include a.out.encap.h if COFF_ENCAPSULATE.
	(N_SET_MAGIC): Defined if not defined by include file.
	(core_file_command): Used N_SET_MAGIC instead of assignment to
	a_magic.
	(exec_file_command): Stuck in a HEADER_SEEK_FD.

	* config.gdb: Added i386-dep.c as depfile for i386gas choice.

	* munch: Added -I. to cc to pick up things included by the param
	file. 

	* stab.gnu.def: Changed name to stab.def (stab.gnu.h needs this name).
	* Makefile: Changed name here also.
	* dbxread.c: Changed name of gnu-stab.h to stab.gnu.h.

	* gnu-stab.h: Changed name to stab.gnu.h.
	* stab.gnu.def: Added as link to binutils.
	* Makefile: Put both in in the distribution.

Thu Jan 12 11:33:49 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* dbxread.c: Made which stab.h is included dependent on
	COFF_ENCAPSULATE; either <stab.h> or "gnu-stab.h".
	* Makefile: Included gnu-stab.h in the list of files to include in
	the distribution.
	* gnu-stab.h: Made a link to /gp/gnu/binutils/stab.h

	* Makefile: Included a.out.gnu.h and m-i386gas.h in list of
	distribution files.
	* m-i386gas.h: Changed to include m-i386.h and fiddle with it
	instead of being a whole new file.
	* a.out.gnu.h: Made a link to /gp/gnu/binutils/a.out.gnu.h.

	Chris Hanson's changes to gdb for hp Unix.
	* Makefile: Modified comments on hpux.
	* hp9k320-dep.c: #define'd WOPR & moved inclusion of signal.h
	* inflow.c: Moved around declaratiosn of <sys/fcntl.h> and
	<sys/ioctl.h> inside of USG depends and deleted all SYSV ifdef's
	(use USG instead).
	* munch: Modified to accept any number of spaces between the T and
	the symbol name.

	Pace's changes to gdb to work with COFF_ENCAPSULATE (robotussin):
	* config.gdb: Added i386gas to targets.
	* default-dep.c: Include a.out.encap.h if COFF_ENCAPSULATE.
	(N_SET_MAGIC): Defined if not defined by include file.
	(core_file_command): Used N_SET_MAGIC instead of assignment to a_magic.
	(exec_file_command): Stuck in a HEADER_SEEK_FD.
	* infrun.c, remote.c: Added an include of a.out.encap.h if
	COFF_ENCAPSULATE defined.  This is commented out in these two
	files, I presume because the definitions aren't used.
	* m-i386gas.h: Created.
	* dbxread.c: Included defintions for USG.
	(READ_FILE_HEADERS): Now uses HEADER_SEEK_FD if it exists.
	(symbol_file_command): Deleted use of HEADER_SEEK_FD.
	* core.c: Deleted extra definition of COFF_FORMAT.
	(N_MAGIC): Defined to be a_magic if not already defined.
	(validate_files): USed N_MAGIC instead of reading a_magic.

Wed Jan 11 12:51:00 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* remote.c: Upped PBUFSIZ.
	(getpkt): Added zeroing of c inside loop in case of error retry.

	* dbxread.c (read_dbx_symtab, process_symbol_for_psymtab): Removed
	code to not put stuff with debugging symbols in the misc function
	list.  Had been ifdef'd out.

	* gdb.texinfo: Added the fact that the return value for a function
	is printed if you use return.

	* infrun.c (wait_for_inferior): Removed test in "Have we hit
	step_resume_breakpoint" for sp values in proper orientation.  Was
	in there for recursive calls in functions without frame pointers
	and it was screwing up calls to alloca.  

	* dbxread.c: Added #ifdef COFF_ENCAPSULATE to include
	a.out.encap.h.
	(symbol_file_command): Do HEADER_SEEK_FD when defined.
	* dbxread.c, core.c: Deleted #ifdef ROBOTUSSIN stuff.
	* robotussin.h: Deleted local copy (was symlink).
	* a.out.encap.h: Created symlink to
	/gp/gnu/binutils/a.out.encap.h.
	* Makefile: Removed robotussin.h and included a.out.encap.h in
	list of files.

	* valprint.c (val_print, print_scalar_formatted): Changed default
	precision of printing float value; now 6 for a float and 16 for a
	double.

	* findvar.c (value_from_register): Added code to deal with the
	case where a value is spread over several registers.  Still don't
	deal with the case when some registers are saved in memory and
	some aren't.

Tue Jan 10 17:04:04 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* xgdb.c (xgdb_create_window): Removed third arg (XtDepth) to
	frameArgs.  

	* infrun.c (handle_command): Error if signal number is less or
	equal to 0 or greater or equal to NSIG or a signal number is not
	provided.

	* command.c (lookup_cmd): Modified to not convert command section
	of command line to lower case in place (in case it isn't a
	subcommand, but an argument to a command).

Fri Jan  6 17:57:34 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* dbxread.c: Changed "text area" to "data area" in comments on
	N_SETV. 

Wed Jan  4 12:29:54 1989  Randall Smith  (randy at gluteus.ai.mit.edu)

	* dbxread.c: Added definitions of gnu symbol types after inclusion
	of a.out.h and stab.h.

Mon Jan  2 20:38:31 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* eval.c (evaluate_subexp): Binary logical operations needed to
	know type to determine whether second value should be evaluated.
	Modified to discover type before binup_user_defined_p branch.
	Also commented "enum noside".

	* Makefile: Changed invocations of munch to be "./munch".

	* gdb.texinfo: Updated to refer to current version of gdb with
	January 1989 last update.

	* coffread.c (end_symtab): Zero context stack when finishing
	lexical contexts.
	(read_coff_symtab): error if context stack 0 in ".ef" else case.

	* m-*.h (FRAME_SAVED_PC): Changed name of argument from "frame" to
	"FRAME" to avoid problems with replacement of "->frame" part of
	macro. 

	* i386-dep.c (i386_get_frame_setup): Added codestream_get() to
	move codestream pointer up to the correct location in "subl $X,
	%esp" case.

Sun Jan  1 14:24:35 1989  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* valprint.c (val_print): Rewrote routine to print string pointed
	to by char pointer; was producing incorrect results when print_max
	was 0.

Fri Dec 30 12:13:35 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* dbxread.c (read_dbx_symtab, process_symbol_for_psymtab): Put
	everything on the misc function list.

	* Checkpointed distribution.

	* Makefile: Added expread.tab.c to the list of things slated for
	distribution. 

Thu Dec 29 10:06:41 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* stack.c (set_backtrace_limit_command, backtrace_limit_info,
	bactrace_command, _initialize_stack): Removed modifications for
	limit on backtrace.  Piping the backtrace through an interuptable
	"more" emulation is a better way to do it.

Wed Dec 28 11:43:09 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* stack.c
	(set_backtrace_limit_command): Added command to set a limit to the
	number of frames for a backtrace to print by default.
	(backtrace_limit_info): To print the current limit.
	(backtrace_command): To use the limit.
	(_initialize_stack): To initialize the limit to its default value
	(30), and add the set and info commands onto the appropriate
	command lists.

    	* gdb.texinfo: Documented changes to "backtrace" and "commands"
	commands.

	* stack.c (backtrace_command): Altered so that a negative argument
	would show the last few frames on the stack instead of the first
	few.
	(_initialize_stack): Modified help documentation.

	* breakpoint.c (commands_command): Altered so that "commands" with
	no argument would refer to the last breakpoint set.
	(_initialize_breakpoint): Modified help documentation.

	* infrun.c (wait_for_inferior): Removed ifdef on Sun4; now you can
	single step through compiler generated sub calls and will die if
	you next off of the end of a function.

	* sparc-dep.c (single_step): Fixed typo; "break_insn" ==> "sizeof
	break_insn". 

	* m-sparc.h (INIT_EXTRA_FRAME_INFO): Set the bottom of a stack
	frame to be the bottom of the stack frame inner from this, if that
	inner one is a leaf node.

	* dbxread.c (read_dbx_symtab): Check to make sure we don't add a
	psymtab to it's own dependency list.

	* dbxread.c (read_dbx_symtab): Modified check for duplicate
	dependencies to catch them correctly.

Tue Dec 27 17:02:09 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* m-*.h (FRAME_SAVED_PC): Modified macro to take frame info
	pointer as argument.
	* stack.c (frame_info), blockframe.c (get_prev_frame_info),
	gld-pinsn.c (findframe), m-*.h (SAVED_PC_AFTER_CALL,
	FRAME_CHAIN_VALID, FRAME_NUM_ARGS): Changed usage of macros to
	conform to above.
	* m-sparc.h (FRAME_SAVED_PC), sparc-dep.c (frame_saved_pc):
	Changed frame_saved_pc to have a frame info pointer as an
	argument. 

	* m-vax.h, m-umax.h, m-npl.h, infrun.c (wait_for_inferior),
	blockframe.c (get_prev_frame_info): Modified SAVED_PC_AFTER_CALL
	to take a frame info pointer as an argument.

	* blockframe.c (get_prev_frame_info): Altered the use of the
	macros FRAME_CHAIN, FRAME_CHAIN_VALID, and FRAME_CHAIN_COMBINE to
	use frame info pointers as arguments instead of frame addresses.
	* m-vax.h, m-umax.h, m-sun3.h, m-sun3.h, m-sparc.h, m-pn.h,
	m-npl.h, m-news.h, m-merlin.h, m-isi.h, m-hp9k320.h, m-i386.h:
	Modified definitions of the above macros to suit.
	* m-pn.h, m-npl.h, gould-dep.c (findframe): Modified findframe to
	use a frame info argument; also fixed internals (wouldn't work
	before).

	* m-sparc.h: Cosmetic changes; reordered some macros and made sure
	that nothing went over 80 lines.

Thu Dec 22 11:49:15 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* Version 3.0 released.

	* README: Deleted note about changing -lobstack to obstack.o.

Wed Dec 21 11:12:47 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* m-vax.h (SKIP_PROLOGUE): Now recognizes gcc prologue also.

	* blockframe.c (get_prev_frame_info): Added FUNCTION_START_OFFSET
	to result of get_pc_function_start.
	* infrun.c (wait_for_inferior): Same.

	* gdb.texinfo: Documented new "step" and "next" behavior in
	functions without line number information.

Tue Dec 20 18:00:45 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* infcmd.c (step_1): Changed behavior of "step" or "next" in a
	function witout line number information.  It now sets the step
	range around the function (to single step out of it) using the
	misc function vector, warns the user, and continues.

	* symtab.c (find_pc_line): Zero "end" subsection of returned
	symtab_and_line if no symtab found.

Mon Dec 19 17:44:35 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* i386-pinsn.c (OP_REG): Added code from pace to streamline
	disassembly and corrected types.
	* i386-dep.c
	(i386_follow_jump): Code added to follow byte and word offset
	branches.
	(i386_get_frame_setup): Expanded to deal with more wide ranging
	function prologue.
	(i386_frame_find_saved_regs, i386_skip_prologue): Changed to use
	i386_get_frame_setup. 
	

Sun Dec 18 11:15:03 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* m-sparc.h: Deleted definition of SUN4_COMPILER_BUG; was designed
	to avoid something that I consider a bug in our code, not theirs,
	and which I fixed earlier.  Also deleted definition of
	CANNOT_USE_ARBITRARY_FRAME; no longer used anywhere.
	FRAME_SPECIFICATION_DYADIC used instead.

	* infrun.c (wait_for_inferior): On the sun 4, if a function
	doesn't have a prologue, a next over it single steps into it.
	This gets around the problem of a "call .stret4" at the end of
	functions returning structures.
	* m-sparc.h: Defined SUN4_COMPILER_FEATURE.

	* main.c (copying_info): Seperated the last printf into two
	printfs.  The 386 compiler will now handle it.

	* i386-pinsn.c, i386-dep.c: Moved print_387_control_word,
	print_387_status_word, print_387_status, and i386_float_info to
	dep.c  Also included reg.h in dep.c.

Sat Dec 17 15:31:38 1988  Randall Smith  (randy at gluteus.ai.mit.edu)

	* main.c (source_command): Don't close instream if it's null
	(indicating execution of a user-defined command).
		 (execute_command): Set instream to null before executing
		 commands and setup clean stuff to put it back on error.

	* inflow.c (terminal_inferior): Went back to not checking the
	ioctl returns; there are some systems when this will simply fail.
	It seems that, on most of these systems, nothing bad will happen
	by that failure.

	* values.c (value_static_field): Fixed dereferencing of null
	pointer. 

	* i386-dep.c (i386_follow_jump): Modified to deal with
	unconditional byte offsets also.

	* dbxread.c (read_type): Fixed typo in function type case of switch.

	* infcmd.c (run_command): Does not prompt to restart if command is
	not from a tty.

Fri Dec 16 15:21:58 1988  Randy Smith  (randy at calvin)

	* gdb.texinfo: Added a third option under the "Cannot Insert
	Breakpoints" workarounds.

	* printcmd.c (display_command): Don't do the display unless there
	is an active inferior; only set it.

	* findvar.c (value_of_register): Added an error check for calling
	this when the inferior isn't active and a core file isn't being
	read. 

	* config.gdb: Added reminder about modifying REGEX in the
	makefile for the 386.

	* i386-pinsn.c, i386-dep.c: Moved m-i386.h helper functions over
	to i386-dep.c.b

Thu Dec 15 14:04:25 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* README: Added a couple of notes about compiling gdb with itself.

	* breakpoint.c (set_momentary_breakpoint): Only takes FRAME_FP of
	frame if frame is non-zero.

	* printcmd.c (print_scalar_formatted): Implemented /g size for
	hexadecimal format on machines without an 8 byte integer type.  It
	seems to be non-trivial to implement /g for other formats.
	(decode_format): Allowed hexadecimal format to make it through /g
	fileter. 

Wed Dec 14 13:27:04 1988  Randall Smith  (randy at gluteus.ai.mit.edu)

	* expread.y: Converted all calls to write_exp_elt from the parser
	to calls to one of write_exp_elt_{opcode, sym, longcst, dblcst,
	char, type, intern}.  Created all of these routines.  This gets
	around possible problems in passing one of these things in one ear
	and getting something different out the other.  Eliminated
	SUN4_COMPILER_BUG ifdef's; they are now superfluous.

	* symmisc.c (free_all_psymtabs): Reinited partial_symtab_list to 0.
		    (_initialize_symmisc): Initialized both symtab_list and
		    partial_symtab_list.

	* dbxread.c (start_psymtab): Didn't allocate anything on
	dependency list.
	(end_psymtab): Allocate dependency list on psymbol obstack from
	local list.
	(add_psymtab_dependency): Deleted.
	(read_dbx_symtab): Put dependency on local list if it isn't on it
	already.

	* symtab.c: Added definition of psymbol_obstack.
	* symtab.h: Added declaration of psymbol_obstack.
	* symmisc.c (free_all_psymtabs): Added freeing and
	reinitionaliztion of psymbol_obstack.
	* dbxread.c (free_all_psymbols): Deleted.
		    (start_psymtab, end_psymtab,
		    process_symbol_for_psymtab):  Changed most allocation
		    of partial symbol stuff to be off of psymbol_obstack.

	* symmisc.c (free_psymtab, free_all_psymtabs): Deleted
	free_psymtab subroutine.

	* symtab.h: Removed num_includes and includes from partial_symtab
	structure; no longer needed now that all include files have their
	own psymtab.
	* dbxread.c (start_psymtab): Eliminated initialization of above.
		    (end_psymtab): Eliminated finalization of above; get
		    includes from seperate list.
		    (read_dbx_symtab): Moved includes from psymtab list to
		    their own list; included in call to end_psymtab.
	* symmisc.c (free_psymtab): Don't free includes.

Tue Dec 13 14:48:14 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* i386-pinsn.c: Reformatted entire file to correspond to gnu
	software indentation conventions.

	* sparc-dep.c (skip_prologue): Added capability of recognizign
	stores of input register parameters into stack slots. 

	* sparc-dep.c: Added an include of sparc-opcode.h.
	* sparc-pinsn.c, sparc-opcode.h: Moved insn_fmt structures and
	unions from pinsn.c to opcode.h.
	* sparc-pinsn.c, sparc-dep.c (isabranch, skip_prologue): Moved
	this function from pinsn.c to dep.c.

	* Makefile: Put in warnings about compiling with gcc (non-ansi
	include files) and compiling with shared libs on Sunos 4.0 (can't
	debug something that's been compiled that way).

	* sparc-pinsn.c: Put in a completely new file (provided by
	Tiemann) to handle floating point disassembly, load and store
	instructions, and etc. better.  Made the modifications this file
	(ChangeLog) list for sparc-pinsn.c again.

	* symtab.c (output_source_filename): Included "more" emulation hack.

	* symtab.c (output_source_filename): Initialized COLUMN to 0.
		   (sources_info): Modified to not print out a line for
		   all of the include files within a partial symtab (since
		   they have pst's of their own now).  Also modified to
		   make a distinction between those pst's read in and
		   those not.

	* infrun.c: Included void declaration of single_step() if it's
	going to be used.
	* sparc-dep.c (single_step): Moved function previous to use of it.

	* Makefile: Took removal of expread.tab.c out of make clean entry
	and put it into a new "squeakyclean" entry.

Mon Dec 12 13:21:02 1988  Randall Smith  (randy at gluteus.ai.mit.edu)

	* sparc-pinsn.c (skip_prologue): Changed a struct insn_fmt to a
	union insn_fmt.

	* inflow.c (terminal_inferior): Checked *all* return codes from
	ioctl's and fcntl's in routine.

	* inflow.c (terminal_inferior): Added check for sucess of
	TIOCSPGRP ioctl call.  Just notifies if bad.

	* dbxread.c (symbol_file_command): Close was getting called twice;
	once directly and once through cleanup.  Killed the direct call.  

Sun Dec 11 19:40:40 1988  & Smith  (randy at hobbes.ai.mit.edu)

	* valprint.c (val_print): Deleted spurious printing of "=" from
	TYPE_CODE_REF case.

Sat Dec 10 16:41:07 1988  Randall Smith  (randy at gluteus.ai.mit.edu)

	* dbxread.c: Changed allocation of psymbols from using malloc and
	realloc to using obstacks.  This means they aren't realloc'd out
	from under the pointers to them.

Fri Dec  9 10:33:24 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* sparc-dep.c inflow.c core.c expread.y command.c infrun.c
	infcmd.c dbxread.c symmisc.c symtab.c printcmd.c valprint.c
	values.c source.c stack.c findvar.c breakpoint.c blockframe.c
	main.c: Various cleanups inspired by "gcc -Wall" (without checking
	for implicit declarations).

	* Makefile: Cleaned up some more.

	* valops.c, m-*.h (FIX_CALL_DUMMY): Modified to take 5 arguments
	as per what sparc needs (programming for a superset of needed
	args).

	* dbxread.c (process_symbol_for_psymtab): Modified to be slightly
	more picky about what it puts on the list of things *not* to be
	put on the misc function list.  When/if I shift everything over to
	being placed on the misc_function_list, this will go away.

	* inferior.h, infrun.c: Added fields to save in inferior_status
	structure. 

	* maketarfile: Deleted; functionality is in Makefile now.

	* infrun.c (wait_for_inferior): Modified algorithm for determining
	whether or not a single-step was through a subroutine call.  See
	comments at top of file.

	* dbxread.c (read_dbx_symtab): Made sure that the IGNORE_SYMBOL
	macro would be checked during initial readin.

	* dbxread.c (read_ofile_symtab): Added macro GCC_COMPILED_FLAG_SYMBOL
	into dbxread.c to indicate what string in a local text symbol will
	indicate a file compiled with gcc.  Defaults to "gcc_compiled.".

Thu Dec  8 11:46:22 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* m-sparc.h (FRAME_FIND_SAVED_REGS): Cleaned up a little to take
	advantage of the new frame cache system.

	* inferior.h, infrun.c, valops.c, valops.c, infcmd.c:  Changed
	mechanism to save inferior status over calls to inferior (eg.
	call_function); implemented save_inferior_info and
	restore_inferior_info.

	* blockframe.c (get_prev_frame): Simplified this by a direct call
	to get_prev_frame_info.

	* frame.h, stack.c, printcmd.c, m-sparc.h, sparc-dep.c: Removed
	all uses of frame_id_from_addr.  There are short routines like it
	still in frame_saved_pc (m-sparc.h) and parse_frame_spec
	(stack.c).  Eventually the one in frame_saved_pc will go away.

	* infcmd.c, sparc-dep.c: Implemented a new mechanism for
	re-selecting the selected frame on return from a call.

	* blockframe.c, stack.c, findvar.c, printcmd.c, m-*.h:  Changed
	all routines and macros that took a "struct frame_info" as an
	argument to take a "struct frame_info *".  Routines: findarg,
	framechain, print_frame_args, FRAME_ARGS_ADDRESS,
	FRAME_STRUCT_ARGS_ADDRESS, FRAME_LOCALS_ADDRESS, FRAME_NUM_ARGS,
	FRAME_FIND_SAVED_REGS.

	* frame.h, stack.c, printcmd.c, infcmd.c, findvar.c, breakpoint.c,
	blockframe.c, xgdb.c, i386-pinsn.c, gld-pinsn.c, m-umax.h,
	m-sun2.h, m-sun3.h, m-sparc.h, m-pn.h, m-npl.h, m-news.h,
	m-merlin.h, m-isi.h, m-i386.h, m-hp9k320.h:  Changed routines to
	use "struct frame_info *" internally.

Wed Dec  7 12:07:54 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* frame.h, blockframe.c, m-sparc.h, sparc-dep.c: Changed all calls
	to get_[prev_]frame_cache_item to get_[prev_]frame_info.

	* blockframe.c: Elminated get_frame_cache_item and
	get_prev_frame_cache_item; functionality now taken care of by
	get_frame_info and get_prev_frame_info.

	* blockframe.c: Put allocation on an obstack and eliminated fancy
	reallocation routines, several variables, and various nasty
	things. 

	* frame.h, stack.c, infrun.c, blockframe.c, sparc-dep.c: Changed
	type FRAME to be a typedef to "struct frame_info *".  Had to also
	change routines that returned frame id's to return the pointer
	instead of the cache index.

	* infcmd.c (finish_command): Used proper method of getting from
	function symbol to start of function.  Was treating a symbol as a
	value. 

	* blockframe.c, breakpoint.c, findvar.c, infcmd.c, stack.c,
	xgdb.c, i386-pinsn.c, frame.h, m-hp9k320.h, m-i386.h, m-isi.h,
	m-merlin.h, m-news.h, m-npl.h, m-pn.h, m-sparc.h, m-sun2.h,
	m-sun3.h, m-umax.h: Changed get_frame_info and get_prev_frame_info
	to return pointers instead of structures.

	* blockframe.c (get_pc_function_start): Modified to go to misc
	function table instead of bombing if pc was in a block without a
	containing function.

	* coffread.c: Dup'd descriptor passed to read_coff_symtab and
	fdopen'd it so that there wouldn't be multiple closes on the same
	fd.  Also put (fclose, stream) on the cleanup list.

	* printcmd.c, stack.c: Changed print_frame_args to take a
	frame_info struct as argument instead of the address of the args
	to the frame.

	* m-i386.h (STORE_STRUCT_RETURN): Decremented sp by sizeof object
	to store (an address) rather than 1.

	* dbxread.c (read_dbx_symtab): Set first_object_file_end in
	read_dbx_symtab (oops).

	* coffread.c (fill_in_vptr_fieldno): Rewrote TYPE_BASECLASS as
	necessary. 

Tue Dec  6 13:03:43 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* coffread.c: Added fake support for partial_symtabs to allow
	compilation and execution without there use.
	* inflow.c: Added a couple of minor USG mods.
	* munch: Put in appropriate conditionals so that it would work on
	USG systems.
	* Makefile: Made regex.* handled same as obstack.*; made sure tar
	file included everything I wanted it to include (including
	malloc.c).

	* dbxread.c (end_psymtab): Create an entry in the
	partial_symtab_list for each subfile of the .o file just read in.
	This allows a "list expread.y:10" to work when we haven't read in
	expread.o's symbol stuff yet.

	* symtab.h, dbxread.c (psymtab_to_symtab): Recognize pst->ldsymlen
	== 0 as indicating a dummy psymtab, only in existence to cause the
	dependency list to be read in.

	* dbxread.c (sort_symtab_syms): Elminated reversal of symbols to
	make sure that register debug symbol decls always come before
	parameter symbols.  After mod below, this is not needed.

	* symtab.c (lookup_block_symbol): Take parameter type symbols
	(LOC_ARG or LOC_REGPARM) after any other symbols which match.

	* dbxread.c (read_type): When defining a type in terms of some
	other type and the other type is supposed to have a pointer back
	to this specific kind of type (pointer, reference, or function),
	check to see if *that* type has been created yet.  If it has, use
	it and fill in the appropriate slot with a pointer to it.

Mon Dec  5 11:25:04 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* symmisc.c: Eliminated existence of free_inclink_symtabs and
	init_free_inclink_symtabs; they aren't called from anywhere, and
	if they were they could disrupt gdb's data structure badly
	(elimination of struct type's which values that stick around past
	elimination of inclink symtabs).

	* dbxread.c (symbol_file_command): Fixed a return pathway out of
	the routine to do_cleanups before it left.

	* infcmd.c (set_environment_command), gdb.texinfo: Added
	capability to set environmental variable values to null.

	* gdb.texinfo: Modified doc on "break" without args slightly.

Sun Dec  4 17:03:16 1988  Randall Smith  (randy at gluteus.ai.mit.edu)

	* dbxread.c (symbol_file_command): Added check; if there weren't
	any debugging symbols in the file just read, the user is warned.

	* infcmd.c: Commented set_environment_command (a little).

	* createtags: Cleaned up and commented.

	* Makefile: Updated dependency list and cleaned it up somewhat
	(used macros, didn't make .o files depend on .c files, etc.)

Fri Dec  2 11:44:46 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* value.h, values.c, infcmd.c, valops.c, m-i386.h, m-sparc.h,
	m-merlin.h, m-npl.h, m-pn.h, m-umax.h, m-vax.h, m-hp9k320.h,
	m-isi.h, m-news.h, m-sun2.h, m-sun3.h:  Cleaned up dealings with
	functions returning structures.  Specifically: Added a function
	called using_struct_return which indicates whether the function
	being called is using the structure returning conventions or it is
	using the value returning conventions on that machine.  Added a
	macro, STORE_STRUCT_RETURN to store the address of the structure
	to be copied into wherever it's supposed to go, and changed
	call_function to handle all of this correctly.

	* symseg.h, symtab.h, dbxread.c: Added hooks to recognize an
	N_TEXT symbol with name "*gcc-compiled*" as being a flag
	indicating that a file had been compiled with gcc and setting a
	flag in all blocks produced during processing of that file.

Thu Dec  1 13:54:29 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* m-sparc.h (PUSH_DUMMY_FRAME): Saved 8 less than the current pc,
	as POP_FRAME and sparc return convention restore the pc to 8 more
	than the value saved.

	* valops.c, printcmd.c, findvar.c, value.h: Added the routine
	value_from_register, to access a specific register of a specific
	frame as containing a specific type, and used it in read_var_value
	and print_frame_args.

Wed Nov 30 17:39:50 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* dbxread.c (read_number): Will accept either the argument passed
	as an ending character, or a null byte as an ending character.

	* Makefile, createtags: Added entry to create tags for gdb
	distribution which will make sure currently configured machine
	dependent files come first in the list.

Wed Nov 23 13:27:34 1988  Randall Smith  (randy at gluteus.ai.mit.edu)

	* stack.c, infcmd.c, sparc-dep.c: Modified record_selected_frame
	to work off of frame address.

	* blockframe.c (create_new_frame, get_prev_frame_cache_item):
	Added code to reset pointers within frame cache if it must be
	realloc'd. 

	* dbxread.c (read_dbx_symtab): Added in optimization comparing
	last couple of characters instead of first couple to avoid
	strcmp's in read_dbx_symtab (recording extern syms in misc
	functions or not).  1 call to strlen is balanced out by many fewer
	calls to strcmp.

Tue Nov 22 16:40:14 1988  Randall Smith  (randy at cream-of-wheat.ai.mit.edu)

	* dbxread.c (read_dbx_symtab): Took out optimization for ignoring
	LSYM's; was disallowing typedefs.  Silly me.  

	* Checkpointed distribution (mostly for sending to Tiemann).

	* expression.h: Added BINOP_MIN and BINOP_MAX operators for C++.
	* symseg.h: Included flags for types having destructors and
		constructors, and flags being defined via public and via
		virtual paths.  Added fields NEXT_VARIANT, N_BASECLASSES,
		and BASECLASSES to this type (tr: Changed types from
		having to be derived from a single baseclass to a multiple
		base class).
	* symtab.h: Added macros to access new fields defined in symseg.h.
		Added decl for lookup_basetype_type.
	* dbxread.c 
	(condense_addl_misc_bunches): Function added to condense the misc
		function bunches added by reading in a new .o file.
	(read_addl_syms): Function added to read in symbols
		from a new .o file (incremental linking).
	(add_file_command): Command interface function to indicate
		incrmental linking of a new .o file; this now calls
		read_addl_syms and condense_addl_misc_bunches.
	(define_symbol): Modified code to handle types defined from base
		types which were not known when the derived class was
		output.
	(read_struct_type): Modified to better handle description of
		struct types as derived types.  Possibly derived from
		several different base classes.  Also added new code to
		mark definitions via virtual paths or via public paths.
		Killed seperate code to handle classes with destructors
		but without constructors and improved marking of classes
		as having destructors and constructors.
	* infcmd.c: Modified call to val_print (one more argument).
	* symtab.c (lookup_member_type): Modified to deal with new
		structure in symseg.h.
	(lookup_basetype_type): Function added to find or construct a type
		?derived? from the given type.
	(decode_line_1): Modified to deal with new type data structures.
		Modified to deal with new number of args for
		decode_line_2.
	(decode_line_2): Changed number of args (?why?).
	(init_type): Added inits for new C++ fields from
		symseg.h.
	*valarith.c
	(value_x_binop, value_binop): Added cases for BINOP_MIN &
		BINOP_MAX.
	* valops.c
	(value_struct_elt, check_field,	value_struct_elt_for_address):
		Changed to deal with multiple possible baseclasses.
	(value_of_this): Made SELECTED_FRAME an extern variable.
	* valprint.c
	(val_print): Added an argument DEREF_REF to dereference references
		automatically, instead of printing them like pointers.
		Changed number of arguments in recursive calls to itself.
		Changed to deal with varibale numbers of base classes.
	(value_print): Changed number of arguments to val_print.  Print
		type of value also if value is a reference.
	(type_print_derivation_info): Added function to print out
		derivation info a a type.
	(type_print_base): Modified to use type_print_derivation_info and
		to handle multiple baseclasses.
	
Mon Nov 21 10:32:07 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* inflow.c (term_status_command): Add trailing newline to output. 

	* sparc-dep.c (do_save_insn, do_restore_insn): Saved
	"stop_registers" over the call for the sake of normal_stop and
	run_stack_dummy.

	* m-sparc.h (EXTRACT_RETURN_VALUE): Put in parenthesis to force
	addition of 8 to the int pointer, not the char pointer.

	* sparc-pinsn.c (print_addr1): Believe that I have gotten the
	syntax right for loads and stores as adb does it.

	* symtab.c (list_symbols): Turned search for match on rexegp into
	a single loop.

	* dbxread.c (psymtab_to_symtab): Don't read it in if it's already
	been read in.

	* dbxread.c (psymtab_to_symtab): Changed error to fatal in
	psymtab_to_symtab. 

	* expread.y (parse_number): Fixed bug which treated 'l' at end of
	number as '0'.

Fri Nov 18 13:57:33 1988  Randall Smith  (randy at gluteus.ai.mit.edu)

	* dbxread.c (read_dbx_symtab, process_symbol_for_psymtab): Was
	being foolish and using pointers into an array I could realloc.
	Converted these pointers into integers.

Wed Nov 16 11:43:10 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* m-sparc.h (POP_FRAME): Made the new frame be PC_ADJUST of the
	old frame.

	* i386-pinsn.c, m-hp9k320.h, m-isi.h, m-merlin.h, m-news.h,
	m-npl.h, m-pn.h, m-sparc.h, m-sun2.h, m-sun3.h, m-umax.h, m-vax.h:
	Modified POP_FRAME to use the current frame instead of
	read_register (FP_REGNUM) and to flush_cached_frames before
	setting the current frame.  Also added a call to set the current
	frame in those POP_FRAMEs that didn't have it.

	* infrun.c (wait_for_inferior): Moved call to set_current_frame up
	to guarrantee that the current frame will always be set when a
	POP_FRAME is done.  

	* infrun.c (normal_stop): Added something to reset the pc of the
	current frame (was incorrect because of DECR_PC_AFTER_BREAK).

	* valprint.c (val_print): Changed to check to see if a string was
	out of bounds when being printed and to indicate this if so.

	* convex-dep.c (read_inferior_memory): Changed to return the value
	of errno if the call failed (which will be 0 if the call
	suceeded). 

Tue Nov 15 10:17:15 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* infrun.c (wait_for_inferior): Two changes: 1) Added code to
	not trigger the step breakpoint on recursive calls to functions
	without frame info, and 2) Added calls to distinguish recursive
	calls within a function without a frame (which next/nexti might
	wish to step over) from jumps to the beginning of a function
	(which it generally doesn't).

	* m-sparc.h (INIT_EXTRA_FRAME_INFO): Bottom set correctly for leaf
	parents. 

	* blockframe.c (get_prev_frame_cache_item): Put in mod to check
	for a leaf node (by presence or lack of function prologue).  If
	there is a leaf node, it is assumed that SAVED_PC_AFTER_CALL is
	valid.  Otherwise, FRAME_SAVED_PC or read_pc is used.

	* blockframe.c, frame.h: Did final deletion of unused routines and
	commented problems with getting a pointer into the frame cache in
	the frame_info structure comment.

	* blockframe.c, frame.h, stack.c: Killed use of
	frame_id_from_frame_info; used frame_id_from_addr instead.

	* blockframe.c, frame.h, stack.c, others (oops): Combined stack
	cache and frame info structures.

	* blockframe.c, sparc-dep.c, stack.c: Created the function
	create_new_frame and used it in place of bad calls to
	frame_id_from_addr. 

	* blockframe.c, inflow.c, infrun.c, i386-pinsn.c, m-hp9k320.h,
	m-npl.h, m-pn.h, m-sparc.h, m-sun3.h, m-vax.h, default-dep.c,
	convex-dep.c, gould-dep.c, hp9k320-dep.c, news-dep.c, sparc-dep.c,
	sun3-dep.c, umax-dep.c: Killed use of
	set_current_Frame_by_address.  Used set_current_frame
	(create_new_frame...) instead.

	* frame.h: Killed use of FRAME_FP_ID.

	* infrun.c, blockframe.c: Killed select_frame_by_address.  Used
	select_frame (get_current_frame (), 0) (which was correct in all
	cases that we need to worry about.

Mon Nov 14 14:19:32 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* frame.h, blockframe.c, stack.c, m-sparc.h, sparc-dep.c: Added
	mechanisms to deal with possible specification of frames
	dyadically. 

Sun Nov 13 16:03:32 1988  Richard Stallman  (rms at sugar-bombs.ai.mit.edu)

	* ns32k-opcode.h: Add insns acbw, acbd.

Sun Nov 13 15:09:58 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* breakpoint.c: Changed breakpoint structure to use the address of
	a given frame (constant across inferior runs) as the criteria for
	stopping instead of the frame ident (which varies across inferior
	calls). 

Fri Nov 11 13:00:22 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* gld-pinsn.c (findframe): Modified to work with the new frame
	id's.  Actually, it looks as if this routine should be called with
	an address anyway.

	* findvar.c (find_saved_register): Altered bactrace loop to work
	off of frames and not frame infos.

	* frame.h, blockframe.c, stack.c, sparc-dep.c, m-sparc.h: Changed
	FRAME from being the address of the frame to being a simple ident
	which is an index into the frame_cache_item list.
	* convex-dep.c, default-dep.c, gould-dep.c, hp9k320-dep.c,
	i386-pinsn.c, inflow.c, infrun.c, news-dep.c, sparc-dep.c,
	sun3-dep.c, umax-dep.c, m-hp9k320.h, m-npl.h, m-pn.h, m-sparc.h,
	m-sun3.h, m-vax.h: Changed calls of the form set_current_frame
	(read_register (FP_REGNUM)) to set_current_frame_by_address (...). 

Thu Nov 10 16:57:57 1988  Randall Smith  (randy at gluteus.ai.mit.edu)

	* frame.h, blockframe.c, gld-pinsn.c, sparc-dep.c, stack.c,
	infrun.c, findvar.c, m-sparc.h: Changed the FRAME type to be
	purely an identifier, using FRAME_FP and FRAME_FP_ID to convert
	back and forth between the two.  The identifier is *currently*
	still the frame pointer value for that frame.

Wed Nov  9 17:28:14 1988  Chris Hanson  (cph at kleph)

	* m-hp9k320.h (FP_REGISTER_ADDR): Redefine this to return
	difference between address of given FP register, and beginning of
	`struct user' that it occurs in.

	* hp9k320-dep.c (core_file_command): Fix sign error in size
	argument to myread.  Change buffer argument to pointer; was
	copying entire structure.
	(fetch_inferior_registers, store_inferior_registers): Replace
	occurrences of `FP_REGISTER_ADDR_DIFF' with `FP_REGISTER_ADDR'.
	Flush former definition.

Wed Nov  9 12:11:37 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* xgdb.c: Killed include of initialize.h.

	* Pulled in xgdb.c from the net.

	* Checkpointed distribution (to provide to 3b2 guy).

	* coffread.c, dbxread.c, symmisc.c, symtab.c, symseg.h: Changed
	format of table of line number--pc mapping information.  Can
	handle negative pc's now.

	* command.c: Deleted local copy of savestring; code in utils.c is
	identical. 

Tue Nov  8 11:12:16 1988  Randall Smith  (randy at plantaris.ai.mit.edu)

	* gdb.texinfo: Added documentation for shell escape.

Mon Nov  7 12:27:16 1988  Randall Smith  (randy at sugar-bombs.ai.mit.edu)

	* command.c: Added commands for shell escape.

	* core.c, dbxread.c: Added ROBOTUSSIN mods.

	* Checkpointed distribution.

	* printcmd.c (x_command): Yanked error if there is no memory to
	examine (could be looking at executable straight).

	* sparc-pinsn.c (print_insn): Amount to leftshift sethi imm by is
	now 10 (matches adb in output).

	* printcmd.c (x_command): Don't attempt to set $_ & $__ if there
	is no last_examine_value (can happen if you did an x/0).

Fri Nov  4 13:44:49 1988  Randall Smith  (randy at gluteus.ai.mit.edu)

	* printcmd.c (x_command): Error if there is no memory to examine.

	* gdb.texinfo: Added "cont" to the command index.

	* sparc-dep.c (do_save_insn): Fixed typo in shift amount.

	* m68k-opcode.h: Fixed opcodes for 68881.

	* breakpoint.c, infcmd.c, source.c: Changed defaults in several
	places for decode_line_1 to work off of the default_breakpoint_*
	values instead of current_source_* values (the current_source_*
	values are off by 5 or so because of listing defaults).

	* stack.c (frame_info): ifdef'd out FRAME_SPECIFCATION_DYADIC in
	the stack.c module.  If I can't do this right, I don't want to do
	it at all.  Read the comment there for more info.

Mon Oct 31 16:23:06 1988  Randall Smith  (randy at gluteus.ai.mit.edu)

	* gdb.texinfo: Added documentation on the "until" command.

Sat Oct 29 17:47:10 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* breakpoint.c, infcmd.c: Added UNTIL_COMMAND and subroutines of
	it. 

	* breakpoint.c, infcmd.c, infrun.c: Added new field to breakpoint
	structure (silent, indicating a silent breakpoint), and modified
	breakpoint_stop_status and things that read it's return value to
	understand it.

Fri Oct 28 17:45:33 1988  Randall Smith  (randy at gluteus.ai.mit.edu)

	* dbxread.c, symmisc.c: Assorted speedups for readin, including
	special casing most common symbols, and doing buffering instead of
	calling malloc.  

Thu Oct 27 11:11:15 1988  Randall Smith  (randy at gluteus.ai.mit.edu)

	* stack.c, sparc-dep.c, m-sparc.h: Modified to allow "info frame"
	to take two arguments on the sparc and do the right thing with
	them. 

	* dbxread.c (read_dbx_symtab, process_symbol_for_psymtab): Put
	stuff to put only symbols that didn't have debugging info on the
	misc functions list back in.

Wed Oct 26 10:10:32 1988  Randall Smith  (randy at gluteus.ai.mit.edu)

	* valprint.c (type_print_varspec_suffix): Added check for
	TYPE_LENGTH(TYPE_TARGET_TYPE(type)) > 0 to prevent divide by 0.

	* printcmd.c (print_formatted): Added check for VALUE_REPEATED;
	value_print needs to be called for that.

	* infrun.c (wait_for_inferior): Added break when you decide to
	stop on a null function prologue rather than continue stepping.

	* m-sun3.h: Added explanatory comment to REGISTER_RAW_SIZE.

	* expread.y (parse_c_1): Initialized paren_depth for each parse.

Tue Oct 25 14:19:38 1988  Randall Smith  (randy at gluteus.ai.mit.edu)

	* valprint.c, coffread.c, dbxread.c: Enum constant values in enum
	type now accessed through TYPE_FIELD_BITPOS.

	* dbxread.c (process_symbol_for_psymtab): Added code to deal with
	possible lack of a ":" in a debugging symbol (do nothing).

	* symtab.c (decode_line_1): Added check in case of all numbers for
	complete lack of symbols.

	* source.c (select_source_symtab): Made sure that this wouldn't
	bomb on complete lack of symbols.

Mon Oct 24 12:28:29 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* m-sparc.h, findvar.c: Ditched REGISTER_SAVED_UNIQUELY and based
	code on REGISTER_IN_WINDOW_P and HAVE_REGISTER_WINDOWS.  This will
	break when we find a register window machine which saves the
	window registers within the context of an inferior frame.

	* sparc-dep.c (frame_saved_pc): Put PC_ADJUST return back in for
	frame_saved_pc.  Seems correct.

	* findvar.c, m-sparc.h: Created the macro REGISTER_SAVED_UNIQUELY
	to handle register window issues (ie. that find_saved_register
	wasn't checking the selected frame itself for shit). 

	* sparc-dep.c (core_file_command): Offset target of o & g register
	bcopy by 1 to hit correct registers.

	* m-sparc.h: Changed STACK_END_ADDR.

Sun Oct 23 19:41:51 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* sparc-dep.c (core_file_command): Added in code to get the i & l
	registers from the stack in the corefile, and blew away some wrong
	code to get i & l from inferior.

Fri Oct 21 15:09:19 1988  Randall Smith  (randy at apple-gunkies.ai.mit.edu)

	* m-sparc.h (PUSH_DUMMY_FRAME): Saved the value of the RP register
	in the location reserved for i7 (in the created frame); this way
	the rp value won't get lost.  The pc (what we put into the rp in
	this routine) gets saved seperately, so we loose no information.

	* sparc-dep.c (do_save_insn & do_restore_insn): Added a wrapper to
	preserve the proceed status state variables around each call to
	proceed (the current frame was getting munged because this wasn't
	being done).

	* m-sparc.h (FRAME_FIND_SAVED_REGS): Fix bug: saved registers
	addresses were being computed using absolute registers number,
	rather than numbers relative to each group of regs.

	* m-sparc.h (POP_FRAME): Fixed a bug (I hope) in the context
	within which saved reg numbers were being interpetted.  The
	values to be restored were being gotten in the inferior frame, and
	the restoring was done in the superior frame.  This means that i
	registers must be restored into o registers.

	* sparc-dep.c (do_restore_insn): Modified to take a pc as an
	argument, instead of a raw_buffer.  This matches (at least it
	appears to match) usage from POP_FRAME, which is the only place
	from which do_restore_insn is called.

	* sparc-dep.c (do_save_insn and do_restore_insn): Added comments.

	* m-sparc.h (FRAME_FIND_SAVED_REGS): Modified my code to find the
	save addresses of out registers to use the in regs off the stack
	pointer when the current frame is 1 from the innermost.

Thu Oct 20 13:56:15 1988  & Smith  (randy at hobbes.ai.mit.edu)

	* blockframe.c, m-sparc.h: Removed code associated with
	GET_PREV_FRAME_FROM_CACHE_ITEM.  This code was not needed for the
	sparc; you can always find the previous frames fp from the fp of
	the current frame (which is the sp of the previous).  It's getting
	the information associated with a given frame (ie. saved
	registers) that's a bitch, because that stuff is saved relative to
	the stack pointer rather than the frame pointer.

	* m-sparc.h (GET_PREV_FRAME_FROM_CACHE_ITEM): Modified to return
	the frame pointer of the previous frame instead of the stack
	pointer of same.

	* blockframe.c (flush_cached_frames): Modified call to
	obstack_free to free back to frame_cache instead of back to zero.
	This leaves the obstack control structure in finite state (and
	still frees the entry allocated at frame_cache).

Sat Oct 15 16:30:47 1988  & Smith  (randy at tartarus.uchicago.edu)

	* valops.c (call_function): Suicide material here.  Fixed a typo;
	CALL_DUMMY_STACK_ADJUST was spelled CAll_DUMMY_STACK_ADJUST on
	line 530 of the file.  This cost me three days.  I'm giving up
	typing for lent.

Fri Oct 14 15:10:43 1988  & Smith  (randy at tartarus.uchicago.edu)

	* m-sparc.h: Corrected a minor mistake in the dummy frame code
	that was getting the 5th argument and the first argument from the
	same place.

Tue Oct 11 11:49:33 1988  & Smith  (randy at tartarus.uchicago.edu)

	* infrun.c: Made stop_after_trap and stop_after_attach extern
	instead of static so that code which used proceed from machine
	dependent files could fiddle with them.

	* blockframe.c, frame.h, sparc-dep.c, m-sparc.h: Changed sense of
	->prev and ->next in struct frame_cache_item to fit usage in rest
	of gdb (oops).

Mon Oct 10 15:32:42 1988  Randy Smith  (randy at gargoyle.uchicago.edu)

	* m-sparc.h, sparc-dep.c, blockframe.c, frame.h: Wrote
	get_frame_cache_item.  Modified FRAME_SAVED_PC and frame_saved_pc
	to take only one argument and do the correct thing with it.  Added
	the two macros I recently defined in blockframe.c to m-sparc.h.
	Have yet to compile this thing on a sparc, but I've now merged in
	everything that I received from tiemann, either exactly, or simply
	effectively. 

	* source.c: Added code to allocated space to sals.sals in the case
	where no line was specified.

	* blockframe.c, infrun.c: Modified to cache stack frames requested
	to minimize accesses to subprocess.

Tue Oct  4 15:10:39 1988  Randall Smith  (randy at cream-of-wheat.ai.mit.edu)

	* config.gdb: Added sparc.

Mon Oct  3 23:01:22 1988  Randall Smith  (randy at cream-of-wheat.ai.mit.edu)

	* Makefile, blockframe.c, command.c, core.c, dbxread.c, defs.h,
	expread.y, findvar.c, infcmd.c, inflow.c, infrun.c, sparc-pinsn.c,
	m-sparc.h, sparc-def.c, printcmd.c, stack.c, symmisc.c, symseg.h,
	valops.c, values.c: Did initial merge of sparc port.  This will
	not compile; have to do stack frame caching and finish port.

	* inflow.c, gdb.texinfo: `tty' now resets the controling terminal. 

Fri Sep 30 11:31:16 1988  Randall Smith  (randy at gluteus.ai.mit.edu)

	* inferior.h, infcmd.c, infrun.c: Changed the variable
	stop_random_signal to stopped_by_random signal to fit in better
	with name conventions (variable is not a direction to the
	proceed/resume set; it is information from it).

Thu Sep 29 13:30:46 1988  Randall Smith  (randy at cream-of-wheat.ai.mit.edu)

	* infcmd.c (finish_command): Value type of return value is now
	whatever the function returns, not the type of the function (fixed
	a bug in printing said value).

	* dbxread.c (read_dbx_symtab, process_symbol_for_psymtab):
	Put *all* global symbols into misc_functions.  This is what was
	happening anyway, and we need it for find_pc_misc_function.

	** This was eventually taken out, but I didn't mark it in the
	ChangeLog.  Oops.

	* dbxread.c (process_symbol_for_psymtab): Put every debugger
	symbol which survives the top case except for constants on the
	symchain.  This means that all of these *won't* show up in misc
	functions (this will be fixed once I make sure it's broken the way
	it's supposed to be).

	* dbxread.c: Modified placement of debugger globals onto the hash
	list; now we exclude the stuff after the colon and don't skip the
	first character (debugger symbols don't have underscores).

	* dbxread.c: Killed debuginfo stuff with ifdef's.

Wed Sep 28 14:31:51 1988  Randall Smith  (randy at cream-of-wheat.ai.mit.edu)

	* symtab.h, dbxread.c: Modified to deal with BINCL, EINCL, and
	EXCL symbols produced by the sun loader by adding a list of
	pre-requisite partial_symtabs that each partial symtab needs.

	* symtab.h, dbxread.c, symtab.c, symmisc.c: Modified to avoid
	doing a qsort on the local (static) psymbols for each file to
	speed startup.  This feature is not completely debugged, but it's
	inclusion has forced the inclusion of another feature (dealing
	with EINCL's, BINCL's and EXCL's) and so I'm going to go in and
	deal with them.

	* dbxread.c (process_symbol_for_psymtab): Made sure that the class
	of the symbol made it into the partial_symbol entry.

Tue Sep 27 15:10:26 1988  Randall Smith  (randy at gluteus.ai.mit.edu)

	* dbxread.c: Fixed bug; init_psymbol_list was not being called
	with the right number of arguments (1).

	* dbxread.c: Put ifdef's around N_MAIN, N_M2C, and N_SCOPE to
	allow compilation on a microvax.

	* config.gdb: Modified so that "config.gdb vax" would work.

	* dbxread.c, symtab.h, symmisc.h, symtab.c, source.c: Put in many
	and varied hacks to speed up gdb startup including: A complete
	rewrite of read_dbx_symtab, a modification of the partial_symtab
	data type, deletion of select_source_symtab from
	symbol_file_command, and optimiztion of the call to strcmp in
	compare_psymbols. 

Thu Sep 22 11:08:54 1988  Randall Smith  (randy at gluteus.ai.mit.edu)

	* dbxread.c (psymtab_to_symtab): Removed call to
	init_misc_functions. 

	* dbxread.c: Fixed enumeration type clash (used enum instead of
	integer constant).

	* breakpoint.c: Fixed typo; lack of \ at end of line in middle of
	string constant. 

	* symseg.h: Fixed typo; lack of semicolon after structure
	definition. 

	* command.c, breakpoint.c, printcmd.c: Added cmdlist editing
	functions to add commands with the abbrev flag set.  Changed
	help_cmd_list to recognize this flag and modified unset,
	undisplay, and enable, disable, and delete breakpoints to have
	this flag set.

Wed Sep 21 13:34:19 1988  Randall Smith  (randy at plantaris.ai.mit.edu)

	* breakpoint.c, infcmd.c, gdb.texinfo: Created "unset" as an alias
	for delete, and changed "unset-environment" to be the
	"environment" subcommand of "delete".

	* gdb.texinfo, valprint.c: Added documentation in the manual for
	breaking the set-* commands into subcommands of set.  Changed "set
	maximum" to "set array-max".

	* main.c, printcmd.c, breakpoint.c: Moved the declaration of
	command lists into main and setup a function in main initializing
	them to guarrantee that they would be initialized before calling
	any of the individual files initialize routines.

	* command.c (lookup_cmd): A null string subcommand is treated as
	an unknown subcommand rather than an ambiguous one (eg. "set $x =
	1" will now work).

	* infrun.c (wait_for_inferior): Put in ifdef for Sony News in
	check for trap by INNER_THAN macro.

	* eval.c (evaluate_subexp): Put in catch to keep the user from
	attempting to call a non function as a function.

Tue Sep 20 10:35:53 1988  Randall Smith  (randy at oatmeal.ai.mit.edu)

	* dbxread.c (read_dbx_symtab): Installed code to keep track of
	which global symbols did not have debugger symbols refering to
	them, and recording these via record_misc_function.

	* dbxread.c: Killed code to check for extra global symbols in the
	debugger symbol table.

	* printcmd.c, breakpoint.c: Modified help entries for several
	commands to make sure that abbreviations were clearly marked and
	that the right commands showed up in the help listings.

	* main.c, command.c, breakpoint.c, infcmd.c, printcmd.c,
	valprint.c, defs.h: Modified help system to allow help on a class
	name to show subcommands as well as commands and help on a command
	to show *all* subcommands of that command.

Fri Sep 16 16:51:19 1988  Randall Smith  (randy at gluteus.ai.mit.edu)

	* breakpoint.c (_initialize_breakpoint): Made "breakpoints"
	subcommands of enable, disable, and delete use class 0 (ie. they
	show up when you do a help xxx now).

	* infcmd.c,printcmd,c,main.c,valprint.c: Changed the set-*
	commands into subcommands of set.  Created "set variable" for use
	with variables whose names might conflict with other subcommands.

	* blockframe.c, dbxread.c, coffread.c, expread.y, source.c:
	Fixed mostly minor (and one major one in block_for_pc) bugs
	involving checking the partial_symtab_list when a scan through the
	symtab_list fails.

Wed Sep 14 12:02:05 1988  Randall Smith  (randy at sugar-smacks.ai.mit.edu)

	* breakpoint.c, gdb.texinfo: Added enable breakpoints, disable
	breakpoints and delete breakpoints as synonyms for enable,
	disable, and delete.  This seemed reasonable because of the
	immeninent arrival of watchpoints & etc.

	* gdb.texinfo: Added enable display, disable display, and delete
	display to manual.

Tue Sep 13 16:53:56 1988  Randall Smith  (randy at sugar-smacks.ai.mit.edu)

	* inferior.h, infrun.c, infcmd.c: Added variable
	stop_random_signal to indicate when a proceed had been stopped by
	an unexpected signal.  Used this to determine (in normal_stop)
	whether the current display point should be deleted.

	* valops.c: Fix to value_ind to check for reference before doing a
	COERCE_ARRAY.

Sun Jul 31 11:42:36 1988  Richard Stallman  (rms at frosted-flakes.ai.mit.edu)

	* breakpoint.c (_initialize_breakpoint): Clean up doc for commands
	that can now apply also to auto-displays.

	* coffread.c (record_line): Corrected a spazz in editing.
	Also removed the two lines that assume line-numbers appear
	only in increasing order.

Tue Jul 26 22:19:06 1988  Peter TerMaat  (pete at corn-chex.ai.mit.edu)

	* expression.h, eval.c, expprint.c, printcmd.c, valarith.c,
	valops.c, valprint.c, values.c, m-*.h: Changes for evaluating and
	displaying 64-bit `long long' integers.  Each machine must define
	a LONGEST type, and a BUILTIN_TYPE_LONGEST.

	* symmisc.c: (print_symtab) check the status of the fopen and call
	perror_with_name if needed.

Thu Jul 21 00:56:11 1988  Peter TerMaat  (pete at corn-chex.ai.mit.edu)

	* Convex: core.c: changes required by Convex's SOFF format were
	isolated in convex-dep.c.

Wed Jul 20 21:26:10 1988  Peter TerMaat  (pete at corn-chex.ai.mit.edu)

	* coffread.c, core.c, expread.y, i386-pinsn.c, infcmd.c, inflow.c,
	infrun.c, m-i386.h, main.c, remote.c, source.c, valops.c:
	Improvements for the handling of the i386 and other machines
	running USG.  (Several of these files just needed extra header files
	such as types.h.) utils.c: added bcopy, bcmp, bzero, getwd, list
	of signals, and queue routines for USG systems.  Added vfork macro
	to i386

	* printcmd.c, breakpoint.c: New commands to enable/disable
	auto-displays.  Also `delete display displaynumber' works like
	`undisplay displaynumber'.

Tue Jul 19 02:17:18 1988  Peter TerMaat  (pete at corn-chex.ai.mit.edu)

	* coffread.c: (coff_lookup_type)  Wrong portion of type_vector was
	being bzero'd after type_vector was reallocated.

	* printcmd.c: (delete_display) Check for a display chain before
	attempting to delete a display.

	* core.c, *-dep.c (*-infdep moved to *-dep): machine-dependent
	parts of core.c (core_file_command, exec_file_command) moved to
	*-dep.c. 

Mon Jul 18 19:45:51 1988  Peter TerMaat  (pete at corn-chex.ai.mit.edu)

	* dbxread.c: typo in read_struct_type (missing '=') was causing a
	C struct to be parsed as a C++ struct, resulting in a `invalid
	character' message.

Sun Jul 17 22:27:32 1988  Peter TerMaat  (pete at corn-chex.ai.mit.edu)

	* printcmd.c, symtab.c, valops.c, expread.y: When an expression is
	read, the innermost block required to evaluate the expression is
	saved in the global variable `innermost_block'.  This information
	is saved in the `block' field of an auto-display so that
	expressions with inactive variables can be skipped.  `info display'
	tells the user which displays are active and which are not.  New
	fn `contained_in' returns nonzero if one block is contained within
	another. 

Fri Jul 15 01:53:14 1988  Peter TerMaat  (pete at corn-chex.ai.mit.edu)

	* infrun.c, m-i386.h: Use macro TRAPS_EXPECTED to set number of
	traps to skip when sh execs the program.  Default is 2, m-i386.h
	overrides this and sets to 4.

	* coffread.c, infrun.c: minor changes for the i386.  May be able
	to eliminate them with more general code.

	* default-infdep.c: #ifdef SYSTEMV, include header file types.h.
	Also switched the order of signal.h and user.h, since System 5
	requires signal.h to come first.

	* core.c main.c, remote,c, source.c, inflow.c: #ifdef SYSTEMV,
	include various header files.  Usually types.h and fcntl.h.

	* utils.c: added queue routines needed by the i386 (and other sys
	5 machines).

	* sys5.c, regex.c, regex.h: new files for sys 5 systems.  (The
	regex files are simply links to /gp/gnu/lib.)

Thu Jul 14 01:47:14 1988  Peter TerMaat  (pete at corn-chex.ai.mit.edu)

	* config.gdb, README: Provide a list of known machines when user
	enters an invalid machine.  New second arg is operating system,
	currently only used with `sunos4' or `os4'. Entry for i386 added.

	* news-infdep.c: new file.

	* m-news.h: new version which deals with new bugs in news800's OS.

Tue Jul 12 19:52:16 1988  Peter TerMaat  (pete at corn-chex.ai.mit.edu)

	* Makefile, *.c, munch, config.gdb, README: New initialization
	scheme uses nm to find functions whose names begin with
	`_initialize_'.  Files `initialize.h', `firstfile.c',
	`lastfile.c', `m-*init.h' no longer needed. 

	* eval.c, symtab.c, valarith.c, valops.c, value.h, values.c: Bug
	fixes from gdb+ 2.5.4.  evaluate_subexp takes a new arg, type
	expected. New fn value_virtual_fn_field.

Mon Jul 11 00:48:49 1988  Peter TerMaat  (pete at corn-chex.ai.mit.edu)

	* core.c (read_memory): xfer_core_file was being called with an
	extra argument (0) by read_memory.

	* core.c (read_memory), *-infdep.c (read_inferior_memory),
	valops.c (value_at): read_memory and read_inferior_memory now work
	like write_memory and write_inferior_memory in that errno is
	checked after each ptrace and returned to the caller.  Used in
	value_at to detect references to addresses which are out of
	bounds.  Also core.c (xfer_core_file): return 1 if invalid
	address, 0 otherwise.

	* inflow.c, <machine>-infdep.c: removed all calls to ptrace from
	inflow.c and put them in machine-dependent files *-infdep.c.

Sun Jul 10 19:19:36 1988  Peter TerMaat  (pete at corn-chex.ai.mit.edu)

	* symmisc.c: (read_symsegs) Accept only format number 2.  Since
	the size of the type structure changed when C++ support was added,
	format 1 can no longer be used.

	* core.c, m-sunos4.h: (core_file_command) support for SunOS 4.0.
	Slight change in the core structure.  #ifdef SUNOS4.  New file
	m-sunos4.h.  May want to change config.gdb also.

Fri Jul  8 19:59:49 1988  Peter TerMaat  (pete at corn-chex.ai.mit.edu)

	* breakpoint.c: (break_command_1) Allow `break if condition'
	rather than parsing `if' as a function name and returning an
	error.

Thu Jul  7 22:22:47 1988  Peter TerMaat  (pete at corn-chex.ai.mit.edu)

	* C++: valops.c, valprint.c, value.h, values.c: merged code to deal
	with C++ expressions.

Wed Jul  6 03:28:18 1988  Peter TerMaat  (pete at corn-chex.ai.mit.edu)

        * C++: dbxread.c: (read_dbx_symtab, condense_misc_bunches,
	add_file_command)  Merged code to read symbol information from
	an incrementally linked file.  symmisc.c:
	(init_free_inclink_symtabs, free_inclink_symtabs) Cleanup
	routines.

Tue Jul  5 02:50:41 1988  Peter TerMaat  (pete at corn-chex.ai.mit.edu)

	* C++: symtab.c, breakpoint.c, source.c:  Merged code to deal with
	ambiguous line specifications.  In C++ one can have overloaded
	function names, so that `list classname::overloadedfuncname'
	refers to several different lines, possibly in different files.

Fri Jul  1 02:44:20 1988  Peter TerMaat  (pete at corn-chex.ai.mit.edu)

	* C++: symtab.c: replaced lookup_symtab_1 and lookup_symtab_2 with
	a modified lookup_symbol which checks for fields of the current
	implied argument `this'.  printcmd.c, source.c, symtab.c,
	valops.c: Need to change callers once callers are
	installed. 

Wed Jun 29 01:26:56 1988  Peter TerMaat  (pete at frosted-flakes.ai.mit.edu)

	* C++: eval.c, expprint.c, expread.y, expression.h, valarith.c, 
	Merged code to deal with evaluation of user-defined operators,
	member functions, and virtual functions.
	binop_must_be_user_defined tests for user-defined binops, 
	value_x_binop calls the appropriate operator function. 

Tue Jun 28 02:56:42 1988  Peter TerMaat  (pete at frosted-flakes.ai.mit.edu)

	* C++: Makefile: changed the echo: expect 101 shift/reduce conflicts 
	and 1 reduce/reduce conflict.

Local Variables:
mode: indented-text
left-margin: 8
fill-column: 74
version-control: never
End:
