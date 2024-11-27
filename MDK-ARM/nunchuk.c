	
#include <LPC17xx.H>
#include <stdio.h>
#include <stdint.h>

#include "i2c_lpc17xx.h"


//uint8_t nunchuk_x, nunchuk_y, nunchuk_ac_x, nunchuk_ac_y, nunchuk_ac_z, ;
//uint8_t nunchuk_flags ;

struct nunchuk
{
	uint8_t x;
	uint8_t y;
	uint16_t ac_x;
	uint16_t ac_y;
  uint16_t	ac_z;
	uint8_t flags ;
	uint8_t c;
	uint8_t z;
};
struct nunchuk nunchuk_d;


//---------------------------------------------------------------------------


//---------------------------------------------------------------------------


//---------------------------------------------------------------------------

void nunchuk_Init(){
	I2Cdelay();
	I2Cdelay();
	
	nunchuk_d.c = 0xF;
	nunchuk_d.z = 0xF;
	
	I2CSendAddr(0x52,0);
	I2CSendByte(0xF0);
	I2CSendByte(0x55);
	I2CSendStop();

	I2CSendAddr(0x52,0);	
	I2CSendByte(0xFB);
	I2CSendByte(0x00);
	I2CSendStop();
	//*/
}

//---------------------------------------------------------------------------

uint8_t nunchuk_read_Tra(){
	
I2CSendAddr(0x52,0);
I2CSendByte(0x00);
	I2Cdelay();
I2CSendAddr(0x52,1);
	I2Cdelay();
	
	nunchuk_d.x =			0;
nunchuk_d.y =			0;
nunchuk_d.ac_x =	0xF;
nunchuk_d.ac_y =	0;
nunchuk_d.ac_z =	0;
nunchuk_d.flags =	0xF; 
	
nunchuk_d.x =			I2CGetByte(0);
nunchuk_d.y =			I2CGetByte(0); 
nunchuk_d.ac_x =	I2CGetByte(0)<<2; 
nunchuk_d.ac_y =	I2CGetByte(0)<<2; 
nunchuk_d.ac_z =	I2CGetByte(0)<<2; 
nunchuk_d.flags =	I2CGetByte(1); 
	
nunchuk_d.ac_x |=	(nunchuk_d.flags & 0xC)>>2;		//0000_1100
nunchuk_d.ac_y |=	(nunchuk_d.flags & 0x30)>>4; 	//0011_0000
nunchuk_d.ac_z |=	(nunchuk_d.flags & 0xC0)>>6;	  //1100_0000
	
nunchuk_d.c = (nunchuk_d.flags & 0x02);
nunchuk_d.z = (nunchuk_d.flags & 0x01);
	
I2Cdelay();
I2CSendStop();
	if(nunchuk_d.x== 0xFF && nunchuk_d.y == 0xFF && nunchuk_d.ac_x == 0x3FF) return 0;
	else return 1;
	
}

//---------------------------------------------------------------------------


