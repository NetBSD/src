#PROG: readelf
#readelf: -Wr
#name: MIPS ELF reloc 27
#as: -32

Relocation section '\.rel\.text' at offset .* contains [34] entries:
 *Offset * Info * Type * Sym\. Value * Symbol's Name
[0-9a-f]+ * [0-9a-f]+ R_(MIPS|MIPS16)_HI16 * [0-9a-f]+ * (\.text|\.L0)
[0-9a-f]+ * [0-9a-f]+ R_(MIPS|MIPS16)_HI16 * [0-9a-f]+ * (\.text|\.L0)
[0-9a-f]+ * [0-9a-f]+ R_(MIPS|MIPS16)_LO16 * [0-9a-f]+ * (\.text|\.L0)
