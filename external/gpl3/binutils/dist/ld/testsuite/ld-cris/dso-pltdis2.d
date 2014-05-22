#source: dsov32-1.s
#source: dsov32-2.s
#source: dsofng.s
#as: --pic --no-underscore --march=v32 --em=criself
#ld: --shared -m crislinux
#objdump: -d 

# Complement to dso-pltdis1.d; merging the other .got.plt entry.
# Depending on reloc order, one of the tests would fail.

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

000001ce <dsofn4@plt>:
 1ce:	6f0d ..00 0000      	addo\.d .*
 1d4:	6ffa                	move\.d \[\$acr\],\$acr
 1d6:	bf09                	jump \$acr
 1d8:	b005                	nop 
 1da:	3f7e .... ....      	move .*,\$mof
 1e0:	bf0e .... ....      	ba .*
 1e6:	b005                	nop 
#...
 1e8:	6f0d ..00 0000      	addo\.d .*
 1ee:	6ffa                	move\.d \[\$acr\],\$acr
 1f0:	bf09                	jump \$acr
 1f2:	b005                	nop 
 1f4:	3f7e .... ....      	move .*,\$mof
 1fa:	bf0e .... ....      	ba .*
 200:	b005                	nop 

Disassembly of section \.text:
#...
0+202 <dsofn3>:
 202:	bfbe e6ff ffff      	bsr 1e8 <(dsofn@plt|dsofn4@plt\+0x1a)>
 208:	b005                	nop 

0+20a <dsofn4>:
 20a:	7f0d ae20 0000      	lapc 22b8 <_GLOBAL_OFFSET_TABLE_>,\$r0
 210:	5f0d ..00           	addo\.w 0x..,\$r0,\$acr
 214:	bfbe baff ffff      	bsr 1ce <dsofn4@plt>
#pass
