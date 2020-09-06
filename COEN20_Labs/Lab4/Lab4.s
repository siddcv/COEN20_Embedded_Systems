		.syntax unified
		.cpu cortex-m4
		.text
		.global MxPlusB
		.thumb_func

MxPlusB:
	PUSH {R4}
	MUL R1, R0, R1		// temp=R0*dvnd
	MUL R0, R1, R2		// temp=mtop*mbottom	
	ASR R0, R0, 31		// temp >>31
	MUL R0, R0, R2		// temp *=dvsr
	LSL R0, R0, 1		// temp <<1
	ADD R0, R0, R2		// temp += dvsr
	MOV R4, 2		// temp1=2	
	SDIV R0, R0, R4		//temp1 /=2
	ADD R0, R0, R1		// temp1=temp1+(R0*dvnd)
	SDIV R0, R0, R2		//temp1-temp1/dvsr
	ADD R0, R0, R3		//temp +=R3
	POP {R4}
	BX LR	
	.end 