/* THIS FILE IS GENERATED -*- buffer-read-only: t -*- */
/* vi:set ro: */

/* To regenerate this file, run:*/
/*      make-target-delegates target.h > target-delegates.c */
static void
delegate_post_attach (struct target_ops *self, int arg1)
{
  self = self->beneath;
  self->to_post_attach (self, arg1);
}

static void
tdefault_post_attach (struct target_ops *self, int arg1)
{
}

static void
debug_post_attach (struct target_ops *self, int arg1)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_post_attach (...)\n", debug_target.to_shortname);
  debug_target.to_post_attach (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_post_attach (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_detach (struct target_ops *self, const char *arg1, int arg2)
{
  self = self->beneath;
  self->to_detach (self, arg1, arg2);
}

static void
tdefault_detach (struct target_ops *self, const char *arg1, int arg2)
{
}

static void
debug_detach (struct target_ops *self, const char *arg1, int arg2)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_detach (...)\n", debug_target.to_shortname);
  debug_target.to_detach (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_detach (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_const_char_p (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_disconnect (struct target_ops *self, const char *arg1, int arg2)
{
  self = self->beneath;
  self->to_disconnect (self, arg1, arg2);
}

static void
tdefault_disconnect (struct target_ops *self, const char *arg1, int arg2)
{
  tcomplain ();
}

static void
debug_disconnect (struct target_ops *self, const char *arg1, int arg2)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_disconnect (...)\n", debug_target.to_shortname);
  debug_target.to_disconnect (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_disconnect (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_const_char_p (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_resume (struct target_ops *self, ptid_t arg1, int arg2, enum gdb_signal arg3)
{
  self = self->beneath;
  self->to_resume (self, arg1, arg2, arg3);
}

static void
tdefault_resume (struct target_ops *self, ptid_t arg1, int arg2, enum gdb_signal arg3)
{
  noprocess ();
}

static void
debug_resume (struct target_ops *self, ptid_t arg1, int arg2, enum gdb_signal arg3)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_resume (...)\n", debug_target.to_shortname);
  debug_target.to_resume (&debug_target, arg1, arg2, arg3);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_resume (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ptid_t (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_step (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_enum_gdb_signal (arg3);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_commit_resume (struct target_ops *self)
{
  self = self->beneath;
  self->to_commit_resume (self);
}

static void
tdefault_commit_resume (struct target_ops *self)
{
}

static void
debug_commit_resume (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_commit_resume (...)\n", debug_target.to_shortname);
  debug_target.to_commit_resume (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_commit_resume (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static ptid_t
delegate_wait (struct target_ops *self, ptid_t arg1, struct target_waitstatus *arg2, int arg3)
{
  self = self->beneath;
  return self->to_wait (self, arg1, arg2, arg3);
}

static ptid_t
debug_wait (struct target_ops *self, ptid_t arg1, struct target_waitstatus *arg2, int arg3)
{
  ptid_t result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_wait (...)\n", debug_target.to_shortname);
  result = debug_target.to_wait (&debug_target, arg1, arg2, arg3);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_wait (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ptid_t (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_target_waitstatus_p (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_options (arg3);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_ptid_t (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static void
delegate_fetch_registers (struct target_ops *self, struct regcache *arg1, int arg2)
{
  self = self->beneath;
  self->to_fetch_registers (self, arg1, arg2);
}

static void
tdefault_fetch_registers (struct target_ops *self, struct regcache *arg1, int arg2)
{
}

static void
debug_fetch_registers (struct target_ops *self, struct regcache *arg1, int arg2)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_fetch_registers (...)\n", debug_target.to_shortname);
  debug_target.to_fetch_registers (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_fetch_registers (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_regcache_p (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_store_registers (struct target_ops *self, struct regcache *arg1, int arg2)
{
  self = self->beneath;
  self->to_store_registers (self, arg1, arg2);
}

static void
tdefault_store_registers (struct target_ops *self, struct regcache *arg1, int arg2)
{
  noprocess ();
}

static void
debug_store_registers (struct target_ops *self, struct regcache *arg1, int arg2)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_store_registers (...)\n", debug_target.to_shortname);
  debug_target.to_store_registers (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_store_registers (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_regcache_p (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_prepare_to_store (struct target_ops *self, struct regcache *arg1)
{
  self = self->beneath;
  self->to_prepare_to_store (self, arg1);
}

static void
tdefault_prepare_to_store (struct target_ops *self, struct regcache *arg1)
{
  noprocess ();
}

static void
debug_prepare_to_store (struct target_ops *self, struct regcache *arg1)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_prepare_to_store (...)\n", debug_target.to_shortname);
  debug_target.to_prepare_to_store (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_prepare_to_store (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_regcache_p (arg1);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_files_info (struct target_ops *self)
{
  self = self->beneath;
  self->to_files_info (self);
}

static void
tdefault_files_info (struct target_ops *self)
{
}

static void
debug_files_info (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_files_info (...)\n", debug_target.to_shortname);
  debug_target.to_files_info (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_files_info (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static int
delegate_insert_breakpoint (struct target_ops *self, struct gdbarch *arg1, struct bp_target_info *arg2)
{
  self = self->beneath;
  return self->to_insert_breakpoint (self, arg1, arg2);
}

static int
debug_insert_breakpoint (struct target_ops *self, struct gdbarch *arg1, struct bp_target_info *arg2)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_insert_breakpoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_insert_breakpoint (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_insert_breakpoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_gdbarch_p (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_bp_target_info_p (arg2);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_remove_breakpoint (struct target_ops *self, struct gdbarch *arg1, struct bp_target_info *arg2, enum remove_bp_reason arg3)
{
  self = self->beneath;
  return self->to_remove_breakpoint (self, arg1, arg2, arg3);
}

static int
debug_remove_breakpoint (struct target_ops *self, struct gdbarch *arg1, struct bp_target_info *arg2, enum remove_bp_reason arg3)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_remove_breakpoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_remove_breakpoint (&debug_target, arg1, arg2, arg3);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_remove_breakpoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_gdbarch_p (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_bp_target_info_p (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_enum_remove_bp_reason (arg3);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_stopped_by_sw_breakpoint (struct target_ops *self)
{
  self = self->beneath;
  return self->to_stopped_by_sw_breakpoint (self);
}

static int
tdefault_stopped_by_sw_breakpoint (struct target_ops *self)
{
  return 0;
}

static int
debug_stopped_by_sw_breakpoint (struct target_ops *self)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_stopped_by_sw_breakpoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_stopped_by_sw_breakpoint (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_stopped_by_sw_breakpoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_supports_stopped_by_sw_breakpoint (struct target_ops *self)
{
  self = self->beneath;
  return self->to_supports_stopped_by_sw_breakpoint (self);
}

static int
tdefault_supports_stopped_by_sw_breakpoint (struct target_ops *self)
{
  return 0;
}

static int
debug_supports_stopped_by_sw_breakpoint (struct target_ops *self)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_supports_stopped_by_sw_breakpoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_supports_stopped_by_sw_breakpoint (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_supports_stopped_by_sw_breakpoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_stopped_by_hw_breakpoint (struct target_ops *self)
{
  self = self->beneath;
  return self->to_stopped_by_hw_breakpoint (self);
}

static int
tdefault_stopped_by_hw_breakpoint (struct target_ops *self)
{
  return 0;
}

static int
debug_stopped_by_hw_breakpoint (struct target_ops *self)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_stopped_by_hw_breakpoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_stopped_by_hw_breakpoint (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_stopped_by_hw_breakpoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_supports_stopped_by_hw_breakpoint (struct target_ops *self)
{
  self = self->beneath;
  return self->to_supports_stopped_by_hw_breakpoint (self);
}

static int
tdefault_supports_stopped_by_hw_breakpoint (struct target_ops *self)
{
  return 0;
}

static int
debug_supports_stopped_by_hw_breakpoint (struct target_ops *self)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_supports_stopped_by_hw_breakpoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_supports_stopped_by_hw_breakpoint (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_supports_stopped_by_hw_breakpoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_can_use_hw_breakpoint (struct target_ops *self, enum bptype arg1, int arg2, int arg3)
{
  self = self->beneath;
  return self->to_can_use_hw_breakpoint (self, arg1, arg2, arg3);
}

static int
tdefault_can_use_hw_breakpoint (struct target_ops *self, enum bptype arg1, int arg2, int arg3)
{
  return 0;
}

static int
debug_can_use_hw_breakpoint (struct target_ops *self, enum bptype arg1, int arg2, int arg3)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_can_use_hw_breakpoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_can_use_hw_breakpoint (&debug_target, arg1, arg2, arg3);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_can_use_hw_breakpoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_enum_bptype (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg3);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_ranged_break_num_registers (struct target_ops *self)
{
  self = self->beneath;
  return self->to_ranged_break_num_registers (self);
}

static int
tdefault_ranged_break_num_registers (struct target_ops *self)
{
  return -1;
}

static int
debug_ranged_break_num_registers (struct target_ops *self)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_ranged_break_num_registers (...)\n", debug_target.to_shortname);
  result = debug_target.to_ranged_break_num_registers (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_ranged_break_num_registers (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_insert_hw_breakpoint (struct target_ops *self, struct gdbarch *arg1, struct bp_target_info *arg2)
{
  self = self->beneath;
  return self->to_insert_hw_breakpoint (self, arg1, arg2);
}

static int
tdefault_insert_hw_breakpoint (struct target_ops *self, struct gdbarch *arg1, struct bp_target_info *arg2)
{
  return -1;
}

static int
debug_insert_hw_breakpoint (struct target_ops *self, struct gdbarch *arg1, struct bp_target_info *arg2)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_insert_hw_breakpoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_insert_hw_breakpoint (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_insert_hw_breakpoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_gdbarch_p (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_bp_target_info_p (arg2);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_remove_hw_breakpoint (struct target_ops *self, struct gdbarch *arg1, struct bp_target_info *arg2)
{
  self = self->beneath;
  return self->to_remove_hw_breakpoint (self, arg1, arg2);
}

static int
tdefault_remove_hw_breakpoint (struct target_ops *self, struct gdbarch *arg1, struct bp_target_info *arg2)
{
  return -1;
}

static int
debug_remove_hw_breakpoint (struct target_ops *self, struct gdbarch *arg1, struct bp_target_info *arg2)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_remove_hw_breakpoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_remove_hw_breakpoint (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_remove_hw_breakpoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_gdbarch_p (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_bp_target_info_p (arg2);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_remove_watchpoint (struct target_ops *self, CORE_ADDR arg1, int arg2, enum target_hw_bp_type arg3, struct expression *arg4)
{
  self = self->beneath;
  return self->to_remove_watchpoint (self, arg1, arg2, arg3, arg4);
}

static int
tdefault_remove_watchpoint (struct target_ops *self, CORE_ADDR arg1, int arg2, enum target_hw_bp_type arg3, struct expression *arg4)
{
  return -1;
}

static int
debug_remove_watchpoint (struct target_ops *self, CORE_ADDR arg1, int arg2, enum target_hw_bp_type arg3, struct expression *arg4)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_remove_watchpoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_remove_watchpoint (&debug_target, arg1, arg2, arg3, arg4);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_remove_watchpoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_enum_target_hw_bp_type (arg3);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_expression_p (arg4);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_insert_watchpoint (struct target_ops *self, CORE_ADDR arg1, int arg2, enum target_hw_bp_type arg3, struct expression *arg4)
{
  self = self->beneath;
  return self->to_insert_watchpoint (self, arg1, arg2, arg3, arg4);
}

static int
tdefault_insert_watchpoint (struct target_ops *self, CORE_ADDR arg1, int arg2, enum target_hw_bp_type arg3, struct expression *arg4)
{
  return -1;
}

static int
debug_insert_watchpoint (struct target_ops *self, CORE_ADDR arg1, int arg2, enum target_hw_bp_type arg3, struct expression *arg4)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_insert_watchpoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_insert_watchpoint (&debug_target, arg1, arg2, arg3, arg4);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_insert_watchpoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_enum_target_hw_bp_type (arg3);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_expression_p (arg4);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_insert_mask_watchpoint (struct target_ops *self, CORE_ADDR arg1, CORE_ADDR arg2, enum target_hw_bp_type arg3)
{
  self = self->beneath;
  return self->to_insert_mask_watchpoint (self, arg1, arg2, arg3);
}

static int
tdefault_insert_mask_watchpoint (struct target_ops *self, CORE_ADDR arg1, CORE_ADDR arg2, enum target_hw_bp_type arg3)
{
  return 1;
}

static int
debug_insert_mask_watchpoint (struct target_ops *self, CORE_ADDR arg1, CORE_ADDR arg2, enum target_hw_bp_type arg3)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_insert_mask_watchpoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_insert_mask_watchpoint (&debug_target, arg1, arg2, arg3);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_insert_mask_watchpoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_enum_target_hw_bp_type (arg3);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_remove_mask_watchpoint (struct target_ops *self, CORE_ADDR arg1, CORE_ADDR arg2, enum target_hw_bp_type arg3)
{
  self = self->beneath;
  return self->to_remove_mask_watchpoint (self, arg1, arg2, arg3);
}

static int
tdefault_remove_mask_watchpoint (struct target_ops *self, CORE_ADDR arg1, CORE_ADDR arg2, enum target_hw_bp_type arg3)
{
  return 1;
}

static int
debug_remove_mask_watchpoint (struct target_ops *self, CORE_ADDR arg1, CORE_ADDR arg2, enum target_hw_bp_type arg3)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_remove_mask_watchpoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_remove_mask_watchpoint (&debug_target, arg1, arg2, arg3);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_remove_mask_watchpoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_enum_target_hw_bp_type (arg3);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_stopped_by_watchpoint (struct target_ops *self)
{
  self = self->beneath;
  return self->to_stopped_by_watchpoint (self);
}

static int
tdefault_stopped_by_watchpoint (struct target_ops *self)
{
  return 0;
}

static int
debug_stopped_by_watchpoint (struct target_ops *self)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_stopped_by_watchpoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_stopped_by_watchpoint (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_stopped_by_watchpoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_stopped_data_address (struct target_ops *self, CORE_ADDR *arg1)
{
  self = self->beneath;
  return self->to_stopped_data_address (self, arg1);
}

static int
tdefault_stopped_data_address (struct target_ops *self, CORE_ADDR *arg1)
{
  return 0;
}

static int
debug_stopped_data_address (struct target_ops *self, CORE_ADDR *arg1)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_stopped_data_address (...)\n", debug_target.to_shortname);
  result = debug_target.to_stopped_data_address (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_stopped_data_address (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR_p (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_watchpoint_addr_within_range (struct target_ops *self, CORE_ADDR arg1, CORE_ADDR arg2, int arg3)
{
  self = self->beneath;
  return self->to_watchpoint_addr_within_range (self, arg1, arg2, arg3);
}

static int
debug_watchpoint_addr_within_range (struct target_ops *self, CORE_ADDR arg1, CORE_ADDR arg2, int arg3)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_watchpoint_addr_within_range (...)\n", debug_target.to_shortname);
  result = debug_target.to_watchpoint_addr_within_range (&debug_target, arg1, arg2, arg3);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_watchpoint_addr_within_range (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg3);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_region_ok_for_hw_watchpoint (struct target_ops *self, CORE_ADDR arg1, int arg2)
{
  self = self->beneath;
  return self->to_region_ok_for_hw_watchpoint (self, arg1, arg2);
}

static int
debug_region_ok_for_hw_watchpoint (struct target_ops *self, CORE_ADDR arg1, int arg2)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_region_ok_for_hw_watchpoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_region_ok_for_hw_watchpoint (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_region_ok_for_hw_watchpoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_can_accel_watchpoint_condition (struct target_ops *self, CORE_ADDR arg1, int arg2, int arg3, struct expression *arg4)
{
  self = self->beneath;
  return self->to_can_accel_watchpoint_condition (self, arg1, arg2, arg3, arg4);
}

static int
tdefault_can_accel_watchpoint_condition (struct target_ops *self, CORE_ADDR arg1, int arg2, int arg3, struct expression *arg4)
{
  return 0;
}

static int
debug_can_accel_watchpoint_condition (struct target_ops *self, CORE_ADDR arg1, int arg2, int arg3, struct expression *arg4)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_can_accel_watchpoint_condition (...)\n", debug_target.to_shortname);
  result = debug_target.to_can_accel_watchpoint_condition (&debug_target, arg1, arg2, arg3, arg4);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_can_accel_watchpoint_condition (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg3);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_expression_p (arg4);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_masked_watch_num_registers (struct target_ops *self, CORE_ADDR arg1, CORE_ADDR arg2)
{
  self = self->beneath;
  return self->to_masked_watch_num_registers (self, arg1, arg2);
}

static int
tdefault_masked_watch_num_registers (struct target_ops *self, CORE_ADDR arg1, CORE_ADDR arg2)
{
  return -1;
}

static int
debug_masked_watch_num_registers (struct target_ops *self, CORE_ADDR arg1, CORE_ADDR arg2)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_masked_watch_num_registers (...)\n", debug_target.to_shortname);
  result = debug_target.to_masked_watch_num_registers (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_masked_watch_num_registers (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg2);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_can_do_single_step (struct target_ops *self)
{
  self = self->beneath;
  return self->to_can_do_single_step (self);
}

static int
tdefault_can_do_single_step (struct target_ops *self)
{
  return -1;
}

static int
debug_can_do_single_step (struct target_ops *self)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_can_do_single_step (...)\n", debug_target.to_shortname);
  result = debug_target.to_can_do_single_step (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_can_do_single_step (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static void
delegate_terminal_init (struct target_ops *self)
{
  self = self->beneath;
  self->to_terminal_init (self);
}

static void
tdefault_terminal_init (struct target_ops *self)
{
}

static void
debug_terminal_init (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_terminal_init (...)\n", debug_target.to_shortname);
  debug_target.to_terminal_init (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_terminal_init (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_terminal_inferior (struct target_ops *self)
{
  self = self->beneath;
  self->to_terminal_inferior (self);
}

static void
tdefault_terminal_inferior (struct target_ops *self)
{
}

static void
debug_terminal_inferior (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_terminal_inferior (...)\n", debug_target.to_shortname);
  debug_target.to_terminal_inferior (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_terminal_inferior (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_terminal_ours_for_output (struct target_ops *self)
{
  self = self->beneath;
  self->to_terminal_ours_for_output (self);
}

static void
tdefault_terminal_ours_for_output (struct target_ops *self)
{
}

static void
debug_terminal_ours_for_output (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_terminal_ours_for_output (...)\n", debug_target.to_shortname);
  debug_target.to_terminal_ours_for_output (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_terminal_ours_for_output (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_terminal_ours (struct target_ops *self)
{
  self = self->beneath;
  self->to_terminal_ours (self);
}

static void
tdefault_terminal_ours (struct target_ops *self)
{
}

static void
debug_terminal_ours (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_terminal_ours (...)\n", debug_target.to_shortname);
  debug_target.to_terminal_ours (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_terminal_ours (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_terminal_info (struct target_ops *self, const char *arg1, int arg2)
{
  self = self->beneath;
  self->to_terminal_info (self, arg1, arg2);
}

static void
debug_terminal_info (struct target_ops *self, const char *arg1, int arg2)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_terminal_info (...)\n", debug_target.to_shortname);
  debug_target.to_terminal_info (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_terminal_info (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_const_char_p (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_kill (struct target_ops *self)
{
  self = self->beneath;
  self->to_kill (self);
}

static void
tdefault_kill (struct target_ops *self)
{
  noprocess ();
}

static void
debug_kill (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_kill (...)\n", debug_target.to_shortname);
  debug_target.to_kill (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_kill (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_load (struct target_ops *self, const char *arg1, int arg2)
{
  self = self->beneath;
  self->to_load (self, arg1, arg2);
}

static void
tdefault_load (struct target_ops *self, const char *arg1, int arg2)
{
  tcomplain ();
}

static void
debug_load (struct target_ops *self, const char *arg1, int arg2)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_load (...)\n", debug_target.to_shortname);
  debug_target.to_load (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_load (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_const_char_p (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_post_startup_inferior (struct target_ops *self, ptid_t arg1)
{
  self = self->beneath;
  self->to_post_startup_inferior (self, arg1);
}

static void
tdefault_post_startup_inferior (struct target_ops *self, ptid_t arg1)
{
}

static void
debug_post_startup_inferior (struct target_ops *self, ptid_t arg1)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_post_startup_inferior (...)\n", debug_target.to_shortname);
  debug_target.to_post_startup_inferior (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_post_startup_inferior (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ptid_t (arg1);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static int
delegate_insert_fork_catchpoint (struct target_ops *self, int arg1)
{
  self = self->beneath;
  return self->to_insert_fork_catchpoint (self, arg1);
}

static int
tdefault_insert_fork_catchpoint (struct target_ops *self, int arg1)
{
  return 1;
}

static int
debug_insert_fork_catchpoint (struct target_ops *self, int arg1)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_insert_fork_catchpoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_insert_fork_catchpoint (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_insert_fork_catchpoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_remove_fork_catchpoint (struct target_ops *self, int arg1)
{
  self = self->beneath;
  return self->to_remove_fork_catchpoint (self, arg1);
}

static int
tdefault_remove_fork_catchpoint (struct target_ops *self, int arg1)
{
  return 1;
}

static int
debug_remove_fork_catchpoint (struct target_ops *self, int arg1)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_remove_fork_catchpoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_remove_fork_catchpoint (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_remove_fork_catchpoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_insert_vfork_catchpoint (struct target_ops *self, int arg1)
{
  self = self->beneath;
  return self->to_insert_vfork_catchpoint (self, arg1);
}

static int
tdefault_insert_vfork_catchpoint (struct target_ops *self, int arg1)
{
  return 1;
}

static int
debug_insert_vfork_catchpoint (struct target_ops *self, int arg1)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_insert_vfork_catchpoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_insert_vfork_catchpoint (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_insert_vfork_catchpoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_remove_vfork_catchpoint (struct target_ops *self, int arg1)
{
  self = self->beneath;
  return self->to_remove_vfork_catchpoint (self, arg1);
}

static int
tdefault_remove_vfork_catchpoint (struct target_ops *self, int arg1)
{
  return 1;
}

static int
debug_remove_vfork_catchpoint (struct target_ops *self, int arg1)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_remove_vfork_catchpoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_remove_vfork_catchpoint (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_remove_vfork_catchpoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_follow_fork (struct target_ops *self, int arg1, int arg2)
{
  self = self->beneath;
  return self->to_follow_fork (self, arg1, arg2);
}

static int
debug_follow_fork (struct target_ops *self, int arg1, int arg2)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_follow_fork (...)\n", debug_target.to_shortname);
  result = debug_target.to_follow_fork (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_follow_fork (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_insert_exec_catchpoint (struct target_ops *self, int arg1)
{
  self = self->beneath;
  return self->to_insert_exec_catchpoint (self, arg1);
}

static int
tdefault_insert_exec_catchpoint (struct target_ops *self, int arg1)
{
  return 1;
}

static int
debug_insert_exec_catchpoint (struct target_ops *self, int arg1)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_insert_exec_catchpoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_insert_exec_catchpoint (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_insert_exec_catchpoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_remove_exec_catchpoint (struct target_ops *self, int arg1)
{
  self = self->beneath;
  return self->to_remove_exec_catchpoint (self, arg1);
}

static int
tdefault_remove_exec_catchpoint (struct target_ops *self, int arg1)
{
  return 1;
}

static int
debug_remove_exec_catchpoint (struct target_ops *self, int arg1)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_remove_exec_catchpoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_remove_exec_catchpoint (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_remove_exec_catchpoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static void
delegate_follow_exec (struct target_ops *self, struct inferior *arg1, char *arg2)
{
  self = self->beneath;
  self->to_follow_exec (self, arg1, arg2);
}

static void
tdefault_follow_exec (struct target_ops *self, struct inferior *arg1, char *arg2)
{
}

static void
debug_follow_exec (struct target_ops *self, struct inferior *arg1, char *arg2)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_follow_exec (...)\n", debug_target.to_shortname);
  debug_target.to_follow_exec (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_follow_exec (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_inferior_p (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_char_p (arg2);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static int
delegate_set_syscall_catchpoint (struct target_ops *self, int arg1, int arg2, int arg3, int arg4, int *arg5)
{
  self = self->beneath;
  return self->to_set_syscall_catchpoint (self, arg1, arg2, arg3, arg4, arg5);
}

static int
tdefault_set_syscall_catchpoint (struct target_ops *self, int arg1, int arg2, int arg3, int arg4, int *arg5)
{
  return 1;
}

static int
debug_set_syscall_catchpoint (struct target_ops *self, int arg1, int arg2, int arg3, int arg4, int *arg5)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_set_syscall_catchpoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_set_syscall_catchpoint (&debug_target, arg1, arg2, arg3, arg4, arg5);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_set_syscall_catchpoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg3);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg4);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int_p (arg5);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_has_exited (struct target_ops *self, int arg1, int arg2, int *arg3)
{
  self = self->beneath;
  return self->to_has_exited (self, arg1, arg2, arg3);
}

static int
tdefault_has_exited (struct target_ops *self, int arg1, int arg2, int *arg3)
{
  return 0;
}

static int
debug_has_exited (struct target_ops *self, int arg1, int arg2, int *arg3)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_has_exited (...)\n", debug_target.to_shortname);
  result = debug_target.to_has_exited (&debug_target, arg1, arg2, arg3);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_has_exited (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int_p (arg3);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static void
delegate_mourn_inferior (struct target_ops *self)
{
  self = self->beneath;
  self->to_mourn_inferior (self);
}

static void
debug_mourn_inferior (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_mourn_inferior (...)\n", debug_target.to_shortname);
  debug_target.to_mourn_inferior (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_mourn_inferior (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static int
delegate_can_run (struct target_ops *self)
{
  self = self->beneath;
  return self->to_can_run (self);
}

static int
tdefault_can_run (struct target_ops *self)
{
  return 0;
}

static int
debug_can_run (struct target_ops *self)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_can_run (...)\n", debug_target.to_shortname);
  result = debug_target.to_can_run (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_can_run (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static void
delegate_pass_signals (struct target_ops *self, int arg1, unsigned char * arg2)
{
  self = self->beneath;
  self->to_pass_signals (self, arg1, arg2);
}

static void
tdefault_pass_signals (struct target_ops *self, int arg1, unsigned char * arg2)
{
}

static void
debug_pass_signals (struct target_ops *self, int arg1, unsigned char * arg2)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_pass_signals (...)\n", debug_target.to_shortname);
  debug_target.to_pass_signals (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_pass_signals (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_signals (arg2);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_program_signals (struct target_ops *self, int arg1, unsigned char * arg2)
{
  self = self->beneath;
  self->to_program_signals (self, arg1, arg2);
}

static void
tdefault_program_signals (struct target_ops *self, int arg1, unsigned char * arg2)
{
}

static void
debug_program_signals (struct target_ops *self, int arg1, unsigned char * arg2)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_program_signals (...)\n", debug_target.to_shortname);
  debug_target.to_program_signals (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_program_signals (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_signals (arg2);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static int
delegate_thread_alive (struct target_ops *self, ptid_t arg1)
{
  self = self->beneath;
  return self->to_thread_alive (self, arg1);
}

static int
tdefault_thread_alive (struct target_ops *self, ptid_t arg1)
{
  return 0;
}

static int
debug_thread_alive (struct target_ops *self, ptid_t arg1)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_thread_alive (...)\n", debug_target.to_shortname);
  result = debug_target.to_thread_alive (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_thread_alive (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ptid_t (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static void
delegate_update_thread_list (struct target_ops *self)
{
  self = self->beneath;
  self->to_update_thread_list (self);
}

static void
tdefault_update_thread_list (struct target_ops *self)
{
}

static void
debug_update_thread_list (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_update_thread_list (...)\n", debug_target.to_shortname);
  debug_target.to_update_thread_list (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_update_thread_list (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static const char *
delegate_pid_to_str (struct target_ops *self, ptid_t arg1)
{
  self = self->beneath;
  return self->to_pid_to_str (self, arg1);
}

static const char *
debug_pid_to_str (struct target_ops *self, ptid_t arg1)
{
  const char * result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_pid_to_str (...)\n", debug_target.to_shortname);
  result = debug_target.to_pid_to_str (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_pid_to_str (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ptid_t (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_const_char_p (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static const char *
delegate_extra_thread_info (struct target_ops *self, struct thread_info *arg1)
{
  self = self->beneath;
  return self->to_extra_thread_info (self, arg1);
}

static const char *
tdefault_extra_thread_info (struct target_ops *self, struct thread_info *arg1)
{
  return NULL;
}

static const char *
debug_extra_thread_info (struct target_ops *self, struct thread_info *arg1)
{
  const char * result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_extra_thread_info (...)\n", debug_target.to_shortname);
  result = debug_target.to_extra_thread_info (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_extra_thread_info (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_thread_info_p (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_const_char_p (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static const char *
delegate_thread_name (struct target_ops *self, struct thread_info *arg1)
{
  self = self->beneath;
  return self->to_thread_name (self, arg1);
}

static const char *
tdefault_thread_name (struct target_ops *self, struct thread_info *arg1)
{
  return NULL;
}

static const char *
debug_thread_name (struct target_ops *self, struct thread_info *arg1)
{
  const char * result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_thread_name (...)\n", debug_target.to_shortname);
  result = debug_target.to_thread_name (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_thread_name (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_thread_info_p (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_const_char_p (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static void
delegate_stop (struct target_ops *self, ptid_t arg1)
{
  self = self->beneath;
  self->to_stop (self, arg1);
}

static void
tdefault_stop (struct target_ops *self, ptid_t arg1)
{
}

static void
debug_stop (struct target_ops *self, ptid_t arg1)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_stop (...)\n", debug_target.to_shortname);
  debug_target.to_stop (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_stop (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ptid_t (arg1);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_interrupt (struct target_ops *self, ptid_t arg1)
{
  self = self->beneath;
  self->to_interrupt (self, arg1);
}

static void
tdefault_interrupt (struct target_ops *self, ptid_t arg1)
{
}

static void
debug_interrupt (struct target_ops *self, ptid_t arg1)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_interrupt (...)\n", debug_target.to_shortname);
  debug_target.to_interrupt (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_interrupt (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ptid_t (arg1);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_pass_ctrlc (struct target_ops *self)
{
  self = self->beneath;
  self->to_pass_ctrlc (self);
}

static void
debug_pass_ctrlc (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_pass_ctrlc (...)\n", debug_target.to_shortname);
  debug_target.to_pass_ctrlc (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_pass_ctrlc (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_rcmd (struct target_ops *self, const char *arg1, struct ui_file *arg2)
{
  self = self->beneath;
  self->to_rcmd (self, arg1, arg2);
}

static void
debug_rcmd (struct target_ops *self, const char *arg1, struct ui_file *arg2)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_rcmd (...)\n", debug_target.to_shortname);
  debug_target.to_rcmd (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_rcmd (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_const_char_p (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_ui_file_p (arg2);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static char *
delegate_pid_to_exec_file (struct target_ops *self, int arg1)
{
  self = self->beneath;
  return self->to_pid_to_exec_file (self, arg1);
}

static char *
tdefault_pid_to_exec_file (struct target_ops *self, int arg1)
{
  return NULL;
}

static char *
debug_pid_to_exec_file (struct target_ops *self, int arg1)
{
  char * result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_pid_to_exec_file (...)\n", debug_target.to_shortname);
  result = debug_target.to_pid_to_exec_file (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_pid_to_exec_file (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_char_p (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static void
delegate_log_command (struct target_ops *self, const char *arg1)
{
  self = self->beneath;
  self->to_log_command (self, arg1);
}

static void
tdefault_log_command (struct target_ops *self, const char *arg1)
{
}

static void
debug_log_command (struct target_ops *self, const char *arg1)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_log_command (...)\n", debug_target.to_shortname);
  debug_target.to_log_command (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_log_command (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_const_char_p (arg1);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static struct target_section_table *
delegate_get_section_table (struct target_ops *self)
{
  self = self->beneath;
  return self->to_get_section_table (self);
}

static struct target_section_table *
tdefault_get_section_table (struct target_ops *self)
{
  return NULL;
}

static struct target_section_table *
debug_get_section_table (struct target_ops *self)
{
  struct target_section_table * result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_get_section_table (...)\n", debug_target.to_shortname);
  result = debug_target.to_get_section_table (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_get_section_table (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_struct_target_section_table_p (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_can_async_p (struct target_ops *self)
{
  self = self->beneath;
  return self->to_can_async_p (self);
}

static int
tdefault_can_async_p (struct target_ops *self)
{
  return 0;
}

static int
debug_can_async_p (struct target_ops *self)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_can_async_p (...)\n", debug_target.to_shortname);
  result = debug_target.to_can_async_p (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_can_async_p (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_is_async_p (struct target_ops *self)
{
  self = self->beneath;
  return self->to_is_async_p (self);
}

static int
tdefault_is_async_p (struct target_ops *self)
{
  return 0;
}

static int
debug_is_async_p (struct target_ops *self)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_is_async_p (...)\n", debug_target.to_shortname);
  result = debug_target.to_is_async_p (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_is_async_p (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static void
delegate_async (struct target_ops *self, int arg1)
{
  self = self->beneath;
  self->to_async (self, arg1);
}

static void
tdefault_async (struct target_ops *self, int arg1)
{
  tcomplain ();
}

static void
debug_async (struct target_ops *self, int arg1)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_async (...)\n", debug_target.to_shortname);
  debug_target.to_async (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_async (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_thread_events (struct target_ops *self, int arg1)
{
  self = self->beneath;
  self->to_thread_events (self, arg1);
}

static void
tdefault_thread_events (struct target_ops *self, int arg1)
{
}

static void
debug_thread_events (struct target_ops *self, int arg1)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_thread_events (...)\n", debug_target.to_shortname);
  debug_target.to_thread_events (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_thread_events (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static int
delegate_supports_non_stop (struct target_ops *self)
{
  self = self->beneath;
  return self->to_supports_non_stop (self);
}

static int
tdefault_supports_non_stop (struct target_ops *self)
{
  return 0;
}

static int
debug_supports_non_stop (struct target_ops *self)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_supports_non_stop (...)\n", debug_target.to_shortname);
  result = debug_target.to_supports_non_stop (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_supports_non_stop (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_always_non_stop_p (struct target_ops *self)
{
  self = self->beneath;
  return self->to_always_non_stop_p (self);
}

static int
tdefault_always_non_stop_p (struct target_ops *self)
{
  return 0;
}

static int
debug_always_non_stop_p (struct target_ops *self)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_always_non_stop_p (...)\n", debug_target.to_shortname);
  result = debug_target.to_always_non_stop_p (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_always_non_stop_p (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_find_memory_regions (struct target_ops *self, find_memory_region_ftype arg1, void *arg2)
{
  self = self->beneath;
  return self->to_find_memory_regions (self, arg1, arg2);
}

static int
debug_find_memory_regions (struct target_ops *self, find_memory_region_ftype arg1, void *arg2)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_find_memory_regions (...)\n", debug_target.to_shortname);
  result = debug_target.to_find_memory_regions (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_find_memory_regions (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_find_memory_region_ftype (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_void_p (arg2);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static char *
delegate_make_corefile_notes (struct target_ops *self, bfd *arg1, int *arg2)
{
  self = self->beneath;
  return self->to_make_corefile_notes (self, arg1, arg2);
}

static char *
debug_make_corefile_notes (struct target_ops *self, bfd *arg1, int *arg2)
{
  char * result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_make_corefile_notes (...)\n", debug_target.to_shortname);
  result = debug_target.to_make_corefile_notes (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_make_corefile_notes (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_bfd_p (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int_p (arg2);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_char_p (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static gdb_byte *
delegate_get_bookmark (struct target_ops *self, const char *arg1, int arg2)
{
  self = self->beneath;
  return self->to_get_bookmark (self, arg1, arg2);
}

static gdb_byte *
tdefault_get_bookmark (struct target_ops *self, const char *arg1, int arg2)
{
  tcomplain ();
}

static gdb_byte *
debug_get_bookmark (struct target_ops *self, const char *arg1, int arg2)
{
  gdb_byte * result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_get_bookmark (...)\n", debug_target.to_shortname);
  result = debug_target.to_get_bookmark (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_get_bookmark (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_const_char_p (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_gdb_byte_p (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static void
delegate_goto_bookmark (struct target_ops *self, const gdb_byte *arg1, int arg2)
{
  self = self->beneath;
  self->to_goto_bookmark (self, arg1, arg2);
}

static void
tdefault_goto_bookmark (struct target_ops *self, const gdb_byte *arg1, int arg2)
{
  tcomplain ();
}

static void
debug_goto_bookmark (struct target_ops *self, const gdb_byte *arg1, int arg2)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_goto_bookmark (...)\n", debug_target.to_shortname);
  debug_target.to_goto_bookmark (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_goto_bookmark (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_const_gdb_byte_p (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static CORE_ADDR
delegate_get_thread_local_address (struct target_ops *self, ptid_t arg1, CORE_ADDR arg2, CORE_ADDR arg3)
{
  self = self->beneath;
  return self->to_get_thread_local_address (self, arg1, arg2, arg3);
}

static CORE_ADDR
tdefault_get_thread_local_address (struct target_ops *self, ptid_t arg1, CORE_ADDR arg2, CORE_ADDR arg3)
{
  generic_tls_error ();
}

static CORE_ADDR
debug_get_thread_local_address (struct target_ops *self, ptid_t arg1, CORE_ADDR arg2, CORE_ADDR arg3)
{
  CORE_ADDR result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_get_thread_local_address (...)\n", debug_target.to_shortname);
  result = debug_target.to_get_thread_local_address (&debug_target, arg1, arg2, arg3);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_get_thread_local_address (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ptid_t (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg3);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_CORE_ADDR (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static enum target_xfer_status
delegate_xfer_partial (struct target_ops *self, enum target_object arg1, const char *arg2, gdb_byte *arg3, const gdb_byte *arg4, ULONGEST arg5, ULONGEST arg6, ULONGEST *arg7)
{
  self = self->beneath;
  return self->to_xfer_partial (self, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
}

static enum target_xfer_status
tdefault_xfer_partial (struct target_ops *self, enum target_object arg1, const char *arg2, gdb_byte *arg3, const gdb_byte *arg4, ULONGEST arg5, ULONGEST arg6, ULONGEST *arg7)
{
  return TARGET_XFER_E_IO;
}

static enum target_xfer_status
debug_xfer_partial (struct target_ops *self, enum target_object arg1, const char *arg2, gdb_byte *arg3, const gdb_byte *arg4, ULONGEST arg5, ULONGEST arg6, ULONGEST *arg7)
{
  enum target_xfer_status result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_xfer_partial (...)\n", debug_target.to_shortname);
  result = debug_target.to_xfer_partial (&debug_target, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_xfer_partial (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_enum_target_object (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_const_char_p (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_gdb_byte_p (arg3);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_const_gdb_byte_p (arg4);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ULONGEST (arg5);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ULONGEST (arg6);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ULONGEST_p (arg7);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_enum_target_xfer_status (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static ULONGEST
delegate_get_memory_xfer_limit (struct target_ops *self)
{
  self = self->beneath;
  return self->to_get_memory_xfer_limit (self);
}

static ULONGEST
tdefault_get_memory_xfer_limit (struct target_ops *self)
{
  return ULONGEST_MAX;
}

static ULONGEST
debug_get_memory_xfer_limit (struct target_ops *self)
{
  ULONGEST result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_get_memory_xfer_limit (...)\n", debug_target.to_shortname);
  result = debug_target.to_get_memory_xfer_limit (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_get_memory_xfer_limit (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_ULONGEST (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static VEC(mem_region_s) *
delegate_memory_map (struct target_ops *self)
{
  self = self->beneath;
  return self->to_memory_map (self);
}

static VEC(mem_region_s) *
tdefault_memory_map (struct target_ops *self)
{
  return NULL;
}

static VEC(mem_region_s) *
debug_memory_map (struct target_ops *self)
{
  VEC(mem_region_s) * result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_memory_map (...)\n", debug_target.to_shortname);
  result = debug_target.to_memory_map (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_memory_map (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_VEC_mem_region_s__p (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static void
delegate_flash_erase (struct target_ops *self, ULONGEST arg1, LONGEST arg2)
{
  self = self->beneath;
  self->to_flash_erase (self, arg1, arg2);
}

static void
tdefault_flash_erase (struct target_ops *self, ULONGEST arg1, LONGEST arg2)
{
  tcomplain ();
}

static void
debug_flash_erase (struct target_ops *self, ULONGEST arg1, LONGEST arg2)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_flash_erase (...)\n", debug_target.to_shortname);
  debug_target.to_flash_erase (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_flash_erase (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ULONGEST (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_LONGEST (arg2);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_flash_done (struct target_ops *self)
{
  self = self->beneath;
  self->to_flash_done (self);
}

static void
tdefault_flash_done (struct target_ops *self)
{
  tcomplain ();
}

static void
debug_flash_done (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_flash_done (...)\n", debug_target.to_shortname);
  debug_target.to_flash_done (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_flash_done (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static const struct target_desc *
delegate_read_description (struct target_ops *self)
{
  self = self->beneath;
  return self->to_read_description (self);
}

static const struct target_desc *
tdefault_read_description (struct target_ops *self)
{
  return NULL;
}

static const struct target_desc *
debug_read_description (struct target_ops *self)
{
  const struct target_desc * result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_read_description (...)\n", debug_target.to_shortname);
  result = debug_target.to_read_description (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_read_description (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_const_struct_target_desc_p (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static ptid_t
delegate_get_ada_task_ptid (struct target_ops *self, long arg1, long arg2)
{
  self = self->beneath;
  return self->to_get_ada_task_ptid (self, arg1, arg2);
}

static ptid_t
debug_get_ada_task_ptid (struct target_ops *self, long arg1, long arg2)
{
  ptid_t result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_get_ada_task_ptid (...)\n", debug_target.to_shortname);
  result = debug_target.to_get_ada_task_ptid (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_get_ada_task_ptid (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_long (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_long (arg2);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_ptid_t (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_auxv_parse (struct target_ops *self, gdb_byte **arg1, gdb_byte *arg2, CORE_ADDR *arg3, CORE_ADDR *arg4)
{
  self = self->beneath;
  return self->to_auxv_parse (self, arg1, arg2, arg3, arg4);
}

static int
debug_auxv_parse (struct target_ops *self, gdb_byte **arg1, gdb_byte *arg2, CORE_ADDR *arg3, CORE_ADDR *arg4)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_auxv_parse (...)\n", debug_target.to_shortname);
  result = debug_target.to_auxv_parse (&debug_target, arg1, arg2, arg3, arg4);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_auxv_parse (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_gdb_byte_pp (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_gdb_byte_p (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR_p (arg3);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR_p (arg4);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_search_memory (struct target_ops *self, CORE_ADDR arg1, ULONGEST arg2, const gdb_byte *arg3, ULONGEST arg4, CORE_ADDR *arg5)
{
  self = self->beneath;
  return self->to_search_memory (self, arg1, arg2, arg3, arg4, arg5);
}

static int
debug_search_memory (struct target_ops *self, CORE_ADDR arg1, ULONGEST arg2, const gdb_byte *arg3, ULONGEST arg4, CORE_ADDR *arg5)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_search_memory (...)\n", debug_target.to_shortname);
  result = debug_target.to_search_memory (&debug_target, arg1, arg2, arg3, arg4, arg5);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_search_memory (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ULONGEST (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_const_gdb_byte_p (arg3);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ULONGEST (arg4);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR_p (arg5);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_can_execute_reverse (struct target_ops *self)
{
  self = self->beneath;
  return self->to_can_execute_reverse (self);
}

static int
tdefault_can_execute_reverse (struct target_ops *self)
{
  return 0;
}

static int
debug_can_execute_reverse (struct target_ops *self)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_can_execute_reverse (...)\n", debug_target.to_shortname);
  result = debug_target.to_can_execute_reverse (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_can_execute_reverse (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static enum exec_direction_kind
delegate_execution_direction (struct target_ops *self)
{
  self = self->beneath;
  return self->to_execution_direction (self);
}

static enum exec_direction_kind
debug_execution_direction (struct target_ops *self)
{
  enum exec_direction_kind result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_execution_direction (...)\n", debug_target.to_shortname);
  result = debug_target.to_execution_direction (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_execution_direction (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_enum_exec_direction_kind (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_supports_multi_process (struct target_ops *self)
{
  self = self->beneath;
  return self->to_supports_multi_process (self);
}

static int
tdefault_supports_multi_process (struct target_ops *self)
{
  return 0;
}

static int
debug_supports_multi_process (struct target_ops *self)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_supports_multi_process (...)\n", debug_target.to_shortname);
  result = debug_target.to_supports_multi_process (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_supports_multi_process (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_supports_enable_disable_tracepoint (struct target_ops *self)
{
  self = self->beneath;
  return self->to_supports_enable_disable_tracepoint (self);
}

static int
tdefault_supports_enable_disable_tracepoint (struct target_ops *self)
{
  return 0;
}

static int
debug_supports_enable_disable_tracepoint (struct target_ops *self)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_supports_enable_disable_tracepoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_supports_enable_disable_tracepoint (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_supports_enable_disable_tracepoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_supports_string_tracing (struct target_ops *self)
{
  self = self->beneath;
  return self->to_supports_string_tracing (self);
}

static int
tdefault_supports_string_tracing (struct target_ops *self)
{
  return 0;
}

static int
debug_supports_string_tracing (struct target_ops *self)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_supports_string_tracing (...)\n", debug_target.to_shortname);
  result = debug_target.to_supports_string_tracing (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_supports_string_tracing (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_supports_evaluation_of_breakpoint_conditions (struct target_ops *self)
{
  self = self->beneath;
  return self->to_supports_evaluation_of_breakpoint_conditions (self);
}

static int
tdefault_supports_evaluation_of_breakpoint_conditions (struct target_ops *self)
{
  return 0;
}

static int
debug_supports_evaluation_of_breakpoint_conditions (struct target_ops *self)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_supports_evaluation_of_breakpoint_conditions (...)\n", debug_target.to_shortname);
  result = debug_target.to_supports_evaluation_of_breakpoint_conditions (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_supports_evaluation_of_breakpoint_conditions (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_can_run_breakpoint_commands (struct target_ops *self)
{
  self = self->beneath;
  return self->to_can_run_breakpoint_commands (self);
}

static int
tdefault_can_run_breakpoint_commands (struct target_ops *self)
{
  return 0;
}

static int
debug_can_run_breakpoint_commands (struct target_ops *self)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_can_run_breakpoint_commands (...)\n", debug_target.to_shortname);
  result = debug_target.to_can_run_breakpoint_commands (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_can_run_breakpoint_commands (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static struct gdbarch *
delegate_thread_architecture (struct target_ops *self, ptid_t arg1)
{
  self = self->beneath;
  return self->to_thread_architecture (self, arg1);
}

static struct gdbarch *
debug_thread_architecture (struct target_ops *self, ptid_t arg1)
{
  struct gdbarch * result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_thread_architecture (...)\n", debug_target.to_shortname);
  result = debug_target.to_thread_architecture (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_thread_architecture (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ptid_t (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_struct_gdbarch_p (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static struct address_space *
delegate_thread_address_space (struct target_ops *self, ptid_t arg1)
{
  self = self->beneath;
  return self->to_thread_address_space (self, arg1);
}

static struct address_space *
debug_thread_address_space (struct target_ops *self, ptid_t arg1)
{
  struct address_space * result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_thread_address_space (...)\n", debug_target.to_shortname);
  result = debug_target.to_thread_address_space (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_thread_address_space (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ptid_t (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_struct_address_space_p (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_filesystem_is_local (struct target_ops *self)
{
  self = self->beneath;
  return self->to_filesystem_is_local (self);
}

static int
tdefault_filesystem_is_local (struct target_ops *self)
{
  return 1;
}

static int
debug_filesystem_is_local (struct target_ops *self)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_filesystem_is_local (...)\n", debug_target.to_shortname);
  result = debug_target.to_filesystem_is_local (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_filesystem_is_local (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static void
delegate_trace_init (struct target_ops *self)
{
  self = self->beneath;
  self->to_trace_init (self);
}

static void
tdefault_trace_init (struct target_ops *self)
{
  tcomplain ();
}

static void
debug_trace_init (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_trace_init (...)\n", debug_target.to_shortname);
  debug_target.to_trace_init (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_trace_init (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_download_tracepoint (struct target_ops *self, struct bp_location *arg1)
{
  self = self->beneath;
  self->to_download_tracepoint (self, arg1);
}

static void
tdefault_download_tracepoint (struct target_ops *self, struct bp_location *arg1)
{
  tcomplain ();
}

static void
debug_download_tracepoint (struct target_ops *self, struct bp_location *arg1)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_download_tracepoint (...)\n", debug_target.to_shortname);
  debug_target.to_download_tracepoint (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_download_tracepoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_bp_location_p (arg1);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static int
delegate_can_download_tracepoint (struct target_ops *self)
{
  self = self->beneath;
  return self->to_can_download_tracepoint (self);
}

static int
tdefault_can_download_tracepoint (struct target_ops *self)
{
  return 0;
}

static int
debug_can_download_tracepoint (struct target_ops *self)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_can_download_tracepoint (...)\n", debug_target.to_shortname);
  result = debug_target.to_can_download_tracepoint (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_can_download_tracepoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static void
delegate_download_trace_state_variable (struct target_ops *self, struct trace_state_variable *arg1)
{
  self = self->beneath;
  self->to_download_trace_state_variable (self, arg1);
}

static void
tdefault_download_trace_state_variable (struct target_ops *self, struct trace_state_variable *arg1)
{
  tcomplain ();
}

static void
debug_download_trace_state_variable (struct target_ops *self, struct trace_state_variable *arg1)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_download_trace_state_variable (...)\n", debug_target.to_shortname);
  debug_target.to_download_trace_state_variable (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_download_trace_state_variable (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_trace_state_variable_p (arg1);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_enable_tracepoint (struct target_ops *self, struct bp_location *arg1)
{
  self = self->beneath;
  self->to_enable_tracepoint (self, arg1);
}

static void
tdefault_enable_tracepoint (struct target_ops *self, struct bp_location *arg1)
{
  tcomplain ();
}

static void
debug_enable_tracepoint (struct target_ops *self, struct bp_location *arg1)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_enable_tracepoint (...)\n", debug_target.to_shortname);
  debug_target.to_enable_tracepoint (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_enable_tracepoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_bp_location_p (arg1);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_disable_tracepoint (struct target_ops *self, struct bp_location *arg1)
{
  self = self->beneath;
  self->to_disable_tracepoint (self, arg1);
}

static void
tdefault_disable_tracepoint (struct target_ops *self, struct bp_location *arg1)
{
  tcomplain ();
}

static void
debug_disable_tracepoint (struct target_ops *self, struct bp_location *arg1)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_disable_tracepoint (...)\n", debug_target.to_shortname);
  debug_target.to_disable_tracepoint (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_disable_tracepoint (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_bp_location_p (arg1);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_trace_set_readonly_regions (struct target_ops *self)
{
  self = self->beneath;
  self->to_trace_set_readonly_regions (self);
}

static void
tdefault_trace_set_readonly_regions (struct target_ops *self)
{
  tcomplain ();
}

static void
debug_trace_set_readonly_regions (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_trace_set_readonly_regions (...)\n", debug_target.to_shortname);
  debug_target.to_trace_set_readonly_regions (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_trace_set_readonly_regions (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_trace_start (struct target_ops *self)
{
  self = self->beneath;
  self->to_trace_start (self);
}

static void
tdefault_trace_start (struct target_ops *self)
{
  tcomplain ();
}

static void
debug_trace_start (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_trace_start (...)\n", debug_target.to_shortname);
  debug_target.to_trace_start (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_trace_start (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static int
delegate_get_trace_status (struct target_ops *self, struct trace_status *arg1)
{
  self = self->beneath;
  return self->to_get_trace_status (self, arg1);
}

static int
tdefault_get_trace_status (struct target_ops *self, struct trace_status *arg1)
{
  return -1;
}

static int
debug_get_trace_status (struct target_ops *self, struct trace_status *arg1)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_get_trace_status (...)\n", debug_target.to_shortname);
  result = debug_target.to_get_trace_status (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_get_trace_status (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_trace_status_p (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static void
delegate_get_tracepoint_status (struct target_ops *self, struct breakpoint *arg1, struct uploaded_tp *arg2)
{
  self = self->beneath;
  self->to_get_tracepoint_status (self, arg1, arg2);
}

static void
tdefault_get_tracepoint_status (struct target_ops *self, struct breakpoint *arg1, struct uploaded_tp *arg2)
{
  tcomplain ();
}

static void
debug_get_tracepoint_status (struct target_ops *self, struct breakpoint *arg1, struct uploaded_tp *arg2)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_get_tracepoint_status (...)\n", debug_target.to_shortname);
  debug_target.to_get_tracepoint_status (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_get_tracepoint_status (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_breakpoint_p (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_uploaded_tp_p (arg2);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_trace_stop (struct target_ops *self)
{
  self = self->beneath;
  self->to_trace_stop (self);
}

static void
tdefault_trace_stop (struct target_ops *self)
{
  tcomplain ();
}

static void
debug_trace_stop (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_trace_stop (...)\n", debug_target.to_shortname);
  debug_target.to_trace_stop (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_trace_stop (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static int
delegate_trace_find (struct target_ops *self, enum trace_find_type arg1, int arg2, CORE_ADDR arg3, CORE_ADDR arg4, int *arg5)
{
  self = self->beneath;
  return self->to_trace_find (self, arg1, arg2, arg3, arg4, arg5);
}

static int
tdefault_trace_find (struct target_ops *self, enum trace_find_type arg1, int arg2, CORE_ADDR arg3, CORE_ADDR arg4, int *arg5)
{
  return -1;
}

static int
debug_trace_find (struct target_ops *self, enum trace_find_type arg1, int arg2, CORE_ADDR arg3, CORE_ADDR arg4, int *arg5)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_trace_find (...)\n", debug_target.to_shortname);
  result = debug_target.to_trace_find (&debug_target, arg1, arg2, arg3, arg4, arg5);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_trace_find (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_enum_trace_find_type (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg3);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg4);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int_p (arg5);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_get_trace_state_variable_value (struct target_ops *self, int arg1, LONGEST *arg2)
{
  self = self->beneath;
  return self->to_get_trace_state_variable_value (self, arg1, arg2);
}

static int
tdefault_get_trace_state_variable_value (struct target_ops *self, int arg1, LONGEST *arg2)
{
  return 0;
}

static int
debug_get_trace_state_variable_value (struct target_ops *self, int arg1, LONGEST *arg2)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_get_trace_state_variable_value (...)\n", debug_target.to_shortname);
  result = debug_target.to_get_trace_state_variable_value (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_get_trace_state_variable_value (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_LONGEST_p (arg2);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_save_trace_data (struct target_ops *self, const char *arg1)
{
  self = self->beneath;
  return self->to_save_trace_data (self, arg1);
}

static int
tdefault_save_trace_data (struct target_ops *self, const char *arg1)
{
  tcomplain ();
}

static int
debug_save_trace_data (struct target_ops *self, const char *arg1)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_save_trace_data (...)\n", debug_target.to_shortname);
  result = debug_target.to_save_trace_data (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_save_trace_data (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_const_char_p (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_upload_tracepoints (struct target_ops *self, struct uploaded_tp **arg1)
{
  self = self->beneath;
  return self->to_upload_tracepoints (self, arg1);
}

static int
tdefault_upload_tracepoints (struct target_ops *self, struct uploaded_tp **arg1)
{
  return 0;
}

static int
debug_upload_tracepoints (struct target_ops *self, struct uploaded_tp **arg1)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_upload_tracepoints (...)\n", debug_target.to_shortname);
  result = debug_target.to_upload_tracepoints (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_upload_tracepoints (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_uploaded_tp_pp (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_upload_trace_state_variables (struct target_ops *self, struct uploaded_tsv **arg1)
{
  self = self->beneath;
  return self->to_upload_trace_state_variables (self, arg1);
}

static int
tdefault_upload_trace_state_variables (struct target_ops *self, struct uploaded_tsv **arg1)
{
  return 0;
}

static int
debug_upload_trace_state_variables (struct target_ops *self, struct uploaded_tsv **arg1)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_upload_trace_state_variables (...)\n", debug_target.to_shortname);
  result = debug_target.to_upload_trace_state_variables (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_upload_trace_state_variables (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_uploaded_tsv_pp (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static LONGEST
delegate_get_raw_trace_data (struct target_ops *self, gdb_byte *arg1, ULONGEST arg2, LONGEST arg3)
{
  self = self->beneath;
  return self->to_get_raw_trace_data (self, arg1, arg2, arg3);
}

static LONGEST
tdefault_get_raw_trace_data (struct target_ops *self, gdb_byte *arg1, ULONGEST arg2, LONGEST arg3)
{
  tcomplain ();
}

static LONGEST
debug_get_raw_trace_data (struct target_ops *self, gdb_byte *arg1, ULONGEST arg2, LONGEST arg3)
{
  LONGEST result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_get_raw_trace_data (...)\n", debug_target.to_shortname);
  result = debug_target.to_get_raw_trace_data (&debug_target, arg1, arg2, arg3);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_get_raw_trace_data (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_gdb_byte_p (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ULONGEST (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_LONGEST (arg3);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_LONGEST (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_get_min_fast_tracepoint_insn_len (struct target_ops *self)
{
  self = self->beneath;
  return self->to_get_min_fast_tracepoint_insn_len (self);
}

static int
tdefault_get_min_fast_tracepoint_insn_len (struct target_ops *self)
{
  return -1;
}

static int
debug_get_min_fast_tracepoint_insn_len (struct target_ops *self)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_get_min_fast_tracepoint_insn_len (...)\n", debug_target.to_shortname);
  result = debug_target.to_get_min_fast_tracepoint_insn_len (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_get_min_fast_tracepoint_insn_len (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static void
delegate_set_disconnected_tracing (struct target_ops *self, int arg1)
{
  self = self->beneath;
  self->to_set_disconnected_tracing (self, arg1);
}

static void
tdefault_set_disconnected_tracing (struct target_ops *self, int arg1)
{
}

static void
debug_set_disconnected_tracing (struct target_ops *self, int arg1)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_set_disconnected_tracing (...)\n", debug_target.to_shortname);
  debug_target.to_set_disconnected_tracing (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_set_disconnected_tracing (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_set_circular_trace_buffer (struct target_ops *self, int arg1)
{
  self = self->beneath;
  self->to_set_circular_trace_buffer (self, arg1);
}

static void
tdefault_set_circular_trace_buffer (struct target_ops *self, int arg1)
{
}

static void
debug_set_circular_trace_buffer (struct target_ops *self, int arg1)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_set_circular_trace_buffer (...)\n", debug_target.to_shortname);
  debug_target.to_set_circular_trace_buffer (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_set_circular_trace_buffer (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_set_trace_buffer_size (struct target_ops *self, LONGEST arg1)
{
  self = self->beneath;
  self->to_set_trace_buffer_size (self, arg1);
}

static void
tdefault_set_trace_buffer_size (struct target_ops *self, LONGEST arg1)
{
}

static void
debug_set_trace_buffer_size (struct target_ops *self, LONGEST arg1)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_set_trace_buffer_size (...)\n", debug_target.to_shortname);
  debug_target.to_set_trace_buffer_size (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_set_trace_buffer_size (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_LONGEST (arg1);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static int
delegate_set_trace_notes (struct target_ops *self, const char *arg1, const char *arg2, const char *arg3)
{
  self = self->beneath;
  return self->to_set_trace_notes (self, arg1, arg2, arg3);
}

static int
tdefault_set_trace_notes (struct target_ops *self, const char *arg1, const char *arg2, const char *arg3)
{
  return 0;
}

static int
debug_set_trace_notes (struct target_ops *self, const char *arg1, const char *arg2, const char *arg3)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_set_trace_notes (...)\n", debug_target.to_shortname);
  result = debug_target.to_set_trace_notes (&debug_target, arg1, arg2, arg3);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_set_trace_notes (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_const_char_p (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_const_char_p (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_const_char_p (arg3);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_core_of_thread (struct target_ops *self, ptid_t arg1)
{
  self = self->beneath;
  return self->to_core_of_thread (self, arg1);
}

static int
tdefault_core_of_thread (struct target_ops *self, ptid_t arg1)
{
  return -1;
}

static int
debug_core_of_thread (struct target_ops *self, ptid_t arg1)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_core_of_thread (...)\n", debug_target.to_shortname);
  result = debug_target.to_core_of_thread (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_core_of_thread (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ptid_t (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_verify_memory (struct target_ops *self, const gdb_byte *arg1, CORE_ADDR arg2, ULONGEST arg3)
{
  self = self->beneath;
  return self->to_verify_memory (self, arg1, arg2, arg3);
}

static int
debug_verify_memory (struct target_ops *self, const gdb_byte *arg1, CORE_ADDR arg2, ULONGEST arg3)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_verify_memory (...)\n", debug_target.to_shortname);
  result = debug_target.to_verify_memory (&debug_target, arg1, arg2, arg3);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_verify_memory (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_const_gdb_byte_p (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ULONGEST (arg3);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_get_tib_address (struct target_ops *self, ptid_t arg1, CORE_ADDR *arg2)
{
  self = self->beneath;
  return self->to_get_tib_address (self, arg1, arg2);
}

static int
tdefault_get_tib_address (struct target_ops *self, ptid_t arg1, CORE_ADDR *arg2)
{
  tcomplain ();
}

static int
debug_get_tib_address (struct target_ops *self, ptid_t arg1, CORE_ADDR *arg2)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_get_tib_address (...)\n", debug_target.to_shortname);
  result = debug_target.to_get_tib_address (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_get_tib_address (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ptid_t (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR_p (arg2);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static void
delegate_set_permissions (struct target_ops *self)
{
  self = self->beneath;
  self->to_set_permissions (self);
}

static void
tdefault_set_permissions (struct target_ops *self)
{
}

static void
debug_set_permissions (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_set_permissions (...)\n", debug_target.to_shortname);
  debug_target.to_set_permissions (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_set_permissions (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static int
delegate_static_tracepoint_marker_at (struct target_ops *self, CORE_ADDR arg1, struct static_tracepoint_marker *arg2)
{
  self = self->beneath;
  return self->to_static_tracepoint_marker_at (self, arg1, arg2);
}

static int
tdefault_static_tracepoint_marker_at (struct target_ops *self, CORE_ADDR arg1, struct static_tracepoint_marker *arg2)
{
  return 0;
}

static int
debug_static_tracepoint_marker_at (struct target_ops *self, CORE_ADDR arg1, struct static_tracepoint_marker *arg2)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_static_tracepoint_marker_at (...)\n", debug_target.to_shortname);
  result = debug_target.to_static_tracepoint_marker_at (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_static_tracepoint_marker_at (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_static_tracepoint_marker_p (arg2);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static VEC(static_tracepoint_marker_p) *
delegate_static_tracepoint_markers_by_strid (struct target_ops *self, const char *arg1)
{
  self = self->beneath;
  return self->to_static_tracepoint_markers_by_strid (self, arg1);
}

static VEC(static_tracepoint_marker_p) *
tdefault_static_tracepoint_markers_by_strid (struct target_ops *self, const char *arg1)
{
  tcomplain ();
}

static VEC(static_tracepoint_marker_p) *
debug_static_tracepoint_markers_by_strid (struct target_ops *self, const char *arg1)
{
  VEC(static_tracepoint_marker_p) * result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_static_tracepoint_markers_by_strid (...)\n", debug_target.to_shortname);
  result = debug_target.to_static_tracepoint_markers_by_strid (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_static_tracepoint_markers_by_strid (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_const_char_p (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_VEC_static_tracepoint_marker_p__p (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static struct traceframe_info *
delegate_traceframe_info (struct target_ops *self)
{
  self = self->beneath;
  return self->to_traceframe_info (self);
}

static struct traceframe_info *
tdefault_traceframe_info (struct target_ops *self)
{
  tcomplain ();
}

static struct traceframe_info *
debug_traceframe_info (struct target_ops *self)
{
  struct traceframe_info * result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_traceframe_info (...)\n", debug_target.to_shortname);
  result = debug_target.to_traceframe_info (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_traceframe_info (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_struct_traceframe_info_p (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_use_agent (struct target_ops *self, int arg1)
{
  self = self->beneath;
  return self->to_use_agent (self, arg1);
}

static int
tdefault_use_agent (struct target_ops *self, int arg1)
{
  tcomplain ();
}

static int
debug_use_agent (struct target_ops *self, int arg1)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_use_agent (...)\n", debug_target.to_shortname);
  result = debug_target.to_use_agent (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_use_agent (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_can_use_agent (struct target_ops *self)
{
  self = self->beneath;
  return self->to_can_use_agent (self);
}

static int
tdefault_can_use_agent (struct target_ops *self)
{
  return 0;
}

static int
debug_can_use_agent (struct target_ops *self)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_can_use_agent (...)\n", debug_target.to_shortname);
  result = debug_target.to_can_use_agent (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_can_use_agent (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_supports_btrace (struct target_ops *self, enum btrace_format arg1)
{
  self = self->beneath;
  return self->to_supports_btrace (self, arg1);
}

static int
tdefault_supports_btrace (struct target_ops *self, enum btrace_format arg1)
{
  return 0;
}

static int
debug_supports_btrace (struct target_ops *self, enum btrace_format arg1)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_supports_btrace (...)\n", debug_target.to_shortname);
  result = debug_target.to_supports_btrace (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_supports_btrace (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_enum_btrace_format (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static struct btrace_target_info *
delegate_enable_btrace (struct target_ops *self, ptid_t arg1, const struct btrace_config *arg2)
{
  self = self->beneath;
  return self->to_enable_btrace (self, arg1, arg2);
}

static struct btrace_target_info *
tdefault_enable_btrace (struct target_ops *self, ptid_t arg1, const struct btrace_config *arg2)
{
  tcomplain ();
}

static struct btrace_target_info *
debug_enable_btrace (struct target_ops *self, ptid_t arg1, const struct btrace_config *arg2)
{
  struct btrace_target_info * result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_enable_btrace (...)\n", debug_target.to_shortname);
  result = debug_target.to_enable_btrace (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_enable_btrace (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ptid_t (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_const_struct_btrace_config_p (arg2);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_struct_btrace_target_info_p (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static void
delegate_disable_btrace (struct target_ops *self, struct btrace_target_info *arg1)
{
  self = self->beneath;
  self->to_disable_btrace (self, arg1);
}

static void
tdefault_disable_btrace (struct target_ops *self, struct btrace_target_info *arg1)
{
  tcomplain ();
}

static void
debug_disable_btrace (struct target_ops *self, struct btrace_target_info *arg1)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_disable_btrace (...)\n", debug_target.to_shortname);
  debug_target.to_disable_btrace (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_disable_btrace (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_btrace_target_info_p (arg1);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_teardown_btrace (struct target_ops *self, struct btrace_target_info *arg1)
{
  self = self->beneath;
  self->to_teardown_btrace (self, arg1);
}

static void
tdefault_teardown_btrace (struct target_ops *self, struct btrace_target_info *arg1)
{
  tcomplain ();
}

static void
debug_teardown_btrace (struct target_ops *self, struct btrace_target_info *arg1)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_teardown_btrace (...)\n", debug_target.to_shortname);
  debug_target.to_teardown_btrace (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_teardown_btrace (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_btrace_target_info_p (arg1);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static enum btrace_error
delegate_read_btrace (struct target_ops *self, struct btrace_data *arg1, struct btrace_target_info *arg2, enum btrace_read_type arg3)
{
  self = self->beneath;
  return self->to_read_btrace (self, arg1, arg2, arg3);
}

static enum btrace_error
tdefault_read_btrace (struct target_ops *self, struct btrace_data *arg1, struct btrace_target_info *arg2, enum btrace_read_type arg3)
{
  tcomplain ();
}

static enum btrace_error
debug_read_btrace (struct target_ops *self, struct btrace_data *arg1, struct btrace_target_info *arg2, enum btrace_read_type arg3)
{
  enum btrace_error result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_read_btrace (...)\n", debug_target.to_shortname);
  result = debug_target.to_read_btrace (&debug_target, arg1, arg2, arg3);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_read_btrace (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_btrace_data_p (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_struct_btrace_target_info_p (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_enum_btrace_read_type (arg3);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_enum_btrace_error (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static const struct btrace_config *
delegate_btrace_conf (struct target_ops *self, const struct btrace_target_info *arg1)
{
  self = self->beneath;
  return self->to_btrace_conf (self, arg1);
}

static const struct btrace_config *
tdefault_btrace_conf (struct target_ops *self, const struct btrace_target_info *arg1)
{
  return NULL;
}

static const struct btrace_config *
debug_btrace_conf (struct target_ops *self, const struct btrace_target_info *arg1)
{
  const struct btrace_config * result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_btrace_conf (...)\n", debug_target.to_shortname);
  result = debug_target.to_btrace_conf (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_btrace_conf (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_const_struct_btrace_target_info_p (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_const_struct_btrace_config_p (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static enum record_method
delegate_record_method (struct target_ops *self, ptid_t arg1)
{
  self = self->beneath;
  return self->to_record_method (self, arg1);
}

static enum record_method
tdefault_record_method (struct target_ops *self, ptid_t arg1)
{
  return RECORD_METHOD_NONE;
}

static enum record_method
debug_record_method (struct target_ops *self, ptid_t arg1)
{
  enum record_method result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_record_method (...)\n", debug_target.to_shortname);
  result = debug_target.to_record_method (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_record_method (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ptid_t (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_enum_record_method (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static void
delegate_stop_recording (struct target_ops *self)
{
  self = self->beneath;
  self->to_stop_recording (self);
}

static void
tdefault_stop_recording (struct target_ops *self)
{
}

static void
debug_stop_recording (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_stop_recording (...)\n", debug_target.to_shortname);
  debug_target.to_stop_recording (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_stop_recording (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_info_record (struct target_ops *self)
{
  self = self->beneath;
  self->to_info_record (self);
}

static void
tdefault_info_record (struct target_ops *self)
{
}

static void
debug_info_record (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_info_record (...)\n", debug_target.to_shortname);
  debug_target.to_info_record (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_info_record (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_save_record (struct target_ops *self, const char *arg1)
{
  self = self->beneath;
  self->to_save_record (self, arg1);
}

static void
tdefault_save_record (struct target_ops *self, const char *arg1)
{
  tcomplain ();
}

static void
debug_save_record (struct target_ops *self, const char *arg1)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_save_record (...)\n", debug_target.to_shortname);
  debug_target.to_save_record (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_save_record (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_const_char_p (arg1);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_delete_record (struct target_ops *self)
{
  self = self->beneath;
  self->to_delete_record (self);
}

static void
tdefault_delete_record (struct target_ops *self)
{
  tcomplain ();
}

static void
debug_delete_record (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_delete_record (...)\n", debug_target.to_shortname);
  debug_target.to_delete_record (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_delete_record (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static int
delegate_record_is_replaying (struct target_ops *self, ptid_t arg1)
{
  self = self->beneath;
  return self->to_record_is_replaying (self, arg1);
}

static int
tdefault_record_is_replaying (struct target_ops *self, ptid_t arg1)
{
  return 0;
}

static int
debug_record_is_replaying (struct target_ops *self, ptid_t arg1)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_record_is_replaying (...)\n", debug_target.to_shortname);
  result = debug_target.to_record_is_replaying (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_record_is_replaying (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ptid_t (arg1);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static int
delegate_record_will_replay (struct target_ops *self, ptid_t arg1, int arg2)
{
  self = self->beneath;
  return self->to_record_will_replay (self, arg1, arg2);
}

static int
tdefault_record_will_replay (struct target_ops *self, ptid_t arg1, int arg2)
{
  return 0;
}

static int
debug_record_will_replay (struct target_ops *self, ptid_t arg1, int arg2)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_record_will_replay (...)\n", debug_target.to_shortname);
  result = debug_target.to_record_will_replay (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_record_will_replay (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ptid_t (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static void
delegate_record_stop_replaying (struct target_ops *self)
{
  self = self->beneath;
  self->to_record_stop_replaying (self);
}

static void
tdefault_record_stop_replaying (struct target_ops *self)
{
}

static void
debug_record_stop_replaying (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_record_stop_replaying (...)\n", debug_target.to_shortname);
  debug_target.to_record_stop_replaying (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_record_stop_replaying (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_goto_record_begin (struct target_ops *self)
{
  self = self->beneath;
  self->to_goto_record_begin (self);
}

static void
tdefault_goto_record_begin (struct target_ops *self)
{
  tcomplain ();
}

static void
debug_goto_record_begin (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_goto_record_begin (...)\n", debug_target.to_shortname);
  debug_target.to_goto_record_begin (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_goto_record_begin (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_goto_record_end (struct target_ops *self)
{
  self = self->beneath;
  self->to_goto_record_end (self);
}

static void
tdefault_goto_record_end (struct target_ops *self)
{
  tcomplain ();
}

static void
debug_goto_record_end (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_goto_record_end (...)\n", debug_target.to_shortname);
  debug_target.to_goto_record_end (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_goto_record_end (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_goto_record (struct target_ops *self, ULONGEST arg1)
{
  self = self->beneath;
  self->to_goto_record (self, arg1);
}

static void
tdefault_goto_record (struct target_ops *self, ULONGEST arg1)
{
  tcomplain ();
}

static void
debug_goto_record (struct target_ops *self, ULONGEST arg1)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_goto_record (...)\n", debug_target.to_shortname);
  debug_target.to_goto_record (&debug_target, arg1);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_goto_record (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ULONGEST (arg1);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_insn_history (struct target_ops *self, int arg1, int arg2)
{
  self = self->beneath;
  self->to_insn_history (self, arg1, arg2);
}

static void
tdefault_insn_history (struct target_ops *self, int arg1, int arg2)
{
  tcomplain ();
}

static void
debug_insn_history (struct target_ops *self, int arg1, int arg2)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_insn_history (...)\n", debug_target.to_shortname);
  debug_target.to_insn_history (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_insn_history (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_insn_history_from (struct target_ops *self, ULONGEST arg1, int arg2, int arg3)
{
  self = self->beneath;
  self->to_insn_history_from (self, arg1, arg2, arg3);
}

static void
tdefault_insn_history_from (struct target_ops *self, ULONGEST arg1, int arg2, int arg3)
{
  tcomplain ();
}

static void
debug_insn_history_from (struct target_ops *self, ULONGEST arg1, int arg2, int arg3)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_insn_history_from (...)\n", debug_target.to_shortname);
  debug_target.to_insn_history_from (&debug_target, arg1, arg2, arg3);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_insn_history_from (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ULONGEST (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg3);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_insn_history_range (struct target_ops *self, ULONGEST arg1, ULONGEST arg2, int arg3)
{
  self = self->beneath;
  self->to_insn_history_range (self, arg1, arg2, arg3);
}

static void
tdefault_insn_history_range (struct target_ops *self, ULONGEST arg1, ULONGEST arg2, int arg3)
{
  tcomplain ();
}

static void
debug_insn_history_range (struct target_ops *self, ULONGEST arg1, ULONGEST arg2, int arg3)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_insn_history_range (...)\n", debug_target.to_shortname);
  debug_target.to_insn_history_range (&debug_target, arg1, arg2, arg3);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_insn_history_range (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ULONGEST (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ULONGEST (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg3);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_call_history (struct target_ops *self, int arg1, int arg2)
{
  self = self->beneath;
  self->to_call_history (self, arg1, arg2);
}

static void
tdefault_call_history (struct target_ops *self, int arg1, int arg2)
{
  tcomplain ();
}

static void
debug_call_history (struct target_ops *self, int arg1, int arg2)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_call_history (...)\n", debug_target.to_shortname);
  debug_target.to_call_history (&debug_target, arg1, arg2);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_call_history (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_call_history_from (struct target_ops *self, ULONGEST arg1, int arg2, int arg3)
{
  self = self->beneath;
  self->to_call_history_from (self, arg1, arg2, arg3);
}

static void
tdefault_call_history_from (struct target_ops *self, ULONGEST arg1, int arg2, int arg3)
{
  tcomplain ();
}

static void
debug_call_history_from (struct target_ops *self, ULONGEST arg1, int arg2, int arg3)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_call_history_from (...)\n", debug_target.to_shortname);
  debug_target.to_call_history_from (&debug_target, arg1, arg2, arg3);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_call_history_from (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ULONGEST (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg3);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_call_history_range (struct target_ops *self, ULONGEST arg1, ULONGEST arg2, int arg3)
{
  self = self->beneath;
  self->to_call_history_range (self, arg1, arg2, arg3);
}

static void
tdefault_call_history_range (struct target_ops *self, ULONGEST arg1, ULONGEST arg2, int arg3)
{
  tcomplain ();
}

static void
debug_call_history_range (struct target_ops *self, ULONGEST arg1, ULONGEST arg2, int arg3)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_call_history_range (...)\n", debug_target.to_shortname);
  debug_target.to_call_history_range (&debug_target, arg1, arg2, arg3);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_call_history_range (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ULONGEST (arg1);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_ULONGEST (arg2);
  fputs_unfiltered (", ", gdb_stdlog);
  target_debug_print_int (arg3);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static int
delegate_augmented_libraries_svr4_read (struct target_ops *self)
{
  self = self->beneath;
  return self->to_augmented_libraries_svr4_read (self);
}

static int
tdefault_augmented_libraries_svr4_read (struct target_ops *self)
{
  return 0;
}

static int
debug_augmented_libraries_svr4_read (struct target_ops *self)
{
  int result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_augmented_libraries_svr4_read (...)\n", debug_target.to_shortname);
  result = debug_target.to_augmented_libraries_svr4_read (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_augmented_libraries_svr4_read (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_int (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static const struct frame_unwind *
delegate_get_unwinder (struct target_ops *self)
{
  self = self->beneath;
  return self->to_get_unwinder (self);
}

static const struct frame_unwind *
tdefault_get_unwinder (struct target_ops *self)
{
  return NULL;
}

static const struct frame_unwind *
debug_get_unwinder (struct target_ops *self)
{
  const struct frame_unwind * result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_get_unwinder (...)\n", debug_target.to_shortname);
  result = debug_target.to_get_unwinder (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_get_unwinder (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_const_struct_frame_unwind_p (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static const struct frame_unwind *
delegate_get_tailcall_unwinder (struct target_ops *self)
{
  self = self->beneath;
  return self->to_get_tailcall_unwinder (self);
}

static const struct frame_unwind *
tdefault_get_tailcall_unwinder (struct target_ops *self)
{
  return NULL;
}

static const struct frame_unwind *
debug_get_tailcall_unwinder (struct target_ops *self)
{
  const struct frame_unwind * result;
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_get_tailcall_unwinder (...)\n", debug_target.to_shortname);
  result = debug_target.to_get_tailcall_unwinder (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_get_tailcall_unwinder (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (") = ", gdb_stdlog);
  target_debug_print_const_struct_frame_unwind_p (result);
  fputs_unfiltered ("\n", gdb_stdlog);
  return result;
}

static void
delegate_prepare_to_generate_core (struct target_ops *self)
{
  self = self->beneath;
  self->to_prepare_to_generate_core (self);
}

static void
tdefault_prepare_to_generate_core (struct target_ops *self)
{
}

static void
debug_prepare_to_generate_core (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_prepare_to_generate_core (...)\n", debug_target.to_shortname);
  debug_target.to_prepare_to_generate_core (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_prepare_to_generate_core (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
delegate_done_generating_core (struct target_ops *self)
{
  self = self->beneath;
  self->to_done_generating_core (self);
}

static void
tdefault_done_generating_core (struct target_ops *self)
{
}

static void
debug_done_generating_core (struct target_ops *self)
{
  fprintf_unfiltered (gdb_stdlog, "-> %s->to_done_generating_core (...)\n", debug_target.to_shortname);
  debug_target.to_done_generating_core (&debug_target);
  fprintf_unfiltered (gdb_stdlog, "<- %s->to_done_generating_core (", debug_target.to_shortname);
  target_debug_print_struct_target_ops_p (&debug_target);
  fputs_unfiltered (")\n", gdb_stdlog);
}

static void
install_delegators (struct target_ops *ops)
{
  if (ops->to_post_attach == NULL)
    ops->to_post_attach = delegate_post_attach;
  if (ops->to_detach == NULL)
    ops->to_detach = delegate_detach;
  if (ops->to_disconnect == NULL)
    ops->to_disconnect = delegate_disconnect;
  if (ops->to_resume == NULL)
    ops->to_resume = delegate_resume;
  if (ops->to_commit_resume == NULL)
    ops->to_commit_resume = delegate_commit_resume;
  if (ops->to_wait == NULL)
    ops->to_wait = delegate_wait;
  if (ops->to_fetch_registers == NULL)
    ops->to_fetch_registers = delegate_fetch_registers;
  if (ops->to_store_registers == NULL)
    ops->to_store_registers = delegate_store_registers;
  if (ops->to_prepare_to_store == NULL)
    ops->to_prepare_to_store = delegate_prepare_to_store;
  if (ops->to_files_info == NULL)
    ops->to_files_info = delegate_files_info;
  if (ops->to_insert_breakpoint == NULL)
    ops->to_insert_breakpoint = delegate_insert_breakpoint;
  if (ops->to_remove_breakpoint == NULL)
    ops->to_remove_breakpoint = delegate_remove_breakpoint;
  if (ops->to_stopped_by_sw_breakpoint == NULL)
    ops->to_stopped_by_sw_breakpoint = delegate_stopped_by_sw_breakpoint;
  if (ops->to_supports_stopped_by_sw_breakpoint == NULL)
    ops->to_supports_stopped_by_sw_breakpoint = delegate_supports_stopped_by_sw_breakpoint;
  if (ops->to_stopped_by_hw_breakpoint == NULL)
    ops->to_stopped_by_hw_breakpoint = delegate_stopped_by_hw_breakpoint;
  if (ops->to_supports_stopped_by_hw_breakpoint == NULL)
    ops->to_supports_stopped_by_hw_breakpoint = delegate_supports_stopped_by_hw_breakpoint;
  if (ops->to_can_use_hw_breakpoint == NULL)
    ops->to_can_use_hw_breakpoint = delegate_can_use_hw_breakpoint;
  if (ops->to_ranged_break_num_registers == NULL)
    ops->to_ranged_break_num_registers = delegate_ranged_break_num_registers;
  if (ops->to_insert_hw_breakpoint == NULL)
    ops->to_insert_hw_breakpoint = delegate_insert_hw_breakpoint;
  if (ops->to_remove_hw_breakpoint == NULL)
    ops->to_remove_hw_breakpoint = delegate_remove_hw_breakpoint;
  if (ops->to_remove_watchpoint == NULL)
    ops->to_remove_watchpoint = delegate_remove_watchpoint;
  if (ops->to_insert_watchpoint == NULL)
    ops->to_insert_watchpoint = delegate_insert_watchpoint;
  if (ops->to_insert_mask_watchpoint == NULL)
    ops->to_insert_mask_watchpoint = delegate_insert_mask_watchpoint;
  if (ops->to_remove_mask_watchpoint == NULL)
    ops->to_remove_mask_watchpoint = delegate_remove_mask_watchpoint;
  if (ops->to_stopped_by_watchpoint == NULL)
    ops->to_stopped_by_watchpoint = delegate_stopped_by_watchpoint;
  if (ops->to_stopped_data_address == NULL)
    ops->to_stopped_data_address = delegate_stopped_data_address;
  if (ops->to_watchpoint_addr_within_range == NULL)
    ops->to_watchpoint_addr_within_range = delegate_watchpoint_addr_within_range;
  if (ops->to_region_ok_for_hw_watchpoint == NULL)
    ops->to_region_ok_for_hw_watchpoint = delegate_region_ok_for_hw_watchpoint;
  if (ops->to_can_accel_watchpoint_condition == NULL)
    ops->to_can_accel_watchpoint_condition = delegate_can_accel_watchpoint_condition;
  if (ops->to_masked_watch_num_registers == NULL)
    ops->to_masked_watch_num_registers = delegate_masked_watch_num_registers;
  if (ops->to_can_do_single_step == NULL)
    ops->to_can_do_single_step = delegate_can_do_single_step;
  if (ops->to_terminal_init == NULL)
    ops->to_terminal_init = delegate_terminal_init;
  if (ops->to_terminal_inferior == NULL)
    ops->to_terminal_inferior = delegate_terminal_inferior;
  if (ops->to_terminal_ours_for_output == NULL)
    ops->to_terminal_ours_for_output = delegate_terminal_ours_for_output;
  if (ops->to_terminal_ours == NULL)
    ops->to_terminal_ours = delegate_terminal_ours;
  if (ops->to_terminal_info == NULL)
    ops->to_terminal_info = delegate_terminal_info;
  if (ops->to_kill == NULL)
    ops->to_kill = delegate_kill;
  if (ops->to_load == NULL)
    ops->to_load = delegate_load;
  if (ops->to_post_startup_inferior == NULL)
    ops->to_post_startup_inferior = delegate_post_startup_inferior;
  if (ops->to_insert_fork_catchpoint == NULL)
    ops->to_insert_fork_catchpoint = delegate_insert_fork_catchpoint;
  if (ops->to_remove_fork_catchpoint == NULL)
    ops->to_remove_fork_catchpoint = delegate_remove_fork_catchpoint;
  if (ops->to_insert_vfork_catchpoint == NULL)
    ops->to_insert_vfork_catchpoint = delegate_insert_vfork_catchpoint;
  if (ops->to_remove_vfork_catchpoint == NULL)
    ops->to_remove_vfork_catchpoint = delegate_remove_vfork_catchpoint;
  if (ops->to_follow_fork == NULL)
    ops->to_follow_fork = delegate_follow_fork;
  if (ops->to_insert_exec_catchpoint == NULL)
    ops->to_insert_exec_catchpoint = delegate_insert_exec_catchpoint;
  if (ops->to_remove_exec_catchpoint == NULL)
    ops->to_remove_exec_catchpoint = delegate_remove_exec_catchpoint;
  if (ops->to_follow_exec == NULL)
    ops->to_follow_exec = delegate_follow_exec;
  if (ops->to_set_syscall_catchpoint == NULL)
    ops->to_set_syscall_catchpoint = delegate_set_syscall_catchpoint;
  if (ops->to_has_exited == NULL)
    ops->to_has_exited = delegate_has_exited;
  if (ops->to_mourn_inferior == NULL)
    ops->to_mourn_inferior = delegate_mourn_inferior;
  if (ops->to_can_run == NULL)
    ops->to_can_run = delegate_can_run;
  if (ops->to_pass_signals == NULL)
    ops->to_pass_signals = delegate_pass_signals;
  if (ops->to_program_signals == NULL)
    ops->to_program_signals = delegate_program_signals;
  if (ops->to_thread_alive == NULL)
    ops->to_thread_alive = delegate_thread_alive;
  if (ops->to_update_thread_list == NULL)
    ops->to_update_thread_list = delegate_update_thread_list;
  if (ops->to_pid_to_str == NULL)
    ops->to_pid_to_str = delegate_pid_to_str;
  if (ops->to_extra_thread_info == NULL)
    ops->to_extra_thread_info = delegate_extra_thread_info;
  if (ops->to_thread_name == NULL)
    ops->to_thread_name = delegate_thread_name;
  if (ops->to_stop == NULL)
    ops->to_stop = delegate_stop;
  if (ops->to_interrupt == NULL)
    ops->to_interrupt = delegate_interrupt;
  if (ops->to_pass_ctrlc == NULL)
    ops->to_pass_ctrlc = delegate_pass_ctrlc;
  if (ops->to_rcmd == NULL)
    ops->to_rcmd = delegate_rcmd;
  if (ops->to_pid_to_exec_file == NULL)
    ops->to_pid_to_exec_file = delegate_pid_to_exec_file;
  if (ops->to_log_command == NULL)
    ops->to_log_command = delegate_log_command;
  if (ops->to_get_section_table == NULL)
    ops->to_get_section_table = delegate_get_section_table;
  if (ops->to_can_async_p == NULL)
    ops->to_can_async_p = delegate_can_async_p;
  if (ops->to_is_async_p == NULL)
    ops->to_is_async_p = delegate_is_async_p;
  if (ops->to_async == NULL)
    ops->to_async = delegate_async;
  if (ops->to_thread_events == NULL)
    ops->to_thread_events = delegate_thread_events;
  if (ops->to_supports_non_stop == NULL)
    ops->to_supports_non_stop = delegate_supports_non_stop;
  if (ops->to_always_non_stop_p == NULL)
    ops->to_always_non_stop_p = delegate_always_non_stop_p;
  if (ops->to_find_memory_regions == NULL)
    ops->to_find_memory_regions = delegate_find_memory_regions;
  if (ops->to_make_corefile_notes == NULL)
    ops->to_make_corefile_notes = delegate_make_corefile_notes;
  if (ops->to_get_bookmark == NULL)
    ops->to_get_bookmark = delegate_get_bookmark;
  if (ops->to_goto_bookmark == NULL)
    ops->to_goto_bookmark = delegate_goto_bookmark;
  if (ops->to_get_thread_local_address == NULL)
    ops->to_get_thread_local_address = delegate_get_thread_local_address;
  if (ops->to_xfer_partial == NULL)
    ops->to_xfer_partial = delegate_xfer_partial;
  if (ops->to_get_memory_xfer_limit == NULL)
    ops->to_get_memory_xfer_limit = delegate_get_memory_xfer_limit;
  if (ops->to_memory_map == NULL)
    ops->to_memory_map = delegate_memory_map;
  if (ops->to_flash_erase == NULL)
    ops->to_flash_erase = delegate_flash_erase;
  if (ops->to_flash_done == NULL)
    ops->to_flash_done = delegate_flash_done;
  if (ops->to_read_description == NULL)
    ops->to_read_description = delegate_read_description;
  if (ops->to_get_ada_task_ptid == NULL)
    ops->to_get_ada_task_ptid = delegate_get_ada_task_ptid;
  if (ops->to_auxv_parse == NULL)
    ops->to_auxv_parse = delegate_auxv_parse;
  if (ops->to_search_memory == NULL)
    ops->to_search_memory = delegate_search_memory;
  if (ops->to_can_execute_reverse == NULL)
    ops->to_can_execute_reverse = delegate_can_execute_reverse;
  if (ops->to_execution_direction == NULL)
    ops->to_execution_direction = delegate_execution_direction;
  if (ops->to_supports_multi_process == NULL)
    ops->to_supports_multi_process = delegate_supports_multi_process;
  if (ops->to_supports_enable_disable_tracepoint == NULL)
    ops->to_supports_enable_disable_tracepoint = delegate_supports_enable_disable_tracepoint;
  if (ops->to_supports_string_tracing == NULL)
    ops->to_supports_string_tracing = delegate_supports_string_tracing;
  if (ops->to_supports_evaluation_of_breakpoint_conditions == NULL)
    ops->to_supports_evaluation_of_breakpoint_conditions = delegate_supports_evaluation_of_breakpoint_conditions;
  if (ops->to_can_run_breakpoint_commands == NULL)
    ops->to_can_run_breakpoint_commands = delegate_can_run_breakpoint_commands;
  if (ops->to_thread_architecture == NULL)
    ops->to_thread_architecture = delegate_thread_architecture;
  if (ops->to_thread_address_space == NULL)
    ops->to_thread_address_space = delegate_thread_address_space;
  if (ops->to_filesystem_is_local == NULL)
    ops->to_filesystem_is_local = delegate_filesystem_is_local;
  if (ops->to_trace_init == NULL)
    ops->to_trace_init = delegate_trace_init;
  if (ops->to_download_tracepoint == NULL)
    ops->to_download_tracepoint = delegate_download_tracepoint;
  if (ops->to_can_download_tracepoint == NULL)
    ops->to_can_download_tracepoint = delegate_can_download_tracepoint;
  if (ops->to_download_trace_state_variable == NULL)
    ops->to_download_trace_state_variable = delegate_download_trace_state_variable;
  if (ops->to_enable_tracepoint == NULL)
    ops->to_enable_tracepoint = delegate_enable_tracepoint;
  if (ops->to_disable_tracepoint == NULL)
    ops->to_disable_tracepoint = delegate_disable_tracepoint;
  if (ops->to_trace_set_readonly_regions == NULL)
    ops->to_trace_set_readonly_regions = delegate_trace_set_readonly_regions;
  if (ops->to_trace_start == NULL)
    ops->to_trace_start = delegate_trace_start;
  if (ops->to_get_trace_status == NULL)
    ops->to_get_trace_status = delegate_get_trace_status;
  if (ops->to_get_tracepoint_status == NULL)
    ops->to_get_tracepoint_status = delegate_get_tracepoint_status;
  if (ops->to_trace_stop == NULL)
    ops->to_trace_stop = delegate_trace_stop;
  if (ops->to_trace_find == NULL)
    ops->to_trace_find = delegate_trace_find;
  if (ops->to_get_trace_state_variable_value == NULL)
    ops->to_get_trace_state_variable_value = delegate_get_trace_state_variable_value;
  if (ops->to_save_trace_data == NULL)
    ops->to_save_trace_data = delegate_save_trace_data;
  if (ops->to_upload_tracepoints == NULL)
    ops->to_upload_tracepoints = delegate_upload_tracepoints;
  if (ops->to_upload_trace_state_variables == NULL)
    ops->to_upload_trace_state_variables = delegate_upload_trace_state_variables;
  if (ops->to_get_raw_trace_data == NULL)
    ops->to_get_raw_trace_data = delegate_get_raw_trace_data;
  if (ops->to_get_min_fast_tracepoint_insn_len == NULL)
    ops->to_get_min_fast_tracepoint_insn_len = delegate_get_min_fast_tracepoint_insn_len;
  if (ops->to_set_disconnected_tracing == NULL)
    ops->to_set_disconnected_tracing = delegate_set_disconnected_tracing;
  if (ops->to_set_circular_trace_buffer == NULL)
    ops->to_set_circular_trace_buffer = delegate_set_circular_trace_buffer;
  if (ops->to_set_trace_buffer_size == NULL)
    ops->to_set_trace_buffer_size = delegate_set_trace_buffer_size;
  if (ops->to_set_trace_notes == NULL)
    ops->to_set_trace_notes = delegate_set_trace_notes;
  if (ops->to_core_of_thread == NULL)
    ops->to_core_of_thread = delegate_core_of_thread;
  if (ops->to_verify_memory == NULL)
    ops->to_verify_memory = delegate_verify_memory;
  if (ops->to_get_tib_address == NULL)
    ops->to_get_tib_address = delegate_get_tib_address;
  if (ops->to_set_permissions == NULL)
    ops->to_set_permissions = delegate_set_permissions;
  if (ops->to_static_tracepoint_marker_at == NULL)
    ops->to_static_tracepoint_marker_at = delegate_static_tracepoint_marker_at;
  if (ops->to_static_tracepoint_markers_by_strid == NULL)
    ops->to_static_tracepoint_markers_by_strid = delegate_static_tracepoint_markers_by_strid;
  if (ops->to_traceframe_info == NULL)
    ops->to_traceframe_info = delegate_traceframe_info;
  if (ops->to_use_agent == NULL)
    ops->to_use_agent = delegate_use_agent;
  if (ops->to_can_use_agent == NULL)
    ops->to_can_use_agent = delegate_can_use_agent;
  if (ops->to_supports_btrace == NULL)
    ops->to_supports_btrace = delegate_supports_btrace;
  if (ops->to_enable_btrace == NULL)
    ops->to_enable_btrace = delegate_enable_btrace;
  if (ops->to_disable_btrace == NULL)
    ops->to_disable_btrace = delegate_disable_btrace;
  if (ops->to_teardown_btrace == NULL)
    ops->to_teardown_btrace = delegate_teardown_btrace;
  if (ops->to_read_btrace == NULL)
    ops->to_read_btrace = delegate_read_btrace;
  if (ops->to_btrace_conf == NULL)
    ops->to_btrace_conf = delegate_btrace_conf;
  if (ops->to_record_method == NULL)
    ops->to_record_method = delegate_record_method;
  if (ops->to_stop_recording == NULL)
    ops->to_stop_recording = delegate_stop_recording;
  if (ops->to_info_record == NULL)
    ops->to_info_record = delegate_info_record;
  if (ops->to_save_record == NULL)
    ops->to_save_record = delegate_save_record;
  if (ops->to_delete_record == NULL)
    ops->to_delete_record = delegate_delete_record;
  if (ops->to_record_is_replaying == NULL)
    ops->to_record_is_replaying = delegate_record_is_replaying;
  if (ops->to_record_will_replay == NULL)
    ops->to_record_will_replay = delegate_record_will_replay;
  if (ops->to_record_stop_replaying == NULL)
    ops->to_record_stop_replaying = delegate_record_stop_replaying;
  if (ops->to_goto_record_begin == NULL)
    ops->to_goto_record_begin = delegate_goto_record_begin;
  if (ops->to_goto_record_end == NULL)
    ops->to_goto_record_end = delegate_goto_record_end;
  if (ops->to_goto_record == NULL)
    ops->to_goto_record = delegate_goto_record;
  if (ops->to_insn_history == NULL)
    ops->to_insn_history = delegate_insn_history;
  if (ops->to_insn_history_from == NULL)
    ops->to_insn_history_from = delegate_insn_history_from;
  if (ops->to_insn_history_range == NULL)
    ops->to_insn_history_range = delegate_insn_history_range;
  if (ops->to_call_history == NULL)
    ops->to_call_history = delegate_call_history;
  if (ops->to_call_history_from == NULL)
    ops->to_call_history_from = delegate_call_history_from;
  if (ops->to_call_history_range == NULL)
    ops->to_call_history_range = delegate_call_history_range;
  if (ops->to_augmented_libraries_svr4_read == NULL)
    ops->to_augmented_libraries_svr4_read = delegate_augmented_libraries_svr4_read;
  if (ops->to_get_unwinder == NULL)
    ops->to_get_unwinder = delegate_get_unwinder;
  if (ops->to_get_tailcall_unwinder == NULL)
    ops->to_get_tailcall_unwinder = delegate_get_tailcall_unwinder;
  if (ops->to_prepare_to_generate_core == NULL)
    ops->to_prepare_to_generate_core = delegate_prepare_to_generate_core;
  if (ops->to_done_generating_core == NULL)
    ops->to_done_generating_core = delegate_done_generating_core;
}

static void
install_dummy_methods (struct target_ops *ops)
{
  ops->to_post_attach = tdefault_post_attach;
  ops->to_detach = tdefault_detach;
  ops->to_disconnect = tdefault_disconnect;
  ops->to_resume = tdefault_resume;
  ops->to_commit_resume = tdefault_commit_resume;
  ops->to_wait = default_target_wait;
  ops->to_fetch_registers = tdefault_fetch_registers;
  ops->to_store_registers = tdefault_store_registers;
  ops->to_prepare_to_store = tdefault_prepare_to_store;
  ops->to_files_info = tdefault_files_info;
  ops->to_insert_breakpoint = memory_insert_breakpoint;
  ops->to_remove_breakpoint = memory_remove_breakpoint;
  ops->to_stopped_by_sw_breakpoint = tdefault_stopped_by_sw_breakpoint;
  ops->to_supports_stopped_by_sw_breakpoint = tdefault_supports_stopped_by_sw_breakpoint;
  ops->to_stopped_by_hw_breakpoint = tdefault_stopped_by_hw_breakpoint;
  ops->to_supports_stopped_by_hw_breakpoint = tdefault_supports_stopped_by_hw_breakpoint;
  ops->to_can_use_hw_breakpoint = tdefault_can_use_hw_breakpoint;
  ops->to_ranged_break_num_registers = tdefault_ranged_break_num_registers;
  ops->to_insert_hw_breakpoint = tdefault_insert_hw_breakpoint;
  ops->to_remove_hw_breakpoint = tdefault_remove_hw_breakpoint;
  ops->to_remove_watchpoint = tdefault_remove_watchpoint;
  ops->to_insert_watchpoint = tdefault_insert_watchpoint;
  ops->to_insert_mask_watchpoint = tdefault_insert_mask_watchpoint;
  ops->to_remove_mask_watchpoint = tdefault_remove_mask_watchpoint;
  ops->to_stopped_by_watchpoint = tdefault_stopped_by_watchpoint;
  ops->to_stopped_data_address = tdefault_stopped_data_address;
  ops->to_watchpoint_addr_within_range = default_watchpoint_addr_within_range;
  ops->to_region_ok_for_hw_watchpoint = default_region_ok_for_hw_watchpoint;
  ops->to_can_accel_watchpoint_condition = tdefault_can_accel_watchpoint_condition;
  ops->to_masked_watch_num_registers = tdefault_masked_watch_num_registers;
  ops->to_can_do_single_step = tdefault_can_do_single_step;
  ops->to_terminal_init = tdefault_terminal_init;
  ops->to_terminal_inferior = tdefault_terminal_inferior;
  ops->to_terminal_ours_for_output = tdefault_terminal_ours_for_output;
  ops->to_terminal_ours = tdefault_terminal_ours;
  ops->to_terminal_info = default_terminal_info;
  ops->to_kill = tdefault_kill;
  ops->to_load = tdefault_load;
  ops->to_post_startup_inferior = tdefault_post_startup_inferior;
  ops->to_insert_fork_catchpoint = tdefault_insert_fork_catchpoint;
  ops->to_remove_fork_catchpoint = tdefault_remove_fork_catchpoint;
  ops->to_insert_vfork_catchpoint = tdefault_insert_vfork_catchpoint;
  ops->to_remove_vfork_catchpoint = tdefault_remove_vfork_catchpoint;
  ops->to_follow_fork = default_follow_fork;
  ops->to_insert_exec_catchpoint = tdefault_insert_exec_catchpoint;
  ops->to_remove_exec_catchpoint = tdefault_remove_exec_catchpoint;
  ops->to_follow_exec = tdefault_follow_exec;
  ops->to_set_syscall_catchpoint = tdefault_set_syscall_catchpoint;
  ops->to_has_exited = tdefault_has_exited;
  ops->to_mourn_inferior = default_mourn_inferior;
  ops->to_can_run = tdefault_can_run;
  ops->to_pass_signals = tdefault_pass_signals;
  ops->to_program_signals = tdefault_program_signals;
  ops->to_thread_alive = tdefault_thread_alive;
  ops->to_update_thread_list = tdefault_update_thread_list;
  ops->to_pid_to_str = default_pid_to_str;
  ops->to_extra_thread_info = tdefault_extra_thread_info;
  ops->to_thread_name = tdefault_thread_name;
  ops->to_stop = tdefault_stop;
  ops->to_interrupt = tdefault_interrupt;
  ops->to_pass_ctrlc = default_target_pass_ctrlc;
  ops->to_rcmd = default_rcmd;
  ops->to_pid_to_exec_file = tdefault_pid_to_exec_file;
  ops->to_log_command = tdefault_log_command;
  ops->to_get_section_table = tdefault_get_section_table;
  ops->to_can_async_p = tdefault_can_async_p;
  ops->to_is_async_p = tdefault_is_async_p;
  ops->to_async = tdefault_async;
  ops->to_thread_events = tdefault_thread_events;
  ops->to_supports_non_stop = tdefault_supports_non_stop;
  ops->to_always_non_stop_p = tdefault_always_non_stop_p;
  ops->to_find_memory_regions = dummy_find_memory_regions;
  ops->to_make_corefile_notes = dummy_make_corefile_notes;
  ops->to_get_bookmark = tdefault_get_bookmark;
  ops->to_goto_bookmark = tdefault_goto_bookmark;
  ops->to_get_thread_local_address = tdefault_get_thread_local_address;
  ops->to_xfer_partial = tdefault_xfer_partial;
  ops->to_get_memory_xfer_limit = tdefault_get_memory_xfer_limit;
  ops->to_memory_map = tdefault_memory_map;
  ops->to_flash_erase = tdefault_flash_erase;
  ops->to_flash_done = tdefault_flash_done;
  ops->to_read_description = tdefault_read_description;
  ops->to_get_ada_task_ptid = default_get_ada_task_ptid;
  ops->to_auxv_parse = default_auxv_parse;
  ops->to_search_memory = default_search_memory;
  ops->to_can_execute_reverse = tdefault_can_execute_reverse;
  ops->to_execution_direction = default_execution_direction;
  ops->to_supports_multi_process = tdefault_supports_multi_process;
  ops->to_supports_enable_disable_tracepoint = tdefault_supports_enable_disable_tracepoint;
  ops->to_supports_string_tracing = tdefault_supports_string_tracing;
  ops->to_supports_evaluation_of_breakpoint_conditions = tdefault_supports_evaluation_of_breakpoint_conditions;
  ops->to_can_run_breakpoint_commands = tdefault_can_run_breakpoint_commands;
  ops->to_thread_architecture = default_thread_architecture;
  ops->to_thread_address_space = default_thread_address_space;
  ops->to_filesystem_is_local = tdefault_filesystem_is_local;
  ops->to_trace_init = tdefault_trace_init;
  ops->to_download_tracepoint = tdefault_download_tracepoint;
  ops->to_can_download_tracepoint = tdefault_can_download_tracepoint;
  ops->to_download_trace_state_variable = tdefault_download_trace_state_variable;
  ops->to_enable_tracepoint = tdefault_enable_tracepoint;
  ops->to_disable_tracepoint = tdefault_disable_tracepoint;
  ops->to_trace_set_readonly_regions = tdefault_trace_set_readonly_regions;
  ops->to_trace_start = tdefault_trace_start;
  ops->to_get_trace_status = tdefault_get_trace_status;
  ops->to_get_tracepoint_status = tdefault_get_tracepoint_status;
  ops->to_trace_stop = tdefault_trace_stop;
  ops->to_trace_find = tdefault_trace_find;
  ops->to_get_trace_state_variable_value = tdefault_get_trace_state_variable_value;
  ops->to_save_trace_data = tdefault_save_trace_data;
  ops->to_upload_tracepoints = tdefault_upload_tracepoints;
  ops->to_upload_trace_state_variables = tdefault_upload_trace_state_variables;
  ops->to_get_raw_trace_data = tdefault_get_raw_trace_data;
  ops->to_get_min_fast_tracepoint_insn_len = tdefault_get_min_fast_tracepoint_insn_len;
  ops->to_set_disconnected_tracing = tdefault_set_disconnected_tracing;
  ops->to_set_circular_trace_buffer = tdefault_set_circular_trace_buffer;
  ops->to_set_trace_buffer_size = tdefault_set_trace_buffer_size;
  ops->to_set_trace_notes = tdefault_set_trace_notes;
  ops->to_core_of_thread = tdefault_core_of_thread;
  ops->to_verify_memory = default_verify_memory;
  ops->to_get_tib_address = tdefault_get_tib_address;
  ops->to_set_permissions = tdefault_set_permissions;
  ops->to_static_tracepoint_marker_at = tdefault_static_tracepoint_marker_at;
  ops->to_static_tracepoint_markers_by_strid = tdefault_static_tracepoint_markers_by_strid;
  ops->to_traceframe_info = tdefault_traceframe_info;
  ops->to_use_agent = tdefault_use_agent;
  ops->to_can_use_agent = tdefault_can_use_agent;
  ops->to_supports_btrace = tdefault_supports_btrace;
  ops->to_enable_btrace = tdefault_enable_btrace;
  ops->to_disable_btrace = tdefault_disable_btrace;
  ops->to_teardown_btrace = tdefault_teardown_btrace;
  ops->to_read_btrace = tdefault_read_btrace;
  ops->to_btrace_conf = tdefault_btrace_conf;
  ops->to_record_method = tdefault_record_method;
  ops->to_stop_recording = tdefault_stop_recording;
  ops->to_info_record = tdefault_info_record;
  ops->to_save_record = tdefault_save_record;
  ops->to_delete_record = tdefault_delete_record;
  ops->to_record_is_replaying = tdefault_record_is_replaying;
  ops->to_record_will_replay = tdefault_record_will_replay;
  ops->to_record_stop_replaying = tdefault_record_stop_replaying;
  ops->to_goto_record_begin = tdefault_goto_record_begin;
  ops->to_goto_record_end = tdefault_goto_record_end;
  ops->to_goto_record = tdefault_goto_record;
  ops->to_insn_history = tdefault_insn_history;
  ops->to_insn_history_from = tdefault_insn_history_from;
  ops->to_insn_history_range = tdefault_insn_history_range;
  ops->to_call_history = tdefault_call_history;
  ops->to_call_history_from = tdefault_call_history_from;
  ops->to_call_history_range = tdefault_call_history_range;
  ops->to_augmented_libraries_svr4_read = tdefault_augmented_libraries_svr4_read;
  ops->to_get_unwinder = tdefault_get_unwinder;
  ops->to_get_tailcall_unwinder = tdefault_get_tailcall_unwinder;
  ops->to_prepare_to_generate_core = tdefault_prepare_to_generate_core;
  ops->to_done_generating_core = tdefault_done_generating_core;
}

static void
init_debug_target (struct target_ops *ops)
{
  ops->to_post_attach = debug_post_attach;
  ops->to_detach = debug_detach;
  ops->to_disconnect = debug_disconnect;
  ops->to_resume = debug_resume;
  ops->to_commit_resume = debug_commit_resume;
  ops->to_wait = debug_wait;
  ops->to_fetch_registers = debug_fetch_registers;
  ops->to_store_registers = debug_store_registers;
  ops->to_prepare_to_store = debug_prepare_to_store;
  ops->to_files_info = debug_files_info;
  ops->to_insert_breakpoint = debug_insert_breakpoint;
  ops->to_remove_breakpoint = debug_remove_breakpoint;
  ops->to_stopped_by_sw_breakpoint = debug_stopped_by_sw_breakpoint;
  ops->to_supports_stopped_by_sw_breakpoint = debug_supports_stopped_by_sw_breakpoint;
  ops->to_stopped_by_hw_breakpoint = debug_stopped_by_hw_breakpoint;
  ops->to_supports_stopped_by_hw_breakpoint = debug_supports_stopped_by_hw_breakpoint;
  ops->to_can_use_hw_breakpoint = debug_can_use_hw_breakpoint;
  ops->to_ranged_break_num_registers = debug_ranged_break_num_registers;
  ops->to_insert_hw_breakpoint = debug_insert_hw_breakpoint;
  ops->to_remove_hw_breakpoint = debug_remove_hw_breakpoint;
  ops->to_remove_watchpoint = debug_remove_watchpoint;
  ops->to_insert_watchpoint = debug_insert_watchpoint;
  ops->to_insert_mask_watchpoint = debug_insert_mask_watchpoint;
  ops->to_remove_mask_watchpoint = debug_remove_mask_watchpoint;
  ops->to_stopped_by_watchpoint = debug_stopped_by_watchpoint;
  ops->to_stopped_data_address = debug_stopped_data_address;
  ops->to_watchpoint_addr_within_range = debug_watchpoint_addr_within_range;
  ops->to_region_ok_for_hw_watchpoint = debug_region_ok_for_hw_watchpoint;
  ops->to_can_accel_watchpoint_condition = debug_can_accel_watchpoint_condition;
  ops->to_masked_watch_num_registers = debug_masked_watch_num_registers;
  ops->to_can_do_single_step = debug_can_do_single_step;
  ops->to_terminal_init = debug_terminal_init;
  ops->to_terminal_inferior = debug_terminal_inferior;
  ops->to_terminal_ours_for_output = debug_terminal_ours_for_output;
  ops->to_terminal_ours = debug_terminal_ours;
  ops->to_terminal_info = debug_terminal_info;
  ops->to_kill = debug_kill;
  ops->to_load = debug_load;
  ops->to_post_startup_inferior = debug_post_startup_inferior;
  ops->to_insert_fork_catchpoint = debug_insert_fork_catchpoint;
  ops->to_remove_fork_catchpoint = debug_remove_fork_catchpoint;
  ops->to_insert_vfork_catchpoint = debug_insert_vfork_catchpoint;
  ops->to_remove_vfork_catchpoint = debug_remove_vfork_catchpoint;
  ops->to_follow_fork = debug_follow_fork;
  ops->to_insert_exec_catchpoint = debug_insert_exec_catchpoint;
  ops->to_remove_exec_catchpoint = debug_remove_exec_catchpoint;
  ops->to_follow_exec = debug_follow_exec;
  ops->to_set_syscall_catchpoint = debug_set_syscall_catchpoint;
  ops->to_has_exited = debug_has_exited;
  ops->to_mourn_inferior = debug_mourn_inferior;
  ops->to_can_run = debug_can_run;
  ops->to_pass_signals = debug_pass_signals;
  ops->to_program_signals = debug_program_signals;
  ops->to_thread_alive = debug_thread_alive;
  ops->to_update_thread_list = debug_update_thread_list;
  ops->to_pid_to_str = debug_pid_to_str;
  ops->to_extra_thread_info = debug_extra_thread_info;
  ops->to_thread_name = debug_thread_name;
  ops->to_stop = debug_stop;
  ops->to_interrupt = debug_interrupt;
  ops->to_pass_ctrlc = debug_pass_ctrlc;
  ops->to_rcmd = debug_rcmd;
  ops->to_pid_to_exec_file = debug_pid_to_exec_file;
  ops->to_log_command = debug_log_command;
  ops->to_get_section_table = debug_get_section_table;
  ops->to_can_async_p = debug_can_async_p;
  ops->to_is_async_p = debug_is_async_p;
  ops->to_async = debug_async;
  ops->to_thread_events = debug_thread_events;
  ops->to_supports_non_stop = debug_supports_non_stop;
  ops->to_always_non_stop_p = debug_always_non_stop_p;
  ops->to_find_memory_regions = debug_find_memory_regions;
  ops->to_make_corefile_notes = debug_make_corefile_notes;
  ops->to_get_bookmark = debug_get_bookmark;
  ops->to_goto_bookmark = debug_goto_bookmark;
  ops->to_get_thread_local_address = debug_get_thread_local_address;
  ops->to_xfer_partial = debug_xfer_partial;
  ops->to_get_memory_xfer_limit = debug_get_memory_xfer_limit;
  ops->to_memory_map = debug_memory_map;
  ops->to_flash_erase = debug_flash_erase;
  ops->to_flash_done = debug_flash_done;
  ops->to_read_description = debug_read_description;
  ops->to_get_ada_task_ptid = debug_get_ada_task_ptid;
  ops->to_auxv_parse = debug_auxv_parse;
  ops->to_search_memory = debug_search_memory;
  ops->to_can_execute_reverse = debug_can_execute_reverse;
  ops->to_execution_direction = debug_execution_direction;
  ops->to_supports_multi_process = debug_supports_multi_process;
  ops->to_supports_enable_disable_tracepoint = debug_supports_enable_disable_tracepoint;
  ops->to_supports_string_tracing = debug_supports_string_tracing;
  ops->to_supports_evaluation_of_breakpoint_conditions = debug_supports_evaluation_of_breakpoint_conditions;
  ops->to_can_run_breakpoint_commands = debug_can_run_breakpoint_commands;
  ops->to_thread_architecture = debug_thread_architecture;
  ops->to_thread_address_space = debug_thread_address_space;
  ops->to_filesystem_is_local = debug_filesystem_is_local;
  ops->to_trace_init = debug_trace_init;
  ops->to_download_tracepoint = debug_download_tracepoint;
  ops->to_can_download_tracepoint = debug_can_download_tracepoint;
  ops->to_download_trace_state_variable = debug_download_trace_state_variable;
  ops->to_enable_tracepoint = debug_enable_tracepoint;
  ops->to_disable_tracepoint = debug_disable_tracepoint;
  ops->to_trace_set_readonly_regions = debug_trace_set_readonly_regions;
  ops->to_trace_start = debug_trace_start;
  ops->to_get_trace_status = debug_get_trace_status;
  ops->to_get_tracepoint_status = debug_get_tracepoint_status;
  ops->to_trace_stop = debug_trace_stop;
  ops->to_trace_find = debug_trace_find;
  ops->to_get_trace_state_variable_value = debug_get_trace_state_variable_value;
  ops->to_save_trace_data = debug_save_trace_data;
  ops->to_upload_tracepoints = debug_upload_tracepoints;
  ops->to_upload_trace_state_variables = debug_upload_trace_state_variables;
  ops->to_get_raw_trace_data = debug_get_raw_trace_data;
  ops->to_get_min_fast_tracepoint_insn_len = debug_get_min_fast_tracepoint_insn_len;
  ops->to_set_disconnected_tracing = debug_set_disconnected_tracing;
  ops->to_set_circular_trace_buffer = debug_set_circular_trace_buffer;
  ops->to_set_trace_buffer_size = debug_set_trace_buffer_size;
  ops->to_set_trace_notes = debug_set_trace_notes;
  ops->to_core_of_thread = debug_core_of_thread;
  ops->to_verify_memory = debug_verify_memory;
  ops->to_get_tib_address = debug_get_tib_address;
  ops->to_set_permissions = debug_set_permissions;
  ops->to_static_tracepoint_marker_at = debug_static_tracepoint_marker_at;
  ops->to_static_tracepoint_markers_by_strid = debug_static_tracepoint_markers_by_strid;
  ops->to_traceframe_info = debug_traceframe_info;
  ops->to_use_agent = debug_use_agent;
  ops->to_can_use_agent = debug_can_use_agent;
  ops->to_supports_btrace = debug_supports_btrace;
  ops->to_enable_btrace = debug_enable_btrace;
  ops->to_disable_btrace = debug_disable_btrace;
  ops->to_teardown_btrace = debug_teardown_btrace;
  ops->to_read_btrace = debug_read_btrace;
  ops->to_btrace_conf = debug_btrace_conf;
  ops->to_record_method = debug_record_method;
  ops->to_stop_recording = debug_stop_recording;
  ops->to_info_record = debug_info_record;
  ops->to_save_record = debug_save_record;
  ops->to_delete_record = debug_delete_record;
  ops->to_record_is_replaying = debug_record_is_replaying;
  ops->to_record_will_replay = debug_record_will_replay;
  ops->to_record_stop_replaying = debug_record_stop_replaying;
  ops->to_goto_record_begin = debug_goto_record_begin;
  ops->to_goto_record_end = debug_goto_record_end;
  ops->to_goto_record = debug_goto_record;
  ops->to_insn_history = debug_insn_history;
  ops->to_insn_history_from = debug_insn_history_from;
  ops->to_insn_history_range = debug_insn_history_range;
  ops->to_call_history = debug_call_history;
  ops->to_call_history_from = debug_call_history_from;
  ops->to_call_history_range = debug_call_history_range;
  ops->to_augmented_libraries_svr4_read = debug_augmented_libraries_svr4_read;
  ops->to_get_unwinder = debug_get_unwinder;
  ops->to_get_tailcall_unwinder = debug_get_tailcall_unwinder;
  ops->to_prepare_to_generate_core = debug_prepare_to_generate_core;
  ops->to_done_generating_core = debug_done_generating_core;
}
