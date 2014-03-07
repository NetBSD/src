. ${srcdir}/emulparams/elf32ppc.sh

case "$target" in
  powerpc64*-*-netbsd*)
    LIB_PATH='=/usr/lib/powerpc'
    ;;
esac
