#name: PLT PC-relative offset overflow check
#as: --64
#ld: -shared -melf_x86_64
#notarget: x86_64-*-linux*-gnux32
#error: .*PC-relative offset overflow in PLT entry for `bar'
