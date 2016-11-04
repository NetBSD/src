#as: -mcpu=archs
#objdump: -dr --show-raw-insn

.*: +file format .*arc.*

Disassembly of section .text:

[0-9a-f]+ <text_label-0x72>:
   0:	20a8 0e40           	lp	72 <text_label>
   4:	20e8 0de0           	lp	72 <text_label>
   8:	20e8 0d60           	lp	72 <text_label>
   c:	20e8 0ce1           	lpeq	72 <text_label>
  10:	20e8 0c61           	lpeq	72 <text_label>
  14:	20e8 0be2           	lpne	72 <text_label>
  18:	20e8 0b62           	lpne	72 <text_label>
  1c:	20e8 0ae3           	lpp	72 <text_label>
  20:	20e8 0a63           	lpp	72 <text_label>
  24:	20e8 09e4           	lpn	72 <text_label>
  28:	20e8 0964           	lpn	72 <text_label>
  2c:	20e8 08e5           	lpc	72 <text_label>
  30:	20e8 0865           	lpc	72 <text_label>
  34:	20e8 07e5           	lpc	72 <text_label>
  38:	20e8 0766           	lpnc	72 <text_label>
  3c:	20e8 06e6           	lpnc	72 <text_label>
  40:	20e8 0666           	lpnc	72 <text_label>
  44:	20e8 05e7           	lpv	72 <text_label>
  48:	20e8 0567           	lpv	72 <text_label>
  4c:	20e8 04e8           	lpnv	72 <text_label>
  50:	20e8 0468           	lpnv	72 <text_label>
  54:	20e8 03e9           	lpgt	72 <text_label>
  58:	20e8 036a           	lpge	72 <text_label>
  5c:	20e8 02eb           	lplt	72 <text_label>
  60:	20e8 026c           	lple	72 <text_label>
  64:	20e8 01ed           	lphi	72 <text_label>
  68:	20e8 016e           	lpls	72 <text_label>
  6c:	20e8 00ef           	lppnz	72 <text_label>
  70:	78e0                	nop_s
