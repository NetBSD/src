. ${srcdir}/emulparams/elf32lriscv-defs.sh
ELFSIZE=64

# Look for 64 bit target libraries in /lib64, /usr/lib64 etc., first
# on Linux.
case "$target" in
  riscv64*-linux*)
    case "$EMULATION_NAME" in
      *64*)
	LIBPATH_SUFFIX=64 ;;
    esac
    ;;
esac
