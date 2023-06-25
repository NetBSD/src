/* Type information for GCC.
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
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "predict.h"
#include "tree.h"
#include "rtl.h"
#include "gimple.h"
#include "fold-const.h"
#include "insn-codes.h"
#include "splay-tree.h"
#include "alias.h"
#include "insn-config.h"
#include "flags.h"
#include "expmed.h"
#include "dojump.h"
#include "explow.h"
#include "calls.h"
#include "memmodel.h"
#include "emit-rtl.h"
#include "varasm.h"
#include "stmt.h"
#include "expr.h"
#include "alloc-pool.h"
#include "cselib.h"
#include "insn-addr.h"
#include "optabs.h"
#include "libfuncs.h"
#include "debug.h"
#include "internal-fn.h"
#include "gimple-fold.h"
#include "value-range.h"
#include "tree-eh.h"
#include "gimple-iterator.h"
#include "gimple-ssa.h"
#include "tree-cfg.h"
#include "tree-vrp.h"
#include "tree-phinodes.h"
#include "ssa-iterators.h"
#include "stringpool.h"
#include "tree-ssanames.h"
#include "tree-ssa-loop.h"
#include "tree-ssa-loop-ivopts.h"
#include "tree-ssa-loop-manip.h"
#include "tree-ssa-loop-niter.h"
#include "tree-into-ssa.h"
#include "tree-dfa.h"
#include "tree-ssa.h"
#include "reload.h"
#include "cpplib.h"
#include "tree-chrec.h"
#include "except.h"
#include "output.h"
#include "cfgloop.h"
#include "target.h"
#include "lto-streamer.h"
#include "target-globals.h"
#include "ipa-ref.h"
#include "cgraph.h"
#include "symbol-summary.h"
#include "ipa-prop.h"
#include "ipa-fnsummary.h"
#include "dwarf2out.h"
#include "omp-general.h"
#include "omp-offload.h"

/* See definition in function.h.  */
#undef cfun

/* Types with a "gcc::" namespace have it stripped
   during gengtype parsing.  Provide a "using" directive
   to ensure that the fully-qualified types are found.  */
using namespace gcc;

void
gt_ggc_mx_line_maps (void *x_p)
{
  struct line_maps * const x = (struct line_maps *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      {
        size_t l0 = (size_t)(((*x).info_ordinary).used);
        if ((*x).info_ordinary.maps != NULL) {
          size_t i0;
          for (i0 = 0; i0 != (size_t)(l0); i0++) {
            gt_ggc_m_S ((*x).info_ordinary.maps[i0].to_file);
          }
          ggc_mark ((*x).info_ordinary.maps);
        }
      }
      {
        size_t l1 = (size_t)(((*x).info_macro).used);
        if ((*x).info_macro.maps != NULL) {
          size_t i1;
          for (i1 = 0; i1 != (size_t)(l1); i1++) {
            {
              union tree_node * const x2 =
                ((*x).info_macro.maps[i1].macro) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).info_macro.maps[i1].macro))) : NULL;
              gt_ggc_m_9tree_node (x2);
            }
            if ((*x).info_macro.maps[i1].macro_locations != NULL) {
              ggc_mark ((*x).info_macro.maps[i1].macro_locations);
            }
          }
          ggc_mark ((*x).info_macro.maps);
        }
      }
      {
        size_t l3 = (size_t)(((*x).location_adhoc_data_map).allocated);
        if ((*x).location_adhoc_data_map.data != NULL) {
          size_t i3;
          for (i3 = 0; i3 != (size_t)(l3); i3++) {
          }
          ggc_mark ((*x).location_adhoc_data_map.data);
        }
      }
    }
}

void
gt_ggc_mx_cpp_token (void *x_p)
{
  struct cpp_token * const x = (struct cpp_token *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      switch ((int) (cpp_token_val_index (&((*x)))))
        {
        case CPP_TOKEN_FLD_NODE:
          {
            union tree_node * const x0 =
              ((*x).val.node.node) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).val.node.node))) : NULL;
            gt_ggc_m_9tree_node (x0);
          }
          {
            union tree_node * const x1 =
              ((*x).val.node.spelling) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).val.node.spelling))) : NULL;
            gt_ggc_m_9tree_node (x1);
          }
          break;
        case CPP_TOKEN_FLD_SOURCE:
          gt_ggc_m_9cpp_token ((*x).val.source);
          break;
        case CPP_TOKEN_FLD_STR:
          gt_ggc_m_S ((*x).val.str.text);
          break;
        case CPP_TOKEN_FLD_ARG_NO:
          {
            union tree_node * const x2 =
              ((*x).val.macro_arg.spelling) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).val.macro_arg.spelling))) : NULL;
            gt_ggc_m_9tree_node (x2);
          }
          break;
        case CPP_TOKEN_FLD_TOKEN_NO:
          break;
        case CPP_TOKEN_FLD_PRAGMA:
          break;
        default:
          break;
        }
    }
}

void
gt_ggc_mx_cpp_macro (void *x_p)
{
  struct cpp_macro * const x = (struct cpp_macro *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      switch ((int) (((*x)).kind == cmk_assert))
        {
        case false:
          if ((*x).parm.params != NULL) {
            size_t i0;
            for (i0 = 0; i0 != (size_t)(((*x)).paramc); i0++) {
              {
                union tree_node * const x1 =
                  ((*x).parm.params[i0]) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).parm.params[i0]))) : NULL;
                gt_ggc_m_9tree_node (x1);
              }
            }
            ggc_mark ((*x).parm.params);
          }
          break;
        case true:
          gt_ggc_m_9cpp_macro ((*x).parm.next);
          break;
        default:
          break;
        }
      switch ((int) (((*x)).kind == cmk_traditional))
        {
        case false:
          {
            size_t i2;
            size_t l2 = (size_t)(((*x)).count);
            for (i2 = 0; i2 != l2; i2++) {
              switch ((int) (cpp_token_val_index (&((*x).exp.tokens[i2]))))
                {
                case CPP_TOKEN_FLD_NODE:
                  {
                    union tree_node * const x3 =
                      ((*x).exp.tokens[i2].val.node.node) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).exp.tokens[i2].val.node.node))) : NULL;
                    gt_ggc_m_9tree_node (x3);
                  }
                  {
                    union tree_node * const x4 =
                      ((*x).exp.tokens[i2].val.node.spelling) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).exp.tokens[i2].val.node.spelling))) : NULL;
                    gt_ggc_m_9tree_node (x4);
                  }
                  break;
                case CPP_TOKEN_FLD_SOURCE:
                  gt_ggc_m_9cpp_token ((*x).exp.tokens[i2].val.source);
                  break;
                case CPP_TOKEN_FLD_STR:
                  gt_ggc_m_S ((*x).exp.tokens[i2].val.str.text);
                  break;
                case CPP_TOKEN_FLD_ARG_NO:
                  {
                    union tree_node * const x5 =
                      ((*x).exp.tokens[i2].val.macro_arg.spelling) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).exp.tokens[i2].val.macro_arg.spelling))) : NULL;
                    gt_ggc_m_9tree_node (x5);
                  }
                  break;
                case CPP_TOKEN_FLD_TOKEN_NO:
                  break;
                case CPP_TOKEN_FLD_PRAGMA:
                  break;
                default:
                  break;
                }
            }
          }
          break;
        case true:
          gt_ggc_m_S ((*x).exp.text);
          break;
        default:
          break;
        }
    }
}

void
gt_ggc_mx_string_concat (void *x_p)
{
  struct string_concat * const x = (struct string_concat *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      if ((*x).m_locs != NULL) {
        ggc_mark ((*x).m_locs);
      }
    }
}

void
gt_ggc_mx_string_concat_db (void *x_p)
{
  struct string_concat_db * const x = (struct string_concat_db *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_38hash_map_location_hash_string_concat__ ((*x).m_table);
    }
}

void
gt_ggc_mx_hash_map_location_hash_string_concat__ (void *x_p)
{
  hash_map<location_hash,string_concat*> * const x = (hash_map<location_hash,string_concat*> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct string_concat *& x)
{
  if (x)
    gt_ggc_mx_string_concat ((void *) x);
}

void
gt_ggc_mx (struct location_hash& x_r ATTRIBUTE_UNUSED)
{
  struct location_hash * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_bitmap_head (void *x_p)
{
  struct bitmap_head * const x = (struct bitmap_head *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_14bitmap_element ((*x).first);
    }
}

void
gt_ggc_mx_rtx_def (void *x_p)
{
  struct rtx_def * x = (struct rtx_def *)x_p;
  struct rtx_def * xlimit = x;
  while (ggc_test_and_set_mark (xlimit))
   xlimit = (RTX_NEXT (&(*xlimit)));
  if (x != xlimit)
    for (;;)
      {
        struct rtx_def * const xprev = (RTX_PREV (&(*x)));
        if (xprev == NULL) break;
        x = xprev;
        (void) ggc_test_and_set_mark (xprev);
      }
  while (x != xlimit)
    {
      switch ((int) (0))
        {
        case 0:
          switch ((int) (GET_CODE (&(*x))))
            {
            case DEBUG_MARKER:
              break;
            case DEBUG_PARAMETER_REF:
              gt_ggc_m_9tree_node ((*x).u.fld[0].rt_tree);
              break;
            case ENTRY_VALUE:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case DEBUG_IMPLICIT_PTR:
              gt_ggc_m_9tree_node ((*x).u.fld[0].rt_tree);
              break;
            case VAR_LOCATION:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_9tree_node ((*x).u.fld[0].rt_tree);
              break;
            case FMA:
              gt_ggc_m_7rtx_def ((*x).u.fld[2].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case US_TRUNCATE:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SS_TRUNCATE:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case US_MINUS:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case US_ASHIFT:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SS_ASHIFT:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SS_ABS:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case US_NEG:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SS_NEG:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SS_MINUS:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case US_PLUS:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SS_PLUS:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case VEC_SERIES:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case VEC_DUPLICATE:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case VEC_CONCAT:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case VEC_SELECT:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case VEC_MERGE:
              gt_ggc_m_7rtx_def ((*x).u.fld[2].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case LO_SUM:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case HIGH:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case ZERO_EXTRACT:
              gt_ggc_m_7rtx_def ((*x).u.fld[2].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SIGN_EXTRACT:
              gt_ggc_m_7rtx_def ((*x).u.fld[2].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case PARITY:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case POPCOUNT:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case CTZ:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case CLZ:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case CLRSB:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case FFS:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case BSWAP:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SQRT:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case ABS:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UNSIGNED_SAT_FRACT:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SAT_FRACT:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UNSIGNED_FRACT_CONVERT:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case FRACT_CONVERT:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UNSIGNED_FIX:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UNSIGNED_FLOAT:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case FIX:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case FLOAT:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case FLOAT_TRUNCATE:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case FLOAT_EXTEND:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case TRUNCATE:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case ZERO_EXTEND:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SIGN_EXTEND:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UNLT:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UNLE:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UNGT:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UNGE:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UNEQ:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case ORDERED:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UNORDERED:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case LTU:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case LEU:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case GTU:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case GEU:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case LTGT:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case LT:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case LE:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case GT:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case GE:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case EQ:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case NE:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case POST_MODIFY:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case PRE_MODIFY:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case POST_INC:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case POST_DEC:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case PRE_INC:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case PRE_DEC:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UMAX:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UMIN:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SMAX:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SMIN:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case ROTATERT:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case LSHIFTRT:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case ASHIFTRT:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case ROTATE:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case ASHIFT:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case NOT:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case XOR:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case IOR:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case AND:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UMOD:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UDIV:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case MOD:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case US_DIV:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SS_DIV:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case DIV:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case US_MULT:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SS_MULT:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case MULT:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case NEG:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case MINUS:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case PLUS:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case COMPARE:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case IF_THEN_ELSE:
              gt_ggc_m_7rtx_def ((*x).u.fld[2].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case CC0:
              break;
            case SYMBOL_REF:
              switch ((int) (SYMBOL_REF_HAS_BLOCK_INFO_P (&(*x))))
                {
                case 1:
                  gt_ggc_m_12object_block ((*x).u.block_sym.block);
                  break;
                default:
                  break;
                }
              switch ((int) (CONSTANT_POOL_ADDRESS_P (&(*x))))
                {
                case 1:
                  gt_ggc_m_23constant_descriptor_rtx ((*x).u.fld[1].rt_constant);
                  break;
                default:
                  gt_ggc_m_9tree_node ((*x).u.fld[1].rt_tree);
                  break;
                }
              gt_ggc_m_S ((*x).u.fld[0].rt_str);
              break;
            case LABEL_REF:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case MEM:
              gt_ggc_m_9mem_attrs ((*x).u.fld[1].rt_mem);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case CONCATN:
              gt_ggc_m_9rtvec_def ((*x).u.fld[0].rt_rtvec);
              break;
            case CONCAT:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case STRICT_LOW_PART:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SUBREG:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SCRATCH:
              break;
            case REG:
              gt_ggc_m_9reg_attrs ((*x).u.reg.attrs);
              break;
            case PC:
              break;
            case CONST:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case CONST_STRING:
              gt_ggc_m_S ((*x).u.fld[0].rt_str);
              break;
            case CONST_VECTOR:
              gt_ggc_m_9rtvec_def ((*x).u.fld[0].rt_rtvec);
              break;
            case CONST_DOUBLE:
              break;
            case CONST_FIXED:
              break;
            case CONST_POLY_INT:
              break;
            case CONST_WIDE_INT:
              break;
            case CONST_INT:
              break;
            case TRAP_IF:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case EH_RETURN:
              break;
            case SIMPLE_RETURN:
              break;
            case RETURN:
              break;
            case CALL:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case CLOBBER:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case USE:
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SET:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case PREFETCH:
              gt_ggc_m_7rtx_def ((*x).u.fld[2].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case ADDR_DIFF_VEC:
              gt_ggc_m_7rtx_def ((*x).u.fld[3].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[2].rt_rtx);
              gt_ggc_m_9rtvec_def ((*x).u.fld[1].rt_rtvec);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case ADDR_VEC:
              gt_ggc_m_9rtvec_def ((*x).u.fld[0].rt_rtvec);
              break;
            case UNSPEC_VOLATILE:
              gt_ggc_m_9rtvec_def ((*x).u.fld[0].rt_rtvec);
              break;
            case UNSPEC:
              gt_ggc_m_9rtvec_def ((*x).u.fld[0].rt_rtvec);
              break;
            case ASM_OPERANDS:
              gt_ggc_m_9rtvec_def ((*x).u.fld[5].rt_rtvec);
              gt_ggc_m_9rtvec_def ((*x).u.fld[4].rt_rtvec);
              gt_ggc_m_9rtvec_def ((*x).u.fld[3].rt_rtvec);
              gt_ggc_m_S ((*x).u.fld[1].rt_str);
              gt_ggc_m_S ((*x).u.fld[0].rt_str);
              break;
            case ASM_INPUT:
              gt_ggc_m_S ((*x).u.fld[0].rt_str);
              break;
            case PARALLEL:
              gt_ggc_m_9rtvec_def ((*x).u.fld[0].rt_rtvec);
              break;
            case COND_EXEC:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case NOTE:
              switch ((int) (NOTE_KIND (&(*x))))
                {
                default:
                  gt_ggc_m_S ((*x).u.fld[3].rt_str);
                  break;
                case NOTE_INSN_UPDATE_SJLJ_CONTEXT:
                  break;
                case NOTE_INSN_CFI_LABEL:
                  break;
                case NOTE_INSN_CFI:
                  break;
                case NOTE_INSN_SWITCH_TEXT_SECTIONS:
                  break;
                case NOTE_INSN_BASIC_BLOCK:
                  break;
                case NOTE_INSN_INLINE_ENTRY:
                  break;
                case NOTE_INSN_BEGIN_STMT:
                  break;
                case NOTE_INSN_VAR_LOCATION:
                  gt_ggc_m_7rtx_def ((*x).u.fld[3].rt_rtx);
                  break;
                case NOTE_INSN_EH_REGION_END:
                  break;
                case NOTE_INSN_EH_REGION_BEG:
                  break;
                case NOTE_INSN_EPILOGUE_BEG:
                  break;
                case NOTE_INSN_PROLOGUE_END:
                  break;
                case NOTE_INSN_FUNCTION_BEG:
                  break;
                case NOTE_INSN_BLOCK_END:
                  gt_ggc_m_9tree_node ((*x).u.fld[3].rt_tree);
                  break;
                case NOTE_INSN_BLOCK_BEG:
                  gt_ggc_m_9tree_node ((*x).u.fld[3].rt_tree);
                  break;
                case NOTE_INSN_DELETED_DEBUG_LABEL:
                  gt_ggc_m_S ((*x).u.fld[3].rt_str);
                  break;
                case NOTE_INSN_DELETED_LABEL:
                  gt_ggc_m_S ((*x).u.fld[3].rt_str);
                  break;
                case NOTE_INSN_DELETED:
                  break;
                }
              gt_ggc_m_15basic_block_def ((*x).u.fld[2].rt_bb);
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case CODE_LABEL:
              gt_ggc_m_S ((*x).u.fld[6].rt_str);
              gt_ggc_m_7rtx_def ((*x).u.fld[3].rt_rtx);
              gt_ggc_m_15basic_block_def ((*x).u.fld[2].rt_bb);
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case BARRIER:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case JUMP_TABLE_DATA:
              gt_ggc_m_7rtx_def ((*x).u.fld[3].rt_rtx);
              gt_ggc_m_15basic_block_def ((*x).u.fld[2].rt_bb);
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case CALL_INSN:
              gt_ggc_m_7rtx_def ((*x).u.fld[7].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[6].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[3].rt_rtx);
              gt_ggc_m_15basic_block_def ((*x).u.fld[2].rt_bb);
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case JUMP_INSN:
              gt_ggc_m_7rtx_def ((*x).u.fld[7].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[6].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[3].rt_rtx);
              gt_ggc_m_15basic_block_def ((*x).u.fld[2].rt_bb);
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case INSN:
              gt_ggc_m_7rtx_def ((*x).u.fld[6].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[3].rt_rtx);
              gt_ggc_m_15basic_block_def ((*x).u.fld[2].rt_bb);
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case DEBUG_INSN:
              gt_ggc_m_7rtx_def ((*x).u.fld[6].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[3].rt_rtx);
              gt_ggc_m_15basic_block_def ((*x).u.fld[2].rt_bb);
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case ADDRESS:
              break;
            case SEQUENCE:
              gt_ggc_m_9rtvec_def ((*x).u.fld[0].rt_rtvec);
              break;
            case INT_LIST:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              break;
            case INSN_LIST:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case EXPR_LIST:
              gt_ggc_m_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_ggc_m_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case DEBUG_EXPR:
              gt_ggc_m_9tree_node ((*x).u.fld[0].rt_tree);
              break;
            case VALUE:
              break;
            case UNKNOWN:
              break;
            default:
              break;
            }
          break;
        /* Unrecognized tag value.  */
        default: gcc_unreachable (); 
        }
      x = (RTX_NEXT (&(*x)));
    }
}

void
gt_ggc_mx_rtvec_def (void *x_p)
{
  struct rtvec_def * const x = (struct rtvec_def *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      {
        size_t l0 = (size_t)(((*x)).num_elem);
        {
          size_t i0;
          for (i0 = 0; i0 != l0; i0++) {
            gt_ggc_m_7rtx_def ((*x).elem[i0]);
          }
        }
      }
    }
}

void
gt_ggc_mx_gimple (void *x_p)
{
  struct gimple * x = (struct gimple *)x_p;
  struct gimple * xlimit = x;
  while (ggc_test_and_set_mark (xlimit))
   xlimit = ((*xlimit).next);
  while (x != xlimit)
    {
      switch ((int) (gimple_statement_structure (&((*x)))))
        {
        case GSS_BASE:
          gt_ggc_m_15basic_block_def ((*x).bb);
          gt_ggc_m_6gimple ((*x).next);
          break;
        case GSS_WCE:
          {
            gimple_statement_wce *sub = static_cast <gimple_statement_wce *> (x);
            gt_ggc_m_6gimple ((*sub).cleanup);
            gt_ggc_m_15basic_block_def ((*sub).bb);
            gt_ggc_m_6gimple ((*sub).next);
          }
          break;
        case GSS_OMP:
          {
            gimple_statement_omp *sub = static_cast <gimple_statement_omp *> (x);
            gt_ggc_m_6gimple ((*sub).body);
            gt_ggc_m_15basic_block_def ((*sub).bb);
            gt_ggc_m_6gimple ((*sub).next);
          }
          break;
        case GSS_OMP_SECTIONS:
          {
            gomp_sections *sub = static_cast <gomp_sections *> (x);
            gt_ggc_m_9tree_node ((*sub).clauses);
            gt_ggc_m_9tree_node ((*sub).control);
            gt_ggc_m_6gimple ((*sub).body);
            gt_ggc_m_15basic_block_def ((*sub).bb);
            gt_ggc_m_6gimple ((*sub).next);
          }
          break;
        case GSS_OMP_PARALLEL_LAYOUT:
          {
            gimple_statement_omp_parallel_layout *sub = static_cast <gimple_statement_omp_parallel_layout *> (x);
            gt_ggc_m_9tree_node ((*sub).clauses);
            gt_ggc_m_9tree_node ((*sub).child_fn);
            gt_ggc_m_9tree_node ((*sub).data_arg);
            gt_ggc_m_6gimple ((*sub).body);
            gt_ggc_m_15basic_block_def ((*sub).bb);
            gt_ggc_m_6gimple ((*sub).next);
          }
          break;
        case GSS_OMP_TASK:
          {
            gomp_task *sub = static_cast <gomp_task *> (x);
            gt_ggc_m_9tree_node ((*sub).copy_fn);
            gt_ggc_m_9tree_node ((*sub).arg_size);
            gt_ggc_m_9tree_node ((*sub).arg_align);
            gt_ggc_m_9tree_node ((*sub).clauses);
            gt_ggc_m_9tree_node ((*sub).child_fn);
            gt_ggc_m_9tree_node ((*sub).data_arg);
            gt_ggc_m_6gimple ((*sub).body);
            gt_ggc_m_15basic_block_def ((*sub).bb);
            gt_ggc_m_6gimple ((*sub).next);
          }
          break;
        case GSS_OMP_FOR:
          {
            gomp_for *sub = static_cast <gomp_for *> (x);
            {
              size_t l0 = (size_t)(((*sub)).collapse);
              gt_ggc_m_9tree_node ((*sub).clauses);
              if ((*sub).iter != NULL) {
                size_t i0;
                for (i0 = 0; i0 != (size_t)(l0); i0++) {
                  gt_ggc_m_9tree_node ((*sub).iter[i0].index);
                  gt_ggc_m_9tree_node ((*sub).iter[i0].initial);
                  gt_ggc_m_9tree_node ((*sub).iter[i0].final);
                  gt_ggc_m_9tree_node ((*sub).iter[i0].incr);
                }
                ggc_mark ((*sub).iter);
              }
              gt_ggc_m_6gimple ((*sub).pre_body);
              gt_ggc_m_6gimple ((*sub).body);
              gt_ggc_m_15basic_block_def ((*sub).bb);
              gt_ggc_m_6gimple ((*sub).next);
            }
          }
          break;
        case GSS_OMP_SINGLE_LAYOUT:
          {
            gimple_statement_omp_single_layout *sub = static_cast <gimple_statement_omp_single_layout *> (x);
            gt_ggc_m_9tree_node ((*sub).clauses);
            gt_ggc_m_6gimple ((*sub).body);
            gt_ggc_m_15basic_block_def ((*sub).bb);
            gt_ggc_m_6gimple ((*sub).next);
          }
          break;
        case GSS_OMP_CRITICAL:
          {
            gomp_critical *sub = static_cast <gomp_critical *> (x);
            gt_ggc_m_9tree_node ((*sub).clauses);
            gt_ggc_m_9tree_node ((*sub).name);
            gt_ggc_m_6gimple ((*sub).body);
            gt_ggc_m_15basic_block_def ((*sub).bb);
            gt_ggc_m_6gimple ((*sub).next);
          }
          break;
        case GSS_OMP_CONTINUE:
          {
            gomp_continue *sub = static_cast <gomp_continue *> (x);
            gt_ggc_m_9tree_node ((*sub).control_def);
            gt_ggc_m_9tree_node ((*sub).control_use);
            gt_ggc_m_15basic_block_def ((*sub).bb);
            gt_ggc_m_6gimple ((*sub).next);
          }
          break;
        case GSS_OMP_ATOMIC_STORE_LAYOUT:
          {
            gimple_statement_omp_atomic_store_layout *sub = static_cast <gimple_statement_omp_atomic_store_layout *> (x);
            gt_ggc_m_9tree_node ((*sub).val);
            gt_ggc_m_15basic_block_def ((*sub).bb);
            gt_ggc_m_6gimple ((*sub).next);
          }
          break;
        case GSS_OMP_ATOMIC_LOAD:
          {
            gomp_atomic_load *sub = static_cast <gomp_atomic_load *> (x);
            gt_ggc_m_9tree_node ((*sub).rhs);
            gt_ggc_m_9tree_node ((*sub).lhs);
            gt_ggc_m_15basic_block_def ((*sub).bb);
            gt_ggc_m_6gimple ((*sub).next);
          }
          break;
        case GSS_TRY:
          {
            gtry *sub = static_cast <gtry *> (x);
            gt_ggc_m_6gimple ((*sub).eval);
            gt_ggc_m_6gimple ((*sub).cleanup);
            gt_ggc_m_15basic_block_def ((*sub).bb);
            gt_ggc_m_6gimple ((*sub).next);
          }
          break;
        case GSS_PHI:
          {
            gphi *sub = static_cast <gphi *> (x);
            {
              size_t l1 = (size_t)(((*sub)).nargs);
              gt_ggc_m_9tree_node ((*sub).result);
              {
                size_t i1;
                for (i1 = 0; i1 != l1; i1++) {
                  gt_ggc_m_9tree_node ((*sub).args[i1].def);
                }
              }
              gt_ggc_m_15basic_block_def ((*sub).bb);
              gt_ggc_m_6gimple ((*sub).next);
            }
          }
          break;
        case GSS_EH_CTRL:
          {
            gimple_statement_eh_ctrl *sub = static_cast <gimple_statement_eh_ctrl *> (x);
            gt_ggc_m_15basic_block_def ((*sub).bb);
            gt_ggc_m_6gimple ((*sub).next);
          }
          break;
        case GSS_EH_ELSE:
          {
            geh_else *sub = static_cast <geh_else *> (x);
            gt_ggc_m_6gimple ((*sub).n_body);
            gt_ggc_m_6gimple ((*sub).e_body);
            gt_ggc_m_15basic_block_def ((*sub).bb);
            gt_ggc_m_6gimple ((*sub).next);
          }
          break;
        case GSS_EH_MNT:
          {
            geh_mnt *sub = static_cast <geh_mnt *> (x);
            gt_ggc_m_9tree_node ((*sub).fndecl);
            gt_ggc_m_15basic_block_def ((*sub).bb);
            gt_ggc_m_6gimple ((*sub).next);
          }
          break;
        case GSS_EH_FILTER:
          {
            geh_filter *sub = static_cast <geh_filter *> (x);
            gt_ggc_m_9tree_node ((*sub).types);
            gt_ggc_m_6gimple ((*sub).failure);
            gt_ggc_m_15basic_block_def ((*sub).bb);
            gt_ggc_m_6gimple ((*sub).next);
          }
          break;
        case GSS_CATCH:
          {
            gcatch *sub = static_cast <gcatch *> (x);
            gt_ggc_m_9tree_node ((*sub).types);
            gt_ggc_m_6gimple ((*sub).handler);
            gt_ggc_m_15basic_block_def ((*sub).bb);
            gt_ggc_m_6gimple ((*sub).next);
          }
          break;
        case GSS_BIND:
          {
            gbind *sub = static_cast <gbind *> (x);
            gt_ggc_m_9tree_node ((*sub).vars);
            gt_ggc_m_9tree_node ((*sub).block);
            gt_ggc_m_6gimple ((*sub).body);
            gt_ggc_m_15basic_block_def ((*sub).bb);
            gt_ggc_m_6gimple ((*sub).next);
          }
          break;
        case GSS_WITH_MEM_OPS_BASE:
          {
            gimple_statement_with_memory_ops_base *sub = static_cast <gimple_statement_with_memory_ops_base *> (x);
            gt_ggc_m_15basic_block_def ((*sub).bb);
            gt_ggc_m_6gimple ((*sub).next);
          }
          break;
        case GSS_TRANSACTION:
          {
            gtransaction *sub = static_cast <gtransaction *> (x);
            gt_ggc_m_6gimple ((*sub).body);
            gt_ggc_m_9tree_node ((*sub).label_norm);
            gt_ggc_m_9tree_node ((*sub).label_uninst);
            gt_ggc_m_9tree_node ((*sub).label_over);
            gt_ggc_m_15basic_block_def ((*sub).bb);
            gt_ggc_m_6gimple ((*sub).next);
          }
          break;
        case GSS_CALL:
          {
            gcall *sub = static_cast <gcall *> (x);
            {
              size_t l2 = (size_t)(((*sub)).num_ops);
              gt_ggc_m_11bitmap_head ((*sub).call_used.vars);
              gt_ggc_m_11bitmap_head ((*sub).call_clobbered.vars);
              switch ((int) (((*sub)).subcode & GF_CALL_INTERNAL))
                {
                case 0:
                  gt_ggc_m_9tree_node ((*sub).u.fntype);
                  break;
                case GF_CALL_INTERNAL:
                  break;
                default:
                  break;
                }
              {
                size_t i2;
                for (i2 = 0; i2 != l2; i2++) {
                  gt_ggc_m_9tree_node ((*sub).op[i2]);
                }
              }
              gt_ggc_m_15basic_block_def ((*sub).bb);
              gt_ggc_m_6gimple ((*sub).next);
            }
          }
          break;
        case GSS_ASM:
          {
            gasm *sub = static_cast <gasm *> (x);
            {
              size_t l3 = (size_t)(((*sub)).num_ops);
              gt_ggc_m_S ((*sub).string);
              {
                size_t i3;
                for (i3 = 0; i3 != l3; i3++) {
                  gt_ggc_m_9tree_node ((*sub).op[i3]);
                }
              }
              gt_ggc_m_15basic_block_def ((*sub).bb);
              gt_ggc_m_6gimple ((*sub).next);
            }
          }
          break;
        case GSS_WITH_MEM_OPS:
          {
            gimple_statement_with_memory_ops *sub = static_cast <gimple_statement_with_memory_ops *> (x);
            {
              size_t l4 = (size_t)(((*sub)).num_ops);
              {
                size_t i4;
                for (i4 = 0; i4 != l4; i4++) {
                  gt_ggc_m_9tree_node ((*sub).op[i4]);
                }
              }
              gt_ggc_m_15basic_block_def ((*sub).bb);
              gt_ggc_m_6gimple ((*sub).next);
            }
          }
          break;
        case GSS_WITH_OPS:
          {
            gimple_statement_with_ops *sub = static_cast <gimple_statement_with_ops *> (x);
            {
              size_t l5 = (size_t)(((*sub)).num_ops);
              {
                size_t i5;
                for (i5 = 0; i5 != l5; i5++) {
                  gt_ggc_m_9tree_node ((*sub).op[i5]);
                }
              }
              gt_ggc_m_15basic_block_def ((*sub).bb);
              gt_ggc_m_6gimple ((*sub).next);
            }
          }
          break;
        /* Unrecognized tag value.  */
        default: gcc_unreachable (); 
        }
      x = ((*x).next);
    }
}

void
gt_ggc_mx_symtab_node (void *x_p)
{
  struct symtab_node * x = (struct symtab_node *)x_p;
  struct symtab_node * xlimit = x;
  while (ggc_test_and_set_mark (xlimit))
   xlimit = ((*xlimit).next);
  if (x != xlimit)
    for (;;)
      {
        struct symtab_node * const xprev = ((*x).previous);
        if (xprev == NULL) break;
        x = xprev;
        (void) ggc_test_and_set_mark (xprev);
      }
  while (x != xlimit)
    {
      switch ((int) (((*x)).type))
        {
        case SYMTAB_SYMBOL:
          gt_ggc_m_9tree_node ((*x).decl);
          gt_ggc_m_11symtab_node ((*x).next);
          gt_ggc_m_11symtab_node ((*x).previous);
          gt_ggc_m_11symtab_node ((*x).next_sharing_asm_name);
          gt_ggc_m_11symtab_node ((*x).previous_sharing_asm_name);
          gt_ggc_m_11symtab_node ((*x).same_comdat_group);
          gt_ggc_m_20vec_ipa_ref_t_va_gc_ ((*x).ref_list.references);
          gt_ggc_m_9tree_node ((*x).alias_target);
          gt_ggc_m_18lto_file_decl_data ((*x).lto_file_data);
          gt_ggc_m_9tree_node ((*x).x_comdat_group);
          gt_ggc_m_18section_hash_entry ((*x).x_section);
          break;
        case SYMTAB_VARIABLE:
          {
            varpool_node *sub = static_cast <varpool_node *> (x);
            gt_ggc_m_9tree_node ((*sub).decl);
            gt_ggc_m_11symtab_node ((*sub).next);
            gt_ggc_m_11symtab_node ((*sub).previous);
            gt_ggc_m_11symtab_node ((*sub).next_sharing_asm_name);
            gt_ggc_m_11symtab_node ((*sub).previous_sharing_asm_name);
            gt_ggc_m_11symtab_node ((*sub).same_comdat_group);
            gt_ggc_m_20vec_ipa_ref_t_va_gc_ ((*sub).ref_list.references);
            gt_ggc_m_9tree_node ((*sub).alias_target);
            gt_ggc_m_18lto_file_decl_data ((*sub).lto_file_data);
            gt_ggc_m_9tree_node ((*sub).x_comdat_group);
            gt_ggc_m_18section_hash_entry ((*sub).x_section);
          }
          break;
        case SYMTAB_FUNCTION:
          {
            cgraph_node *sub = static_cast <cgraph_node *> (x);
            gt_ggc_m_11cgraph_edge ((*sub).callees);
            gt_ggc_m_11cgraph_edge ((*sub).callers);
            gt_ggc_m_11cgraph_edge ((*sub).indirect_calls);
            gt_ggc_m_11symtab_node ((*sub).origin);
            gt_ggc_m_11symtab_node ((*sub).nested);
            gt_ggc_m_11symtab_node ((*sub).next_nested);
            gt_ggc_m_11symtab_node ((*sub).next_sibling_clone);
            gt_ggc_m_11symtab_node ((*sub).prev_sibling_clone);
            gt_ggc_m_11symtab_node ((*sub).clones);
            gt_ggc_m_11symtab_node ((*sub).clone_of);
            gt_ggc_m_30hash_table_cgraph_edge_hasher_ ((*sub).call_site_hash);
            gt_ggc_m_9tree_node ((*sub).former_clone_of);
            gt_ggc_m_17cgraph_simd_clone ((*sub).simdclone);
            gt_ggc_m_11symtab_node ((*sub).simd_clones);
            gt_ggc_m_11symtab_node ((*sub).inlined_to);
            gt_ggc_m_15cgraph_rtl_info ((*sub).rtl);
            gt_ggc_m_27vec_ipa_replace_map__va_gc_ ((*sub).clone.tree_map);
            gt_ggc_m_21ipa_param_adjustments ((*sub).clone.param_adjustments);
            gt_ggc_m_36vec_ipa_param_performed_split_va_gc_ ((*sub).clone.performed_splits);
            gt_ggc_m_9tree_node ((*sub).thunk.alias);
            gt_ggc_m_9tree_node ((*sub).decl);
            gt_ggc_m_11symtab_node ((*sub).next);
            gt_ggc_m_11symtab_node ((*sub).previous);
            gt_ggc_m_11symtab_node ((*sub).next_sharing_asm_name);
            gt_ggc_m_11symtab_node ((*sub).previous_sharing_asm_name);
            gt_ggc_m_11symtab_node ((*sub).same_comdat_group);
            gt_ggc_m_20vec_ipa_ref_t_va_gc_ ((*sub).ref_list.references);
            gt_ggc_m_9tree_node ((*sub).alias_target);
            gt_ggc_m_18lto_file_decl_data ((*sub).lto_file_data);
            gt_ggc_m_9tree_node ((*sub).x_comdat_group);
            gt_ggc_m_18section_hash_entry ((*sub).x_section);
          }
          break;
        /* Unrecognized tag value.  */
        default: gcc_unreachable (); 
        }
      x = ((*x).next);
    }
}

void
gt_ggc_mx_cgraph_edge (void *x_p)
{
  struct cgraph_edge * x = (struct cgraph_edge *)x_p;
  struct cgraph_edge * xlimit = x;
  while (ggc_test_and_set_mark (xlimit))
   xlimit = ((*xlimit).next_caller);
  if (x != xlimit)
    for (;;)
      {
        struct cgraph_edge * const xprev = ((*x).prev_caller);
        if (xprev == NULL) break;
        x = xprev;
        (void) ggc_test_and_set_mark (xprev);
      }
  while (x != xlimit)
    {
      gt_ggc_m_11symtab_node ((*x).caller);
      gt_ggc_m_11symtab_node ((*x).callee);
      gt_ggc_m_11cgraph_edge ((*x).prev_caller);
      gt_ggc_m_11cgraph_edge ((*x).next_caller);
      gt_ggc_m_11cgraph_edge ((*x).prev_callee);
      gt_ggc_m_11cgraph_edge ((*x).next_callee);
      gt_ggc_m_6gimple ((*x).call_stmt);
      gt_ggc_m_25cgraph_indirect_call_info ((*x).indirect_info);
      x = ((*x).next_caller);
    }
}

void
gt_ggc_mx (struct cgraph_edge& x_r ATTRIBUTE_UNUSED)
{
  struct cgraph_edge * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_11symtab_node ((*x).caller);
  gt_ggc_m_11symtab_node ((*x).callee);
  gt_ggc_m_11cgraph_edge ((*x).prev_caller);
  gt_ggc_m_11cgraph_edge ((*x).next_caller);
  gt_ggc_m_11cgraph_edge ((*x).prev_callee);
  gt_ggc_m_11cgraph_edge ((*x).next_callee);
  gt_ggc_m_6gimple ((*x).call_stmt);
  gt_ggc_m_25cgraph_indirect_call_info ((*x).indirect_info);
}

void
gt_ggc_mx (struct cgraph_edge *& x)
{
  if (x)
    gt_ggc_mx_cgraph_edge ((void *) x);
}

void
gt_ggc_mx_section (void *x_p)
{
  union section * const x = (union section *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      switch ((int) (SECTION_STYLE (&(((*x))))))
        {
        case SECTION_NAMED:
          gt_ggc_m_S ((*x).named.name);
          gt_ggc_m_9tree_node ((*x).named.decl);
          break;
        case SECTION_UNNAMED:
          gt_ggc_m_7section ((*x).unnamed.next);
          break;
        case SECTION_NOSWITCH:
          break;
        default:
          break;
        }
    }
}

void
gt_ggc_mx (union section& x_r ATTRIBUTE_UNUSED)
{
  union section * ATTRIBUTE_UNUSED x = &x_r;
  switch ((int) (SECTION_STYLE (&(((*x))))))
    {
    case SECTION_NAMED:
      gt_ggc_m_S ((*x).named.name);
      gt_ggc_m_9tree_node ((*x).named.decl);
      break;
    case SECTION_UNNAMED:
      gt_ggc_m_7section ((*x).unnamed.next);
      break;
    case SECTION_NOSWITCH:
      break;
    default:
      break;
    }
}

void
gt_ggc_mx (union section *& x)
{
  if (x)
    gt_ggc_mx_section ((void *) x);
}

void
gt_ggc_mx_cl_target_option (void *x_p)
{
  struct cl_target_option * const x = (struct cl_target_option *)x_p;
  if (ggc_test_and_set_mark (x))
    {
    }
}

void
gt_ggc_mx_cl_optimization (void *x_p)
{
  struct cl_optimization * const x = (struct cl_optimization *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_S ((*x).x_str_align_functions);
      gt_ggc_m_S ((*x).x_str_align_jumps);
      gt_ggc_m_S ((*x).x_str_align_labels);
      gt_ggc_m_S ((*x).x_str_align_loops);
    }
}

void
gt_ggc_mx_edge_def (void *x_p)
{
  edge_def * const x = (edge_def *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx_basic_block_def (void *x_p)
{
  struct basic_block_def * x = (struct basic_block_def *)x_p;
  struct basic_block_def * xlimit = x;
  while (ggc_test_and_set_mark (xlimit))
   xlimit = ((*xlimit).next_bb);
  if (x != xlimit)
    for (;;)
      {
        struct basic_block_def * const xprev = ((*x).prev_bb);
        if (xprev == NULL) break;
        x = xprev;
        (void) ggc_test_and_set_mark (xprev);
      }
  while (x != xlimit)
    {
      gt_ggc_m_15vec_edge_va_gc_ ((*x).preds);
      gt_ggc_m_15vec_edge_va_gc_ ((*x).succs);
      gt_ggc_m_4loop ((*x).loop_father);
      gt_ggc_m_15basic_block_def ((*x).prev_bb);
      gt_ggc_m_15basic_block_def ((*x).next_bb);
      switch ((int) (((((*x)).flags & BB_RTL) != 0)))
        {
        case 0:
          gt_ggc_m_6gimple ((*x).il.gimple.seq);
          gt_ggc_m_6gimple ((*x).il.gimple.phi_nodes);
          break;
        case 1:
          gt_ggc_m_7rtx_def ((*x).il.x.head_);
          gt_ggc_m_11rtl_bb_info ((*x).il.x.rtl);
          break;
        default:
          break;
        }
      x = ((*x).next_bb);
    }
}

void
gt_ggc_mx_bitmap_element (void *x_p)
{
  struct bitmap_element * x = (struct bitmap_element *)x_p;
  struct bitmap_element * xlimit = x;
  while (ggc_test_and_set_mark (xlimit))
   xlimit = ((*xlimit).next);
  while (x != xlimit)
    {
      gt_ggc_m_14bitmap_element ((*x).next);
      gt_ggc_m_14bitmap_element ((*x).prev);
      x = ((*x).next);
    }
}

void
gt_ggc_mx_generic_wide_int_wide_int_storage_ (void *x_p)
{
  generic_wide_int<wide_int_storage> * const x = (generic_wide_int<wide_int_storage> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct wide_int_storage& x_r ATTRIBUTE_UNUSED)
{
  struct wide_int_storage * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_mem_attrs (void *x_p)
{
  struct mem_attrs * const x = (struct mem_attrs *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).expr);
    }
}

void
gt_ggc_mx_reg_attrs (void *x_p)
{
  struct reg_attrs * const x = (struct reg_attrs *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).decl);
    }
}

void
gt_ggc_mx (struct reg_attrs& x_r ATTRIBUTE_UNUSED)
{
  struct reg_attrs * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).decl);
}

void
gt_ggc_mx (struct reg_attrs *& x)
{
  if (x)
    gt_ggc_mx_reg_attrs ((void *) x);
}

void
gt_ggc_mx_object_block (void *x_p)
{
  struct object_block * const x = (struct object_block *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_7section ((*x).sect);
      gt_ggc_m_14vec_rtx_va_gc_ ((*x).objects);
      gt_ggc_m_14vec_rtx_va_gc_ ((*x).anchors);
    }
}

void
gt_ggc_mx (struct object_block& x_r ATTRIBUTE_UNUSED)
{
  struct object_block * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_7section ((*x).sect);
  gt_ggc_m_14vec_rtx_va_gc_ ((*x).objects);
  gt_ggc_m_14vec_rtx_va_gc_ ((*x).anchors);
}

void
gt_ggc_mx (struct object_block *& x)
{
  if (x)
    gt_ggc_mx_object_block ((void *) x);
}

void
gt_ggc_mx_vec_rtx_va_gc_ (void *x_p)
{
  vec<rtx,va_gc> * const x = (vec<rtx,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct rtx_def *& x)
{
  if (x)
    gt_ggc_mx_rtx_def ((void *) x);
}

void
gt_ggc_mx_real_value (void *x_p)
{
  struct real_value * const x = (struct real_value *)x_p;
  if (ggc_test_and_set_mark (x))
    {
    }
}

void
gt_ggc_mx_fixed_value (void *x_p)
{
  struct fixed_value * const x = (struct fixed_value *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (&((*x).mode));
    }
}

void
gt_ggc_mx_function (void *x_p)
{
  struct function * const x = (struct function *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9eh_status ((*x).eh);
      gt_ggc_m_18control_flow_graph ((*x).cfg);
      gt_ggc_m_6gimple ((*x).gimple_body);
      gt_ggc_m_9gimple_df ((*x).gimple_df);
      gt_ggc_m_5loops ((*x).x_current_loops);
      gt_ggc_m_S ((*x).pass_startwith);
      gt_ggc_m_11stack_usage ((*x).su);
      gt_ggc_m_9tree_node ((*x).decl);
      gt_ggc_m_9tree_node ((*x).static_chain_decl);
      gt_ggc_m_9tree_node ((*x).nonlocal_goto_save_area);
      gt_ggc_m_15vec_tree_va_gc_ ((*x).local_decls);
      gcc_assert (!(*x).machine);
      gt_ggc_m_17language_function ((*x).language);
      gt_ggc_m_14hash_set_tree_ ((*x).used_types_hash);
      gt_ggc_m_11dw_fde_node ((*x).fde);
    }
}

void
gt_ggc_mx_target_rtl (void *x_p)
{
  struct target_rtl * const x = (struct target_rtl *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      {
        size_t i0;
        size_t l0 = (size_t)(GR_MAX);
        for (i0 = 0; i0 != l0; i0++) {
          gt_ggc_m_7rtx_def ((*x).x_global_rtl[i0]);
        }
      }
      gt_ggc_m_7rtx_def ((*x).x_pic_offset_table_rtx);
      gt_ggc_m_7rtx_def ((*x).x_return_address_pointer_rtx);
      {
        size_t i1;
        size_t l1 = (size_t)(FIRST_PSEUDO_REGISTER);
        for (i1 = 0; i1 != l1; i1++) {
          gt_ggc_m_7rtx_def ((*x).x_initial_regno_reg_rtx[i1]);
        }
      }
      {
        size_t i2;
        size_t l2 = (size_t)(MAX_MACHINE_MODE);
        for (i2 = 0; i2 != l2; i2++) {
          gt_ggc_m_7rtx_def ((*x).x_top_of_stack[i2]);
        }
      }
      {
        size_t i3;
        size_t l3 = (size_t)(FIRST_PSEUDO_REGISTER);
        for (i3 = 0; i3 != l3; i3++) {
          gt_ggc_m_7rtx_def ((*x).x_static_reg_base_value[i3]);
        }
      }
      {
        size_t i4;
        size_t l4 = (size_t)((int) MAX_MACHINE_MODE);
        for (i4 = 0; i4 != l4; i4++) {
          gt_ggc_m_9mem_attrs ((*x).x_mode_mem_attrs[i4]);
        }
      }
    }
}

void
gt_ggc_mx_cgraph_rtl_info (void *x_p)
{
  struct cgraph_rtl_info * const x = (struct cgraph_rtl_info *)x_p;
  if (ggc_test_and_set_mark (x))
    {
    }
}

void
gt_ggc_mx_hash_map_tree_tree_decl_tree_cache_traits_ (void *x_p)
{
  hash_map<tree,tree,decl_tree_cache_traits> * const x = (hash_map<tree,tree,decl_tree_cache_traits> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct decl_tree_cache_traits& x_r ATTRIBUTE_UNUSED)
{
  struct decl_tree_cache_traits * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx (union tree_node *& x)
{
  if (x)
    gt_ggc_mx_lang_tree_node ((void *) x);
}

void
gt_ggc_mx_hash_map_tree_tree_type_tree_cache_traits_ (void *x_p)
{
  hash_map<tree,tree,type_tree_cache_traits> * const x = (hash_map<tree,tree,type_tree_cache_traits> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct type_tree_cache_traits& x_r ATTRIBUTE_UNUSED)
{
  struct type_tree_cache_traits * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_hash_map_tree_tree_decl_tree_traits_ (void *x_p)
{
  hash_map<tree,tree,decl_tree_traits> * const x = (hash_map<tree,tree,decl_tree_traits> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct decl_tree_traits& x_r ATTRIBUTE_UNUSED)
{
  struct decl_tree_traits * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_ptr_info_def (void *x_p)
{
  struct ptr_info_def * const x = (struct ptr_info_def *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_11bitmap_head ((*x).pt.vars);
    }
}

void
gt_ggc_mx_range_info_def (void *x_p)
{
  struct range_info_def * const x = (struct range_info_def *)x_p;
  if (ggc_test_and_set_mark (x))
    {
    }
}

void
gt_ggc_mx_vec_constructor_elt_va_gc_ (void *x_p)
{
  vec<constructor_elt,va_gc> * const x = (vec<constructor_elt,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct constructor_elt& x_r ATTRIBUTE_UNUSED)
{
  struct constructor_elt * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).index);
  gt_ggc_m_9tree_node ((*x).value);
}

void
gt_ggc_mx_vec_tree_va_gc_ (void *x_p)
{
  vec<tree,va_gc> * const x = (vec<tree,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx_tree_statement_list_node (void *x_p)
{
  struct tree_statement_list_node * x = (struct tree_statement_list_node *)x_p;
  struct tree_statement_list_node * xlimit = x;
  while (ggc_test_and_set_mark (xlimit))
   xlimit = ((*xlimit).next);
  if (x != xlimit)
    for (;;)
      {
        struct tree_statement_list_node * const xprev = ((*x).prev);
        if (xprev == NULL) break;
        x = xprev;
        (void) ggc_test_and_set_mark (xprev);
      }
  while (x != xlimit)
    {
      gt_ggc_m_24tree_statement_list_node ((*x).prev);
      gt_ggc_m_24tree_statement_list_node ((*x).next);
      gt_ggc_m_9tree_node ((*x).stmt);
      x = ((*x).next);
    }
}

void
gt_ggc_mx_target_globals (void *x_p)
{
  struct target_globals * const x = (struct target_globals *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_10target_rtl ((*x).rtl);
      gt_ggc_m_15target_libfuncs ((*x).libfuncs);
    }
}

void
gt_ggc_mx_tree_map (void *x_p)
{
  struct tree_map * const x = (struct tree_map *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).base.from);
      gt_ggc_m_9tree_node ((*x).to);
    }
}

void
gt_ggc_mx (struct tree_map& x_r ATTRIBUTE_UNUSED)
{
  struct tree_map * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).base.from);
  gt_ggc_m_9tree_node ((*x).to);
}

void
gt_ggc_mx (struct tree_map *& x)
{
  if (x)
    gt_ggc_mx_tree_map ((void *) x);
}

void
gt_ggc_mx_tree_decl_map (void *x_p)
{
  struct tree_decl_map * const x = (struct tree_decl_map *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).base.from);
      gt_ggc_m_9tree_node ((*x).to);
    }
}

void
gt_ggc_mx (struct tree_decl_map& x_r ATTRIBUTE_UNUSED)
{
  struct tree_decl_map * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).base.from);
  gt_ggc_m_9tree_node ((*x).to);
}

void
gt_ggc_mx (struct tree_decl_map *& x)
{
  if (x)
    gt_ggc_mx_tree_decl_map ((void *) x);
}

void
gt_ggc_mx_tree_int_map (void *x_p)
{
  struct tree_int_map * const x = (struct tree_int_map *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).base.from);
    }
}

void
gt_ggc_mx (struct tree_int_map& x_r ATTRIBUTE_UNUSED)
{
  struct tree_int_map * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).base.from);
}

void
gt_ggc_mx (struct tree_int_map *& x)
{
  if (x)
    gt_ggc_mx_tree_int_map ((void *) x);
}

void
gt_ggc_mx_tree_vec_map (void *x_p)
{
  struct tree_vec_map * const x = (struct tree_vec_map *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).base.from);
      gt_ggc_m_15vec_tree_va_gc_ ((*x).to);
    }
}

void
gt_ggc_mx (struct tree_vec_map& x_r ATTRIBUTE_UNUSED)
{
  struct tree_vec_map * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).base.from);
  gt_ggc_m_15vec_tree_va_gc_ ((*x).to);
}

void
gt_ggc_mx (struct tree_vec_map *& x)
{
  if (x)
    gt_ggc_mx_tree_vec_map ((void *) x);
}

void
gt_ggc_mx_vec_alias_pair_va_gc_ (void *x_p)
{
  vec<alias_pair,va_gc> * const x = (vec<alias_pair,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct alias_pair& x_r ATTRIBUTE_UNUSED)
{
  struct alias_pair * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).decl);
  gt_ggc_m_9tree_node ((*x).target);
}

void
gt_ggc_mx_libfunc_entry (void *x_p)
{
  struct libfunc_entry * const x = (struct libfunc_entry *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_7rtx_def ((*x).libfunc);
    }
}

void
gt_ggc_mx (struct libfunc_entry& x_r ATTRIBUTE_UNUSED)
{
  struct libfunc_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_7rtx_def ((*x).libfunc);
}

void
gt_ggc_mx (struct libfunc_entry *& x)
{
  if (x)
    gt_ggc_mx_libfunc_entry ((void *) x);
}

void
gt_ggc_mx_hash_table_libfunc_hasher_ (void *x_p)
{
  hash_table<libfunc_hasher> * const x = (hash_table<libfunc_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct libfunc_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct libfunc_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_target_libfuncs (void *x_p)
{
  struct target_libfuncs * const x = (struct target_libfuncs *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      {
        size_t i0;
        size_t l0 = (size_t)(LTI_MAX);
        for (i0 = 0; i0 != l0; i0++) {
          gt_ggc_m_7rtx_def ((*x).x_libfunc_table[i0]);
        }
      }
      gt_ggc_m_26hash_table_libfunc_hasher_ ((*x).x_libfunc_hash);
    }
}

void
gt_ggc_mx_sequence_stack (void *x_p)
{
  struct sequence_stack * const x = (struct sequence_stack *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_7rtx_def ((*x).first);
      gt_ggc_m_7rtx_def ((*x).last);
      gt_ggc_m_14sequence_stack ((*x).next);
    }
}

void
gt_ggc_mx_vec_rtx_insn__va_gc_ (void *x_p)
{
  vec<rtx_insn*,va_gc> * const x = (vec<rtx_insn*,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct rtx_insn *& x)
{
  if (x)
    gt_ggc_mx_rtx_def ((void *) x);
}

void
gt_ggc_mx_vec_uchar_va_gc_ (void *x_p)
{
  vec<uchar,va_gc> * const x = (vec<uchar,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx_vec_call_site_record_va_gc_ (void *x_p)
{
  vec<call_site_record,va_gc> * const x = (vec<call_site_record,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct call_site_record_d *& x)
{
  if (x)
    gt_ggc_mx_call_site_record_d ((void *) x);
}

void
gt_ggc_mx_gimple_df (void *x_p)
{
  struct gimple_df * const x = (struct gimple_df *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_15vec_tree_va_gc_ ((*x).ssa_names);
      gt_ggc_m_9tree_node ((*x).vop);
      gt_ggc_m_11bitmap_head ((*x).escaped.vars);
      gt_ggc_m_15vec_tree_va_gc_ ((*x).free_ssanames);
      gt_ggc_m_15vec_tree_va_gc_ ((*x).free_ssanames_queue);
      gt_ggc_m_27hash_table_ssa_name_hasher_ ((*x).default_defs);
      gt_ggc_m_20ssa_operand_memory_d ((*x).ssa_operands.operand_memory);
      gt_ggc_m_29hash_table_tm_restart_hasher_ ((*x).tm_restart);
    }
}

void
gt_ggc_mx_dw_fde_node (void *x_p)
{
  struct dw_fde_node * const x = (struct dw_fde_node *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).decl);
      gt_ggc_m_S ((*x).dw_fde_begin);
      gt_ggc_m_S ((*x).dw_fde_current_label);
      gt_ggc_m_S ((*x).dw_fde_end);
      gt_ggc_m_S ((*x).dw_fde_vms_end_prologue);
      gt_ggc_m_S ((*x).dw_fde_vms_begin_epilogue);
      gt_ggc_m_S ((*x).dw_fde_second_begin);
      gt_ggc_m_S ((*x).dw_fde_second_end);
      gt_ggc_m_21vec_dw_cfi_ref_va_gc_ ((*x).dw_fde_cfi);
    }
}

void
gt_ggc_mx_frame_space (void *x_p)
{
  struct frame_space * const x = (struct frame_space *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_11frame_space ((*x).next);
    }
}

void
gt_ggc_mx_vec_callinfo_callee_va_gc_ (void *x_p)
{
  vec<callinfo_callee,va_gc> * const x = (vec<callinfo_callee,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct callinfo_callee& x_r ATTRIBUTE_UNUSED)
{
  struct callinfo_callee * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).decl);
}

void
gt_ggc_mx_vec_callinfo_dalloc_va_gc_ (void *x_p)
{
  vec<callinfo_dalloc,va_gc> * const x = (vec<callinfo_dalloc,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct callinfo_dalloc& x_r ATTRIBUTE_UNUSED)
{
  struct callinfo_dalloc * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_S ((*x).name);
}

void
gt_ggc_mx_stack_usage (void *x_p)
{
  struct stack_usage * const x = (struct stack_usage *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_26vec_callinfo_callee_va_gc_ ((*x).callees);
      gt_ggc_m_26vec_callinfo_dalloc_va_gc_ ((*x).dallocs);
    }
}

void
gt_ggc_mx_eh_status (void *x_p)
{
  struct eh_status * const x = (struct eh_status *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_11eh_region_d ((*x).region_tree);
      gt_ggc_m_20vec_eh_region_va_gc_ ((*x).region_array);
      gt_ggc_m_25vec_eh_landing_pad_va_gc_ ((*x).lp_array);
      gt_ggc_m_21hash_map_gimple__int_ ((*x).throw_stmt_table);
      gt_ggc_m_15vec_tree_va_gc_ ((*x).ttype_data);
      switch ((int) (targetm.arm_eabi_unwinder))
        {
        case 1:
          gt_ggc_m_15vec_tree_va_gc_ ((*x).ehspec_data.arm_eabi);
          break;
        case 0:
          gt_ggc_m_16vec_uchar_va_gc_ ((*x).ehspec_data.other);
          break;
        default:
          break;
        }
    }
}

void
gt_ggc_mx_control_flow_graph (void *x_p)
{
  struct control_flow_graph * const x = (struct control_flow_graph *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_15basic_block_def ((*x).x_entry_block_ptr);
      gt_ggc_m_15basic_block_def ((*x).x_exit_block_ptr);
      gt_ggc_m_22vec_basic_block_va_gc_ ((*x).x_basic_block_info);
      gt_ggc_m_22vec_basic_block_va_gc_ ((*x).x_label_to_block_map);
    }
}

void
gt_ggc_mx_loops (void *x_p)
{
  struct loops * const x = (struct loops *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_17vec_loop_p_va_gc_ ((*x).larray);
      gt_ggc_m_28hash_table_loop_exit_hasher_ ((*x).exits);
      gt_ggc_m_4loop ((*x).tree_root);
    }
}

void
gt_ggc_mx_hash_set_tree_ (void *x_p)
{
  hash_set<tree> * const x = (hash_set<tree> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx_types_used_by_vars_entry (void *x_p)
{
  struct types_used_by_vars_entry * const x = (struct types_used_by_vars_entry *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).type);
      gt_ggc_m_9tree_node ((*x).var_decl);
    }
}

void
gt_ggc_mx (struct types_used_by_vars_entry& x_r ATTRIBUTE_UNUSED)
{
  struct types_used_by_vars_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).type);
  gt_ggc_m_9tree_node ((*x).var_decl);
}

void
gt_ggc_mx (struct types_used_by_vars_entry *& x)
{
  if (x)
    gt_ggc_mx_types_used_by_vars_entry ((void *) x);
}

void
gt_ggc_mx_hash_table_used_type_hasher_ (void *x_p)
{
  hash_table<used_type_hasher> * const x = (hash_table<used_type_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct used_type_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct used_type_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_nb_iter_bound (void *x_p)
{
  struct nb_iter_bound * x = (struct nb_iter_bound *)x_p;
  struct nb_iter_bound * xlimit = x;
  while (ggc_test_and_set_mark (xlimit))
   xlimit = ((*xlimit).next);
  while (x != xlimit)
    {
      gt_ggc_m_6gimple ((*x).stmt);
      gt_ggc_m_13nb_iter_bound ((*x).next);
      x = ((*x).next);
    }
}

void
gt_ggc_mx_loop_exit (void *x_p)
{
  struct loop_exit * const x = (struct loop_exit *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_8edge_def ((*x).e);
      gt_ggc_m_9loop_exit ((*x).prev);
      gt_ggc_m_9loop_exit ((*x).next);
      gt_ggc_m_9loop_exit ((*x).next_e);
    }
}

void
gt_ggc_mx (struct loop_exit& x_r ATTRIBUTE_UNUSED)
{
  struct loop_exit * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_8edge_def ((*x).e);
  gt_ggc_m_9loop_exit ((*x).prev);
  gt_ggc_m_9loop_exit ((*x).next);
  gt_ggc_m_9loop_exit ((*x).next_e);
}

void
gt_ggc_mx (struct loop_exit *& x)
{
  if (x)
    gt_ggc_mx_loop_exit ((void *) x);
}

void
gt_ggc_mx_loop (void *x_p)
{
  struct loop * x = (struct loop *)x_p;
  struct loop * xlimit = x;
  while (ggc_test_and_set_mark (xlimit))
   xlimit = ((*xlimit).next);
  while (x != xlimit)
    {
      gt_ggc_m_15basic_block_def ((*x).header);
      gt_ggc_m_15basic_block_def ((*x).latch);
      gt_ggc_m_17vec_loop_p_va_gc_ ((*x).superloops);
      gt_ggc_m_4loop ((*x).inner);
      gt_ggc_m_4loop ((*x).next);
      gt_ggc_m_9tree_node ((*x).nb_iterations);
      gt_ggc_m_9tree_node ((*x).simduid);
      gt_ggc_m_13nb_iter_bound ((*x).bounds);
      gt_ggc_m_10control_iv ((*x).control_ivs);
      gt_ggc_m_9loop_exit ((*x).exits);
      gt_ggc_m_10niter_desc ((*x).simple_loop_desc);
      gt_ggc_m_15basic_block_def ((*x).former_header);
      x = ((*x).next);
    }
}

void
gt_ggc_mx_control_iv (void *x_p)
{
  struct control_iv * x = (struct control_iv *)x_p;
  struct control_iv * xlimit = x;
  while (ggc_test_and_set_mark (xlimit))
   xlimit = ((*xlimit).next);
  while (x != xlimit)
    {
      gt_ggc_m_9tree_node ((*x).base);
      gt_ggc_m_9tree_node ((*x).step);
      gt_ggc_m_10control_iv ((*x).next);
      x = ((*x).next);
    }
}

void
gt_ggc_mx_vec_loop_p_va_gc_ (void *x_p)
{
  vec<loop_p,va_gc> * const x = (vec<loop_p,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct loop *& x)
{
  if (x)
    gt_ggc_mx_loop ((void *) x);
}

void
gt_ggc_mx_niter_desc (void *x_p)
{
  struct niter_desc * const x = (struct niter_desc *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_8edge_def ((*x).out_edge);
      gt_ggc_m_8edge_def ((*x).in_edge);
      gt_ggc_m_7rtx_def ((*x).assumptions);
      gt_ggc_m_7rtx_def ((*x).noloop_assumptions);
      gt_ggc_m_7rtx_def ((*x).infinite);
      gt_ggc_m_7rtx_def ((*x).niter_expr);
    }
}

void
gt_ggc_mx_hash_table_loop_exit_hasher_ (void *x_p)
{
  hash_table<loop_exit_hasher> * const x = (hash_table<loop_exit_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct loop_exit_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct loop_exit_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_vec_basic_block_va_gc_ (void *x_p)
{
  vec<basic_block,va_gc> * const x = (vec<basic_block,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct basic_block_def *& x)
{
  if (x)
    gt_ggc_mx_basic_block_def ((void *) x);
}

void
gt_ggc_mx_rtl_bb_info (void *x_p)
{
  struct rtl_bb_info * const x = (struct rtl_bb_info *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_7rtx_def ((*x).end_);
      gt_ggc_m_7rtx_def ((*x).header_);
      gt_ggc_m_7rtx_def ((*x).footer_);
    }
}

void
gt_ggc_mx_vec_edge_va_gc_ (void *x_p)
{
  vec<edge,va_gc> * const x = (vec<edge,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (edge_def *& x)
{
  if (x)
    gt_ggc_mx_edge_def ((void *) x);
}

void
gt_ggc_mx_vec_ipa_ref_t_va_gc_ (void *x_p)
{
  vec<ipa_ref_t,va_gc> * const x = (vec<ipa_ref_t,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct ipa_ref& x_r ATTRIBUTE_UNUSED)
{
  struct ipa_ref * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_11symtab_node ((*x).referring);
  gt_ggc_m_11symtab_node ((*x).referred);
  gt_ggc_m_6gimple ((*x).stmt);
}

void
gt_ggc_mx_section_hash_entry (void *x_p)
{
  struct section_hash_entry * const x = (struct section_hash_entry *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_S ((*x).name);
    }
}

void
gt_ggc_mx (struct section_hash_entry& x_r ATTRIBUTE_UNUSED)
{
  struct section_hash_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_S ((*x).name);
}

void
gt_ggc_mx (struct section_hash_entry *& x)
{
  if (x)
    gt_ggc_mx_section_hash_entry ((void *) x);
}

void
gt_ggc_mx_lto_file_decl_data (void *x_p)
{
  struct lto_file_decl_data * const x = (struct lto_file_decl_data *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_17lto_in_decl_state ((*x).current_decl_state);
      gt_ggc_m_17lto_in_decl_state ((*x).global_decl_state);
      gt_ggc_m_29hash_table_decl_state_hasher_ ((*x).function_decl_states);
      gt_ggc_m_18lto_file_decl_data ((*x).next);
      gt_ggc_m_S ((*x).mode_table);
    }
}

void
gt_ggc_mx_ipa_replace_map (void *x_p)
{
  struct ipa_replace_map * const x = (struct ipa_replace_map *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).new_tree);
    }
}

void
gt_ggc_mx_vec_ipa_replace_map__va_gc_ (void *x_p)
{
  vec<ipa_replace_map*,va_gc> * const x = (vec<ipa_replace_map*,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct ipa_replace_map *& x)
{
  if (x)
    gt_ggc_mx_ipa_replace_map ((void *) x);
}

void
gt_ggc_mx_ipa_param_adjustments (void *x_p)
{
  struct ipa_param_adjustments * const x = (struct ipa_param_adjustments *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_29vec_ipa_adjusted_param_va_gc_ ((*x).m_adj_params);
    }
}

void
gt_ggc_mx_vec_ipa_param_performed_split_va_gc_ (void *x_p)
{
  vec<ipa_param_performed_split,va_gc> * const x = (vec<ipa_param_performed_split,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct ipa_param_performed_split& x_r ATTRIBUTE_UNUSED)
{
  struct ipa_param_performed_split * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).dummy_decl);
}

void
gt_ggc_mx_cgraph_simd_clone (void *x_p)
{
  struct cgraph_simd_clone * const x = (struct cgraph_simd_clone *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      {
        size_t l0 = (size_t)(((*x)).nargs);
        gt_ggc_m_11symtab_node ((*x).prev_clone);
        gt_ggc_m_11symtab_node ((*x).next_clone);
        gt_ggc_m_11symtab_node ((*x).origin);
        {
          size_t i0;
          for (i0 = 0; i0 != l0; i0++) {
            gt_ggc_m_9tree_node ((*x).args[i0].orig_arg);
            gt_ggc_m_9tree_node ((*x).args[i0].orig_type);
            gt_ggc_m_9tree_node ((*x).args[i0].vector_arg);
            gt_ggc_m_9tree_node ((*x).args[i0].vector_type);
            gt_ggc_m_9tree_node ((*x).args[i0].simd_array);
          }
        }
      }
    }
}

void
gt_ggc_mx_cgraph_function_version_info (void *x_p)
{
  struct cgraph_function_version_info * const x = (struct cgraph_function_version_info *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_11symtab_node ((*x).this_node);
      gt_ggc_m_28cgraph_function_version_info ((*x).prev);
      gt_ggc_m_28cgraph_function_version_info ((*x).next);
      gt_ggc_m_9tree_node ((*x).dispatcher_resolver);
    }
}

void
gt_ggc_mx (struct cgraph_function_version_info& x_r ATTRIBUTE_UNUSED)
{
  struct cgraph_function_version_info * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_11symtab_node ((*x).this_node);
  gt_ggc_m_28cgraph_function_version_info ((*x).prev);
  gt_ggc_m_28cgraph_function_version_info ((*x).next);
  gt_ggc_m_9tree_node ((*x).dispatcher_resolver);
}

void
gt_ggc_mx (struct cgraph_function_version_info *& x)
{
  if (x)
    gt_ggc_mx_cgraph_function_version_info ((void *) x);
}

void
gt_ggc_mx_hash_table_cgraph_edge_hasher_ (void *x_p)
{
  hash_table<cgraph_edge_hasher> * const x = (hash_table<cgraph_edge_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct cgraph_edge_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct cgraph_edge_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_cgraph_indirect_call_info (void *x_p)
{
  struct cgraph_indirect_call_info * const x = (struct cgraph_indirect_call_info *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).context.outer_type);
      gt_ggc_m_9tree_node ((*x).context.speculative_outer_type);
      gt_ggc_m_9tree_node ((*x).otr_type);
    }
}

void
gt_ggc_mx_asm_node (void *x_p)
{
  struct asm_node * const x = (struct asm_node *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_8asm_node ((*x).next);
      gt_ggc_m_9tree_node ((*x).asm_str);
    }
}

void
gt_ggc_mx_symbol_table (void *x_p)
{
  struct symbol_table * const x = (struct symbol_table *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_11symtab_node ((*x).nodes);
      gt_ggc_m_8asm_node ((*x).asmnodes);
      gt_ggc_m_8asm_node ((*x).asm_last_node);
      gt_ggc_m_31hash_table_section_name_hasher_ ((*x).section_hash);
      gt_ggc_m_26hash_table_asmname_hasher_ ((*x).assembler_name_hash);
      gt_ggc_m_42hash_map_symtab_node__symbol_priority_map_ ((*x).init_priority_hash);
    }
}

void
gt_ggc_mx_hash_table_section_name_hasher_ (void *x_p)
{
  hash_table<section_name_hasher> * const x = (hash_table<section_name_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct section_name_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct section_name_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_hash_table_asmname_hasher_ (void *x_p)
{
  hash_table<asmname_hasher> * const x = (hash_table<asmname_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct asmname_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct asmname_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_hash_map_symtab_node__symbol_priority_map_ (void *x_p)
{
  hash_map<symtab_node*,symbol_priority_map> * const x = (hash_map<symtab_node*,symbol_priority_map> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct symbol_priority_map& x_r ATTRIBUTE_UNUSED)
{
  struct symbol_priority_map * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx (struct symtab_node *& x)
{
  if (x)
    gt_ggc_mx_symtab_node ((void *) x);
}

void
gt_ggc_mx_constant_descriptor_tree (void *x_p)
{
  struct constant_descriptor_tree * const x = (struct constant_descriptor_tree *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_7rtx_def ((*x).rtl);
      gt_ggc_m_9tree_node ((*x).value);
    }
}

void
gt_ggc_mx (struct constant_descriptor_tree& x_r ATTRIBUTE_UNUSED)
{
  struct constant_descriptor_tree * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_7rtx_def ((*x).rtl);
  gt_ggc_m_9tree_node ((*x).value);
}

void
gt_ggc_mx (struct constant_descriptor_tree *& x)
{
  if (x)
    gt_ggc_mx_constant_descriptor_tree ((void *) x);
}

void
gt_ggc_mx_lto_in_decl_state (void *x_p)
{
  struct lto_in_decl_state * const x = (struct lto_in_decl_state *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      {
        size_t i0;
        size_t l0 = (size_t)(LTO_N_DECL_STREAMS);
        for (i0 = 0; i0 != l0; i0++) {
          gt_ggc_m_15vec_tree_va_gc_ ((*x).streams[i0]);
        }
      }
      gt_ggc_m_9tree_node ((*x).fn_decl);
    }
}

void
gt_ggc_mx (struct lto_in_decl_state& x_r ATTRIBUTE_UNUSED)
{
  struct lto_in_decl_state * ATTRIBUTE_UNUSED x = &x_r;
  {
    size_t i1;
    size_t l1 = (size_t)(LTO_N_DECL_STREAMS);
    for (i1 = 0; i1 != l1; i1++) {
      gt_ggc_m_15vec_tree_va_gc_ ((*x).streams[i1]);
    }
  }
  gt_ggc_m_9tree_node ((*x).fn_decl);
}

void
gt_ggc_mx (struct lto_in_decl_state *& x)
{
  if (x)
    gt_ggc_mx_lto_in_decl_state ((void *) x);
}

void
gt_ggc_mx_ipa_node_params (void *x_p)
{
  struct ipa_node_params * const x = (struct ipa_node_params *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_31vec_ipa_param_descriptor_va_gc_ ((*x).descriptors);
    }
}

void
gt_ggc_mx (struct ipa_node_params& x_r ATTRIBUTE_UNUSED)
{
  struct ipa_node_params * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_31vec_ipa_param_descriptor_va_gc_ ((*x).descriptors);
}

void
gt_ggc_mx (struct ipa_node_params *& x)
{
  if (x)
    gt_ggc_mx_ipa_node_params ((void *) x);
}

void
gt_ggc_mx_ipa_edge_args (void *x_p)
{
  struct ipa_edge_args * const x = (struct ipa_edge_args *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_24vec_ipa_jump_func_va_gc_ ((*x).jump_functions);
      gt_ggc_m_39vec_ipa_polymorphic_call_context_va_gc_ ((*x).polymorphic_call_contexts);
    }
}

void
gt_ggc_mx (struct ipa_edge_args& x_r ATTRIBUTE_UNUSED)
{
  struct ipa_edge_args * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_24vec_ipa_jump_func_va_gc_ ((*x).jump_functions);
  gt_ggc_m_39vec_ipa_polymorphic_call_context_va_gc_ ((*x).polymorphic_call_contexts);
}

void
gt_ggc_mx (struct ipa_edge_args *& x)
{
  if (x)
    gt_ggc_mx_ipa_edge_args ((void *) x);
}

void
gt_ggc_mx_ipa_agg_replacement_value (void *x_p)
{
  struct ipa_agg_replacement_value * const x = (struct ipa_agg_replacement_value *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_25ipa_agg_replacement_value ((*x).next);
      gt_ggc_m_9tree_node ((*x).value);
    }
}

void
gt_ggc_mx_ipa_fn_summary (void *x_p)
{
  struct ipa_fn_summary * const x = (struct ipa_fn_summary *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_20vec_condition_va_gc_ ((*x).conds);
      gt_ggc_m_26vec_size_time_entry_va_gc_ ((*x).size_time_table);
      gt_ggc_m_26vec_size_time_entry_va_gc_ ((*x).call_size_time_table);
    }
}

void
gt_ggc_mx_vec_ipa_adjusted_param_va_gc_ (void *x_p)
{
  vec<ipa_adjusted_param,va_gc> * const x = (vec<ipa_adjusted_param,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct ipa_adjusted_param& x_r ATTRIBUTE_UNUSED)
{
  struct ipa_adjusted_param * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).type);
  gt_ggc_m_9tree_node ((*x).alias_ptr_type);
}

void
gt_ggc_mx_dw_cfi_node (void *x_p)
{
  struct dw_cfi_node * const x = (struct dw_cfi_node *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      switch ((int) (dw_cfi_oprnd1_desc (((*x)).dw_cfi_opc)))
        {
        case dw_cfi_oprnd_reg_num:
          break;
        case dw_cfi_oprnd_offset:
          break;
        case dw_cfi_oprnd_addr:
          gt_ggc_m_S ((*x).dw_cfi_oprnd1.dw_cfi_addr);
          break;
        case dw_cfi_oprnd_loc:
          gt_ggc_m_17dw_loc_descr_node ((*x).dw_cfi_oprnd1.dw_cfi_loc);
          break;
        case dw_cfi_oprnd_cfa_loc:
          gt_ggc_m_15dw_cfa_location ((*x).dw_cfi_oprnd1.dw_cfi_cfa_loc);
          break;
        default:
          break;
        }
      switch ((int) (dw_cfi_oprnd2_desc (((*x)).dw_cfi_opc)))
        {
        case dw_cfi_oprnd_reg_num:
          break;
        case dw_cfi_oprnd_offset:
          break;
        case dw_cfi_oprnd_addr:
          gt_ggc_m_S ((*x).dw_cfi_oprnd2.dw_cfi_addr);
          break;
        case dw_cfi_oprnd_loc:
          gt_ggc_m_17dw_loc_descr_node ((*x).dw_cfi_oprnd2.dw_cfi_loc);
          break;
        case dw_cfi_oprnd_cfa_loc:
          gt_ggc_m_15dw_cfa_location ((*x).dw_cfi_oprnd2.dw_cfi_cfa_loc);
          break;
        default:
          break;
        }
    }
}

void
gt_ggc_mx_dw_loc_descr_node (void *x_p)
{
  struct dw_loc_descr_node * x = (struct dw_loc_descr_node *)x_p;
  struct dw_loc_descr_node * xlimit = x;
  while (ggc_test_and_set_mark (xlimit))
   xlimit = ((*xlimit).dw_loc_next);
  while (x != xlimit)
    {
      gt_ggc_m_17dw_loc_descr_node ((*x).dw_loc_next);
      gt_ggc_m_16addr_table_entry ((*x).dw_loc_oprnd1.val_entry);
      switch ((int) (((*x).dw_loc_oprnd1).val_class))
        {
        case dw_val_class_addr:
          gt_ggc_m_7rtx_def ((*x).dw_loc_oprnd1.v.val_addr);
          break;
        case dw_val_class_offset:
          break;
        case dw_val_class_loc_list:
          gt_ggc_m_18dw_loc_list_struct ((*x).dw_loc_oprnd1.v.val_loc_list);
          break;
        case dw_val_class_view_list:
          gt_ggc_m_10die_struct ((*x).dw_loc_oprnd1.v.val_view_list);
          break;
        case dw_val_class_loc:
          gt_ggc_m_17dw_loc_descr_node ((*x).dw_loc_oprnd1.v.val_loc);
          break;
        default:
          break;
        case dw_val_class_unsigned_const:
          break;
        case dw_val_class_const_double:
          break;
        case dw_val_class_wide_int:
          gt_ggc_m_34generic_wide_int_wide_int_storage_ ((*x).dw_loc_oprnd1.v.val_wide);
          break;
        case dw_val_class_vec:
          if ((*x).dw_loc_oprnd1.v.val_vec.array != NULL) {
            ggc_mark ((*x).dw_loc_oprnd1.v.val_vec.array);
          }
          break;
        case dw_val_class_die_ref:
          gt_ggc_m_10die_struct ((*x).dw_loc_oprnd1.v.val_die_ref.die);
          break;
        case dw_val_class_fde_ref:
          break;
        case dw_val_class_str:
          gt_ggc_m_20indirect_string_node ((*x).dw_loc_oprnd1.v.val_str);
          break;
        case dw_val_class_lbl_id:
          gt_ggc_m_S ((*x).dw_loc_oprnd1.v.val_lbl_id);
          break;
        case dw_val_class_flag:
          break;
        case dw_val_class_file:
          gt_ggc_m_15dwarf_file_data ((*x).dw_loc_oprnd1.v.val_file);
          break;
        case dw_val_class_file_implicit:
          gt_ggc_m_15dwarf_file_data ((*x).dw_loc_oprnd1.v.val_file_implicit);
          break;
        case dw_val_class_data8:
          break;
        case dw_val_class_decl_ref:
          gt_ggc_m_9tree_node ((*x).dw_loc_oprnd1.v.val_decl_ref);
          break;
        case dw_val_class_vms_delta:
          gt_ggc_m_S ((*x).dw_loc_oprnd1.v.val_vms_delta.lbl1);
          gt_ggc_m_S ((*x).dw_loc_oprnd1.v.val_vms_delta.lbl2);
          break;
        case dw_val_class_discr_value:
          switch ((int) (((*x).dw_loc_oprnd1.v.val_discr_value).pos))
            {
            case 0:
              break;
            case 1:
              break;
            default:
              break;
            }
          break;
        case dw_val_class_discr_list:
          gt_ggc_m_18dw_discr_list_node ((*x).dw_loc_oprnd1.v.val_discr_list);
          break;
        case dw_val_class_symview:
          gt_ggc_m_S ((*x).dw_loc_oprnd1.v.val_symbolic_view);
          break;
        }
      gt_ggc_m_16addr_table_entry ((*x).dw_loc_oprnd2.val_entry);
      switch ((int) (((*x).dw_loc_oprnd2).val_class))
        {
        case dw_val_class_addr:
          gt_ggc_m_7rtx_def ((*x).dw_loc_oprnd2.v.val_addr);
          break;
        case dw_val_class_offset:
          break;
        case dw_val_class_loc_list:
          gt_ggc_m_18dw_loc_list_struct ((*x).dw_loc_oprnd2.v.val_loc_list);
          break;
        case dw_val_class_view_list:
          gt_ggc_m_10die_struct ((*x).dw_loc_oprnd2.v.val_view_list);
          break;
        case dw_val_class_loc:
          gt_ggc_m_17dw_loc_descr_node ((*x).dw_loc_oprnd2.v.val_loc);
          break;
        default:
          break;
        case dw_val_class_unsigned_const:
          break;
        case dw_val_class_const_double:
          break;
        case dw_val_class_wide_int:
          gt_ggc_m_34generic_wide_int_wide_int_storage_ ((*x).dw_loc_oprnd2.v.val_wide);
          break;
        case dw_val_class_vec:
          if ((*x).dw_loc_oprnd2.v.val_vec.array != NULL) {
            ggc_mark ((*x).dw_loc_oprnd2.v.val_vec.array);
          }
          break;
        case dw_val_class_die_ref:
          gt_ggc_m_10die_struct ((*x).dw_loc_oprnd2.v.val_die_ref.die);
          break;
        case dw_val_class_fde_ref:
          break;
        case dw_val_class_str:
          gt_ggc_m_20indirect_string_node ((*x).dw_loc_oprnd2.v.val_str);
          break;
        case dw_val_class_lbl_id:
          gt_ggc_m_S ((*x).dw_loc_oprnd2.v.val_lbl_id);
          break;
        case dw_val_class_flag:
          break;
        case dw_val_class_file:
          gt_ggc_m_15dwarf_file_data ((*x).dw_loc_oprnd2.v.val_file);
          break;
        case dw_val_class_file_implicit:
          gt_ggc_m_15dwarf_file_data ((*x).dw_loc_oprnd2.v.val_file_implicit);
          break;
        case dw_val_class_data8:
          break;
        case dw_val_class_decl_ref:
          gt_ggc_m_9tree_node ((*x).dw_loc_oprnd2.v.val_decl_ref);
          break;
        case dw_val_class_vms_delta:
          gt_ggc_m_S ((*x).dw_loc_oprnd2.v.val_vms_delta.lbl1);
          gt_ggc_m_S ((*x).dw_loc_oprnd2.v.val_vms_delta.lbl2);
          break;
        case dw_val_class_discr_value:
          switch ((int) (((*x).dw_loc_oprnd2.v.val_discr_value).pos))
            {
            case 0:
              break;
            case 1:
              break;
            default:
              break;
            }
          break;
        case dw_val_class_discr_list:
          gt_ggc_m_18dw_discr_list_node ((*x).dw_loc_oprnd2.v.val_discr_list);
          break;
        case dw_val_class_symview:
          gt_ggc_m_S ((*x).dw_loc_oprnd2.v.val_symbolic_view);
          break;
        }
      x = ((*x).dw_loc_next);
    }
}

void
gt_ggc_mx_dw_discr_list_node (void *x_p)
{
  struct dw_discr_list_node * const x = (struct dw_discr_list_node *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_18dw_discr_list_node ((*x).dw_discr_next);
      switch ((int) (((*x).dw_discr_lower_bound).pos))
        {
        case 0:
          break;
        case 1:
          break;
        default:
          break;
        }
      switch ((int) (((*x).dw_discr_upper_bound).pos))
        {
        case 0:
          break;
        case 1:
          break;
        default:
          break;
        }
    }
}

void
gt_ggc_mx_vec_dw_cfi_ref_va_gc_ (void *x_p)
{
  vec<dw_cfi_ref,va_gc> * const x = (vec<dw_cfi_ref,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct dw_cfi_node *& x)
{
  if (x)
    gt_ggc_mx_dw_cfi_node ((void *) x);
}

void
gt_ggc_mx_vec_temp_slot_p_va_gc_ (void *x_p)
{
  vec<temp_slot_p,va_gc> * const x = (vec<temp_slot_p,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct temp_slot *& x)
{
  if (x)
    gt_ggc_mx_temp_slot ((void *) x);
}

void
gt_ggc_mx_eh_region_d (void *x_p)
{
  struct eh_region_d * const x = (struct eh_region_d *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_11eh_region_d ((*x).outer);
      gt_ggc_m_11eh_region_d ((*x).inner);
      gt_ggc_m_11eh_region_d ((*x).next_peer);
      switch ((int) ((*x).type))
        {
        case ERT_TRY:
          gt_ggc_m_10eh_catch_d ((*x).u.eh_try.first_catch);
          gt_ggc_m_10eh_catch_d ((*x).u.eh_try.last_catch);
          break;
        case ERT_ALLOWED_EXCEPTIONS:
          gt_ggc_m_9tree_node ((*x).u.allowed.type_list);
          gt_ggc_m_9tree_node ((*x).u.allowed.label);
          break;
        case ERT_MUST_NOT_THROW:
          gt_ggc_m_9tree_node ((*x).u.must_not_throw.failure_decl);
          break;
        default:
          break;
        }
      gt_ggc_m_16eh_landing_pad_d ((*x).landing_pads);
      gt_ggc_m_7rtx_def ((*x).exc_ptr_reg);
      gt_ggc_m_7rtx_def ((*x).filter_reg);
    }
}

void
gt_ggc_mx_eh_landing_pad_d (void *x_p)
{
  struct eh_landing_pad_d * const x = (struct eh_landing_pad_d *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_16eh_landing_pad_d ((*x).next_lp);
      gt_ggc_m_11eh_region_d ((*x).region);
      gt_ggc_m_9tree_node ((*x).post_landing_pad);
      gt_ggc_m_7rtx_def ((*x).landing_pad);
    }
}

void
gt_ggc_mx_eh_catch_d (void *x_p)
{
  struct eh_catch_d * const x = (struct eh_catch_d *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_10eh_catch_d ((*x).next_catch);
      gt_ggc_m_10eh_catch_d ((*x).prev_catch);
      gt_ggc_m_9tree_node ((*x).type_list);
      gt_ggc_m_9tree_node ((*x).filter_list);
      gt_ggc_m_9tree_node ((*x).label);
    }
}

void
gt_ggc_mx_vec_eh_region_va_gc_ (void *x_p)
{
  vec<eh_region,va_gc> * const x = (vec<eh_region,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct eh_region_d *& x)
{
  if (x)
    gt_ggc_mx_eh_region_d ((void *) x);
}

void
gt_ggc_mx_vec_eh_landing_pad_va_gc_ (void *x_p)
{
  vec<eh_landing_pad,va_gc> * const x = (vec<eh_landing_pad,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct eh_landing_pad_d *& x)
{
  if (x)
    gt_ggc_mx_eh_landing_pad_d ((void *) x);
}

void
gt_ggc_mx_hash_map_gimple__int_ (void *x_p)
{
  hash_map<gimple*,int> * const x = (hash_map<gimple*,int> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct gimple *& x)
{
  if (x)
    gt_ggc_mx_gimple ((void *) x);
}

void
gt_ggc_mx_tm_restart_node (void *x_p)
{
  struct tm_restart_node * const x = (struct tm_restart_node *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_6gimple ((*x).stmt);
      gt_ggc_m_9tree_node ((*x).label_or_list);
    }
}

void
gt_ggc_mx (struct tm_restart_node& x_r ATTRIBUTE_UNUSED)
{
  struct tm_restart_node * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_6gimple ((*x).stmt);
  gt_ggc_m_9tree_node ((*x).label_or_list);
}

void
gt_ggc_mx (struct tm_restart_node *& x)
{
  if (x)
    gt_ggc_mx_tm_restart_node ((void *) x);
}

void
gt_ggc_mx_hash_map_tree_tree_ (void *x_p)
{
  hash_map<tree,tree> * const x = (hash_map<tree,tree> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx_hash_table_ssa_name_hasher_ (void *x_p)
{
  hash_table<ssa_name_hasher> * const x = (hash_table<ssa_name_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct ssa_name_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct ssa_name_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_hash_table_tm_restart_hasher_ (void *x_p)
{
  hash_table<tm_restart_hasher> * const x = (hash_table<tm_restart_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct tm_restart_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct tm_restart_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_ssa_operand_memory_d (void *x_p)
{
  struct ssa_operand_memory_d * x = (struct ssa_operand_memory_d *)x_p;
  struct ssa_operand_memory_d * xlimit = x;
  while (ggc_test_and_set_mark (xlimit))
   xlimit = ((*xlimit).next);
  while (x != xlimit)
    {
      gt_ggc_m_20ssa_operand_memory_d ((*x).next);
      x = ((*x).next);
    }
}

void
gt_ggc_mx_vec_ipa_agg_jf_item_va_gc_ (void *x_p)
{
  vec<ipa_agg_jf_item,va_gc> * const x = (vec<ipa_agg_jf_item,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct ipa_agg_jf_item& x_r ATTRIBUTE_UNUSED)
{
  struct ipa_agg_jf_item * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).type);
  switch ((int) (((*x)).jftype))
    {
    case IPA_JF_CONST:
      gt_ggc_m_9tree_node ((*x).value.constant);
      break;
    case IPA_JF_PASS_THROUGH:
      gt_ggc_m_9tree_node ((*x).value.pass_through.operand);
      break;
    case IPA_JF_LOAD_AGG:
      gt_ggc_m_9tree_node ((*x).value.load_agg.pass_through.operand);
      gt_ggc_m_9tree_node ((*x).value.load_agg.type);
      break;
    default:
      break;
    }
}

void
gt_ggc_mx_ipa_bits (void *x_p)
{
  struct ipa_bits * const x = (struct ipa_bits *)x_p;
  if (ggc_test_and_set_mark (x))
    {
    }
}

void
gt_ggc_mx_value_range (void *x_p)
{
  struct value_range * const x = (struct value_range *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).m_min);
      gt_ggc_m_9tree_node ((*x).m_max);
    }
}

void
gt_ggc_mx (struct value_range& x_r ATTRIBUTE_UNUSED)
{
  struct value_range * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).m_min);
  gt_ggc_m_9tree_node ((*x).m_max);
}

void
gt_ggc_mx (struct value_range *& x)
{
  if (x)
    gt_ggc_mx_value_range ((void *) x);
}

void
gt_ggc_mx_vec_ipa_param_descriptor_va_gc_ (void *x_p)
{
  vec<ipa_param_descriptor,va_gc> * const x = (vec<ipa_param_descriptor,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct ipa_param_descriptor& x_r ATTRIBUTE_UNUSED)
{
  struct ipa_param_descriptor * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).decl_or_type);
}

void
gt_ggc_mx_vec_ipa_bits__va_gc_ (void *x_p)
{
  vec<ipa_bits*,va_gc> * const x = (vec<ipa_bits*,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct ipa_bits *& x)
{
  if (x)
    gt_ggc_mx_ipa_bits ((void *) x);
}

void
gt_ggc_mx_vec_ipa_vr_va_gc_ (void *x_p)
{
  vec<ipa_vr,va_gc> * const x = (vec<ipa_vr,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct ipa_vr& x_r ATTRIBUTE_UNUSED)
{
  struct ipa_vr * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_mx (&((*x).min));
  gt_ggc_mx (&((*x).max));
}

void
gt_ggc_mx_ipcp_transformation (void *x_p)
{
  struct ipcp_transformation * const x = (struct ipcp_transformation *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_25ipa_agg_replacement_value ((*x).agg_values);
      gt_ggc_m_20vec_ipa_bits__va_gc_ ((*x).bits);
      gt_ggc_m_17vec_ipa_vr_va_gc_ ((*x).m_vr);
    }
}

void
gt_ggc_mx_vec_ipa_jump_func_va_gc_ (void *x_p)
{
  vec<ipa_jump_func,va_gc> * const x = (vec<ipa_jump_func,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct ipa_jump_func& x_r ATTRIBUTE_UNUSED)
{
  struct ipa_jump_func * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_26vec_ipa_agg_jf_item_va_gc_ ((*x).agg.items);
  gt_ggc_m_8ipa_bits ((*x).bits);
  gt_ggc_m_11value_range ((*x).m_vr);
  switch ((int) (((*x)).type))
    {
    case IPA_JF_CONST:
      gt_ggc_m_9tree_node ((*x).value.constant.value);
      break;
    case IPA_JF_PASS_THROUGH:
      gt_ggc_m_9tree_node ((*x).value.pass_through.operand);
      break;
    case IPA_JF_ANCESTOR:
      break;
    default:
      break;
    }
}

void
gt_ggc_mx_vec_ipa_polymorphic_call_context_va_gc_ (void *x_p)
{
  vec<ipa_polymorphic_call_context,va_gc> * const x = (vec<ipa_polymorphic_call_context,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct ipa_polymorphic_call_context& x_r ATTRIBUTE_UNUSED)
{
  struct ipa_polymorphic_call_context * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).outer_type);
  gt_ggc_m_9tree_node ((*x).speculative_outer_type);
}

void
gt_ggc_mx_ipa_node_params_t (void *x_p)
{
  ipa_node_params_t * const x = (ipa_node_params_t *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx_ipa_edge_args_sum_t (void *x_p)
{
  ipa_edge_args_sum_t * const x = (ipa_edge_args_sum_t *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx_function_summary_ipcp_transformation__ (void *x_p)
{
  function_summary<ipcp_transformation*> * const x = (function_summary<ipcp_transformation*> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct ipcp_transformation *& x)
{
  if (x)
    gt_ggc_mx_ipcp_transformation ((void *) x);
}

void
gt_ggc_mx_hash_table_decl_state_hasher_ (void *x_p)
{
  hash_table<decl_state_hasher> * const x = (hash_table<decl_state_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct decl_state_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct decl_state_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_vec_expr_eval_op_va_gc_ (void *x_p)
{
  vec<expr_eval_op,va_gc> * const x = (vec<expr_eval_op,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct expr_eval_op& x_r ATTRIBUTE_UNUSED)
{
  struct expr_eval_op * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).type);
  {
    size_t i0;
    size_t l0 = (size_t)(2);
    for (i0 = 0; i0 != l0; i0++) {
      gt_ggc_m_9tree_node ((*x).val[i0]);
    }
  }
}

void
gt_ggc_mx_vec_condition_va_gc_ (void *x_p)
{
  vec<condition,va_gc> * const x = (vec<condition,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct condition& x_r ATTRIBUTE_UNUSED)
{
  struct condition * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).type);
  gt_ggc_m_9tree_node ((*x).val);
  gt_ggc_m_23vec_expr_eval_op_va_gc_ ((*x).param_ops);
}

void
gt_ggc_mx_vec_size_time_entry_va_gc_ (void *x_p)
{
  vec<size_time_entry,va_gc> * const x = (vec<size_time_entry,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct size_time_entry& x_r ATTRIBUTE_UNUSED)
{
  struct size_time_entry * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_fast_function_summary_ipa_fn_summary__va_gc_ (void *x_p)
{
  fast_function_summary<ipa_fn_summary*,va_gc> * const x = (fast_function_summary<ipa_fn_summary*,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct ipa_fn_summary *& x)
{
  if (x)
    gt_ggc_mx_ipa_fn_summary ((void *) x);
}

void
gt_pch_nx_line_maps (void *x_p)
{
  struct line_maps * const x = (struct line_maps *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_9line_maps))
    {
      {
        size_t l0 = (size_t)(((*x).info_ordinary).used);
        if ((*x).info_ordinary.maps != NULL) {
          size_t i0;
          for (i0 = 0; i0 != (size_t)(l0); i0++) {
            gt_pch_n_S ((*x).info_ordinary.maps[i0].to_file);
          }
          gt_pch_note_object ((*x).info_ordinary.maps, x, gt_pch_p_9line_maps);
        }
      }
      {
        size_t l1 = (size_t)(((*x).info_macro).used);
        if ((*x).info_macro.maps != NULL) {
          size_t i1;
          for (i1 = 0; i1 != (size_t)(l1); i1++) {
            {
              union tree_node * const x2 =
                ((*x).info_macro.maps[i1].macro) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).info_macro.maps[i1].macro))) : NULL;
              gt_pch_n_9tree_node (x2);
            }
            if ((*x).info_macro.maps[i1].macro_locations != NULL) {
              gt_pch_note_object ((*x).info_macro.maps[i1].macro_locations, x, gt_pch_p_9line_maps);
            }
          }
          gt_pch_note_object ((*x).info_macro.maps, x, gt_pch_p_9line_maps);
        }
      }
      {
        size_t l3 = (size_t)(((*x).location_adhoc_data_map).allocated);
        if ((*x).location_adhoc_data_map.data != NULL) {
          size_t i3;
          for (i3 = 0; i3 != (size_t)(l3); i3++) {
          }
          gt_pch_note_object ((*x).location_adhoc_data_map.data, x, gt_pch_p_9line_maps);
        }
      }
    }
}

void
gt_pch_nx_cpp_token (void *x_p)
{
  struct cpp_token * const x = (struct cpp_token *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_9cpp_token))
    {
      switch ((int) (cpp_token_val_index (&((*x)))))
        {
        case CPP_TOKEN_FLD_NODE:
          {
            union tree_node * const x0 =
              ((*x).val.node.node) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).val.node.node))) : NULL;
            gt_pch_n_9tree_node (x0);
          }
          {
            union tree_node * const x1 =
              ((*x).val.node.spelling) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).val.node.spelling))) : NULL;
            gt_pch_n_9tree_node (x1);
          }
          break;
        case CPP_TOKEN_FLD_SOURCE:
          gt_pch_n_9cpp_token ((*x).val.source);
          break;
        case CPP_TOKEN_FLD_STR:
          gt_pch_n_S ((*x).val.str.text);
          break;
        case CPP_TOKEN_FLD_ARG_NO:
          {
            union tree_node * const x2 =
              ((*x).val.macro_arg.spelling) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).val.macro_arg.spelling))) : NULL;
            gt_pch_n_9tree_node (x2);
          }
          break;
        case CPP_TOKEN_FLD_TOKEN_NO:
          break;
        case CPP_TOKEN_FLD_PRAGMA:
          break;
        default:
          break;
        }
    }
}

void
gt_pch_nx_cpp_macro (void *x_p)
{
  struct cpp_macro * const x = (struct cpp_macro *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_9cpp_macro))
    {
      switch ((int) (((*x)).kind == cmk_assert))
        {
        case false:
          if ((*x).parm.params != NULL) {
            size_t i0;
            for (i0 = 0; i0 != (size_t)(((*x)).paramc); i0++) {
              {
                union tree_node * const x1 =
                  ((*x).parm.params[i0]) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).parm.params[i0]))) : NULL;
                gt_pch_n_9tree_node (x1);
              }
            }
            gt_pch_note_object ((*x).parm.params, x, gt_pch_p_9cpp_macro);
          }
          break;
        case true:
          gt_pch_n_9cpp_macro ((*x).parm.next);
          break;
        default:
          break;
        }
      switch ((int) (((*x)).kind == cmk_traditional))
        {
        case false:
          {
            size_t i2;
            size_t l2 = (size_t)(((*x)).count);
            for (i2 = 0; i2 != l2; i2++) {
              switch ((int) (cpp_token_val_index (&((*x).exp.tokens[i2]))))
                {
                case CPP_TOKEN_FLD_NODE:
                  {
                    union tree_node * const x3 =
                      ((*x).exp.tokens[i2].val.node.node) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).exp.tokens[i2].val.node.node))) : NULL;
                    gt_pch_n_9tree_node (x3);
                  }
                  {
                    union tree_node * const x4 =
                      ((*x).exp.tokens[i2].val.node.spelling) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).exp.tokens[i2].val.node.spelling))) : NULL;
                    gt_pch_n_9tree_node (x4);
                  }
                  break;
                case CPP_TOKEN_FLD_SOURCE:
                  gt_pch_n_9cpp_token ((*x).exp.tokens[i2].val.source);
                  break;
                case CPP_TOKEN_FLD_STR:
                  gt_pch_n_S ((*x).exp.tokens[i2].val.str.text);
                  break;
                case CPP_TOKEN_FLD_ARG_NO:
                  {
                    union tree_node * const x5 =
                      ((*x).exp.tokens[i2].val.macro_arg.spelling) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).exp.tokens[i2].val.macro_arg.spelling))) : NULL;
                    gt_pch_n_9tree_node (x5);
                  }
                  break;
                case CPP_TOKEN_FLD_TOKEN_NO:
                  break;
                case CPP_TOKEN_FLD_PRAGMA:
                  break;
                default:
                  break;
                }
            }
          }
          break;
        case true:
          gt_pch_n_S ((*x).exp.text);
          break;
        default:
          break;
        }
    }
}

void
gt_pch_nx_string_concat (void *x_p)
{
  struct string_concat * const x = (struct string_concat *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_13string_concat))
    {
      if ((*x).m_locs != NULL) {
        gt_pch_note_object ((*x).m_locs, x, gt_pch_p_13string_concat);
      }
    }
}

void
gt_pch_nx_string_concat_db (void *x_p)
{
  struct string_concat_db * const x = (struct string_concat_db *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_16string_concat_db))
    {
      gt_pch_n_38hash_map_location_hash_string_concat__ ((*x).m_table);
    }
}

void
gt_pch_nx_hash_map_location_hash_string_concat__ (void *x_p)
{
  hash_map<location_hash,string_concat*> * const x = (hash_map<location_hash,string_concat*> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_38hash_map_location_hash_string_concat__))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct string_concat *& x)
{
  if (x)
    gt_pch_nx_string_concat ((void *) x);
}

void
gt_pch_nx (struct location_hash& x_r ATTRIBUTE_UNUSED)
{
  struct location_hash * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_bitmap_head (void *x_p)
{
  struct bitmap_head * const x = (struct bitmap_head *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_11bitmap_head))
    {
      gt_pch_n_14bitmap_element ((*x).first);
    }
}

void
gt_pch_nx_rtx_def (void *x_p)
{
  struct rtx_def * x = (struct rtx_def *)x_p;
  struct rtx_def * xlimit = x;
  while (gt_pch_note_object (xlimit, xlimit, gt_pch_p_7rtx_def))
   xlimit = (RTX_NEXT (&(*xlimit)));
  if (x != xlimit)
    for (;;)
      {
        struct rtx_def * const xprev = (RTX_PREV (&(*x)));
        if (xprev == NULL) break;
        x = xprev;
        (void) gt_pch_note_object (xprev, xprev, gt_pch_p_7rtx_def);
      }
  while (x != xlimit)
    {
      switch ((int) (0))
        {
        case 0:
          switch ((int) (GET_CODE (&(*x))))
            {
            case DEBUG_MARKER:
              break;
            case DEBUG_PARAMETER_REF:
              gt_pch_n_9tree_node ((*x).u.fld[0].rt_tree);
              break;
            case ENTRY_VALUE:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case DEBUG_IMPLICIT_PTR:
              gt_pch_n_9tree_node ((*x).u.fld[0].rt_tree);
              break;
            case VAR_LOCATION:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_9tree_node ((*x).u.fld[0].rt_tree);
              break;
            case FMA:
              gt_pch_n_7rtx_def ((*x).u.fld[2].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case US_TRUNCATE:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SS_TRUNCATE:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case US_MINUS:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case US_ASHIFT:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SS_ASHIFT:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SS_ABS:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case US_NEG:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SS_NEG:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SS_MINUS:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case US_PLUS:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SS_PLUS:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case VEC_SERIES:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case VEC_DUPLICATE:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case VEC_CONCAT:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case VEC_SELECT:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case VEC_MERGE:
              gt_pch_n_7rtx_def ((*x).u.fld[2].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case LO_SUM:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case HIGH:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case ZERO_EXTRACT:
              gt_pch_n_7rtx_def ((*x).u.fld[2].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SIGN_EXTRACT:
              gt_pch_n_7rtx_def ((*x).u.fld[2].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case PARITY:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case POPCOUNT:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case CTZ:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case CLZ:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case CLRSB:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case FFS:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case BSWAP:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SQRT:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case ABS:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UNSIGNED_SAT_FRACT:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SAT_FRACT:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UNSIGNED_FRACT_CONVERT:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case FRACT_CONVERT:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UNSIGNED_FIX:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UNSIGNED_FLOAT:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case FIX:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case FLOAT:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case FLOAT_TRUNCATE:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case FLOAT_EXTEND:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case TRUNCATE:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case ZERO_EXTEND:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SIGN_EXTEND:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UNLT:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UNLE:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UNGT:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UNGE:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UNEQ:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case ORDERED:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UNORDERED:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case LTU:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case LEU:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case GTU:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case GEU:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case LTGT:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case LT:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case LE:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case GT:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case GE:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case EQ:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case NE:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case POST_MODIFY:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case PRE_MODIFY:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case POST_INC:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case POST_DEC:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case PRE_INC:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case PRE_DEC:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UMAX:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UMIN:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SMAX:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SMIN:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case ROTATERT:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case LSHIFTRT:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case ASHIFTRT:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case ROTATE:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case ASHIFT:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case NOT:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case XOR:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case IOR:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case AND:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UMOD:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case UDIV:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case MOD:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case US_DIV:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SS_DIV:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case DIV:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case US_MULT:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SS_MULT:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case MULT:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case NEG:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case MINUS:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case PLUS:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case COMPARE:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case IF_THEN_ELSE:
              gt_pch_n_7rtx_def ((*x).u.fld[2].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case CC0:
              break;
            case SYMBOL_REF:
              switch ((int) (SYMBOL_REF_HAS_BLOCK_INFO_P (&(*x))))
                {
                case 1:
                  gt_pch_n_12object_block ((*x).u.block_sym.block);
                  break;
                default:
                  break;
                }
              switch ((int) (CONSTANT_POOL_ADDRESS_P (&(*x))))
                {
                case 1:
                  gt_pch_n_23constant_descriptor_rtx ((*x).u.fld[1].rt_constant);
                  break;
                default:
                  gt_pch_n_9tree_node ((*x).u.fld[1].rt_tree);
                  break;
                }
              gt_pch_n_S ((*x).u.fld[0].rt_str);
              break;
            case LABEL_REF:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case MEM:
              gt_pch_n_9mem_attrs ((*x).u.fld[1].rt_mem);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case CONCATN:
              gt_pch_n_9rtvec_def ((*x).u.fld[0].rt_rtvec);
              break;
            case CONCAT:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case STRICT_LOW_PART:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SUBREG:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SCRATCH:
              break;
            case REG:
              gt_pch_n_9reg_attrs ((*x).u.reg.attrs);
              break;
            case PC:
              break;
            case CONST:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case CONST_STRING:
              gt_pch_n_S ((*x).u.fld[0].rt_str);
              break;
            case CONST_VECTOR:
              gt_pch_n_9rtvec_def ((*x).u.fld[0].rt_rtvec);
              break;
            case CONST_DOUBLE:
              break;
            case CONST_FIXED:
              break;
            case CONST_POLY_INT:
              break;
            case CONST_WIDE_INT:
              break;
            case CONST_INT:
              break;
            case TRAP_IF:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case EH_RETURN:
              break;
            case SIMPLE_RETURN:
              break;
            case RETURN:
              break;
            case CALL:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case CLOBBER:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case USE:
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case SET:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case PREFETCH:
              gt_pch_n_7rtx_def ((*x).u.fld[2].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case ADDR_DIFF_VEC:
              gt_pch_n_7rtx_def ((*x).u.fld[3].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[2].rt_rtx);
              gt_pch_n_9rtvec_def ((*x).u.fld[1].rt_rtvec);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case ADDR_VEC:
              gt_pch_n_9rtvec_def ((*x).u.fld[0].rt_rtvec);
              break;
            case UNSPEC_VOLATILE:
              gt_pch_n_9rtvec_def ((*x).u.fld[0].rt_rtvec);
              break;
            case UNSPEC:
              gt_pch_n_9rtvec_def ((*x).u.fld[0].rt_rtvec);
              break;
            case ASM_OPERANDS:
              gt_pch_n_9rtvec_def ((*x).u.fld[5].rt_rtvec);
              gt_pch_n_9rtvec_def ((*x).u.fld[4].rt_rtvec);
              gt_pch_n_9rtvec_def ((*x).u.fld[3].rt_rtvec);
              gt_pch_n_S ((*x).u.fld[1].rt_str);
              gt_pch_n_S ((*x).u.fld[0].rt_str);
              break;
            case ASM_INPUT:
              gt_pch_n_S ((*x).u.fld[0].rt_str);
              break;
            case PARALLEL:
              gt_pch_n_9rtvec_def ((*x).u.fld[0].rt_rtvec);
              break;
            case COND_EXEC:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case NOTE:
              switch ((int) (NOTE_KIND (&(*x))))
                {
                default:
                  gt_pch_n_S ((*x).u.fld[3].rt_str);
                  break;
                case NOTE_INSN_UPDATE_SJLJ_CONTEXT:
                  break;
                case NOTE_INSN_CFI_LABEL:
                  break;
                case NOTE_INSN_CFI:
                  break;
                case NOTE_INSN_SWITCH_TEXT_SECTIONS:
                  break;
                case NOTE_INSN_BASIC_BLOCK:
                  break;
                case NOTE_INSN_INLINE_ENTRY:
                  break;
                case NOTE_INSN_BEGIN_STMT:
                  break;
                case NOTE_INSN_VAR_LOCATION:
                  gt_pch_n_7rtx_def ((*x).u.fld[3].rt_rtx);
                  break;
                case NOTE_INSN_EH_REGION_END:
                  break;
                case NOTE_INSN_EH_REGION_BEG:
                  break;
                case NOTE_INSN_EPILOGUE_BEG:
                  break;
                case NOTE_INSN_PROLOGUE_END:
                  break;
                case NOTE_INSN_FUNCTION_BEG:
                  break;
                case NOTE_INSN_BLOCK_END:
                  gt_pch_n_9tree_node ((*x).u.fld[3].rt_tree);
                  break;
                case NOTE_INSN_BLOCK_BEG:
                  gt_pch_n_9tree_node ((*x).u.fld[3].rt_tree);
                  break;
                case NOTE_INSN_DELETED_DEBUG_LABEL:
                  gt_pch_n_S ((*x).u.fld[3].rt_str);
                  break;
                case NOTE_INSN_DELETED_LABEL:
                  gt_pch_n_S ((*x).u.fld[3].rt_str);
                  break;
                case NOTE_INSN_DELETED:
                  break;
                }
              gt_pch_n_15basic_block_def ((*x).u.fld[2].rt_bb);
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case CODE_LABEL:
              gt_pch_n_S ((*x).u.fld[6].rt_str);
              gt_pch_n_7rtx_def ((*x).u.fld[3].rt_rtx);
              gt_pch_n_15basic_block_def ((*x).u.fld[2].rt_bb);
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case BARRIER:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case JUMP_TABLE_DATA:
              gt_pch_n_7rtx_def ((*x).u.fld[3].rt_rtx);
              gt_pch_n_15basic_block_def ((*x).u.fld[2].rt_bb);
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case CALL_INSN:
              gt_pch_n_7rtx_def ((*x).u.fld[7].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[6].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[3].rt_rtx);
              gt_pch_n_15basic_block_def ((*x).u.fld[2].rt_bb);
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case JUMP_INSN:
              gt_pch_n_7rtx_def ((*x).u.fld[7].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[6].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[3].rt_rtx);
              gt_pch_n_15basic_block_def ((*x).u.fld[2].rt_bb);
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case INSN:
              gt_pch_n_7rtx_def ((*x).u.fld[6].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[3].rt_rtx);
              gt_pch_n_15basic_block_def ((*x).u.fld[2].rt_bb);
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case DEBUG_INSN:
              gt_pch_n_7rtx_def ((*x).u.fld[6].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[3].rt_rtx);
              gt_pch_n_15basic_block_def ((*x).u.fld[2].rt_bb);
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case ADDRESS:
              break;
            case SEQUENCE:
              gt_pch_n_9rtvec_def ((*x).u.fld[0].rt_rtvec);
              break;
            case INT_LIST:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              break;
            case INSN_LIST:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case EXPR_LIST:
              gt_pch_n_7rtx_def ((*x).u.fld[1].rt_rtx);
              gt_pch_n_7rtx_def ((*x).u.fld[0].rt_rtx);
              break;
            case DEBUG_EXPR:
              gt_pch_n_9tree_node ((*x).u.fld[0].rt_tree);
              break;
            case VALUE:
              break;
            case UNKNOWN:
              break;
            default:
              break;
            }
          break;
        /* Unrecognized tag value.  */
        default: gcc_unreachable (); 
        }
      x = (RTX_NEXT (&(*x)));
    }
}

void
gt_pch_nx_rtvec_def (void *x_p)
{
  struct rtvec_def * const x = (struct rtvec_def *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_9rtvec_def))
    {
      {
        size_t l0 = (size_t)(((*x)).num_elem);
        {
          size_t i0;
          for (i0 = 0; i0 != l0; i0++) {
            gt_pch_n_7rtx_def ((*x).elem[i0]);
          }
        }
      }
    }
}

void
gt_pch_nx_gimple (void *x_p)
{
  struct gimple * x = (struct gimple *)x_p;
  struct gimple * xlimit = x;
  while (gt_pch_note_object (xlimit, xlimit, gt_pch_p_6gimple))
   xlimit = ((*xlimit).next);
  while (x != xlimit)
    {
      switch ((int) (gimple_statement_structure (&((*x)))))
        {
        case GSS_BASE:
          gt_pch_n_15basic_block_def ((*x).bb);
          gt_pch_n_6gimple ((*x).next);
          break;
        case GSS_WCE:
          {
            gimple_statement_wce *sub = static_cast <gimple_statement_wce *> (x);
            gt_pch_n_6gimple ((*sub).cleanup);
            gt_pch_n_15basic_block_def ((*sub).bb);
            gt_pch_n_6gimple ((*sub).next);
          }
          break;
        case GSS_OMP:
          {
            gimple_statement_omp *sub = static_cast <gimple_statement_omp *> (x);
            gt_pch_n_6gimple ((*sub).body);
            gt_pch_n_15basic_block_def ((*sub).bb);
            gt_pch_n_6gimple ((*sub).next);
          }
          break;
        case GSS_OMP_SECTIONS:
          {
            gomp_sections *sub = static_cast <gomp_sections *> (x);
            gt_pch_n_9tree_node ((*sub).clauses);
            gt_pch_n_9tree_node ((*sub).control);
            gt_pch_n_6gimple ((*sub).body);
            gt_pch_n_15basic_block_def ((*sub).bb);
            gt_pch_n_6gimple ((*sub).next);
          }
          break;
        case GSS_OMP_PARALLEL_LAYOUT:
          {
            gimple_statement_omp_parallel_layout *sub = static_cast <gimple_statement_omp_parallel_layout *> (x);
            gt_pch_n_9tree_node ((*sub).clauses);
            gt_pch_n_9tree_node ((*sub).child_fn);
            gt_pch_n_9tree_node ((*sub).data_arg);
            gt_pch_n_6gimple ((*sub).body);
            gt_pch_n_15basic_block_def ((*sub).bb);
            gt_pch_n_6gimple ((*sub).next);
          }
          break;
        case GSS_OMP_TASK:
          {
            gomp_task *sub = static_cast <gomp_task *> (x);
            gt_pch_n_9tree_node ((*sub).copy_fn);
            gt_pch_n_9tree_node ((*sub).arg_size);
            gt_pch_n_9tree_node ((*sub).arg_align);
            gt_pch_n_9tree_node ((*sub).clauses);
            gt_pch_n_9tree_node ((*sub).child_fn);
            gt_pch_n_9tree_node ((*sub).data_arg);
            gt_pch_n_6gimple ((*sub).body);
            gt_pch_n_15basic_block_def ((*sub).bb);
            gt_pch_n_6gimple ((*sub).next);
          }
          break;
        case GSS_OMP_FOR:
          {
            gomp_for *sub = static_cast <gomp_for *> (x);
            {
              size_t l0 = (size_t)(((*sub)).collapse);
              gt_pch_n_9tree_node ((*sub).clauses);
              if ((*sub).iter != NULL) {
                size_t i0;
                for (i0 = 0; i0 != (size_t)(l0); i0++) {
                  gt_pch_n_9tree_node ((*sub).iter[i0].index);
                  gt_pch_n_9tree_node ((*sub).iter[i0].initial);
                  gt_pch_n_9tree_node ((*sub).iter[i0].final);
                  gt_pch_n_9tree_node ((*sub).iter[i0].incr);
                }
                gt_pch_note_object ((*sub).iter, x, gt_pch_p_6gimple);
              }
              gt_pch_n_6gimple ((*sub).pre_body);
              gt_pch_n_6gimple ((*sub).body);
              gt_pch_n_15basic_block_def ((*sub).bb);
              gt_pch_n_6gimple ((*sub).next);
            }
          }
          break;
        case GSS_OMP_SINGLE_LAYOUT:
          {
            gimple_statement_omp_single_layout *sub = static_cast <gimple_statement_omp_single_layout *> (x);
            gt_pch_n_9tree_node ((*sub).clauses);
            gt_pch_n_6gimple ((*sub).body);
            gt_pch_n_15basic_block_def ((*sub).bb);
            gt_pch_n_6gimple ((*sub).next);
          }
          break;
        case GSS_OMP_CRITICAL:
          {
            gomp_critical *sub = static_cast <gomp_critical *> (x);
            gt_pch_n_9tree_node ((*sub).clauses);
            gt_pch_n_9tree_node ((*sub).name);
            gt_pch_n_6gimple ((*sub).body);
            gt_pch_n_15basic_block_def ((*sub).bb);
            gt_pch_n_6gimple ((*sub).next);
          }
          break;
        case GSS_OMP_CONTINUE:
          {
            gomp_continue *sub = static_cast <gomp_continue *> (x);
            gt_pch_n_9tree_node ((*sub).control_def);
            gt_pch_n_9tree_node ((*sub).control_use);
            gt_pch_n_15basic_block_def ((*sub).bb);
            gt_pch_n_6gimple ((*sub).next);
          }
          break;
        case GSS_OMP_ATOMIC_STORE_LAYOUT:
          {
            gimple_statement_omp_atomic_store_layout *sub = static_cast <gimple_statement_omp_atomic_store_layout *> (x);
            gt_pch_n_9tree_node ((*sub).val);
            gt_pch_n_15basic_block_def ((*sub).bb);
            gt_pch_n_6gimple ((*sub).next);
          }
          break;
        case GSS_OMP_ATOMIC_LOAD:
          {
            gomp_atomic_load *sub = static_cast <gomp_atomic_load *> (x);
            gt_pch_n_9tree_node ((*sub).rhs);
            gt_pch_n_9tree_node ((*sub).lhs);
            gt_pch_n_15basic_block_def ((*sub).bb);
            gt_pch_n_6gimple ((*sub).next);
          }
          break;
        case GSS_TRY:
          {
            gtry *sub = static_cast <gtry *> (x);
            gt_pch_n_6gimple ((*sub).eval);
            gt_pch_n_6gimple ((*sub).cleanup);
            gt_pch_n_15basic_block_def ((*sub).bb);
            gt_pch_n_6gimple ((*sub).next);
          }
          break;
        case GSS_PHI:
          {
            gphi *sub = static_cast <gphi *> (x);
            {
              size_t l1 = (size_t)(((*sub)).nargs);
              gt_pch_n_9tree_node ((*sub).result);
              {
                size_t i1;
                for (i1 = 0; i1 != l1; i1++) {
                  gt_pch_n_9tree_node ((*sub).args[i1].def);
                }
              }
              gt_pch_n_15basic_block_def ((*sub).bb);
              gt_pch_n_6gimple ((*sub).next);
            }
          }
          break;
        case GSS_EH_CTRL:
          {
            gimple_statement_eh_ctrl *sub = static_cast <gimple_statement_eh_ctrl *> (x);
            gt_pch_n_15basic_block_def ((*sub).bb);
            gt_pch_n_6gimple ((*sub).next);
          }
          break;
        case GSS_EH_ELSE:
          {
            geh_else *sub = static_cast <geh_else *> (x);
            gt_pch_n_6gimple ((*sub).n_body);
            gt_pch_n_6gimple ((*sub).e_body);
            gt_pch_n_15basic_block_def ((*sub).bb);
            gt_pch_n_6gimple ((*sub).next);
          }
          break;
        case GSS_EH_MNT:
          {
            geh_mnt *sub = static_cast <geh_mnt *> (x);
            gt_pch_n_9tree_node ((*sub).fndecl);
            gt_pch_n_15basic_block_def ((*sub).bb);
            gt_pch_n_6gimple ((*sub).next);
          }
          break;
        case GSS_EH_FILTER:
          {
            geh_filter *sub = static_cast <geh_filter *> (x);
            gt_pch_n_9tree_node ((*sub).types);
            gt_pch_n_6gimple ((*sub).failure);
            gt_pch_n_15basic_block_def ((*sub).bb);
            gt_pch_n_6gimple ((*sub).next);
          }
          break;
        case GSS_CATCH:
          {
            gcatch *sub = static_cast <gcatch *> (x);
            gt_pch_n_9tree_node ((*sub).types);
            gt_pch_n_6gimple ((*sub).handler);
            gt_pch_n_15basic_block_def ((*sub).bb);
            gt_pch_n_6gimple ((*sub).next);
          }
          break;
        case GSS_BIND:
          {
            gbind *sub = static_cast <gbind *> (x);
            gt_pch_n_9tree_node ((*sub).vars);
            gt_pch_n_9tree_node ((*sub).block);
            gt_pch_n_6gimple ((*sub).body);
            gt_pch_n_15basic_block_def ((*sub).bb);
            gt_pch_n_6gimple ((*sub).next);
          }
          break;
        case GSS_WITH_MEM_OPS_BASE:
          {
            gimple_statement_with_memory_ops_base *sub = static_cast <gimple_statement_with_memory_ops_base *> (x);
            gt_pch_n_15basic_block_def ((*sub).bb);
            gt_pch_n_6gimple ((*sub).next);
          }
          break;
        case GSS_TRANSACTION:
          {
            gtransaction *sub = static_cast <gtransaction *> (x);
            gt_pch_n_6gimple ((*sub).body);
            gt_pch_n_9tree_node ((*sub).label_norm);
            gt_pch_n_9tree_node ((*sub).label_uninst);
            gt_pch_n_9tree_node ((*sub).label_over);
            gt_pch_n_15basic_block_def ((*sub).bb);
            gt_pch_n_6gimple ((*sub).next);
          }
          break;
        case GSS_CALL:
          {
            gcall *sub = static_cast <gcall *> (x);
            {
              size_t l2 = (size_t)(((*sub)).num_ops);
              gt_pch_n_11bitmap_head ((*sub).call_used.vars);
              gt_pch_n_11bitmap_head ((*sub).call_clobbered.vars);
              switch ((int) (((*sub)).subcode & GF_CALL_INTERNAL))
                {
                case 0:
                  gt_pch_n_9tree_node ((*sub).u.fntype);
                  break;
                case GF_CALL_INTERNAL:
                  break;
                default:
                  break;
                }
              {
                size_t i2;
                for (i2 = 0; i2 != l2; i2++) {
                  gt_pch_n_9tree_node ((*sub).op[i2]);
                }
              }
              gt_pch_n_15basic_block_def ((*sub).bb);
              gt_pch_n_6gimple ((*sub).next);
            }
          }
          break;
        case GSS_ASM:
          {
            gasm *sub = static_cast <gasm *> (x);
            {
              size_t l3 = (size_t)(((*sub)).num_ops);
              gt_pch_n_S ((*sub).string);
              {
                size_t i3;
                for (i3 = 0; i3 != l3; i3++) {
                  gt_pch_n_9tree_node ((*sub).op[i3]);
                }
              }
              gt_pch_n_15basic_block_def ((*sub).bb);
              gt_pch_n_6gimple ((*sub).next);
            }
          }
          break;
        case GSS_WITH_MEM_OPS:
          {
            gimple_statement_with_memory_ops *sub = static_cast <gimple_statement_with_memory_ops *> (x);
            {
              size_t l4 = (size_t)(((*sub)).num_ops);
              {
                size_t i4;
                for (i4 = 0; i4 != l4; i4++) {
                  gt_pch_n_9tree_node ((*sub).op[i4]);
                }
              }
              gt_pch_n_15basic_block_def ((*sub).bb);
              gt_pch_n_6gimple ((*sub).next);
            }
          }
          break;
        case GSS_WITH_OPS:
          {
            gimple_statement_with_ops *sub = static_cast <gimple_statement_with_ops *> (x);
            {
              size_t l5 = (size_t)(((*sub)).num_ops);
              {
                size_t i5;
                for (i5 = 0; i5 != l5; i5++) {
                  gt_pch_n_9tree_node ((*sub).op[i5]);
                }
              }
              gt_pch_n_15basic_block_def ((*sub).bb);
              gt_pch_n_6gimple ((*sub).next);
            }
          }
          break;
        /* Unrecognized tag value.  */
        default: gcc_unreachable (); 
        }
      x = ((*x).next);
    }
}

void
gt_pch_nx_symtab_node (void *x_p)
{
  struct symtab_node * x = (struct symtab_node *)x_p;
  struct symtab_node * xlimit = x;
  while (gt_pch_note_object (xlimit, xlimit, gt_pch_p_11symtab_node))
   xlimit = ((*xlimit).next);
  if (x != xlimit)
    for (;;)
      {
        struct symtab_node * const xprev = ((*x).previous);
        if (xprev == NULL) break;
        x = xprev;
        (void) gt_pch_note_object (xprev, xprev, gt_pch_p_11symtab_node);
      }
  while (x != xlimit)
    {
      switch ((int) (((*x)).type))
        {
        case SYMTAB_SYMBOL:
          gt_pch_n_9tree_node ((*x).decl);
          gt_pch_n_11symtab_node ((*x).next);
          gt_pch_n_11symtab_node ((*x).previous);
          gt_pch_n_11symtab_node ((*x).next_sharing_asm_name);
          gt_pch_n_11symtab_node ((*x).previous_sharing_asm_name);
          gt_pch_n_11symtab_node ((*x).same_comdat_group);
          gt_pch_n_20vec_ipa_ref_t_va_gc_ ((*x).ref_list.references);
          gt_pch_n_9tree_node ((*x).alias_target);
          gt_pch_n_18lto_file_decl_data ((*x).lto_file_data);
          gt_pch_n_9tree_node ((*x).x_comdat_group);
          gt_pch_n_18section_hash_entry ((*x).x_section);
          break;
        case SYMTAB_VARIABLE:
          {
            varpool_node *sub = static_cast <varpool_node *> (x);
            gt_pch_n_9tree_node ((*sub).decl);
            gt_pch_n_11symtab_node ((*sub).next);
            gt_pch_n_11symtab_node ((*sub).previous);
            gt_pch_n_11symtab_node ((*sub).next_sharing_asm_name);
            gt_pch_n_11symtab_node ((*sub).previous_sharing_asm_name);
            gt_pch_n_11symtab_node ((*sub).same_comdat_group);
            gt_pch_n_20vec_ipa_ref_t_va_gc_ ((*sub).ref_list.references);
            gt_pch_n_9tree_node ((*sub).alias_target);
            gt_pch_n_18lto_file_decl_data ((*sub).lto_file_data);
            gt_pch_n_9tree_node ((*sub).x_comdat_group);
            gt_pch_n_18section_hash_entry ((*sub).x_section);
          }
          break;
        case SYMTAB_FUNCTION:
          {
            cgraph_node *sub = static_cast <cgraph_node *> (x);
            gt_pch_n_11cgraph_edge ((*sub).callees);
            gt_pch_n_11cgraph_edge ((*sub).callers);
            gt_pch_n_11cgraph_edge ((*sub).indirect_calls);
            gt_pch_n_11symtab_node ((*sub).origin);
            gt_pch_n_11symtab_node ((*sub).nested);
            gt_pch_n_11symtab_node ((*sub).next_nested);
            gt_pch_n_11symtab_node ((*sub).next_sibling_clone);
            gt_pch_n_11symtab_node ((*sub).prev_sibling_clone);
            gt_pch_n_11symtab_node ((*sub).clones);
            gt_pch_n_11symtab_node ((*sub).clone_of);
            gt_pch_n_30hash_table_cgraph_edge_hasher_ ((*sub).call_site_hash);
            gt_pch_n_9tree_node ((*sub).former_clone_of);
            gt_pch_n_17cgraph_simd_clone ((*sub).simdclone);
            gt_pch_n_11symtab_node ((*sub).simd_clones);
            gt_pch_n_11symtab_node ((*sub).inlined_to);
            gt_pch_n_15cgraph_rtl_info ((*sub).rtl);
            gt_pch_n_27vec_ipa_replace_map__va_gc_ ((*sub).clone.tree_map);
            gt_pch_n_21ipa_param_adjustments ((*sub).clone.param_adjustments);
            gt_pch_n_36vec_ipa_param_performed_split_va_gc_ ((*sub).clone.performed_splits);
            gt_pch_n_9tree_node ((*sub).thunk.alias);
            gt_pch_n_9tree_node ((*sub).decl);
            gt_pch_n_11symtab_node ((*sub).next);
            gt_pch_n_11symtab_node ((*sub).previous);
            gt_pch_n_11symtab_node ((*sub).next_sharing_asm_name);
            gt_pch_n_11symtab_node ((*sub).previous_sharing_asm_name);
            gt_pch_n_11symtab_node ((*sub).same_comdat_group);
            gt_pch_n_20vec_ipa_ref_t_va_gc_ ((*sub).ref_list.references);
            gt_pch_n_9tree_node ((*sub).alias_target);
            gt_pch_n_18lto_file_decl_data ((*sub).lto_file_data);
            gt_pch_n_9tree_node ((*sub).x_comdat_group);
            gt_pch_n_18section_hash_entry ((*sub).x_section);
          }
          break;
        /* Unrecognized tag value.  */
        default: gcc_unreachable (); 
        }
      x = ((*x).next);
    }
}

void
gt_pch_nx_cgraph_edge (void *x_p)
{
  struct cgraph_edge * x = (struct cgraph_edge *)x_p;
  struct cgraph_edge * xlimit = x;
  while (gt_pch_note_object (xlimit, xlimit, gt_pch_p_11cgraph_edge))
   xlimit = ((*xlimit).next_caller);
  if (x != xlimit)
    for (;;)
      {
        struct cgraph_edge * const xprev = ((*x).prev_caller);
        if (xprev == NULL) break;
        x = xprev;
        (void) gt_pch_note_object (xprev, xprev, gt_pch_p_11cgraph_edge);
      }
  while (x != xlimit)
    {
      gt_pch_n_11symtab_node ((*x).caller);
      gt_pch_n_11symtab_node ((*x).callee);
      gt_pch_n_11cgraph_edge ((*x).prev_caller);
      gt_pch_n_11cgraph_edge ((*x).next_caller);
      gt_pch_n_11cgraph_edge ((*x).prev_callee);
      gt_pch_n_11cgraph_edge ((*x).next_callee);
      gt_pch_n_6gimple ((*x).call_stmt);
      gt_pch_n_25cgraph_indirect_call_info ((*x).indirect_info);
      x = ((*x).next_caller);
    }
}

void
gt_pch_nx (struct cgraph_edge& x_r ATTRIBUTE_UNUSED)
{
  struct cgraph_edge * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_11symtab_node ((*x).caller);
  gt_pch_n_11symtab_node ((*x).callee);
  gt_pch_n_11cgraph_edge ((*x).prev_caller);
  gt_pch_n_11cgraph_edge ((*x).next_caller);
  gt_pch_n_11cgraph_edge ((*x).prev_callee);
  gt_pch_n_11cgraph_edge ((*x).next_callee);
  gt_pch_n_6gimple ((*x).call_stmt);
  gt_pch_n_25cgraph_indirect_call_info ((*x).indirect_info);
}

void
gt_pch_nx (struct cgraph_edge *& x)
{
  if (x)
    gt_pch_nx_cgraph_edge ((void *) x);
}

void
gt_pch_nx_section (void *x_p)
{
  union section * const x = (union section *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_7section))
    {
      switch ((int) (SECTION_STYLE (&(((*x))))))
        {
        case SECTION_NAMED:
          gt_pch_n_S ((*x).named.name);
          gt_pch_n_9tree_node ((*x).named.decl);
          break;
        case SECTION_UNNAMED:
          gt_pch_n_7section ((*x).unnamed.next);
          break;
        case SECTION_NOSWITCH:
          break;
        default:
          break;
        }
    }
}

void
gt_pch_nx (union section& x_r ATTRIBUTE_UNUSED)
{
  union section * ATTRIBUTE_UNUSED x = &x_r;
  switch ((int) (SECTION_STYLE (&(((*x))))))
    {
    case SECTION_NAMED:
      gt_pch_n_S ((*x).named.name);
      gt_pch_n_9tree_node ((*x).named.decl);
      break;
    case SECTION_UNNAMED:
      gt_pch_n_7section ((*x).unnamed.next);
      break;
    case SECTION_NOSWITCH:
      break;
    default:
      break;
    }
}

void
gt_pch_nx (union section *& x)
{
  if (x)
    gt_pch_nx_section ((void *) x);
}

void
gt_pch_nx_cl_target_option (void *x_p)
{
  struct cl_target_option * const x = (struct cl_target_option *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_16cl_target_option))
    {
    }
}

void
gt_pch_nx_cl_optimization (void *x_p)
{
  struct cl_optimization * const x = (struct cl_optimization *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_15cl_optimization))
    {
      gt_pch_n_S ((*x).x_str_align_functions);
      gt_pch_n_S ((*x).x_str_align_jumps);
      gt_pch_n_S ((*x).x_str_align_labels);
      gt_pch_n_S ((*x).x_str_align_loops);
    }
}

void
gt_pch_nx_edge_def (void *x_p)
{
  edge_def * const x = (edge_def *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_8edge_def))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx_basic_block_def (void *x_p)
{
  struct basic_block_def * x = (struct basic_block_def *)x_p;
  struct basic_block_def * xlimit = x;
  while (gt_pch_note_object (xlimit, xlimit, gt_pch_p_15basic_block_def))
   xlimit = ((*xlimit).next_bb);
  if (x != xlimit)
    for (;;)
      {
        struct basic_block_def * const xprev = ((*x).prev_bb);
        if (xprev == NULL) break;
        x = xprev;
        (void) gt_pch_note_object (xprev, xprev, gt_pch_p_15basic_block_def);
      }
  while (x != xlimit)
    {
      gt_pch_n_15vec_edge_va_gc_ ((*x).preds);
      gt_pch_n_15vec_edge_va_gc_ ((*x).succs);
      gt_pch_n_4loop ((*x).loop_father);
      gt_pch_n_15basic_block_def ((*x).prev_bb);
      gt_pch_n_15basic_block_def ((*x).next_bb);
      switch ((int) (((((*x)).flags & BB_RTL) != 0)))
        {
        case 0:
          gt_pch_n_6gimple ((*x).il.gimple.seq);
          gt_pch_n_6gimple ((*x).il.gimple.phi_nodes);
          break;
        case 1:
          gt_pch_n_7rtx_def ((*x).il.x.head_);
          gt_pch_n_11rtl_bb_info ((*x).il.x.rtl);
          break;
        default:
          break;
        }
      x = ((*x).next_bb);
    }
}

void
gt_pch_nx_bitmap_element (void *x_p)
{
  struct bitmap_element * x = (struct bitmap_element *)x_p;
  struct bitmap_element * xlimit = x;
  while (gt_pch_note_object (xlimit, xlimit, gt_pch_p_14bitmap_element))
   xlimit = ((*xlimit).next);
  while (x != xlimit)
    {
      gt_pch_n_14bitmap_element ((*x).next);
      gt_pch_n_14bitmap_element ((*x).prev);
      x = ((*x).next);
    }
}

void
gt_pch_nx_generic_wide_int_wide_int_storage_ (void *x_p)
{
  generic_wide_int<wide_int_storage> * const x = (generic_wide_int<wide_int_storage> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_34generic_wide_int_wide_int_storage_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct wide_int_storage& x_r ATTRIBUTE_UNUSED)
{
  struct wide_int_storage * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_mem_attrs (void *x_p)
{
  struct mem_attrs * const x = (struct mem_attrs *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_9mem_attrs))
    {
      gt_pch_n_9tree_node ((*x).expr);
    }
}

void
gt_pch_nx_reg_attrs (void *x_p)
{
  struct reg_attrs * const x = (struct reg_attrs *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_9reg_attrs))
    {
      gt_pch_n_9tree_node ((*x).decl);
    }
}

void
gt_pch_nx (struct reg_attrs& x_r ATTRIBUTE_UNUSED)
{
  struct reg_attrs * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).decl);
}

void
gt_pch_nx (struct reg_attrs *& x)
{
  if (x)
    gt_pch_nx_reg_attrs ((void *) x);
}

void
gt_pch_nx_object_block (void *x_p)
{
  struct object_block * const x = (struct object_block *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_12object_block))
    {
      gt_pch_n_7section ((*x).sect);
      gt_pch_n_14vec_rtx_va_gc_ ((*x).objects);
      gt_pch_n_14vec_rtx_va_gc_ ((*x).anchors);
    }
}

void
gt_pch_nx (struct object_block& x_r ATTRIBUTE_UNUSED)
{
  struct object_block * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_7section ((*x).sect);
  gt_pch_n_14vec_rtx_va_gc_ ((*x).objects);
  gt_pch_n_14vec_rtx_va_gc_ ((*x).anchors);
}

void
gt_pch_nx (struct object_block *& x)
{
  if (x)
    gt_pch_nx_object_block ((void *) x);
}

void
gt_pch_nx_vec_rtx_va_gc_ (void *x_p)
{
  vec<rtx,va_gc> * const x = (vec<rtx,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_14vec_rtx_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct rtx_def *& x)
{
  if (x)
    gt_pch_nx_rtx_def ((void *) x);
}

void
gt_pch_nx_real_value (void *x_p)
{
  struct real_value * const x = (struct real_value *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_10real_value))
    {
    }
}

void
gt_pch_nx_fixed_value (void *x_p)
{
  struct fixed_value * const x = (struct fixed_value *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_11fixed_value))
    {
      gt_pch_nx (&((*x).mode));
    }
}

void
gt_pch_nx_function (void *x_p)
{
  struct function * const x = (struct function *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_8function))
    {
      gt_pch_n_9eh_status ((*x).eh);
      gt_pch_n_18control_flow_graph ((*x).cfg);
      gt_pch_n_6gimple ((*x).gimple_body);
      gt_pch_n_9gimple_df ((*x).gimple_df);
      gt_pch_n_5loops ((*x).x_current_loops);
      gt_pch_n_S ((*x).pass_startwith);
      gt_pch_n_11stack_usage ((*x).su);
      gt_pch_n_9tree_node ((*x).decl);
      gt_pch_n_9tree_node ((*x).static_chain_decl);
      gt_pch_n_9tree_node ((*x).nonlocal_goto_save_area);
      gt_pch_n_15vec_tree_va_gc_ ((*x).local_decls);
      gcc_assert (!(*x).machine);
      gt_pch_n_17language_function ((*x).language);
      gt_pch_n_14hash_set_tree_ ((*x).used_types_hash);
      gt_pch_n_11dw_fde_node ((*x).fde);
    }
}

void
gt_pch_nx_target_rtl (void *x_p)
{
  struct target_rtl * const x = (struct target_rtl *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_10target_rtl))
    {
      {
        size_t i0;
        size_t l0 = (size_t)(GR_MAX);
        for (i0 = 0; i0 != l0; i0++) {
          gt_pch_n_7rtx_def ((*x).x_global_rtl[i0]);
        }
      }
      gt_pch_n_7rtx_def ((*x).x_pic_offset_table_rtx);
      gt_pch_n_7rtx_def ((*x).x_return_address_pointer_rtx);
      {
        size_t i1;
        size_t l1 = (size_t)(FIRST_PSEUDO_REGISTER);
        for (i1 = 0; i1 != l1; i1++) {
          gt_pch_n_7rtx_def ((*x).x_initial_regno_reg_rtx[i1]);
        }
      }
      {
        size_t i2;
        size_t l2 = (size_t)(MAX_MACHINE_MODE);
        for (i2 = 0; i2 != l2; i2++) {
          gt_pch_n_7rtx_def ((*x).x_top_of_stack[i2]);
        }
      }
      {
        size_t i3;
        size_t l3 = (size_t)(FIRST_PSEUDO_REGISTER);
        for (i3 = 0; i3 != l3; i3++) {
          gt_pch_n_7rtx_def ((*x).x_static_reg_base_value[i3]);
        }
      }
      {
        size_t i4;
        size_t l4 = (size_t)((int) MAX_MACHINE_MODE);
        for (i4 = 0; i4 != l4; i4++) {
          gt_pch_n_9mem_attrs ((*x).x_mode_mem_attrs[i4]);
        }
      }
    }
}

void
gt_pch_nx_cgraph_rtl_info (void *x_p)
{
  struct cgraph_rtl_info * const x = (struct cgraph_rtl_info *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_15cgraph_rtl_info))
    {
    }
}

void
gt_pch_nx_hash_map_tree_tree_decl_tree_cache_traits_ (void *x_p)
{
  hash_map<tree,tree,decl_tree_cache_traits> * const x = (hash_map<tree,tree,decl_tree_cache_traits> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_42hash_map_tree_tree_decl_tree_cache_traits_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct decl_tree_cache_traits& x_r ATTRIBUTE_UNUSED)
{
  struct decl_tree_cache_traits * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx (union tree_node *& x)
{
  if (x)
    gt_pch_nx_lang_tree_node ((void *) x);
}

void
gt_pch_nx_hash_map_tree_tree_type_tree_cache_traits_ (void *x_p)
{
  hash_map<tree,tree,type_tree_cache_traits> * const x = (hash_map<tree,tree,type_tree_cache_traits> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_42hash_map_tree_tree_type_tree_cache_traits_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct type_tree_cache_traits& x_r ATTRIBUTE_UNUSED)
{
  struct type_tree_cache_traits * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_hash_map_tree_tree_decl_tree_traits_ (void *x_p)
{
  hash_map<tree,tree,decl_tree_traits> * const x = (hash_map<tree,tree,decl_tree_traits> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_36hash_map_tree_tree_decl_tree_traits_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct decl_tree_traits& x_r ATTRIBUTE_UNUSED)
{
  struct decl_tree_traits * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_ptr_info_def (void *x_p)
{
  struct ptr_info_def * const x = (struct ptr_info_def *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_12ptr_info_def))
    {
      gt_pch_n_11bitmap_head ((*x).pt.vars);
    }
}

void
gt_pch_nx_range_info_def (void *x_p)
{
  struct range_info_def * const x = (struct range_info_def *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_14range_info_def))
    {
    }
}

void
gt_pch_nx_vec_constructor_elt_va_gc_ (void *x_p)
{
  vec<constructor_elt,va_gc> * const x = (vec<constructor_elt,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_26vec_constructor_elt_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct constructor_elt& x_r ATTRIBUTE_UNUSED)
{
  struct constructor_elt * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).index);
  gt_pch_n_9tree_node ((*x).value);
}

void
gt_pch_nx_vec_tree_va_gc_ (void *x_p)
{
  vec<tree,va_gc> * const x = (vec<tree,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_15vec_tree_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx_tree_statement_list_node (void *x_p)
{
  struct tree_statement_list_node * x = (struct tree_statement_list_node *)x_p;
  struct tree_statement_list_node * xlimit = x;
  while (gt_pch_note_object (xlimit, xlimit, gt_pch_p_24tree_statement_list_node))
   xlimit = ((*xlimit).next);
  if (x != xlimit)
    for (;;)
      {
        struct tree_statement_list_node * const xprev = ((*x).prev);
        if (xprev == NULL) break;
        x = xprev;
        (void) gt_pch_note_object (xprev, xprev, gt_pch_p_24tree_statement_list_node);
      }
  while (x != xlimit)
    {
      gt_pch_n_24tree_statement_list_node ((*x).prev);
      gt_pch_n_24tree_statement_list_node ((*x).next);
      gt_pch_n_9tree_node ((*x).stmt);
      x = ((*x).next);
    }
}

void
gt_pch_nx_target_globals (void *x_p)
{
  struct target_globals * const x = (struct target_globals *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_14target_globals))
    {
      gt_pch_n_10target_rtl ((*x).rtl);
      gt_pch_n_15target_libfuncs ((*x).libfuncs);
    }
}

void
gt_pch_nx_tree_map (void *x_p)
{
  struct tree_map * const x = (struct tree_map *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_8tree_map))
    {
      gt_pch_n_9tree_node ((*x).base.from);
      gt_pch_n_9tree_node ((*x).to);
    }
}

void
gt_pch_nx (struct tree_map& x_r ATTRIBUTE_UNUSED)
{
  struct tree_map * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).base.from);
  gt_pch_n_9tree_node ((*x).to);
}

void
gt_pch_nx (struct tree_map *& x)
{
  if (x)
    gt_pch_nx_tree_map ((void *) x);
}

void
gt_pch_nx_tree_decl_map (void *x_p)
{
  struct tree_decl_map * const x = (struct tree_decl_map *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_13tree_decl_map))
    {
      gt_pch_n_9tree_node ((*x).base.from);
      gt_pch_n_9tree_node ((*x).to);
    }
}

void
gt_pch_nx (struct tree_decl_map& x_r ATTRIBUTE_UNUSED)
{
  struct tree_decl_map * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).base.from);
  gt_pch_n_9tree_node ((*x).to);
}

void
gt_pch_nx (struct tree_decl_map *& x)
{
  if (x)
    gt_pch_nx_tree_decl_map ((void *) x);
}

void
gt_pch_nx_tree_int_map (void *x_p)
{
  struct tree_int_map * const x = (struct tree_int_map *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_12tree_int_map))
    {
      gt_pch_n_9tree_node ((*x).base.from);
    }
}

void
gt_pch_nx (struct tree_int_map& x_r ATTRIBUTE_UNUSED)
{
  struct tree_int_map * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).base.from);
}

void
gt_pch_nx (struct tree_int_map *& x)
{
  if (x)
    gt_pch_nx_tree_int_map ((void *) x);
}

void
gt_pch_nx_tree_vec_map (void *x_p)
{
  struct tree_vec_map * const x = (struct tree_vec_map *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_12tree_vec_map))
    {
      gt_pch_n_9tree_node ((*x).base.from);
      gt_pch_n_15vec_tree_va_gc_ ((*x).to);
    }
}

void
gt_pch_nx (struct tree_vec_map& x_r ATTRIBUTE_UNUSED)
{
  struct tree_vec_map * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).base.from);
  gt_pch_n_15vec_tree_va_gc_ ((*x).to);
}

void
gt_pch_nx (struct tree_vec_map *& x)
{
  if (x)
    gt_pch_nx_tree_vec_map ((void *) x);
}

void
gt_pch_nx_vec_alias_pair_va_gc_ (void *x_p)
{
  vec<alias_pair,va_gc> * const x = (vec<alias_pair,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_21vec_alias_pair_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct alias_pair& x_r ATTRIBUTE_UNUSED)
{
  struct alias_pair * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).decl);
  gt_pch_n_9tree_node ((*x).target);
}

void
gt_pch_nx_libfunc_entry (void *x_p)
{
  struct libfunc_entry * const x = (struct libfunc_entry *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_13libfunc_entry))
    {
      gt_pch_n_7rtx_def ((*x).libfunc);
    }
}

void
gt_pch_nx (struct libfunc_entry& x_r ATTRIBUTE_UNUSED)
{
  struct libfunc_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_7rtx_def ((*x).libfunc);
}

void
gt_pch_nx (struct libfunc_entry *& x)
{
  if (x)
    gt_pch_nx_libfunc_entry ((void *) x);
}

void
gt_pch_nx_hash_table_libfunc_hasher_ (void *x_p)
{
  hash_table<libfunc_hasher> * const x = (hash_table<libfunc_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_26hash_table_libfunc_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct libfunc_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct libfunc_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_target_libfuncs (void *x_p)
{
  struct target_libfuncs * const x = (struct target_libfuncs *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_15target_libfuncs))
    {
      {
        size_t i0;
        size_t l0 = (size_t)(LTI_MAX);
        for (i0 = 0; i0 != l0; i0++) {
          gt_pch_n_7rtx_def ((*x).x_libfunc_table[i0]);
        }
      }
      gt_pch_n_26hash_table_libfunc_hasher_ ((*x).x_libfunc_hash);
    }
}

void
gt_pch_nx_sequence_stack (void *x_p)
{
  struct sequence_stack * const x = (struct sequence_stack *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_14sequence_stack))
    {
      gt_pch_n_7rtx_def ((*x).first);
      gt_pch_n_7rtx_def ((*x).last);
      gt_pch_n_14sequence_stack ((*x).next);
    }
}

void
gt_pch_nx_vec_rtx_insn__va_gc_ (void *x_p)
{
  vec<rtx_insn*,va_gc> * const x = (vec<rtx_insn*,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_20vec_rtx_insn__va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct rtx_insn *& x)
{
  if (x)
    gt_pch_nx_rtx_def ((void *) x);
}

void
gt_pch_nx_vec_uchar_va_gc_ (void *x_p)
{
  vec<uchar,va_gc> * const x = (vec<uchar,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_16vec_uchar_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx_vec_call_site_record_va_gc_ (void *x_p)
{
  vec<call_site_record,va_gc> * const x = (vec<call_site_record,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_27vec_call_site_record_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct call_site_record_d *& x)
{
  if (x)
    gt_pch_nx_call_site_record_d ((void *) x);
}

void
gt_pch_nx_gimple_df (void *x_p)
{
  struct gimple_df * const x = (struct gimple_df *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_9gimple_df))
    {
      gt_pch_n_15vec_tree_va_gc_ ((*x).ssa_names);
      gt_pch_n_9tree_node ((*x).vop);
      gt_pch_n_11bitmap_head ((*x).escaped.vars);
      gt_pch_n_15vec_tree_va_gc_ ((*x).free_ssanames);
      gt_pch_n_15vec_tree_va_gc_ ((*x).free_ssanames_queue);
      gt_pch_n_27hash_table_ssa_name_hasher_ ((*x).default_defs);
      gt_pch_n_20ssa_operand_memory_d ((*x).ssa_operands.operand_memory);
      gt_pch_n_29hash_table_tm_restart_hasher_ ((*x).tm_restart);
    }
}

void
gt_pch_nx_dw_fde_node (void *x_p)
{
  struct dw_fde_node * const x = (struct dw_fde_node *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_11dw_fde_node))
    {
      gt_pch_n_9tree_node ((*x).decl);
      gt_pch_n_S ((*x).dw_fde_begin);
      gt_pch_n_S ((*x).dw_fde_current_label);
      gt_pch_n_S ((*x).dw_fde_end);
      gt_pch_n_S ((*x).dw_fde_vms_end_prologue);
      gt_pch_n_S ((*x).dw_fde_vms_begin_epilogue);
      gt_pch_n_S ((*x).dw_fde_second_begin);
      gt_pch_n_S ((*x).dw_fde_second_end);
      gt_pch_n_21vec_dw_cfi_ref_va_gc_ ((*x).dw_fde_cfi);
    }
}

void
gt_pch_nx_frame_space (void *x_p)
{
  struct frame_space * const x = (struct frame_space *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_11frame_space))
    {
      gt_pch_n_11frame_space ((*x).next);
    }
}

void
gt_pch_nx_vec_callinfo_callee_va_gc_ (void *x_p)
{
  vec<callinfo_callee,va_gc> * const x = (vec<callinfo_callee,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_26vec_callinfo_callee_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct callinfo_callee& x_r ATTRIBUTE_UNUSED)
{
  struct callinfo_callee * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).decl);
}

void
gt_pch_nx_vec_callinfo_dalloc_va_gc_ (void *x_p)
{
  vec<callinfo_dalloc,va_gc> * const x = (vec<callinfo_dalloc,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_26vec_callinfo_dalloc_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct callinfo_dalloc& x_r ATTRIBUTE_UNUSED)
{
  struct callinfo_dalloc * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_S ((*x).name);
}

void
gt_pch_nx_stack_usage (void *x_p)
{
  struct stack_usage * const x = (struct stack_usage *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_11stack_usage))
    {
      gt_pch_n_26vec_callinfo_callee_va_gc_ ((*x).callees);
      gt_pch_n_26vec_callinfo_dalloc_va_gc_ ((*x).dallocs);
    }
}

void
gt_pch_nx_eh_status (void *x_p)
{
  struct eh_status * const x = (struct eh_status *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_9eh_status))
    {
      gt_pch_n_11eh_region_d ((*x).region_tree);
      gt_pch_n_20vec_eh_region_va_gc_ ((*x).region_array);
      gt_pch_n_25vec_eh_landing_pad_va_gc_ ((*x).lp_array);
      gt_pch_n_21hash_map_gimple__int_ ((*x).throw_stmt_table);
      gt_pch_n_15vec_tree_va_gc_ ((*x).ttype_data);
      switch ((int) (targetm.arm_eabi_unwinder))
        {
        case 1:
          gt_pch_n_15vec_tree_va_gc_ ((*x).ehspec_data.arm_eabi);
          break;
        case 0:
          gt_pch_n_16vec_uchar_va_gc_ ((*x).ehspec_data.other);
          break;
        default:
          break;
        }
    }
}

void
gt_pch_nx_control_flow_graph (void *x_p)
{
  struct control_flow_graph * const x = (struct control_flow_graph *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_18control_flow_graph))
    {
      gt_pch_n_15basic_block_def ((*x).x_entry_block_ptr);
      gt_pch_n_15basic_block_def ((*x).x_exit_block_ptr);
      gt_pch_n_22vec_basic_block_va_gc_ ((*x).x_basic_block_info);
      gt_pch_n_22vec_basic_block_va_gc_ ((*x).x_label_to_block_map);
    }
}

void
gt_pch_nx_loops (void *x_p)
{
  struct loops * const x = (struct loops *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_5loops))
    {
      gt_pch_n_17vec_loop_p_va_gc_ ((*x).larray);
      gt_pch_n_28hash_table_loop_exit_hasher_ ((*x).exits);
      gt_pch_n_4loop ((*x).tree_root);
    }
}

void
gt_pch_nx_hash_set_tree_ (void *x_p)
{
  hash_set<tree> * const x = (hash_set<tree> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_14hash_set_tree_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx_types_used_by_vars_entry (void *x_p)
{
  struct types_used_by_vars_entry * const x = (struct types_used_by_vars_entry *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_24types_used_by_vars_entry))
    {
      gt_pch_n_9tree_node ((*x).type);
      gt_pch_n_9tree_node ((*x).var_decl);
    }
}

void
gt_pch_nx (struct types_used_by_vars_entry& x_r ATTRIBUTE_UNUSED)
{
  struct types_used_by_vars_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).type);
  gt_pch_n_9tree_node ((*x).var_decl);
}

void
gt_pch_nx (struct types_used_by_vars_entry *& x)
{
  if (x)
    gt_pch_nx_types_used_by_vars_entry ((void *) x);
}

void
gt_pch_nx_hash_table_used_type_hasher_ (void *x_p)
{
  hash_table<used_type_hasher> * const x = (hash_table<used_type_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_28hash_table_used_type_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct used_type_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct used_type_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_nb_iter_bound (void *x_p)
{
  struct nb_iter_bound * x = (struct nb_iter_bound *)x_p;
  struct nb_iter_bound * xlimit = x;
  while (gt_pch_note_object (xlimit, xlimit, gt_pch_p_13nb_iter_bound))
   xlimit = ((*xlimit).next);
  while (x != xlimit)
    {
      gt_pch_n_6gimple ((*x).stmt);
      gt_pch_n_13nb_iter_bound ((*x).next);
      x = ((*x).next);
    }
}

void
gt_pch_nx_loop_exit (void *x_p)
{
  struct loop_exit * const x = (struct loop_exit *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_9loop_exit))
    {
      gt_pch_n_8edge_def ((*x).e);
      gt_pch_n_9loop_exit ((*x).prev);
      gt_pch_n_9loop_exit ((*x).next);
      gt_pch_n_9loop_exit ((*x).next_e);
    }
}

void
gt_pch_nx (struct loop_exit& x_r ATTRIBUTE_UNUSED)
{
  struct loop_exit * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_8edge_def ((*x).e);
  gt_pch_n_9loop_exit ((*x).prev);
  gt_pch_n_9loop_exit ((*x).next);
  gt_pch_n_9loop_exit ((*x).next_e);
}

void
gt_pch_nx (struct loop_exit *& x)
{
  if (x)
    gt_pch_nx_loop_exit ((void *) x);
}

void
gt_pch_nx_loop (void *x_p)
{
  struct loop * x = (struct loop *)x_p;
  struct loop * xlimit = x;
  while (gt_pch_note_object (xlimit, xlimit, gt_pch_p_4loop))
   xlimit = ((*xlimit).next);
  while (x != xlimit)
    {
      gt_pch_n_15basic_block_def ((*x).header);
      gt_pch_n_15basic_block_def ((*x).latch);
      gt_pch_n_17vec_loop_p_va_gc_ ((*x).superloops);
      gt_pch_n_4loop ((*x).inner);
      gt_pch_n_4loop ((*x).next);
      gt_pch_n_9tree_node ((*x).nb_iterations);
      gt_pch_n_9tree_node ((*x).simduid);
      gt_pch_n_13nb_iter_bound ((*x).bounds);
      gt_pch_n_10control_iv ((*x).control_ivs);
      gt_pch_n_9loop_exit ((*x).exits);
      gt_pch_n_10niter_desc ((*x).simple_loop_desc);
      gt_pch_n_15basic_block_def ((*x).former_header);
      x = ((*x).next);
    }
}

void
gt_pch_nx_control_iv (void *x_p)
{
  struct control_iv * x = (struct control_iv *)x_p;
  struct control_iv * xlimit = x;
  while (gt_pch_note_object (xlimit, xlimit, gt_pch_p_10control_iv))
   xlimit = ((*xlimit).next);
  while (x != xlimit)
    {
      gt_pch_n_9tree_node ((*x).base);
      gt_pch_n_9tree_node ((*x).step);
      gt_pch_n_10control_iv ((*x).next);
      x = ((*x).next);
    }
}

void
gt_pch_nx_vec_loop_p_va_gc_ (void *x_p)
{
  vec<loop_p,va_gc> * const x = (vec<loop_p,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_17vec_loop_p_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct loop *& x)
{
  if (x)
    gt_pch_nx_loop ((void *) x);
}

void
gt_pch_nx_niter_desc (void *x_p)
{
  struct niter_desc * const x = (struct niter_desc *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_10niter_desc))
    {
      gt_pch_n_8edge_def ((*x).out_edge);
      gt_pch_n_8edge_def ((*x).in_edge);
      gt_pch_n_7rtx_def ((*x).assumptions);
      gt_pch_n_7rtx_def ((*x).noloop_assumptions);
      gt_pch_n_7rtx_def ((*x).infinite);
      gt_pch_n_7rtx_def ((*x).niter_expr);
    }
}

void
gt_pch_nx_hash_table_loop_exit_hasher_ (void *x_p)
{
  hash_table<loop_exit_hasher> * const x = (hash_table<loop_exit_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_28hash_table_loop_exit_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct loop_exit_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct loop_exit_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_vec_basic_block_va_gc_ (void *x_p)
{
  vec<basic_block,va_gc> * const x = (vec<basic_block,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_22vec_basic_block_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct basic_block_def *& x)
{
  if (x)
    gt_pch_nx_basic_block_def ((void *) x);
}

void
gt_pch_nx_rtl_bb_info (void *x_p)
{
  struct rtl_bb_info * const x = (struct rtl_bb_info *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_11rtl_bb_info))
    {
      gt_pch_n_7rtx_def ((*x).end_);
      gt_pch_n_7rtx_def ((*x).header_);
      gt_pch_n_7rtx_def ((*x).footer_);
    }
}

void
gt_pch_nx_vec_edge_va_gc_ (void *x_p)
{
  vec<edge,va_gc> * const x = (vec<edge,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_15vec_edge_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (edge_def *& x)
{
  if (x)
    gt_pch_nx_edge_def ((void *) x);
}

void
gt_pch_nx_vec_ipa_ref_t_va_gc_ (void *x_p)
{
  vec<ipa_ref_t,va_gc> * const x = (vec<ipa_ref_t,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_20vec_ipa_ref_t_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct ipa_ref& x_r ATTRIBUTE_UNUSED)
{
  struct ipa_ref * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_11symtab_node ((*x).referring);
  gt_pch_n_11symtab_node ((*x).referred);
  gt_pch_n_6gimple ((*x).stmt);
}

void
gt_pch_nx_section_hash_entry (void *x_p)
{
  struct section_hash_entry * const x = (struct section_hash_entry *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_18section_hash_entry))
    {
      gt_pch_n_S ((*x).name);
    }
}

void
gt_pch_nx (struct section_hash_entry& x_r ATTRIBUTE_UNUSED)
{
  struct section_hash_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_S ((*x).name);
}

void
gt_pch_nx (struct section_hash_entry *& x)
{
  if (x)
    gt_pch_nx_section_hash_entry ((void *) x);
}

void
gt_pch_nx_lto_file_decl_data (void *x_p)
{
  struct lto_file_decl_data * const x = (struct lto_file_decl_data *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_18lto_file_decl_data))
    {
      gt_pch_n_17lto_in_decl_state ((*x).current_decl_state);
      gt_pch_n_17lto_in_decl_state ((*x).global_decl_state);
      gt_pch_n_29hash_table_decl_state_hasher_ ((*x).function_decl_states);
      gt_pch_n_18lto_file_decl_data ((*x).next);
      gt_pch_n_S ((*x).mode_table);
    }
}

void
gt_pch_nx_ipa_replace_map (void *x_p)
{
  struct ipa_replace_map * const x = (struct ipa_replace_map *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_15ipa_replace_map))
    {
      gt_pch_n_9tree_node ((*x).new_tree);
    }
}

void
gt_pch_nx_vec_ipa_replace_map__va_gc_ (void *x_p)
{
  vec<ipa_replace_map*,va_gc> * const x = (vec<ipa_replace_map*,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_27vec_ipa_replace_map__va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct ipa_replace_map *& x)
{
  if (x)
    gt_pch_nx_ipa_replace_map ((void *) x);
}

void
gt_pch_nx_ipa_param_adjustments (void *x_p)
{
  struct ipa_param_adjustments * const x = (struct ipa_param_adjustments *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_21ipa_param_adjustments))
    {
      gt_pch_n_29vec_ipa_adjusted_param_va_gc_ ((*x).m_adj_params);
    }
}

void
gt_pch_nx_vec_ipa_param_performed_split_va_gc_ (void *x_p)
{
  vec<ipa_param_performed_split,va_gc> * const x = (vec<ipa_param_performed_split,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_36vec_ipa_param_performed_split_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct ipa_param_performed_split& x_r ATTRIBUTE_UNUSED)
{
  struct ipa_param_performed_split * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).dummy_decl);
}

void
gt_pch_nx_cgraph_simd_clone (void *x_p)
{
  struct cgraph_simd_clone * const x = (struct cgraph_simd_clone *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_17cgraph_simd_clone))
    {
      {
        size_t l0 = (size_t)(((*x)).nargs);
        gt_pch_n_11symtab_node ((*x).prev_clone);
        gt_pch_n_11symtab_node ((*x).next_clone);
        gt_pch_n_11symtab_node ((*x).origin);
        {
          size_t i0;
          for (i0 = 0; i0 != l0; i0++) {
            gt_pch_n_9tree_node ((*x).args[i0].orig_arg);
            gt_pch_n_9tree_node ((*x).args[i0].orig_type);
            gt_pch_n_9tree_node ((*x).args[i0].vector_arg);
            gt_pch_n_9tree_node ((*x).args[i0].vector_type);
            gt_pch_n_9tree_node ((*x).args[i0].simd_array);
          }
        }
      }
    }
}

void
gt_pch_nx_cgraph_function_version_info (void *x_p)
{
  struct cgraph_function_version_info * const x = (struct cgraph_function_version_info *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_28cgraph_function_version_info))
    {
      gt_pch_n_11symtab_node ((*x).this_node);
      gt_pch_n_28cgraph_function_version_info ((*x).prev);
      gt_pch_n_28cgraph_function_version_info ((*x).next);
      gt_pch_n_9tree_node ((*x).dispatcher_resolver);
    }
}

void
gt_pch_nx (struct cgraph_function_version_info& x_r ATTRIBUTE_UNUSED)
{
  struct cgraph_function_version_info * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_11symtab_node ((*x).this_node);
  gt_pch_n_28cgraph_function_version_info ((*x).prev);
  gt_pch_n_28cgraph_function_version_info ((*x).next);
  gt_pch_n_9tree_node ((*x).dispatcher_resolver);
}

void
gt_pch_nx (struct cgraph_function_version_info *& x)
{
  if (x)
    gt_pch_nx_cgraph_function_version_info ((void *) x);
}

void
gt_pch_nx_hash_table_cgraph_edge_hasher_ (void *x_p)
{
  hash_table<cgraph_edge_hasher> * const x = (hash_table<cgraph_edge_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_30hash_table_cgraph_edge_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct cgraph_edge_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct cgraph_edge_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_cgraph_indirect_call_info (void *x_p)
{
  struct cgraph_indirect_call_info * const x = (struct cgraph_indirect_call_info *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_25cgraph_indirect_call_info))
    {
      gt_pch_n_9tree_node ((*x).context.outer_type);
      gt_pch_n_9tree_node ((*x).context.speculative_outer_type);
      gt_pch_n_9tree_node ((*x).otr_type);
    }
}

void
gt_pch_nx_asm_node (void *x_p)
{
  struct asm_node * const x = (struct asm_node *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_8asm_node))
    {
      gt_pch_n_8asm_node ((*x).next);
      gt_pch_n_9tree_node ((*x).asm_str);
    }
}

void
gt_pch_nx_symbol_table (void *x_p)
{
  struct symbol_table * const x = (struct symbol_table *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_12symbol_table))
    {
      gt_pch_n_11symtab_node ((*x).nodes);
      gt_pch_n_8asm_node ((*x).asmnodes);
      gt_pch_n_8asm_node ((*x).asm_last_node);
      gt_pch_n_31hash_table_section_name_hasher_ ((*x).section_hash);
      gt_pch_n_26hash_table_asmname_hasher_ ((*x).assembler_name_hash);
      gt_pch_n_42hash_map_symtab_node__symbol_priority_map_ ((*x).init_priority_hash);
    }
}

void
gt_pch_nx_hash_table_section_name_hasher_ (void *x_p)
{
  hash_table<section_name_hasher> * const x = (hash_table<section_name_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_31hash_table_section_name_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct section_name_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct section_name_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_hash_table_asmname_hasher_ (void *x_p)
{
  hash_table<asmname_hasher> * const x = (hash_table<asmname_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_26hash_table_asmname_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct asmname_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct asmname_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_hash_map_symtab_node__symbol_priority_map_ (void *x_p)
{
  hash_map<symtab_node*,symbol_priority_map> * const x = (hash_map<symtab_node*,symbol_priority_map> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_42hash_map_symtab_node__symbol_priority_map_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct symbol_priority_map& x_r ATTRIBUTE_UNUSED)
{
  struct symbol_priority_map * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx (struct symtab_node *& x)
{
  if (x)
    gt_pch_nx_symtab_node ((void *) x);
}

void
gt_pch_nx_constant_descriptor_tree (void *x_p)
{
  struct constant_descriptor_tree * const x = (struct constant_descriptor_tree *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_24constant_descriptor_tree))
    {
      gt_pch_n_7rtx_def ((*x).rtl);
      gt_pch_n_9tree_node ((*x).value);
    }
}

void
gt_pch_nx (struct constant_descriptor_tree& x_r ATTRIBUTE_UNUSED)
{
  struct constant_descriptor_tree * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_7rtx_def ((*x).rtl);
  gt_pch_n_9tree_node ((*x).value);
}

void
gt_pch_nx (struct constant_descriptor_tree *& x)
{
  if (x)
    gt_pch_nx_constant_descriptor_tree ((void *) x);
}

void
gt_pch_nx_lto_in_decl_state (void *x_p)
{
  struct lto_in_decl_state * const x = (struct lto_in_decl_state *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_17lto_in_decl_state))
    {
      {
        size_t i0;
        size_t l0 = (size_t)(LTO_N_DECL_STREAMS);
        for (i0 = 0; i0 != l0; i0++) {
          gt_pch_n_15vec_tree_va_gc_ ((*x).streams[i0]);
        }
      }
      gt_pch_n_9tree_node ((*x).fn_decl);
    }
}

void
gt_pch_nx (struct lto_in_decl_state& x_r ATTRIBUTE_UNUSED)
{
  struct lto_in_decl_state * ATTRIBUTE_UNUSED x = &x_r;
  {
    size_t i1;
    size_t l1 = (size_t)(LTO_N_DECL_STREAMS);
    for (i1 = 0; i1 != l1; i1++) {
      gt_pch_n_15vec_tree_va_gc_ ((*x).streams[i1]);
    }
  }
  gt_pch_n_9tree_node ((*x).fn_decl);
}

void
gt_pch_nx (struct lto_in_decl_state *& x)
{
  if (x)
    gt_pch_nx_lto_in_decl_state ((void *) x);
}

void
gt_pch_nx_ipa_node_params (void *x_p)
{
  struct ipa_node_params * const x = (struct ipa_node_params *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_15ipa_node_params))
    {
      gt_pch_n_31vec_ipa_param_descriptor_va_gc_ ((*x).descriptors);
    }
}

void
gt_pch_nx (struct ipa_node_params& x_r ATTRIBUTE_UNUSED)
{
  struct ipa_node_params * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_31vec_ipa_param_descriptor_va_gc_ ((*x).descriptors);
}

void
gt_pch_nx (struct ipa_node_params *& x)
{
  if (x)
    gt_pch_nx_ipa_node_params ((void *) x);
}

void
gt_pch_nx_ipa_edge_args (void *x_p)
{
  struct ipa_edge_args * const x = (struct ipa_edge_args *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_13ipa_edge_args))
    {
      gt_pch_n_24vec_ipa_jump_func_va_gc_ ((*x).jump_functions);
      gt_pch_n_39vec_ipa_polymorphic_call_context_va_gc_ ((*x).polymorphic_call_contexts);
    }
}

void
gt_pch_nx (struct ipa_edge_args& x_r ATTRIBUTE_UNUSED)
{
  struct ipa_edge_args * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_24vec_ipa_jump_func_va_gc_ ((*x).jump_functions);
  gt_pch_n_39vec_ipa_polymorphic_call_context_va_gc_ ((*x).polymorphic_call_contexts);
}

void
gt_pch_nx (struct ipa_edge_args *& x)
{
  if (x)
    gt_pch_nx_ipa_edge_args ((void *) x);
}

void
gt_pch_nx_ipa_agg_replacement_value (void *x_p)
{
  struct ipa_agg_replacement_value * const x = (struct ipa_agg_replacement_value *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_25ipa_agg_replacement_value))
    {
      gt_pch_n_25ipa_agg_replacement_value ((*x).next);
      gt_pch_n_9tree_node ((*x).value);
    }
}

void
gt_pch_nx_ipa_fn_summary (void *x_p)
{
  struct ipa_fn_summary * const x = (struct ipa_fn_summary *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_14ipa_fn_summary))
    {
      gt_pch_n_20vec_condition_va_gc_ ((*x).conds);
      gt_pch_n_26vec_size_time_entry_va_gc_ ((*x).size_time_table);
      gt_pch_n_26vec_size_time_entry_va_gc_ ((*x).call_size_time_table);
    }
}

void
gt_pch_nx_vec_ipa_adjusted_param_va_gc_ (void *x_p)
{
  vec<ipa_adjusted_param,va_gc> * const x = (vec<ipa_adjusted_param,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_29vec_ipa_adjusted_param_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct ipa_adjusted_param& x_r ATTRIBUTE_UNUSED)
{
  struct ipa_adjusted_param * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).type);
  gt_pch_n_9tree_node ((*x).alias_ptr_type);
}

void
gt_pch_nx_dw_cfi_node (void *x_p)
{
  struct dw_cfi_node * const x = (struct dw_cfi_node *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_11dw_cfi_node))
    {
      switch ((int) (dw_cfi_oprnd1_desc (((*x)).dw_cfi_opc)))
        {
        case dw_cfi_oprnd_reg_num:
          break;
        case dw_cfi_oprnd_offset:
          break;
        case dw_cfi_oprnd_addr:
          gt_pch_n_S ((*x).dw_cfi_oprnd1.dw_cfi_addr);
          break;
        case dw_cfi_oprnd_loc:
          gt_pch_n_17dw_loc_descr_node ((*x).dw_cfi_oprnd1.dw_cfi_loc);
          break;
        case dw_cfi_oprnd_cfa_loc:
          gt_pch_n_15dw_cfa_location ((*x).dw_cfi_oprnd1.dw_cfi_cfa_loc);
          break;
        default:
          break;
        }
      switch ((int) (dw_cfi_oprnd2_desc (((*x)).dw_cfi_opc)))
        {
        case dw_cfi_oprnd_reg_num:
          break;
        case dw_cfi_oprnd_offset:
          break;
        case dw_cfi_oprnd_addr:
          gt_pch_n_S ((*x).dw_cfi_oprnd2.dw_cfi_addr);
          break;
        case dw_cfi_oprnd_loc:
          gt_pch_n_17dw_loc_descr_node ((*x).dw_cfi_oprnd2.dw_cfi_loc);
          break;
        case dw_cfi_oprnd_cfa_loc:
          gt_pch_n_15dw_cfa_location ((*x).dw_cfi_oprnd2.dw_cfi_cfa_loc);
          break;
        default:
          break;
        }
    }
}

void
gt_pch_nx_dw_loc_descr_node (void *x_p)
{
  struct dw_loc_descr_node * x = (struct dw_loc_descr_node *)x_p;
  struct dw_loc_descr_node * xlimit = x;
  while (gt_pch_note_object (xlimit, xlimit, gt_pch_p_17dw_loc_descr_node))
   xlimit = ((*xlimit).dw_loc_next);
  while (x != xlimit)
    {
      gt_pch_n_17dw_loc_descr_node ((*x).dw_loc_next);
      gt_pch_n_16addr_table_entry ((*x).dw_loc_oprnd1.val_entry);
      switch ((int) (((*x).dw_loc_oprnd1).val_class))
        {
        case dw_val_class_addr:
          gt_pch_n_7rtx_def ((*x).dw_loc_oprnd1.v.val_addr);
          break;
        case dw_val_class_offset:
          break;
        case dw_val_class_loc_list:
          gt_pch_n_18dw_loc_list_struct ((*x).dw_loc_oprnd1.v.val_loc_list);
          break;
        case dw_val_class_view_list:
          gt_pch_n_10die_struct ((*x).dw_loc_oprnd1.v.val_view_list);
          break;
        case dw_val_class_loc:
          gt_pch_n_17dw_loc_descr_node ((*x).dw_loc_oprnd1.v.val_loc);
          break;
        default:
          break;
        case dw_val_class_unsigned_const:
          break;
        case dw_val_class_const_double:
          break;
        case dw_val_class_wide_int:
          gt_pch_n_34generic_wide_int_wide_int_storage_ ((*x).dw_loc_oprnd1.v.val_wide);
          break;
        case dw_val_class_vec:
          if ((*x).dw_loc_oprnd1.v.val_vec.array != NULL) {
            gt_pch_note_object ((*x).dw_loc_oprnd1.v.val_vec.array, x, gt_pch_p_17dw_loc_descr_node);
          }
          break;
        case dw_val_class_die_ref:
          gt_pch_n_10die_struct ((*x).dw_loc_oprnd1.v.val_die_ref.die);
          break;
        case dw_val_class_fde_ref:
          break;
        case dw_val_class_str:
          gt_pch_n_20indirect_string_node ((*x).dw_loc_oprnd1.v.val_str);
          break;
        case dw_val_class_lbl_id:
          gt_pch_n_S ((*x).dw_loc_oprnd1.v.val_lbl_id);
          break;
        case dw_val_class_flag:
          break;
        case dw_val_class_file:
          gt_pch_n_15dwarf_file_data ((*x).dw_loc_oprnd1.v.val_file);
          break;
        case dw_val_class_file_implicit:
          gt_pch_n_15dwarf_file_data ((*x).dw_loc_oprnd1.v.val_file_implicit);
          break;
        case dw_val_class_data8:
          break;
        case dw_val_class_decl_ref:
          gt_pch_n_9tree_node ((*x).dw_loc_oprnd1.v.val_decl_ref);
          break;
        case dw_val_class_vms_delta:
          gt_pch_n_S ((*x).dw_loc_oprnd1.v.val_vms_delta.lbl1);
          gt_pch_n_S ((*x).dw_loc_oprnd1.v.val_vms_delta.lbl2);
          break;
        case dw_val_class_discr_value:
          switch ((int) (((*x).dw_loc_oprnd1.v.val_discr_value).pos))
            {
            case 0:
              break;
            case 1:
              break;
            default:
              break;
            }
          break;
        case dw_val_class_discr_list:
          gt_pch_n_18dw_discr_list_node ((*x).dw_loc_oprnd1.v.val_discr_list);
          break;
        case dw_val_class_symview:
          gt_pch_n_S ((*x).dw_loc_oprnd1.v.val_symbolic_view);
          break;
        }
      gt_pch_n_16addr_table_entry ((*x).dw_loc_oprnd2.val_entry);
      switch ((int) (((*x).dw_loc_oprnd2).val_class))
        {
        case dw_val_class_addr:
          gt_pch_n_7rtx_def ((*x).dw_loc_oprnd2.v.val_addr);
          break;
        case dw_val_class_offset:
          break;
        case dw_val_class_loc_list:
          gt_pch_n_18dw_loc_list_struct ((*x).dw_loc_oprnd2.v.val_loc_list);
          break;
        case dw_val_class_view_list:
          gt_pch_n_10die_struct ((*x).dw_loc_oprnd2.v.val_view_list);
          break;
        case dw_val_class_loc:
          gt_pch_n_17dw_loc_descr_node ((*x).dw_loc_oprnd2.v.val_loc);
          break;
        default:
          break;
        case dw_val_class_unsigned_const:
          break;
        case dw_val_class_const_double:
          break;
        case dw_val_class_wide_int:
          gt_pch_n_34generic_wide_int_wide_int_storage_ ((*x).dw_loc_oprnd2.v.val_wide);
          break;
        case dw_val_class_vec:
          if ((*x).dw_loc_oprnd2.v.val_vec.array != NULL) {
            gt_pch_note_object ((*x).dw_loc_oprnd2.v.val_vec.array, x, gt_pch_p_17dw_loc_descr_node);
          }
          break;
        case dw_val_class_die_ref:
          gt_pch_n_10die_struct ((*x).dw_loc_oprnd2.v.val_die_ref.die);
          break;
        case dw_val_class_fde_ref:
          break;
        case dw_val_class_str:
          gt_pch_n_20indirect_string_node ((*x).dw_loc_oprnd2.v.val_str);
          break;
        case dw_val_class_lbl_id:
          gt_pch_n_S ((*x).dw_loc_oprnd2.v.val_lbl_id);
          break;
        case dw_val_class_flag:
          break;
        case dw_val_class_file:
          gt_pch_n_15dwarf_file_data ((*x).dw_loc_oprnd2.v.val_file);
          break;
        case dw_val_class_file_implicit:
          gt_pch_n_15dwarf_file_data ((*x).dw_loc_oprnd2.v.val_file_implicit);
          break;
        case dw_val_class_data8:
          break;
        case dw_val_class_decl_ref:
          gt_pch_n_9tree_node ((*x).dw_loc_oprnd2.v.val_decl_ref);
          break;
        case dw_val_class_vms_delta:
          gt_pch_n_S ((*x).dw_loc_oprnd2.v.val_vms_delta.lbl1);
          gt_pch_n_S ((*x).dw_loc_oprnd2.v.val_vms_delta.lbl2);
          break;
        case dw_val_class_discr_value:
          switch ((int) (((*x).dw_loc_oprnd2.v.val_discr_value).pos))
            {
            case 0:
              break;
            case 1:
              break;
            default:
              break;
            }
          break;
        case dw_val_class_discr_list:
          gt_pch_n_18dw_discr_list_node ((*x).dw_loc_oprnd2.v.val_discr_list);
          break;
        case dw_val_class_symview:
          gt_pch_n_S ((*x).dw_loc_oprnd2.v.val_symbolic_view);
          break;
        }
      x = ((*x).dw_loc_next);
    }
}

void
gt_pch_nx_dw_discr_list_node (void *x_p)
{
  struct dw_discr_list_node * const x = (struct dw_discr_list_node *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_18dw_discr_list_node))
    {
      gt_pch_n_18dw_discr_list_node ((*x).dw_discr_next);
      switch ((int) (((*x).dw_discr_lower_bound).pos))
        {
        case 0:
          break;
        case 1:
          break;
        default:
          break;
        }
      switch ((int) (((*x).dw_discr_upper_bound).pos))
        {
        case 0:
          break;
        case 1:
          break;
        default:
          break;
        }
    }
}

void
gt_pch_nx_vec_dw_cfi_ref_va_gc_ (void *x_p)
{
  vec<dw_cfi_ref,va_gc> * const x = (vec<dw_cfi_ref,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_21vec_dw_cfi_ref_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct dw_cfi_node *& x)
{
  if (x)
    gt_pch_nx_dw_cfi_node ((void *) x);
}

void
gt_pch_nx_vec_temp_slot_p_va_gc_ (void *x_p)
{
  vec<temp_slot_p,va_gc> * const x = (vec<temp_slot_p,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_22vec_temp_slot_p_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct temp_slot *& x)
{
  if (x)
    gt_pch_nx_temp_slot ((void *) x);
}

void
gt_pch_nx_eh_region_d (void *x_p)
{
  struct eh_region_d * const x = (struct eh_region_d *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_11eh_region_d))
    {
      gt_pch_n_11eh_region_d ((*x).outer);
      gt_pch_n_11eh_region_d ((*x).inner);
      gt_pch_n_11eh_region_d ((*x).next_peer);
      switch ((int) ((*x).type))
        {
        case ERT_TRY:
          gt_pch_n_10eh_catch_d ((*x).u.eh_try.first_catch);
          gt_pch_n_10eh_catch_d ((*x).u.eh_try.last_catch);
          break;
        case ERT_ALLOWED_EXCEPTIONS:
          gt_pch_n_9tree_node ((*x).u.allowed.type_list);
          gt_pch_n_9tree_node ((*x).u.allowed.label);
          break;
        case ERT_MUST_NOT_THROW:
          gt_pch_n_9tree_node ((*x).u.must_not_throw.failure_decl);
          break;
        default:
          break;
        }
      gt_pch_n_16eh_landing_pad_d ((*x).landing_pads);
      gt_pch_n_7rtx_def ((*x).exc_ptr_reg);
      gt_pch_n_7rtx_def ((*x).filter_reg);
    }
}

void
gt_pch_nx_eh_landing_pad_d (void *x_p)
{
  struct eh_landing_pad_d * const x = (struct eh_landing_pad_d *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_16eh_landing_pad_d))
    {
      gt_pch_n_16eh_landing_pad_d ((*x).next_lp);
      gt_pch_n_11eh_region_d ((*x).region);
      gt_pch_n_9tree_node ((*x).post_landing_pad);
      gt_pch_n_7rtx_def ((*x).landing_pad);
    }
}

void
gt_pch_nx_eh_catch_d (void *x_p)
{
  struct eh_catch_d * const x = (struct eh_catch_d *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_10eh_catch_d))
    {
      gt_pch_n_10eh_catch_d ((*x).next_catch);
      gt_pch_n_10eh_catch_d ((*x).prev_catch);
      gt_pch_n_9tree_node ((*x).type_list);
      gt_pch_n_9tree_node ((*x).filter_list);
      gt_pch_n_9tree_node ((*x).label);
    }
}

void
gt_pch_nx_vec_eh_region_va_gc_ (void *x_p)
{
  vec<eh_region,va_gc> * const x = (vec<eh_region,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_20vec_eh_region_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct eh_region_d *& x)
{
  if (x)
    gt_pch_nx_eh_region_d ((void *) x);
}

void
gt_pch_nx_vec_eh_landing_pad_va_gc_ (void *x_p)
{
  vec<eh_landing_pad,va_gc> * const x = (vec<eh_landing_pad,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_25vec_eh_landing_pad_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct eh_landing_pad_d *& x)
{
  if (x)
    gt_pch_nx_eh_landing_pad_d ((void *) x);
}

void
gt_pch_nx_hash_map_gimple__int_ (void *x_p)
{
  hash_map<gimple*,int> * const x = (hash_map<gimple*,int> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_21hash_map_gimple__int_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct gimple *& x)
{
  if (x)
    gt_pch_nx_gimple ((void *) x);
}

void
gt_pch_nx_tm_restart_node (void *x_p)
{
  struct tm_restart_node * const x = (struct tm_restart_node *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_15tm_restart_node))
    {
      gt_pch_n_6gimple ((*x).stmt);
      gt_pch_n_9tree_node ((*x).label_or_list);
    }
}

void
gt_pch_nx (struct tm_restart_node& x_r ATTRIBUTE_UNUSED)
{
  struct tm_restart_node * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_6gimple ((*x).stmt);
  gt_pch_n_9tree_node ((*x).label_or_list);
}

void
gt_pch_nx (struct tm_restart_node *& x)
{
  if (x)
    gt_pch_nx_tm_restart_node ((void *) x);
}

void
gt_pch_nx_hash_map_tree_tree_ (void *x_p)
{
  hash_map<tree,tree> * const x = (hash_map<tree,tree> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_19hash_map_tree_tree_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx_hash_table_ssa_name_hasher_ (void *x_p)
{
  hash_table<ssa_name_hasher> * const x = (hash_table<ssa_name_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_27hash_table_ssa_name_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct ssa_name_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct ssa_name_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_hash_table_tm_restart_hasher_ (void *x_p)
{
  hash_table<tm_restart_hasher> * const x = (hash_table<tm_restart_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_29hash_table_tm_restart_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct tm_restart_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct tm_restart_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_ssa_operand_memory_d (void *x_p)
{
  struct ssa_operand_memory_d * x = (struct ssa_operand_memory_d *)x_p;
  struct ssa_operand_memory_d * xlimit = x;
  while (gt_pch_note_object (xlimit, xlimit, gt_pch_p_20ssa_operand_memory_d))
   xlimit = ((*xlimit).next);
  while (x != xlimit)
    {
      gt_pch_n_20ssa_operand_memory_d ((*x).next);
      x = ((*x).next);
    }
}

void
gt_pch_nx_vec_ipa_agg_jf_item_va_gc_ (void *x_p)
{
  vec<ipa_agg_jf_item,va_gc> * const x = (vec<ipa_agg_jf_item,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_26vec_ipa_agg_jf_item_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct ipa_agg_jf_item& x_r ATTRIBUTE_UNUSED)
{
  struct ipa_agg_jf_item * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).type);
  switch ((int) (((*x)).jftype))
    {
    case IPA_JF_CONST:
      gt_pch_n_9tree_node ((*x).value.constant);
      break;
    case IPA_JF_PASS_THROUGH:
      gt_pch_n_9tree_node ((*x).value.pass_through.operand);
      break;
    case IPA_JF_LOAD_AGG:
      gt_pch_n_9tree_node ((*x).value.load_agg.pass_through.operand);
      gt_pch_n_9tree_node ((*x).value.load_agg.type);
      break;
    default:
      break;
    }
}

void
gt_pch_nx_ipa_bits (void *x_p)
{
  struct ipa_bits * const x = (struct ipa_bits *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_8ipa_bits))
    {
    }
}

void
gt_pch_nx_value_range (void *x_p)
{
  struct value_range * const x = (struct value_range *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_11value_range))
    {
      gt_pch_n_9tree_node ((*x).m_min);
      gt_pch_n_9tree_node ((*x).m_max);
    }
}

void
gt_pch_nx (struct value_range& x_r ATTRIBUTE_UNUSED)
{
  struct value_range * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).m_min);
  gt_pch_n_9tree_node ((*x).m_max);
}

void
gt_pch_nx (struct value_range *& x)
{
  if (x)
    gt_pch_nx_value_range ((void *) x);
}

void
gt_pch_nx_vec_ipa_param_descriptor_va_gc_ (void *x_p)
{
  vec<ipa_param_descriptor,va_gc> * const x = (vec<ipa_param_descriptor,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_31vec_ipa_param_descriptor_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct ipa_param_descriptor& x_r ATTRIBUTE_UNUSED)
{
  struct ipa_param_descriptor * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).decl_or_type);
}

void
gt_pch_nx_vec_ipa_bits__va_gc_ (void *x_p)
{
  vec<ipa_bits*,va_gc> * const x = (vec<ipa_bits*,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_20vec_ipa_bits__va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct ipa_bits *& x)
{
  if (x)
    gt_pch_nx_ipa_bits ((void *) x);
}

void
gt_pch_nx_vec_ipa_vr_va_gc_ (void *x_p)
{
  vec<ipa_vr,va_gc> * const x = (vec<ipa_vr,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_17vec_ipa_vr_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct ipa_vr& x_r ATTRIBUTE_UNUSED)
{
  struct ipa_vr * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_nx (&((*x).min));
  gt_pch_nx (&((*x).max));
}

void
gt_pch_nx_ipcp_transformation (void *x_p)
{
  struct ipcp_transformation * const x = (struct ipcp_transformation *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_19ipcp_transformation))
    {
      gt_pch_n_25ipa_agg_replacement_value ((*x).agg_values);
      gt_pch_n_20vec_ipa_bits__va_gc_ ((*x).bits);
      gt_pch_n_17vec_ipa_vr_va_gc_ ((*x).m_vr);
    }
}

void
gt_pch_nx_vec_ipa_jump_func_va_gc_ (void *x_p)
{
  vec<ipa_jump_func,va_gc> * const x = (vec<ipa_jump_func,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_24vec_ipa_jump_func_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct ipa_jump_func& x_r ATTRIBUTE_UNUSED)
{
  struct ipa_jump_func * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_26vec_ipa_agg_jf_item_va_gc_ ((*x).agg.items);
  gt_pch_n_8ipa_bits ((*x).bits);
  gt_pch_n_11value_range ((*x).m_vr);
  switch ((int) (((*x)).type))
    {
    case IPA_JF_CONST:
      gt_pch_n_9tree_node ((*x).value.constant.value);
      break;
    case IPA_JF_PASS_THROUGH:
      gt_pch_n_9tree_node ((*x).value.pass_through.operand);
      break;
    case IPA_JF_ANCESTOR:
      break;
    default:
      break;
    }
}

void
gt_pch_nx_vec_ipa_polymorphic_call_context_va_gc_ (void *x_p)
{
  vec<ipa_polymorphic_call_context,va_gc> * const x = (vec<ipa_polymorphic_call_context,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_39vec_ipa_polymorphic_call_context_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct ipa_polymorphic_call_context& x_r ATTRIBUTE_UNUSED)
{
  struct ipa_polymorphic_call_context * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).outer_type);
  gt_pch_n_9tree_node ((*x).speculative_outer_type);
}

void
gt_pch_nx_ipa_node_params_t (void *x_p)
{
  ipa_node_params_t * const x = (ipa_node_params_t *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_17ipa_node_params_t))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx_ipa_edge_args_sum_t (void *x_p)
{
  ipa_edge_args_sum_t * const x = (ipa_edge_args_sum_t *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_19ipa_edge_args_sum_t))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx_function_summary_ipcp_transformation__ (void *x_p)
{
  function_summary<ipcp_transformation*> * const x = (function_summary<ipcp_transformation*> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_38function_summary_ipcp_transformation__))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct ipcp_transformation *& x)
{
  if (x)
    gt_pch_nx_ipcp_transformation ((void *) x);
}

void
gt_pch_nx_hash_table_decl_state_hasher_ (void *x_p)
{
  hash_table<decl_state_hasher> * const x = (hash_table<decl_state_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_29hash_table_decl_state_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct decl_state_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct decl_state_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_vec_expr_eval_op_va_gc_ (void *x_p)
{
  vec<expr_eval_op,va_gc> * const x = (vec<expr_eval_op,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_23vec_expr_eval_op_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct expr_eval_op& x_r ATTRIBUTE_UNUSED)
{
  struct expr_eval_op * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).type);
  {
    size_t i0;
    size_t l0 = (size_t)(2);
    for (i0 = 0; i0 != l0; i0++) {
      gt_pch_n_9tree_node ((*x).val[i0]);
    }
  }
}

void
gt_pch_nx_vec_condition_va_gc_ (void *x_p)
{
  vec<condition,va_gc> * const x = (vec<condition,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_20vec_condition_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct condition& x_r ATTRIBUTE_UNUSED)
{
  struct condition * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).type);
  gt_pch_n_9tree_node ((*x).val);
  gt_pch_n_23vec_expr_eval_op_va_gc_ ((*x).param_ops);
}

void
gt_pch_nx_vec_size_time_entry_va_gc_ (void *x_p)
{
  vec<size_time_entry,va_gc> * const x = (vec<size_time_entry,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_26vec_size_time_entry_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct size_time_entry& x_r ATTRIBUTE_UNUSED)
{
  struct size_time_entry * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_fast_function_summary_ipa_fn_summary__va_gc_ (void *x_p)
{
  fast_function_summary<ipa_fn_summary*,va_gc> * const x = (fast_function_summary<ipa_fn_summary*,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_44fast_function_summary_ipa_fn_summary__va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct ipa_fn_summary *& x)
{
  if (x)
    gt_pch_nx_ipa_fn_summary ((void *) x);
}

void
gt_pch_p_9line_maps (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct line_maps * x ATTRIBUTE_UNUSED = (struct line_maps *)x_p;
  {
    size_t l0 = (size_t)(((*x).info_ordinary).used);
    if ((*x).info_ordinary.maps != NULL) {
      size_t i0;
      for (i0 = 0; i0 != (size_t)(l0) && ((void *)(*x).info_ordinary.maps == this_obj); i0++) {
        if ((void *)((*x).info_ordinary.maps) == this_obj)
          op (&((*x).info_ordinary.maps[i0].to_file), cookie);
      }
      if ((void *)(x) == this_obj)
        op (&((*x).info_ordinary.maps), cookie);
    }
  }
  {
    size_t l1 = (size_t)(((*x).info_macro).used);
    if ((*x).info_macro.maps != NULL) {
      size_t i1;
      for (i1 = 0; i1 != (size_t)(l1) && ((void *)(*x).info_macro.maps == this_obj); i1++) {
        {
          union tree_node * x2 =
            ((*x).info_macro.maps[i1].macro) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).info_macro.maps[i1].macro))) : NULL;
          if ((void *)((*x).info_macro.maps) == this_obj)
            op (&(x2), cookie);
          (*x).info_macro.maps[i1].macro = (x2) ? CPP_HASHNODE (GCC_IDENT_TO_HT_IDENT ((x2))) : NULL;
        }
        if ((*x).info_macro.maps[i1].macro_locations != NULL) {
          if ((void *)((*x).info_macro.maps) == this_obj)
            op (&((*x).info_macro.maps[i1].macro_locations), cookie);
        }
      }
      if ((void *)(x) == this_obj)
        op (&((*x).info_macro.maps), cookie);
    }
  }
  {
    size_t l3 = (size_t)(((*x).location_adhoc_data_map).allocated);
    if ((*x).location_adhoc_data_map.data != NULL) {
      size_t i3;
      for (i3 = 0; i3 != (size_t)(l3) && ((void *)(*x).location_adhoc_data_map.data == this_obj); i3++) {
      }
      if ((void *)(x) == this_obj)
        op (&((*x).location_adhoc_data_map.data), cookie);
    }
  }
}

void
gt_pch_p_9cpp_token (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct cpp_token * x ATTRIBUTE_UNUSED = (struct cpp_token *)x_p;
  switch ((int) (cpp_token_val_index (&((*x)))))
    {
    case CPP_TOKEN_FLD_NODE:
      {
        union tree_node * x0 =
          ((*x).val.node.node) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).val.node.node))) : NULL;
        if ((void *)(x) == this_obj)
          op (&(x0), cookie);
        (*x).val.node.node = (x0) ? CPP_HASHNODE (GCC_IDENT_TO_HT_IDENT ((x0))) : NULL;
      }
      {
        union tree_node * x1 =
          ((*x).val.node.spelling) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).val.node.spelling))) : NULL;
        if ((void *)(x) == this_obj)
          op (&(x1), cookie);
        (*x).val.node.spelling = (x1) ? CPP_HASHNODE (GCC_IDENT_TO_HT_IDENT ((x1))) : NULL;
      }
      break;
    case CPP_TOKEN_FLD_SOURCE:
      if ((void *)(x) == this_obj)
        op (&((*x).val.source), cookie);
      break;
    case CPP_TOKEN_FLD_STR:
      if ((void *)(x) == this_obj)
        op (&((*x).val.str.text), cookie);
      break;
    case CPP_TOKEN_FLD_ARG_NO:
      {
        union tree_node * x2 =
          ((*x).val.macro_arg.spelling) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).val.macro_arg.spelling))) : NULL;
        if ((void *)(x) == this_obj)
          op (&(x2), cookie);
        (*x).val.macro_arg.spelling = (x2) ? CPP_HASHNODE (GCC_IDENT_TO_HT_IDENT ((x2))) : NULL;
      }
      break;
    case CPP_TOKEN_FLD_TOKEN_NO:
      break;
    case CPP_TOKEN_FLD_PRAGMA:
      break;
    default:
      break;
    }
}

void
gt_pch_p_9cpp_macro (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct cpp_macro * x ATTRIBUTE_UNUSED = (struct cpp_macro *)x_p;
  switch ((int) (((*x)).kind == cmk_assert))
    {
    case false:
      if ((*x).parm.params != NULL) {
        size_t i0;
        for (i0 = 0; i0 != (size_t)(((*x)).paramc) && ((void *)(*x).parm.params == this_obj); i0++) {
          {
            union tree_node * x1 =
              ((*x).parm.params[i0]) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).parm.params[i0]))) : NULL;
            if ((void *)((*x).parm.params) == this_obj)
              op (&(x1), cookie);
            (*x).parm.params[i0] = (x1) ? CPP_HASHNODE (GCC_IDENT_TO_HT_IDENT ((x1))) : NULL;
          }
        }
        if ((void *)(x) == this_obj)
          op (&((*x).parm.params), cookie);
      }
      break;
    case true:
      if ((void *)(x) == this_obj)
        op (&((*x).parm.next), cookie);
      break;
    default:
      break;
    }
  switch ((int) (((*x)).kind == cmk_traditional))
    {
    case false:
      {
        size_t i2;
        size_t l2 = (size_t)(((*x)).count);
        for (i2 = 0; i2 != l2; i2++) {
          switch ((int) (cpp_token_val_index (&((*x).exp.tokens[i2]))))
            {
            case CPP_TOKEN_FLD_NODE:
              {
                union tree_node * x3 =
                  ((*x).exp.tokens[i2].val.node.node) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).exp.tokens[i2].val.node.node))) : NULL;
                if ((void *)(x) == this_obj)
                  op (&(x3), cookie);
                (*x).exp.tokens[i2].val.node.node = (x3) ? CPP_HASHNODE (GCC_IDENT_TO_HT_IDENT ((x3))) : NULL;
              }
              {
                union tree_node * x4 =
                  ((*x).exp.tokens[i2].val.node.spelling) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).exp.tokens[i2].val.node.spelling))) : NULL;
                if ((void *)(x) == this_obj)
                  op (&(x4), cookie);
                (*x).exp.tokens[i2].val.node.spelling = (x4) ? CPP_HASHNODE (GCC_IDENT_TO_HT_IDENT ((x4))) : NULL;
              }
              break;
            case CPP_TOKEN_FLD_SOURCE:
              if ((void *)(x) == this_obj)
                op (&((*x).exp.tokens[i2].val.source), cookie);
              break;
            case CPP_TOKEN_FLD_STR:
              if ((void *)(x) == this_obj)
                op (&((*x).exp.tokens[i2].val.str.text), cookie);
              break;
            case CPP_TOKEN_FLD_ARG_NO:
              {
                union tree_node * x5 =
                  ((*x).exp.tokens[i2].val.macro_arg.spelling) ? HT_IDENT_TO_GCC_IDENT (HT_NODE (((*x).exp.tokens[i2].val.macro_arg.spelling))) : NULL;
                if ((void *)(x) == this_obj)
                  op (&(x5), cookie);
                (*x).exp.tokens[i2].val.macro_arg.spelling = (x5) ? CPP_HASHNODE (GCC_IDENT_TO_HT_IDENT ((x5))) : NULL;
              }
              break;
            case CPP_TOKEN_FLD_TOKEN_NO:
              break;
            case CPP_TOKEN_FLD_PRAGMA:
              break;
            default:
              break;
            }
        }
      }
      break;
    case true:
      if ((void *)(x) == this_obj)
        op (&((*x).exp.text), cookie);
      break;
    default:
      break;
    }
}

void
gt_pch_p_13string_concat (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct string_concat * x ATTRIBUTE_UNUSED = (struct string_concat *)x_p;
  if ((*x).m_locs != NULL) {
    if ((void *)(x) == this_obj)
      op (&((*x).m_locs), cookie);
  }
}

void
gt_pch_p_16string_concat_db (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct string_concat_db * x ATTRIBUTE_UNUSED = (struct string_concat_db *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).m_table), cookie);
}

void
gt_pch_p_38hash_map_location_hash_string_concat__ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_map<location_hash,string_concat*> * x ATTRIBUTE_UNUSED = (struct hash_map<location_hash,string_concat*> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct location_hash* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_11bitmap_head (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct bitmap_head * x ATTRIBUTE_UNUSED = (struct bitmap_head *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).first), cookie);
}

void
gt_pch_p_7rtx_def (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct rtx_def * x ATTRIBUTE_UNUSED = (struct rtx_def *)x_p;
  switch ((int) (0))
    {
    case 0:
      switch ((int) (GET_CODE (&(*x))))
        {
        case DEBUG_MARKER:
          break;
        case DEBUG_PARAMETER_REF:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_tree), cookie);
          break;
        case ENTRY_VALUE:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case DEBUG_IMPLICIT_PTR:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_tree), cookie);
          break;
        case VAR_LOCATION:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_tree), cookie);
          break;
        case FMA:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[2].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case US_TRUNCATE:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case SS_TRUNCATE:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case US_MINUS:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case US_ASHIFT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case SS_ASHIFT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case SS_ABS:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case US_NEG:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case SS_NEG:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case SS_MINUS:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case US_PLUS:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case SS_PLUS:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case VEC_SERIES:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case VEC_DUPLICATE:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case VEC_CONCAT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case VEC_SELECT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case VEC_MERGE:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[2].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case LO_SUM:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case HIGH:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case ZERO_EXTRACT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[2].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case SIGN_EXTRACT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[2].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case PARITY:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case POPCOUNT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case CTZ:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case CLZ:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case CLRSB:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case FFS:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case BSWAP:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case SQRT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case ABS:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case UNSIGNED_SAT_FRACT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case SAT_FRACT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case UNSIGNED_FRACT_CONVERT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case FRACT_CONVERT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case UNSIGNED_FIX:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case UNSIGNED_FLOAT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case FIX:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case FLOAT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case FLOAT_TRUNCATE:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case FLOAT_EXTEND:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case TRUNCATE:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case ZERO_EXTEND:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case SIGN_EXTEND:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case UNLT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case UNLE:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case UNGT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case UNGE:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case UNEQ:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case ORDERED:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case UNORDERED:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case LTU:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case LEU:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case GTU:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case GEU:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case LTGT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case LT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case LE:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case GT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case GE:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case EQ:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case NE:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case POST_MODIFY:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case PRE_MODIFY:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case POST_INC:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case POST_DEC:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case PRE_INC:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case PRE_DEC:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case UMAX:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case UMIN:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case SMAX:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case SMIN:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case ROTATERT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case LSHIFTRT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case ASHIFTRT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case ROTATE:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case ASHIFT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case NOT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case XOR:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case IOR:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case AND:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case UMOD:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case UDIV:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case MOD:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case US_DIV:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case SS_DIV:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case DIV:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case US_MULT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case SS_MULT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case MULT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case NEG:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case MINUS:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case PLUS:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case COMPARE:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case IF_THEN_ELSE:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[2].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case CC0:
          break;
        case SYMBOL_REF:
          switch ((int) (SYMBOL_REF_HAS_BLOCK_INFO_P (&(*x))))
            {
            case 1:
              if ((void *)(x) == this_obj)
                op (&((*x).u.block_sym.block), cookie);
              break;
            default:
              break;
            }
          switch ((int) (CONSTANT_POOL_ADDRESS_P (&(*x))))
            {
            case 1:
              if ((void *)(x) == this_obj)
                op (&((*x).u.fld[1].rt_constant), cookie);
              break;
            default:
              if ((void *)(x) == this_obj)
                op (&((*x).u.fld[1].rt_tree), cookie);
              break;
            }
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_str), cookie);
          break;
        case LABEL_REF:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case MEM:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_mem), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case CONCATN:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtvec), cookie);
          break;
        case CONCAT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case STRICT_LOW_PART:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case SUBREG:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case SCRATCH:
          break;
        case REG:
          if ((void *)(x) == this_obj)
            op (&((*x).u.reg.attrs), cookie);
          break;
        case PC:
          break;
        case CONST:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case CONST_STRING:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_str), cookie);
          break;
        case CONST_VECTOR:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtvec), cookie);
          break;
        case CONST_DOUBLE:
          break;
        case CONST_FIXED:
          break;
        case CONST_POLY_INT:
          break;
        case CONST_WIDE_INT:
          break;
        case CONST_INT:
          break;
        case TRAP_IF:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case EH_RETURN:
          break;
        case SIMPLE_RETURN:
          break;
        case RETURN:
          break;
        case CALL:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case CLOBBER:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case USE:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case SET:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case PREFETCH:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[2].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case ADDR_DIFF_VEC:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[3].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[2].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtvec), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case ADDR_VEC:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtvec), cookie);
          break;
        case UNSPEC_VOLATILE:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtvec), cookie);
          break;
        case UNSPEC:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtvec), cookie);
          break;
        case ASM_OPERANDS:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[5].rt_rtvec), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[4].rt_rtvec), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[3].rt_rtvec), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_str), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_str), cookie);
          break;
        case ASM_INPUT:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_str), cookie);
          break;
        case PARALLEL:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtvec), cookie);
          break;
        case COND_EXEC:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case NOTE:
          switch ((int) (NOTE_KIND (&(*x))))
            {
            default:
              if ((void *)(x) == this_obj)
                op (&((*x).u.fld[3].rt_str), cookie);
              break;
            case NOTE_INSN_UPDATE_SJLJ_CONTEXT:
              break;
            case NOTE_INSN_CFI_LABEL:
              break;
            case NOTE_INSN_CFI:
              break;
            case NOTE_INSN_SWITCH_TEXT_SECTIONS:
              break;
            case NOTE_INSN_BASIC_BLOCK:
              break;
            case NOTE_INSN_INLINE_ENTRY:
              break;
            case NOTE_INSN_BEGIN_STMT:
              break;
            case NOTE_INSN_VAR_LOCATION:
              if ((void *)(x) == this_obj)
                op (&((*x).u.fld[3].rt_rtx), cookie);
              break;
            case NOTE_INSN_EH_REGION_END:
              break;
            case NOTE_INSN_EH_REGION_BEG:
              break;
            case NOTE_INSN_EPILOGUE_BEG:
              break;
            case NOTE_INSN_PROLOGUE_END:
              break;
            case NOTE_INSN_FUNCTION_BEG:
              break;
            case NOTE_INSN_BLOCK_END:
              if ((void *)(x) == this_obj)
                op (&((*x).u.fld[3].rt_tree), cookie);
              break;
            case NOTE_INSN_BLOCK_BEG:
              if ((void *)(x) == this_obj)
                op (&((*x).u.fld[3].rt_tree), cookie);
              break;
            case NOTE_INSN_DELETED_DEBUG_LABEL:
              if ((void *)(x) == this_obj)
                op (&((*x).u.fld[3].rt_str), cookie);
              break;
            case NOTE_INSN_DELETED_LABEL:
              if ((void *)(x) == this_obj)
                op (&((*x).u.fld[3].rt_str), cookie);
              break;
            case NOTE_INSN_DELETED:
              break;
            }
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[2].rt_bb), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case CODE_LABEL:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[6].rt_str), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[3].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[2].rt_bb), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case BARRIER:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case JUMP_TABLE_DATA:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[3].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[2].rt_bb), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case CALL_INSN:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[7].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[6].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[3].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[2].rt_bb), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case JUMP_INSN:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[7].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[6].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[3].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[2].rt_bb), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case INSN:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[6].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[3].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[2].rt_bb), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case DEBUG_INSN:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[6].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[3].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[2].rt_bb), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case ADDRESS:
          break;
        case SEQUENCE:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtvec), cookie);
          break;
        case INT_LIST:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          break;
        case INSN_LIST:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case EXPR_LIST:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[1].rt_rtx), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_rtx), cookie);
          break;
        case DEBUG_EXPR:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fld[0].rt_tree), cookie);
          break;
        case VALUE:
          break;
        case UNKNOWN:
          break;
        default:
          break;
        }
      break;
    /* Unrecognized tag value.  */
    default: gcc_unreachable (); 
    }
}

void
gt_pch_p_9rtvec_def (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct rtvec_def * x ATTRIBUTE_UNUSED = (struct rtvec_def *)x_p;
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
gt_pch_p_6gimple (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct gimple * x ATTRIBUTE_UNUSED = (struct gimple *)x_p;
  switch ((int) (gimple_statement_structure (&((*x)))))
    {
    case GSS_BASE:
      if ((void *)(x) == this_obj)
        op (&((*x).bb), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).next), cookie);
      break;
    case GSS_WCE:
      {
        gimple_statement_wce *sub = static_cast <gimple_statement_wce *> (x);
        if ((void *)(x) == this_obj)
          op (&((*sub).cleanup), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).bb), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next), cookie);
      }
      break;
    case GSS_OMP:
      {
        gimple_statement_omp *sub = static_cast <gimple_statement_omp *> (x);
        if ((void *)(x) == this_obj)
          op (&((*sub).body), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).bb), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next), cookie);
      }
      break;
    case GSS_OMP_SECTIONS:
      {
        gomp_sections *sub = static_cast <gomp_sections *> (x);
        if ((void *)(x) == this_obj)
          op (&((*sub).clauses), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).control), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).body), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).bb), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next), cookie);
      }
      break;
    case GSS_OMP_PARALLEL_LAYOUT:
      {
        gimple_statement_omp_parallel_layout *sub = static_cast <gimple_statement_omp_parallel_layout *> (x);
        if ((void *)(x) == this_obj)
          op (&((*sub).clauses), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).child_fn), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).data_arg), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).body), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).bb), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next), cookie);
      }
      break;
    case GSS_OMP_TASK:
      {
        gomp_task *sub = static_cast <gomp_task *> (x);
        if ((void *)(x) == this_obj)
          op (&((*sub).copy_fn), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).arg_size), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).arg_align), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).clauses), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).child_fn), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).data_arg), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).body), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).bb), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next), cookie);
      }
      break;
    case GSS_OMP_FOR:
      {
        gomp_for *sub = static_cast <gomp_for *> (x);
        {
          size_t l0 = (size_t)(((*sub)).collapse);
          if ((void *)(x) == this_obj)
            op (&((*sub).clauses), cookie);
          if ((*sub).iter != NULL) {
            size_t i0;
            for (i0 = 0; i0 != (size_t)(l0) && ((void *)(*sub).iter == this_obj); i0++) {
              if ((void *)((*sub).iter) == this_obj)
                op (&((*sub).iter[i0].index), cookie);
              if ((void *)((*sub).iter) == this_obj)
                op (&((*sub).iter[i0].initial), cookie);
              if ((void *)((*sub).iter) == this_obj)
                op (&((*sub).iter[i0].final), cookie);
              if ((void *)((*sub).iter) == this_obj)
                op (&((*sub).iter[i0].incr), cookie);
            }
            if ((void *)(x) == this_obj)
              op (&((*sub).iter), cookie);
          }
          if ((void *)(x) == this_obj)
            op (&((*sub).pre_body), cookie);
          if ((void *)(x) == this_obj)
            op (&((*sub).body), cookie);
          if ((void *)(x) == this_obj)
            op (&((*sub).bb), cookie);
          if ((void *)(x) == this_obj)
            op (&((*sub).next), cookie);
        }
      }
      break;
    case GSS_OMP_SINGLE_LAYOUT:
      {
        gimple_statement_omp_single_layout *sub = static_cast <gimple_statement_omp_single_layout *> (x);
        if ((void *)(x) == this_obj)
          op (&((*sub).clauses), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).body), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).bb), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next), cookie);
      }
      break;
    case GSS_OMP_CRITICAL:
      {
        gomp_critical *sub = static_cast <gomp_critical *> (x);
        if ((void *)(x) == this_obj)
          op (&((*sub).clauses), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).name), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).body), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).bb), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next), cookie);
      }
      break;
    case GSS_OMP_CONTINUE:
      {
        gomp_continue *sub = static_cast <gomp_continue *> (x);
        if ((void *)(x) == this_obj)
          op (&((*sub).control_def), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).control_use), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).bb), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next), cookie);
      }
      break;
    case GSS_OMP_ATOMIC_STORE_LAYOUT:
      {
        gimple_statement_omp_atomic_store_layout *sub = static_cast <gimple_statement_omp_atomic_store_layout *> (x);
        if ((void *)(x) == this_obj)
          op (&((*sub).val), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).bb), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next), cookie);
      }
      break;
    case GSS_OMP_ATOMIC_LOAD:
      {
        gomp_atomic_load *sub = static_cast <gomp_atomic_load *> (x);
        if ((void *)(x) == this_obj)
          op (&((*sub).rhs), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).lhs), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).bb), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next), cookie);
      }
      break;
    case GSS_TRY:
      {
        gtry *sub = static_cast <gtry *> (x);
        if ((void *)(x) == this_obj)
          op (&((*sub).eval), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).cleanup), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).bb), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next), cookie);
      }
      break;
    case GSS_PHI:
      {
        gphi *sub = static_cast <gphi *> (x);
        {
          size_t l1 = (size_t)(((*sub)).nargs);
          if ((void *)(x) == this_obj)
            op (&((*sub).result), cookie);
          {
            size_t i1;
            for (i1 = 0; i1 != l1; i1++) {
              if ((void *)(x) == this_obj)
                op (&((*sub).args[i1].def), cookie);
            }
          }
          if ((void *)(x) == this_obj)
            op (&((*sub).bb), cookie);
          if ((void *)(x) == this_obj)
            op (&((*sub).next), cookie);
        }
      }
      break;
    case GSS_EH_CTRL:
      {
        gimple_statement_eh_ctrl *sub = static_cast <gimple_statement_eh_ctrl *> (x);
        if ((void *)(x) == this_obj)
          op (&((*sub).bb), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next), cookie);
      }
      break;
    case GSS_EH_ELSE:
      {
        geh_else *sub = static_cast <geh_else *> (x);
        if ((void *)(x) == this_obj)
          op (&((*sub).n_body), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).e_body), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).bb), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next), cookie);
      }
      break;
    case GSS_EH_MNT:
      {
        geh_mnt *sub = static_cast <geh_mnt *> (x);
        if ((void *)(x) == this_obj)
          op (&((*sub).fndecl), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).bb), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next), cookie);
      }
      break;
    case GSS_EH_FILTER:
      {
        geh_filter *sub = static_cast <geh_filter *> (x);
        if ((void *)(x) == this_obj)
          op (&((*sub).types), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).failure), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).bb), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next), cookie);
      }
      break;
    case GSS_CATCH:
      {
        gcatch *sub = static_cast <gcatch *> (x);
        if ((void *)(x) == this_obj)
          op (&((*sub).types), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).handler), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).bb), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next), cookie);
      }
      break;
    case GSS_BIND:
      {
        gbind *sub = static_cast <gbind *> (x);
        if ((void *)(x) == this_obj)
          op (&((*sub).vars), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).block), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).body), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).bb), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next), cookie);
      }
      break;
    case GSS_WITH_MEM_OPS_BASE:
      {
        gimple_statement_with_memory_ops_base *sub = static_cast <gimple_statement_with_memory_ops_base *> (x);
        if ((void *)(x) == this_obj)
          op (&((*sub).bb), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next), cookie);
      }
      break;
    case GSS_TRANSACTION:
      {
        gtransaction *sub = static_cast <gtransaction *> (x);
        if ((void *)(x) == this_obj)
          op (&((*sub).body), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).label_norm), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).label_uninst), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).label_over), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).bb), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next), cookie);
      }
      break;
    case GSS_CALL:
      {
        gcall *sub = static_cast <gcall *> (x);
        {
          size_t l2 = (size_t)(((*sub)).num_ops);
          if ((void *)(x) == this_obj)
            op (&((*sub).call_used.vars), cookie);
          if ((void *)(x) == this_obj)
            op (&((*sub).call_clobbered.vars), cookie);
          switch ((int) (((*sub)).subcode & GF_CALL_INTERNAL))
            {
            case 0:
              if ((void *)(x) == this_obj)
                op (&((*sub).u.fntype), cookie);
              break;
            case GF_CALL_INTERNAL:
              break;
            default:
              break;
            }
          {
            size_t i2;
            for (i2 = 0; i2 != l2; i2++) {
              if ((void *)(x) == this_obj)
                op (&((*sub).op[i2]), cookie);
            }
          }
          if ((void *)(x) == this_obj)
            op (&((*sub).bb), cookie);
          if ((void *)(x) == this_obj)
            op (&((*sub).next), cookie);
        }
      }
      break;
    case GSS_ASM:
      {
        gasm *sub = static_cast <gasm *> (x);
        {
          size_t l3 = (size_t)(((*sub)).num_ops);
          if ((void *)(x) == this_obj)
            op (&((*sub).string), cookie);
          {
            size_t i3;
            for (i3 = 0; i3 != l3; i3++) {
              if ((void *)(x) == this_obj)
                op (&((*sub).op[i3]), cookie);
            }
          }
          if ((void *)(x) == this_obj)
            op (&((*sub).bb), cookie);
          if ((void *)(x) == this_obj)
            op (&((*sub).next), cookie);
        }
      }
      break;
    case GSS_WITH_MEM_OPS:
      {
        gimple_statement_with_memory_ops *sub = static_cast <gimple_statement_with_memory_ops *> (x);
        {
          size_t l4 = (size_t)(((*sub)).num_ops);
          {
            size_t i4;
            for (i4 = 0; i4 != l4; i4++) {
              if ((void *)(x) == this_obj)
                op (&((*sub).op[i4]), cookie);
            }
          }
          if ((void *)(x) == this_obj)
            op (&((*sub).bb), cookie);
          if ((void *)(x) == this_obj)
            op (&((*sub).next), cookie);
        }
      }
      break;
    case GSS_WITH_OPS:
      {
        gimple_statement_with_ops *sub = static_cast <gimple_statement_with_ops *> (x);
        {
          size_t l5 = (size_t)(((*sub)).num_ops);
          {
            size_t i5;
            for (i5 = 0; i5 != l5; i5++) {
              if ((void *)(x) == this_obj)
                op (&((*sub).op[i5]), cookie);
            }
          }
          if ((void *)(x) == this_obj)
            op (&((*sub).bb), cookie);
          if ((void *)(x) == this_obj)
            op (&((*sub).next), cookie);
        }
      }
      break;
    /* Unrecognized tag value.  */
    default: gcc_unreachable (); 
    }
}

void
gt_pch_p_11symtab_node (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct symtab_node * x ATTRIBUTE_UNUSED = (struct symtab_node *)x_p;
  switch ((int) (((*x)).type))
    {
    case SYMTAB_SYMBOL:
      if ((void *)(x) == this_obj)
        op (&((*x).decl), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).next), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).previous), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).next_sharing_asm_name), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).previous_sharing_asm_name), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).same_comdat_group), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).ref_list.references), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).alias_target), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).lto_file_data), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).x_comdat_group), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).x_section), cookie);
      break;
    case SYMTAB_VARIABLE:
      {
        varpool_node *sub = static_cast <varpool_node *> (x);
        if ((void *)(x) == this_obj)
          op (&((*sub).decl), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).previous), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next_sharing_asm_name), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).previous_sharing_asm_name), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).same_comdat_group), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).ref_list.references), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).alias_target), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).lto_file_data), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).x_comdat_group), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).x_section), cookie);
      }
      break;
    case SYMTAB_FUNCTION:
      {
        cgraph_node *sub = static_cast <cgraph_node *> (x);
        if ((void *)(x) == this_obj)
          op (&((*sub).callees), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).callers), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).indirect_calls), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).origin), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).nested), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next_nested), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next_sibling_clone), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).prev_sibling_clone), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).clones), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).clone_of), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).call_site_hash), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).former_clone_of), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).simdclone), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).simd_clones), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).inlined_to), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).rtl), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).clone.tree_map), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).clone.param_adjustments), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).clone.performed_splits), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).thunk.alias), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).decl), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).previous), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).next_sharing_asm_name), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).previous_sharing_asm_name), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).same_comdat_group), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).ref_list.references), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).alias_target), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).lto_file_data), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).x_comdat_group), cookie);
        if ((void *)(x) == this_obj)
          op (&((*sub).x_section), cookie);
      }
      break;
    /* Unrecognized tag value.  */
    default: gcc_unreachable (); 
    }
}

void
gt_pch_p_11cgraph_edge (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct cgraph_edge * x ATTRIBUTE_UNUSED = (struct cgraph_edge *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).caller), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).callee), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).prev_caller), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).next_caller), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).prev_callee), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).next_callee), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).call_stmt), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).indirect_info), cookie);
}

void
gt_pch_nx (struct cgraph_edge* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).caller), cookie);
    op (&((*x).callee), cookie);
    op (&((*x).prev_caller), cookie);
    op (&((*x).next_caller), cookie);
    op (&((*x).prev_callee), cookie);
    op (&((*x).next_callee), cookie);
    op (&((*x).call_stmt), cookie);
    op (&((*x).indirect_info), cookie);
}

void
gt_pch_p_7section (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  union section * x ATTRIBUTE_UNUSED = (union section *)x_p;
  switch ((int) (SECTION_STYLE (&(((*x))))))
    {
    case SECTION_NAMED:
      if ((void *)(x) == this_obj)
        op (&((*x).named.name), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).named.decl), cookie);
      break;
    case SECTION_UNNAMED:
      if ((void *)(x) == this_obj)
        op (&((*x).unnamed.next), cookie);
      break;
    case SECTION_NOSWITCH:
      break;
    default:
      break;
    }
}

void
gt_pch_nx (union section* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  switch ((int) (SECTION_STYLE (&(((*x))))))
    {
    case SECTION_NAMED:
        op (&((*x).named.name), cookie);
        op (&((*x).named.decl), cookie);
      break;
    case SECTION_UNNAMED:
        op (&((*x).unnamed.next), cookie);
      break;
    case SECTION_NOSWITCH:
      break;
    default:
      break;
    }
}

void
gt_pch_p_16cl_target_option (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct cl_target_option * x ATTRIBUTE_UNUSED = (struct cl_target_option *)x_p;
}

void
gt_pch_p_15cl_optimization (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct cl_optimization * x ATTRIBUTE_UNUSED = (struct cl_optimization *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).x_str_align_functions), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).x_str_align_jumps), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).x_str_align_labels), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).x_str_align_loops), cookie);
}

void
gt_pch_p_8edge_def (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct edge_def * x ATTRIBUTE_UNUSED = (struct edge_def *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_p_15basic_block_def (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct basic_block_def * x ATTRIBUTE_UNUSED = (struct basic_block_def *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).preds), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).succs), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).loop_father), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).prev_bb), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).next_bb), cookie);
  switch ((int) (((((*x)).flags & BB_RTL) != 0)))
    {
    case 0:
      if ((void *)(x) == this_obj)
        op (&((*x).il.gimple.seq), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).il.gimple.phi_nodes), cookie);
      break;
    case 1:
      if ((void *)(x) == this_obj)
        op (&((*x).il.x.head_), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).il.x.rtl), cookie);
      break;
    default:
      break;
    }
}

void
gt_pch_p_14bitmap_element (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct bitmap_element * x ATTRIBUTE_UNUSED = (struct bitmap_element *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).prev), cookie);
}

void
gt_pch_p_34generic_wide_int_wide_int_storage_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct generic_wide_int<wide_int_storage> * x ATTRIBUTE_UNUSED = (struct generic_wide_int<wide_int_storage> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct wide_int_storage* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_9mem_attrs (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct mem_attrs * x ATTRIBUTE_UNUSED = (struct mem_attrs *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).expr), cookie);
}

void
gt_pch_p_9reg_attrs (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct reg_attrs * x ATTRIBUTE_UNUSED = (struct reg_attrs *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).decl), cookie);
}

void
gt_pch_nx (struct reg_attrs* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).decl), cookie);
}

void
gt_pch_p_12object_block (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct object_block * x ATTRIBUTE_UNUSED = (struct object_block *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).sect), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).objects), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).anchors), cookie);
}

void
gt_pch_nx (struct object_block* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).sect), cookie);
    op (&((*x).objects), cookie);
    op (&((*x).anchors), cookie);
}

void
gt_pch_p_14vec_rtx_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<rtx,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<rtx,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_p_10real_value (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct real_value * x ATTRIBUTE_UNUSED = (struct real_value *)x_p;
}

void
gt_pch_p_11fixed_value (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct fixed_value * x ATTRIBUTE_UNUSED = (struct fixed_value *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x).mode), op, cookie);
}

void
gt_pch_p_8function (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct function * x ATTRIBUTE_UNUSED = (struct function *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).eh), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).cfg), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).gimple_body), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).gimple_df), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).x_current_loops), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).pass_startwith), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).su), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).decl), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).static_chain_decl), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).nonlocal_goto_save_area), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).local_decls), cookie);
  gcc_assert (!(*x).machine);
  if ((void *)(x) == this_obj)
    op (&((*x).language), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).used_types_hash), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).fde), cookie);
}

void
gt_pch_p_10target_rtl (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct target_rtl * x ATTRIBUTE_UNUSED = (struct target_rtl *)x_p;
  {
    size_t i0;
    size_t l0 = (size_t)(GR_MAX);
    for (i0 = 0; i0 != l0; i0++) {
      if ((void *)(x) == this_obj)
        op (&((*x).x_global_rtl[i0]), cookie);
    }
  }
  if ((void *)(x) == this_obj)
    op (&((*x).x_pic_offset_table_rtx), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).x_return_address_pointer_rtx), cookie);
  {
    size_t i1;
    size_t l1 = (size_t)(FIRST_PSEUDO_REGISTER);
    for (i1 = 0; i1 != l1; i1++) {
      if ((void *)(x) == this_obj)
        op (&((*x).x_initial_regno_reg_rtx[i1]), cookie);
    }
  }
  {
    size_t i2;
    size_t l2 = (size_t)(MAX_MACHINE_MODE);
    for (i2 = 0; i2 != l2; i2++) {
      if ((void *)(x) == this_obj)
        op (&((*x).x_top_of_stack[i2]), cookie);
    }
  }
  {
    size_t i3;
    size_t l3 = (size_t)(FIRST_PSEUDO_REGISTER);
    for (i3 = 0; i3 != l3; i3++) {
      if ((void *)(x) == this_obj)
        op (&((*x).x_static_reg_base_value[i3]), cookie);
    }
  }
  {
    size_t i4;
    size_t l4 = (size_t)((int) MAX_MACHINE_MODE);
    for (i4 = 0; i4 != l4; i4++) {
      if ((void *)(x) == this_obj)
        op (&((*x).x_mode_mem_attrs[i4]), cookie);
    }
  }
}

void
gt_pch_p_15cgraph_rtl_info (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct cgraph_rtl_info * x ATTRIBUTE_UNUSED = (struct cgraph_rtl_info *)x_p;
}

void
gt_pch_p_42hash_map_tree_tree_decl_tree_cache_traits_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_map<tree,tree,decl_tree_cache_traits> * x ATTRIBUTE_UNUSED = (struct hash_map<tree,tree,decl_tree_cache_traits> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct decl_tree_cache_traits* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_42hash_map_tree_tree_type_tree_cache_traits_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_map<tree,tree,type_tree_cache_traits> * x ATTRIBUTE_UNUSED = (struct hash_map<tree,tree,type_tree_cache_traits> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct type_tree_cache_traits* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_36hash_map_tree_tree_decl_tree_traits_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_map<tree,tree,decl_tree_traits> * x ATTRIBUTE_UNUSED = (struct hash_map<tree,tree,decl_tree_traits> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct decl_tree_traits* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_12ptr_info_def (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct ptr_info_def * x ATTRIBUTE_UNUSED = (struct ptr_info_def *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).pt.vars), cookie);
}

void
gt_pch_p_14range_info_def (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct range_info_def * x ATTRIBUTE_UNUSED = (struct range_info_def *)x_p;
}

void
gt_pch_p_26vec_constructor_elt_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<constructor_elt,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<constructor_elt,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct constructor_elt* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).index), cookie);
    op (&((*x).value), cookie);
}

void
gt_pch_p_15vec_tree_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<tree,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<tree,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_p_24tree_statement_list_node (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct tree_statement_list_node * x ATTRIBUTE_UNUSED = (struct tree_statement_list_node *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).prev), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).stmt), cookie);
}

void
gt_pch_p_14target_globals (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct target_globals * x ATTRIBUTE_UNUSED = (struct target_globals *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).rtl), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).libfuncs), cookie);
}

void
gt_pch_p_8tree_map (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct tree_map * x ATTRIBUTE_UNUSED = (struct tree_map *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).base.from), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).to), cookie);
}

void
gt_pch_nx (struct tree_map* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).base.from), cookie);
    op (&((*x).to), cookie);
}

void
gt_pch_p_13tree_decl_map (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct tree_decl_map * x ATTRIBUTE_UNUSED = (struct tree_decl_map *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).base.from), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).to), cookie);
}

void
gt_pch_nx (struct tree_decl_map* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).base.from), cookie);
    op (&((*x).to), cookie);
}

void
gt_pch_p_12tree_int_map (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct tree_int_map * x ATTRIBUTE_UNUSED = (struct tree_int_map *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).base.from), cookie);
}

void
gt_pch_nx (struct tree_int_map* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).base.from), cookie);
}

void
gt_pch_p_12tree_vec_map (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct tree_vec_map * x ATTRIBUTE_UNUSED = (struct tree_vec_map *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).base.from), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).to), cookie);
}

void
gt_pch_nx (struct tree_vec_map* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).base.from), cookie);
    op (&((*x).to), cookie);
}

void
gt_pch_p_21vec_alias_pair_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<alias_pair,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<alias_pair,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct alias_pair* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).decl), cookie);
    op (&((*x).target), cookie);
}

void
gt_pch_p_13libfunc_entry (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct libfunc_entry * x ATTRIBUTE_UNUSED = (struct libfunc_entry *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).libfunc), cookie);
}

void
gt_pch_nx (struct libfunc_entry* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).libfunc), cookie);
}

void
gt_pch_p_26hash_table_libfunc_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<libfunc_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<libfunc_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct libfunc_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_15target_libfuncs (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct target_libfuncs * x ATTRIBUTE_UNUSED = (struct target_libfuncs *)x_p;
  {
    size_t i0;
    size_t l0 = (size_t)(LTI_MAX);
    for (i0 = 0; i0 != l0; i0++) {
      if ((void *)(x) == this_obj)
        op (&((*x).x_libfunc_table[i0]), cookie);
    }
  }
  if ((void *)(x) == this_obj)
    op (&((*x).x_libfunc_hash), cookie);
}

void
gt_pch_p_14sequence_stack (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct sequence_stack * x ATTRIBUTE_UNUSED = (struct sequence_stack *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).first), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).last), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
}

void
gt_pch_p_20vec_rtx_insn__va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<rtx_insn*,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<rtx_insn*,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_p_16vec_uchar_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<uchar,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<uchar,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_p_27vec_call_site_record_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<call_site_record,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<call_site_record,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_p_9gimple_df (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct gimple_df * x ATTRIBUTE_UNUSED = (struct gimple_df *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).ssa_names), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).vop), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).escaped.vars), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).free_ssanames), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).free_ssanames_queue), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).default_defs), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).ssa_operands.operand_memory), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).tm_restart), cookie);
}

void
gt_pch_p_11dw_fde_node (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct dw_fde_node * x ATTRIBUTE_UNUSED = (struct dw_fde_node *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).decl), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).dw_fde_begin), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).dw_fde_current_label), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).dw_fde_end), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).dw_fde_vms_end_prologue), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).dw_fde_vms_begin_epilogue), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).dw_fde_second_begin), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).dw_fde_second_end), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).dw_fde_cfi), cookie);
}

void
gt_pch_p_11frame_space (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct frame_space * x ATTRIBUTE_UNUSED = (struct frame_space *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
}

void
gt_pch_p_26vec_callinfo_callee_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<callinfo_callee,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<callinfo_callee,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct callinfo_callee* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).decl), cookie);
}

void
gt_pch_p_26vec_callinfo_dalloc_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<callinfo_dalloc,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<callinfo_dalloc,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct callinfo_dalloc* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).name), cookie);
}

void
gt_pch_p_11stack_usage (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct stack_usage * x ATTRIBUTE_UNUSED = (struct stack_usage *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).callees), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).dallocs), cookie);
}

void
gt_pch_p_9eh_status (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct eh_status * x ATTRIBUTE_UNUSED = (struct eh_status *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).region_tree), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).region_array), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).lp_array), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).throw_stmt_table), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).ttype_data), cookie);
  switch ((int) (targetm.arm_eabi_unwinder))
    {
    case 1:
      if ((void *)(x) == this_obj)
        op (&((*x).ehspec_data.arm_eabi), cookie);
      break;
    case 0:
      if ((void *)(x) == this_obj)
        op (&((*x).ehspec_data.other), cookie);
      break;
    default:
      break;
    }
}

void
gt_pch_p_18control_flow_graph (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct control_flow_graph * x ATTRIBUTE_UNUSED = (struct control_flow_graph *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).x_entry_block_ptr), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).x_exit_block_ptr), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).x_basic_block_info), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).x_label_to_block_map), cookie);
}

void
gt_pch_p_5loops (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct loops * x ATTRIBUTE_UNUSED = (struct loops *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).larray), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).exits), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).tree_root), cookie);
}

void
gt_pch_p_14hash_set_tree_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_set<tree> * x ATTRIBUTE_UNUSED = (struct hash_set<tree> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_p_24types_used_by_vars_entry (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct types_used_by_vars_entry * x ATTRIBUTE_UNUSED = (struct types_used_by_vars_entry *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).type), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).var_decl), cookie);
}

void
gt_pch_nx (struct types_used_by_vars_entry* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).type), cookie);
    op (&((*x).var_decl), cookie);
}

void
gt_pch_p_28hash_table_used_type_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<used_type_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<used_type_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct used_type_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_13nb_iter_bound (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct nb_iter_bound * x ATTRIBUTE_UNUSED = (struct nb_iter_bound *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).stmt), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
}

void
gt_pch_p_9loop_exit (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct loop_exit * x ATTRIBUTE_UNUSED = (struct loop_exit *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).e), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).prev), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).next_e), cookie);
}

void
gt_pch_nx (struct loop_exit* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).e), cookie);
    op (&((*x).prev), cookie);
    op (&((*x).next), cookie);
    op (&((*x).next_e), cookie);
}

void
gt_pch_p_4loop (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct loop * x ATTRIBUTE_UNUSED = (struct loop *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).header), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).latch), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).superloops), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).inner), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).nb_iterations), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).simduid), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).bounds), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).control_ivs), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).exits), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).simple_loop_desc), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).former_header), cookie);
}

void
gt_pch_p_10control_iv (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct control_iv * x ATTRIBUTE_UNUSED = (struct control_iv *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).base), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).step), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
}

void
gt_pch_p_17vec_loop_p_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<loop_p,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<loop_p,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_p_10niter_desc (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct niter_desc * x ATTRIBUTE_UNUSED = (struct niter_desc *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).out_edge), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).in_edge), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).assumptions), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).noloop_assumptions), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).infinite), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).niter_expr), cookie);
}

void
gt_pch_p_28hash_table_loop_exit_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<loop_exit_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<loop_exit_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct loop_exit_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_22vec_basic_block_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<basic_block,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<basic_block,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_p_11rtl_bb_info (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct rtl_bb_info * x ATTRIBUTE_UNUSED = (struct rtl_bb_info *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).end_), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).header_), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).footer_), cookie);
}

void
gt_pch_p_15vec_edge_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<edge,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<edge,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_p_20vec_ipa_ref_t_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<ipa_ref_t,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<ipa_ref_t,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct ipa_ref* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).referring), cookie);
    op (&((*x).referred), cookie);
    op (&((*x).stmt), cookie);
}

void
gt_pch_p_18section_hash_entry (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct section_hash_entry * x ATTRIBUTE_UNUSED = (struct section_hash_entry *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).name), cookie);
}

void
gt_pch_nx (struct section_hash_entry* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).name), cookie);
}

void
gt_pch_p_18lto_file_decl_data (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct lto_file_decl_data * x ATTRIBUTE_UNUSED = (struct lto_file_decl_data *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).current_decl_state), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).global_decl_state), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).function_decl_states), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).mode_table), cookie);
}

void
gt_pch_p_15ipa_replace_map (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct ipa_replace_map * x ATTRIBUTE_UNUSED = (struct ipa_replace_map *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).new_tree), cookie);
}

void
gt_pch_p_27vec_ipa_replace_map__va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<ipa_replace_map*,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<ipa_replace_map*,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_p_21ipa_param_adjustments (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct ipa_param_adjustments * x ATTRIBUTE_UNUSED = (struct ipa_param_adjustments *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).m_adj_params), cookie);
}

void
gt_pch_p_36vec_ipa_param_performed_split_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<ipa_param_performed_split,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<ipa_param_performed_split,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct ipa_param_performed_split* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).dummy_decl), cookie);
}

void
gt_pch_p_17cgraph_simd_clone (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct cgraph_simd_clone * x ATTRIBUTE_UNUSED = (struct cgraph_simd_clone *)x_p;
  {
    size_t l0 = (size_t)(((*x)).nargs);
    if ((void *)(x) == this_obj)
      op (&((*x).prev_clone), cookie);
    if ((void *)(x) == this_obj)
      op (&((*x).next_clone), cookie);
    if ((void *)(x) == this_obj)
      op (&((*x).origin), cookie);
    {
      size_t i0;
      for (i0 = 0; i0 != l0; i0++) {
        if ((void *)(x) == this_obj)
          op (&((*x).args[i0].orig_arg), cookie);
        if ((void *)(x) == this_obj)
          op (&((*x).args[i0].orig_type), cookie);
        if ((void *)(x) == this_obj)
          op (&((*x).args[i0].vector_arg), cookie);
        if ((void *)(x) == this_obj)
          op (&((*x).args[i0].vector_type), cookie);
        if ((void *)(x) == this_obj)
          op (&((*x).args[i0].simd_array), cookie);
      }
    }
  }
}

void
gt_pch_p_28cgraph_function_version_info (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct cgraph_function_version_info * x ATTRIBUTE_UNUSED = (struct cgraph_function_version_info *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).this_node), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).prev), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).dispatcher_resolver), cookie);
}

void
gt_pch_nx (struct cgraph_function_version_info* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).this_node), cookie);
    op (&((*x).prev), cookie);
    op (&((*x).next), cookie);
    op (&((*x).dispatcher_resolver), cookie);
}

void
gt_pch_p_30hash_table_cgraph_edge_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<cgraph_edge_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<cgraph_edge_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct cgraph_edge_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_25cgraph_indirect_call_info (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct cgraph_indirect_call_info * x ATTRIBUTE_UNUSED = (struct cgraph_indirect_call_info *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).context.outer_type), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).context.speculative_outer_type), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).otr_type), cookie);
}

void
gt_pch_p_8asm_node (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct asm_node * x ATTRIBUTE_UNUSED = (struct asm_node *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).asm_str), cookie);
}

void
gt_pch_p_12symbol_table (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct symbol_table * x ATTRIBUTE_UNUSED = (struct symbol_table *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).nodes), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).asmnodes), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).asm_last_node), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).section_hash), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).assembler_name_hash), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).init_priority_hash), cookie);
}

void
gt_pch_p_31hash_table_section_name_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<section_name_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<section_name_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct section_name_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_26hash_table_asmname_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<asmname_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<asmname_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct asmname_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_42hash_map_symtab_node__symbol_priority_map_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_map<symtab_node*,symbol_priority_map> * x ATTRIBUTE_UNUSED = (struct hash_map<symtab_node*,symbol_priority_map> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct symbol_priority_map* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_24constant_descriptor_tree (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct constant_descriptor_tree * x ATTRIBUTE_UNUSED = (struct constant_descriptor_tree *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).rtl), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).value), cookie);
}

void
gt_pch_nx (struct constant_descriptor_tree* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).rtl), cookie);
    op (&((*x).value), cookie);
}

void
gt_pch_p_17lto_in_decl_state (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct lto_in_decl_state * x ATTRIBUTE_UNUSED = (struct lto_in_decl_state *)x_p;
  {
    size_t i0;
    size_t l0 = (size_t)(LTO_N_DECL_STREAMS);
    for (i0 = 0; i0 != l0; i0++) {
      if ((void *)(x) == this_obj)
        op (&((*x).streams[i0]), cookie);
    }
  }
  if ((void *)(x) == this_obj)
    op (&((*x).fn_decl), cookie);
}

void
gt_pch_nx (struct lto_in_decl_state* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  {
    size_t i1;
    size_t l1 = (size_t)(LTO_N_DECL_STREAMS);
    for (i1 = 0; i1 != l1; i1++) {
        op (&((*x).streams[i1]), cookie);
    }
  }
    op (&((*x).fn_decl), cookie);
}

void
gt_pch_p_15ipa_node_params (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct ipa_node_params * x ATTRIBUTE_UNUSED = (struct ipa_node_params *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).descriptors), cookie);
}

void
gt_pch_nx (struct ipa_node_params* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).descriptors), cookie);
}

void
gt_pch_p_13ipa_edge_args (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct ipa_edge_args * x ATTRIBUTE_UNUSED = (struct ipa_edge_args *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).jump_functions), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).polymorphic_call_contexts), cookie);
}

void
gt_pch_nx (struct ipa_edge_args* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).jump_functions), cookie);
    op (&((*x).polymorphic_call_contexts), cookie);
}

void
gt_pch_p_25ipa_agg_replacement_value (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct ipa_agg_replacement_value * x ATTRIBUTE_UNUSED = (struct ipa_agg_replacement_value *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).value), cookie);
}

void
gt_pch_p_14ipa_fn_summary (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct ipa_fn_summary * x ATTRIBUTE_UNUSED = (struct ipa_fn_summary *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).conds), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).size_time_table), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).call_size_time_table), cookie);
}

void
gt_pch_p_29vec_ipa_adjusted_param_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<ipa_adjusted_param,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<ipa_adjusted_param,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct ipa_adjusted_param* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).type), cookie);
    op (&((*x).alias_ptr_type), cookie);
}

void
gt_pch_p_11dw_cfi_node (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct dw_cfi_node * x ATTRIBUTE_UNUSED = (struct dw_cfi_node *)x_p;
  switch ((int) (dw_cfi_oprnd1_desc (((*x)).dw_cfi_opc)))
    {
    case dw_cfi_oprnd_reg_num:
      break;
    case dw_cfi_oprnd_offset:
      break;
    case dw_cfi_oprnd_addr:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_cfi_oprnd1.dw_cfi_addr), cookie);
      break;
    case dw_cfi_oprnd_loc:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_cfi_oprnd1.dw_cfi_loc), cookie);
      break;
    case dw_cfi_oprnd_cfa_loc:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_cfi_oprnd1.dw_cfi_cfa_loc), cookie);
      break;
    default:
      break;
    }
  switch ((int) (dw_cfi_oprnd2_desc (((*x)).dw_cfi_opc)))
    {
    case dw_cfi_oprnd_reg_num:
      break;
    case dw_cfi_oprnd_offset:
      break;
    case dw_cfi_oprnd_addr:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_cfi_oprnd2.dw_cfi_addr), cookie);
      break;
    case dw_cfi_oprnd_loc:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_cfi_oprnd2.dw_cfi_loc), cookie);
      break;
    case dw_cfi_oprnd_cfa_loc:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_cfi_oprnd2.dw_cfi_cfa_loc), cookie);
      break;
    default:
      break;
    }
}

void
gt_pch_p_17dw_loc_descr_node (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct dw_loc_descr_node * x ATTRIBUTE_UNUSED = (struct dw_loc_descr_node *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).dw_loc_next), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).dw_loc_oprnd1.val_entry), cookie);
  switch ((int) (((*x).dw_loc_oprnd1).val_class))
    {
    case dw_val_class_addr:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd1.v.val_addr), cookie);
      break;
    case dw_val_class_offset:
      break;
    case dw_val_class_loc_list:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd1.v.val_loc_list), cookie);
      break;
    case dw_val_class_view_list:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd1.v.val_view_list), cookie);
      break;
    case dw_val_class_loc:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd1.v.val_loc), cookie);
      break;
    default:
      break;
    case dw_val_class_unsigned_const:
      break;
    case dw_val_class_const_double:
      break;
    case dw_val_class_wide_int:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd1.v.val_wide), cookie);
      break;
    case dw_val_class_vec:
      if ((*x).dw_loc_oprnd1.v.val_vec.array != NULL) {
        if ((void *)(x) == this_obj)
          op (&((*x).dw_loc_oprnd1.v.val_vec.array), cookie);
      }
      break;
    case dw_val_class_die_ref:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd1.v.val_die_ref.die), cookie);
      break;
    case dw_val_class_fde_ref:
      break;
    case dw_val_class_str:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd1.v.val_str), cookie);
      break;
    case dw_val_class_lbl_id:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd1.v.val_lbl_id), cookie);
      break;
    case dw_val_class_flag:
      break;
    case dw_val_class_file:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd1.v.val_file), cookie);
      break;
    case dw_val_class_file_implicit:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd1.v.val_file_implicit), cookie);
      break;
    case dw_val_class_data8:
      break;
    case dw_val_class_decl_ref:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd1.v.val_decl_ref), cookie);
      break;
    case dw_val_class_vms_delta:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd1.v.val_vms_delta.lbl1), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd1.v.val_vms_delta.lbl2), cookie);
      break;
    case dw_val_class_discr_value:
      switch ((int) (((*x).dw_loc_oprnd1.v.val_discr_value).pos))
        {
        case 0:
          break;
        case 1:
          break;
        default:
          break;
        }
      break;
    case dw_val_class_discr_list:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd1.v.val_discr_list), cookie);
      break;
    case dw_val_class_symview:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd1.v.val_symbolic_view), cookie);
      break;
    }
  if ((void *)(x) == this_obj)
    op (&((*x).dw_loc_oprnd2.val_entry), cookie);
  switch ((int) (((*x).dw_loc_oprnd2).val_class))
    {
    case dw_val_class_addr:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd2.v.val_addr), cookie);
      break;
    case dw_val_class_offset:
      break;
    case dw_val_class_loc_list:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd2.v.val_loc_list), cookie);
      break;
    case dw_val_class_view_list:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd2.v.val_view_list), cookie);
      break;
    case dw_val_class_loc:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd2.v.val_loc), cookie);
      break;
    default:
      break;
    case dw_val_class_unsigned_const:
      break;
    case dw_val_class_const_double:
      break;
    case dw_val_class_wide_int:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd2.v.val_wide), cookie);
      break;
    case dw_val_class_vec:
      if ((*x).dw_loc_oprnd2.v.val_vec.array != NULL) {
        if ((void *)(x) == this_obj)
          op (&((*x).dw_loc_oprnd2.v.val_vec.array), cookie);
      }
      break;
    case dw_val_class_die_ref:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd2.v.val_die_ref.die), cookie);
      break;
    case dw_val_class_fde_ref:
      break;
    case dw_val_class_str:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd2.v.val_str), cookie);
      break;
    case dw_val_class_lbl_id:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd2.v.val_lbl_id), cookie);
      break;
    case dw_val_class_flag:
      break;
    case dw_val_class_file:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd2.v.val_file), cookie);
      break;
    case dw_val_class_file_implicit:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd2.v.val_file_implicit), cookie);
      break;
    case dw_val_class_data8:
      break;
    case dw_val_class_decl_ref:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd2.v.val_decl_ref), cookie);
      break;
    case dw_val_class_vms_delta:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd2.v.val_vms_delta.lbl1), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd2.v.val_vms_delta.lbl2), cookie);
      break;
    case dw_val_class_discr_value:
      switch ((int) (((*x).dw_loc_oprnd2.v.val_discr_value).pos))
        {
        case 0:
          break;
        case 1:
          break;
        default:
          break;
        }
      break;
    case dw_val_class_discr_list:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd2.v.val_discr_list), cookie);
      break;
    case dw_val_class_symview:
      if ((void *)(x) == this_obj)
        op (&((*x).dw_loc_oprnd2.v.val_symbolic_view), cookie);
      break;
    }
}

void
gt_pch_p_18dw_discr_list_node (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct dw_discr_list_node * x ATTRIBUTE_UNUSED = (struct dw_discr_list_node *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).dw_discr_next), cookie);
  switch ((int) (((*x).dw_discr_lower_bound).pos))
    {
    case 0:
      break;
    case 1:
      break;
    default:
      break;
    }
  switch ((int) (((*x).dw_discr_upper_bound).pos))
    {
    case 0:
      break;
    case 1:
      break;
    default:
      break;
    }
}

void
gt_pch_p_21vec_dw_cfi_ref_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<dw_cfi_ref,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<dw_cfi_ref,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_p_22vec_temp_slot_p_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<temp_slot_p,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<temp_slot_p,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_p_11eh_region_d (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct eh_region_d * x ATTRIBUTE_UNUSED = (struct eh_region_d *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).outer), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).inner), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).next_peer), cookie);
  switch ((int) ((*x).type))
    {
    case ERT_TRY:
      if ((void *)(x) == this_obj)
        op (&((*x).u.eh_try.first_catch), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).u.eh_try.last_catch), cookie);
      break;
    case ERT_ALLOWED_EXCEPTIONS:
      if ((void *)(x) == this_obj)
        op (&((*x).u.allowed.type_list), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).u.allowed.label), cookie);
      break;
    case ERT_MUST_NOT_THROW:
      if ((void *)(x) == this_obj)
        op (&((*x).u.must_not_throw.failure_decl), cookie);
      break;
    default:
      break;
    }
  if ((void *)(x) == this_obj)
    op (&((*x).landing_pads), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).exc_ptr_reg), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).filter_reg), cookie);
}

void
gt_pch_p_16eh_landing_pad_d (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct eh_landing_pad_d * x ATTRIBUTE_UNUSED = (struct eh_landing_pad_d *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).next_lp), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).region), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).post_landing_pad), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).landing_pad), cookie);
}

void
gt_pch_p_10eh_catch_d (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct eh_catch_d * x ATTRIBUTE_UNUSED = (struct eh_catch_d *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).next_catch), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).prev_catch), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).type_list), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).filter_list), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).label), cookie);
}

void
gt_pch_p_20vec_eh_region_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<eh_region,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<eh_region,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_p_25vec_eh_landing_pad_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<eh_landing_pad,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<eh_landing_pad,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_p_21hash_map_gimple__int_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_map<gimple*,int> * x ATTRIBUTE_UNUSED = (struct hash_map<gimple*,int> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_p_15tm_restart_node (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct tm_restart_node * x ATTRIBUTE_UNUSED = (struct tm_restart_node *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).stmt), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).label_or_list), cookie);
}

void
gt_pch_nx (struct tm_restart_node* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).stmt), cookie);
    op (&((*x).label_or_list), cookie);
}

void
gt_pch_p_19hash_map_tree_tree_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_map<tree,tree> * x ATTRIBUTE_UNUSED = (struct hash_map<tree,tree> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_p_27hash_table_ssa_name_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<ssa_name_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<ssa_name_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct ssa_name_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_29hash_table_tm_restart_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<tm_restart_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<tm_restart_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct tm_restart_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_20ssa_operand_memory_d (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct ssa_operand_memory_d * x ATTRIBUTE_UNUSED = (struct ssa_operand_memory_d *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
}

void
gt_pch_p_26vec_ipa_agg_jf_item_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<ipa_agg_jf_item,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<ipa_agg_jf_item,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct ipa_agg_jf_item* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).type), cookie);
  switch ((int) (((*x)).jftype))
    {
    case IPA_JF_CONST:
        op (&((*x).value.constant), cookie);
      break;
    case IPA_JF_PASS_THROUGH:
        op (&((*x).value.pass_through.operand), cookie);
      break;
    case IPA_JF_LOAD_AGG:
        op (&((*x).value.load_agg.pass_through.operand), cookie);
        op (&((*x).value.load_agg.type), cookie);
      break;
    default:
      break;
    }
}

void
gt_pch_p_8ipa_bits (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct ipa_bits * x ATTRIBUTE_UNUSED = (struct ipa_bits *)x_p;
}

void
gt_pch_p_11value_range (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct value_range * x ATTRIBUTE_UNUSED = (struct value_range *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).m_min), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).m_max), cookie);
}

void
gt_pch_nx (struct value_range* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).m_min), cookie);
    op (&((*x).m_max), cookie);
}

void
gt_pch_p_31vec_ipa_param_descriptor_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<ipa_param_descriptor,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<ipa_param_descriptor,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct ipa_param_descriptor* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).decl_or_type), cookie);
}

void
gt_pch_p_20vec_ipa_bits__va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<ipa_bits*,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<ipa_bits*,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_p_17vec_ipa_vr_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<ipa_vr,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<ipa_vr,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct ipa_vr* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    gt_pch_nx (&((*x).min), op, cookie);
    gt_pch_nx (&((*x).max), op, cookie);
}

void
gt_pch_p_19ipcp_transformation (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct ipcp_transformation * x ATTRIBUTE_UNUSED = (struct ipcp_transformation *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).agg_values), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).bits), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).m_vr), cookie);
}

void
gt_pch_p_24vec_ipa_jump_func_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<ipa_jump_func,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<ipa_jump_func,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct ipa_jump_func* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).agg.items), cookie);
    op (&((*x).bits), cookie);
    op (&((*x).m_vr), cookie);
  switch ((int) (((*x)).type))
    {
    case IPA_JF_CONST:
        op (&((*x).value.constant.value), cookie);
      break;
    case IPA_JF_PASS_THROUGH:
        op (&((*x).value.pass_through.operand), cookie);
      break;
    case IPA_JF_ANCESTOR:
      break;
    default:
      break;
    }
}

void
gt_pch_p_39vec_ipa_polymorphic_call_context_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<ipa_polymorphic_call_context,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<ipa_polymorphic_call_context,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct ipa_polymorphic_call_context* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).outer_type), cookie);
    op (&((*x).speculative_outer_type), cookie);
}

void
gt_pch_p_17ipa_node_params_t (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct ipa_node_params_t * x ATTRIBUTE_UNUSED = (struct ipa_node_params_t *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_p_19ipa_edge_args_sum_t (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct ipa_edge_args_sum_t * x ATTRIBUTE_UNUSED = (struct ipa_edge_args_sum_t *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_p_38function_summary_ipcp_transformation__ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct function_summary<ipcp_transformation*> * x ATTRIBUTE_UNUSED = (struct function_summary<ipcp_transformation*> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_p_29hash_table_decl_state_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<decl_state_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<decl_state_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct decl_state_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_23vec_expr_eval_op_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<expr_eval_op,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<expr_eval_op,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct expr_eval_op* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).type), cookie);
  {
    size_t i0;
    size_t l0 = (size_t)(2);
    for (i0 = 0; i0 != l0; i0++) {
        op (&((*x).val[i0]), cookie);
    }
  }
}

void
gt_pch_p_20vec_condition_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<condition,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<condition,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct condition* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).type), cookie);
    op (&((*x).val), cookie);
    op (&((*x).param_ops), cookie);
}

void
gt_pch_p_26vec_size_time_entry_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<size_time_entry,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<size_time_entry,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct size_time_entry* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_44fast_function_summary_ipa_fn_summary__va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct fast_function_summary<ipa_fn_summary*,va_gc> * x ATTRIBUTE_UNUSED = (struct fast_function_summary<ipa_fn_summary*,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

/* GC roots.  */

static void gt_ggc_ma_regno_reg_rtx (void *);
static void
gt_ggc_ma_regno_reg_rtx (ATTRIBUTE_UNUSED void *x_p)
{
  if (regno_reg_rtx != NULL) {
    size_t i0;
    for (i0 = 0; i0 != (size_t)(crtl->emit.x_reg_rtx_no); i0++) {
      gt_ggc_m_7rtx_def (regno_reg_rtx[i0]);
    }
    ggc_mark (regno_reg_rtx);
  }
}

static void gt_pch_pa_regno_reg_rtx
    (void *, void *, gt_pointer_operator, void *);
static void gt_pch_pa_regno_reg_rtx (ATTRIBUTE_UNUSED void *this_obj,
      ATTRIBUTE_UNUSED void *x_p,
      ATTRIBUTE_UNUSED gt_pointer_operator op,
      ATTRIBUTE_UNUSED void * cookie)
{
  if (regno_reg_rtx != NULL) {
    size_t i0;
    for (i0 = 0; i0 != (size_t)(crtl->emit.x_reg_rtx_no) && ((void *)regno_reg_rtx == this_obj); i0++) {
      if ((void *)(regno_reg_rtx) == this_obj)
        op (&(regno_reg_rtx[i0]), cookie);
    }
    if ((void *)(&regno_reg_rtx) == this_obj)
      op (&(regno_reg_rtx), cookie);
  }
}

static void gt_pch_na_regno_reg_rtx (void *);
static void
gt_pch_na_regno_reg_rtx (ATTRIBUTE_UNUSED void *x_p)
{
  if (regno_reg_rtx != NULL) {
    size_t i0;
    for (i0 = 0; i0 != (size_t)(crtl->emit.x_reg_rtx_no); i0++) {
      gt_pch_n_7rtx_def (regno_reg_rtx[i0]);
    }
    gt_pch_note_object (regno_reg_rtx, &regno_reg_rtx, gt_pch_pa_regno_reg_rtx);
  }
}

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gtype_desc_c[] = {
  {
    &internal_fn_fnspec_array[0],
    1 * (IFN_LAST + 1),
    sizeof (internal_fn_fnspec_array[0]),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &ipa_fn_summaries,
    1,
    sizeof (ipa_fn_summaries),
    &gt_ggc_mx_fast_function_summary_ipa_fn_summary__va_gc_,
    &gt_pch_nx_fast_function_summary_ipa_fn_summary__va_gc_
  },
  {
    &ipcp_transformation_sum,
    1,
    sizeof (ipcp_transformation_sum),
    &gt_ggc_mx_function_summary_ipcp_transformation__,
    &gt_pch_nx_function_summary_ipcp_transformation__
  },
  {
    &ipa_edge_args_sum,
    1,
    sizeof (ipa_edge_args_sum),
    &gt_ggc_mx_ipa_edge_args_sum_t,
    &gt_pch_nx_ipa_edge_args_sum_t
  },
  {
    &ipa_node_params_sum,
    1,
    sizeof (ipa_node_params_sum),
    &gt_ggc_mx_ipa_node_params_t,
    &gt_pch_nx_ipa_node_params_t
  },
  {
    &ipa_escaped_pt.vars,
    1,
    sizeof (ipa_escaped_pt.vars),
    &gt_ggc_mx_bitmap_head,
    &gt_pch_nx_bitmap_head
  },
  {
    &offload_vars,
    1,
    sizeof (offload_vars),
    &gt_ggc_mx_vec_tree_va_gc_,
    &gt_pch_nx_vec_tree_va_gc_
  },
  {
    &offload_funcs,
    1,
    sizeof (offload_funcs),
    &gt_ggc_mx_vec_tree_va_gc_,
    &gt_pch_nx_vec_tree_va_gc_
  },
  {
    &x_rtl.expr.x_saveregs_value,
    1,
    sizeof (x_rtl.expr.x_saveregs_value),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &x_rtl.expr.x_apply_args_value,
    1,
    sizeof (x_rtl.expr.x_apply_args_value),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &x_rtl.expr.x_forced_labels,
    1,
    sizeof (x_rtl.expr.x_forced_labels),
    &gt_ggc_mx_vec_rtx_insn__va_gc_,
    &gt_pch_nx_vec_rtx_insn__va_gc_
  },
  {
    &x_rtl.emit.seq.first,
    1,
    sizeof (x_rtl.emit.seq.first),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &x_rtl.emit.seq.last,
    1,
    sizeof (x_rtl.emit.seq.last),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &x_rtl.emit.seq.next,
    1,
    sizeof (x_rtl.emit.seq.next),
    &gt_ggc_mx_sequence_stack,
    &gt_pch_nx_sequence_stack
  },
  {
    &x_rtl.varasm.pool,
    1,
    sizeof (x_rtl.varasm.pool),
    &gt_ggc_mx_rtx_constant_pool,
    &gt_pch_nx_rtx_constant_pool
  },
  {
    &x_rtl.args.arg_offset_rtx,
    1,
    sizeof (x_rtl.args.arg_offset_rtx),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &x_rtl.args.internal_arg_pointer,
    1,
    sizeof (x_rtl.args.internal_arg_pointer),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &x_rtl.subsections.hot_section_label,
    1,
    sizeof (x_rtl.subsections.hot_section_label),
    (gt_pointer_walker) &gt_ggc_m_S,
    (gt_pointer_walker) &gt_pch_n_S
  },
  {
    &x_rtl.subsections.cold_section_label,
    1,
    sizeof (x_rtl.subsections.cold_section_label),
    (gt_pointer_walker) &gt_ggc_m_S,
    (gt_pointer_walker) &gt_pch_n_S
  },
  {
    &x_rtl.subsections.hot_section_end_label,
    1,
    sizeof (x_rtl.subsections.hot_section_end_label),
    (gt_pointer_walker) &gt_ggc_m_S,
    (gt_pointer_walker) &gt_pch_n_S
  },
  {
    &x_rtl.subsections.cold_section_end_label,
    1,
    sizeof (x_rtl.subsections.cold_section_end_label),
    (gt_pointer_walker) &gt_ggc_m_S,
    (gt_pointer_walker) &gt_pch_n_S
  },
  {
    &x_rtl.eh.ehr_stackadj,
    1,
    sizeof (x_rtl.eh.ehr_stackadj),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &x_rtl.eh.ehr_handler,
    1,
    sizeof (x_rtl.eh.ehr_handler),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &x_rtl.eh.ehr_label,
    1,
    sizeof (x_rtl.eh.ehr_label),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &x_rtl.eh.sjlj_fc,
    1,
    sizeof (x_rtl.eh.sjlj_fc),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &x_rtl.eh.sjlj_exit_after,
    1,
    sizeof (x_rtl.eh.sjlj_exit_after),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &x_rtl.eh.action_record_data,
    1,
    sizeof (x_rtl.eh.action_record_data),
    &gt_ggc_mx_vec_uchar_va_gc_,
    &gt_pch_nx_vec_uchar_va_gc_
  },
  {
    &x_rtl.eh.call_site_record_v[0],
    1 * (2),
    sizeof (x_rtl.eh.call_site_record_v[0]),
    &gt_ggc_mx_vec_call_site_record_va_gc_,
    &gt_pch_nx_vec_call_site_record_va_gc_
  },
  {
    &x_rtl.return_rtx,
    1,
    sizeof (x_rtl.return_rtx),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &x_rtl.hard_reg_initial_vals,
    1,
    sizeof (x_rtl.hard_reg_initial_vals),
    &gt_ggc_mx_initial_value_struct,
    &gt_pch_nx_initial_value_struct
  },
  {
    &x_rtl.stack_protect_guard,
    1,
    sizeof (x_rtl.stack_protect_guard),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &x_rtl.stack_protect_guard_decl,
    1,
    sizeof (x_rtl.stack_protect_guard_decl),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &x_rtl.x_nonlocal_goto_handler_labels,
    1,
    sizeof (x_rtl.x_nonlocal_goto_handler_labels),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &x_rtl.x_return_label,
    1,
    sizeof (x_rtl.x_return_label),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &x_rtl.x_naked_return_label,
    1,
    sizeof (x_rtl.x_naked_return_label),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &x_rtl.x_stack_slot_list,
    1,
    sizeof (x_rtl.x_stack_slot_list),
    &gt_ggc_mx_vec_rtx_va_gc_,
    &gt_pch_nx_vec_rtx_va_gc_
  },
  {
    &x_rtl.frame_space_list,
    1,
    sizeof (x_rtl.frame_space_list),
    &gt_ggc_mx_frame_space,
    &gt_pch_nx_frame_space
  },
  {
    &x_rtl.x_stack_check_probe_note,
    1,
    sizeof (x_rtl.x_stack_check_probe_note),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &x_rtl.x_arg_pointer_save_area,
    1,
    sizeof (x_rtl.x_arg_pointer_save_area),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &x_rtl.drap_reg,
    1,
    sizeof (x_rtl.drap_reg),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &x_rtl.x_parm_birth_insn,
    1,
    sizeof (x_rtl.x_parm_birth_insn),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &x_rtl.x_used_temp_slots,
    1,
    sizeof (x_rtl.x_used_temp_slots),
    &gt_ggc_mx_vec_temp_slot_p_va_gc_,
    &gt_pch_nx_vec_temp_slot_p_va_gc_
  },
  {
    &x_rtl.x_avail_temp_slots,
    1,
    sizeof (x_rtl.x_avail_temp_slots),
    &gt_ggc_mx_temp_slot,
    &gt_pch_nx_temp_slot
  },
  {
    &cie_cfi_vec,
    1,
    sizeof (cie_cfi_vec),
    &gt_ggc_mx_vec_dw_cfi_ref_va_gc_,
    &gt_pch_nx_vec_dw_cfi_ref_va_gc_
  },
  {
    &saved_symtab,
    1,
    sizeof (saved_symtab),
    &gt_ggc_mx_symbol_table,
    &gt_pch_nx_symbol_table
  },
  {
    &symtab,
    1,
    sizeof (symtab),
    &gt_ggc_mx_symbol_table,
    &gt_pch_nx_symbol_table
  },
  {
    &in_section,
    1,
    sizeof (in_section),
    &gt_ggc_mx_section,
    &gt_pch_nx_section
  },
  {
    &bss_noswitch_section,
    1,
    sizeof (bss_noswitch_section),
    &gt_ggc_mx_section,
    &gt_pch_nx_section
  },
  {
    &lcomm_section,
    1,
    sizeof (lcomm_section),
    &gt_ggc_mx_section,
    &gt_pch_nx_section
  },
  {
    &comm_section,
    1,
    sizeof (comm_section),
    &gt_ggc_mx_section,
    &gt_pch_nx_section
  },
  {
    &tls_comm_section,
    1,
    sizeof (tls_comm_section),
    &gt_ggc_mx_section,
    &gt_pch_nx_section
  },
  {
    &eh_frame_section,
    1,
    sizeof (eh_frame_section),
    &gt_ggc_mx_section,
    &gt_pch_nx_section
  },
  {
    &exception_section,
    1,
    sizeof (exception_section),
    &gt_ggc_mx_section,
    &gt_pch_nx_section
  },
  {
    &sbss_section,
    1,
    sizeof (sbss_section),
    &gt_ggc_mx_section,
    &gt_pch_nx_section
  },
  {
    &bss_section,
    1,
    sizeof (bss_section),
    &gt_ggc_mx_section,
    &gt_pch_nx_section
  },
  {
    &dtors_section,
    1,
    sizeof (dtors_section),
    &gt_ggc_mx_section,
    &gt_pch_nx_section
  },
  {
    &ctors_section,
    1,
    sizeof (ctors_section),
    &gt_ggc_mx_section,
    &gt_pch_nx_section
  },
  {
    &sdata_section,
    1,
    sizeof (sdata_section),
    &gt_ggc_mx_section,
    &gt_pch_nx_section
  },
  {
    &readonly_data_section,
    1,
    sizeof (readonly_data_section),
    &gt_ggc_mx_section,
    &gt_pch_nx_section
  },
  {
    &data_section,
    1,
    sizeof (data_section),
    &gt_ggc_mx_section,
    &gt_pch_nx_section
  },
  {
    &text_section,
    1,
    sizeof (text_section),
    &gt_ggc_mx_section,
    &gt_pch_nx_section
  },
  {
    &types_used_by_cur_var_decl,
    1,
    sizeof (types_used_by_cur_var_decl),
    &gt_ggc_mx_vec_tree_va_gc_,
    &gt_pch_nx_vec_tree_va_gc_
  },
  {
    &types_used_by_vars_hash,
    1,
    sizeof (types_used_by_vars_hash),
    &gt_ggc_mx_hash_table_used_type_hasher_,
    &gt_pch_nx_hash_table_used_type_hasher_
  },
  {
    &cfun,
    1,
    sizeof (cfun),
    &gt_ggc_mx_function,
    &gt_pch_nx_function
  },
  {
    &regno_reg_rtx,
    1,
    sizeof (regno_reg_rtx),
    &gt_ggc_ma_regno_reg_rtx,
    &gt_pch_na_regno_reg_rtx
  },
  {
    &default_target_libfuncs.x_libfunc_table[0],
    1 * (LTI_MAX),
    sizeof (default_target_libfuncs.x_libfunc_table[0]),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &default_target_libfuncs.x_libfunc_hash,
    1,
    sizeof (default_target_libfuncs.x_libfunc_hash),
    &gt_ggc_mx_hash_table_libfunc_hasher_,
    &gt_pch_nx_hash_table_libfunc_hasher_
  },
  {
    &current_function_func_begin_label,
    1,
    sizeof (current_function_func_begin_label),
    (gt_pointer_walker) &gt_ggc_m_S,
    (gt_pointer_walker) &gt_pch_n_S
  },
  {
    &current_function_decl,
    1,
    sizeof (current_function_decl),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &builtin_info[0].decl,
    1 * ((int)END_BUILTINS),
    sizeof (builtin_info[0]),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &sizetype_tab[0],
    1 * ((int) stk_type_kind_last),
    sizeof (sizetype_tab[0]),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &integer_types[0],
    1 * (itk_none),
    sizeof (integer_types[0]),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &global_trees[0],
    1 * (TI_MAX),
    sizeof (global_trees[0]),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &all_translation_units,
    1,
    sizeof (all_translation_units),
    &gt_ggc_mx_vec_tree_va_gc_,
    &gt_pch_nx_vec_tree_va_gc_
  },
  {
    &alias_pairs,
    1,
    sizeof (alias_pairs),
    &gt_ggc_mx_vec_alias_pair_va_gc_,
    &gt_pch_nx_vec_alias_pair_va_gc_
  },
  {
    &int_n_trees[0].signed_type,
    1 * (NUM_INT_N_ENTS),
    sizeof (int_n_trees[0]),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &int_n_trees[0].unsigned_type,
    1 * (NUM_INT_N_ENTS),
    sizeof (int_n_trees[0]),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &stack_limit_rtx,
    1,
    sizeof (stack_limit_rtx),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &default_target_rtl.x_global_rtl[0],
    1 * (GR_MAX),
    sizeof (default_target_rtl.x_global_rtl[0]),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &default_target_rtl.x_pic_offset_table_rtx,
    1,
    sizeof (default_target_rtl.x_pic_offset_table_rtx),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &default_target_rtl.x_return_address_pointer_rtx,
    1,
    sizeof (default_target_rtl.x_return_address_pointer_rtx),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &default_target_rtl.x_initial_regno_reg_rtx[0],
    1 * (FIRST_PSEUDO_REGISTER),
    sizeof (default_target_rtl.x_initial_regno_reg_rtx[0]),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &default_target_rtl.x_top_of_stack[0],
    1 * (MAX_MACHINE_MODE),
    sizeof (default_target_rtl.x_top_of_stack[0]),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &default_target_rtl.x_static_reg_base_value[0],
    1 * (FIRST_PSEUDO_REGISTER),
    sizeof (default_target_rtl.x_static_reg_base_value[0]),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &default_target_rtl.x_mode_mem_attrs[0],
    1 * ((int) MAX_MACHINE_MODE),
    sizeof (default_target_rtl.x_mode_mem_attrs[0]),
    &gt_ggc_mx_mem_attrs,
    &gt_pch_nx_mem_attrs
  },
  {
    &invalid_insn_rtx,
    1,
    sizeof (invalid_insn_rtx),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &simple_return_rtx,
    1,
    sizeof (simple_return_rtx),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &ret_rtx,
    1,
    sizeof (ret_rtx),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &cc0_rtx,
    1,
    sizeof (cc0_rtx),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &pc_rtx,
    1,
    sizeof (pc_rtx),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &const_tiny_rtx[0][0],
    1 * (4) * ((int) MAX_MACHINE_MODE),
    sizeof (const_tiny_rtx[0][0]),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &const_true_rtx,
    1,
    sizeof (const_true_rtx),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &const_int_rtx[0],
    1 * (MAX_SAVED_CONST_INT * 2 + 1),
    sizeof (const_int_rtx[0]),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &saved_line_table,
    1,
    sizeof (saved_line_table),
    &gt_ggc_mx_line_maps,
    &gt_pch_nx_line_maps
  },
  {
    &line_table,
    1,
    sizeof (line_table),
    &gt_ggc_mx_line_maps,
    &gt_pch_nx_line_maps
  },
  LAST_GGC_ROOT_TAB
};

EXPORTED_CONST struct ggc_root_tab gt_pch_rs_gtype_desc_c[] = {
  { &omp_requires_mask, 1, sizeof (omp_requires_mask), NULL, NULL },
  { &ipa_escaped_pt, 1, sizeof (ipa_escaped_pt), NULL, NULL },
  { &x_rtl, 1, sizeof (x_rtl), NULL, NULL },
  { &in_cold_section_p, 1, sizeof (in_cold_section_p), NULL, NULL },
  { &default_target_libfuncs, 1, sizeof (default_target_libfuncs), NULL, NULL },
  { &builtin_info, 1, sizeof (builtin_info), NULL, NULL },
  { &int_n_trees, 1, sizeof (int_n_trees), NULL, NULL },
  { &default_target_rtl, 1, sizeof (default_target_rtl), NULL, NULL },
  LAST_GGC_ROOT_TAB
};


/* Used to implement the RTX_NEXT macro.  */
EXPORTED_CONST unsigned char rtx_next[NUM_RTX_CODE] = {
  0,
  0,
  0,
  RTX_HDR_SIZE + 1 * sizeof (rtunion),
  RTX_HDR_SIZE + 1 * sizeof (rtunion),
  RTX_HDR_SIZE + 1 * sizeof (rtunion),
  0,
  0,
  RTX_HDR_SIZE + 1 * sizeof (rtunion),
  RTX_HDR_SIZE + 1 * sizeof (rtunion),
  RTX_HDR_SIZE + 1 * sizeof (rtunion),
  RTX_HDR_SIZE + 1 * sizeof (rtunion),
  RTX_HDR_SIZE + 1 * sizeof (rtunion),
  RTX_HDR_SIZE + 1 * sizeof (rtunion),
  RTX_HDR_SIZE + 1 * sizeof (rtunion),
  RTX_HDR_SIZE + 1 * sizeof (rtunion),
  RTX_HDR_SIZE + 1 * sizeof (rtunion),
  0,
  0,
  0,
  0,
  0,
  0,
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 1 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  0,
  0,
  0,
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  0,
  0,
  0,
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  0,
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  0,
  0,
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 0 * sizeof (rtunion),
  RTX_HDR_SIZE + 1 * sizeof (rtunion),
  0,
  0,
  0,
  0,
};
