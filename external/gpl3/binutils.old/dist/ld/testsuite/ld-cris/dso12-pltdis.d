#source: expdyn1.s
#source: dsov32-1.s
#source: dsov32-2.s
#as: --pic --no-underscore --march=v32 --em=criself
#ld: --shared -m crislinux -z nocombreloc
#objdump: -d 

# Check dissassembly of .plt section.

.*:     file format elf32-cris

Disassembly of section \.plt:

0+1e4 <dsofn4@plt-0x1a>:

 1e4:	84e2                	subq 4,\$sp
 1e6:	0401                	addoq 4,\$r0,\$acr
 1e8:	7e7a                	move \$mof,\[\$sp\]
 1ea:	3f7a                	move \[\$acr\],\$mof
 1ec:	04f2                	addq 4,\$acr
 1ee:	6ffa                	move\.d \[\$acr\],\$acr
 1f0:	bf09                	jump \$acr
 1f2:	b005                	nop 
	\.\.\.

0+1fe <dsofn4@plt>:
 1fe:	6f0d 0c00 0000      	addo\.d c <dsofn4@plt-0x1f2>,\$r0,\$acr
 204:	6ffa                	move\.d \[\$acr\],\$acr
 206:	bf09                	jump \$acr
 208:	b005                	nop 
 20a:	3f7e 0000 0000      	move 0 <dsofn4@plt-0x1fe>,\$mof
 210:	bf0e d4ff ffff      	ba 1e4 <dsofn4@plt-0x1a>
 216:	b005                	nop 

0+218 <dsofn@plt>:
 218:	6f0d 1000 0000      	addo\.d 10 <dsofn4@plt-0x1ee>,\$r0,\$acr
 21e:	6ffa                	move\.d \[\$acr\],\$acr
 220:	bf09                	jump \$acr
 222:	b005                	nop 
 224:	3f7e 0c00 0000      	move c <dsofn4@plt-0x1f2>,\$mof
 22a:	bf0e baff ffff      	ba 1e4 <dsofn4@plt-0x1a>
 230:	b005                	nop 

Disassembly of section \.text:
#...
0+236 <dsofn3>:
 236:	bfbe e2ff ffff      	bsr 218 <dsofn@plt>
 23c:	b005                	nop 

0+23e <dsofn4>:
 23e:	7f0d a620 0000      	lapc 22e4 <_GLOBAL_OFFSET_TABLE_>,\$r0
 244:	5f0d 1400           	addo\.w 0x14,\$r0,\$acr
 248:	bfbe b6ff ffff      	bsr 1fe <dsofn4@plt>
#pass
