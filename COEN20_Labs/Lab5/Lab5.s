.syntax unified
.cpu cortex-m4
.text
.global MatrixMultiply 
.thumb_func

MatrixMultiply:
		PUSH {R4-R9, LR}
		MOV R4, R0				
		MOV R5, R1				
		MOV R6, R2				
		LDR R7,=0				
top1:	CMP R7,3				
	BGE btm3				
	LDR R8,=0				

top2:	CMP R8,3				
	BGE btm2				
	LDR R3,=3
	MLA R3,R3,R7,R8
	LDR R1,=0	
	STR R1,[R4,R3,LSL 2]	
	LDR R9,=0				

top3:	CMP R9,3				
	BGE btm1				
	LDR R3,=3
	MLA R3,R3,R7,R8		
	LDR R0,[R4,R3,LSL 2]	
	LDR R3,=3
	MLA R3,R3,R7,R9		
	LDR R1,[R5,R3,LSL 2]	
	LDR R3,=3
	MLA R3,R3,R9,R8		
	LDR R2,[R6,R3,LSL 2]	
	BL MultAndAdd			
	LDR R3,=3
	MLA R3,R3,R7,R8		
	STR R0,[R4,R3,LSL 2]	
	ADD R9,R9,1			
	B top3				
		
btm1:	ADD R8,R8,1			
	B top2				
		
btm2:	ADD R7,R7,1			
	B top1				
		
btm3:	POP {R4-R9, PC}
	.end