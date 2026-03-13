/* AArch64-specific backend routines.
   Copyright (C) 2009-2024 Free Software Foundation, Inc.
   Contributed by ARM Ltd.

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
   along with this program; see the file COPYING3. If not,
   see <http://www.gnu.org/licenses/>.  */

extern void bfd_elf64_aarch64_init_maps
  (bfd *);

extern void bfd_elf32_aarch64_init_maps
  (bfd *);

/* Types of PLTs based on the level of security.  This would be a
   bit-mask to denote which of the combinations of security features
   are enabled:
   - No security feature PLTs
   - PLTs with BTI instruction
   - PLTs with PAC instruction
*/
typedef enum
{
  PLT_NORMAL	= 0x0,  /* Normal plts.  */
  PLT_BTI	= 0x1,  /* plts with BTI.  */
  PLT_PAC	= 0x2,  /* plts with pointer authentication.  */
  PLT_BTI_PAC	= PLT_BTI | PLT_PAC
} aarch64_plt_type;

/* Indicates whether the linker should generate warnings, errors, or nothing
   when input objects are missing GNU feature property markings and the output
   has the markings.  */
typedef enum
{
  MARKING_NONE	= 0,  /* Does not emit any warning/error messages.  */
  MARKING_WARN	= 1,  /* Emit warning when the input objects are missing GNU
			 feature property markings, and the output has the
			 markings.  */
  MARKING_ERROR	= 2,  /* Emit error when the input objects are missing GNU
			 feature property markings, and the output has the
			 markings.  */
} aarch64_feature_marking_report;

/* To indicate whether GNU_PROPERTY_AARCH64_FEATURE_1_GCS bit is
   enabled/disabled on the output when -z gcs linker
   command line option is passed.  */
typedef enum
{
  GCS_NEVER	= 0,  /* gcs is disabled on output.  */
  GCS_IMPLICIT  = 1,  /* gcs is deduced from input object.  */
  GCS_ALWAYS	= 2,  /* gsc is enabled on output.  */
} aarch64_gcs_type;

/* A structure to encompass all information about software protections coming
   from BTI or PAC related command line options.  */
struct aarch64_protection_opts
{
  /* PLT type to use depending on the selected software proctections.  */
  aarch64_plt_type plt_type;

  /* Report level for BTI issues.  */
  aarch64_feature_marking_report bti_report;

  /* Look-up mode for GCS property.  */
  aarch64_gcs_type gcs_type;

  /* Report level for GCS issues.  */
  aarch64_feature_marking_report gcs_report;
};
typedef struct aarch64_protection_opts aarch64_protection_opts;

struct elf_aarch64_local_symbol;
struct elf_aarch64_obj_tdata
{
  struct elf_obj_tdata root;

  /* local symbol descriptors */
  struct elf_aarch64_local_symbol *locals;

  /* Zero to warn when linking objects with incompatible enum sizes.  */
  int no_enum_size_warning;

  /* Zero to warn when linking objects with incompatible wchar_t sizes.  */
  int no_wchar_size_warning;

  /* All GNU_PROPERTY_AARCH64_FEATURE_1_AND properties.  */
  uint32_t gnu_property_aarch64_feature_1_and;

  /* Software protections options.  */
  struct aarch64_protection_opts sw_protections;

  /* Number of reported BTI issues.  */
  int n_bti_issues;

  /* Number of reported GCS issues.  */
  int n_gcs_issues;
};

#define elf_aarch64_tdata(bfd)				\
  ((struct elf_aarch64_obj_tdata *) (bfd)->tdata.any)

/* An enum to define what kind of erratum fixes we should apply.  This gives the
   user a bit more control over the sequences we generate.  */
typedef enum
{
  ERRAT_NONE  = (1 << 0),  /* No erratum workarounds allowed.  */
  ERRAT_ADR   = (1 << 1),  /* Erratum workarounds using ADR allowed.  */
  ERRAT_ADRP  = (1 << 2),  /* Erratum workarounds using ADRP are allowed.  */
} erratum_84319_opts;

extern void bfd_elf64_aarch64_set_options
  (bfd *, struct bfd_link_info *, int, int, int, int, erratum_84319_opts, int,
   const aarch64_protection_opts *);

extern void bfd_elf32_aarch64_set_options
  (bfd *, struct bfd_link_info *, int, int, int, int, erratum_84319_opts, int,
   const aarch64_protection_opts *);

/* AArch64 stub generation support for ELF64.  Called from the linker.  */
extern int elf64_aarch64_setup_section_lists
  (bfd *, struct bfd_link_info *);
extern void elf64_aarch64_next_input_section
  (struct bfd_link_info *, struct bfd_section *);
extern bool elf64_aarch64_size_stubs
  (bfd *, bfd *, struct bfd_link_info *, bfd_signed_vma,
   struct bfd_section * (*) (const char *, struct bfd_section *),
   void (*) (void));
extern bool elf64_aarch64_build_stubs
  (struct bfd_link_info *);
/* AArch64 stub generation support for ELF32.  Called from the linker.  */
extern int elf32_aarch64_setup_section_lists
  (bfd *, struct bfd_link_info *);
extern void elf32_aarch64_next_input_section
  (struct bfd_link_info *, struct bfd_section *);
extern bool elf32_aarch64_size_stubs
  (bfd *, bfd *, struct bfd_link_info *, bfd_signed_vma,
   struct bfd_section * (*) (const char *, struct bfd_section *),
   void (*) (void));
extern bool elf32_aarch64_build_stubs
  (struct bfd_link_info *);

/* AArch64 relative relocation packing support for ELF64.  */
extern bool elf64_aarch64_size_relative_relocs
  (struct bfd_link_info *, bool *);
extern bool elf64_aarch64_finish_relative_relocs
  (struct bfd_link_info *);
/* AArch64 relative relocation packing support for ELF32.  */
extern bool elf32_aarch64_size_relative_relocs
  (struct bfd_link_info *, bool *);
extern bool elf32_aarch64_finish_relative_relocs
  (struct bfd_link_info *);

/* Take the PAGE component of an address or offset.  */
#define PG(x)	     ((x) & ~ (bfd_vma) 0xfff)
#define PG_OFFSET(x) ((x) &   (bfd_vma) 0xfff)

#define AARCH64_ADR_OP		0x10000000
#define AARCH64_ADRP_OP		0x90000000
#define AARCH64_ADRP_OP_MASK	0x9F000000

extern bfd_signed_vma
_bfd_aarch64_sign_extend (bfd_vma, int);

extern uint32_t
_bfd_aarch64_decode_adrp_imm (uint32_t);

extern uint32_t
_bfd_aarch64_reencode_adr_imm (uint32_t, uint32_t);

extern bfd_reloc_status_type
_bfd_aarch64_elf_put_addend (bfd *, bfd_byte *, bfd_reloc_code_real_type,
			     reloc_howto_type *, bfd_signed_vma);

extern bfd_vma
_bfd_aarch64_elf_resolve_relocation (bfd *, bfd_reloc_code_real_type, bfd_vma,
				     bfd_vma, bfd_vma, bool);

extern bool
_bfd_aarch64_elf_grok_prstatus (bfd *, Elf_Internal_Note *);

extern bool
_bfd_aarch64_elf_grok_psinfo (bfd *, Elf_Internal_Note *);

extern char *
_bfd_aarch64_elf_write_core_note (bfd *, char *, int *, int, ...);

#define elf_backend_grok_prstatus	_bfd_aarch64_elf_grok_prstatus
#define elf_backend_grok_psinfo		_bfd_aarch64_elf_grok_psinfo
#define elf_backend_write_core_note	_bfd_aarch64_elf_write_core_note

extern bfd *
_bfd_aarch64_elf_link_setup_gnu_properties (struct bfd_link_info *);

extern enum elf_property_kind
_bfd_aarch64_elf_parse_gnu_properties (bfd *, unsigned int,
				       bfd_byte *, unsigned int);

extern bool
_bfd_aarch64_elf_merge_gnu_properties (struct bfd_link_info *, bfd *,
				       elf_property *, elf_property *,
				       uint32_t);

extern void
_bfd_aarch64_elf_check_bti_report (struct bfd_link_info *, bfd *);

extern void
_bfd_aarch64_elf_check_gcs_report (struct bfd_link_info *, bfd *);

extern void
_bfd_aarch64_elf_link_fixup_gnu_properties (struct bfd_link_info *,
					    elf_property_list **);

#define elf_backend_parse_gnu_properties	\
  _bfd_aarch64_elf_parse_gnu_properties

#define elf_backend_fixup_gnu_properties	\
  _bfd_aarch64_elf_link_fixup_gnu_properties
