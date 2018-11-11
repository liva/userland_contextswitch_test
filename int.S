.section .text
.globl handler

	// https://stackoverflow.com/questions/38006277/weird-behavior-setting-rip-with-ptrace
	nop
	nop
handler:
	incq number
	ret