.*: Assembler messages:
.*:3: Error: .*
.*:5: Error: .*
.*:6: Error: .*
GAS LISTING .*


[ 	]*1[ 	]+\.text
[ 	]*2[ 	]+\?\?\?\? 90       		nop
[ 	]*3[ 	]+\{disp32\}
[ 	]*4[ 	]+\?\?\?\? 90       		nop
[ 	]*5[ 	]+\{disp32\} movb \(%bp\),%al
[ 	]*6[ 	]+\{disp16\} movb \(%ebp\),%al
[ 	]*7[ 	]+\?\?\?\? 00000000 		\.p2align 4,0
#...
