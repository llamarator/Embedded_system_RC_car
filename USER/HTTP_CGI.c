/*----------------------------------------------------------------------------
 *      RL-ARM - TCPnet
 *----------------------------------------------------------------------------
 *      Name:    HTTP_CGI.C
 *      Purpose: HTTP Server CGI Module
 *      Rev.:    V4.22
 *----------------------------------------------------------------------------
 *      This code is part of the RealView Run-Time Library.
 *      Copyright (c) 2004-2011 KEIL - An ARM Company. All rights reserved.
 *---------------------------------------------------------------------------*/

#include <Net_Config.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
extern int indice_decod;
extern float velDerecha;
extern float velIzquierda;
extern float dato;
/* ---------------------------------------------------------------------------
 * The HTTP server provides a small scripting language.
 *
 * The script language is simple and works as follows. Each script line starts
 * with a command character, either "i", "t", "c", "#" or ".".
 *   "i" - command tells the script interpreter to "include" a file from the
 *         virtual file system and output it to the web browser.
 *   "t" - command should be followed by a line of text that is to be output
 *         to the browser.
 *   "c" - command is used to call one of the C functions from the this file.
 *         It may be followed by the line of text. This text is passed to
 *         'cgi_func()' as a pointer to environment variable.
 *   "#' - command is a comment line and is ignored (the "#" denotes a comment)
 *   "." - denotes the last script line.
 *
 * --------------------------------------------------------------------------*/


extern char modo;

/* http_demo.c */
extern U16 AD_in (U32 ch);
extern U8  get_button (void);

/* at_System.c */
extern  LOCALM localm[];
#define LocM   localm[NETIF_ETH]

/* Net_Config.c */
extern struct tcp_cfg   tcp_config;
extern struct http_cfg  http_config;
#define tcp_NumSocks    tcp_config.NumSocks
#define tcp_socket      tcp_config.Scb
#define http_EnAuth     http_config.EnAuth
#define http_auth_passw http_config.Passw

extern BOOL LEDrun;
extern void LED_out (U32 val);
extern BOOL LCDupdate;
extern U8   lcd_text[2][16+1];

extern double V_bat;
extern char order_ej[15];
/* Local variables. */
static char const state[][9] = {
  "FREE",
  "CLOSED",
  "LISTEN",
  "SYN_REC",
  "SYN_SENT",
  "FINW1",
  "FINW2",
  "CLOSING",
  "LAST_ACK",
  "TWAIT",
  "CONNECT"};

/* My structure of CGI status U32 variable. This variable is private for */
/* each HTTP Session and is not altered by HTTP Server. It is only set to  */
/* zero when the cgi_func() is called for the first time.                  */
typedef struct {
  U16 xcnt;
  U16 unused;
} MY_BUF;
#define MYBUF(p)        ((MY_BUF *)p)

/*----------------------------------------------------------------------------
 * HTTP Server Common Gateway Interface Functions
 *---------------------------------------------------------------------------*/

/*--------------------------- cgi_process_var -------------------------------*/

void cgi_process_var (U8 *qs) {
  /* This function is called by HTTP server to process the Querry_String   */
  /* for the CGI Form GET method. It is called on SUBMIT from the browser. */
  /*.The Querry_String.is SPACE terminated.                                */
  U8 *var;
  var = (U8 *)alloc_mem (40);
  do {
    /* Loop through all the parameters. */
    qs = http_get_var (qs, var, 40);
    /* Check the returned string, 'qs' now points to the next. */
    if (var[0] != 0) {
      /* Returned string is non 0-length. */
			if (str_scomp (var, "modo=MANUAL") == __TRUE) {
         modo = 'M';
      }
			else if (str_scomp (var, "modo=AUTOMATICO") == __TRUE) {
         modo = 'A';
      }
			else if (str_scomp (var, "modo=DEBUG") == __TRUE) {
         modo = 'D';
      }
			else if (str_scomp (var, "command=") == __TRUE) {
				memset(order_ej, 0, 14);
         strncpy((char*)order_ej,(var+8),10);
				 indice_decod=0;
      }
			else if (str_scomp (var, "motor_izq=") == __TRUE) {
				velIzquierda = atof((char*)var +10);
				dato=velIzquierda;
      }
			else if (str_scomp (var, "motor_dcha=") == __TRUE) {
				velDerecha = atof((char*)var +11);
				dato = velDerecha;
      }
			
   }
  }while (qs);

  free_mem ((OS_FRAME *)var);
}


/*--------------------------- cgi_process_data ------------------------------*/

void cgi_process_data (U8 code, U8 *dat, U16 len) {
  /* This function is called by HTTP server to process the returned Data    */
  /* for the CGI Form POST method. It is called on SUBMIT from the browser. */
  /* Parameters:                                                            */
  /*   code  - callback context code                                        */
  /*           0 = www-url-encoded form data                                */
  /*           1 = filename for file upload (0-terminated string)           */
  /*           2 = file upload raw data                                     */
  /*           3 = end of file upload (file close requested)                */
  /*           4 = any xml encoded POST data (single or last stream)        */
  /*           5 = the same as 4, but with more xml data to follow          */
  /*               Use http_get_content_type() to check the content type    */  
  /*   dat   - pointer to POST received data                                */
  /*   len   - received data length                                         */
	/**************************************************************************/
	
 
  switch (code) {
    case 0:
      /* Url encoded form data received. */
      break;

    default:
      /* Ignore all other codes. */
      return;
  }

 }


/*--------------------------- cgi_func --------------------------------------*/

U16 cgi_func (U8 *env, U8 *buf, U16 buflen, U32 *pcgi) {
  /* This function is called by HTTP server script interpreter to make a    */
  /* formated output for 'stdout'. It returns the number of bytes written   */
  /* to the output buffer. Hi-bit of return value (len is or-ed with 0x8000)*/
  /* is a repeat flag for the system script interpreter. If this bit is set */
  /* to 1, the system will call the 'cgi_func()' again for the same script  */
  /* line with parameter 'pcgi' pointing to a 4-byte buffer. This buffer    */
  /* can be used for storing different status variables for this function.  */
  /* It is set to 0 by HTTP Server on first call and is not altered by      */
  /* HTTP server for repeated calls. This function should NEVER write more  */
  /* than 'buflen' bytes to the buffer.                                     */
  /* Parameters:                                                            */
  /*   env    - environment variable string                                 */
  /*   buf    - HTTP transmit buffer                                        */
  /*   buflen - length of this buffer (500-1400 bytes - depends on MSS)     */
  /*   pcgi   - pointer to session local buffer used for repeated loops     */
  /*            This is a U32 variable - size is 4 bytes. Value is:         */
  /*            - on 1st call = 0                                           */
  /*            - 2nd call    = as set by this function on first call       */
  U32 len = 0;
	uint8_t A,M,D;									//estas 3 variables indican el modo actual del sistema
	if(modo == 'A')A=1; else A=0;			
	if(modo == 'M')M=1; else M=0;
	if(modo == 'D')D=1; else D=0;

  switch (env[0]) {
    /* Analyze the environment string. It is the script 'c' line starting */
    /* at position 2. What you write to the script file is returned here. */
		case 'g':
			switch(env[2]) {
				case '1':
					len = sprintf((char *)buf,(const char *)&env[4],(float)V_bat); //adc);
				break;
				default:
					break;
			}
			break;
    case 'a' :
      switch (env[2]) {
				case '5':
          /* Write local Net mask. */
          len = sprintf((char *)buf,(const char *)&env[4],M ? "checked" : "");
          break;
				case '6':
          /* Write local Net mask. */
          len = sprintf((char *)buf,(const char *)&env[4],A ? "checked" : "");
          break;
				case '7':
          /* Write local Net mask. */
          len = sprintf((char *)buf,(const char *)&env[4],D ? "checked" : "");
          break;
				
        default:
           break;
      }
      break;
      
    default:
       break;

  }
  return ((U16)len);
}


/*----------------------------------------------------------------------------
 * end of file
 *---------------------------------------------------------------------------*/

