#include <stdint.h>
#include <math.h>


uint32_t Bits2Unsigned(int8_t bits[8]){
	int i;
	int n;
	int ans=0;
	for(i=7;i>=0;i--)//for loop that converts bits into unsigned number
	{
	  n=pow(2,i)*bits[i];
	  ans=ans+n;
	}
return ans;
}

int32_t Bits2Signed(int8_t bits[8]){
	int32_t output=(int32_t)Bits2Unsigned(bits);//calls unsigned function above
	if(output>127)
	{
	output=output-256;
	}
	return output;
//	int ovf=0;
//	int i;
//	for(i=7;i>=0;i--)
//	{
//	  if(bits[i]==1)
//	  	bits[i]=0;
//	  else
//		bits[i]=1;
//	}
//
//	if(bits[0]==0)
//		bits[0]=1;
//	else
//	{
//		bits[0]=0;
//		ovf=1;
//	}
//
//	for(i=1;i<=7;i++)
//	{
//	  if(bits[i]==0)
//		bits[i]=0+ovf;
//	  else
//		if(ovf==1)
//		  { 
//			bits[i]=0;
//			ovf=1;
//		  }
//	}
//	
//	int n;
//	int ans=0;
//	for(i=7;i>=0;i--)
//	{
//	  n=pow(2,i)*bits[i];
//	  ans=ans+n;
//	}

//return ans;
}



void Increment(int8_t bits[8]){
//	int ovf=0;
	int i;

//	if(bits[0]==0)
//		{bits[0]=1;
//		return;}
//	else
//	{
//		bits[0]=0;
//		ovf=1;
//	}

	for(i=0;i<=7;i++)
	{
	  if(bits[i]==0)//if the bits are 0 it becomes 1 and you can return the function since the overflow wont come
		{bits[i]=1;
		return;}
	  else
		bits[i]=0;
//		if(ovf==1)
//		  { 
//			bits[i]=0;
//			ovf=1;
//		  } 
	}

}

void Unsigned2Bits(uint32_t n, int8_t bits[8]){
	int i=0;
	for(i=7;i>=0;i--)
	{
	  if(pow(2,i)>n)//find largest power of 2 which is less than the n
	   bits[i]=0;
	  else
          {
	    bits[i]=1;
	    n=n-(pow(2,i));//decrement n by the largest power of n
	  }
	}

}