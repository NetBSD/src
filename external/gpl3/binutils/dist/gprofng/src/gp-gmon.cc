/* Copyright (C) 2025-2026 Free Software Foundation, Inc.
   Contributed by Oracle.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "config.h"
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <libgen.h>
#include <sys/mman.h>

#include "util.h"
#include "DbeApplication.h"

#include "bfd.h"
#include "gmon_io.h"

#include "gp-gmon.h"
#include "corefile.h"
#include "data_pckts.h"
#include "call_graph.h"
#include "hist.h"

#include "symtab.h"
#include "cg_arcs.h"

#include "gp-experiment.h"
#include "collctrl.h"
#include "util.h"
#include "gp-defs.h"

class er_gmon : public DbeApplication
{
public:
  er_gmon (int argc, char *argv[]);
  virtual ~er_gmon ();
  void start (int argc, char *argv[]);

private:
  // override methods in base class
  void usage ();
  int check_mods (int argc, char *argv[], bool check);

  bool overwrite = false;
  Coll_Ctrl *cc;
};

/*
 * Default options values:
 */
#define HZ_WRONG 0
#define INC_DEPTH (128)
#define DFN_BUSY -1
#define DFN_NAN   0
#define CLOCK_TYPE OPROF_PCKT

/****/
#define ROOT_UID	801425552975190205ULL
#define ROOT_UID_INV	92251691606677ULL
#define ROOT_IDX	13907816567264074199ULL
#define ROOT_IDX_INV	2075111ULL
#define UIDTableSize	1048576

#define NATIVE_FRAME_BYTES(nframes) ( ((nframes)+1) * sizeof(long) )
#define OVERHEAD_BYTES ( 2 * sizeof(long) + 2 * sizeof(Stack_info) )
#define DEFAULT_MAX_NFRAMES 256

typedef struct
{
  Sym *sym;
} DFN_Stack;

typedef struct CM_Array
{
  unsigned int length;          /* in bytes, not including length */
  void *bytes;
} CM_Array;

typedef struct ClockPacket
{ /* clock profiling packet */
  CM_Packet comm;
  pthread_t lwp_id;
  pthread_t thr_id;
  uint32_t  cpu_id;
  hrtime_t  tstamp __attribute__ ((packed));
  uint64_t  frinfo __attribute__ ((packed));
  int       mstate;     /* kernel microstate */
  int       nticks;     /* number of ticks in that state */
} ClockPacket;

static unsigned long *pc_array = NULL;
static char *base_folder;
static DFN_Stack *dfn_stack = NULL;
static int dfn_maxdepth = 0;
static int dfn_depth = 0;
static int pc_size = 0;
static int pc_maxdepth = 0;
static hrtime_t mytime = 0;
static uint64_t *UIDTable;

// forward declarations
static uint64_t get_frame_info (const CM_Array *array);
static uint64_t compute_uid (Frame_packet *frp);
static void writeBufferToFile (const CM_Packet *pckt,
			       const char *filename);

static int
real_main (int argc, char *argv[])
{
  er_gmon *src = new er_gmon (argc, argv);
  src->start (argc, argv);
  delete src;
  return 0;
}

/* Push a symbol into stack, increase size stack if too low.  */
static void
pre_visit (Sym *sym)
{
  ++dfn_depth;

  if (dfn_depth >= dfn_maxdepth)
    {
      dfn_maxdepth += INC_DEPTH;
      dfn_stack = (DFN_Stack *) xrealloc (dfn_stack,
					  dfn_maxdepth * sizeof (*dfn_stack));
    }
  dfn_stack[dfn_depth - 1].sym = sym;
  // Mark the input sym as busy
  sym->cg.top_order = DFN_BUSY;

  /* Mimic time.  */
  DBG (LOOKUPDEBUG,
       printf (">>> Run for:%s ticks:%lf\n", sym->name, sym->hist.time));
  DBG (LOOKUPDEBUG, printf (">>> Call chain:"));
  pc_size = 0;
  for (int i = (dfn_depth - 1); i >= 0; i--)
    {
      Sym *t = dfn_stack[i].sym;
      ++pc_size;
      if (pc_size >= pc_maxdepth)
	{
	  pc_maxdepth += INC_DEPTH;
	  pc_array = (unsigned long *)
	    xrealloc (pc_array, pc_maxdepth * sizeof (unsigned long));
	}
      pc_array[pc_size - 1] = t->addr;
      DBG (LOOKUPDEBUG, printf ("----> %s: 0x%lx (%lf)\n", t->name,
				t->addr, t->hist.time));
    }
  DBG (LOOKUPDEBUG, printf ("\n"));
  ClockPacket pckt;
  memset (&pckt, 0, sizeof (ClockPacket));
  pckt.comm.type = CLOCK_TYPE;
  pckt.comm.tsize = sizeof (ClockPacket);
  pckt.lwp_id = 0xdeadbeef;
  pckt.thr_id = 0xcafecafe;
  pckt.cpu_id = 0;
  pckt.tstamp = mytime++;
  // Output `data.frameinfo` packet
  CM_Array array;
  array.length = pc_size * sizeof (unsigned long);
  array.bytes = (void *) pc_array;

  pckt.frinfo = get_frame_info (&array);

  pckt.mstate = LMS_LINUX_CPU;
  int calls = sym->ncalls > 0 ? sym->ncalls : 1;
  double time = sym->hist.time > 0 ? sym->hist.time : 1;
  uint64_t nticks = (uint64_t)(time / calls);
  pckt.nticks = nticks > 0 ? nticks : 1;

  // Output `profile` packet
  writeBufferToFile ((CM_Packet*) &pckt, SP_PROFILE_FILE);
}

/* Take the last symbol out of the stack.  */
static void
post_visit (Sym *sym)
{
  sym->cg.top_order = DFN_NAN;
  if (dfn_depth)
    --dfn_depth;
}

static void
dfn (Sym *parent)
{
  /* Detect cycles.  */
  if (parent->cg.top_order == DFN_BUSY)
    {
      parent->ncalls = 1;
      pre_visit (parent);
      return;
    }

  pre_visit (parent);
  for (Arc *arc = parent->cg.children; arc; arc = arc->next_child)
    {
      dfn (arc->child);
    }
  post_visit(parent);
}

/* Go through all the symbols, start from the top of a call chain.
   Mimimc a stack trace using a DF algorithm.  */

static int
cg_traverse_arcs (const char *whoami)
{
  Sym *sym;
  Sym_Table *symtab = get_symtab (whoami);

  for (sym = symtab->base; sym < symtab->limit; sym++)
    {
      /* Start DF algorithm from the top symbol.  */
      if (sym->cg.parents)
	continue;
      /* Skip unused symbols.  */
      if (!sym->cg.children)
	continue;
      dfn (sym);
    }
  return pc_size;
}

static void
writeBufferToFile (const CM_Packet *pckt,
		   const char *filename)
{
  size_t size = pckt->tsize;
  size_t asz;
  char *buffer = (char *) pckt;
  char *new_file_path;
  FILE *outFile;

  asz = strlen (base_folder) + strlen (filename) + 1 + 1;
  new_file_path = (char *) alloca (asz);
  snprintf (new_file_path, asz, "%s/%s", base_folder, filename);

  // Open the file in binary mode
  outFile = fopen (new_file_path, "ab");
  if (!outFile)
    {
      perror ("Fopen failed");
      exit (1);
    }

  // Write the buffer to the file
  fwrite (buffer, size, 1, outFile);

  fclose (outFile);
}

static uint64_t
compute_uid (Frame_packet *frp)
{
  uint64_t idxs[LAST_INFO];
  uint64_t uid = ROOT_UID;
  uint64_t idx = ROOT_IDX;

  Common_info *cinfo = (Common_info*) ((char*) frp + frp->hsize);
  char *end = (char*) frp + frp->tsize;
  for (;;)
    {
      if ((char*) cinfo >= end || cinfo->hsize == 0 ||
	  (char*) cinfo + cinfo->hsize > end)
	break;

      /* Start with a different value to avoid matching with uid */
      uint64_t uidt = 1;
      uint64_t idxt = 1;
      long *ptr = (long*) ((char*) cinfo + cinfo->hsize);
      long *bnd = (long*) ((char*) cinfo + sizeof (Common_info));
      DBG (SAMPLEDEBUG,
	   printf( "compute_uid: Cnt=%ld: ", (long) cinfo->hsize));
      while (ptr > bnd)
	{
	  long val = *(--ptr);
	  DBG (SAMPLEDEBUG,
	       printf ("0x%8.8llx ", (unsigned long long) val));
	  uidt = (uidt + val) * ROOT_UID;
	  idxt = (idxt + val) * ROOT_IDX;
	  uid = (uid + val) * ROOT_UID;
	  idx = (idx + val) * ROOT_IDX;
	}
      if (cinfo->kind == STACK_INFO)
	{
	  cinfo->uid = uidt;
	  idxs[cinfo->kind] = idxt;
	}
      cinfo = (Common_info*) ((char*) cinfo + cinfo->hsize);
    }
  DBG (SAMPLEDEBUG, printf ("\n"));

  /* Check if we have already recorded that uid.
   * The following fragment contains benign data races.
   * It's important, though, that all reads from UIDTable
   * happen before writes.
   */
  int found1 = 0;
  int idx1 = (int) ((idx >> 44) % UIDTableSize);
  if (UIDTable[idx1] == uid)
    found1 = 1;
  int found2 = 0;
  int idx2 = (int) ((idx >> 24) % UIDTableSize);
  if (UIDTable[idx2] == uid)
    found2 = 1;
  int found3 = 0;
  int idx3 = (int) ((idx >> 4) % UIDTableSize);
  if (UIDTable[idx3] == uid)
    found3 = 1;
  if (!found1)
    UIDTable[idx1] = uid;
  if (!found2)
    UIDTable[idx2] = uid;
  if (!found3)
    UIDTable[idx3] = uid;

  if (found1 || found2 || found3)
    {
      return uid;
    }
  frp->uid = uid;

  /* Compress info's */
  cinfo = (Common_info*) ((char*) frp + frp->hsize);
  for (;;)
    {
      if ((char*) cinfo >= end || cinfo->hsize == 0 ||
	  (char*) cinfo + cinfo->hsize > end)
	break;
      if (cinfo->kind == STACK_INFO || cinfo->kind == JAVA_INFO)
	{
	  long *ptr = (long*) ((char*) cinfo + sizeof (Common_info));
	  long *bnd = (long*) ((char*) cinfo + cinfo->hsize);
	  uint64_t uidt = cinfo->uid;
	  uint64_t idxt = idxs[cinfo->kind];
	  int found = 0;
	  int first = 1;
	  while (ptr < bnd - 1)
	    {
	      int idx1 = (int) ((idxt >> 44) % UIDTableSize);
	      if (UIDTable[idx1] == uidt)
		{
		  found = 1;
		  break;
		}
	      else if (first)
		{
		  first = 0;
		  UIDTable[idx1] = uidt;
		}
	      long val = *ptr++;
	      uidt = uidt * ROOT_UID_INV - val;
	      idxt = idxt * ROOT_IDX_INV - val;
	    }
	  if (found)
	    {
	      char *d = (char*) ptr;
	      char *s = (char*) bnd;
	      if (!first)
		{
		  int i;
		  for (i = 0; i < (int) sizeof (uidt); i++)
		    {
		      *d++ = (char) uidt;
		      uidt = uidt >> 8;
		    }
		}
	      int delta = s - d;
	      while (s < end)
		*d++ = *s++;
	      cinfo->kind |= COMPRESSED_INFO;
	      cinfo->hsize -= delta;
	      frp->tsize -= delta;
	      end -= delta;
	    }
	}
      cinfo = (Common_info*) ((char*) cinfo + cinfo->hsize);
    }
  writeBufferToFile ((CM_Packet*) frp, "data.frameinfo");
  return uid;
}

static uint64_t
get_frame_info (const CM_Array *array)
{
  int max_native_nframes = DEFAULT_MAX_NFRAMES;

  if (array == NULL || array->length <= 0)
    return 0;

  int max_frame_size = OVERHEAD_BYTES + NATIVE_FRAME_BYTES (max_native_nframes);
  Frame_packet *frpckt = (Frame_packet *) alloca (sizeof (Frame_packet) + max_frame_size);
  frpckt->type = FRAME_PCKT;
  frpckt->hsize = sizeof (Frame_packet);

  char *d = (char*) (frpckt + 1);
  int size = max_frame_size;

  /* create a stack image from user data */
  Stack_info *sinfo = (Stack_info*) d;
  int sz = sizeof (Stack_info);
  d += sz;
  size -= sz;
  sz = array->length;
  if (sz > size)
    sz = size;  // YXXX should we mark this with truncation frame?
  memcpy (d, array->bytes, sz);
  d += sz;
  size -= sz;
  sinfo->kind = STACK_INFO;
  sinfo->hsize = (d - (char*) sinfo);

  /* Compute the total size */
  frpckt->tsize = d - (char*) frpckt;
  return compute_uid (frpckt);
}

/* Generate the log.xml file.  */
static void
gen_gmon_log (void)
{
  FILE *logx;
  char *new_file_path;
  size_t asz = strlen (base_folder) + strlen (SP_LOG_FILE) + 1 + 1;

  new_file_path = (char *) alloca (asz);
  snprintf (new_file_path, asz, "%s/%s", base_folder, SP_LOG_FILE);

  logx = fopen (new_file_path, "w");

  long hz = hist_get_hz ();
  int gmon_interval = (hz == HZ_WRONG ? 1000 : hz * 100);

  fprintf (logx, "<profile name=\"%s\" ptimer=\"%d\" numstates=\"%d\">\n",
	   SP_JCMD_PROFILE, gmon_interval, LMS_MAGIC_ID_LINUX);
  fprintf (logx, "  <profdata fname=\"profile\"/>\n");

  /* Record Profile packet description */
  fprintf (logx, "  <profpckt kind=\"%d\" uname=\"Clock profiling data\">\n",
	   CLOCK_TYPE);
  fprintf (logx, "    <field name=\"LWPID\" uname=\"Lightweight process id\" \
offset=\"%d\" type=\"%s\"/>\n",
	   (int) offsetof (ClockPacket, lwp_id),
	   fld_sizeof (ClockPacket, lwp_id) == 4 ? "INT32" : "INT64");
  fprintf (logx, "    <field name=\"THRID\" uname=\"Thread number\" \
offset=\"%d\" type=\"%s\"/>\n",
	   (int) offsetof (ClockPacket, thr_id),
	   fld_sizeof (ClockPacket, thr_id) == 4 ? "INT32" : "INT64");
  fprintf (logx, "    <field name=\"CPUID\" uname=\"CPU id\" offset=\"%d\" \
type=\"%s\"/>\n",
	   (int) offsetof (ClockPacket, cpu_id),
	   fld_sizeof (ClockPacket, cpu_id) == 4 ? "INT32" : "INT64");
  fprintf (logx, "    <field name=\"TSTAMP\" uname=\"High resolution \
timestamp\" offset=\"%d\" type=\"%s\"/>\n",
	   (int) offsetof (ClockPacket, tstamp),
	   fld_sizeof (ClockPacket, tstamp) == 4 ? "INT32" : "INT64");
  fprintf (logx, "    <field name=\"FRINFO\" offset=\"%d\" type=\"%s\"/>\n",
	   (int) offsetof (ClockPacket, frinfo),
	   fld_sizeof (ClockPacket, frinfo) == 4 ? "INT32" : "INT64");
  fprintf (logx, "    <field name=\"MSTATE\" uname=\"Thread state\" \
offset=\"%d\" type=\"%s\"/>\n",
	   (int) offsetof (ClockPacket, mstate),
	   fld_sizeof (ClockPacket, mstate) == 4 ? "INT32" : "INT64");
  fprintf (logx, "    <field name=\"NTICK\" uname=\"Duration\" offset=\"%d\" \
type=\"%s\"/>\n",
	   (int) offsetof (ClockPacket, nticks),
	   fld_sizeof (ClockPacket, nticks) == 4 ? "INT32" : "INT64");
  fprintf (logx, "  </profpckt>\n");
  fprintf (logx,"</profile>\n");
  fprintf (logx, "<event kind=\"%s\" tstamp=\"%u.%09u\" time=\"%lld\" \
tm_zone=\"%lld\"/>\n",
	   SP_JCMD_RUN, 0, 0, 0LL, 0LL);

  fclose (logx);
}

/* This will not work for PIE execs.  */

static void
gen_gmon_map (char *name)
{
  bfd_vma mpage, page_size = sysconf(_SC_PAGESIZE);
  bfd_vma loadaddr, vaddr = core_text_sect->vma; //lma?
  bfd_size_type msize = core_text_sect->size;
  int timestamp = 1;
  int offset = 0;
  int check = -1;
  int modeflags = PROT_READ | PROT_EXEC; //0x05
  char *new_file_path;
  size_t asz = strlen (base_folder) + strlen (SP_MAP_FILE) + 1 + 1;

  new_file_path = (char *) alloca (asz);
  snprintf (new_file_path, asz, "%s/%s", base_folder, SP_MAP_FILE);

  FILE *mapx = fopen (new_file_path, "w");

  if (mapx == NULL) {
    return;
  }
  /* alignment mask.  */
  mpage = ~(page_size - 1);
  // Round down to page size alignment
  loadaddr = vaddr & mpage;
  // Compensate for the alignment gain
  msize += vaddr - loadaddr;
  // Round up to a multiple of page size;
  msize = (msize + page_size - 1) & mpage;

  fprintf (mapx, "<event kind=\"map\" object=\"segment\" tstamp=\"%u.%09u\" "
	   "vaddr=\"0x%016llX\" size=\"%lu\" pagesz=\"%d\" foffset=\"%c0x%08llX\" "
	   "modes=\"0x%03X\" chksum=\"0x%0X\" name=\"%s\"/>\n",
	   (unsigned) (timestamp / NANOSEC),
	   (unsigned) (timestamp % NANOSEC),
	   (long long unsigned) loadaddr, msize, (int) page_size,
	   offset < 0 ? '-' : '+',
	   (long long unsigned) (offset < 0 ? -offset : offset),
	   modeflags, check, name);

  fclose (mapx);
}

static int
checkflagterm (const char *c)
{
  if (c[2] != 0)
    {
      dbe_write (2, GTXT ("collect: unrecognized argument `%s'\n"), c);
      return -1;
    }
  return 0;
}

static void
writeStr (int f, const char *buf)
{
  if (buf != NULL)
    write (f, buf, strlen (buf));
}

/* main entry point. It calls catch_out_of_memory which will call
   real_main() function.  */
int
main (int argc, char *argv[])
{
  xmalloc_set_program_name (argv[0]);
  return catch_out_of_memory (real_main, argc, argv);
}

er_gmon::er_gmon (int argc, char *argv[]) : DbeApplication (argc, argv)
{
  int sz = UIDTableSize * sizeof (*UIDTable);
  UIDTable = (uint64_t *) xmalloc (sz);
  if (!UIDTable)
    return;
  memset (UIDTable, 0, sz);
  overwrite = false;
  cc = NULL;
}

void
er_gmon::start (int argc, char *argv[])
{
  char *gmon_name;
  char *a_out_name;
  // Create a collector control structure, disabling aggressive
  // warning.
  cc = new Coll_Ctrl (0, false, false);
  cc->set_default_stem ("gmon");

  whoami = argv[0];
  if (argc > 1 && strncmp (argv[1], NTXT ("--whoami="), 9) == 0)
    {
      whoami = argv[1] + 9;
      argc--;
      argv++;
    }
  if (argc == 2 && strcmp (argv[1], NTXT ("-h")) == 0)
    {
      /* only one argument, -h */
      usage ();
      exit (0);
    }
  else if (argc == 2 && (strcmp (argv[1], NTXT ("-help")) == 0 ||
			 strcmp (argv[1], NTXT ("--help")) == 0))
    {
      /* only one argument, -help or --help */
      usage ();
      exit (0);
    }
  else if ((argc == 2) &&
	   (strcmp (argv[1], NTXT ("--version")) == 0))
    {
      /* only one argument, --version */

      /* print the version info */
      Application::print_version_info ();
      exit (0);
    }

  check_mods (argc, argv, true);
  int adj = check_mods (argc, argv, false);
  if (adj < 0)
    {
      usage ();
      exit (0);
    }

  char *ret = cc->create_exp_dir ();
  if (ret != NULL)
    {
      dbe_write (2, NTXT ("%s\n"), ret);
      free (ret);
      exit (1);
    }

  // Get the file path.
  base_folder = cc->get_experiment ();

  // Default names
  a_out_name =  xstrdup ("a.out");
  gmon_name =  xstrdup ("gmon.out");
  // Get the ELF and the GMON.OUT file if they exist.
  if (argc == (adj + 1))
    {
      a_out_name = argv[adj];
    }
  else if (argc == (adj + 2))
    {
      a_out_name = argv[adj++];
      gmon_name = argv[adj];
    }
  else if (argc != adj)
    {
      usage ();
      exit (0);
    }

  /* Read the elf syms and the gmon file.  */
  if (core_init (a_out_name, whoami) < 0)
    {
      cc->remove_exp_dir ();
      exit (1);
    }
  if (gmon_out_read (gmon_name, FF_AUTO, whoami) < 0)
    {
      cc->remove_exp_dir ();
      exit (1);
    }

  /* Process the gmon file, and output the gprofng project files.  */
  hist_assign_samples (whoami);
  cg_traverse_arcs (whoami);
  gen_gmon_map (a_out_name);
  gen_gmon_log ();
}

/* Get the args and search for modifiers.  */
int
er_gmon::check_mods (int argc, char *argv[], bool check)
{
  char *expName = NULL;
  int i = -1;
  for (i = 1; i < argc; i++)
    {
      if (argv[i] == NULL)
	break;
      if (argv[i][0] != '-')
	break;
      switch (argv[i][1])
	{
	case 'O':
	  overwrite = true;
	  //FALLTHROU
	case 'o':
	  if (check)
	    return i;
	  if (checkflagterm (argv[i]) == -1)
	    return -1;
	  if (argv[i + 1] == NULL)
	    {
	      dbe_write (2,
			 GTXT ("Argument %s must be followed by a file name\n"),
			 argv[i]);
	      return -1;
	    }
	  if (expName != NULL)
	    {
	      dbe_write (2, GTXT ("Only one -o or -O argument may be used\n"));
	      return -1;
	    }
	  expName = argv[i + 1];
	  i++;
	  break;

	default:
	  dbe_write (2, GTXT ("gmon: unrecognized argument `%s'\n"),
		     argv[i]);
	  return -1;
	}
    }
  if (check)
    return i;
  if (expName)
    {
      char *ccret;
      char *ccwarn = NULL;
      ccret = cc->set_expt (expName, &ccwarn, overwrite);
      if (ccwarn)
	{
	  writeStr (2, ccwarn);
	  free (ccwarn);
	}
      if (ccret)
	{
	  writeStr (2, ccret);
	  return -1;
	}
    }
  return (check ? -1 : i);
}

void
er_gmon::usage ()
{
  printf ( GTXT (
    "Usage: gprofng display gmon [OPTION(S)] [TARGET-OBJECT [GMON-FILE]]\n"));
  printf ( GTXT (
    "\n"
    "Convert an gmon.out file to a gprofng experiment.\n"
    "\n"
    "Options:\n"
    "\n"
    " --version           print the version number and exit.\n"
    " -h/--help           print usage information and exit.\n"
    "\n"
    " -o <exp_name>     specify the name for (and path to) the experiment directory; the\n"
    "                    the default path is the current directory.\n"
    "\n"
    " -O <exp_name>     the same as -o, but unlike the -o option, silently overwrite an\n"
    "                    existing experiment directory with the same name.\n"
    "\n"));

    exit (0);
}

er_gmon::~er_gmon ()
{
  free (UIDTable);
  delete cc;
}
