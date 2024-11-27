t <!DOCTYPE html>
t     <html>
t       <head>
t         <meta content="text/html; charset=UTF-8" http-equiv="content-type">
t         <meta http-equiv="refresh" content="20; url=http://192.168.1.70/index.cgi">
t         <title>Control de LEDs</title>
t       </head>
t       <body> 
t         <form id="formulario" action="/index.cgi" method="get"> 
t           <table style="width: 200px" border="0">
t             <tbody>
t               <tr align="center">
t                 <td border="1" style="width: 100%">
t                   <div style="font-weight:bold">
t                     Seleccion Modo
t                   </div>
t                 </td>
t               </tr>
t               <tr align="center">
t                 <td border="1" style="width: 100%">
c a 5 				<input type="radio" id="manual" name="modo" value="MANUAL"onclick="submit()" %s>
t  				<label for="manual">MANUAL</label><br>
c a 6  				<input type="radio" id="automatico" name="modo" value="AUTOMATICO"onclick="submit()" %s>
t  				<label for="automatico">AUTOMATICO</label><br>
c a 7  				<input type="radio" id="debug" name="modo" value="DEBUG"onclick="submit()" %s>
t  				<label for="debug">DEBUG</label>
t                 </td>
t               </tr>
t               <tr align="center">
t                <td border="1" style="width: 100%">
t                   <label for="username">Comando:</label>
t                  <input type="text" id="com" name="command" value="P20">
t                   <br>
t                  <input type="submit" value="Enviar">
t                </td>
t              </tr>
t              <td align="center">
t              <label for="username">V bateria:</label>
t               <input type="text" readonly style="background-color: transparent; border: 0px"
c g 1           size="10" id="ad_volts" value="%5.3f V">
t               <tr align="center">
t                <td border="1" style="width: 100%">
t                   <label for="username">Motor_Izq:</label>
t                  <input type="text" id="m_i" name="motor_izq" value="0">
t                   <br>
t                  <input type="submit" value="Enviar">
t                </td>
t               </tr>
t               <tr align="center">
t                <td border="1" style="width: 100%">
t                   <label for="username">Motor_dcha:</label>
t                  <input type="text" id="m_d" name="motor_dcha" value="0">
t                   <br>
t                  <input type="submit" value="Enviar">
t                </td>
t               </tr>
t             </tbody>
t           </table>
t         </form>
t         <br>
t       </body>
t     </html>
.