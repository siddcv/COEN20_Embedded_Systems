		.syntax unified
		.cpu cortex-m4
		.text		
		.global GetNibble
		.thumb_func

GetNibble:
		LSR R2,R1,1		// Divide by 2
		LDRB R2,[R0,R2]		// Address shift 
		AND R1,R1,1		// Test if odd/even
		CMP R1,1		
		ITE EQ			
		UBFXEQ R0,R2,#4,#4	// IF EQUAL INSERT 4 BITS STARTING AT 4
		UBFXNE R0,R2,#0,#4	// IF NEQUAL INSERT 4 BITS STARTING AT 0
		BX LR		
	
		.global PutNibble
		.thumb_func

PutNibble:
		PUSH {R4, LR}
		LSR R4,R1,1		// Divide which 
		LDRB R3,[R0,R4]		// Address shfit
		AND R1,R1,1		// Test if odd/even
		CMP R1,1			
		ITE EQ				
		BFIEQ R3,R2,#4,#4	// IF EQUAL INSERT 4 BITS STARTING AT 4
		BFINE R3,R2,#0,#4	// IF NEQUAL INSERT 4 BITS STARTING AT 0
		STRB R3,[R0,R4]		// Store back into shifted address
		POP {R4,PC}		
		.end