Este proyecto de ejemplo complementa el ejemplo de implementaci�n de un men�
utilizando m�quina de estados con la posibilidad de cambiar el estado de los LEDs
por una p�gina web.

Por defecto est� configurada la posibilidad de depurar utilizando una comunicaci�n serie utilizando la
un canal ITM interno de depuraci�n (variable de entorno __DBG_ITM). Se podr�a configurar para enviar datos 
por la __UART0 o __UART1.

Por defecto est� desconfigurada la posibilidad de depuraci�n de las comunicaciones. Para activarlo habr�a
que deshabilitar el fichero TCP_CM3.lib y habilitar TCPD_CM3.lib y Net_Debug.c. La informaci�n de depuraci�n
saldr�a por el canal configurado como STDOUT.