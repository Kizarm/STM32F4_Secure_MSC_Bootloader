	.global DataSource, DataLenght

	.section .rodata.snd
	.align 2
DataSource:
	.incbin "./02.raw.gsm"
data_x_end:
	.word 0
  .align 2
DataLenght:
	.long data_x_end - DataSource
	.long 0
