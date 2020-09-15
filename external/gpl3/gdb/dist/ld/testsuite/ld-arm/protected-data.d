#target: [check_shared_lib_support]
#ld: -shared
#readelf: -r -W
#...
.* R_ARM_GLOB_DAT .* var.*
