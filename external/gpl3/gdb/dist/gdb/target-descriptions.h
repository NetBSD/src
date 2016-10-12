/* Target description support for GDB.

   Copyright (C) 2006-2016 Free Software Foundation, Inc.

   Contributed by CodeSourcery.

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

#ifndef TARGET_DESCRIPTIONS_H
#define TARGET_DESCRIPTIONS_H 1

struct tdesc_feature;
struct tdesc_arch_data;
struct tdesc_type;
struct tdesc_reg;
struct target_desc;
struct target_ops;
struct target_desc;
/* An inferior's target description info is stored in this opaque
   object.  There's one such object per inferior.  */
struct target_desc_info;
struct inferior;

/* Fetch the current inferior's description, and switch its current
   architecture to one which incorporates that description.  */

void target_find_description (void);

/* Discard any description fetched from the target for the current
   inferior, and switch the current architecture to one with no target
   description.  */

void target_clear_description (void);

/* Return the current inferior's target description.  This should only
   be used by gdbarch initialization code; most access should be
   through an existing gdbarch.  */

const struct target_desc *target_current_description (void);

/* Copy inferior target description data.  Used for example when
   handling (v)forks, where child's description is the same as the
   parent's, since the child really is a copy of the parent.  */

void copy_inferior_target_desc_info (struct inferior *destinf,
				     struct inferior *srcinf);

/* Free a target_desc_info object.  */

void target_desc_info_free (struct target_desc_info *tdesc_info);

/* Returns true if INFO indicates the target description had been
   supplied by the user.  */

int target_desc_info_from_user_p (struct target_desc_info *info);

/* Record architecture-specific functions to call for pseudo-register
   support.  If tdesc_use_registers is called and gdbarch_num_pseudo_regs
   is greater than zero, then these should be called as well.
   They are equivalent to the gdbarch methods with similar names,
   except that they will only be called for pseudo registers.  */

void set_tdesc_pseudo_register_name
  (struct gdbarch *gdbarch, gdbarch_register_name_ftype *pseudo_name);

void set_tdesc_pseudo_register_type
  (struct gdbarch *gdbarch, gdbarch_register_type_ftype *pseudo_type);

void set_tdesc_pseudo_register_reggroup_p
  (struct gdbarch *gdbarch,
   gdbarch_register_reggroup_p_ftype *pseudo_reggroup_p);

/* Update GDBARCH to use the TARGET_DESC for registers.  TARGET_DESC
   may be GDBARCH's target description or (if GDBARCH does not have
   one which describes registers) another target description
   constructed by the gdbarch initialization routine.

   Fixed register assignments are taken from EARLY_DATA, which is freed.
   All registers which have not been assigned fixed numbers are given
   numbers above the current value of gdbarch_num_regs.
   gdbarch_num_regs and various  register-related predicates are updated to
   refer to the target description.  This function should only be called from
   the architecture's gdbarch initialization routine, and only after
   successfully validating the required registers.  */

void tdesc_use_registers (struct gdbarch *gdbarch,
			  const struct target_desc *target_desc,
			  struct tdesc_arch_data *early_data);

/* Allocate initial data for validation of a target description during
   gdbarch initialization.  */

struct tdesc_arch_data *tdesc_data_alloc (void);

/* Clean up data allocated by tdesc_data_alloc.  This should only
   be called to discard the data; tdesc_use_registers takes ownership
   of its EARLY_DATA argument.  */

void tdesc_data_cleanup (void *data_untyped);

/* Search FEATURE for a register named NAME.  Record REGNO and the
   register in DATA; when tdesc_use_registers is called, REGNO will be
   assigned to the register.  1 is returned if the register was found,
   0 if it was not.  */

int tdesc_numbered_register (const struct tdesc_feature *feature,
			     struct tdesc_arch_data *data,
			     int regno, const char *name);

/* Search FEATURE for a register named NAME, but do not assign a fixed
   register number to it.  */

int tdesc_unnumbered_register (const struct tdesc_feature *feature,
			       const char *name);

/* Search FEATURE for a register named NAME, and return its size in
   bits.  The register must exist.  */

int tdesc_register_size (const struct tdesc_feature *feature,
			 const char *name);

/* Search FEATURE for a register with any of the names from NAMES
   (NULL-terminated).  Record REGNO and the register in DATA; when
   tdesc_use_registers is called, REGNO will be assigned to the
   register.  1 is returned if the register was found, 0 if it was
   not.  */

int tdesc_numbered_register_choices (const struct tdesc_feature *feature,
				     struct tdesc_arch_data *data,
				     int regno, const char *const names[]);


/* Accessors for target descriptions.  */

/* Return the BFD architecture associated with this target
   description, or NULL if no architecture was specified.  */

const struct bfd_arch_info *tdesc_architecture
  (const struct target_desc *);

/* Return the OSABI associated with this target description, or
   GDB_OSABI_UNKNOWN if no osabi was specified.  */

enum gdb_osabi tdesc_osabi (const struct target_desc *);

/* Return non-zero if this target description is compatible
   with the given BFD architecture.  */

int tdesc_compatible_p (const struct target_desc *,
			const struct bfd_arch_info *);

/* Return the string value of a property named KEY, or NULL if the
   property was not specified.  */

const char *tdesc_property (const struct target_desc *,
			    const char *key);

/* Return 1 if this target description describes any registers.  */

int tdesc_has_registers (const struct target_desc *);

/* Return the feature with the given name, if present, or NULL if
   the named feature is not found.  */

const struct tdesc_feature *tdesc_find_feature (const struct target_desc *,
						const char *name);

/* Return the name of FEATURE.  */

const char *tdesc_feature_name (const struct tdesc_feature *feature);

/* Return the type associated with ID in the context of FEATURE, or
   NULL if none.  */

struct tdesc_type *tdesc_named_type (const struct tdesc_feature *feature,
				     const char *id);

/* Return the name of register REGNO, from the target description or
   from an architecture-provided pseudo_register_name method.  */

const char *tdesc_register_name (struct gdbarch *gdbarch, int regno);

/* Return the type of register REGNO, from the target description or
   from an architecture-provided pseudo_register_type method.  */

struct type *tdesc_register_type (struct gdbarch *gdbarch, int regno);

/* Return the type associated with ID, from the target description.  */

struct type *tdesc_find_type (struct gdbarch *gdbarch, const char *id);

/* Check whether REGNUM is a member of REGGROUP using the target
   description.  Return -1 if the target description does not
   specify a group.  */

int tdesc_register_in_reggroup_p (struct gdbarch *gdbarch, int regno,
				  struct reggroup *reggroup);

/* Methods for constructing a target description.  */

struct target_desc *allocate_target_description (void);
struct cleanup *make_cleanup_free_target_description (struct target_desc *);
void set_tdesc_architecture (struct target_desc *,
			     const struct bfd_arch_info *);
void set_tdesc_osabi (struct target_desc *, enum gdb_osabi osabi);
void set_tdesc_property (struct target_desc *,
			 const char *key, const char *value);
void tdesc_add_compatible (struct target_desc *,
			   const struct bfd_arch_info *);

struct tdesc_feature *tdesc_create_feature (struct target_desc *tdesc,
					    const char *name);
struct tdesc_type *tdesc_create_vector (struct tdesc_feature *feature,
					const char *name,
					struct tdesc_type *field_type,
					int count);
struct tdesc_type *tdesc_create_struct (struct tdesc_feature *feature,
					const char *name);
void tdesc_set_struct_size (struct tdesc_type *type, int size);
struct tdesc_type *tdesc_create_union (struct tdesc_feature *feature,
				       const char *name);
struct tdesc_type *tdesc_create_flags (struct tdesc_feature *feature,
				       const char *name,
				       int size);
struct tdesc_type *tdesc_create_enum (struct tdesc_feature *feature,
				      const char *name,
				      int size);
void tdesc_add_field (struct tdesc_type *type, const char *field_name,
		      struct tdesc_type *field_type);
void tdesc_add_typed_bitfield (struct tdesc_type *type, const char *field_name,
			       int start, int end,
			       struct tdesc_type *field_type);
void tdesc_add_bitfield (struct tdesc_type *type, const char *field_name,
			 int start, int end);
void tdesc_add_flag (struct tdesc_type *type, int start,
		     const char *flag_name);
void tdesc_add_enum_value (struct tdesc_type *type, int value,
			   const char *name);
void tdesc_create_reg (struct tdesc_feature *feature, const char *name,
		       int regnum, int save_restore, const char *group,
		       int bitsize, const char *type);

#endif /* TARGET_DESCRIPTIONS_H */
