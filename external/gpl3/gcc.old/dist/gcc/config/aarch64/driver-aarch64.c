/* Native CPU detection for aarch64.
   Copyright (C) 2015-2020 Free Software Foundation, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#define IN_TARGET_CODE 1

#include "config.h"
#define INCLUDE_STRING
#define INCLUDE_SET
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "diagnostic-core.h"

/* Defined in common/config/aarch64/aarch64-common.c.  */
std::string aarch64_get_extension_string_for_isa_flags (uint64_t, uint64_t);

struct aarch64_arch_extension
{
  const char *ext;
  uint64_t flag;
  const char *feat_string;
};

#define AARCH64_OPT_EXTENSION(EXT_NAME, FLAG_CANONICAL, FLAGS_ON, FLAGS_OFF, \
			      SYNTHETIC, FEATURE_STRING) \
  { EXT_NAME, FLAG_CANONICAL, FEATURE_STRING },
static struct aarch64_arch_extension aarch64_extensions[] =
{
#include "aarch64-option-extensions.def"
};


struct aarch64_core_data
{
  const char* name;
  const char* arch;
  unsigned char implementer_id; /* Exactly 8 bits */
  unsigned int part_no; /* 12 bits + 12 bits */
  unsigned variant;
  const uint64_t flags;
};

#define AARCH64_BIG_LITTLE(BIG, LITTLE) \
  (((BIG)&0xFFFu) << 12 | ((LITTLE) & 0xFFFu))
#define INVALID_IMP ((unsigned char) -1)
#define INVALID_CORE ((unsigned)-1)
#define ALL_VARIANTS ((unsigned)-1)

#define AARCH64_CORE(CORE_NAME, CORE_IDENT, SCHED, ARCH, FLAGS, COSTS, IMP, PART, VARIANT) \
  { CORE_NAME, #ARCH, IMP, PART, VARIANT, FLAGS },

static struct aarch64_core_data aarch64_cpu_data[] =
{
#include "aarch64-cores.def"
  { NULL, NULL, INVALID_IMP, INVALID_CORE, ALL_VARIANTS, 0 }
};


struct aarch64_arch_driver_info
{
  const char* id;
  const char* name;
  const uint64_t flags;
};

#define AARCH64_ARCH(NAME, CORE, ARCH_IDENT, ARCH_REV, FLAGS) \
  { #ARCH_IDENT, NAME, FLAGS },

static struct aarch64_arch_driver_info aarch64_arches[] =
{
#include "aarch64-arches.def"
  {NULL, NULL, 0}
};


/* Return an aarch64_arch_driver_info for the architecture described
   by ID, or NULL if ID describes something we don't know about.  */

static struct aarch64_arch_driver_info*
get_arch_from_id (const char* id)
{
  unsigned int i = 0;

  for (i = 0; aarch64_arches[i].id != NULL; i++)
    {
      if (strcmp (id, aarch64_arches[i].id) == 0)
	return &aarch64_arches[i];
    }

  return NULL;
}

/* Check wether the CORE array is the same as the big.LITTLE BL_CORE.
   For an example CORE={0xd08, 0xd03} and
   BL_CORE=AARCH64_BIG_LITTLE (0xd08, 0xd03) will return true.  */

static bool
valid_bL_core_p (unsigned int *core, unsigned int bL_core)
{
  return AARCH64_BIG_LITTLE (core[0], core[1]) == bL_core
         || AARCH64_BIG_LITTLE (core[1], core[0]) == bL_core;
}

/* Returns the hex integer that is after ':' for the FIELD.
   Returns -1 is returned if there was problem parsing the integer. */
static unsigned
parse_field (const std::string &field)
{
  const char *rest = strchr (field.c_str (), ':');

  /* The line must be in the format of <name>:<value>, if it's not
     then we have a weird format.  */
  if (rest == NULL)
    return -1;

  char *after;
  unsigned fint = strtol (rest + 1, &after, 16);
  if (after == rest + 1)
    return -1;
  return fint;
}

/* Returns the index of the ':' inside the FIELD which must be found
   after the value of KEY.  Returns string::npos if line does not contain
   a field.  */

static size_t
find_field (const std::string &field, const std::string &key)
{
  size_t key_pos, sep_pos;
  key_pos = field.find (key);
  if (key_pos == std::string::npos)
    return std::string::npos;

  sep_pos = field.find (":", key_pos + 1);
  if (sep_pos == std::string::npos)
    return std::string::npos;

  return sep_pos;
}

/* Splits and returns a string based on whitespace and return it as
   part of a set. Empty strings are ignored.  */

static void
split_words (const std::string &val, std::set<std::string> &result)
{
  size_t cur, prev = 0;
  std::string word;
  while ((cur = val.find_first_of (" \n", prev)) != std::string::npos)
    {
      word = val.substr (prev, cur - prev);
      /* Skip adding empty words.  */
      if (!word.empty ())
	result.insert (word);
      prev = cur + 1;
    }

  if (prev != cur)
    result.insert (val.substr (prev));
}

/* Read an entire line from F until '\n' or EOF.  */

static std::string
readline (FILE *f)
{
  char *buf = NULL;
  int size = 0;
  int last = 0;
  const int buf_size = 128;

  if (feof (f))
    return std::string ();

  do
    {
      size += buf_size;
      buf = (char*) xrealloc (buf, size);
      gcc_assert (buf);
      /* If fgets fails it returns NULL, but if it reaches EOF
	 with 0 characters read it also returns EOF.  However
	 the condition on the loop would have broken out of the
	 loop in that case,  and if we are in the first iteration
	 then the empty string is the correct thing to return.  */
      if (!fgets (buf + last, buf_size, f))
	return std::string ();
      /* If we're not at the end of the line then override the
	 \0 added by fgets.  */
      last = strnlen (buf, size) - 1;
    }
  while (!feof (f) && buf[last] != '\n');

  std::string result (buf);
  free (buf);
  return result;
}

/*  Return true iff ARR contains CORE, in either of the two elements. */

static bool
contains_core_p (unsigned *arr, unsigned core)
{
  if (arr[0] != INVALID_CORE)
    {
      if (arr[0] == core)
        return true;

      if (arr[1] != INVALID_CORE)
        return arr[1] == core;
    }

  return false;
}

/* This will be called by the spec parser in gcc.c when it sees
   a %:local_cpu_detect(args) construct.  Currently it will be called
   with either "arch", "cpu" or "tune" as argument depending on if
   -march=native, -mcpu=native or -mtune=native is to be substituted.

   It returns a string containing new command line parameters to be
   put at the place of the above two options, depending on what CPU
   this is executed.  E.g. "-march=armv8-a" on a Cortex-A57 for
   -march=native.  If the routine can't detect a known processor,
   the -march or -mtune option is discarded.

   For -mtune and -mcpu arguments it attempts to detect the CPU or
   a big.LITTLE system.
   ARGC and ARGV are set depending on the actual arguments given
   in the spec.  */

#ifdef __NetBSD__
/* The NetBSD/arm64 platform may not export linux-style /proc/cpuinfo,
   but the data is available via a sysctl(3) interface.  */
#include <sys/param.h>
#include <sys/sysctl.h>
#include <aarch64/armreg.h>
#endif

const char *
host_detect_local_cpu (int argc, const char **argv)
{
  const char *res = NULL;
  static const int num_exts = ARRAY_SIZE (aarch64_extensions);
  FILE *f = NULL;
  bool arch = false;
  bool tune = false;
  bool cpu = false;
  unsigned int i = 0;
  unsigned char imp = INVALID_IMP;
  unsigned int cores[2] = { INVALID_CORE, INVALID_CORE };
  unsigned int n_cores = 0;
  unsigned int variants[2] = { ALL_VARIANTS, ALL_VARIANTS };
  unsigned int n_variants = 0;
  bool processed_exts = false;
  uint64_t extension_flags = 0;
  uint64_t default_flags = 0;
  std::string buf;
  size_t sep_pos = -1;
  char *fcpu_info;

  gcc_assert (argc);

  if (!argv[0])
    goto not_found;

  /* Are we processing -march, mtune or mcpu?  */
  arch = strcmp (argv[0], "arch") == 0;
  if (!arch)
    tune = strcmp (argv[0], "tune") == 0;

  if (!arch && !tune)
    cpu = strcmp (argv[0], "cpu") == 0;

  if (!arch && !tune && !cpu)
    goto not_found;

#ifndef __NetBSD__
  fcpu_info = getenv ("GCC_CPUINFO");
  if (fcpu_info)
    f = fopen (fcpu_info, "r");
  else
    f = fopen ("/proc/cpuinfo", "r");

  if (f == NULL)
    goto not_found;

  /* Look through /proc/cpuinfo to determine the implementer
     and then the part number that identifies a particular core.  */
  while (!(buf = readline (f)).empty ())
    {
      if (find_field (buf, "implementer") != std::string::npos)
	{
	  unsigned cimp = parse_field (buf);
	  if (cimp == INVALID_IMP)
	    goto not_found;

	  if (imp == INVALID_IMP)
	    imp = cimp;
	  /* FIXME: BIG.little implementers are always equal. */
	  else if (imp != cimp)
	    goto not_found;
	}
      else if (find_field (buf, "variant") != std::string::npos)
	{
	  unsigned cvariant = parse_field (buf);
	  if (!contains_core_p (variants, cvariant))
	    {
              if (n_variants == 2)
                goto not_found;

              variants[n_variants++] = cvariant;
	    }
          continue;
        }
      else if (find_field (buf, "part") != std::string::npos)
	{
	  unsigned ccore = parse_field (buf);
	  if (!contains_core_p (cores, ccore))
	    {
	      if (n_cores == 2)
		goto not_found;

	      cores[n_cores++] = ccore;
	    }
	  continue;
	}
      else if (!tune && !processed_exts
	       && (sep_pos = find_field (buf, "Features")) != std::string::npos)
	{
	  /* First create the list of features in the buffer.  */
	  std::set<std::string> features;
	  /* Drop everything till the :.  */
	  buf = buf.substr (sep_pos + 1);
	  split_words (buf, features);

	  for (i = 0; i < num_exts; i++)
	    {
	      const std::string val (aarch64_extensions[i].feat_string);

	      /* If the feature contains no HWCAPS string then ignore it for the
		 auto detection.  */
	      if (val.empty ())
		continue;

	      bool enabled = true;

	      /* This may be a multi-token feature string.  We need
		 to match all parts, which could be in any order.  */
	      std::set<std::string> tokens;
	      split_words (val, tokens);
	      std::set<std::string>::iterator it;

	      /* Iterate till the first feature isn't found or all of them
		 are found.  */
	      for (it = tokens.begin (); enabled && it != tokens.end (); ++it)
		enabled = enabled && features.count (*it);

	      if (enabled)
		extension_flags |= aarch64_extensions[i].flag;
	      else
		extension_flags &= ~(aarch64_extensions[i].flag);
	    }

	  processed_exts = true;
	}
    }

  fclose (f);
  f = NULL;
#else
  unsigned int curcpu;
  size_t len;
  char impl_buf[8];
  int mib[2], ncpu;

  mib[0] = CTL_HW;
  mib[1] = HW_NCPU; 
  len = sizeof(ncpu);
  if (sysctl(mib, 2, &ncpu, &len, NULL, 0) == -1)
    goto not_found;

  for (curcpu = 0; curcpu < ncpu; curcpu++)
    {
      char path[128];
      struct aarch64_sysctl_cpu_id id;

      len = sizeof id;
      snprintf(path, sizeof path, "machdep.cpu%d.cpu_id", curcpu);
      if (sysctlbyname(path, &id, &len, NULL, 0) != 0)
        goto not_found;

      unsigned cimp = __SHIFTOUT(id.ac_midr, MIDR_EL1_IMPL);
      if (cimp == INVALID_IMP)
        goto not_found;

      if (imp == INVALID_IMP)
        imp = cimp;
	/* FIXME: BIG.little implementers are always equal. */
      else if (imp != cimp)
        goto not_found;
  
      unsigned cvariant = __SHIFTOUT(id.ac_midr, MIDR_EL1_VARIANT);
      if (!contains_core_p (variants, cvariant))
        {
          if (n_variants == 2)
            goto not_found;
  
          variants[n_variants++] = cvariant;
  	}

      unsigned ccore = __SHIFTOUT(id.ac_midr, MIDR_EL1_PARTNUM);
      if (!contains_core_p (cores, ccore))
  	{
  	  if (n_cores == 2)
  	    goto not_found;
  
  	  cores[n_cores++] = ccore;
  	}

      if (!tune && !processed_exts)
        {
          std::string exts;

	  /* These are all the extensions from aarch64-option-extensions.def.  */
          if (__SHIFTOUT(id.ac_aa64pfr0, ID_AA64PFR0_EL1_FP) == ID_AA64PFR0_EL1_FP_IMPL)
	    exts += "fp ";
          if (__SHIFTOUT(id.ac_aa64pfr0, ID_AA64PFR0_EL1_ADVSIMD) == ID_AA64PFR0_EL1_ADV_SIMD_IMPL)
	    exts += "asimd ";
#ifdef ID_AA64ISAR0_EL1_RDM
          if (__SHIFTOUT(id.ac_aa64isar0, ID_AA64ISAR0_EL1_RDM) == ID_AA64ISAR0_EL1_RDM_SQRDML)
	    exts += "asimdrdm ";
          if (__SHIFTOUT(id.ac_aa64isar0, ID_AA64ISAR0_EL1_DP) == ID_AA64ISAR0_EL1_DP_UDOT)
	    exts += "asimddp ";
          if (__SHIFTOUT(id.ac_aa64isar0, ID_AA64ISAR0_EL1_FHM) == ID_AA64ISAR0_EL1_FHM_FMLAL)
	    exts += "asimdfml ";
#endif
          if (__SHIFTOUT(id.ac_aa64isar0, ID_AA64ISAR0_EL1_AES) == ID_AA64ISAR0_EL1_AES_AES)
	    exts += "aes ";
          if (__SHIFTOUT(id.ac_aa64isar0, ID_AA64ISAR0_EL1_AES) == ID_AA64ISAR0_EL1_AES_PMUL)
	    exts += "aes pmull ";
          if (__SHIFTOUT(id.ac_aa64isar0, ID_AA64ISAR0_EL1_CRC32) == ID_AA64ISAR0_EL1_CRC32_CRC32X)
	    exts += "crc32 ";
#ifdef ID_AA64ISAR0_EL1_ATOMIC
          if (__SHIFTOUT(id.ac_aa64isar0, ID_AA64ISAR0_EL1_ATOMIC) == ID_AA64ISAR0_EL1_ATOMIC_SWP)
	    exts += "atomics ";
#endif
          if ((__SHIFTOUT(id.ac_aa64isar0, ID_AA64ISAR0_EL1_SHA1) & ID_AA64ISAR0_EL1_SHA1_SHA1CPMHSU) != 0)
	    exts += "sha1 ";
          if ((__SHIFTOUT(id.ac_aa64isar0, ID_AA64ISAR0_EL1_SHA2) & ID_AA64ISAR0_EL1_SHA2_SHA256HSU) != 0)
	    exts += "sha2 ";
#ifdef ID_AA64ISAR0_EL1_SHA2_SHA512HSU
          if ((__SHIFTOUT(id.ac_aa64isar0, ID_AA64ISAR0_EL1_SHA2) & ID_AA64ISAR0_EL1_SHA2_SHA512HSU) != 0)
	    exts += "sha512 ";
          if ((__SHIFTOUT(id.ac_aa64isar0, ID_AA64ISAR0_EL1_SHA3) & ID_AA64ISAR0_EL1_SHA3_EOR3) != 0)
	    exts += "sha3 ";
          if (__SHIFTOUT(id.ac_aa64isar0, ID_AA64ISAR0_EL1_SM3) == ID_AA64ISAR0_EL1_SM3_SM3)
	    exts += "sm3 ";
          if (__SHIFTOUT(id.ac_aa64isar0, ID_AA64ISAR0_EL1_SM4) == ID_AA64ISAR0_EL1_SM4_SM4)
	    exts += "sm4 ";
          if (__SHIFTOUT(id.ac_aa64pfr0, ID_AA64PFR0_EL1_SVE) == ID_AA64PFR0_EL1_SVE_IMPL)
	    exts += "sve ";
          if (__SHIFTOUT(id.ac_aa64isar1, ID_AA64ISAR1_EL1_LRCPC) == ID_AA64ISAR1_EL1_LRCPC_PR)
	    exts += "lrcpc ";
#endif

          for (i = 0; i < num_exts; i++)
            {
	      const char *p = aarch64_extensions[i].feat_string;

	      /* If the feature contains no HWCAPS string then ignore it for the
		 auto detection.  */
	      if (*p == '\0')
		continue;

	      bool enabled = true;

	      /* This may be a multi-token feature string.  We need
		 to match all parts, which could be in any order.  */
	      size_t len = strlen (exts.c_str());
	      do
		{
		  const char *end = strchr (p, ' ');
		  if (end == NULL)
		    end = strchr (p, '\0');
		  if (memmem (exts.c_str(), len, p, end - p) == NULL)
		    {
		      /* Failed to match this token.  Turn off the
			 features we'd otherwise enable.  */
		      enabled = false;
		      break;
		    }
		  if (*end == '\0')
		    break;
		  p = end + 1;
		}
	      while (1);

              if (enabled)
                extension_flags |= aarch64_extensions[i].flag;
              else
                extension_flags &= ~(aarch64_extensions[i].flag);
            }

          processed_exts = true;
	}
    }
  /* End of NetBSD specific section.  */
#endif

  /* Weird cpuinfo format that we don't know how to handle.  */
  if (n_cores == 0
      || n_cores > 2
      || (n_cores == 1 && n_variants != 1)
      || imp == INVALID_IMP)
    goto not_found;

  /* Simple case, one core type or just looking for the arch. */
  if (n_cores == 1 || arch)
    {
      /* Search for one of the cores in the list. */
      for (i = 0; aarch64_cpu_data[i].name != NULL; i++)
	if (aarch64_cpu_data[i].implementer_id == imp
            && cores[0] == aarch64_cpu_data[i].part_no
            && (aarch64_cpu_data[i].variant == ALL_VARIANTS
                || variants[0] == aarch64_cpu_data[i].variant))
	  break;
      if (aarch64_cpu_data[i].name == NULL)
        goto not_found;

      if (arch)
	{
	  const char *arch_id = aarch64_cpu_data[i].arch;
	  aarch64_arch_driver_info* arch_info = get_arch_from_id (arch_id);

	  /* We got some arch indentifier that's not in aarch64-arches.def?  */
	  if (!arch_info)
	    goto not_found;

	  res = concat ("-march=", arch_info->name, NULL);
	  default_flags = arch_info->flags;
	}
      else
	{
	  default_flags = aarch64_cpu_data[i].flags;
	  res = concat ("-m",
			cpu ? "cpu" : "tune", "=",
			aarch64_cpu_data[i].name,
			NULL);
	}
    }
  /* We have big.LITTLE.  */
  else
    {
      for (i = 0; aarch64_cpu_data[i].name != NULL; i++)
	{
	  if (aarch64_cpu_data[i].implementer_id == imp
	      && valid_bL_core_p (cores, aarch64_cpu_data[i].part_no))
	    {
	      res = concat ("-m",
			    cpu ? "cpu" : "tune", "=",
			    aarch64_cpu_data[i].name,
			    NULL);
	      default_flags = aarch64_cpu_data[i].flags;
	      break;
	    }
	}
      if (!res)
	goto not_found;
    }

  if (tune)
    return res;

  {
    std::string extension
      = aarch64_get_extension_string_for_isa_flags (extension_flags,
						    default_flags);
    res = concat (res, extension.c_str (), NULL);
  }

  return res;

not_found:
  {
   /* If detection fails we ignore the option.
      Clean up and return NULL.  */

    if (f)
      fclose (f);

    return NULL;
  }
}

