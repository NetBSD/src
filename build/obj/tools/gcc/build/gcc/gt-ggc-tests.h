/* Type information for ggc-tests.c.
   Copyright (C) 2004-2020 Free Software Foundation, Inc.

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

/* This file is machine generated.  Do not edit.  */

void
gt_ggc_mx_test_struct (void *x_p)
{
  struct test_struct * const x = (struct test_struct *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_11test_struct ((*x).other);
    }
}

void
gt_ggc_mx_test_of_length (void *x_p)
{
  struct test_of_length * const x = (struct test_of_length *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      {
        size_t l0 = (size_t)(((*x)).num_elem);
        {
          size_t i0;
          for (i0 = 0; i0 != l0; i0++) {
            gt_ggc_m_14test_of_length ((*x).elem[i0]);
          }
        }
      }
    }
}

void
gt_ggc_mx_test_other (void *x_p)
{
  struct test_other * const x = (struct test_other *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_11test_struct ((*x).m_ptr);
    }
}

void
gt_ggc_mx_test_of_union (void *x_p)
{
  struct test_of_union * const x = (struct test_of_union *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      switch ((int) (calc_desc ((*x).m_kind)))
        {
        case WHICH_FIELD_USE_TEST_STRUCT:
          gt_ggc_m_11test_struct ((*x).m_u.u_test_struct);
          break;
        case WHICH_FIELD_USE_TEST_OTHER:
          gt_ggc_m_10test_other ((*x).m_u.u_test_other);
          break;
        default:
          break;
        }
    }
}

void
gt_ggc_mx_example_base (void *x_p)
{
  struct example_base * const x = (struct example_base *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      switch ((int) (((*x)).m_kind))
        {
        case 0:
          gt_ggc_m_11test_struct ((*x).m_a);
          break;
        case 2:
          {
            some_other_subclass *sub = static_cast <some_other_subclass *> (x);
            gt_ggc_m_11test_struct ((*sub).m_c);
            gt_ggc_m_11test_struct ((*sub).m_a);
          }
          break;
        case 1:
          {
            some_subclass *sub = static_cast <some_subclass *> (x);
            gt_ggc_m_11test_struct ((*sub).m_b);
            gt_ggc_m_11test_struct ((*sub).m_a);
          }
          break;
        /* Unrecognized tag value.  */
        default: gcc_unreachable (); 
        }
    }
}

void
gt_ggc_mx_test_node (void *x_p)
{
  struct test_node * x = (struct test_node *)x_p;
  struct test_node * xlimit = x;
  while (ggc_test_and_set_mark (xlimit))
   xlimit = ((*xlimit).m_next);
  if (x != xlimit)
    for (;;)
      {
        struct test_node * const xprev = ((*x).m_prev);
        if (xprev == NULL) break;
        x = xprev;
        (void) ggc_test_and_set_mark (xprev);
      }
  while (x != xlimit)
    {
      gt_ggc_m_9test_node ((*x).m_prev);
      gt_ggc_m_9test_node ((*x).m_next);
      x = ((*x).m_next);
    }
}

void
gt_ggc_mx_user_struct (void *x_p)
{
  user_struct * const x = (user_struct *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_pch_nx_test_struct (void *x_p)
{
  struct test_struct * const x = (struct test_struct *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_11test_struct))
    {
      gt_pch_n_11test_struct ((*x).other);
    }
}

void
gt_pch_nx_test_of_length (void *x_p)
{
  struct test_of_length * const x = (struct test_of_length *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_14test_of_length))
    {
      {
        size_t l0 = (size_t)(((*x)).num_elem);
        {
          size_t i0;
          for (i0 = 0; i0 != l0; i0++) {
            gt_pch_n_14test_of_length ((*x).elem[i0]);
          }
        }
      }
    }
}

void
gt_pch_nx_test_other (void *x_p)
{
  struct test_other * const x = (struct test_other *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_10test_other))
    {
      gt_pch_n_11test_struct ((*x).m_ptr);
    }
}

void
gt_pch_nx_test_of_union (void *x_p)
{
  struct test_of_union * const x = (struct test_of_union *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_13test_of_union))
    {
      switch ((int) (calc_desc ((*x).m_kind)))
        {
        case WHICH_FIELD_USE_TEST_STRUCT:
          gt_pch_n_11test_struct ((*x).m_u.u_test_struct);
          break;
        case WHICH_FIELD_USE_TEST_OTHER:
          gt_pch_n_10test_other ((*x).m_u.u_test_other);
          break;
        default:
          break;
        }
    }
}

void
gt_pch_nx_example_base (void *x_p)
{
  struct example_base * const x = (struct example_base *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_12example_base))
    {
      switch ((int) (((*x)).m_kind))
        {
        case 0:
          gt_pch_n_11test_struct ((*x).m_a);
          break;
        case 2:
          {
            some_other_subclass *sub = static_cast <some_other_subclass *> (x);
            gt_pch_n_11test_struct ((*sub).m_c);
            gt_pch_n_11test_struct ((*sub).m_a);
          }
          break;
        case 1:
          {
            some_subclass *sub = static_cast <some_subclass *> (x);
            gt_pch_n_11test_struct ((*sub).m_b);
            gt_pch_n_11test_struct ((*sub).m_a);
          }
          break;
        /* Unrecognized tag value.  */
        default: gcc_unreachable (); 
        }
    }
}

void
gt_pch_nx_test_node (void *x_p)
{
  struct test_node * x = (struct test_node *)x_p;
  struct test_node * xlimit = x;
  while (gt_pch_note_object (xlimit, xlimit, gt_pch_p_9test_node))
   xlimit = ((*xlimit).m_next);
  if (x != xlimit)
    for (;;)
      {
        struct test_node * const xprev = ((*x).m_prev);
        if (xprev == NULL) break;
        x = xprev;
        (void) gt_pch_note_object (xprev, xprev, gt_pch_p_9test_node);
      }
  while (x != xlimit)
    {
      gt_pch_n_9test_node ((*x).m_prev);
      gt_pch_n_9test_node ((*x).m_next);
      x = ((*x).m_next);
    }
}

void
gt_pch_nx_user_struct (void *x_p)
{
  user_struct * const x = (user_struct *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_11user_struct))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_p_11test_struct (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct test_struct * x ATTRIBUTE_UNUSED = (struct test_struct *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).other), cookie);
}

void
gt_pch_p_14test_of_length (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct test_of_length * x ATTRIBUTE_UNUSED = (struct test_of_length *)x_p;
  {
    size_t l0 = (size_t)(((*x)).num_elem);
    {
      size_t i0;
      for (i0 = 0; i0 != l0; i0++) {
        if ((void *)(x) == this_obj)
          op (&((*x).elem[i0]), cookie);
      }
    }
  }
}

void
gt_pch_p_10test_other (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct test_other * x ATTRIBUTE_UNUSED = (struct test_other *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).m_ptr), cookie);
}

void
gt_pch_p_13test_of_union (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct test_of_union * x ATTRIBUTE_UNUSED = (struct test_of_union *)x_p;
  switch ((int) (calc_desc ((*x).m_kind)))
    {
    case WHICH_FIELD_USE_TEST_STRUCT:
      if ((void *)(x) == this_obj)
        op (&((*x).m_u.u_test_struct), cookie);
      break;
    case WHICH_FIELD_USE_TEST_OTHER:
      if ((void *)(x) == this_obj)
        op (&((*x).m_u.u_test_other), cookie);
      break;
    default:
      break;
    }
}

void
gt_pch_p_12example_base (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct example_base * x ATTRIBUTE_UNUSED = (struct example_base *)x_p;
  switch ((int) (((*x)).m_kind))
    {
    case 0:
      if ((void *)(x) == this_obj)
        op (&((*x).m_a), cookie);
      break;
    case 2:
      {
        some_other_subclass *sub = static_cast <some_other_subclass *> (x);
        if ((void *)(x) == this_obj)
          op (&((*sub).m_c), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).m_a), cookie);
      }
      break;
    case 1:
      {
        some_subclass *sub = static_cast <some_subclass *> (x);
        if ((void *)(x) == this_obj)
          op (&((*sub).m_b), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).m_a), cookie);
      }
      break;
    /* Unrecognized tag value.  */
    default: gcc_unreachable (); 
    }
}

void
gt_pch_p_9test_node (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct test_node * x ATTRIBUTE_UNUSED = (struct test_node *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).m_prev), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).m_next), cookie);
}

void
gt_pch_p_11user_struct (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct user_struct * x ATTRIBUTE_UNUSED = (struct user_struct *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_ggc_tests_h[] = {
  {
    &dummy_unittesting_tree,
    1,
    sizeof (dummy_unittesting_tree),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &root_user_struct_ptr,
    1,
    sizeof (root_user_struct_ptr),
    &gt_ggc_mx_user_struct,
    &gt_pch_nx_user_struct
  },
  {
    &root_test_node,
    1,
    sizeof (root_test_node),
    &gt_ggc_mx_test_node,
    &gt_pch_nx_test_node
  },
  {
    &test_some_other_subclass_as_base_ptr,
    1,
    sizeof (test_some_other_subclass_as_base_ptr),
    &gt_ggc_mx_example_base,
    &gt_pch_nx_example_base
  },
  {
    &test_some_subclass_as_base_ptr,
    1,
    sizeof (test_some_subclass_as_base_ptr),
    &gt_ggc_mx_example_base,
    &gt_pch_nx_example_base
  },
  {
    &test_some_other_subclass,
    1,
    sizeof (test_some_other_subclass),
    &gt_ggc_mx_example_base,
    &gt_pch_nx_example_base
  },
  {
    &test_some_subclass,
    1,
    sizeof (test_some_subclass),
    &gt_ggc_mx_example_base,
    &gt_pch_nx_example_base
  },
  {
    &test_example_base,
    1,
    sizeof (test_example_base),
    &gt_ggc_mx_example_base,
    &gt_pch_nx_example_base
  },
  {
    &root_test_of_union_2,
    1,
    sizeof (root_test_of_union_2),
    &gt_ggc_mx_test_of_union,
    &gt_pch_nx_test_of_union
  },
  {
    &root_test_of_union_1,
    1,
    sizeof (root_test_of_union_1),
    &gt_ggc_mx_test_of_union,
    &gt_pch_nx_test_of_union
  },
  {
    &root_test_of_length,
    1,
    sizeof (root_test_of_length),
    &gt_ggc_mx_test_of_length,
    &gt_pch_nx_test_of_length
  },
  {
    &root_test_struct,
    1,
    sizeof (root_test_struct),
    &gt_ggc_mx_test_struct,
    &gt_pch_nx_test_struct
  },
  LAST_GGC_ROOT_TAB
};

EXPORTED_CONST struct ggc_root_tab gt_ggc_rd_gt_ggc_tests_h[] = {
  { &test_of_deletable, 1, sizeof (test_of_deletable), NULL, NULL },
  LAST_GGC_ROOT_TAB
};

