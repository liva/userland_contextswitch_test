.section .text
.globl next_worker
.globl handler

	// https://stackoverflow.com/questions/38006277/weird-behavior-setting-rip-with-ptrace
	nop
	nop
handler:
	pushfq
	pushq %r15
	pushq %r14
	pushq %r13
	pushq %r12
	pushq %r11
	pushq %r10
	pushq %r9
	pushq %r8
	pushq %rdi
	pushq %rsi
	pushq %rbp
	pushq %rdx
	pushq %rcx
	pushq %rbx
	pushq %rax

	// swap context
	xchgq next_worker + 8, %rsp
	movq $return_from_handler, %rax
	xchgq next_worker, %rax
	jmp *%rax

return_from_handler:
	popq %rax	
	popq %rbx
	popq %rcx
	popq %rdx
	popq %rbp
	popq %rsi
	popq %rdi
	popq %r8
	popq %r9
	popq %r10
	popq %r11
	popq %r12
	popq %r13
	popq %r14
	popq %r15
	popfq
	ret