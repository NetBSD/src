    .data
foo:
    .string "foo"
bar:
    .string "bar"
d64:
    .dword bar
d32:
    .word d64

    .text
main:
    mov %r1, bar
    jeq32 %r1, bar, baz
    ldabsdw d32
    exit

baz:
    add %r1, foo
    stw [%r2 + 8], d64
