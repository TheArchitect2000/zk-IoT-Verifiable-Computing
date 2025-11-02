	.arch armv8-a
	.file	"program_AND.cpp"
	.text
	.align	2
	.global	main
	.type	main, %function
main:
.LFB0:
	.cfi_startproc
#APP
// 16 "program_AND.cpp" 1
	mov x17, #3
mov x18, #8
mov x19, #15
mov x20, #2
mov x21, #6
and x17, x18, x19
and x18, x19, x20
and x19, x17, x21
and x20, x18, x21

// 0 "" 2
#NO_APP
	mov	w0, 0
	ret
	.cfi_endproc
.LFE0:
	.size	main, .-main
	.ident	"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0"
	.section	.note.GNU-stack,"",@progbits
