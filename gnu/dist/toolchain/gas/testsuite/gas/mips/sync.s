	.text
foo:
	sync
	sync.p
	sync.l

# Force at least 8 (non-delay-slot) zero bytes, to make 'objdump' print ...
	.space  8
