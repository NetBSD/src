#NO_APP
gcc2_compiled.:
.globl _itesw
.data
	.even
_itesw:
	.long _customc_init
	.long _customc_deinit
	.long _customc_clear
	.long _customc_putc
	.long _customc_cursor
	.long _customc_scroll
	.long _tiga_init
	.long _tiga_deinit
	.long _tiga_clear
	.long _tiga_putc
	.long _tiga_cursor
	.long _tiga_scroll
.globl _iteburst
	.even
_iteburst:
	.long 64
.globl _nite
	.even
_nite:
	.long 1
.globl _kbd_tty
	.even
_kbd_tty:
	.long 0
.globl _ite_tty
	.even
_ite_tty:
	.long _ite_cons
.globl _start_repeat_timo
	.even
_start_repeat_timo:
	.long 50
.globl _next_repeat_timo
	.even
_next_repeat_timo:
	.long 10
_last_char:
	.byte 0
_tout_pending:
	.byte 0
_mod.22:
	.byte 0
_last_dead.23:
	.byte 0
.globl _whichconsole
	.even
_whichconsole:
	.long 0
	.even
_paniced.76:
	.long 0
.comm _cpuspeed,4
.comm _zombproc,4
.comm _allproc,4
.comm _initproc,4
.comm _pageproc,4
.comm _qs,256
.comm _whichqs,4
.comm _ite_softc,344
.comm _astpending,4
.comm _want_resched,4
.comm _ite_cons,6294
.comm _console_attributes,8704
