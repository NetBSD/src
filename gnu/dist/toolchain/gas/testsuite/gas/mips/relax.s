# Source file used to test relaxation.

	.text
foo:
	move    $2, $3          # just something
        .space  0x20000         # to make a 128kb loop body
        beq     $2, $3, foo
