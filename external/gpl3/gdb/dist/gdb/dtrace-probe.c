/* DTrace probe support for GDB.

   Copyright (C) 2014-2017 Free Software Foundation, Inc.

   Contributed by Oracle, Inc.

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

#include "defs.h"
#include "probe.h"
#include "vec.h"
#include "elf-bfd.h"
#include "gdbtypes.h"
#include "obstack.h"
#include "objfiles.h"
#include "complaints.h"
#include "value.h"
#include "ax.h"
#include "ax-gdb.h"
#include "language.h"
#include "parser-defs.h"
#include "inferior.h"

/* The type of the ELF sections where we will find the DOF programs
   with information about probes.  */

#ifndef SHT_SUNW_dof
# define SHT_SUNW_dof	0x6ffffff4
#endif

/* Forward declaration.  */

extern const struct probe_ops dtrace_probe_ops;

/* The following structure represents a single argument for the
   probe.  */

struct dtrace_probe_arg
{
  /* The type of the probe argument.  */
  struct type *type;

  /* A string describing the type.  */
  char *type_str;

  /* The argument converted to an internal GDB expression.  */
  struct expression *expr;
};

typedef struct dtrace_probe_arg dtrace_probe_arg_s;
DEF_VEC_O (dtrace_probe_arg_s);

/* The following structure represents an enabler for a probe.  */

struct dtrace_probe_enabler
{
  /* Program counter where the is-enabled probe is installed.  The
     contents (nops, whatever...) stored at this address are
     architecture dependent.  */
  CORE_ADDR address;
};

typedef struct dtrace_probe_enabler dtrace_probe_enabler_s;
DEF_VEC_O (dtrace_probe_enabler_s);

/* The following structure represents a dtrace probe.  */

struct dtrace_probe
{
  /* Generic information about the probe.  This must be the first
     element of this struct, in order to maintain binary compatibility
     with the `struct probe' and be able to fully abstract it.  */
  struct probe p;

  /* A probe can have zero or more arguments.  */
  int probe_argc;
  VEC (dtrace_probe_arg_s) *args;

  /* A probe can have zero or more "enablers" associated with it.  */
  VEC (dtrace_probe_enabler_s) *enablers;

  /* Whether the expressions for the arguments have been built.  */
  unsigned int args_expr_built : 1;
};

/* Implementation of the probe_is_linespec method.  */

static int
dtrace_probe_is_linespec (const char **linespecp)
{
  static const char *const keywords[] = { "-pdtrace", "-probe-dtrace", NULL };

  return probe_is_linespec_by_keyword (linespecp, keywords);
}

/* DOF programs can contain an arbitrary number of sections of 26
   different types.  In order to support DTrace USDT probes we only
   need to handle a subset of these section types, fortunately.  These
   section types are defined in the following enumeration.

   See linux/dtrace/dof_defines.h for a complete list of section types
   along with their values.  */

enum dtrace_dof_sect_type
{
  /* Null section.  */
  DTRACE_DOF_SECT_TYPE_NONE = 0,
  /* A dof_ecbdesc_t. */
  DTRACE_DOF_SECT_TYPE_ECBDESC = 3,
  /* A string table.  */
  DTRACE_DOF_SECT_TYPE_STRTAB = 8,
  /* A dof_provider_t  */
  DTRACE_DOF_SECT_TYPE_PROVIDER = 15,
  /* Array of dof_probe_t  */
  DTRACE_DOF_SECT_TYPE_PROBES = 16,
  /* An array of probe arg mappings.  */
  DTRACE_DOF_SECT_TYPE_PRARGS = 17,
  /* An array of probe arg offsets.  */
  DTRACE_DOF_SECT_TYPE_PROFFS = 18,
  /* An array of probe is-enabled offsets.  */
  DTRACE_DOF_SECT_TYPE_PRENOFFS = 26
};

/* The following collection of data structures map the structure of
   DOF entities.  Again, we only cover the subset of DOF used to
   implement USDT probes.

   See linux/dtrace/dof.h header for a complete list of data
   structures.  */

/* Offsets to index the dofh_ident[] array defined below.  */

enum dtrace_dof_ident
{
  /* First byte of the magic number.  */
  DTRACE_DOF_ID_MAG0 = 0,
  /* Second byte of the magic number.  */
  DTRACE_DOF_ID_MAG1 = 1,
  /* Third byte of the magic number.  */
  DTRACE_DOF_ID_MAG2 = 2,
  /* Fourth byte of the magic number.  */
  DTRACE_DOF_ID_MAG3 = 3,
  /* An enum_dof_encoding value.  */
  DTRACE_DOF_ID_ENCODING = 5
};

/* Possible values for dofh_ident[DOF_ID_ENCODING].  */

enum dtrace_dof_encoding
{
  /* The DOF program is little-endian.  */
  DTRACE_DOF_ENCODE_LSB = 1,
  /* The DOF program is big-endian.  */
  DTRACE_DOF_ENCODE_MSB = 2
};

/* A DOF header, which describes the contents of a DOF program: number
   of sections, size, etc.  */

struct dtrace_dof_hdr
{
  /* Identification bytes (see above). */
  uint8_t dofh_ident[16];
  /* File attribute flags (if any). */
  uint32_t dofh_flags;   
  /* Size of file header in bytes. */
  uint32_t dofh_hdrsize; 
  /* Size of section header in bytes. */
  uint32_t dofh_secsize; 
  /* Number of section headers. */
  uint32_t dofh_secnum;  
  /* File offset of section headers. */
  uint64_t dofh_secoff;  
  /* File size of loadable portion. */
  uint64_t dofh_loadsz;  
  /* File size of entire DOF file. */
  uint64_t dofh_filesz;  
  /* Reserved for future use. */
  uint64_t dofh_pad;     
};

/* A DOF section, whose contents depend on its type.  The several
   supported section types are described in the enum
   dtrace_dof_sect_type above.  */

struct dtrace_dof_sect
{
  /* Section type (see the define above). */
  uint32_t dofs_type;
  /* Section data memory alignment. */
  uint32_t dofs_align; 
  /* Section flags (if any). */
  uint32_t dofs_flags; 
  /* Size of section entry (if table). */
  uint32_t dofs_entsize;
  /* DOF + offset points to the section data. */
  uint64_t dofs_offset;
  /* Size of section data in bytes.  */
  uint64_t dofs_size;  
};

/* A DOF provider, which is the provider of a probe.  */

struct dtrace_dof_provider
{
  /* Link to a DTRACE_DOF_SECT_TYPE_STRTAB section. */
  uint32_t dofpv_strtab; 
  /* Link to a DTRACE_DOF_SECT_TYPE_PROBES section. */
  uint32_t dofpv_probes; 
  /* Link to a DTRACE_DOF_SECT_TYPE_PRARGS section. */
  uint32_t dofpv_prargs; 
  /* Link to a DTRACE_DOF_SECT_TYPE_PROFFS section. */
  uint32_t dofpv_proffs; 
  /* Provider name string. */
  uint32_t dofpv_name;   
  /* Provider attributes. */
  uint32_t dofpv_provattr;
  /* Module attributes. */
  uint32_t dofpv_modattr; 
  /* Function attributes. */
  uint32_t dofpv_funcattr;
  /* Name attributes. */
  uint32_t dofpv_nameattr;
  /* Args attributes. */
  uint32_t dofpv_argsattr;
  /* Link to a DTRACE_DOF_SECT_PRENOFFS section. */
  uint32_t dofpv_prenoffs;
};

/* A set of DOF probes and is-enabled probes sharing a base address
   and several attributes.  The particular locations and attributes of
   each probe are maintained in arrays in several other DOF sections.
   See the comment in dtrace_process_dof_probe for details on how
   these attributes are stored.  */

struct dtrace_dof_probe
{
  /* Probe base address or offset. */
  uint64_t dofpr_addr;   
  /* Probe function string. */
  uint32_t dofpr_func;   
  /* Probe name string. */
  uint32_t dofpr_name;   
  /* Native argument type strings. */
  uint32_t dofpr_nargv;  
  /* Translated argument type strings. */
  uint32_t dofpr_xargv;  
  /* Index of first argument mapping. */
  uint32_t dofpr_argidx; 
  /* Index of first offset entry. */
  uint32_t dofpr_offidx; 
  /* Native argument count. */
  uint8_t  dofpr_nargc;  
  /* Translated argument count. */
  uint8_t  dofpr_xargc;  
  /* Number of offset entries for probe. */
  uint16_t dofpr_noffs;  
  /* Index of first is-enabled offset. */
  uint32_t dofpr_enoffidx;
  /* Number of is-enabled offsets. */
  uint16_t dofpr_nenoffs;
  /* Reserved for future use. */
  uint16_t dofpr_pad1;   
  /* Reserved for future use. */
  uint32_t dofpr_pad2;   
};

/* DOF supports two different encodings: MSB (big-endian) and LSB
   (little-endian).  The encoding is itself encoded in the DOF header.
   The following function returns an unsigned value in the host
   endianness.  */

#define DOF_UINT(dof, field)						\
  extract_unsigned_integer ((gdb_byte *) &(field),			\
			    sizeof ((field)),				\
			    (((dof)->dofh_ident[DTRACE_DOF_ID_ENCODING] \
			      == DTRACE_DOF_ENCODE_MSB)			\
			     ? BFD_ENDIAN_BIG : BFD_ENDIAN_LITTLE))

/* The following macro applies a given byte offset to a DOF (a pointer
   to a dtrace_dof_hdr structure) and returns the resulting
   address.  */

#define DTRACE_DOF_PTR(dof, offset) (&((char *) (dof))[(offset)])

/* The following macro returns a pointer to the beginning of a given
   section in a DOF object.  The section is referred to by its index
   in the sections array.  */

#define DTRACE_DOF_SECT(dof, idx)					\
  ((struct dtrace_dof_sect *)						\
   DTRACE_DOF_PTR ((dof),						\
		   DOF_UINT ((dof), (dof)->dofh_secoff)			\
		   + ((idx) * DOF_UINT ((dof), (dof)->dofh_secsize))))

/* Helper function to examine the probe described by the given PROBE
   and PROVIDER data structures and add it to the PROBESP vector.
   STRTAB, OFFTAB, EOFFTAB and ARGTAB are pointers to tables in the
   DOF program containing the attributes for the probe.  */

static void
dtrace_process_dof_probe (struct objfile *objfile,
			  struct gdbarch *gdbarch, VEC (probe_p) **probesp,
			  struct dtrace_dof_hdr *dof,
			  struct dtrace_dof_probe *probe,
			  struct dtrace_dof_provider *provider,
			  char *strtab, char *offtab, char *eofftab,
			  char *argtab, uint64_t strtab_size)
{
  int i, j, num_probes, num_enablers;
  struct cleanup *cleanup;
  VEC (dtrace_probe_enabler_s) *enablers;
  char *p;

  /* Each probe section can define zero or more probes of two
     different types:

     - probe->dofpr_noffs regular probes whose program counters are
       stored in 32bit words starting at probe->dofpr_addr +
       offtab[probe->dofpr_offidx].

     - probe->dofpr_nenoffs is-enabled probes whose program counters
       are stored in 32bit words starting at probe->dofpr_addr +
       eofftab[probe->dofpr_enoffidx].

     However is-enabled probes are not probes per-se, but an
     optimization hack that is implemented in the kernel in a very
     similar way than normal probes.  This is how we support
     is-enabled probes on GDB:

     - Our probes are always DTrace regular probes.

     - Our probes can be associated with zero or more "enablers".  The
       list of enablers is built from the is-enabled probes defined in
       the Probe section.

     - Probes having a non-empty list of enablers can be enabled or
       disabled using the `enable probe' and `disable probe' commands
       respectively.  The `Enabled' column in the output of `info
       probes' will read `yes' if the enablers are activated, `no'
       otherwise.

     - Probes having an empty list of enablers are always enabled.
       The `Enabled' column in the output of `info probes' will
       read `always'.

     It follows that if there are DTrace is-enabled probes defined for
     some provider/name but no DTrace regular probes defined then the
     GDB user wont be able to enable/disable these conditionals.  */

  num_probes = DOF_UINT (dof, probe->dofpr_noffs);
  if (num_probes == 0)
    return;

  /* Build the list of enablers for the probes defined in this Probe
     DOF section.  */
  enablers = NULL;
  cleanup
    = make_cleanup (VEC_cleanup (dtrace_probe_enabler_s), &enablers);
  num_enablers = DOF_UINT (dof, probe->dofpr_nenoffs);
  for (i = 0; i < num_enablers; i++)
    {
      struct dtrace_probe_enabler enabler;
      uint32_t enabler_offset
	= ((uint32_t *) eofftab)[DOF_UINT (dof, probe->dofpr_enoffidx) + i];

      enabler.address = DOF_UINT (dof, probe->dofpr_addr)
	+ DOF_UINT (dof, enabler_offset);
      VEC_safe_push (dtrace_probe_enabler_s, enablers, &enabler);
    }

  for (i = 0; i < num_probes; i++)
    {
      uint32_t probe_offset
	= ((uint32_t *) offtab)[DOF_UINT (dof, probe->dofpr_offidx) + i];
      struct dtrace_probe *ret =
	XOBNEW (&objfile->per_bfd->storage_obstack, struct dtrace_probe);

      ret->p.pops = &dtrace_probe_ops;
      ret->p.arch = gdbarch;
      ret->args_expr_built = 0;

      /* Set the provider and the name of the probe.  */
      ret->p.provider
	= xstrdup (strtab + DOF_UINT (dof, provider->dofpv_name));
      ret->p.name = xstrdup (strtab + DOF_UINT (dof, probe->dofpr_name));

      /* The probe address.  */
      ret->p.address
	= DOF_UINT (dof, probe->dofpr_addr) + DOF_UINT (dof, probe_offset);

      /* Number of arguments in the probe.  */
      ret->probe_argc = DOF_UINT (dof, probe->dofpr_nargc);

      /* Store argument type descriptions.  A description of the type
         of the argument is in the (J+1)th null-terminated string
         starting at 'strtab' + 'probe->dofpr_nargv'.  */
      ret->args = NULL;
      p = strtab + DOF_UINT (dof, probe->dofpr_nargv);
      for (j = 0; j < ret->probe_argc; j++)
	{
	  struct dtrace_probe_arg arg;
	  expression_up expr;

	  /* Set arg.expr to ensure all fields in expr are initialized and
	     the compiler will not warn when arg is used.  */
	  arg.expr = NULL;
	  arg.type_str = xstrdup (p);

	  /* Use strtab_size as a sentinel.  */
	  while (*p != '\0' && p - strtab < strtab_size)
	    ++p;

	  /* Try to parse a type expression from the type string.  If
	     this does not work then we set the type to `long
	     int'.  */
          arg.type = builtin_type (gdbarch)->builtin_long;

	  TRY
	    {
	      expr = parse_expression_with_language (arg.type_str, language_c);
	    }
	  CATCH (ex, RETURN_MASK_ERROR)
	    {
	    }
	  END_CATCH

	  if (expr != NULL && expr->elts[0].opcode == OP_TYPE)
	    arg.type = expr->elts[1].type;

	  VEC_safe_push (dtrace_probe_arg_s, ret->args, &arg);
	}

      /* Add the vector of enablers to this probe, if any.  */
      ret->enablers = VEC_copy (dtrace_probe_enabler_s, enablers);

      /* Successfully created probe.  */
      VEC_safe_push (probe_p, *probesp, (struct probe *) ret);
    }

  do_cleanups (cleanup);
}

/* Helper function to collect the probes described in the DOF program
   whose header is pointed by DOF and add them to the PROBESP vector.
   SECT is the ELF section containing the DOF program and OBJFILE is
   its containing object file.  */

static void
dtrace_process_dof (asection *sect, struct objfile *objfile,
		    VEC (probe_p) **probesp, struct dtrace_dof_hdr *dof)
{
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  struct dtrace_dof_sect *section;
  int i;

  /* The first step is to check for the DOF magic number.  If no valid
     DOF data is found in the section then a complaint is issued to
     the user and the section skipped.  */
  if (dof->dofh_ident[DTRACE_DOF_ID_MAG0] != 0x7F
      || dof->dofh_ident[DTRACE_DOF_ID_MAG1] != 'D'
      || dof->dofh_ident[DTRACE_DOF_ID_MAG2] != 'O'
      || dof->dofh_ident[DTRACE_DOF_ID_MAG3] != 'F')
    goto invalid_dof_data;

  /* Make sure the encoding mark is either DTRACE_DOF_ENCODE_LSB or
     DTRACE_DOF_ENCODE_MSB.  */
  if (dof->dofh_ident[DTRACE_DOF_ID_ENCODING] != DTRACE_DOF_ENCODE_LSB
      && dof->dofh_ident[DTRACE_DOF_ID_ENCODING] != DTRACE_DOF_ENCODE_MSB)
    goto invalid_dof_data;

  /* Make sure this DOF is not an enabling DOF, i.e. there are no ECB
     Description sections.  */
  section = (struct dtrace_dof_sect *) DTRACE_DOF_PTR (dof,
						       DOF_UINT (dof, dof->dofh_secoff));
  for (i = 0; i < DOF_UINT (dof, dof->dofh_secnum); i++, section++)
    if (section->dofs_type == DTRACE_DOF_SECT_TYPE_ECBDESC)
      return;

  /* Iterate over any section of type Provider and extract the probe
     information from them.  If there are no "provider" sections on
     the DOF then we just return.  */
  section = (struct dtrace_dof_sect *) DTRACE_DOF_PTR (dof,
						       DOF_UINT (dof, dof->dofh_secoff));
  for (i = 0; i < DOF_UINT (dof, dof->dofh_secnum); i++, section++)
    if (DOF_UINT (dof, section->dofs_type) == DTRACE_DOF_SECT_TYPE_PROVIDER)
      {
	struct dtrace_dof_provider *provider = (struct dtrace_dof_provider *)
	  DTRACE_DOF_PTR (dof, DOF_UINT (dof, section->dofs_offset));
	struct dtrace_dof_sect *strtab_s
	  = DTRACE_DOF_SECT (dof, DOF_UINT (dof, provider->dofpv_strtab));
	struct dtrace_dof_sect *probes_s
	  = DTRACE_DOF_SECT (dof, DOF_UINT (dof, provider->dofpv_probes));
	struct dtrace_dof_sect *args_s
	  = DTRACE_DOF_SECT (dof, DOF_UINT (dof, provider->dofpv_prargs));
	struct dtrace_dof_sect *offsets_s
	  = DTRACE_DOF_SECT (dof, DOF_UINT (dof, provider->dofpv_proffs));
	struct dtrace_dof_sect *eoffsets_s
	  = DTRACE_DOF_SECT (dof, DOF_UINT (dof, provider->dofpv_prenoffs));
	char *strtab  = DTRACE_DOF_PTR (dof, DOF_UINT (dof, strtab_s->dofs_offset));
	char *offtab  = DTRACE_DOF_PTR (dof, DOF_UINT (dof, offsets_s->dofs_offset));
	char *eofftab = DTRACE_DOF_PTR (dof, DOF_UINT (dof, eoffsets_s->dofs_offset));
	char *argtab  = DTRACE_DOF_PTR (dof, DOF_UINT (dof, args_s->dofs_offset));
	unsigned int entsize = DOF_UINT (dof, probes_s->dofs_entsize);
	int num_probes;

	if (DOF_UINT (dof, section->dofs_size)
	    < sizeof (struct dtrace_dof_provider))
	  {
	    /* The section is smaller than expected, so do not use it.
	       This has been observed on x86-solaris 10.  */
	    goto invalid_dof_data;
	  }

	/* Very, unlikely, but could crash gdb if not handled
	   properly.  */
	if (entsize == 0)
	  goto invalid_dof_data;

	num_probes = DOF_UINT (dof, probes_s->dofs_size) / entsize;

	for (i = 0; i < num_probes; i++)
	  {
	    struct dtrace_dof_probe *probe = (struct dtrace_dof_probe *)
	      DTRACE_DOF_PTR (dof, DOF_UINT (dof, probes_s->dofs_offset)
			      + (i * DOF_UINT (dof, probes_s->dofs_entsize)));

	    dtrace_process_dof_probe (objfile,
				      gdbarch, probesp,
				      dof, probe,
				      provider, strtab, offtab, eofftab, argtab,
				      DOF_UINT (dof, strtab_s->dofs_size));
	  }
      }

  return;
	  
 invalid_dof_data:
  complaint (&symfile_complaints,
	     _("skipping section '%s' which does not contain valid DOF data."),
	     sect->name);
}

/* Helper function to build the GDB internal expressiosn that, once
   evaluated, will calculate the values of the arguments of a given
   PROBE.  */

static void
dtrace_build_arg_exprs (struct dtrace_probe *probe,
			struct gdbarch *gdbarch)
{
  struct parser_state pstate;
  struct dtrace_probe_arg *arg;
  int i;

  probe->args_expr_built = 1;

  /* Iterate over the arguments in the probe and build the
     corresponding GDB internal expression that will generate the
     value of the argument when executed at the PC of the probe.  */
  for (i = 0; i < probe->probe_argc; i++)
    {
      struct cleanup *back_to;

      arg = VEC_index (dtrace_probe_arg_s, probe->args, i);

      /* Initialize the expression buffer in the parser state.  The
	 language does not matter, since we are using our own
	 parser.  */
      initialize_expout (&pstate, 10, current_language, gdbarch);
      back_to = make_cleanup (free_current_contents, &pstate.expout);

      /* The argument value, which is ABI dependent and casted to
	 `long int'.  */
      gdbarch_dtrace_parse_probe_argument (gdbarch, &pstate, i);

      discard_cleanups (back_to);

      /* Casting to the expected type, but only if the type was
	 recognized at probe load time.  Otherwise the argument will
	 be evaluated as the long integer passed to the probe.  */
      if (arg->type != NULL)
	{
	  write_exp_elt_opcode (&pstate, UNOP_CAST);
	  write_exp_elt_type (&pstate, arg->type);
	  write_exp_elt_opcode (&pstate, UNOP_CAST);
	}

      reallocate_expout (&pstate);
      arg->expr = pstate.expout;
      prefixify_expression (arg->expr);
    }
}

/* Helper function to return the Nth argument of a given PROBE.  */

static struct dtrace_probe_arg *
dtrace_get_arg (struct dtrace_probe *probe, unsigned n,
		struct gdbarch *gdbarch)
{
  if (!probe->args_expr_built)
    dtrace_build_arg_exprs (probe, gdbarch);

  return VEC_index (dtrace_probe_arg_s, probe->args, n);
}

/* Implementation of the get_probes method.  */

static void
dtrace_get_probes (VEC (probe_p) **probesp, struct objfile *objfile)
{
  bfd *abfd = objfile->obfd;
  asection *sect = NULL;

  /* Do nothing in case this is a .debug file, instead of the objfile
     itself.  */
  if (objfile->separate_debug_objfile_backlink != NULL)
    return;

  /* Iterate over the sections in OBJFILE looking for DTrace
     information.  */
  for (sect = abfd->sections; sect != NULL; sect = sect->next)
    {
      if (elf_section_data (sect)->this_hdr.sh_type == SHT_SUNW_dof)
	{
	  bfd_byte *dof;

	  /* Read the contents of the DOF section and then process it to
	     extract the information of any probe defined into it.  */
	  if (!bfd_malloc_and_get_section (abfd, sect, &dof))
	    complaint (&symfile_complaints,
		       _("could not obtain the contents of"
			 "section '%s' in objfile `%s'."),
		       sect->name, abfd->filename);
      
	  dtrace_process_dof (sect, objfile, probesp,
			      (struct dtrace_dof_hdr *) dof);
	  xfree (dof);
	}
    }
}

/* Helper function to determine whether a given probe is "enabled" or
   "disabled".  A disabled probe is a probe in which one or more
   enablers are disabled.  */

static int
dtrace_probe_is_enabled (struct dtrace_probe *probe)
{
  int i;
  struct gdbarch *gdbarch = probe->p.arch;
  struct dtrace_probe_enabler *enabler;

  for (i = 0;
       VEC_iterate (dtrace_probe_enabler_s, probe->enablers, i, enabler);
       i++)
    if (!gdbarch_dtrace_probe_is_enabled (gdbarch, enabler->address))
      return 0;

  return 1;
}

/* Implementation of the get_probe_address method.  */

static CORE_ADDR
dtrace_get_probe_address (struct probe *probe, struct objfile *objfile)
{
  gdb_assert (probe->pops == &dtrace_probe_ops);
  return probe->address + ANOFFSET (objfile->section_offsets,
				    SECT_OFF_DATA (objfile));
}

/* Implementation of the get_probe_argument_count method.  */

static unsigned
dtrace_get_probe_argument_count (struct probe *probe_generic,
				 struct frame_info *frame)
{
  struct dtrace_probe *dtrace_probe = (struct dtrace_probe *) probe_generic;

  gdb_assert (probe_generic->pops == &dtrace_probe_ops);

  return dtrace_probe->probe_argc;
}

/* Implementation of the can_evaluate_probe_arguments method.  */

static int
dtrace_can_evaluate_probe_arguments (struct probe *probe_generic)
{
  struct gdbarch *gdbarch = probe_generic->arch;

  gdb_assert (probe_generic->pops == &dtrace_probe_ops);
  return gdbarch_dtrace_parse_probe_argument_p (gdbarch);
}

/* Implementation of the evaluate_probe_argument method.  */

static struct value *
dtrace_evaluate_probe_argument (struct probe *probe_generic, unsigned n,
				struct frame_info *frame)
{
  struct gdbarch *gdbarch = probe_generic->arch;
  struct dtrace_probe *dtrace_probe = (struct dtrace_probe *) probe_generic;
  struct dtrace_probe_arg *arg;
  int pos = 0;

  gdb_assert (probe_generic->pops == &dtrace_probe_ops);

  arg = dtrace_get_arg (dtrace_probe, n, gdbarch);
  return evaluate_subexp_standard (arg->type, arg->expr, &pos, EVAL_NORMAL);
}

/* Implementation of the compile_to_ax method.  */

static void
dtrace_compile_to_ax (struct probe *probe_generic, struct agent_expr *expr,
		      struct axs_value *value, unsigned n)
{
  struct dtrace_probe *dtrace_probe = (struct dtrace_probe *) probe_generic;
  struct dtrace_probe_arg *arg;
  union exp_element *pc;

  gdb_assert (probe_generic->pops == &dtrace_probe_ops);

  arg = dtrace_get_arg (dtrace_probe, n, expr->gdbarch);

  pc = arg->expr->elts;
  gen_expr (arg->expr, &pc, expr, value);

  require_rvalue (expr, value);
  value->type = arg->type;
}

/* Implementation of the probe_destroy method.  */

static void
dtrace_probe_destroy (struct probe *probe_generic)
{
  struct dtrace_probe *probe = (struct dtrace_probe *) probe_generic;
  struct dtrace_probe_arg *arg;
  int i;

  gdb_assert (probe_generic->pops == &dtrace_probe_ops);

  for (i = 0; VEC_iterate (dtrace_probe_arg_s, probe->args, i, arg); i++)
    {
      xfree (arg->type_str);
      xfree (arg->expr);
    }

  VEC_free (dtrace_probe_enabler_s, probe->enablers);
  VEC_free (dtrace_probe_arg_s, probe->args);
}

/* Implementation of the type_name method.  */

static const char *
dtrace_type_name (struct probe *probe_generic)
{
  gdb_assert (probe_generic->pops == &dtrace_probe_ops);
  return "dtrace";
}

/* Implementation of the gen_info_probes_table_header method.  */

static void
dtrace_gen_info_probes_table_header (VEC (info_probe_column_s) **heads)
{
  info_probe_column_s dtrace_probe_column;

  dtrace_probe_column.field_name = "enabled";
  dtrace_probe_column.print_name = _("Enabled");

  VEC_safe_push (info_probe_column_s, *heads, &dtrace_probe_column);
}

/* Implementation of the gen_info_probes_table_values method.  */

static void
dtrace_gen_info_probes_table_values (struct probe *probe_generic,
				     VEC (const_char_ptr) **ret)
{
  struct dtrace_probe *probe = (struct dtrace_probe *) probe_generic;
  const char *val = NULL;

  gdb_assert (probe_generic->pops == &dtrace_probe_ops);

  if (VEC_empty (dtrace_probe_enabler_s, probe->enablers))
    val = "always";
  else if (!gdbarch_dtrace_probe_is_enabled_p (probe_generic->arch))
    val = "unknown";
  else if (dtrace_probe_is_enabled (probe))
    val = "yes";
  else
    val = "no";

  VEC_safe_push (const_char_ptr, *ret, val);
}

/* Implementation of the enable_probe method.  */

static void
dtrace_enable_probe (struct probe *probe)
{
  struct gdbarch *gdbarch = probe->arch;
  struct dtrace_probe *dtrace_probe = (struct dtrace_probe *) probe;
  struct dtrace_probe_enabler *enabler;
  int i;

  gdb_assert (probe->pops == &dtrace_probe_ops);

  /* Enabling a dtrace probe implies patching the text section of the
     running process, so make sure the inferior is indeed running.  */
  if (ptid_equal (inferior_ptid, null_ptid))
    error (_("No inferior running"));

  /* Fast path.  */
  if (dtrace_probe_is_enabled (dtrace_probe))
    return;

  /* Iterate over all defined enabler in the given probe and enable
     them all using the corresponding gdbarch hook.  */

  for (i = 0;
       VEC_iterate (dtrace_probe_enabler_s, dtrace_probe->enablers, i, enabler);
       i++)
    if (gdbarch_dtrace_enable_probe_p (gdbarch))
      gdbarch_dtrace_enable_probe (gdbarch, enabler->address);
}


/* Implementation of the disable_probe method.  */

static void
dtrace_disable_probe (struct probe *probe)
{
  struct gdbarch *gdbarch = probe->arch;
  struct dtrace_probe *dtrace_probe = (struct dtrace_probe *) probe;
  struct dtrace_probe_enabler *enabler;
  int i;

  gdb_assert (probe->pops == &dtrace_probe_ops);

  /* Disabling a dtrace probe implies patching the text section of the
     running process, so make sure the inferior is indeed running.  */
  if (ptid_equal (inferior_ptid, null_ptid))
    error (_("No inferior running"));

  /* Fast path.  */
  if (!dtrace_probe_is_enabled (dtrace_probe))
    return;

  /* Are we trying to disable a probe that does not have any enabler
     associated?  */
  if (VEC_empty (dtrace_probe_enabler_s, dtrace_probe->enablers))
    error (_("Probe %s:%s cannot be disabled: no enablers."), probe->provider, probe->name);

  /* Iterate over all defined enabler in the given probe and disable
     them all using the corresponding gdbarch hook.  */

  for (i = 0;
       VEC_iterate (dtrace_probe_enabler_s, dtrace_probe->enablers, i, enabler);
       i++)
    if (gdbarch_dtrace_disable_probe_p (gdbarch))
      gdbarch_dtrace_disable_probe (gdbarch, enabler->address);
}

/* DTrace probe_ops.  */

const struct probe_ops dtrace_probe_ops =
{
  dtrace_probe_is_linespec,
  dtrace_get_probes,
  dtrace_get_probe_address,
  dtrace_get_probe_argument_count,
  dtrace_can_evaluate_probe_arguments,
  dtrace_evaluate_probe_argument,
  dtrace_compile_to_ax,
  NULL, /* set_semaphore  */
  NULL, /* clear_semaphore  */
  dtrace_probe_destroy,
  dtrace_type_name,
  dtrace_gen_info_probes_table_header,
  dtrace_gen_info_probes_table_values,
  dtrace_enable_probe,
  dtrace_disable_probe
};

/* Implementation of the `info probes dtrace' command.  */

static void
info_probes_dtrace_command (char *arg, int from_tty)
{
  info_probes_for_ops (arg, from_tty, &dtrace_probe_ops);
}

void _initialize_dtrace_probe (void);

void
_initialize_dtrace_probe (void)
{
  VEC_safe_push (probe_ops_cp, all_probe_ops, &dtrace_probe_ops);

  add_cmd ("dtrace", class_info, info_probes_dtrace_command,
	   _("\
Show information about DTrace static probes.\n\
Usage: info probes dtrace [PROVIDER [NAME [OBJECT]]]\n\
Each argument is a regular expression, used to select probes.\n\
PROVIDER matches probe provider names.\n\
NAME matches the probe names.\n\
OBJECT matches the executable or shared library name."),
	   info_probes_cmdlist_get ());
}
