/* Definitions of target machine for gcc for Hitachi Super-H using ELF.
   Copyright (C) 1996, 1997 Free Software Foundation, Inc.
   Contributed by Ian Lance Taylor <ian@cygnus.com>.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#undef  CPP_SPEC
#define CPP_SPEC "%{posix:-D_POSIX_SOURCE} \
%{!mb:-D__LITTLE_ENDIAN__} \
%{m1:-D__sh1__} \
%{m2:-D__sh2__} \
%{m3:-D__sh3__} \
%{m3e:-D__SH3E__} \
%{m4-single-only:-D__SH4_SINGLE_ONLY__} \
%{m4-single:-D__SH4_SINGLE__} \
%{m4:-D__SH4__} \
%{!m1:%{!m2:%{!m3:%{!m3e:%{!m4:%{!m4-single:%{!m4-single-only:-D__sh1__}}}}}}} \
%{mhitachi:-D__HITACHI__}"

/* Pass -ml and -mrelax to the assembler and linker.  */
#undef ASM_SPEC
#define ASM_SPEC  "%{!mb:-little} %{mrelax:-relax}"

#undef LINK_SPEC
#define LINK_SPEC \
"%{mb:-m elf32shnbsd} %{mrelax:-relax} \
 %{assert*} \
 %{shared:-shared} \
 %{!shared: \
   -dc -dp \
   %{!nostdlib:%{!r*:%{!e*:-e __start}}} \
   %{!static: \
     %{rdynamic:-export-dynamic} \
     %{!dynamic-linker:-dynamic-linker /usr/libexec/ld.elf_so}} \
   %{static:-static}}"

#undef TARGET_DEFAULT
#define TARGET_DEFAULT  LITTLE_ENDIAN_BIT
