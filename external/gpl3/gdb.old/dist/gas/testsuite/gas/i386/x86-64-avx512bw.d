#as:
#objdump: -dw
#name: x86_64 AVX512BW insns
#source: x86-64-avx512bw.s

.*: +file format .*


Disassembly of section \.text:

0+ <_start>:
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 48 1c f5[ 	]*vpabsb %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 4f 1c f5[ 	]*vpabsb %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7d cf 1c f5[ 	]*vpabsb %zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 1c 31[ 	]*vpabsb \(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 7d 48 1c b4 f0 23 01 00 00[ 	]*vpabsb 0x123\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 1c 72 7f[ 	]*vpabsb 0x1fc0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 1c b2 00 20 00 00[ 	]*vpabsb 0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 1c 72 80[ 	]*vpabsb -0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 1c b2 c0 df ff ff[ 	]*vpabsb -0x2040\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 48 1d f5[ 	]*vpabsw %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 4f 1d f5[ 	]*vpabsw %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7d cf 1d f5[ 	]*vpabsw %zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 1d 31[ 	]*vpabsw \(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 7d 48 1d b4 f0 23 01 00 00[ 	]*vpabsw 0x123\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 1d 72 7f[ 	]*vpabsw 0x1fc0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 1d b2 00 20 00 00[ 	]*vpabsw 0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 1d 72 80[ 	]*vpabsw -0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 1d b2 c0 df ff ff[ 	]*vpabsw -0x2040\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 6b f4[ 	]*vpackssdw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 6b f4[ 	]*vpackssdw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 6b f4[ 	]*vpackssdw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 6b 31[ 	]*vpackssdw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 6b b4 f0 23 01 00 00[ 	]*vpackssdw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 50 6b 31[ 	]*vpackssdw \(%rcx\)\{1to16\},%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 6b 72 7f[ 	]*vpackssdw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 6b b2 00 20 00 00[ 	]*vpackssdw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 6b 72 80[ 	]*vpackssdw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 6b b2 c0 df ff ff[ 	]*vpackssdw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 50 6b 72 7f[ 	]*vpackssdw 0x1fc\(%rdx\)\{1to16\},%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 50 6b b2 00 02 00 00[ 	]*vpackssdw 0x200\(%rdx\)\{1to16\},%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 50 6b 72 80[ 	]*vpackssdw -0x200\(%rdx\)\{1to16\},%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 50 6b b2 fc fd ff ff[ 	]*vpackssdw -0x204\(%rdx\)\{1to16\},%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 63 f4[ 	]*vpacksswb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 63 f4[ 	]*vpacksswb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 63 f4[ 	]*vpacksswb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 63 31[ 	]*vpacksswb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 63 b4 f0 23 01 00 00[ 	]*vpacksswb 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 63 72 7f[ 	]*vpacksswb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 63 b2 00 20 00 00[ 	]*vpacksswb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 63 72 80[ 	]*vpacksswb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 63 b2 c0 df ff ff[ 	]*vpacksswb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 40 2b f4[ 	]*vpackusdw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 47 2b f4[ 	]*vpackusdw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 15 c7 2b f4[ 	]*vpackusdw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 2b 31[ 	]*vpackusdw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 15 40 2b b4 f0 23 01 00 00[ 	]*vpackusdw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 50 2b 31[ 	]*vpackusdw \(%rcx\)\{1to16\},%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 2b 72 7f[ 	]*vpackusdw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 2b b2 00 20 00 00[ 	]*vpackusdw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 2b 72 80[ 	]*vpackusdw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 2b b2 c0 df ff ff[ 	]*vpackusdw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 50 2b 72 7f[ 	]*vpackusdw 0x1fc\(%rdx\)\{1to16\},%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 50 2b b2 00 02 00 00[ 	]*vpackusdw 0x200\(%rdx\)\{1to16\},%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 50 2b 72 80[ 	]*vpackusdw -0x200\(%rdx\)\{1to16\},%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 50 2b b2 fc fd ff ff[ 	]*vpackusdw -0x204\(%rdx\)\{1to16\},%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 67 f4[ 	]*vpackuswb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 67 f4[ 	]*vpackuswb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 67 f4[ 	]*vpackuswb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 67 31[ 	]*vpackuswb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 67 b4 f0 23 01 00 00[ 	]*vpackuswb 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 67 72 7f[ 	]*vpackuswb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 67 b2 00 20 00 00[ 	]*vpackuswb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 67 72 80[ 	]*vpackuswb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 67 b2 c0 df ff ff[ 	]*vpackuswb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 fc f4[ 	]*vpaddb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 fc f4[ 	]*vpaddb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 fc f4[ 	]*vpaddb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 fc 31[ 	]*vpaddb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 fc b4 f0 23 01 00 00[ 	]*vpaddb 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 fc 72 7f[ 	]*vpaddb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 fc b2 00 20 00 00[ 	]*vpaddb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 fc 72 80[ 	]*vpaddb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 fc b2 c0 df ff ff[ 	]*vpaddb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 ec f4[ 	]*vpaddsb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 ec f4[ 	]*vpaddsb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 ec f4[ 	]*vpaddsb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ec 31[ 	]*vpaddsb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 ec b4 f0 23 01 00 00[ 	]*vpaddsb 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ec 72 7f[ 	]*vpaddsb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ec b2 00 20 00 00[ 	]*vpaddsb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ec 72 80[ 	]*vpaddsb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ec b2 c0 df ff ff[ 	]*vpaddsb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 ed f4[ 	]*vpaddsw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 ed f4[ 	]*vpaddsw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 ed f4[ 	]*vpaddsw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ed 31[ 	]*vpaddsw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 ed b4 f0 23 01 00 00[ 	]*vpaddsw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ed 72 7f[ 	]*vpaddsw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ed b2 00 20 00 00[ 	]*vpaddsw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ed 72 80[ 	]*vpaddsw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ed b2 c0 df ff ff[ 	]*vpaddsw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 dc f4[ 	]*vpaddusb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 dc f4[ 	]*vpaddusb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 dc f4[ 	]*vpaddusb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 dc 31[ 	]*vpaddusb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 dc b4 f0 23 01 00 00[ 	]*vpaddusb 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 dc 72 7f[ 	]*vpaddusb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 dc b2 00 20 00 00[ 	]*vpaddusb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 dc 72 80[ 	]*vpaddusb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 dc b2 c0 df ff ff[ 	]*vpaddusb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 dd f4[ 	]*vpaddusw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 dd f4[ 	]*vpaddusw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 dd f4[ 	]*vpaddusw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 dd 31[ 	]*vpaddusw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 dd b4 f0 23 01 00 00[ 	]*vpaddusw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 dd 72 7f[ 	]*vpaddusw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 dd b2 00 20 00 00[ 	]*vpaddusw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 dd 72 80[ 	]*vpaddusw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 dd b2 c0 df ff ff[ 	]*vpaddusw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 fd f4[ 	]*vpaddw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 fd f4[ 	]*vpaddw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 fd f4[ 	]*vpaddw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 fd 31[ 	]*vpaddw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 fd b4 f0 23 01 00 00[ 	]*vpaddw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 fd 72 7f[ 	]*vpaddw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 fd b2 00 20 00 00[ 	]*vpaddw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 fd 72 80[ 	]*vpaddw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 fd b2 c0 df ff ff[ 	]*vpaddw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 03 15 40 0f f4 ab[ 	]*vpalignr \$0xab,%zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 03 15 47 0f f4 ab[ 	]*vpalignr \$0xab,%zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 03 15 c7 0f f4 ab[ 	]*vpalignr \$0xab,%zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 03 15 40 0f f4 7b[ 	]*vpalignr \$0x7b,%zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 40 0f 31 7b[ 	]*vpalignr \$0x7b,\(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 23 15 40 0f b4 f0 23 01 00 00 7b[ 	]*vpalignr \$0x7b,0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 40 0f 72 7f 7b[ 	]*vpalignr \$0x7b,0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 40 0f b2 00 20 00 00 7b[ 	]*vpalignr \$0x7b,0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 40 0f 72 80 7b[ 	]*vpalignr \$0x7b,-0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 40 0f b2 c0 df ff ff 7b[ 	]*vpalignr \$0x7b,-0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 e0 f4[ 	]*vpavgb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 e0 f4[ 	]*vpavgb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 e0 f4[ 	]*vpavgb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e0 31[ 	]*vpavgb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 e0 b4 f0 23 01 00 00[ 	]*vpavgb 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e0 72 7f[ 	]*vpavgb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e0 b2 00 20 00 00[ 	]*vpavgb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e0 72 80[ 	]*vpavgb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e0 b2 c0 df ff ff[ 	]*vpavgb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 e3 f4[ 	]*vpavgw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 e3 f4[ 	]*vpavgw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 e3 f4[ 	]*vpavgw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e3 31[ 	]*vpavgw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 e3 b4 f0 23 01 00 00[ 	]*vpavgw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e3 72 7f[ 	]*vpavgw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e3 b2 00 20 00 00[ 	]*vpavgw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e3 72 80[ 	]*vpavgw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e3 b2 c0 df ff ff[ 	]*vpavgw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 40 66 f4[ 	]*vpblendmb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 47 66 f4[ 	]*vpblendmb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 15 c7 66 f4[ 	]*vpblendmb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 66 31[ 	]*vpblendmb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 15 40 66 b4 f0 23 01 00 00[ 	]*vpblendmb 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 66 72 7f[ 	]*vpblendmb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 66 b2 00 20 00 00[ 	]*vpblendmb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 66 72 80[ 	]*vpblendmb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 66 b2 c0 df ff ff[ 	]*vpblendmb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 48 78 f5[ 	]*vpbroadcastb %xmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 4f 78 f5[ 	]*vpbroadcastb %xmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7d cf 78 f5[ 	]*vpbroadcastb %xmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 78 31[ 	]*vpbroadcastb \(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 7d 48 78 b4 f0 23 01 00 00[ 	]*vpbroadcastb 0x123\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 78 72 7f[ 	]*vpbroadcastb 0x7f\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 78 b2 80 00 00 00[ 	]*vpbroadcastb 0x80\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 78 72 80[ 	]*vpbroadcastb -0x80\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 78 b2 7f ff ff ff[ 	]*vpbroadcastb -0x81\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 7a f0[ 	]*vpbroadcastb %eax,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 4f 7a f0[ 	]*vpbroadcastb %eax,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 62 7d cf 7a f0[ 	]*vpbroadcastb %eax,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 48 79 f5[ 	]*vpbroadcastw %xmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 4f 79 f5[ 	]*vpbroadcastw %xmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7d cf 79 f5[ 	]*vpbroadcastw %xmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 79 31[ 	]*vpbroadcastw \(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 7d 48 79 b4 f0 23 01 00 00[ 	]*vpbroadcastw 0x123\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 79 72 7f[ 	]*vpbroadcastw 0xfe\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 79 b2 00 01 00 00[ 	]*vpbroadcastw 0x100\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 79 72 80[ 	]*vpbroadcastw -0x100\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 79 b2 fe fe ff ff[ 	]*vpbroadcastw -0x102\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 7b f0[ 	]*vpbroadcastw %eax,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 4f 7b f0[ 	]*vpbroadcastw %eax,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 62 7d cf 7b f0[ 	]*vpbroadcastw %eax,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 74 ed[ 	]*vpcmpeqb %zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 47 74 ed[ 	]*vpcmpeqb %zmm29,%zmm30,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 74 29[ 	]*vpcmpeqb \(%rcx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b1 0d 40 74 ac f0 23 01 00 00[ 	]*vpcmpeqb 0x123\(%rax,%r14,8\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 74 6a 7f[ 	]*vpcmpeqb 0x1fc0\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 74 aa 00 20 00 00[ 	]*vpcmpeqb 0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 74 6a 80[ 	]*vpcmpeqb -0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 74 aa c0 df ff ff[ 	]*vpcmpeqb -0x2040\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 75 ed[ 	]*vpcmpeqw %zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 47 75 ed[ 	]*vpcmpeqw %zmm29,%zmm30,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 75 29[ 	]*vpcmpeqw \(%rcx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b1 0d 40 75 ac f0 23 01 00 00[ 	]*vpcmpeqw 0x123\(%rax,%r14,8\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 75 6a 7f[ 	]*vpcmpeqw 0x1fc0\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 75 aa 00 20 00 00[ 	]*vpcmpeqw 0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 75 6a 80[ 	]*vpcmpeqw -0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 75 aa c0 df ff ff[ 	]*vpcmpeqw -0x2040\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 64 ed[ 	]*vpcmpgtb %zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 47 64 ed[ 	]*vpcmpgtb %zmm29,%zmm30,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 64 29[ 	]*vpcmpgtb \(%rcx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b1 0d 40 64 ac f0 23 01 00 00[ 	]*vpcmpgtb 0x123\(%rax,%r14,8\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 64 6a 7f[ 	]*vpcmpgtb 0x1fc0\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 64 aa 00 20 00 00[ 	]*vpcmpgtb 0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 64 6a 80[ 	]*vpcmpgtb -0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 64 aa c0 df ff ff[ 	]*vpcmpgtb -0x2040\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 65 ed[ 	]*vpcmpgtw %zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 47 65 ed[ 	]*vpcmpgtw %zmm29,%zmm30,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 65 29[ 	]*vpcmpgtw \(%rcx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b1 0d 40 65 ac f0 23 01 00 00[ 	]*vpcmpgtw 0x123\(%rax,%r14,8\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 65 6a 7f[ 	]*vpcmpgtw 0x1fc0\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 65 aa 00 20 00 00[ 	]*vpcmpgtw 0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 65 6a 80[ 	]*vpcmpgtw -0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 65 aa c0 df ff ff[ 	]*vpcmpgtw -0x2040\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 02 95 40 66 f4[ 	]*vpblendmw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 47 66 f4[ 	]*vpblendmw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 95 c7 66 f4[ 	]*vpblendmw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 66 31[ 	]*vpblendmw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 95 40 66 b4 f0 23 01 00 00[ 	]*vpblendmw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 66 72 7f[ 	]*vpblendmw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 66 b2 00 20 00 00[ 	]*vpblendmw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 66 72 80[ 	]*vpblendmw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 66 b2 c0 df ff ff[ 	]*vpblendmw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 14 e8 ab[ 	]*vpextrb \$0xab,%xmm29,%eax
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 14 e8 7b[ 	]*vpextrb \$0x7b,%xmm29,%eax
[ 	]*[a-f0-9]+:[ 	]*62 43 7d 08 14 e8 7b[ 	]*vpextrb \$0x7b,%xmm29,%r8d
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 14 29 7b[ 	]*vpextrb \$0x7b,%xmm29,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*62 23 7d 08 14 ac f0 23 01 00 00 7b[ 	]*vpextrb \$0x7b,%xmm29,0x123\(%rax,%r14,8\)
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 14 6a 7f 7b[ 	]*vpextrb \$0x7b,%xmm29,0x7f\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 14 aa 80 00 00 00 7b[ 	]*vpextrb \$0x7b,%xmm29,0x80\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 14 6a 80 7b[ 	]*vpextrb \$0x7b,%xmm29,-0x80\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 14 aa 7f ff ff ff 7b[ 	]*vpextrb \$0x7b,%xmm29,-0x81\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 15 29 7b[ 	]*vpextrw \$0x7b,%xmm29,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*62 23 7d 08 15 ac f0 23 01 00 00 7b[ 	]*vpextrw \$0x7b,%xmm29,0x123\(%rax,%r14,8\)
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 15 6a 7f 7b[ 	]*vpextrw \$0x7b,%xmm29,0xfe\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 15 aa 00 01 00 00 7b[ 	]*vpextrw \$0x7b,%xmm29,0x100\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 15 6a 80 7b[ 	]*vpextrw \$0x7b,%xmm29,-0x100\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 15 aa fe fe ff ff 7b[ 	]*vpextrw \$0x7b,%xmm29,-0x102\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 91 7d 08 c5 c6 ab[ 	]*vpextrw \$0xab,%xmm30,%eax
[ 	]*[a-f0-9]+:[ 	]*62 91 7d 08 c5 c6 7b[ 	]*vpextrw \$0x7b,%xmm30,%eax
[ 	]*[a-f0-9]+:[ 	]*62 11 7d 08 c5 c6 7b[ 	]*vpextrw \$0x7b,%xmm30,%r8d
[ 	]*[a-f0-9]+:[ 	]*62 63 15 00 20 f0 ab[ 	]*vpinsrb \$0xab,%eax,%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 00 20 f0 7b[ 	]*vpinsrb \$0x7b,%eax,%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 00 20 f5 7b[ 	]*vpinsrb \$0x7b,%ebp,%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 43 15 00 20 f5 7b[ 	]*vpinsrb \$0x7b,%r13d,%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 00 20 31 7b[ 	]*vpinsrb \$0x7b,\(%rcx\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 23 15 00 20 b4 f0 23 01 00 00 7b[ 	]*vpinsrb \$0x7b,0x123\(%rax,%r14,8\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 00 20 72 7f 7b[ 	]*vpinsrb \$0x7b,0x7f\(%rdx\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 00 20 b2 80 00 00 00 7b[ 	]*vpinsrb \$0x7b,0x80\(%rdx\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 00 20 72 80 7b[ 	]*vpinsrb \$0x7b,-0x80\(%rdx\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 00 20 b2 7f ff ff ff 7b[ 	]*vpinsrb \$0x7b,-0x81\(%rdx\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 00 c4 f0 ab[ 	]*vpinsrw \$0xab,%eax,%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 00 c4 f0 7b[ 	]*vpinsrw \$0x7b,%eax,%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 00 c4 f5 7b[ 	]*vpinsrw \$0x7b,%ebp,%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 41 15 00 c4 f5 7b[ 	]*vpinsrw \$0x7b,%r13d,%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 00 c4 31 7b[ 	]*vpinsrw \$0x7b,\(%rcx\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 00 c4 b4 f0 23 01 00 00 7b[ 	]*vpinsrw \$0x7b,0x123\(%rax,%r14,8\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 00 c4 72 7f 7b[ 	]*vpinsrw \$0x7b,0xfe\(%rdx\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 00 c4 b2 00 01 00 00 7b[ 	]*vpinsrw \$0x7b,0x100\(%rdx\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 00 c4 72 80 7b[ 	]*vpinsrw \$0x7b,-0x100\(%rdx\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 00 c4 b2 fe fe ff ff 7b[ 	]*vpinsrw \$0x7b,-0x102\(%rdx\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 40 04 f4[ 	]*vpmaddubsw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 47 04 f4[ 	]*vpmaddubsw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 15 c7 04 f4[ 	]*vpmaddubsw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 04 31[ 	]*vpmaddubsw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 15 40 04 b4 f0 23 01 00 00[ 	]*vpmaddubsw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 04 72 7f[ 	]*vpmaddubsw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 04 b2 00 20 00 00[ 	]*vpmaddubsw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 04 72 80[ 	]*vpmaddubsw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 04 b2 c0 df ff ff[ 	]*vpmaddubsw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 f5 f4[ 	]*vpmaddwd %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 f5 f4[ 	]*vpmaddwd %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 f5 f4[ 	]*vpmaddwd %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f5 31[ 	]*vpmaddwd \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 f5 b4 f0 23 01 00 00[ 	]*vpmaddwd 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f5 72 7f[ 	]*vpmaddwd 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f5 b2 00 20 00 00[ 	]*vpmaddwd 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f5 72 80[ 	]*vpmaddwd -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f5 b2 c0 df ff ff[ 	]*vpmaddwd -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 40 3c f4[ 	]*vpmaxsb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 47 3c f4[ 	]*vpmaxsb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 15 c7 3c f4[ 	]*vpmaxsb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3c 31[ 	]*vpmaxsb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 15 40 3c b4 f0 23 01 00 00[ 	]*vpmaxsb 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3c 72 7f[ 	]*vpmaxsb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3c b2 00 20 00 00[ 	]*vpmaxsb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3c 72 80[ 	]*vpmaxsb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3c b2 c0 df ff ff[ 	]*vpmaxsb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 ee f4[ 	]*vpmaxsw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 ee f4[ 	]*vpmaxsw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 ee f4[ 	]*vpmaxsw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ee 31[ 	]*vpmaxsw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 ee b4 f0 23 01 00 00[ 	]*vpmaxsw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ee 72 7f[ 	]*vpmaxsw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ee b2 00 20 00 00[ 	]*vpmaxsw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ee 72 80[ 	]*vpmaxsw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ee b2 c0 df ff ff[ 	]*vpmaxsw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 de f4[ 	]*vpmaxub %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 de f4[ 	]*vpmaxub %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 de f4[ 	]*vpmaxub %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 de 31[ 	]*vpmaxub \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 de b4 f0 23 01 00 00[ 	]*vpmaxub 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 de 72 7f[ 	]*vpmaxub 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 de b2 00 20 00 00[ 	]*vpmaxub 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 de 72 80[ 	]*vpmaxub -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 de b2 c0 df ff ff[ 	]*vpmaxub -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 40 3e f4[ 	]*vpmaxuw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 47 3e f4[ 	]*vpmaxuw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 15 c7 3e f4[ 	]*vpmaxuw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3e 31[ 	]*vpmaxuw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 15 40 3e b4 f0 23 01 00 00[ 	]*vpmaxuw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3e 72 7f[ 	]*vpmaxuw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3e b2 00 20 00 00[ 	]*vpmaxuw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3e 72 80[ 	]*vpmaxuw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3e b2 c0 df ff ff[ 	]*vpmaxuw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 40 38 f4[ 	]*vpminsb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 47 38 f4[ 	]*vpminsb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 15 c7 38 f4[ 	]*vpminsb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 38 31[ 	]*vpminsb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 15 40 38 b4 f0 23 01 00 00[ 	]*vpminsb 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 38 72 7f[ 	]*vpminsb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 38 b2 00 20 00 00[ 	]*vpminsb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 38 72 80[ 	]*vpminsb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 38 b2 c0 df ff ff[ 	]*vpminsb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 ea f4[ 	]*vpminsw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 ea f4[ 	]*vpminsw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 ea f4[ 	]*vpminsw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ea 31[ 	]*vpminsw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 ea b4 f0 23 01 00 00[ 	]*vpminsw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ea 72 7f[ 	]*vpminsw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ea b2 00 20 00 00[ 	]*vpminsw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ea 72 80[ 	]*vpminsw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ea b2 c0 df ff ff[ 	]*vpminsw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 da f4[ 	]*vpminub %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 da f4[ 	]*vpminub %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 da f4[ 	]*vpminub %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 da 31[ 	]*vpminub \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 da b4 f0 23 01 00 00[ 	]*vpminub 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 da 72 7f[ 	]*vpminub 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 da b2 00 20 00 00[ 	]*vpminub 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 da 72 80[ 	]*vpminub -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 da b2 c0 df ff ff[ 	]*vpminub -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 40 3a f4[ 	]*vpminuw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 47 3a f4[ 	]*vpminuw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 15 c7 3a f4[ 	]*vpminuw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3a 31[ 	]*vpminuw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 15 40 3a b4 f0 23 01 00 00[ 	]*vpminuw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3a 72 7f[ 	]*vpminuw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3a b2 00 20 00 00[ 	]*vpminuw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3a 72 80[ 	]*vpminuw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3a b2 c0 df ff ff[ 	]*vpminuw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 48 20 f5[ 	]*vpmovsxbw %ymm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 4f 20 f5[ 	]*vpmovsxbw %ymm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7d cf 20 f5[ 	]*vpmovsxbw %ymm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 20 31[ 	]*vpmovsxbw \(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 7d 48 20 b4 f0 23 01 00 00[ 	]*vpmovsxbw 0x123\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 20 72 7f[ 	]*vpmovsxbw 0xfe0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 20 b2 00 10 00 00[ 	]*vpmovsxbw 0x1000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 20 72 80[ 	]*vpmovsxbw -0x1000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 20 b2 e0 ef ff ff[ 	]*vpmovsxbw -0x1020\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 48 30 f5[ 	]*vpmovzxbw %ymm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 4f 30 f5[ 	]*vpmovzxbw %ymm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7d cf 30 f5[ 	]*vpmovzxbw %ymm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 30 31[ 	]*vpmovzxbw \(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 7d 48 30 b4 f0 23 01 00 00[ 	]*vpmovzxbw 0x123\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 30 72 7f[ 	]*vpmovzxbw 0xfe0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 30 b2 00 10 00 00[ 	]*vpmovzxbw 0x1000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 30 72 80[ 	]*vpmovzxbw -0x1000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 30 b2 e0 ef ff ff[ 	]*vpmovzxbw -0x1020\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 40 0b f4[ 	]*vpmulhrsw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 47 0b f4[ 	]*vpmulhrsw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 15 c7 0b f4[ 	]*vpmulhrsw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 0b 31[ 	]*vpmulhrsw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 15 40 0b b4 f0 23 01 00 00[ 	]*vpmulhrsw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 0b 72 7f[ 	]*vpmulhrsw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 0b b2 00 20 00 00[ 	]*vpmulhrsw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 0b 72 80[ 	]*vpmulhrsw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 0b b2 c0 df ff ff[ 	]*vpmulhrsw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 e4 f4[ 	]*vpmulhuw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 e4 f4[ 	]*vpmulhuw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 e4 f4[ 	]*vpmulhuw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e4 31[ 	]*vpmulhuw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 e4 b4 f0 23 01 00 00[ 	]*vpmulhuw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e4 72 7f[ 	]*vpmulhuw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e4 b2 00 20 00 00[ 	]*vpmulhuw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e4 72 80[ 	]*vpmulhuw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e4 b2 c0 df ff ff[ 	]*vpmulhuw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 e5 f4[ 	]*vpmulhw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 e5 f4[ 	]*vpmulhw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 e5 f4[ 	]*vpmulhw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e5 31[ 	]*vpmulhw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 e5 b4 f0 23 01 00 00[ 	]*vpmulhw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e5 72 7f[ 	]*vpmulhw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e5 b2 00 20 00 00[ 	]*vpmulhw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e5 72 80[ 	]*vpmulhw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e5 b2 c0 df ff ff[ 	]*vpmulhw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 d5 f4[ 	]*vpmullw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 d5 f4[ 	]*vpmullw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 d5 f4[ 	]*vpmullw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d5 31[ 	]*vpmullw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 d5 b4 f0 23 01 00 00[ 	]*vpmullw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d5 72 7f[ 	]*vpmullw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d5 b2 00 20 00 00[ 	]*vpmullw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d5 72 80[ 	]*vpmullw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d5 b2 c0 df ff ff[ 	]*vpmullw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 f6 f4[ 	]*vpsadbw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f6 31[ 	]*vpsadbw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 f6 b4 f0 23 01 00 00[ 	]*vpsadbw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f6 72 7f[ 	]*vpsadbw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f6 b2 00 20 00 00[ 	]*vpsadbw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f6 72 80[ 	]*vpsadbw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f6 b2 c0 df ff ff[ 	]*vpsadbw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 40 00 f4[ 	]*vpshufb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 47 00 f4[ 	]*vpshufb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 15 c7 00 f4[ 	]*vpshufb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 00 31[ 	]*vpshufb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 15 40 00 b4 f0 23 01 00 00[ 	]*vpshufb 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 00 72 7f[ 	]*vpshufb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 00 b2 00 20 00 00[ 	]*vpshufb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 00 72 80[ 	]*vpshufb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 00 b2 c0 df ff ff[ 	]*vpshufb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 7e 48 70 f5 ab[ 	]*vpshufhw \$0xab,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 7e 4f 70 f5 ab[ 	]*vpshufhw \$0xab,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7e cf 70 f5 ab[ 	]*vpshufhw \$0xab,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7e 48 70 f5 7b[ 	]*vpshufhw \$0x7b,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7e 48 70 31 7b[ 	]*vpshufhw \$0x7b,\(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 7e 48 70 b4 f0 23 01 00 00 7b[ 	]*vpshufhw \$0x7b,0x123\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7e 48 70 72 7f 7b[ 	]*vpshufhw \$0x7b,0x1fc0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7e 48 70 b2 00 20 00 00 7b[ 	]*vpshufhw \$0x7b,0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7e 48 70 72 80 7b[ 	]*vpshufhw \$0x7b,-0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7e 48 70 b2 c0 df ff ff 7b[ 	]*vpshufhw \$0x7b,-0x2040\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 48 70 f5 ab[ 	]*vpshuflw \$0xab,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 4f 70 f5 ab[ 	]*vpshuflw \$0xab,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7f cf 70 f5 ab[ 	]*vpshuflw \$0xab,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 48 70 f5 7b[ 	]*vpshuflw \$0x7b,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 70 31 7b[ 	]*vpshuflw \$0x7b,\(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 7f 48 70 b4 f0 23 01 00 00 7b[ 	]*vpshuflw \$0x7b,0x123\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 70 72 7f 7b[ 	]*vpshuflw \$0x7b,0x1fc0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 70 b2 00 20 00 00 7b[ 	]*vpshuflw \$0x7b,0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 70 72 80 7b[ 	]*vpshuflw \$0x7b,-0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 70 b2 c0 df ff ff 7b[ 	]*vpshuflw \$0x7b,-0x2040\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 f1 f4[ 	]*vpsllw %xmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 f1 f4[ 	]*vpsllw %xmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 f1 f4[ 	]*vpsllw %xmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f1 31[ 	]*vpsllw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 f1 b4 f0 23 01 00 00[ 	]*vpsllw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f1 72 7f[ 	]*vpsllw 0x7f0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f1 b2 00 08 00 00[ 	]*vpsllw 0x800\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f1 72 80[ 	]*vpsllw -0x800\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f1 b2 f0 f7 ff ff[ 	]*vpsllw -0x810\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 e1 f4[ 	]*vpsraw %xmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 e1 f4[ 	]*vpsraw %xmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 e1 f4[ 	]*vpsraw %xmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e1 31[ 	]*vpsraw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 e1 b4 f0 23 01 00 00[ 	]*vpsraw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e1 72 7f[ 	]*vpsraw 0x7f0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e1 b2 00 08 00 00[ 	]*vpsraw 0x800\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e1 72 80[ 	]*vpsraw -0x800\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e1 b2 f0 f7 ff ff[ 	]*vpsraw -0x810\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 d1 f4[ 	]*vpsrlw %xmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 d1 f4[ 	]*vpsrlw %xmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 d1 f4[ 	]*vpsrlw %xmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d1 31[ 	]*vpsrlw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 d1 b4 f0 23 01 00 00[ 	]*vpsrlw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d1 72 7f[ 	]*vpsrlw 0x7f0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d1 b2 00 08 00 00[ 	]*vpsrlw 0x800\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d1 72 80[ 	]*vpsrlw -0x800\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d1 b2 f0 f7 ff ff[ 	]*vpsrlw -0x810\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 73 dd ab[ 	]*vpsrldq \$0xab,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 73 dd 7b[ 	]*vpsrldq \$0x7b,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 73 19 7b[ 	]*vpsrldq \$0x7b,\(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 b1 0d 40 73 9c f0 23 01 00 00 7b[ 	]*vpsrldq \$0x7b,0x123\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 73 5a 7f 7b[ 	]*vpsrldq \$0x7b,0x1fc0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 73 9a 00 20 00 00 7b[ 	]*vpsrldq \$0x7b,0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 73 5a 80 7b[ 	]*vpsrldq \$0x7b,-0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 73 9a c0 df ff ff 7b[ 	]*vpsrldq \$0x7b,-0x2040\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 71 d5 ab[ 	]*vpsrlw \$0xab,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 47 71 d5 ab[ 	]*vpsrlw \$0xab,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 91 0d c7 71 d5 ab[ 	]*vpsrlw \$0xab,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 71 d5 7b[ 	]*vpsrlw \$0x7b,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 11 7b[ 	]*vpsrlw \$0x7b,\(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 b1 0d 40 71 94 f0 23 01 00 00 7b[ 	]*vpsrlw \$0x7b,0x123\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 52 7f 7b[ 	]*vpsrlw \$0x7b,0x1fc0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 92 00 20 00 00 7b[ 	]*vpsrlw \$0x7b,0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 52 80 7b[ 	]*vpsrlw \$0x7b,-0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 92 c0 df ff ff 7b[ 	]*vpsrlw \$0x7b,-0x2040\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 71 e5 ab[ 	]*vpsraw \$0xab,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 47 71 e5 ab[ 	]*vpsraw \$0xab,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 91 0d c7 71 e5 ab[ 	]*vpsraw \$0xab,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 71 e5 7b[ 	]*vpsraw \$0x7b,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 21 7b[ 	]*vpsraw \$0x7b,\(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 b1 0d 40 71 a4 f0 23 01 00 00 7b[ 	]*vpsraw \$0x7b,0x123\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 62 7f 7b[ 	]*vpsraw \$0x7b,0x1fc0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 a2 00 20 00 00 7b[ 	]*vpsraw \$0x7b,0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 62 80 7b[ 	]*vpsraw \$0x7b,-0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 a2 c0 df ff ff 7b[ 	]*vpsraw \$0x7b,-0x2040\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 40 10 f4[ 	]*vpsrlvw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 47 10 f4[ 	]*vpsrlvw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 95 c7 10 f4[ 	]*vpsrlvw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 10 31[ 	]*vpsrlvw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 95 40 10 b4 f0 23 01 00 00[ 	]*vpsrlvw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 10 72 7f[ 	]*vpsrlvw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 10 b2 00 20 00 00[ 	]*vpsrlvw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 10 72 80[ 	]*vpsrlvw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 10 b2 c0 df ff ff[ 	]*vpsrlvw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 40 11 f4[ 	]*vpsravw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 47 11 f4[ 	]*vpsravw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 95 c7 11 f4[ 	]*vpsravw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 11 31[ 	]*vpsravw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 95 40 11 b4 f0 23 01 00 00[ 	]*vpsravw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 11 72 7f[ 	]*vpsravw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 11 b2 00 20 00 00[ 	]*vpsravw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 11 72 80[ 	]*vpsravw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 11 b2 c0 df ff ff[ 	]*vpsravw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 f8 f4[ 	]*vpsubb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 f8 f4[ 	]*vpsubb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 f8 f4[ 	]*vpsubb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f8 31[ 	]*vpsubb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 f8 b4 f0 23 01 00 00[ 	]*vpsubb 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f8 72 7f[ 	]*vpsubb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f8 b2 00 20 00 00[ 	]*vpsubb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f8 72 80[ 	]*vpsubb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f8 b2 c0 df ff ff[ 	]*vpsubb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 e8 f4[ 	]*vpsubsb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 e8 f4[ 	]*vpsubsb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 e8 f4[ 	]*vpsubsb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e8 31[ 	]*vpsubsb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 e8 b4 f0 23 01 00 00[ 	]*vpsubsb 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e8 72 7f[ 	]*vpsubsb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e8 b2 00 20 00 00[ 	]*vpsubsb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e8 72 80[ 	]*vpsubsb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e8 b2 c0 df ff ff[ 	]*vpsubsb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 e9 f4[ 	]*vpsubsw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 e9 f4[ 	]*vpsubsw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 e9 f4[ 	]*vpsubsw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e9 31[ 	]*vpsubsw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 e9 b4 f0 23 01 00 00[ 	]*vpsubsw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e9 72 7f[ 	]*vpsubsw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e9 b2 00 20 00 00[ 	]*vpsubsw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e9 72 80[ 	]*vpsubsw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e9 b2 c0 df ff ff[ 	]*vpsubsw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 d8 f4[ 	]*vpsubusb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 d8 f4[ 	]*vpsubusb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 d8 f4[ 	]*vpsubusb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d8 31[ 	]*vpsubusb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 d8 b4 f0 23 01 00 00[ 	]*vpsubusb 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d8 72 7f[ 	]*vpsubusb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d8 b2 00 20 00 00[ 	]*vpsubusb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d8 72 80[ 	]*vpsubusb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d8 b2 c0 df ff ff[ 	]*vpsubusb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 d9 f4[ 	]*vpsubusw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 d9 f4[ 	]*vpsubusw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 d9 f4[ 	]*vpsubusw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d9 31[ 	]*vpsubusw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 d9 b4 f0 23 01 00 00[ 	]*vpsubusw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d9 72 7f[ 	]*vpsubusw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d9 b2 00 20 00 00[ 	]*vpsubusw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d9 72 80[ 	]*vpsubusw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d9 b2 c0 df ff ff[ 	]*vpsubusw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 f9 f4[ 	]*vpsubw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 f9 f4[ 	]*vpsubw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 f9 f4[ 	]*vpsubw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f9 31[ 	]*vpsubw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 f9 b4 f0 23 01 00 00[ 	]*vpsubw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f9 72 7f[ 	]*vpsubw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f9 b2 00 20 00 00[ 	]*vpsubw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f9 72 80[ 	]*vpsubw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f9 b2 c0 df ff ff[ 	]*vpsubw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 68 f4[ 	]*vpunpckhbw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 68 f4[ 	]*vpunpckhbw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 68 f4[ 	]*vpunpckhbw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 68 31[ 	]*vpunpckhbw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 68 b4 f0 23 01 00 00[ 	]*vpunpckhbw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 68 72 7f[ 	]*vpunpckhbw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 68 b2 00 20 00 00[ 	]*vpunpckhbw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 68 72 80[ 	]*vpunpckhbw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 68 b2 c0 df ff ff[ 	]*vpunpckhbw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 69 f4[ 	]*vpunpckhwd %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 69 f4[ 	]*vpunpckhwd %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 69 f4[ 	]*vpunpckhwd %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 69 31[ 	]*vpunpckhwd \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 69 b4 f0 23 01 00 00[ 	]*vpunpckhwd 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 69 72 7f[ 	]*vpunpckhwd 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 69 b2 00 20 00 00[ 	]*vpunpckhwd 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 69 72 80[ 	]*vpunpckhwd -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 69 b2 c0 df ff ff[ 	]*vpunpckhwd -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 60 f4[ 	]*vpunpcklbw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 60 f4[ 	]*vpunpcklbw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 60 f4[ 	]*vpunpcklbw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 60 31[ 	]*vpunpcklbw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 60 b4 f0 23 01 00 00[ 	]*vpunpcklbw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 60 72 7f[ 	]*vpunpcklbw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 60 b2 00 20 00 00[ 	]*vpunpcklbw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 60 72 80[ 	]*vpunpcklbw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 60 b2 c0 df ff ff[ 	]*vpunpcklbw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 61 f4[ 	]*vpunpcklwd %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 61 f4[ 	]*vpunpcklwd %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 61 f4[ 	]*vpunpcklwd %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 61 31[ 	]*vpunpcklwd \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 61 b4 f0 23 01 00 00[ 	]*vpunpcklwd 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 61 72 7f[ 	]*vpunpcklwd 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 61 b2 00 20 00 00[ 	]*vpunpcklwd 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 61 72 80[ 	]*vpunpcklwd -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 61 b2 c0 df ff ff[ 	]*vpunpcklwd -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7e 48 30 ee[ 	]*vpmovwb %zmm29,%ymm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7e 4f 30 ee[ 	]*vpmovwb %zmm29,%ymm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7e cf 30 ee[ 	]*vpmovwb %zmm29,%ymm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7e 48 20 ee[ 	]*vpmovswb %zmm29,%ymm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7e 4f 20 ee[ 	]*vpmovswb %zmm29,%ymm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7e cf 20 ee[ 	]*vpmovswb %zmm29,%ymm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7e 48 10 ee[ 	]*vpmovuswb %zmm29,%ymm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7e 4f 10 ee[ 	]*vpmovuswb %zmm29,%ymm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7e cf 10 ee[ 	]*vpmovuswb %zmm29,%ymm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 03 15 40 42 f4 ab[ 	]*vdbpsadbw \$0xab,%zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 03 15 47 42 f4 ab[ 	]*vdbpsadbw \$0xab,%zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 03 15 c7 42 f4 ab[ 	]*vdbpsadbw \$0xab,%zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 03 15 40 42 f4 7b[ 	]*vdbpsadbw \$0x7b,%zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 40 42 31 7b[ 	]*vdbpsadbw \$0x7b,\(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 23 15 40 42 b4 f0 23 01 00 00 7b[ 	]*vdbpsadbw \$0x7b,0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 40 42 72 7f 7b[ 	]*vdbpsadbw \$0x7b,0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 40 42 b2 00 20 00 00 7b[ 	]*vdbpsadbw \$0x7b,0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 40 42 72 80 7b[ 	]*vdbpsadbw \$0x7b,-0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 40 42 b2 c0 df ff ff 7b[ 	]*vdbpsadbw \$0x7b,-0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 40 8d f4[ 	]*vpermw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 47 8d f4[ 	]*vpermw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 95 c7 8d f4[ 	]*vpermw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 8d 31[ 	]*vpermw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 95 40 8d b4 f0 23 01 00 00[ 	]*vpermw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 8d 72 7f[ 	]*vpermw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 8d b2 00 20 00 00[ 	]*vpermw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 8d 72 80[ 	]*vpermw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 8d b2 c0 df ff ff[ 	]*vpermw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 40 7d f4[ 	]*vpermt2w %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 47 7d f4[ 	]*vpermt2w %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 95 c7 7d f4[ 	]*vpermt2w %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 7d 31[ 	]*vpermt2w \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 95 40 7d b4 f0 23 01 00 00[ 	]*vpermt2w 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 7d 72 7f[ 	]*vpermt2w 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 7d b2 00 20 00 00[ 	]*vpermt2w 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 7d 72 80[ 	]*vpermt2w -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 7d b2 c0 df ff ff[ 	]*vpermt2w -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 73 fd ab[ 	]*vpslldq \$0xab,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 73 fd 7b[ 	]*vpslldq \$0x7b,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 73 39 7b[ 	]*vpslldq \$0x7b,\(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 b1 0d 40 73 bc f0 23 01 00 00 7b[ 	]*vpslldq \$0x7b,0x123\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 73 7a 7f 7b[ 	]*vpslldq \$0x7b,0x1fc0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 73 ba 00 20 00 00 7b[ 	]*vpslldq \$0x7b,0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 73 7a 80 7b[ 	]*vpslldq \$0x7b,-0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 73 ba c0 df ff ff 7b[ 	]*vpslldq \$0x7b,-0x2040\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 71 f5 ab[ 	]*vpsllw \$0xab,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 47 71 f5 ab[ 	]*vpsllw \$0xab,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 91 0d c7 71 f5 ab[ 	]*vpsllw \$0xab,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 71 f5 7b[ 	]*vpsllw \$0x7b,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 31 7b[ 	]*vpsllw \$0x7b,\(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 b1 0d 40 71 b4 f0 23 01 00 00 7b[ 	]*vpsllw \$0x7b,0x123\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 72 7f 7b[ 	]*vpsllw \$0x7b,0x1fc0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 b2 00 20 00 00 7b[ 	]*vpsllw \$0x7b,0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 72 80 7b[ 	]*vpsllw \$0x7b,-0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 b2 c0 df ff ff 7b[ 	]*vpsllw \$0x7b,-0x2040\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 40 12 f4[ 	]*vpsllvw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 47 12 f4[ 	]*vpsllvw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 95 c7 12 f4[ 	]*vpsllvw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 12 31[ 	]*vpsllvw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 95 40 12 b4 f0 23 01 00 00[ 	]*vpsllvw 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 12 72 7f[ 	]*vpsllvw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 12 b2 00 20 00 00[ 	]*vpsllvw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 12 72 80[ 	]*vpsllvw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 12 b2 c0 df ff ff[ 	]*vpsllvw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 48 6f f5[ 	]*vmovdqu8 %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 4f 6f f5[ 	]*vmovdqu8 %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7f cf 6f f5[ 	]*vmovdqu8 %zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 6f 31[ 	]*vmovdqu8 \(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 7f 48 6f b4 f0 23 01 00 00[ 	]*vmovdqu8 0x123\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 6f 72 7f[ 	]*vmovdqu8 0x1fc0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 6f b2 00 20 00 00[ 	]*vmovdqu8 0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 6f 72 80[ 	]*vmovdqu8 -0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 6f b2 c0 df ff ff[ 	]*vmovdqu8 -0x2040\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 ff 48 6f f5[ 	]*vmovdqu16 %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 ff 4f 6f f5[ 	]*vmovdqu16 %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 ff cf 6f f5[ 	]*vmovdqu16 %zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 ff 48 6f 31[ 	]*vmovdqu16 \(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 ff 48 6f b4 f0 23 01 00 00[ 	]*vmovdqu16 0x123\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 ff 48 6f 72 7f[ 	]*vmovdqu16 0x1fc0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 ff 48 6f b2 00 20 00 00[ 	]*vmovdqu16 0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 ff 48 6f 72 80[ 	]*vmovdqu16 -0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 ff 48 6f b2 c0 df ff ff[ 	]*vmovdqu16 -0x2040\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cc 41 ef[ 	]*kandq  %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cd 41 ef[ 	]*kandd  %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cc 42 ef[ 	]*kandnq %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cd 42 ef[ 	]*kandnd %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cc 45 ef[ 	]*korq   %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cd 45 ef[ 	]*kord   %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cc 46 ef[ 	]*kxnorq %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cd 46 ef[ 	]*kxnord %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cc 47 ef[ 	]*kxorq  %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cd 47 ef[ 	]*kxord  %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f8 44 ee[ 	]*knotq  %k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f9 44 ee[ 	]*knotd  %k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f8 98 ee[ 	]*kortestq %k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f9 98 ee[ 	]*kortestd %k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f8 99 ee[ 	]*ktestq %k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f9 99 ee[ 	]*ktestd %k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e3 f9 31 ee ab[ 	]*kshiftrq \$0xab,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e3 f9 31 ee 7b[ 	]*kshiftrq \$0x7b,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e3 79 31 ee ab[ 	]*kshiftrd \$0xab,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e3 79 31 ee 7b[ 	]*kshiftrd \$0x7b,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e3 f9 33 ee ab[ 	]*kshiftlq \$0xab,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e3 f9 33 ee 7b[ 	]*kshiftlq \$0x7b,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e3 79 33 ee ab[ 	]*kshiftld \$0xab,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e3 79 33 ee 7b[ 	]*kshiftld \$0x7b,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f8 90 ee[ 	]*kmovq  %k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f8 90 29[ 	]*kmovq  \(%rcx\),%k5
[ 	]*[a-f0-9]+:[ 	]*c4 a1 f8 90 ac f0 23 01 00 00[ 	]*kmovq  0x123\(%rax,%r14,8\),%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f9 90 ee[ 	]*kmovd  %k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f9 90 29[ 	]*kmovd  \(%rcx\),%k5
[ 	]*[a-f0-9]+:[ 	]*c4 a1 f9 90 ac f0 23 01 00 00[ 	]*kmovd  0x123\(%rax,%r14,8\),%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f8 91 29[ 	]*kmovq  %k5,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*c4 a1 f8 91 ac f0 23 01 00 00[ 	]*kmovq  %k5,0x123\(%rax,%r14,8\)
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f9 91 29[ 	]*kmovd  %k5,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*c4 a1 f9 91 ac f0 23 01 00 00[ 	]*kmovd  %k5,0x123\(%rax,%r14,8\)
[ 	]*[a-f0-9]+:[ 	]*c4 e1 fb 92 e8[ 	]*kmovq  %rax,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 c1 fb 92 e8[ 	]*kmovq  %r8,%k5
[ 	]*[a-f0-9]+:[ 	]*c5 fb 92 e8[ 	]*kmovd  %eax,%k5
[ 	]*[a-f0-9]+:[ 	]*c5 fb 92 ed[ 	]*kmovd  %ebp,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 c1 7b 92 ed[ 	]*kmovd  %r13d,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 fb 93 c5[ 	]*kmovq  %k5,%rax
[ 	]*[a-f0-9]+:[ 	]*c4 61 fb 93 c5[ 	]*kmovq  %k5,%r8
[ 	]*[a-f0-9]+:[ 	]*c5 fb 93 c5[ 	]*kmovd  %k5,%eax
[ 	]*[a-f0-9]+:[ 	]*c5 fb 93 ed[ 	]*kmovd  %k5,%ebp
[ 	]*[a-f0-9]+:[ 	]*c5 7b 93 ed[ 	]*kmovd  %k5,%r13d
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cc 4a ef[ 	]*kaddq  %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cd 4a ef[ 	]*kaddd  %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c5 cc 4b ef[ 	]*kunpckwd %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cc 4b ef[ 	]*kunpckdq %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 30 31[ 	]*vpmovwb %zmm30,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 4f 30 31[ 	]*vpmovwb %zmm30,\(%rcx\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 22 7e 48 30 b4 f0 23 01 00 00[ 	]*vpmovwb %zmm30,0x123\(%rax,%r14,8\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 30 72 7f[ 	]*vpmovwb %zmm30,0xfe0\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 30 b2 00 10 00 00[ 	]*vpmovwb %zmm30,0x1000\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 30 72 80[ 	]*vpmovwb %zmm30,-0x1000\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 30 b2 e0 ef ff ff[ 	]*vpmovwb %zmm30,-0x1020\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 20 31[ 	]*vpmovswb %zmm30,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 4f 20 31[ 	]*vpmovswb %zmm30,\(%rcx\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 22 7e 48 20 b4 f0 23 01 00 00[ 	]*vpmovswb %zmm30,0x123\(%rax,%r14,8\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 20 72 7f[ 	]*vpmovswb %zmm30,0xfe0\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 20 b2 00 10 00 00[ 	]*vpmovswb %zmm30,0x1000\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 20 72 80[ 	]*vpmovswb %zmm30,-0x1000\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 20 b2 e0 ef ff ff[ 	]*vpmovswb %zmm30,-0x1020\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 10 31[ 	]*vpmovuswb %zmm30,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 4f 10 31[ 	]*vpmovuswb %zmm30,\(%rcx\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 22 7e 48 10 b4 f0 23 01 00 00[ 	]*vpmovuswb %zmm30,0x123\(%rax,%r14,8\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 10 72 7f[ 	]*vpmovuswb %zmm30,0xfe0\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 10 b2 00 10 00 00[ 	]*vpmovuswb %zmm30,0x1000\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 10 72 80[ 	]*vpmovuswb %zmm30,-0x1000\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 10 b2 e0 ef ff ff[ 	]*vpmovuswb %zmm30,-0x1020\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 7f 31[ 	]*vmovdqu8 %zmm30,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 4f 7f 31[ 	]*vmovdqu8 %zmm30,\(%rcx\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 21 7f 48 7f b4 f0 23 01 00 00[ 	]*vmovdqu8 %zmm30,0x123\(%rax,%r14,8\)
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 7f 72 7f[ 	]*vmovdqu8 %zmm30,0x1fc0\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 7f b2 00 20 00 00[ 	]*vmovdqu8 %zmm30,0x2000\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 7f 72 80[ 	]*vmovdqu8 %zmm30,-0x2000\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 7f b2 c0 df ff ff[ 	]*vmovdqu8 %zmm30,-0x2040\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 61 ff 48 7f 31[ 	]*vmovdqu16 %zmm30,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*62 61 ff 4f 7f 31[ 	]*vmovdqu16 %zmm30,\(%rcx\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 21 ff 48 7f b4 f0 23 01 00 00[ 	]*vmovdqu16 %zmm30,0x123\(%rax,%r14,8\)
[ 	]*[a-f0-9]+:[ 	]*62 61 ff 48 7f 72 7f[ 	]*vmovdqu16 %zmm30,0x1fc0\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 61 ff 48 7f b2 00 20 00 00[ 	]*vmovdqu16 %zmm30,0x2000\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 61 ff 48 7f 72 80[ 	]*vmovdqu16 %zmm30,-0x2000\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 61 ff 48 7f b2 c0 df ff ff[ 	]*vmovdqu16 %zmm30,-0x2040\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 02 95 40 75 f4[ 	]*vpermi2w %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 47 75 f4[ 	]*vpermi2w %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 95 c7 75 f4[ 	]*vpermi2w %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 75 31[ 	]*vpermi2w \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 95 40 75 b4 f0 23 01 00 00[ 	]*vpermi2w 0x123\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 75 72 7f[ 	]*vpermi2w 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 75 b2 00 20 00 00[ 	]*vpermi2w 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 75 72 80[ 	]*vpermi2w -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 75 b2 c0 df ff ff[ 	]*vpermi2w -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 92 0d 40 26 ed[ 	]*vptestmb %zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 92 0d 47 26 ed[ 	]*vptestmb %zmm29,%zmm30,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 0d 40 26 29[ 	]*vptestmb \(%rcx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b2 0d 40 26 ac f0 23 01 00 00[ 	]*vptestmb 0x123\(%rax,%r14,8\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 0d 40 26 6a 7f[ 	]*vptestmb 0x1fc0\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 0d 40 26 aa 00 20 00 00[ 	]*vptestmb 0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 0d 40 26 6a 80[ 	]*vptestmb -0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 0d 40 26 aa c0 df ff ff[ 	]*vptestmb -0x2040\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 92 8d 40 26 ed[ 	]*vptestmw %zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 92 8d 47 26 ed[ 	]*vptestmw %zmm29,%zmm30,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 8d 40 26 29[ 	]*vptestmw \(%rcx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b2 8d 40 26 ac f0 23 01 00 00[ 	]*vptestmw 0x123\(%rax,%r14,8\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 8d 40 26 6a 7f[ 	]*vptestmw 0x1fc0\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 8d 40 26 aa 00 20 00 00[ 	]*vptestmw 0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 8d 40 26 6a 80[ 	]*vptestmw -0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 8d 40 26 aa c0 df ff ff[ 	]*vptestmw -0x2040\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 92 7e 48 29 ee[ 	]*vpmovb2m %zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 92 fe 48 29 ee[ 	]*vpmovw2m %zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 28 f5[ 	]*vpmovm2b %k5,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 fe 48 28 f5[ 	]*vpmovm2w %k5,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 92 16 40 26 ec[ 	]*vptestnmb %zmm28,%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 92 16 47 26 ec[ 	]*vptestnmb %zmm28,%zmm29,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 16 40 26 29[ 	]*vptestnmb \(%rcx\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b2 16 40 26 ac f0 23 01 00 00[ 	]*vptestnmb 0x123\(%rax,%r14,8\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 16 40 26 6a 7f[ 	]*vptestnmb 0x1fc0\(%rdx\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 16 40 26 aa 00 20 00 00[ 	]*vptestnmb 0x2000\(%rdx\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 16 40 26 6a 80[ 	]*vptestnmb -0x2000\(%rdx\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 16 40 26 aa c0 df ff ff[ 	]*vptestnmb -0x2040\(%rdx\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 92 96 40 26 ec[ 	]*vptestnmw %zmm28,%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 92 96 47 26 ec[ 	]*vptestnmw %zmm28,%zmm29,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 96 40 26 29[ 	]*vptestnmw \(%rcx\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b2 96 40 26 ac f0 23 01 00 00[ 	]*vptestnmw 0x123\(%rax,%r14,8\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 96 40 26 6a 7f[ 	]*vptestnmw 0x1fc0\(%rdx\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 96 40 26 aa 00 20 00 00[ 	]*vptestnmw 0x2000\(%rdx\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 96 40 26 6a 80[ 	]*vptestnmw -0x2000\(%rdx\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 96 40 26 aa c0 df ff ff[ 	]*vptestnmw -0x2040\(%rdx\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 93 0d 40 3f ed ab[ 	]*vpcmpb \$0xab,%zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 93 0d 47 3f ed ab[ 	]*vpcmpb \$0xab,%zmm29,%zmm30,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 93 0d 40 3f ed 7b[ 	]*vpcmpb \$0x7b,%zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 0d 40 3f 29 7b[ 	]*vpcmpb \$0x7b,\(%rcx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b3 0d 40 3f ac f0 23 01 00 00 7b[ 	]*vpcmpb \$0x7b,0x123\(%rax,%r14,8\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 0d 40 3f 6a 7f 7b[ 	]*vpcmpb \$0x7b,0x1fc0\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 0d 40 3f aa 00 20 00 00 7b[ 	]*vpcmpb \$0x7b,0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 0d 40 3f 6a 80 7b[ 	]*vpcmpb \$0x7b,-0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 0d 40 3f aa c0 df ff ff 7b[ 	]*vpcmpb \$0x7b,-0x2040\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 93 8d 40 3f ed ab[ 	]*vpcmpw \$0xab,%zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 93 8d 47 3f ed ab[ 	]*vpcmpw \$0xab,%zmm29,%zmm30,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 93 8d 40 3f ed 7b[ 	]*vpcmpw \$0x7b,%zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 8d 40 3f 29 7b[ 	]*vpcmpw \$0x7b,\(%rcx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b3 8d 40 3f ac f0 23 01 00 00 7b[ 	]*vpcmpw \$0x7b,0x123\(%rax,%r14,8\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 8d 40 3f 6a 7f 7b[ 	]*vpcmpw \$0x7b,0x1fc0\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 8d 40 3f aa 00 20 00 00 7b[ 	]*vpcmpw \$0x7b,0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 8d 40 3f 6a 80 7b[ 	]*vpcmpw \$0x7b,-0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 8d 40 3f aa c0 df ff ff 7b[ 	]*vpcmpw \$0x7b,-0x2040\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 93 0d 40 3e ed ab[ 	]*vpcmpub \$0xab,%zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 93 0d 47 3e ed ab[ 	]*vpcmpub \$0xab,%zmm29,%zmm30,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 93 0d 40 3e ed 7b[ 	]*vpcmpub \$0x7b,%zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 0d 40 3e 29 7b[ 	]*vpcmpub \$0x7b,\(%rcx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b3 0d 40 3e ac f0 23 01 00 00 7b[ 	]*vpcmpub \$0x7b,0x123\(%rax,%r14,8\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 0d 40 3e 6a 7f 7b[ 	]*vpcmpub \$0x7b,0x1fc0\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 0d 40 3e aa 00 20 00 00 7b[ 	]*vpcmpub \$0x7b,0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 0d 40 3e 6a 80 7b[ 	]*vpcmpub \$0x7b,-0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 0d 40 3e aa c0 df ff ff 7b[ 	]*vpcmpub \$0x7b,-0x2040\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 93 8d 40 3e ed ab[ 	]*vpcmpuw \$0xab,%zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 93 8d 47 3e ed ab[ 	]*vpcmpuw \$0xab,%zmm29,%zmm30,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 93 8d 40 3e ed 7b[ 	]*vpcmpuw \$0x7b,%zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 8d 40 3e 29 7b[ 	]*vpcmpuw \$0x7b,\(%rcx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b3 8d 40 3e ac f0 23 01 00 00 7b[ 	]*vpcmpuw \$0x7b,0x123\(%rax,%r14,8\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 8d 40 3e 6a 7f 7b[ 	]*vpcmpuw \$0x7b,0x1fc0\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 8d 40 3e aa 00 20 00 00 7b[ 	]*vpcmpuw \$0x7b,0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 8d 40 3e 6a 80 7b[ 	]*vpcmpuw \$0x7b,-0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 8d 40 3e aa c0 df ff ff 7b[ 	]*vpcmpuw \$0x7b,-0x2040\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 48 1c f5[ 	]*vpabsb %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 4f 1c f5[ 	]*vpabsb %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7d cf 1c f5[ 	]*vpabsb %zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 1c 31[ 	]*vpabsb \(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 7d 48 1c b4 f0 34 12 00 00[ 	]*vpabsb 0x1234\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 1c 72 7f[ 	]*vpabsb 0x1fc0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 1c b2 00 20 00 00[ 	]*vpabsb 0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 1c 72 80[ 	]*vpabsb -0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 1c b2 c0 df ff ff[ 	]*vpabsb -0x2040\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 48 1d f5[ 	]*vpabsw %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 4f 1d f5[ 	]*vpabsw %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7d cf 1d f5[ 	]*vpabsw %zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 1d 31[ 	]*vpabsw \(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 7d 48 1d b4 f0 34 12 00 00[ 	]*vpabsw 0x1234\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 1d 72 7f[ 	]*vpabsw 0x1fc0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 1d b2 00 20 00 00[ 	]*vpabsw 0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 1d 72 80[ 	]*vpabsw -0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 1d b2 c0 df ff ff[ 	]*vpabsw -0x2040\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 6b f4[ 	]*vpackssdw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 6b f4[ 	]*vpackssdw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 6b f4[ 	]*vpackssdw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 6b 31[ 	]*vpackssdw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 6b b4 f0 34 12 00 00[ 	]*vpackssdw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 50 6b 31[ 	]*vpackssdw \(%rcx\)\{1to16\},%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 6b 72 7f[ 	]*vpackssdw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 6b b2 00 20 00 00[ 	]*vpackssdw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 6b 72 80[ 	]*vpackssdw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 6b b2 c0 df ff ff[ 	]*vpackssdw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 50 6b 72 7f[ 	]*vpackssdw 0x1fc\(%rdx\)\{1to16\},%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 50 6b b2 00 02 00 00[ 	]*vpackssdw 0x200\(%rdx\)\{1to16\},%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 50 6b 72 80[ 	]*vpackssdw -0x200\(%rdx\)\{1to16\},%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 50 6b b2 fc fd ff ff[ 	]*vpackssdw -0x204\(%rdx\)\{1to16\},%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 63 f4[ 	]*vpacksswb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 63 f4[ 	]*vpacksswb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 63 f4[ 	]*vpacksswb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 63 31[ 	]*vpacksswb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 63 b4 f0 34 12 00 00[ 	]*vpacksswb 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 63 72 7f[ 	]*vpacksswb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 63 b2 00 20 00 00[ 	]*vpacksswb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 63 72 80[ 	]*vpacksswb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 63 b2 c0 df ff ff[ 	]*vpacksswb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 40 2b f4[ 	]*vpackusdw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 47 2b f4[ 	]*vpackusdw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 15 c7 2b f4[ 	]*vpackusdw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 2b 31[ 	]*vpackusdw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 15 40 2b b4 f0 34 12 00 00[ 	]*vpackusdw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 50 2b 31[ 	]*vpackusdw \(%rcx\)\{1to16\},%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 2b 72 7f[ 	]*vpackusdw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 2b b2 00 20 00 00[ 	]*vpackusdw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 2b 72 80[ 	]*vpackusdw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 2b b2 c0 df ff ff[ 	]*vpackusdw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 50 2b 72 7f[ 	]*vpackusdw 0x1fc\(%rdx\)\{1to16\},%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 50 2b b2 00 02 00 00[ 	]*vpackusdw 0x200\(%rdx\)\{1to16\},%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 50 2b 72 80[ 	]*vpackusdw -0x200\(%rdx\)\{1to16\},%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 50 2b b2 fc fd ff ff[ 	]*vpackusdw -0x204\(%rdx\)\{1to16\},%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 67 f4[ 	]*vpackuswb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 67 f4[ 	]*vpackuswb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 67 f4[ 	]*vpackuswb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 67 31[ 	]*vpackuswb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 67 b4 f0 34 12 00 00[ 	]*vpackuswb 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 67 72 7f[ 	]*vpackuswb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 67 b2 00 20 00 00[ 	]*vpackuswb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 67 72 80[ 	]*vpackuswb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 67 b2 c0 df ff ff[ 	]*vpackuswb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 fc f4[ 	]*vpaddb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 fc f4[ 	]*vpaddb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 fc f4[ 	]*vpaddb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 fc 31[ 	]*vpaddb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 fc b4 f0 34 12 00 00[ 	]*vpaddb 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 fc 72 7f[ 	]*vpaddb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 fc b2 00 20 00 00[ 	]*vpaddb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 fc 72 80[ 	]*vpaddb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 fc b2 c0 df ff ff[ 	]*vpaddb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 ec f4[ 	]*vpaddsb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 ec f4[ 	]*vpaddsb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 ec f4[ 	]*vpaddsb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ec 31[ 	]*vpaddsb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 ec b4 f0 34 12 00 00[ 	]*vpaddsb 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ec 72 7f[ 	]*vpaddsb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ec b2 00 20 00 00[ 	]*vpaddsb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ec 72 80[ 	]*vpaddsb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ec b2 c0 df ff ff[ 	]*vpaddsb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 ed f4[ 	]*vpaddsw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 ed f4[ 	]*vpaddsw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 ed f4[ 	]*vpaddsw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ed 31[ 	]*vpaddsw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 ed b4 f0 34 12 00 00[ 	]*vpaddsw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ed 72 7f[ 	]*vpaddsw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ed b2 00 20 00 00[ 	]*vpaddsw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ed 72 80[ 	]*vpaddsw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ed b2 c0 df ff ff[ 	]*vpaddsw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 dc f4[ 	]*vpaddusb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 dc f4[ 	]*vpaddusb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 dc f4[ 	]*vpaddusb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 dc 31[ 	]*vpaddusb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 dc b4 f0 34 12 00 00[ 	]*vpaddusb 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 dc 72 7f[ 	]*vpaddusb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 dc b2 00 20 00 00[ 	]*vpaddusb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 dc 72 80[ 	]*vpaddusb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 dc b2 c0 df ff ff[ 	]*vpaddusb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 dd f4[ 	]*vpaddusw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 dd f4[ 	]*vpaddusw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 dd f4[ 	]*vpaddusw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 dd 31[ 	]*vpaddusw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 dd b4 f0 34 12 00 00[ 	]*vpaddusw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 dd 72 7f[ 	]*vpaddusw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 dd b2 00 20 00 00[ 	]*vpaddusw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 dd 72 80[ 	]*vpaddusw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 dd b2 c0 df ff ff[ 	]*vpaddusw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 fd f4[ 	]*vpaddw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 fd f4[ 	]*vpaddw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 fd f4[ 	]*vpaddw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 fd 31[ 	]*vpaddw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 fd b4 f0 34 12 00 00[ 	]*vpaddw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 fd 72 7f[ 	]*vpaddw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 fd b2 00 20 00 00[ 	]*vpaddw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 fd 72 80[ 	]*vpaddw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 fd b2 c0 df ff ff[ 	]*vpaddw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 03 15 40 0f f4 ab[ 	]*vpalignr \$0xab,%zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 03 15 47 0f f4 ab[ 	]*vpalignr \$0xab,%zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 03 15 c7 0f f4 ab[ 	]*vpalignr \$0xab,%zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 03 15 40 0f f4 7b[ 	]*vpalignr \$0x7b,%zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 40 0f 31 7b[ 	]*vpalignr \$0x7b,\(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 23 15 40 0f b4 f0 34 12 00 00 7b[ 	]*vpalignr \$0x7b,0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 40 0f 72 7f 7b[ 	]*vpalignr \$0x7b,0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 40 0f b2 00 20 00 00 7b[ 	]*vpalignr \$0x7b,0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 40 0f 72 80 7b[ 	]*vpalignr \$0x7b,-0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 40 0f b2 c0 df ff ff 7b[ 	]*vpalignr \$0x7b,-0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 e0 f4[ 	]*vpavgb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 e0 f4[ 	]*vpavgb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 e0 f4[ 	]*vpavgb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e0 31[ 	]*vpavgb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 e0 b4 f0 34 12 00 00[ 	]*vpavgb 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e0 72 7f[ 	]*vpavgb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e0 b2 00 20 00 00[ 	]*vpavgb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e0 72 80[ 	]*vpavgb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e0 b2 c0 df ff ff[ 	]*vpavgb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 e3 f4[ 	]*vpavgw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 e3 f4[ 	]*vpavgw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 e3 f4[ 	]*vpavgw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e3 31[ 	]*vpavgw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 e3 b4 f0 34 12 00 00[ 	]*vpavgw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e3 72 7f[ 	]*vpavgw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e3 b2 00 20 00 00[ 	]*vpavgw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e3 72 80[ 	]*vpavgw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e3 b2 c0 df ff ff[ 	]*vpavgw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 40 66 f4[ 	]*vpblendmb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 47 66 f4[ 	]*vpblendmb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 15 c7 66 f4[ 	]*vpblendmb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 66 31[ 	]*vpblendmb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 15 40 66 b4 f0 34 12 00 00[ 	]*vpblendmb 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 66 72 7f[ 	]*vpblendmb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 66 b2 00 20 00 00[ 	]*vpblendmb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 66 72 80[ 	]*vpblendmb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 66 b2 c0 df ff ff[ 	]*vpblendmb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 48 78 f5[ 	]*vpbroadcastb %xmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 4f 78 f5[ 	]*vpbroadcastb %xmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7d cf 78 f5[ 	]*vpbroadcastb %xmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 78 31[ 	]*vpbroadcastb \(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 7d 48 78 b4 f0 34 12 00 00[ 	]*vpbroadcastb 0x1234\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 78 72 7f[ 	]*vpbroadcastb 0x7f\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 78 b2 80 00 00 00[ 	]*vpbroadcastb 0x80\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 78 72 80[ 	]*vpbroadcastb -0x80\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 78 b2 7f ff ff ff[ 	]*vpbroadcastb -0x81\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 7a f0[ 	]*vpbroadcastb %eax,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 4f 7a f0[ 	]*vpbroadcastb %eax,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 62 7d cf 7a f0[ 	]*vpbroadcastb %eax,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 48 79 f5[ 	]*vpbroadcastw %xmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 4f 79 f5[ 	]*vpbroadcastw %xmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7d cf 79 f5[ 	]*vpbroadcastw %xmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 79 31[ 	]*vpbroadcastw \(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 7d 48 79 b4 f0 34 12 00 00[ 	]*vpbroadcastw 0x1234\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 79 72 7f[ 	]*vpbroadcastw 0xfe\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 79 b2 00 01 00 00[ 	]*vpbroadcastw 0x100\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 79 72 80[ 	]*vpbroadcastw -0x100\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 79 b2 fe fe ff ff[ 	]*vpbroadcastw -0x102\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 7b f0[ 	]*vpbroadcastw %eax,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 4f 7b f0[ 	]*vpbroadcastw %eax,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 62 7d cf 7b f0[ 	]*vpbroadcastw %eax,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 74 ed[ 	]*vpcmpeqb %zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 47 74 ed[ 	]*vpcmpeqb %zmm29,%zmm30,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 74 29[ 	]*vpcmpeqb \(%rcx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b1 0d 40 74 ac f0 34 12 00 00[ 	]*vpcmpeqb 0x1234\(%rax,%r14,8\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 74 6a 7f[ 	]*vpcmpeqb 0x1fc0\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 74 aa 00 20 00 00[ 	]*vpcmpeqb 0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 74 6a 80[ 	]*vpcmpeqb -0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 74 aa c0 df ff ff[ 	]*vpcmpeqb -0x2040\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 75 ed[ 	]*vpcmpeqw %zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 47 75 ed[ 	]*vpcmpeqw %zmm29,%zmm30,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 75 29[ 	]*vpcmpeqw \(%rcx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b1 0d 40 75 ac f0 34 12 00 00[ 	]*vpcmpeqw 0x1234\(%rax,%r14,8\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 75 6a 7f[ 	]*vpcmpeqw 0x1fc0\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 75 aa 00 20 00 00[ 	]*vpcmpeqw 0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 75 6a 80[ 	]*vpcmpeqw -0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 75 aa c0 df ff ff[ 	]*vpcmpeqw -0x2040\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 64 ed[ 	]*vpcmpgtb %zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 47 64 ed[ 	]*vpcmpgtb %zmm29,%zmm30,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 64 29[ 	]*vpcmpgtb \(%rcx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b1 0d 40 64 ac f0 34 12 00 00[ 	]*vpcmpgtb 0x1234\(%rax,%r14,8\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 64 6a 7f[ 	]*vpcmpgtb 0x1fc0\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 64 aa 00 20 00 00[ 	]*vpcmpgtb 0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 64 6a 80[ 	]*vpcmpgtb -0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 64 aa c0 df ff ff[ 	]*vpcmpgtb -0x2040\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 65 ed[ 	]*vpcmpgtw %zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 47 65 ed[ 	]*vpcmpgtw %zmm29,%zmm30,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 65 29[ 	]*vpcmpgtw \(%rcx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b1 0d 40 65 ac f0 34 12 00 00[ 	]*vpcmpgtw 0x1234\(%rax,%r14,8\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 65 6a 7f[ 	]*vpcmpgtw 0x1fc0\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 65 aa 00 20 00 00[ 	]*vpcmpgtw 0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 65 6a 80[ 	]*vpcmpgtw -0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 65 aa c0 df ff ff[ 	]*vpcmpgtw -0x2040\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 02 95 40 66 f4[ 	]*vpblendmw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 47 66 f4[ 	]*vpblendmw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 95 c7 66 f4[ 	]*vpblendmw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 66 31[ 	]*vpblendmw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 95 40 66 b4 f0 34 12 00 00[ 	]*vpblendmw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 66 72 7f[ 	]*vpblendmw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 66 b2 00 20 00 00[ 	]*vpblendmw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 66 72 80[ 	]*vpblendmw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 66 b2 c0 df ff ff[ 	]*vpblendmw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 14 e8 ab[ 	]*vpextrb \$0xab,%xmm29,%eax
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 14 e8 7b[ 	]*vpextrb \$0x7b,%xmm29,%eax
[ 	]*[a-f0-9]+:[ 	]*62 43 7d 08 14 e8 7b[ 	]*vpextrb \$0x7b,%xmm29,%r8d
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 14 29 7b[ 	]*vpextrb \$0x7b,%xmm29,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*62 23 7d 08 14 ac f0 34 12 00 00 7b[ 	]*vpextrb \$0x7b,%xmm29,0x1234\(%rax,%r14,8\)
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 14 6a 7f 7b[ 	]*vpextrb \$0x7b,%xmm29,0x7f\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 14 aa 80 00 00 00 7b[ 	]*vpextrb \$0x7b,%xmm29,0x80\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 14 6a 80 7b[ 	]*vpextrb \$0x7b,%xmm29,-0x80\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 14 aa 7f ff ff ff 7b[ 	]*vpextrb \$0x7b,%xmm29,-0x81\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 15 29 7b[ 	]*vpextrw \$0x7b,%xmm29,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*62 23 7d 08 15 ac f0 34 12 00 00 7b[ 	]*vpextrw \$0x7b,%xmm29,0x1234\(%rax,%r14,8\)
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 15 6a 7f 7b[ 	]*vpextrw \$0x7b,%xmm29,0xfe\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 15 aa 00 01 00 00 7b[ 	]*vpextrw \$0x7b,%xmm29,0x100\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 15 6a 80 7b[ 	]*vpextrw \$0x7b,%xmm29,-0x100\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 63 7d 08 15 aa fe fe ff ff 7b[ 	]*vpextrw \$0x7b,%xmm29,-0x102\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 91 7d 08 c5 c6 ab[ 	]*vpextrw \$0xab,%xmm30,%eax
[ 	]*[a-f0-9]+:[ 	]*62 91 7d 08 c5 c6 7b[ 	]*vpextrw \$0x7b,%xmm30,%eax
[ 	]*[a-f0-9]+:[ 	]*62 11 7d 08 c5 c6 7b[ 	]*vpextrw \$0x7b,%xmm30,%r8d
[ 	]*[a-f0-9]+:[ 	]*62 63 15 00 20 f0 ab[ 	]*vpinsrb \$0xab,%eax,%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 00 20 f0 7b[ 	]*vpinsrb \$0x7b,%eax,%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 00 20 f5 7b[ 	]*vpinsrb \$0x7b,%ebp,%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 43 15 00 20 f5 7b[ 	]*vpinsrb \$0x7b,%r13d,%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 00 20 31 7b[ 	]*vpinsrb \$0x7b,\(%rcx\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 23 15 00 20 b4 f0 34 12 00 00 7b[ 	]*vpinsrb \$0x7b,0x1234\(%rax,%r14,8\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 00 20 72 7f 7b[ 	]*vpinsrb \$0x7b,0x7f\(%rdx\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 00 20 b2 80 00 00 00 7b[ 	]*vpinsrb \$0x7b,0x80\(%rdx\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 00 20 72 80 7b[ 	]*vpinsrb \$0x7b,-0x80\(%rdx\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 00 20 b2 7f ff ff ff 7b[ 	]*vpinsrb \$0x7b,-0x81\(%rdx\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 00 c4 f0 ab[ 	]*vpinsrw \$0xab,%eax,%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 00 c4 f0 7b[ 	]*vpinsrw \$0x7b,%eax,%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 00 c4 f5 7b[ 	]*vpinsrw \$0x7b,%ebp,%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 41 15 00 c4 f5 7b[ 	]*vpinsrw \$0x7b,%r13d,%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 00 c4 31 7b[ 	]*vpinsrw \$0x7b,\(%rcx\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 00 c4 b4 f0 34 12 00 00 7b[ 	]*vpinsrw \$0x7b,0x1234\(%rax,%r14,8\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 00 c4 72 7f 7b[ 	]*vpinsrw \$0x7b,0xfe\(%rdx\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 00 c4 b2 00 01 00 00 7b[ 	]*vpinsrw \$0x7b,0x100\(%rdx\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 00 c4 72 80 7b[ 	]*vpinsrw \$0x7b,-0x100\(%rdx\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 00 c4 b2 fe fe ff ff 7b[ 	]*vpinsrw \$0x7b,-0x102\(%rdx\),%xmm29,%xmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 40 04 f4[ 	]*vpmaddubsw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 47 04 f4[ 	]*vpmaddubsw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 15 c7 04 f4[ 	]*vpmaddubsw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 04 31[ 	]*vpmaddubsw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 15 40 04 b4 f0 34 12 00 00[ 	]*vpmaddubsw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 04 72 7f[ 	]*vpmaddubsw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 04 b2 00 20 00 00[ 	]*vpmaddubsw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 04 72 80[ 	]*vpmaddubsw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 04 b2 c0 df ff ff[ 	]*vpmaddubsw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 f5 f4[ 	]*vpmaddwd %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 f5 f4[ 	]*vpmaddwd %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 f5 f4[ 	]*vpmaddwd %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f5 31[ 	]*vpmaddwd \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 f5 b4 f0 34 12 00 00[ 	]*vpmaddwd 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f5 72 7f[ 	]*vpmaddwd 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f5 b2 00 20 00 00[ 	]*vpmaddwd 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f5 72 80[ 	]*vpmaddwd -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f5 b2 c0 df ff ff[ 	]*vpmaddwd -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 40 3c f4[ 	]*vpmaxsb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 47 3c f4[ 	]*vpmaxsb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 15 c7 3c f4[ 	]*vpmaxsb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3c 31[ 	]*vpmaxsb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 15 40 3c b4 f0 34 12 00 00[ 	]*vpmaxsb 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3c 72 7f[ 	]*vpmaxsb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3c b2 00 20 00 00[ 	]*vpmaxsb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3c 72 80[ 	]*vpmaxsb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3c b2 c0 df ff ff[ 	]*vpmaxsb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 ee f4[ 	]*vpmaxsw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 ee f4[ 	]*vpmaxsw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 ee f4[ 	]*vpmaxsw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ee 31[ 	]*vpmaxsw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 ee b4 f0 34 12 00 00[ 	]*vpmaxsw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ee 72 7f[ 	]*vpmaxsw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ee b2 00 20 00 00[ 	]*vpmaxsw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ee 72 80[ 	]*vpmaxsw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ee b2 c0 df ff ff[ 	]*vpmaxsw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 de f4[ 	]*vpmaxub %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 de f4[ 	]*vpmaxub %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 de f4[ 	]*vpmaxub %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 de 31[ 	]*vpmaxub \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 de b4 f0 34 12 00 00[ 	]*vpmaxub 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 de 72 7f[ 	]*vpmaxub 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 de b2 00 20 00 00[ 	]*vpmaxub 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 de 72 80[ 	]*vpmaxub -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 de b2 c0 df ff ff[ 	]*vpmaxub -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 40 3e f4[ 	]*vpmaxuw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 47 3e f4[ 	]*vpmaxuw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 15 c7 3e f4[ 	]*vpmaxuw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3e 31[ 	]*vpmaxuw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 15 40 3e b4 f0 34 12 00 00[ 	]*vpmaxuw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3e 72 7f[ 	]*vpmaxuw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3e b2 00 20 00 00[ 	]*vpmaxuw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3e 72 80[ 	]*vpmaxuw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3e b2 c0 df ff ff[ 	]*vpmaxuw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 40 38 f4[ 	]*vpminsb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 47 38 f4[ 	]*vpminsb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 15 c7 38 f4[ 	]*vpminsb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 38 31[ 	]*vpminsb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 15 40 38 b4 f0 34 12 00 00[ 	]*vpminsb 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 38 72 7f[ 	]*vpminsb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 38 b2 00 20 00 00[ 	]*vpminsb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 38 72 80[ 	]*vpminsb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 38 b2 c0 df ff ff[ 	]*vpminsb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 ea f4[ 	]*vpminsw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 ea f4[ 	]*vpminsw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 ea f4[ 	]*vpminsw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ea 31[ 	]*vpminsw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 ea b4 f0 34 12 00 00[ 	]*vpminsw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ea 72 7f[ 	]*vpminsw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ea b2 00 20 00 00[ 	]*vpminsw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ea 72 80[ 	]*vpminsw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 ea b2 c0 df ff ff[ 	]*vpminsw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 da f4[ 	]*vpminub %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 da f4[ 	]*vpminub %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 da f4[ 	]*vpminub %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 da 31[ 	]*vpminub \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 da b4 f0 34 12 00 00[ 	]*vpminub 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 da 72 7f[ 	]*vpminub 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 da b2 00 20 00 00[ 	]*vpminub 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 da 72 80[ 	]*vpminub -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 da b2 c0 df ff ff[ 	]*vpminub -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 40 3a f4[ 	]*vpminuw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 47 3a f4[ 	]*vpminuw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 15 c7 3a f4[ 	]*vpminuw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3a 31[ 	]*vpminuw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 15 40 3a b4 f0 34 12 00 00[ 	]*vpminuw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3a 72 7f[ 	]*vpminuw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3a b2 00 20 00 00[ 	]*vpminuw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3a 72 80[ 	]*vpminuw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 3a b2 c0 df ff ff[ 	]*vpminuw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 48 20 f5[ 	]*vpmovsxbw %ymm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 4f 20 f5[ 	]*vpmovsxbw %ymm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7d cf 20 f5[ 	]*vpmovsxbw %ymm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 20 31[ 	]*vpmovsxbw \(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 7d 48 20 b4 f0 34 12 00 00[ 	]*vpmovsxbw 0x1234\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 20 72 7f[ 	]*vpmovsxbw 0xfe0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 20 b2 00 10 00 00[ 	]*vpmovsxbw 0x1000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 20 72 80[ 	]*vpmovsxbw -0x1000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 20 b2 e0 ef ff ff[ 	]*vpmovsxbw -0x1020\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 48 30 f5[ 	]*vpmovzxbw %ymm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7d 4f 30 f5[ 	]*vpmovzxbw %ymm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7d cf 30 f5[ 	]*vpmovzxbw %ymm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 30 31[ 	]*vpmovzxbw \(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 7d 48 30 b4 f0 34 12 00 00[ 	]*vpmovzxbw 0x1234\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 30 72 7f[ 	]*vpmovzxbw 0xfe0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 30 b2 00 10 00 00[ 	]*vpmovzxbw 0x1000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 30 72 80[ 	]*vpmovzxbw -0x1000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 7d 48 30 b2 e0 ef ff ff[ 	]*vpmovzxbw -0x1020\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 40 0b f4[ 	]*vpmulhrsw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 47 0b f4[ 	]*vpmulhrsw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 15 c7 0b f4[ 	]*vpmulhrsw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 0b 31[ 	]*vpmulhrsw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 15 40 0b b4 f0 34 12 00 00[ 	]*vpmulhrsw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 0b 72 7f[ 	]*vpmulhrsw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 0b b2 00 20 00 00[ 	]*vpmulhrsw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 0b 72 80[ 	]*vpmulhrsw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 0b b2 c0 df ff ff[ 	]*vpmulhrsw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 e4 f4[ 	]*vpmulhuw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 e4 f4[ 	]*vpmulhuw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 e4 f4[ 	]*vpmulhuw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e4 31[ 	]*vpmulhuw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 e4 b4 f0 34 12 00 00[ 	]*vpmulhuw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e4 72 7f[ 	]*vpmulhuw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e4 b2 00 20 00 00[ 	]*vpmulhuw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e4 72 80[ 	]*vpmulhuw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e4 b2 c0 df ff ff[ 	]*vpmulhuw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 e5 f4[ 	]*vpmulhw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 e5 f4[ 	]*vpmulhw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 e5 f4[ 	]*vpmulhw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e5 31[ 	]*vpmulhw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 e5 b4 f0 34 12 00 00[ 	]*vpmulhw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e5 72 7f[ 	]*vpmulhw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e5 b2 00 20 00 00[ 	]*vpmulhw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e5 72 80[ 	]*vpmulhw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e5 b2 c0 df ff ff[ 	]*vpmulhw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 d5 f4[ 	]*vpmullw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 d5 f4[ 	]*vpmullw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 d5 f4[ 	]*vpmullw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d5 31[ 	]*vpmullw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 d5 b4 f0 34 12 00 00[ 	]*vpmullw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d5 72 7f[ 	]*vpmullw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d5 b2 00 20 00 00[ 	]*vpmullw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d5 72 80[ 	]*vpmullw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d5 b2 c0 df ff ff[ 	]*vpmullw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 f6 f4[ 	]*vpsadbw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f6 31[ 	]*vpsadbw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 f6 b4 f0 34 12 00 00[ 	]*vpsadbw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f6 72 7f[ 	]*vpsadbw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f6 b2 00 20 00 00[ 	]*vpsadbw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f6 72 80[ 	]*vpsadbw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f6 b2 c0 df ff ff[ 	]*vpsadbw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 40 00 f4[ 	]*vpshufb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 15 47 00 f4[ 	]*vpshufb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 15 c7 00 f4[ 	]*vpshufb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 00 31[ 	]*vpshufb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 15 40 00 b4 f0 34 12 00 00[ 	]*vpshufb 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 00 72 7f[ 	]*vpshufb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 00 b2 00 20 00 00[ 	]*vpshufb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 00 72 80[ 	]*vpshufb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 15 40 00 b2 c0 df ff ff[ 	]*vpshufb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 7e 48 70 f5 ab[ 	]*vpshufhw \$0xab,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 7e 4f 70 f5 ab[ 	]*vpshufhw \$0xab,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7e cf 70 f5 ab[ 	]*vpshufhw \$0xab,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7e 48 70 f5 7b[ 	]*vpshufhw \$0x7b,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7e 48 70 31 7b[ 	]*vpshufhw \$0x7b,\(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 7e 48 70 b4 f0 34 12 00 00 7b[ 	]*vpshufhw \$0x7b,0x1234\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7e 48 70 72 7f 7b[ 	]*vpshufhw \$0x7b,0x1fc0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7e 48 70 b2 00 20 00 00 7b[ 	]*vpshufhw \$0x7b,0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7e 48 70 72 80 7b[ 	]*vpshufhw \$0x7b,-0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7e 48 70 b2 c0 df ff ff 7b[ 	]*vpshufhw \$0x7b,-0x2040\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 48 70 f5 ab[ 	]*vpshuflw \$0xab,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 4f 70 f5 ab[ 	]*vpshuflw \$0xab,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7f cf 70 f5 ab[ 	]*vpshuflw \$0xab,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 48 70 f5 7b[ 	]*vpshuflw \$0x7b,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 70 31 7b[ 	]*vpshuflw \$0x7b,\(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 7f 48 70 b4 f0 34 12 00 00 7b[ 	]*vpshuflw \$0x7b,0x1234\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 70 72 7f 7b[ 	]*vpshuflw \$0x7b,0x1fc0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 70 b2 00 20 00 00 7b[ 	]*vpshuflw \$0x7b,0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 70 72 80 7b[ 	]*vpshuflw \$0x7b,-0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 70 b2 c0 df ff ff 7b[ 	]*vpshuflw \$0x7b,-0x2040\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 f1 f4[ 	]*vpsllw %xmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 f1 f4[ 	]*vpsllw %xmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 f1 f4[ 	]*vpsllw %xmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f1 31[ 	]*vpsllw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 f1 b4 f0 34 12 00 00[ 	]*vpsllw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f1 72 7f[ 	]*vpsllw 0x7f0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f1 b2 00 08 00 00[ 	]*vpsllw 0x800\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f1 72 80[ 	]*vpsllw -0x800\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f1 b2 f0 f7 ff ff[ 	]*vpsllw -0x810\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 e1 f4[ 	]*vpsraw %xmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 e1 f4[ 	]*vpsraw %xmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 e1 f4[ 	]*vpsraw %xmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e1 31[ 	]*vpsraw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 e1 b4 f0 34 12 00 00[ 	]*vpsraw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e1 72 7f[ 	]*vpsraw 0x7f0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e1 b2 00 08 00 00[ 	]*vpsraw 0x800\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e1 72 80[ 	]*vpsraw -0x800\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e1 b2 f0 f7 ff ff[ 	]*vpsraw -0x810\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 d1 f4[ 	]*vpsrlw %xmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 d1 f4[ 	]*vpsrlw %xmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 d1 f4[ 	]*vpsrlw %xmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d1 31[ 	]*vpsrlw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 d1 b4 f0 34 12 00 00[ 	]*vpsrlw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d1 72 7f[ 	]*vpsrlw 0x7f0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d1 b2 00 08 00 00[ 	]*vpsrlw 0x800\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d1 72 80[ 	]*vpsrlw -0x800\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d1 b2 f0 f7 ff ff[ 	]*vpsrlw -0x810\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 73 dd ab[ 	]*vpsrldq \$0xab,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 73 dd 7b[ 	]*vpsrldq \$0x7b,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 73 19 7b[ 	]*vpsrldq \$0x7b,\(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 b1 0d 40 73 9c f0 34 12 00 00 7b[ 	]*vpsrldq \$0x7b,0x1234\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 73 5a 7f 7b[ 	]*vpsrldq \$0x7b,0x1fc0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 73 9a 00 20 00 00 7b[ 	]*vpsrldq \$0x7b,0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 73 5a 80 7b[ 	]*vpsrldq \$0x7b,-0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 73 9a c0 df ff ff 7b[ 	]*vpsrldq \$0x7b,-0x2040\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 71 d5 ab[ 	]*vpsrlw \$0xab,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 47 71 d5 ab[ 	]*vpsrlw \$0xab,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 91 0d c7 71 d5 ab[ 	]*vpsrlw \$0xab,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 71 d5 7b[ 	]*vpsrlw \$0x7b,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 11 7b[ 	]*vpsrlw \$0x7b,\(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 b1 0d 40 71 94 f0 34 12 00 00 7b[ 	]*vpsrlw \$0x7b,0x1234\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 52 7f 7b[ 	]*vpsrlw \$0x7b,0x1fc0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 92 00 20 00 00 7b[ 	]*vpsrlw \$0x7b,0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 52 80 7b[ 	]*vpsrlw \$0x7b,-0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 92 c0 df ff ff 7b[ 	]*vpsrlw \$0x7b,-0x2040\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 71 e5 ab[ 	]*vpsraw \$0xab,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 47 71 e5 ab[ 	]*vpsraw \$0xab,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 91 0d c7 71 e5 ab[ 	]*vpsraw \$0xab,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 71 e5 7b[ 	]*vpsraw \$0x7b,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 21 7b[ 	]*vpsraw \$0x7b,\(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 b1 0d 40 71 a4 f0 34 12 00 00 7b[ 	]*vpsraw \$0x7b,0x1234\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 62 7f 7b[ 	]*vpsraw \$0x7b,0x1fc0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 a2 00 20 00 00 7b[ 	]*vpsraw \$0x7b,0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 62 80 7b[ 	]*vpsraw \$0x7b,-0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 a2 c0 df ff ff 7b[ 	]*vpsraw \$0x7b,-0x2040\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 40 10 f4[ 	]*vpsrlvw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 47 10 f4[ 	]*vpsrlvw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 95 c7 10 f4[ 	]*vpsrlvw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 10 31[ 	]*vpsrlvw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 95 40 10 b4 f0 34 12 00 00[ 	]*vpsrlvw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 10 72 7f[ 	]*vpsrlvw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 10 b2 00 20 00 00[ 	]*vpsrlvw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 10 72 80[ 	]*vpsrlvw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 10 b2 c0 df ff ff[ 	]*vpsrlvw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 40 11 f4[ 	]*vpsravw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 47 11 f4[ 	]*vpsravw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 95 c7 11 f4[ 	]*vpsravw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 11 31[ 	]*vpsravw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 95 40 11 b4 f0 34 12 00 00[ 	]*vpsravw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 11 72 7f[ 	]*vpsravw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 11 b2 00 20 00 00[ 	]*vpsravw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 11 72 80[ 	]*vpsravw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 11 b2 c0 df ff ff[ 	]*vpsravw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 f8 f4[ 	]*vpsubb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 f8 f4[ 	]*vpsubb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 f8 f4[ 	]*vpsubb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f8 31[ 	]*vpsubb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 f8 b4 f0 34 12 00 00[ 	]*vpsubb 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f8 72 7f[ 	]*vpsubb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f8 b2 00 20 00 00[ 	]*vpsubb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f8 72 80[ 	]*vpsubb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f8 b2 c0 df ff ff[ 	]*vpsubb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 e8 f4[ 	]*vpsubsb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 e8 f4[ 	]*vpsubsb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 e8 f4[ 	]*vpsubsb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e8 31[ 	]*vpsubsb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 e8 b4 f0 34 12 00 00[ 	]*vpsubsb 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e8 72 7f[ 	]*vpsubsb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e8 b2 00 20 00 00[ 	]*vpsubsb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e8 72 80[ 	]*vpsubsb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e8 b2 c0 df ff ff[ 	]*vpsubsb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 e9 f4[ 	]*vpsubsw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 e9 f4[ 	]*vpsubsw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 e9 f4[ 	]*vpsubsw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e9 31[ 	]*vpsubsw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 e9 b4 f0 34 12 00 00[ 	]*vpsubsw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e9 72 7f[ 	]*vpsubsw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e9 b2 00 20 00 00[ 	]*vpsubsw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e9 72 80[ 	]*vpsubsw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 e9 b2 c0 df ff ff[ 	]*vpsubsw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 d8 f4[ 	]*vpsubusb %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 d8 f4[ 	]*vpsubusb %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 d8 f4[ 	]*vpsubusb %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d8 31[ 	]*vpsubusb \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 d8 b4 f0 34 12 00 00[ 	]*vpsubusb 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d8 72 7f[ 	]*vpsubusb 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d8 b2 00 20 00 00[ 	]*vpsubusb 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d8 72 80[ 	]*vpsubusb -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d8 b2 c0 df ff ff[ 	]*vpsubusb -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 d9 f4[ 	]*vpsubusw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 d9 f4[ 	]*vpsubusw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 d9 f4[ 	]*vpsubusw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d9 31[ 	]*vpsubusw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 d9 b4 f0 34 12 00 00[ 	]*vpsubusw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d9 72 7f[ 	]*vpsubusw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d9 b2 00 20 00 00[ 	]*vpsubusw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d9 72 80[ 	]*vpsubusw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 d9 b2 c0 df ff ff[ 	]*vpsubusw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 f9 f4[ 	]*vpsubw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 f9 f4[ 	]*vpsubw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 f9 f4[ 	]*vpsubw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f9 31[ 	]*vpsubw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 f9 b4 f0 34 12 00 00[ 	]*vpsubw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f9 72 7f[ 	]*vpsubw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f9 b2 00 20 00 00[ 	]*vpsubw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f9 72 80[ 	]*vpsubw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 f9 b2 c0 df ff ff[ 	]*vpsubw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 68 f4[ 	]*vpunpckhbw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 68 f4[ 	]*vpunpckhbw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 68 f4[ 	]*vpunpckhbw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 68 31[ 	]*vpunpckhbw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 68 b4 f0 34 12 00 00[ 	]*vpunpckhbw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 68 72 7f[ 	]*vpunpckhbw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 68 b2 00 20 00 00[ 	]*vpunpckhbw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 68 72 80[ 	]*vpunpckhbw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 68 b2 c0 df ff ff[ 	]*vpunpckhbw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 69 f4[ 	]*vpunpckhwd %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 69 f4[ 	]*vpunpckhwd %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 69 f4[ 	]*vpunpckhwd %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 69 31[ 	]*vpunpckhwd \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 69 b4 f0 34 12 00 00[ 	]*vpunpckhwd 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 69 72 7f[ 	]*vpunpckhwd 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 69 b2 00 20 00 00[ 	]*vpunpckhwd 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 69 72 80[ 	]*vpunpckhwd -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 69 b2 c0 df ff ff[ 	]*vpunpckhwd -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 60 f4[ 	]*vpunpcklbw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 60 f4[ 	]*vpunpcklbw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 60 f4[ 	]*vpunpcklbw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 60 31[ 	]*vpunpcklbw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 60 b4 f0 34 12 00 00[ 	]*vpunpcklbw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 60 72 7f[ 	]*vpunpcklbw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 60 b2 00 20 00 00[ 	]*vpunpcklbw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 60 72 80[ 	]*vpunpcklbw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 60 b2 c0 df ff ff[ 	]*vpunpcklbw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 40 61 f4[ 	]*vpunpcklwd %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 15 47 61 f4[ 	]*vpunpcklwd %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 15 c7 61 f4[ 	]*vpunpcklwd %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 61 31[ 	]*vpunpcklwd \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 15 40 61 b4 f0 34 12 00 00[ 	]*vpunpcklwd 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 61 72 7f[ 	]*vpunpcklwd 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 61 b2 00 20 00 00[ 	]*vpunpcklwd 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 61 72 80[ 	]*vpunpcklwd -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 15 40 61 b2 c0 df ff ff[ 	]*vpunpcklwd -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7e 48 30 ee[ 	]*vpmovwb %zmm29,%ymm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7e 4f 30 ee[ 	]*vpmovwb %zmm29,%ymm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7e cf 30 ee[ 	]*vpmovwb %zmm29,%ymm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7e 48 20 ee[ 	]*vpmovswb %zmm29,%ymm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7e 4f 20 ee[ 	]*vpmovswb %zmm29,%ymm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7e cf 20 ee[ 	]*vpmovswb %zmm29,%ymm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7e 48 10 ee[ 	]*vpmovuswb %zmm29,%ymm30
[ 	]*[a-f0-9]+:[ 	]*62 02 7e 4f 10 ee[ 	]*vpmovuswb %zmm29,%ymm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 7e cf 10 ee[ 	]*vpmovuswb %zmm29,%ymm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 03 15 40 42 f4 ab[ 	]*vdbpsadbw \$0xab,%zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 03 15 47 42 f4 ab[ 	]*vdbpsadbw \$0xab,%zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 03 15 c7 42 f4 ab[ 	]*vdbpsadbw \$0xab,%zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 03 15 40 42 f4 7b[ 	]*vdbpsadbw \$0x7b,%zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 40 42 31 7b[ 	]*vdbpsadbw \$0x7b,\(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 23 15 40 42 b4 f0 34 12 00 00 7b[ 	]*vdbpsadbw \$0x7b,0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 40 42 72 7f 7b[ 	]*vdbpsadbw \$0x7b,0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 40 42 b2 00 20 00 00 7b[ 	]*vdbpsadbw \$0x7b,0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 40 42 72 80 7b[ 	]*vdbpsadbw \$0x7b,-0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 63 15 40 42 b2 c0 df ff ff 7b[ 	]*vdbpsadbw \$0x7b,-0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 40 8d f4[ 	]*vpermw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 47 8d f4[ 	]*vpermw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 95 c7 8d f4[ 	]*vpermw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 8d 31[ 	]*vpermw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 95 40 8d b4 f0 34 12 00 00[ 	]*vpermw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 8d 72 7f[ 	]*vpermw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 8d b2 00 20 00 00[ 	]*vpermw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 8d 72 80[ 	]*vpermw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 8d b2 c0 df ff ff[ 	]*vpermw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 40 7d f4[ 	]*vpermt2w %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 47 7d f4[ 	]*vpermt2w %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 95 c7 7d f4[ 	]*vpermt2w %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 7d 31[ 	]*vpermt2w \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 95 40 7d b4 f0 34 12 00 00[ 	]*vpermt2w 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 7d 72 7f[ 	]*vpermt2w 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 7d b2 00 20 00 00[ 	]*vpermt2w 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 7d 72 80[ 	]*vpermt2w -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 7d b2 c0 df ff ff[ 	]*vpermt2w -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 73 fd ab[ 	]*vpslldq \$0xab,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 73 fd 7b[ 	]*vpslldq \$0x7b,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 73 39 7b[ 	]*vpslldq \$0x7b,\(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 b1 0d 40 73 bc f0 34 12 00 00 7b[ 	]*vpslldq \$0x7b,0x1234\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 73 7a 7f 7b[ 	]*vpslldq \$0x7b,0x1fc0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 73 ba 00 20 00 00 7b[ 	]*vpslldq \$0x7b,0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 73 7a 80 7b[ 	]*vpslldq \$0x7b,-0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 73 ba c0 df ff ff 7b[ 	]*vpslldq \$0x7b,-0x2040\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 71 f5 ab[ 	]*vpsllw \$0xab,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 47 71 f5 ab[ 	]*vpsllw \$0xab,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 91 0d c7 71 f5 ab[ 	]*vpsllw \$0xab,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 91 0d 40 71 f5 7b[ 	]*vpsllw \$0x7b,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 31 7b[ 	]*vpsllw \$0x7b,\(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 b1 0d 40 71 b4 f0 34 12 00 00 7b[ 	]*vpsllw \$0x7b,0x1234\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 72 7f 7b[ 	]*vpsllw \$0x7b,0x1fc0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 b2 00 20 00 00 7b[ 	]*vpsllw \$0x7b,0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 72 80 7b[ 	]*vpsllw \$0x7b,-0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 f1 0d 40 71 b2 c0 df ff ff 7b[ 	]*vpsllw \$0x7b,-0x2040\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 40 12 f4[ 	]*vpsllvw %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 47 12 f4[ 	]*vpsllvw %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 95 c7 12 f4[ 	]*vpsllvw %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 12 31[ 	]*vpsllvw \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 95 40 12 b4 f0 34 12 00 00[ 	]*vpsllvw 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 12 72 7f[ 	]*vpsllvw 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 12 b2 00 20 00 00[ 	]*vpsllvw 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 12 72 80[ 	]*vpsllvw -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 12 b2 c0 df ff ff[ 	]*vpsllvw -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 48 6f f5[ 	]*vmovdqu8 %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 7f 4f 6f f5[ 	]*vmovdqu8 %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 7f cf 6f f5[ 	]*vmovdqu8 %zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 6f 31[ 	]*vmovdqu8 \(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 7f 48 6f b4 f0 34 12 00 00[ 	]*vmovdqu8 0x1234\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 6f 72 7f[ 	]*vmovdqu8 0x1fc0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 6f b2 00 20 00 00[ 	]*vmovdqu8 0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 6f 72 80[ 	]*vmovdqu8 -0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 6f b2 c0 df ff ff[ 	]*vmovdqu8 -0x2040\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 ff 48 6f f5[ 	]*vmovdqu16 %zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 01 ff 4f 6f f5[ 	]*vmovdqu16 %zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 01 ff cf 6f f5[ 	]*vmovdqu16 %zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 61 ff 48 6f 31[ 	]*vmovdqu16 \(%rcx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 21 ff 48 6f b4 f0 34 12 00 00[ 	]*vmovdqu16 0x1234\(%rax,%r14,8\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 ff 48 6f 72 7f[ 	]*vmovdqu16 0x1fc0\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 ff 48 6f b2 00 20 00 00[ 	]*vmovdqu16 0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 ff 48 6f 72 80[ 	]*vmovdqu16 -0x2000\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 61 ff 48 6f b2 c0 df ff ff[ 	]*vmovdqu16 -0x2040\(%rdx\),%zmm30
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cc 41 ef[ 	]*kandq  %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cd 41 ef[ 	]*kandd  %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cc 42 ef[ 	]*kandnq %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cd 42 ef[ 	]*kandnd %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cc 45 ef[ 	]*korq   %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cd 45 ef[ 	]*kord   %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cc 46 ef[ 	]*kxnorq %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cd 46 ef[ 	]*kxnord %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cc 47 ef[ 	]*kxorq  %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cd 47 ef[ 	]*kxord  %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f8 44 ee[ 	]*knotq  %k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f9 44 ee[ 	]*knotd  %k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f8 98 ee[ 	]*kortestq %k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f9 98 ee[ 	]*kortestd %k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f8 99 ee[ 	]*ktestq %k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f9 99 ee[ 	]*ktestd %k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e3 f9 31 ee ab[ 	]*kshiftrq \$0xab,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e3 f9 31 ee 7b[ 	]*kshiftrq \$0x7b,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e3 79 31 ee ab[ 	]*kshiftrd \$0xab,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e3 79 31 ee 7b[ 	]*kshiftrd \$0x7b,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e3 f9 33 ee ab[ 	]*kshiftlq \$0xab,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e3 f9 33 ee 7b[ 	]*kshiftlq \$0x7b,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e3 79 33 ee ab[ 	]*kshiftld \$0xab,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e3 79 33 ee 7b[ 	]*kshiftld \$0x7b,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f8 90 ee[ 	]*kmovq  %k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f8 90 29[ 	]*kmovq  \(%rcx\),%k5
[ 	]*[a-f0-9]+:[ 	]*c4 a1 f8 90 ac f0 34 12 00 00[ 	]*kmovq  0x1234\(%rax,%r14,8\),%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f9 90 ee[ 	]*kmovd  %k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f9 90 29[ 	]*kmovd  \(%rcx\),%k5
[ 	]*[a-f0-9]+:[ 	]*c4 a1 f9 90 ac f0 34 12 00 00[ 	]*kmovd  0x1234\(%rax,%r14,8\),%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f8 91 29[ 	]*kmovq  %k5,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*c4 a1 f8 91 ac f0 34 12 00 00[ 	]*kmovq  %k5,0x1234\(%rax,%r14,8\)
[ 	]*[a-f0-9]+:[ 	]*c4 e1 f9 91 29[ 	]*kmovd  %k5,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*c4 a1 f9 91 ac f0 34 12 00 00[ 	]*kmovd  %k5,0x1234\(%rax,%r14,8\)
[ 	]*[a-f0-9]+:[ 	]*c4 e1 fb 92 e8[ 	]*kmovq  %rax,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 c1 fb 92 e8[ 	]*kmovq  %r8,%k5
[ 	]*[a-f0-9]+:[ 	]*c5 fb 92 e8[ 	]*kmovd  %eax,%k5
[ 	]*[a-f0-9]+:[ 	]*c5 fb 92 ed[ 	]*kmovd  %ebp,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 c1 7b 92 ed[ 	]*kmovd  %r13d,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 fb 93 c5[ 	]*kmovq  %k5,%rax
[ 	]*[a-f0-9]+:[ 	]*c4 61 fb 93 c5[ 	]*kmovq  %k5,%r8
[ 	]*[a-f0-9]+:[ 	]*c5 fb 93 c5[ 	]*kmovd  %k5,%eax
[ 	]*[a-f0-9]+:[ 	]*c5 fb 93 ed[ 	]*kmovd  %k5,%ebp
[ 	]*[a-f0-9]+:[ 	]*c5 7b 93 ed[ 	]*kmovd  %k5,%r13d
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cc 4a ef[ 	]*kaddq  %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cd 4a ef[ 	]*kaddd  %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c5 cc 4b ef[ 	]*kunpckwd %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*c4 e1 cc 4b ef[ 	]*kunpckdq %k7,%k6,%k5
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 30 31[ 	]*vpmovwb %zmm30,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 4f 30 31[ 	]*vpmovwb %zmm30,\(%rcx\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 22 7e 48 30 b4 f0 34 12 00 00[ 	]*vpmovwb %zmm30,0x1234\(%rax,%r14,8\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 30 72 7f[ 	]*vpmovwb %zmm30,0xfe0\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 30 b2 00 10 00 00[ 	]*vpmovwb %zmm30,0x1000\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 30 72 80[ 	]*vpmovwb %zmm30,-0x1000\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 30 b2 e0 ef ff ff[ 	]*vpmovwb %zmm30,-0x1020\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 20 31[ 	]*vpmovswb %zmm30,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 4f 20 31[ 	]*vpmovswb %zmm30,\(%rcx\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 22 7e 48 20 b4 f0 34 12 00 00[ 	]*vpmovswb %zmm30,0x1234\(%rax,%r14,8\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 20 72 7f[ 	]*vpmovswb %zmm30,0xfe0\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 20 b2 00 10 00 00[ 	]*vpmovswb %zmm30,0x1000\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 20 72 80[ 	]*vpmovswb %zmm30,-0x1000\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 20 b2 e0 ef ff ff[ 	]*vpmovswb %zmm30,-0x1020\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 10 31[ 	]*vpmovuswb %zmm30,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 4f 10 31[ 	]*vpmovuswb %zmm30,\(%rcx\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 22 7e 48 10 b4 f0 34 12 00 00[ 	]*vpmovuswb %zmm30,0x1234\(%rax,%r14,8\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 10 72 7f[ 	]*vpmovuswb %zmm30,0xfe0\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 10 b2 00 10 00 00[ 	]*vpmovuswb %zmm30,0x1000\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 10 72 80[ 	]*vpmovuswb %zmm30,-0x1000\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 10 b2 e0 ef ff ff[ 	]*vpmovuswb %zmm30,-0x1020\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 7f 31[ 	]*vmovdqu8 %zmm30,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 4f 7f 31[ 	]*vmovdqu8 %zmm30,\(%rcx\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 21 7f 48 7f b4 f0 34 12 00 00[ 	]*vmovdqu8 %zmm30,0x1234\(%rax,%r14,8\)
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 7f 72 7f[ 	]*vmovdqu8 %zmm30,0x1fc0\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 7f b2 00 20 00 00[ 	]*vmovdqu8 %zmm30,0x2000\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 7f 72 80[ 	]*vmovdqu8 %zmm30,-0x2000\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 61 7f 48 7f b2 c0 df ff ff[ 	]*vmovdqu8 %zmm30,-0x2040\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 61 ff 48 7f 31[ 	]*vmovdqu16 %zmm30,\(%rcx\)
[ 	]*[a-f0-9]+:[ 	]*62 61 ff 4f 7f 31[ 	]*vmovdqu16 %zmm30,\(%rcx\)\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 21 ff 48 7f b4 f0 34 12 00 00[ 	]*vmovdqu16 %zmm30,0x1234\(%rax,%r14,8\)
[ 	]*[a-f0-9]+:[ 	]*62 61 ff 48 7f 72 7f[ 	]*vmovdqu16 %zmm30,0x1fc0\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 61 ff 48 7f b2 00 20 00 00[ 	]*vmovdqu16 %zmm30,0x2000\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 61 ff 48 7f 72 80[ 	]*vmovdqu16 %zmm30,-0x2000\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 61 ff 48 7f b2 c0 df ff ff[ 	]*vmovdqu16 %zmm30,-0x2040\(%rdx\)
[ 	]*[a-f0-9]+:[ 	]*62 02 95 40 75 f4[ 	]*vpermi2w %zmm28,%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 02 95 47 75 f4[ 	]*vpermi2w %zmm28,%zmm29,%zmm30\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 02 95 c7 75 f4[ 	]*vpermi2w %zmm28,%zmm29,%zmm30\{%k7\}\{z\}
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 75 31[ 	]*vpermi2w \(%rcx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 22 95 40 75 b4 f0 34 12 00 00[ 	]*vpermi2w 0x1234\(%rax,%r14,8\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 75 72 7f[ 	]*vpermi2w 0x1fc0\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 75 b2 00 20 00 00[ 	]*vpermi2w 0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 75 72 80[ 	]*vpermi2w -0x2000\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 95 40 75 b2 c0 df ff ff[ 	]*vpermi2w -0x2040\(%rdx\),%zmm29,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 92 0d 40 26 ed[ 	]*vptestmb %zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 92 0d 47 26 ed[ 	]*vptestmb %zmm29,%zmm30,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 0d 40 26 29[ 	]*vptestmb \(%rcx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b2 0d 40 26 ac f0 34 12 00 00[ 	]*vptestmb 0x1234\(%rax,%r14,8\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 0d 40 26 6a 7f[ 	]*vptestmb 0x1fc0\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 0d 40 26 aa 00 20 00 00[ 	]*vptestmb 0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 0d 40 26 6a 80[ 	]*vptestmb -0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 0d 40 26 aa c0 df ff ff[ 	]*vptestmb -0x2040\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 92 8d 40 26 ed[ 	]*vptestmw %zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 92 8d 47 26 ed[ 	]*vptestmw %zmm29,%zmm30,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 8d 40 26 29[ 	]*vptestmw \(%rcx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b2 8d 40 26 ac f0 34 12 00 00[ 	]*vptestmw 0x1234\(%rax,%r14,8\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 8d 40 26 6a 7f[ 	]*vptestmw 0x1fc0\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 8d 40 26 aa 00 20 00 00[ 	]*vptestmw 0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 8d 40 26 6a 80[ 	]*vptestmw -0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 8d 40 26 aa c0 df ff ff[ 	]*vptestmw -0x2040\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 92 7e 48 29 ee[ 	]*vpmovb2m %zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 92 fe 48 29 ee[ 	]*vpmovw2m %zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 62 7e 48 28 f5[ 	]*vpmovm2b %k5,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 62 fe 48 28 f5[ 	]*vpmovm2w %k5,%zmm30
[ 	]*[a-f0-9]+:[ 	]*62 92 16 40 26 ec[ 	]*vptestnmb %zmm28,%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 92 16 47 26 ec[ 	]*vptestnmb %zmm28,%zmm29,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 16 40 26 29[ 	]*vptestnmb \(%rcx\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b2 16 40 26 ac f0 34 12 00 00[ 	]*vptestnmb 0x1234\(%rax,%r14,8\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 16 40 26 6a 7f[ 	]*vptestnmb 0x1fc0\(%rdx\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 16 40 26 aa 00 20 00 00[ 	]*vptestnmb 0x2000\(%rdx\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 16 40 26 6a 80[ 	]*vptestnmb -0x2000\(%rdx\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 16 40 26 aa c0 df ff ff[ 	]*vptestnmb -0x2040\(%rdx\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 92 96 40 26 ec[ 	]*vptestnmw %zmm28,%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 92 96 47 26 ec[ 	]*vptestnmw %zmm28,%zmm29,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 f2 96 40 26 29[ 	]*vptestnmw \(%rcx\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b2 96 40 26 ac f0 34 12 00 00[ 	]*vptestnmw 0x1234\(%rax,%r14,8\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 96 40 26 6a 7f[ 	]*vptestnmw 0x1fc0\(%rdx\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 96 40 26 aa 00 20 00 00[ 	]*vptestnmw 0x2000\(%rdx\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 96 40 26 6a 80[ 	]*vptestnmw -0x2000\(%rdx\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f2 96 40 26 aa c0 df ff ff[ 	]*vptestnmw -0x2040\(%rdx\),%zmm29,%k5
[ 	]*[a-f0-9]+:[ 	]*62 93 0d 40 3f ed ab[ 	]*vpcmpb \$0xab,%zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 93 0d 47 3f ed ab[ 	]*vpcmpb \$0xab,%zmm29,%zmm30,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 93 0d 40 3f ed 7b[ 	]*vpcmpb \$0x7b,%zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 0d 40 3f 29 7b[ 	]*vpcmpb \$0x7b,\(%rcx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b3 0d 40 3f ac f0 34 12 00 00 7b[ 	]*vpcmpb \$0x7b,0x1234\(%rax,%r14,8\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 0d 40 3f 6a 7f 7b[ 	]*vpcmpb \$0x7b,0x1fc0\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 0d 40 3f aa 00 20 00 00 7b[ 	]*vpcmpb \$0x7b,0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 0d 40 3f 6a 80 7b[ 	]*vpcmpb \$0x7b,-0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 0d 40 3f aa c0 df ff ff 7b[ 	]*vpcmpb \$0x7b,-0x2040\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 93 8d 40 3f ed ab[ 	]*vpcmpw \$0xab,%zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 93 8d 47 3f ed ab[ 	]*vpcmpw \$0xab,%zmm29,%zmm30,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 93 8d 40 3f ed 7b[ 	]*vpcmpw \$0x7b,%zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 8d 40 3f 29 7b[ 	]*vpcmpw \$0x7b,\(%rcx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b3 8d 40 3f ac f0 34 12 00 00 7b[ 	]*vpcmpw \$0x7b,0x1234\(%rax,%r14,8\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 8d 40 3f 6a 7f 7b[ 	]*vpcmpw \$0x7b,0x1fc0\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 8d 40 3f aa 00 20 00 00 7b[ 	]*vpcmpw \$0x7b,0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 8d 40 3f 6a 80 7b[ 	]*vpcmpw \$0x7b,-0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 8d 40 3f aa c0 df ff ff 7b[ 	]*vpcmpw \$0x7b,-0x2040\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 93 0d 40 3e ed ab[ 	]*vpcmpub \$0xab,%zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 93 0d 47 3e ed ab[ 	]*vpcmpub \$0xab,%zmm29,%zmm30,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 93 0d 40 3e ed 7b[ 	]*vpcmpub \$0x7b,%zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 0d 40 3e 29 7b[ 	]*vpcmpub \$0x7b,\(%rcx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b3 0d 40 3e ac f0 34 12 00 00 7b[ 	]*vpcmpub \$0x7b,0x1234\(%rax,%r14,8\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 0d 40 3e 6a 7f 7b[ 	]*vpcmpub \$0x7b,0x1fc0\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 0d 40 3e aa 00 20 00 00 7b[ 	]*vpcmpub \$0x7b,0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 0d 40 3e 6a 80 7b[ 	]*vpcmpub \$0x7b,-0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 0d 40 3e aa c0 df ff ff 7b[ 	]*vpcmpub \$0x7b,-0x2040\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 93 8d 40 3e ed ab[ 	]*vpcmpuw \$0xab,%zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 93 8d 47 3e ed ab[ 	]*vpcmpuw \$0xab,%zmm29,%zmm30,%k5\{%k7\}
[ 	]*[a-f0-9]+:[ 	]*62 93 8d 40 3e ed 7b[ 	]*vpcmpuw \$0x7b,%zmm29,%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 8d 40 3e 29 7b[ 	]*vpcmpuw \$0x7b,\(%rcx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 b3 8d 40 3e ac f0 34 12 00 00 7b[ 	]*vpcmpuw \$0x7b,0x1234\(%rax,%r14,8\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 8d 40 3e 6a 7f 7b[ 	]*vpcmpuw \$0x7b,0x1fc0\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 8d 40 3e aa 00 20 00 00 7b[ 	]*vpcmpuw \$0x7b,0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 8d 40 3e 6a 80 7b[ 	]*vpcmpuw \$0x7b,-0x2000\(%rdx\),%zmm30,%k5
[ 	]*[a-f0-9]+:[ 	]*62 f3 8d 40 3e aa c0 df ff ff 7b[ 	]*vpcmpuw \$0x7b,-0x2040\(%rdx\),%zmm30,%k5
#pass
