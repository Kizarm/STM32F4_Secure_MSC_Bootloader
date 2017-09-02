	.cpu cortex-m4
	.text
	.section	.text.jump_to_firmware,"ax",%progbits
	.align	1
	.global	jump_to_firmware
	.global user_code_begin
	.syntax unified
	.thumb
	.thumb_func
	.fpu softvfp
	.type	jump_to_firmware, %function
jump_to_firmware:
  ldr   r0,user_code_begin  @ load sp and pc from user code (vector table)
  ldr   sp,[r0]             @ ldmia not support sp
  ldr   pc,[r0, #4]         @ jump to user
  
  @bkpt 0
	.align 2
user_code_begin:
  .word _external_code  @ provided by linker script
.end_func_jump:
	.size	jump_to_firmware, .-jump_to_firmware
