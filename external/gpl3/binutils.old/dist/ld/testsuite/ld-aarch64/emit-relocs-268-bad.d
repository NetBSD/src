#source: emit-relocs-268.s
#ld: -T relocs.ld --defsym tempy=0x11000 --defsym tempy2=0x45000 --defsym tempy3=0x1234  -e0 -shared
#error: .*relocation R_AARCH64_MOVW_UABS_G2_NC.*can not.*shared object.*fPIC
