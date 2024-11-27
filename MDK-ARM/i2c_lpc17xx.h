#ifndef __I2C_LPC17XX_H
#define __I2C_LPC17XX_H

extern void I2CSendAddr(unsigned char, unsigned char);
extern unsigned char I2CGetByte(unsigned char);
extern void I2CSendStop(void);
extern void I2CSendByte(unsigned char);
extern void I2Cdelay(void);

#endif
