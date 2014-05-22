/* Web construction code for GNU compiler.
   Contributed by Jan Hubicka.
   Copyright (C) 2001-2013 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

/* Simple optimization pass that splits independent uses of each pseudo,
   increasing effectiveness of other optimizations.  The optimization can
   serve as an example of use for the dataflow module.

   We don't split registers with REG_USERVAR set unless -fmessy-debugging
   is specified, because debugging information about such split variables
   is almost unusable.

   TODO
    - We may use profile information and ignore infrequent use for the
      purpose of web unifying, inserting the compensation code later to
      implement full induction variable expansion for loops (currently
      we expand only if the induction variable is dead afterward, which
      is often the case).  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "diagnostic-core.h"

#include "rtl.h"
#include "hard-reg-set.h"
#include "flags.h"
#include "obstack.h"
#include "basic-block.h"
#include "df.h"
#include "function.h"
#include "insn-config.h"
#include "recog.h"
#include "tree-pass.h"


/* Find the root of unionfind tree (the representative of set).  */

struct web_entry *
unionfind_root (struct web_entry *element)
{
  struct web_entry *element1 = element, *element2;

  while (element->pred)
    element = element->pred;
  while (element1->pred)
    {
      element2 = element1->pred;
      element1->pred = element;
      element1 = element2;
    }
  return element;
}

/* Union sets.
   Return true if FIRST and SECOND points to the same web entry structure and
   nothing is done.  Otherwise, return false.  */

bool
unionfind_union (struct web_entry *first, struct web_entry *second)
{
  first = unionfind_root (first);
  second = unionfind_root (second);
  if (first == second)
    return true;
  second->pred = first;
  return false;
}

/* For INSN, union all defs and uses that are linked by match_dup.
   FUN is the function that does the union.  */

static void
union_match_dups (rtx insn, struct web_entry *def_entry,
		  struct web_entry *use_entry,
		  bool (*fun) (struct web_entry *, struct web_entry *))
{
  struct df_insn_info *insn_info = DF_INSN_INFO_GET (insn);
  df_ref *use_link = DF_INSN_INFO_USES (insn_info);
  df_ref *def_link = DF_INSN_INFO_DEFS (insn_info);
  struct web_entry *dup_entry;
  int i;

  extract_insn (insn);

  for (i = 0; i < recog_data.n_dups; i++)
    {
      int op = recog_data.dup_num[i];
      enum op_type type = recog_data.operand_type[op];
      df_ref *ref, *dupref;
      struct web_entry *entry;

      for (dup_entry = use_entry, dupref = use_link; *dupref; dupref++)
	if (DF_REF_LOC (*dupref) == recog_data.dup_loc[i])
	  break;

      if (*dupref == NULL && type == OP_INOUT)
	{

	  for (dup_entry = def_entry, dupref = def_link; *dupref; dupref++)
	    if (DF_REF_LOC (*dupref) == recog_data.dup_loc[i])
	      break;
	}
      /* ??? *DUPREF can still be zero, because when an operand matches
	 a memory, DF_REF_LOC (use_link[n]) points to the register part
	 of the address, whereas recog_data.dup_loc[m] points to the
	 entire memory ref, thus we fail to find the duplicate entry,
         even though it is there.
         Example: i686-pc-linux-gnu gcc.c-torture/compile/950607-1.c
		  -O3 -fomit-frame-pointer -funroll-loops  */
      if (*dupref == NULL
	  || DF_REF_REGNO (*dupref) < FIRST_PSEUDO_REGISTER)
	continue;

      ref = type == OP_IN ? use_link : def_link;
      entry = type == OP_IN ? use_entry : def_entry;
      for (; *ref; ref++)
	if (DF_REF_LOC (*ref) == recog_data.operand_loc[op])
	  break;

      if (!*ref && type == OP_INOUT)
	{
	  for (ref = use_link, entry = use_entry; *ref; ref++)
	    if (DF_REF_LOC (*ref) == recog_data.operand_loc[op])
	      break;
	}

      gcc_assert (*ref);
      (*fun) (dup_entry + DF_REF_ID (*dupref), entry + DF_REF_ID (*ref));
    }
}

/* For each use, all possible defs reaching it must come in the same
   register, union them.
   FUN is the function that does the union.

   In USED, we keep the DF_REF_ID of the first uninitialized uses of a
   register, so that all uninitialized uses of the register can be
   combined into a single web.  We actually offset it by 2, because
   the values 0 and 1 are reserved for use by entry_register.  */

void
union_defs (df_ref use, struct web_entry *def_entry,
	    unsigned int *used, struct web_entry *use_entry,
 	    bool (*fun) (struct web_entry *, struct web_entry *))
{
  struct df_insn_info *insn_info = DF_REF_INSN_INFO (use);
  struct df_link *link = DF_REF_CHAIN (use);
  df_ref *eq_use_link;
  df_ref *def_link;
  rtx set;

  if (insn_info)
    {
      rtx insn = insn_info->insn;
      eq_use_link = DF_INSN_INFO_EQ_USES (insn_info);
      def_link = DF_INSN_INFO_DEFS (insn_info);
      set = single_set (insn);
    }
  else
    {
      /* An artificial use.  It links up with nothing.  */
      eq_use_link = NULL;
      def_link = NULL;
      set = NULL;
    }

  /* Union all occurrences of the same register in reg notes.  */

  if (eq_use_link)
    while (*eq_use_link)
      {
	if (use != *eq_use_link
	    && DF_REF_REAL_REG (use) == DF_REF_REAL_REG (*eq_use_link))
	  (*fun) (use_entry + DF_REF_ID (use),
		  use_entry + DF_REF_ID (*eq_use_link));
	eq_use_link++;
    }

  /* Recognize trivial noop moves and attempt to keep them as noop.  */

  if (set
      && SET_SRC (set) == DF_REF_REG (use)
      && SET_SRC (set) == SET_DEST (set))
    {
      if (def_link)
	while (*def_link)
	  {
	    if (DF_REF_REAL_REG (use) == DF_REF_REAL_REG (*def_link))
	      (*fun) (use_entry + DF_REF_ID (use),
		      def_entry + DF_REF_ID (*def_link));
	    def_link++;
	  }
    }

  /* UD chains of uninitialized REGs are empty.  Keeping all uses of
     the same uninitialized REG in a single web is not necessary for
     correctness, since the uses are undefined, but it's wasteful to
     allocate one register or slot for each reference.  Furthermore,
     creating new pseudos for uninitialized references in debug insns
     (see PR 42631) causes -fcompare-debug failures.  We record the
     number of the first uninitialized reference we found, and merge
     with it any other uninitialized references to the same
     register.  */
  if (!link)
    {
      int regno = REGNO (DF_REF_REAL_REG (use));
      if (used[regno])
	(*fun) (use_entry + DF_REF_ID (use), use_entry + used[regno] - 2);
      else
	used[regno] = DF_REF_ID (use) + 2;
    }

  while (link)
    {
      (*fun) (use_entry + DF_REF_ID (use),
	      def_entry + DF_REF_ID (link->ref));
      link = link->next;
    }

  /* A READ_WRITE use requires the corresponding def to be in the same
     register.  Find it and union.  */
  if (DF_REF_FLAGS (use) & DF_REF_READ_WRITE)
    {
      df_ref *link;

      if (insn_info)
	link = DF_INSN_INFO_DEFS (insn_info);
      else
	link = NULL;

      if (link)
	while (*link)
	  {
	    if (DF_REF_REAL_REG (*link) == DF_REF_REAL_REG (use))
	      (*fun) (use_entry + DF_REF_ID (use),
		      def_entry + DF_REF_ID (*link));
	    link++;
	  }
    }
}

/* Find the corresponding register for the given entry.  */

static rtx
entry_register (struct web_entry *entry, df_ref ref, unsigned int *used)
{
  struct web_entry *root;
  rtx reg, newreg;

  /* Find the corresponding web and see if it has been visited.  */
  root = unionfind_root (entry);
  if (root->reg)
    return root->reg;

  /* We are seeing this web for the first time, do the assignment.  */
  reg = DF_REF_REAL_REG (ref);

  /* In case the original register is already assigned, generate new
     one.  Since we use USED to merge uninitialized refs into a single
     web, we might found an element to be nonzero without our having
     used it.  Test for 1, because union_defs saves it for our use,
     and there won't be any use for the other values when we get to
     this point.  */
  if (used[REGNO (reg)] != 1)
    newreg = reg, used[REGNO (reg)] = 1;
  else
    {
      newreg = gen_reg_rtx (GET_MODE (reg));
      REG_USERVAR_P (newreg) = REG_USERVAR_P (reg);
      REG_POINTER (newreg) = REG_POINTER (reg);
      REG_ATTRS (newreg) = REG_ATTRS (reg);
      if (dump_file)
	fprintf (dump_file, "Web oldreg=%i newreg=%i\n", REGNO (reg),
		 REGNO (newreg));
    }

  root->reg = newreg;
  return newreg;
}

/* Replace the reference by REG.  */

static void
replace_ref (df_ref ref, rtx reg)
{
  rtx oldreg = DF_REF_REAL_REG (ref);
  rtx *loc = DF_REF_REAL_LOC (ref);
  unsigned int uid = DF_REF_INSN_UID (ref);

  if (oldreg == reg)
    return;
  if (dump_file)
    fprintf (dump_file, "Updating insn %i (%i->%i)\n",
	     uid, REGNO (oldreg), REGNO (reg));
  *loc = reg;
  df_insn_rescan (DF_REF_INSN (ref));
}


static bool
gate_handle_web (void)
{
  return (optimize > 0 && flag_web);
}

/* Main entry point.  */

static unsigned int
web_main (void)
{
  struct web_entry *def_entry;
  struct web_entry *use_entry;
  unsigned int max = max_reg_num ();
  unsigned int *used;
  basic_block bb;
  unsigned int uses_num = 0;
  rtx insn;

  df_set_flags (DF_NO_HARD_REGS + DF_EQ_NOTES);
  df_set_flags (DF_RD_PRUNE_DEAD_DEFS);
  df_chain_add_problem (DF_UD_CHAIN);
  df_analyze ();
  df_set_flags (DF_DEFER_INSN_RESCAN);

  /* Assign ids to the uses.  */
  FOR_ALL_BB (bb)
    FOR_BB_INSNS (bb, insn)
    {
      unsigned int uid = INSN_UID (insn);
      if (NONDEBUG_INSN_P (insn))
	{
	  df_ref *use_rec;
	  for (use_rec = DF_INSN_UID_USES (uid); *use_rec; use_rec++)
	    {
	      df_ref use = *use_rec;
	      if (DF_REF_REGNO (use) >= FIRST_PSEUDO_REGISTER)
		DF_REF_ID (use) = uses_num++;
	    }
	  for (use_rec = DF_INSN_UID_EQ_USES (uid); *use_rec; use_rec++)
	    {
	      df_ref use = *use_rec;
	      if (DF_REF_REGNO (use) >= FIRST_PSEUDO_REGISTER)
		DF_REF_ID (use) = uses_num++;
	    }
	}
    }

  /* Record the number of uses and defs at the beginning of the optimization.  */
  def_entry = XCNEWVEC (struct web_entry, DF_DEFS_TABLE_SIZE());
  used = XCNEWVEC (unsigned, max);
  use_entry = XCNEWVEC (struct web_entry, uses_num);

  /* Produce the web.  */
  FOR_ALL_BB (bb)
    FOR_BB_INSNS (bb, insn)
    {
      unsigned int uid = INSN_UID (insn);
      if (NONDEBUG_INSN_P (insn))
	{
	  df_ref *use_rec;
	  union_match_dups (insn, def_entry, use_entry, unionfind_union);
	  for (use_rec = DF_INSN_UID_USES (uid); *use_rec; use_rec++)
	    {
	      df_ref use = *use_rec;
	      if (DF_REF_REGNO (use) >= FIRST_PSEUDO_REGISTER)
		union_defs (use, def_entry, used, use_entry, unionfind_union);
	    }
	  for (use_rec = DF_INSN_UID_EQ_USES (uid); *use_rec; use_rec++)
	    {
	      df_ref use = *use_rec;
	      if (DF_REF_REGNO (use) >= FIRST_PSEUDO_REGISTER)
		union_defs (use, def_entry, used, use_entry, unionfind_union);
	    }
	}
    }

  /* Update the instruction stream, allocating new registers for split pseudos
     in progress.  */
  FOR_ALL_BB (bb)
    FOR_BB_INSNS (bb, insn)
    {
      unsigned int uid = INSN_UID (insn);

      if (NONDEBUG_INSN_P (insn)
	  /* Ignore naked clobber.  For example, reg 134 in the second insn
	     of the following sequence will not be replaced.

	       (insn (clobber (reg:SI 134)))

	       (insn (set (reg:SI 0 r0) (reg:SI 134)))

	     Thus the later passes can optimize them away.  */
	  && GET_CODE (PATTERN (insn)) != CLOBBER)
	{
	  df_ref *use_rec;
	  df_ref *def_rec;
	  for (use_rec = DF_INSN_UID_USES (uid); *use_rec; use_rec++)
	    {
	      df_ref use = *use_rec;
	      if (DF_REF_REGNO (use) >= FIRST_PSEUDO_REGISTER)
		replace_ref (use, entry_register (use_entry + DF_REF_ID (use), use, used));
	    }
	  for (use_rec = DF_INSN_UID_EQ_USES (uid); *use_rec; use_rec++)
	    {
	      df_ref use = *use_rec;
	      if (DF_REF_REGNO (use) >= FIRST_PSEUDO_REGISTER)
		replace_ref (use, entry_register (use_entry + DF_REF_ID (use), use, used));
	    }
	  for (def_rec = DF_INSN_UID_DEFS (uid); *def_rec; def_rec++)
	    {
	      df_ref def = *def_rec;
	      if (DF_REF_REGNO (def) >= FIRST_PSEUDO_REGISTER)
		replace_ref (def, entry_register (def_entry + DF_REF_ID (def), def, used));
	    }
	}
    }

  free (def_entry);
  free (use_entry);
  free (used);
  return 0;
}

struct rtl_opt_pass pass_web =
{
 {
  RTL_PASS,
  "web",                                /* name */
  OPTGROUP_NONE,                        /* optinfo_flags */
  gate_handle_web,                      /* gate */
  web_main,		                /* execute */
  NULL,                                 /* sub */
  NULL,                                 /* next */
  0,                                    /* static_pass_number */
  TV_WEB,                               /* tv_id */
  0,                                    /* properties_required */
  0,                                    /* properties_provided */
  0,                                    /* properties_destroyed */
  0,                                    /* todo_flags_start */
  TODO_df_finish | TODO_verify_rtl_sharing  /* todo_flags_finish */
 }
};
