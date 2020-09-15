#target: [check_shared_lib_support]
#ld: -shared
#readelf: -r -W
#...
.* R_AARCH64_GLOB_DAT .* var.*
