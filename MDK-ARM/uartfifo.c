/*
 * uartfifo.c
 *
 *  Created on: 1-Oct-2011
 *      Author: J.M.V.C.
 */
#include <LPC17xx.h>
#include <string.h>
#include "uartfifo.h"


// Accepted Error baud rate value (in %)
#define UART_ACCEPTED_BAUDRATE_ERROR    3

#define CHAR_8_BIT                      (3 << 0)
#define STOP_1_BIT                      (0 << 2)
#define PARITY_NONE                     (0 << 3)
#define DLAB_ENABLE                     (1 << 7)
#define FIFO_ENABLE                     (1 << 0)
#define RBR_IRQ_ENABLE                  (1 << 0)
#define THRE_IRQ_ENABLE                 (1 << 1)
#define UART_LSR_THRE                   (1 << 5)
#define RDA_INTERRUPT                   (2 << 1)
#define CTI_INTERRUPT                   (6 << 1)
#define THE_INTERRUPT										(1 << 1)



static int uart0_set_baudrate(unsigned int baudrate);
extern char i;
extern char modo;
extern char order_ej[15];
extern int indice_decod;
extern double L_total;
extern double angulo;

/*
	G:GetPosicion
	M:SetModo
	C:SetComando
*/

/*
 * UART0 interrupt handler
 */
void UART0_IRQHandler(void) {
	
	  static char j; // Tamaño FIFO en TX  16 bytes
    switch(LPC_UART0->IIR & 0x0E) {
	  case RDA_INTERRUPT: // FIFO llena ? en recepción
    case CTI_INTERRUPT: // Character Time Out ? (Por si no se llena la FIFO en recepción)
			rxbuf[j++]=LPC_UART0->RBR; 													// Leemos hasta vaciar la FIFO
			if(rxbuf[j-1]==0xA){																	// Return?
			rx_completa=1;
			j=0;
			if(rxbuf[0]=='G')//GetPosicion
			{
				
				i=sprintf((char*)txbuf,"distancia=%f orientacion=%f",L_total,angulo);
				txbuf_vacio=0;
				
				}else if(rxbuf[3]=='M'){//SetModo
				if(strncmp((const char*)(&rxbuf[8]),"DEBUG",5)==0)modo = 'D';
				if(strncmp((const char*)(&rxbuf[8]),"DEBUGUART",9)==0)modo = 'U';
				if(strncmp((const char*)(&rxbuf[8]),"MANUAL",6)==0)modo = 'M';
				if(strncmp((const char*)(&rxbuf[8]),"AUTOMATICO",10)==0)modo = 'A';
					
			}else if(rxbuf[3]=='C' && modo=='A' ){//SetComando
				memset(order_ej, 0, 13);
				strncpy(order_ej,(const char*)(rxbuf+11),strlen(rxbuf)-1);
				order_ej[strcspn(order_ej, "\n")] = 0;
				indice_decod=0;
				memset(rxbuf, 0, 100);
			}
				
			}

		case THE_INTERRUPT: 	// FIFO vacia? en transmisión (Tamaño de la FIFO 16 bytes)
		if (i && !txbuf_vacio){
						if(txbuf[ptrd]!= 0)LPC_UART0->THR = txbuf[ptrd++];									// Leemos del buffer y cargamos en la FIFO (max. 16 carac.)
						else{
							LPC_UART0->THR = 127;
							ptrd++;
						}
						txbuf_lleno=0;  																// Indica que se ha escrito
						if(ptrd >= TX_BUFSIZE) ptrd = 0;								// Buffer circular (ptrd&=TX_BUFSIZE-1)
						if (ptrd==ptwr) txbuf_vacio=1;									// Hemos transmitido todo
						i --;
						if(i==0){
							txbuf_vacio=1;
							ptrd=0;
						}
						}
		break;

    }
}
void SendString(char *text)//borrar
{
while(*text && !txbuf_lleno){ 			// Mientras no se llene el buffer o se alcance el fin de cadena:
	txbuf[ptwr++] = *text++;					// Guardamos la(s) cadena(s) dentro del buffer auxiliar, una detrás de otra (ojo, si llamamos varias veces a SendString())
	if(ptwr >= TX_BUFSIZE) ptwr = 0;  // buffer circular	 (ptwr&=TX_BUFSIZE-1)
	if (ptwr==ptrd) txbuf_lleno=1;    // buffer lleno (Tenemos que vaciar(leer) antes de seguir guardando)
	}
if (txbuf_vacio){
	LPC_UART0->THR = txbuf[ptrd++];	  // Necesario escribir algo la primera vez para que se inicie la TX.
	txbuf_vacio=0;
	}
}





int uart0_init(int baudrate) {
    int err = 0;
    LPC_PINCON->PINSEL0 |= (1 << 4) | (1 << 6);		// Change P0.2 and P0.3 mode to TXD0 and RXD0
    LPC_UART0->LCR &= ~STOP_1_BIT & ~PARITY_NONE; // Set 8N1 format
    LPC_UART0->LCR |= CHAR_8_BIT;
    err = uart0_set_baudrate(baudrate);									// Set the baud rate
    LPC_UART0->FCR |= FIFO_ENABLE;								// Enable TX and RX FIFO  
    LPC_UART0->FCR |= (3 << 6);										// Set FIFO to trigger when at least 14 characters available   
    LPC_UART0->IER = THRE_IRQ_ENABLE|RBR_IRQ_ENABLE;// Enable UART TX and RX interrupt (for LPC17xx UART)   
    NVIC_EnableIRQ(UART0_IRQn);											// Enable the UART interrupt (for Cortex-CM3 NVIC)
	return err;
}


static int uart0_set_baudrate(unsigned int baudrate) {
    int errorStatus = -1; //< Failure

    // UART clock (FCCO / PCLK_UART0)
   // unsigned int uClk = SystemFrequency / 4;
    unsigned int uClk =SystemCoreClock/4;
    unsigned int calcBaudrate = 0;
    unsigned int temp = 0;

    unsigned int mulFracDiv, dividerAddFracDiv;
    unsigned int divider = 0;
    unsigned int mulFracDivOptimal = 1;
    unsigned int dividerAddOptimal = 0;
    unsigned int dividerOptimal = 0;

    unsigned int relativeError = 0;
    unsigned int relativeOptimalError = 100000;

    uClk = uClk >> 4; /* div by 16 */

    /*
     *  The formula is :
     * BaudRate= uClk * (mulFracDiv/(mulFracDiv+dividerAddFracDiv) / (16 * DLL)
     *
     * The value of mulFracDiv and dividerAddFracDiv should comply to the following expressions:
     * 0 < mulFracDiv <= 15, 0 <= dividerAddFracDiv <= 15
     */
    for (mulFracDiv = 1; mulFracDiv <= 15; mulFracDiv++) {
        for (dividerAddFracDiv = 0; dividerAddFracDiv <= 15; dividerAddFracDiv++) {
            temp = (mulFracDiv * uClk) / (mulFracDiv + dividerAddFracDiv);

            divider = temp / baudrate;
            if ((temp % baudrate) > (baudrate / 2))
                divider++;

            if (divider > 2 && divider < 65536) {
                calcBaudrate = temp / divider;

                if (calcBaudrate <= baudrate) {
                    relativeError = baudrate - calcBaudrate;
                } else {
                    relativeError = calcBaudrate - baudrate;
                }

                if (relativeError < relativeOptimalError) {
                    mulFracDivOptimal = mulFracDiv;
                    dividerAddOptimal = dividerAddFracDiv;
                    dividerOptimal = divider;
                    relativeOptimalError = relativeError;
                    if (relativeError == 0)
                        break;
                }
            }
        }

        if (relativeError == 0)
            break;
    }

    if (relativeOptimalError
            < ((baudrate * UART_ACCEPTED_BAUDRATE_ERROR) / 100)) {

        LPC_UART0->LCR |= DLAB_ENABLE;
        LPC_UART0->DLM = (unsigned char) ((dividerOptimal >> 8) & 0xFF);
        LPC_UART0->DLL = (unsigned char) dividerOptimal;
        LPC_UART0->LCR &= ~DLAB_ENABLE;

        LPC_UART0->FDR = ((mulFracDivOptimal << 4) & 0xF0) | (dividerAddOptimal & 0x0F);

        errorStatus = 0; //< Success
    }

    return errorStatus;
}








 					   					  