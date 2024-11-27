/*
 * uartfifo.h
 *
 *  Created on: 1-Oct-2011
 *      Author: J.M.V.C.
 */

#ifndef UARTFIFO_H_
#define UARTFIFO_H_


#define TX_BUFSIZE 256
#define RX_BUFSIZE 100

unsigned char txbuf[TX_BUFSIZE];
unsigned char rxbuf[RX_BUFSIZE];
unsigned char ptwr = 0;
unsigned char ptrd = 0;

unsigned char txbuf_lleno = 0;
unsigned char txbuf_vacio = 1;
unsigned char rx_completa = 0;

int uart0_init(int baudrate);
void SendString(char *text);

#endif /* UART_FUNCIONES_H_ */
