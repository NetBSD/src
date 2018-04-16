. ${srcdir}/emulparams/elf32ppc.sh

case "$target" in
  powerpc64*-*-netbsd*)
    case "$EMULATION_NAME" in
    *32*)
      LIB_PATH='=/usr/lib/powerpc'
    ;;
    esac
esac
