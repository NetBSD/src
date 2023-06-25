#ifndef TARGET_D_CPU_VERSIONS
#define TARGET_D_CPU_VERSIONS hook_void_void
#endif
#ifndef TARGET_D_OS_VERSIONS
#define TARGET_D_OS_VERSIONS hook_void_void
#endif
#ifndef TARGET_D_CRITSEC_SIZE
#define TARGET_D_CRITSEC_SIZE hook_uint_void_0
#endif

#define TARGETDM_INITIALIZER \
  { \
    TARGET_D_CPU_VERSIONS, \
    TARGET_D_OS_VERSIONS, \
    TARGET_D_CRITSEC_SIZE, \
  }
