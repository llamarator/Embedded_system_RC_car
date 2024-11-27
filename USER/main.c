/**--------------File Info---------------------------------------------------------------------------------
** File name:               main.c
** Descriptions:            Example of simple state machine implemented menu with an HTTP server 
**                          and FTP server with data in a SD card 
**
**--------------------------------------------------------------------------------------------------------
** Created by:              JPM (UAH)
** Created date:            2017-08-17
** Version:                 v1.0
** Descriptions:            The original version
**
**--------------------------------------------------------------------------------------------------------
** Modified by:             
** Modified date:           
** Version:                 
** Descriptions:            
**
*********************************************************************************************************/

/* Includes ------------------------------------------------------------------*/

#include <stdio.h>
#include <RTL.h>
#include <Net_Config.h>
#include <LPC17xx.h>                    /* LPC17xx definitions               */
#include <GLCD.h>
#include <serial.h>

#include <TouchPanel.h>
#include <menu.h>
#include <leds.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

//#include "nunchuk.c"

extern   U32 CheckMedia (void);


#define Fpclk 25e6								// Fcpu/4 (defecto después del reset)
#define F_cpu 100e6								// Defecto Keil (xtal=12Mhz)
#define F_sample 8000							// frecuencia de muestreo
#define N_muestras 16000 					// igual a la longitud del array generado en Matlab
#define N_muestras_pitido 2000 		//2KHz durante 1 segundo

#define Th (5e-3*Fpclk)	  				// 5 ms
#define T (20e-3*Fpclk)    				// 20 ms

#define F_out 1		  							// 100 Hz
#define Tpwm 15e-3								// Perido de la señal PWM (15ms)
#define N 20											// nº de agujeros en el encoder
#define R_rueda 3									// 3cm
#define pi 3.1415926535897				// número piu
#define ppv 20										// pulsos por vuelta
#define D_ruedas 14								//distancia entre las ruedas en cm 
#define filtro_const 10

#define F_wdclk Fpclk/4 					// WDT clk

uint8_t screenState; 							//estados de la pantalla
uint8_t motor_state;							//estados de la pantalla en modo DEBUG dependiendo de cada motor
//0-izq
//1-dcho

/* Definición de los estados */
#define SCREEN_WELCOME      		0
#define SCREEN_WELCOME_WAIT 		1
#define SCREEN_MANUAL         	2
#define SCREEN_MANUAL_WAIT    	3
#define SCREEN_AUTOMATICO       4
#define SCREEN_AUTOMATICO_WAIT  5
#define SCREEN_DEBUG         		6
#define SCREEN_DEBUG_WAIT    		7
#define MOTOR_IZQ 0
#define MTOR_DCHO 1


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
extern struct nunchuk nunchuk_d;
extern struct t_screenZone
{
   uint16_t x;         
	uint16_t y;
	uint16_t size_x;
	uint16_t size_y;
	uint8_t  pressed;
};

extern struct t_screenZone zone_1;
extern struct t_screenZone zone_2;
extern struct t_screenZone zone_3;
extern struct t_screenZone zone_4;
extern struct t_screenZone zone_5;
extern struct t_screenZone zone_6;
extern struct t_screenZone zone_7;
extern struct t_screenZone zone_8;
extern struct t_screenZone zone_9;

/* Variable que contiene el dato del programa */
int finish_r = 0;								//acaba de avanzar recto
int finish_g = 0;								//acaba de girar

char modo = 'A';								//modo en el que está el sistema
/*
	A : Automático
	M : Manual
	D : Debug
*/
short int estado_decod = 0;			//estado del decodificador
int indice_decod = 0;
int count_decod = 0;
char order= '0';
char order_ej[15] = "P5";
/*
Rxxx Avanzar
D giro90º derecha
I giro90º izquierda
Pxx el robot se detiene xx segundos
G graba audio
A reproduce audio
*/
	

float dato = 0;											//dato que posteriormente se asignará a VelDerecha o VelIzquierda
float dato_prev =0;									//El valor previo de la variable dato
float dato_f = 5;										//como dato pero en incrementos de 0.5 (no se utiliza esta funcionalidad)
float velDerecha = 0;								//velocidad del motor derecho(ciclo de trabajo)
float velIzquierda = 0;							//velocidad del motor izquierdo(ciclo de trabajo)

double L=0;													//Distancia a recorrer
double L_izq = 0;										//Distancia recorrida por motor izquierdo
double L_dcha = 0;									//Distancia recorrida por motor DERECHO
double L_total = 0;									//Distancia recorrida por el robot
double L_inicial = 0;								//Distancia inciial
long pulsos_dcha_1 =0 ; 						//pulsos encoder derecho procedimiento 1
long pulsos_dcha_2 =0 ; 						//pulsos encoder derecho procedimiento 2
long pulsos_izq_1 =0 ; 							//pulsos encoder izquierda procedimiento 1
long pulsos_izq_2=0 ; 							//pulsos encoder izquierda procedimiento 2
double V_izq_1 = 0;									//velocidad motor izquierdo procedimiento 1
double V_dcha_1 = 0;								//velocidad motor derecho procedimiento 1
double V_izq_2 = 0;									//velocidad motor izquierdo procedimiento 2
double V_dcha_2 = 0;								//velocidad motor derecho procedimiento 2
double angulo = 0;									//angulo recorrido
double angulo_inicial = 0;					//angulo inicial
double ang = 0;											//angulo a recorrer
double velocidad_1 = 0;							//velocidad total procedimiento 1
double velocidad_2 = 0;							//velocidad total procedimiento 1


 short int act =0;									//indica a 1 que la pantalla tiene que refrescarse 
 
 uint8_t muestras[N_muestras];								//muestras de audio
 uint8_t muestras_pitido[N_muestras_pitido];	//muestras del pitido

 uint8_t recording =0;							//indica que está grabando audio
 uint8_t playing =0;								//indica que está reproduciendo audio
 uint8_t pitido=0;									//indica que está reproduciendo el pitido
 uint16_t n_muestra = 0;						//muestra actual para ir controlando el array de muestras


#define TX_BUFSIZE 256							//tamaño buffer de transmisión
#define RX_BUFSIZE 100							//tamaño buffer de recepción

extern unsigned char txbuf[TX_BUFSIZE];			//buffer de transmisión
extern unsigned char rxbuf[RX_BUFSIZE];			//buffer de recepción
extern unsigned char ptrd;									//posicion del buffer que se está transmitiendo

extern unsigned char txbuf_lleno;						//buffer de transmision lleno con 1
extern unsigned char txbuf_vacio;						//buffer vacio con 1
extern unsigned char rx_completa;						//trama recibida

char i=15;													//caracteres a transmitir pot txbuf

/* Variable temporal donde almacenar cadenas de caracteres */
char texto[25];															//texto para escribir en el LCD
char titulo[25] = "velDerecha";							//titulo para cada estado de la pantalla

uint8_t flag_I2C = 0;							//indica que hay que leer el I2C


/*--------------------------- init ------------------------------------------*/

/*******************************************************************************
* Function Name  : init
* Description    : Initialize every subsystem
* Input          : None
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
static void init () {
  LCD_Initializtion();	
	screenWelcome();
  /*  Serial init for debug. Need enviroment constant __UART0, __UART1 o __DBG_ITM  */
  SER_Init ();
  init_TcpNet (); // volver a descomentar
  /* Setup and enable the SysTick timer for 100ms. */
  SysTick->LOAD = (SystemCoreClock / 10) - 1;
  SysTick->CTRL = 0x05;
  /* TouchPanel Init  */   
  TP_Init(); 
}



/*--------------------------- timer_poll ------------------------------------*/

/*******************************************************************************
* Function Name  : timer_poll
* Description    : Call timer_tick() if every 100ms (aprox) 
* Input          : None
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
static void timer_poll () {
  /* System tick timer running in poll mode */

  if (SysTick->CTRL & 0x10000) {
    /* Timer tick every 100 ms */
    timer_tick ();
  }
}


void setVelDerecha(char ciclo){
	if (ciclo < -100)ciclo=-100;
	if (ciclo > 100)ciclo=100;
	LPC_PWM1->MR3= LPC_PWM1->MR0*ciclo/100; // duty cycle
	LPC_PWM1->LER|=(1<<3)|(1<<0);
	}

void setVelIzquierda(char ciclo){
	if (ciclo < -100)ciclo=-100;
	if (ciclo > 100)ciclo=100;
	LPC_PWM1->MR2= LPC_PWM1->MR0*ciclo/100; // duty cycle
	LPC_PWM1->LER|=(1<<2)|(1<<0);
}


////////////////////////////////////////////////////////////////////////

void configGPIO(void)
{
	LPC_PINCON->PINSEL1 |= (0<<2*(21-16));		//0.21 GPIO motor izq EN
	LPC_PINCON->PINSEL1 |= (0<<2*(22-16));	//0.22 GPIO		motor dcho EN
	LPC_PINCON->PINSEL3|=(0<<(2*9)); // P1.25 
  LPC_GPIO1->FIODIR |= (1<<25); //P1.25 salida
	LPC_PINCON->PINSEL3|=(0<<14); // P1.23 
  LPC_GPIO1->FIODIR |= (1<<23); //P1.23 salida
	
	
	LPC_GPIO0->FIODIR |= (1<<11);	//P0.11 salida
	LPC_GPIO0->FIODIR |= (1<<10);	//P0.10 salida
	
}

/* variables de tiempo pulsado*/
uint8_t T_cz =0;	//en C y Z
uint8_t T_c =0;		//en C

void check_cz(void)
{
	if(nunchuk_d.c == 0x00)T_c++;
	else if(T_c>4){
		if (modo=='A')modo='M';
		T_c=0;
	}else T_c=0;
	
	if(nunchuk_d.c==0x00 && nunchuk_d.z==0x00)
	{
		T_cz++;
		
	}else if(T_cz >1){
		switch(modo){
				case 'M':
				if(T_cz < 4)modo='A';
				if(T_cz > 4)modo='D';
				break;
				default:
					modo = 'A';	
			}
				T_cz=0;
		}else T_cz=0;

			if(modo=='M')
			{
				if(nunchuk_d.c ==0 && recording ==0)//recording
				{
					/*ponemos el resto de interrupciones a minima prioridad*/
					NVIC_SetPriority(ADC_IRQn,7);	
					NVIC_SetPriority(SysTick_IRQn,7);
					NVIC_SetPriority(ENET_IRQn,7);
					NVIC_SetPriority(UART0_IRQn,7);
					NVIC_SetPriority(TIMER0_IRQn,1);
					NVIC_SetPriority(TIMER1_IRQn,7);
					NVIC_SetPriority(TIMER2_IRQn,7);
					NVIC_SetPriority(TIMER3_IRQn,7);
					
					  NVIC_DisableIRQ(ADC_IRQn);					// 
					
					
					//MODIFICAR el timer0 para ajustar la frecuencia del ADC
					LPC_ADC->ADINTEN=0;
					LPC_ADC->ADCR&=~ (1<<0)|(1<<2);//dejamos solo el canal 4	
					LPC_ADC->ADCR = (1<<4)|		  	  // Canal 4
											 (0x01<<8)|		  	  // CLKDIV=1   (Fclk_ADC=25Mhz /(1+1)= 12.5Mhz)
											 (0x01<<21)|			 	// PDN=1
											 (5<<24);				    // Inicio de conversión con el Match 0.3 del Timer 0
						
					NVIC_EnableIRQ(ADC_IRQn);					//activamos nuevamente la IRQ
					NVIC_SetPriority(ADC_IRQn,0);			//Ajustamos la prioridad máxima para evitar interferencias   
					
					LPC_TIM0->TCR=0x01;		//Start Timer .
					recording = 1;
					LPC_ADC->ADINTEN=(1<<4);				//interrupción por canal 4
				}
				if(nunchuk_d.z ==0 && playing ==0)	//playing
				{
					LPC_TIM0->TCR=0x01;		//Start Timer .
					playing = 1;
				}
			}
}
/* variables de estado de los botones*/
uint8_t ISP =0;
uint8_t Key1 =0;
uint8_t Key2 =0;
uint8_t T_K1K2 =0; //temporizador para emular el tiempo pulsado de Key1 y Key2
void check_ISPKey1Key2(void)
	{
		
	if(LPC_GPIO2->FIOPIN&(1<<10)) ISP=0;
	else ISP=1;
	if(LPC_GPIO2->FIOPIN&(1<<11)) Key1=0;
	else Key1=1;
	if(LPC_GPIO2->FIOPIN&(1<<12)) Key2=0;		
	else Key2=1;	
		
	if((Key1==1 && Key2==1))
		{
			T_K1K2++;
			
		return;
		}else if(T_K1K2 > 1){
			
				switch(modo){
				case 'M':
				if(T_K1K2 > 10)modo='D';
				if(T_K1K2 < 10)modo='A';
				break;
				case 'A':
					if(T_K1K2 > 10)modo='D';
				break;
				case 'D':
					if(T_K1K2 < 10)modo='A';
					if(T_K1K2 > 10)modo='M';
				break;
				default:
					modo = 'M';	
			}
				T_K1K2=0;
		}else{
			if(ISP==1)
				{
					pitido=1;
					LPC_TIM0->TCR=0x01;
				}
				if(Key1 ==1 )//recording
				{
					/*ponemos el resto de interrupciones a minima prioridad*/
					NVIC_SetPriority(ADC_IRQn,7);					
					NVIC_SetPriority(SysTick_IRQn,7);
					NVIC_SetPriority(ENET_IRQn,7);
					NVIC_SetPriority(UART0_IRQn,7);
					NVIC_SetPriority(TIMER0_IRQn,1);
					NVIC_SetPriority(TIMER1_IRQn,7);
					NVIC_SetPriority(TIMER2_IRQn,7);
					NVIC_SetPriority(TIMER3_IRQn,7);
					
					  NVIC_DisableIRQ(ADC_IRQn);					// 
					
					
					//MODIFICAR el timer0 para ajustar la frecuencia del ADC
					LPC_ADC->ADINTEN=0;
					LPC_ADC->ADCR&=~ (1<<0)|(1<<2);//dejamos solo el canal 4	
					LPC_ADC->ADCR = (1<<4)|		  	  // Canal 4
											 (0x01<<8)|		  	  // CLKDIV=1   (Fclk_ADC=25Mhz /(1+1)= 12.5Mhz)
											 (0x01<<21)|			 	// PDN=1
											 (5<<24);				    // Inicio de conversión con el Match 0.3 del Timer 0
						
					NVIC_EnableIRQ(ADC_IRQn);					//volvemos a activar la IRQ 
					NVIC_SetPriority(ADC_IRQn,0);			//Ponemos máxima prioridad al ADC para evitar interferencias   
					
					LPC_TIM0->TCR=0x01;		//Start Timer .
					recording = 1;
					LPC_ADC->ADINTEN=(1<<4);
				}
				if(Key2 ==1)	//playing
				{
					LPC_TIM0->TCR=0x01;		//Start Timer .
					playing = 1;
				}
		}
	}

uint8_t counter_TM0 =0;			//para sacar muestras del pitido
uint32_t counter_TM0_2 =0;	//para 20s entre pitidos
void TIMER0_IRQHandler(void)
{
	static uint16_t indice_muestra;	
	static uint16_t indice_muestra_pitido;
	LPC_TIM0->IR|= (1<<0); 										// borrar flag MR0
	if (pitido ==1 && recording == 0)					//no grabando y si pitido
	{
	counter_TM0_2++;
	LPC_TIM0->MR0 = (Fpclk/(2000*10))-1; 		//ajustamos MR0 para frecuencia del pitido
	}else{
		LPC_TIM0->MR0 = (Fpclk/(F_sample)); 	//MR0 para frecuencia de muestreo
		LPC_TIM0->MR3 = (Fpclk/(F_sample))-50;//MR3 para frecuencia de muestreo y que esté antes que MR0
	}
	if (recording == 0)
	{
		if (pitido==1 && indice_muestra_pitido < N_muestras_pitido)
		{
			
			counter_TM0++;
			if (counter_TM0==2)//prescaler
			{
			LPC_DAC->DACR= muestras_pitido[indice_muestra_pitido++]<<8;		//saca por DAC muestras de pitido
			counter_TM0=0;
			}
		}
		
		
	if(playing == 1 && pitido ==0 && recording == 0){
		LPC_DAC->DACR= muestras[indice_muestra++]<<8;			//sacr por DAC muestras de audio
	}
		if(counter_TM0_2 == 400000 && indice_muestra_pitido == N_muestras_pitido) //prescaler para 20s
		{
				indice_muestra_pitido=0;
				LPC_TIM0->TCR=0x02;	//Stop Timer and reset, DAC= 0V.
			  LPC_DAC->DACR=0;	 // 0 V
				pitido = 0;
			counter_TM0_2=0;
			
		}else if(indice_muestra==N_muestras-1) 
		{
				indice_muestra=0;
				LPC_TIM0->TCR=0x02;	//Stop Timer and reset, DAC= 0V.
			  LPC_DAC->DACR=0;	 // 0 V
				playing = 0;
			counter_TM0_2 =0;
		}
		if(counter_TM0_2 < 20000 && indice_muestra_pitido == N_muestras_pitido)indice_muestra_pitido=0;
	
}else LPC_TIM0->EMR &=~ (1<<3);//MAT0.3  a 0 (cuando MR3 se active MAT0.3 se pondrá a 1 y activará el ADC)

}	
unsigned long ticks_25MHz =0,ticks_25MHz_NM1;
unsigned long ticks2_25MHz =0,ticks2_25MHz_NM1;
 //PROCEDIMIENTO 1
void TIMER1_IRQHandler(void)
{
	if((LPC_TIM1->IR & 0x10) == 0x10)   //CAP1.0
	{
		LPC_TIM1->IR|= (1<<4);
		
	ticks_25MHz = LPC_TIM1->CR0 - ticks_25MHz_NM1;	//tiempo transcurrido entre pulsos
	ticks_25MHz_NM1 = LPC_TIM1->CR0;								//tiempo actual
		pulsos_dcha_1 ++;			//aumenta nº pulsos motor dcha de procedimiento 1
		pulsos_dcha_2 ++;			//aumenta nº pulsos motor dcha de procedimiento 2
		if(velDerecha >= 0)
			{
		L_dcha = pulsos_dcha_1*2*pi*R_rueda/ppv;  //cm
		V_dcha_1 = (2*pi*R_rueda/ppv) /(ticks_25MHz /25e6);
			}else{
		L_dcha = pulsos_dcha_1*2*pi*R_rueda/ppv;  //cm
		V_dcha_1 = -(2*pi*R_rueda/ppv) /(ticks_25MHz /25e6);
			}

	}
	
	if((LPC_TIM1->IR & 0x20) == 0x20)  //CAP1.1
	{
	LPC_TIM1->IR |= (1<<5);
	ticks2_25MHz = LPC_TIM1->CR1 - ticks2_25MHz_NM1;		//tiempo transcurrido entre pulsos
	ticks2_25MHz_NM1 = LPC_TIM1->CR1;										//tiempo actual
	pulsos_izq_1 ++;			//aumenta nº pulsos motor izq de procedimiento 1
	pulsos_izq_2 ++;			//aumenta nº pulsos motor izq de procedimiento 2
		if(velIzquierda >= 0)
			{
		L_izq= pulsos_izq_1*2*pi*R_rueda/ppv;								//distancia recorrdia
		V_izq_1 = (2*pi*R_rueda/ppv) /(ticks2_25MHz/25e6);	//velocidad procedimiento 1 motor izquierdo
			}else{
		L_izq= pulsos_izq_1*2*pi*R_rueda/ppv;								//distancia recorrida 
		V_izq_1 = -(2*pi*R_rueda/ppv) /(ticks2_25MHz/25e6);	//velocidad procedimiento 1 motor izquierdo
			}
	}
	L_total = (L_izq+L_dcha)/2;
	
	velocidad_1 = (V_dcha_1+V_izq_1)/2;
	
}


// PROCEDIMIENTO 2
uint8_t TIM2_0 =0;
uint8_t TIM2_1 =0;
uint8_t TIM2_2 =0;
void TIMER2_IRQHandler(void)//100ms
{
		V_dcha_2=0;
	V_izq_2 =0;
	LPC_TIM2->IR|= (1<<0);
	if (pulsos_dcha_2 != 0 || pulsos_izq_2 !=0)
	{
		if(velDerecha >= 0.0)
			{
	V_dcha_2 = pulsos_dcha_2 * (2.0*pi*R_rueda/ppv)/100e-3;
			}else{
	V_dcha_2 = -pulsos_dcha_2 * (2.0*pi*R_rueda/ppv)/100e-3;
			}
			
		if(velIzquierda >= 0.0)
			{
		V_izq_2 = pulsos_izq_2 * (2.0*pi*R_rueda/ppv)/100e-3;
			}else{
		V_izq_2 = -pulsos_izq_2 * (2.0*pi*R_rueda/ppv)/100e-3;
			}
	}
	
	angulo += 100e-3*(V_izq_2-V_dcha_2)/D_ruedas;
	pulsos_dcha_2 = 0;
	pulsos_izq_2 = 0;
	if (velocidad_1 < 3.00)velocidad_1 =0;
	
	//parte del ADC(aprovecho este timer)
	   if(recording ==0)LPC_ADC->ADCR|=(1<<16); // BURST=1 --> Cada 65TclkADC se toma una muestra de cada canal comenzando 
   
	if(modo=='U')TIM2_0++;		//modo DEBUGUART
	if(TIM2_0 ==10){
		/*
		*/ // PARA ENVIAR DATOS POR LA UART CADA 1s falta completarlo
		i=sprintf((char*)txbuf,"distancia=%f orientacion=%f \n ",L_total,angulo);
		txbuf_vacio=0;
		TIM2_0=0;
		LPC_UART0->THR = txbuf[ptrd++];	  // Necesario escribir algo la primera vez para que se inicie la TX.
	}
	TIM2_1++;
	if(TIM2_1 ==5){    //tiempo de refresco I2C 0.5s
		 flag_I2C = 1;
		velocidad_1=0;
	}
	TIM2_2++;
	if(TIM2_2 ==2){				//0,2s
	check_ISPKey1Key2();
		check_cz();
		TIM2_2=0;
	}

}
uint32_t canal_0[filtro_const], canal_2[filtro_const], canal_4[filtro_const];
double V_bat =0, V_th = 0;
uint8_t n_adc =0;

void ADC_IRQHandler(void)
{	
	uint8_t V_audio =0;
	int a=0;
	LPC_ADC->ADCR&=~(1<<16); // BURST=0     // Deshabilitamos el modo Ráfaga (ojo continua la conversión del siguiente canal) 
  if(recording == 0)
	{
   //Almacenamos las muestras a modo de ejemplo
   canal_0[n_adc] = ((LPC_ADC->ADDR0 >>4)&0xFFF);	// flag DONE se borra automat. al leer ADDR0  potenciometro
   canal_2[n_adc] = ((LPC_ADC->ADDR2 >>4)&0xFFF);	// flag DONE se borra automat. al leer ADDR2	señal de la bateria
	
	n_adc++;
	if(n_adc == filtro_const)
	{
		n_adc=0;
	}
		for( a = 0; a<filtro_const; a++)
		{
		V_bat += ((double)canal_2[a])/4096 * 3.3*0.95*3.2;
		V_th += ((double)canal_0[a])/4096 * 3.3*0.95 *2.5;
		}
		V_bat = V_bat/filtro_const;
		V_th = V_th/filtro_const;
		
	}else V_audio = 1.3*((LPC_ADC->ADDR4 >>8)&0xFF);	// flag DONE se borra automat. al leer ADDR4  AUDIO
	
		if(recording == 1 && n_muestra < N_muestras)
		{
			muestras[n_muestra] = V_audio; //activar para el ADC
			n_muestra++;
			
		}
		if(n_muestra == N_muestras){
			recording=0;
			n_muestra = 0;
			//devolvemos el ADC a su funcionamiento normal
			   LPC_ADC->ADCR =(1<<0)|(1<<2)            // canales 0,2
                        | (0x01<<8)                   // CLKDIV=1   (Fclk_ADC=25Mhz /(1+1)= 12.5Mhz)
                        | (0x01<<21);                 // PDN=1
			LPC_ADC->ADINTEN=(1<<2);	
			/*devolvemos las prioridades a su funcionamiento normal*/
					NVIC_SetPriority(ADC_IRQn,2);
					NVIC_SetPriority(SysTick_IRQn,0);
					NVIC_SetPriority(ENET_IRQn,0);
					NVIC_SetPriority(UART0_IRQn,0);
					NVIC_SetPriority(TIMER0_IRQn,1);
					NVIC_SetPriority(TIMER1_IRQn,1);
					NVIC_SetPriority(TIMER2_IRQn,1);
					NVIC_SetPriority(TIMER3_IRQn,1);
		}
}


void init_DAC(void)
{
	LPC_PINCON->PINSEL1|= (2<<20); 	 	// DAC output = P0.26 (AOUT)
	LPC_PINCON->PINMODE1|= (2<<20); 	// Deshabilita pullup/pulldown
	LPC_DAC->DACCTRL=0;								//  
}
void init_ADC(void)
{	
   LPC_SC->PCONP|= (1<<12);                           // POwer ON
   LPC_PINCON->PINSEL1|= ((1<<14)|(1<<18));   // (AD0.0, AD0.2)  (P0.23 y P0.25)
	 LPC_PINCON->PINSEL3|= (3<<28);							//P1.30 como AD0.4 
   LPC_PINCON->PINMODE1|= ((2<<14)|(2<<18));           // Deshabilita pullup/pulldown
   LPC_PINCON->PINMODE3|= (2<<28);                    // Deshabilita pullup/pulldown
   LPC_SC->PCLKSEL0|= (0x00<<8);                      // CCLK/4 (Fpclk después del reset) (100 Mhz/4 = 25Mhz)
   LPC_ADC->ADCR|=(1<<0)|(1<<2)|(1<<4)                // canales 0,2,4
                        | (0x01<<8)                   // CLKDIV=1   (Fclk_ADC=25Mhz /(1+1)= 12.5Mhz)
                        | (0x01<<21);                 // PDN=1
   // Ojo no se habilita el modo BURST en la inicialización  
   //LPC_ADC->ADINTEN=(1<<2);					// Hab. interrupción fin de conversión del PENÚLTIMO canal(canal 2)
	 LPC_ADC->ADINTEN=(1<<2);					  //No me funcionaba la configuracion de arriba asi que lo puse en el canal4 porque lo estaba ignorando
   NVIC_EnableIRQ(ADC_IRQn);					// 
   NVIC_SetPriority(ADC_IRQn,2);			   //         
}	


void recto(double L_intro)
{
	if (L_intro >= 0.00)
	{
	L = L_intro;
	L_inicial = L_total;
	velDerecha = 80;
	velIzquierda = 80;
	finish_r = 1;
	}
	else
	{
	L = -L_intro;
	L_inicial = L_total;
	velDerecha = -80;
	velIzquierda = -80;
	finish_r = 1;
	}

}
void G(double O, char dir)
{
	if (dir=='D')
	{
	ang = O;
	angulo_inicial = angulo;
	velDerecha = 0;
	velIzquierda = 80;
	finish_g = 1;
	}
	if(dir == 'I')
	{
	ang = O;
	angulo_inicial = angulo;
	velDerecha = 80;
	velIzquierda = 0;
	finish_g = 1;
	}
}
int func(int n)  //contar digitos en un in
{  
    int counter=0;  
    while(n!=0)  
    {  
        n=n/10;  
        counter++;  
    }  
    return counter;  
}  
void TIMER3_IRQHandler(void)
{
	 int u=strlen(order_ej);
	int j = 0;//borrar
	LPC_TIM3->IR|= (1<<0);
	if((L_total-L_inicial > L) && finish_r == 1)
	{
	velDerecha = 0;
	velIzquierda = 0;
		finish_r = 0;
	}
	if((fabs(angulo - angulo_inicial) > ang) && finish_g == 1)
	{
	velDerecha = 0;
	velIzquierda = 0;
		finish_g = 0;
	}
	
	if(finish_g == 0 && finish_r == 0 && recording==0 && playing==0 && (indice_decod != strlen(order_ej)))
	{
		 
		
		switch(order_ej[indice_decod]){
			case 'R':			//recto
				indice_decod ++;
			if(order_ej[indice_decod] == '-')	//si es una distancia negativa
			{
			indice_decod++;
			recto(-atof(&order_ej[indice_decod]));
				indice_decod = indice_decod + func(atoi(&order_ej[indice_decod]));
			}
			else{
			recto(atof(&order_ej[indice_decod]));
				indice_decod = indice_decod + func(atoi(&order_ej[indice_decod]));
			}
				break;
			case 'D':	//Derecha 90º
				G(1.57,'D');
			indice_decod++;
			break;
			case 'I':	//Izquierda 90º
				G(1.57,'I');
			indice_decod++;
			break;
			case 'P'://Parado
				count_decod++;
			 j = atoi(&order_ej[indice_decod+1])/10e-3; //borrar
				if(count_decod == j)
					{
					indice_decod = indice_decod + func(atoi(&order_ej[indice_decod+1]))+1;
					count_decod=0;
					}
					
			break;
			case 'G'://graba(recording)
					
			/*ponemos el resto de interrupciones a minima prioridad*/
					NVIC_SetPriority(ADC_IRQn,7);
					NVIC_SetPriority(SysTick_IRQn,7);
					NVIC_SetPriority(ENET_IRQn,7);
					NVIC_SetPriority(UART0_IRQn,7);
					NVIC_SetPriority(TIMER0_IRQn,1);
					NVIC_SetPriority(TIMER1_IRQn,7);
					NVIC_SetPriority(TIMER2_IRQn,7);
					NVIC_SetPriority(TIMER3_IRQn,7);
					
					  NVIC_DisableIRQ(ADC_IRQn);					// desactivamos la IRQ del ADC y modificamos sus registros
					
					
					//MODIFICAR el timer0 para ajustar la frecuencia del ADC
					LPC_ADC->ADINTEN=0;
					LPC_ADC->ADCR&=~ (1<<0)|(1<<2);//dejamos solo el canal 4	
					LPC_ADC->ADCR = (1<<4)|		  	  // Canal 4
											 (0x01<<8)|		  	  // CLKDIV=1   (Fclk_ADC=25Mhz /(1+1)= 12.5Mhz)
											 (0x01<<21)|			 	// PDN=1
											 (5<<24);				    // Inicio de conversión con el Match 0.3 del Timer 0
						
					NVIC_EnableIRQ(ADC_IRQn);				//activamos la IRQ del ADC 	
					NVIC_SetPriority(ADC_IRQn,0);		//ponemos la máxima prioridad posible para una grabación correcta
					
					LPC_TIM0->TCR=0x01;		//Start Timer .
					recording = 1;
					LPC_ADC->ADINTEN=(1<<4);
					indice_decod++;
			break;
			case 'A'://reproduce
				LPC_TIM0->TCR=0x01;		//Start Timer .
				playing = 1;
			indice_decod++;
			break;
			default:
				break;
		
	}
		
}
	
		if (velIzquierda >=0.0 )
	{
		/*pines de sentido de giro*/
		LPC_GPIO1->FIOCLR|=(1<<23);
		LPC_GPIO0->FIOSET|=(1<<11);
		/*pines de sentido de giro*/
		setVelIzquierda(velIzquierda);
	}else 
	{
		/*pines de sentido de giro*/
		LPC_GPIO0->FIOCLR|=(1<<11);
		LPC_GPIO1->FIOSET|=(1<<23);
		/*pines de sentido de giro*/
		setVelIzquierda(velIzquierda-2*velIzquierda);
	}
	
	if (velDerecha >= 0.0) //P.3.25
	{
		/*pines de sentido de giro*/
		LPC_GPIO1->FIOCLR|=(1<<25);
		LPC_GPIO0->FIOSET|=(1<<10);
		/*pines de sentido de giro*/
		setVelDerecha(velDerecha);
	}
	else
	{			
		/*pines de sentido de giro*/
		LPC_GPIO0->FIOCLR|=(1<<10);
		LPC_GPIO1->FIOSET|=(1<<25);
		/*pines de sentido de giro*/
		setVelDerecha(velDerecha-2*velDerecha);
	}
	
}
void genera_muestras(uint16_t muestras_ciclo)
{
	uint16_t i;
	//señal senoidal
	for(i=0;i<muestras_ciclo;i++)
	muestras_pitido[i]=(uint8_t)(127+127*sin(2*pi*i/10));
}
void init_TIMER0(void)
{
	  LPC_SC->PCONP|=(1<<1);						// 	Power ON
    LPC_TIM0->PR = 0x0;     	 				//  Prescaler =1
    LPC_TIM0->MCR = 0x03;							//  Reset TC on Match, e interrumpe!  
    LPC_TIM0->MR0 = (Fpclk/F_sample);  // El timer 1 interrumpe cada 125 us 
		LPC_TIM0->MR3 = (Fpclk/F_sample)-1;  // El timer 1 interrumpe cada 125 us 
    LPC_TIM0->EMR = (0x2<<10);   					//  No actúa sobre el HW
    LPC_TIM0->TCR = 0x02;							// Timer STOP y RESET
	    
    
    NVIC_EnableIRQ(TIMER0_IRQn);			//  Habilita NVIC
	  NVIC_SetPriority(TIMER0_IRQn,1);  //  Nivel 1 prioridad   
}
 //PROCEDIMIENTO 1
void init_TIMER1(void)   //CAPTURE 
{
	  LPC_SC->PCONP|=(1<<2);						// 	Power ON
    LPC_TIM1->PR = 0;     	 				//  
    //LPC_TIM1->MR0 = 100e-3*25e6;  // 100ms
    LPC_TIM1->EMR = 0x00;   					//  No actúa sobre el HW
    LPC_TIM1->TCR = 0x01;							//  Start Timer
	
		LPC_PINCON->PINSEL3 |= (3<<4) | (3<<6); //CAP1.0 Y 1.1 en 1.18 y 1.19
		LPC_TIM1->CCR &=~ 0x3F; //pone a 0 los bits del registro
		LPC_TIM1->CCR |= 0x36; // 11_0110
	
    NVIC_EnableIRQ(TIMER1_IRQn);			//  Habilita NVIC
	  NVIC_SetPriority(TIMER1_IRQn,1);  //  Nivel 1 prioridad   
}

 //PROCEDIMIENTO 2
void init_TIMER2(void)   
{
	  LPC_SC->PCONP|=(1<<22);						// 	Power ON
    LPC_TIM2->PR = 24999;     	 				//  Prescaler =1
    LPC_TIM2->MCR = 0x03;							//  Reset TC on Match, e interrumpe!  
    LPC_TIM2->MR0 = 100;  // 100ms
    LPC_TIM2->EMR = 0x00;   					//  No actúa sobre el HW
    LPC_TIM2->TCR = 0x01;							//  Start Timer
    NVIC_EnableIRQ(TIMER2_IRQn);			//  Habilita NVIC
	  NVIC_SetPriority(TIMER2_IRQn,1);  //  Nivel 1 prioridad   
}
//PRACTICA 4
void init_TIMER3(void)   
{
	  LPC_SC->PCONP|=(1<<23);						// 	Power ON
    LPC_TIM3->PR = 24999;     	 				//  Prescaler =1
    LPC_TIM3->MCR = 0x03;							//  Reset TC on Match, e interrumpe!  
    LPC_TIM3->MR0 = 10;  // 10ms
    LPC_TIM3->EMR = 0x00;   					//  No actúa sobre el HW
    LPC_TIM3->TCR = 0x01;							//  Start Timer
    NVIC_EnableIRQ(TIMER3_IRQn);			//  Habilita NVIC
	  NVIC_SetPriority(TIMER3_IRQn,1);  //  Nivel 1 prioridad   
}
///////////////////////////////////////////////////////////////////////
void init_Externas()
{
// Configuración interrupciones externas
  LPC_PINCON->PINSEL4|=(0x00<<24); 		// P2.12 GPIO
	
	LPC_PINCON->PINSEL4|=(0x00<<22); 		// P2.11 GPIO
	
	LPC_PINCON->PINSEL4|=(0x00<<20); 		// P2.10 GPIO

}
///////////////////////////////////////////////////////////////////////////////
void configPWM(void) {
  //1ºpwm  De momento no está en uso
   LPC_SC->PCONP|=(1<<6);
   LPC_PWM1->MR0=Fpclk*Tpwm-1;
   LPC_PWM1->PCR|=(1<<10); //configurado el ENA2 (1.2)
   LPC_PWM1->MCR|=(1<<1);
   LPC_PWM1->TCR|=(1<<0)|(1<<3);

	//2ºpwm
		LPC_PINCON->PINSEL7|=(3<<18); // P3..25 salida PWM (PWM1.2)
   LPC_PWM1->PCR|=(1<<10); //configurado el ENA2 (1.2)

	//3ºpwm
		LPC_PINCON->PINSEL7|=(3<<20); // P3..26 salida PWM (PWM1.3)
   LPC_PWM1->PCR|=(1<<11); //configurado el ENA2 (1.3)
}

void WDT_Feed(void)
{
	LPC_WDT->WDFEED=0xAA;
LPC_WDT->WDFEED=0x55;

}
void init_WDT(void)
{
	LPC_WDT->WDTC=F_wdclk*2; // Timeout=2seg.
	LPC_WDT->WDCLKSEL=0x01;  // Clock=PCLK
	LPC_WDT->WDMOD=0x03;		// Enable y Reset	if Timeout
  LPC_WDT->WDFEED=0xAA;
  LPC_WDT->WDFEED=0x55;
}



/*******************************************************************************
* Function Name  : main
* Description    : Main program
* Input          : None
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
int main(void)
{ 
		int error =0;
	float x,y;
	
	error = uart0_init(19200);
		init ();
	init_TIMER0();
	init_TIMER1();
	init_TIMER2();
	init_TIMER3();
  configPWM();
	configGPIO();
	init_ADC();
	init_DAC();
	init_Externas();
	nunchuk_Init();
	I2Cdelay();					// Para visualizar en simulacion un retardo desde el RESET
	init_WDT();
	genera_muestras(2000);
  screenMain();
	while (1)	
	{
		LPC_WDT->WDFEED=0xAA;
		LPC_WDT->WDFEED=0x55;
		//screenStateMachine();
      timer_poll ();
      main_TcpNet ();
		if(flag_I2C){												//Cada x tiempo el flag se activa
			nunchuk_Init();										//inicializa el nunchuk por si ha sido desconectado
			if(modo == 'M' || modo == 'U')		//sólo modo Manual o DebugUart
			{
			if(nunchuk_read_Tra()==1)					//función de lectura nunchuk i2c
			{
			y = ((float)nunchuk_d.y-(float)0x80)/128;
			x = ((float)nunchuk_d.x-(float)0x80)/128;
			if(y*70 <2.0 && y*70 >-2.0)y=0.0;
			if(x*70 <2.0 && x*70 >-2.0)x=0.0;
			
			if(y>=0.0)
			{
			if(x >= 0.0){
				velDerecha = 70*(y-x)  ; 
				velIzquierda = 70*(y);
			}else{
				velDerecha = 70*(y)  ; 
				velIzquierda = 70*(y+x);		
			}
		}else
			{
				if(x >= 0.0){
				velDerecha = 70*(y+x)  ; 
				velIzquierda = 70*(y);
			}else{
				velDerecha = 70*(y)  ; 
				velIzquierda = 70*(y-x);		
			}
			}
		}
	}
			flag_I2C = 0;
			//__wfi();
			TIM2_1 = 0;		
		
				nunchuk_read_Tra();		//en modo DEBUG sólo lee las variables, no las asigna a nada
			
		
		}
		if(V_bat < V_th)
	{
		pitido=1;
		LPC_TIM0->TCR=0x01;
	}
		
		checkTouchPanel();
		 if (zoneNewPressed(&zone_7)){
			 if (screenState == SCREEN_DEBUG_WAIT){motor_state = 0;}
					act = 1;
					dato = velDerecha;
			}
		 if (zoneNewPressed(&zone_8)){
					if (screenState == SCREEN_DEBUG_WAIT){motor_state = 1;}
					act = 1;
					dato = velIzquierda;
			}
		
      if (zonePressed(&zone_2)){		//zona de reset
        dato = 0;
				dato_f = 0;
				L_izq = 0;
				L_dcha = 0;
				pulsos_dcha_1 =0 ; 
				pulsos_izq_1 =0 ; 
				pulsos_dcha_2 =0 ; 
				pulsos_izq_2 =0 ; 
				angulo =0;
				velDerecha =0;
				velIzquierda=0;
			}
     if (zonePressed(&zone_3)){
        dato --;
		    dato_f -= 0.5;}
     if (zonePressed(&zone_4)){
        dato ++; 
				dato_f += 0.5;	}	 
     if (zoneNewPressed(&zone_5)){
        dato =0;
				dato_f =0;}
     if (zoneNewPressed(&zone_6)){
        dato ++;  
				dato_f += 0.5;}
		 if(dato_f < 5.0)dato_f = 5.0;
		 if(dato_f > 150.0)dato_f = 150.0;
				
				
		 switch(screenState){
			case SCREEN_WELCOME:
			LCD_Clear(Black);
			screenMain();
			screenState = SCREEN_WELCOME_WAIT;
				break;
			
			case SCREEN_WELCOME_WAIT:
			screenState = SCREEN_MANUAL;
				break;
			
			case SCREEN_MANUAL:
				LCD_Clear(Black);
			strncpy(titulo, "MANUAL", 20);	
			screenMain();
			screenState = SCREEN_MANUAL_WAIT;
				break;
			case SCREEN_MANUAL_WAIT:
				if (modo=='A')screenState=SCREEN_AUTOMATICO;
				if (modo=='D')screenState=SCREEN_DEBUG;
			
			sprintf(texto,"vel_1 = %10.3f cm/s", velocidad_1);		
		 GUI_Text(zone_5.x + zone_5.size_x/2 - (strlen(texto)/2)*4, zone_5.y + zone_5.size_y/2 - 27,
             (uint8_t*) texto, White, Black);	
		
								 if(V_bat < V_th)sprintf(texto,"V_bat = %2.3f V BATERIA BAJA!", V_bat);
				 else if (V_bat > V_th +0.2)sprintf(texto,"V_bat = %3.2f V              ", V_bat);				//histeresis de 0.2
		 GUI_Text(zone_5.x + zone_5.size_x/2 - (strlen(texto)/2)*4, zone_5.y + zone_5.size_y/2 - 14,
             (uint8_t*) texto, White, Black);	
		
		sprintf(texto,"v_th = %3.2f V", V_th);		
		 GUI_Text(zone_5.x + zone_5.size_x/2 - (strlen(texto)/2)*4, zone_5.y + zone_5.size_y/2 - 1,
             (uint8_t*) texto, White, Black);	
				break;
			case SCREEN_AUTOMATICO:
				LCD_Clear(Black);
				strncpy(titulo, "AUTOMATICO", 20);
				screenMain();
				screenState = SCREEN_AUTOMATICO_WAIT;
			break;
			case SCREEN_AUTOMATICO_WAIT:
				if (modo=='M')screenState=SCREEN_MANUAL;
				if (modo=='D')screenState=SCREEN_DEBUG;
			memset(texto, 0, 25);
			sprintf(texto,"comando:%s", order_ej);
			if(indice_decod>0)strncpy(texto, texto, strlen(texto)-2);//para que no salga el \n en la pantalla
				GUI_Text(zone_2.x + zone_2.size_x/2 - (strlen(texto)/2)*8, zone_2.y + zone_2.size_y/2 - 8,
             (uint8_t*) texto, White, Black);	
				
			break;
			case SCREEN_DEBUG:
				LCD_Clear(Black);
				strncpy(titulo, "DEBUG", 20);
				screenMain();
			screenState = SCREEN_DEBUG_WAIT;
				
			break;
			case SCREEN_DEBUG_WAIT:
				if (modo=='A')screenState=SCREEN_AUTOMATICO;
				if (modo=='M')screenState=SCREEN_MANUAL;
			screenMessageIP();
			switch(motor_state){
			case 0:
				strncpy(titulo, "velDerecha", 20);
				if (dato<100 && dato>(-100))velDerecha = dato;
				if(dato != dato_prev || act ==1)
				{
				screenMain();
				sprintf(texto,"%20.3f %%", dato);
				GUI_Text(zone_2.x + zone_2.size_x/2 - (strlen(texto)/2)*8, zone_2.y + zone_2.size_y/2 - 8,
             (uint8_t*) texto, White, Black);	
				}
				dato_prev = dato;
				dato = velDerecha;
			
				break;
			case 1:
				strncpy(titulo, "velIzquierda", 20);
				if (dato<100 && dato>(-100))velIzquierda = dato;
			if(dato != dato_prev || act ==1)
			{
				screenMain();
				sprintf(texto,"%20.3f %%", dato);
				GUI_Text(zone_2.x + zone_2.size_x/2 - (strlen(texto)/2)*8, zone_2.y + zone_2.size_y/2 - 8,
             (uint8_t*) texto, White, Black);	
			}
			dato_prev=dato;
			dato = velIzquierda;
				
				break;	
		}
			if(act == 1){
					LCD_Clear(Black);	
					screenMain();
								sprintf(texto,"%20.3f %%", dato);
				GUI_Text(zone_2.x + zone_2.size_x/2 - (strlen(texto)/2)*8, zone_2.y + zone_2.size_y/2 - 8,
             (uint8_t*) texto, White, Black);	
			act=0;
			}
		
				sprintf(texto,"vel_izq = %10.3f cm/s", V_izq_1);		
		 GUI_Text(zone_5.x + zone_5.size_x/2 - (strlen(texto)/2)*4, zone_5.y + zone_5.size_y/2 - 27,
             (uint8_t*) texto, White, Black);	
		
		sprintf(texto,"vel_dcha = %10.3f cm/s", V_dcha_1);		
		 GUI_Text(zone_5.x + zone_5.size_x/2 - (strlen(texto)/2)*4, zone_5.y + zone_5.size_y/2 - 40,
             (uint8_t*) texto, White, Black);
		
								 if(V_bat < V_th)sprintf(texto,"V_bat = %2.3f V BATERIA BAJA!", V_bat);
				 else if (V_bat > V_th +0.2)sprintf(texto,"V_bat = %3.2f V              ", V_bat);				//histeresis de 0.2
		 GUI_Text(zone_5.x + zone_5.size_x/2 - (strlen(texto)/2)*4, zone_5.y + zone_5.size_y/2 - 14,
             (uint8_t*) texto, White, Black);	
		
		sprintf(texto,"v_th = %3.2f V", V_th);		
		 GUI_Text(zone_5.x + zone_5.size_x/2 - (strlen(texto)/2)*4, zone_5.y + zone_5.size_y/2 - 1,
             (uint8_t*) texto, White, Black);	
		
		sprintf(texto,"nunchuk.x= %d", nunchuk_d.x);		
		 GUI_Text(zone_5.x + zone_5.size_x/2 - (strlen(texto)/2)*4, zone_5.y + zone_5.size_y/2 + 13,
             (uint8_t*) texto, White, Black);	
		
		sprintf(texto,"nunchuk.y= %d", nunchuk_d.y);		
		 GUI_Text(zone_5.x + zone_5.size_x/2 - (strlen(texto)/2)*4, zone_5.y + zone_5.size_y/2 + 26,
             (uint8_t*) texto, White, Black);	
						 
		sprintf(texto,"nunchuk.c= %d", nunchuk_d.c);		
		 GUI_Text(zone_5.x + zone_5.size_x/2 - (strlen(texto)/2)*4, zone_5.y + zone_5.size_y/2 + 36,
             (uint8_t*) texto, White, Black);	
		sprintf(texto,"nunchuk.z= %d", nunchuk_d.z);		
		 GUI_Text(zone_5.x + zone_5.size_x/2 - (strlen(texto)/2)*4, zone_5.y + zone_5.size_y/2 + 49,
             (uint8_t*) texto, White, Black);	
		
	}
	}
}




/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
