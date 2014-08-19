#name: Mode Change Error 1
#source: mode-change-error-1a.s
#source: mode-change-error-1b.s
#ld: -e 0x8000000
#error: .*: Unsupported jump between ISA modes; consider recompiling with interlinking enabled.
