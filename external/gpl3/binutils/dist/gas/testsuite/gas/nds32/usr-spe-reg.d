#objdump: -d --prefix-addresses
#name: nds32 usr-spe-reg instructions
#as:

# Test user specail register instructions

.*:     file format .*

Disassembly of section .text:
0+0000 <[^>]*> mfusr	\$r0, \$d0.lo
0+0004 <[^>]*> mfusr	\$r0, \$d0.hi
0+0008 <[^>]*> mfusr	\$r0, \$d1.lo
0+000c <[^>]*> mfusr	\$r0, \$d1.hi
0+0010 <[^>]*> mfusr	\$r0, \$pc
0+0014 <[^>]*> mfusr	\$r0, \$DMA_CFG
0+0018 <[^>]*> mfusr	\$r0, \$DMA_GCSW
0+001c <[^>]*> mfusr	\$r0, \$DMA_CHNSEL
0+0020 <[^>]*> mfusr	\$r0, \$DMA_ACT
0+0024 <[^>]*> mfusr	\$r0, \$DMA_SETUP
0+0028 <[^>]*> mfusr	\$r0, \$DMA_ISADDR
0+002c <[^>]*> mfusr	\$r0, \$DMA_ESADDR
0+0030 <[^>]*> mfusr	\$r0, \$DMA_TCNT
0+0034 <[^>]*> mfusr	\$r0, \$DMA_STATUS
0+0038 <[^>]*> mfusr	\$r0, \$DMA_2DSET
0+003c <[^>]*> mfusr	\$r0, \$DMA_2DSCTL
0+0040 <[^>]*> mfusr	\$r0, \$PFMC0
0+0044 <[^>]*> mfusr	\$r0, \$PFMC1
0+0048 <[^>]*> mfusr	\$r0, \$PFMC2
0+004c <[^>]*> mfusr	\$r0, \$PFM_CTL
