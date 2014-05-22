# name: ELF microMIPS ASE markings 2
# source: nop.s
# objdump: -p
# as: -32 -mmicromips

.*:.*file format.*mips.*
private flags = [0-9a-f]*[2367abef]......: .*[[,]micromips[],].*

