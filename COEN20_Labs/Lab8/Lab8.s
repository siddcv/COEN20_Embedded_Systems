.syntax	unified
.cpu cortex-m4
.text

.global	Discriminant
.thumb_func
Discriminant:
		VMUL.F32 S0,S0,S2	//a*c		
		VMOV S2,4			
		VMUL.F32 S0,S0,S2	//=4*a*c		
		VMUL.F32 S1,S1,S1	//=b^2
		VSUB.F32 S0,S1,S0	//=b^2-4*a*c
		BX LR

.global Quadratic
.thumb_func
Quadratic:
		VMUL.F32 S2,S0,S2	//=b*x	
		VMUL.F32 S0,S0,S0	//=x*x
		VMUL.F32 S0,S0,S1	//=a*x^2
		VADD.F32 S0,S0,S2		
		VADD.F32 S0,S0,S3	//=ax^2+bx+c	
		BX LR

.global Root1
.thumb_func
Root1:
		PUSH {LR}
		VPUSH {S16-S17}
		VMOV S16,S0		//a=>s16	
		VMOV S17,S1		//b=>s17			
		BL Discriminant		//calling discriminant
		VSQRT.F32 S0,S0			
		VNEG.F32 S17,S17		
		VADD.F32 S0,S17,S0		
		VMOV S1,2
		VMUL.F32 S16,S1,S16	
		VDIV.F32 S0,S0,S16	//=s0/s16
		VPOP {S16-S17}
		POP {PC}

			
.global	Root2
.thumb_func
Root2:
		PUSH {LR}
		VPUSH {S16-S17}
		VMOV S16,S0		//a=>s16		
		VMOV S17,S1		//b=>s17			
		BL Discriminant	        //calling discriminant
		VSQRT.F32 S0,S0		
		VNEG.F32 S17,S17	
		VSUB.F32 S0,S17,S0	
		VMOV S1,2
		VMUL.F32 S16,S1,S16
		VDIV.F32 S0,S0,S16	//=s0/s16	
		VPOP {S16-S17}
		POP {PC}
		.end
		
			

		


		