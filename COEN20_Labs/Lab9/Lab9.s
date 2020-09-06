.syntax unified
.cpu cortex-m4
.text

.global Q16Divide
.align
.thumb_func

Q16Divide:
	PUSH {R4}
	EOR R2,R0,R1
	EOR R3,R0,R0,ASR 31
	ADD R0,R3,R0,LSR 31
	EOR R3,R1,R1,ASR 31
	ADD R1,R3,R1,LSR 31
	UDIV R3,R0,R1		//r3-quotient
	MLS R4,R3,R1,R0		//r4-remainder
	.rept 16		//loops 16 times
	LSL R3,R3,1
	LSL R4,R4,1
	CMP R4,R1
	ITT HS
	SUBHS R4,R4,R1		//if rem>=div then rem-=div
	ADDHS R3,R3,1		//increments quotient
	.endr			//loop ends
	EOR R4,R3,R2, ASR 31
	ADD R0,R4,R2,LSR 31	//r0-val of quotient
	POP {R4}
	BX LR
.end