/* Generated automatically by the program `gentarget-def'.  */
#ifndef GCC_INSN_TARGET_DEF_H
#define GCC_INSN_TARGET_DEF_H

static inline rtx_insn *
insnify (rtx x)
{
  if (!x)
    return NULL;
  if (rtx_insn *insn = dyn_cast <rtx_insn *> (x))
    return insn;
  start_sequence ();
  emit (x, false);
  rtx_insn *res = get_insns ();
  end_sequence ();
  return res;
}

static rtx_insn *
invalid_rtx_rtx (rtx, rtx)
{
  gcc_unreachable ();
}

#undef TARGET_HAVE_ALLOCATE_STACK
#define TARGET_HAVE_ALLOCATE_STACK hook_bool_void_false
#undef TARGET_GEN_ALLOCATE_STACK
#define TARGET_GEN_ALLOCATE_STACK invalid_rtx_rtx
#undef TARGET_CODE_FOR_ALLOCATE_STACK
#define TARGET_CODE_FOR_ALLOCATE_STACK CODE_FOR_nothing

static bool
target_have_atomic_test_and_set (void)
{
  return 
#line 62 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/sync.md"
(ISA_HAS_TAS);
}

static rtx_insn *
target_gen_atomic_test_and_set (rtx x0, rtx x1, rtx x2)
{
  gcc_checking_assert (targetm.have_atomic_test_and_set ());
  return insnify (gen_atomic_test_and_set (x0, x1, x2));
}

#undef TARGET_HAVE_ATOMIC_TEST_AND_SET
#define TARGET_HAVE_ATOMIC_TEST_AND_SET target_have_atomic_test_and_set
#undef TARGET_GEN_ATOMIC_TEST_AND_SET
#define TARGET_GEN_ATOMIC_TEST_AND_SET target_gen_atomic_test_and_set
#undef TARGET_CODE_FOR_ATOMIC_TEST_AND_SET
#define TARGET_CODE_FOR_ATOMIC_TEST_AND_SET CODE_FOR_atomic_test_and_set

static rtx_insn *
invalid_rtx (rtx)
{
  gcc_unreachable ();
}

#undef TARGET_HAVE_BUILTIN_LONGJMP
#define TARGET_HAVE_BUILTIN_LONGJMP hook_bool_void_false
#undef TARGET_GEN_BUILTIN_LONGJMP
#define TARGET_GEN_BUILTIN_LONGJMP invalid_rtx
#undef TARGET_CODE_FOR_BUILTIN_LONGJMP
#define TARGET_CODE_FOR_BUILTIN_LONGJMP CODE_FOR_nothing

#undef TARGET_HAVE_BUILTIN_SETJMP_RECEIVER
#define TARGET_HAVE_BUILTIN_SETJMP_RECEIVER hook_bool_void_false
#undef TARGET_GEN_BUILTIN_SETJMP_RECEIVER
#define TARGET_GEN_BUILTIN_SETJMP_RECEIVER invalid_rtx
#undef TARGET_CODE_FOR_BUILTIN_SETJMP_RECEIVER
#define TARGET_CODE_FOR_BUILTIN_SETJMP_RECEIVER CODE_FOR_nothing

#undef TARGET_HAVE_BUILTIN_SETJMP_SETUP
#define TARGET_HAVE_BUILTIN_SETJMP_SETUP hook_bool_void_false
#undef TARGET_GEN_BUILTIN_SETJMP_SETUP
#define TARGET_GEN_BUILTIN_SETJMP_SETUP invalid_rtx
#undef TARGET_CODE_FOR_BUILTIN_SETJMP_SETUP
#define TARGET_CODE_FOR_BUILTIN_SETJMP_SETUP CODE_FOR_nothing

#undef TARGET_HAVE_CANONICALIZE_FUNCPTR_FOR_COMPARE
#define TARGET_HAVE_CANONICALIZE_FUNCPTR_FOR_COMPARE hook_bool_void_false
#undef TARGET_GEN_CANONICALIZE_FUNCPTR_FOR_COMPARE
#define TARGET_GEN_CANONICALIZE_FUNCPTR_FOR_COMPARE invalid_rtx_rtx
#undef TARGET_CODE_FOR_CANONICALIZE_FUNCPTR_FOR_COMPARE
#define TARGET_CODE_FOR_CANONICALIZE_FUNCPTR_FOR_COMPARE CODE_FOR_nothing

static rtx_insn *
target_gen_call (rtx x0, rtx opt1, rtx, rtx)
{
  return insnify (gen_call (x0, opt1));
}

#undef TARGET_HAVE_CALL
#define TARGET_HAVE_CALL hook_bool_void_true
#undef TARGET_GEN_CALL
#define TARGET_GEN_CALL target_gen_call
#undef TARGET_CODE_FOR_CALL
#define TARGET_CODE_FOR_CALL CODE_FOR_call

static rtx_insn *
invalid_rtx_rtx_rtx_rtx (rtx, rtx, rtx, rtx)
{
  gcc_unreachable ();
}

#undef TARGET_HAVE_CALL_POP
#define TARGET_HAVE_CALL_POP hook_bool_void_false
#undef TARGET_GEN_CALL_POP
#define TARGET_GEN_CALL_POP invalid_rtx_rtx_rtx_rtx
#undef TARGET_CODE_FOR_CALL_POP
#define TARGET_CODE_FOR_CALL_POP CODE_FOR_nothing

static rtx_insn *
target_gen_call_value (rtx x0, rtx x1, rtx opt2, rtx, rtx)
{
  return insnify (gen_call_value (x0, x1, opt2));
}

#undef TARGET_HAVE_CALL_VALUE
#define TARGET_HAVE_CALL_VALUE hook_bool_void_true
#undef TARGET_GEN_CALL_VALUE
#define TARGET_GEN_CALL_VALUE target_gen_call_value
#undef TARGET_CODE_FOR_CALL_VALUE
#define TARGET_CODE_FOR_CALL_VALUE CODE_FOR_call_value

static rtx_insn *
invalid_rtx_rtx_rtx_rtx_rtx (rtx, rtx, rtx, rtx, rtx)
{
  gcc_unreachable ();
}

#undef TARGET_HAVE_CALL_VALUE_POP
#define TARGET_HAVE_CALL_VALUE_POP hook_bool_void_false
#undef TARGET_GEN_CALL_VALUE_POP
#define TARGET_GEN_CALL_VALUE_POP invalid_rtx_rtx_rtx_rtx_rtx
#undef TARGET_CODE_FOR_CALL_VALUE_POP
#define TARGET_CODE_FOR_CALL_VALUE_POP CODE_FOR_nothing

#undef TARGET_HAVE_CASESI
#define TARGET_HAVE_CASESI hook_bool_void_false
#undef TARGET_GEN_CASESI
#define TARGET_GEN_CASESI invalid_rtx_rtx_rtx_rtx_rtx
#undef TARGET_CODE_FOR_CASESI
#define TARGET_CODE_FOR_CASESI CODE_FOR_nothing

#undef TARGET_HAVE_CHECK_STACK
#define TARGET_HAVE_CHECK_STACK hook_bool_void_false
#undef TARGET_GEN_CHECK_STACK
#define TARGET_GEN_CHECK_STACK invalid_rtx
#undef TARGET_CODE_FOR_CHECK_STACK
#define TARGET_CODE_FOR_CHECK_STACK CODE_FOR_nothing

#undef TARGET_HAVE_CLEAR_CACHE
#define TARGET_HAVE_CLEAR_CACHE hook_bool_void_false
#undef TARGET_GEN_CLEAR_CACHE
#define TARGET_GEN_CLEAR_CACHE invalid_rtx_rtx
#undef TARGET_CODE_FOR_CLEAR_CACHE
#define TARGET_CODE_FOR_CLEAR_CACHE CODE_FOR_nothing

#undef TARGET_HAVE_DOLOOP_BEGIN
#define TARGET_HAVE_DOLOOP_BEGIN hook_bool_void_false
#undef TARGET_GEN_DOLOOP_BEGIN
#define TARGET_GEN_DOLOOP_BEGIN invalid_rtx_rtx
#undef TARGET_CODE_FOR_DOLOOP_BEGIN
#define TARGET_CODE_FOR_DOLOOP_BEGIN CODE_FOR_nothing

#undef TARGET_HAVE_DOLOOP_END
#define TARGET_HAVE_DOLOOP_END hook_bool_void_false
#undef TARGET_GEN_DOLOOP_END
#define TARGET_GEN_DOLOOP_END invalid_rtx_rtx
#undef TARGET_CODE_FOR_DOLOOP_END
#define TARGET_CODE_FOR_DOLOOP_END CODE_FOR_nothing

#undef TARGET_HAVE_EH_RETURN
#define TARGET_HAVE_EH_RETURN hook_bool_void_false
#undef TARGET_GEN_EH_RETURN
#define TARGET_GEN_EH_RETURN invalid_rtx
#undef TARGET_CODE_FOR_EH_RETURN
#define TARGET_CODE_FOR_EH_RETURN CODE_FOR_nothing

static rtx_insn *
target_gen_epilogue (void)
{
  return insnify (gen_epilogue ());
}

#undef TARGET_HAVE_EPILOGUE
#define TARGET_HAVE_EPILOGUE hook_bool_void_true
#undef TARGET_GEN_EPILOGUE
#define TARGET_GEN_EPILOGUE target_gen_epilogue
#undef TARGET_CODE_FOR_EPILOGUE
#define TARGET_CODE_FOR_EPILOGUE CODE_FOR_epilogue

static rtx_insn *
invalid_void (void)
{
  gcc_unreachable ();
}

#undef TARGET_HAVE_EXCEPTION_RECEIVER
#define TARGET_HAVE_EXCEPTION_RECEIVER hook_bool_void_false
#undef TARGET_GEN_EXCEPTION_RECEIVER
#define TARGET_GEN_EXCEPTION_RECEIVER invalid_void
#undef TARGET_CODE_FOR_EXCEPTION_RECEIVER
#define TARGET_CODE_FOR_EXCEPTION_RECEIVER CODE_FOR_nothing

static bool
target_have_extv (void)
{
  return 
#line 5600 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(TARGET_68020 && TARGET_BITFIELD);
}

static rtx_insn *
target_gen_extv (rtx x0, rtx x1, rtx x2, rtx x3)
{
  gcc_checking_assert (targetm.have_extv ());
  return insnify (gen_extv (x0, x1, x2, x3));
}

#undef TARGET_HAVE_EXTV
#define TARGET_HAVE_EXTV target_have_extv
#undef TARGET_GEN_EXTV
#define TARGET_GEN_EXTV target_gen_extv
#undef TARGET_CODE_FOR_EXTV
#define TARGET_CODE_FOR_EXTV CODE_FOR_extv

static rtx_insn *
target_gen_extzv (rtx x0, rtx x1, rtx x2, rtx x3)
{
  gcc_checking_assert (targetm.have_extzv ());
  return insnify (gen_extzv (x0, x1, x2, x3));
}

#undef TARGET_HAVE_EXTZV
#define TARGET_HAVE_EXTZV target_have_extv
#undef TARGET_GEN_EXTZV
#define TARGET_GEN_EXTZV target_gen_extzv
#undef TARGET_CODE_FOR_EXTZV
#define TARGET_CODE_FOR_EXTZV CODE_FOR_extzv

static rtx_insn *
target_gen_indirect_jump (rtx x0)
{
  return insnify (gen_indirect_jump (x0));
}

#undef TARGET_HAVE_INDIRECT_JUMP
#define TARGET_HAVE_INDIRECT_JUMP hook_bool_void_true
#undef TARGET_GEN_INDIRECT_JUMP
#define TARGET_GEN_INDIRECT_JUMP target_gen_indirect_jump
#undef TARGET_CODE_FOR_INDIRECT_JUMP
#define TARGET_CODE_FOR_INDIRECT_JUMP CODE_FOR_indirect_jump

static rtx_insn *
target_gen_insv (rtx x0, rtx x1, rtx x2, rtx x3)
{
  gcc_checking_assert (targetm.have_insv ());
  return insnify (gen_insv (x0, x1, x2, x3));
}

#undef TARGET_HAVE_INSV
#define TARGET_HAVE_INSV target_have_extv
#undef TARGET_GEN_INSV
#define TARGET_GEN_INSV target_gen_insv
#undef TARGET_CODE_FOR_INSV
#define TARGET_CODE_FOR_INSV CODE_FOR_insv

static rtx_insn *
target_gen_jump (rtx x0)
{
  return insnify (gen_jump (x0));
}

#undef TARGET_HAVE_JUMP
#define TARGET_HAVE_JUMP hook_bool_void_true
#undef TARGET_GEN_JUMP
#define TARGET_GEN_JUMP target_gen_jump
#undef TARGET_CODE_FOR_JUMP
#define TARGET_CODE_FOR_JUMP CODE_FOR_jump

static rtx_insn *
invalid_rtx_rtx_rtx (rtx, rtx, rtx)
{
  gcc_unreachable ();
}

#undef TARGET_HAVE_LOAD_MULTIPLE
#define TARGET_HAVE_LOAD_MULTIPLE hook_bool_void_false
#undef TARGET_GEN_LOAD_MULTIPLE
#define TARGET_GEN_LOAD_MULTIPLE invalid_rtx_rtx_rtx
#undef TARGET_CODE_FOR_LOAD_MULTIPLE
#define TARGET_CODE_FOR_LOAD_MULTIPLE CODE_FOR_nothing

#undef TARGET_HAVE_MEM_THREAD_FENCE
#define TARGET_HAVE_MEM_THREAD_FENCE hook_bool_void_false
#undef TARGET_GEN_MEM_THREAD_FENCE
#define TARGET_GEN_MEM_THREAD_FENCE invalid_rtx
#undef TARGET_CODE_FOR_MEM_THREAD_FENCE
#define TARGET_CODE_FOR_MEM_THREAD_FENCE CODE_FOR_nothing

#undef TARGET_HAVE_MEMORY_BARRIER
#define TARGET_HAVE_MEMORY_BARRIER hook_bool_void_false
#undef TARGET_GEN_MEMORY_BARRIER
#define TARGET_GEN_MEMORY_BARRIER invalid_void
#undef TARGET_CODE_FOR_MEMORY_BARRIER
#define TARGET_CODE_FOR_MEMORY_BARRIER CODE_FOR_nothing

#undef TARGET_HAVE_MEMORY_BLOCKAGE
#define TARGET_HAVE_MEMORY_BLOCKAGE hook_bool_void_false
#undef TARGET_GEN_MEMORY_BLOCKAGE
#define TARGET_GEN_MEMORY_BLOCKAGE invalid_void
#undef TARGET_CODE_FOR_MEMORY_BLOCKAGE
#define TARGET_CODE_FOR_MEMORY_BLOCKAGE CODE_FOR_nothing

#undef TARGET_HAVE_MOVSTR
#define TARGET_HAVE_MOVSTR hook_bool_void_false
#undef TARGET_GEN_MOVSTR
#define TARGET_GEN_MOVSTR invalid_rtx_rtx_rtx
#undef TARGET_CODE_FOR_MOVSTR
#define TARGET_CODE_FOR_MOVSTR CODE_FOR_nothing

#undef TARGET_HAVE_NONLOCAL_GOTO
#define TARGET_HAVE_NONLOCAL_GOTO hook_bool_void_false
#undef TARGET_GEN_NONLOCAL_GOTO
#define TARGET_GEN_NONLOCAL_GOTO invalid_rtx_rtx_rtx_rtx
#undef TARGET_CODE_FOR_NONLOCAL_GOTO
#define TARGET_CODE_FOR_NONLOCAL_GOTO CODE_FOR_nothing

#undef TARGET_HAVE_NONLOCAL_GOTO_RECEIVER
#define TARGET_HAVE_NONLOCAL_GOTO_RECEIVER hook_bool_void_false
#undef TARGET_GEN_NONLOCAL_GOTO_RECEIVER
#define TARGET_GEN_NONLOCAL_GOTO_RECEIVER invalid_void
#undef TARGET_CODE_FOR_NONLOCAL_GOTO_RECEIVER
#define TARGET_CODE_FOR_NONLOCAL_GOTO_RECEIVER CODE_FOR_nothing

#undef TARGET_HAVE_OACC_DIM_POS
#define TARGET_HAVE_OACC_DIM_POS hook_bool_void_false
#undef TARGET_GEN_OACC_DIM_POS
#define TARGET_GEN_OACC_DIM_POS invalid_rtx_rtx
#undef TARGET_CODE_FOR_OACC_DIM_POS
#define TARGET_CODE_FOR_OACC_DIM_POS CODE_FOR_nothing

#undef TARGET_HAVE_OACC_DIM_SIZE
#define TARGET_HAVE_OACC_DIM_SIZE hook_bool_void_false
#undef TARGET_GEN_OACC_DIM_SIZE
#define TARGET_GEN_OACC_DIM_SIZE invalid_rtx_rtx
#undef TARGET_CODE_FOR_OACC_DIM_SIZE
#define TARGET_CODE_FOR_OACC_DIM_SIZE CODE_FOR_nothing

#undef TARGET_HAVE_OACC_FORK
#define TARGET_HAVE_OACC_FORK hook_bool_void_false
#undef TARGET_GEN_OACC_FORK
#define TARGET_GEN_OACC_FORK invalid_rtx_rtx_rtx
#undef TARGET_CODE_FOR_OACC_FORK
#define TARGET_CODE_FOR_OACC_FORK CODE_FOR_nothing

#undef TARGET_HAVE_OACC_JOIN
#define TARGET_HAVE_OACC_JOIN hook_bool_void_false
#undef TARGET_GEN_OACC_JOIN
#define TARGET_GEN_OACC_JOIN invalid_rtx_rtx_rtx
#undef TARGET_CODE_FOR_OACC_JOIN
#define TARGET_CODE_FOR_OACC_JOIN CODE_FOR_nothing

#undef TARGET_HAVE_OMP_SIMT_ENTER
#define TARGET_HAVE_OMP_SIMT_ENTER hook_bool_void_false
#undef TARGET_GEN_OMP_SIMT_ENTER
#define TARGET_GEN_OMP_SIMT_ENTER invalid_rtx_rtx_rtx
#undef TARGET_CODE_FOR_OMP_SIMT_ENTER
#define TARGET_CODE_FOR_OMP_SIMT_ENTER CODE_FOR_nothing

#undef TARGET_HAVE_OMP_SIMT_EXIT
#define TARGET_HAVE_OMP_SIMT_EXIT hook_bool_void_false
#undef TARGET_GEN_OMP_SIMT_EXIT
#define TARGET_GEN_OMP_SIMT_EXIT invalid_rtx
#undef TARGET_CODE_FOR_OMP_SIMT_EXIT
#define TARGET_CODE_FOR_OMP_SIMT_EXIT CODE_FOR_nothing

#undef TARGET_HAVE_OMP_SIMT_LANE
#define TARGET_HAVE_OMP_SIMT_LANE hook_bool_void_false
#undef TARGET_GEN_OMP_SIMT_LANE
#define TARGET_GEN_OMP_SIMT_LANE invalid_rtx
#undef TARGET_CODE_FOR_OMP_SIMT_LANE
#define TARGET_CODE_FOR_OMP_SIMT_LANE CODE_FOR_nothing

#undef TARGET_HAVE_OMP_SIMT_LAST_LANE
#define TARGET_HAVE_OMP_SIMT_LAST_LANE hook_bool_void_false
#undef TARGET_GEN_OMP_SIMT_LAST_LANE
#define TARGET_GEN_OMP_SIMT_LAST_LANE invalid_rtx_rtx
#undef TARGET_CODE_FOR_OMP_SIMT_LAST_LANE
#define TARGET_CODE_FOR_OMP_SIMT_LAST_LANE CODE_FOR_nothing

#undef TARGET_HAVE_OMP_SIMT_ORDERED
#define TARGET_HAVE_OMP_SIMT_ORDERED hook_bool_void_false
#undef TARGET_GEN_OMP_SIMT_ORDERED
#define TARGET_GEN_OMP_SIMT_ORDERED invalid_rtx_rtx
#undef TARGET_CODE_FOR_OMP_SIMT_ORDERED
#define TARGET_CODE_FOR_OMP_SIMT_ORDERED CODE_FOR_nothing

#undef TARGET_HAVE_OMP_SIMT_VOTE_ANY
#define TARGET_HAVE_OMP_SIMT_VOTE_ANY hook_bool_void_false
#undef TARGET_GEN_OMP_SIMT_VOTE_ANY
#define TARGET_GEN_OMP_SIMT_VOTE_ANY invalid_rtx_rtx
#undef TARGET_CODE_FOR_OMP_SIMT_VOTE_ANY
#define TARGET_CODE_FOR_OMP_SIMT_VOTE_ANY CODE_FOR_nothing

#undef TARGET_HAVE_OMP_SIMT_XCHG_BFLY
#define TARGET_HAVE_OMP_SIMT_XCHG_BFLY hook_bool_void_false
#undef TARGET_GEN_OMP_SIMT_XCHG_BFLY
#define TARGET_GEN_OMP_SIMT_XCHG_BFLY invalid_rtx_rtx_rtx
#undef TARGET_CODE_FOR_OMP_SIMT_XCHG_BFLY
#define TARGET_CODE_FOR_OMP_SIMT_XCHG_BFLY CODE_FOR_nothing

#undef TARGET_HAVE_OMP_SIMT_XCHG_IDX
#define TARGET_HAVE_OMP_SIMT_XCHG_IDX hook_bool_void_false
#undef TARGET_GEN_OMP_SIMT_XCHG_IDX
#define TARGET_GEN_OMP_SIMT_XCHG_IDX invalid_rtx_rtx_rtx
#undef TARGET_CODE_FOR_OMP_SIMT_XCHG_IDX
#define TARGET_CODE_FOR_OMP_SIMT_XCHG_IDX CODE_FOR_nothing

#undef TARGET_HAVE_PREFETCH
#define TARGET_HAVE_PREFETCH hook_bool_void_false
#undef TARGET_GEN_PREFETCH
#define TARGET_GEN_PREFETCH invalid_rtx_rtx_rtx
#undef TARGET_CODE_FOR_PREFETCH
#define TARGET_CODE_FOR_PREFETCH CODE_FOR_nothing

#undef TARGET_HAVE_PROBE_STACK
#define TARGET_HAVE_PROBE_STACK hook_bool_void_false
#undef TARGET_GEN_PROBE_STACK
#define TARGET_GEN_PROBE_STACK invalid_rtx
#undef TARGET_CODE_FOR_PROBE_STACK
#define TARGET_CODE_FOR_PROBE_STACK CODE_FOR_nothing

#undef TARGET_HAVE_PROBE_STACK_ADDRESS
#define TARGET_HAVE_PROBE_STACK_ADDRESS hook_bool_void_false
#undef TARGET_GEN_PROBE_STACK_ADDRESS
#define TARGET_GEN_PROBE_STACK_ADDRESS invalid_rtx
#undef TARGET_CODE_FOR_PROBE_STACK_ADDRESS
#define TARGET_CODE_FOR_PROBE_STACK_ADDRESS CODE_FOR_nothing

static rtx_insn *
target_gen_prologue (void)
{
  return insnify (gen_prologue ());
}

#undef TARGET_HAVE_PROLOGUE
#define TARGET_HAVE_PROLOGUE hook_bool_void_true
#undef TARGET_GEN_PROLOGUE
#define TARGET_GEN_PROLOGUE target_gen_prologue
#undef TARGET_CODE_FOR_PROLOGUE
#define TARGET_CODE_FOR_PROLOGUE CODE_FOR_prologue

#undef TARGET_HAVE_PTR_EXTEND
#define TARGET_HAVE_PTR_EXTEND hook_bool_void_false
#undef TARGET_GEN_PTR_EXTEND
#define TARGET_GEN_PTR_EXTEND invalid_rtx_rtx
#undef TARGET_CODE_FOR_PTR_EXTEND
#define TARGET_CODE_FOR_PTR_EXTEND CODE_FOR_nothing

#undef TARGET_HAVE_RELOAD_LOAD_ADDRESS
#define TARGET_HAVE_RELOAD_LOAD_ADDRESS hook_bool_void_false
#undef TARGET_GEN_RELOAD_LOAD_ADDRESS
#define TARGET_GEN_RELOAD_LOAD_ADDRESS invalid_rtx_rtx
#undef TARGET_CODE_FOR_RELOAD_LOAD_ADDRESS
#define TARGET_CODE_FOR_RELOAD_LOAD_ADDRESS CODE_FOR_nothing

#undef TARGET_HAVE_RESTORE_STACK_BLOCK
#define TARGET_HAVE_RESTORE_STACK_BLOCK hook_bool_void_false
#undef TARGET_GEN_RESTORE_STACK_BLOCK
#define TARGET_GEN_RESTORE_STACK_BLOCK invalid_rtx_rtx
#undef TARGET_CODE_FOR_RESTORE_STACK_BLOCK
#define TARGET_CODE_FOR_RESTORE_STACK_BLOCK CODE_FOR_nothing

#undef TARGET_HAVE_RESTORE_STACK_FUNCTION
#define TARGET_HAVE_RESTORE_STACK_FUNCTION hook_bool_void_false
#undef TARGET_GEN_RESTORE_STACK_FUNCTION
#define TARGET_GEN_RESTORE_STACK_FUNCTION invalid_rtx_rtx
#undef TARGET_CODE_FOR_RESTORE_STACK_FUNCTION
#define TARGET_CODE_FOR_RESTORE_STACK_FUNCTION CODE_FOR_nothing

#undef TARGET_HAVE_RESTORE_STACK_NONLOCAL
#define TARGET_HAVE_RESTORE_STACK_NONLOCAL hook_bool_void_false
#undef TARGET_GEN_RESTORE_STACK_NONLOCAL
#define TARGET_GEN_RESTORE_STACK_NONLOCAL invalid_rtx_rtx
#undef TARGET_CODE_FOR_RESTORE_STACK_NONLOCAL
#define TARGET_CODE_FOR_RESTORE_STACK_NONLOCAL CODE_FOR_nothing

static bool
target_have_return (void)
{
  return 
#line 6129 "/Users/sidqian/Downloads/summer/L2S/netbsd-src/tools/gcc/../../external/gpl3/gcc.old/dist/gcc/config/m68k/m68k.md"
(m68k_use_return_insn ());
}

static rtx_insn *
target_gen_return (void)
{
  gcc_checking_assert (targetm.have_return ());
  return insnify (gen_return ());
}

#undef TARGET_HAVE_RETURN
#define TARGET_HAVE_RETURN target_have_return
#undef TARGET_GEN_RETURN
#define TARGET_GEN_RETURN target_gen_return
#undef TARGET_CODE_FOR_RETURN
#define TARGET_CODE_FOR_RETURN CODE_FOR_return

#undef TARGET_HAVE_SAVE_STACK_BLOCK
#define TARGET_HAVE_SAVE_STACK_BLOCK hook_bool_void_false
#undef TARGET_GEN_SAVE_STACK_BLOCK
#define TARGET_GEN_SAVE_STACK_BLOCK invalid_rtx_rtx
#undef TARGET_CODE_FOR_SAVE_STACK_BLOCK
#define TARGET_CODE_FOR_SAVE_STACK_BLOCK CODE_FOR_nothing

#undef TARGET_HAVE_SAVE_STACK_FUNCTION
#define TARGET_HAVE_SAVE_STACK_FUNCTION hook_bool_void_false
#undef TARGET_GEN_SAVE_STACK_FUNCTION
#define TARGET_GEN_SAVE_STACK_FUNCTION invalid_rtx_rtx
#undef TARGET_CODE_FOR_SAVE_STACK_FUNCTION
#define TARGET_CODE_FOR_SAVE_STACK_FUNCTION CODE_FOR_nothing

#undef TARGET_HAVE_SAVE_STACK_NONLOCAL
#define TARGET_HAVE_SAVE_STACK_NONLOCAL hook_bool_void_false
#undef TARGET_GEN_SAVE_STACK_NONLOCAL
#define TARGET_GEN_SAVE_STACK_NONLOCAL invalid_rtx_rtx
#undef TARGET_CODE_FOR_SAVE_STACK_NONLOCAL
#define TARGET_CODE_FOR_SAVE_STACK_NONLOCAL CODE_FOR_nothing

static rtx_insn *
target_gen_sibcall (rtx x0, rtx opt1, rtx, rtx)
{
  return insnify (gen_sibcall (x0, opt1));
}

#undef TARGET_HAVE_SIBCALL
#define TARGET_HAVE_SIBCALL hook_bool_void_true
#undef TARGET_GEN_SIBCALL
#define TARGET_GEN_SIBCALL target_gen_sibcall
#undef TARGET_CODE_FOR_SIBCALL
#define TARGET_CODE_FOR_SIBCALL CODE_FOR_sibcall

static rtx_insn *
target_gen_sibcall_epilogue (void)
{
  return insnify (gen_sibcall_epilogue ());
}

#undef TARGET_HAVE_SIBCALL_EPILOGUE
#define TARGET_HAVE_SIBCALL_EPILOGUE hook_bool_void_true
#undef TARGET_GEN_SIBCALL_EPILOGUE
#define TARGET_GEN_SIBCALL_EPILOGUE target_gen_sibcall_epilogue
#undef TARGET_CODE_FOR_SIBCALL_EPILOGUE
#define TARGET_CODE_FOR_SIBCALL_EPILOGUE CODE_FOR_sibcall_epilogue

static rtx_insn *
target_gen_sibcall_value (rtx x0, rtx x1, rtx opt2, rtx, rtx)
{
  return insnify (gen_sibcall_value (x0, x1, opt2));
}

#undef TARGET_HAVE_SIBCALL_VALUE
#define TARGET_HAVE_SIBCALL_VALUE hook_bool_void_true
#undef TARGET_GEN_SIBCALL_VALUE
#define TARGET_GEN_SIBCALL_VALUE target_gen_sibcall_value
#undef TARGET_CODE_FOR_SIBCALL_VALUE
#define TARGET_CODE_FOR_SIBCALL_VALUE CODE_FOR_sibcall_value

#undef TARGET_HAVE_SIMPLE_RETURN
#define TARGET_HAVE_SIMPLE_RETURN hook_bool_void_false
#undef TARGET_GEN_SIMPLE_RETURN
#define TARGET_GEN_SIMPLE_RETURN invalid_void
#undef TARGET_CODE_FOR_SIMPLE_RETURN
#define TARGET_CODE_FOR_SIMPLE_RETURN CODE_FOR_nothing

#undef TARGET_HAVE_SPLIT_STACK_PROLOGUE
#define TARGET_HAVE_SPLIT_STACK_PROLOGUE hook_bool_void_false
#undef TARGET_GEN_SPLIT_STACK_PROLOGUE
#define TARGET_GEN_SPLIT_STACK_PROLOGUE invalid_void
#undef TARGET_CODE_FOR_SPLIT_STACK_PROLOGUE
#define TARGET_CODE_FOR_SPLIT_STACK_PROLOGUE CODE_FOR_nothing

#undef TARGET_HAVE_SPLIT_STACK_SPACE_CHECK
#define TARGET_HAVE_SPLIT_STACK_SPACE_CHECK hook_bool_void_false
#undef TARGET_GEN_SPLIT_STACK_SPACE_CHECK
#define TARGET_GEN_SPLIT_STACK_SPACE_CHECK invalid_rtx_rtx
#undef TARGET_CODE_FOR_SPLIT_STACK_SPACE_CHECK
#define TARGET_CODE_FOR_SPLIT_STACK_SPACE_CHECK CODE_FOR_nothing

#undef TARGET_HAVE_STACK_PROTECT_COMBINED_SET
#define TARGET_HAVE_STACK_PROTECT_COMBINED_SET hook_bool_void_false
#undef TARGET_GEN_STACK_PROTECT_COMBINED_SET
#define TARGET_GEN_STACK_PROTECT_COMBINED_SET invalid_rtx_rtx
#undef TARGET_CODE_FOR_STACK_PROTECT_COMBINED_SET
#define TARGET_CODE_FOR_STACK_PROTECT_COMBINED_SET CODE_FOR_nothing

#undef TARGET_HAVE_STACK_PROTECT_SET
#define TARGET_HAVE_STACK_PROTECT_SET hook_bool_void_false
#undef TARGET_GEN_STACK_PROTECT_SET
#define TARGET_GEN_STACK_PROTECT_SET invalid_rtx_rtx
#undef TARGET_CODE_FOR_STACK_PROTECT_SET
#define TARGET_CODE_FOR_STACK_PROTECT_SET CODE_FOR_nothing

#undef TARGET_HAVE_STACK_PROTECT_COMBINED_TEST
#define TARGET_HAVE_STACK_PROTECT_COMBINED_TEST hook_bool_void_false
#undef TARGET_GEN_STACK_PROTECT_COMBINED_TEST
#define TARGET_GEN_STACK_PROTECT_COMBINED_TEST invalid_rtx_rtx_rtx
#undef TARGET_CODE_FOR_STACK_PROTECT_COMBINED_TEST
#define TARGET_CODE_FOR_STACK_PROTECT_COMBINED_TEST CODE_FOR_nothing

#undef TARGET_HAVE_STACK_PROTECT_TEST
#define TARGET_HAVE_STACK_PROTECT_TEST hook_bool_void_false
#undef TARGET_GEN_STACK_PROTECT_TEST
#define TARGET_GEN_STACK_PROTECT_TEST invalid_rtx_rtx_rtx
#undef TARGET_CODE_FOR_STACK_PROTECT_TEST
#define TARGET_CODE_FOR_STACK_PROTECT_TEST CODE_FOR_nothing

#undef TARGET_HAVE_STORE_MULTIPLE
#define TARGET_HAVE_STORE_MULTIPLE hook_bool_void_false
#undef TARGET_GEN_STORE_MULTIPLE
#define TARGET_GEN_STORE_MULTIPLE invalid_rtx_rtx_rtx
#undef TARGET_CODE_FOR_STORE_MULTIPLE
#define TARGET_CODE_FOR_STORE_MULTIPLE CODE_FOR_nothing

static rtx_insn *
target_gen_tablejump (rtx x0, rtx x1)
{
  return insnify (gen_tablejump (x0, x1));
}

#undef TARGET_HAVE_TABLEJUMP
#define TARGET_HAVE_TABLEJUMP hook_bool_void_true
#undef TARGET_GEN_TABLEJUMP
#define TARGET_GEN_TABLEJUMP target_gen_tablejump
#undef TARGET_CODE_FOR_TABLEJUMP
#define TARGET_CODE_FOR_TABLEJUMP CODE_FOR_tablejump

static rtx_insn *
target_gen_trap (void)
{
  return insnify (gen_trap ());
}

#undef TARGET_HAVE_TRAP
#define TARGET_HAVE_TRAP hook_bool_void_true
#undef TARGET_GEN_TRAP
#define TARGET_GEN_TRAP target_gen_trap
#undef TARGET_CODE_FOR_TRAP
#define TARGET_CODE_FOR_TRAP CODE_FOR_trap

#undef TARGET_HAVE_UNIQUE
#define TARGET_HAVE_UNIQUE hook_bool_void_false
#undef TARGET_GEN_UNIQUE
#define TARGET_GEN_UNIQUE invalid_void
#undef TARGET_CODE_FOR_UNIQUE
#define TARGET_CODE_FOR_UNIQUE CODE_FOR_nothing

static rtx_insn *
target_gen_untyped_call (rtx x0, rtx x1, rtx x2)
{
  return insnify (gen_untyped_call (x0, x1, x2));
}

#undef TARGET_HAVE_UNTYPED_CALL
#define TARGET_HAVE_UNTYPED_CALL hook_bool_void_true
#undef TARGET_GEN_UNTYPED_CALL
#define TARGET_GEN_UNTYPED_CALL target_gen_untyped_call
#undef TARGET_CODE_FOR_UNTYPED_CALL
#define TARGET_CODE_FOR_UNTYPED_CALL CODE_FOR_untyped_call

#undef TARGET_HAVE_UNTYPED_RETURN
#define TARGET_HAVE_UNTYPED_RETURN hook_bool_void_false
#undef TARGET_GEN_UNTYPED_RETURN
#define TARGET_GEN_UNTYPED_RETURN invalid_rtx_rtx
#undef TARGET_CODE_FOR_UNTYPED_RETURN
#define TARGET_CODE_FOR_UNTYPED_RETURN CODE_FOR_nothing

#endif /* GCC_INSN_TARGET_DEF_H */
