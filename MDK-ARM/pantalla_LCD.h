/*
 * uartfifo.h
 *
 */

#include <LPC17xx.h>
/* Estructura que define una zona de la pantalla */
struct t_screenZone
{
   uint16_t x;         
	uint16_t y;
	uint16_t size_x;
	uint16_t size_y;
	uint8_t  pressed;
};

/* Definicion de las diferentes zonas de la pantalla */
struct t_screenZone zone_1 = { 36,  20, 163,  50, 0}; /* Mensaje   */
struct t_screenZone zone_2 = { 20,  80, 200,  50, 0}; /* Visualizacion de datos       */
struct t_screenZone zone_3 = { 40, 140,  60,  60, 0}; /* Botón  Disminuir              */
struct t_screenZone zone_4 = {140, 140,  60,  60, 0}; /* Botón  Aumentar             */
struct t_screenZone zone_5 = { 40, 220,  60,  60, 0}; /* Botón  Aumentar con flanco   */
struct t_screenZone zone_6 = {140, 220,  60,  60, 0}; /* Botón  Disminuir con flanco   */
struct t_screenZone zone_7 = {5, 20,  30,  50, 0}; /* Botón  cambiar ventana izq   */
struct t_screenZone zone_8 = {200, 20,  30,  50, 0}; /* Botón  cambiar ventana dcha  */
struct t_screenZone zone_9 = {15 ,45 ,  140,  20, 0}; /* Botón  cambiar ventana dcha  */

/* Flag que indica si se detecta una pulsación válida */
uint8_t pressedTouchPanel = 0;


void squareButton(struct t_screenZone* zone, char * text, uint16_t textColor, uint16_t lineColor);
void drawMinus(struct t_screenZone* zone, uint16_t lineColor);
void drawAdd(struct t_screenZone* zone, uint16_t lineColor);
void screenMain(void);
void checkTouchPanel(void);
void screenMessageIP(void);
int8_t zonePressed(struct t_screenZone* zone);
int8_t zoneNewPressed(struct t_screenZone* zone);


