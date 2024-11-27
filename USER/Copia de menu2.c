
/* Includes ------------------------------------------------------------------*/
#include "GLCD.h"
#include "TouchPanel.h"
#include <string.h>
#include "leds.h"

#define SCREEN_WELCOME      0
#define SCREEN_WELCOME_WAIT 1
#define SCREEN_MAIN         2
#define SCREEN_MAIN_WAIT    3
#define SCREEN_TOGGLE       4
#define SCREEN_TOGGLE_WAIT  5

uint8_t screenState;   

struct t_screenZone
{
   uint16_t x;         
	 uint16_t y;
	 uint16_t size_x;
	 uint16_t size_y;
	 uint8_t  oldpressed;
};

uint8_t led_1 = 0;
uint8_t led_2 = 0;

struct t_screenZone zone_0 = { 20,  20, 200, 110, 0};
struct t_screenZone zone_1 = { 20,  20, 200,  50, 0};
struct t_screenZone zone_2 = { 20,  80, 200,  50, 0};
struct t_screenZone zone_3 = { 20, 140, 200,  50, 0};
struct t_screenZone zone_4 = { 40, 200,  60,  60, 0};
struct t_screenZone zone_5 = {140, 200,  60,  60, 0};


void squareButton(uint16_t x, uint16_t y, uint16_t size_x, uint16_t size_y, char * text, uint16_t textColor, uint16_t lineColor)
{
   LCD_DrawLine( x, y, x + size_x, y, lineColor);
   LCD_DrawLine( x, y, x, y + size_y, lineColor);
   LCD_DrawLine( x, y + size_y, x + size_x, y + size_y, lineColor);
   LCD_DrawLine( x + size_x, y, x + size_x, y + size_y, lineColor);
	 GUI_Text(x + size_x/2 - (strlen(text)/2)*8, y + size_y/2 - 8, (uint8_t*) text, textColor, Black);
	
}

void screenWelcome()
{
		squareButton(zone_0.x, zone_0.y, zone_0.size_x, zone_0.size_y, "Ejemplo de los LEDS", White, Red);
}

void screenMain()
{
		squareButton(zone_1.x, zone_1.y, zone_1.size_x, zone_1.size_y, "Enciende LEDs", White, Blue);
		squareButton(zone_2.x, zone_2.y, zone_2.size_x, zone_2.size_y, "Apaga LEDs", White, Blue);
		squareButton(zone_3.x, zone_3.y, zone_3.size_x, zone_3.size_y, "Conmuta LEDs", White, Blue);

  	squareButton(zone_4.x, zone_4.y, zone_4.size_x, zone_4.size_y, " ", White, Blue);
		squareButton(zone_5.x, zone_5.y, zone_5.size_x, zone_5.size_y, " ", White, Blue);
}

void screenToggle()
{
		squareButton(zone_0.x, zone_0.y, zone_0.size_x, zone_0.size_y, "Pulsa en el LED", White, Blue);
		squareButton(zone_3.x, zone_3.y, zone_3.size_x, zone_3.size_y, "Volver", White, Blue);

  	squareButton(zone_4.x, zone_4.y, zone_4.size_x, zone_4.size_y, " ", White, Blue);
		squareButton(zone_5.x, zone_5.y, zone_5.size_x, zone_5.size_y, " ", White, Blue);
}
uint8_t pressedTouchPanel = 0;
uint8_t newPressedTouchPanel = 0;

void checkTouchPanel(void)
{
	Coordinate* coord;
	
	coord = Read_Ads7846();
	
	if (coord > 0) {
		getDisplayPoint(&display, coord, &matrix );
      if (pressedTouchPanel == 0)
         newPressedTouchPanel = 1;
      pressedTouchPanel = 1;
   }   
   else
      pressedTouchPanel = 0;
}

int8_t zonePressed(struct t_screenZone* zone)
{
	
	if (pressedTouchPanel == 1) {

		if ((display.x > zone->x) && (display.x < zone->x + zone->size_x) && 
			  (display.y > zone->y) && (display.y < zone->y + zone->size_y))
		   return 1;
	}
	
	return 0;
}


void fillRect(struct t_screenZone* zone, uint16_t color)
{
	uint16_t i;
	
	for (i = zone->y+1; i < zone->y + zone->size_y-1; i ++) {
		LCD_DrawLine(zone->x + 1, i, zone->x + zone->size_x -1, i, color);
	}
}


void updateLEDs(void)
{
	if (led_1 ==1) {
		fillRect(&zone_4,Red);
		LED1_ON;
  }
	else {
		fillRect(&zone_4,Blue);
		LED1_OFF;
  }
	if (led_2 ==1) {
		fillRect(&zone_5,Red);
		LED2_ON;
  }
	else {
		fillRect(&zone_5,Blue);
		LED2_OFF;
  }
}

void 	initScreenStateMachine(void)
{
	screenState = SCREEN_WELCOME;
}


void screenStateMachine(void)
{ 
	
    uint8_t newPressedZone3 = 0;
    uint8_t newPressedZone4 = 0;
    uint8_t newPressedZone5 = 0;
    static uint8_t oldPressedZone3 = 0;
    static uint8_t oldPressedZone4 = 0;
    static uint8_t oldPressedZone5 = 0;

   checkTouchPanel();

	
		switch (screenState)
		{
		   case SCREEN_WELCOME:
				  LCD_Clear(Black);
				  screenWelcome();
			    screenState = SCREEN_WELCOME_WAIT;
			    break;
			 
		   case SCREEN_WELCOME_WAIT:
				  if (zonePressed(&zone_0)) {
						 screenState = SCREEN_MAIN;
					}					
			    break;
			 
			 case SCREEN_MAIN:
				  LCD_Clear(Black);
				  screenMain();
			    updateLEDs();
			    screenState = SCREEN_MAIN_WAIT;
				  break;
			 
			 case SCREEN_MAIN_WAIT:
				  if (zonePressed(&zone_1)) {
						 led_1 = 1;
						 led_2 = 1;
             updateLEDs();					
					}					
				  if (zonePressed(&zone_2)) {
						 led_1 = 0;
						 led_2 = 0;
             updateLEDs();					
					}					

          newPressedZone3 = zonePressed(&zone_3);
			    if ((newPressedZone3 == 1) && (oldPressedZone3 == 0)) {
						 screenState = SCREEN_TOGGLE;
					}
          oldPressedZone3 = newPressedZone3;

				  break;
					
			 case SCREEN_TOGGLE:
				  LCD_Clear(Black);
				  screenToggle();
			    updateLEDs();
			    screenState = SCREEN_TOGGLE_WAIT;
				  break;
			 
			 case SCREEN_TOGGLE_WAIT:
          				 
          newPressedZone3 = zonePressed(&zone_3);
			    if ((newPressedZone3 == 1) && (oldPressedZone3 == 0)) {
						 screenState = SCREEN_MAIN;
					}
          oldPressedZone3 = newPressedZone3;
					
          newPressedZone4 = zonePressed(&zone_4);
			    if ((newPressedZone4 == 1) && (oldPressedZone4 == 0)) {
						 led_1 ^= 0x01;
	           updateLEDs();											
					}
          oldPressedZone4 = newPressedZone4;
			 
          newPressedZone5 = zonePressed(&zone_5);
			    if ((newPressedZone5 == 1) && (oldPressedZone5 == 0)) {
						 led_2 ^= 0x01;
	           updateLEDs();											
					}
          oldPressedZone5 = newPressedZone5;

				  break;
			 default:
			    break;
		}
}

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
