. ${srcdir}/emulparams/armelf_nbsd.sh

# Use the ARM ABI-compliant exception-handling sections.
OTHER_READONLY_SECTIONS="
  .ARM.extab ${RELOCATING-0} : { *(.ARM.extab${RELOCATING+* .gnu.linkonce.armextab.*}) }
  ${RELOCATING+ PROVIDE_HIDDEN (__exidx_start = .); }
  .ARM.exidx ${RELOCATING-0} : { *(.ARM.exidx${RELOCATING+* .gnu.linkonce.armexidx.*}) }
  ${RELOCATING+ PROVIDE_HIDDEN (__exidx_end = .); }"

case "$target" in
  arm*-*-netbsdelf*-*eabihf*)
    ;;
  aarch64*-*-netbsd* | arm*-*-netbsdelf*)
    case "$EMULATION_NAME" in
    *armelf*eabihf)
      LIB_PATH='=/usr/lib/eabihf'
      ;;
    esac
    ;;
esac
