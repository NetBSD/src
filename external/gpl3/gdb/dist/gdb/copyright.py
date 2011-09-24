#! /usr/bin/env python

"""copyright.py

This script updates most of the files that are not already handled
by copyright.sh.  It must be run from the gdb/ subdirectory of the
GDB source tree.

"""

import datetime
import re
import os
import os.path

class Comment(object):
    """A class describing comment.

    ATTRIBUTES
      start:  A string describing how comments are started.
      stop:   A string describing how comments end.  If None, then
              a comment ends at the end of the line.
      start2: Some files accept more than 1 kind of comment.
              For those that do, this is the alternative form.
              For now, it is assumed that if start2 is not None,
              then stop is None (thus no stop2 attribute).
    """
    def __init__(self, start, stop=None, start2=None, max_lines=30):
        """The "Copyright" keyword should be within MAX_LINES lines
        from the start of the file."""
        self.start = start
        self.stop = stop
        self.start2 = start2
        self.max_lines = max_lines

# The Comment object for Ada code (and GPR files).
ADA_COMMENT = Comment(start="--")

THIS_YEAR = str(datetime.date.today().year)

# Files which should not be modified, either because they are
# generated, non-FSF, or otherwise special (e.g. license text,
# or test cases which must be sensitive to line numbering).
EXCLUSION_LIST = (
  "COPYING", "COPYING.LIB", "CVS", "configure", "copying.c", "gdbarch.c",
  "gdbarch.h", "fdl.texi", "gpl.texi", "gdbtk", "gdb.gdbtk", "osf-share",
  "aclocal.m4", "step-line.inp", "step-line.c",
  )

# Files that are too different from the rest to be processed automatically.
BY_HAND = ['../sim/ppc/psim.texinfo']

# Files for which we know that they do not have a copyright header.
# Ideally, this list should be empty (but it may not be possible to
# add a copyright header in some of them).
NO_COPYRIGHT = (
   # Configure files.  We should fix those, one day.
   "testsuite/gdb.cell/configure.ac", "testsuite/gdb.hp/configure.ac",
   "testsuite/gdb.hp/gdb.aCC/configure.ac",
   "testsuite/gdb.hp/gdb.base-hp/configure.ac",
   "testsuite/gdb.hp/gdb.compat/configure.ac",
   "testsuite/gdb.hp/gdb.defects/configure.ac",
   "testsuite/gdb.hp/gdb.objdbg/configure.ac",
   "testsuite/gdb.stabs/configure.ac",
   "../sim/arm/configure.ac", "../sim/avr/configure.ac",
   "../sim/common/configure.ac", "../sim/configure.ac",
   "../sim/cr16/configure.ac", "../sim/cris/configure.ac",
   "../sim/d10v/configure.ac", "../sim/erc32/configure.ac",
   "../sim/frv/configure.ac", "../sim/h8300/configure.ac",
   "../sim/igen/configure.ac", "../sim/iq2000/configure.ac",
   "../sim/lm32/configure.ac", "../sim/m32r/configure.ac",
   "../sim/m68hc11/configure.ac", "../sim/mcore/configure.ac",
   "../sim/microblaze/configure.ac", "../sim/mips/configure.ac",
   "../sim/mn10300/configure.ac", "../sim/moxie/configure.ac",
   "../sim/ppc/configure.ac", "../sim/sh/configure.ac",
   "../sim/sh64/configure.ac", "../sim/testsuite/configure.ac",
   "../sim/testsuite/d10v-elf/configure.ac",
   "../sim/testsuite/frv-elf/configure.ac",
   "../sim/testsuite/m32r-elf/configure.ac",
   "../sim/testsuite/mips64el-elf/configure.ac", "../sim/v850/configure.ac",
   # Assembly files.  It's not certain that we can add a copyright
   # header in a way that works for all platforms supported by the
   # testcase...
   "testsuite/gdb.arch/pa-nullify.s", "testsuite/gdb.arch/pa64-nullify.s",
   "testsuite/gdb.asm/asmsrc1.s", "testsuite/gdb.asm/asmsrc2.s",
   "testsuite/gdb.disasm/am33.s", "testsuite/gdb.disasm/h8300s.s",
   "testsuite/gdb.disasm/hppa.s", "testsuite/gdb.disasm/mn10200.s",
   "testsuite/gdb.disasm/mn10300.s", "testsuite/gdb.disasm/sh3.s",
   "testsuite/gdb.disasm/t01_mov.s", "testsuite/gdb.disasm/t02_mova.s",
   "testsuite/gdb.disasm/t03_add.s", "testsuite/gdb.disasm/t04_sub.s",
   "testsuite/gdb.disasm/t05_cmp.s", "testsuite/gdb.disasm/t06_ari2.s",
   "testsuite/gdb.disasm/t07_ari3.s", "testsuite/gdb.disasm/t08_or.s",
   "testsuite/gdb.disasm/t09_xor.s", "testsuite/gdb.disasm/t10_and.s",
   "testsuite/gdb.disasm/t11_logs.s", "testsuite/gdb.disasm/t12_bit.s",
   "testsuite/gdb.disasm/t13_otr.s", "testsuite/gdb.hp/gdb.base-hp/reg-pa64.s",
   "testsuite/gdb.hp/gdb.base-hp/reg.s",
   "../sim/testsuite/d10v-elf/exit47.s",
   "../sim/testsuite/d10v-elf/hello.s",
   "../sim/testsuite/d10v-elf/loop.s",
   "../sim/testsuite/d10v-elf/t-ae-ld-d.s",
   "../sim/testsuite/d10v-elf/t-ae-ld-i.s",
   "../sim/testsuite/d10v-elf/t-ae-ld-id.s",
   "../sim/testsuite/d10v-elf/t-ae-ld-im.s",
   "../sim/testsuite/d10v-elf/t-ae-ld-ip.s",
   "../sim/testsuite/d10v-elf/t-ae-ld2w-d.s",
   "../sim/testsuite/d10v-elf/t-ae-ld2w-i.s",
   "../sim/testsuite/d10v-elf/t-ae-ld2w-id.s",
   "../sim/testsuite/d10v-elf/t-ae-ld2w-im.s",
   "../sim/testsuite/d10v-elf/t-ae-ld2w-ip.s",
   "../sim/testsuite/d10v-elf/t-ae-st-d.s",
   "../sim/testsuite/d10v-elf/t-ae-st-i.s",
   "../sim/testsuite/d10v-elf/t-ae-st-id.s",
   "../sim/testsuite/d10v-elf/t-ae-st-im.s",
   "../sim/testsuite/d10v-elf/t-ae-st-ip.s",
   "../sim/testsuite/d10v-elf/t-ae-st-is.s",
   "../sim/testsuite/d10v-elf/t-ae-st2w-d.s",
   "../sim/testsuite/d10v-elf/t-ae-st2w-i.s",
   "../sim/testsuite/d10v-elf/t-ae-st2w-id.s",
   "../sim/testsuite/d10v-elf/t-ae-st2w-im.s",
   "../sim/testsuite/d10v-elf/t-ae-st2w-ip.s",
   "../sim/testsuite/d10v-elf/t-ae-st2w-is.s",
   "../sim/testsuite/d10v-elf/t-dbt.s",
   "../sim/testsuite/d10v-elf/t-ld-st.s",
   "../sim/testsuite/d10v-elf/t-mac.s",
   "../sim/testsuite/d10v-elf/t-mod-ld-pre.s",
   "../sim/testsuite/d10v-elf/t-msbu.s",
   "../sim/testsuite/d10v-elf/t-mulxu.s",
   "../sim/testsuite/d10v-elf/t-mvtac.s",
   "../sim/testsuite/d10v-elf/t-mvtc.s",
   "../sim/testsuite/d10v-elf/t-rac.s",
   "../sim/testsuite/d10v-elf/t-rachi.s",
   "../sim/testsuite/d10v-elf/t-rdt.s",
   "../sim/testsuite/d10v-elf/t-rep.s",
   "../sim/testsuite/d10v-elf/t-rie-xx.s",
   "../sim/testsuite/d10v-elf/t-rte.s",
   "../sim/testsuite/d10v-elf/t-sac.s",
   "../sim/testsuite/d10v-elf/t-sachi.s",
   "../sim/testsuite/d10v-elf/t-sadd.s",
   "../sim/testsuite/d10v-elf/t-slae.s",
   "../sim/testsuite/d10v-elf/t-sp.s",
   "../sim/testsuite/d10v-elf/t-sub.s",
   "../sim/testsuite/d10v-elf/t-sub2w.s",
   "../sim/testsuite/d10v-elf/t-subi.s",
   "../sim/testsuite/d10v-elf/t-trap.s",
   "../sim/testsuite/frv-elf/cache.s",
   "../sim/testsuite/frv-elf/exit47.s",
   "../sim/testsuite/frv-elf/grloop.s",
   "../sim/testsuite/frv-elf/hello.s",
   "../sim/testsuite/frv-elf/loop.s",
   "../sim/testsuite/m32r-elf/exit47.s",
   "../sim/testsuite/m32r-elf/hello.s",
   "../sim/testsuite/m32r-elf/loop.s",
   "../sim/testsuite/sim/cris/hw/rv-n-cris/quit.s",
   "../sim/testsuite/sim/h8300/addb.s",
   "../sim/testsuite/sim/h8300/addl.s",
   "../sim/testsuite/sim/h8300/adds.s",
   "../sim/testsuite/sim/h8300/addw.s",
   "../sim/testsuite/sim/h8300/addx.s",
   "../sim/testsuite/sim/h8300/andb.s",
   "../sim/testsuite/sim/h8300/andl.s",
   "../sim/testsuite/sim/h8300/andw.s",
   "../sim/testsuite/sim/h8300/band.s",
   "../sim/testsuite/sim/h8300/bfld.s",
   "../sim/testsuite/sim/h8300/biand.s",
   "../sim/testsuite/sim/h8300/bra.s",
   "../sim/testsuite/sim/h8300/brabc.s",
   "../sim/testsuite/sim/h8300/bset.s",
   "../sim/testsuite/sim/h8300/cmpb.s",
   "../sim/testsuite/sim/h8300/cmpl.s",
   "../sim/testsuite/sim/h8300/cmpw.s",
   "../sim/testsuite/sim/h8300/daa.s",
   "../sim/testsuite/sim/h8300/das.s",
   "../sim/testsuite/sim/h8300/dec.s",
   "../sim/testsuite/sim/h8300/div.s",
   "../sim/testsuite/sim/h8300/extl.s",
   "../sim/testsuite/sim/h8300/extw.s",
   "../sim/testsuite/sim/h8300/inc.s",
   "../sim/testsuite/sim/h8300/jmp.s",
   "../sim/testsuite/sim/h8300/ldc.s",
   "../sim/testsuite/sim/h8300/ldm.s",
   "../sim/testsuite/sim/h8300/mac.s",
   "../sim/testsuite/sim/h8300/mova.s",
   "../sim/testsuite/sim/h8300/movb.s",
   "../sim/testsuite/sim/h8300/movl.s",
   "../sim/testsuite/sim/h8300/movmd.s",
   "../sim/testsuite/sim/h8300/movsd.s",
   "../sim/testsuite/sim/h8300/movw.s",
   "../sim/testsuite/sim/h8300/mul.s",
   "../sim/testsuite/sim/h8300/neg.s",
   "../sim/testsuite/sim/h8300/nop.s",
   "../sim/testsuite/sim/h8300/not.s",
   "../sim/testsuite/sim/h8300/orb.s",
   "../sim/testsuite/sim/h8300/orl.s",
   "../sim/testsuite/sim/h8300/orw.s",
   "../sim/testsuite/sim/h8300/rotl.s",
   "../sim/testsuite/sim/h8300/rotr.s",
   "../sim/testsuite/sim/h8300/rotxl.s",
   "../sim/testsuite/sim/h8300/rotxr.s",
   "../sim/testsuite/sim/h8300/shal.s",
   "../sim/testsuite/sim/h8300/shar.s",
   "../sim/testsuite/sim/h8300/shll.s",
   "../sim/testsuite/sim/h8300/shlr.s",
   "../sim/testsuite/sim/h8300/stack.s",
   "../sim/testsuite/sim/h8300/stc.s",
   "../sim/testsuite/sim/h8300/subb.s",
   "../sim/testsuite/sim/h8300/subl.s",
   "../sim/testsuite/sim/h8300/subs.s",
   "../sim/testsuite/sim/h8300/subw.s",
   "../sim/testsuite/sim/h8300/subx.s",
   "../sim/testsuite/sim/h8300/tas.s",
   "../sim/testsuite/sim/h8300/xorb.s",
   "../sim/testsuite/sim/h8300/xorl.s",
   "../sim/testsuite/sim/h8300/xorw.s",
   "../sim/testsuite/sim/mips/fpu64-ps-sb1.s",
   "../sim/testsuite/sim/mips/fpu64-ps.s",
   "../sim/testsuite/sim/mips/hilo-hazard-1.s",
   "../sim/testsuite/sim/mips/hilo-hazard-2.s",
   "../sim/testsuite/sim/mips/hilo-hazard-3.s",
   "../sim/testsuite/sim/mips/mdmx-ob-sb1.s",
   "../sim/testsuite/sim/mips/mdmx-ob.s",
   "../sim/testsuite/sim/mips/mips32-dsp.s",
   "../sim/testsuite/sim/mips/mips32-dsp2.s",
   "../sim/testsuite/sim/mips/sanity.s",
   "../sim/testsuite/sim/sh/add.s",
   "../sim/testsuite/sim/sh/and.s",
   "../sim/testsuite/sim/sh/bandor.s",
   "../sim/testsuite/sim/sh/bandornot.s",
   "../sim/testsuite/sim/sh/bclr.s",
   "../sim/testsuite/sim/sh/bld.s",
   "../sim/testsuite/sim/sh/bldnot.s",
   "../sim/testsuite/sim/sh/bset.s",
   "../sim/testsuite/sim/sh/bst.s",
   "../sim/testsuite/sim/sh/bxor.s",
   "../sim/testsuite/sim/sh/clip.s",
   "../sim/testsuite/sim/sh/div.s",
   "../sim/testsuite/sim/sh/dmxy.s",
   "../sim/testsuite/sim/sh/fabs.s",
   "../sim/testsuite/sim/sh/fadd.s",
   "../sim/testsuite/sim/sh/fail.s",
   "../sim/testsuite/sim/sh/fcmpeq.s",
   "../sim/testsuite/sim/sh/fcmpgt.s",
   "../sim/testsuite/sim/sh/fcnvds.s",
   "../sim/testsuite/sim/sh/fcnvsd.s",
   "../sim/testsuite/sim/sh/fdiv.s",
   "../sim/testsuite/sim/sh/fipr.s",
   "../sim/testsuite/sim/sh/fldi0.s",
   "../sim/testsuite/sim/sh/fldi1.s",
   "../sim/testsuite/sim/sh/flds.s",
   "../sim/testsuite/sim/sh/float.s",
   "../sim/testsuite/sim/sh/fmac.s",
   "../sim/testsuite/sim/sh/fmov.s",
   "../sim/testsuite/sim/sh/fmul.s",
   "../sim/testsuite/sim/sh/fneg.s",
   "../sim/testsuite/sim/sh/fpchg.s",
   "../sim/testsuite/sim/sh/frchg.s",
   "../sim/testsuite/sim/sh/fsca.s",
   "../sim/testsuite/sim/sh/fschg.s",
   "../sim/testsuite/sim/sh/fsqrt.s",
   "../sim/testsuite/sim/sh/fsrra.s",
   "../sim/testsuite/sim/sh/fsub.s",
   "../sim/testsuite/sim/sh/ftrc.s",
   "../sim/testsuite/sim/sh/ldrc.s",
   "../sim/testsuite/sim/sh/loop.s",
   "../sim/testsuite/sim/sh/macl.s",
   "../sim/testsuite/sim/sh/macw.s",
   "../sim/testsuite/sim/sh/mov.s",
   "../sim/testsuite/sim/sh/movi.s",
   "../sim/testsuite/sim/sh/movli.s",
   "../sim/testsuite/sim/sh/movua.s",
   "../sim/testsuite/sim/sh/movxy.s",
   "../sim/testsuite/sim/sh/mulr.s",
   "../sim/testsuite/sim/sh/pabs.s",
   "../sim/testsuite/sim/sh/padd.s",
   "../sim/testsuite/sim/sh/paddc.s",
   "../sim/testsuite/sim/sh/pand.s",
   "../sim/testsuite/sim/sh/pass.s",
   "../sim/testsuite/sim/sh/pclr.s",
   "../sim/testsuite/sim/sh/pdec.s",
   "../sim/testsuite/sim/sh/pdmsb.s",
   "../sim/testsuite/sim/sh/pinc.s",
   "../sim/testsuite/sim/sh/pmuls.s",
   "../sim/testsuite/sim/sh/prnd.s",
   "../sim/testsuite/sim/sh/pshai.s",
   "../sim/testsuite/sim/sh/pshar.s",
   "../sim/testsuite/sim/sh/pshli.s",
   "../sim/testsuite/sim/sh/pshlr.s",
   "../sim/testsuite/sim/sh/psub.s",
   "../sim/testsuite/sim/sh/pswap.s",
   "../sim/testsuite/sim/sh/pushpop.s",
   "../sim/testsuite/sim/sh/resbank.s",
   "../sim/testsuite/sim/sh/sett.s",
   "../sim/testsuite/sim/sh/shll.s",
   "../sim/testsuite/sim/sh/shll16.s",
   "../sim/testsuite/sim/sh/shll2.s",
   "../sim/testsuite/sim/sh/shll8.s",
   "../sim/testsuite/sim/sh/shlr.s",
   "../sim/testsuite/sim/sh/shlr16.s",
   "../sim/testsuite/sim/sh/shlr2.s",
   "../sim/testsuite/sim/sh/shlr8.s",
   "../sim/testsuite/sim/sh/swap.s",
   "../sim/testsuite/sim/sh64/misc/fr-dr.s",
   # .inc files.  These are usually assembly or C files...
   "testsuite/gdb.asm/alpha.inc",
   "testsuite/gdb.asm/arm.inc",
   "testsuite/gdb.asm/common.inc",
   "testsuite/gdb.asm/empty.inc",
   "testsuite/gdb.asm/frv.inc",
   "testsuite/gdb.asm/h8300.inc",
   "testsuite/gdb.asm/i386.inc",
   "testsuite/gdb.asm/ia64.inc",
   "testsuite/gdb.asm/iq2000.inc",
   "testsuite/gdb.asm/m32c.inc",
   "testsuite/gdb.asm/m32r-linux.inc",
   "testsuite/gdb.asm/m32r.inc",
   "testsuite/gdb.asm/m68hc11.inc",
   "testsuite/gdb.asm/m68k.inc",
   "testsuite/gdb.asm/mips.inc",
   "testsuite/gdb.asm/netbsd.inc",
   "testsuite/gdb.asm/openbsd.inc",
   "testsuite/gdb.asm/pa.inc",
   "testsuite/gdb.asm/pa64.inc",
   "testsuite/gdb.asm/powerpc.inc",
   "testsuite/gdb.asm/powerpc64.inc",
   "testsuite/gdb.asm/s390.inc",
   "testsuite/gdb.asm/s390x.inc",
   "testsuite/gdb.asm/sh.inc",
   "testsuite/gdb.asm/sparc.inc",
   "testsuite/gdb.asm/sparc64.inc",
   "testsuite/gdb.asm/spu.inc",
   "testsuite/gdb.asm/v850.inc",
   "testsuite/gdb.asm/x86_64.inc",
   "testsuite/gdb.asm/xstormy16.inc",
   "../sim/testsuite/sim/arm/iwmmxt/testutils.inc",
   "../sim/testsuite/sim/arm/testutils.inc",
   "../sim/testsuite/sim/arm/thumb/testutils.inc",
   "../sim/testsuite/sim/arm/xscale/testutils.inc",
   "../sim/testsuite/sim/cr16/testutils.inc",
   "../sim/testsuite/sim/cris/asm/testutils.inc",
   "../sim/testsuite/sim/cris/hw/rv-n-cris/testutils.inc",
   "../sim/testsuite/sim/fr30/testutils.inc",
   "../sim/testsuite/sim/frv/testutils.inc",
   "../sim/testsuite/sim/h8300/testutils.inc",
   "../sim/testsuite/sim/m32r/testutils.inc",
   "../sim/testsuite/sim/sh/testutils.inc",
   "../sim/testsuite/sim/sh64/compact/testutils.inc",
   "../sim/testsuite/sim/sh64/media/testutils.inc",
   "../sim/testsuite/sim/v850/testutils.inc",
   )

# A mapping between file extensions to their associated Comment object.
# This dictionary also contains a number of exceptions, based on
# filename.
COMMENT_MAP = \
  {".1" : Comment(start=r'.\"'),
   ".ac" : Comment(start="dnl", start2="#"),
   ".ads" : ADA_COMMENT,
   ".adb" : ADA_COMMENT,
   ".f" : Comment(start="c"),
   ".f90" : Comment(start="!"),
   ".gpr" : ADA_COMMENT,
   ".inc" : Comment(start="#", start2=";"),
   ".s" : Comment(start="!"),
   ".tex" : Comment(start="%"),
   ".texi" : Comment(start="@c"),
   ".texinfo" : Comment(start="@c"),

   # Files that use a different way of including the copyright
   # header...
   "ada-operator.inc" : Comment(start="/*", stop="*/"),
   "gdbint.texinfo" : Comment(start='@copying', stop="@end copying"),
   "annotate.texinfo" : Comment(start='@copying', stop="@end copying",
                                max_lines=50),
   "stabs.texinfo" : Comment(start='@copying', stop="@end copying"),
  }

class NotFound(Exception):
    pass

class AlreadyDone(Exception):
    pass

def process_header(src, dst, cdescr):
    """Read from SRC for up to CDESCR.MAX_LINES until we find a copyright
    notice.  If found, then write the entire file, with the copyright
    noticed updated with the current year added.

    Raises NotFound if the copyright notice could not be found or has
    some inconsistencies.

    Raises AlreadyDone if the copyright notice already includes the current
    year.
    """
    line_count = 0
    # The start-of-comment marker used for this file.  Only really useful
    # in the case where comments ends at the end of the line, as this
    # allows us to know which comment marker to use when breaking long
    # lines (in the cases where there are more than one.
    cdescr_start = ""

    while True:
        # If we still haven't found a copyright line within a certain
        # number of lines, then give up.
        if line_count > cdescr.max_lines:
            raise NotFound("start of Copyright not found")

        line = src.readline()
        line_count += 1
        if not line:
            raise NotFound("start of Copyright not found (EOF)")

        # Is this a copyright line?  If not, then no transformation is
        # needed.  Write it as is, and continue.
        if not re.search(r"Copyright\b.*\b(199\d|20\d\d)\b", line):
            dst.write(line)
            continue

        # If a start-of-comment marker is needed for every line, try to
        # figure out which one it is that is being used in this file (most
        # files only accept one, in which case it's easy - but some accept
        # two or more...).
        if cdescr.stop is None:
            stripped_line = line.lstrip()
            if stripped_line.startswith(cdescr.start):
                cdescr_start = cdescr.start
            elif (cdescr.start2 is not None
                  and stripped_line.startswith(cdescr.start2)):
                cdescr_start = cdescr.start2
            elif cdescr.start in stripped_line:
                cdescr_start = cdescr.start
            elif (cdescr.start2 is not None
                  and cdescr.start2 in stripped_line):
                cdescr_start = cdescr.start2
            else:
                # This can't be a line with a comment, so not the copyright
                # line we were looking for.  Ignore.
                continue

        comment = line
        break

    while not re.search(r"Free\s+Software\s+Foundation", comment):
        line = src.readline()
        line_count += 1
        if not line:
            raise NotFound("Copyright owner not found (EOF)")

        if cdescr.stop is None:
            # Expect a new comment marker at the start of each line
            line = line.lstrip()
            if not line.startswith(cdescr_start):
                raise NotFound("Copyright owner not found "
                               "(end of comment)")
            comment += " " + line[len(cdescr_start):]
        else:
            if cdescr.stop in comment:
                raise NotFound("Copyright owner not found "
                               "(end of comment)")
            comment += line

    # Normalize a bit the copyright string (we preserve the string
    # up until "Copyright", in order to help preserve any original
    # alignment.
    (before, after) = comment.split("Copyright", 1)
    after = after.replace("\n", " ")
    after = re.sub("\s+", " ", after)
    after = after.rstrip()

    # If the copyright year has already been added, the nothing else
    # to do.
    if THIS_YEAR in after:
        raise AlreadyDone

    m = re.match("(.*[0-9]+)(.*)", after)
    if m is None:
        raise NotFound("Internal error - cannot split copyright line: "
                       "`%s'" % comment)

    # Reconstruct the comment line
    comment = before + "Copyright" + m.group(1) + ', %s' % THIS_YEAR
    owner_part = m.group(2).lstrip()

    # Max comment len...
    max_len = 76

    # If we have to break the copyright line into multiple lines,
    # we want to align all the lines on the "Copyright" keyword.
    # Create a small "indent" string that we can use for that.
    if cdescr.stop is None:
        # The comment marker is needed on every line, so put it at the
        # start of our "indent" string.
        indent = cdescr_start + ' ' * (len(before) - len(cdescr_start))
    else:
        indent = ' ' * len(before)

    # If the line is too long...
    while len(comment) > max_len:
        # Split the line at the first space before max_len.
        space_index = comment[0:max_len].rfind(' ')
        if space_index < 0:  # No space in the first max_len characters???
            # Split at the first space, then...
            space_index = comment.find(' ')
        if space_index < 0:
            # Still no space found.  This is extremely unlikely, but
            # just pretend there is one at the end of the string.
            space_index = len(comment)

        # Write the first part of the string up until the space
        # we selected to break our line.
        dst.write(comment[:space_index] + '\n')

        # Strip the part of comment that we have finished printing.
        if space_index < len(comment):
            comment = comment[space_index + 1:]
        else:
            comment = ""

        # Prepend the "indent" string to make sure that we remain
        # aligned on the "Copyright" word.
        comment = indent + comment

    # And finally, write the rest of the last line...  We want to write
    # "Free Software Foundation, Inc" on the same line, so handle this
    # with extra care.
    dst.write(comment)
    if len(comment) + 1 + len (owner_part) > max_len:
        dst.write('\n' + indent)
    else:
        dst.write(' ')
    dst.write(owner_part + '\n')

def comment_for_filename(filename):
    """Return the Comment object that best describes the given file.
    This a smart lookup of the COMMENT_MAP dictionary where we check
    for filename-based exceptions first, before looking up the comment
    by filename extension.  """
    # First, consult the COMMENT_MAP using the filename, in case this
    # file needs special treatment.
    basename = os.path.basename(filename)
    if basename in COMMENT_MAP:
        return COMMENT_MAP[basename]
    # Not a special file.  Check the file extension.
    ext = os.path.splitext(filename)[1]
    if ext in COMMENT_MAP:
        return COMMENT_MAP[ext]
    # Not a know extension either, return None.
    return None

def process_file(filename):
    """Processes the given file.
    """
    cdescr = comment_for_filename(filename)
    if cdescr is None:
        # Either no filename extension, or not an extension that we
        # know how to handle.
        return

    dst_filename = filename + '.new'
    src = open(filename)
    dst = open(dst_filename, 'w')
    try:
        process_header(src, dst, cdescr)
    except AlreadyDone:
        print "+++ Already up to date: `%s'." % filename
        dst.close()
        os.unlink(dst_filename)
        if filename in NO_COPYRIGHT:
            # We expect the search for a copyright header to fail, and
            # yet we found one...
            print "Warning: `%s' should not be in NO_COPYRIGHT" % filename
        return
    except NotFound as inst:
        dst.close()
        os.unlink(dst_filename)
        if not filename in NO_COPYRIGHT:
            print "*** \033[31m%s\033[0m: %s" % (filename, inst)
        return

    if filename in NO_COPYRIGHT:
        # We expect the search for a copyright header to fail, and
        # yet we found one...
        print "Warning: `%s' should not be in NO_COPYRIGHT" % filename

    for line in src:
        dst.write(line)
    src.close()
    dst.close()
    os.rename(dst_filename, filename)

if __name__ == "__main__":
    if not os.path.isfile("doc/gdb.texinfo"):
        print "Error: This script must be called from the gdb directory."
    for gdb_dir in ('.', '../sim', '../include/gdb'):
        for root, dirs, files in os.walk(gdb_dir):
            for filename in files:
                fullpath = os.path.join(root, filename)
                if fullpath.startswith('./'):
                    fullpath = fullpath[2:]
                if filename not in EXCLUSION_LIST and fullpath not in BY_HAND:
                    # Paths that start with './' are ugly, so strip that.
                    # This also allows us to omit them in the NO_COPYRIGHT
                    # list...
                    process_file(fullpath)
    print
    print "\033[32mREMINDER: The following files must be updated by hand." \
          "\033[0m"
    for filename in BY_HAND:
        print "  ", filename

