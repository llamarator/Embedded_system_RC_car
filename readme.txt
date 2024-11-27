Este proyecto de ejemplo complementa el ejemplo de implementación de un menú
utilizando máquina de estados con la posibilidad de cambiar el estado de los LEDs
por una página web.

Por defecto está configurada la posibilidad de depurar utilizando una comunicación serie utilizando la
un canal ITM interno de depuración (variable de entorno __DBG_ITM). Se podría configurar para enviar datos 
por la __UART0 o __UART1.

Por defecto está desconfigurada la posibilidad de depuración de las comunicaciones. Para activarlo habría
que deshabilitar el fichero TCP_CM3.lib y habilitar TCPD_CM3.lib y Net_Debug.c. La información de depuración
saldría por el canal configurado como STDOUT.