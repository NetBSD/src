# This shell script emits a C file. -*- C -*-
# It does some substitutions.
if [ -z "$MACHINE" ]; then
  OUTPUT_ARCH=${ARCH}
else
  OUTPUT_ARCH=${ARCH}:${MACHINE}
fi
rm -f e${EMULATION_NAME}.c
(echo;echo;echo;echo;echo)>e${EMULATION_NAME}.c # there, now line numbers match ;-)
fragment <<EOF
/* Copyright (C) 1995-2016 Free Software Foundation, Inc.

   This file is part of the GNU Binutils.

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


/* For WINDOWS_NT */
/* The original file generated returned different default scripts depending
   on whether certain switches were set, but these switches pertain to the
   Linux system and that particular version of coff.  In the NT case, we
   only determine if the subsystem is console or windows in order to select
   the correct entry point by default. */

#define TARGET_IS_${EMULATION_NAME}

/* Do this before including bfd.h, so we prototype the right functions.  */

#if defined(TARGET_IS_armpe) \
    || defined(TARGET_IS_arm_epoc_pe) \
    || defined(TARGET_IS_arm_wince_pe)
#define bfd_arm_allocate_interworking_sections \
	bfd_${EMULATION_NAME}_allocate_interworking_sections
#define bfd_arm_get_bfd_for_interworking \
	bfd_${EMULATION_NAME}_get_bfd_for_interworking
#define bfd_arm_process_before_allocation \
	bfd_${EMULATION_NAME}_process_before_allocation
#endif

#include "sysdep.h"
#include "bfd.h"
#include "bfdlink.h"
#include "getopt.h"
#include "libiberty.h"
#include "filenames.h"
#include "ld.h"
#include "ldmain.h"
#include "ldexp.h"
#include "ldlang.h"
#include "ldfile.h"
#include "ldemul.h"
#include <ldgram.h>
#include "ldlex.h"
#include "ldmisc.h"
#include "ldctor.h"
#include "ldbuildid.h"
#include "coff/internal.h"

/* FIXME: See bfd/peXXigen.c for why we include an architecture specific
   header in generic PE code.  */
#include "coff/i386.h"
#include "coff/pe.h"

/* FIXME: These are BFD internal header files, and we should not be
   using it here.  */
#include "../bfd/libcoff.h"
#include "../bfd/libpei.h"

#include "deffile.h"
#include "pe-dll.h"
#include "safe-ctype.h"

/* Permit the emulation parameters to override the default section
   alignment by setting OVERRIDE_SECTION_ALIGNMENT.  FIXME: This makes
   it seem that include/coff/internal.h should not define
   PE_DEF_SECTION_ALIGNMENT.  */
#if PE_DEF_SECTION_ALIGNMENT != ${OVERRIDE_SECTION_ALIGNMENT:-PE_DEF_SECTION_ALIGNMENT}
#undef PE_DEF_SECTION_ALIGNMENT
#define PE_DEF_SECTION_ALIGNMENT ${OVERRIDE_SECTION_ALIGNMENT}
#endif

#if defined(TARGET_IS_i386pe) \
    || defined(TARGET_IS_shpe) \
    || defined(TARGET_IS_armpe) \
    || defined(TARGET_IS_arm_epoc_pe) \
    || defined(TARGET_IS_arm_wince_pe)
#define DLL_SUPPORT
#endif

#if defined(TARGET_IS_i386pe)
#define DEFAULT_PSEUDO_RELOC_VERSION 2
#else
#define DEFAULT_PSEUDO_RELOC_VERSION 1
#endif

#if defined(TARGET_IS_i386pe) || ! defined(DLL_SUPPORT)
#define	PE_DEF_SUBSYSTEM		3
#else
#undef NT_EXE_IMAGE_BASE
#undef PE_DEF_SECTION_ALIGNMENT
#undef PE_DEF_FILE_ALIGNMENT
#define NT_EXE_IMAGE_BASE		0x00010000

#if defined(TARGET_IS_armpe) || defined(TARGET_IS_arm_wince_pe)
#define PE_DEF_SECTION_ALIGNMENT	0x00001000
#define	PE_DEF_SUBSYSTEM		9
#else
#define PE_DEF_SECTION_ALIGNMENT	0x00000400
#define	PE_DEF_SUBSYSTEM		2
#endif
#define PE_DEF_FILE_ALIGNMENT		0x00000200
#endif

static struct internal_extra_pe_aouthdr pe;
static int dll;
static int pe_subsystem = ${SUBSYSTEM};
static flagword real_flags = 0;
static int support_old_code = 0;
static char * thumb_entry_symbol = NULL;
static lang_assignment_statement_type *image_base_statement = 0;
static unsigned short pe_dll_characteristics = 0;
static bfd_boolean insert_timestamp = TRUE;
static const char *emit_build_id;

#ifdef DLL_SUPPORT
static int pe_enable_stdcall_fixup = -1; /* 0=disable 1=enable.  */
static char *pe_out_def_filename = NULL;
static char *pe_implib_filename = NULL;
static int pe_enable_auto_image_base = 0;
static unsigned long pe_auto_image_base = 0x61500000;
static char *pe_dll_search_prefix = NULL;
#endif

extern const char *output_filename;

static int is_underscoring (void)
{
  int u = 0;
  if (pe_leading_underscore != -1)
    return pe_leading_underscore;
  if (!bfd_get_target_info ("${OUTPUT_FORMAT}", NULL, NULL, &u, NULL))
    bfd_get_target_info ("${RELOCATEABLE_OUTPUT_FORMAT}", NULL, NULL, &u, NULL);

  if (u == -1)
    abort ();
  pe_leading_underscore = (u != 0 ? 1 : 0);
  return pe_leading_underscore;
}

static void
gld_${EMULATION_NAME}_before_parse (void)
{
  is_underscoring ();
  ldfile_set_output_arch ("${OUTPUT_ARCH}", bfd_arch_`echo ${ARCH} | sed -e 's/:.*//'`);
  output_filename = "${EXECUTABLE_NAME:-a.exe}";
#ifdef DLL_SUPPORT
  input_flags.dynamic = TRUE;
  config.has_shared = 1;
EOF

# Cygwin no longer wants these noisy warnings.  Other PE
# targets might like to consider adding themselves here.
# See also the mail thread starting here for the reason why
# merge_rdata defaults to 0 for cygwin:
#  http://cygwin.com/ml/cygwin-apps/2013-04/msg00187.html
case ${target} in
  *-*-cygwin*)
    default_auto_import=1
    default_merge_rdata=0
    ;;
  i[3-7]86-*-mingw* | x86_64-*-mingw*)
    default_auto_import=1
    default_merge_rdata=0
    ;;
  *)
    default_auto_import=-1
    default_merge_rdata=1
    ;;
esac

fragment <<EOF
  link_info.pei386_auto_import = ${default_auto_import};
  /* Use by default version.  */
  link_info.pei386_runtime_pseudo_reloc = DEFAULT_PSEUDO_RELOC_VERSION;
#endif
}

/* Indicates if RDATA shall be merged into DATA when pseudo-relocation
   version 2 is used and auto-import is enabled.  */
#define MERGE_RDATA_V2 ${default_merge_rdata}

/* PE format extra command line options.  */

/* Used for setting flags in the PE header.  */
#define OPTION_BASE_FILE		(300  + 1)
#define OPTION_DLL			(OPTION_BASE_FILE + 1)
#define OPTION_FILE_ALIGNMENT		(OPTION_DLL + 1)
#define OPTION_IMAGE_BASE		(OPTION_FILE_ALIGNMENT + 1)
#define OPTION_MAJOR_IMAGE_VERSION	(OPTION_IMAGE_BASE + 1)
#define OPTION_MAJOR_OS_VERSION		(OPTION_MAJOR_IMAGE_VERSION + 1)
#define OPTION_MAJOR_SUBSYSTEM_VERSION	(OPTION_MAJOR_OS_VERSION + 1)
#define OPTION_MINOR_IMAGE_VERSION	(OPTION_MAJOR_SUBSYSTEM_VERSION + 1)
#define OPTION_MINOR_OS_VERSION		(OPTION_MINOR_IMAGE_VERSION + 1)
#define OPTION_MINOR_SUBSYSTEM_VERSION	(OPTION_MINOR_OS_VERSION + 1)
#define OPTION_SECTION_ALIGNMENT	(OPTION_MINOR_SUBSYSTEM_VERSION + 1)
#define OPTION_STACK			(OPTION_SECTION_ALIGNMENT + 1)
#define OPTION_SUBSYSTEM		(OPTION_STACK + 1)
#define OPTION_HEAP			(OPTION_SUBSYSTEM + 1)
#define OPTION_SUPPORT_OLD_CODE		(OPTION_HEAP + 1)
#define OPTION_OUT_DEF			(OPTION_SUPPORT_OLD_CODE + 1)
#define OPTION_EXPORT_ALL		(OPTION_OUT_DEF + 1)
#define OPTION_EXCLUDE_SYMBOLS		(OPTION_EXPORT_ALL + 1)
#define OPTION_EXCLUDE_ALL_SYMBOLS	(OPTION_EXCLUDE_SYMBOLS + 1)
#define OPTION_KILL_ATS			(OPTION_EXCLUDE_ALL_SYMBOLS + 1)
#define OPTION_STDCALL_ALIASES		(OPTION_KILL_ATS + 1)
#define OPTION_ENABLE_STDCALL_FIXUP	(OPTION_STDCALL_ALIASES + 1)
#define OPTION_DISABLE_STDCALL_FIXUP	(OPTION_ENABLE_STDCALL_FIXUP + 1)
#define OPTION_IMPLIB_FILENAME		(OPTION_DISABLE_STDCALL_FIXUP + 1)
#define OPTION_THUMB_ENTRY		(OPTION_IMPLIB_FILENAME + 1)
#define OPTION_WARN_DUPLICATE_EXPORTS	(OPTION_THUMB_ENTRY + 1)
#define OPTION_IMP_COMPAT		(OPTION_WARN_DUPLICATE_EXPORTS + 1)
#define OPTION_ENABLE_AUTO_IMAGE_BASE	(OPTION_IMP_COMPAT + 1)
#define OPTION_DISABLE_AUTO_IMAGE_BASE	(OPTION_ENABLE_AUTO_IMAGE_BASE + 1)
#define OPTION_DLL_SEARCH_PREFIX	(OPTION_DISABLE_AUTO_IMAGE_BASE + 1)
#define OPTION_NO_DEFAULT_EXCLUDES	(OPTION_DLL_SEARCH_PREFIX + 1)
#define OPTION_DLL_ENABLE_AUTO_IMPORT	(OPTION_NO_DEFAULT_EXCLUDES + 1)
#define OPTION_DLL_DISABLE_AUTO_IMPORT	(OPTION_DLL_ENABLE_AUTO_IMPORT + 1)
#define OPTION_ENABLE_EXTRA_PE_DEBUG	(OPTION_DLL_DISABLE_AUTO_IMPORT + 1)
#define OPTION_EXCLUDE_LIBS		(OPTION_ENABLE_EXTRA_PE_DEBUG + 1)
#define OPTION_DLL_ENABLE_RUNTIME_PSEUDO_RELOC	\
					(OPTION_EXCLUDE_LIBS + 1)
#define OPTION_DLL_DISABLE_RUNTIME_PSEUDO_RELOC	\
					(OPTION_DLL_ENABLE_RUNTIME_PSEUDO_RELOC + 1)
#define OPTION_LARGE_ADDRESS_AWARE 	(OPTION_DLL_DISABLE_RUNTIME_PSEUDO_RELOC + 1)
#define OPTION_DISABLE_LARGE_ADDRESS_AWARE \
 					(OPTION_LARGE_ADDRESS_AWARE + 1)
#define OPTION_DLL_ENABLE_RUNTIME_PSEUDO_RELOC_V1	\
					(OPTION_DISABLE_LARGE_ADDRESS_AWARE + 1)
#define OPTION_DLL_ENABLE_RUNTIME_PSEUDO_RELOC_V2	\
					(OPTION_DLL_ENABLE_RUNTIME_PSEUDO_RELOC_V1 + 1)
#define OPTION_EXCLUDE_MODULES_FOR_IMPLIB \
					(OPTION_DLL_ENABLE_RUNTIME_PSEUDO_RELOC_V2 + 1)
#define OPTION_USE_NUL_PREFIXED_IMPORT_TABLES \
					(OPTION_EXCLUDE_MODULES_FOR_IMPLIB + 1)
#define OPTION_NO_LEADING_UNDERSCORE 	(OPTION_USE_NUL_PREFIXED_IMPORT_TABLES + 1)
#define OPTION_LEADING_UNDERSCORE 	(OPTION_NO_LEADING_UNDERSCORE + 1)
#define OPTION_ENABLE_LONG_SECTION_NAMES \
					(OPTION_LEADING_UNDERSCORE + 1)
#define OPTION_DISABLE_LONG_SECTION_NAMES \
					(OPTION_ENABLE_LONG_SECTION_NAMES + 1)
/* DLLCharacteristics flags.  */
#define OPTION_DYNAMIC_BASE		(OPTION_DISABLE_LONG_SECTION_NAMES + 1)
#define OPTION_FORCE_INTEGRITY		(OPTION_DYNAMIC_BASE + 1)
#define OPTION_NX_COMPAT		(OPTION_FORCE_INTEGRITY + 1)
#define OPTION_NO_ISOLATION		(OPTION_NX_COMPAT + 1)
#define OPTION_NO_SEH			(OPTION_NO_ISOLATION + 1)
#define OPTION_NO_BIND			(OPTION_NO_SEH + 1)
#define OPTION_WDM_DRIVER		(OPTION_NO_BIND + 1)
#define OPTION_TERMINAL_SERVER_AWARE	(OPTION_WDM_DRIVER + 1)
/* Determinism.  */
#define OPTION_INSERT_TIMESTAMP		(OPTION_TERMINAL_SERVER_AWARE + 1)
#define OPTION_NO_INSERT_TIMESTAMP	(OPTION_INSERT_TIMESTAMP + 1)
#define OPTION_BUILD_ID			(OPTION_NO_INSERT_TIMESTAMP + 1)

static void
gld${EMULATION_NAME}_add_options
  (int ns ATTRIBUTE_UNUSED,
   char **shortopts ATTRIBUTE_UNUSED,
   int nl,
   struct option **longopts,
   int nrl ATTRIBUTE_UNUSED,
   struct option **really_longopts ATTRIBUTE_UNUSED)
{
  static const struct option xtra_long[] =
  {
    /* PE options.  */
    {"base-file", required_argument, NULL, OPTION_BASE_FILE},
    {"dll", no_argument, NULL, OPTION_DLL},
    {"file-alignment", required_argument, NULL, OPTION_FILE_ALIGNMENT},
    {"heap", required_argument, NULL, OPTION_HEAP},
    {"image-base", required_argument, NULL, OPTION_IMAGE_BASE},
    {"major-image-version", required_argument, NULL, OPTION_MAJOR_IMAGE_VERSION},
    {"major-os-version", required_argument, NULL, OPTION_MAJOR_OS_VERSION},
    {"major-subsystem-version", required_argument, NULL, OPTION_MAJOR_SUBSYSTEM_VERSION},
    {"minor-image-version", required_argument, NULL, OPTION_MINOR_IMAGE_VERSION},
    {"minor-os-version", required_argument, NULL, OPTION_MINOR_OS_VERSION},
    {"minor-subsystem-version", required_argument, NULL, OPTION_MINOR_SUBSYSTEM_VERSION},
    {"section-alignment", required_argument, NULL, OPTION_SECTION_ALIGNMENT},
    {"stack", required_argument, NULL, OPTION_STACK},
    {"subsystem", required_argument, NULL, OPTION_SUBSYSTEM},
    {"support-old-code", no_argument, NULL, OPTION_SUPPORT_OLD_CODE},
    {"thumb-entry", required_argument, NULL, OPTION_THUMB_ENTRY},
    {"use-nul-prefixed-import-tables", no_argument, NULL,
     OPTION_USE_NUL_PREFIXED_IMPORT_TABLES},
    {"no-leading-underscore", no_argument, NULL, OPTION_NO_LEADING_UNDERSCORE},
    {"leading-underscore", no_argument, NULL, OPTION_LEADING_UNDERSCORE},
    {"insert-timestamp", no_argument, NULL, OPTION_INSERT_TIMESTAMP},
    {"no-insert-timestamp", no_argument, NULL, OPTION_NO_INSERT_TIMESTAMP},
#ifdef DLL_SUPPORT
    /* getopt allows abbreviations, so we do this to stop it
       from treating -o as an abbreviation for this option.  */
    {"output-def", required_argument, NULL, OPTION_OUT_DEF},
    {"output-def", required_argument, NULL, OPTION_OUT_DEF},
    {"export-all-symbols", no_argument, NULL, OPTION_EXPORT_ALL},
    {"exclude-symbols", required_argument, NULL, OPTION_EXCLUDE_SYMBOLS},
    {"exclude-all-symbols", no_argument, NULL, OPTION_EXCLUDE_ALL_SYMBOLS},
    {"exclude-libs", required_argument, NULL, OPTION_EXCLUDE_LIBS},
    {"exclude-modules-for-implib", required_argument, NULL, OPTION_EXCLUDE_MODULES_FOR_IMPLIB},
    {"kill-at", no_argument, NULL, OPTION_KILL_ATS},
    {"add-stdcall-alias", no_argument, NULL, OPTION_STDCALL_ALIASES},
    {"enable-stdcall-fixup", no_argument, NULL, OPTION_ENABLE_STDCALL_FIXUP},
    {"disable-stdcall-fixup", no_argument, NULL, OPTION_DISABLE_STDCALL_FIXUP},
    {"out-implib", required_argument, NULL, OPTION_IMPLIB_FILENAME},
    {"warn-duplicate-exports", no_argument, NULL, OPTION_WARN_DUPLICATE_EXPORTS},
    /* getopt() allows abbreviations, so we do this to stop it from
       treating -c as an abbreviation for these --compat-implib.  */
    {"compat-implib", no_argument, NULL, OPTION_IMP_COMPAT},
    {"compat-implib", no_argument, NULL, OPTION_IMP_COMPAT},
    {"enable-auto-image-base", optional_argument, NULL, OPTION_ENABLE_AUTO_IMAGE_BASE},
    {"disable-auto-image-base", no_argument, NULL, OPTION_DISABLE_AUTO_IMAGE_BASE},
    {"dll-search-prefix", required_argument, NULL, OPTION_DLL_SEARCH_PREFIX},
    {"no-default-excludes", no_argument, NULL, OPTION_NO_DEFAULT_EXCLUDES},
    {"enable-auto-import", no_argument, NULL, OPTION_DLL_ENABLE_AUTO_IMPORT},
    {"disable-auto-import", no_argument, NULL, OPTION_DLL_DISABLE_AUTO_IMPORT},
    {"enable-extra-pe-debug", no_argument, NULL, OPTION_ENABLE_EXTRA_PE_DEBUG},
    {"enable-runtime-pseudo-reloc", no_argument, NULL, OPTION_DLL_ENABLE_RUNTIME_PSEUDO_RELOC},
    {"disable-runtime-pseudo-reloc", no_argument, NULL, OPTION_DLL_DISABLE_RUNTIME_PSEUDO_RELOC},
    {"enable-runtime-pseudo-reloc-v1", no_argument, NULL, OPTION_DLL_ENABLE_RUNTIME_PSEUDO_RELOC_V1},
    {"enable-runtime-pseudo-reloc-v2", no_argument, NULL, OPTION_DLL_ENABLE_RUNTIME_PSEUDO_RELOC_V2},
#endif
    {"large-address-aware", no_argument, NULL, OPTION_LARGE_ADDRESS_AWARE},
    {"disable-large-address-aware", no_argument, NULL, OPTION_DISABLE_LARGE_ADDRESS_AWARE},
    {"enable-long-section-names", no_argument, NULL, OPTION_ENABLE_LONG_SECTION_NAMES},
    {"disable-long-section-names", no_argument, NULL, OPTION_DISABLE_LONG_SECTION_NAMES},
    {"dynamicbase",no_argument, NULL, OPTION_DYNAMIC_BASE},
    {"forceinteg", no_argument, NULL, OPTION_FORCE_INTEGRITY},
    {"nxcompat", no_argument, NULL, OPTION_NX_COMPAT},
    {"no-isolation", no_argument, NULL, OPTION_NO_ISOLATION},
    {"no-seh", no_argument, NULL, OPTION_NO_SEH},
    {"no-bind", no_argument, NULL, OPTION_NO_BIND},
    {"wdmdriver", no_argument, NULL, OPTION_WDM_DRIVER},
    {"tsaware", no_argument, NULL, OPTION_TERMINAL_SERVER_AWARE},
    {"build-id", optional_argument, NULL, OPTION_BUILD_ID},
    {NULL, no_argument, NULL, 0}
  };

  *longopts
    = xrealloc (*longopts, nl * sizeof (struct option) + sizeof (xtra_long));
  memcpy (*longopts + nl, &xtra_long, sizeof (xtra_long));
}

/* PE/WIN32; added routines to get the subsystem type, heap and/or stack
   parameters which may be input from the command line.  */

typedef struct
{
  void *ptr;
  int size;
  int value;
  char *symbol;
  int inited;
  /* FALSE for an assembly level symbol and TRUE for a C visible symbol.
     C visible symbols can be prefixed by underscore dependent to target's
     settings.  */
  bfd_boolean is_c_symbol;
} definfo;

/* Get symbol name dependent to kind and C visible state of
   underscore.  */
#define GET_INIT_SYMBOL_NAME(IDX) \
  (init[(IDX)].symbol \
  + ((init[(IDX)].is_c_symbol == FALSE || (is_underscoring () != 0)) ? 0 : 1))

/* Decorates the C visible symbol by underscore, if target requires.  */
#define U(CSTR) \
  ((is_underscoring () == 0) ? CSTR : "_" CSTR)

/* Get size of constant string for a possible underscore prefixed
   C visible symbol.  */
#define U_SIZE(CSTR) \
  (sizeof (CSTR) + (is_underscoring () == 0 ? 0 : 1))

#define D(field,symbol,def,usc)  {&pe.field, sizeof (pe.field), def, symbol, 0, usc}

static definfo init[] =
{
  /* imagebase must be first */
#define IMAGEBASEOFF 0
  D(ImageBase,"__image_base__", NT_EXE_IMAGE_BASE, FALSE),
#define DLLOFF 1
  {&dll, sizeof(dll), 0, "__dll__", 0, FALSE},
#define MSIMAGEBASEOFF	2
  D(ImageBase, "___ImageBase", NT_EXE_IMAGE_BASE, TRUE),
  D(SectionAlignment,"__section_alignment__", PE_DEF_SECTION_ALIGNMENT, FALSE),
  D(FileAlignment,"__file_alignment__", PE_DEF_FILE_ALIGNMENT, FALSE),
  D(MajorOperatingSystemVersion,"__major_os_version__", 4, FALSE),
  D(MinorOperatingSystemVersion,"__minor_os_version__", 0, FALSE),
  D(MajorImageVersion,"__major_image_version__", 1, FALSE),
  D(MinorImageVersion,"__minor_image_version__", 0, FALSE),
#if defined(TARGET_IS_armpe)  || defined(TARGET_IS_arm_wince_pe)
  D(MajorSubsystemVersion,"__major_subsystem_version__", 3, FALSE),
#else
  D(MajorSubsystemVersion,"__major_subsystem_version__", 4, FALSE),
#endif
  D(MinorSubsystemVersion,"__minor_subsystem_version__", 0, FALSE),
  D(Subsystem,"__subsystem__", ${SUBSYSTEM}, FALSE),
  D(SizeOfStackReserve,"__size_of_stack_reserve__", 0x200000, FALSE),
  D(SizeOfStackCommit,"__size_of_stack_commit__", 0x1000, FALSE),
  D(SizeOfHeapReserve,"__size_of_heap_reserve__", 0x100000, FALSE),
  D(SizeOfHeapCommit,"__size_of_heap_commit__", 0x1000, FALSE),
  D(LoaderFlags,"__loader_flags__", 0x0, FALSE),
  D(DllCharacteristics, "__dll_characteristics__", 0x0, FALSE),
  { NULL, 0, 0, NULL, 0 , FALSE}
};


static void
gld_${EMULATION_NAME}_list_options (FILE *file)
{
  fprintf (file, _("  --base_file <basefile>             Generate a base file for relocatable DLLs\n"));
  fprintf (file, _("  --dll                              Set image base to the default for DLLs\n"));
  fprintf (file, _("  --file-alignment <size>            Set file alignment\n"));
  fprintf (file, _("  --heap <size>                      Set initial size of the heap\n"));
  fprintf (file, _("  --image-base <address>             Set start address of the executable\n"));
  fprintf (file, _("  --major-image-version <number>     Set version number of the executable\n"));
  fprintf (file, _("  --major-os-version <number>        Set minimum required OS version\n"));
  fprintf (file, _("  --major-subsystem-version <number> Set minimum required OS subsystem version\n"));
  fprintf (file, _("  --minor-image-version <number>     Set revision number of the executable\n"));
  fprintf (file, _("  --minor-os-version <number>        Set minimum required OS revision\n"));
  fprintf (file, _("  --minor-subsystem-version <number> Set minimum required OS subsystem revision\n"));
  fprintf (file, _("  --section-alignment <size>         Set section alignment\n"));
  fprintf (file, _("  --stack <size>                     Set size of the initial stack\n"));
  fprintf (file, _("  --subsystem <name>[:<version>]     Set required OS subsystem [& version]\n"));
  fprintf (file, _("  --support-old-code                 Support interworking with old code\n"));
  fprintf (file, _("  --[no-]leading-underscore          Set explicit symbol underscore prefix mode\n"));
  fprintf (file, _("  --thumb-entry=<symbol>             Set the entry point to be Thumb <symbol>\n"));
  fprintf (file, _("  --[no-]insert-timestamp            Use a real timestamp rather than zero (default).\n"));
  fprintf (file, _("                                     This makes binaries non-deterministic\n"));
#ifdef DLL_SUPPORT
  fprintf (file, _("  --add-stdcall-alias                Export symbols with and without @nn\n"));
  fprintf (file, _("  --disable-stdcall-fixup            Don't link _sym to _sym@nn\n"));
  fprintf (file, _("  --enable-stdcall-fixup             Link _sym to _sym@nn without warnings\n"));
  fprintf (file, _("  --exclude-symbols sym,sym,...      Exclude symbols from automatic export\n"));
  fprintf (file, _("  --exclude-all-symbols              Exclude all symbols from automatic export\n"));
  fprintf (file, _("  --exclude-libs lib,lib,...         Exclude libraries from automatic export\n"));
  fprintf (file, _("  --exclude-modules-for-implib mod,mod,...\n"));
  fprintf (file, _("                                     Exclude objects, archive members from auto\n"));
  fprintf (file, _("                                     export, place into import library instead.\n"));
  fprintf (file, _("  --export-all-symbols               Automatically export all globals to DLL\n"));
  fprintf (file, _("  --kill-at                          Remove @nn from exported symbols\n"));
  fprintf (file, _("  --out-implib <file>                Generate import library\n"));
  fprintf (file, _("  --output-def <file>                Generate a .DEF file for the built DLL\n"));
  fprintf (file, _("  --warn-duplicate-exports           Warn about duplicate exports\n"));
  fprintf (file, _("  --compat-implib                    Create backward compatible import libs;\n\
                                       create __imp_<SYMBOL> as well.\n"));
  fprintf (file, _("  --enable-auto-image-base[=<address>] Automatically choose image base for DLLs\n\
                                       (optionally starting with address) unless\n\
                                       specifically set with --image-base\n"));
  fprintf (file, _("  --disable-auto-image-base          Do not auto-choose image base. (default)\n"));
  fprintf (file, _("  --dll-search-prefix=<string>       When linking dynamically to a dll without\n\
                                       an importlib, use <string><basename>.dll\n\
                                       in preference to lib<basename>.dll \n"));
  fprintf (file, _("  --enable-auto-import               Do sophisticated linking of _sym to\n\
                                       __imp_sym for DATA references\n"));
  fprintf (file, _("  --disable-auto-import              Do not auto-import DATA items from DLLs\n"));
  fprintf (file, _("  --enable-runtime-pseudo-reloc      Work around auto-import limitations by\n\
                                       adding pseudo-relocations resolved at\n\
                                       runtime.\n"));
  fprintf (file, _("  --disable-runtime-pseudo-reloc     Do not add runtime pseudo-relocations for\n\
                                       auto-imported DATA.\n"));
  fprintf (file, _("  --enable-extra-pe-debug            Enable verbose debug output when building\n\
                                       or linking to DLLs (esp. auto-import)\n"));
#endif
  fprintf (file, _("  --large-address-aware              Executable supports virtual addresses\n\
                                       greater than 2 gigabytes\n"));
  fprintf (file, _("  --disable-large-address-aware      Executable does not support virtual\n\
                                       addresses greater than 2 gigabytes\n"));
  fprintf (file, _("  --enable-long-section-names        Use long COFF section names even in\n\
                                       executable image files\n"));
  fprintf (file, _("  --disable-long-section-names       Never use long COFF section names, even\n\
                                       in object files\n"));
  fprintf (file, _("  --dynamicbase			 Image base address may be relocated using\n\
				       address space layout randomization (ASLR)\n"));
  fprintf (file, _("  --forceinteg		 Code integrity checks are enforced\n"));
  fprintf (file, _("  --nxcompat		 Image is compatible with data execution prevention\n"));
  fprintf (file, _("  --no-isolation		 Image understands isolation but do not isolate the image\n"));
  fprintf (file, _("  --no-seh			 Image does not use SEH. No SE handler may\n\
				       be called in this image\n"));
  fprintf (file, _("  --no-bind			 Do not bind this image\n"));
  fprintf (file, _("  --wdmdriver		 Driver uses the WDM model\n"));
  fprintf (file, _("  --tsaware                  Image is Terminal Server aware\n"));
  fprintf (file, _("  --build-id[=STYLE]         Generate build ID\n"));
}


static void
set_pe_name (char *name, long val)
{
  int i;
  is_underscoring ();

  /* Find the name and set it.  */
  for (i = 0; init[i].ptr; i++)
    {
      if (strcmp (name, GET_INIT_SYMBOL_NAME (i)) == 0)
	{
	  init[i].value = val;
	  init[i].inited = 1;
	  if (strcmp (name,"__image_base__") == 0)
	    set_pe_name (U ("__ImageBase"), val);
	  return;
	}
    }
  abort ();
}

static void
set_entry_point (void)
{
  const char *entry;
  const char *initial_symbol_char;
  int i;

  static const struct
    {
      const int value;
      const char *entry;
    }
  v[] =
    {
      { 1, "NtProcessStartup"  },
      { 2, "WinMainCRTStartup" },
      { 3, "mainCRTStartup"    },
      { 7, "__PosixProcessStartup"},
      { 9, "WinMainCRTStartup" },
      {14, "mainCRTStartup"    },
      { 0, NULL          }
    };

  /* Entry point name for arbitrary subsystem numbers.  */
  static const char default_entry[] = "mainCRTStartup";

  if (bfd_link_pic (&link_info) || dll)
    {
#if defined (TARGET_IS_i386pe)
      entry = "DllMainCRTStartup@12";
#else
      entry = "DllMainCRTStartup";
#endif
    }
  else
    {
      for (i = 0; v[i].entry; i++)
        if (v[i].value == pe_subsystem)
          break;

      /* If no match, use the default.  */
      if (v[i].entry != NULL)
        entry = v[i].entry;
      else
        entry = default_entry;
    }

  initial_symbol_char = (is_underscoring () != 0 ? "_" : "");

  if (*initial_symbol_char != '\0')
    {
      char *alc_entry;

      /* lang_default_entry expects its argument to be permanently
	 allocated, so we don't free this string.  */
      alc_entry = xmalloc (strlen (initial_symbol_char)
			   + strlen (entry)
			   + 1);
      strcpy (alc_entry, initial_symbol_char);
      strcat (alc_entry, entry);
      entry = alc_entry;
    }

  lang_default_entry (entry);
}

static void
set_pe_subsystem (void)
{
  const char *sver;
  char *end;
  int len;
  int i;
  unsigned long temp_subsystem;
  static const struct
    {
      const char *name;
      const int value;
    }
  v[] =
    {
      { "native",  1},
      { "windows", 2},
      { "console", 3},
      { "posix",   7},
      { "wince",   9},
      { "xbox",   14},
      { NULL, 0 }
    };

  /* Check for the presence of a version number.  */
  sver = strchr (optarg, ':');
  if (sver == NULL)
    len = strlen (optarg);
  else
    {
      len = sver - optarg;
      set_pe_name ("__major_subsystem_version__",
		    strtoul (sver + 1, &end, 0));
      if (*end == '.')
	set_pe_name ("__minor_subsystem_version__",
		      strtoul (end + 1, &end, 0));
      if (*end != '\0')
	einfo (_("%P: warning: bad version number in -subsystem option\n"));
    }

  /* Check for numeric subsystem.  */
  temp_subsystem = strtoul (optarg, & end, 0);
  if ((*end == ':' || *end == '\0') && (temp_subsystem < 65536))
    {
      /* Search list for a numeric match to use its entry point.  */
      for (i = 0; v[i].name; i++)
	if (v[i].value == (int) temp_subsystem)
	  break;

      /* Use this subsystem.  */
      pe_subsystem = (int) temp_subsystem;
    }
  else
    {
      /* Search for subsystem by name.  */
      for (i = 0; v[i].name; i++)
	if (strncmp (optarg, v[i].name, len) == 0
	    && v[i].name[len] == '\0')
	  break;

      if (v[i].name == NULL)
	{
	  einfo (_("%P%F: invalid subsystem type %s\n"), optarg);
	  return;
	}

      pe_subsystem = v[i].value;
    }

  set_pe_name ("__subsystem__", pe_subsystem);

  return;
}


static void
set_pe_value (char *name)
{
  char *end;

  set_pe_name (name,  strtoul (optarg, &end, 0));

  if (end == optarg)
    einfo (_("%P%F: invalid hex number for PE parameter '%s'\n"), optarg);

  optarg = end;
}


static void
set_pe_stack_heap (char *resname, char *comname)
{
  set_pe_value (resname);

  if (*optarg == ',')
    {
      optarg++;
      set_pe_value (comname);
    }
  else if (*optarg)
    einfo (_("%P%F: strange hex info for PE parameter '%s'\n"), optarg);
}

#define DEFAULT_BUILD_ID_STYLE	"md5"

static bfd_boolean
gld${EMULATION_NAME}_handle_option (int optc)
{
  switch (optc)
    {
    default:
      return FALSE;

    case OPTION_BASE_FILE:
      link_info.base_file = fopen (optarg, FOPEN_WB);
      if (link_info.base_file == NULL)
	einfo (_("%F%P: cannot open base file %s\n"), optarg);
      break;

      /* PE options.  */
    case OPTION_HEAP:
      set_pe_stack_heap ("__size_of_heap_reserve__", "__size_of_heap_commit__");
      break;
    case OPTION_STACK:
      set_pe_stack_heap ("__size_of_stack_reserve__", "__size_of_stack_commit__");
      break;
    case OPTION_SUBSYSTEM:
      set_pe_subsystem ();
      break;
    case OPTION_MAJOR_OS_VERSION:
      set_pe_value ("__major_os_version__");
      break;
    case OPTION_MINOR_OS_VERSION:
      set_pe_value ("__minor_os_version__");
      break;
    case OPTION_MAJOR_SUBSYSTEM_VERSION:
      set_pe_value ("__major_subsystem_version__");
      break;
    case OPTION_MINOR_SUBSYSTEM_VERSION:
      set_pe_value ("__minor_subsystem_version__");
      break;
    case OPTION_MAJOR_IMAGE_VERSION:
      set_pe_value ("__major_image_version__");
      break;
    case OPTION_MINOR_IMAGE_VERSION:
      set_pe_value ("__minor_image_version__");
      break;
    case OPTION_FILE_ALIGNMENT:
      set_pe_value ("__file_alignment__");
      break;
    case OPTION_SECTION_ALIGNMENT:
      set_pe_value ("__section_alignment__");
      break;
    case OPTION_DLL:
      set_pe_name ("__dll__", 1);
      break;
    case OPTION_IMAGE_BASE:
      set_pe_value ("__image_base__");
      break;
    case OPTION_SUPPORT_OLD_CODE:
      support_old_code = 1;
      break;
    case OPTION_THUMB_ENTRY:
      thumb_entry_symbol = optarg;
      break;
    case OPTION_USE_NUL_PREFIXED_IMPORT_TABLES:
      pe_use_nul_prefixed_import_tables = TRUE;
      break;
    case OPTION_NO_LEADING_UNDERSCORE:
      pe_leading_underscore = 0;
      break;
    case OPTION_LEADING_UNDERSCORE:
      pe_leading_underscore = 1;
      break;
    case OPTION_INSERT_TIMESTAMP:
      insert_timestamp = TRUE;
      break;
    case OPTION_NO_INSERT_TIMESTAMP:
      insert_timestamp = FALSE;
      break;
#ifdef DLL_SUPPORT
    case OPTION_OUT_DEF:
      pe_out_def_filename = xstrdup (optarg);
      break;
    case OPTION_EXPORT_ALL:
      pe_dll_export_everything = 1;
      break;
    case OPTION_EXCLUDE_SYMBOLS:
      pe_dll_add_excludes (optarg, EXCLUDESYMS);
      break;
    case OPTION_EXCLUDE_ALL_SYMBOLS:
      pe_dll_exclude_all_symbols = 1;
      break;
    case OPTION_EXCLUDE_LIBS:
      pe_dll_add_excludes (optarg, EXCLUDELIBS);
      break;
    case OPTION_EXCLUDE_MODULES_FOR_IMPLIB:
      pe_dll_add_excludes (optarg, EXCLUDEFORIMPLIB);
      break;
    case OPTION_KILL_ATS:
      pe_dll_kill_ats = 1;
      break;
    case OPTION_STDCALL_ALIASES:
      pe_dll_stdcall_aliases = 1;
      break;
    case OPTION_ENABLE_STDCALL_FIXUP:
      pe_enable_stdcall_fixup = 1;
      break;
    case OPTION_DISABLE_STDCALL_FIXUP:
      pe_enable_stdcall_fixup = 0;
      break;
    case OPTION_IMPLIB_FILENAME:
      pe_implib_filename = xstrdup (optarg);
      break;
    case OPTION_WARN_DUPLICATE_EXPORTS:
      pe_dll_warn_dup_exports = 1;
      break;
    case OPTION_IMP_COMPAT:
      pe_dll_compat_implib = 1;
      break;
    case OPTION_ENABLE_AUTO_IMAGE_BASE:
      pe_enable_auto_image_base = 1;
      if (optarg && *optarg)
	{
	  char *end;
	  pe_auto_image_base = strtoul (optarg, &end, 0);
	  /* XXX should check that we actually parsed something */
	}
      break;
    case OPTION_DISABLE_AUTO_IMAGE_BASE:
      pe_enable_auto_image_base = 0;
      break;
    case OPTION_DLL_SEARCH_PREFIX:
      pe_dll_search_prefix = xstrdup (optarg);
      break;
    case OPTION_NO_DEFAULT_EXCLUDES:
      pe_dll_do_default_excludes = 0;
      break;
    case OPTION_DLL_ENABLE_AUTO_IMPORT:
      link_info.pei386_auto_import = 1;
      break;
    case OPTION_DLL_DISABLE_AUTO_IMPORT:
      link_info.pei386_auto_import = 0;
      break;
    case OPTION_DLL_ENABLE_RUNTIME_PSEUDO_RELOC:
      link_info.pei386_runtime_pseudo_reloc =
	DEFAULT_PSEUDO_RELOC_VERSION;
      break;
    case OPTION_DLL_ENABLE_RUNTIME_PSEUDO_RELOC_V1:
      link_info.pei386_runtime_pseudo_reloc = 1;
      break;
    case OPTION_DLL_ENABLE_RUNTIME_PSEUDO_RELOC_V2:
      link_info.pei386_runtime_pseudo_reloc = 2;
      break;
    case OPTION_DLL_DISABLE_RUNTIME_PSEUDO_RELOC:
      link_info.pei386_runtime_pseudo_reloc = 0;
      break;
    case OPTION_ENABLE_EXTRA_PE_DEBUG:
      pe_dll_extra_pe_debug = 1;
      break;
#endif
    case OPTION_LARGE_ADDRESS_AWARE:
      real_flags |= IMAGE_FILE_LARGE_ADDRESS_AWARE;
      break;
    case OPTION_DISABLE_LARGE_ADDRESS_AWARE:
      real_flags &= ~ IMAGE_FILE_LARGE_ADDRESS_AWARE;
      break;
    case OPTION_ENABLE_LONG_SECTION_NAMES:
      pe_use_coff_long_section_names = 1;
      break;
    case OPTION_DISABLE_LONG_SECTION_NAMES:
      pe_use_coff_long_section_names = 0;
      break;
/*  Get DLLCharacteristics bits  */
    case OPTION_DYNAMIC_BASE:
      pe_dll_characteristics |= IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE;
      break;
    case OPTION_FORCE_INTEGRITY:
      pe_dll_characteristics |= IMAGE_DLL_CHARACTERISTICS_FORCE_INTEGRITY;
      break;
    case OPTION_NX_COMPAT:
      pe_dll_characteristics |= IMAGE_DLL_CHARACTERISTICS_NX_COMPAT;
      break;
    case OPTION_NO_ISOLATION:
      pe_dll_characteristics |= IMAGE_DLLCHARACTERISTICS_NO_ISOLATION;
      break;
    case OPTION_NO_SEH:
      pe_dll_characteristics |= IMAGE_DLLCHARACTERISTICS_NO_SEH;
      break;
    case OPTION_NO_BIND:
      pe_dll_characteristics |= IMAGE_DLLCHARACTERISTICS_NO_BIND;
      break;
    case OPTION_WDM_DRIVER:
      pe_dll_characteristics |= IMAGE_DLLCHARACTERISTICS_WDM_DRIVER;
      break;
    case OPTION_TERMINAL_SERVER_AWARE:
      pe_dll_characteristics |= IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE;
      break;
    case OPTION_BUILD_ID:
      if (emit_build_id != NULL)
	{
	  free ((char *) emit_build_id);
	  emit_build_id = NULL;
	}
      if (optarg == NULL)
	optarg = DEFAULT_BUILD_ID_STYLE;
      if (strcmp (optarg, "none"))
	emit_build_id = xstrdup (optarg);
      break;
    }

  /*  Set DLLCharacteristics bits  */
  set_pe_name ("__dll_characteristics__", pe_dll_characteristics);

  return TRUE;
}


#ifdef DLL_SUPPORT
static unsigned long
strhash (const char *str)
{
  const unsigned char *s;
  unsigned long hash;
  unsigned int c;
  unsigned int len;

  hash = 0;
  len = 0;
  s = (const unsigned char *) str;
  while ((c = *s++) != '\0')
    {
      hash += c + (c << 17);
      hash ^= hash >> 2;
      ++len;
    }
  hash += len + (len << 17);
  hash ^= hash >> 2;

  return hash;
}

/* Use the output file to create a image base for relocatable DLLs.  */

static unsigned long
compute_dll_image_base (const char *ofile)
{
  unsigned long hash = strhash (ofile);
  return pe_auto_image_base + ((hash << 16) & 0x0FFC0000);
}
#endif

/* Assign values to the special symbols before the linker script is
   read.  */

static void
gld_${EMULATION_NAME}_set_symbols (void)
{
  /* Run through and invent symbols for all the
     names and insert the defaults.  */
  int j;

  is_underscoring ();

  if (!init[IMAGEBASEOFF].inited)
    {
      if (bfd_link_relocatable (&link_info))
	init[IMAGEBASEOFF].value = 0;
      else if (init[DLLOFF].value || bfd_link_dll (&link_info))
	{
#ifdef DLL_SUPPORT
	  init[IMAGEBASEOFF].value = (pe_enable_auto_image_base
				      ? compute_dll_image_base (output_filename)
				      : NT_DLL_IMAGE_BASE);
#else
	  init[IMAGEBASEOFF].value = NT_DLL_IMAGE_BASE;
#endif
	}
      else
	init[IMAGEBASEOFF].value = NT_EXE_IMAGE_BASE;
      init[MSIMAGEBASEOFF].value = init[IMAGEBASEOFF].value;
    }

  /* Don't do any symbol assignments if this is a relocatable link.  */
  if (bfd_link_relocatable (&link_info))
    return;

  /* Glue the assignments into the abs section.  */
  push_stat_ptr (&abs_output_section->children);

  for (j = 0; init[j].ptr; j++)
    {
      long val = init[j].value;
      lang_assignment_statement_type *rv;

      rv = lang_add_assignment (exp_assign (GET_INIT_SYMBOL_NAME (j),
					    exp_intop (val), FALSE));
      if (init[j].size == sizeof (short))
	*(short *) init[j].ptr = val;
      else if (init[j].size == sizeof (int))
	*(int *) init[j].ptr = val;
      else if (init[j].size == sizeof (long))
	*(long *) init[j].ptr = val;
      /* This might be a long long or other special type.  */
      else if (init[j].size == sizeof (bfd_vma))
	*(bfd_vma *) init[j].ptr = val;
      else	abort ();
      if (j == IMAGEBASEOFF)
	image_base_statement = rv;
    }
  /* Restore the pointer.  */
  pop_stat_ptr ();

  if (pe.FileAlignment > pe.SectionAlignment)
    {
      einfo (_("%P: warning, file alignment > section alignment.\n"));
    }
}

/* This is called after the linker script and the command line options
   have been read.  */

static void
gld_${EMULATION_NAME}_after_parse (void)
{
  /* PR ld/6744:  Warn the user if they have used an ELF-only
     option hoping it will work on PE.  */
  if (link_info.export_dynamic)
    einfo (_("%P: warning: --export-dynamic is not supported for PE "
      "targets, did you mean --export-all-symbols?\n"));

  set_entry_point ();

  after_parse_default ();
}

/* pe-dll.c directly accesses pe_data_import_dll,
   so it must be defined outside of #ifdef DLL_SUPPORT.
   Note - this variable is deliberately not initialised.
   This allows it to be treated as a common varaible, and only
   exist in one incarnation in a multiple target enabled linker.  */
char * pe_data_import_dll;

#ifdef DLL_SUPPORT
static struct bfd_link_hash_entry *pe_undef_found_sym;

static bfd_boolean
pe_undef_cdecl_match (struct bfd_link_hash_entry *h, void *inf)
{
  int sl;
  char *string = inf;
  const char *hs = h->root.string;

  sl = strlen (string);
  if (h->type == bfd_link_hash_defined
      && ((*hs == '@' && *string == '_'
		   && strncmp (hs + 1, string + 1, sl - 1) == 0)
		  || strncmp (hs, string, sl) == 0)
      && h->root.string[sl] == '@')
    {
      pe_undef_found_sym = h;
      return FALSE;
    }
  return TRUE;
}

/* Change UNDEF to a defined symbol, taking data from SYM.  */

static void
change_undef (struct bfd_link_hash_entry * undef,
	      struct bfd_link_hash_entry * sym)
{
  static bfd_boolean  gave_warning_message = FALSE;

  undef->type = bfd_link_hash_defined;
  undef->u.def.value = sym->u.def.value;
  undef->u.def.section = sym->u.def.section;

  if (pe_enable_stdcall_fixup == -1)
    {
      einfo (_("Warning: resolving %s by linking to %s\n"),
	     undef->root.string, sym->root.string);

      if (! gave_warning_message)
	{
	  einfo (_("Use --enable-stdcall-fixup to disable these warnings\n"));
	  einfo (_("Use --disable-stdcall-fixup to disable these fixups\n"));
	  gave_warning_message = TRUE;
	}
    }

  /* PR 19803: Make sure that the linked symbol is not garbage collected.  */
  lang_add_gc_name (sym->root.string);
}

static void
pe_fixup_stdcalls (void)
{
  struct bfd_link_hash_entry *undef, *sym;

  if (pe_dll_extra_pe_debug)
    printf ("%s\n", __FUNCTION__);

  for (undef = link_info.hash->undefs; undef; undef=undef->u.undef.next)
    if (undef->type == bfd_link_hash_undefined)
      {
	const char * name = undef->root.string;
	char * at;
	int lead_at = (*name == '@');

	if (lead_at)
	  at = strchr (name + 1, '@');
	else
	  at = strchr (name, '@');

	if (at || lead_at)
	  {
	    /* The symbol is a stdcall symbol, so let's look for a
	       cdecl symbol with the same name and resolve to that.  */
	    char *cname = xstrdup (name);

	    if (lead_at)
	      *cname = '_';
	    if (at)	      
	      * strchr (cname, '@') = 0;
	    sym = bfd_link_hash_lookup (link_info.hash, cname, FALSE, FALSE, TRUE);

	    if (sym && sym->type == bfd_link_hash_defined)
	      change_undef (undef, sym);
	  }
	else
	  {
	    /* The symbol is a cdecl symbol, so we look for stdcall
	       symbols - which means scanning the whole symbol table.  */
	    pe_undef_found_sym = NULL;
	    bfd_link_hash_traverse (link_info.hash, pe_undef_cdecl_match,
				    (char *) name);
	    if (pe_undef_found_sym)
	      change_undef (undef, pe_undef_found_sym);
	  }
      }
}

static int
make_import_fixup (arelent *rel, asection *s)
{
  struct bfd_symbol *sym = *rel->sym_ptr_ptr;
  char addend[4];

  if (pe_dll_extra_pe_debug)
    printf ("arelent: %s@%#lx: add=%li\n", sym->name,
	    (unsigned long) rel->address, (long) rel->addend);

  if (! bfd_get_section_contents (s->owner, s, addend, rel->address, sizeof (addend)))
    einfo (_("%C: Cannot get section contents - auto-import exception\n"),
	   s->owner, s, rel->address);

  pe_create_import_fixup (rel, s, bfd_get_32 (s->owner, addend));

  return 1;
}

static void
pe_find_data_imports (void)
{
  struct bfd_link_hash_entry *undef, *sym;

  if (link_info.pei386_auto_import == 0)
    return;

  for (undef = link_info.hash->undefs; undef; undef=undef->u.undef.next)
    {
      if (undef->type == bfd_link_hash_undefined)
	{
	  /* C++ symbols are *long*.  */
#define BUF_SIZE 4096
	  char buf[BUF_SIZE];

	  if (pe_dll_extra_pe_debug)
	    printf ("%s:%s\n", __FUNCTION__, undef->root.string);

	  if (strlen (undef->root.string) > (BUF_SIZE - 6))
	    {
	      /* PR linker/18466.  */
	      einfo (_("%P: internal error: symbol too long: %s\n"),
		     undef->root.string);
	      return;
	    }

	  sprintf (buf, "__imp_%s", undef->root.string);

	  sym = bfd_link_hash_lookup (link_info.hash, buf, 0, 0, 1);

	  if (sym && sym->type == bfd_link_hash_defined)
	    {
	      bfd *b = sym->u.def.section->owner;
	      asymbol **symbols;
	      int nsyms, i;

	      if (link_info.pei386_auto_import == -1)
		{
		  static bfd_boolean warned = FALSE;

		  info_msg (_("Info: resolving %s by linking to %s (auto-import)\n"),
			    undef->root.string, buf);

		  /* PR linker/4844.  */
		  if (! warned)
		    {
		      warned = TRUE;
		      einfo (_("%P: warning: auto-importing has been activated without --enable-auto-import specified on the command line.\n\
This should work unless it involves constant data structures referencing symbols from auto-imported DLLs.\n"));
		    }
		}

	      if (!bfd_generic_link_read_symbols (b))
		{
		  einfo (_("%B%F: could not read symbols: %E\n"), b);
		  return;
		}

	      symbols = bfd_get_outsymbols (b);
	      nsyms = bfd_get_symcount (b);

	      for (i = 0; i < nsyms; i++)
		{
		  if (! CONST_STRNEQ (symbols[i]->name,
				      U ("_head_")))
		    continue;

		  if (pe_dll_extra_pe_debug)
		    printf ("->%s\n", symbols[i]->name);

		  pe_data_import_dll = (char *) (symbols[i]->name
						 + U_SIZE ("_head_") - 1);
		  break;
		}

	      pe_walk_relocs_of_symbol (&link_info, undef->root.string,
					make_import_fixup);

	      /* Let's differentiate it somehow from defined.  */
	      undef->type = bfd_link_hash_defweak;
	      /* We replace original name with __imp_ prefixed, this
		 1) may trash memory 2) leads to duplicate symbol generation.
		 Still, IMHO it's better than having name poluted.  */
	      undef->root.string = sym->root.string;
	      undef->u.def.value = sym->u.def.value;
	      undef->u.def.section = sym->u.def.section;
	    }
	}
    }
}

static bfd_boolean
pr_sym (struct bfd_hash_entry *h, void *inf ATTRIBUTE_UNUSED)
{
  printf ("+%s\n", h->string);

  return TRUE;
}
#endif /* DLL_SUPPORT */

static void
debug_section_p (bfd *abfd ATTRIBUTE_UNUSED, asection *sect, void *obj)
{
  int *found = (int *) obj;
  if (strncmp (".debug_", sect->name, sizeof (".debug_") - 1) == 0)
    *found = 1;
}

static bfd_boolean
pecoff_checksum_contents (bfd *abfd,
			  void (*process) (const void *, size_t, void *),
			  void *arg)
{
  file_ptr filepos = (file_ptr) 0;

  while (1)
    {
      unsigned char b;
      int status;

      if (bfd_seek (abfd, filepos, SEEK_SET) != 0)
	return 0;

      status = bfd_bread (&b, (bfd_size_type) 1, abfd);
      if (status < 1)
        {
          break;
        }

      (*process) (&b, 1, arg);
      filepos += 1;
    }

  return TRUE;
}

static bfd_boolean
write_build_id (bfd *abfd)
{
  struct pe_tdata *t = pe_data (abfd);
  asection *asec;
  struct bfd_link_order *link_order = NULL;
  unsigned char *contents;
  bfd_size_type size;
  bfd_size_type build_id_size;
  unsigned char *build_id;

  /* Find the section the .buildid output section has been merged info.  */
  for (asec = abfd->sections; asec != NULL; asec = asec->next)
    {
      struct bfd_link_order *l = NULL;
      for (l = asec->map_head.link_order; l != NULL; l = l->next)
        {
          if (l->type == bfd_indirect_link_order)
            {
              if (l->u.indirect.section == t->build_id.sec)
                {
                  link_order = l;
                  break;
                }
            }
        }

      if (link_order)
        break;
    }

  if (!link_order)
    {
      einfo (_("%P: warning: .buildid section discarded,"
               " --build-id ignored.\n"));
      return TRUE;
    }

  if (t->build_id.sec->contents == NULL)
    t->build_id.sec->contents = (unsigned char *) xmalloc (t->build_id.sec->size);
  contents = t->build_id.sec->contents;
  size = t->build_id.sec->size;

  build_id_size = compute_build_id_size (t->build_id.style);
  build_id = xmalloc (build_id_size);
  generate_build_id (abfd, t->build_id.style, pecoff_checksum_contents, build_id, build_id_size);

  bfd_vma ib = pe_data (link_info.output_bfd)->pe_opthdr.ImageBase;

  /* Construct a debug directory entry which points to an immediately following CodeView record.  */
  struct internal_IMAGE_DEBUG_DIRECTORY idd;
  idd.Characteristics = 0;
  idd.TimeDateStamp = 0;
  idd.MajorVersion = 0;
  idd.MinorVersion = 0;
  idd.Type = PE_IMAGE_DEBUG_TYPE_CODEVIEW;
  idd.SizeOfData = sizeof (CV_INFO_PDB70) + 1;
  idd.AddressOfRawData = asec->vma - ib + link_order->offset
    + sizeof (struct external_IMAGE_DEBUG_DIRECTORY);
  idd.PointerToRawData = asec->filepos + link_order->offset
    + sizeof (struct external_IMAGE_DEBUG_DIRECTORY);

  struct external_IMAGE_DEBUG_DIRECTORY *ext = (struct external_IMAGE_DEBUG_DIRECTORY *)contents;
  _bfd_XXi_swap_debugdir_out (abfd, &idd, ext);

  /* Write the debug directory entry.  */
  if (bfd_seek (abfd, asec->filepos + link_order->offset, SEEK_SET) != 0)
    return 0;

  if (bfd_bwrite (contents, size, abfd) != size)
    return 0;

  /* Construct the CodeView record.  */
  CODEVIEW_INFO cvinfo;
  cvinfo.CVSignature = CVINFO_PDB70_CVSIGNATURE;
  cvinfo.Age = 1;

  /* Zero pad or truncate the generated build_id to fit in the CodeView record.  */
  memset (&(cvinfo.Signature), 0, CV_INFO_SIGNATURE_LENGTH);
  memcpy (&(cvinfo.Signature), build_id, (build_id_size > CV_INFO_SIGNATURE_LENGTH)
	  ? CV_INFO_SIGNATURE_LENGTH :  build_id_size);

  free (build_id);

  /* Write the codeview record.  */
  if (_bfd_XXi_write_codeview_record (abfd, idd.PointerToRawData, &cvinfo) == 0)
    return 0;

  /* Record the location of the debug directory in the data directory.  */
  pe_data (link_info.output_bfd)->pe_opthdr.DataDirectory[PE_DEBUG_DATA].VirtualAddress
    = asec->vma  - ib + link_order->offset;
  pe_data (link_info.output_bfd)->pe_opthdr.DataDirectory[PE_DEBUG_DATA].Size
    = sizeof (struct external_IMAGE_DEBUG_DIRECTORY);

  return TRUE;
}

/* Make .buildid section, and set up coff_tdata->build_id. */
static bfd_boolean
setup_build_id (bfd *ibfd)
{
  asection *s;
  flagword flags;

  if (!validate_build_id_style (emit_build_id))
    {
      einfo ("%P: warning: unrecognized --build-id style ignored.\n");
      return FALSE;
    }

  flags = (SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD | SEC_IN_MEMORY
	   | SEC_LINKER_CREATED | SEC_READONLY | SEC_DATA);
  s = bfd_make_section_anyway_with_flags (ibfd, ".buildid", flags);
  if (s != NULL)
    {
      struct pe_tdata *t = pe_data (link_info.output_bfd);
      t->build_id.after_write_object_contents = &write_build_id;
      t->build_id.style = emit_build_id;
      t->build_id.sec = s;

      /* Section is a fixed size:
	 One IMAGE_DEBUG_DIRECTORY entry, of type IMAGE_DEBUG_TYPE_CODEVIEW,
	 pointing at a CV_INFO_PDB70 record containing the build-id, with a
	 null byte for PdbFileName.  */
      s->size = sizeof (struct external_IMAGE_DEBUG_DIRECTORY)
	+ sizeof (CV_INFO_PDB70) + 1;

      return TRUE;
    }

  einfo ("%P: warning: Cannot create .buildid section,"
	 " --build-id ignored.\n");
  return FALSE;
}

static void
gld_${EMULATION_NAME}_after_open (void)
{
  after_open_default ();

#ifdef DLL_SUPPORT
  if (pe_dll_extra_pe_debug)
    {
      bfd *a;
      struct bfd_link_hash_entry *sym;

      printf ("%s()\n", __FUNCTION__);

      for (sym = link_info.hash->undefs; sym; sym=sym->u.undef.next)
	printf ("-%s\n", sym->root.string);
      bfd_hash_traverse (&link_info.hash->table, pr_sym, NULL);

      for (a = link_info.input_bfds; a; a = a->link.next)
	printf ("*%s\n",a->filename);
    }
#endif

  if (emit_build_id != NULL)
    {
      bfd *abfd;

      /* Find a COFF input.  */
      for (abfd = link_info.input_bfds;
	   abfd != (bfd *) NULL; abfd = abfd->link.next)
	if (bfd_get_flavour (abfd) == bfd_target_coff_flavour)
	  break;

      /* If there are no COFF input files do not try to
	 add a build-id section.  */
      if (abfd == NULL
	  || !setup_build_id (abfd))
	{
	  free ((char *) emit_build_id);
	  emit_build_id = NULL;
	}
    }

  /* Pass the wacky PE command line options into the output bfd.
     FIXME: This should be done via a function, rather than by
     including an internal BFD header.  */

  if (coff_data (link_info.output_bfd) == NULL
      || coff_data (link_info.output_bfd)->pe == 0)
    einfo (_("%F%P: cannot perform PE operations on non PE output file '%B'.\n"),
	   link_info.output_bfd);

  pe_data (link_info.output_bfd)->pe_opthdr = pe;
  pe_data (link_info.output_bfd)->dll = init[DLLOFF].value;
  pe_data (link_info.output_bfd)->real_flags |= real_flags;
  pe_data (link_info.output_bfd)->insert_timestamp = insert_timestamp;

  /* At this point we must decide whether to use long section names
     in the output or not.  If the user hasn't explicitly specified
     on the command line, we leave it to the default for the format
     (object files yes, image files no), except if there is debug
     information present; GDB relies on the long section names to
     find it, so enable it in that case.  */
  if (pe_use_coff_long_section_names < 0 && link_info.strip == strip_none)
    {
      if (bfd_link_relocatable (&link_info))
	pe_use_coff_long_section_names = 1;
      else
	{
	  /* Iterate over all sections of all input BFDs, checking
	     for any that begin 'debug_' and are long names.  */
	  LANG_FOR_EACH_INPUT_STATEMENT (is)
	  {
	    int found_debug = 0;

	    bfd_map_over_sections (is->the_bfd, debug_section_p, &found_debug);
	    if (found_debug)
	      {
		pe_use_coff_long_section_names = 1;
		break;
	      }
	  }
	}
    }

  pe_output_file_set_long_section_names (link_info.output_bfd);

#ifdef DLL_SUPPORT
  if (pe_enable_stdcall_fixup) /* -1=warn or 1=disable */
    pe_fixup_stdcalls ();

  pe_process_import_defs (link_info.output_bfd, &link_info);

  pe_find_data_imports ();

  /* As possibly new symbols are added by imports, we rerun
     stdcall/fastcall fixup here.  */
  if (pe_enable_stdcall_fixup) /* -1=warn or 1=disable */
    pe_fixup_stdcalls ();

#if defined (TARGET_IS_i386pe) \
    || defined (TARGET_IS_armpe) \
    || defined (TARGET_IS_arm_epoc_pe) \
    || defined (TARGET_IS_arm_wince_pe)
  if (!bfd_link_relocatable (&link_info))
    pe_dll_build_sections (link_info.output_bfd, &link_info);
#else
  if (bfd_link_pic (&link_info))
    pe_dll_build_sections (link_info.output_bfd, &link_info);
  else
    pe_exe_build_sections (link_info.output_bfd, &link_info);
#endif
#endif /* DLL_SUPPORT */

#if defined(TARGET_IS_armpe) || defined(TARGET_IS_arm_epoc_pe) || defined(TARGET_IS_arm_wince_pe)
  if (strstr (bfd_get_target (link_info.output_bfd), "arm") == NULL)
    {
      /* The arm backend needs special fields in the output hash structure.
	 These will only be created if the output format is an arm format,
	 hence we do not support linking and changing output formats at the
	 same time.  Use a link followed by objcopy to change output formats.  */
      einfo ("%F%X%P: error: cannot change output format whilst linking ARM binaries\n");
      return;
    }
  {
    /* Find a BFD that can hold the interworking stubs.  */
    LANG_FOR_EACH_INPUT_STATEMENT (is)
      {
	if (bfd_arm_get_bfd_for_interworking (is->the_bfd, & link_info))
	  break;
      }
  }
#endif

  {
    /* This next chunk of code tries to detect the case where you have
       two import libraries for the same DLL (specifically,
       symbolically linking libm.a and libc.a in cygwin to
       libcygwin.a).  In those cases, it's possible for function
       thunks from the second implib to be used but without the
       head/tail objects, causing an improper import table.  We detect
       those cases and rename the "other" import libraries to match
       the one the head/tail come from, so that the linker will sort
       things nicely and produce a valid import table.  */

    LANG_FOR_EACH_INPUT_STATEMENT (is)
      {
	if (is->the_bfd->my_archive)
	  {
	    int idata2 = 0, reloc_count=0, is_imp = 0;
	    asection *sec;

	    /* See if this is an import library thunk.  */
	    for (sec = is->the_bfd->sections; sec; sec = sec->next)
	      {
		if (strcmp (sec->name, ".idata\$2") == 0)
		  idata2 = 1;
		if (CONST_STRNEQ (sec->name, ".idata\$"))
		  is_imp = 1;
		reloc_count += sec->reloc_count;
	      }

	    if (is_imp && !idata2 && reloc_count)
	      {
		/* It is, look for the reference to head and see if it's
		   from our own library.  */
		for (sec = is->the_bfd->sections; sec; sec = sec->next)
		  {
		    int i;
		    long relsize;
		    asymbol **symbols;
		    arelent **relocs;
		    int nrelocs;

		    relsize = bfd_get_reloc_upper_bound (is->the_bfd, sec);
		    if (relsize < 1)
		      break;

		    if (!bfd_generic_link_read_symbols (is->the_bfd))
		      {
			einfo (_("%B%F: could not read symbols: %E\n"),
			       is->the_bfd);
			return;
		      }
		    symbols = bfd_get_outsymbols (is->the_bfd);

		    relocs = xmalloc ((size_t) relsize);
		    nrelocs = bfd_canonicalize_reloc (is->the_bfd, sec,
						      relocs, symbols);
		    if (nrelocs < 0)
		      {
			free (relocs);
			einfo ("%X%P: unable to process relocs: %E\n");
			return;
		      }

		    for (i = 0; i < nrelocs; i++)
		      {
			struct bfd_symbol *s;
			struct bfd_link_hash_entry * blhe;
			char *other_bfd_filename;
			char *n;

			s = (relocs[i]->sym_ptr_ptr)[0];

			if (s->flags & BSF_LOCAL)
			  continue;

			/* Thunk section with reloc to another bfd.  */
			blhe = bfd_link_hash_lookup (link_info.hash,
						     s->name,
						     FALSE, FALSE, TRUE);

			if (blhe == NULL
			    || blhe->type != bfd_link_hash_defined)
			  continue;

			other_bfd_filename
			  = blhe->u.def.section->owner->my_archive
			    ? bfd_get_filename (blhe->u.def.section->owner->my_archive)
			    : bfd_get_filename (blhe->u.def.section->owner);

			if (filename_cmp (bfd_get_filename
					    (is->the_bfd->my_archive),
					  other_bfd_filename) == 0)
			  continue;

			/* Rename this implib to match the other one.  */
			n = xmalloc (strlen (other_bfd_filename) + 1);
			strcpy (n, other_bfd_filename);
			is->the_bfd->my_archive->filename = n;
		      }

		    free (relocs);
		    /* Note - we do not free the symbols,
		       they are now cached in the BFD.  */
		  }
	      }
	  }
      }
  }

  {
    int is_ms_arch = 0;
    bfd *cur_arch = 0;
    lang_input_statement_type *is2;
    lang_input_statement_type *is3;

    /* Careful - this is a shell script.  Watch those dollar signs! */
    /* Microsoft import libraries have every member named the same,
       and not in the right order for us to link them correctly.  We
       must detect these and rename the members so that they'll link
       correctly.  There are three types of objects: the head, the
       thunks, and the sentinel(s).  The head is easy; it's the one
       with idata2.  We assume that the sentinels won't have relocs,
       and the thunks will.  It's easier than checking the symbol
       table for external references.  */
    LANG_FOR_EACH_INPUT_STATEMENT (is)
      {
	if (is->the_bfd->my_archive)
	  {
	    char *pnt;
	    bfd *arch = is->the_bfd->my_archive;

	    if (cur_arch != arch)
	      {
		cur_arch = arch;
		is_ms_arch = 1;

		for (is3 = is;
		     is3 && is3->the_bfd->my_archive == arch;
		     is3 = (lang_input_statement_type *) is3->next)
		  {
		    /* A MS dynamic import library can also contain static
		       members, so look for the first element with a .dll
		       extension, and use that for the remainder of the
		       comparisons.  */
		    pnt = strrchr (is3->the_bfd->filename, '.');
		    if (pnt != NULL && filename_cmp (pnt, ".dll") == 0)
		      break;
		  }

		if (is3 == NULL)
		  is_ms_arch = 0;
		else
		  {
		    /* OK, found one.  Now look to see if the remaining
		       (dynamic import) members use the same name.  */
		    for (is2 = is;
			 is2 && is2->the_bfd->my_archive == arch;
			 is2 = (lang_input_statement_type *) is2->next)
		      {
			/* Skip static members, ie anything with a .obj
			   extension.  */
			pnt = strrchr (is2->the_bfd->filename, '.');
			if (pnt != NULL && filename_cmp (pnt, ".obj") == 0)
			  continue;

			if (filename_cmp (is3->the_bfd->filename,
					  is2->the_bfd->filename))
			  {
			    is_ms_arch = 0;
			    break;
			  }
		      }
		  }
	      }

	    /* This fragment might have come from an .obj file in a Microsoft
	       import, and not an actual import record. If this is the case,
	       then leave the filename alone.  */
	    pnt = strrchr (is->the_bfd->filename, '.');

	    if (is_ms_arch && (filename_cmp (pnt, ".dll") == 0))
	      {
		int idata2 = 0, reloc_count=0;
		asection *sec;
		char *new_name, seq;

		for (sec = is->the_bfd->sections; sec; sec = sec->next)
		  {
		    if (strcmp (sec->name, ".idata\$2") == 0)
		      idata2 = 1;
		    reloc_count += sec->reloc_count;
		  }

		if (idata2) /* .idata2 is the TOC */
		  seq = 'a';
		else if (reloc_count > 0) /* thunks */
		  seq = 'b';
		else /* sentinel */
		  seq = 'c';

		new_name = xmalloc (strlen (is->the_bfd->filename) + 3);
		sprintf (new_name, "%s.%c", is->the_bfd->filename, seq);
		is->the_bfd->filename = new_name;

		new_name = xmalloc (strlen (is->filename) + 3);
		sprintf (new_name, "%s.%c", is->filename, seq);
		is->filename = new_name;
	      }
	  }
      }
  }

  {
    /* The following chunk of code tries to identify jump stubs in
       import libraries which are dead code and eliminates them
       from the final link. For each exported symbol <sym>, there
       is a object file in the import library with a .text section
       and several .idata\$* sections. The .text section contains the
       symbol definition for <sym> which is a jump stub of the form
       jmp *__imp_<sym>. The .idata\$5 contains the symbol definition
       for __imp_<sym> which is the address of the slot for <sym> in
       the import address table. When a symbol is imported explicitly
       using __declspec(dllimport) declaration, the compiler generates
       a reference to __imp_<sym> which directly resolves to the
       symbol in .idata\$5, in which case the jump stub code is not
       needed. The following code tries to identify jump stub sections
       in import libraries which are not referred to by anyone and
       marks them for exclusion from the final link.  */
    LANG_FOR_EACH_INPUT_STATEMENT (is)
      {
	if (is->the_bfd->my_archive)
	  {
	    int is_imp = 0;
	    asection *sec, *stub_sec = NULL;

	    /* See if this is an import library thunk.  */
	    for (sec = is->the_bfd->sections; sec; sec = sec->next)
	      {
		if (strncmp (sec->name, ".idata\$", 7) == 0)
		  is_imp = 1;
		/* The section containing the jmp stub has code
		   and has a reloc.  */
		if ((sec->flags & SEC_CODE) && sec->reloc_count)
		  stub_sec = sec;
	      }

	    if (is_imp && stub_sec)
	      {
		asymbol **symbols;
		long nsyms, src_count;
		struct bfd_link_hash_entry * blhe;

		if (!bfd_generic_link_read_symbols (is->the_bfd))
		  {
		    einfo (_("%B%F: could not read symbols: %E\n"),
			   is->the_bfd);
		    return;
		  }
		symbols = bfd_get_outsymbols (is->the_bfd);
		nsyms = bfd_get_symcount (is->the_bfd);

		for (src_count = 0; src_count < nsyms; src_count++)
		  {
		    if (symbols[src_count]->section->id == stub_sec->id)
		      {
			/* This symbol belongs to the section containing
			   the stub.  */
			blhe = bfd_link_hash_lookup (link_info.hash,
						     symbols[src_count]->name,
						     FALSE, FALSE, TRUE);
			/* If the symbol in the stub section has no other
			   undefined references, exclude the stub section
			   from the final link.  */
			if (blhe != NULL
			    && blhe->type == bfd_link_hash_defined
			    && blhe->u.undef.next == NULL
			    && blhe != link_info.hash->undefs_tail)
			  stub_sec->flags |= SEC_EXCLUDE;
		      }
		  }
	      }
	  }
      }
  }
}

static void
gld_${EMULATION_NAME}_before_allocation (void)
{
#ifdef TARGET_IS_ppcpe
  /* Here we rummage through the found bfds to collect toc information.  */
  {
    LANG_FOR_EACH_INPUT_STATEMENT (is)
      {
	if (!ppc_process_before_allocation (is->the_bfd, &link_info))
	  {
	    /* xgettext:c-format */
	    einfo (_("Errors encountered processing file %s\n"), is->filename);
	  }
      }
  }

  /* We have seen it all. Allocate it, and carry on.  */
  ppc_allocate_toc_section (&link_info);
#endif /* TARGET_IS_ppcpe */

#if defined(TARGET_IS_armpe) || defined(TARGET_IS_arm_epoc_pe) || defined(TARGET_IS_arm_wince_pe)
  /* FIXME: we should be able to set the size of the interworking stub
     section.

     Here we rummage through the found bfds to collect glue
     information.  FIXME: should this be based on a command line
     option?  krk@cygnus.com.  */
  {
    LANG_FOR_EACH_INPUT_STATEMENT (is)
      {
	if (! bfd_arm_process_before_allocation
	    (is->the_bfd, & link_info, support_old_code))
	  {
	    /* xgettext:c-format */
	    einfo (_("Errors encountered processing file %s for interworking\n"),
		   is->filename);
	  }
      }
  }

  /* We have seen it all. Allocate it, and carry on.  */
  bfd_arm_allocate_interworking_sections (& link_info);
#endif /* TARGET_IS_armpe || TARGET_IS_arm_epoc_pe || TARGET_IS_arm_wince_pe */

  before_allocation_default ();
}

#ifdef DLL_SUPPORT
/* This is called when an input file isn't recognized as a BFD.  We
   check here for .DEF files and pull them in automatically.  */

static int
saw_option (char *option)
{
  int i;

  for (i = 0; init[i].ptr; i++)
    if (strcmp (GET_INIT_SYMBOL_NAME (i), option) == 0)
      return init[i].inited;
  return 0;
}
#endif /* DLL_SUPPORT */

static bfd_boolean
gld_${EMULATION_NAME}_unrecognized_file (lang_input_statement_type *entry ATTRIBUTE_UNUSED)
{
#ifdef DLL_SUPPORT
  const char *ext = entry->filename + strlen (entry->filename) - 4;

  if (filename_cmp (ext, ".def") == 0 || filename_cmp (ext, ".DEF") == 0)
    {
      pe_def_file = def_file_parse (entry->filename, pe_def_file);

      if (pe_def_file)
	{
	  int i, buflen=0, len;
	  char *buf;

	  for (i = 0; i < pe_def_file->num_exports; i++)
	    {
	      len = strlen (pe_def_file->exports[i].internal_name);
	      if (buflen < len + 2)
		buflen = len + 2;
	    }

	  buf = xmalloc (buflen);

	  for (i = 0; i < pe_def_file->num_exports; i++)
	    {
	      struct bfd_link_hash_entry *h;

	      sprintf (buf, "%s%s", U (""),
	               pe_def_file->exports[i].internal_name);

	      h = bfd_link_hash_lookup (link_info.hash, buf, TRUE, TRUE, TRUE);
	      if (h == (struct bfd_link_hash_entry *) NULL)
		einfo (_("%P%F: bfd_link_hash_lookup failed: %E\n"));
	      if (h->type == bfd_link_hash_new)
		{
		  h->type = bfd_link_hash_undefined;
		  h->u.undef.abfd = NULL;
		  bfd_link_add_undef (link_info.hash, h);
		}
	    }
	  free (buf);

	  /* def_file_print (stdout, pe_def_file); */
	  if (pe_def_file->is_dll == 1)
	    link_info.type = type_dll;

	  if (pe_def_file->base_address != (bfd_vma)(-1))
	    {
	      pe.ImageBase
		= pe_data (link_info.output_bfd)->pe_opthdr.ImageBase
		= init[IMAGEBASEOFF].value
		= pe_def_file->base_address;
	      init[IMAGEBASEOFF].inited = 1;
	      if (image_base_statement)
		image_base_statement->exp
		  = exp_assign ("__image_base__", exp_intop (pe.ImageBase),
				FALSE);
	    }

	  if (pe_def_file->stack_reserve != -1
	      && ! saw_option ("__size_of_stack_reserve__"))
	    {
	      pe.SizeOfStackReserve = pe_def_file->stack_reserve;
	      if (pe_def_file->stack_commit != -1)
		pe.SizeOfStackCommit = pe_def_file->stack_commit;
	    }
	  if (pe_def_file->heap_reserve != -1
	      && ! saw_option ("__size_of_heap_reserve__"))
	    {
	      pe.SizeOfHeapReserve = pe_def_file->heap_reserve;
	      if (pe_def_file->heap_commit != -1)
		pe.SizeOfHeapCommit = pe_def_file->heap_commit;
	    }
	  return TRUE;
	}
    }
#endif
  return FALSE;
}

static bfd_boolean
gld_${EMULATION_NAME}_recognized_file (lang_input_statement_type *entry ATTRIBUTE_UNUSED)
{
#ifdef DLL_SUPPORT
#ifdef TARGET_IS_i386pe
  pe_dll_id_target ("pei-i386");
#endif
#ifdef TARGET_IS_shpe
  pe_dll_id_target ("pei-shl");
#endif
#ifdef TARGET_IS_armpe
  pe_dll_id_target ("pei-arm-little");
#endif
#ifdef TARGET_IS_arm_epoc_pe
  pe_dll_id_target ("epoc-pei-arm-little");
#endif
#ifdef TARGET_IS_arm_wince_pe
  pe_dll_id_target ("pei-arm-wince-little");
#endif
  if (pe_bfd_is_dll (entry->the_bfd))
    return pe_implied_import_dll (entry->filename);
#endif
  return FALSE;
}

static void
gld_${EMULATION_NAME}_finish (void)
{
#if defined(TARGET_IS_armpe) || defined(TARGET_IS_arm_epoc_pe) || defined(TARGET_IS_arm_wince_pe)
  struct bfd_link_hash_entry * h;

  if (thumb_entry_symbol != NULL)
    {
      h = bfd_link_hash_lookup (link_info.hash, thumb_entry_symbol,
				FALSE, FALSE, TRUE);

      if (h != (struct bfd_link_hash_entry *) NULL
	  && (h->type == bfd_link_hash_defined
	      || h->type == bfd_link_hash_defweak)
	  && h->u.def.section->output_section != NULL)
	{
	  static char buffer[32];
	  bfd_vma val;

	  /* Special procesing is required for a Thumb entry symbol.  The
	     bottom bit of its address must be set.  */
	  val = (h->u.def.value
		 + bfd_get_section_vma (link_info.output_bfd,
					h->u.def.section->output_section)
		 + h->u.def.section->output_offset);

	  val |= 1;

	  /* Now convert this value into a string and store it in entry_symbol
	     where the lang_finish() function will pick it up.  */
	  buffer[0] = '0';
	  buffer[1] = 'x';

	  sprintf_vma (buffer + 2, val);

	  if (entry_symbol.name != NULL && entry_from_cmdline)
	    einfo (_("%P: warning: '--thumb-entry %s' is overriding '-e %s'\n"),
		   thumb_entry_symbol, entry_symbol.name);
	  entry_symbol.name = buffer;
	}
      else
	einfo (_("%P: warning: cannot find thumb start symbol %s\n"), thumb_entry_symbol);
    }
#endif /* defined(TARGET_IS_armpe) || defined(TARGET_IS_arm_epoc_pe) || defined(TARGET_IS_arm_wince_pe) */

  finish_default ();

#ifdef DLL_SUPPORT
  if (bfd_link_pic (&link_info)
#if !defined(TARGET_IS_shpe)
      || (!bfd_link_relocatable (&link_info)
	  && pe_def_file->num_exports != 0)
#endif
    )
    {
      pe_dll_fill_sections (link_info.output_bfd, &link_info);
      if (pe_implib_filename)
	pe_dll_generate_implib (pe_def_file, pe_implib_filename, &link_info);
    }
#if defined(TARGET_IS_shpe)
  /* ARM doesn't need relocs.  */
  else
    {
      pe_exe_fill_sections (link_info.output_bfd, &link_info);
    }
#endif

  if (pe_out_def_filename)
    pe_dll_generate_def_file (pe_out_def_filename);
#endif /* DLL_SUPPORT */

  /* I don't know where .idata gets set as code, but it shouldn't be.  */
  {
    asection *asec = bfd_get_section_by_name (link_info.output_bfd, ".idata");

    if (asec)
      {
	asec->flags &= ~SEC_CODE;
	asec->flags |= SEC_DATA;
      }
  }
}


/* Place an orphan section.

   We use this to put sections in a reasonable place in the file, and
   to ensure that they are aligned as required.

   We handle grouped sections here as well.  A section named .foo\$nn
   goes into the output section .foo.  All grouped sections are sorted
   by name.

   Grouped sections for the default sections are handled by the
   default linker script using wildcards, and are sorted by
   sort_sections.  */

static lang_output_section_statement_type *
gld_${EMULATION_NAME}_place_orphan (asection *s,
				    const char *secname,
				    int constraint)
{
  const char *orig_secname = secname;
  char *dollar = NULL;
  lang_output_section_statement_type *os;
  lang_statement_list_type add_child;
  lang_output_section_statement_type *match_by_name = NULL;
  lang_statement_union_type **pl;

  /* Look through the script to see where to place this section.  */
  if (!bfd_link_relocatable (&link_info)
      && (dollar = strchr (secname, '\$')) != NULL)
    {
      size_t len = dollar - secname;
      char *newname = xmalloc (len + 1);
      memcpy (newname, secname, len);
      newname[len] = '\0';
      secname = newname;
    }

  lang_list_init (&add_child);

  os = NULL;
  if (constraint == 0)
    for (os = lang_output_section_find (secname);
	 os != NULL;
	 os = next_matching_output_section_statement (os, 0))
      {
	/* If we don't match an existing output section, tell
	   lang_insert_orphan to create a new output section.  */
	constraint = SPECIAL;

	if (os->bfd_section != NULL
	    && (os->bfd_section->flags == 0
		|| ((s->flags ^ os->bfd_section->flags)
		    & (SEC_LOAD | SEC_ALLOC)) == 0))
	  {
	    /* We already have an output section statement with this
	       name, and its bfd section has compatible flags.
	       If the section already exists but does not have any flags set,
	       then it has been created by the linker, probably as a result of
	       a --section-start command line switch.  */
	    lang_add_section (&add_child, s, NULL, os);
	    break;
	  }

	/* Save unused output sections in case we can match them
	   against orphans later.  */
	if (os->bfd_section == NULL)
	  match_by_name = os;
      }

  /* If we didn't match an active output section, see if we matched an
     unused one and use that.  */
  if (os == NULL && match_by_name)
    {
      lang_add_section (&match_by_name->children, s, NULL, match_by_name);
      return match_by_name;
    }

  if (os == NULL)
    {
      static struct orphan_save hold[] =
	{
	  { ".text",
	    SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD | SEC_READONLY | SEC_CODE,
	    0, 0, 0, 0 },
	  { ".idata",
	    SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD | SEC_READONLY | SEC_DATA,
	    0, 0, 0, 0 },
	  { ".rdata",
	    SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD | SEC_READONLY | SEC_DATA,
	    0, 0, 0, 0 },
	  { ".data",
	    SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD | SEC_DATA,
	    0, 0, 0, 0 },
	  { ".bss",
	    SEC_ALLOC,
	    0, 0, 0, 0 }
	};
      enum orphan_save_index
	{
	  orphan_text = 0,
	  orphan_idata,
	  orphan_rodata,
	  orphan_data,
	  orphan_bss
	};
      static int orphan_init_done = 0;
      struct orphan_save *place;
      lang_output_section_statement_type *after;
      etree_type *address;
      flagword flags;
      asection *nexts;

      if (!orphan_init_done)
	{
	  struct orphan_save *ho;
	  for (ho = hold; ho < hold + sizeof (hold) / sizeof (hold[0]); ++ho)
	    if (ho->name != NULL)
	      {
		ho->os = lang_output_section_find (ho->name);
		if (ho->os != NULL && ho->os->flags == 0)
		  ho->os->flags = ho->flags;
	      }
	  orphan_init_done = 1;
	}

      flags = s->flags;
      if (!bfd_link_relocatable (&link_info))
	{
	  nexts = s;
	  while ((nexts = bfd_get_next_section_by_name (nexts->owner,
							nexts)))
	    if (nexts->output_section == NULL
		&& (nexts->flags & SEC_EXCLUDE) == 0
		&& ((nexts->flags ^ flags) & (SEC_LOAD | SEC_ALLOC)) == 0
		&& (nexts->owner->flags & DYNAMIC) == 0
		&& nexts->owner->usrdata != NULL
		&& !(((lang_input_statement_type *) nexts->owner->usrdata)
		     ->flags.just_syms))
	      flags = (((flags ^ SEC_READONLY)
			| (nexts->flags ^ SEC_READONLY))
		       ^ SEC_READONLY);
	}

      /* Try to put the new output section in a reasonable place based
	 on the section name and section flags.  */

      place = NULL;
      if ((flags & SEC_ALLOC) == 0)
	;
      else if ((flags & (SEC_LOAD | SEC_HAS_CONTENTS)) == 0)
	place = &hold[orphan_bss];
      else if ((flags & SEC_READONLY) == 0)
	place = &hold[orphan_data];
      else if ((flags & SEC_CODE) == 0)
	{
	  place = (!strncmp (secname, ".idata\$", 7) ? &hold[orphan_idata]
						     : &hold[orphan_rodata]);
	}
      else
	place = &hold[orphan_text];

      after = NULL;
      if (place != NULL)
	{
	  if (place->os == NULL)
	    place->os = lang_output_section_find (place->name);
	  after = place->os;
	  if (after == NULL)
	    after = lang_output_section_find_by_flags (s, flags, &place->os,
						       NULL);
	  if (after == NULL)
	    /* *ABS* is always the first output section statement.  */
	    after = (&lang_output_section_statement.head
		     ->output_section_statement);
	}

      /* All sections in an executable must be aligned to a page boundary.
	 In a relocatable link, just preserve the incoming alignment; the
	 address is discarded by lang_insert_orphan in that case, anyway.  */
      address = exp_unop (ALIGN_K, exp_nameop (NAME, "__section_alignment__"));
      os = lang_insert_orphan (s, secname, constraint, after, place, address,
			       &add_child);
      if (bfd_link_relocatable (&link_info))
	{
	  os->section_alignment = s->alignment_power;
	  os->bfd_section->alignment_power = s->alignment_power;
	}
    }

  /* If the section name has a '\$', sort it with the other '\$'
     sections.  */
  for (pl = &os->children.head; *pl != NULL; pl = &(*pl)->header.next)
    {
      lang_input_section_type *ls;
      const char *lname;

      if ((*pl)->header.type != lang_input_section_enum)
	continue;

      ls = &(*pl)->input_section;

      lname = bfd_get_section_name (ls->section->owner, ls->section);
      if (strchr (lname, '\$') != NULL
	  && (dollar == NULL || strcmp (orig_secname, lname) < 0))
	break;
    }

  if (add_child.head != NULL)
    {
      *add_child.tail = *pl;
      *pl = add_child.head;
    }

  return os;
}

static bfd_boolean
gld_${EMULATION_NAME}_open_dynamic_archive
  (const char *arch ATTRIBUTE_UNUSED,
   search_dirs_type *search,
   lang_input_statement_type *entry)
{
  static const struct
    {
      const char * format;
      bfd_boolean use_prefix;
    }
  libname_fmt [] =
    {
      /* Preferred explicit import library for dll's.  */
      { "lib%s.dll.a", FALSE },
      /* Alternate explicit import library for dll's.  */
      { "%s.dll.a", FALSE },
      /* "libfoo.a" could be either an import lib or a static lib.
          For backwards compatibility, libfoo.a needs to precede
          libfoo.dll and foo.dll in the search.  */
      { "lib%s.a", FALSE },
      /* The 'native' spelling of an import lib name is "foo.lib".  */
      { "%s.lib", FALSE },
#ifdef DLL_SUPPORT
      /* Try "<prefix>foo.dll" (preferred dll name, if specified).  */
      {	"%s%s.dll", TRUE },
#endif
      /* Try "libfoo.dll" (default preferred dll name).  */
      {	"lib%s.dll", FALSE },
      /* Finally try 'native' dll name "foo.dll".  */
      {  "%s.dll", FALSE },
      /* Note: If adding more formats to this table, make sure to check to
	 see if their length is longer than libname_fmt[0].format, and if
	 so, update the call to xmalloc() below.  */
      { NULL, FALSE }
    };
  static unsigned int format_max_len = 0;
  const char * filename;
  char * full_string;
  char * base_string;
  unsigned int i;


  if (! entry->flags.maybe_archive || entry->flags.full_name_provided)
    return FALSE;

  filename = entry->filename;

  if (format_max_len == 0)
    /* We need to allow space in the memory that we are going to allocate
       for the characters in the format string.  Since the format array is
       static we only need to calculate this information once.  In theory
       this value could also be computed statically, but this introduces
       the possibility for a discrepancy and hence a possible memory
       corruption.  The lengths we compute here will be too long because
       they will include any formating characters (%s) in the strings, but
       this will not matter.  */
    for (i = 0; libname_fmt[i].format; i++)
      if (format_max_len < strlen (libname_fmt[i].format))
	format_max_len = strlen (libname_fmt[i].format);

  full_string = xmalloc (strlen (search->name)
			 + strlen (filename)
			 + format_max_len
#ifdef DLL_SUPPORT
			 + (pe_dll_search_prefix
			    ? strlen (pe_dll_search_prefix) : 0)
#endif
			 /* Allow for the terminating NUL and for the path
			    separator character that is inserted between
			    search->name and the start of the format string.  */
			 + 2);

  sprintf (full_string, "%s/", search->name);
  base_string = full_string + strlen (full_string);

  for (i = 0; libname_fmt[i].format; i++)
    {
#ifdef DLL_SUPPORT
      if (libname_fmt[i].use_prefix)
	{
	  if (!pe_dll_search_prefix)
	    continue;
	  sprintf (base_string, libname_fmt[i].format, pe_dll_search_prefix, filename);
	}
      else
#endif
	sprintf (base_string, libname_fmt[i].format, filename);

      if (ldfile_try_open_bfd (full_string, entry))
	break;
    }

  if (!libname_fmt[i].format)
    {
      free (full_string);
      return FALSE;
    }

  entry->filename = full_string;

  return TRUE;
}

static int
gld_${EMULATION_NAME}_find_potential_libraries
  (char *name, lang_input_statement_type *entry)
{
  return ldfile_open_file_search (name, entry, "", ".lib");
}

static char *
gld_${EMULATION_NAME}_get_script (int *isfile)
EOF
# Scripts compiled in.
# sed commands to quote an ld script as a C string.
sc="-f stringify.sed"

fragment <<EOF
{
  *isfile = 0;

  if (bfd_link_relocatable (&link_info) && config.build_constructors)
    return
EOF
sed $sc ldscripts/${EMULATION_NAME}.xu			>> e${EMULATION_NAME}.c
echo '  ; else if (bfd_link_relocatable (&link_info)) return' >> e${EMULATION_NAME}.c
sed $sc ldscripts/${EMULATION_NAME}.xr			>> e${EMULATION_NAME}.c
echo '  ; else if (!config.text_read_only) return'	>> e${EMULATION_NAME}.c
sed $sc ldscripts/${EMULATION_NAME}.xbn			>> e${EMULATION_NAME}.c
echo '  ; else if (!config.magic_demand_paged) return'	>> e${EMULATION_NAME}.c
sed $sc ldscripts/${EMULATION_NAME}.xn			>> e${EMULATION_NAME}.c
if test -n "$GENERATE_AUTO_IMPORT_SCRIPT" ; then
echo '  ; else if (link_info.pei386_auto_import == 1 && (MERGE_RDATA_V2 || link_info.pei386_runtime_pseudo_reloc != 2)) return'	>> e${EMULATION_NAME}.c
sed $sc ldscripts/${EMULATION_NAME}.xa			>> e${EMULATION_NAME}.c
fi
echo '  ; else return'					>> e${EMULATION_NAME}.c
sed $sc ldscripts/${EMULATION_NAME}.x			>> e${EMULATION_NAME}.c
echo '; }'						>> e${EMULATION_NAME}.c

fragment <<EOF


struct ld_emulation_xfer_struct ld_${EMULATION_NAME}_emulation =
{
  gld_${EMULATION_NAME}_before_parse,
  syslib_default,
  hll_default,
  gld_${EMULATION_NAME}_after_parse,
  gld_${EMULATION_NAME}_after_open,
  after_allocation_default,
  set_output_arch_default,
  ldemul_default_target,
  gld_${EMULATION_NAME}_before_allocation,
  gld_${EMULATION_NAME}_get_script,
  "${EMULATION_NAME}",
  "${OUTPUT_FORMAT}",
  gld_${EMULATION_NAME}_finish,
  NULL, /* Create output section statements.  */
  gld_${EMULATION_NAME}_open_dynamic_archive,
  gld_${EMULATION_NAME}_place_orphan,
  gld_${EMULATION_NAME}_set_symbols,
  NULL, /* parse_args */
  gld${EMULATION_NAME}_add_options,
  gld${EMULATION_NAME}_handle_option,
  gld_${EMULATION_NAME}_unrecognized_file,
  gld_${EMULATION_NAME}_list_options,
  gld_${EMULATION_NAME}_recognized_file,
  gld_${EMULATION_NAME}_find_potential_libraries,
  NULL,	/* new_vers_pattern.  */
  NULL	/* extra_map_file_text.  */
};
EOF
