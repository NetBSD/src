#source: relocs-1027-symbolic-func.s
#ld: -shared -Bsymbolic-functions
#readelf: -r --wide
#...
.* +R_AARCH64_RELATIVE +.*
