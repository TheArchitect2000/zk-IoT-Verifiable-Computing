	.arch armv8-a
	.file	"sample.cpp"
	.text
.Ltext0:
	.file 0 "/home/ubuntu/zk-IoT/assistedTrigger-ARM/Ver_2" "sample.cpp"
	.align	2
	.type	_ZL1fmmm, %function
_ZL1fmmm:
.LFB2:
	.file 1 "sample.cpp"
	.loc 1 4 55
	.cfi_startproc
	sub	sp, sp, #64
	.cfi_def_cfa_offset 64
	str	x0, [sp, 24]
	str	x1, [sp, 16]
	str	x2, [sp, 8]
	.loc 1 6 14
	ldr	x1, [sp, 24]
	ldr	x0, [sp, 16]
	add	x0, x1, x0
	str	x0, [sp, 40]
	.loc 1 7 14
	ldr	x1, [sp, 8]
	mov	x0, x1
	lsl	x0, x0, 1
	add	x0, x0, x1
	str	x0, [sp, 48]
	.loc 1 8 21
	ldr	x0, [sp, 40]
	and	x1, x0, 15
	.loc 1 8 33
	ldr	x0, [sp, 48]
	and	x0, x0, 3
	.loc 1 8 14
	orr	x0, x1, x0
	str	x0, [sp, 56]
	.loc 1 9 19
	ldr	x1, [sp, 48]
	ldr	x0, [sp, 24]
	sub	x1, x1, x0
	.loc 1 9 22
	ldr	x0, [sp, 56]
	add	x0, x1, x0
	.loc 1 10 1
	add	sp, sp, 64
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE2:
	.size	_ZL1fmmm, .-_ZL1fmmm
	.section	.rodata
	.align	3
.LC0:
	.string	"out=%llu\n"
	.text
	.align	2
	.global	main
	.type	main, %function
main:
.LFB3:
	.loc 1 12 12
	.cfi_startproc
	stp	x29, x30, [sp, -48]!
	.cfi_def_cfa_offset 48
	.cfi_offset 29, -48
	.cfi_offset 30, -40
	mov	x29, sp
	.loc 1 13 14
	mov	x0, 5
	str	x0, [sp, 16]
	.loc 1 13 21
	mov	x0, 7
	str	x0, [sp, 24]
	.loc 1 13 28
	mov	x0, 2
	str	x0, [sp, 32]
	.loc 1 14 21
	ldr	x2, [sp, 32]
	ldr	x1, [sp, 24]
	ldr	x0, [sp, 16]
	bl	_ZL1fmmm
	str	x0, [sp, 40]
	.loc 1 16 16
	ldr	x1, [sp, 40]
	adrp	x0, .LC0
	add	x0, x0, :lo12:.LC0
	bl	printf
	.loc 1 17 22
	ldr	x0, [sp, 40]
	.loc 1 17 28
	and	w0, w0, 255
	.loc 1 18 1
	ldp	x29, x30, [sp], 48
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE3:
	.size	main, .-main
.Letext0:
	.file 2 "/usr/include/aarch64-linux-gnu/bits/types.h"
	.file 3 "/usr/include/aarch64-linux-gnu/bits/stdint-intn.h"
	.file 4 "/usr/include/aarch64-linux-gnu/bits/stdint-uintn.h"
	.file 5 "/usr/include/aarch64-linux-gnu/bits/stdint-least.h"
	.file 6 "/usr/include/stdint.h"
	.file 7 "/usr/include/c++/13/cstdint"
	.file 8 "/usr/include/c++/13/cstdio"
	.file 9 "/usr/lib/gcc/aarch64-linux-gnu/13/include/stddef.h"
	.file 10 "/usr/lib/gcc/aarch64-linux-gnu/13/include/stdarg.h"
	.file 11 "/usr/include/aarch64-linux-gnu/bits/types/__mbstate_t.h"
	.file 12 "/usr/include/aarch64-linux-gnu/bits/types/__fpos_t.h"
	.file 13 "/usr/include/aarch64-linux-gnu/bits/types/struct_FILE.h"
	.file 14 "/usr/include/aarch64-linux-gnu/bits/types/FILE.h"
	.file 15 "/usr/include/stdio.h"
	.file 16 "/usr/include/aarch64-linux-gnu/c++/13/bits/c++config.h"
	.file 17 "<built-in>"
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.4byte	0xda8
	.2byte	0x5
	.byte	0x1
	.byte	0x8
	.4byte	.Ldebug_abbrev0
	.uleb128 0x15
	.4byte	.LASF161
	.byte	0x21
	.4byte	.LASF0
	.4byte	.LASF1
	.8byte	.Ltext0
	.8byte	.Letext0-.Ltext0
	.4byte	.Ldebug_line0
	.uleb128 0x7
	.byte	0x1
	.byte	0x8
	.4byte	.LASF2
	.uleb128 0x7
	.byte	0x2
	.byte	0x7
	.4byte	.LASF3
	.uleb128 0x7
	.byte	0x4
	.byte	0x7
	.4byte	.LASF4
	.uleb128 0x7
	.byte	0x8
	.byte	0x7
	.4byte	.LASF5
	.uleb128 0x3
	.4byte	.LASF7
	.byte	0x2
	.byte	0x25
	.byte	0x15
	.4byte	0x56
	.uleb128 0x7
	.byte	0x1
	.byte	0x6
	.4byte	.LASF6
	.uleb128 0x3
	.4byte	.LASF8
	.byte	0x2
	.byte	0x26
	.byte	0x17
	.4byte	0x2e
	.uleb128 0x3
	.4byte	.LASF9
	.byte	0x2
	.byte	0x27
	.byte	0x1a
	.4byte	0x75
	.uleb128 0x7
	.byte	0x2
	.byte	0x5
	.4byte	.LASF10
	.uleb128 0x3
	.4byte	.LASF11
	.byte	0x2
	.byte	0x28
	.byte	0x1c
	.4byte	0x35
	.uleb128 0x3
	.4byte	.LASF12
	.byte	0x2
	.byte	0x29
	.byte	0x14
	.4byte	0x94
	.uleb128 0x16
	.byte	0x4
	.byte	0x5
	.string	"int"
	.uleb128 0x3
	.4byte	.LASF13
	.byte	0x2
	.byte	0x2a
	.byte	0x16
	.4byte	0x3c
	.uleb128 0x3
	.4byte	.LASF14
	.byte	0x2
	.byte	0x2c
	.byte	0x19
	.4byte	0xb3
	.uleb128 0x7
	.byte	0x8
	.byte	0x5
	.4byte	.LASF15
	.uleb128 0x3
	.4byte	.LASF16
	.byte	0x2
	.byte	0x2d
	.byte	0x1b
	.4byte	0x43
	.uleb128 0x3
	.4byte	.LASF17
	.byte	0x2
	.byte	0x34
	.byte	0x12
	.4byte	0x4a
	.uleb128 0x3
	.4byte	.LASF18
	.byte	0x2
	.byte	0x35
	.byte	0x13
	.4byte	0x5d
	.uleb128 0x3
	.4byte	.LASF19
	.byte	0x2
	.byte	0x36
	.byte	0x13
	.4byte	0x69
	.uleb128 0x3
	.4byte	.LASF20
	.byte	0x2
	.byte	0x37
	.byte	0x14
	.4byte	0x7c
	.uleb128 0x3
	.4byte	.LASF21
	.byte	0x2
	.byte	0x38
	.byte	0x13
	.4byte	0x88
	.uleb128 0x3
	.4byte	.LASF22
	.byte	0x2
	.byte	0x39
	.byte	0x14
	.4byte	0x9b
	.uleb128 0x3
	.4byte	.LASF23
	.byte	0x2
	.byte	0x3a
	.byte	0x13
	.4byte	0xa7
	.uleb128 0x3
	.4byte	.LASF24
	.byte	0x2
	.byte	0x3b
	.byte	0x14
	.4byte	0xba
	.uleb128 0x3
	.4byte	.LASF25
	.byte	0x2
	.byte	0x48
	.byte	0x12
	.4byte	0xb3
	.uleb128 0x3
	.4byte	.LASF26
	.byte	0x2
	.byte	0x49
	.byte	0x1b
	.4byte	0x43
	.uleb128 0x3
	.4byte	.LASF27
	.byte	0x2
	.byte	0x98
	.byte	0x19
	.4byte	0xb3
	.uleb128 0x3
	.4byte	.LASF28
	.byte	0x2
	.byte	0x99
	.byte	0x1b
	.4byte	0xb3
	.uleb128 0x17
	.byte	0x8
	.uleb128 0x6
	.4byte	0x15d
	.uleb128 0x7
	.byte	0x1
	.byte	0x8
	.4byte	.LASF29
	.uleb128 0x12
	.4byte	0x15d
	.uleb128 0x3
	.4byte	.LASF30
	.byte	0x3
	.byte	0x18
	.byte	0x12
	.4byte	0x4a
	.uleb128 0x3
	.4byte	.LASF31
	.byte	0x3
	.byte	0x19
	.byte	0x13
	.4byte	0x69
	.uleb128 0x3
	.4byte	.LASF32
	.byte	0x3
	.byte	0x1a
	.byte	0x13
	.4byte	0x88
	.uleb128 0x3
	.4byte	.LASF33
	.byte	0x3
	.byte	0x1b
	.byte	0x13
	.4byte	0xa7
	.uleb128 0x3
	.4byte	.LASF34
	.byte	0x4
	.byte	0x18
	.byte	0x13
	.4byte	0x5d
	.uleb128 0x3
	.4byte	.LASF35
	.byte	0x4
	.byte	0x19
	.byte	0x14
	.4byte	0x7c
	.uleb128 0x3
	.4byte	.LASF36
	.byte	0x4
	.byte	0x1a
	.byte	0x14
	.4byte	0x9b
	.uleb128 0x3
	.4byte	.LASF37
	.byte	0x4
	.byte	0x1b
	.byte	0x14
	.4byte	0xba
	.uleb128 0x3
	.4byte	.LASF38
	.byte	0x5
	.byte	0x19
	.byte	0x18
	.4byte	0xc6
	.uleb128 0x3
	.4byte	.LASF39
	.byte	0x5
	.byte	0x1a
	.byte	0x19
	.4byte	0xde
	.uleb128 0x3
	.4byte	.LASF40
	.byte	0x5
	.byte	0x1b
	.byte	0x19
	.4byte	0xf6
	.uleb128 0x3
	.4byte	.LASF41
	.byte	0x5
	.byte	0x1c
	.byte	0x19
	.4byte	0x10e
	.uleb128 0x3
	.4byte	.LASF42
	.byte	0x5
	.byte	0x1f
	.byte	0x19
	.4byte	0xd2
	.uleb128 0x3
	.4byte	.LASF43
	.byte	0x5
	.byte	0x20
	.byte	0x1a
	.4byte	0xea
	.uleb128 0x3
	.4byte	.LASF44
	.byte	0x5
	.byte	0x21
	.byte	0x1a
	.4byte	0x102
	.uleb128 0x3
	.4byte	.LASF45
	.byte	0x5
	.byte	0x22
	.byte	0x1a
	.4byte	0x11a
	.uleb128 0x3
	.4byte	.LASF46
	.byte	0x6
	.byte	0x2f
	.byte	0x16
	.4byte	0x56
	.uleb128 0x3
	.4byte	.LASF47
	.byte	0x6
	.byte	0x31
	.byte	0x13
	.4byte	0xb3
	.uleb128 0x3
	.4byte	.LASF48
	.byte	0x6
	.byte	0x32
	.byte	0x13
	.4byte	0xb3
	.uleb128 0x3
	.4byte	.LASF49
	.byte	0x6
	.byte	0x33
	.byte	0x13
	.4byte	0xb3
	.uleb128 0x3
	.4byte	.LASF50
	.byte	0x6
	.byte	0x3c
	.byte	0x18
	.4byte	0x2e
	.uleb128 0x3
	.4byte	.LASF51
	.byte	0x6
	.byte	0x3e
	.byte	0x1b
	.4byte	0x43
	.uleb128 0x3
	.4byte	.LASF52
	.byte	0x6
	.byte	0x3f
	.byte	0x1b
	.4byte	0x43
	.uleb128 0x3
	.4byte	.LASF53
	.byte	0x6
	.byte	0x40
	.byte	0x1b
	.4byte	0x43
	.uleb128 0x3
	.4byte	.LASF54
	.byte	0x6
	.byte	0x4c
	.byte	0x13
	.4byte	0xb3
	.uleb128 0x3
	.4byte	.LASF55
	.byte	0x6
	.byte	0x4f
	.byte	0x1b
	.4byte	0x43
	.uleb128 0x3
	.4byte	.LASF56
	.byte	0x6
	.byte	0x5a
	.byte	0x15
	.4byte	0x126
	.uleb128 0x3
	.4byte	.LASF57
	.byte	0x6
	.byte	0x5b
	.byte	0x16
	.4byte	0x132
	.uleb128 0x18
	.string	"std"
	.byte	0x10
	.2byte	0x132
	.byte	0xb
	.4byte	0x51f
	.uleb128 0x2
	.byte	0x7
	.byte	0x33
	.byte	0xb
	.4byte	0x169
	.uleb128 0x2
	.byte	0x7
	.byte	0x34
	.byte	0xb
	.4byte	0x175
	.uleb128 0x2
	.byte	0x7
	.byte	0x35
	.byte	0xb
	.4byte	0x181
	.uleb128 0x2
	.byte	0x7
	.byte	0x36
	.byte	0xb
	.4byte	0x18d
	.uleb128 0x2
	.byte	0x7
	.byte	0x38
	.byte	0xb
	.4byte	0x229
	.uleb128 0x2
	.byte	0x7
	.byte	0x39
	.byte	0xb
	.4byte	0x235
	.uleb128 0x2
	.byte	0x7
	.byte	0x3a
	.byte	0xb
	.4byte	0x241
	.uleb128 0x2
	.byte	0x7
	.byte	0x3b
	.byte	0xb
	.4byte	0x24d
	.uleb128 0x2
	.byte	0x7
	.byte	0x3d
	.byte	0xb
	.4byte	0x1c9
	.uleb128 0x2
	.byte	0x7
	.byte	0x3e
	.byte	0xb
	.4byte	0x1d5
	.uleb128 0x2
	.byte	0x7
	.byte	0x3f
	.byte	0xb
	.4byte	0x1e1
	.uleb128 0x2
	.byte	0x7
	.byte	0x40
	.byte	0xb
	.4byte	0x1ed
	.uleb128 0x2
	.byte	0x7
	.byte	0x42
	.byte	0xb
	.4byte	0x2a1
	.uleb128 0x2
	.byte	0x7
	.byte	0x43
	.byte	0xb
	.4byte	0x289
	.uleb128 0x2
	.byte	0x7
	.byte	0x45
	.byte	0xb
	.4byte	0x199
	.uleb128 0x2
	.byte	0x7
	.byte	0x46
	.byte	0xb
	.4byte	0x1a5
	.uleb128 0x2
	.byte	0x7
	.byte	0x47
	.byte	0xb
	.4byte	0x1b1
	.uleb128 0x2
	.byte	0x7
	.byte	0x48
	.byte	0xb
	.4byte	0x1bd
	.uleb128 0x2
	.byte	0x7
	.byte	0x4a
	.byte	0xb
	.4byte	0x259
	.uleb128 0x2
	.byte	0x7
	.byte	0x4b
	.byte	0xb
	.4byte	0x265
	.uleb128 0x2
	.byte	0x7
	.byte	0x4c
	.byte	0xb
	.4byte	0x271
	.uleb128 0x2
	.byte	0x7
	.byte	0x4d
	.byte	0xb
	.4byte	0x27d
	.uleb128 0x2
	.byte	0x7
	.byte	0x4f
	.byte	0xb
	.4byte	0x1f9
	.uleb128 0x2
	.byte	0x7
	.byte	0x50
	.byte	0xb
	.4byte	0x205
	.uleb128 0x2
	.byte	0x7
	.byte	0x51
	.byte	0xb
	.4byte	0x211
	.uleb128 0x2
	.byte	0x7
	.byte	0x52
	.byte	0xb
	.4byte	0x21d
	.uleb128 0x2
	.byte	0x7
	.byte	0x54
	.byte	0xb
	.4byte	0x2ad
	.uleb128 0x2
	.byte	0x7
	.byte	0x55
	.byte	0xb
	.4byte	0x295
	.uleb128 0x2
	.byte	0x8
	.byte	0x62
	.byte	0xb
	.4byte	0x795
	.uleb128 0x2
	.byte	0x8
	.byte	0x63
	.byte	0xb
	.4byte	0x7f6
	.uleb128 0x2
	.byte	0x8
	.byte	0x65
	.byte	0xb
	.4byte	0x80c
	.uleb128 0x2
	.byte	0x8
	.byte	0x66
	.byte	0xb
	.4byte	0x81d
	.uleb128 0x2
	.byte	0x8
	.byte	0x67
	.byte	0xb
	.4byte	0x832
	.uleb128 0x2
	.byte	0x8
	.byte	0x68
	.byte	0xb
	.4byte	0x848
	.uleb128 0x2
	.byte	0x8
	.byte	0x69
	.byte	0xb
	.4byte	0x85e
	.uleb128 0x2
	.byte	0x8
	.byte	0x6a
	.byte	0xb
	.4byte	0x873
	.uleb128 0x2
	.byte	0x8
	.byte	0x6b
	.byte	0xb
	.4byte	0x889
	.uleb128 0x2
	.byte	0x8
	.byte	0x6c
	.byte	0xb
	.4byte	0x8a9
	.uleb128 0x2
	.byte	0x8
	.byte	0x6d
	.byte	0xb
	.4byte	0x8c9
	.uleb128 0x2
	.byte	0x8
	.byte	0x6e
	.byte	0xb
	.4byte	0x8e4
	.uleb128 0x2
	.byte	0x8
	.byte	0x6f
	.byte	0xb
	.4byte	0x900
	.uleb128 0x2
	.byte	0x8
	.byte	0x70
	.byte	0xb
	.4byte	0x91b
	.uleb128 0x2
	.byte	0x8
	.byte	0x71
	.byte	0xb
	.4byte	0x936
	.uleb128 0x2
	.byte	0x8
	.byte	0x72
	.byte	0xb
	.4byte	0x95b
	.uleb128 0x2
	.byte	0x8
	.byte	0x73
	.byte	0xb
	.4byte	0x97b
	.uleb128 0x2
	.byte	0x8
	.byte	0x74
	.byte	0xb
	.4byte	0x99a
	.uleb128 0x2
	.byte	0x8
	.byte	0x75
	.byte	0xb
	.4byte	0x9ba
	.uleb128 0x2
	.byte	0x8
	.byte	0x76
	.byte	0xb
	.4byte	0x9da
	.uleb128 0x2
	.byte	0x8
	.byte	0x77
	.byte	0xb
	.4byte	0x9f0
	.uleb128 0x2
	.byte	0x8
	.byte	0x78
	.byte	0xb
	.4byte	0xa1b
	.uleb128 0x2
	.byte	0x8
	.byte	0x79
	.byte	0xb
	.4byte	0xa31
	.uleb128 0x2
	.byte	0x8
	.byte	0x7e
	.byte	0xb
	.4byte	0xa3e
	.uleb128 0x2
	.byte	0x8
	.byte	0x7f
	.byte	0xb
	.4byte	0xa4f
	.uleb128 0x2
	.byte	0x8
	.byte	0x80
	.byte	0xb
	.4byte	0xa66
	.uleb128 0x2
	.byte	0x8
	.byte	0x81
	.byte	0xb
	.4byte	0xa81
	.uleb128 0x2
	.byte	0x8
	.byte	0x82
	.byte	0xb
	.4byte	0xa97
	.uleb128 0x2
	.byte	0x8
	.byte	0x83
	.byte	0xb
	.4byte	0xaad
	.uleb128 0x2
	.byte	0x8
	.byte	0x84
	.byte	0xb
	.4byte	0xac2
	.uleb128 0x2
	.byte	0x8
	.byte	0x85
	.byte	0xb
	.4byte	0xadc
	.uleb128 0x2
	.byte	0x8
	.byte	0x86
	.byte	0xb
	.4byte	0xaed
	.uleb128 0x2
	.byte	0x8
	.byte	0x87
	.byte	0xb
	.4byte	0xb07
	.uleb128 0x2
	.byte	0x8
	.byte	0x88
	.byte	0xb
	.4byte	0xb1d
	.uleb128 0x2
	.byte	0x8
	.byte	0x89
	.byte	0xb
	.4byte	0xb42
	.uleb128 0x2
	.byte	0x8
	.byte	0x8a
	.byte	0xb
	.4byte	0xb5e
	.uleb128 0x2
	.byte	0x8
	.byte	0x8b
	.byte	0xb
	.4byte	0xb7d
	.uleb128 0x2
	.byte	0x8
	.byte	0x8d
	.byte	0xb
	.4byte	0xb89
	.uleb128 0x2
	.byte	0x8
	.byte	0x8f
	.byte	0xb
	.4byte	0xb9e
	.uleb128 0x2
	.byte	0x8
	.byte	0x90
	.byte	0xb
	.4byte	0xbb9
	.uleb128 0x2
	.byte	0x8
	.byte	0x91
	.byte	0xb
	.4byte	0xbd9
	.uleb128 0x2
	.byte	0x8
	.byte	0x92
	.byte	0xb
	.4byte	0xbf4
	.uleb128 0x2
	.byte	0x8
	.byte	0xb9
	.byte	0x16
	.4byte	0xc4a
	.uleb128 0x2
	.byte	0x8
	.byte	0xba
	.byte	0x16
	.4byte	0xc6b
	.uleb128 0x2
	.byte	0x8
	.byte	0xbb
	.byte	0x16
	.4byte	0xc8e
	.uleb128 0x2
	.byte	0x8
	.byte	0xbc
	.byte	0x16
	.4byte	0xcac
	.uleb128 0x2
	.byte	0x8
	.byte	0xbd
	.byte	0x16
	.4byte	0xcd1
	.byte	0
	.uleb128 0x3
	.4byte	.LASF58
	.byte	0x9
	.byte	0xd6
	.byte	0x17
	.4byte	0x43
	.uleb128 0x3
	.4byte	.LASF59
	.byte	0xa
	.byte	0x28
	.byte	0x1b
	.4byte	0x537
	.uleb128 0x19
	.4byte	.LASF162
	.byte	0x20
	.byte	0x11
	.byte	0
	.4byte	0x576
	.uleb128 0xb
	.4byte	.LASF60
	.4byte	0x156
	.byte	0
	.uleb128 0xb
	.4byte	.LASF61
	.4byte	0x156
	.byte	0x8
	.uleb128 0xb
	.4byte	.LASF62
	.4byte	0x156
	.byte	0x10
	.uleb128 0xb
	.4byte	.LASF63
	.4byte	0x94
	.byte	0x18
	.uleb128 0xb
	.4byte	.LASF64
	.4byte	0x94
	.byte	0x1c
	.byte	0
	.uleb128 0x1a
	.byte	0x8
	.byte	0xb
	.byte	0xe
	.byte	0x1
	.4byte	.LASF163
	.4byte	0x5be
	.uleb128 0x1b
	.byte	0x4
	.byte	0xb
	.byte	0x11
	.byte	0x3
	.4byte	0x5a3
	.uleb128 0x13
	.4byte	.LASF65
	.byte	0x12
	.byte	0x13
	.4byte	0x3c
	.uleb128 0x13
	.4byte	.LASF66
	.byte	0x13
	.byte	0xa
	.4byte	0x5be
	.byte	0
	.uleb128 0x4
	.4byte	.LASF67
	.byte	0xb
	.byte	0xf
	.byte	0x7
	.4byte	0x94
	.byte	0
	.uleb128 0x4
	.4byte	.LASF68
	.byte	0xb
	.byte	0x14
	.byte	0x5
	.4byte	0x583
	.byte	0x4
	.byte	0
	.uleb128 0xe
	.4byte	0x15d
	.4byte	0x5ce
	.uleb128 0xf
	.4byte	0x43
	.byte	0x3
	.byte	0
	.uleb128 0x3
	.4byte	.LASF69
	.byte	0xb
	.byte	0x15
	.byte	0x3
	.4byte	0x576
	.uleb128 0x14
	.4byte	.LASF73
	.byte	0x10
	.byte	0xc
	.byte	0xa
	.byte	0x10
	.4byte	0x602
	.uleb128 0x4
	.4byte	.LASF70
	.byte	0xc
	.byte	0xc
	.byte	0xb
	.4byte	0x13e
	.byte	0
	.uleb128 0x4
	.4byte	.LASF71
	.byte	0xc
	.byte	0xd
	.byte	0xf
	.4byte	0x5ce
	.byte	0x8
	.byte	0
	.uleb128 0x3
	.4byte	.LASF72
	.byte	0xc
	.byte	0xe
	.byte	0x3
	.4byte	0x5da
	.uleb128 0x14
	.4byte	.LASF74
	.byte	0xd8
	.byte	0xd
	.byte	0x31
	.byte	0x8
	.4byte	0x795
	.uleb128 0x4
	.4byte	.LASF75
	.byte	0xd
	.byte	0x33
	.byte	0x7
	.4byte	0x94
	.byte	0
	.uleb128 0x4
	.4byte	.LASF76
	.byte	0xd
	.byte	0x36
	.byte	0x9
	.4byte	0x158
	.byte	0x8
	.uleb128 0x4
	.4byte	.LASF77
	.byte	0xd
	.byte	0x37
	.byte	0x9
	.4byte	0x158
	.byte	0x10
	.uleb128 0x4
	.4byte	.LASF78
	.byte	0xd
	.byte	0x38
	.byte	0x9
	.4byte	0x158
	.byte	0x18
	.uleb128 0x4
	.4byte	.LASF79
	.byte	0xd
	.byte	0x39
	.byte	0x9
	.4byte	0x158
	.byte	0x20
	.uleb128 0x4
	.4byte	.LASF80
	.byte	0xd
	.byte	0x3a
	.byte	0x9
	.4byte	0x158
	.byte	0x28
	.uleb128 0x4
	.4byte	.LASF81
	.byte	0xd
	.byte	0x3b
	.byte	0x9
	.4byte	0x158
	.byte	0x30
	.uleb128 0x4
	.4byte	.LASF82
	.byte	0xd
	.byte	0x3c
	.byte	0x9
	.4byte	0x158
	.byte	0x38
	.uleb128 0x4
	.4byte	.LASF83
	.byte	0xd
	.byte	0x3d
	.byte	0x9
	.4byte	0x158
	.byte	0x40
	.uleb128 0x4
	.4byte	.LASF84
	.byte	0xd
	.byte	0x40
	.byte	0x9
	.4byte	0x158
	.byte	0x48
	.uleb128 0x4
	.4byte	.LASF85
	.byte	0xd
	.byte	0x41
	.byte	0x9
	.4byte	0x158
	.byte	0x50
	.uleb128 0x4
	.4byte	.LASF86
	.byte	0xd
	.byte	0x42
	.byte	0x9
	.4byte	0x158
	.byte	0x58
	.uleb128 0x4
	.4byte	.LASF87
	.byte	0xd
	.byte	0x44
	.byte	0x16
	.4byte	0x7ae
	.byte	0x60
	.uleb128 0x4
	.4byte	.LASF88
	.byte	0xd
	.byte	0x46
	.byte	0x14
	.4byte	0x7b3
	.byte	0x68
	.uleb128 0x4
	.4byte	.LASF89
	.byte	0xd
	.byte	0x48
	.byte	0x7
	.4byte	0x94
	.byte	0x70
	.uleb128 0x4
	.4byte	.LASF90
	.byte	0xd
	.byte	0x49
	.byte	0x7
	.4byte	0x94
	.byte	0x74
	.uleb128 0x4
	.4byte	.LASF91
	.byte	0xd
	.byte	0x4a
	.byte	0xb
	.4byte	0x13e
	.byte	0x78
	.uleb128 0x4
	.4byte	.LASF92
	.byte	0xd
	.byte	0x4d
	.byte	0x12
	.4byte	0x35
	.byte	0x80
	.uleb128 0x4
	.4byte	.LASF93
	.byte	0xd
	.byte	0x4e
	.byte	0xf
	.4byte	0x56
	.byte	0x82
	.uleb128 0x4
	.4byte	.LASF94
	.byte	0xd
	.byte	0x4f
	.byte	0x8
	.4byte	0x7b8
	.byte	0x83
	.uleb128 0x4
	.4byte	.LASF95
	.byte	0xd
	.byte	0x51
	.byte	0xf
	.4byte	0x7c8
	.byte	0x88
	.uleb128 0x4
	.4byte	.LASF96
	.byte	0xd
	.byte	0x59
	.byte	0xd
	.4byte	0x14a
	.byte	0x90
	.uleb128 0x4
	.4byte	.LASF97
	.byte	0xd
	.byte	0x5b
	.byte	0x17
	.4byte	0x7d2
	.byte	0x98
	.uleb128 0x4
	.4byte	.LASF98
	.byte	0xd
	.byte	0x5c
	.byte	0x19
	.4byte	0x7dc
	.byte	0xa0
	.uleb128 0x4
	.4byte	.LASF99
	.byte	0xd
	.byte	0x5d
	.byte	0x14
	.4byte	0x7b3
	.byte	0xa8
	.uleb128 0x4
	.4byte	.LASF100
	.byte	0xd
	.byte	0x5e
	.byte	0x9
	.4byte	0x156
	.byte	0xb0
	.uleb128 0x4
	.4byte	.LASF101
	.byte	0xd
	.byte	0x5f
	.byte	0xa
	.4byte	0x51f
	.byte	0xb8
	.uleb128 0x4
	.4byte	.LASF102
	.byte	0xd
	.byte	0x60
	.byte	0x7
	.4byte	0x94
	.byte	0xc0
	.uleb128 0x4
	.4byte	.LASF103
	.byte	0xd
	.byte	0x62
	.byte	0x8
	.4byte	0x7e1
	.byte	0xc4
	.byte	0
	.uleb128 0x3
	.4byte	.LASF104
	.byte	0xe
	.byte	0x7
	.byte	0x19
	.4byte	0x60e
	.uleb128 0x1c
	.4byte	.LASF164
	.byte	0xd
	.byte	0x2b
	.byte	0xe
	.uleb128 0x10
	.4byte	.LASF105
	.uleb128 0x6
	.4byte	0x7a9
	.uleb128 0x6
	.4byte	0x60e
	.uleb128 0xe
	.4byte	0x15d
	.4byte	0x7c8
	.uleb128 0xf
	.4byte	0x43
	.byte	0
	.byte	0
	.uleb128 0x6
	.4byte	0x7a1
	.uleb128 0x10
	.4byte	.LASF106
	.uleb128 0x6
	.4byte	0x7cd
	.uleb128 0x10
	.4byte	.LASF107
	.uleb128 0x6
	.4byte	0x7d7
	.uleb128 0xe
	.4byte	0x15d
	.4byte	0x7f1
	.uleb128 0xf
	.4byte	0x43
	.byte	0x13
	.byte	0
	.uleb128 0x6
	.4byte	0x164
	.uleb128 0x3
	.4byte	.LASF108
	.byte	0xf
	.byte	0x55
	.byte	0x12
	.4byte	0x602
	.uleb128 0x12
	.4byte	0x7f6
	.uleb128 0x6
	.4byte	0x795
	.uleb128 0xd
	.4byte	.LASF127
	.2byte	0x35c
	.4byte	0x81d
	.uleb128 0x1
	.4byte	0x807
	.byte	0
	.uleb128 0xc
	.4byte	.LASF109
	.byte	0xb8
	.byte	0xc
	.4byte	0x94
	.4byte	0x832
	.uleb128 0x1
	.4byte	0x807
	.byte	0
	.uleb128 0x5
	.4byte	.LASF110
	.2byte	0x35e
	.byte	0xc
	.4byte	0x94
	.4byte	0x848
	.uleb128 0x1
	.4byte	0x807
	.byte	0
	.uleb128 0x5
	.4byte	.LASF111
	.2byte	0x360
	.byte	0xc
	.4byte	0x94
	.4byte	0x85e
	.uleb128 0x1
	.4byte	0x807
	.byte	0
	.uleb128 0xc
	.4byte	.LASF112
	.byte	0xec
	.byte	0xc
	.4byte	0x94
	.4byte	0x873
	.uleb128 0x1
	.4byte	0x807
	.byte	0
	.uleb128 0x5
	.4byte	.LASF113
	.2byte	0x23f
	.byte	0xc
	.4byte	0x94
	.4byte	0x889
	.uleb128 0x1
	.4byte	0x807
	.byte	0
	.uleb128 0x5
	.4byte	.LASF114
	.2byte	0x33d
	.byte	0xc
	.4byte	0x94
	.4byte	0x8a4
	.uleb128 0x1
	.4byte	0x807
	.uleb128 0x1
	.4byte	0x8a4
	.byte	0
	.uleb128 0x6
	.4byte	0x7f6
	.uleb128 0x5
	.4byte	.LASF115
	.2byte	0x28e
	.byte	0xe
	.4byte	0x158
	.4byte	0x8c9
	.uleb128 0x1
	.4byte	0x158
	.uleb128 0x1
	.4byte	0x94
	.uleb128 0x1
	.4byte	0x807
	.byte	0
	.uleb128 0x5
	.4byte	.LASF116
	.2byte	0x108
	.byte	0xe
	.4byte	0x807
	.4byte	0x8e4
	.uleb128 0x1
	.4byte	0x7f1
	.uleb128 0x1
	.4byte	0x7f1
	.byte	0
	.uleb128 0x5
	.4byte	.LASF117
	.2byte	0x165
	.byte	0xc
	.4byte	0x94
	.4byte	0x900
	.uleb128 0x1
	.4byte	0x807
	.uleb128 0x1
	.4byte	0x7f1
	.uleb128 0x8
	.byte	0
	.uleb128 0x5
	.4byte	.LASF118
	.2byte	0x263
	.byte	0xc
	.4byte	0x94
	.4byte	0x91b
	.uleb128 0x1
	.4byte	0x94
	.uleb128 0x1
	.4byte	0x807
	.byte	0
	.uleb128 0x5
	.4byte	.LASF119
	.2byte	0x2cd
	.byte	0xc
	.4byte	0x94
	.4byte	0x936
	.uleb128 0x1
	.4byte	0x7f1
	.uleb128 0x1
	.4byte	0x807
	.byte	0
	.uleb128 0x5
	.4byte	.LASF120
	.2byte	0x2e2
	.byte	0xf
	.4byte	0x51f
	.4byte	0x95b
	.uleb128 0x1
	.4byte	0x156
	.uleb128 0x1
	.4byte	0x51f
	.uleb128 0x1
	.4byte	0x51f
	.uleb128 0x1
	.4byte	0x807
	.byte	0
	.uleb128 0x5
	.4byte	.LASF121
	.2byte	0x10f
	.byte	0xe
	.4byte	0x807
	.4byte	0x97b
	.uleb128 0x1
	.4byte	0x7f1
	.uleb128 0x1
	.4byte	0x7f1
	.uleb128 0x1
	.4byte	0x807
	.byte	0
	.uleb128 0xa
	.4byte	.LASF136
	.2byte	0x1ba
	.4byte	.LASF138
	.4byte	0x94
	.4byte	0x99a
	.uleb128 0x1
	.4byte	0x807
	.uleb128 0x1
	.4byte	0x7f1
	.uleb128 0x8
	.byte	0
	.uleb128 0x5
	.4byte	.LASF122
	.2byte	0x30b
	.byte	0xc
	.4byte	0x94
	.4byte	0x9ba
	.uleb128 0x1
	.4byte	0x807
	.uleb128 0x1
	.4byte	0xb3
	.uleb128 0x1
	.4byte	0x94
	.byte	0
	.uleb128 0x5
	.4byte	.LASF123
	.2byte	0x343
	.byte	0xc
	.4byte	0x94
	.4byte	0x9d5
	.uleb128 0x1
	.4byte	0x807
	.uleb128 0x1
	.4byte	0x9d5
	.byte	0
	.uleb128 0x6
	.4byte	0x802
	.uleb128 0x5
	.4byte	.LASF124
	.2byte	0x311
	.byte	0x11
	.4byte	0xb3
	.4byte	0x9f0
	.uleb128 0x1
	.4byte	0x807
	.byte	0
	.uleb128 0x5
	.4byte	.LASF125
	.2byte	0x2e9
	.byte	0xf
	.4byte	0x51f
	.4byte	0xa15
	.uleb128 0x1
	.4byte	0xa15
	.uleb128 0x1
	.4byte	0x51f
	.uleb128 0x1
	.4byte	0x51f
	.uleb128 0x1
	.4byte	0x807
	.byte	0
	.uleb128 0x6
	.4byte	0xa1a
	.uleb128 0x1d
	.uleb128 0x5
	.4byte	.LASF126
	.2byte	0x240
	.byte	0xc
	.4byte	0x94
	.4byte	0xa31
	.uleb128 0x1
	.4byte	0x807
	.byte	0
	.uleb128 0x1e
	.4byte	.LASF145
	.byte	0xf
	.2byte	0x246
	.byte	0xc
	.4byte	0x94
	.uleb128 0xd
	.4byte	.LASF128
	.2byte	0x36e
	.4byte	0xa4f
	.uleb128 0x1
	.4byte	0x7f1
	.byte	0
	.uleb128 0x5
	.4byte	.LASF129
	.2byte	0x16b
	.byte	0xc
	.4byte	0x94
	.4byte	0xa66
	.uleb128 0x1
	.4byte	0x7f1
	.uleb128 0x8
	.byte	0
	.uleb128 0x5
	.4byte	.LASF130
	.2byte	0x264
	.byte	0xc
	.4byte	0x94
	.4byte	0xa81
	.uleb128 0x1
	.4byte	0x94
	.uleb128 0x1
	.4byte	0x807
	.byte	0
	.uleb128 0x5
	.4byte	.LASF131
	.2byte	0x26a
	.byte	0xc
	.4byte	0x94
	.4byte	0xa97
	.uleb128 0x1
	.4byte	0x94
	.byte	0
	.uleb128 0x5
	.4byte	.LASF132
	.2byte	0x2d4
	.byte	0xc
	.4byte	0x94
	.4byte	0xaad
	.uleb128 0x1
	.4byte	0x7f1
	.byte	0
	.uleb128 0xc
	.4byte	.LASF133
	.byte	0x9e
	.byte	0xc
	.4byte	0x94
	.4byte	0xac2
	.uleb128 0x1
	.4byte	0x7f1
	.byte	0
	.uleb128 0xc
	.4byte	.LASF134
	.byte	0xa0
	.byte	0xc
	.4byte	0x94
	.4byte	0xadc
	.uleb128 0x1
	.4byte	0x7f1
	.uleb128 0x1
	.4byte	0x7f1
	.byte	0
	.uleb128 0xd
	.4byte	.LASF135
	.2byte	0x316
	.4byte	0xaed
	.uleb128 0x1
	.4byte	0x807
	.byte	0
	.uleb128 0xa
	.4byte	.LASF137
	.2byte	0x1bd
	.4byte	.LASF139
	.4byte	0x94
	.4byte	0xb07
	.uleb128 0x1
	.4byte	0x7f1
	.uleb128 0x8
	.byte	0
	.uleb128 0xd
	.4byte	.LASF140
	.2byte	0x14e
	.4byte	0xb1d
	.uleb128 0x1
	.4byte	0x807
	.uleb128 0x1
	.4byte	0x158
	.byte	0
	.uleb128 0x5
	.4byte	.LASF141
	.2byte	0x153
	.byte	0xc
	.4byte	0x94
	.4byte	0xb42
	.uleb128 0x1
	.4byte	0x807
	.uleb128 0x1
	.4byte	0x158
	.uleb128 0x1
	.4byte	0x94
	.uleb128 0x1
	.4byte	0x51f
	.byte	0
	.uleb128 0x5
	.4byte	.LASF142
	.2byte	0x16d
	.byte	0xc
	.4byte	0x94
	.4byte	0xb5e
	.uleb128 0x1
	.4byte	0x158
	.uleb128 0x1
	.4byte	0x7f1
	.uleb128 0x8
	.byte	0
	.uleb128 0xa
	.4byte	.LASF143
	.2byte	0x1bf
	.4byte	.LASF144
	.4byte	0x94
	.4byte	0xb7d
	.uleb128 0x1
	.4byte	0x7f1
	.uleb128 0x1
	.4byte	0x7f1
	.uleb128 0x8
	.byte	0
	.uleb128 0x1f
	.4byte	.LASF146
	.byte	0xf
	.byte	0xc2
	.byte	0xe
	.4byte	0x807
	.uleb128 0xc
	.4byte	.LASF147
	.byte	0xd3
	.byte	0xe
	.4byte	0x158
	.4byte	0xb9e
	.uleb128 0x1
	.4byte	0x158
	.byte	0
	.uleb128 0x5
	.4byte	.LASF148
	.2byte	0x2db
	.byte	0xc
	.4byte	0x94
	.4byte	0xbb9
	.uleb128 0x1
	.4byte	0x94
	.uleb128 0x1
	.4byte	0x807
	.byte	0
	.uleb128 0x5
	.4byte	.LASF149
	.2byte	0x174
	.byte	0xc
	.4byte	0x94
	.4byte	0xbd9
	.uleb128 0x1
	.4byte	0x807
	.uleb128 0x1
	.4byte	0x7f1
	.uleb128 0x1
	.4byte	0x52b
	.byte	0
	.uleb128 0x5
	.4byte	.LASF150
	.2byte	0x17a
	.byte	0xc
	.4byte	0x94
	.4byte	0xbf4
	.uleb128 0x1
	.4byte	0x7f1
	.uleb128 0x1
	.4byte	0x52b
	.byte	0
	.uleb128 0x5
	.4byte	.LASF151
	.2byte	0x17c
	.byte	0xc
	.4byte	0x94
	.4byte	0xc14
	.uleb128 0x1
	.4byte	0x158
	.uleb128 0x1
	.4byte	0x7f1
	.uleb128 0x1
	.4byte	0x52b
	.byte	0
	.uleb128 0x20
	.4byte	.LASF152
	.byte	0x10
	.2byte	0x157
	.byte	0xb
	.4byte	0xc4a
	.uleb128 0x2
	.byte	0x8
	.byte	0xaf
	.byte	0xb
	.4byte	0xc4a
	.uleb128 0x2
	.byte	0x8
	.byte	0xb0
	.byte	0xb
	.4byte	0xc6b
	.uleb128 0x2
	.byte	0x8
	.byte	0xb1
	.byte	0xb
	.4byte	0xc8e
	.uleb128 0x2
	.byte	0x8
	.byte	0xb2
	.byte	0xb
	.4byte	0xcac
	.uleb128 0x2
	.byte	0x8
	.byte	0xb3
	.byte	0xb
	.4byte	0xcd1
	.byte	0
	.uleb128 0x5
	.4byte	.LASF153
	.2byte	0x181
	.byte	0xc
	.4byte	0x94
	.4byte	0xc6b
	.uleb128 0x1
	.4byte	0x158
	.uleb128 0x1
	.4byte	0x51f
	.uleb128 0x1
	.4byte	0x7f1
	.uleb128 0x8
	.byte	0
	.uleb128 0xa
	.4byte	.LASF154
	.2byte	0x1ff
	.4byte	.LASF155
	.4byte	0x94
	.4byte	0xc8e
	.uleb128 0x1
	.4byte	0x807
	.uleb128 0x1
	.4byte	0x7f1
	.uleb128 0x1
	.4byte	0x52b
	.byte	0
	.uleb128 0xa
	.4byte	.LASF156
	.2byte	0x204
	.4byte	.LASF157
	.4byte	0x94
	.4byte	0xcac
	.uleb128 0x1
	.4byte	0x7f1
	.uleb128 0x1
	.4byte	0x52b
	.byte	0
	.uleb128 0x5
	.4byte	.LASF158
	.2byte	0x185
	.byte	0xc
	.4byte	0x94
	.4byte	0xcd1
	.uleb128 0x1
	.4byte	0x158
	.uleb128 0x1
	.4byte	0x51f
	.uleb128 0x1
	.4byte	0x7f1
	.uleb128 0x1
	.4byte	0x52b
	.byte	0
	.uleb128 0xa
	.4byte	.LASF159
	.2byte	0x207
	.4byte	.LASF160
	.4byte	0x94
	.4byte	0xcf4
	.uleb128 0x1
	.4byte	0x7f1
	.uleb128 0x1
	.4byte	0x7f1
	.uleb128 0x1
	.4byte	0x52b
	.byte	0
	.uleb128 0x21
	.4byte	.LASF165
	.byte	0x1
	.byte	0xc
	.byte	0x5
	.4byte	0x94
	.8byte	.LFB3
	.8byte	.LFE3-.LFB3
	.uleb128 0x1
	.byte	0x9c
	.4byte	0xd49
	.uleb128 0x9
	.string	"a"
	.byte	0xd
	.byte	0xe
	.4byte	0x1bd
	.uleb128 0x2
	.byte	0x91
	.sleb128 -32
	.uleb128 0x9
	.string	"b"
	.byte	0xd
	.byte	0x15
	.4byte	0x1bd
	.uleb128 0x2
	.byte	0x91
	.sleb128 -24
	.uleb128 0x9
	.string	"c"
	.byte	0xd
	.byte	0x1c
	.4byte	0x1bd
	.uleb128 0x2
	.byte	0x91
	.sleb128 -16
	.uleb128 0x9
	.string	"out"
	.byte	0xe
	.byte	0xe
	.4byte	0x1bd
	.uleb128 0x2
	.byte	0x91
	.sleb128 -8
	.byte	0
	.uleb128 0x22
	.string	"f"
	.byte	0x1
	.byte	0x4
	.byte	0x11
	.4byte	0x1bd
	.8byte	.LFB2
	.8byte	.LFE2-.LFB2
	.uleb128 0x1
	.byte	0x9c
	.uleb128 0x11
	.string	"a"
	.byte	0x1c
	.4byte	0x1bd
	.uleb128 0x2
	.byte	0x91
	.sleb128 -40
	.uleb128 0x11
	.string	"b"
	.byte	0x28
	.4byte	0x1bd
	.uleb128 0x2
	.byte	0x91
	.sleb128 -48
	.uleb128 0x11
	.string	"c"
	.byte	0x34
	.4byte	0x1bd
	.uleb128 0x2
	.byte	0x91
	.sleb128 -56
	.uleb128 0x9
	.string	"x"
	.byte	0x6
	.byte	0xe
	.4byte	0x1bd
	.uleb128 0x2
	.byte	0x91
	.sleb128 -24
	.uleb128 0x9
	.string	"y"
	.byte	0x7
	.byte	0xe
	.4byte	0x1bd
	.uleb128 0x2
	.byte	0x91
	.sleb128 -16
	.uleb128 0x9
	.string	"z"
	.byte	0x8
	.byte	0xe
	.4byte	0x1bd
	.uleb128 0x2
	.byte	0x91
	.sleb128 -8
	.byte	0
	.byte	0
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1
	.uleb128 0x5
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x2
	.uleb128 0x8
	.byte	0
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x18
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x3
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x4
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x5
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0x21
	.sleb128 15
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3c
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x6
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0x21
	.sleb128 8
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x7
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.byte	0
	.byte	0
	.uleb128 0x8
	.uleb128 0x18
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0x9
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0x21
	.sleb128 1
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0xa
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0x21
	.sleb128 15
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x39
	.uleb128 0x21
	.sleb128 12
	.uleb128 0x6e
	.uleb128 0xe
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3c
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xb
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0xb
	.uleb128 0x34
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0xc
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0x21
	.sleb128 15
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3c
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xd
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0x21
	.sleb128 15
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x39
	.uleb128 0x21
	.sleb128 13
	.uleb128 0x3c
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xe
	.uleb128 0x1
	.byte	0x1
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xf
	.uleb128 0x21
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2f
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x10
	.uleb128 0x13
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x11
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0x21
	.sleb128 1
	.uleb128 0x3b
	.uleb128 0x21
	.sleb128 4
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x12
	.uleb128 0x26
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x13
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0x21
	.sleb128 11
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x14
	.uleb128 0x13
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x15
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x1f
	.uleb128 0x1b
	.uleb128 0x1f
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x10
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x16
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x8
	.byte	0
	.byte	0
	.uleb128 0x17
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x18
	.uleb128 0x39
	.byte	0x1
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x19
	.uleb128 0x13
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x1a
	.uleb128 0x13
	.byte	0x1
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x6e
	.uleb128 0xe
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x1b
	.uleb128 0x17
	.byte	0x1
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x1c
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x1d
	.uleb128 0x26
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0x1e
	.uleb128 0x2e
	.byte	0
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x1f
	.uleb128 0x2e
	.byte	0
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x20
	.uleb128 0x39
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x21
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x7c
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x22
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x7a
	.uleb128 0x19
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_aranges,"",@progbits
	.4byte	0x2c
	.2byte	0x2
	.4byte	.Ldebug_info0
	.byte	0x8
	.byte	0
	.2byte	0
	.2byte	0
	.8byte	.Ltext0
	.8byte	.Letext0-.Ltext0
	.8byte	0
	.8byte	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF129:
	.string	"printf"
.LASF27:
	.string	"__off_t"
.LASF76:
	.string	"_IO_read_ptr"
.LASF88:
	.string	"_chain"
.LASF47:
	.string	"int_fast16_t"
.LASF94:
	.string	"_shortbuf"
.LASF58:
	.string	"size_t"
.LASF55:
	.string	"uintptr_t"
.LASF37:
	.string	"uint64_t"
.LASF8:
	.string	"__uint8_t"
.LASF82:
	.string	"_IO_buf_base"
.LASF159:
	.string	"vsscanf"
.LASF123:
	.string	"fsetpos"
.LASF14:
	.string	"__int64_t"
.LASF69:
	.string	"__mbstate_t"
.LASF22:
	.string	"__uint_least32_t"
.LASF7:
	.string	"__int8_t"
.LASF23:
	.string	"__int_least64_t"
.LASF97:
	.string	"_codecvt"
.LASF31:
	.string	"int16_t"
.LASF6:
	.string	"signed char"
.LASF57:
	.string	"uintmax_t"
.LASF89:
	.string	"_fileno"
.LASF77:
	.string	"_IO_read_end"
.LASF156:
	.string	"vscanf"
.LASF15:
	.string	"long int"
.LASF75:
	.string	"_flags"
.LASF40:
	.string	"int_least32_t"
.LASF83:
	.string	"_IO_buf_end"
.LASF92:
	.string	"_cur_column"
.LASF131:
	.string	"putchar"
.LASF46:
	.string	"int_fast8_t"
.LASF35:
	.string	"uint16_t"
.LASF142:
	.string	"sprintf"
.LASF147:
	.string	"tmpnam"
.LASF162:
	.string	"__va_list"
.LASF91:
	.string	"_old_offset"
.LASF96:
	.string	"_offset"
.LASF43:
	.string	"uint_least16_t"
.LASF73:
	.string	"_G_fpos_t"
.LASF63:
	.string	"__gr_offs"
.LASF13:
	.string	"__uint32_t"
.LASF70:
	.string	"__pos"
.LASF18:
	.string	"__uint_least8_t"
.LASF155:
	.string	"__isoc23_vfscanf"
.LASF9:
	.string	"__int16_t"
.LASF64:
	.string	"__vr_offs"
.LASF44:
	.string	"uint_least32_t"
.LASF19:
	.string	"__int_least16_t"
.LASF105:
	.string	"_IO_marker"
.LASF4:
	.string	"unsigned int"
.LASF66:
	.string	"__wchb"
.LASF126:
	.string	"getc"
.LASF117:
	.string	"fprintf"
.LASF145:
	.string	"getchar"
.LASF157:
	.string	"__isoc23_vscanf"
.LASF5:
	.string	"long unsigned int"
.LASF111:
	.string	"ferror"
.LASF80:
	.string	"_IO_write_ptr"
.LASF133:
	.string	"remove"
.LASF160:
	.string	"__isoc23_vsscanf"
.LASF3:
	.string	"short unsigned int"
.LASF25:
	.string	"__intmax_t"
.LASF154:
	.string	"vfscanf"
.LASF84:
	.string	"_IO_save_base"
.LASF134:
	.string	"rename"
.LASF65:
	.string	"__wch"
.LASF113:
	.string	"fgetc"
.LASF53:
	.string	"uint_fast64_t"
.LASF121:
	.string	"freopen"
.LASF54:
	.string	"intptr_t"
.LASF95:
	.string	"_lock"
.LASF90:
	.string	"_flags2"
.LASF102:
	.string	"_mode"
.LASF115:
	.string	"fgets"
.LASF130:
	.string	"putc"
.LASF127:
	.string	"clearerr"
.LASF128:
	.string	"perror"
.LASF132:
	.string	"puts"
.LASF122:
	.string	"fseek"
.LASF38:
	.string	"int_least8_t"
.LASF60:
	.string	"__stack"
.LASF143:
	.string	"sscanf"
.LASF48:
	.string	"int_fast32_t"
.LASF41:
	.string	"int_least64_t"
.LASF59:
	.string	"__gnuc_va_list"
.LASF81:
	.string	"_IO_write_end"
.LASF118:
	.string	"fputc"
.LASF148:
	.string	"ungetc"
.LASF56:
	.string	"intmax_t"
.LASF164:
	.string	"_IO_lock_t"
.LASF74:
	.string	"_IO_FILE"
.LASF119:
	.string	"fputs"
.LASF150:
	.string	"vprintf"
.LASF140:
	.string	"setbuf"
.LASF16:
	.string	"__uint64_t"
.LASF72:
	.string	"__fpos_t"
.LASF116:
	.string	"fopen"
.LASF161:
	.string	"GNU C++17 13.3.0 -mlittle-endian -mabi=lp64 -g -O0 -fno-inline -fno-builtin -fno-omit-frame-pointer -fasynchronous-unwind-tables -fstack-protector-strong -fstack-clash-protection"
.LASF45:
	.string	"uint_least64_t"
.LASF51:
	.string	"uint_fast16_t"
.LASF71:
	.string	"__state"
.LASF52:
	.string	"uint_fast32_t"
.LASF120:
	.string	"fread"
.LASF24:
	.string	"__uint_least64_t"
.LASF32:
	.string	"int32_t"
.LASF2:
	.string	"unsigned char"
.LASF61:
	.string	"__gr_top"
.LASF10:
	.string	"short int"
.LASF107:
	.string	"_IO_wide_data"
.LASF144:
	.string	"__isoc23_sscanf"
.LASF50:
	.string	"uint_fast8_t"
.LASF93:
	.string	"_vtable_offset"
.LASF137:
	.string	"scanf"
.LASF104:
	.string	"FILE"
.LASF87:
	.string	"_markers"
.LASF39:
	.string	"int_least16_t"
.LASF136:
	.string	"fscanf"
.LASF67:
	.string	"__count"
.LASF36:
	.string	"uint32_t"
.LASF29:
	.string	"char"
.LASF153:
	.string	"snprintf"
.LASF11:
	.string	"__uint16_t"
.LASF108:
	.string	"fpos_t"
.LASF42:
	.string	"uint_least8_t"
.LASF110:
	.string	"feof"
.LASF12:
	.string	"__int32_t"
.LASF146:
	.string	"tmpfile"
.LASF109:
	.string	"fclose"
.LASF106:
	.string	"_IO_codecvt"
.LASF26:
	.string	"__uintmax_t"
.LASF28:
	.string	"__off64_t"
.LASF21:
	.string	"__int_least32_t"
.LASF78:
	.string	"_IO_read_base"
.LASF151:
	.string	"vsprintf"
.LASF86:
	.string	"_IO_save_end"
.LASF20:
	.string	"__uint_least16_t"
.LASF149:
	.string	"vfprintf"
.LASF49:
	.string	"int_fast64_t"
.LASF30:
	.string	"int8_t"
.LASF101:
	.string	"__pad5"
.LASF163:
	.string	"11__mbstate_t"
.LASF103:
	.string	"_unused2"
.LASF112:
	.string	"fflush"
.LASF138:
	.string	"__isoc23_fscanf"
.LASF17:
	.string	"__int_least8_t"
.LASF100:
	.string	"_freeres_buf"
.LASF68:
	.string	"__value"
.LASF34:
	.string	"uint8_t"
.LASF85:
	.string	"_IO_backup_base"
.LASF141:
	.string	"setvbuf"
.LASF125:
	.string	"fwrite"
.LASF158:
	.string	"vsnprintf"
.LASF99:
	.string	"_freeres_list"
.LASF135:
	.string	"rewind"
.LASF98:
	.string	"_wide_data"
.LASF33:
	.string	"int64_t"
.LASF124:
	.string	"ftell"
.LASF114:
	.string	"fgetpos"
.LASF139:
	.string	"__isoc23_scanf"
.LASF165:
	.string	"main"
.LASF79:
	.string	"_IO_write_base"
.LASF62:
	.string	"__vr_top"
.LASF152:
	.string	"__gnu_cxx"
	.section	.debug_line_str,"MS",@progbits,1
.LASF0:
	.string	"sample.cpp"
.LASF1:
	.string	"/home/ubuntu/zk-IoT/assistedTrigger-ARM/Ver_2"
	.ident	"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0"
	.section	.note.GNU-stack,"",@progbits
