#source: dsov32-1.s
#source: dsov32-2.s
#source: dsofn4g.s
#as: --pic --no-underscore --march=v32 --em=criself
#ld: --shared -m crislinux
#objdump: -d -R

# Check dissassembly of the .plt section, specifically the synthetic
# symbols, in a DSO in which a .got.plt entry has been merged into a
# regular .got entry.  There was a bug in which some (i.e. subsequent
# with regards to reloc order) synthetic X@plt entries were wrong if
# there were merged .got entries present; dsofn4@plt below.  The
# alternatives in the matching regexps are placeholders for a future
# improvement: synthetic symbols for .plt entries with merged .got
# entries (lost as a consequence of the relocs no longer accounted for
# in .rela.plt and the default synthetic-symbol implementation just
# iterating over .rela.plt).

.*:     file format elf32-cris

Disassembly of section \.plt:

0+1b4 <(dsofn4@plt-0x1a|dsofn@plt-0x34)>:
 1b4:	84e2                	subq 4,\$sp
 1b6:	0401                	addoq 4,\$r0,\$acr
 1b8:	7e7a                	move \$mof,\[\$sp\]
 1ba:	3f7a                	move \[\$acr\],\$mof
 1bc:	04f2                	addq 4,\$acr
 1be:	6ffa                	move\.d \[\$acr\],\$acr
 1c0:	bf09                	jump \$acr
 1c2:	b005                	nop 
	\.\.\.
#...
 1ce:	6f0d ..00 0000      	addo\.d .*
 1d4:	6ffa                	move\.d \[\$acr\],\$acr
 1d6:	bf09                	jump \$acr
 1d8:	b005                	nop 
 1da:	3f7e .... ....      	move .*,\$mof
 1e0:	bf0e .... ....      	ba .*
 1e6:	b005                	nop 

0+1e8 <dsofn@plt>:
 1e8:	6f0d ..00 0000      	addo\.d .*
 1ee:	6ffa                	move\.d \[\$acr\],\$acr
 1f0:	bf09                	jump \$acr
 1f2:	b005                	nop 
 1f4:	3f7e .... ....      	move .*,\$mof
 1fa:	bf0e baff ffff      	ba 1b4 <(dsofn4@plt-0x1a|dsofn@plt-0x34)>
 200:	b005                	nop 

Disassembly of section \.text:
#...
0+202 <dsofn3>:
 202:	bfbe e6ff ffff      	bsr 1e8 <dsofn@plt>
 208:	b005                	nop 

0+20a <dsofn4>:
 20a:	7f0d ae20 0000      	lapc 22b8 <_GLOBAL_OFFSET_TABLE_>,\$r0
 210:	5f0d 1400           	addo\.w 0x14,\$r0,\$acr
 214:	bfbe baff ffff      	bsr 1ce <(dsofn4@plt|dsofn@plt-0x1a)>
#pass
