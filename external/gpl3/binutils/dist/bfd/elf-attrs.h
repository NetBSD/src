/* ELF attributes support (based on ARM EABI attributes).
   Copyright (C) 2025 Free Software Foundation, Inc.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#pragma once

#include "hidden.h"
#include <stdint.h>
#include "hidden.h"

typedef enum obj_attr_version {
  OBJ_ATTR_VERSION_NONE = 0,
  OBJ_ATTR_VERSION_UNSUPPORTED,
  OBJ_ATTR_V1,
  OBJ_ATTR_V2,
  OBJ_ATTR_VERSION_MAX = OBJ_ATTR_V2,
} obj_attr_version_t;

/* --------------------
   Object attributes v2
   -------------------- */

typedef enum obj_attr_encoding_v2
{
  OA_ENC_UNSET   = 0,
  OA_ENC_ULEB128,
  OA_ENC_NTBS,
  OA_ENC_MAX     = OA_ENC_NTBS,
} obj_attr_encoding_v2_t;

#define obj_attr_encoding_v2_from_u8(value) \
  ((enum obj_attr_encoding_v2) ((value) + 1))
#define obj_attr_encoding_v2_to_u8(value) \
  ((uint8_t) ((value) - 1))

extern const char *
bfd_oav2_encoding_to_string (obj_attr_encoding_v2_t);

typedef union obj_attr_value_v2 {
  uint32_t uint;

  /* Note: this field cannot hold e.g. a string literal as the value has to be
     freeable.  */
  const char *string;
} obj_attr_value_v2_t;

typedef enum obj_attr_v2_status
{
  /* An attribute that is unknown to the linker, and so cannot be merged.  */
  obj_attr_v2_unknown = 0,
  /* An attribute that was reported as corrupted.  */
  obj_attr_v2_corrupted,
  /* A valid attribute.  */
  obj_attr_v2_ok,
} obj_attr_v2_status_t;

typedef uint64_t obj_attr_tag_t;

typedef struct obj_attr_v2 {
  /* The name/tag of an attribute.  */
  obj_attr_tag_t tag;

  /* The value assigned to an attribute, can be ULEB128 or NTBS.  */
  union obj_attr_value_v2 val;

  /* The attribute status after merge.  */
  obj_attr_v2_status_t status;

  /* The next attribute in the list or NULL.  */
  struct obj_attr_v2 *next;

  /* The previous attribute in the list or NULL.  */
  struct obj_attr_v2 *prev;
} obj_attr_v2_t;

typedef enum obj_attr_subsection_scope_v2
{
  OA_SUBSEC_PUBLIC,
  OA_SUBSEC_PRIVATE,
} obj_attr_subsection_scope_v2_t;

typedef enum obj_attr_subsection_v2_status
{
  /* A subsection that is unknown to the linker, and so cannot be merged.  */
  obj_attr_subsection_v2_unknown = 0,
  /* A subsection that was reported as corrupted.  */
  obj_attr_subsection_v2_corrupted,
  /* A valid subsection.  */
  obj_attr_subsection_v2_ok,
} obj_attr_subsection_v2_status_t;

typedef struct obj_attr_subsection_v2 {
  /* The name of the subsection.
     Note: this field cannot hold e.g. a string literal as the value has to be
     freeable.  */
  const char *name;

  /* The scope of the subsection.  */
  obj_attr_subsection_scope_v2_t scope;

  /* Is this subsection optional ?  Can it be skipped ?  */
  bool optional;

  /* The value encoding of attributes in this subsection.  */
  obj_attr_encoding_v2_t encoding;

  /* The subsection status after merge.  */
  obj_attr_subsection_v2_status_t status;

  /* The size of the list.  */
  unsigned int size;

  /* The next subsection in the list, or NULL.  */
  struct obj_attr_subsection_v2 *next;

  /* The previous subsection in the list, or NULL.  */
  struct obj_attr_subsection_v2 *prev;

  /* A pointer to the first node of the list.  */
  struct obj_attr_v2 *first;

  /* A pointer to the last node of the list.  */
  struct obj_attr_v2 *last;
} obj_attr_subsection_v2_t;

extern const char *
bfd_oav2_comprehension_to_string (bool);

typedef struct obj_attr_subsection_list
{
  /* A pointer to the first node of the list.  */
  obj_attr_subsection_v2_t *first;

  /* A pointer to the last node of the list.  */
  obj_attr_subsection_v2_t *last;

  /* The size of the list.  */
  unsigned int size;
} obj_attr_subsection_list_t;

typedef struct {
  const char *const name;
  obj_attr_tag_t value;
} obj_attr_tag_info_t;

/* Attribute information.  */
typedef struct {
  obj_attr_tag_info_t tag;
  obj_attr_value_v2_t default_value;
} obj_attr_info_t;

typedef struct
{
  const char *const subsec_name;
  const obj_attr_info_t *known_attrs;
  const bool optional;
  const obj_attr_encoding_v2_t encoding;
  const size_t len;
} known_subsection_v2_t;

struct elf_backend_data;

extern const known_subsection_v2_t *
bfd_obj_attr_v2_identify_subsection (const struct elf_backend_data *,
				     const char *);

extern const obj_attr_info_t *
_bfd_obj_attr_v2_find_known_by_tag (const struct elf_backend_data *,
				    const char *,
				    obj_attr_tag_t) ATTRIBUTE_HIDDEN;

extern const char *
_bfd_obj_attr_v2_tag_to_string (const struct elf_backend_data *,
				const char *,
				obj_attr_tag_t) ATTRIBUTE_HIDDEN;

enum obj_attr_v2_merge_result_reason
{
  /* Default: everything is ok.  */
  OAv2_MERGE_OK = 0,
  /* The result value of the merge is the same as REF.  */
  OAv2_MERGE_SAME_VALUE_AS_REF,
  /* No implementation of a merge for this attribute exists.  */
  OAv2_MERGE_UNSUPPORTED,
  /* The merge failed, an error message should be logged.  */
  OAv2_MERGE_ERROR,
};
typedef struct {
  /* Should the merge be performed ?  */
  bool merge;
  /* The merged value.  */
  union obj_attr_value_v2 val;
  /* If the merge should not be performed, give the reason to differentiate
     error cases from normal cases.  Typically, if REF already is set to the
     same value as the merged result, no merge is needed, and this is not an
     error.  */
  enum obj_attr_v2_merge_result_reason reason;
} obj_attr_v2_merge_result_t;

/* Re-usable merge policies.  */
/* For now, only AND-merge is used by AArch64 backend.  Additional policies
   (Integer-OR, String-ADD) are part of the GNU testing namespace.  If they
   appear to be usefull for a backend at some point, they should be exposed
   to the backend here below.  */
extern obj_attr_v2_merge_result_t
_bfd_obj_attr_v2_merge_AND (const struct bfd_link_info *, const bfd *,
			    const obj_attr_subsection_v2_t *,
			    const obj_attr_v2_t *, const obj_attr_v2_t *,
			    const obj_attr_v2_t *) ATTRIBUTE_HIDDEN;
