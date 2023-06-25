/* Type information for cp/tree.c.
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
gt_ggc_mx_lang_type (void *x_p)
{
  struct lang_type * const x = (struct lang_type *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).primary_base);
      gt_ggc_m_22vec_tree_pair_s_va_gc_ ((*x).vcall_indices);
      gt_ggc_m_9tree_node ((*x).vtables);
      gt_ggc_m_9tree_node ((*x).typeinfo_var);
      gt_ggc_m_15vec_tree_va_gc_ ((*x).vbases);
      gt_ggc_m_15binding_table_s ((*x).nested_udts);
      gt_ggc_m_9tree_node ((*x).as_base);
      gt_ggc_m_15vec_tree_va_gc_ ((*x).pure_virtuals);
      gt_ggc_m_9tree_node ((*x).friend_classes);
      gt_ggc_m_15vec_tree_va_gc_ ((*x).members);
      gt_ggc_m_9tree_node ((*x).key_method);
      gt_ggc_m_9tree_node ((*x).decl_list);
      gt_ggc_m_9tree_node ((*x).befriending_classes);
      gt_ggc_m_9tree_node ((*x).objc_info);
      gt_ggc_m_9tree_node ((*x).lambda_expr);
    }
}

void
gt_ggc_mx_lang_decl (void *x_p)
{
  struct lang_decl * const x = (struct lang_decl *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      switch ((int) (((*x).u).base.selector))
        {
        default:
          break;
        case lds_min:
          gt_ggc_m_9tree_node ((*x).u.min.template_info);
          gt_ggc_m_9tree_node ((*x).u.min.access);
          break;
        case lds_fn:
          gt_ggc_m_9tree_node ((*x).u.fn.min.template_info);
          gt_ggc_m_9tree_node ((*x).u.fn.min.access);
          gt_ggc_m_9tree_node ((*x).u.fn.befriending_classes);
          gt_ggc_m_9tree_node ((*x).u.fn.context);
          switch ((int) (((*x).u.fn).thunk_p))
            {
            case 0:
              gt_ggc_m_9tree_node ((*x).u.fn.u5.cloned_function);
              break;
            case 1:
              break;
            default:
              break;
            }
          switch ((int) (((*x).u.fn).pending_inline_p))
            {
            case 1:
              gt_ggc_m_14cp_token_cache ((*x).u.fn.u.pending_inline_info);
              break;
            case 0:
              gt_ggc_m_9tree_node ((*x).u.fn.u.saved_auto_return_type);
              break;
            default:
              break;
            }
          break;
        case lds_ns:
          gt_ggc_m_16cp_binding_level ((*x).u.ns.level);
          gt_ggc_m_15vec_tree_va_gc_ ((*x).u.ns.inlinees);
          gt_ggc_m_27hash_table_named_decl_hash_ ((*x).u.ns.bindings);
          break;
        case lds_parm:
          break;
        case lds_decomp:
          gt_ggc_m_9tree_node ((*x).u.decomp.min.template_info);
          gt_ggc_m_9tree_node ((*x).u.decomp.min.access);
          gt_ggc_m_9tree_node ((*x).u.decomp.base);
          break;
        }
    }
}

void
gt_ggc_mx_lang_tree_node (void *x_p)
{
  union lang_tree_node * x = (union lang_tree_node *)x_p;
  union lang_tree_node * xlimit = x;
  while (ggc_test_and_set_mark (xlimit))
   xlimit = ((union lang_tree_node *) c_tree_chain_next (&(*xlimit).generic));
  while (x != xlimit)
    {
      switch ((int) (cp_tree_node_structure (&((*x)))))
        {
        case TS_CP_GENERIC:
          switch ((int) (tree_node_structure (&((*x).generic))))
            {
            case TS_BASE:
              break;
            case TS_TYPED:
              gt_ggc_m_9tree_node ((*x).generic.typed.type);
              break;
            case TS_COMMON:
              gt_ggc_m_9tree_node ((*x).generic.common.typed.type);
              gt_ggc_m_9tree_node ((*x).generic.common.chain);
              break;
            case TS_INT_CST:
              gt_ggc_m_9tree_node ((*x).generic.int_cst.typed.type);
              break;
            case TS_POLY_INT_CST:
              gt_ggc_m_9tree_node ((*x).generic.poly_int_cst.typed.type);
              {
                size_t i0;
                size_t l0 = (size_t)(NUM_POLY_INT_COEFFS);
                for (i0 = 0; i0 != l0; i0++) {
                  gt_ggc_m_9tree_node ((*x).generic.poly_int_cst.coeffs[i0]);
                }
              }
              break;
            case TS_REAL_CST:
              gt_ggc_m_9tree_node ((*x).generic.real_cst.typed.type);
              gt_ggc_m_10real_value ((*x).generic.real_cst.real_cst_ptr);
              break;
            case TS_FIXED_CST:
              gt_ggc_m_9tree_node ((*x).generic.fixed_cst.typed.type);
              gt_ggc_m_11fixed_value ((*x).generic.fixed_cst.fixed_cst_ptr);
              break;
            case TS_VECTOR:
              {
                size_t l1 = (size_t)(vector_cst_encoded_nelts ((tree) &((*x).generic.vector)));
                gt_ggc_m_9tree_node ((*x).generic.vector.typed.type);
                {
                  size_t i1;
                  for (i1 = 0; i1 != l1; i1++) {
                    gt_ggc_m_9tree_node ((*x).generic.vector.elts[i1]);
                  }
                }
              }
              break;
            case TS_STRING:
              gt_ggc_m_9tree_node ((*x).generic.string.typed.type);
              break;
            case TS_COMPLEX:
              gt_ggc_m_9tree_node ((*x).generic.complex.typed.type);
              gt_ggc_m_9tree_node ((*x).generic.complex.real);
              gt_ggc_m_9tree_node ((*x).generic.complex.imag);
              break;
            case TS_IDENTIFIER:
              gt_ggc_m_9tree_node ((*x).generic.identifier.common.typed.type);
              gt_ggc_m_9tree_node ((*x).generic.identifier.common.chain);
              gt_ggc_m_S ((*x).generic.identifier.id.str);
              break;
            case TS_DECL_MINIMAL:
              gt_ggc_m_9tree_node ((*x).generic.decl_minimal.common.typed.type);
              gt_ggc_m_9tree_node ((*x).generic.decl_minimal.common.chain);
              gt_ggc_m_9tree_node ((*x).generic.decl_minimal.name);
              gt_ggc_m_9tree_node ((*x).generic.decl_minimal.context);
              break;
            case TS_DECL_COMMON:
              gt_ggc_m_9tree_node ((*x).generic.decl_common.common.common.typed.type);
              gt_ggc_m_9tree_node ((*x).generic.decl_common.common.common.chain);
              gt_ggc_m_9tree_node ((*x).generic.decl_common.common.name);
              gt_ggc_m_9tree_node ((*x).generic.decl_common.common.context);
              gt_ggc_m_9tree_node ((*x).generic.decl_common.size);
              gt_ggc_m_9tree_node ((*x).generic.decl_common.size_unit);
              gt_ggc_m_9tree_node ((*x).generic.decl_common.initial);
              gt_ggc_m_9tree_node ((*x).generic.decl_common.attributes);
              gt_ggc_m_9tree_node ((*x).generic.decl_common.abstract_origin);
              gt_ggc_m_9lang_decl ((*x).generic.decl_common.lang_specific);
              break;
            case TS_DECL_WRTL:
              gt_ggc_m_9tree_node ((*x).generic.decl_with_rtl.common.common.common.typed.type);
              gt_ggc_m_9tree_node ((*x).generic.decl_with_rtl.common.common.common.chain);
              gt_ggc_m_9tree_node ((*x).generic.decl_with_rtl.common.common.name);
              gt_ggc_m_9tree_node ((*x).generic.decl_with_rtl.common.common.context);
              gt_ggc_m_9tree_node ((*x).generic.decl_with_rtl.common.size);
              gt_ggc_m_9tree_node ((*x).generic.decl_with_rtl.common.size_unit);
              gt_ggc_m_9tree_node ((*x).generic.decl_with_rtl.common.initial);
              gt_ggc_m_9tree_node ((*x).generic.decl_with_rtl.common.attributes);
              gt_ggc_m_9tree_node ((*x).generic.decl_with_rtl.common.abstract_origin);
              gt_ggc_m_9lang_decl ((*x).generic.decl_with_rtl.common.lang_specific);
              gt_ggc_m_7rtx_def ((*x).generic.decl_with_rtl.rtl);
              break;
            case TS_DECL_NON_COMMON:
              gt_ggc_m_9tree_node ((*x).generic.decl_non_common.common.common.common.common.common.typed.type);
              gt_ggc_m_9tree_node ((*x).generic.decl_non_common.common.common.common.common.common.chain);
              gt_ggc_m_9tree_node ((*x).generic.decl_non_common.common.common.common.common.name);
              gt_ggc_m_9tree_node ((*x).generic.decl_non_common.common.common.common.common.context);
              gt_ggc_m_9tree_node ((*x).generic.decl_non_common.common.common.common.size);
              gt_ggc_m_9tree_node ((*x).generic.decl_non_common.common.common.common.size_unit);
              gt_ggc_m_9tree_node ((*x).generic.decl_non_common.common.common.common.initial);
              gt_ggc_m_9tree_node ((*x).generic.decl_non_common.common.common.common.attributes);
              gt_ggc_m_9tree_node ((*x).generic.decl_non_common.common.common.common.abstract_origin);
              gt_ggc_m_9lang_decl ((*x).generic.decl_non_common.common.common.common.lang_specific);
              gt_ggc_m_7rtx_def ((*x).generic.decl_non_common.common.common.rtl);
              gt_ggc_m_9tree_node ((*x).generic.decl_non_common.common.assembler_name);
              gt_ggc_m_11symtab_node ((*x).generic.decl_non_common.common.symtab_node);
              gt_ggc_m_9tree_node ((*x).generic.decl_non_common.result);
              break;
            case TS_PARM_DECL:
              gt_ggc_m_9tree_node ((*x).generic.parm_decl.common.common.common.common.typed.type);
              gt_ggc_m_9tree_node ((*x).generic.parm_decl.common.common.common.common.chain);
              gt_ggc_m_9tree_node ((*x).generic.parm_decl.common.common.common.name);
              gt_ggc_m_9tree_node ((*x).generic.parm_decl.common.common.common.context);
              gt_ggc_m_9tree_node ((*x).generic.parm_decl.common.common.size);
              gt_ggc_m_9tree_node ((*x).generic.parm_decl.common.common.size_unit);
              gt_ggc_m_9tree_node ((*x).generic.parm_decl.common.common.initial);
              gt_ggc_m_9tree_node ((*x).generic.parm_decl.common.common.attributes);
              gt_ggc_m_9tree_node ((*x).generic.parm_decl.common.common.abstract_origin);
              gt_ggc_m_9lang_decl ((*x).generic.parm_decl.common.common.lang_specific);
              gt_ggc_m_7rtx_def ((*x).generic.parm_decl.common.rtl);
              gt_ggc_m_7rtx_def ((*x).generic.parm_decl.incoming_rtl);
              break;
            case TS_DECL_WITH_VIS:
              gt_ggc_m_9tree_node ((*x).generic.decl_with_vis.common.common.common.common.typed.type);
              gt_ggc_m_9tree_node ((*x).generic.decl_with_vis.common.common.common.common.chain);
              gt_ggc_m_9tree_node ((*x).generic.decl_with_vis.common.common.common.name);
              gt_ggc_m_9tree_node ((*x).generic.decl_with_vis.common.common.common.context);
              gt_ggc_m_9tree_node ((*x).generic.decl_with_vis.common.common.size);
              gt_ggc_m_9tree_node ((*x).generic.decl_with_vis.common.common.size_unit);
              gt_ggc_m_9tree_node ((*x).generic.decl_with_vis.common.common.initial);
              gt_ggc_m_9tree_node ((*x).generic.decl_with_vis.common.common.attributes);
              gt_ggc_m_9tree_node ((*x).generic.decl_with_vis.common.common.abstract_origin);
              gt_ggc_m_9lang_decl ((*x).generic.decl_with_vis.common.common.lang_specific);
              gt_ggc_m_7rtx_def ((*x).generic.decl_with_vis.common.rtl);
              gt_ggc_m_9tree_node ((*x).generic.decl_with_vis.assembler_name);
              gt_ggc_m_11symtab_node ((*x).generic.decl_with_vis.symtab_node);
              break;
            case TS_VAR_DECL:
              gt_ggc_m_9tree_node ((*x).generic.var_decl.common.common.common.common.common.typed.type);
              gt_ggc_m_9tree_node ((*x).generic.var_decl.common.common.common.common.common.chain);
              gt_ggc_m_9tree_node ((*x).generic.var_decl.common.common.common.common.name);
              gt_ggc_m_9tree_node ((*x).generic.var_decl.common.common.common.common.context);
              gt_ggc_m_9tree_node ((*x).generic.var_decl.common.common.common.size);
              gt_ggc_m_9tree_node ((*x).generic.var_decl.common.common.common.size_unit);
              gt_ggc_m_9tree_node ((*x).generic.var_decl.common.common.common.initial);
              gt_ggc_m_9tree_node ((*x).generic.var_decl.common.common.common.attributes);
              gt_ggc_m_9tree_node ((*x).generic.var_decl.common.common.common.abstract_origin);
              gt_ggc_m_9lang_decl ((*x).generic.var_decl.common.common.common.lang_specific);
              gt_ggc_m_7rtx_def ((*x).generic.var_decl.common.common.rtl);
              gt_ggc_m_9tree_node ((*x).generic.var_decl.common.assembler_name);
              gt_ggc_m_11symtab_node ((*x).generic.var_decl.common.symtab_node);
              break;
            case TS_FIELD_DECL:
              gt_ggc_m_9tree_node ((*x).generic.field_decl.common.common.common.typed.type);
              gt_ggc_m_9tree_node ((*x).generic.field_decl.common.common.common.chain);
              gt_ggc_m_9tree_node ((*x).generic.field_decl.common.common.name);
              gt_ggc_m_9tree_node ((*x).generic.field_decl.common.common.context);
              gt_ggc_m_9tree_node ((*x).generic.field_decl.common.size);
              gt_ggc_m_9tree_node ((*x).generic.field_decl.common.size_unit);
              gt_ggc_m_9tree_node ((*x).generic.field_decl.common.initial);
              gt_ggc_m_9tree_node ((*x).generic.field_decl.common.attributes);
              gt_ggc_m_9tree_node ((*x).generic.field_decl.common.abstract_origin);
              gt_ggc_m_9lang_decl ((*x).generic.field_decl.common.lang_specific);
              gt_ggc_m_9tree_node ((*x).generic.field_decl.offset);
              gt_ggc_m_9tree_node ((*x).generic.field_decl.bit_field_type);
              gt_ggc_m_9tree_node ((*x).generic.field_decl.qualifier);
              gt_ggc_m_9tree_node ((*x).generic.field_decl.bit_offset);
              gt_ggc_m_9tree_node ((*x).generic.field_decl.fcontext);
              break;
            case TS_LABEL_DECL:
              gt_ggc_m_9tree_node ((*x).generic.label_decl.common.common.common.common.typed.type);
              gt_ggc_m_9tree_node ((*x).generic.label_decl.common.common.common.common.chain);
              gt_ggc_m_9tree_node ((*x).generic.label_decl.common.common.common.name);
              gt_ggc_m_9tree_node ((*x).generic.label_decl.common.common.common.context);
              gt_ggc_m_9tree_node ((*x).generic.label_decl.common.common.size);
              gt_ggc_m_9tree_node ((*x).generic.label_decl.common.common.size_unit);
              gt_ggc_m_9tree_node ((*x).generic.label_decl.common.common.initial);
              gt_ggc_m_9tree_node ((*x).generic.label_decl.common.common.attributes);
              gt_ggc_m_9tree_node ((*x).generic.label_decl.common.common.abstract_origin);
              gt_ggc_m_9lang_decl ((*x).generic.label_decl.common.common.lang_specific);
              gt_ggc_m_7rtx_def ((*x).generic.label_decl.common.rtl);
              break;
            case TS_RESULT_DECL:
              gt_ggc_m_9tree_node ((*x).generic.result_decl.common.common.common.common.typed.type);
              gt_ggc_m_9tree_node ((*x).generic.result_decl.common.common.common.common.chain);
              gt_ggc_m_9tree_node ((*x).generic.result_decl.common.common.common.name);
              gt_ggc_m_9tree_node ((*x).generic.result_decl.common.common.common.context);
              gt_ggc_m_9tree_node ((*x).generic.result_decl.common.common.size);
              gt_ggc_m_9tree_node ((*x).generic.result_decl.common.common.size_unit);
              gt_ggc_m_9tree_node ((*x).generic.result_decl.common.common.initial);
              gt_ggc_m_9tree_node ((*x).generic.result_decl.common.common.attributes);
              gt_ggc_m_9tree_node ((*x).generic.result_decl.common.common.abstract_origin);
              gt_ggc_m_9lang_decl ((*x).generic.result_decl.common.common.lang_specific);
              gt_ggc_m_7rtx_def ((*x).generic.result_decl.common.rtl);
              break;
            case TS_CONST_DECL:
              gt_ggc_m_9tree_node ((*x).generic.const_decl.common.common.common.typed.type);
              gt_ggc_m_9tree_node ((*x).generic.const_decl.common.common.common.chain);
              gt_ggc_m_9tree_node ((*x).generic.const_decl.common.common.name);
              gt_ggc_m_9tree_node ((*x).generic.const_decl.common.common.context);
              gt_ggc_m_9tree_node ((*x).generic.const_decl.common.size);
              gt_ggc_m_9tree_node ((*x).generic.const_decl.common.size_unit);
              gt_ggc_m_9tree_node ((*x).generic.const_decl.common.initial);
              gt_ggc_m_9tree_node ((*x).generic.const_decl.common.attributes);
              gt_ggc_m_9tree_node ((*x).generic.const_decl.common.abstract_origin);
              gt_ggc_m_9lang_decl ((*x).generic.const_decl.common.lang_specific);
              break;
            case TS_TYPE_DECL:
              gt_ggc_m_9tree_node ((*x).generic.type_decl.common.common.common.common.common.common.typed.type);
              gt_ggc_m_9tree_node ((*x).generic.type_decl.common.common.common.common.common.common.chain);
              gt_ggc_m_9tree_node ((*x).generic.type_decl.common.common.common.common.common.name);
              gt_ggc_m_9tree_node ((*x).generic.type_decl.common.common.common.common.common.context);
              gt_ggc_m_9tree_node ((*x).generic.type_decl.common.common.common.common.size);
              gt_ggc_m_9tree_node ((*x).generic.type_decl.common.common.common.common.size_unit);
              gt_ggc_m_9tree_node ((*x).generic.type_decl.common.common.common.common.initial);
              gt_ggc_m_9tree_node ((*x).generic.type_decl.common.common.common.common.attributes);
              gt_ggc_m_9tree_node ((*x).generic.type_decl.common.common.common.common.abstract_origin);
              gt_ggc_m_9lang_decl ((*x).generic.type_decl.common.common.common.common.lang_specific);
              gt_ggc_m_7rtx_def ((*x).generic.type_decl.common.common.common.rtl);
              gt_ggc_m_9tree_node ((*x).generic.type_decl.common.common.assembler_name);
              gt_ggc_m_11symtab_node ((*x).generic.type_decl.common.common.symtab_node);
              gt_ggc_m_9tree_node ((*x).generic.type_decl.common.result);
              break;
            case TS_FUNCTION_DECL:
              gt_ggc_m_9tree_node ((*x).generic.function_decl.common.common.common.common.common.common.typed.type);
              gt_ggc_m_9tree_node ((*x).generic.function_decl.common.common.common.common.common.common.chain);
              gt_ggc_m_9tree_node ((*x).generic.function_decl.common.common.common.common.common.name);
              gt_ggc_m_9tree_node ((*x).generic.function_decl.common.common.common.common.common.context);
              gt_ggc_m_9tree_node ((*x).generic.function_decl.common.common.common.common.size);
              gt_ggc_m_9tree_node ((*x).generic.function_decl.common.common.common.common.size_unit);
              gt_ggc_m_9tree_node ((*x).generic.function_decl.common.common.common.common.initial);
              gt_ggc_m_9tree_node ((*x).generic.function_decl.common.common.common.common.attributes);
              gt_ggc_m_9tree_node ((*x).generic.function_decl.common.common.common.common.abstract_origin);
              gt_ggc_m_9lang_decl ((*x).generic.function_decl.common.common.common.common.lang_specific);
              gt_ggc_m_7rtx_def ((*x).generic.function_decl.common.common.common.rtl);
              gt_ggc_m_9tree_node ((*x).generic.function_decl.common.common.assembler_name);
              gt_ggc_m_11symtab_node ((*x).generic.function_decl.common.common.symtab_node);
              gt_ggc_m_9tree_node ((*x).generic.function_decl.common.result);
              gt_ggc_m_8function ((*x).generic.function_decl.f);
              gt_ggc_m_9tree_node ((*x).generic.function_decl.arguments);
              gt_ggc_m_9tree_node ((*x).generic.function_decl.personality);
              gt_ggc_m_9tree_node ((*x).generic.function_decl.function_specific_target);
              gt_ggc_m_9tree_node ((*x).generic.function_decl.function_specific_optimization);
              gt_ggc_m_9tree_node ((*x).generic.function_decl.saved_tree);
              gt_ggc_m_9tree_node ((*x).generic.function_decl.vindex);
              break;
            case TS_TRANSLATION_UNIT_DECL:
              gt_ggc_m_9tree_node ((*x).generic.translation_unit_decl.common.common.common.typed.type);
              gt_ggc_m_9tree_node ((*x).generic.translation_unit_decl.common.common.common.chain);
              gt_ggc_m_9tree_node ((*x).generic.translation_unit_decl.common.common.name);
              gt_ggc_m_9tree_node ((*x).generic.translation_unit_decl.common.common.context);
              gt_ggc_m_9tree_node ((*x).generic.translation_unit_decl.common.size);
              gt_ggc_m_9tree_node ((*x).generic.translation_unit_decl.common.size_unit);
              gt_ggc_m_9tree_node ((*x).generic.translation_unit_decl.common.initial);
              gt_ggc_m_9tree_node ((*x).generic.translation_unit_decl.common.attributes);
              gt_ggc_m_9tree_node ((*x).generic.translation_unit_decl.common.abstract_origin);
              gt_ggc_m_9lang_decl ((*x).generic.translation_unit_decl.common.lang_specific);
              break;
            case TS_TYPE_COMMON:
              gt_ggc_m_9tree_node ((*x).generic.type_common.common.typed.type);
              gt_ggc_m_9tree_node ((*x).generic.type_common.common.chain);
              gt_ggc_m_9tree_node ((*x).generic.type_common.size);
              gt_ggc_m_9tree_node ((*x).generic.type_common.size_unit);
              gt_ggc_m_9tree_node ((*x).generic.type_common.attributes);
              gt_ggc_m_9tree_node ((*x).generic.type_common.pointer_to);
              gt_ggc_m_9tree_node ((*x).generic.type_common.reference_to);
              switch ((int) (debug_hooks->tree_type_symtab_field))
                {
                case TYPE_SYMTAB_IS_ADDRESS:
                  break;
                case TYPE_SYMTAB_IS_DIE:
                  gt_ggc_m_10die_struct ((*x).generic.type_common.symtab.die);
                  break;
                default:
                  break;
                }
              gt_ggc_m_9tree_node ((*x).generic.type_common.canonical);
              gt_ggc_m_9tree_node ((*x).generic.type_common.next_variant);
              gt_ggc_m_9tree_node ((*x).generic.type_common.main_variant);
              gt_ggc_m_9tree_node ((*x).generic.type_common.context);
              gt_ggc_m_9tree_node ((*x).generic.type_common.name);
              break;
            case TS_TYPE_WITH_LANG_SPECIFIC:
              gt_ggc_m_9tree_node ((*x).generic.type_with_lang_specific.common.common.typed.type);
              gt_ggc_m_9tree_node ((*x).generic.type_with_lang_specific.common.common.chain);
              gt_ggc_m_9tree_node ((*x).generic.type_with_lang_specific.common.size);
              gt_ggc_m_9tree_node ((*x).generic.type_with_lang_specific.common.size_unit);
              gt_ggc_m_9tree_node ((*x).generic.type_with_lang_specific.common.attributes);
              gt_ggc_m_9tree_node ((*x).generic.type_with_lang_specific.common.pointer_to);
              gt_ggc_m_9tree_node ((*x).generic.type_with_lang_specific.common.reference_to);
              switch ((int) (debug_hooks->tree_type_symtab_field))
                {
                case TYPE_SYMTAB_IS_ADDRESS:
                  break;
                case TYPE_SYMTAB_IS_DIE:
                  gt_ggc_m_10die_struct ((*x).generic.type_with_lang_specific.common.symtab.die);
                  break;
                default:
                  break;
                }
              gt_ggc_m_9tree_node ((*x).generic.type_with_lang_specific.common.canonical);
              gt_ggc_m_9tree_node ((*x).generic.type_with_lang_specific.common.next_variant);
              gt_ggc_m_9tree_node ((*x).generic.type_with_lang_specific.common.main_variant);
              gt_ggc_m_9tree_node ((*x).generic.type_with_lang_specific.common.context);
              gt_ggc_m_9tree_node ((*x).generic.type_with_lang_specific.common.name);
              gt_ggc_m_9lang_type ((*x).generic.type_with_lang_specific.lang_specific);
              break;
            case TS_TYPE_NON_COMMON:
              gt_ggc_m_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.common.typed.type);
              gt_ggc_m_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.common.chain);
              gt_ggc_m_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.size);
              gt_ggc_m_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.size_unit);
              gt_ggc_m_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.attributes);
              gt_ggc_m_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.pointer_to);
              gt_ggc_m_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.reference_to);
              switch ((int) (debug_hooks->tree_type_symtab_field))
                {
                case TYPE_SYMTAB_IS_ADDRESS:
                  break;
                case TYPE_SYMTAB_IS_DIE:
                  gt_ggc_m_10die_struct ((*x).generic.type_non_common.with_lang_specific.common.symtab.die);
                  break;
                default:
                  break;
                }
              gt_ggc_m_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.canonical);
              gt_ggc_m_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.next_variant);
              gt_ggc_m_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.main_variant);
              gt_ggc_m_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.context);
              gt_ggc_m_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.name);
              gt_ggc_m_9lang_type ((*x).generic.type_non_common.with_lang_specific.lang_specific);
              gt_ggc_m_9tree_node ((*x).generic.type_non_common.values);
              gt_ggc_m_9tree_node ((*x).generic.type_non_common.minval);
              gt_ggc_m_9tree_node ((*x).generic.type_non_common.maxval);
              gt_ggc_m_9tree_node ((*x).generic.type_non_common.lang_1);
              break;
            case TS_LIST:
              gt_ggc_m_9tree_node ((*x).generic.list.common.typed.type);
              gt_ggc_m_9tree_node ((*x).generic.list.common.chain);
              gt_ggc_m_9tree_node ((*x).generic.list.purpose);
              gt_ggc_m_9tree_node ((*x).generic.list.value);
              break;
            case TS_VEC:
              {
                size_t l2 = (size_t)(TREE_VEC_LENGTH ((tree)&((*x).generic.vec)));
                gt_ggc_m_9tree_node ((*x).generic.vec.common.typed.type);
                gt_ggc_m_9tree_node ((*x).generic.vec.common.chain);
                {
                  size_t i2;
                  for (i2 = 0; i2 != l2; i2++) {
                    gt_ggc_m_9tree_node ((*x).generic.vec.a[i2]);
                  }
                }
              }
              break;
            case TS_EXP:
              gt_ggc_m_9tree_node ((*x).generic.exp.typed.type);
              switch ((int) (TREE_CODE ((tree) &(*x))))
                {
                default:
                  {
                    size_t i3;
                    size_t l3 = (size_t)(TREE_OPERAND_LENGTH ((tree) &(*x)));
                    for (i3 = 0; i3 != l3; i3++) {
                      gt_ggc_m_9tree_node ((*x).generic.exp.operands[i3]);
                    }
                  }
                  break;
                }
              break;
            case TS_SSA_NAME:
              gt_ggc_m_9tree_node ((*x).generic.ssa_name.typed.type);
              gt_ggc_m_9tree_node ((*x).generic.ssa_name.var);
              gt_ggc_m_6gimple ((*x).generic.ssa_name.def_stmt);
              switch ((int) (((*x).generic.ssa_name).typed.type ?!POINTER_TYPE_P (TREE_TYPE ((tree)&((*x).generic.ssa_name))) : 2))
                {
                case 0:
                  gt_ggc_m_12ptr_info_def ((*x).generic.ssa_name.info.ptr_info);
                  break;
                case 1:
                  gt_ggc_m_14range_info_def ((*x).generic.ssa_name.info.range_info);
                  break;
                default:
                  break;
                }
              break;
            case TS_BLOCK:
              gt_ggc_m_9tree_node ((*x).generic.block.chain);
              gt_ggc_m_9tree_node ((*x).generic.block.vars);
              gt_ggc_m_15vec_tree_va_gc_ ((*x).generic.block.nonlocalized_vars);
              gt_ggc_m_9tree_node ((*x).generic.block.subblocks);
              gt_ggc_m_9tree_node ((*x).generic.block.supercontext);
              gt_ggc_m_9tree_node ((*x).generic.block.abstract_origin);
              gt_ggc_m_9tree_node ((*x).generic.block.fragment_origin);
              gt_ggc_m_9tree_node ((*x).generic.block.fragment_chain);
              gt_ggc_m_10die_struct ((*x).generic.block.die);
              break;
            case TS_BINFO:
              gt_ggc_m_9tree_node ((*x).generic.binfo.common.typed.type);
              gt_ggc_m_9tree_node ((*x).generic.binfo.common.chain);
              gt_ggc_m_9tree_node ((*x).generic.binfo.offset);
              gt_ggc_m_9tree_node ((*x).generic.binfo.vtable);
              gt_ggc_m_9tree_node ((*x).generic.binfo.virtuals);
              gt_ggc_m_9tree_node ((*x).generic.binfo.vptr_field);
              gt_ggc_m_15vec_tree_va_gc_ ((*x).generic.binfo.base_accesses);
              gt_ggc_m_9tree_node ((*x).generic.binfo.inheritance);
              gt_ggc_m_9tree_node ((*x).generic.binfo.vtt_subvtt);
              gt_ggc_m_9tree_node ((*x).generic.binfo.vtt_vptr);
              gt_ggc_mx (&((*x).generic.binfo.base_binfos));
              break;
            case TS_STATEMENT_LIST:
              gt_ggc_m_9tree_node ((*x).generic.stmt_list.typed.type);
              gt_ggc_m_24tree_statement_list_node ((*x).generic.stmt_list.head);
              gt_ggc_m_24tree_statement_list_node ((*x).generic.stmt_list.tail);
              break;
            case TS_CONSTRUCTOR:
              gt_ggc_m_9tree_node ((*x).generic.constructor.typed.type);
              gt_ggc_m_26vec_constructor_elt_va_gc_ ((*x).generic.constructor.elts);
              break;
            case TS_OMP_CLAUSE:
              {
                size_t l4 = (size_t)(omp_clause_num_ops[OMP_CLAUSE_CODE ((tree)&((*x).generic.omp_clause))]);
                gt_ggc_m_9tree_node ((*x).generic.omp_clause.common.typed.type);
                gt_ggc_m_9tree_node ((*x).generic.omp_clause.common.chain);
                gt_ggc_m_6gimple ((*x).generic.omp_clause.gimple_reduction_init);
                gt_ggc_m_6gimple ((*x).generic.omp_clause.gimple_reduction_merge);
                {
                  size_t i4;
                  for (i4 = 0; i4 != l4; i4++) {
                    gt_ggc_m_9tree_node ((*x).generic.omp_clause.ops[i4]);
                  }
                }
              }
              break;
            case TS_OPTIMIZATION:
              gt_ggc_m_15cl_optimization ((*x).generic.optimization.opts);
              if ((*x).generic.optimization.optabs != NULL) {
                ggc_mark ((*x).generic.optimization.optabs);
              }
              break;
            case TS_TARGET_OPTION:
              gt_ggc_m_14target_globals ((*x).generic.target_option.globals);
              gt_ggc_m_16cl_target_option ((*x).generic.target_option.opts);
              break;
            default:
              break;
            }
          break;
        case TS_CP_TPI:
          gt_ggc_m_9tree_node ((*x).tpi.common.typed.type);
          gt_ggc_m_9tree_node ((*x).tpi.common.chain);
          gt_ggc_m_9tree_node ((*x).tpi.decl);
          break;
        case TS_CP_PTRMEM:
          gt_ggc_m_9tree_node ((*x).ptrmem.common.typed.type);
          gt_ggc_m_9tree_node ((*x).ptrmem.common.chain);
          gt_ggc_m_9tree_node ((*x).ptrmem.member);
          break;
        case TS_CP_OVERLOAD:
          gt_ggc_m_9tree_node ((*x).overload.common.typed.type);
          gt_ggc_m_9tree_node ((*x).overload.common.chain);
          gt_ggc_m_9tree_node ((*x).overload.function);
          break;
        case TS_CP_BASELINK:
          gt_ggc_m_9tree_node ((*x).baselink.common.typed.type);
          gt_ggc_m_9tree_node ((*x).baselink.common.chain);
          gt_ggc_m_9tree_node ((*x).baselink.binfo);
          gt_ggc_m_9tree_node ((*x).baselink.functions);
          gt_ggc_m_9tree_node ((*x).baselink.access_binfo);
          break;
        case TS_CP_TEMPLATE_DECL:
          gt_ggc_m_9tree_node ((*x).template_decl.common.common.common.typed.type);
          gt_ggc_m_9tree_node ((*x).template_decl.common.common.common.chain);
          gt_ggc_m_9tree_node ((*x).template_decl.common.common.name);
          gt_ggc_m_9tree_node ((*x).template_decl.common.common.context);
          gt_ggc_m_9tree_node ((*x).template_decl.common.size);
          gt_ggc_m_9tree_node ((*x).template_decl.common.size_unit);
          gt_ggc_m_9tree_node ((*x).template_decl.common.initial);
          gt_ggc_m_9tree_node ((*x).template_decl.common.attributes);
          gt_ggc_m_9tree_node ((*x).template_decl.common.abstract_origin);
          gt_ggc_m_9lang_decl ((*x).template_decl.common.lang_specific);
          gt_ggc_m_9tree_node ((*x).template_decl.arguments);
          gt_ggc_m_9tree_node ((*x).template_decl.result);
          break;
        case TS_CP_DEFERRED_PARSE:
          gt_ggc_m_14cp_token_cache ((*x).deferred_parse.tokens);
          gt_ggc_m_15vec_tree_va_gc_ ((*x).deferred_parse.instantiations);
          break;
        case TS_CP_DEFERRED_NOEXCEPT:
          gt_ggc_m_9tree_node ((*x).deferred_noexcept.pattern);
          gt_ggc_m_9tree_node ((*x).deferred_noexcept.args);
          break;
        case TS_CP_IDENTIFIER:
          gt_ggc_m_9tree_node ((*x).identifier.c_common.common.typed.type);
          gt_ggc_m_9tree_node ((*x).identifier.c_common.common.chain);
          gt_ggc_m_S ((*x).identifier.c_common.node.ident.str);
          switch ((int) (((*x).identifier.c_common.node).type))
            {
            case NT_VOID:
              gt_ggc_m_9cpp_macro ((*x).identifier.c_common.node.value.answers);
              break;
            case NT_USER_MACRO:
              gt_ggc_m_9cpp_macro ((*x).identifier.c_common.node.value.macro);
              break;
            case NT_BUILTIN_MACRO:
              break;
            case NT_MACRO_ARG:
              break;
            default:
              break;
            }
          gt_ggc_m_11cxx_binding ((*x).identifier.bindings);
          break;
        case TS_CP_STATIC_ASSERT:
          gt_ggc_m_9tree_node ((*x).static_assertion.common.typed.type);
          gt_ggc_m_9tree_node ((*x).static_assertion.common.chain);
          gt_ggc_m_9tree_node ((*x).static_assertion.condition);
          gt_ggc_m_9tree_node ((*x).static_assertion.message);
          break;
        case TS_CP_ARGUMENT_PACK_SELECT:
          gt_ggc_m_9tree_node ((*x).argument_pack_select.common.typed.type);
          gt_ggc_m_9tree_node ((*x).argument_pack_select.common.chain);
          gt_ggc_m_9tree_node ((*x).argument_pack_select.argument_pack);
          break;
        case TS_CP_TRAIT_EXPR:
          gt_ggc_m_9tree_node ((*x).trait_expression.common.typed.type);
          gt_ggc_m_9tree_node ((*x).trait_expression.common.chain);
          gt_ggc_m_9tree_node ((*x).trait_expression.type1);
          gt_ggc_m_9tree_node ((*x).trait_expression.type2);
          break;
        case TS_CP_LAMBDA_EXPR:
          gt_ggc_m_9tree_node ((*x).lambda_expression.typed.type);
          gt_ggc_m_9tree_node ((*x).lambda_expression.capture_list);
          gt_ggc_m_9tree_node ((*x).lambda_expression.this_capture);
          gt_ggc_m_9tree_node ((*x).lambda_expression.extra_scope);
          gt_ggc_m_15vec_tree_va_gc_ ((*x).lambda_expression.pending_proxies);
          break;
        case TS_CP_TEMPLATE_INFO:
          gt_ggc_m_9tree_node ((*x).template_info.tmpl);
          gt_ggc_m_9tree_node ((*x).template_info.args);
          gt_ggc_m_36vec_qualified_typedef_usage_t_va_gc_ ((*x).template_info.typedefs_needing_access_checking);
          break;
        case TS_CP_CONSTRAINT_INFO:
          gt_ggc_m_9tree_node ((*x).constraint_info.template_reqs);
          gt_ggc_m_9tree_node ((*x).constraint_info.declarator_reqs);
          gt_ggc_m_9tree_node ((*x).constraint_info.associated_constr);
          break;
        case TS_CP_USERDEF_LITERAL:
          gt_ggc_m_9tree_node ((*x).userdef_literal.suffix_id);
          gt_ggc_m_9tree_node ((*x).userdef_literal.value);
          gt_ggc_m_9tree_node ((*x).userdef_literal.num_string);
          break;
        default:
          break;
        }
      x = ((union lang_tree_node *) c_tree_chain_next (&(*x).generic));
    }
}

void
gt_ggc_mx_language_function (void *x_p)
{
  struct language_function * const x = (struct language_function *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_15vec_tree_va_gc_ ((*x).base.x_stmt_tree.x_cur_stmt_list);
      gt_ggc_m_15vec_tree_va_gc_ ((*x).base.local_typedefs);
      gt_ggc_m_9tree_node ((*x).x_cdtor_label);
      gt_ggc_m_9tree_node ((*x).x_current_class_ptr);
      gt_ggc_m_9tree_node ((*x).x_current_class_ref);
      gt_ggc_m_9tree_node ((*x).x_eh_spec_block);
      gt_ggc_m_9tree_node ((*x).x_in_charge_parm);
      gt_ggc_m_9tree_node ((*x).x_vtt_parm);
      gt_ggc_m_9tree_node ((*x).x_return_value);
      gt_ggc_m_28hash_table_named_label_hash_ ((*x).x_named_labels);
      gt_ggc_m_16cp_binding_level ((*x).bindings);
      gt_ggc_m_15vec_tree_va_gc_ ((*x).infinite_loops);
      gt_ggc_m_35hash_table_cxx_int_tree_map_hasher_ ((*x).extern_decl_map);
    }
}

void
gt_ggc_mx_vec_qualified_typedef_usage_t_va_gc_ (void *x_p)
{
  vec<qualified_typedef_usage_t,va_gc> * const x = (vec<qualified_typedef_usage_t,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct qualified_typedef_usage_s& x_r ATTRIBUTE_UNUSED)
{
  struct qualified_typedef_usage_s * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).typedef_decl);
  gt_ggc_m_9tree_node ((*x).context);
}

void
gt_ggc_mx_vec_cxx_saved_binding_va_gc_ (void *x_p)
{
  vec<cxx_saved_binding,va_gc> * const x = (vec<cxx_saved_binding,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct cxx_saved_binding& x_r ATTRIBUTE_UNUSED)
{
  struct cxx_saved_binding * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).identifier);
  gt_ggc_m_11cxx_binding ((*x).binding);
  gt_ggc_m_9tree_node ((*x).real_type_value);
}

void
gt_ggc_mx_saved_scope (void *x_p)
{
  struct saved_scope * const x = (struct saved_scope *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_28vec_cxx_saved_binding_va_gc_ ((*x).old_bindings);
      gt_ggc_m_9tree_node ((*x).old_namespace);
      gt_ggc_m_15vec_tree_va_gc_ ((*x).decl_ns_list);
      gt_ggc_m_9tree_node ((*x).class_name);
      gt_ggc_m_9tree_node ((*x).class_type);
      gt_ggc_m_9tree_node ((*x).access_specifier);
      gt_ggc_m_9tree_node ((*x).function_decl);
      gt_ggc_m_15vec_tree_va_gc_ ((*x).lang_base);
      gt_ggc_m_9tree_node ((*x).lang_name);
      gt_ggc_m_9tree_node ((*x).template_parms);
      gt_ggc_m_16cp_binding_level ((*x).x_previous_class_level);
      gt_ggc_m_9tree_node ((*x).x_saved_tree);
      gt_ggc_m_9tree_node ((*x).x_current_class_ptr);
      gt_ggc_m_9tree_node ((*x).x_current_class_ref);
      gt_ggc_m_15vec_tree_va_gc_ ((*x).x_stmt_tree.x_cur_stmt_list);
      gt_ggc_m_16cp_binding_level ((*x).class_bindings);
      gt_ggc_m_16cp_binding_level ((*x).bindings);
      gt_ggc_m_11saved_scope ((*x).prev);
    }
}

void
gt_ggc_mx_cxx_int_tree_map (void *x_p)
{
  struct cxx_int_tree_map * const x = (struct cxx_int_tree_map *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).to);
    }
}

void
gt_ggc_mx (struct cxx_int_tree_map& x_r ATTRIBUTE_UNUSED)
{
  struct cxx_int_tree_map * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).to);
}

void
gt_ggc_mx (struct cxx_int_tree_map *& x)
{
  if (x)
    gt_ggc_mx_cxx_int_tree_map ((void *) x);
}

void
gt_ggc_mx_hash_table_named_label_hash_ (void *x_p)
{
  hash_table<named_label_hash> * const x = (hash_table<named_label_hash> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct named_label_hash& x_r ATTRIBUTE_UNUSED)
{
  struct named_label_hash * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_hash_table_cxx_int_tree_map_hasher_ (void *x_p)
{
  hash_table<cxx_int_tree_map_hasher> * const x = (hash_table<cxx_int_tree_map_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct cxx_int_tree_map_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct cxx_int_tree_map_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_tree_pair_s (void *x_p)
{
  struct tree_pair_s * const x = (struct tree_pair_s *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).purpose);
      gt_ggc_m_9tree_node ((*x).value);
    }
}

void
gt_ggc_mx_vec_tree_pair_s_va_gc_ (void *x_p)
{
  vec<tree_pair_s,va_gc> * const x = (vec<tree_pair_s,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct tree_pair_s& x_r ATTRIBUTE_UNUSED)
{
  struct tree_pair_s * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).purpose);
  gt_ggc_m_9tree_node ((*x).value);
}

void
gt_ggc_mx_hash_table_named_decl_hash_ (void *x_p)
{
  hash_table<named_decl_hash> * const x = (hash_table<named_decl_hash> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct named_decl_hash& x_r ATTRIBUTE_UNUSED)
{
  struct named_decl_hash * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_hash_table_cplus_array_hasher_ (void *x_p)
{
  hash_table<cplus_array_hasher> * const x = (hash_table<cplus_array_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct cplus_array_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct cplus_array_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_hash_table_list_hasher_ (void *x_p)
{
  hash_table<list_hasher> * const x = (hash_table<list_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct list_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct list_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_lang_type (void *x_p)
{
  struct lang_type * const x = (struct lang_type *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_9lang_type))
    {
      gt_pch_n_9tree_node ((*x).primary_base);
      gt_pch_n_22vec_tree_pair_s_va_gc_ ((*x).vcall_indices);
      gt_pch_n_9tree_node ((*x).vtables);
      gt_pch_n_9tree_node ((*x).typeinfo_var);
      gt_pch_n_15vec_tree_va_gc_ ((*x).vbases);
      gt_pch_n_15binding_table_s ((*x).nested_udts);
      gt_pch_n_9tree_node ((*x).as_base);
      gt_pch_n_15vec_tree_va_gc_ ((*x).pure_virtuals);
      gt_pch_n_9tree_node ((*x).friend_classes);
      gt_pch_n_15vec_tree_va_gc_ ((*x).members);
      gt_pch_note_reorder ((*x).members, (*x).members, resort_type_member_vec);
      gt_pch_n_9tree_node ((*x).key_method);
      gt_pch_n_9tree_node ((*x).decl_list);
      gt_pch_n_9tree_node ((*x).befriending_classes);
      gt_pch_n_9tree_node ((*x).objc_info);
      gt_pch_n_9tree_node ((*x).lambda_expr);
    }
}

void
gt_pch_nx_lang_decl (void *x_p)
{
  struct lang_decl * const x = (struct lang_decl *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_9lang_decl))
    {
      switch ((int) (((*x).u).base.selector))
        {
        default:
          break;
        case lds_min:
          gt_pch_n_9tree_node ((*x).u.min.template_info);
          gt_pch_n_9tree_node ((*x).u.min.access);
          break;
        case lds_fn:
          gt_pch_n_9tree_node ((*x).u.fn.min.template_info);
          gt_pch_n_9tree_node ((*x).u.fn.min.access);
          gt_pch_n_9tree_node ((*x).u.fn.befriending_classes);
          gt_pch_n_9tree_node ((*x).u.fn.context);
          switch ((int) (((*x).u.fn).thunk_p))
            {
            case 0:
              gt_pch_n_9tree_node ((*x).u.fn.u5.cloned_function);
              break;
            case 1:
              break;
            default:
              break;
            }
          switch ((int) (((*x).u.fn).pending_inline_p))
            {
            case 1:
              gt_pch_n_14cp_token_cache ((*x).u.fn.u.pending_inline_info);
              break;
            case 0:
              gt_pch_n_9tree_node ((*x).u.fn.u.saved_auto_return_type);
              break;
            default:
              break;
            }
          break;
        case lds_ns:
          gt_pch_n_16cp_binding_level ((*x).u.ns.level);
          gt_pch_n_15vec_tree_va_gc_ ((*x).u.ns.inlinees);
          gt_pch_n_27hash_table_named_decl_hash_ ((*x).u.ns.bindings);
          break;
        case lds_parm:
          break;
        case lds_decomp:
          gt_pch_n_9tree_node ((*x).u.decomp.min.template_info);
          gt_pch_n_9tree_node ((*x).u.decomp.min.access);
          gt_pch_n_9tree_node ((*x).u.decomp.base);
          break;
        }
    }
}

void
gt_pch_nx_lang_tree_node (void *x_p)
{
  union lang_tree_node * x = (union lang_tree_node *)x_p;
  union lang_tree_node * xlimit = x;
  while (gt_pch_note_object (xlimit, xlimit, gt_pch_p_14lang_tree_node))
   xlimit = ((union lang_tree_node *) c_tree_chain_next (&(*xlimit).generic));
  while (x != xlimit)
    {
      switch ((int) (cp_tree_node_structure (&((*x)))))
        {
        case TS_CP_GENERIC:
          switch ((int) (tree_node_structure (&((*x).generic))))
            {
            case TS_BASE:
              break;
            case TS_TYPED:
              gt_pch_n_9tree_node ((*x).generic.typed.type);
              break;
            case TS_COMMON:
              gt_pch_n_9tree_node ((*x).generic.common.typed.type);
              gt_pch_n_9tree_node ((*x).generic.common.chain);
              break;
            case TS_INT_CST:
              gt_pch_n_9tree_node ((*x).generic.int_cst.typed.type);
              break;
            case TS_POLY_INT_CST:
              gt_pch_n_9tree_node ((*x).generic.poly_int_cst.typed.type);
              {
                size_t i0;
                size_t l0 = (size_t)(NUM_POLY_INT_COEFFS);
                for (i0 = 0; i0 != l0; i0++) {
                  gt_pch_n_9tree_node ((*x).generic.poly_int_cst.coeffs[i0]);
                }
              }
              break;
            case TS_REAL_CST:
              gt_pch_n_9tree_node ((*x).generic.real_cst.typed.type);
              gt_pch_n_10real_value ((*x).generic.real_cst.real_cst_ptr);
              break;
            case TS_FIXED_CST:
              gt_pch_n_9tree_node ((*x).generic.fixed_cst.typed.type);
              gt_pch_n_11fixed_value ((*x).generic.fixed_cst.fixed_cst_ptr);
              break;
            case TS_VECTOR:
              {
                size_t l1 = (size_t)(vector_cst_encoded_nelts ((tree) &((*x).generic.vector)));
                gt_pch_n_9tree_node ((*x).generic.vector.typed.type);
                {
                  size_t i1;
                  for (i1 = 0; i1 != l1; i1++) {
                    gt_pch_n_9tree_node ((*x).generic.vector.elts[i1]);
                  }
                }
              }
              break;
            case TS_STRING:
              gt_pch_n_9tree_node ((*x).generic.string.typed.type);
              break;
            case TS_COMPLEX:
              gt_pch_n_9tree_node ((*x).generic.complex.typed.type);
              gt_pch_n_9tree_node ((*x).generic.complex.real);
              gt_pch_n_9tree_node ((*x).generic.complex.imag);
              break;
            case TS_IDENTIFIER:
              gt_pch_n_9tree_node ((*x).generic.identifier.common.typed.type);
              gt_pch_n_9tree_node ((*x).generic.identifier.common.chain);
              gt_pch_n_S ((*x).generic.identifier.id.str);
              break;
            case TS_DECL_MINIMAL:
              gt_pch_n_9tree_node ((*x).generic.decl_minimal.common.typed.type);
              gt_pch_n_9tree_node ((*x).generic.decl_minimal.common.chain);
              gt_pch_n_9tree_node ((*x).generic.decl_minimal.name);
              gt_pch_n_9tree_node ((*x).generic.decl_minimal.context);
              break;
            case TS_DECL_COMMON:
              gt_pch_n_9tree_node ((*x).generic.decl_common.common.common.typed.type);
              gt_pch_n_9tree_node ((*x).generic.decl_common.common.common.chain);
              gt_pch_n_9tree_node ((*x).generic.decl_common.common.name);
              gt_pch_n_9tree_node ((*x).generic.decl_common.common.context);
              gt_pch_n_9tree_node ((*x).generic.decl_common.size);
              gt_pch_n_9tree_node ((*x).generic.decl_common.size_unit);
              gt_pch_n_9tree_node ((*x).generic.decl_common.initial);
              gt_pch_n_9tree_node ((*x).generic.decl_common.attributes);
              gt_pch_n_9tree_node ((*x).generic.decl_common.abstract_origin);
              gt_pch_n_9lang_decl ((*x).generic.decl_common.lang_specific);
              break;
            case TS_DECL_WRTL:
              gt_pch_n_9tree_node ((*x).generic.decl_with_rtl.common.common.common.typed.type);
              gt_pch_n_9tree_node ((*x).generic.decl_with_rtl.common.common.common.chain);
              gt_pch_n_9tree_node ((*x).generic.decl_with_rtl.common.common.name);
              gt_pch_n_9tree_node ((*x).generic.decl_with_rtl.common.common.context);
              gt_pch_n_9tree_node ((*x).generic.decl_with_rtl.common.size);
              gt_pch_n_9tree_node ((*x).generic.decl_with_rtl.common.size_unit);
              gt_pch_n_9tree_node ((*x).generic.decl_with_rtl.common.initial);
              gt_pch_n_9tree_node ((*x).generic.decl_with_rtl.common.attributes);
              gt_pch_n_9tree_node ((*x).generic.decl_with_rtl.common.abstract_origin);
              gt_pch_n_9lang_decl ((*x).generic.decl_with_rtl.common.lang_specific);
              gt_pch_n_7rtx_def ((*x).generic.decl_with_rtl.rtl);
              break;
            case TS_DECL_NON_COMMON:
              gt_pch_n_9tree_node ((*x).generic.decl_non_common.common.common.common.common.common.typed.type);
              gt_pch_n_9tree_node ((*x).generic.decl_non_common.common.common.common.common.common.chain);
              gt_pch_n_9tree_node ((*x).generic.decl_non_common.common.common.common.common.name);
              gt_pch_n_9tree_node ((*x).generic.decl_non_common.common.common.common.common.context);
              gt_pch_n_9tree_node ((*x).generic.decl_non_common.common.common.common.size);
              gt_pch_n_9tree_node ((*x).generic.decl_non_common.common.common.common.size_unit);
              gt_pch_n_9tree_node ((*x).generic.decl_non_common.common.common.common.initial);
              gt_pch_n_9tree_node ((*x).generic.decl_non_common.common.common.common.attributes);
              gt_pch_n_9tree_node ((*x).generic.decl_non_common.common.common.common.abstract_origin);
              gt_pch_n_9lang_decl ((*x).generic.decl_non_common.common.common.common.lang_specific);
              gt_pch_n_7rtx_def ((*x).generic.decl_non_common.common.common.rtl);
              gt_pch_n_9tree_node ((*x).generic.decl_non_common.common.assembler_name);
              gt_pch_n_11symtab_node ((*x).generic.decl_non_common.common.symtab_node);
              gt_pch_n_9tree_node ((*x).generic.decl_non_common.result);
              break;
            case TS_PARM_DECL:
              gt_pch_n_9tree_node ((*x).generic.parm_decl.common.common.common.common.typed.type);
              gt_pch_n_9tree_node ((*x).generic.parm_decl.common.common.common.common.chain);
              gt_pch_n_9tree_node ((*x).generic.parm_decl.common.common.common.name);
              gt_pch_n_9tree_node ((*x).generic.parm_decl.common.common.common.context);
              gt_pch_n_9tree_node ((*x).generic.parm_decl.common.common.size);
              gt_pch_n_9tree_node ((*x).generic.parm_decl.common.common.size_unit);
              gt_pch_n_9tree_node ((*x).generic.parm_decl.common.common.initial);
              gt_pch_n_9tree_node ((*x).generic.parm_decl.common.common.attributes);
              gt_pch_n_9tree_node ((*x).generic.parm_decl.common.common.abstract_origin);
              gt_pch_n_9lang_decl ((*x).generic.parm_decl.common.common.lang_specific);
              gt_pch_n_7rtx_def ((*x).generic.parm_decl.common.rtl);
              gt_pch_n_7rtx_def ((*x).generic.parm_decl.incoming_rtl);
              break;
            case TS_DECL_WITH_VIS:
              gt_pch_n_9tree_node ((*x).generic.decl_with_vis.common.common.common.common.typed.type);
              gt_pch_n_9tree_node ((*x).generic.decl_with_vis.common.common.common.common.chain);
              gt_pch_n_9tree_node ((*x).generic.decl_with_vis.common.common.common.name);
              gt_pch_n_9tree_node ((*x).generic.decl_with_vis.common.common.common.context);
              gt_pch_n_9tree_node ((*x).generic.decl_with_vis.common.common.size);
              gt_pch_n_9tree_node ((*x).generic.decl_with_vis.common.common.size_unit);
              gt_pch_n_9tree_node ((*x).generic.decl_with_vis.common.common.initial);
              gt_pch_n_9tree_node ((*x).generic.decl_with_vis.common.common.attributes);
              gt_pch_n_9tree_node ((*x).generic.decl_with_vis.common.common.abstract_origin);
              gt_pch_n_9lang_decl ((*x).generic.decl_with_vis.common.common.lang_specific);
              gt_pch_n_7rtx_def ((*x).generic.decl_with_vis.common.rtl);
              gt_pch_n_9tree_node ((*x).generic.decl_with_vis.assembler_name);
              gt_pch_n_11symtab_node ((*x).generic.decl_with_vis.symtab_node);
              break;
            case TS_VAR_DECL:
              gt_pch_n_9tree_node ((*x).generic.var_decl.common.common.common.common.common.typed.type);
              gt_pch_n_9tree_node ((*x).generic.var_decl.common.common.common.common.common.chain);
              gt_pch_n_9tree_node ((*x).generic.var_decl.common.common.common.common.name);
              gt_pch_n_9tree_node ((*x).generic.var_decl.common.common.common.common.context);
              gt_pch_n_9tree_node ((*x).generic.var_decl.common.common.common.size);
              gt_pch_n_9tree_node ((*x).generic.var_decl.common.common.common.size_unit);
              gt_pch_n_9tree_node ((*x).generic.var_decl.common.common.common.initial);
              gt_pch_n_9tree_node ((*x).generic.var_decl.common.common.common.attributes);
              gt_pch_n_9tree_node ((*x).generic.var_decl.common.common.common.abstract_origin);
              gt_pch_n_9lang_decl ((*x).generic.var_decl.common.common.common.lang_specific);
              gt_pch_n_7rtx_def ((*x).generic.var_decl.common.common.rtl);
              gt_pch_n_9tree_node ((*x).generic.var_decl.common.assembler_name);
              gt_pch_n_11symtab_node ((*x).generic.var_decl.common.symtab_node);
              break;
            case TS_FIELD_DECL:
              gt_pch_n_9tree_node ((*x).generic.field_decl.common.common.common.typed.type);
              gt_pch_n_9tree_node ((*x).generic.field_decl.common.common.common.chain);
              gt_pch_n_9tree_node ((*x).generic.field_decl.common.common.name);
              gt_pch_n_9tree_node ((*x).generic.field_decl.common.common.context);
              gt_pch_n_9tree_node ((*x).generic.field_decl.common.size);
              gt_pch_n_9tree_node ((*x).generic.field_decl.common.size_unit);
              gt_pch_n_9tree_node ((*x).generic.field_decl.common.initial);
              gt_pch_n_9tree_node ((*x).generic.field_decl.common.attributes);
              gt_pch_n_9tree_node ((*x).generic.field_decl.common.abstract_origin);
              gt_pch_n_9lang_decl ((*x).generic.field_decl.common.lang_specific);
              gt_pch_n_9tree_node ((*x).generic.field_decl.offset);
              gt_pch_n_9tree_node ((*x).generic.field_decl.bit_field_type);
              gt_pch_n_9tree_node ((*x).generic.field_decl.qualifier);
              gt_pch_n_9tree_node ((*x).generic.field_decl.bit_offset);
              gt_pch_n_9tree_node ((*x).generic.field_decl.fcontext);
              break;
            case TS_LABEL_DECL:
              gt_pch_n_9tree_node ((*x).generic.label_decl.common.common.common.common.typed.type);
              gt_pch_n_9tree_node ((*x).generic.label_decl.common.common.common.common.chain);
              gt_pch_n_9tree_node ((*x).generic.label_decl.common.common.common.name);
              gt_pch_n_9tree_node ((*x).generic.label_decl.common.common.common.context);
              gt_pch_n_9tree_node ((*x).generic.label_decl.common.common.size);
              gt_pch_n_9tree_node ((*x).generic.label_decl.common.common.size_unit);
              gt_pch_n_9tree_node ((*x).generic.label_decl.common.common.initial);
              gt_pch_n_9tree_node ((*x).generic.label_decl.common.common.attributes);
              gt_pch_n_9tree_node ((*x).generic.label_decl.common.common.abstract_origin);
              gt_pch_n_9lang_decl ((*x).generic.label_decl.common.common.lang_specific);
              gt_pch_n_7rtx_def ((*x).generic.label_decl.common.rtl);
              break;
            case TS_RESULT_DECL:
              gt_pch_n_9tree_node ((*x).generic.result_decl.common.common.common.common.typed.type);
              gt_pch_n_9tree_node ((*x).generic.result_decl.common.common.common.common.chain);
              gt_pch_n_9tree_node ((*x).generic.result_decl.common.common.common.name);
              gt_pch_n_9tree_node ((*x).generic.result_decl.common.common.common.context);
              gt_pch_n_9tree_node ((*x).generic.result_decl.common.common.size);
              gt_pch_n_9tree_node ((*x).generic.result_decl.common.common.size_unit);
              gt_pch_n_9tree_node ((*x).generic.result_decl.common.common.initial);
              gt_pch_n_9tree_node ((*x).generic.result_decl.common.common.attributes);
              gt_pch_n_9tree_node ((*x).generic.result_decl.common.common.abstract_origin);
              gt_pch_n_9lang_decl ((*x).generic.result_decl.common.common.lang_specific);
              gt_pch_n_7rtx_def ((*x).generic.result_decl.common.rtl);
              break;
            case TS_CONST_DECL:
              gt_pch_n_9tree_node ((*x).generic.const_decl.common.common.common.typed.type);
              gt_pch_n_9tree_node ((*x).generic.const_decl.common.common.common.chain);
              gt_pch_n_9tree_node ((*x).generic.const_decl.common.common.name);
              gt_pch_n_9tree_node ((*x).generic.const_decl.common.common.context);
              gt_pch_n_9tree_node ((*x).generic.const_decl.common.size);
              gt_pch_n_9tree_node ((*x).generic.const_decl.common.size_unit);
              gt_pch_n_9tree_node ((*x).generic.const_decl.common.initial);
              gt_pch_n_9tree_node ((*x).generic.const_decl.common.attributes);
              gt_pch_n_9tree_node ((*x).generic.const_decl.common.abstract_origin);
              gt_pch_n_9lang_decl ((*x).generic.const_decl.common.lang_specific);
              break;
            case TS_TYPE_DECL:
              gt_pch_n_9tree_node ((*x).generic.type_decl.common.common.common.common.common.common.typed.type);
              gt_pch_n_9tree_node ((*x).generic.type_decl.common.common.common.common.common.common.chain);
              gt_pch_n_9tree_node ((*x).generic.type_decl.common.common.common.common.common.name);
              gt_pch_n_9tree_node ((*x).generic.type_decl.common.common.common.common.common.context);
              gt_pch_n_9tree_node ((*x).generic.type_decl.common.common.common.common.size);
              gt_pch_n_9tree_node ((*x).generic.type_decl.common.common.common.common.size_unit);
              gt_pch_n_9tree_node ((*x).generic.type_decl.common.common.common.common.initial);
              gt_pch_n_9tree_node ((*x).generic.type_decl.common.common.common.common.attributes);
              gt_pch_n_9tree_node ((*x).generic.type_decl.common.common.common.common.abstract_origin);
              gt_pch_n_9lang_decl ((*x).generic.type_decl.common.common.common.common.lang_specific);
              gt_pch_n_7rtx_def ((*x).generic.type_decl.common.common.common.rtl);
              gt_pch_n_9tree_node ((*x).generic.type_decl.common.common.assembler_name);
              gt_pch_n_11symtab_node ((*x).generic.type_decl.common.common.symtab_node);
              gt_pch_n_9tree_node ((*x).generic.type_decl.common.result);
              break;
            case TS_FUNCTION_DECL:
              gt_pch_n_9tree_node ((*x).generic.function_decl.common.common.common.common.common.common.typed.type);
              gt_pch_n_9tree_node ((*x).generic.function_decl.common.common.common.common.common.common.chain);
              gt_pch_n_9tree_node ((*x).generic.function_decl.common.common.common.common.common.name);
              gt_pch_n_9tree_node ((*x).generic.function_decl.common.common.common.common.common.context);
              gt_pch_n_9tree_node ((*x).generic.function_decl.common.common.common.common.size);
              gt_pch_n_9tree_node ((*x).generic.function_decl.common.common.common.common.size_unit);
              gt_pch_n_9tree_node ((*x).generic.function_decl.common.common.common.common.initial);
              gt_pch_n_9tree_node ((*x).generic.function_decl.common.common.common.common.attributes);
              gt_pch_n_9tree_node ((*x).generic.function_decl.common.common.common.common.abstract_origin);
              gt_pch_n_9lang_decl ((*x).generic.function_decl.common.common.common.common.lang_specific);
              gt_pch_n_7rtx_def ((*x).generic.function_decl.common.common.common.rtl);
              gt_pch_n_9tree_node ((*x).generic.function_decl.common.common.assembler_name);
              gt_pch_n_11symtab_node ((*x).generic.function_decl.common.common.symtab_node);
              gt_pch_n_9tree_node ((*x).generic.function_decl.common.result);
              gt_pch_n_8function ((*x).generic.function_decl.f);
              gt_pch_n_9tree_node ((*x).generic.function_decl.arguments);
              gt_pch_n_9tree_node ((*x).generic.function_decl.personality);
              gt_pch_n_9tree_node ((*x).generic.function_decl.function_specific_target);
              gt_pch_n_9tree_node ((*x).generic.function_decl.function_specific_optimization);
              gt_pch_n_9tree_node ((*x).generic.function_decl.saved_tree);
              gt_pch_n_9tree_node ((*x).generic.function_decl.vindex);
              break;
            case TS_TRANSLATION_UNIT_DECL:
              gt_pch_n_9tree_node ((*x).generic.translation_unit_decl.common.common.common.typed.type);
              gt_pch_n_9tree_node ((*x).generic.translation_unit_decl.common.common.common.chain);
              gt_pch_n_9tree_node ((*x).generic.translation_unit_decl.common.common.name);
              gt_pch_n_9tree_node ((*x).generic.translation_unit_decl.common.common.context);
              gt_pch_n_9tree_node ((*x).generic.translation_unit_decl.common.size);
              gt_pch_n_9tree_node ((*x).generic.translation_unit_decl.common.size_unit);
              gt_pch_n_9tree_node ((*x).generic.translation_unit_decl.common.initial);
              gt_pch_n_9tree_node ((*x).generic.translation_unit_decl.common.attributes);
              gt_pch_n_9tree_node ((*x).generic.translation_unit_decl.common.abstract_origin);
              gt_pch_n_9lang_decl ((*x).generic.translation_unit_decl.common.lang_specific);
              break;
            case TS_TYPE_COMMON:
              gt_pch_n_9tree_node ((*x).generic.type_common.common.typed.type);
              gt_pch_n_9tree_node ((*x).generic.type_common.common.chain);
              gt_pch_n_9tree_node ((*x).generic.type_common.size);
              gt_pch_n_9tree_node ((*x).generic.type_common.size_unit);
              gt_pch_n_9tree_node ((*x).generic.type_common.attributes);
              gt_pch_n_9tree_node ((*x).generic.type_common.pointer_to);
              gt_pch_n_9tree_node ((*x).generic.type_common.reference_to);
              switch ((int) (debug_hooks->tree_type_symtab_field))
                {
                case TYPE_SYMTAB_IS_ADDRESS:
                  break;
                case TYPE_SYMTAB_IS_DIE:
                  gt_pch_n_10die_struct ((*x).generic.type_common.symtab.die);
                  break;
                default:
                  break;
                }
              gt_pch_n_9tree_node ((*x).generic.type_common.canonical);
              gt_pch_n_9tree_node ((*x).generic.type_common.next_variant);
              gt_pch_n_9tree_node ((*x).generic.type_common.main_variant);
              gt_pch_n_9tree_node ((*x).generic.type_common.context);
              gt_pch_n_9tree_node ((*x).generic.type_common.name);
              break;
            case TS_TYPE_WITH_LANG_SPECIFIC:
              gt_pch_n_9tree_node ((*x).generic.type_with_lang_specific.common.common.typed.type);
              gt_pch_n_9tree_node ((*x).generic.type_with_lang_specific.common.common.chain);
              gt_pch_n_9tree_node ((*x).generic.type_with_lang_specific.common.size);
              gt_pch_n_9tree_node ((*x).generic.type_with_lang_specific.common.size_unit);
              gt_pch_n_9tree_node ((*x).generic.type_with_lang_specific.common.attributes);
              gt_pch_n_9tree_node ((*x).generic.type_with_lang_specific.common.pointer_to);
              gt_pch_n_9tree_node ((*x).generic.type_with_lang_specific.common.reference_to);
              switch ((int) (debug_hooks->tree_type_symtab_field))
                {
                case TYPE_SYMTAB_IS_ADDRESS:
                  break;
                case TYPE_SYMTAB_IS_DIE:
                  gt_pch_n_10die_struct ((*x).generic.type_with_lang_specific.common.symtab.die);
                  break;
                default:
                  break;
                }
              gt_pch_n_9tree_node ((*x).generic.type_with_lang_specific.common.canonical);
              gt_pch_n_9tree_node ((*x).generic.type_with_lang_specific.common.next_variant);
              gt_pch_n_9tree_node ((*x).generic.type_with_lang_specific.common.main_variant);
              gt_pch_n_9tree_node ((*x).generic.type_with_lang_specific.common.context);
              gt_pch_n_9tree_node ((*x).generic.type_with_lang_specific.common.name);
              gt_pch_n_9lang_type ((*x).generic.type_with_lang_specific.lang_specific);
              break;
            case TS_TYPE_NON_COMMON:
              gt_pch_n_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.common.typed.type);
              gt_pch_n_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.common.chain);
              gt_pch_n_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.size);
              gt_pch_n_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.size_unit);
              gt_pch_n_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.attributes);
              gt_pch_n_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.pointer_to);
              gt_pch_n_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.reference_to);
              switch ((int) (debug_hooks->tree_type_symtab_field))
                {
                case TYPE_SYMTAB_IS_ADDRESS:
                  break;
                case TYPE_SYMTAB_IS_DIE:
                  gt_pch_n_10die_struct ((*x).generic.type_non_common.with_lang_specific.common.symtab.die);
                  break;
                default:
                  break;
                }
              gt_pch_n_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.canonical);
              gt_pch_n_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.next_variant);
              gt_pch_n_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.main_variant);
              gt_pch_n_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.context);
              gt_pch_n_9tree_node ((*x).generic.type_non_common.with_lang_specific.common.name);
              gt_pch_n_9lang_type ((*x).generic.type_non_common.with_lang_specific.lang_specific);
              gt_pch_n_9tree_node ((*x).generic.type_non_common.values);
              gt_pch_n_9tree_node ((*x).generic.type_non_common.minval);
              gt_pch_n_9tree_node ((*x).generic.type_non_common.maxval);
              gt_pch_n_9tree_node ((*x).generic.type_non_common.lang_1);
              break;
            case TS_LIST:
              gt_pch_n_9tree_node ((*x).generic.list.common.typed.type);
              gt_pch_n_9tree_node ((*x).generic.list.common.chain);
              gt_pch_n_9tree_node ((*x).generic.list.purpose);
              gt_pch_n_9tree_node ((*x).generic.list.value);
              break;
            case TS_VEC:
              {
                size_t l2 = (size_t)(TREE_VEC_LENGTH ((tree)&((*x).generic.vec)));
                gt_pch_n_9tree_node ((*x).generic.vec.common.typed.type);
                gt_pch_n_9tree_node ((*x).generic.vec.common.chain);
                {
                  size_t i2;
                  for (i2 = 0; i2 != l2; i2++) {
                    gt_pch_n_9tree_node ((*x).generic.vec.a[i2]);
                  }
                }
              }
              break;
            case TS_EXP:
              gt_pch_n_9tree_node ((*x).generic.exp.typed.type);
              switch ((int) (TREE_CODE ((tree) &(*x))))
                {
                default:
                  {
                    size_t i3;
                    size_t l3 = (size_t)(TREE_OPERAND_LENGTH ((tree) &(*x)));
                    for (i3 = 0; i3 != l3; i3++) {
                      gt_pch_n_9tree_node ((*x).generic.exp.operands[i3]);
                    }
                  }
                  break;
                }
              break;
            case TS_SSA_NAME:
              gt_pch_n_9tree_node ((*x).generic.ssa_name.typed.type);
              gt_pch_n_9tree_node ((*x).generic.ssa_name.var);
              gt_pch_n_6gimple ((*x).generic.ssa_name.def_stmt);
              switch ((int) (((*x).generic.ssa_name).typed.type ?!POINTER_TYPE_P (TREE_TYPE ((tree)&((*x).generic.ssa_name))) : 2))
                {
                case 0:
                  gt_pch_n_12ptr_info_def ((*x).generic.ssa_name.info.ptr_info);
                  break;
                case 1:
                  gt_pch_n_14range_info_def ((*x).generic.ssa_name.info.range_info);
                  break;
                default:
                  break;
                }
              break;
            case TS_BLOCK:
              gt_pch_n_9tree_node ((*x).generic.block.chain);
              gt_pch_n_9tree_node ((*x).generic.block.vars);
              gt_pch_n_15vec_tree_va_gc_ ((*x).generic.block.nonlocalized_vars);
              gt_pch_n_9tree_node ((*x).generic.block.subblocks);
              gt_pch_n_9tree_node ((*x).generic.block.supercontext);
              gt_pch_n_9tree_node ((*x).generic.block.abstract_origin);
              gt_pch_n_9tree_node ((*x).generic.block.fragment_origin);
              gt_pch_n_9tree_node ((*x).generic.block.fragment_chain);
              gt_pch_n_10die_struct ((*x).generic.block.die);
              break;
            case TS_BINFO:
              gt_pch_n_9tree_node ((*x).generic.binfo.common.typed.type);
              gt_pch_n_9tree_node ((*x).generic.binfo.common.chain);
              gt_pch_n_9tree_node ((*x).generic.binfo.offset);
              gt_pch_n_9tree_node ((*x).generic.binfo.vtable);
              gt_pch_n_9tree_node ((*x).generic.binfo.virtuals);
              gt_pch_n_9tree_node ((*x).generic.binfo.vptr_field);
              gt_pch_n_15vec_tree_va_gc_ ((*x).generic.binfo.base_accesses);
              gt_pch_n_9tree_node ((*x).generic.binfo.inheritance);
              gt_pch_n_9tree_node ((*x).generic.binfo.vtt_subvtt);
              gt_pch_n_9tree_node ((*x).generic.binfo.vtt_vptr);
              gt_pch_nx (&((*x).generic.binfo.base_binfos));
              break;
            case TS_STATEMENT_LIST:
              gt_pch_n_9tree_node ((*x).generic.stmt_list.typed.type);
              gt_pch_n_24tree_statement_list_node ((*x).generic.stmt_list.head);
              gt_pch_n_24tree_statement_list_node ((*x).generic.stmt_list.tail);
              break;
            case TS_CONSTRUCTOR:
              gt_pch_n_9tree_node ((*x).generic.constructor.typed.type);
              gt_pch_n_26vec_constructor_elt_va_gc_ ((*x).generic.constructor.elts);
              break;
            case TS_OMP_CLAUSE:
              {
                size_t l4 = (size_t)(omp_clause_num_ops[OMP_CLAUSE_CODE ((tree)&((*x).generic.omp_clause))]);
                gt_pch_n_9tree_node ((*x).generic.omp_clause.common.typed.type);
                gt_pch_n_9tree_node ((*x).generic.omp_clause.common.chain);
                gt_pch_n_6gimple ((*x).generic.omp_clause.gimple_reduction_init);
                gt_pch_n_6gimple ((*x).generic.omp_clause.gimple_reduction_merge);
                {
                  size_t i4;
                  for (i4 = 0; i4 != l4; i4++) {
                    gt_pch_n_9tree_node ((*x).generic.omp_clause.ops[i4]);
                  }
                }
              }
              break;
            case TS_OPTIMIZATION:
              gt_pch_n_15cl_optimization ((*x).generic.optimization.opts);
              if ((*x).generic.optimization.optabs != NULL) {
                gt_pch_note_object ((*x).generic.optimization.optabs, x, gt_pch_p_14lang_tree_node);
              }
              break;
            case TS_TARGET_OPTION:
              gt_pch_n_14target_globals ((*x).generic.target_option.globals);
              gt_pch_n_16cl_target_option ((*x).generic.target_option.opts);
              break;
            default:
              break;
            }
          break;
        case TS_CP_TPI:
          gt_pch_n_9tree_node ((*x).tpi.common.typed.type);
          gt_pch_n_9tree_node ((*x).tpi.common.chain);
          gt_pch_n_9tree_node ((*x).tpi.decl);
          break;
        case TS_CP_PTRMEM:
          gt_pch_n_9tree_node ((*x).ptrmem.common.typed.type);
          gt_pch_n_9tree_node ((*x).ptrmem.common.chain);
          gt_pch_n_9tree_node ((*x).ptrmem.member);
          break;
        case TS_CP_OVERLOAD:
          gt_pch_n_9tree_node ((*x).overload.common.typed.type);
          gt_pch_n_9tree_node ((*x).overload.common.chain);
          gt_pch_n_9tree_node ((*x).overload.function);
          break;
        case TS_CP_BASELINK:
          gt_pch_n_9tree_node ((*x).baselink.common.typed.type);
          gt_pch_n_9tree_node ((*x).baselink.common.chain);
          gt_pch_n_9tree_node ((*x).baselink.binfo);
          gt_pch_n_9tree_node ((*x).baselink.functions);
          gt_pch_n_9tree_node ((*x).baselink.access_binfo);
          break;
        case TS_CP_TEMPLATE_DECL:
          gt_pch_n_9tree_node ((*x).template_decl.common.common.common.typed.type);
          gt_pch_n_9tree_node ((*x).template_decl.common.common.common.chain);
          gt_pch_n_9tree_node ((*x).template_decl.common.common.name);
          gt_pch_n_9tree_node ((*x).template_decl.common.common.context);
          gt_pch_n_9tree_node ((*x).template_decl.common.size);
          gt_pch_n_9tree_node ((*x).template_decl.common.size_unit);
          gt_pch_n_9tree_node ((*x).template_decl.common.initial);
          gt_pch_n_9tree_node ((*x).template_decl.common.attributes);
          gt_pch_n_9tree_node ((*x).template_decl.common.abstract_origin);
          gt_pch_n_9lang_decl ((*x).template_decl.common.lang_specific);
          gt_pch_n_9tree_node ((*x).template_decl.arguments);
          gt_pch_n_9tree_node ((*x).template_decl.result);
          break;
        case TS_CP_DEFERRED_PARSE:
          gt_pch_n_14cp_token_cache ((*x).deferred_parse.tokens);
          gt_pch_n_15vec_tree_va_gc_ ((*x).deferred_parse.instantiations);
          break;
        case TS_CP_DEFERRED_NOEXCEPT:
          gt_pch_n_9tree_node ((*x).deferred_noexcept.pattern);
          gt_pch_n_9tree_node ((*x).deferred_noexcept.args);
          break;
        case TS_CP_IDENTIFIER:
          gt_pch_n_9tree_node ((*x).identifier.c_common.common.typed.type);
          gt_pch_n_9tree_node ((*x).identifier.c_common.common.chain);
          gt_pch_n_S ((*x).identifier.c_common.node.ident.str);
          switch ((int) (((*x).identifier.c_common.node).type))
            {
            case NT_VOID:
              gt_pch_n_9cpp_macro ((*x).identifier.c_common.node.value.answers);
              break;
            case NT_USER_MACRO:
              gt_pch_n_9cpp_macro ((*x).identifier.c_common.node.value.macro);
              break;
            case NT_BUILTIN_MACRO:
              break;
            case NT_MACRO_ARG:
              break;
            default:
              break;
            }
          gt_pch_n_11cxx_binding ((*x).identifier.bindings);
          break;
        case TS_CP_STATIC_ASSERT:
          gt_pch_n_9tree_node ((*x).static_assertion.common.typed.type);
          gt_pch_n_9tree_node ((*x).static_assertion.common.chain);
          gt_pch_n_9tree_node ((*x).static_assertion.condition);
          gt_pch_n_9tree_node ((*x).static_assertion.message);
          break;
        case TS_CP_ARGUMENT_PACK_SELECT:
          gt_pch_n_9tree_node ((*x).argument_pack_select.common.typed.type);
          gt_pch_n_9tree_node ((*x).argument_pack_select.common.chain);
          gt_pch_n_9tree_node ((*x).argument_pack_select.argument_pack);
          break;
        case TS_CP_TRAIT_EXPR:
          gt_pch_n_9tree_node ((*x).trait_expression.common.typed.type);
          gt_pch_n_9tree_node ((*x).trait_expression.common.chain);
          gt_pch_n_9tree_node ((*x).trait_expression.type1);
          gt_pch_n_9tree_node ((*x).trait_expression.type2);
          break;
        case TS_CP_LAMBDA_EXPR:
          gt_pch_n_9tree_node ((*x).lambda_expression.typed.type);
          gt_pch_n_9tree_node ((*x).lambda_expression.capture_list);
          gt_pch_n_9tree_node ((*x).lambda_expression.this_capture);
          gt_pch_n_9tree_node ((*x).lambda_expression.extra_scope);
          gt_pch_n_15vec_tree_va_gc_ ((*x).lambda_expression.pending_proxies);
          break;
        case TS_CP_TEMPLATE_INFO:
          gt_pch_n_9tree_node ((*x).template_info.tmpl);
          gt_pch_n_9tree_node ((*x).template_info.args);
          gt_pch_n_36vec_qualified_typedef_usage_t_va_gc_ ((*x).template_info.typedefs_needing_access_checking);
          break;
        case TS_CP_CONSTRAINT_INFO:
          gt_pch_n_9tree_node ((*x).constraint_info.template_reqs);
          gt_pch_n_9tree_node ((*x).constraint_info.declarator_reqs);
          gt_pch_n_9tree_node ((*x).constraint_info.associated_constr);
          break;
        case TS_CP_USERDEF_LITERAL:
          gt_pch_n_9tree_node ((*x).userdef_literal.suffix_id);
          gt_pch_n_9tree_node ((*x).userdef_literal.value);
          gt_pch_n_9tree_node ((*x).userdef_literal.num_string);
          break;
        default:
          break;
        }
      x = ((union lang_tree_node *) c_tree_chain_next (&(*x).generic));
    }
}

void
gt_pch_nx_language_function (void *x_p)
{
  struct language_function * const x = (struct language_function *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_17language_function))
    {
      gt_pch_n_15vec_tree_va_gc_ ((*x).base.x_stmt_tree.x_cur_stmt_list);
      gt_pch_n_15vec_tree_va_gc_ ((*x).base.local_typedefs);
      gt_pch_n_9tree_node ((*x).x_cdtor_label);
      gt_pch_n_9tree_node ((*x).x_current_class_ptr);
      gt_pch_n_9tree_node ((*x).x_current_class_ref);
      gt_pch_n_9tree_node ((*x).x_eh_spec_block);
      gt_pch_n_9tree_node ((*x).x_in_charge_parm);
      gt_pch_n_9tree_node ((*x).x_vtt_parm);
      gt_pch_n_9tree_node ((*x).x_return_value);
      gt_pch_n_28hash_table_named_label_hash_ ((*x).x_named_labels);
      gt_pch_n_16cp_binding_level ((*x).bindings);
      gt_pch_n_15vec_tree_va_gc_ ((*x).infinite_loops);
      gt_pch_n_35hash_table_cxx_int_tree_map_hasher_ ((*x).extern_decl_map);
    }
}

void
gt_pch_nx_vec_qualified_typedef_usage_t_va_gc_ (void *x_p)
{
  vec<qualified_typedef_usage_t,va_gc> * const x = (vec<qualified_typedef_usage_t,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_36vec_qualified_typedef_usage_t_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct qualified_typedef_usage_s& x_r ATTRIBUTE_UNUSED)
{
  struct qualified_typedef_usage_s * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).typedef_decl);
  gt_pch_n_9tree_node ((*x).context);
}

void
gt_pch_nx_vec_cxx_saved_binding_va_gc_ (void *x_p)
{
  vec<cxx_saved_binding,va_gc> * const x = (vec<cxx_saved_binding,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_28vec_cxx_saved_binding_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct cxx_saved_binding& x_r ATTRIBUTE_UNUSED)
{
  struct cxx_saved_binding * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).identifier);
  gt_pch_n_11cxx_binding ((*x).binding);
  gt_pch_n_9tree_node ((*x).real_type_value);
}

void
gt_pch_nx_saved_scope (void *x_p)
{
  struct saved_scope * const x = (struct saved_scope *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_11saved_scope))
    {
      gt_pch_n_28vec_cxx_saved_binding_va_gc_ ((*x).old_bindings);
      gt_pch_n_9tree_node ((*x).old_namespace);
      gt_pch_n_15vec_tree_va_gc_ ((*x).decl_ns_list);
      gt_pch_n_9tree_node ((*x).class_name);
      gt_pch_n_9tree_node ((*x).class_type);
      gt_pch_n_9tree_node ((*x).access_specifier);
      gt_pch_n_9tree_node ((*x).function_decl);
      gt_pch_n_15vec_tree_va_gc_ ((*x).lang_base);
      gt_pch_n_9tree_node ((*x).lang_name);
      gt_pch_n_9tree_node ((*x).template_parms);
      gt_pch_n_16cp_binding_level ((*x).x_previous_class_level);
      gt_pch_n_9tree_node ((*x).x_saved_tree);
      gt_pch_n_9tree_node ((*x).x_current_class_ptr);
      gt_pch_n_9tree_node ((*x).x_current_class_ref);
      gt_pch_n_15vec_tree_va_gc_ ((*x).x_stmt_tree.x_cur_stmt_list);
      gt_pch_n_16cp_binding_level ((*x).class_bindings);
      gt_pch_n_16cp_binding_level ((*x).bindings);
      gt_pch_n_11saved_scope ((*x).prev);
    }
}

void
gt_pch_nx_cxx_int_tree_map (void *x_p)
{
  struct cxx_int_tree_map * const x = (struct cxx_int_tree_map *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_16cxx_int_tree_map))
    {
      gt_pch_n_9tree_node ((*x).to);
    }
}

void
gt_pch_nx (struct cxx_int_tree_map& x_r ATTRIBUTE_UNUSED)
{
  struct cxx_int_tree_map * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).to);
}

void
gt_pch_nx (struct cxx_int_tree_map *& x)
{
  if (x)
    gt_pch_nx_cxx_int_tree_map ((void *) x);
}

void
gt_pch_nx_hash_table_named_label_hash_ (void *x_p)
{
  hash_table<named_label_hash> * const x = (hash_table<named_label_hash> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_28hash_table_named_label_hash_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct named_label_hash& x_r ATTRIBUTE_UNUSED)
{
  struct named_label_hash * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_hash_table_cxx_int_tree_map_hasher_ (void *x_p)
{
  hash_table<cxx_int_tree_map_hasher> * const x = (hash_table<cxx_int_tree_map_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_35hash_table_cxx_int_tree_map_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct cxx_int_tree_map_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct cxx_int_tree_map_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_tree_pair_s (void *x_p)
{
  struct tree_pair_s * const x = (struct tree_pair_s *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_11tree_pair_s))
    {
      gt_pch_n_9tree_node ((*x).purpose);
      gt_pch_n_9tree_node ((*x).value);
    }
}

void
gt_pch_nx_vec_tree_pair_s_va_gc_ (void *x_p)
{
  vec<tree_pair_s,va_gc> * const x = (vec<tree_pair_s,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_22vec_tree_pair_s_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct tree_pair_s& x_r ATTRIBUTE_UNUSED)
{
  struct tree_pair_s * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).purpose);
  gt_pch_n_9tree_node ((*x).value);
}

void
gt_pch_nx_hash_table_named_decl_hash_ (void *x_p)
{
  hash_table<named_decl_hash> * const x = (hash_table<named_decl_hash> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_27hash_table_named_decl_hash_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct named_decl_hash& x_r ATTRIBUTE_UNUSED)
{
  struct named_decl_hash * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_hash_table_cplus_array_hasher_ (void *x_p)
{
  hash_table<cplus_array_hasher> * const x = (hash_table<cplus_array_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_30hash_table_cplus_array_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct cplus_array_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct cplus_array_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_hash_table_list_hasher_ (void *x_p)
{
  hash_table<list_hasher> * const x = (hash_table<list_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_23hash_table_list_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct list_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct list_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_p_9lang_type (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct lang_type * x ATTRIBUTE_UNUSED = (struct lang_type *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).primary_base), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).vcall_indices), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).vtables), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).typeinfo_var), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).vbases), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).nested_udts), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).as_base), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).pure_virtuals), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).friend_classes), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).members), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).key_method), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).decl_list), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).befriending_classes), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).objc_info), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).lambda_expr), cookie);
}

void
gt_pch_p_9lang_decl (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct lang_decl * x ATTRIBUTE_UNUSED = (struct lang_decl *)x_p;
  switch ((int) (((*x).u).base.selector))
    {
    default:
      break;
    case lds_min:
      if ((void *)(x) == this_obj)
        op (&((*x).u.min.template_info), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).u.min.access), cookie);
      break;
    case lds_fn:
      if ((void *)(x) == this_obj)
        op (&((*x).u.fn.min.template_info), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).u.fn.min.access), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).u.fn.befriending_classes), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).u.fn.context), cookie);
      switch ((int) (((*x).u.fn).thunk_p))
        {
        case 0:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fn.u5.cloned_function), cookie);
          break;
        case 1:
          break;
        default:
          break;
        }
      switch ((int) (((*x).u.fn).pending_inline_p))
        {
        case 1:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fn.u.pending_inline_info), cookie);
          break;
        case 0:
          if ((void *)(x) == this_obj)
            op (&((*x).u.fn.u.saved_auto_return_type), cookie);
          break;
        default:
          break;
        }
      break;
    case lds_ns:
      if ((void *)(x) == this_obj)
        op (&((*x).u.ns.level), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).u.ns.inlinees), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).u.ns.bindings), cookie);
      break;
    case lds_parm:
      break;
    case lds_decomp:
      if ((void *)(x) == this_obj)
        op (&((*x).u.decomp.min.template_info), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).u.decomp.min.access), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).u.decomp.base), cookie);
      break;
    }
}

void
gt_pch_p_14lang_tree_node (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  union lang_tree_node * x ATTRIBUTE_UNUSED = (union lang_tree_node *)x_p;
  switch ((int) (cp_tree_node_structure (&((*x)))))
    {
    case TS_CP_GENERIC:
      switch ((int) (tree_node_structure (&((*x).generic))))
        {
        case TS_BASE:
          break;
        case TS_TYPED:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.typed.type), cookie);
          break;
        case TS_COMMON:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.common.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.common.chain), cookie);
          break;
        case TS_INT_CST:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.int_cst.typed.type), cookie);
          break;
        case TS_POLY_INT_CST:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.poly_int_cst.typed.type), cookie);
          {
            size_t i0;
            size_t l0 = (size_t)(NUM_POLY_INT_COEFFS);
            for (i0 = 0; i0 != l0; i0++) {
              if ((void *)(x) == this_obj)
                op (&((*x).generic.poly_int_cst.coeffs[i0]), cookie);
            }
          }
          break;
        case TS_REAL_CST:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.real_cst.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.real_cst.real_cst_ptr), cookie);
          break;
        case TS_FIXED_CST:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.fixed_cst.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.fixed_cst.fixed_cst_ptr), cookie);
          break;
        case TS_VECTOR:
          {
            size_t l1 = (size_t)(vector_cst_encoded_nelts ((tree) &((*x).generic.vector)));
            if ((void *)(x) == this_obj)
              op (&((*x).generic.vector.typed.type), cookie);
            {
              size_t i1;
              for (i1 = 0; i1 != l1; i1++) {
                if ((void *)(x) == this_obj)
                  op (&((*x).generic.vector.elts[i1]), cookie);
              }
            }
          }
          break;
        case TS_STRING:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.string.typed.type), cookie);
          break;
        case TS_COMPLEX:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.complex.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.complex.real), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.complex.imag), cookie);
          break;
        case TS_IDENTIFIER:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.identifier.common.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.identifier.common.chain), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.identifier.id.str), cookie);
          break;
        case TS_DECL_MINIMAL:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_minimal.common.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_minimal.common.chain), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_minimal.name), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_minimal.context), cookie);
          break;
        case TS_DECL_COMMON:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_common.common.common.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_common.common.common.chain), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_common.common.name), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_common.common.context), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_common.size), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_common.size_unit), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_common.initial), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_common.attributes), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_common.abstract_origin), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_common.lang_specific), cookie);
          break;
        case TS_DECL_WRTL:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_rtl.common.common.common.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_rtl.common.common.common.chain), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_rtl.common.common.name), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_rtl.common.common.context), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_rtl.common.size), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_rtl.common.size_unit), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_rtl.common.initial), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_rtl.common.attributes), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_rtl.common.abstract_origin), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_rtl.common.lang_specific), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_rtl.rtl), cookie);
          break;
        case TS_DECL_NON_COMMON:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_non_common.common.common.common.common.common.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_non_common.common.common.common.common.common.chain), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_non_common.common.common.common.common.name), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_non_common.common.common.common.common.context), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_non_common.common.common.common.size), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_non_common.common.common.common.size_unit), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_non_common.common.common.common.initial), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_non_common.common.common.common.attributes), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_non_common.common.common.common.abstract_origin), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_non_common.common.common.common.lang_specific), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_non_common.common.common.rtl), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_non_common.common.assembler_name), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_non_common.common.symtab_node), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_non_common.result), cookie);
          break;
        case TS_PARM_DECL:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.parm_decl.common.common.common.common.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.parm_decl.common.common.common.common.chain), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.parm_decl.common.common.common.name), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.parm_decl.common.common.common.context), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.parm_decl.common.common.size), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.parm_decl.common.common.size_unit), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.parm_decl.common.common.initial), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.parm_decl.common.common.attributes), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.parm_decl.common.common.abstract_origin), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.parm_decl.common.common.lang_specific), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.parm_decl.common.rtl), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.parm_decl.incoming_rtl), cookie);
          break;
        case TS_DECL_WITH_VIS:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_vis.common.common.common.common.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_vis.common.common.common.common.chain), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_vis.common.common.common.name), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_vis.common.common.common.context), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_vis.common.common.size), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_vis.common.common.size_unit), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_vis.common.common.initial), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_vis.common.common.attributes), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_vis.common.common.abstract_origin), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_vis.common.common.lang_specific), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_vis.common.rtl), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_vis.assembler_name), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.decl_with_vis.symtab_node), cookie);
          break;
        case TS_VAR_DECL:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.var_decl.common.common.common.common.common.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.var_decl.common.common.common.common.common.chain), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.var_decl.common.common.common.common.name), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.var_decl.common.common.common.common.context), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.var_decl.common.common.common.size), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.var_decl.common.common.common.size_unit), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.var_decl.common.common.common.initial), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.var_decl.common.common.common.attributes), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.var_decl.common.common.common.abstract_origin), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.var_decl.common.common.common.lang_specific), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.var_decl.common.common.rtl), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.var_decl.common.assembler_name), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.var_decl.common.symtab_node), cookie);
          break;
        case TS_FIELD_DECL:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.field_decl.common.common.common.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.field_decl.common.common.common.chain), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.field_decl.common.common.name), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.field_decl.common.common.context), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.field_decl.common.size), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.field_decl.common.size_unit), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.field_decl.common.initial), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.field_decl.common.attributes), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.field_decl.common.abstract_origin), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.field_decl.common.lang_specific), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.field_decl.offset), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.field_decl.bit_field_type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.field_decl.qualifier), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.field_decl.bit_offset), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.field_decl.fcontext), cookie);
          break;
        case TS_LABEL_DECL:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.label_decl.common.common.common.common.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.label_decl.common.common.common.common.chain), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.label_decl.common.common.common.name), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.label_decl.common.common.common.context), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.label_decl.common.common.size), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.label_decl.common.common.size_unit), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.label_decl.common.common.initial), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.label_decl.common.common.attributes), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.label_decl.common.common.abstract_origin), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.label_decl.common.common.lang_specific), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.label_decl.common.rtl), cookie);
          break;
        case TS_RESULT_DECL:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.result_decl.common.common.common.common.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.result_decl.common.common.common.common.chain), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.result_decl.common.common.common.name), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.result_decl.common.common.common.context), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.result_decl.common.common.size), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.result_decl.common.common.size_unit), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.result_decl.common.common.initial), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.result_decl.common.common.attributes), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.result_decl.common.common.abstract_origin), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.result_decl.common.common.lang_specific), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.result_decl.common.rtl), cookie);
          break;
        case TS_CONST_DECL:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.const_decl.common.common.common.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.const_decl.common.common.common.chain), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.const_decl.common.common.name), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.const_decl.common.common.context), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.const_decl.common.size), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.const_decl.common.size_unit), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.const_decl.common.initial), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.const_decl.common.attributes), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.const_decl.common.abstract_origin), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.const_decl.common.lang_specific), cookie);
          break;
        case TS_TYPE_DECL:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_decl.common.common.common.common.common.common.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_decl.common.common.common.common.common.common.chain), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_decl.common.common.common.common.common.name), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_decl.common.common.common.common.common.context), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_decl.common.common.common.common.size), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_decl.common.common.common.common.size_unit), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_decl.common.common.common.common.initial), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_decl.common.common.common.common.attributes), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_decl.common.common.common.common.abstract_origin), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_decl.common.common.common.common.lang_specific), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_decl.common.common.common.rtl), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_decl.common.common.assembler_name), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_decl.common.common.symtab_node), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_decl.common.result), cookie);
          break;
        case TS_FUNCTION_DECL:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.function_decl.common.common.common.common.common.common.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.function_decl.common.common.common.common.common.common.chain), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.function_decl.common.common.common.common.common.name), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.function_decl.common.common.common.common.common.context), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.function_decl.common.common.common.common.size), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.function_decl.common.common.common.common.size_unit), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.function_decl.common.common.common.common.initial), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.function_decl.common.common.common.common.attributes), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.function_decl.common.common.common.common.abstract_origin), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.function_decl.common.common.common.common.lang_specific), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.function_decl.common.common.common.rtl), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.function_decl.common.common.assembler_name), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.function_decl.common.common.symtab_node), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.function_decl.common.result), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.function_decl.f), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.function_decl.arguments), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.function_decl.personality), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.function_decl.function_specific_target), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.function_decl.function_specific_optimization), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.function_decl.saved_tree), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.function_decl.vindex), cookie);
          break;
        case TS_TRANSLATION_UNIT_DECL:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.translation_unit_decl.common.common.common.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.translation_unit_decl.common.common.common.chain), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.translation_unit_decl.common.common.name), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.translation_unit_decl.common.common.context), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.translation_unit_decl.common.size), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.translation_unit_decl.common.size_unit), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.translation_unit_decl.common.initial), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.translation_unit_decl.common.attributes), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.translation_unit_decl.common.abstract_origin), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.translation_unit_decl.common.lang_specific), cookie);
          break;
        case TS_TYPE_COMMON:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_common.common.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_common.common.chain), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_common.size), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_common.size_unit), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_common.attributes), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_common.pointer_to), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_common.reference_to), cookie);
          switch ((int) (debug_hooks->tree_type_symtab_field))
            {
            case TYPE_SYMTAB_IS_ADDRESS:
              break;
            case TYPE_SYMTAB_IS_DIE:
              if ((void *)(x) == this_obj)
                op (&((*x).generic.type_common.symtab.die), cookie);
              break;
            default:
              break;
            }
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_common.canonical), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_common.next_variant), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_common.main_variant), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_common.context), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_common.name), cookie);
          break;
        case TS_TYPE_WITH_LANG_SPECIFIC:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_with_lang_specific.common.common.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_with_lang_specific.common.common.chain), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_with_lang_specific.common.size), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_with_lang_specific.common.size_unit), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_with_lang_specific.common.attributes), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_with_lang_specific.common.pointer_to), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_with_lang_specific.common.reference_to), cookie);
          switch ((int) (debug_hooks->tree_type_symtab_field))
            {
            case TYPE_SYMTAB_IS_ADDRESS:
              break;
            case TYPE_SYMTAB_IS_DIE:
              if ((void *)(x) == this_obj)
                op (&((*x).generic.type_with_lang_specific.common.symtab.die), cookie);
              break;
            default:
              break;
            }
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_with_lang_specific.common.canonical), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_with_lang_specific.common.next_variant), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_with_lang_specific.common.main_variant), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_with_lang_specific.common.context), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_with_lang_specific.common.name), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_with_lang_specific.lang_specific), cookie);
          break;
        case TS_TYPE_NON_COMMON:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_non_common.with_lang_specific.common.common.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_non_common.with_lang_specific.common.common.chain), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_non_common.with_lang_specific.common.size), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_non_common.with_lang_specific.common.size_unit), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_non_common.with_lang_specific.common.attributes), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_non_common.with_lang_specific.common.pointer_to), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_non_common.with_lang_specific.common.reference_to), cookie);
          switch ((int) (debug_hooks->tree_type_symtab_field))
            {
            case TYPE_SYMTAB_IS_ADDRESS:
              break;
            case TYPE_SYMTAB_IS_DIE:
              if ((void *)(x) == this_obj)
                op (&((*x).generic.type_non_common.with_lang_specific.common.symtab.die), cookie);
              break;
            default:
              break;
            }
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_non_common.with_lang_specific.common.canonical), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_non_common.with_lang_specific.common.next_variant), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_non_common.with_lang_specific.common.main_variant), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_non_common.with_lang_specific.common.context), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_non_common.with_lang_specific.common.name), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_non_common.with_lang_specific.lang_specific), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_non_common.values), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_non_common.minval), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_non_common.maxval), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.type_non_common.lang_1), cookie);
          break;
        case TS_LIST:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.list.common.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.list.common.chain), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.list.purpose), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.list.value), cookie);
          break;
        case TS_VEC:
          {
            size_t l2 = (size_t)(TREE_VEC_LENGTH ((tree)&((*x).generic.vec)));
            if ((void *)(x) == this_obj)
              op (&((*x).generic.vec.common.typed.type), cookie);
            if ((void *)(x) == this_obj)
              op (&((*x).generic.vec.common.chain), cookie);
            {
              size_t i2;
              for (i2 = 0; i2 != l2; i2++) {
                if ((void *)(x) == this_obj)
                  op (&((*x).generic.vec.a[i2]), cookie);
              }
            }
          }
          break;
        case TS_EXP:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.exp.typed.type), cookie);
          switch ((int) (TREE_CODE ((tree) &(*x))))
            {
            default:
              {
                size_t i3;
                size_t l3 = (size_t)(TREE_OPERAND_LENGTH ((tree) &(*x)));
                for (i3 = 0; i3 != l3; i3++) {
                  if ((void *)(x) == this_obj)
                    op (&((*x).generic.exp.operands[i3]), cookie);
                }
              }
              break;
            }
          break;
        case TS_SSA_NAME:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.ssa_name.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.ssa_name.var), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.ssa_name.def_stmt), cookie);
          switch ((int) (((*x).generic.ssa_name).typed.type ?!POINTER_TYPE_P (TREE_TYPE ((tree)&((*x).generic.ssa_name))) : 2))
            {
            case 0:
              if ((void *)(x) == this_obj)
                op (&((*x).generic.ssa_name.info.ptr_info), cookie);
              break;
            case 1:
              if ((void *)(x) == this_obj)
                op (&((*x).generic.ssa_name.info.range_info), cookie);
              break;
            default:
              break;
            }
          break;
        case TS_BLOCK:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.block.chain), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.block.vars), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.block.nonlocalized_vars), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.block.subblocks), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.block.supercontext), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.block.abstract_origin), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.block.fragment_origin), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.block.fragment_chain), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.block.die), cookie);
          break;
        case TS_BINFO:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.binfo.common.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.binfo.common.chain), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.binfo.offset), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.binfo.vtable), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.binfo.virtuals), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.binfo.vptr_field), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.binfo.base_accesses), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.binfo.inheritance), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.binfo.vtt_subvtt), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.binfo.vtt_vptr), cookie);
          if ((void *)(x) == this_obj)
            gt_pch_nx (&((*x).generic.binfo.base_binfos), op, cookie);
          break;
        case TS_STATEMENT_LIST:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.stmt_list.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.stmt_list.head), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.stmt_list.tail), cookie);
          break;
        case TS_CONSTRUCTOR:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.constructor.typed.type), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.constructor.elts), cookie);
          break;
        case TS_OMP_CLAUSE:
          {
            size_t l4 = (size_t)(omp_clause_num_ops[OMP_CLAUSE_CODE ((tree)&((*x).generic.omp_clause))]);
            if ((void *)(x) == this_obj)
              op (&((*x).generic.omp_clause.common.typed.type), cookie);
            if ((void *)(x) == this_obj)
              op (&((*x).generic.omp_clause.common.chain), cookie);
            if ((void *)(x) == this_obj)
              op (&((*x).generic.omp_clause.gimple_reduction_init), cookie);
            if ((void *)(x) == this_obj)
              op (&((*x).generic.omp_clause.gimple_reduction_merge), cookie);
            {
              size_t i4;
              for (i4 = 0; i4 != l4; i4++) {
                if ((void *)(x) == this_obj)
                  op (&((*x).generic.omp_clause.ops[i4]), cookie);
              }
            }
          }
          break;
        case TS_OPTIMIZATION:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.optimization.opts), cookie);
          if ((*x).generic.optimization.optabs != NULL) {
            if ((void *)(x) == this_obj)
              op (&((*x).generic.optimization.optabs), cookie);
          }
          break;
        case TS_TARGET_OPTION:
          if ((void *)(x) == this_obj)
            op (&((*x).generic.target_option.globals), cookie);
          if ((void *)(x) == this_obj)
            op (&((*x).generic.target_option.opts), cookie);
          break;
        default:
          break;
        }
      break;
    case TS_CP_TPI:
      if ((void *)(x) == this_obj)
        op (&((*x).tpi.common.typed.type), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).tpi.common.chain), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).tpi.decl), cookie);
      break;
    case TS_CP_PTRMEM:
      if ((void *)(x) == this_obj)
        op (&((*x).ptrmem.common.typed.type), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).ptrmem.common.chain), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).ptrmem.member), cookie);
      break;
    case TS_CP_OVERLOAD:
      if ((void *)(x) == this_obj)
        op (&((*x).overload.common.typed.type), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).overload.common.chain), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).overload.function), cookie);
      break;
    case TS_CP_BASELINK:
      if ((void *)(x) == this_obj)
        op (&((*x).baselink.common.typed.type), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).baselink.common.chain), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).baselink.binfo), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).baselink.functions), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).baselink.access_binfo), cookie);
      break;
    case TS_CP_TEMPLATE_DECL:
      if ((void *)(x) == this_obj)
        op (&((*x).template_decl.common.common.common.typed.type), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).template_decl.common.common.common.chain), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).template_decl.common.common.name), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).template_decl.common.common.context), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).template_decl.common.size), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).template_decl.common.size_unit), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).template_decl.common.initial), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).template_decl.common.attributes), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).template_decl.common.abstract_origin), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).template_decl.common.lang_specific), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).template_decl.arguments), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).template_decl.result), cookie);
      break;
    case TS_CP_DEFERRED_PARSE:
      if ((void *)(x) == this_obj)
        op (&((*x).deferred_parse.tokens), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).deferred_parse.instantiations), cookie);
      break;
    case TS_CP_DEFERRED_NOEXCEPT:
      if ((void *)(x) == this_obj)
        op (&((*x).deferred_noexcept.pattern), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).deferred_noexcept.args), cookie);
      break;
    case TS_CP_IDENTIFIER:
      if ((void *)(x) == this_obj)
        op (&((*x).identifier.c_common.common.typed.type), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).identifier.c_common.common.chain), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).identifier.c_common.node.ident.str), cookie);
      switch ((int) (((*x).identifier.c_common.node).type))
        {
        case NT_VOID:
          if ((void *)(x) == this_obj)
            op (&((*x).identifier.c_common.node.value.answers), cookie);
          break;
        case NT_USER_MACRO:
          if ((void *)(x) == this_obj)
            op (&((*x).identifier.c_common.node.value.macro), cookie);
          break;
        case NT_BUILTIN_MACRO:
          break;
        case NT_MACRO_ARG:
          break;
        default:
          break;
        }
      if ((void *)(x) == this_obj)
        op (&((*x).identifier.bindings), cookie);
      break;
    case TS_CP_STATIC_ASSERT:
      if ((void *)(x) == this_obj)
        op (&((*x).static_assertion.common.typed.type), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).static_assertion.common.chain), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).static_assertion.condition), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).static_assertion.message), cookie);
      break;
    case TS_CP_ARGUMENT_PACK_SELECT:
      if ((void *)(x) == this_obj)
        op (&((*x).argument_pack_select.common.typed.type), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).argument_pack_select.common.chain), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).argument_pack_select.argument_pack), cookie);
      break;
    case TS_CP_TRAIT_EXPR:
      if ((void *)(x) == this_obj)
        op (&((*x).trait_expression.common.typed.type), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).trait_expression.common.chain), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).trait_expression.type1), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).trait_expression.type2), cookie);
      break;
    case TS_CP_LAMBDA_EXPR:
      if ((void *)(x) == this_obj)
        op (&((*x).lambda_expression.typed.type), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).lambda_expression.capture_list), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).lambda_expression.this_capture), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).lambda_expression.extra_scope), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).lambda_expression.pending_proxies), cookie);
      break;
    case TS_CP_TEMPLATE_INFO:
      if ((void *)(x) == this_obj)
        op (&((*x).template_info.tmpl), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).template_info.args), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).template_info.typedefs_needing_access_checking), cookie);
      break;
    case TS_CP_CONSTRAINT_INFO:
      if ((void *)(x) == this_obj)
        op (&((*x).constraint_info.template_reqs), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).constraint_info.declarator_reqs), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).constraint_info.associated_constr), cookie);
      break;
    case TS_CP_USERDEF_LITERAL:
      if ((void *)(x) == this_obj)
        op (&((*x).userdef_literal.suffix_id), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).userdef_literal.value), cookie);
      if ((void *)(x) == this_obj)
        op (&((*x).userdef_literal.num_string), cookie);
      break;
    default:
      break;
    }
}

void
gt_pch_p_17language_function (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct language_function * x ATTRIBUTE_UNUSED = (struct language_function *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).base.x_stmt_tree.x_cur_stmt_list), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).base.local_typedefs), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).x_cdtor_label), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).x_current_class_ptr), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).x_current_class_ref), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).x_eh_spec_block), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).x_in_charge_parm), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).x_vtt_parm), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).x_return_value), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).x_named_labels), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).bindings), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).infinite_loops), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).extern_decl_map), cookie);
}

void
gt_pch_p_36vec_qualified_typedef_usage_t_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<qualified_typedef_usage_t,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<qualified_typedef_usage_t,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct qualified_typedef_usage_s* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).typedef_decl), cookie);
    op (&((*x).context), cookie);
}

void
gt_pch_p_28vec_cxx_saved_binding_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<cxx_saved_binding,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<cxx_saved_binding,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct cxx_saved_binding* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).identifier), cookie);
    op (&((*x).binding), cookie);
    op (&((*x).real_type_value), cookie);
}

void
gt_pch_p_11saved_scope (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct saved_scope * x ATTRIBUTE_UNUSED = (struct saved_scope *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).old_bindings), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).old_namespace), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).decl_ns_list), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).class_name), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).class_type), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).access_specifier), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).function_decl), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).lang_base), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).lang_name), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).template_parms), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).x_previous_class_level), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).x_saved_tree), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).x_current_class_ptr), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).x_current_class_ref), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).x_stmt_tree.x_cur_stmt_list), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).class_bindings), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).bindings), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).prev), cookie);
}

void
gt_pch_p_16cxx_int_tree_map (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct cxx_int_tree_map * x ATTRIBUTE_UNUSED = (struct cxx_int_tree_map *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).to), cookie);
}

void
gt_pch_nx (struct cxx_int_tree_map* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).to), cookie);
}

void
gt_pch_p_28hash_table_named_label_hash_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<named_label_hash> * x ATTRIBUTE_UNUSED = (struct hash_table<named_label_hash> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct named_label_hash* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_35hash_table_cxx_int_tree_map_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<cxx_int_tree_map_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<cxx_int_tree_map_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct cxx_int_tree_map_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_11tree_pair_s (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct tree_pair_s * x ATTRIBUTE_UNUSED = (struct tree_pair_s *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).purpose), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).value), cookie);
}

void
gt_pch_p_22vec_tree_pair_s_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<tree_pair_s,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<tree_pair_s,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct tree_pair_s* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).purpose), cookie);
    op (&((*x).value), cookie);
}

void
gt_pch_p_27hash_table_named_decl_hash_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<named_decl_hash> * x ATTRIBUTE_UNUSED = (struct hash_table<named_decl_hash> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct named_decl_hash* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_30hash_table_cplus_array_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<cplus_array_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<cplus_array_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct cplus_array_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_23hash_table_list_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<list_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<list_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct list_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_cp_tree_h[] = {
  {
    &deleted_copy_types,
    1,
    sizeof (deleted_copy_types),
    &gt_ggc_mx_hash_set_tree_,
    &gt_pch_nx_hash_set_tree_
  },
  {
    &list_hash_table,
    1,
    sizeof (list_hash_table),
    &gt_ggc_mx_hash_table_list_hasher_,
    &gt_pch_nx_hash_table_list_hasher_
  },
  {
    &cplus_array_htab,
    1,
    sizeof (cplus_array_htab),
    &gt_ggc_mx_hash_table_cplus_array_hasher_,
    &gt_pch_nx_hash_table_cplus_array_hasher_
  },
  {
    &unemitted_tinfo_decls,
    1,
    sizeof (unemitted_tinfo_decls),
    &gt_ggc_mx_vec_tree_va_gc_,
    &gt_pch_nx_vec_tree_va_gc_
  },
  {
    &ovl_op_info[0][0].identifier,
    1 * (2) * (OVL_OP_MAX),
    sizeof (ovl_op_info[0][0]),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &ovl_op_info[0][0].name,
    1 * (2) * (OVL_OP_MAX),
    sizeof (ovl_op_info[0][0]),
    (gt_pointer_walker) &gt_ggc_m_S,
    (gt_pointer_walker) &gt_pch_n_S
  },
  {
    &ovl_op_info[0][0].mangled_name,
    1 * (2) * (OVL_OP_MAX),
    sizeof (ovl_op_info[0][0]),
    (gt_pointer_walker) &gt_ggc_m_S,
    (gt_pointer_walker) &gt_pch_n_S
  },
  {
    &tls_aggregates,
    1,
    sizeof (tls_aggregates),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &static_aggregates,
    1,
    sizeof (static_aggregates),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &keyed_classes,
    1,
    sizeof (keyed_classes),
    &gt_ggc_mx_vec_tree_va_gc_,
    &gt_pch_nx_vec_tree_va_gc_
  },
  {
    &static_decls,
    1,
    sizeof (static_decls),
    &gt_ggc_mx_vec_tree_va_gc_,
    &gt_pch_nx_vec_tree_va_gc_
  },
  {
    &integer_two_node,
    1,
    sizeof (integer_two_node),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &scope_chain,
    1,
    sizeof (scope_chain),
    &gt_ggc_mx_saved_scope,
    &gt_pch_nx_saved_scope
  },
  {
    &cp_global_trees[0],
    1 * (CPTI_MAX),
    sizeof (cp_global_trees[0]),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  LAST_GGC_ROOT_TAB
};

EXPORTED_CONST struct ggc_root_tab gt_pch_rs_gt_cp_tree_h[] = {
  { &ovl_op_alternate, 1, sizeof (ovl_op_alternate), NULL, NULL },
  { &ovl_op_mapping, 1, sizeof (ovl_op_mapping), NULL, NULL },
  { &ovl_op_info, 1, sizeof (ovl_op_info), NULL, NULL },
  LAST_GGC_ROOT_TAB
};

