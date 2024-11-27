/*
 * uartfifo.c
 *
 *  Created on: 1-Oct-2011
 *      Author: J.M.V.C.
 */
#include <LPC17xx.H>
#include <glcd.h>
#include <TouchPanel.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "pantalla_LCD.h"
#include <Net_Config.h>

extern LOCALM localm[];  
#define MY_IP localm[NETIF_ETH].IpAdr

extern char titulo[25];
/*******************************************************************************
* Function Name  : squareButton
* Description    : Dibuja un cuadrado en las coordenadas especificadas colocando 
*                  un texto en el centro del recuadro
* Input          : zone: zone struct
*                  text: texto a representar en el cuadro
*                  textColor: color del texto
*                  lineColor: color de la línea
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
void squareButton(struct t_screenZone* zone, char * text, uint16_t textColor, uint16_t lineColor)
{
   LCD_DrawLine( zone->x, zone->y, zone->x + zone->size_x, zone->y, lineColor);
   LCD_DrawLine( zone->x, zone->y, zone->x, zone->y + zone->size_y, lineColor);
   LCD_DrawLine( zone->x, zone->y + zone->size_y, zone->x + zone->size_x, zone->y + zone->size_y, lineColor);
   LCD_DrawLine( zone->x + zone->size_x, zone->y, zone->x + zone->size_x, zone->y + zone->size_y, lineColor);
	GUI_Text(zone->x + zone->size_x/2 - (strlen(text)/2)*8, zone->y + zone->size_y/2 - 8,
            (uint8_t*) text, textColor, Black);	
}

/*******************************************************************************
* Function Name  : drawMinus
* Description    : Draw a minus sign in the center of the zone
* Input          : zone: zone struct
*                  lineColor
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
void drawMinus(struct t_screenZone* zone, uint16_t lineColor)
{
   LCD_DrawLine( zone->x + 5 , zone->y + zone->size_y/2 - 1, 
                 zone->x + zone->size_x-5, zone->y + zone->size_y/2 - 1,
                 lineColor);
   LCD_DrawLine( zone->x + 5 , zone->y + zone->size_y/2, 
                 zone->x + zone->size_x-5, zone->y + zone->size_y/2,
                 lineColor);
   LCD_DrawLine( zone->x + 5 , zone->y + zone->size_y/2 + 1, 
                 zone->x + zone->size_x-5, zone->y + zone->size_y/2 + 1,
                 lineColor);
}

/*******************************************************************************
* Function Name  : drawMinus
* Description    : Draw a minus sign in the center of the zone
* Input          : zone: zone struct
*                  lineColor
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
void drawAdd(struct t_screenZone* zone, uint16_t lineColor)
{
   drawMinus(zone, lineColor);
   
   LCD_DrawLine( zone->x + zone->size_x/2 - 1,  zone->y + 5 ,
                 zone->x + zone->size_x/2 - 1,  zone->y + zone->size_y - 5, 
                 lineColor);
   LCD_DrawLine( zone->x + zone->size_x/2 ,  zone->y + 5 ,
                 zone->x + zone->size_x/2 ,  zone->y + zone->size_y - 5, 
                 lineColor);
   LCD_DrawLine( zone->x + zone->size_x/2 + 1,  zone->y + 5 ,
                 zone->x + zone->size_x/2 + 1,  zone->y + zone->size_y - 5, 
                 lineColor);
}


/*******************************************************************************
* Function Name  : screenMain
* Description    : Visualiza la pantalla principal
* Input          : None
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
void screenMain(void)
{
   squareButton(&zone_1, titulo, White, Blue);
   squareButton(&zone_2, "                        ", White, Blue);
   drawMinus(&zone_3, White);
   drawAdd(&zone_4, White);
	
	//squareButton(&zone_5, "                        ", White, Blue);
 //  drawMinus(&zone_5, Yellow);
 //  drawAdd(&zone_6, Yellow);
	squareButton(&zone_7, "<", White, Blue);
	squareButton(&zone_8, ">", White, Blue);
}

/*******************************************************************************
* Function Name  : checkTouchPanel
* Description    : Lee el TouchPanel y almacena las coordenadas si detecta pulsación
* Input          : None
* Output         : Modifica pressedTouchPanel
*                    0 - si no se detecta pulsación
*                    1 - si se detecta pulsación
*                        En este caso se actualizan las coordinadas en la estructura display
* Return         : None
* Attention		  : None
*******************************************************************************/
void checkTouchPanel(void)
{
	Coordinate* coord;
	
	coord = Read_Ads7846();
	
	if (coord > 0) {
	  getDisplayPoint(&display, coord, &matrix );
     pressedTouchPanel = 1;
   }   
   else
   {   
     pressedTouchPanel = 0;
      
     // Esto es necesario hacerlo si hay dos zonas diferentes en 
     // dos pantallas secuenciales que se solapen      
     zone_1.pressed = 1;
     zone_2.pressed = 1;
     zone_3.pressed = 1;
     zone_4.pressed = 1;
     zone_5.pressed = 1;
     zone_6.pressed = 1;
     zone_7.pressed = 1;
     zone_8.pressed = 1;
   }  
}

/*******************************************************************************
* Function Name  : zonePressed
* Description    : Detecta si se ha producido una pulsación en una zona contreta
* Input          : zone: Estructura con la información de la zona
* Output         : Modifica zone->pressed
*                    0 - si no se detecta pulsación en la zona
*                    1 - si se detecta pulsación en la zona
* Return         : 0 - si no se detecta pulsación en la zona
*                  1 - si se detecta pulsación en la zona
* Attention		  : None
*******************************************************************************/
int8_t zonePressed(struct t_screenZone* zone)
{
	if (pressedTouchPanel == 1) {


		if ((display.x > zone->x) && (display.x < zone->x + zone->size_x) && 
			  (display.y > zone->y) && (display.y < zone->y + zone->size_y))
      {
         zone->pressed = 1;
		   return 1;
      }   
	}
   
	zone->pressed = 0;
	return 0;
}

/*******************************************************************************
* Function Name  : zoneNewPressed
* Description    : Detecta si se ha producido el flanco de una nueva pulsación en 
*                  una zona contreta
* Input          : zone: Estructura con la información de la zona
* Output         : Modifica zone->pressed
*                    0 - si no se detecta pulsación en la zona
*                    1 - si se detecta pulsación en la zona
* Return         : 0 - si no se detecta nueva pulsación en la zona
*                  1 - si se detecta una nueva pulsación en la zona
* Attention		  : None
*******************************************************************************/
int8_t zoneNewPressed(struct t_screenZone* zone)
{
	if (pressedTouchPanel == 1) {

		if ((display.x > zone->x) && (display.x < zone->x + zone->size_x) && 
			  (display.y > zone->y) && (display.y < zone->y + zone->size_y))
      {
         if (zone->pressed == 0)
         {   
            zone->pressed = 1;
            return 1;
         }
		   return 0;
      }
	}

   zone->pressed = 0;
	return 0;
}
//////////////////////////////////////////////////////////////
void screenWelcome(void)
{
		squareButton(&zone_1, "David Rodriguez Martinez", White, Red);
      //screenMessageIP();
}

////////////////////////////////////////////////////////////////////////

void screenMessageIP()
{     
	char messageText[20];
   sprintf(messageText,"   IP: %d.%d.%d.%d  ", MY_IP[0], MY_IP[1],
                                                       MY_IP[2], MY_IP[3]);
	GUI_Text(zone_9.x + zone_9.size_x/2 - (strlen(messageText)/2)*4, zone_9.y + zone_9.size_y/2,
             (uint8_t*) messageText, Red, Black);	
   //squareButton(&zone_9, messageText, Red, White);
}




 					   					  