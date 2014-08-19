# name: ELF microMIPS ASE markings
# source: empty.s
# objdump: -p
# as: -32 -mmicromips

.*:.*file format.*mips.*
!private flags = .*micromips.*

