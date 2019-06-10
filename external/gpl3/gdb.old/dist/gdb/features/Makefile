# Copyright (C) 2007-2017 Free Software Foundation, Inc.

# This file is part of GDB.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


# This file requires GNU make!

# This Makefile updates files in ../regformats from their XML master
# copies.  Because this relies on xsltproc, it is not normally run
# during the build process; it should be run by hand when one of the
# affected XML files is changed, and the results should be kept in the
# GDB repository.

# It can also update the C files in the features directory from their
# XML master copies.  This relies on a GDB linked with expat and that
# is configured to include support for the architecture, so the files
# are kept in the GDB repository.
#
# By default we'll generate all C files, and you must point make at a
# GDB that has been configured with --enable-targets=all:
#
#   make GDB=/path/to/gdb cfiles
#
# To regenerate all C files from scratch:
#
#   make GDB=/path/to/gdb clean-cfiles cfiles
#
# To generate specific C files, you can override the XMLTOC make
# variable:
#
#   make GDB=/path/to/gdb XMLTOC="xml files" cfiles

WHICH = aarch64 \
	arm/arm-with-iwmmxt arm/arm-with-vfpv2 arm/arm-with-vfpv3 \
	arm/arm-with-neon \
	i386/i386 i386/i386-linux \
	i386/i386-mmx i386/i386-mmx-linux \
	i386/amd64 i386/amd64-linux \
	i386/i386-avx i386/i386-avx-linux \
	i386/i386-mpx i386/i386-mpx-linux \
	i386/i386-avx-mpx i386/i386-avx-mpx-linux \
	i386/i386-avx-avx512 i386/i386-avx-avx512-linux \
	i386/i386-avx-mpx-avx512-pku i386/i386-avx-mpx-avx512-pku-linux \
	i386/amd64-avx i386/amd64-avx-linux \
	i386/amd64-mpx i386/amd64-mpx-linux \
	i386/amd64-avx-mpx i386/amd64-avx-mpx-linux \
	i386/amd64-avx-avx512 i386/amd64-avx-avx512-linux \
	i386/amd64-avx-mpx-avx512-pku i386/amd64-avx-mpx-avx512-pku-linux \
	i386/x32 i386/x32-linux \
	i386/x32-avx i386/x32-avx-linux \
	i386/x32-avx-avx512 i386/x32-avx-avx512-linux \
	mips-linux mips-dsp-linux \
	microblaze-with-stack-protect \
	mips64-linux mips64-dsp-linux \
	nios2-linux \
	rs6000/powerpc-32 \
	rs6000/powerpc-32l rs6000/powerpc-altivec32l rs6000/powerpc-e500l \
	rs6000/powerpc-64l rs6000/powerpc-altivec64l rs6000/powerpc-vsx32l \
	rs6000/powerpc-vsx64l rs6000/powerpc-cell32l rs6000/powerpc-cell64l \
	rs6000/powerpc-isa205-32l rs6000/powerpc-isa205-64l \
	rs6000/powerpc-isa205-altivec32l rs6000/powerpc-isa205-altivec64l \
	rs6000/powerpc-isa205-vsx32l rs6000/powerpc-isa205-vsx64l \
	s390-linux32 s390-linux64 s390x-linux64 \
	s390-linux32v1 s390-linux64v1 s390x-linux64v1 \
	s390-linux32v2 s390-linux64v2 s390x-linux64v2 \
	s390-te-linux64 s390x-te-linux64 s390-vx-linux64 s390x-vx-linux64 \
	s390-tevx-linux64 s390x-tevx-linux64 \
	tic6x-c64xp tic6x-c64x tic6x-c62x \
	tic6x-c64xp-linux tic6x-c64x-linux tic6x-c62x-linux

# Record which registers should be sent to GDB by default after stop.
aarch64-expedite = x29,sp,pc
arm-expedite = r11,sp,pc
i386-expedite = ebp,esp,eip
amd64-expedite = rbp,rsp,rip
x32-expedite = rbp,rsp,rip
mips-expedite = r29,pc
mips-dsp-expedite = r29,pc
mips64-expedite = r29,pc
mips64-dsp-expedite = r29,pc
microblaze-expedite = r1,rpc
nios2-linux-expedite = sp,pc
powerpc-expedite = r1,pc
rs6000/powerpc-cell32l-expedite = r1,pc,r0,orig_r3,r4
rs6000/powerpc-cell64l-expedite = r1,pc,r0,orig_r3,r4
s390-linux32-expedite = r14,r15,pswa
s390-linux32v1-expedite = r14,r15,pswa
s390-linux32v2-expedite = r14,r15,pswa
s390-linux64-expedite = r14l,r15l,pswa
s390-linux64v1-expedite = r14l,r15l,pswa
s390-linux64v2-expedite = r14l,r15l,pswa
s390-te-linux64-expedite = r14l,r15l,pswa
s390-vx-linux64-expedite = r14l,r15l,pswa
s390-tevx-linux64-expedite = r14l,r15l,pswa
s390x-linux64-expedite = r14,r15,pswa
s390x-linux64v1-expedite = r14,r15,pswa
s390x-linux64v2-expedite = r14,r15,pswa
s390x-te-linux64-expedite = r14,r15,pswa
s390x-vx-linux64-expedite = r14,r15,pswa
s390x-tevx-linux64-expedite = r14,r15,pswa
tic6x-c64xp-expedite = A15,PC
tic6x-c64x-expedite = A15,PC
tic6x-c62x-expedite = A15,PC
tic6x-c64xp-linux-expedite = A15,PC
tic6x-c64x-linux-expedite = A15,PC
tic6x-c62x-linux-expedite = A15,PC


XSLTPROC = xsltproc
outdir = ../regformats
OUTPUTS = $(patsubst %,$(outdir)/%.dat,$(WHICH))

# The set of xml files we'll generate .c files for GDB from.  By
# default we'll build all .c files, which requires an
# --enable-targets=all GDB.  You can override this by passing XMLTOC
# to make on the command line.
XMLTOC = \
	aarch64.xml \
	arc-v2.xml \
	arc-arcompact.xml \
	arm/arm-with-iwmmxt.xml \
	arm/arm-with-m-fpa-layout.xml \
	arm/arm-with-m-vfp-d16.xml \
	arm/arm-with-m.xml \
	arm/arm-with-neon.xml \
	arm/arm-with-vfpv2.xml \
	arm/arm-with-vfpv3.xml \
	i386/amd64-avx-linux.xml \
	i386/amd64-avx.xml \
	i386/amd64-avx-avx512-linux.xml \
	i386/amd64-avx-avx512.xml \
	i386/amd64-avx-mpx-avx512-pku-linux.xml \
	i386/amd64-avx-mpx-avx512-pku.xml \
	i386/amd64-linux.xml \
	i386/amd64-mpx-linux.xml \
	i386/amd64-mpx.xml \
	i386/amd64-avx-mpx-linux.xml \
	i386/amd64-avx-mpx.xml \
	i386/amd64.xml \
	i386/i386-avx-linux.xml \
	i386/i386-avx.xml \
	i386/i386-avx-avx512-linux.xml \
	i386/i386-avx-avx512.xml \
	i386/i386-avx-mpx-avx512-pku-linux.xml \
	i386/i386-avx-mpx-avx512-pku.xml \
	i386/i386-linux.xml \
	i386/i386-mmx-linux.xml \
	i386/i386-mmx.xml \
	i386/i386-mpx-linux.xml \
	i386/i386-mpx.xml \
	i386/i386-avx-mpx-linux.xml \
	i386/i386-avx-mpx.xml \
	i386/i386.xml \
	i386/x32-avx-linux.xml \
	i386/x32-avx.xml \
	i386/x32-avx-avx512-linux.xml \
	i386/x32-avx-avx512.xml \
	i386/x32-linux.xml \
	i386/x32.xml \
	microblaze-with-stack-protect.xml \
	microblaze.xml \
	mips-dsp-linux.xml \
	mips-linux.xml \
	mips64-dsp-linux.xml \
	mips64-linux.xml \
	nds32.xml \
	nios2-linux.xml \
	nios2.xml \
	rs6000/powerpc-32.xml \
	rs6000/powerpc-32l.xml \
	rs6000/powerpc-403.xml \
	rs6000/powerpc-403gc.xml \
	rs6000/powerpc-405.xml \
	rs6000/powerpc-505.xml \
	rs6000/powerpc-601.xml \
	rs6000/powerpc-602.xml \
	rs6000/powerpc-603.xml \
	rs6000/powerpc-604.xml \
	rs6000/powerpc-64.xml \
	rs6000/powerpc-64l.xml \
	rs6000/powerpc-7400.xml \
	rs6000/powerpc-750.xml \
	rs6000/powerpc-860.xml \
	rs6000/powerpc-altivec32.xml \
	rs6000/powerpc-altivec32l.xml \
	rs6000/powerpc-altivec64.xml \
	rs6000/powerpc-altivec64l.xml \
	rs6000/powerpc-cell32l.xml \
	rs6000/powerpc-cell64l.xml \
	rs6000/powerpc-e500.xml \
	rs6000/powerpc-e500l.xml \
	rs6000/powerpc-isa205-32l.xml \
	rs6000/powerpc-isa205-64l.xml \
	rs6000/powerpc-isa205-altivec32l.xml \
	rs6000/powerpc-isa205-altivec64l.xml \
	rs6000/powerpc-isa205-vsx32l.xml \
	rs6000/powerpc-isa205-vsx64l.xml \
	rs6000/powerpc-vsx32.xml \
	rs6000/powerpc-vsx32l.xml \
	rs6000/powerpc-vsx64.xml \
	rs6000/powerpc-vsx64l.xml \
	rs6000/rs6000.xml \
	s390-linux32.xml \
	s390-linux32v1.xml \
	s390-linux32v2.xml \
	s390-linux64.xml \
	s390-linux64v1.xml \
	s390-linux64v2.xml \
	s390-te-linux64.xml \
	s390x-linux64.xml \
	s390x-linux64v1.xml \
	s390x-linux64v2.xml \
	s390x-te-linux64.xml \
	s390-tevx-linux64.xml \
	s390-vx-linux64.xml \
	s390x-tevx-linux64.xml \
	s390x-vx-linux64.xml \
	tic6x-c62x-linux.xml \
	tic6x-c62x.xml \
	tic6x-c64x-linux.xml \
	tic6x-c64x.xml \
	tic6x-c64xp-linux.xml \
	tic6x-c64xp.xml

CFILES = $(patsubst %.xml,%.c,$(XMLTOC))
GDB = false

all: $(OUTPUTS)

$(outdir)/%.dat: %.xml number-regs.xsl sort-regs.xsl gdbserver-regs.xsl
	echo "# THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi :set ro:" > $(outdir)/$*.tmp
	echo "# Generated from: $<" >> $(outdir)/$*.tmp
	echo "name:`echo $(notdir $*) | sed 's/-/_/g'`" >> $(outdir)/$*.tmp
	echo "xmltarget:$(<F)" >> $(outdir)/$*.tmp
	echo "expedite:$(if $($*-expedite),$($*-expedite),$($(firstword $(subst -, ,$(notdir $*)))-expedite))" \
	  >> $(outdir)/$*.tmp
	$(XSLTPROC) --path "$(PWD)" --xinclude number-regs.xsl $< | \
	  $(XSLTPROC) sort-regs.xsl - | \
	  $(XSLTPROC) gdbserver-regs.xsl - >> $(outdir)/$*.tmp
	sh ../../move-if-change $(outdir)/$*.tmp $(outdir)/$*.dat

cfiles: $(CFILES)
%.c: %.xml
	$(GDB) -nx -q -batch \
	  -ex "set tdesc filename $<" -ex 'maint print c-tdesc' > $@.tmp
	sh ../../move-if-change $@.tmp $@

# Other dependencies.
$(outdir)/arm/arm-with-iwmmxt.dat: arm/arm-core.xml arm/xscale-iwmmxt.xml
$(outdir)/i386/i386.dat: i386/32bit-core.xml i386/32bit-sse.xml
$(outdir)/i386/i386-linux.dat: i386/32bit-core.xml i386/32bit-sse.xml \
			       i386/32bit-linux.xml
$(outdir)/i386/amd64.dat: i386/64bit-core.xml i386/64bit-sse.xml
$(outdir)/i386/amd64-linux.dat: i386/64bit-core.xml i386/64bit-sse.xml \
			       i386/64bit-linux.xml i386/64bit-segments.xml
$(outdir)/i386/i386-avx.dat: i386/32bit-core.xml i386/32bit-avx.xml
$(outdir)/i386/i386-avx-linux.dat: i386/32bit-core.xml i386/32bit-avx.xml \
			       i386/32bit-linux.xml
$(outdir)/i386/i386-mpx.dat: i386/32bit-core.xml i386/32bit-avx.xml \
			       i386/32bit-mpx.xml
$(outdir)/i386/i386-mpx-linux.dat: i386/32bit-core.xml 	i386/32bit-avx.xml \
			       i386/32bit-linux.xml i386/32bit-mpx.xml
$(outdir)/i386/i386-mpx-linux.dat: i386/32bit-core.xml \
			       i386/32bit-linux.xml i386/32bit-mpx.xml
$(outdir)/i386/i386-avx-mpx-linux.dat: i386/32bit-core.xml \
			       i386/32bit-linux.xml i386/32bit-mpx.xml
$(outdir)/i386/i386-avx-avx512.dat: i386/32bit-core.xml i386/32bit-avx.xml \
			       i386/32bit-avx512.xml
$(outdir)/i386/i386-avx-avx512-linux.dat: i386/32bit-core.xml i386/32bit-avx.xml \
			       i386/32bit-linux.xml i386/32bit-avx512.xml
$(outdir)/i386/i386-avx-mpx-avx512-pku.dat: i386/32bit-core.xml
			       i386/32bit-avx.xml i386/32bit-mpx.xml i386/32bit-avx512.xml \
			       i386/32bit-pkeys.xml
$(outdir)/i386/i386-avx-mpx-avx512-pku-linux.dat: i386/32bit-core.xml \
			       i386/32bit-avx.xml i386/32bit-mpx.xml i386/32bit-avx512.xml \
			       i386/32bit-pkeys.xml i386/32bit-linux.xml
$(outdir)/i386/i386-mmx.dat: i386/32bit-core.xml 
$(outdir)/i386/i386-mmx-linux.dat: i386/32bit-core.xml i386/32bit-linux.xml
$(outdir)/i386/amd64-avx.dat: i386/64bit-core.xml i386/64bit-avx.xml
$(outdir)/i386/amd64-avx-linux.dat: i386/64bit-core.xml i386/64bit-avx.xml \
			       i386/64bit-linux.xml i386/64bit-segments.xml
$(outdir)/i386/amd64-mpx-linux.dat: i386/64bit-core.xml i386/64bit-avx.xml \
			       i386/64bit-linux.xml i386/64bit-segments.xml i386/64bit-mpx.xml
$(outdir)/i386/amd64-avx-mpx-linux.dat: i386/64bit-core.xml \
			       i386/64bit-linux.xml i386/64bit-segments.xml i386/64bit-mpx.xml
$(outdir)/i386/amd64-mpx.dat: i386/64bit-core.xml i386/64bit-avx.xml \
			       i386/64bit-mpx.xml
$(outdir)/i386/amd64-avx-mpx.dat: i386/64bit-core.xml \
			       i386/64bit-mpx.xml
$(outdir)/i386/amd64-avx-avx512.dat: i386/64bit-core.xml i386/64bit-avx.xml \
			       i386/64bit-avx512.xml
$(outdir)/i386/amd64-avx-avx512-linux.dat: i386/64bit-core.xml i386/64bit-avx.xml \
			       i386/64bit-avx512.xml i386/64bit-linux.xml
$(outdir)/i386/amd64-avx-mpx-avx512-pku.dat: i386/64bit-core.xml \
			       i386/64bit-avx.xml i386/64bit-mpx.xml i386/64bit-avx512.xml \
			       i386/64bit-pkeys.xml
$(outdir)/i386/amd64-avx-mpx-avx512-pku-linux.dat: i386/64bit-core.xml \
			       i386/64bit-avx.xml i386/64bit-mpx.xml i386/64bit-avx512.xml \
			       i386/64bit-linux.xml i386/64bit-segments.xml \
			       i386/64bit-pkeys.xml
$(outdir)/i386/x32.dat: i386/x32-core.xml i386/64bit-sse.xml
$(outdir)/i386/x32-linux.dat: i386/x32-core.xml i386/64bit-sse.xml \
			       i386/64bit-linux.xml i386/64bit-segments.xml
$(outdir)/i386/x32-avx.dat: i386/x32-core.xml i386/64bit-avx.xml
$(outdir)/i386/x32-avx-linux.dat: i386/x32-core.xml i386/64bit-avx.xml \
			       i386/64bit-linux.xml i386/64bit-segments.xml
$(outdir)/i386/x32-avx-avx512.dat: i386/x32-core.xml i386/64bit-avx.xml \
			       i386/64bit-avx512.xml
$(outdir)/i386/x32-avx-avx512-linux.dat: i386/x32-core.xml i386/64bit-avx.xml \
			       i386/64bit-avx512.xml i386/64bit-linux.xml \
			       i386/64bit-segments.xml

# 'all' doesn't build the C files, so don't delete them in 'clean'
# either.
clean-cfiles:
	rm -f $(CFILES)

clean:
	rm -f $(OUTPUTS)
