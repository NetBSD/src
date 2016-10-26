# Check illegal AVX instructions
	.text
_start:
	vcvtpd2dq (%ecx),%xmm2
	vcvtpd2ps (%ecx),%xmm2
	vcvttpd2dq (%ecx),%xmm2

	.intel_syntax noprefix
	vcvtpd2dq xmm2,[ecx]
	vcvtpd2ps xmm2,[ecx]
	vcvttpd2dq xmm2,[ecx]
