#objdump: -dw
#name: i386 ADX

.*: +file format .*


Disassembly of section .text:

0+ <_start>:
[       ]*[a-f0-9]+:	66 0f 38 f6 81 90 01 00 00 	adcx   0x190\(%ecx\),%eax
[       ]*[a-f0-9]+:	66 0f 38 f6 ca       	adcx   %edx,%ecx
[       ]*[a-f0-9]+:	66 0f 38 f6 94 f4 0f 04 f6 ff 	adcx   -0x9fbf1\(%esp,%esi,8\),%edx
[       ]*[a-f0-9]+:	66 0f 38 f6 00       	adcx   \(%eax\),%eax
[       ]*[a-f0-9]+:	66 0f 38 f6 ca       	adcx   %edx,%ecx
[       ]*[a-f0-9]+:	66 0f 38 f6 00       	adcx   \(%eax\),%eax
[       ]*[a-f0-9]+:	f3 0f 38 f6 81 90 01 00 00 	adox   0x190\(%ecx\),%eax
[       ]*[a-f0-9]+:	f3 0f 38 f6 ca       	adox   %edx,%ecx
[       ]*[a-f0-9]+:	f3 0f 38 f6 94 f4 0f 04 f6 ff 	adox   -0x9fbf1\(%esp,%esi,8\),%edx
[       ]*[a-f0-9]+:	f3 0f 38 f6 00       	adox   \(%eax\),%eax
[       ]*[a-f0-9]+:	f3 0f 38 f6 ca       	adox   %edx,%ecx
[       ]*[a-f0-9]+:	f3 0f 38 f6 00       	adox   \(%eax\),%eax
[       ]*[a-f0-9]+:	66 0f 38 f6 82 8f 01 00 00 	adcx   0x18f\(%edx\),%eax
[       ]*[a-f0-9]+:	66 0f 38 f6 d1       	adcx   %ecx,%edx
[       ]*[a-f0-9]+:	66 0f 38 f6 94 f4 c0 1d fe ff 	adcx   -0x1e240\(%esp,%esi,8\),%edx
[       ]*[a-f0-9]+:	66 0f 38 f6 00       	adcx   \(%eax\),%eax
[       ]*[a-f0-9]+:	f3 0f 38 f6 82 8f 01 00 00 	adox   0x18f\(%edx\),%eax
[       ]*[a-f0-9]+:	f3 0f 38 f6 d1       	adox   %ecx,%edx
[       ]*[a-f0-9]+:	f3 0f 38 f6 94 f4 c0 1d fe ff 	adox   -0x1e240\(%esp,%esi,8\),%edx
[       ]*[a-f0-9]+:	f3 0f 38 f6 00       	adox   \(%eax\),%eax
#pass
