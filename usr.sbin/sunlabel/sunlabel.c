/* This file is in the public domain. */
/*
 * Compile-time defines of note:
 *
 *	S_COMMAND
 *	NO_S_COMMAND
 *		Provide, or don't provide, the S command, which sets
 *		the in-core disklabel (as opposed to the on-disk
 *		disklabel).  This depends on <sys/disklabel.h> and
 *		DIOCSDINFO and supporting types as provided by NetBSD.
 *
 *	GNUC_ATTRIBUTE
 *	NO_GNUC_ATTRIBUTE
 *		Use, or don't use, GNU C's __attribute__ mechanism.
 *		This is presently also overloaded to control use of
 *		__inline__.
 *
 *	NO_TERMCAP_WIDTH
 *		Never try to use tgetnum() to get the terminal's width.
 */

#ifdef DISTRIB

/* This code compensates for a lack of __progname, by using argv[0]
   instead.  Define DISTRIB if you're on a system with no __progname. */
const char *__progname;
int main(int, char **);
int main_(int, char **);
int main(int ac, char **av) { __progname = av[0]; return main_(ac,av); }
#define main main_

#endif

/* If neither S_COMMAND nor NO_S_COMMAND is defined, guess. */
#if !defined(S_COMMAND) && !defined(NO_S_COMMAND)
#ifdef __NetBSD__
#define S_COMMAND
#include <util.h>
#endif
#endif

/* If neither GNUC_ATTRIBUTE nor NO_GNUC_ATTRIBUTE is defined, guess. */
#if !defined(GNUC_ATTRIBUTE) && !defined(NO_GNUC_ATTRIBUTE)
#if defined(__GNUC__) &&			\
    ( (__GNUC__ > 2) ||				\
      ( (__GNUC__ == 2) &&			\
	defined(__GNUC_MINOR__) &&		\
	(__GNUC_MINOR__ >= 7) ) )
#define GNUC_ATTRIBUTE
#endif
#endif

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <termcap.h>
#include <strings.h>
#include <sys/file.h>
#include <sys/ioctl.h>

#ifdef S_COMMAND
#include <sys/disklabel.h>
#endif

#ifdef GNUC_ATTRIBUTE
#define UNUSED(x) x __attribute__ ((__unused__))
#define ATTRIB(x) __attribute__ (x)
#define INLINE __inline__
#else
#define UNUSED(x) x
#define ATTRIB(x)
#define INLINE
#endif

extern const char *__progname;

/* NPART is the total number of partitions.  This must be <=43, given the
   amount of space available to store extended partitions.  It also must
   be <=26, given the use of single letters to name partitions.  The 8 is
   the number of `standard' partitions; this arguably should be a #define,
   since it occurs not only here but scattered throughout the code. */
#define NPART 16
#define NXPART (NPART-8)
#define PARTLETTER(i) ((i)+'a')
#define LETTERPART(i) ((i)-'a')

/* Struct types. */
typedef struct field FIELD;
typedef struct label LABEL;
typedef struct part PART;

/*
 * A partition.  We keep redundant information around, making sure that
 *  whenever we change one, we keep another constant and update the
 *  third.  Which one is which depends.  Arguably a partition should
 *  also know its partition number; here, if we need that we cheat,
 *  using (effectively) ptr-&label.partitions[0].
 */
struct part {
  unsigned int startcyl;
  unsigned int nblk;
  unsigned int endcyl;
  } ;

/*
 * A label.  As the embedded comments indicate, much of this structure
 *  corresponds directly to Sun's struct dk_label.  Some of the values
 *  here are historical holdovers.  Apparently really old Suns did
 *  their own sparing in software, so a sector or two per cylinder,
 *  plus a whole cylinder or two at the end, got set aside as spares.
 *  acyl and apc count those spares, and this is also why ncyl and pcyl
 *  both exist.  These days the spares generally are hidden from the
 *  host by the disk, and there's no reason not to set
 *  ncyl=pcyl=ceil(device size/spc) and acyl=apc=0.
 *
 * Note also that the geometry assumptions behind having nhead and
 *  nsect assume that the sect/trk and trk/cyl values are constant
 *  across the whole drive.  The latter is still usually true; the
 *  former isn't.  In my experience, you can just put fixed values
 *  here; the basis for software knowing the drive geometry is also
 *  mostly invalid these days anyway.  (I just use nhead=32 nsect=64,
 *  which gives me 1M "cylinders", a convenient size.)
 */
struct label {
  /* BEGIN fields taken directly from struct dk_label */
  char asciilabel[128];
  unsigned int rpm;	/* Spindle rotation speed - arguably useless now */
  unsigned int pcyl;	/* Physical cylinders */
  unsigned int apc;	/* Alternative sectors per cylinder */
  unsigned int obs1;	/* Obsolete? */
  unsigned int obs2;	/* Obsolete? */
  unsigned int intrlv;	/* Interleave - never anything but 1 IME */
  unsigned int ncyl;	/* Number of usable cylinders */
  unsigned int acyl;	/* Alternative cylinders - pcyl minus ncyl */
  unsigned int nhead;	/* Tracks-per-cylinder (usually # of heads) */
  unsigned int nsect;	/* Sectors-per-track */
  unsigned int obs3;	/* Obsolete? */
  unsigned int obs4;	/* Obsolete? */
  /* END fields taken directly from struct dk_label */
  unsigned int spc;	/* Sectors per cylinder - nhead*nsect */
  unsigned int dirty : 1; /* Modified since last read */
  PART partitions[NPART]; /* The partitions themselves */
  } ;

/*
 * Describes a field in the label.
 *
 * tag is a short name for the field, like "apc" or "nsect".  loc is a
 *  pointer to the place in the label where it's stored.  print is a
 *  function to print the value; the second argument is the current
 *  column number, and the return value is the new current column
 *  number.  (This allows print functions to do proper line wrapping.)
 *  chval is called to change a field; the first argument is the
 *  command line portion that contains the new value (in text form).
 *  The chval function is responsible for parsing and error-checking as
 *  well as doing the modification.  changed is a function which does
 *  field-specific actions necessary when the field has been changed.
 *  This could be rolled into the chval function, but I believe this
 *  way provides better code sharing.
 *
 * Note that while the fields in the label vary in size (8, 16, or 32
 *  bits), we store everything as ints in the label struct, above, and
 *  convert when packing and unpacking.  This allows us to have only
 *  one numeric chval function.
 */
struct field {
  const char *tag;
  void *loc;
  int (*print)(FIELD *, int);
  void (*chval)(const char *, FIELD *);
  void (*changed)(void);
  int taglen;
  } ;

/* LABEL_MAGIC was chosen by Sun and cannot be trivially changed. */
#define LABEL_MAGIC 0xdabe
/* LABEL_XMAGIC needs to agree between here and any other code that uses
   extended partitions (mainly the kernel). */
#define LABEL_XMAGIC (0x199d1fe2+8)

static int diskfd;		/* fd on the disk */
static const char *diskname;	/* name of the disk, for messages */
static int readonly;		/* true iff it's open RO */
static unsigned char labelbuf[512]; /* Buffer holding the label sector */
static LABEL label;		/* The label itself. */
static int fixmagic;		/* True iff -fixmagic, ignore bad magic #s */
static int fixcksum;		/* True iff -fixcksum, ignore bad cksums */
static int newlabel;		/* True iff -new, ignore all on-disk values */
static int quiet;		/* True iff -quiet, don't print chatter */

/*
 * The various functions that go in the field function pointers.  The
 *  _ascii functions are for 128-byte string fields (the ASCII label);
 *  the _int functions are for int-valued fields (everything else).
 *  update_spc is a `changed' function for updating the spc value when
 *  changing one of the two values that make it up.
 */
static int print_ascii(FIELD *, int);
static void chval_ascii(const char *, FIELD *);
static int print_int(FIELD *, int);
static void chval_int(const char *, FIELD *);
static void update_spc(void);

/* The fields themselves. */
static FIELD fields[]
 = { { "ascii",  &label.asciilabel[0], print_ascii, chval_ascii, 0          },
     { "rpm",    &label.rpm,           print_int,   chval_int,   0          },
     { "pcyl",   &label.pcyl,          print_int,   chval_int,   0          },
     { "apc",    &label.apc,           print_int,   chval_int,   0          },
     { "obs1",   &label.obs1,          print_int,   chval_int,   0          },
     { "obs2",   &label.obs2,          print_int,   chval_int,   0          },
     { "intrlv", &label.intrlv,        print_int,   chval_int,   0          },
     { "ncyl",   &label.ncyl,          print_int,   chval_int,   0          },
     { "acyl",   &label.acyl,          print_int,   chval_int,   0          },
     { "nhead",  &label.nhead,         print_int,   chval_int,   update_spc },
     { "nsect",  &label.nsect,         print_int,   chval_int,   update_spc },
     { "obs3",   &label.obs3,          print_int,   chval_int,   0          },
     { "obs4",   &label.obs4,          print_int,   chval_int,   0          },
     { 0 } };

/*
 * We'd _like_ to use howmany() from the include files, but can't count
 *  on its being present or working.
 */
static INLINE unsigned int how_many(unsigned int, unsigned int)
	ATTRIB((__const__));
static INLINE unsigned int how_many(unsigned int amt, unsigned int unit)
{
 return((amt+unit-1)/unit);
}

/*
 * Try opening the disk, given a name.  If mustsucceed is true, we
 *  "cannot fail"; failures produce gripe-and-exit, and if we return,
 *  our return value is 1.  Otherwise, we return 1 on success and 0 on
 *  failure.
 */
static int trydisk(const char *s, int mustsucceed)
{
 int ro;

 ro = 0;
 diskname = s;
 diskfd = open(s,O_RDWR,0);
 if (diskfd < 0)
  { diskfd = open(s,O_RDWR|O_NDELAY,0);
  }
 if (diskfd < 0)
  { diskfd = open(s,O_RDONLY,0);
    ro = 1;
  }
 if (diskfd < 0)
  { if (mustsucceed)
     { fprintf(stderr,"%s: can't open %s: %s\n",__progname,s,strerror(errno));
       exit(1);
     }
    return(0);
  }
 if (ro && !quiet) fprintf(stderr,"Note: no write access, label is readonly\n");
 readonly = ro;
 return(1);
}

/*
 * Set the disk device, given the user-supplied string.  Note that even
 *  if we malloc, we never free, because either trydisk eventually
 *  succeeds, in which case the string is saved in diskname, or it
 *  fails, in which case we exit and freeing is irrelevant.
 */
static void setdisk(const char *s)
{
 char *tmp;

 if (index(s,'/'))
  { trydisk(s,1);
    return;
  }
 if (trydisk(s,0)) return;
 tmp = malloc(strlen(s)+7);
 sprintf(tmp,"/dev/%s",s);
 if (trydisk(tmp,0)) return;
 sprintf(tmp,"/dev/%sc",s);
 if (trydisk(tmp,0)) return;
 fprintf(stderr,"%s: can't find device for disk %s\n",__progname,s);
 exit(1);
}

static void usage(void)
{
 fprintf(stderr,"Usage: %s [options] [disk]\n",__progname);
 fprintf(stderr,"Options:\n");
 fprintf(stderr,"\t-disk <disk>   Use disk <disk>.\n");
 fprintf(stderr,"\t-fixmagic      Allow broken magic numbers.\n");
 fprintf(stderr,"\t-fixsum        Allow broken check sums.\n");
 fprintf(stderr,"\t-new           Ignore currently label.\n");
 fprintf(stderr,"\t-q             Quiet mode.\n");
 fprintf(stderr,"\t-h             Print this help.\n");
}

/*
 * Handle command-line arguments.  We can have at most one non-flag
 *  argument, which is the disk name; we can also have flags
 *
 *	-disk diskdev
 *		Specifies disk device unambiguously (if it begins with
 *		a dash, it will be mistaken for a flag if simply placed
 *		on the command line).
 *
 *	-fixmagic
 *		Turns on fixmagic, which causes bad magic numbers to be
 *		ignored (though a complaint is still printed), rather
 *		than being fatal errors.
 *
 *	-fixsum
 *		Turns on fixcksum, which causes bad checksums to be
 *		ignored (though a complaint is still printed), rather
 *		than being fatal errors.
 *
 *	-new
 *		Turns on newlabel, which means we're creating a new
 *		label and anything in the label sector should be
 *		ignored.  This is a bit like -fixmagic -fixsum, except
 *		that it doesn't print complaints and it ignores
 *		possible garbage on-disk.
 *
 *	-q
 *		Turns on quiet, which suppresses printing of prompts
 *		and other irrelevant chatter.  If you're trying to use
 *		sunlabel in an automated way, you probably want this.
 *
 *	-h
 *		Print a usage message.
 */
static void handleargs(int ac, char **av)
{
 int skip;
 int errs;
 int argno;

 skip = 0;
 errs = 0;
 argno = 0;
 for (ac--,av++;ac;ac--,av++)
  { if (skip > 0)
     { skip --;
       continue;
     }
    if (**av != '-')
     { switch (argno++)
	{ case 0:
	     setdisk(*av);
	     break;
	  default:
	     fprintf(stderr,"%s: unrecognized argument `%s'\n",__progname,*av);
	     errs ++;
	     break;
	}
       continue;
     }
    if (0)
     {
needarg:;
       fprintf(stderr,"%s: %s needs a following argument\n",__progname,*av);
       errs ++;
       continue;
     }
#define WANTARG() do { if (++skip >= ac) goto needarg; } while (0)
    if (!strcmp(*av,"-disk"))
     { WANTARG();
       setdisk(av[skip]);
       continue;
     }
    if (!strcmp(*av,"-fixmagic"))
     { fixmagic = 1;
       continue;
     }
    if (!strcmp(*av,"-fixsum"))
     { fixcksum = 1;
       continue;
     }
    if (!strcmp(*av,"-new"))
     { newlabel = 1;
       continue;
     }
    if (!strcmp(*av,"-q"))
     { quiet = 1;
       continue;
     }
    if (!strcmp(*av,"-h"))
     { usage();
       exit(0);
     }
#undef WANTARG
    fprintf(stderr,"%s: unrecognized option `%s'\n",__progname,*av);
    errs ++;
  }
 if (errs)
  { exit(1);
  }
}

/*
 * Sets the ending cylinder for a partition.  This exists mainly to
 *  centralize the check.  (If spc is zero, cylinder numbers make
 *  little sense, and the code would otherwise die on divide-by-0 if we
 *  barged blindly ahead.)  We need to call this on a partition
 *  whenever we change it; we need to call it on all partitions
 *  whenever we change spc.
 */
static void set_endcyl(PART *p)
{
 if (label.spc == 0)
  { p->endcyl = p->startcyl;
  }
 else
  { p->endcyl = p->startcyl + how_many(p->nblk,label.spc);
  }
}

/*
 * Unpack a label from disk into the in-core label structure.  If
 *  newlabel is set, we don't actually do so; we just synthesize a
 *  blank label instead.  This is where knowledge of the Sun label
 *  format is kept for read; pack_label is the corresponding routine
 *  for write.  We are careful to use labelbuf, l_s, or l_l as
 *  appropriate to avoid byte-sex issues, so we can work on
 *  little-endian machines.
 *
 * Note that a bad magic number for the extended partition information
 *  is not considered an error; it simply indicates there is no
 *  extended partition information.  Arguably this is the Wrong Thing,
 *  and we should take zero as meaning no info, and anything other than
 *  zero or LABEL_XMAGIC as reason to gripe.
 */
static const char *unpack_label(void)
{
 unsigned short int l_s[256];
 unsigned long int l_l[128];
 int i;
 unsigned long int sum;
 int have_x;

 if (newlabel)
  { bzero(&label.asciilabel[0],128);
    label.rpm = 0;
    label.pcyl = 0;
    label.apc = 0;
    label.obs1 = 0;
    label.obs2 = 0;
    label.intrlv = 0;
    label.ncyl = 0;
    label.acyl = 0;
    label.nhead = 0;
    label.nsect = 0;
    label.obs3 = 0;
    label.obs4 = 0;
    for (i=0;i<NPART;i++)
     { label.partitions[i].startcyl = 0;
       label.partitions[i].nblk = 0;
       set_endcyl(&label.partitions[i]);
     }
    label.spc = 0;
    label.dirty = 1;
    return(0);
  }
 for (i=0;i<256;i++) l_s[i] = (labelbuf[i+i] << 8) | labelbuf[i+i+1];
 for (i=0;i<128;i++) l_l[i] = (l_s[i+i] << 16) | l_s[i+i+1];
 if (l_s[254] != LABEL_MAGIC)
  { if (fixmagic)
     { label.dirty = 1;
       printf("(ignoring incorrect magic number)\n");
     }
    else
     { return("bad magic number");
     }
  }
 sum = 0;
 for (i=0;i<256;i++) sum ^= l_s[i];
 label.dirty = 0;
 if (sum != 0)
  { if (fixcksum)
     { label.dirty = 1;
       printf("(ignoring incorrect checksum)\n");
     }
    else
     { return("checksum wrong");
     }
  }
 bcopy(&labelbuf[0],&label.asciilabel[0],128);
 label.rpm = l_s[210];
 label.pcyl = l_s[211];
 label.apc = l_s[212];
 label.obs1 = l_s[213];
 label.obs2 = l_s[214];
 label.intrlv = l_s[215];
 label.ncyl = l_s[216];
 label.acyl = l_s[217];
 label.nhead = l_s[218];
 label.nsect = l_s[219];
 label.obs3 = l_s[220];
 label.obs4 = l_s[221];
 label.spc = label.nhead * label.nsect;
 for (i=0;i<8;i++)
  { label.partitions[i].startcyl = l_l[i+i+111];
    label.partitions[i].nblk = l_l[i+i+112];
    set_endcyl(&label.partitions[i]);
  }
 have_x = 0;
 if (l_l[33] == LABEL_XMAGIC)
  { sum = 0;
    for (i=0;i<((NXPART*2)+1);i++) sum += l_l[33+i];
    if (sum != l_l[32])
     { if (fixcksum)
	{ label.dirty = 1;
	  printf("(ignoring incorrect extended-partition checksum)\n");
	  have_x = 1;
	}
       else
	{ printf("(note: extended-partition magic right but checksum wrong)\n");
	}
     }
    else
     { have_x = 1;
     }
  }
 if (have_x)
  { for (i=0;i<NXPART;i++)
     { label.partitions[i+8].startcyl = l_l[i+i+34];
       label.partitions[i+8].nblk = l_l[i+i+35];
       set_endcyl(&label.partitions[i+8]);
     }
  }
 else
  { for (i=0;i<NXPART;i++)
     { label.partitions[i+8].startcyl = 0;
       label.partitions[i+8].nblk = 0;
       set_endcyl(&label.partitions[i+8]);
     }
  }
 return(0);
}

/*
 * Pack a label from the in-core label structure into on-disk format.
 *  This is where knowledge of the Sun label format is kept for write;
 *  unpack_label is the corresponding routine for read.  If all
 *  partitions past the first 8 are size=0 cyl=0, we store all-0s in
 *  the extended partition space, to be fully compatible with Sun
 *  labels.  Since AFIAK nothing works in that case that would break if
 *  we put extended partition info there in the same format we'd use if
 *  there were real info there, this is arguably unnecessary, but it's
 *  easy to do.
 *
 * We are careful to avoid endianness issues by constructing everything
 *  in an array of shorts.  We do this rather than using chars or longs
 *  because the checksum is defined in terms of shorts; using chars or
 *  longs would simplify small amounts of code at the price of
 *  complicating more.
 */
static void pack_label(void)
{
 unsigned short int l_s[256];
 int i;
 unsigned short int sum;

 bzero(&l_s[0],512);
 bcopy(&label.asciilabel[0],&labelbuf[0],128);
 for (i=0;i<64;i++) l_s[i] = (labelbuf[i+i] << 8) | labelbuf[i+i+1];
 l_s[210] = label.rpm;
 l_s[211] = label.pcyl;
 l_s[212] = label.apc;
 l_s[213] = label.obs1;
 l_s[214] = label.obs2;
 l_s[215] = label.intrlv;
 l_s[216] = label.ncyl;
 l_s[217] = label.acyl;
 l_s[218] = label.nhead;
 l_s[219] = label.nsect;
 l_s[220] = label.obs3;
 l_s[221] = label.obs4;
 for (i=0;i<8;i++)
  { l_s[(i*4)+222] = label.partitions[i].startcyl >> 16;
    l_s[(i*4)+223] = label.partitions[i].startcyl & 0xffff;
    l_s[(i*4)+224] = label.partitions[i].nblk >> 16;
    l_s[(i*4)+225] = label.partitions[i].nblk & 0xffff;
  }
 for (i=0;i<NXPART;i++)
  { if (label.partitions[i+8].startcyl || label.partitions[i+8].nblk) break;
  }
 if (i < NXPART)
  { unsigned long int xsum;
    l_s[66] = LABEL_XMAGIC >> 16;
    l_s[67] = LABEL_XMAGIC & 0xffff;
    for (i=0;i<NXPART;i++)
     { l_s[(i*4)+68] = label.partitions[i+8].startcyl >> 16;
       l_s[(i*4)+69] = label.partitions[i+8].startcyl & 0xffff;
       l_s[(i*4)+70] = label.partitions[i+8].nblk >> 16;
       l_s[(i*4)+71] = label.partitions[i+8].nblk & 0xffff;
     }
    xsum = 0;
    for (i=0;i<((NXPART*2)+1);i++) xsum += (l_s[i+i+66] << 16) | l_s[i+i+67];
    l_s[64] = xsum >> 16;
    l_s[65] = xsum & 0xffff;
  }
 l_s[254] = LABEL_MAGIC;
 sum = 0;
 for (i=0;i<255;i++) sum ^= l_s[i];
 l_s[255] = sum;
 for (i=0;i<256;i++)
  { labelbuf[i+i] = l_s[i] >> 8;
    labelbuf[i+i+1] = l_s[i] & 0xff;
  }
}

/*
 * Get the label.  Read it off the disk and unpack it.  This function
 *  is nothing but lseek, read, unpack_label, and error checking.
 */
static void getlabel(void)
{
 int rv;
 const char *lerr;

 if (lseek(diskfd,0,L_SET) < 0)
  { fprintf(stderr,"%s: lseek to 0 on %s: %s\n",__progname,diskname,strerror(errno));
    exit(1);
  }
 rv = read(diskfd,&labelbuf[0],512);
 if (rv < 0)
  { fprintf(stderr,"%s: read label from %s: %s\n",__progname,diskname,strerror(errno));
    exit(1);
  }
 if (rv != 512)
  { fprintf(stderr,"%s: short read from %s: wanted %d, got %d\n",__progname,diskname,512,rv);
    exit(1);
  }
 lerr = unpack_label();
 if (lerr)
  { fprintf(stderr,"%s: bogus label on %s: %s\n",__progname,diskname,lerr);
    exit(1);
  }
}

/*
 * Put the label.  Pack it and write it to the disk.  This function is
 *  little more than pack_label, lseek, write, and error checking.
 */
static void putlabel(void)
{
 int rv;

 if (readonly)
  { fprintf(stderr,"%s: no write access to %s\n",__progname,diskname);
    return;
  }
 if (lseek(diskfd,0,L_SET) < 0)
  { fprintf(stderr,"%s: lseek to 0 on %s: %s\n",__progname,diskname,strerror(errno));
    exit(1);
  }
 pack_label();
 rv = write(diskfd,&labelbuf[0],512);
 if (rv < 0)
  { fprintf(stderr,"%s: write label to %s: %s\n",__progname,diskname,strerror(errno));
    exit(1);
  }
 if (rv != 512)
  { fprintf(stderr,"%s: short write to %s: wanted %d, got %d\n",__progname,diskname,512,rv);
    exit(1);
  }
 label.dirty = 0;
}

/*
 * Skip whitespace.  Used several places in the command-line parsing
 *  code.
 */
static void skipspaces(const char **cpp)
#define cp (*cpp)
{
 while (*cp && isspace(*cp)) cp ++;
}
#undef cp

/*
 * Scan a number.  The first arg points to the char * that's moving
 *  along the string.  The second arg points to where we should store
 *  the result.  The third arg says what we're scanning, for errors.
 *  The return value is 0 on error, or nonzero if all goes well.
 */
static int scannum(const char **cpp, unsigned int *np, const char *tag)
#define cp (*cpp)
{
 unsigned int v;
 int nd;

 skipspaces(cpp);
 v = 0;
 nd = 0;
 while (*cp && isdigit(*cp))
  { v = (10 * v) + (*cp++ - '0');
    nd ++;
  }
 if (nd == 0)
  { printf("Missing/invalid %s: %s\n",tag,cp);
    return(0);
  }
 *np = v;
 return(1);
}
#undef cp

/*
 * Change a partition.  pno is the number of the partition to change;
 *  numbers is a pointer to the string containing the specification for
 *  the new start and size.  This always takes the form "start size",
 *  where start can be
 *
 *	a number
 *		The partition starts at the beginning of that cylinder.
 *
 *	start-X
 *		The partition starts at the same place partition X does.
 *
 *	end-X
 *		The partition starts at the place partition X ends.  If
 *		partition X does not exactly on a cylinder boundary, it
 *		is effectively rounded up.
 *
 *  and size can be
 *
 *	a number
 *		The partition is that many sectors long.
 *
 *	num/num/num
 *		The three numbers are cyl/trk/sect counts.  n1/n2/n3 is
 *		equivalent to specifying a single number
 *		((n1*label.nhead)+n2)*label.nsect)+n3.  In particular,
 *		if label.nhead or label.nsect is zero, this has limited
 *		usefulness.
 *
 *	end-X
 *		The partition ends where partition X ends.  It is an
 *		error for partition X to end before the specified start
 *		point.  This always goes to exactly where partition X
 *		ends, even if that's partway through a cylinder.
 *
 *	start-X
 *		The partition extends to end exactly where partition X
 *		begins.  It is an error for partition X to begin before
 *		the specified start point.
 *
 *	size-X
 *		The partition has the same size as partition X.
 *
 * If label.spc is nonzero but the partition size is not a multiple of
 *  it, a warning is printed, since you usually don't want this.  Most
 *  often, in my experience, this comes from specifying a cylinder
 *  count as a single number N instead of N/0/0.
 */
static void chpart(int pno, const char *numbers)
{
 unsigned int cyl0;
 unsigned int size;
 unsigned int sizec;
 unsigned int sizet;
 unsigned int sizes;

 skipspaces(&numbers);
 if (!bcmp(numbers,"end-",4) && numbers[4])
  { int epno;
    epno = LETTERPART(numbers[4]);
    if ((epno >= 0) && (epno < NPART))
     { cyl0 = label.partitions[epno].endcyl;
       numbers += 5;
     }
    else
     { if (! scannum(&numbers,&cyl0,"starting cylinder")) return;
     }
  }
 else if (!bcmp(numbers,"start-",6) && numbers[6])
  { int spno;
    spno = LETTERPART(numbers[6]);
    if ((spno >= 0) && (spno < NPART))
     { cyl0 = label.partitions[spno].startcyl;
       numbers += 7;
     }
    else
     { if (! scannum(&numbers,&cyl0,"starting cylinder")) return;
     }
  }
 else
  { if (! scannum(&numbers,&cyl0,"starting cylinder")) return;
  }
 skipspaces(&numbers);
 if (!bcmp(numbers,"end-",4) && numbers[4])
  { int epno;
    epno = LETTERPART(numbers[4]);
    if ((epno >= 0) && (epno < NPART))
     { if (label.partitions[epno].endcyl <= cyl0)
	{ printf("Partition %c ends before cylinder %u\n",PARTLETTER(epno),cyl0);
	  return;
	}
       size = label.partitions[epno].nblk;
       /* Be careful of unsigned arithmetic */
       if (cyl0 > label.partitions[epno].startcyl)
	{ size -= (cyl0 - label.partitions[epno].startcyl) * label.spc;
	}
       else if (cyl0 < label.partitions[epno].startcyl)
	{ size += (label.partitions[epno].startcyl - cyl0) * label.spc;
	}
       numbers += 5;
     }
    else
     { if (! scannum(&numbers,&size,"partition size")) return;
     }
  }
 else if (!bcmp(numbers,"start-",6) && numbers[6])
  { int spno;
    spno = LETTERPART(numbers[6]);
    if ((spno >= 0) && (spno < NPART))
     { if (label.partitions[spno].startcyl <= cyl0)
	{ printf("Partition %c starts before cylinder %u\n",PARTLETTER(spno),cyl0);
	  return;
	}
       size = (label.partitions[spno].startcyl - cyl0) * label.spc;
       numbers += 7;
     }
    else
     { if (! scannum(&numbers,&size,"partition size")) return;
     }
  }
 else if (!bcmp(numbers,"size-",5) && numbers[5])
  { int spno;
    spno = LETTERPART(numbers[5]);
    if ((spno >= 0) && (spno < NPART))
     { size = label.partitions[spno].nblk;
       numbers += 6;
     }
    else
     { if (! scannum(&numbers,&size,"partition size")) return;
     }
  }
 else
  { if (! scannum(&numbers,&size,"partition size")) return;
    skipspaces(&numbers);
    if (*numbers == '/')
     { sizec = size;
       numbers ++;
       if (! scannum(&numbers,&sizet,"partition size track value")) return;
       skipspaces(&numbers);
       if (*numbers != '/')
	{ printf("invalid c/t/s syntax - no second slash\n");
	  return;
	}
       numbers ++;
       if (! scannum(&numbers,&sizes,"partition size sector value")) return;
       size = sizes + (label.nsect * (sizet + (label.nhead * sizec)));
     }
  }
 if (label.spc && (size % label.spc))
  { printf("Warning: size is not a multiple of cylinder size (is %u/%u/%u)\n",size/label.spc,(size%label.spc)/label.nsect,size%label.nsect);
  }
 label.partitions[pno].startcyl = cyl0;
 label.partitions[pno].nblk = size;
 set_endcyl(&label.partitions[pno]);
 if ( (label.partitions[pno].startcyl*label.spc)+label.partitions[pno].nblk >
      label.spc*label.ncyl )
  { printf("Warning: partition extends beyond end of disk\n");
  }
 label.dirty = 1;
}

/*
 * Change a 128-byte-string field.  There's currently only one such,
 *  the ASCII label field.
 */
static void chval_ascii(const char *cp, FIELD *f)
{
 const char *nl;

 skipspaces(&cp);
 nl = index(cp,'\n');
 if (nl == 0) nl = cp + strlen(cp);
 if (nl-cp > 128)
  { printf("ascii label string too long - max 128 characters\n");
  }
 else
  { bzero(f->loc,128);
    bcopy(cp,f->loc,nl-cp);
    label.dirty = 1;
  }
}

/*
 * Change an int-valued field.  As noted above, there's only one
 *  function, regardless of the field size in the on-disk label.
 */
static void chval_int(const char *cp, FIELD *f)
{
 int v;

 if (! scannum(&cp,&v,"value")) return;
 *(unsigned int *)f->loc = v;
 label.dirty = 1;
}

/*
 * Change a field's value.  The string argument contains the field name
 *  and the new value in text form.  Look up the field and call its
 *  chval and changed functions.
 */
static void chvalue(const char *str)
{
 const char *cp;
 int n;
 int i;

 if (fields[0].taglen < 1)
  { for (i=0;fields[i].tag;i++) fields[i].taglen = strlen(fields[i].tag);
  }
 skipspaces(&str);
 cp = str;
 while (*cp && !isspace(*cp)) cp ++;
 n = cp - str;
 for (i=0;fields[i].tag;i++)
  { if ((n == fields[i].taglen) && !bcmp(str,fields[i].tag,n))
     { (*fields[i].chval)(cp,&fields[i]);
       if (fields[i].changed) (*fields[i].changed)();
       break;
     }
  }
 if (! fields[i].tag)
  { printf("bad name %.*s - see l output for names\n",n,str);
  }
}

/*
 * `changed' function for the ntrack and nsect fields; update label.spc
 *  and call set_endcyl on all partitions.
 */
static void update_spc(void)
{
 int i;

 label.spc = label.nhead * label.nsect;
 for (i=0;i<NPART;i++) set_endcyl(&label.partitions[i]);
}

/*
 * Print function for 128-byte-string fields.  Currently only the ASCII
 *  label, but we don't depend on that.
 */
static int print_ascii(FIELD *f, UNUSED(int sofar))
{
 printf("%s: %.128s\n",f->tag,(char *)f->loc);
 return(0);
}

/*
 * Print an int-valued field.  We are careful to do proper line wrap,
 *  making each value occupy 16 columns.
 */
static int print_int(FIELD *f, int sofar)
{
 if (sofar >= 60)
  { printf("\n");
    sofar = 0;
  }
 printf("%s: %-*u",f->tag,14-(int)strlen(f->tag),*(unsigned int *)f->loc);
 return(sofar+16);
}

/*
 * Print the whole label.  Just call the print function for each field,
 *  then append a newline if necessary.
 */
static void print_label(void)
{
 int i;
 int c;

 c = 0;
 for (i=0;fields[i].tag;i++) c = (*fields[i].print)(&fields[i],c);
 if (c > 0) printf("\n");
}

/*
 * Figure out how many columns wide the screen is.  We impose a minimum
 *  width of 20 columns; I suspect the output code has some issues if
 *  we have fewer columns than partitions.
 */
static int screen_columns(void)
{
 int ncols;
#ifndef NO_TERMCAP_WIDTH
 char *term;
 char tbuf[1024];
#endif
#if defined(TIOCGWINSZ)
 struct winsize wsz;
#elif defined(TIOCGSIZE)
 struct ttysize tsz;
#endif

 ncols = 80;
#ifndef NO_TERMCAP_WIDTH
 term = getenv("TERM");
 if (term && (tgetent(&tbuf[0],term) == 1))
  { int n;
    n = tgetnum("co");
    if (n > 1) ncols = n;
  }
#endif
#if defined(TIOCGWINSZ)
 if ((ioctl(1,TIOCGWINSZ,&wsz) == 0) && (wsz.ws_col > 0))
  { ncols = wsz.ws_col;
  }
#elif defined(TIOCGSIZE)
 if ((ioctl(1,TIOCGSIZE,&tsz) == 0) && (tsz.ts_cols > 0))
  { ncols = tsz.ts_cols;
  }
#endif
 if (ncols < 20) ncols = 20;
 return(ncols);
}

/*
 * Print the partitions.  The argument is true iff we should print all
 *  partitions, even those set start=0 size=0.  We generate one line
 *  per partition (or, if all==0, per `interesting' partition), plus a
 *  visually graphic map of partition letters.  Most of the hair in the
 *  visual display lies in ensuring that nothing takes up less than one
 *  character column, that if two boundaries appear visually identical,
 *  they _are_ identical.  Within that constraint, we try to make the
 *  number of character columns proportional to the size....
 */
static void print_part(int all)
{
 int i;
 int j;
 int k;
 int n;
 int ncols;
 int r;
 int c;
 unsigned int edges[2*NPART];
 int ce[2*NPART];
 int row[NPART];
 unsigned char table[2*NPART][NPART];
 char *line;
#define p label.partitions

 for (i=0;i<NPART;i++)
  { if (all || label.partitions[i].startcyl || label.partitions[i].nblk)
     { printf("%c: start cyl = %6u, size = %8u (",
		PARTLETTER(i),
		label.partitions[i].startcyl, label.partitions[i].nblk );
       if (label.spc)
	{ printf("%u/%u/%u - ",
	  p[i].nblk/label.spc,
	  (p[i].nblk%label.spc)/label.nsect,
	  p[i].nblk%label.nsect );
	}
       printf("%gMb)\n",p[i].nblk/2048.0);
     }
  }
 j = 0;
 for (i=0;i<NPART;i++)
  { if (p[i].nblk > 0)
     { edges[j++] = p[i].startcyl;
       edges[j++] = p[i].endcyl;
     }
  }
 do
  { n = 0;
    for (i=1;i<j;i++)
     { if (edges[i] < edges[i-1])
	{ unsigned int t;
	  t = edges[i];
	  edges[i] = edges[i-1];
	  edges[i-1] = t;
	  n ++;
	}
     }
  } while (n > 0);
 for (i=1;i<j;i++)
  { if (edges[i] != edges[n])
     { n ++;
       if (n != i) edges[n] = edges[i];
     }
  }
 n ++;
 for (i=0;i<NPART;i++)
  { if (p[i].nblk > 0)
     { for (j=0;j<n;j++)
	{ if ( (p[i].startcyl <= edges[j]) &&
	       (p[i].endcyl > edges[j]) )
	   { table[j][i] = 1;
	   }
	  else
	   { table[j][i] = 0;
	   }
	}
     }
  }
 ncols = screen_columns() - 2;
 for (i=0;i<n;i++) ce[i] = (edges[i] * ncols) / (double)edges[n-1];
 for (i=1;i<n;i++) if (ce[i] <= ce[i-1]) ce[i] = ce[i-1] + 1;
 if (ce[n-1] > ncols)
  { ce[n-1] = ncols;
    for (i=n-1;(i>0)&&(ce[i]<=ce[i-1]);i--) ce[i-1] = ce[i] - 1;
    if (ce[0] < 0) for (i=0;i<n;i++) ce[i] = i;
  }
 printf("\n");
 for (i=0;i<NPART;i++)
  { if (p[i].nblk > 0)
     { r = -1;
       do
	{ r ++;
	  for (j=i-1;j>=0;j--)
	   { if (row[j] != r) continue;
	     for (k=0;k<n;k++) if (table[k][i] && table[k][j]) break;
	     if (k < n) break;
	   }
	} while (j >= 0);
       row[i] = r;
     }
    else
     { row[i] = -1;
     }
  }
 r = row[0];
 for (i=1;i<NPART;i++) if (row[i] > r) r = row[i];
 line = malloc(ncols+1);
 for (i=0;i<=r;i++)
  { for (j=0;j<ncols;j++) line[j] = ' ';
    for (j=0;j<NPART;j++)
     { if (row[j] != i) continue;
       k = 0;
       for (k=0;k<n;k++)
	{ if (table[k][j])
	   { for (c=ce[k];c<ce[k+1];c++) line[c] = 'a' + j;
	   }
	}
     }
    for (j=ncols-1;(j>=0)&&(line[j]==' ');j--) ;
    printf("%.*s\n",j+1,line);
  }
 free(line);
#undef p
}

#ifdef S_COMMAND
/*
 * This computes an appropriate checksum for an in-core label.  It's
 *  not really related to the S command, except that it's needed only
 *  by setlabel(), which is #ifdef S_COMMAND.
 */
static unsigned short int dkcksum(const struct disklabel *lp)
{
 const unsigned short int *start;
 const unsigned short int *end;
 unsigned short int sum;
 const unsigned short int *p;

 start = (const void *) lp;
 end = (const void *) &lp->d_partitions[lp->d_npartitions];
 sum = 0;
 for (p=start;p<end;p++) sum ^= *p;
 return(sum);
}
#endif

#ifdef S_COMMAND
/*
 * Set the in-core label.  This is basically putlabel, except it builds
 *  a struct disklabel instead of a Sun label buffer, and uses
 *  DIOCSDINFO instead of lseek-and-write.
 */
static void setlabel(void)
{
 union {
   struct disklabel l;
   char pad[ sizeof(struct disklabel) -
	       (MAXPARTITIONS*sizeof(struct partition)) +
	       (16*sizeof(struct partition)) ];
   } u;
 int i;

 if (ioctl(diskfd,DIOCGDINFO,&u.l) < 0)
  { printf("DIOCGDINFO: %s\n",strerror(errno));
    return;
  }
 if (u.l.d_secsize != 512)
  { printf("warning, disk claims %d-byte sectors\n",(int)u.l.d_secsize);
  }
 u.l.d_nsectors = label.nsect;
 u.l.d_ntracks = label.nhead;
 u.l.d_ncylinders = label.ncyl;
 u.l.d_secpercyl = label.nsect * label.nhead;
 u.l.d_rpm = label.rpm;
 u.l.d_interleave = label.intrlv;
 u.l.d_npartitions = getmaxpartitions();
 bzero(&u.l.d_partitions[0],u.l.d_npartitions*sizeof(struct partition));
 for (i=0;i<u.l.d_npartitions;i++)
  { u.l.d_partitions[i].p_size = label.partitions[i].nblk;
    u.l.d_partitions[i].p_offset = label.partitions[i].startcyl * label.nsect * label.nhead;
    u.l.d_partitions[i].p_fsize = 0;
    u.l.d_partitions[i].p_fstype = (i == 1) ? FS_SWAP :
				   (i == 2) ? FS_UNUSED :
					      FS_BSDFFS;
    u.l.d_partitions[i].p_frag = 0;
    u.l.d_partitions[i].p_cpg = 0;
  }
 u.l.d_checksum = 0;
 u.l.d_checksum = dkcksum(&u.l);
 if (ioctl(diskfd,DIOCSDINFO,&u.l) < 0)
  { printf("DIOCSDINFO: %s\n",strerror(errno));
    return;
  }
}
#endif

/*
 * Read and execute one command line from the user.
 */
static void docmd(void)
{
 char cmdline[512];

 if (! quiet) printf("sunlabel> ");
 if (fgets(&cmdline[0],sizeof(cmdline),stdin) != &cmdline[0]) exit(0);
 switch (cmdline[0])
  { case '?':
       printf("? - print this help\n");
       printf("L - print label, except for partition table\n");
       printf("P - print partition table\n");
       printf("PP - print partition table including size=0 offset=0 entries\n");
       printf("[abcdefghijklmnop] <cylno> <size> - change partition\n");
       printf("V <name> <value> - change a non-partition label value\n");
       printf("W - write (possibly modified) label out\n");
#ifdef S_COMMAND
       printf("S - set label in the kernel (orthogonal to W)\n");
#endif
       printf("Q - quit program (error if no write since last change)\n");
       printf("Q! - quit program (unconditionally) [EOF also quits]\n");
       break;
    case 'L':
       print_label();
       break;
    case 'P':
       print_part(cmdline[1]=='P');
       break;
    case 'W':
       putlabel();
       break;
    case 'S':
#ifdef S_COMMAND
       setlabel();
#else
       printf("This compilation doesn't support S.\n");
#endif
       break;
    case 'Q':
       if ((cmdline[1] == '!') || !label.dirty) exit(0);
       printf("Label is dirty - use w to write it, or Q! to quit anyway.\n");
       break;
    case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h':
    case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p':
       chpart(LETTERPART(cmdline[0]),&cmdline[1]);
       break;
    case 'V':
       chvalue(&cmdline[1]);
       break;
    case '\n':
       break;
    default:
       printf("(Unrecognized command character %c ignored.)\n",cmdline[0]);
       break;
  }
}

/*
 * main() (duh!).  Pretty boring.
 */
int main(int, char **);
int main(int ac, char **av)
{
 handleargs(ac,av);
 getlabel();
 while (1) docmd();
}
