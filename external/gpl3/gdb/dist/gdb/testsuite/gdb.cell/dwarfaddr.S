/* Copyright 2010-2013 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   This file is part of the gdb testsuite.

   Contributed by Ulrich Weigand <uweigand@de.ibm.com>.
   Tests for SPU addresses resulting from complex DWARF expressions.  */

	.text
main:
.Ltext_s:
.LFB1:
	stqd        $1,-240($1)
	ai          $1,$1,-240
	ai          $2,$1,32
	ai          $2,$2,127
	il          $3,127
	andc        $3,$2,$3
	il          $2,1
	lqd         $4,0($3)
	cwd         $5,0($3)
	shufb       $2,$2,$4,$5
	stqd        $2,0($3)
	lqd         $1,0($1)
	bi          $0
.LFE1:
	.global main
	.type	main,@function
	.size	main,.LFE1-.LFB1
.Ltext_e:

	.section	.debug_info,"",@progbits
.Ldebug_info_s:
	.int	.debug_info_size-4
	.short	0x2
	.int	.Ldebug_abbrev_s
	.byte	0x4
.Ldie0:
	.uleb128	0x1
	.string	"test.c"
	.int	.Ltext_s
	.int	.Ltext_e
	.byte	0x1
.Ldie1:
	.uleb128	0x2
	.string	"int"
	.byte	0x4
	.byte	0x5
.Ldie2:
	.uleb128	0x3
	.int	.Ldie4-.Ldebug_info_s
	.int	.Ldie1-.Ldebug_info_s
.Ldie3:
	.uleb128	0x4
	.byte	0
	.byte	0xf
	.uleb128	0
.Ldie4:
	.uleb128	0x5
	.string	"main"
	.int	.LFB1
	.int	.LFE1
	.byte	0x1
	.byte	0x1
	.byte	0x3
	.byte	0x1
	.byte	0x1
	.byte	0x51
.Ldie5:
	.uleb128	0x6
	.byte	0xe
	.byte	0x91
	.sleb128	0x20
	.byte	0xd
	.int	0x7f
	.byte	0x22
	.byte	0xd
	.int	0xffffff80
	.byte	0x1a
	.string	"x"
	.byte	0x1
	.byte	0
	.int	.Ldie2-.Ldebug_info_s
	.uleb128	0
	.uleb128	0
.Ldebug_info_e:
	.set	.debug_info_size,.Ldebug_info_e-.Ldebug_info_s


	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev_s:
	.uleb128	0x1
	.uleb128	0x11
	.byte	0x1
	.uleb128	0x3
	.uleb128	0x8
	.uleb128	0x11
	.uleb128	0x1
	.uleb128	0x12
	.uleb128	0x1
	.uleb128	0x13
	.uleb128	0xb
	.uleb128	0
	.uleb128	0

	.uleb128	0x2
	.uleb128	0x24
	.byte	0
	.uleb128	0x3
	.uleb128	0x8
	.uleb128	0xb
	.uleb128	0xb
	.uleb128	0x3e
	.uleb128	0xb
	.uleb128	0
	.uleb128	0

	.uleb128	0x3
	.uleb128	0x1
	.byte	0x1
	.uleb128	0x1
	.uleb128	0x13
	.uleb128	0x49
	.uleb128	0x13
	.uleb128	0
	.uleb128	0

	.uleb128	0x4
	.uleb128	0x21
	.byte	0
	.uleb128	0x22
	.uleb128	0xb
	.uleb128	0x2f
	.uleb128	0xb
	.uleb128	0
	.uleb128	0

	.uleb128	0x5
	.uleb128	0x2e
	.byte	0x1
	.uleb128	0x3
	.uleb128	0x8
	.uleb128	0x11
	.uleb128	0x1
	.uleb128	0x12
	.uleb128	0x1
	.uleb128	0x27
	.uleb128	0xc
	.uleb128	0x3a
	.uleb128	0xb
	.uleb128	0x3b
	.uleb128	0xb
	.uleb128	0x3f
	.uleb128	0xc
	.uleb128	0x40
	.uleb128	0xa
	.uleb128	0
	.uleb128	0

	.uleb128	0x6
	.uleb128	0x34
	.byte	0
	.uleb128	0x2
	.uleb128	0xa
	.uleb128	0x3
	.uleb128	0x8
	.uleb128	0x3a
	.uleb128	0xb
	.uleb128	0x3b
	.uleb128	0xb
	.uleb128	0x49
	.uleb128	0x13
	.uleb128	0
	.uleb128	0
	.uleb128	0
.Ldebug_abbrev_e:
	.set	.debug_abbrev_size,.Ldebug_abbrev_e-.Ldebug_abbrev_s

