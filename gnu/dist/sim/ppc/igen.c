/*  This file is part of the program psim.

    Copyright (C) 1994-1995, Andrew Cagney <cagney@highland.com.au>

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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 
    */

#include <stdio.h>
#include <ctype.h>
#include <getopt.h>

#include "misc.h"
#include "lf.h"
#include "table.h"
#include "config.h"

#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif



/****************************************************************/

enum {
  max_insn_size = 32,
};

static int hi_bit_nr = 0;
static int insn_size = max_insn_size;
static int idecode_expand_semantics = 0;
static int idecode_cache = 0;
static int semantics_use_cache_struct = 0;
static int number_lines = 1;


/****************************************************************/


static char *cache_idecode_formal  =
"cpu *processor,\n\
 instruction_word instruction,\n\
 unsigned_word cia,\n\
 idecode_cache *cache_entry";

static char *cache_idecode_actual = "processor, instruction, cia, cache_entry";

static char *cache_semantic_formal =
"cpu *processor,\n\
 idecode_cache *cache_entry,\n\
 unsigned_word cia";

static char *semantic_formal = 
"cpu *processor,\n\
 instruction_word instruction,\n\
 unsigned_word cia";

static char *semantic_actual = "processor, instruction, cia";



/****************************************************************/


typedef struct _filter filter;
struct _filter {
  char *flag;
  filter *next;
};
static filter *filters = NULL;


/****************************************************************/


typedef struct _cache_rules cache_rules;
struct _cache_rules {
  int valid;
  char *old_name;
  char *new_name;
  char *type;
  char *expression;
  cache_rules *next;
};
static cache_rules *cache_table;


enum {
  ca_valid,
  ca_old_name,
  ca_new_name,
  ca_type,
  ca_expression,
  nr_cache_rule_fields,
};

static cache_rules *
load_cache_rules(char *file_name)
{
  table *file = table_open(file_name, nr_cache_rule_fields, 0);
  table_entry *entry;
  cache_rules *table = NULL;
  cache_rules **curr_rule = &table;
  while ((entry = table_entry_read(file)) != NULL) {
    cache_rules *new_rule = ZALLOC(cache_rules);
    new_rule->valid = target_a2i(hi_bit_nr, entry->fields[ca_valid]);
    new_rule->old_name = entry->fields[ca_old_name];
    new_rule->new_name = entry->fields[ca_new_name];
    new_rule->type = (strlen(entry->fields[ca_type])
		      ? entry->fields[ca_type]
		      : NULL);
    new_rule->expression = (strlen(entry->fields[ca_expression]) > 0
			    ? entry->fields[ca_expression]
			    : NULL);
    *curr_rule = new_rule;
    curr_rule = &new_rule->next;
  }
  return table;
}



static void
dump_cache_rule(cache_rules* rule,
		int indent)
{
  dumpf(indent, "((cache_rules*)0x%x\n", rule);
  dumpf(indent, " (valid %d)\n", rule->valid);
  dumpf(indent, " (old_name \"%s\")\n", rule->old_name);
  dumpf(indent, " (new_name \"%s\")\n", rule->new_name);
  dumpf(indent, " (type \"%s\")\n", rule->type);
  dumpf(indent, " (expression \"%s\")\n", rule->expression);
  dumpf(indent, " (next 0x%x)\n", rule->next);
  dumpf(indent, " )\n");
}


static void
dump_cache_rules(cache_rules* rule, int indent)
{
  while (rule) {
    dump_cache_rule(rule, indent);
    rule = rule->next;
  }
}


/****************************************************************/


typedef struct _opcode_rules opcode_rules;
struct _opcode_rules {
  int first;
  int last;
  int force_first;
  int force_last;
  int force_slash;
  char *force_expansion;
  int use_switch;
  unsigned special_mask;
  unsigned special_value;
  unsigned special_rule;
  opcode_rules *next;
};
static opcode_rules *opcode_table;


enum {
  op_first,
  op_last,
  op_force_first,
  op_force_last,
  op_force_slash,
  op_force_expansion,
  op_use_switch,
  op_special_mask,
  op_special_value,
  op_special_rule,
  nr_opcode_fields,
};


static opcode_rules *
load_opcode_rules(char *file_name)
{
  table *file = table_open(file_name, nr_opcode_fields, 0);
  table_entry *entry;
  opcode_rules *table = NULL;
  opcode_rules **curr_rule = &table;
  while ((entry = table_entry_read(file)) != NULL) {
    opcode_rules *new_rule = ZALLOC(opcode_rules);
    new_rule->first = target_a2i(hi_bit_nr, entry->fields[op_first]);
    new_rule->last = target_a2i(hi_bit_nr, entry->fields[op_last]);
    new_rule->force_first = target_a2i(hi_bit_nr, entry->fields[op_force_first]);
    new_rule->force_last = target_a2i(hi_bit_nr, entry->fields[op_force_last]);
    new_rule->force_slash = a2i(entry->fields[op_force_slash]);
    new_rule->force_expansion = entry->fields[op_force_expansion];
    new_rule->use_switch = a2i(entry->fields[op_use_switch]);
    new_rule->special_mask = a2i(entry->fields[op_special_mask]);
    new_rule->special_value = a2i(entry->fields[op_special_value]);
    new_rule->special_rule = a2i(entry->fields[op_special_rule]);
    *curr_rule = new_rule;
    curr_rule = &new_rule->next;
  }
  return table;
}

  
static void
dump_opcode_rule(opcode_rules *rule,
		 int indent)
{
  dumpf(indent, "((opcode_rules*)%p\n", rule);
  if (rule) {
    dumpf(indent, " (first %d)\n", rule->first);
    dumpf(indent, " (last %d)\n", rule->last);
    dumpf(indent, " (force_first %d)\n", rule->force_first);
    dumpf(indent, " (force_last %d)\n", rule->force_last);
    dumpf(indent, " (force_slash %d)\n", rule->force_slash);
    dumpf(indent, " (force_expansion \"%s\")\n", rule->force_expansion);
    dumpf(indent, " (use_switch %d)\n", rule->use_switch);
    dumpf(indent, " (special_mask 0x%x)\n", rule->special_mask);
    dumpf(indent, " (special_value 0x%x)\n", rule->special_value);
    dumpf(indent, " (special_rule 0x%x)\n", rule->special_rule);
    dumpf(indent, " (next 0x%x)\n", rule->next);
  }
  dumpf(indent, " )\n");
}


static void
dump_opcode_rules(opcode_rules *rule,
		  int indent)
{
  while (rule) {
    dump_opcode_rule(rule, indent);
    rule = rule->next;
  }
}


/****************************************************************/

typedef struct _insn_field insn_field;
struct _insn_field {
  int first;
  int last;
  int width;
  int is_int;
  int is_slash;
  int is_string;
  int val_int;
  char *pos_string;
  char *val_string;
  insn_field *next;
  insn_field *prev;
};

typedef struct _insn_fields insn_fields;
struct _insn_fields {
  insn_field *bits[max_insn_size];
  insn_field *first;
  insn_field *last;
  unsigned value;
};

static insn_fields *
parse_insn_format(table_entry *entry,
		  char *format)
{
  char *chp;
  insn_fields *fields = ZALLOC(insn_fields);

  /* create a leading sentinal */
  fields->first = ZALLOC(insn_field);
  fields->first->first = -1;
  fields->first->last = -1;
  fields->first->width = 0;

  /* and a trailing sentinal */
  fields->last = ZALLOC(insn_field);
  fields->last->first = insn_size;
  fields->last->last = insn_size;
  fields->last->width = 0;

  /* link them together */
  fields->first->next = fields->last;
  fields->last->prev = fields->first;

  /* now work through the formats */
  chp = format;

  while (*chp != '\0') {
    char *start_pos;
    char *start_val;
    int strlen_val;
    int strlen_pos;
    insn_field *new_field;

    /* sanity check */
    if (!isdigit(*chp)) {
      error("%s:%d: missing position field at `%s'\n",
	    entry->file_name, entry->line_nr, chp);
    }

    /* break out the bit position */
    start_pos = chp;
    while (isdigit(*chp))
      chp++;
    strlen_pos = chp - start_pos;
    if (*chp == '.' && strlen_pos > 0)
      chp++;
    else {
      error("%s:%d: missing field value at %s\n",
	    entry->file_name, entry->line_nr, chp);
      break;
    }

    /* break out the value */
    start_val = chp;
    while ((*start_val == '/' && *chp == '/')
	   || (isdigit(*start_val) && isdigit(*chp))
	   || (isalpha(*start_val) && (isalnum(*chp) || *chp == '_')))
      chp++;
    strlen_val = chp - start_val;
    if (*chp == ',')
      chp++;
    else if (*chp != '\0' || strlen_val == 0) {
      error("%s:%d: missing field terminator at %s\n",
	    entry->file_name, entry->line_nr, chp);
      break;
    }

    /* create a new field and insert it */
    new_field = ZALLOC(insn_field);
    new_field->next = fields->last;
    new_field->prev = fields->last->prev;
    new_field->next->prev = new_field;
    new_field->prev->next = new_field;

    /* the value */
    new_field->val_string = (char*)zalloc(strlen_val+1);
    strncpy(new_field->val_string, start_val, strlen_val);
    if (isdigit(*new_field->val_string)) {
      new_field->val_int = a2i(new_field->val_string);
      new_field->is_int = 1;
    }
    else if (new_field->val_string[0] == '/') {
      new_field->is_slash = 1;
    }
    else {
      new_field->is_string = 1;
    }
    
    /* the pos */
    new_field->pos_string = (char*)zalloc(strlen_pos+1);
    strncpy(new_field->pos_string, start_pos, strlen_pos);
    new_field->first = target_a2i(hi_bit_nr, new_field->pos_string);
    new_field->last = new_field->next->first - 1; /* guess */
    new_field->width = new_field->last - new_field->first + 1; /* guess */
    new_field->prev->last = new_field->first-1; /*fix*/
    new_field->prev->width = new_field->first - new_field->prev->first; /*fix*/
  }

  /* fiddle first/last so that the sentinals `disapear' */
  ASSERT(fields->first->last < 0);
  ASSERT(fields->last->first >= insn_size);
  fields->first = fields->first->next;
  fields->last = fields->last->prev;

  /* now go over this again, pointing each bit position at a field
     record */
  {
    int i;
    insn_field *field;
    field = fields->first;
    for (i = 0; i < insn_size; i++) {
      while (field->last < i)
	field = field->next;
      fields->bits[i] = field;
    }
  }

  /* go over each of the fields, and compute a `value' for the insn */
  {
    insn_field *field;
    fields->value = 0;
    for (field = fields->first;
	 field->last < insn_size;
	 field = field->next) {
      fields->value <<= field->width;
      if (field->is_int)
	fields->value |= field->val_int;
    }
  }
  return fields;
}


typedef enum {
  field_constant_int = 1,
  field_constant_slash = 2,
  field_constant_string = 3
} constant_field_types;


static int
insn_field_is_constant(insn_field *field,
		       opcode_rules *rule)
{
  /* field is an integer */
  if (field->is_int)
    return field_constant_int;
  /* field is `/' and treating that as a constant */
  if (field->is_slash && rule->force_slash)
    return field_constant_slash;
  /* field, though variable is on the list */
  if (field->is_string && rule->force_expansion != NULL) {
    char *forced_fields = rule->force_expansion;
    while (*forced_fields != '\0') {
      int field_len;
      char *end = strchr(forced_fields, ',');
      if (end == NULL)
	field_len = strlen(forced_fields);
      else
	field_len = end-forced_fields;
      if (strncmp(forced_fields, field->val_string, field_len) == 0
	  && field->val_string[field_len] == '\0')
	return field_constant_string;
      forced_fields += field_len;
      if (*forced_fields == ',')
	forced_fields++;
    }
  }
  return 0;
}


static void
dump_insn_field(insn_field *field,
		int indent)
{

  printf("(insn_field*)0x%x\n", (unsigned)field);

  dumpf(indent, "(first %d)\n", field->first);

  dumpf(indent, "(last %d)\n", field->last);

  dumpf(indent, "(width %d)\n", field->width);

  if (field->is_int)
    dumpf(indent, "(is_int %d)\n", field->val_int);

  if (field->is_slash)
    dumpf(indent, "(is_slash)\n");

  if (field->is_string)
    dumpf(indent, "(is_string `%s')\n", field->val_string);
  
  dumpf(indent, "(next 0x%x)\n", field->next);
  
  dumpf(indent, "(prev 0x%x)\n", field->prev);
  

}

static void
dump_insn_fields(insn_fields *fields,
		 int indent)
{
  int i;

  printf("(insn_fields*)%p\n", fields);

  dumpf(indent, "(first 0x%x)\n", fields->first);
  dumpf(indent, "(last 0x%x)\n", fields->last);

  dumpf(indent, "(value 0x%x)\n", fields->value);

  for (i = 0; i < insn_size; i++) {
    dumpf(indent, "(bits[%d] ", i, fields->bits[i]);
    dump_insn_field(fields->bits[i], indent+1);
    dumpf(indent, " )\n");
  }

}


/****************************************************************/

typedef struct _opcode_field opcode_field;
struct _opcode_field {
  int first;
  int last;
  int is_boolean;
  opcode_field *parent;
};

static void
dump_opcode_field(opcode_field *field, int indent, int levels)
{
  printf("(opcode_field*)%p\n", field);
  if (levels && field != NULL) {
    dumpf(indent, "(first %d)\n", field->first);
    dumpf(indent, "(last %d)\n", field->last);
    dumpf(indent, "(is_boolean %d)\n", field->is_boolean);
    dumpf(indent, "(parent ");
    dump_opcode_field(field->parent, indent, levels-1);
  }
}


/****************************************************************/

typedef struct _insn_bits insn_bits;
struct _insn_bits {
  int is_expanded;
  int value;
  insn_field *field;
  opcode_field *opcode;
  insn_bits *last;
};


static void
dump_insn_bits(insn_bits *bits, int indent, int levels)
{
  printf("(insn_bits*)%p\n", bits);

  if (levels && bits != NULL) {
    dumpf(indent, "(value %d)\n", bits->value);
    dumpf(indent, "(opcode ");
    dump_opcode_field(bits->opcode, indent+1, 0);
    dumpf(indent, " )\n");
    dumpf(indent, "(field ");
    dump_insn_field(bits->field, indent+1);
    dumpf(indent, " )\n");
    dumpf(indent, "(last ");
    dump_insn_bits(bits->last, indent+1, levels-1);
  }
}


/****************************************************************/


typedef enum {
  insn_format,
  insn_form,
  insn_flags,
  insn_mnemonic,
  insn_name,
  insn_comment,
  nr_insn_table_fields
} insn_table_fields;

typedef enum {
  function_type = insn_format,
  function_name = insn_name,
  function_param = insn_comment
} function_table_fields;

typedef enum {
  model_name = insn_mnemonic,
  model_identifer = insn_name,
  model_default = insn_comment,
} model_table_fields;

typedef struct _insn insn;
struct _insn {
  table_entry *file_entry;
  insn_fields *fields;
  insn *next;
};

typedef struct _insn_undef insn_undef;
struct _insn_undef {
  insn_undef *next;
  char *name;
};

static insn_undef *first_undef, *last_undef;

typedef struct _model model;
struct _model {
  model *next;
  char *name;
  char *printable_name;
  char *insn_default;
  table_model_entry *func_unit_start;
  table_model_entry *func_unit_end;
};
  
typedef struct _insn_table insn_table;
struct _insn_table {
  int opcode_nr;
  insn_bits *expanded_bits;
  int nr_insn;
  insn *insns;
  insn *functions;
  insn *last_function;
  opcode_rules *opcode_rule;
  opcode_field *opcode;
  int nr_entries;
  insn_table *entries;
  insn_table *sibling;
  insn_table *parent;
};

typedef enum {
  insn_model_name,
  insn_model_fields,
  nr_insn_model_table_fields
} insn_model_table_fields;

static model *models;
static model *last_model;

static insn *model_macros;
static insn *last_model_macro;

static insn *model_functions;
static insn *last_model_function;

static insn *model_internal;
static insn *last_model_internal;

static insn *model_static;
static insn *last_model_static;

static insn *model_data;
static insn *last_model_data;

static int max_model_fields_len;

static void
insn_table_insert_function(insn_table *table,
			   table_entry *file_entry)
{
  /* create a new function */
  insn *new_function = ZALLOC(insn);
  new_function->file_entry = file_entry;

  /* append it to the end of the function list */
  if (table->last_function)
    table->last_function->next = new_function;
  else
    table->functions = new_function;
  table->last_function = new_function;
}

static void
insn_table_insert_insn(insn_table *table,
		       table_entry *file_entry,
		       insn_fields *fields)
{
  insn **ptr_to_cur_insn = &table->insns;
  insn *cur_insn = *ptr_to_cur_insn;
  table_model_entry *insn_model_ptr;
  model *model_ptr;

  /* create a new instruction */
  insn *new_insn = ZALLOC(insn);
  new_insn->file_entry = file_entry;
  new_insn->fields = fields;

  /* Check out any model information returned to make sure the model
     is correct.  */
  for(insn_model_ptr = file_entry->model_first; insn_model_ptr; insn_model_ptr = insn_model_ptr->next) {
    char *name = insn_model_ptr->fields[insn_model_name];
    int len = strlen (insn_model_ptr->fields[insn_model_fields]);

    while (len > 0 && isspace(*insn_model_ptr->fields[insn_model_fields])) {
      len--;
      insn_model_ptr->fields[insn_model_fields]++;
    }

    if (max_model_fields_len < len)
      max_model_fields_len = len;

    for(model_ptr = models; model_ptr; model_ptr = model_ptr->next) {
      if (strcmp(name, model_ptr->printable_name) == 0) {

	/* Replace the name field with that of the global model, so that when we
	   want to print it out, we can just compare pointers.  */
	insn_model_ptr->fields[insn_model_name] = model_ptr->printable_name;
	break;
      }
    }

    if (!model_ptr)
      error("%s:%d: machine model `%s' was not known about\n",
	    file_entry->file_name, file_entry->line_nr, name);
  }

  /* insert it according to the order of the fields */
  while (cur_insn != NULL
	 && new_insn->fields->value >= cur_insn->fields->value) {
    ptr_to_cur_insn = &cur_insn->next;
    cur_insn = *ptr_to_cur_insn;
  }

  new_insn->next = cur_insn;
  *ptr_to_cur_insn = new_insn;

  table->nr_insn++;
}


static opcode_field *
insn_table_find_opcode_field(insn *insns,
			     opcode_rules *rule,
			     int string_only)
{
  opcode_field *curr_opcode = ZALLOC(opcode_field);
  insn *entry;
  ASSERT(rule);

  curr_opcode->first = insn_size;
  curr_opcode->last = -1;
  for (entry = insns; entry != NULL; entry = entry->next) {
    insn_fields *fields = entry->fields;
    opcode_field new_opcode;

    /* find a start point for the opcode field */
    new_opcode.first = rule->first;
    while (new_opcode.first <= rule->last
	   && (!string_only
	       || insn_field_is_constant(fields->bits[new_opcode.first],
					 rule) != field_constant_string)
	   && (string_only
	       || !insn_field_is_constant(fields->bits[new_opcode.first],
					  rule)))
      new_opcode.first = fields->bits[new_opcode.first]->last + 1;
    ASSERT(new_opcode.first > rule->last
	   || (string_only
	       && insn_field_is_constant(fields->bits[new_opcode.first],
					 rule) == field_constant_string)
	   || (!string_only
	       && insn_field_is_constant(fields->bits[new_opcode.first],
					 rule)));
  
    /* find the end point for the opcode field */
    new_opcode.last = rule->last;
    while (new_opcode.last >= rule->first
	   && (!string_only
	       || insn_field_is_constant(fields->bits[new_opcode.last],
					 rule) != field_constant_string)
	   && (string_only
	       || !insn_field_is_constant(fields->bits[new_opcode.last],
					  rule)))
      new_opcode.last = fields->bits[new_opcode.last]->first - 1;
    ASSERT(new_opcode.last < rule->first
	   || (string_only
	       && insn_field_is_constant(fields->bits[new_opcode.last],
					 rule) == field_constant_string)
	   || (!string_only
	       && insn_field_is_constant(fields->bits[new_opcode.last],
					 rule)));

    /* now see if our current opcode needs expanding */
    if (new_opcode.first <= rule->last
	&& curr_opcode->first > new_opcode.first)
      curr_opcode->first = new_opcode.first;
    if (new_opcode.last >= rule->first
	&& curr_opcode->last < new_opcode.last)
      curr_opcode->last = new_opcode.last;
    
  }

  /* was any thing interesting found? */
  if (curr_opcode->first > rule->last) {
    ASSERT(curr_opcode->last < rule->first);
    return NULL;
  }
  ASSERT(curr_opcode->last >= rule->first);
  ASSERT(curr_opcode->first <= rule->last);

  /* if something was found, check it includes the forced field range */
  if (!string_only
      && curr_opcode->first > rule->force_first) {
    curr_opcode->first = rule->force_first;
  }
  if (!string_only
      && curr_opcode->last < rule->force_last) {
    curr_opcode->last = rule->force_last;
  }
  /* handle special case elminating any need to do shift after mask */
  if (string_only
      && rule->force_last == insn_size-1) {
    curr_opcode->last = insn_size-1;
  }

  /* handle any special cases */
  switch (rule->special_rule) {
  case 0: /* let the above apply */
    break;
  case 1: /* expand a limited nr of bits, ignoring the rest */
    curr_opcode->first = rule->force_first;
    curr_opcode->last = rule->force_last;
    break;
  case 2: /* boolean field */
    curr_opcode->is_boolean = 1;
    break;
  }

  return curr_opcode;
}


static void
insn_table_insert_expanded(insn_table *table,
			   insn *old_insn,
			   int new_opcode_nr,
			   insn_bits *new_bits)
{
  insn_table **ptr_to_cur_entry = &table->entries;
  insn_table *cur_entry = *ptr_to_cur_entry;

  /* find the new table for this entry */
  while (cur_entry != NULL
	 && cur_entry->opcode_nr < new_opcode_nr) {
    ptr_to_cur_entry = &cur_entry->sibling;
    cur_entry = *ptr_to_cur_entry;
  }

  if (cur_entry == NULL || cur_entry->opcode_nr != new_opcode_nr) {
    insn_table *new_entry = ZALLOC(insn_table);
    new_entry->opcode_nr = new_opcode_nr;
    new_entry->expanded_bits = new_bits;
    new_entry->opcode_rule = table->opcode_rule->next;
    new_entry->sibling = cur_entry;
    new_entry->parent = table;
    *ptr_to_cur_entry = new_entry;
    cur_entry = new_entry;
    table->nr_entries++;
  }
  /* ASSERT new_bits == cur_entry bits */
  ASSERT(cur_entry != NULL && cur_entry->opcode_nr == new_opcode_nr);
  insn_table_insert_insn(cur_entry,
			 old_insn->file_entry,
			 old_insn->fields);
}

static void
insn_table_expand_opcode(insn_table *table,
			 insn *instruction,
			 int field_nr,
			 int opcode_nr,
			 insn_bits *bits)
{

  if (field_nr > table->opcode->last) {
    insn_table_insert_expanded(table, instruction, opcode_nr, bits);
  }
  else {
    insn_field *field = instruction->fields->bits[field_nr];
    if (field->is_int || field->is_slash) {
      ASSERT(field->first >= table->opcode->first
	     && field->last <= table->opcode->last);
      insn_table_expand_opcode(table, instruction, field->last+1,
			       ((opcode_nr << field->width) + field->val_int),
			       bits);
    }
    else {
      int val;
      int last_pos = ((field->last < table->opcode->last)
			? field->last : table->opcode->last);
      int first_pos = ((field->first > table->opcode->first)
			 ? field->first : table->opcode->first);
      int width = last_pos - first_pos + 1;
      int last_val = (table->opcode->is_boolean
		      ? 2 : (1 << width));
      for (val = 0; val < last_val; val++) {
	insn_bits *new_bits = ZALLOC(insn_bits);
	new_bits->field = field;
	new_bits->value = val;
	new_bits->last = bits;
	new_bits->opcode = table->opcode;
	insn_table_expand_opcode(table, instruction, last_pos+1,
				 ((opcode_nr << width) | val),
				 new_bits);
      }
    }
  }
}

static void
insn_table_insert_expanding(insn_table *table,
			    insn *entry)
{
  insn_table_expand_opcode(table,
			   entry,
			   table->opcode->first,
			   0,
			   table->expanded_bits);
}


static void
insn_table_expand_insns(insn_table *table)
{

  ASSERT(table->nr_insn >= 1);

  /* determine a valid opcode */
  while (table->opcode_rule) {
    /* specials only for single instructions */
    if ((table->nr_insn > 1
	 && table->opcode_rule->special_mask == 0
	 && table->opcode_rule->special_rule == 0)
	|| (table->nr_insn == 1
	    && table->opcode_rule->special_mask != 0
	    && ((table->insns->fields->value
		 & table->opcode_rule->special_mask)
		== table->opcode_rule->special_value))
	|| (idecode_expand_semantics
	    && table->opcode_rule->special_mask == 0
	    && table->opcode_rule->special_rule == 0))
      table->opcode =
	insn_table_find_opcode_field(table->insns,
				     table->opcode_rule,
				     table->nr_insn == 1/*string*/
				     );
    if (table->opcode != NULL)
      break;
    table->opcode_rule = table->opcode_rule->next;
  }

  /* did we find anything */
  if (table->opcode == NULL) {
    return;
  }
  ASSERT(table->opcode != NULL);

  /* back link what we found to its parent */
  if (table->parent != NULL) {
    ASSERT(table->parent->opcode != NULL);
    table->opcode->parent = table->parent->opcode;
  }

  /* expand the raw instructions according to the opcode */
  {
    insn *entry;
    for (entry = table->insns; entry != NULL; entry = entry->next) {
      insn_table_insert_expanding(table, entry);
    }
  }

  /* and do the same for the sub entries */
  {
    insn_table *entry;
    for (entry = table->entries; entry != NULL; entry =  entry->sibling) {
      insn_table_expand_insns(entry);
    }
  }
}


static void
model_table_insert(insn_table *table,
		   table_entry *file_entry)
{
  int len;

  /* create a new model */
  model *new_model = ZALLOC(model);

  new_model->name = file_entry->fields[model_identifer];
  new_model->printable_name = file_entry->fields[model_name];
  new_model->insn_default = file_entry->fields[model_default];

  while (*new_model->insn_default && isspace(*new_model->insn_default))
    new_model->insn_default++;

  len = strlen(new_model->insn_default);
  if (max_model_fields_len < len)
    max_model_fields_len = len;

  /* append it to the end of the model list */
  if (last_model)
    last_model->next = new_model;
  else
    models = new_model;
  last_model = new_model;
}

static void
model_table_insert_specific(insn_table *table,
			    table_entry *file_entry,
			    insn **start_ptr,
			    insn **end_ptr)
{
  insn *ptr = ZALLOC(insn);
  ptr->file_entry = file_entry;
  if (*end_ptr)
    (*end_ptr)->next = ptr;
  else
    (*start_ptr) = ptr;
  (*end_ptr) = ptr;
}



static insn_table *
insn_table_load_insns(char *file_name)
{
  table *file = table_open(file_name, nr_insn_table_fields, nr_insn_model_table_fields);
  insn_table *table = ZALLOC(insn_table);
  table_entry *file_entry;
  table->opcode_rule = opcode_table;

  while ((file_entry = table_entry_read(file)) != NULL) {
    if (it_is("function", file_entry->fields[insn_flags])
	|| it_is("internal", file_entry->fields[insn_flags])) {
      insn_table_insert_function(table, file_entry);
    }
    else if (it_is("model", file_entry->fields[insn_flags])) {
      model_table_insert(table, file_entry);
    }
    else if (it_is("model-macro", file_entry->fields[insn_flags])) {
      model_table_insert_specific(table, file_entry, &model_macros, &last_model_macro);
    }
    else if (it_is("model-function", file_entry->fields[insn_flags])) {
      model_table_insert_specific(table, file_entry, &model_functions, &last_model_function);
    }
    else if (it_is("model-internal", file_entry->fields[insn_flags])) {
      model_table_insert_specific(table, file_entry, &model_internal, &last_model_internal);
    }
    else if (it_is("model-static", file_entry->fields[insn_flags])) {
      model_table_insert_specific(table, file_entry, &model_static, &last_model_static);
    }
    else if (it_is("model-data", file_entry->fields[insn_flags])) {
      model_table_insert_specific(table, file_entry, &model_data, &last_model_data);
    }
    else {
      insn_fields *fields;
      /* skip instructions that aren't relevant to the mode */
      filter *filt = filters;
      while (filt != NULL) {
	if (it_is(filt->flag, file_entry->fields[insn_flags]))
	  break;
	filt = filt->next;
      }
      if (filt == NULL) {
	/* create/insert the new instruction */
	fields = parse_insn_format(file_entry,
				   file_entry->fields[insn_format]);
	insn_table_insert_insn(table, file_entry, fields);
      }
    }
  }
  return table;
}


static void
dump_insn(insn *entry, int indent, int levels)
{
  printf("(insn*)%p\n", entry);

  if (levels && entry != NULL) {

    dumpf(indent, "(file_entry ");
    dump_table_entry(entry->file_entry, indent+1);
    dumpf(indent, " )\n");

    dumpf(indent, "(fields ");
    dump_insn_fields(entry->fields, indent+1);
    dumpf(indent, " )\n");

    dumpf(indent, "(next ");
    dump_insn(entry->next, indent+1, levels-1);
    dumpf(indent, " )\n");

  }

}


static void
dump_insn_table(insn_table *table,
		int indent, int levels)
{

  printf("(insn_table*)%p\n", table);

  if (levels && table != NULL) {

    dumpf(indent, "(opcode_nr %d)\n", table->opcode_nr);

    dumpf(indent, "(expanded_bits ");
    dump_insn_bits(table->expanded_bits, indent+1, -1);
    dumpf(indent, " )\n");

    dumpf(indent, "(int nr_insn %d)\n", table->nr_insn);

    dumpf(indent, "(insns ");
    dump_insn(table->insns, indent+1, table->nr_insn);
    dumpf(indent, " )\n");

    dumpf(indent, "(opcode_rule ");
    dump_opcode_rule(table->opcode_rule, indent+1);
    dumpf(indent, " )\n");

    dumpf(indent, "(opcode ");
    dump_opcode_field(table->opcode, indent+1, 1);
    dumpf(indent, " )\n");

    dumpf(indent, "(nr_entries %d)\n", table->entries);
    dumpf(indent, "(entries ");
    dump_insn_table(table->entries, indent+1, table->nr_entries);
    dumpf(indent, " )\n");

    dumpf(indent, "(sibling ", table->sibling);
    dump_insn_table(table->sibling, indent+1, levels-1);
    dumpf(indent, " )\n");

    dumpf(indent, "(parent ", table->parent);
    dump_insn_table(table->parent, indent+1, 0);
    dumpf(indent, " )\n");

  }
}


/****************************************************************/


static void
lf_print_insn_bits(lf *file, insn_bits *bits)
{
  if (bits == NULL)
    return;
  lf_print_insn_bits(file, bits->last);
  lf_putchr(file, '_');
  lf_putstr(file, bits->field->val_string);
  if (!bits->opcode->is_boolean || bits->value == 0) {
    if (bits->opcode->last < bits->field->last)
      lf_putint(file, bits->value << (bits->field->last - bits->opcode->last));
    else
      lf_putint(file, bits->value);
  }
}

static void
lf_print_opcodes(lf *file,
		 insn_table *table)
{
  if (table != NULL) {
    while (1) {
      lf_printf(file, "_%d_%d",
		table->opcode->first,
		table->opcode->last);
      if (table->parent == NULL) break;
      lf_printf(file, "__%d", table->opcode_nr);
      table = table->parent;
    }
  }
}

static void
lf_print_table_name(lf *file,
		    insn_table *table)
{
  lf_printf(file, "idecode_table");
  lf_print_opcodes(file, table);
}



typedef enum {
  function_name_prefix_semantics,
  function_name_prefix_idecode,
  function_name_prefix_itable,
  function_name_prefix_none
} lf_function_name_prefixes;

static void
lf_print_function_name(lf *file,
		       char *basename,
		       insn_bits *expanded_bits,
		       lf_function_name_prefixes prefix)
{

  /* the prefix */
  switch (prefix) {
  case function_name_prefix_semantics:
    lf_putstr(file, "semantic_");
    break;
  case function_name_prefix_idecode:
    lf_printf(file, "idecode_");
    break;
  case function_name_prefix_itable:
    lf_putstr(file, "itable_");
    break;
  default:
    break;
  }

  /* the function name */
  {
    char *pos;
    for (pos = basename;
	 *pos != '\0';
	 pos++) {
      switch (*pos) {
      case '/':
      case '-':
	break;
      case ' ':
	lf_putchr(file, '_');
	break;
      default:
	lf_putchr(file, *pos);
	break;
      }
    }
  }

  /* the suffix */
  if (idecode_expand_semantics)
    lf_print_insn_bits(file, expanded_bits);
}


static void
lf_print_idecode_table(lf *file,
		       insn_table *entry)
{
  int can_assume_leaf;
  opcode_rules *opcode_rule;

  /* have a look at the rule table, if all table rules follow all
     switch rules, I can assume that all end points are leaves */
  opcode_rule = opcode_table;
  while (opcode_rule != NULL
	 && opcode_rule->use_switch)
    opcode_rule = opcode_rule->next;
  while (opcode_rule != NULL
	 && opcode_rule->use_switch
	 && opcode_rule->special_rule)
    opcode_rule = opcode_rule->next;
  can_assume_leaf = opcode_rule == NULL;

  lf_printf(file, "{\n");
  lf_indent(file, +2);
  {
    lf_printf(file, "idecode_table_entry *table = ");
    lf_print_table_name(file, entry);
    lf_printf(file, ";\n");
    lf_printf(file, "int opcode = EXTRACTED32(instruction, %d, %d);\n",
	      i2target(hi_bit_nr, entry->opcode->first),
	      i2target(hi_bit_nr, entry->opcode->last));
    lf_printf(file, "idecode_table_entry *table_entry = table + opcode;\n");
    lf_printf(file, "while (1) {\n");
    lf_indent(file, +2);
    {
      lf_printf(file, "/* nonzero mask -> another table */\n");
      lf_printf(file, "while (table_entry->mask != 0) {\n");
      lf_indent(file, +2);
      {
	lf_printf(file, "table = ((idecode_table_entry*)\n");
	lf_printf(file, "         table_entry->function_or_table);\n");
	lf_printf(file, "opcode = ((instruction & table_entry->mask)\n");
	lf_printf(file, "          >> table_entry->shift);\n");
	lf_printf(file, "table_entry = table + opcode;\n");
      }
      lf_indent(file, -2);
      lf_printf(file, "}\n");
      lf_printf(file, "ASSERT(table_entry->mask == 0);\n");
      if (can_assume_leaf)
	lf_printf(file, "ASSERT(table_entry->shift == 0);\n");
      else {
	lf_printf(file, "if (table_entry->shift == 0)\n");
	lf_indent(file, +2);
      }
      if (idecode_cache) {
	lf_printf(file, "return (((idecode_crack*)\n");
	lf_printf(file, "         table_entry->function_or_table)\n");
	lf_printf(file, "        (%s));\n", cache_idecode_actual);
      }
      else {
	lf_printf(file, "return (((idecode_semantic*)\n");
	lf_printf(file, "         table_entry->function_or_table)\n");
	lf_printf(file, "        (%s));\n", semantic_actual);
      }
      if (!can_assume_leaf) {
	lf_indent(file, -2);
	lf_printf(file, "/* must be a boolean */\n");
	lf_printf(file, "opcode = (instruction & table_entry->shift) != 0;\n");
	lf_printf(file, "table = ((idecode_table_entry*)\n");
	lf_printf(file, "         table_entry->function_or_table);\n");
	lf_printf(file, "table_entry = table + opcode;\n");
      }
    }
    lf_indent(file, -2);
    lf_printf(file, "}\n");
  }
  lf_indent(file, -2);
  lf_printf(file, "}\n");
}


static void
lf_print_my_prefix(lf *file,
		   table_entry *file_entry,
		   int idecode)
{
  lf_printf(file, "const char *const my_prefix __attribute__((__unused__)) = \n");
  lf_printf(file, "  \"%s:%s:%s:%d\";\n",
	    filter_filename (file_entry->file_name),
	    (idecode ? "idecode" : "semantics"),
	    file_entry->fields[insn_name],
	    file_entry->line_nr);
  lf_printf(file, "const itable_index my_index __attribute__((__unused__)) = ");
  lf_print_function_name(file,
			 file_entry->fields[insn_name],
			 NULL,
			 function_name_prefix_itable);
  lf_printf(file, ";\n");
}


static void
lf_print_ptrace(lf *file,
		int idecode)
{
  lf_printf(file, "\n");
  lf_printf(file, "ITRACE(trace_%s, (\"\\n\"));\n",
	    (idecode ? "idecode" : "semantics"));
}


/****************************************************************/

typedef void leaf_handler
(insn_table *entry,
 void *data,
 int depth);
typedef void padding_handler
(insn_table *table,
 void *data,
 int depth,
 int opcode_nr);


static void
insn_table_traverse_tree(insn_table *table,
			 void *data,
			 int depth,
			 leaf_handler *start,
			 leaf_handler *leaf,
			 leaf_handler *end,
			 padding_handler *padding)
{
  insn_table *entry;
  int entry_nr;
  
  ASSERT(table != NULL
	 && table->opcode != NULL
	 && table->nr_entries > 0
	 && table->entries != 0);

  if (start != NULL && depth >= 0)
    start(table, data, depth);

  for (entry_nr = 0, entry = table->entries;
       entry_nr < (table->opcode->is_boolean
		   ? 2
		   : (1 << (table->opcode->last - table->opcode->first + 1)));
       entry_nr ++) {
    if (entry == NULL
	|| (!table->opcode->is_boolean
	    && entry_nr < entry->opcode_nr)) {
      if (padding != NULL && depth >= 0)
	padding(table, data, depth, entry_nr);
    }
    else {
      ASSERT(entry != NULL && (entry->opcode_nr == entry_nr
			       || table->opcode->is_boolean));
      if (entry->opcode != NULL && depth != 0) {
	insn_table_traverse_tree(entry, data, depth+1,
				 start, leaf, end, padding);
      }
      else if (depth >= 0) {
	if (leaf != NULL)
	  leaf(entry, data, depth);
      }
      entry = entry->sibling;
    }
  }
  if (end != NULL && depth >= 0)
    end(table, data, depth);
}


typedef void function_handler
(insn_table *table,
 void *data,
 table_entry *function);

static void
insn_table_traverse_function(insn_table *table,
			     void *data,
			     function_handler *leaf)
{
  insn *function;
  for (function = table->functions;
       function != NULL;
       function = function->next) {
    leaf(table, data, function->file_entry);
  }
}


typedef void insn_handler
(insn_table *table,
 void *data,
 insn *instruction);

static void
insn_table_traverse_insn(insn_table *table,
			 void *data,
			 insn_handler *leaf)
{
  insn *instruction;
  for (instruction = table->insns;
       instruction != NULL;
       instruction = instruction->next) {
    leaf(table, data, instruction);
  }
}


static void
update_depth(insn_table *entry,
	     void *data,
	     int depth)
{
  int *max_depth = (int*)data;
  if (*max_depth < depth)
    *max_depth = depth;
}


static int
insn_table_depth(insn_table *table)
{
  int depth = 0;
  insn_table_traverse_tree(table,
			   &depth,
			   1,
			   NULL, /*start*/
			   update_depth,
			   NULL, /*end*/
			   NULL); /*padding*/
  return depth;
}


/****************************************************************/

static void
dump_traverse_start(insn_table *table,
		    void *data,
		    int depth)
{
  dumpf(depth*2, "(%d\n", table->opcode_nr);
}

static void
dump_traverse_leaf(insn_table *entry,
		   void *data,
		   int depth)
{
  ASSERT(entry->nr_entries == 0
	 && entry->nr_insn == 1
	 && entry->opcode == NULL);
  dumpf(depth*2, ".%d %s\n", entry->opcode_nr,
	entry->insns->file_entry->fields[insn_format]);
}

static void
dump_traverse_end(insn_table *table,
		  void *data,
		  int depth)
{
  dumpf(depth*2, ")\n");
}

static void
dump_traverse_padding(insn_table *table,
		      void *data,
		      int depth,
		      int opcode_nr)
{
  dumpf(depth*2, ".<%d>\n", opcode_nr);
}


static void
dump_traverse(insn_table *table)
{
  insn_table_traverse_tree(table, NULL, 1,
			   dump_traverse_start,
			   dump_traverse_leaf,
			   dump_traverse_end,
			   dump_traverse_padding);
}


/****************************************************************/


static void
lf_print_semantic_function_header(lf *file,
				  char *basename,
				  insn_bits *expanded_bits,
				  int is_function_definition,
				  int is_inline_function)
{
  lf_printf(file, "\n");
  lf_print_function_type(file, "unsigned_word", "EXTERN_SEMANTICS",
			 " ");
  lf_print_function_name(file,
			 basename,
			 expanded_bits,
			 function_name_prefix_semantics);
  lf_printf(file, "\n(%s)", 
	    (idecode_cache ? cache_semantic_formal : semantic_formal));
  if (!is_function_definition)
    lf_printf(file, ";");
  lf_printf(file, "\n");
}


static void
semantics_h_leaf(insn_table *entry,
		 void *data,
		 int depth)
{
  lf *file = (lf*)data;
  ASSERT(entry->nr_insn == 1);
  lf_print_semantic_function_header(file,
				    entry->insns->file_entry->fields[insn_name],
				    entry->expanded_bits,
				    0/* isnt function definition*/,
				    !idecode_cache && entry->parent->opcode_rule->use_switch);
}

static void
semantics_h_insn(insn_table *entry,
		 void *data,
		 insn *instruction)
{
  lf *file = (lf*)data;
  lf_print_semantic_function_header(file,
				    instruction->file_entry->fields[insn_name],
				    NULL,
				    0/*isnt function definition*/,
				    0/*isnt inline function*/);
}

static void
semantics_h_function(insn_table *entry,
		     void *data,
		     table_entry *function)
{
  lf *file = (lf*)data;
  if (function->fields[function_type] == NULL
      || function->fields[function_type][0] == '\0') {
    lf_print_semantic_function_header(file,
				      function->fields[function_name],
				      NULL,
				      0/*isnt function definition*/,
				      1/*is inline function*/);
  }
  else {
    lf_printf(file, "\n");
    lf_print_function_type(file, function->fields[function_type],
			   "INLINE_SEMANTICS", " ");
    lf_printf(file, "%s\n(%s);\n",
	      function->fields[function_name],
	      function->fields[function_param]);
  }
}


static void 
gen_semantics_h(insn_table *table, lf *file)
{

  lf_print_copyleft(file);
  lf_printf(file, "\n");
  lf_printf(file, "#ifndef _SEMANTICS_H_\n");
  lf_printf(file, "#define _SEMANTICS_H_\n");
  lf_printf(file, "\n");

  /* output a declaration for all functions */
  insn_table_traverse_function(table,
			       file,
			       semantics_h_function);

  /* output a declaration for all instructions */
  if (idecode_expand_semantics)
    insn_table_traverse_tree(table,
			     file,
			     1,
			     NULL, /* start */
			     semantics_h_leaf, /* leaf */
			     NULL, /* end */
			     NULL); /* padding */
  else
    insn_table_traverse_insn(table,
			     file,
			     semantics_h_insn);

  lf_printf(file, "\n");
  lf_printf(file, "#endif /* _SEMANTICS_H_ */\n");

}

/****************************************************************/

typedef struct _icache_tree icache_tree;
struct _icache_tree {
  char *name;
  icache_tree *next;
  icache_tree *children;
};

static icache_tree *
icache_tree_insert(icache_tree *tree,
		   char *name)
{
  icache_tree *new_tree;
  /* find it */
  icache_tree **ptr_to_cur_tree = &tree->children;
  icache_tree *cur_tree = *ptr_to_cur_tree;
  while (cur_tree != NULL
	 && strcmp(cur_tree->name, name) < 0) {
    ptr_to_cur_tree = &cur_tree->next;
    cur_tree = *ptr_to_cur_tree;
  }
  ASSERT(cur_tree == NULL
	 || strcmp(cur_tree->name, name) >= 0);
  /* already in the tree */
  if (cur_tree != NULL
      && strcmp(cur_tree->name, name) == 0)
    return cur_tree;
  /* missing, insert it */
  ASSERT(cur_tree == NULL
	 || strcmp(cur_tree->name, name) > 0);
  new_tree = ZALLOC(icache_tree);
  new_tree->name = name;
  new_tree->next = cur_tree;
  *ptr_to_cur_tree = new_tree;
  return new_tree;
}


static icache_tree *
insn_table_cache_fields(insn_table *table)
{
  icache_tree *tree = ZALLOC(icache_tree);
  insn *instruction;
  for (instruction = table->insns;
       instruction != NULL;
       instruction = instruction->next) {
    insn_field *field;
    icache_tree *form =
      icache_tree_insert(tree,
			 instruction->file_entry->fields[insn_form]);
    for (field = instruction->fields->first;
	 field != NULL;
	 field = field->next) {
      if (field->is_string)
	icache_tree_insert(form, field->val_string);
    }
  }
  return tree;
}



static void
gen_icache_h(icache_tree *tree,
	     lf *file)
{
  lf_print_copyleft(file);
  lf_printf(file, "\n");
  lf_printf(file, "#ifndef _ICACHE_H_\n");
  lf_printf(file, "#define _ICACHE_H_\n");
  lf_printf(file, "\n");

  lf_printf(file, "#define WITH_IDECODE_CACHE_SIZE %d\n",
	    idecode_cache);
  lf_printf(file, "\n");

  /* create an instruction cache if being used */
  if (idecode_cache) {
    icache_tree *form;
    lf_printf(file, "typedef struct _idecode_cache {\n");
    lf_printf(file, "  unsigned_word address;\n");
    lf_printf(file, "  void *semantic;\n");
    lf_printf(file, "  union {\n");
    for (form = tree->children;
	 form != NULL;
	 form = form->next) {
      icache_tree *field;
      lf_printf(file, "    struct {\n");
      for (field = form->children;
	   field != NULL;
	   field = field->next) {
	cache_rules *cache_rule;
	int found_rule = 0;
	for (cache_rule = cache_table;
	     cache_rule != NULL;
	     cache_rule = cache_rule->next) {
	  if (strcmp(field->name, cache_rule->old_name) == 0) {
	    found_rule = 1;
	    if (cache_rule->new_name != NULL)
	      lf_printf(file, "      %s %s; /* %s */\n",
			(cache_rule->type == NULL
			 ? "unsigned" 
			 : cache_rule->type),
			cache_rule->new_name,
			cache_rule->old_name);
	  }
	}
	if (!found_rule)
	  lf_printf(file, "      unsigned %s;\n", field->name);
      }
      lf_printf(file, "    } %s;\n", form->name);
    }
    lf_printf(file, "  } crack;\n");
    lf_printf(file, "} idecode_cache;\n");
  }
  else {
    /* alernativly, since no cache, #define the fields to be
       extractions from the instruction variable.  Emit a dummy
       definition for idecode_cache to allow model_issue to not
       be #ifdefed at the call level */
    cache_rules *cache_rule;
    lf_printf(file, "\n");
    lf_printf(file, "typedef void idecode_cache;\n");
    lf_printf(file, "\n");
    for (cache_rule = cache_table;
	 cache_rule != NULL;
	 cache_rule = cache_rule->next) {
      if (cache_rule->expression != NULL
	  && strlen(cache_rule->expression) > 0)
	lf_printf(file, "#define %s %s\n",
		  cache_rule->new_name, cache_rule->expression);
    }
  }

  lf_printf(file, "\n");
  lf_printf(file, "#endif /* _ICACHE_H_ */\n");
}




/****************************************************************/


static void
lf_print_c_extraction(lf *file,
		      insn *instruction,
		      char *field_name,
		      char *field_type,
		      char *field_expression,
		      insn_field *cur_field,
		      insn_bits *bits,
		      int get_value_from_cache,
		      int put_value_in_cache)
{
  ASSERT(field_name != NULL);
  if (bits != NULL
      && (!bits->opcode->is_boolean || bits->value == 0)
      && strcmp(field_name, cur_field->val_string) == 0) {
    ASSERT(bits->field == cur_field);
    ASSERT(field_type == NULL);
    table_entry_lf_c_line_nr(file, instruction->file_entry);
    lf_printf(file, "const unsigned %s __attribute__((__unused__)) = ",
	      field_name);
    if (bits->opcode->last < bits->field->last)
      lf_printf(file, "%d;\n",
		bits->value << (bits->field->last - bits->opcode->last));
    else
      lf_printf(file, "%d;\n", bits->value);
  }
  else if (get_value_from_cache && !put_value_in_cache
	   && semantics_use_cache_struct) {
    insn_undef *undef = ZALLOC(insn_undef);
    /* Use #define to reference the cache struct directly, rather than putting
       them into local variables */
    lf_indent_suppress(file);
    lf_printf(file, "#define %s (cache_entry->crack.%s.%s)\n",
	      field_name,
	      instruction->file_entry->fields[insn_form],
	      field_name);

    if (first_undef)
      last_undef->next = undef;
    else
      first_undef = undef;
    last_undef = undef;;
    undef->name = field_name;
  }
  else {
    /* put the field in the local variable */
    table_entry_lf_c_line_nr(file, instruction->file_entry);
    lf_printf(file, "%s const %s __attribute__((__unused__)) = ",
	      field_type == NULL ? "unsigned" : field_type,
	      field_name);
    /* getting it from the cache */
    if (get_value_from_cache || put_value_in_cache) {
      lf_printf(file, "cache_entry->crack.%s.%s",
		instruction->file_entry->fields[insn_form],
		field_name);
      if (put_value_in_cache) /* also put it in the cache? */
	lf_printf(file, " = ");
    }
    if (!get_value_from_cache) {
      if (strcmp(field_name, cur_field->val_string) == 0)
	lf_printf(file, "EXTRACTED32(instruction, %d, %d)",
		  i2target(hi_bit_nr, cur_field->first),
		  i2target(hi_bit_nr, cur_field->last));
      else if (field_expression != NULL)
	lf_printf(file, "%s", field_expression);
      else
	lf_printf(file, "eval_%s", field_name);
    }
    lf_printf(file, ";\n");
  }
}


static void
lf_print_c_extractions(lf *file,
		       insn *instruction,
		       insn_bits *expanded_bits,
		       int get_value_from_cache,
		       int put_value_in_cache)
{
  insn_field *cur_field;

  /* extract instruction fields */
  lf_printf(file, "/* extraction: %s */\n",
	    instruction->file_entry->fields[insn_format]);

  for (cur_field = instruction->fields->first;
       cur_field->first < insn_size;
       cur_field = cur_field->next) {
    if (cur_field->is_string) {
      insn_bits *bits;
      int found_rule = 0;
      /* find any corresponding value */
      for (bits = expanded_bits;
	   bits != NULL;
	   bits = bits->last) {
	if (bits->field == cur_field)
	  break;
      }
      /* try the cache rule table for what to do */
      if (get_value_from_cache || put_value_in_cache) {      
	cache_rules *cache_rule;
	for (cache_rule = cache_table;
	     cache_rule != NULL;
	     cache_rule = cache_rule->next) {
	  if (strcmp(cur_field->val_string, cache_rule->old_name) == 0) {
	    found_rule = 1;
	    if (cache_rule->valid > 1 && put_value_in_cache)
	      lf_print_c_extraction(file,
				    instruction,
				    cache_rule->new_name,
				    cache_rule->type,
				    cache_rule->expression,
				    cur_field,
				    bits,
				    0,
				    0);
	    else if (cache_rule->valid == 1)
	      lf_print_c_extraction(file,
				    instruction,
				    cache_rule->new_name,
				    cache_rule->type,
				    cache_rule->expression,
				    cur_field,
				    bits,
				    get_value_from_cache,
				    put_value_in_cache);
	  }
	}
      }
      if (found_rule == 0)
	lf_print_c_extraction(file,
			      instruction,
			      cur_field->val_string,
			      0,
			      0,
			      cur_field,
			      bits,
			      get_value_from_cache,
			      put_value_in_cache);
      /* if any (XXX == 0), output a corresponding test */
      if (instruction->file_entry->annex != NULL) {
	char *field_name = cur_field->val_string;
	char *is_0_ptr = instruction->file_entry->annex;
	int field_len = strlen(field_name);
	if (strlen(is_0_ptr) >= (strlen("_is_0") + field_len)) {
	  is_0_ptr += field_len;
	  while ((is_0_ptr = strstr(is_0_ptr, "_is_0")) != NULL) {
	    if (strncmp(is_0_ptr - field_len, field_name, field_len) == 0
		&& !isalpha(is_0_ptr[ - field_len - 1])) {
	      table_entry_lf_c_line_nr(file, instruction->file_entry);
	      lf_printf(file, "const unsigned %s_is_0 __attribute__((__unused__)) = (", field_name);
	      if (bits != NULL)
		lf_printf(file, "%d", bits->value);
	      else
		lf_printf(file, "%s", field_name);
	      lf_printf(file, " == 0);\n");
	      break;
	    }
	    is_0_ptr += strlen("_is_0");
	  }
	}
      }
      /* any thing else ... */
    }
  }
  lf_print_lf_c_line_nr(file);
}


static void
lf_print_idecode_illegal(lf *file)
{
  if (idecode_cache)
    lf_printf(file, "return idecode_illegal(%s);\n", cache_idecode_actual);
  else
    lf_printf(file, "return semantic_illegal(%s);\n", semantic_actual);
}


static void
lf_print_idecode_floating_point_unavailable(lf *file)
{
  if (idecode_cache)
    lf_printf(file, "return idecode_floating_point_unavailable(%s);\n",
	      cache_idecode_actual);
  else
    lf_printf(file, "return semantic_floating_point_unavailable(%s);\n",
	      semantic_actual);
}


/* Output code to do any final checks on the decoded instruction.
   This includes things like verifying any on decoded fields have the
   correct value and checking that (for floating point) floating point
   hardware isn't disabled */

static void
lf_print_c_validate(lf *file,
		    insn *instruction,
		    opcode_field *opcodes)
{
  /* Validate: unchecked instruction fields

     If any constant fields in the instruction were not checked by the
     idecode tables, output code to check that they have the correct
     value here */
  { 
    unsigned check_mask = 0;
    unsigned check_val = 0;
    insn_field *field;
    opcode_field *opcode;

    /* form check_mask/check_val containing what needs to be checked
       in the instruction */
    for (field = instruction->fields->first;
	 field->first < insn_size;
	 field = field->next) {

      check_mask <<= field->width;
      check_val <<= field->width;

      /* is it a constant that could need validating? */
      if (!field->is_int && !field->is_slash)
	continue;

      /* has it been checked by a table? */
      for (opcode = opcodes; opcode != NULL; opcode = opcode->parent) {
	if (field->first >= opcode->first
	    && field->last <= opcode->last)
	  break;
      }
      if (opcode != NULL)
	continue;

      check_mask |= (1 << field->width)-1;
      check_val |= field->val_int;
    }

    /* if any bits not checked by opcode tables, output code to check them */
    if (check_mask) {
      lf_printf(file, "\n");
      lf_printf(file, "/* validate: %s */\n",
		instruction->file_entry->fields[insn_format]);
      lf_printf(file, "if (WITH_RESERVED_BITS && (instruction & 0x%x) != 0x%x)\n",
		check_mask, check_val);
      lf_indent(file, +2);
      lf_print_idecode_illegal(file);
      lf_indent(file, -2);
    }
  }

  /* Validate floating point hardware

     If the simulator is being built with out floating point hardware
     (different to it being disabled in the MSR) then floating point
     instructions are invalid */
  {
    if (it_is("f", instruction->file_entry->fields[insn_flags])) {
      lf_printf(file, "\n");
      lf_printf(file, "/* Validate: FP hardware exists */\n");
      lf_printf(file, "if (CURRENT_FLOATING_POINT != HARD_FLOATING_POINT)\n");
      lf_indent(file, +2);
      lf_print_idecode_illegal(file);
      lf_indent(file, -2);
    }
  }

  /* Validate: Floating Point available

     If floating point is not available, we enter a floating point
     unavailable interrupt into the cache instead of the instruction
     proper.

     The PowerPC spec requires a CSI after MSR[FP] is changed and when
     ever a CSI occures we flush the instruction cache. */

  {
    if (it_is("f", instruction->file_entry->fields[insn_flags])) {
      lf_printf(file, "\n");
      lf_printf(file, "/* Validate: FP available according to MSR[FP] */\n");
      lf_printf(file, "if (!IS_FP_AVAILABLE(processor))\n");
      lf_indent(file, +2);
      lf_print_idecode_floating_point_unavailable(file);
      lf_indent(file, -2);
    }
  }
}


static void
lf_print_c_cracker(lf *file,
		   insn *instruction,
		   insn_bits *expanded_bits,
		   opcode_field *opcodes)
{

  /* function header */
  lf_printf(file, "{\n");
  lf_indent(file, +2);

  lf_print_my_prefix(file,
		     instruction->file_entry,
		     1/*putting-value-in-cache*/);

  lf_print_ptrace(file,
		  1/*putting-value-in-cache*/);

  lf_print_c_validate(file, instruction, opcodes);

  lf_printf(file, "\n");
  lf_printf(file, "{\n");
  lf_indent(file, +2);
  lf_print_c_extractions(file,
			 instruction,
			 expanded_bits,
			 0/*get_value_from_cache*/,
			 1/*put_value_in_cache*/);
  lf_indent(file, -2);
  lf_printf(file, "}\n");

  /* return the function propper (main sorts this one out) */
  lf_printf(file, "\n");
  lf_printf(file, "/* semantic routine */\n");
  table_entry_lf_c_line_nr(file, instruction->file_entry);
  lf_printf(file, "return ");
  lf_print_function_name(file,
			 instruction->file_entry->fields[insn_name],
			 expanded_bits,
			 function_name_prefix_semantics);
  lf_printf(file, ";\n");

  lf_print_lf_c_line_nr(file);
  lf_indent(file, -2);
  lf_printf(file, "}\n");
}


static void
lf_print_c_semantic(lf *file,
		    insn *instruction,
		    insn_bits *expanded_bits,
		    opcode_field *opcodes)
{

  lf_printf(file, "{\n");
  lf_indent(file, +2);

  lf_print_my_prefix(file,
		     instruction->file_entry,
		     0/*not putting value in cache*/);
  lf_printf(file, "unsigned_word nia = cia + %d;\n", insn_size / 8);

  lf_printf(file, "\n");
  lf_print_c_extractions(file,
			 instruction,
			 expanded_bits,
			 idecode_cache/*get_value_from_cache*/,
			 0/*put_value_in_cache*/);

  lf_print_ptrace(file,
		  0/*put_value_in_cache*/);

  /* validate the instruction, if a cache this has already been done */
  if (!idecode_cache)
    lf_print_c_validate(file, instruction, opcodes);

  /* generate the profiling call - this is delayed until after the
     instruction has been verified */
  lf_printf(file, "\n");
  lf_printf(file, "if (WITH_MON & MONITOR_INSTRUCTION_ISSUE) {\n");
  lf_printf(file, "  mon_issue(");
  lf_print_function_name(file,
			 instruction->file_entry->fields[insn_name],
			 NULL,
			 function_name_prefix_itable);
  lf_printf(file, ", processor, cia);\n");
  lf_printf(file, "}\n");
  lf_printf(file, "\n");

  /* generate the code (or at least something */
  if (instruction->file_entry->annex != NULL) {
    /* true code */
    lf_printf(file, "\n");
    table_entry_lf_c_line_nr(file, instruction->file_entry);
    lf_printf(file, "{\n");
    lf_indent(file, +2);
    lf_print_c_code(file, instruction->file_entry->annex);
    lf_indent(file, -2);
    lf_printf(file, "}\n");
    lf_print_lf_c_line_nr(file);
  }
  else if (it_is("nop", instruction->file_entry->fields[insn_flags])) {
    lf_print_lf_c_line_nr(file);
  }
  else if (it_is("f", instruction->file_entry->fields[insn_flags])) {
    /* unimplemented floating point instruction - call for assistance */
    lf_printf(file, "\n");
    lf_printf(file, "/* unimplemented floating point instruction - call for assistance */\n");
    table_entry_lf_c_line_nr(file, instruction->file_entry);
    lf_putstr(file, "floating_point_assist_interrupt(processor, cia);\n");
    lf_print_lf_c_line_nr(file);
  }
  else {
    /* abort so it is implemented now */
    table_entry_lf_c_line_nr(file, instruction->file_entry);
    lf_putstr(file, "error(\"%s: unimplemented, cia=0x%x\\n\", my_prefix, cia);\n");
    lf_print_lf_c_line_nr(file);
    lf_printf(file, "\n");
  }

  lf_printf(file, "return nia;\n");
  lf_indent(file, -2);
  lf_printf(file, "}\n");
}

static void
lf_print_c_semantic_function(lf *file,
			     insn *instruction,
			     insn_bits *expanded_bits,
			     opcode_field *opcodes,
			     int is_inline_function)
{
  insn_undef *undef, *next;

  /* build the semantic routine to execute the instruction */
  lf_print_semantic_function_header(file,
				    instruction->file_entry->fields[insn_name],
				    expanded_bits,
				    1/*is-function-definition*/,
				    is_inline_function);
  lf_print_c_semantic(file,
		      instruction,
		      expanded_bits,
		      opcodes);

  /* If we are referencing the cache structure directly instead of putting the values
     in local variables, undef any defines we created */
  for(undef = first_undef; undef; undef = next) {
    next = undef->next;
    lf_indent_suppress(file);
    lf_printf(file, "#undef %s\n", undef->name);
    free((void *)undef);
  }
  first_undef = (insn_undef *)0;
  last_undef = (insn_undef *)0;
}


static void
semantics_c_leaf(insn_table *entry,
		 void *data,
		 int depth)
{
  lf *file = (lf*)data;
  ASSERT(entry->nr_insn == 1
	 && entry->opcode == NULL
	 && entry->parent != NULL
	 && entry->parent->opcode != NULL);
  lf_print_c_semantic_function(file,
			       entry->insns,
			       entry->expanded_bits,
			       entry->parent->opcode,
			       !idecode_cache && entry->parent->opcode_rule->use_switch);
}

static void
semantics_c_insn(insn_table *table,
		 void *data,
		 insn *instruction)
{
  lf *file = (lf*)data;
  lf_print_c_semantic_function(file, instruction,
			       NULL, NULL,
			       0/*isnt_inline_function*/);
}

static void
semantics_c_function(insn_table *table,
		     void *data,
		     table_entry *function)
{
  lf *file = (lf*)data;
  if (function->fields[function_type] == NULL
      || function->fields[function_type][0] == '\0') {
    lf_print_semantic_function_header(file,
				      function->fields[function_name],
				      NULL,
				      1/*is function definition*/,
				      1/*is inline function*/);
  }
  else {
    lf_printf(file, "\n");
    lf_print_function_type(file, function->fields[function_type],
			   "INLINE_SEMANTICS", "\n");
    lf_printf(file, "%s(%s)\n",
	      function->fields[function_name],
	      function->fields[function_param]);
  }
  table_entry_lf_c_line_nr(file, function);
  lf_printf(file, "{\n");
  lf_indent(file, +2);
  lf_print_c_code(file, function->annex);
  lf_indent(file, -2);
  lf_printf(file, "}\n");
  lf_print_lf_c_line_nr(file);
}



static void 
gen_semantics_c(insn_table *table, lf *file)
{
  lf_print_copyleft(file);
  lf_printf(file, "\n");
  lf_printf(file, "#ifndef _SEMANTICS_C_\n");
  lf_printf(file, "#define _SEMANTICS_C_\n");
  lf_printf(file, "\n");
  lf_printf(file, "#include \"cpu.h\"\n");
  lf_printf(file, "#include \"idecode.h\"\n");
  lf_printf(file, "#include \"semantics.h\"\n");
  lf_printf(file, "\n");

  /* output a definition (c-code) for all functions */
  insn_table_traverse_function(table,
			       file,
			       semantics_c_function);

  /* output a definition (c-code) for all instructions */
  if (idecode_expand_semantics)
    insn_table_traverse_tree(table,
			     file,
			     1,
			     NULL, /* start */
			     semantics_c_leaf,
			     NULL, /* end */
			     NULL); /* padding */
  else
    insn_table_traverse_insn(table,
			     file,
			     semantics_c_insn);

  lf_printf(file, "\n");
  lf_printf(file, "#endif /* _SEMANTICS_C_ */\n");
}


/****************************************************************/

static void
gen_idecode_h(insn_table *table, lf *file)
{
  lf_print_copyleft(file);
  lf_printf(file, "\n");
  lf_printf(file, "#ifndef _IDECODE_H_\n");
  lf_printf(file, "#define _IDECODE_H_\n");
  lf_printf(file, "\n");
  lf_printf(file, "#include \"idecode_expression.h\"\n");
  lf_printf(file, "#include \"idecode_fields.h\"\n");
  lf_printf(file, "#include \"idecode_branch.h\"\n");
  lf_printf(file, "\n");
  lf_printf(file, "#include \"icache.h\"\n");
  lf_printf(file, "\n");
  lf_printf(file, "typedef unsigned_word idecode_semantic\n(%s);\n",
	    (idecode_cache ? cache_semantic_formal : semantic_formal));
  lf_printf(file, "\n");
  if (idecode_cache) {
    lf_print_function_type(file, "idecode_semantic *", "INLINE_IDECODE", " ");
    lf_printf(file, "idecode\n(%s);\n", cache_idecode_formal);
  }
  else {
    lf_print_function_type(file, "unsigned_word", "INLINE_IDECODE", " ");
    lf_printf(file, "idecode_issue\n(%s);\n", semantic_formal);
  }
  lf_printf(file, "\n");
  lf_printf(file, "#endif /* _IDECODE_H_ */\n");
}


/****************************************************************/


static void
idecode_table_start(insn_table *table,
		    void *data,
		    int depth)
{
  lf *file = (lf*)data;
  ASSERT(depth == 0);
  /* start of the table */
  if (!table->opcode_rule->use_switch) {
    lf_printf(file, "\n");
    lf_printf(file, "static idecode_table_entry ");
    lf_print_table_name(file, table);
    lf_printf(file, "[] = {\n");
  }
}

static void
idecode_table_leaf(insn_table *entry,
		   void *data,
		   int depth)
{
  lf *file = (lf*)data;
  ASSERT(entry->parent != NULL);
  ASSERT(depth == 0);

  /* add an entry to the table */
  if (!entry->parent->opcode_rule->use_switch) {
    if (entry->opcode == NULL) {
      /* table leaf entry */
      lf_printf(file, "  /*%d*/ { 0, 0, ", entry->opcode_nr);
      lf_print_function_name(file,
			     entry->insns->file_entry->fields[insn_name],
			     entry->expanded_bits,
			     (idecode_cache
			      ? function_name_prefix_idecode
			      : function_name_prefix_semantics));
      lf_printf(file, " },\n");
    }
    else if (entry->opcode_rule->use_switch) {
      /* table calling switch statement */
      lf_printf(file, "  /*%d*/ { 0, 0, ",
		entry->opcode_nr);
      lf_print_table_name(file, entry);
      lf_printf(file, " },\n");
    }
    else {
      /* table `calling' another table */
      lf_printf(file, "  /*%d*/ { ", entry->opcode_nr);
      if (entry->opcode->is_boolean)
	lf_printf(file, "MASK32(%d,%d), 0, ",
		  i2target(hi_bit_nr, entry->opcode->first),
		  i2target(hi_bit_nr, entry->opcode->last));
      else
	lf_printf(file, "%d, MASK32(%d,%d), ",
		  insn_size - entry->opcode->last - 1,
		  i2target(hi_bit_nr, entry->opcode->first),
		  i2target(hi_bit_nr, entry->opcode->last));
      lf_print_table_name(file, entry);
      lf_printf(file, " },\n");
    }
  }
}

static void
idecode_table_end(insn_table *table,
		  void *data,
		  int depth)
{
  lf *file = (lf*)data;
  ASSERT(depth == 0);

  if (!table->opcode_rule->use_switch) {
    lf_printf(file, "};\n");
  }
}

static void
idecode_table_padding(insn_table *table,
		      void *data,
		      int depth,
		      int opcode_nr)
{
  lf *file = (lf*)data;
  ASSERT(depth == 0);

  if (!table->opcode_rule->use_switch) {
    lf_printf(file, "  /*%d*/ { 0, 0, %s_illegal },\n",
	      opcode_nr, (idecode_cache ? "idecode" : "semantic"));
  }
}


/****************************************************************/


void lf_print_idecode_switch
(lf *file, 
 insn_table *table);


static void
idecode_switch_start(insn_table *table,
		void *data,
		int depth)
{
  lf *file = (lf*)data;
  ASSERT(depth == 0);
  ASSERT(table->opcode_rule->use_switch);

  lf_printf(file, "switch (EXTRACTED32(instruction, %d, %d)) {\n",
	    i2target(hi_bit_nr, table->opcode->first),
	    i2target(hi_bit_nr, table->opcode->last));
}


static void
idecode_switch_leaf(insn_table *entry,
		    void *data,
		    int depth)
{
  lf *file = (lf*)data;
  ASSERT(entry->parent != NULL);
  ASSERT(depth == 0);
  ASSERT(entry->parent->opcode_rule->use_switch);
  ASSERT(entry->parent->opcode);

  if (!entry->parent->opcode->is_boolean
      || entry->opcode_nr == 0)
    lf_printf(file, "case %d:\n", entry->opcode_nr);
  else
    lf_printf(file, "default:\n");
  lf_indent(file, +2);
  {
    if (entry->opcode == NULL) {
      /* switch calling leaf */
      lf_printf(file, "return ");
      lf_print_function_name(file,
			     entry->insns->file_entry->fields[insn_name],
			     entry->expanded_bits,
			     (idecode_cache
			      ? function_name_prefix_idecode
			      : function_name_prefix_semantics));
      if (idecode_cache)
	lf_printf(file, "(%s);\n", cache_idecode_actual);
      else
	lf_printf(file, "(%s);\n", semantic_actual);
    }
    else if (entry->opcode_rule->use_switch) {
      /* switch calling switch */
      lf_print_idecode_switch(file, entry);
    }
    else {
      /* switch looking up a table */
      lf_print_idecode_table(file, entry);
    }
    lf_printf(file, "break;\n");
  }
  lf_indent(file, -2);
}


static void
lf_print_idecode_switch_illegal(lf *file)
{
  lf_indent(file, +2);
  lf_print_idecode_illegal(file);
  lf_printf(file, "break;\n");
  lf_indent(file, -2);
}

static void
idecode_switch_end(insn_table *table,
		   void *data,
		   int depth)
{
  lf *file = (lf*)data;
  ASSERT(depth == 0);
  ASSERT(table->opcode_rule->use_switch);
  ASSERT(table->opcode);

  if (table->opcode_rule->use_switch == 1
      && !table->opcode->is_boolean) {
    lf_printf(file, "default:\n");
    lf_print_idecode_switch_illegal(file);
  }
  lf_printf(file, "}\n");
}

static void
idecode_switch_padding(insn_table *table,
		       void *data,
		       int depth,
		       int opcode_nr)
{
  lf *file = (lf*)data;

  ASSERT(depth == 0);
  ASSERT(table->opcode_rule->use_switch);

  if (table->opcode_rule->use_switch > 1) {
    lf_printf(file, "case %d:\n", opcode_nr);
    lf_print_idecode_switch_illegal(file);
  }
}


void
lf_print_idecode_switch(lf *file, 
			insn_table *table)
{
  insn_table_traverse_tree(table,
			   file,
			   0,
			   idecode_switch_start,
			   idecode_switch_leaf,
			   idecode_switch_end,
			   idecode_switch_padding);
}


static void
lf_print_idecode_switch_function_header(lf *file,
					insn_table *table,
					int is_function_definition)
{
  lf_printf(file, "\n");
  lf_printf(file, "static ");
  if (idecode_cache)
    lf_printf(file, "idecode_semantic *");
  else
    lf_printf(file, "unsigned_word");
  if (is_function_definition)
    lf_printf(file, "\n");
  else
    lf_printf(file, " ");
  lf_print_table_name(file, table);
  lf_printf(file, "\n(%s)",
	    (idecode_cache ? cache_idecode_formal : semantic_formal));
  if (!is_function_definition)
    lf_printf(file, ";");
  lf_printf(file, "\n");
}


static void
idecode_declare_if_switch(insn_table *table,
			  void *data,
			  int depth)
{
  lf *file = (lf*)data;

  if (table->opcode_rule->use_switch
      && table->parent != NULL /* don't declare the top one yet */
      && !table->parent->opcode_rule->use_switch) {
    lf_print_idecode_switch_function_header(file,
					    table,
					    0/*isnt function definition*/);
  }
}


static void
idecode_expand_if_switch(insn_table *table,
			 void *data,
			 int depth)
{
  lf *file = (lf*)data;

  if (table->opcode_rule->use_switch
      && table->parent != NULL /* don't expand the top one yet */
      && !table->parent->opcode_rule->use_switch) {
    lf_print_idecode_switch_function_header(file,
					    table,
					    1/*is function definition*/);
    lf_printf(file, "{\n");
    {
      lf_indent(file, +2);
      lf_print_idecode_switch(file, table);
      lf_indent(file, -2);
    }
    lf_printf(file, "}\n");
  }
}


static void
lf_print_c_cracker_function(lf *file,
			    insn *instruction,
			    insn_bits *expanded_bits,
			    opcode_field *opcodes,
			    int is_inline_function)
{
  /* if needed, generate code to enter this routine into a cache */
  lf_printf(file, "\n");
  lf_printf(file, "static idecode_semantic *\n");
  lf_print_function_name(file,
			 instruction->file_entry->fields[insn_name],
			 expanded_bits,
			 function_name_prefix_idecode);
  lf_printf(file, "\n(%s)\n", cache_idecode_formal);

  lf_print_c_cracker(file,
		     instruction,
		     expanded_bits,
		     opcodes);
}

static void
idecode_crack_leaf(insn_table *entry,
		   void *data,
		   int depth)
{
  lf *file = (lf*)data;
  ASSERT(entry->nr_insn == 1
	 && entry->opcode == NULL
	 && entry->parent != NULL
	 && entry->parent->opcode != NULL
	 && entry->parent->opcode_rule != NULL);
  lf_print_c_cracker_function(file,
			      entry->insns,
			      entry->expanded_bits,
			      entry->opcode,
			      entry->parent->opcode_rule->use_switch);
}

static void
idecode_crack_insn(insn_table *entry,
		   void *data,
		   insn *instruction)
{
  lf *file = (lf*)data;
  lf_print_c_cracker_function(file,
			      instruction,
			      NULL,
			      NULL,
			      0/*isnt inline function*/);
}

static void
idecode_c_internal_function(insn_table *table,
			    void *data,
			    table_entry *function)
{
  lf *file = (lf*)data;
  ASSERT(idecode_cache != 0);
  if (it_is("internal", function->fields[insn_flags])) {
    lf_printf(file, "\n");
    lf_print_function_type(file, "idecode_semantic *", "STATIC_INLINE_IDECODE",
			   "\n");
    lf_print_function_name(file,
			   function->fields[insn_name],
			   NULL,
			   function_name_prefix_idecode);
    lf_printf(file, "\n(%s)\n", cache_idecode_formal);
    lf_printf(file, "{\n");
    lf_indent(file, +2);
    lf_printf(file, "/* semantic routine */\n");
    table_entry_lf_c_line_nr(file, function);
    lf_printf(file, "return ");
    lf_print_function_name(file,
			   function->fields[insn_name],
			   NULL,
			   function_name_prefix_semantics);
    lf_printf(file, ";\n");

    lf_print_lf_c_line_nr(file);
    lf_indent(file, -2);
    lf_printf(file, "}\n");
  }
}


/****************************************************************/

static void
gen_idecode_c(insn_table *table, lf *file)
{
  int depth;

  /* the intro */
  lf_print_copyleft(file);
  lf_printf(file, "\n");
  lf_printf(file, "\n");
  lf_printf(file, "#ifndef _IDECODE_C_\n");
  lf_printf(file, "#define _IDECODE_C_\n");
  lf_printf(file, "\n");
  lf_printf(file, "#include \"cpu.h\"\n");
  lf_printf(file, "#include \"idecode.h\"\n");
  lf_printf(file, "#include \"semantics.h\"\n");
  lf_printf(file, "\n");
  lf_printf(file, "\n");
  lf_printf(file, "typedef idecode_semantic *idecode_crack\n(%s);\n",
	    (idecode_cache ? cache_idecode_formal : semantic_formal));
  lf_printf(file, "\n");
  lf_printf(file, "typedef struct _idecode_table_entry {\n");
  lf_printf(file, "  unsigned shift;\n");
  lf_printf(file, "  unsigned mask;\n");
  lf_printf(file, "  void *function_or_table;\n");
  lf_printf(file, "} idecode_table_entry;\n");
  lf_printf(file, "\n");
  lf_printf(file, "\n");

  /* output `internal' invalid/floating-point unavailable functions
     where needed */
  if (idecode_cache) {
    insn_table_traverse_function(table,
				 file,
				 idecode_c_internal_function);
  }

  /* output cracking functions where needed */
  if (idecode_cache) {
    if (idecode_expand_semantics)
      insn_table_traverse_tree(table,
			       file,
			       1,
			       NULL,
			       idecode_crack_leaf,
			       NULL,
			       NULL);
    else
      insn_table_traverse_insn(table,
			       file,
			       idecode_crack_insn);
  }

  /* output switch function declarations where needed by tables */
  insn_table_traverse_tree(table,
			   file,
			   1,
			   idecode_declare_if_switch, /* START */
			   NULL, NULL, NULL);

  /* output tables where needed */
  for (depth = insn_table_depth(table);
       depth > 0;
       depth--) {
    insn_table_traverse_tree(table,
			     file,
			     1-depth,
			     idecode_table_start,
			     idecode_table_leaf,
			     idecode_table_end,
			     idecode_table_padding);
  }

  /* output switch functions where needed */
  insn_table_traverse_tree(table,
			   file,
			   1,
			   idecode_expand_if_switch, /* START */
			   NULL, NULL, NULL);

  /* output the main idecode routine */
  lf_printf(file, "\n");
  if (idecode_cache) {
    lf_print_function_type(file, "idecode_semantic *", "INLINE_IDECODE", "\n");
    lf_printf(file, "idecode\n(%s)\n", cache_idecode_formal);
  }
  else {
    lf_print_function_type(file, "unsigned_word", "INLINE_IDECODE", "\n");
    lf_printf(file, "idecode_issue\n(%s)\n", semantic_formal);
  }
  lf_printf(file, "{\n");
  lf_indent(file, +2);
  if (table->opcode_rule->use_switch)
    lf_print_idecode_switch(file, table);
  else
    lf_print_idecode_table(file, table);
  lf_indent(file, -2);
  lf_printf(file, "}\n");
  lf_printf(file, "\n");
  lf_printf(file, "#endif /* _IDECODE_C_ */\n");
}


/****************************************************************/

static void
itable_h_insn(insn_table *entry,
	      void *data,
	      insn *instruction)
{
  lf *file = (lf*)data;
  lf_printf(file, "  ");
  lf_print_function_name(file,
			 instruction->file_entry->fields[insn_name],
			 NULL,
			 function_name_prefix_itable);
  lf_printf(file, ",\n");
}


static void 
gen_itable_h(insn_table *table, lf *file)
{

  lf_print_copyleft(file);
  lf_printf(file, "\n");
  lf_printf(file, "#ifndef _ITABLE_H_\n");
  lf_printf(file, "#define _ITABLE_H_\n");
  lf_printf(file, "\n");

  /* output an enumerated type for each instruction */
  lf_printf(file, "typedef enum {\n");
  insn_table_traverse_insn(table,
			   file,
			   itable_h_insn);
  lf_printf(file, "  nr_itable_entries,\n");
  lf_printf(file, "} itable_index;\n");
  lf_printf(file, "\n");

  /* output the table that contains the actual instruction info */
  lf_printf(file, "typedef struct _itable_instruction_info {\n");
  lf_printf(file, "  itable_index nr;\n");
  lf_printf(file, "  char *format;\n");
  lf_printf(file, "  char *form;\n");
  lf_printf(file, "  char *flags;\n");
  lf_printf(file, "  char *mnemonic;\n");
  lf_printf(file, "  char *name;\n");
  lf_printf(file, "} itable_info;\n");
  lf_printf(file, "\n");
  lf_printf(file, "extern itable_info itable[nr_itable_entries];\n");

  lf_printf(file, "\n");
  lf_printf(file, "#endif /* _ITABLE_C_ */\n");

}

/****************************************************************/

static void
itable_c_insn(insn_table *entry,
	      void *data,
	      insn *instruction)
{
  lf *file = (lf*)data;
  char **fields = instruction->file_entry->fields;
  lf_printf(file, "  { ");
  lf_print_function_name(file,
			 instruction->file_entry->fields[insn_name],
			 NULL,
			 function_name_prefix_itable);
  lf_printf(file, ",\n");
  lf_printf(file, "    \"%s\",\n", fields[insn_format]);
  lf_printf(file, "    \"%s\",\n", fields[insn_form]);
  lf_printf(file, "    \"%s\",\n", fields[insn_flags]);
  lf_printf(file, "    \"%s\",\n", fields[insn_mnemonic]);
  lf_printf(file, "    \"%s\",\n", fields[insn_name]);
  lf_printf(file, "    },\n");
}


static void 
gen_itable_c(insn_table *table, lf *file)
{

  lf_print_copyleft(file);
  lf_printf(file, "\n");
  lf_printf(file, "#ifndef _ITABLE_C_\n");
  lf_printf(file, "#define _ITABLE_C_\n");
  lf_printf(file, "\n");
  lf_printf(file, "#include \"itable.h\"\n");
  lf_printf(file, "\n");

  /* output the table that contains the actual instruction info */
  lf_printf(file, "itable_info itable[nr_itable_entries] = {\n");
  insn_table_traverse_insn(table,
			   file,
			   itable_c_insn);
  lf_printf(file, "};\n");
  lf_printf(file, "\n");

  lf_printf(file, "\n");
  lf_printf(file, "#endif /* _ITABLE_C_ */\n");
}

/****************************************************************/

static void
model_c_or_h_data(insn_table *table,
		  lf *file,
		  table_entry *data)
{
  if (data->annex) {
    table_entry_lf_c_line_nr(file, data);
    lf_print_c_code(file, data->annex);
    lf_print_lf_c_line_nr(file);
    lf_printf(file, "\n");
  }
}

static void
model_c_or_h_function(insn_table *entry,
		      lf *file,
		      table_entry *function,
		      char *prefix)
{
  if (function->fields[function_type] == NULL
      || function->fields[function_type][0] == '\0') {
    error("Model function type not specified for %s", function->fields[function_name]);
  }
  lf_printf(file, "\n");
  lf_print_function_type(file, function->fields[function_type], prefix, " ");
  lf_printf(file, "%s\n(%s);\n",
	    function->fields[function_name],
	    function->fields[function_param]);
  lf_printf(file, "\n");
}

static void 
gen_model_h(insn_table *table, lf *file)
{
  insn *insn_ptr;
  model *model_ptr;
  insn *macro;
  char *name;
  int model_create_p = 0;
  int model_init_p = 0;
  int model_halt_p = 0;
  int model_mon_info_p = 0;
  int model_mon_info_free_p = 0;

  lf_print_copyleft(file);
  lf_printf(file, "\n");
  lf_printf(file, "#ifndef _MODEL_H_\n");
  lf_printf(file, "#define _MODEL_H_\n");
  lf_printf(file, "\n");

  for(macro = model_macros; macro; macro = macro->next) {
    model_c_or_h_data(table, file, macro->file_entry);
  }

  lf_printf(file, "typedef enum _model_enum {\n");
  lf_printf(file, "  MODEL_NONE,\n");
  for (model_ptr = models; model_ptr; model_ptr = model_ptr->next) {
    lf_printf(file, "  MODEL_%s,\n", model_ptr->name);
  }
  lf_printf(file, "  nr_models\n");
  lf_printf(file, "} model_enum;\n");
  lf_printf(file, "\n");

  lf_printf(file, "#define DEFAULT_MODEL MODEL_%s\n", (models) ? models->name : "NONE");
  lf_printf(file, "\n");

  lf_printf(file, "typedef struct _model_data model_data;\n");
  lf_printf(file, "typedef struct _model_time model_time;\n");
  lf_printf(file, "\n");

  lf_printf(file, "extern model_enum current_model;\n");
  lf_printf(file, "extern const char *model_name[ (int)nr_models ];\n");
  lf_printf(file, "extern const char *const *const model_func_unit_name[ (int)nr_models ];\n");
  lf_printf(file, "extern const model_time *const model_time_mapping[ (int)nr_models ];\n");
  lf_printf(file, "\n");

  for(insn_ptr = model_functions; insn_ptr; insn_ptr = insn_ptr->next) {
    model_c_or_h_function(table, file, insn_ptr->file_entry, "INLINE_MODEL");
    name = insn_ptr->file_entry->fields[function_name];
    if (strcmp (name, "model_create") == 0)
      model_create_p = 1;
    else if (strcmp (name, "model_init") == 0)
      model_init_p = 1;
    else if (strcmp (name, "model_halt") == 0)
      model_halt_p = 1;
    else if (strcmp (name, "model_mon_info") == 0)
      model_mon_info_p = 1;
    else if (strcmp (name, "model_mon_info_free") == 0)
      model_mon_info_free_p = 1;
  }

  if (!model_create_p) {
    lf_print_function_type(file, "model_data *", "INLINE_MODEL", " ");
    lf_printf(file, "model_create\n");
    lf_printf(file, "(cpu *processor);\n");
    lf_printf(file, "\n");
  }

  if (!model_init_p) {
    lf_print_function_type(file, "void", "INLINE_MODEL", " ");
    lf_printf(file, "model_init\n");
    lf_printf(file, "(model_data *model_ptr);\n");
    lf_printf(file, "\n");
  }

  if (!model_halt_p) {
    lf_print_function_type(file, "void", "INLINE_MODEL", " ");
    lf_printf(file, "model_halt\n");
    lf_printf(file, "(model_data *model_ptr);\n");
    lf_printf(file, "\n");
  }

  if (!model_mon_info_p) {
    lf_print_function_type(file, "model_print *", "INLINE_MODEL", " ");
    lf_printf(file, "model_mon_info\n");
    lf_printf(file, "(model_data *model_ptr);\n");
    lf_printf(file, "\n");
  }

  if (!model_mon_info_free_p) {
    lf_print_function_type(file, "void", "INLINE_MODEL", " ");
    lf_printf(file, "model_mon_info_free\n");
    lf_printf(file, "(model_data *model_ptr,\n");
    lf_printf(file, " model_print *info_ptr);\n");
    lf_printf(file, "\n");
  }

  lf_print_function_type(file, "void", "INLINE_MODEL", " ");
  lf_printf(file, "model_set\n");
  lf_printf(file, "(const char *name);\n");
  lf_printf(file, "\n");
  lf_printf(file, "#endif /* _MODEL_H_ */\n");
}

/****************************************************************/

typedef struct _model_c_passed_data model_c_passed_data;
struct _model_c_passed_data {
  lf *file;
  model *model_ptr;
};

static void
model_c_insn(insn_table *entry,
	      void *data,
	      insn *instruction)
{
  model_c_passed_data *data_ptr = (model_c_passed_data *)data;
  lf *file = data_ptr->file;
  char *current_name = data_ptr->model_ptr->printable_name;
  table_model_entry *model_ptr = instruction->file_entry->model_first;

  while (model_ptr) {
    if (model_ptr->fields[insn_model_name] == current_name) {
      lf_printf(file, "  { %-*s },  /* %s */\n",
		max_model_fields_len,
		model_ptr->fields[insn_model_fields],
		instruction->file_entry->fields[insn_name]);
      return;
    }

    model_ptr = model_ptr->next;
  }

  lf_printf(file, "  { %-*s },  /* %s */\n",
	    max_model_fields_len,
	    data_ptr->model_ptr->insn_default,
	    instruction->file_entry->fields[insn_name]);
}

static void
model_c_function(insn_table *table,
		 lf *file,
		 table_entry *function,
		 const char *prefix)
{
  if (function->fields[function_type] == NULL
      || function->fields[function_type][0] == '\0') {
    error("Model function return type not specified for %s", function->fields[function_name]);
  }
  else {
    lf_printf(file, "\n");
    lf_print_function_type(file, function->fields[function_type], prefix, "\n");
    lf_printf(file, "%s(%s)\n",
	      function->fields[function_name],
	      function->fields[function_param]);
  }
  table_entry_lf_c_line_nr(file, function);
  lf_printf(file, "{\n");
  if (function->annex) {
    lf_indent(file, +2);
    lf_print_c_code(file, function->annex);
    lf_indent(file, -2);
  }
  lf_printf(file, "}\n");
  lf_print_lf_c_line_nr(file);
  lf_printf(file, "\n");
}

static void 
gen_model_c(insn_table *table, lf *file)
{
  insn *insn_ptr;
  model *model_ptr;
  char *name;
  int model_create_p = 0;
  int model_init_p = 0;
  int model_halt_p = 0;
  int model_mon_info_p = 0;
  int model_mon_info_free_p = 0;

  lf_print_copyleft(file);
  lf_printf(file, "\n");
  lf_printf(file, "#ifndef _MODEL_C_\n");
  lf_printf(file, "#define _MODEL_C_\n");
  lf_printf(file, "\n");
  lf_printf(file, "#include \"cpu.h\"\n");
  lf_printf(file, "#include \"mon.h\"\n");
  lf_printf(file, "\n");
  lf_printf(file, "#ifdef HAVE_STDLIB_H\n");
  lf_printf(file, "#include <stdlib.h>\n");
  lf_printf(file, "#endif\n");
  lf_printf(file, "\n");

  for(insn_ptr = model_data; insn_ptr; insn_ptr = insn_ptr->next) {
    model_c_or_h_data(table, file, insn_ptr->file_entry);
  }

  for(insn_ptr = model_static; insn_ptr; insn_ptr = insn_ptr->next) {
    model_c_or_h_function(table, file, insn_ptr->file_entry, "/*h*/STATIC");
  }

  for(insn_ptr = model_internal; insn_ptr; insn_ptr = insn_ptr->next) {
    model_c_or_h_function(table, file, insn_ptr->file_entry, "STATIC_INLINE_MODEL");
  }

  for(insn_ptr = model_static; insn_ptr; insn_ptr = insn_ptr->next) {
    model_c_function(table, file, insn_ptr->file_entry, "/*c*/STATIC");
  }

  for(insn_ptr = model_internal; insn_ptr; insn_ptr = insn_ptr->next) {
    model_c_function(table, file, insn_ptr->file_entry, "STATIC_INLINE_MODEL");
  }

  for(insn_ptr = model_functions; insn_ptr; insn_ptr = insn_ptr->next) {
    model_c_function(table, file, insn_ptr->file_entry, "INLINE_MODEL");
    name = insn_ptr->file_entry->fields[function_name];
    if (strcmp (name, "model_create") == 0)
      model_create_p = 1;
    else if (strcmp (name, "model_init") == 0)
      model_init_p = 1;
    else if (strcmp (name, "model_halt") == 0)
      model_halt_p = 1;
    else if (strcmp (name, "model_mon_info") == 0)
      model_mon_info_p = 1;
    else if (strcmp (name, "model_mon_info_free") == 0)
      model_mon_info_free_p = 1;
  }

  if (!model_create_p) {
    lf_print_function_type(file, "model_data *", "INLINE_MODEL", "\n");
    lf_printf(file, "model_create(cpu *processor)\n");
    lf_printf(file, "{\n");
    lf_printf(file, "  return (model_data *)0;\n");
    lf_printf(file, "}\n");
    lf_printf(file, "\n");
  }

  if (!model_init_p) {
    lf_print_function_type(file, "void", "INLINE_MODEL", "\n");
    lf_printf(file, "model_init(model_data *model_ptr)\n");
    lf_printf(file, "{\n");
    lf_printf(file, "}\n");
    lf_printf(file, "\n");
  }

  if (!model_halt_p) {
    lf_print_function_type(file, "void", "INLINE_MODEL", "\n");
    lf_printf(file, "model_halt(model_data *model_ptr)\n");
    lf_printf(file, "{\n");
    lf_printf(file, "}\n");
    lf_printf(file, "\n");
  }

  if (!model_mon_info_p) {
    lf_print_function_type(file, "model_print *", "INLINE_MODEL", "\n");
    lf_printf(file, "model_mon_info(model_data *model_ptr)\n");
    lf_printf(file, "{\n");
    lf_printf(file, "  return (model_print *)0;\n");
    lf_printf(file, "}\n");
    lf_printf(file, "\n");
  }

  if (!model_mon_info_free_p) {
    lf_print_function_type(file, "void", "INLINE_MODEL", "\n");
    lf_printf(file, "model_mon_info_free(model_data *model_ptr,\n");
    lf_printf(file, "                    model_print *info_ptr)\n");
    lf_printf(file, "{\n");
    lf_printf(file, "}\n");
    lf_printf(file, "\n");
  }

  lf_printf(file, "/* Insn functional unit info */\n");
  for(model_ptr = models; model_ptr; model_ptr = model_ptr->next) {
    model_c_passed_data data;

    lf_printf(file, "static const model_time model_time_%s[] = {\n", model_ptr->name);
    data.file = file;
    data.model_ptr = model_ptr;
    insn_table_traverse_insn(table,
			     (void *)&data,
			     model_c_insn);

    lf_printf(file, "};\n");
    lf_printf(file, "\n");
    lf_printf(file, "\f\n");
  }

  lf_printf(file, "#ifndef _INLINE_C_\n");
  lf_printf(file, "const model_time *const model_time_mapping[ (int)nr_models ] = {\n");
  lf_printf(file, "  (const model_time *const)0,\n");
  for(model_ptr = models; model_ptr; model_ptr = model_ptr->next) {
    lf_printf(file, "  model_time_%s,\n", model_ptr->name);
  }
  lf_printf(file, "};\n");
  lf_printf(file, "#endif\n");
  lf_printf(file, "\n");

  lf_printf(file, "\f\n");
  lf_printf(file, "/* map model enumeration into printable string */\n");
  lf_printf(file, "#ifndef _INLINE_C_\n");
  lf_printf(file, "const char *model_name[ (int)nr_models ] = {\n");
  lf_printf(file, "  \"NONE\",\n");
  for (model_ptr = models; model_ptr; model_ptr = model_ptr->next) {
    lf_printf(file, "  \"%s\",\n", model_ptr->printable_name);
  }
  lf_printf(file, "};\n");
  lf_printf(file, "#endif\n");
  lf_printf(file, "\n");

  lf_print_function_type(file, "void", "INLINE_MODEL", "\n");
  lf_printf(file, "model_set(const char *name)\n");
  lf_printf(file, "{\n");
  if (models) {
    lf_printf(file, "  model_enum model;\n");
    lf_printf(file, "  for(model = MODEL_%s; model < nr_models; model++) {\n", models->name);
    lf_printf(file, "    if(strcmp(name, model_name[model]) == 0) {\n");
    lf_printf(file, "      current_model = model;\n");
    lf_printf(file, "      return;\n");
    lf_printf(file, "    }\n");
    lf_printf(file, "  }\n");
    lf_printf(file, "\n");
    lf_printf(file, "  error(\"Unknown model '%%s', Models which are known are:%%s\n\",\n");
    lf_printf(file, "        name,\n");
    lf_printf(file, "        \"");
    for(model_ptr = models; model_ptr; model_ptr = model_ptr->next) {
      lf_printf(file, "\\n\\t%s", model_ptr->printable_name);
    }
    lf_printf(file, "\");\n");
  } else {
    lf_printf(file, "  error(\"No models are currently known about\");\n");
  }

  lf_printf(file, "}\n");
  lf_printf(file, "\n");

  lf_printf(file, "#endif /* _MODEL_C_ */\n");

}

/****************************************************************/


int
main(int argc,
     char **argv,
     char **envp)
{
  insn_table *instructions = NULL;
  icache_tree *cache_fields = NULL;
  char *real_file_name = NULL;
  int ch;

  if (argc == 1) {
    printf("Usage:\n");
    printf("  igen <config-opts> ... <input-opts>... <output-opts>...\n");
    printf("Config options:\n");
    printf("  -f <filter-out-flag>  eg -f 64 to skip 64bit instructions\n");
    printf("  -e                    Expand (duplicate) semantic functions\n");
    printf("  -r <icache-size>      Generate cracking cache version\n");
    printf("  -R                    Use defines to reference cache vars\n");
    printf("  -l                    Supress line numbering in output files\n");
    printf("  -b <bit-size>         Set the number of bits in an instruction\n");
    printf("  -h <high-bit>         Set the nr of the high (msb bit)\n");
    printf("\n");
    printf("Input options (ucase version also dumps loaded table):\n");
    printf("  -[Oo] <opcode-rules>\n");
    printf("  -[Kk] <cache-rules>\n");
    printf("  -[Ii] <instruction-table>\n");
    printf("\n");
    printf("Output options:\n");
    printf("  -[Cc] <output-file>  output icache.h(C) invalid(c)\n");
    printf("  -[Dd] <output-file>  output idecode.h(D) idecode.c(d)\n");
    printf("  -[Mm] <output-file>  output model.h(M) model.c(M)\n");
    printf("  -[Ss] <output-file>  output schematic.h(S) schematic.c(s)\n");
    printf("  -[Tt] <table>        output itable.h(T) itable.c(t)\n");
  }

  while ((ch = getopt(argc, argv,
		      "leb:h:r:Rf:I:i:O:o:K:k:M:m:n:S:s:D:d:T:t:C:")) != -1) {
    fprintf(stderr, "\t-%c %s\n", ch, (optarg ? optarg : ""));
    switch(ch) {
    case 'l':
      number_lines = 0;
      break;
    case 'e':
      idecode_expand_semantics = 1;
      break;
    case 'r':
      idecode_cache = a2i(optarg);
      break;
    case 'R':
      semantics_use_cache_struct = 1;
      break;
    case 'b':
      insn_size = a2i(optarg);
      ASSERT(insn_size > 0 && insn_size <= max_insn_size
	     && (hi_bit_nr == insn_size-1 || hi_bit_nr == 0));
      break;
    case 'h':
      hi_bit_nr = a2i(optarg);
      ASSERT(hi_bit_nr == insn_size-1 || hi_bit_nr == 0);
      break;
    case 'f':
      {
	filter *new_filter = ZALLOC(filter);
	new_filter->flag = strdup(optarg);
	new_filter->next = filters;
	filters = new_filter;
	break;
      }
    case 'I':
    case 'i':
      ASSERT(opcode_table != NULL);
      ASSERT(cache_table != NULL);
      instructions = insn_table_load_insns(optarg);
      fprintf(stderr, "\texpanding ...\n");
      insn_table_expand_insns(instructions);
      fprintf(stderr, "\tcache fields ...\n");
      cache_fields = insn_table_cache_fields(instructions);
      if (ch == 'I') {
	dump_traverse(instructions);
	dump_insn_table(instructions, 0, 1);
      }
      break;
    case 'O':
    case 'o':
      opcode_table = load_opcode_rules(optarg);
      if (ch == 'O')
	dump_opcode_rules(opcode_table, 0);
      break;
    case 'K':
    case 'k':
      cache_table = load_cache_rules(optarg);
      if (ch == 'K')
	dump_cache_rules(cache_table, 0);
      break;
    case 'n':
      real_file_name = strdup(optarg);
      break;
    case 'S':
    case 's':
    case 'D':
    case 'd':
    case 'M':
    case 'm':
    case 'T':
    case 't':
    case 'C':
      {
	lf *file = lf_open(optarg, real_file_name, number_lines);
	ASSERT(instructions != NULL);
	switch (ch) {
	case 'S':
	  gen_semantics_h(instructions, file);
	  break;
	case 's':
	  gen_semantics_c(instructions, file);
	  break;
	case 'D':
	  gen_idecode_h(instructions, file);
	  break;
	case 'd':
	  gen_idecode_c(instructions, file);
	  break;
	case 'M':
	  gen_model_h(instructions, file);
	  break;
	case 'm':
	  gen_model_c(instructions, file);
	  break;
	case 'T':
	  gen_itable_h(instructions, file);
	  break;
	case 't':
	  gen_itable_c(instructions, file);
	  break;
	case 'C':
	  gen_icache_h(cache_fields, file);
	  break;
	}
	lf_close(file);
      }
      real_file_name = NULL;
      break;
    default:
      error("unknown option\n");
    }
  }
  return 0;
}
