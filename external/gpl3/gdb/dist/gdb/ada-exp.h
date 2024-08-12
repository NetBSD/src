/* Definitions for Ada expressions

   Copyright (C) 2020-2024 Free Software Foundation, Inc.

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

#ifndef ADA_EXP_H
#define ADA_EXP_H

#include "expop.h"

extern struct value *ada_unop_neg (struct type *expect_type,
				   struct expression *exp,
				   enum noside noside, enum exp_opcode op,
				   struct value *arg1);
extern struct value *ada_atr_tag (struct type *expect_type,
				  struct expression *exp,
				  enum noside noside, enum exp_opcode op,
				  struct value *arg1);
extern struct value *ada_atr_size (struct type *expect_type,
				   struct expression *exp,
				   enum noside noside, enum exp_opcode op,
				   struct value *arg1);
extern struct value *ada_abs (struct type *expect_type,
			      struct expression *exp,
			      enum noside noside, enum exp_opcode op,
			      struct value *arg1);
extern struct value *ada_unop_in_range (struct type *expect_type,
					struct expression *exp,
					enum noside noside, enum exp_opcode op,
					struct value *arg1, struct type *type);
extern struct value *ada_mult_binop (struct type *expect_type,
				     struct expression *exp,
				     enum noside noside, enum exp_opcode op,
				     struct value *arg1, struct value *arg2);
extern struct value *ada_equal_binop (struct type *expect_type,
				      struct expression *exp,
				      enum noside noside, enum exp_opcode op,
				      struct value *arg1, struct value *arg2);
extern struct value *ada_ternop_slice (struct expression *exp,
				       enum noside noside,
				       struct value *array,
				       struct value *low_bound_val,
				       struct value *high_bound_val);
extern struct value *ada_binop_in_bounds (struct expression *exp,
					  enum noside noside,
					  struct value *arg1,
					  struct value *arg2,
					  int n);
extern struct value *ada_binop_minmax (struct type *expect_type,
				       struct expression *exp,
				       enum noside noside, enum exp_opcode op,
				       struct value *arg1,
				       struct value *arg2);
extern struct value *ada_pos_atr (struct type *expect_type,
				  struct expression *exp,
				  enum noside noside, enum exp_opcode op,
				  struct value *arg);
extern struct value *ada_atr_enum_rep (struct expression *exp,
				       enum noside noside, struct type *type,
				       struct value *arg);
extern struct value *ada_atr_enum_val (struct expression *exp,
				       enum noside noside, struct type *type,
				       struct value *arg);
extern struct value *ada_val_atr (struct expression *exp,
				  enum noside noside, struct type *type,
				  struct value *arg);
extern struct value *ada_binop_exp (struct type *expect_type,
				    struct expression *exp,
				    enum noside noside, enum exp_opcode op,
				    struct value *arg1, struct value *arg2);

namespace expr
{

/* The base class for Ada type resolution.  Ada operations that want
   to participate in resolution implement this interface.  */
struct ada_resolvable
{
  /* Resolve this object.  EXP is the expression being resolved.
     DEPROCEDURE_P is true if a symbol that refers to a zero-argument
     function may be turned into a function call.  PARSE_COMPLETION
     and TRACKER are passed in from the parser context.  CONTEXT_TYPE
     is the expected type of the expression, or nullptr if none is
     known.  This method should return true if the operation should be
     replaced by a function call with this object as the callee.  */
  virtual bool resolve (struct expression *exp,
			bool deprocedure_p,
			bool parse_completion,
			innermost_block_tracker *tracker,
			struct type *context_type) = 0;

  /* Possibly replace this object with some other expression object.
     This is like 'resolve', but can return a replacement.

     The default implementation calls 'resolve' and wraps this object
     in a function call if that call returns true.  OWNER is a
     reference to the unique pointer that owns the 'this'; it can be
     'move'd from to construct the replacement.

     This should either return a new object, or OWNER -- never
     nullptr.  */

  virtual operation_up replace (operation_up &&owner,
				struct expression *exp,
				bool deprocedure_p,
				bool parse_completion,
				innermost_block_tracker *tracker,
				struct type *context_type);
};

/* In Ada, some generic operations must be wrapped with a handler that
   handles some Ada-specific type conversions.  */
class ada_wrapped_operation
  : public tuple_holding_operation<operation_up>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return std::get<0> (m_storage)->opcode (); }

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override;
};

/* An Ada string constant.  */
class ada_string_operation
  : public string_operation
{
public:

  using string_operation::string_operation;

  /* Return the underlying string.  */
  const char *get_name () const
  {
    return std::get<0> (m_storage).c_str ();
  }

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;
};

/* The Ada TYPE'(EXP) construct.  */
class ada_qual_operation
  : public tuple_holding_operation<operation_up, struct type *>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return UNOP_QUAL; }
};

/* Ternary in-range operator.  */
class ada_ternop_range_operation
  : public tuple_holding_operation<operation_up, operation_up, operation_up>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return TERNOP_IN_RANGE; }
};

using ada_neg_operation = unop_operation<UNOP_NEG, ada_unop_neg>;
using ada_atr_tag_operation = unop_operation<OP_ATR_TAG, ada_atr_tag>;
using ada_atr_size_operation = unop_operation<OP_ATR_SIZE, ada_atr_size>;
using ada_abs_operation = unop_operation<UNOP_ABS, ada_abs>;
using ada_pos_operation = unop_operation<OP_ATR_POS, ada_pos_atr>;

/* The in-range operation, given a type.  */
class ada_unop_range_operation
  : public tuple_holding_operation<operation_up, struct type *>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *val = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
    return ada_unop_in_range (expect_type, exp, noside, UNOP_IN_RANGE,
			      val, std::get<1> (m_storage));
  }

  enum exp_opcode opcode () const override
  { return UNOP_IN_RANGE; }
};

/* The Ada + and - operators.  */
class ada_binop_addsub_operation
  : public tuple_holding_operation<enum exp_opcode, operation_up, operation_up>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return std::get<0> (m_storage); }
};

using ada_binop_mul_operation = binop_operation<BINOP_MUL, ada_mult_binop>;
using ada_binop_div_operation = binop_operation<BINOP_DIV, ada_mult_binop>;
using ada_binop_rem_operation = binop_operation<BINOP_REM, ada_mult_binop>;
using ada_binop_mod_operation = binop_operation<BINOP_MOD, ada_mult_binop>;

using ada_binop_min_operation = binop_operation<BINOP_MIN, ada_binop_minmax>;
using ada_binop_max_operation = binop_operation<BINOP_MAX, ada_binop_minmax>;

using ada_binop_exp_operation = binop_operation<BINOP_EXP, ada_binop_exp>;

/* Implement the equal and not-equal operations for Ada.  */
class ada_binop_equal_operation
  : public tuple_holding_operation<enum exp_opcode, operation_up, operation_up>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *arg1 = std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
    value *arg2 = std::get<2> (m_storage)->evaluate (arg1->type (),
						     exp, noside);
    return ada_equal_binop (expect_type, exp, noside, std::get<0> (m_storage),
			    arg1, arg2);
  }

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override
  {
    gen_expr_binop (exp, opcode (),
		    std::get<1> (this->m_storage).get (),
		    std::get<2> (this->m_storage).get (),
		    ax, value);
  }

  enum exp_opcode opcode () const override
  { return std::get<0> (m_storage); }
};

/* Ada array- or string-slice operation.  */
class ada_ternop_slice_operation
  : public maybe_constant_operation<operation_up, operation_up, operation_up>,
    public ada_resolvable
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *array = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
    value *low = std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
    value *high = std::get<2> (m_storage)->evaluate (nullptr, exp, noside);
    return ada_ternop_slice (exp, noside, array, low, high);
  }

  enum exp_opcode opcode () const override
  { return TERNOP_SLICE; }

  bool resolve (struct expression *exp,
		bool deprocedure_p,
		bool parse_completion,
		innermost_block_tracker *tracker,
		struct type *context_type) override;
};

/* Implement BINOP_IN_BOUNDS for Ada.  */
class ada_binop_in_bounds_operation
  : public maybe_constant_operation<operation_up, operation_up, int>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *arg1 = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
    value *arg2 = std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
    return ada_binop_in_bounds (exp, noside, arg1, arg2,
				std::get<2> (m_storage));
  }

  enum exp_opcode opcode () const override
  { return BINOP_IN_BOUNDS; }
};

/* Implement several unary Ada OP_ATR_* operations.  */
class ada_unop_atr_operation
  : public maybe_constant_operation<operation_up, enum exp_opcode, int>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return std::get<1> (m_storage); }
};

/* Variant of var_value_operation for Ada.  */
class ada_var_value_operation
  : public var_value_operation, public ada_resolvable
{
public:

  using var_value_operation::var_value_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  value *evaluate_for_cast (struct type *expect_type,
			    struct expression *exp,
			    enum noside noside) override;

  const block *get_block () const
  { return std::get<0> (m_storage).block; }

  bool resolve (struct expression *exp,
		bool deprocedure_p,
		bool parse_completion,
		innermost_block_tracker *tracker,
		struct type *context_type) override;

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override;
};

/* Variant of var_msym_value_operation for Ada.  */
class ada_var_msym_value_operation
  : public var_msym_value_operation
{
public:

  using var_msym_value_operation::var_msym_value_operation;

  value *evaluate_for_cast (struct type *expect_type,
			    struct expression *exp,
			    enum noside noside) override;

protected:

  using operation::do_generate_ax;
};

typedef struct value *ada_atr_ftype (struct expression *exp,
				     enum noside noside,
				     struct type *type,
				     struct value *arg);

/* Implement several Ada attributes.  */
template<ada_atr_ftype FUNC>
class ada_atr_operation
  : public tuple_holding_operation<struct type *, operation_up>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *arg = std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
    return FUNC (exp, noside, std::get<0> (m_storage), arg);
  }

  enum exp_opcode opcode () const override
  {
    /* The value here generally doesn't matter.  */
    return OP_ATR_VAL;
  }
};

using ada_atr_val_operation = ada_atr_operation<ada_val_atr>;
using ada_atr_enum_rep_operation = ada_atr_operation<ada_atr_enum_rep>;
using ada_atr_enum_val_operation = ada_atr_operation<ada_atr_enum_val>;

/* The indirection operator for Ada.  */
class ada_unop_ind_operation
  : public unop_ind_base_operation
{
public:

  using unop_ind_base_operation::unop_ind_base_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;
};

/* Implement STRUCTOP_STRUCT for Ada.  */
class ada_structop_operation
  : public structop_base_operation
{
public:

  using structop_base_operation::structop_base_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return STRUCTOP_STRUCT; }

  /* Set the completion prefix.  */
  void set_prefix (std::string &&prefix)
  {
    m_prefix = std::move (prefix);
  }

  bool complete (struct expression *exp, completion_tracker &tracker) override
  {
    return structop_base_operation::complete (exp, tracker, m_prefix.c_str ());
  }

  void dump (struct ui_file *stream, int depth) const override
  {
    structop_base_operation::dump (stream, depth);
    dump_for_expression (stream, depth + 1, m_prefix);
  }

private:

  /* We may need to provide a prefix to field name completion.  See
     ada-exp.y:find_completion_bounds for details.  */
  std::string m_prefix;
};

/* Function calls for Ada.  */
class ada_funcall_operation
  : public tuple_holding_operation<operation_up, std::vector<operation_up>>,
    public ada_resolvable
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  bool resolve (struct expression *exp,
		bool deprocedure_p,
		bool parse_completion,
		innermost_block_tracker *tracker,
		struct type *context_type) override;

  enum exp_opcode opcode () const override
  { return OP_FUNCALL; }
};

/* An Ada assignment operation.  */
class ada_assign_operation
  : public assign_operation
{
public:

  using assign_operation::assign_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return BINOP_ASSIGN; }

  value *current ()
  { return m_current; }

  /* A helper function for the parser to evaluate just the LHS of the
     assignment.  */
  value *eval_for_resolution (struct expression *exp)
  {
    return std::get<0> (m_storage)->evaluate (nullptr, exp,
					      EVAL_AVOID_SIDE_EFFECTS);
  }

  /* The parser must construct the assignment node before parsing the
     RHS, so that '@' can access the assignment, so this helper
     function is needed to set the RHS after construction.  */
  void set_rhs (operation_up rhs)
  {
    std::get<1> (m_storage) = std::move (rhs);
  }

private:

  /* Temporary storage for the value of the left-hand-side.  */
  value *m_current = nullptr;
};

/* Implement the Ada target name symbol ('@').  This is used to refer
   to the LHS of an assignment from the RHS.  */
class ada_target_operation : public operation
{
public:

  explicit ada_target_operation (ada_assign_operation *lhs)
    : m_lhs (lhs)
  { }

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    if (noside == EVAL_AVOID_SIDE_EFFECTS)
      return m_lhs->eval_for_resolution (exp);
    return m_lhs->current ();
  }

  enum exp_opcode opcode () const override
  {
    /* It doesn't really matter.  */
    return OP_VAR_VALUE;
  }

  void dump (struct ui_file *stream, int depth) const override
  {
    gdb_printf (stream, _("%*sAda target symbol '@'\n"), depth, "");
  }

private:

  /* The left hand side of the assignment.  */
  ada_assign_operation *m_lhs;
};

/* When constructing an aggregate, an object of this type is created
   to track the needed state.  */

struct aggregate_assigner
{
  /* An lvalue containing LHS (possibly LHS itself).  */
  value *container;

  /* An lvalue of record or array type; this is the object being
     assigned to.  */
  value *lhs;

  /* The expression being evaluated.  */
  expression *exp;

  /* The bounds of LHS.  This is used by the 'others' component.  */
  LONGEST low;
  LONGEST high;

  /* This indicates which sub-components have already been assigned
     to.  */
  std::vector<LONGEST> indices;

private:

  /* The current index value.  This is only valid during the 'assign'
     operation and is part of the implementation of iterated component
     association.  */
  LONGEST m_current_index = 0;

public:

  /* Assign the result of evaluating ARG to the INDEXth component of
     LHS (a simple array or a record).  Does not modify the inferior's
     memory, nor does it modify LHS (unless LHS == CONTAINER).  */
  void assign (LONGEST index, operation_up &arg);

  /* Add the interval [FROM .. TO] to the sorted set of intervals
     [ INDICES[0] .. INDICES[1] ],...  The resulting intervals do not
     overlap.  */
  void add_interval (LONGEST low, LONGEST high);

  /* Return the current index as a value, using the index type of
     LHS.  */
  value *current_value () const;
};

/* This abstract class represents a single component in an Ada
   aggregate assignment.  */
class ada_component
{
public:

  /* Assign to ASSIGNER.  */
  virtual void assign (aggregate_assigner &assigner) = 0;

  /* Same as operation::uses_objfile.  */
  virtual bool uses_objfile (struct objfile *objfile) = 0;

  /* Same as operation::dump.  */
  virtual void dump (ui_file *stream, int depth) = 0;

  virtual ~ada_component () = default;

protected:

  ada_component () = default;
  DISABLE_COPY_AND_ASSIGN (ada_component);
};

/* Unique pointer specialization for Ada assignment components.  */
typedef std::unique_ptr<ada_component> ada_component_up;

/* An operation that holds a single component.  */
class ada_aggregate_operation
  : public tuple_holding_operation<ada_component_up>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  /* Assuming that LHS represents an lvalue having a record or array
     type, evaluate an assignment of this aggregate's value to LHS.
     CONTAINER is an lvalue containing LHS (possibly LHS itself).
     Does not modify the inferior's memory, nor does it modify the
     contents of LHS (unless == CONTAINER).  Returns the modified
     CONTAINER.  */

  value *assign_aggregate (struct value *container,
			   struct value *lhs,
			   struct expression *exp);

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    error (_("Aggregates only allowed on the right of an assignment"));
  }

  enum exp_opcode opcode () const override
  { return OP_AGGREGATE; }
};

/* A component holding a vector of other components to assign.  */
class ada_aggregate_component : public ada_component
{
public:

  explicit ada_aggregate_component (std::vector<ada_component_up> &&components)
    : m_components (std::move (components))
  {
  }

  /* This is the "with delta" form -- BASE is the base expression.  */
  ada_aggregate_component (operation_up &&base,
			   std::vector<ada_component_up> &&components);

  void assign (aggregate_assigner &assigner) override;

  bool uses_objfile (struct objfile *objfile) override;

  void dump (ui_file *stream, int depth) override;

private:

  /* If the assignment has a "with delta" clause, this is the
     base expression.  */
  operation_up m_base;
  /* The individual components to assign.  */
  std::vector<ada_component_up> m_components;
};

/* A component that assigns according to a provided index (which is
   relative to the "low" value).  */
class ada_positional_component : public ada_component
{
public:

  ada_positional_component (int index, operation_up &&op)
    : m_index (index),
      m_op (std::move (op))
  {
  }

  void assign (aggregate_assigner &assigner) override;

  bool uses_objfile (struct objfile *objfile) override;

  void dump (ui_file *stream, int depth) override;

private:

  int m_index;
  operation_up m_op;
};

/* A component which handles an "others" clause.  */
class ada_others_component : public ada_component
{
public:

  explicit ada_others_component (operation_up &&op)
    : m_op (std::move (op))
  {
  }

  void assign (aggregate_assigner &assigner) override;

  bool uses_objfile (struct objfile *objfile) override;

  void dump (ui_file *stream, int depth) override;

private:

  operation_up m_op;
};

/* An interface that represents an association that is used in
   aggregate assignment.  */
class ada_association
{
public:

  /* Like ada_component::assign, but takes an operation as a
     parameter.  The operation is evaluated and then assigned into
     ASSIGNER according to the rules of the concrete
     implementation.  */
  virtual void assign (aggregate_assigner &assigner, operation_up &op) = 0;

  /* Same as operation::uses_objfile.  */
  virtual bool uses_objfile (struct objfile *objfile) = 0;

  /* Same as operation::dump.  */
  virtual void dump (ui_file *stream, int depth) = 0;

  virtual ~ada_association () = default;

protected:

  ada_association () = default;
  DISABLE_COPY_AND_ASSIGN (ada_association);
};

/* Unique pointer specialization for Ada assignment associations.  */
typedef std::unique_ptr<ada_association> ada_association_up;

/* A component that holds a vector of associations and an operation.
   The operation is re-evaluated for each choice.  */
class ada_choices_component : public ada_component
{
public:

  explicit ada_choices_component (operation_up &&op)
    : m_op (std::move (op))
  {
  }

  /* Set the vector of associations.  This is done separately from the
     constructor because it was simpler for the implementation of the
     parser.  */
  void set_associations (std::vector<ada_association_up> &&assoc)
  {
    m_assocs = std::move (assoc);
  }

  /* Set the underlying operation  */
  void set_operation (operation_up op)
  { m_op = std::move (op); }

  /* Set the index variable name for an iterated association.  */
  void set_name (std::string &&name)
  { m_name = std::move (name); }

  /* The name of this choice component.  This is empty unless this is
     an iterated association.  */
  const std::string &name () const
  { return m_name; }

  void assign (aggregate_assigner &assigner) override;

  bool uses_objfile (struct objfile *objfile) override;

  void dump (ui_file *stream, int depth) override;

  /* Return the current value of the index variable.  This may only be
     called underneath a call to 'assign'.  */
  value *current_value () const
  { return m_assigner->current_value (); }

private:

  std::vector<ada_association_up> m_assocs;
  operation_up m_op;

  /* Name of the variable used for iteration.  This isn't needed for
     evaluation, only for debug dumping.  This is the empty string for
     ordinary (non-iterated) choices.  */
  std::string m_name;

  /* A pointer to the current assignment operation; only valid when in
     a call to the 'assign' method.  This is used to find the index
     variable value during the evaluation of the RHS of the =>, via
     ada_index_var_operation.  */
  const aggregate_assigner *m_assigner = nullptr;
};

/* Implement the index variable for iterated component
   association.  */
class ada_index_var_operation : public operation
{
public:

  ada_index_var_operation ()
  { }

  /* Link this variable to the choices object.  May only be called
     once.  */
  void set_choices (ada_choices_component *var)
  {
    gdb_assert (m_var == nullptr && var != nullptr);
    m_var = var;
  }

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  {
    /* It doesn't really matter.  */
    return OP_VAR_VALUE;
  }

  void dump (struct ui_file *stream, int depth) const override;

private:

  /* The choices component that introduced the index variable.  */
  ada_choices_component *m_var = nullptr;
};

/* An association that uses a discrete range.  */
class ada_discrete_range_association : public ada_association
{
public:

  ada_discrete_range_association (operation_up &&low, operation_up &&high)
    : m_low (std::move (low)),
      m_high (std::move (high))
  {
  }

  void assign (aggregate_assigner &assigner, operation_up &op) override;

  bool uses_objfile (struct objfile *objfile) override;

  void dump (ui_file *stream, int depth) override;

private:

  operation_up m_low;
  operation_up m_high;
};

/* An association that uses a name.  The name may be an expression
   that evaluates to an integer (for arrays), or an Ada string or
   variable value operation.  */
class ada_name_association : public ada_association
{
public:

  explicit ada_name_association (operation_up val)
    : m_val (std::move (val))
  {
  }

  void assign (aggregate_assigner &assigner, operation_up &op) override;

  bool uses_objfile (struct objfile *objfile) override;

  void dump (ui_file *stream, int depth) override;

private:

  operation_up m_val;
};

/* A character constant expression.  This is a separate operation so
   that it can participate in resolution, so that TYPE'(CST) can
   work correctly for enums with character enumerators.  */
class ada_char_operation : public long_const_operation,
			   public ada_resolvable
{
public:

  using long_const_operation::long_const_operation;

  bool resolve (struct expression *exp,
		bool deprocedure_p,
		bool parse_completion,
		innermost_block_tracker *tracker,
		struct type *context_type) override
  {
    /* This should never be called, because this class also implements
       'replace'.  */
    gdb_assert_not_reached ("unexpected call");
  }

  operation_up replace (operation_up &&owner,
			struct expression *exp,
			bool deprocedure_p,
			bool parse_completion,
			innermost_block_tracker *tracker,
			struct type *context_type) override;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;
};

class ada_concat_operation : public concat_operation
{
public:

  using concat_operation::concat_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;
};

} /* namespace expr */

#endif /* ADA_EXP_H */
