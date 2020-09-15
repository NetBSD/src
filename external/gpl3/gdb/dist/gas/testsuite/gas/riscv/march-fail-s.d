#as: -march=rv32isfoo
#objdump: -dr
#source: empty.s
#error_output: march-fail-s.l

.*:     file format elf32-littleriscv
