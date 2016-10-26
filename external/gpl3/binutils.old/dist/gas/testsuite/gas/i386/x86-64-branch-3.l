.*: Assembler messages:
.*:2: Warning: indirect jmp without `\*'
.*:7: Warning: indirect call without `\*'
GAS LISTING .*


[ 	]*1[ 	]+\.text
[ 	]*2[ 	]+0000 66FF2C25 		data16 jmp foo
\*\*\*\*  Warning: indirect jmp without `\*'
[ 	]*2[ 	]+00000000 
[ 	]*3[ 	]+
[ 	]*4[ 	]+bar:
[ 	]*5[ 	]+0008 89C3     		mov %eax, %ebx
[ 	]*6[ 	]+
[ 	]*7[ 	]+000a 66FF1C25 		data16 call foo
\*\*\*\*  Warning: indirect call without `\*'
[ 	]*7[ 	]+00000000 
