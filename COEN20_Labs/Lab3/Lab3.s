	.syntax	unified
	.cpu	cortex-m4
	.text

	.global	UseLDRB
	.thumb_func
UseLDRB: 
         .rept 512	//512/1=512
	 LDRB R2,[R1],1	//LD data from R1 to R2 and increment by 1
	 STRB R2,[R0],1
	 .endr
	 BX LR

	.global	UseLDRH
	.thumb_func
UseLDRH: 
	 .rept 256	//512/2=256
	 LDRH R2,[R1],2	//LD data from R1 to R2 and increment by 2
	 STRH R2,[R0],2
	 .endr
	 BX LR

	.global	UseLDR
	.thumb_func
UseLDR: 
	.rept 128	//512/4-128
	LDR R2,[R1],4	//LD data from R1 to R2 and increment by 4
	STR R2,[R0],4
	.endr
	BX LR

	.global	UseLDRD
	.thumb_func
UseLDRD: 
	 .rept 64	//256/4=64
	 LDRD R2,R3,[R1],8	//LD data from R1 to R2 and R3 and increment by 8
	 STRD R2,R3,[R0],8
	 .endr
	 BX LR

	.global	UseLDM
	.thumb_func
UseLDM:
        PUSH {R4-R9}
	.rept 16
	LDMIA 	R1!,{R2-R9}	//LD R1 into 8 reg
	STMIA	R0!,{R2-R9}
	.endr
	POP {R4-R9}
	BX LR
.end

