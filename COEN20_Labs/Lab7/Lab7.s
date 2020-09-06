.syntax	unified
.cpu cortex-m4
.text
.global Zeller1
.thumb_func

Zeller1:
	ADD R0,R0,R2		// f = k + D	
	ADD R0,R0,R2,LSR 2	// D / 4
	ADD R0,R0,R3,LSR 2	// C / 4
	SUB R0,R0,R3,LSL 1	
	LDR R3,=13				
	MUL R2,R1,R3		// 13 * m		
	SUB R2,R2,1		
	LDR R3,=5
	UDIV R2,R2,R3
	ADD R0,R0,R2			
	LDR R3,=7
	SDIV R2,R0,R3		// temp = f / 7		
	MUL R2,R2,R3			
	SUB R2,R0,R2			
	CMP R2,0			
	IT LT				
	ADDLT R2,R2,R3		// if temp < 0, temp += 7	
	MOV R0,R2
	BX LR
	
.global	Zeller2
.thumb_func

Zeller2:
	PUSH {R4}
	ADD R0,R0,R2		// f = k + D		
	ADD R0,R0,R2,LSR 2	// D / 4
	ADD R0,R0,R3,LSR 2	// C / 4
	SUB R0,R0,R3,LSL 1	
	LDR R3,=13			
	MUL R2,R1,R3		
	SUB R2,R2,1		
	LDR R3,=858993459	// 2^32 / 5	
	SMULL R2,R3,R2,R3	
	ADD R0,R0,R3		
	LDR R3,=613566757	// 2^32 / 7
	SMULL R2,R3,R0,R3	
	LDR R4,=7
	MUL R3,R3,R4		
	SUB R3,R0,R3		
	CMP R3,0		
	IT LT			
	ADDLT R3,R3,7		// if temp < 0, temp += 7	
	MOV R0,R3
	POP {R4}
	BX LR
		
.global	Zeller3
.thumb_func
		
Zeller3:
	PUSH {R4}
	ADD R0,R0,R2		// f = k + D
	ADD R0,R0,R2,LSR 2	// D / 4
	ADD R0,R0,R3,LSR 2	// C / 4
	SUB R0,R0,R3,LSL 1	
	ADD R4,R1,R1,LSL 3	
	ADD R2,R4,R1,LSL 2	
	SUB R2,R2,1		
	LDR R4,=5
	UDIV R2,R2,R4			
	ADD R0,R0,R2			
	LDR R4,=7
	SDIV R2,R0,R4			
	LSL R4,R2,3			
	SUB R2,R4,R2			
	SUB R2,R0,R2			
	CMP R2,0			
	IT LT				
	ADDLT R2,R2,7		// if temp < 0, temp += 7		
	MOV R0,R2
	POP {R4}
	BX LR
	.end