#as: -J
#objdump: -dw
#name: i386 opcodes

.*: +file format .*

Disassembly of section .text:

0+000 <foo>:
   0:	00 90 90 90 90 90 [ 	]*add    %dl,-0x6f6f6f70\(%eax\)
   6:	01 90 90 90 90 90 [ 	]*add    %edx,-0x6f6f6f70\(%eax\)
   c:	02 90 90 90 90 90 [ 	]*add    -0x6f6f6f70\(%eax\),%dl
  12:	03 90 90 90 90 90 [ 	]*add    -0x6f6f6f70\(%eax\),%edx
  18:	04 90 [ 	]*add    \$0x90,%al
  1a:	05 90 90 90 90 [ 	]*add    \$0x90909090,%eax
  1f:	06 [ 	]*push   %es
  20:	07 [ 	]*pop    %es
  21:	08 90 90 90 90 90 [ 	]*or     %dl,-0x6f6f6f70\(%eax\)
  27:	09 90 90 90 90 90 [ 	]*or     %edx,-0x6f6f6f70\(%eax\)
  2d:	0a 90 90 90 90 90 [ 	]*or     -0x6f6f6f70\(%eax\),%dl
  33:	0b 90 90 90 90 90 [ 	]*or     -0x6f6f6f70\(%eax\),%edx
  39:	0c 90 [ 	]*or     \$0x90,%al
  3b:	0d 90 90 90 90 [ 	]*or     \$0x90909090,%eax
  40:	0e [ 	]*push   %cs
  41:	10 90 90 90 90 90 [ 	]*adc    %dl,-0x6f6f6f70\(%eax\)
  47:	11 90 90 90 90 90 [ 	]*adc    %edx,-0x6f6f6f70\(%eax\)
  4d:	12 90 90 90 90 90 [ 	]*adc    -0x6f6f6f70\(%eax\),%dl
  53:	13 90 90 90 90 90 [ 	]*adc    -0x6f6f6f70\(%eax\),%edx
  59:	14 90 [ 	]*adc    \$0x90,%al
  5b:	15 90 90 90 90 [ 	]*adc    \$0x90909090,%eax
  60:	16 [ 	]*push   %ss
  61:	17 [ 	]*pop    %ss
  62:	18 90 90 90 90 90 [ 	]*sbb    %dl,-0x6f6f6f70\(%eax\)
  68:	19 90 90 90 90 90 [ 	]*sbb    %edx,-0x6f6f6f70\(%eax\)
  6e:	1a 90 90 90 90 90 [ 	]*sbb    -0x6f6f6f70\(%eax\),%dl
  74:	1b 90 90 90 90 90 [ 	]*sbb    -0x6f6f6f70\(%eax\),%edx
  7a:	1c 90 [ 	]*sbb    \$0x90,%al
  7c:	1d 90 90 90 90 [ 	]*sbb    \$0x90909090,%eax
  81:	1e [ 	]*push   %ds
  82:	1f [ 	]*pop    %ds
  83:	20 90 90 90 90 90 [ 	]*and    %dl,-0x6f6f6f70\(%eax\)
  89:	21 90 90 90 90 90 [ 	]*and    %edx,-0x6f6f6f70\(%eax\)
  8f:	22 90 90 90 90 90 [ 	]*and    -0x6f6f6f70\(%eax\),%dl
  95:	23 90 90 90 90 90 [ 	]*and    -0x6f6f6f70\(%eax\),%edx
  9b:	24 90 [ 	]*and    \$0x90,%al
  9d:	25 90 90 90 90 [ 	]*and    \$0x90909090,%eax
  a2:	27 [ 	]*daa    
  a3:	28 90 90 90 90 90 [ 	]*sub    %dl,-0x6f6f6f70\(%eax\)
  a9:	29 90 90 90 90 90 [ 	]*sub    %edx,-0x6f6f6f70\(%eax\)
  af:	2a 90 90 90 90 90 [ 	]*sub    -0x6f6f6f70\(%eax\),%dl
  b5:	2b 90 90 90 90 90 [ 	]*sub    -0x6f6f6f70\(%eax\),%edx
  bb:	2c 90 [ 	]*sub    \$0x90,%al
  bd:	2d 90 90 90 90 [ 	]*sub    \$0x90909090,%eax
  c2:	2f [ 	]*das    
  c3:	30 90 90 90 90 90 [ 	]*xor    %dl,-0x6f6f6f70\(%eax\)
  c9:	31 90 90 90 90 90 [ 	]*xor    %edx,-0x6f6f6f70\(%eax\)
  cf:	32 90 90 90 90 90 [ 	]*xor    -0x6f6f6f70\(%eax\),%dl
  d5:	33 90 90 90 90 90 [ 	]*xor    -0x6f6f6f70\(%eax\),%edx
  db:	34 90 [ 	]*xor    \$0x90,%al
  dd:	35 90 90 90 90 [ 	]*xor    \$0x90909090,%eax
  e2:	37 [ 	]*aaa    
  e3:	38 90 90 90 90 90 [ 	]*cmp    %dl,-0x6f6f6f70\(%eax\)
  e9:	39 90 90 90 90 90 [ 	]*cmp    %edx,-0x6f6f6f70\(%eax\)
  ef:	3a 90 90 90 90 90 [ 	]*cmp    -0x6f6f6f70\(%eax\),%dl
  f5:	3b 90 90 90 90 90 [ 	]*cmp    -0x6f6f6f70\(%eax\),%edx
  fb:	3c 90 [ 	]*cmp    \$0x90,%al
  fd:	3d 90 90 90 90 [ 	]*cmp    \$0x90909090,%eax
 102:	3f [ 	]*aas    
 103:	40 [ 	]*inc    %eax
 104:	41 [ 	]*inc    %ecx
 105:	42 [ 	]*inc    %edx
 106:	43 [ 	]*inc    %ebx
 107:	44 [ 	]*inc    %esp
 108:	45 [ 	]*inc    %ebp
 109:	46 [ 	]*inc    %esi
 10a:	47 [ 	]*inc    %edi
 10b:	48 [ 	]*dec    %eax
 10c:	49 [ 	]*dec    %ecx
 10d:	4a [ 	]*dec    %edx
 10e:	4b [ 	]*dec    %ebx
 10f:	4c [ 	]*dec    %esp
 110:	4d [ 	]*dec    %ebp
 111:	4e [ 	]*dec    %esi
 112:	4f [ 	]*dec    %edi
 113:	50 [ 	]*push   %eax
 114:	51 [ 	]*push   %ecx
 115:	52 [ 	]*push   %edx
 116:	53 [ 	]*push   %ebx
 117:	54 [ 	]*push   %esp
 118:	55 [ 	]*push   %ebp
 119:	56 [ 	]*push   %esi
 11a:	57 [ 	]*push   %edi
 11b:	58 [ 	]*pop    %eax
 11c:	59 [ 	]*pop    %ecx
 11d:	5a [ 	]*pop    %edx
 11e:	5b [ 	]*pop    %ebx
 11f:	5c [ 	]*pop    %esp
 120:	5d [ 	]*pop    %ebp
 121:	5e [ 	]*pop    %esi
 122:	5f [ 	]*pop    %edi
 123:	60 [ 	]*pusha  
 124:	61 [ 	]*popa   
 125:	62 90 90 90 90 90 [ 	]*bound  %edx,-0x6f6f6f70\(%eax\)
 12b:	63 90 90 90 90 90 [ 	]*arpl   %dx,-0x6f6f6f70\(%eax\)
 131:	68 90 90 90 90 [ 	]*push   \$0x90909090
 136:	69 90 90 90 90 90 90 90 90 90 [ 	]*imul   \$0x90909090,-0x6f6f6f70\(%eax\),%edx
 140:	6a 90 [ 	]*push   \$0xffffff90
 142:	6b 90 90 90 90 90 90 [ 	]*imul   \$0xffffff90,-0x6f6f6f70\(%eax\),%edx
 149:	6c [ 	]*insb   \(%dx\),%es:\(%edi\)
 14a:	6d [ 	]*insl   \(%dx\),%es:\(%edi\)
 14b:	6e [ 	]*outsb  %ds:\(%esi\),\(%dx\)
 14c:	6f [ 	]*outsl  %ds:\(%esi\),\(%dx\)
 14d:	70 90 [ 	]*jo     (0x)?df.*
 14f:	71 90 [ 	]*jno    (0x)?e1.*
 151:	72 90 [ 	]*jb     (0x)?e3.*
 153:	73 90 [ 	]*jae    (0x)?e5.*
 155:	74 90 [ 	]*je     (0x)?e7.*
 157:	75 90 [ 	]*jne    (0x)?e9.*
 159:	76 90 [ 	]*jbe    (0x)?eb.*
 15b:	77 90 [ 	]*ja     (0x)?ed.*
 15d:	78 90 [ 	]*js     (0x)?ef.*
 15f:	79 90 [ 	]*jns    (0x)?f1.*
 161:	7a 90 [ 	]*jp     (0x)?f3.*
 163:	7b 90 [ 	]*jnp    (0x)?f5.*
 165:	7c 90 [ 	]*jl     (0x)?f7.*
 167:	7d 90 [ 	]*jge    (0x)?f9.*
 169:	7e 90 [ 	]*jle    (0x)?fb.*
 16b:	7f 90 [ 	]*jg     (0x)?fd.*
 16d:	80 90 90 90 90 90 90 [ 	]*adcb   \$0x90,-0x6f6f6f70\(%eax\)
 174:	81 90 90 90 90 90 90 90 90 90 [ 	]*adcl   \$0x90909090,-0x6f6f6f70\(%eax\)
 17e:	83 90 90 90 90 90 90 [ 	]*adcl   \$0xffffff90,-0x6f6f6f70\(%eax\)
 185:	84 90 90 90 90 90 [ 	]*test   %dl,-0x6f6f6f70\(%eax\)
 18b:	85 90 90 90 90 90 [ 	]*test   %edx,-0x6f6f6f70\(%eax\)
 191:	86 90 90 90 90 90 [ 	]*xchg   %dl,-0x6f6f6f70\(%eax\)
 197:	87 90 90 90 90 90 [ 	]*xchg   %edx,-0x6f6f6f70\(%eax\)
 19d:	88 90 90 90 90 90 [ 	]*mov    %dl,-0x6f6f6f70\(%eax\)
 1a3:	89 90 90 90 90 90 [ 	]*mov    %edx,-0x6f6f6f70\(%eax\)
 1a9:	8a 90 90 90 90 90 [ 	]*mov    -0x6f6f6f70\(%eax\),%dl
 1af:	8b 90 90 90 90 90 [ 	]*mov    -0x6f6f6f70\(%eax\),%edx
 1b5:	8c 90 90 90 90 90 [ 	]*mov    %ss,-0x6f6f6f70\(%eax\)
 1bb:	8d 90 90 90 90 90 [ 	]*lea    -0x6f6f6f70\(%eax\),%edx
 1c1:	8e 90 90 90 90 90 [ 	]*mov    -0x6f6f6f70\(%eax\),%ss
 1c7:	8f 80 90 90 90 90 [ 	]*popl   -0x6f6f6f70\(%eax\)
 1cd:	90 [ 	]*nop
 1ce:	91 [ 	]*xchg   %eax,%ecx
 1cf:	92 [ 	]*xchg   %eax,%edx
 1d0:	93 [ 	]*xchg   %eax,%ebx
 1d1:	94 [ 	]*xchg   %eax,%esp
 1d2:	95 [ 	]*xchg   %eax,%ebp
 1d3:	96 [ 	]*xchg   %eax,%esi
 1d4:	97 [ 	]*xchg   %eax,%edi
 1d5:	98 [ 	]*cwtl   
 1d6:	99 [ 	]*cltd   
 1d7:	9a 90 90 90 90 90 90 [ 	]*lcall  \$0x9090,\$0x90909090
 1de:	9b [ 	]*fwait
 1df:	9c [ 	]*pushf  
 1e0:	9d [ 	]*popf   
 1e1:	9e [ 	]*sahf   
 1e2:	9f [ 	]*lahf   
 1e3:	a0 90 90 90 90 [ 	]*mov    0x90909090,%al
 1e8:	a1 90 90 90 90 [ 	]*mov    0x90909090,%eax
 1ed:	a2 90 90 90 90 [ 	]*mov    %al,0x90909090
 1f2:	a3 90 90 90 90 [ 	]*mov    %eax,0x90909090
 1f7:	a4 [ 	]*movsb  %ds:\(%esi\),%es:\(%edi\)
 1f8:	a5 [ 	]*movsl  %ds:\(%esi\),%es:\(%edi\)
 1f9:	a6 [ 	]*cmpsb  %es:\(%edi\),%ds:\(%esi\)
 1fa:	a7 [ 	]*cmpsl  %es:\(%edi\),%ds:\(%esi\)
 1fb:	a8 90 [ 	]*test   \$0x90,%al
 1fd:	a9 90 90 90 90 [ 	]*test   \$0x90909090,%eax
 202:	aa [ 	]*stos   %al,%es:\(%edi\)
 203:	ab [ 	]*stos   %eax,%es:\(%edi\)
 204:	ac [ 	]*lods   %ds:\(%esi\),%al
 205:	ad [ 	]*lods   %ds:\(%esi\),%eax
 206:	ae [ 	]*scas   %es:\(%edi\),%al
 207:	af [ 	]*scas   %es:\(%edi\),%eax
 208:	b0 90 [ 	]*mov    \$0x90,%al
 20a:	b1 90 [ 	]*mov    \$0x90,%cl
 20c:	b2 90 [ 	]*mov    \$0x90,%dl
 20e:	b3 90 [ 	]*mov    \$0x90,%bl
 210:	b4 90 [ 	]*mov    \$0x90,%ah
 212:	b5 90 [ 	]*mov    \$0x90,%ch
 214:	b6 90 [ 	]*mov    \$0x90,%dh
 216:	b7 90 [ 	]*mov    \$0x90,%bh
 218:	b8 90 90 90 90 [ 	]*mov    \$0x90909090,%eax
 21d:	b9 90 90 90 90 [ 	]*mov    \$0x90909090,%ecx
 222:	ba 90 90 90 90 [ 	]*mov    \$0x90909090,%edx
 227:	bb 90 90 90 90 [ 	]*mov    \$0x90909090,%ebx
 22c:	bc 90 90 90 90 [ 	]*mov    \$0x90909090,%esp
 231:	bd 90 90 90 90 [ 	]*mov    \$0x90909090,%ebp
 236:	be 90 90 90 90 [ 	]*mov    \$0x90909090,%esi
 23b:	bf 90 90 90 90 [ 	]*mov    \$0x90909090,%edi
 240:	c0 90 90 90 90 90 90 [ 	]*rclb   \$0x90,-0x6f6f6f70\(%eax\)
 247:	c1 90 90 90 90 90 90 [ 	]*rcll   \$0x90,-0x6f6f6f70\(%eax\)
 24e:	c2 90 90 [ 	]*ret    \$0x9090
 251:	c3 [ 	]*ret    
 252:	c4 90 90 90 90 90 [ 	]*les    -0x6f6f6f70\(%eax\),%edx
 258:	c5 90 90 90 90 90 [ 	]*lds    -0x6f6f6f70\(%eax\),%edx
 25e:	c6 80 90 90 90 90 90 [ 	]*movb   \$0x90,-0x6f6f6f70\(%eax\)
 265:	c7 80 90 90 90 90 90 90 90 90 [ 	]*movl   \$0x90909090,-0x6f6f6f70\(%eax\)
 26f:	c8 90 90 90 [ 	]*enter  \$0x9090,\$0x90
 273:	c9 [ 	]*leave  
 274:	ca 90 90 [ 	]*lret   \$0x9090
 277:	cb [ 	]*lret   
 278:	cc [ 	]*int3   
 279:	cd 90 [ 	]*int    \$0x90
 27b:	ce [ 	]*into   
 27c:	cf [ 	]*iret   
 27d:	d0 90 90 90 90 90 [ 	]*rclb   -0x6f6f6f70\(%eax\)
 283:	d1 90 90 90 90 90 [ 	]*rcll   -0x6f6f6f70\(%eax\)
 289:	d2 90 90 90 90 90 [ 	]*rclb   %cl,-0x6f6f6f70\(%eax\)
 28f:	d3 90 90 90 90 90 [ 	]*rcll   %cl,-0x6f6f6f70\(%eax\)
 295:	d4 90 [ 	]*aam    \$0x90
 297:	d5 90 [ 	]*aad    \$0x90
 299:	d7 [ 	]*xlat   %ds:\(%ebx\)
 29a:	d8 90 90 90 90 90 [ 	]*fcoms  -0x6f6f6f70\(%eax\)
 2a0:	d9 90 90 90 90 90 [ 	]*fsts   -0x6f6f6f70\(%eax\)
 2a6:	da 90 90 90 90 90 [ 	]*ficoml -0x6f6f6f70\(%eax\)
 2ac:	db 90 90 90 90 90 [ 	]*fistl  -0x6f6f6f70\(%eax\)
 2b2:	dc 90 90 90 90 90 [ 	]*fcoml  -0x6f6f6f70\(%eax\)
 2b8:	dd 90 90 90 90 90 [ 	]*fstl   -0x6f6f6f70\(%eax\)
 2be:	de 90 90 90 90 90 [ 	]*ficom  -0x6f6f6f70\(%eax\)
 2c4:	df 90 90 90 90 90 [ 	]*fist   -0x6f6f6f70\(%eax\)
 2ca:	e0 90 [ 	]*loopne (0x)?25c.*
 2cc:	e1 90 [ 	]*loope  (0x)?25e.*
 2ce:	e2 90 [ 	]*loop   (0x)?260.*
 2d0:	e3 90 [ 	]*jecxz  (0x)?262.*
 2d2:	e4 90 [ 	]*in     \$0x90,%al
 2d4:	e5 90 [ 	]*in     \$0x90,%eax
 2d6:	e6 90 [ 	]*out    %al,\$0x90
 2d8:	e7 90 [ 	]*out    %eax,\$0x90
 2da:	e8 90 90 90 90 [ 	]*call   (0x)?9090936f.*
 2df:	e9 90 90 90 90 [ 	]*jmp    (0x)?90909374.*
 2e4:	ea 90 90 90 90 90 90 [ 	]*ljmp   \$0x9090,\$0x90909090
 2eb:	eb 90 [ 	]*jmp    (0x)?27d.*
 2ed:	ec [ 	]*in     \(%dx\),%al
 2ee:	ed [ 	]*in     \(%dx\),%eax
 2ef:	ee [ 	]*out    %al,\(%dx\)
 2f0:	ef [ 	]*out    %eax,\(%dx\)
 2f1:	f4 [ 	]*hlt    
 2f2:	f5 [ 	]*cmc    
 2f3:	f6 90 90 90 90 90 [ 	]*notb   -0x6f6f6f70\(%eax\)
 2f9:	f7 90 90 90 90 90 [ 	]*notl   -0x6f6f6f70\(%eax\)
 2ff:	f8 [ 	]*clc    
 300:	f9 [ 	]*stc    
 301:	fa [ 	]*cli    
 302:	fb [ 	]*sti    
 303:	fc [ 	]*cld    
 304:	fd [ 	]*std    
 305:	ff 90 90 90 90 90 [ 	]*call   \*-0x6f6f6f70\(%eax\)
 30b:	0f 00 90 90 90 90 90 [ 	]*lldt   -0x6f6f6f70\(%eax\)
 312:	0f 01 90 90 90 90 90 [ 	]*lgdtl  -0x6f6f6f70\(%eax\)
 319:	0f 02 90 90 90 90 90 [ 	]*lar    -0x6f6f6f70\(%eax\),%edx
 320:	0f 03 90 90 90 90 90 [ 	]*lsl    -0x6f6f6f70\(%eax\),%edx
 327:	0f 06 [ 	]*clts   
 329:	0f 08 [ 	]*invd   
 32b:	0f 09 [ 	]*wbinvd 
 32d:	0f 0b [ 	]*ud2    
 32f:	0f 20 d0 [ 	]*mov    %cr2,%eax
 332:	0f 21 d0 [ 	]*mov    %db2,%eax
 335:	0f 22 d0 [ 	]*mov    %eax,%cr2
 338:	0f 23 d0 [ 	]*mov    %eax,%db2
 33b:	0f 24 d0 [ 	]*mov    %tr2,%eax
 33e:	0f 26 d0 [ 	]*mov    %eax,%tr2
 341:	0f 30 [ 	]*wrmsr  
 343:	0f 31 [ 	]*rdtsc  
 345:	0f 32 [ 	]*rdmsr  
 347:	0f 33 [ 	]*rdpmc  
 349:	0f 40 90 90 90 90 90 [ 	]*cmovo  -0x6f6f6f70\(%eax\),%edx
 350:	0f 41 90 90 90 90 90 [ 	]*cmovno -0x6f6f6f70\(%eax\),%edx
 357:	0f 42 90 90 90 90 90 [ 	]*cmovb  -0x6f6f6f70\(%eax\),%edx
 35e:	0f 43 90 90 90 90 90 [ 	]*cmovae -0x6f6f6f70\(%eax\),%edx
 365:	0f 44 90 90 90 90 90 [ 	]*cmove  -0x6f6f6f70\(%eax\),%edx
 36c:	0f 45 90 90 90 90 90 [ 	]*cmovne -0x6f6f6f70\(%eax\),%edx
 373:	0f 46 90 90 90 90 90 [ 	]*cmovbe -0x6f6f6f70\(%eax\),%edx
 37a:	0f 47 90 90 90 90 90 [ 	]*cmova  -0x6f6f6f70\(%eax\),%edx
 381:	0f 48 90 90 90 90 90 [ 	]*cmovs  -0x6f6f6f70\(%eax\),%edx
 388:	0f 49 90 90 90 90 90 [ 	]*cmovns -0x6f6f6f70\(%eax\),%edx
 38f:	0f 4a 90 90 90 90 90 [ 	]*cmovp  -0x6f6f6f70\(%eax\),%edx
 396:	0f 4b 90 90 90 90 90 [ 	]*cmovnp -0x6f6f6f70\(%eax\),%edx
 39d:	0f 4c 90 90 90 90 90 [ 	]*cmovl  -0x6f6f6f70\(%eax\),%edx
 3a4:	0f 4d 90 90 90 90 90 [ 	]*cmovge -0x6f6f6f70\(%eax\),%edx
 3ab:	0f 4e 90 90 90 90 90 [ 	]*cmovle -0x6f6f6f70\(%eax\),%edx
 3b2:	0f 4f 90 90 90 90 90 [ 	]*cmovg  -0x6f6f6f70\(%eax\),%edx
 3b9:	0f 60 90 90 90 90 90 [ 	]*punpcklbw -0x6f6f6f70\(%eax\),%mm2
 3c0:	0f 61 90 90 90 90 90 [ 	]*punpcklwd -0x6f6f6f70\(%eax\),%mm2
 3c7:	0f 62 90 90 90 90 90 [ 	]*punpckldq -0x6f6f6f70\(%eax\),%mm2
 3ce:	0f 63 90 90 90 90 90 [ 	]*packsswb -0x6f6f6f70\(%eax\),%mm2
 3d5:	0f 64 90 90 90 90 90 [ 	]*pcmpgtb -0x6f6f6f70\(%eax\),%mm2
 3dc:	0f 65 90 90 90 90 90 [ 	]*pcmpgtw -0x6f6f6f70\(%eax\),%mm2
 3e3:	0f 66 90 90 90 90 90 [ 	]*pcmpgtd -0x6f6f6f70\(%eax\),%mm2
 3ea:	0f 67 90 90 90 90 90 [ 	]*packuswb -0x6f6f6f70\(%eax\),%mm2
 3f1:	0f 68 90 90 90 90 90 [ 	]*punpckhbw -0x6f6f6f70\(%eax\),%mm2
 3f8:	0f 69 90 90 90 90 90 [ 	]*punpckhwd -0x6f6f6f70\(%eax\),%mm2
 3ff:	0f 6a 90 90 90 90 90 [ 	]*punpckhdq -0x6f6f6f70\(%eax\),%mm2
 406:	0f 6b 90 90 90 90 90 [ 	]*packssdw -0x6f6f6f70\(%eax\),%mm2
 40d:	0f 6e 90 90 90 90 90 [ 	]*movd   -0x6f6f6f70\(%eax\),%mm2
 414:	0f 6f 90 90 90 90 90 [ 	]*movq   -0x6f6f6f70\(%eax\),%mm2
 41b:	0f 71 d0 90 [ 	]*psrlw  \$0x90,%mm0
 41f:	0f 72 d0 90 [ 	]*psrld  \$0x90,%mm0
 423:	0f 73 d0 90 [ 	]*psrlq  \$0x90,%mm0
 427:	0f 74 90 90 90 90 90 [ 	]*pcmpeqb -0x6f6f6f70\(%eax\),%mm2
 42e:	0f 75 90 90 90 90 90 [ 	]*pcmpeqw -0x6f6f6f70\(%eax\),%mm2
 435:	0f 76 90 90 90 90 90 [ 	]*pcmpeqd -0x6f6f6f70\(%eax\),%mm2
 43c:	0f 77 [ 	]*emms   
 43e:	0f 7e 90 90 90 90 90 [ 	]*movd   %mm2,-0x6f6f6f70\(%eax\)
 445:	0f 7f 90 90 90 90 90 [ 	]*movq   %mm2,-0x6f6f6f70\(%eax\)
 44c:	0f 80 90 90 90 90 [ 	]*jo     (0x)?909094e2.*
 452:	0f 81 90 90 90 90 [ 	]*jno    (0x)?909094e8.*
 458:	0f 82 90 90 90 90 [ 	]*jb     (0x)?909094ee.*
 45e:	0f 83 90 90 90 90 [ 	]*jae    (0x)?909094f4.*
 464:	0f 84 90 90 90 90 [ 	]*je     (0x)?909094fa.*
 46a:	0f 85 90 90 90 90 [ 	]*jne    (0x)?90909500.*
 470:	0f 86 90 90 90 90 [ 	]*jbe    (0x)?90909506.*
 476:	0f 87 90 90 90 90 [ 	]*ja     (0x)?9090950c.*
 47c:	0f 88 90 90 90 90 [ 	]*js     (0x)?90909512.*
 482:	0f 89 90 90 90 90 [ 	]*jns    (0x)?90909518.*
 488:	0f 8a 90 90 90 90 [ 	]*jp     (0x)?9090951e.*
 48e:	0f 8b 90 90 90 90 [ 	]*jnp    (0x)?90909524.*
 494:	0f 8c 90 90 90 90 [ 	]*jl     (0x)?9090952a.*
 49a:	0f 8d 90 90 90 90 [ 	]*jge    (0x)?90909530.*
 4a0:	0f 8e 90 90 90 90 [ 	]*jle    (0x)?90909536.*
 4a6:	0f 8f 90 90 90 90 [ 	]*jg     (0x)?9090953c.*
 4ac:	0f 90 80 90 90 90 90 [ 	]*seto   -0x6f6f6f70\(%eax\)
 4b3:	0f 91 80 90 90 90 90 [ 	]*setno  -0x6f6f6f70\(%eax\)
 4ba:	0f 92 80 90 90 90 90 [ 	]*setb   -0x6f6f6f70\(%eax\)
 4c1:	0f 93 80 90 90 90 90 [ 	]*setae  -0x6f6f6f70\(%eax\)
 4c8:	0f 94 80 90 90 90 90 [ 	]*sete   -0x6f6f6f70\(%eax\)
 4cf:	0f 95 80 90 90 90 90 [ 	]*setne  -0x6f6f6f70\(%eax\)
 4d6:	0f 96 80 90 90 90 90 [ 	]*setbe  -0x6f6f6f70\(%eax\)
 4dd:	0f 97 80 90 90 90 90 [ 	]*seta   -0x6f6f6f70\(%eax\)
 4e4:	0f 98 80 90 90 90 90 [ 	]*sets   -0x6f6f6f70\(%eax\)
 4eb:	0f 99 80 90 90 90 90 [ 	]*setns  -0x6f6f6f70\(%eax\)
 4f2:	0f 9a 80 90 90 90 90 [ 	]*setp   -0x6f6f6f70\(%eax\)
 4f9:	0f 9b 80 90 90 90 90 [ 	]*setnp  -0x6f6f6f70\(%eax\)
 500:	0f 9c 80 90 90 90 90 [ 	]*setl   -0x6f6f6f70\(%eax\)
 507:	0f 9d 80 90 90 90 90 [ 	]*setge  -0x6f6f6f70\(%eax\)
 50e:	0f 9e 80 90 90 90 90 [ 	]*setle  -0x6f6f6f70\(%eax\)
 515:	0f 9f 80 90 90 90 90 [ 	]*setg   -0x6f6f6f70\(%eax\)
 51c:	0f a0 [ 	]*push   %fs
 51e:	0f a1 [ 	]*pop    %fs
 520:	0f a2 [ 	]*cpuid  
 522:	0f a3 90 90 90 90 90 [ 	]*bt     %edx,-0x6f6f6f70\(%eax\)
 529:	0f a4 90 90 90 90 90 90 [ 	]*shld   \$0x90,%edx,-0x6f6f6f70\(%eax\)
 531:	0f a5 90 90 90 90 90 [ 	]*shld   %cl,%edx,-0x6f6f6f70\(%eax\)
 538:	0f a8 [ 	]*push   %gs
 53a:	0f a9 [ 	]*pop    %gs
 53c:	0f aa [ 	]*rsm    
 53e:	0f ab 90 90 90 90 90 [ 	]*bts    %edx,-0x6f6f6f70\(%eax\)
 545:	0f ac 90 90 90 90 90 90 [ 	]*shrd   \$0x90,%edx,-0x6f6f6f70\(%eax\)
 54d:	0f ad 90 90 90 90 90 [ 	]*shrd   %cl,%edx,-0x6f6f6f70\(%eax\)
 554:	0f af 90 90 90 90 90 [ 	]*imul   -0x6f6f6f70\(%eax\),%edx
 55b:	0f b0 90 90 90 90 90 [ 	]*cmpxchg %dl,-0x6f6f6f70\(%eax\)
 562:	0f b1 90 90 90 90 90 [ 	]*cmpxchg %edx,-0x6f6f6f70\(%eax\)
 569:	0f b2 90 90 90 90 90 [ 	]*lss    -0x6f6f6f70\(%eax\),%edx
 570:	0f b3 90 90 90 90 90 [ 	]*btr    %edx,-0x6f6f6f70\(%eax\)
 577:	0f b4 90 90 90 90 90 [ 	]*lfs    -0x6f6f6f70\(%eax\),%edx
 57e:	0f b5 90 90 90 90 90 [ 	]*lgs    -0x6f6f6f70\(%eax\),%edx
 585:	0f b6 90 90 90 90 90 [ 	]*movzbl -0x6f6f6f70\(%eax\),%edx
 58c:	0f b7 90 90 90 90 90 [ 	]*movzwl -0x6f6f6f70\(%eax\),%edx
 593:	0f b9 [ 	]*ud1    
 595:	0f bb 90 90 90 90 90 [ 	]*btc    %edx,-0x6f6f6f70\(%eax\)
 59c:	0f bc 90 90 90 90 90 [ 	]*bsf    -0x6f6f6f70\(%eax\),%edx
 5a3:	0f bd 90 90 90 90 90 [ 	]*bsr    -0x6f6f6f70\(%eax\),%edx
 5aa:	0f be 90 90 90 90 90 [ 	]*movsbl -0x6f6f6f70\(%eax\),%edx
 5b1:	0f bf 90 90 90 90 90 [ 	]*movswl -0x6f6f6f70\(%eax\),%edx
 5b8:	0f c0 90 90 90 90 90 [ 	]*xadd   %dl,-0x6f6f6f70\(%eax\)
 5bf:	0f c1 90 90 90 90 90 [ 	]*xadd   %edx,-0x6f6f6f70\(%eax\)
 5c6:	0f c8 [ 	]*bswap  %eax
 5c8:	0f c9 [ 	]*bswap  %ecx
 5ca:	0f ca [ 	]*bswap  %edx
 5cc:	0f cb [ 	]*bswap  %ebx
 5ce:	0f cc [ 	]*bswap  %esp
 5d0:	0f cd [ 	]*bswap  %ebp
 5d2:	0f ce [ 	]*bswap  %esi
 5d4:	0f cf [ 	]*bswap  %edi
 5d6:	0f d1 90 90 90 90 90 [ 	]*psrlw  -0x6f6f6f70\(%eax\),%mm2
 5dd:	0f d2 90 90 90 90 90 [ 	]*psrld  -0x6f6f6f70\(%eax\),%mm2
 5e4:	0f d3 90 90 90 90 90 [ 	]*psrlq  -0x6f6f6f70\(%eax\),%mm2
 5eb:	0f d5 90 90 90 90 90 [ 	]*pmullw -0x6f6f6f70\(%eax\),%mm2
 5f2:	0f d8 90 90 90 90 90 [ 	]*psubusb -0x6f6f6f70\(%eax\),%mm2
 5f9:	0f d9 90 90 90 90 90 [ 	]*psubusw -0x6f6f6f70\(%eax\),%mm2
 600:	0f db 90 90 90 90 90 [ 	]*pand   -0x6f6f6f70\(%eax\),%mm2
 607:	0f dc 90 90 90 90 90 [ 	]*paddusb -0x6f6f6f70\(%eax\),%mm2
 60e:	0f dd 90 90 90 90 90 [ 	]*paddusw -0x6f6f6f70\(%eax\),%mm2
 615:	0f df 90 90 90 90 90 [ 	]*pandn  -0x6f6f6f70\(%eax\),%mm2
 61c:	0f e1 90 90 90 90 90 [ 	]*psraw  -0x6f6f6f70\(%eax\),%mm2
 623:	0f e2 90 90 90 90 90 [ 	]*psrad  -0x6f6f6f70\(%eax\),%mm2
 62a:	0f e5 90 90 90 90 90 [ 	]*pmulhw -0x6f6f6f70\(%eax\),%mm2
 631:	0f e8 90 90 90 90 90 [ 	]*psubsb -0x6f6f6f70\(%eax\),%mm2
 638:	0f e9 90 90 90 90 90 [ 	]*psubsw -0x6f6f6f70\(%eax\),%mm2
 63f:	0f eb 90 90 90 90 90 [ 	]*por    -0x6f6f6f70\(%eax\),%mm2
 646:	0f ec 90 90 90 90 90 [ 	]*paddsb -0x6f6f6f70\(%eax\),%mm2
 64d:	0f ed 90 90 90 90 90 [ 	]*paddsw -0x6f6f6f70\(%eax\),%mm2
 654:	0f ef 90 90 90 90 90 [ 	]*pxor   -0x6f6f6f70\(%eax\),%mm2
 65b:	0f f1 90 90 90 90 90 [ 	]*psllw  -0x6f6f6f70\(%eax\),%mm2
 662:	0f f2 90 90 90 90 90 [ 	]*pslld  -0x6f6f6f70\(%eax\),%mm2
 669:	0f f3 90 90 90 90 90 [ 	]*psllq  -0x6f6f6f70\(%eax\),%mm2
 670:	0f f5 90 90 90 90 90 [ 	]*pmaddwd -0x6f6f6f70\(%eax\),%mm2
 677:	0f f8 90 90 90 90 90 [ 	]*psubb  -0x6f6f6f70\(%eax\),%mm2
 67e:	0f f9 90 90 90 90 90 [ 	]*psubw  -0x6f6f6f70\(%eax\),%mm2
 685:	0f fa 90 90 90 90 90 [ 	]*psubd  -0x6f6f6f70\(%eax\),%mm2
 68c:	0f fc 90 90 90 90 90 [ 	]*paddb  -0x6f6f6f70\(%eax\),%mm2
 693:	0f fd 90 90 90 90 90 [ 	]*paddw  -0x6f6f6f70\(%eax\),%mm2
 69a:	0f fe 90 90 90 90 90 [ 	]*paddd  -0x6f6f6f70\(%eax\),%mm2
 6a1:	66 01 90 90 90 90 90 [ 	]*add    %dx,-0x6f6f6f70\(%eax\)
 6a8:	66 03 90 90 90 90 90 [ 	]*add    -0x6f6f6f70\(%eax\),%dx
 6af:	66 05 90 90 [ 	]*add    \$0x9090,%ax
 6b3:	66 06 [ 	]*pushw  %es
 6b5:	66 07 [ 	]*popw   %es
 6b7:	66 09 90 90 90 90 90 [ 	]*or     %dx,-0x6f6f6f70\(%eax\)
 6be:	66 0b 90 90 90 90 90 [ 	]*or     -0x6f6f6f70\(%eax\),%dx
 6c5:	66 0d 90 90 [ 	]*or     \$0x9090,%ax
 6c9:	66 0e [ 	]*pushw  %cs
 6cb:	66 11 90 90 90 90 90 [ 	]*adc    %dx,-0x6f6f6f70\(%eax\)
 6d2:	66 13 90 90 90 90 90 [ 	]*adc    -0x6f6f6f70\(%eax\),%dx
 6d9:	66 15 90 90 [ 	]*adc    \$0x9090,%ax
 6dd:	66 16 [ 	]*pushw  %ss
 6df:	66 17 [ 	]*popw   %ss
 6e1:	66 19 90 90 90 90 90 [ 	]*sbb    %dx,-0x6f6f6f70\(%eax\)
 6e8:	66 1b 90 90 90 90 90 [ 	]*sbb    -0x6f6f6f70\(%eax\),%dx
 6ef:	66 1d 90 90 [ 	]*sbb    \$0x9090,%ax
 6f3:	66 1e [ 	]*pushw  %ds
 6f5:	66 1f [ 	]*popw   %ds
 6f7:	66 21 90 90 90 90 90 [ 	]*and    %dx,-0x6f6f6f70\(%eax\)
 6fe:	66 23 90 90 90 90 90 [ 	]*and    -0x6f6f6f70\(%eax\),%dx
 705:	66 25 90 90 [ 	]*and    \$0x9090,%ax
 709:	66 29 90 90 90 90 90 [ 	]*sub    %dx,-0x6f6f6f70\(%eax\)
 710:	66 2b 90 90 90 90 90 [ 	]*sub    -0x6f6f6f70\(%eax\),%dx
 717:	66 2d 90 90 [ 	]*sub    \$0x9090,%ax
 71b:	66 31 90 90 90 90 90 [ 	]*xor    %dx,-0x6f6f6f70\(%eax\)
 722:	66 33 90 90 90 90 90 [ 	]*xor    -0x6f6f6f70\(%eax\),%dx
 729:	66 35 90 90 [ 	]*xor    \$0x9090,%ax
 72d:	66 39 90 90 90 90 90 [ 	]*cmp    %dx,-0x6f6f6f70\(%eax\)
 734:	66 3b 90 90 90 90 90 [ 	]*cmp    -0x6f6f6f70\(%eax\),%dx
 73b:	66 3d 90 90 [ 	]*cmp    \$0x9090,%ax
 73f:	66 40 [ 	]*inc    %ax
 741:	66 41 [ 	]*inc    %cx
 743:	66 42 [ 	]*inc    %dx
 745:	66 43 [ 	]*inc    %bx
 747:	66 44 [ 	]*inc    %sp
 749:	66 45 [ 	]*inc    %bp
 74b:	66 46 [ 	]*inc    %si
 74d:	66 47 [ 	]*inc    %di
 74f:	66 48 [ 	]*dec    %ax
 751:	66 49 [ 	]*dec    %cx
 753:	66 4a [ 	]*dec    %dx
 755:	66 4b [ 	]*dec    %bx
 757:	66 4c [ 	]*dec    %sp
 759:	66 4d [ 	]*dec    %bp
 75b:	66 4e [ 	]*dec    %si
 75d:	66 4f [ 	]*dec    %di
 75f:	66 50 [ 	]*push   %ax
 761:	66 51 [ 	]*push   %cx
 763:	66 52 [ 	]*push   %dx
 765:	66 53 [ 	]*push   %bx
 767:	66 54 [ 	]*push   %sp
 769:	66 55 [ 	]*push   %bp
 76b:	66 56 [ 	]*push   %si
 76d:	66 57 [ 	]*push   %di
 76f:	66 58 [ 	]*pop    %ax
 771:	66 59 [ 	]*pop    %cx
 773:	66 5a [ 	]*pop    %dx
 775:	66 5b [ 	]*pop    %bx
 777:	66 5c [ 	]*pop    %sp
 779:	66 5d [ 	]*pop    %bp
 77b:	66 5e [ 	]*pop    %si
 77d:	66 5f [ 	]*pop    %di
 77f:	66 60 [ 	]*pushaw 
 781:	66 61 [ 	]*popaw  
 783:	66 62 90 90 90 90 90 [ 	]*bound  %dx,-0x6f6f6f70\(%eax\)
 78a:	66 68 90 90 [ 	]*pushw  \$0x9090
 78e:	66 69 90 90 90 90 90 90 90 [ 	]*imul   \$0x9090,-0x6f6f6f70\(%eax\),%dx
 797:	66 6a 90 [ 	]*pushw  \$0xff90
 79a:	66 6b 90 90 90 90 90 90 [ 	]*imul   \$0xff90,-0x6f6f6f70\(%eax\),%dx
 7a2:	66 6d [ 	]*insw   \(%dx\),%es:\(%edi\)
 7a4:	66 6f [ 	]*outsw  %ds:\(%esi\),\(%dx\)
 7a6:	66 81 90 90 90 90 90 90 90 [ 	]*adcw   \$0x9090,-0x6f6f6f70\(%eax\)
 7af:	66 83 90 90 90 90 90 90 [ 	]*adcw   \$0xff90,-0x6f6f6f70\(%eax\)
 7b7:	66 85 90 90 90 90 90 [ 	]*test   %dx,-0x6f6f6f70\(%eax\)
 7be:	66 87 90 90 90 90 90 [ 	]*xchg   %dx,-0x6f6f6f70\(%eax\)
 7c5:	66 89 90 90 90 90 90 [ 	]*mov    %dx,-0x6f6f6f70\(%eax\)
 7cc:	66 8b 90 90 90 90 90 [ 	]*mov    -0x6f6f6f70\(%eax\),%dx
 7d3:	8c 90 90 90 90 90 [ 	]*mov[w ]   %ss,-0x6f6f6f70\(%eax\)
 7d9:	66 8d 90 90 90 90 90 [ 	]*lea    -0x6f6f6f70\(%eax\),%dx
 7e0:	66 8f 80 90 90 90 90 [ 	]*popw   -0x6f6f6f70\(%eax\)
 7e7:	66 91 [ 	]*xchg   %ax,%cx
 7e9:	66 92 [ 	]*xchg   %ax,%dx
 7eb:	66 93 [ 	]*xchg   %ax,%bx
 7ed:	66 94 [ 	]*xchg   %ax,%sp
 7ef:	66 95 [ 	]*xchg   %ax,%bp
 7f1:	66 96 [ 	]*xchg   %ax,%si
 7f3:	66 97 [ 	]*xchg   %ax,%di
 7f5:	66 98 [ 	]*cbtw   
 7f7:	66 99 [ 	]*cwtd   
 7f9:	66 9a 90 90 90 90 [ 	]*lcallw \$0x9090,\$0x9090
 7ff:	66 9c [ 	]*pushfw 
 801:	66 9d [ 	]*popfw  
 803:	66 a1 90 90 90 90 [ 	]*mov    0x90909090,%ax
 809:	66 a3 90 90 90 90 [ 	]*mov    %ax,0x90909090
 80f:	66 a5 [ 	]*movsw  %ds:\(%esi\),%es:\(%edi\)
 811:	66 a7 [ 	]*cmpsw  %es:\(%edi\),%ds:\(%esi\)
 813:	66 a9 90 90 [ 	]*test   \$0x9090,%ax
 817:	66 ab [ 	]*stos   %ax,%es:\(%edi\)
 819:	66 ad [ 	]*lods   %ds:\(%esi\),%ax
 81b:	66 af [ 	]*scas   %es:\(%edi\),%ax
 81d:	66 b8 90 90 [ 	]*mov    \$0x9090,%ax
 821:	66 b9 90 90 [ 	]*mov    \$0x9090,%cx
 825:	66 ba 90 90 [ 	]*mov    \$0x9090,%dx
 829:	66 bb 90 90 [ 	]*mov    \$0x9090,%bx
 82d:	66 bc 90 90 [ 	]*mov    \$0x9090,%sp
 831:	66 bd 90 90 [ 	]*mov    \$0x9090,%bp
 835:	66 be 90 90 [ 	]*mov    \$0x9090,%si
 839:	66 bf 90 90 [ 	]*mov    \$0x9090,%di
 83d:	66 c1 90 90 90 90 90 90 [ 	]*rclw   \$0x90,-0x6f6f6f70\(%eax\)
 845:	66 c2 90 90 [ 	]*retw   \$0x9090
 849:	66 c3 [ 	]*retw   
 84b:	66 c4 90 90 90 90 90 [ 	]*les    -0x6f6f6f70\(%eax\),%dx
 852:	66 c5 90 90 90 90 90 [ 	]*lds    -0x6f6f6f70\(%eax\),%dx
 859:	66 c7 80 90 90 90 90 90 90 [ 	]*movw   \$0x9090,-0x6f6f6f70\(%eax\)
 862:	66 c8 90 90 90 [ 	]*enterw \$0x9090,\$0x90
 867:	66 c9 [ 	]*leavew 
 869:	66 ca 90 90 [ 	]*lretw  \$0x9090
 86d:	66 cb [ 	]*lretw  
 86f:	66 cf [ 	]*iretw  
 871:	66 d1 90 90 90 90 90 [ 	]*rclw   -0x6f6f6f70\(%eax\)
 878:	66 d3 90 90 90 90 90 [ 	]*rclw   %cl,-0x6f6f6f70\(%eax\)
 87f:	66 e5 90 [ 	]*in     \$0x90,%ax
 882:	66 e7 90 [ 	]*out    %ax,\$0x90
 885:	66 e8 8f 90 [ 	]*callw  (0x)?9918.*
 889:	66 ea 90 90 90 90 [ 	]*ljmpw  \$0x9090,\$0x9090
 88f:	66 ed [ 	]*in     \(%dx\),%ax
 891:	66 ef [ 	]*out    %ax,\(%dx\)
 893:	66 f7 90 90 90 90 90 [ 	]*notw   -0x6f6f6f70\(%eax\)
 89a:	66 ff 90 90 90 90 90 [ 	]*callw  \*-0x6f6f6f70\(%eax\)
 8a1:	66 0f 02 90 90 90 90 90 [ 	]*lar    -0x6f6f6f70\(%eax\),%dx
 8a9:	66 0f 03 90 90 90 90 90 [ 	]*lsl    -0x6f6f6f70\(%eax\),%dx
 8b1:	66 0f 40 90 90 90 90 90 [ 	]*cmovo  -0x6f6f6f70\(%eax\),%dx
 8b9:	66 0f 41 90 90 90 90 90 [ 	]*cmovno -0x6f6f6f70\(%eax\),%dx
 8c1:	66 0f 42 90 90 90 90 90 [ 	]*cmovb  -0x6f6f6f70\(%eax\),%dx
 8c9:	66 0f 43 90 90 90 90 90 [ 	]*cmovae -0x6f6f6f70\(%eax\),%dx
 8d1:	66 0f 44 90 90 90 90 90 [ 	]*cmove  -0x6f6f6f70\(%eax\),%dx
 8d9:	66 0f 45 90 90 90 90 90 [ 	]*cmovne -0x6f6f6f70\(%eax\),%dx
 8e1:	66 0f 46 90 90 90 90 90 [ 	]*cmovbe -0x6f6f6f70\(%eax\),%dx
 8e9:	66 0f 47 90 90 90 90 90 [ 	]*cmova  -0x6f6f6f70\(%eax\),%dx
 8f1:	66 0f 48 90 90 90 90 90 [ 	]*cmovs  -0x6f6f6f70\(%eax\),%dx
 8f9:	66 0f 49 90 90 90 90 90 [ 	]*cmovns -0x6f6f6f70\(%eax\),%dx
 901:	66 0f 4a 90 90 90 90 90 [ 	]*cmovp  -0x6f6f6f70\(%eax\),%dx
 909:	66 0f 4b 90 90 90 90 90 [ 	]*cmovnp -0x6f6f6f70\(%eax\),%dx
 911:	66 0f 4c 90 90 90 90 90 [ 	]*cmovl  -0x6f6f6f70\(%eax\),%dx
 919:	66 0f 4d 90 90 90 90 90 [ 	]*cmovge -0x6f6f6f70\(%eax\),%dx
 921:	66 0f 4e 90 90 90 90 90 [ 	]*cmovle -0x6f6f6f70\(%eax\),%dx
 929:	66 0f 4f 90 90 90 90 90 [ 	]*cmovg  -0x6f6f6f70\(%eax\),%dx
 931:	66 0f a0 [ 	]*pushw  %fs
 934:	66 0f a1 [ 	]*popw   %fs
 937:	66 0f a3 90 90 90 90 90 [ 	]*bt     %dx,-0x6f6f6f70\(%eax\)
 93f:	66 0f a4 90 90 90 90 90 90 [ 	]*shld   \$0x90,%dx,-0x6f6f6f70\(%eax\)
 948:	66 0f a5 90 90 90 90 90 [ 	]*shld   %cl,%dx,-0x6f6f6f70\(%eax\)
 950:	66 0f a8 [ 	]*pushw  %gs
 953:	66 0f a9 [ 	]*popw   %gs
 956:	66 0f ab 90 90 90 90 90 [ 	]*bts    %dx,-0x6f6f6f70\(%eax\)
 95e:	66 0f ac 90 90 90 90 90 90 [ 	]*shrd   \$0x90,%dx,-0x6f6f6f70\(%eax\)
 967:	66 0f ad 90 90 90 90 90 [ 	]*shrd   %cl,%dx,-0x6f6f6f70\(%eax\)
 96f:	66 0f af 90 90 90 90 90 [ 	]*imul   -0x6f6f6f70\(%eax\),%dx
 977:	66 0f b1 90 90 90 90 90 [ 	]*cmpxchg %dx,-0x6f6f6f70\(%eax\)
 97f:	66 0f b2 90 90 90 90 90 [ 	]*lss    -0x6f6f6f70\(%eax\),%dx
 987:	66 0f b3 90 90 90 90 90 [ 	]*btr    %dx,-0x6f6f6f70\(%eax\)
 98f:	66 0f b4 90 90 90 90 90 [ 	]*lfs    -0x6f6f6f70\(%eax\),%dx
 997:	66 0f b5 90 90 90 90 90 [ 	]*lgs    -0x6f6f6f70\(%eax\),%dx
 99f:	66 0f b6 90 90 90 90 90 [ 	]*movzbw -0x6f6f6f70\(%eax\),%dx
 9a7:	66 0f bb 90 90 90 90 90 [ 	]*btc    %dx,-0x6f6f6f70\(%eax\)
 9af:	66 0f bc 90 90 90 90 90 [ 	]*bsf    -0x6f6f6f70\(%eax\),%dx
 9b7:	66 0f bd 90 90 90 90 90 [ 	]*bsr    -0x6f6f6f70\(%eax\),%dx
 9bf:	66 0f be 90 90 90 90 90 [ 	]*movsbw -0x6f6f6f70\(%eax\),%dx
 9c7:	66 0f c1 90 90 90 90 90 [ 	]*xadd   %dx,-0x6f6f6f70\(%eax\)
 9cf:	66 90 [ 	]*xchg   %ax,%ax
 9d1:	0f 00 c0 [ 	]*sldt   %eax
 9d4:	66 0f 00 c0 [ 	]*sldt   %ax
 9d8:	0f 00 00 [ 	]*sldt   \(%eax\)
 9db:	0f 01 e0 [ 	]*smsw   %eax
 9de:	66 0f 01 e0 [ 	]*smsw   %ax
 9e2:	0f 01 20 [ 	]*smsw   \(%eax\)
 9e5:	0f 00 c8 [ 	]*str    %eax
 9e8:	66 0f 00 c8 [ 	]*str    %ax
 9ec:	0f 00 08 [ 	]*str    \(%eax\)
 9ef:	0f ad d0 [ 	]*shrd   %cl,%edx,%eax
 9f2:	0f a5 d0 [ 	]*shld   %cl,%edx,%eax
 9f5:	85 c3 [ 	]*test   %eax,%ebx
 9f7:	85 d8 [ 	]*test   %ebx,%eax
 9f9:	85 18 [ 	]*test   %ebx,\(%eax\)
[ 	]*[a-f0-9]+:	0f 4a 90 90 90 90 90 	cmovp  -0x6f6f6f70\(%eax\),%edx
[ 	]*[a-f0-9]+:	0f 4b 90 90 90 90 90 	cmovnp -0x6f6f6f70\(%eax\),%edx
[ 	]*[a-f0-9]+:	66 0f 4a 90 90 90 90 90 	cmovp  -0x6f6f6f70\(%eax\),%dx
[ 	]*[a-f0-9]+:	66 0f 4b 90 90 90 90 90 	cmovnp -0x6f6f6f70\(%eax\),%dx
#pass
