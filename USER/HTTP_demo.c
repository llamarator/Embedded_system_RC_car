/*----------------------------------------------------------------------------
 *      RL-ARM - TCPnet
 *----------------------------------------------------------------------------
 *      Name:    HTTP_DEMO.C
 *      Purpose: HTTP Server demo example
 *----------------------------------------------------------------------------
 *      This code is part of the RealView Run-Time Library.
 *      Copyright (c) 2004-2011 KEIL - An ARM Company. All rights reserved.
 *----------------------------------------------------------------------------
 *      Modified: JPM (UAH) 16/08/2017
 *                Compatibility for Mini-DK2 board.
 *                Web server files in SD Card.
 *                Option for retarget STDOUT
 *---------------------------------------------------------------------------*/


#include <stdio.h>
#include <RTL.h>
#include <Net_Config.h>
#include <LPC17xx.h>                    /* LPC17xx definitions               */
#include <GLCD.h>
#include <serial.h>
#include <File_Config.h>


BOOL LEDrun;
BOOL LCDupdate;
BOOL tick;
U32  dhcp_tout;
U8   lcd_text[2][16+1] = {" ",                /* Buffer for LCD text         */
                          "Waiting for DHCP"};

extern LOCALM localm[];                       /* Local Machine Settings      */
#define MY_IP localm[NETIF_ETH].IpAdr
#define DHCP_TOUT   50                        /* DHCP timeout 5 seconds      */

static void init_io (void);
static void init_display (void);
static void init_card (void);
static void init_stdout(void);

/*--------------------------- init ------------------------------------------*/

static void init () {
  /* Add System initialisation code here */ 

  init_io ();
  SER_Init ();
  init_display ();
  init_TcpNet ();
  init_stdout();
  init_card ();

  /* Setup and enable the SysTick timer for 100ms. */
  SysTick->LOAD = (SystemCoreClock / 10) - 1;
  SysTick->CTRL = 0x05;
}

/*--------------------------- timer_poll ------------------------------------*/

static void timer_poll () {
  /* System tick timer running in poll mode */

  if (SysTick->CTRL & 0x10000) {
    /* Timer tick every 100 ms */
    timer_tick ();
    tick = __TRUE;
  }
}

/*--------------------------- init_io ---------------------------------------*/

static void init_io () {

  /* Configure the GPIO for Push Buttons */
  LPC_PINCON->PINSEL4 &= 0xFFCFFFFF;
  LPC_GPIO2->FIODIR   &= 0xFFFFFBFF;

  /* Configure the GPIO for LEDs. */
  LPC_GPIO3->FIODIR   |= (1<<25)|(1<<26);

  /* Configure AD0.2 input. */
  LPC_PINCON->PINSEL1 &= 0xFFF3FFFF;
  LPC_PINCON->PINSEL1 |= 0x00040000;
  LPC_SC->PCONP       |= 0x00001000;
  LPC_ADC->ADCR        = 0x00200404;               /* ADC enable, ADC clock 25MHz/5, select AD0.2 pin */
}

/*--------------------------- LED_out ---------------------------------------*/

void LED_out (U32 val) {
  const U8 led_pos[2] = {25, 26};
  U32 i,mask;

  for (i = 0; i < 2; i++) {
    mask = 1 << led_pos[i];
    if (val & (1<<i)) {
      LPC_GPIO3->FIOCLR = mask;
    }
    else {
      LPC_GPIO3->FIOSET = mask;
    }
  }
}


/*--------------------------- AD_in -----------------------------------------*/

U16 AD_in (U32 ch) {
  /* Read ARM Analog Input */
  U32 val = 0;
  U32 adcr;

  if (ch < 8) {
    adcr = 0x01000000 | (1 << ch);
    LPC_ADC->ADCR = adcr | 0x00200100;        /* Setup A/D: 10-bit @ 9MHz  */

    do {
      val = LPC_ADC->ADGDR;                   /* Read A/D Data Register    */
    } while ((val & 0x80000000) == 0);        /* Wait for end of A/D Conv. */
    LPC_ADC->ADCR &= ~adcr;                   /* Stop A/D Conversion       */
    val = (val >> 6) & 0x03FF;                /* Extract AINx Value        */
  }
  return (val);
}


/*--------------------------- get_button ------------------------------------*/

U8 get_button (void) {
  /* Read ARM Digital Input */
  U32 val = 0;

  if ((LPC_GPIO2->FIOPIN & (1 << 10)) == 0) {
    /* INT0 button */
    val |= 0x01;
  }
  if ((LPC_GPIO2->FIOPIN & (1 << 11)) == 0) {
    /* Joystick left */
    val |= 0x02;
  }
  if ((LPC_GPIO2->FIOPIN & (1 << 12)) == 0) {
    /* Joystick right */
    val |= 0x04;
  }

  return (val);
}


/*--------------------------- upd_display -----------------------------------*/

static void upd_display () {
  /* Update GLCD Module display text. */

   LCD_Clear(Black);
   GUI_Text(60,144,lcd_text[0],White,Red);
   GUI_Text(52,160,lcd_text[1],White,Red);

   LCDupdate =__FALSE;
}


/*--------------------------- init_display ----------------------------------*/

static void init_display () {
  /* LCD Module init */
	
  LCD_Initializtion();
  LCD_Clear(Black);
  GUI_Text(60,144, "   RL-ARM2",White,Red);
  GUI_Text(52,160, "HTTP example",White,Red);
 
}


/*--------------------------- dhcp_check ------------------------------------*/

static void dhcp_check () {
  /* Monitor DHCP IP address assignment. */

  if (tick == __FALSE || dhcp_tout == 0) {
    return;
  }
  if (mem_test (&MY_IP, 0, IP_ADRLEN) == __FALSE && !(dhcp_tout & 0x80000000)) {
    /* Success, DHCP has already got the IP address. */
    dhcp_tout = 0;
    sprintf((char *)lcd_text[0]," IP address:");
    sprintf((char *)lcd_text[1]," %d.%d.%d.%d", MY_IP[0], MY_IP[1],
                                                MY_IP[2], MY_IP[3]);
    LCDupdate = __TRUE;
    return;
  }
  if (--dhcp_tout == 0) {
    /* A timeout, disable DHCP and use static IP address. */
    dhcp_disable ();
    sprintf((char *)lcd_text[1]," DHCP failed    " );
    LCDupdate = __TRUE;
    dhcp_tout = 30 | 0x80000000;
    return;
  }
  if (dhcp_tout == 0x80000000) {
    dhcp_tout = 0;
    sprintf((char *)lcd_text[0]," IP address:");
    sprintf((char *)lcd_text[1]," %d.%d.%d.%d", MY_IP[0], MY_IP[1],
                                                MY_IP[2], MY_IP[3]);
    LCDupdate = __TRUE;
  }
}


/*--------------------------- blink_led -------------------------------------*/

static void blink_led () {
  /* Blink the LEDs on an eval board */

   const U8 led_val[2] = { 0x01,0x02 };

//   const U8 led_val[16] = { 0x48,0x88,0x84,0x44,0x42,0x22,0x21,0x11,
//                           0x12,0x0A,0x0C,0x14,0x18,0x28,0x30,0x50 };
  static U32 cnt = 0;

  if (tick == __TRUE) {
    /* Every 100 ms */
    tick = __FALSE;
    if (LEDrun == __TRUE) {
      LED_out (led_val[cnt]);
      if (++cnt >= sizeof(led_val)) {
        cnt = 0;
      }
    }
    if (LCDupdate == __TRUE) {
      upd_display ();
    }
  }
}


/*---------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 *        Initialize a Flash Memory Card
 *---------------------------------------------------------------------------*/
static void init_card (void) {
  U32 retv;

  while ((retv = finit (NULL)) != 0) {        /* Wait until the Card is ready*/
    if (retv == 1) {
       sprintf((char *)lcd_text[0]," Inserte Tarjeta");
       sprintf((char *)lcd_text[1],"                ");
//       LCD_Clear(Black);
       GUI_Text(60,144, lcd_text[0],White,Red);
       GUI_Text(52,160, lcd_text[1],White,Red);

    }
    else {
       sprintf((char *)lcd_text[0]," Tarjeta SD sin formato");
       sprintf((char *)lcd_text[1],"                     ");
       GUI_Text(60,144, lcd_text[0],White,Red);
       GUI_Text(52,160, lcd_text[1],White,Black);
    }
  }
  LCDupdate = __TRUE;

}
extern   U32 CheckMedia (void);
void sd_check(void)
{
   static BOOL no_sd = 0;
   BOOL check;
   
   if ((check = CheckMedia()) & !no_sd)
      return;
   else if (check)
   {   
      init_card();
      sprintf((char *)lcd_text[0]," IP address:");
      sprintf((char *)lcd_text[1]," %d.%d.%d.%d", MY_IP[0], MY_IP[1],
                                                  MY_IP[2], MY_IP[3]);
      no_sd = 0;
      return;
   }
   if (no_sd == 0)
   {   
     sprintf((char *)lcd_text[0]," Inserte Tarjeta");
     sprintf((char *)lcd_text[1],"                ");
     LCDupdate = __TRUE;
   }
   no_sd = 1;   
      

}


FILE* _stdout;

static void init_stdout(void)
{
  _stdout=fopen("STDOUT","w");
}


int main (void) {
  /* Main Thread of the TcpNet */

  init ();
   
  LEDrun = __TRUE;
  dhcp_tout = DHCP_TOUT;
   
  fprintf(_stdout,"Informacion de depuracion\n");
  fprintf(_stdout,"Inicio del programa\n");
   
  while (1) {
    timer_poll ();
    sd_check(); 
    main_TcpNet ();
    dhcp_check ();
    blink_led ();
  }
}





/*----------------------------------------------------------------------------
 * end of file
 *---------------------------------------------------------------------------*/
