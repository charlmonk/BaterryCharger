/*
* Este programa se desarrollo con la finalidad de controlar
* el ciclo de operación de un cargador de baterias genérico
* al mismo se le configurarán 3 variables:
*    - Tiempo de carga TienpoCarga
*    - Tiempo de reposo TiempoReposo
*    - Número de ciclos NroCiclos
* estos parámetros permitirán repetir los ciclos de forma 
* automáticas y mejorar así el proceso de carga y reducir
* el tererioro de una carga prolongada le hace a una batería
* de plomo automotriz.
*
* Desarrollador: Carlos Rangel
* fecha: 16/06/2020
* Revisión: V.1.0
*/ 

#include <Arduino.h>
#include <PinButton.h>
#include <TimerOne.h>
#include <TM1637Display.h>

/**************************
* Definición de Variables *
***************************/

/*********Display TM1637 *************/
unsigned char pinClk = 9;         // Pin de Clock con display TM1637
unsigned char pinDIO = 10;        // Pin de Datos con display TM1637
unsigned char brightness = 3;    // Variable para intensidad del display del 0 al 7
//unsigned int printDelay = 800;    // Retardo del scroll del display en ms
//bool DosPuntos = true;            // Estado de los 2 puntos del display
unsigned char pos = 1;

/*********** Configuración predefinida del cargador ************/
unsigned char TiempoCarga = 30;   // Variable de tiempo de carga
unsigned char TiempoReposo = 30;  // Variable de tiempo de reposo
unsigned char NroCiclos = 2;      // Número de ciclos de carga

/**************** Pines de salida del Arduino *******************/
unsigned char rele = 12;          // Pin de salida que conttrola el rele de carga
unsigned char led = LED_BUILTIN;  // Variable reservada para el pin 13 con led interno

/*********** Banderas de los diferentes menus de configuración **********/
bool EstadoLed = false;           // Estado del led en pin 13
bool MenuTcarga = false;          // Bandera de evento de entrada a Cfg (tiempo de carga)
bool MenuTreposo = false;         // Bandera de evento de entrada al segundo nivel de Cfg (tiempo de reposo)
bool MenuNciclos = false;         // Bandera DE evento de entrada al tercer nivel de Cfg (número de ciclos)
bool Arranque = false;            // Bandera de evento entrada a Run
bool FlagEst[] = { MenuTcarga, MenuTreposo, MenuNciclos, Arranque };                 // Banderas acomodadas en un array : MenuCarga, MenuPausa, MenuCiclos, Arranque

/*********** Función de interupción del contador ***************/
int conteo = -1;                  // Variable de los segundos del contador
unsigned char ContMin = 0;        // Variable de registro de minutos transcurridos

/************** Variables auxiliares ********************/
bool EstReposo = false;           // Estado del reposo para diferenciar de la carga
//unsigned char varTiempo;
//unsigned char Tcarga;
//unsigned char Treposo;
unsigned char Nciclo;
//char varCfg [3] = {30,30,2};      // Arreglo con las variables de configuración : T Carga, T Pausa, N Ciclos
bool Estado = false;              // Variable de estado del cargador

const uint8_t SEG_CARGA[] = {SEG_A};
const uint8_t SEG_REPOSO[] = {SEG_G};
const uint8_t SEG_CICLO[] = {SEG_D};

const uint8_t SEG_HOLA[] = {
  SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,           // H
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
	SEG_D | SEG_E | SEG_F,                           // L
  SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,   // A
  };

const uint8_t SEG_DONE[] = {
	SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
	SEG_C | SEG_E | SEG_G,                           // n
	SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
	};

// Creación de los objetos basados en las librerías

PinButton BotonCfg(6);
PinButton BotonEntrada(7);
PinButton BotonSubir(4);
PinButton BotonBajar(5);
TM1637Display display(pinClk, pinDIO);

/*
* Muestra en el display el valor de los minutos restantes en el ciclo de
* automatico, dependiendo de si esta cargando o en reposo, se ver la 
* diferencia dependiendo de la ubicación el conteo, si aparece a la 
* izquierda esta cargando y si aparece a la derecha esta en reposo.
*/
void Visualizar(void)  
{  
  if (!EstReposo && FlagEst[4]) 
  {
    display.clear();
    display.showNumberDec((TiempoCarga - ContMin), false, 2, 0);
  }  
  if (EstReposo && FlagEst[4]) 
  {
    display.clear();
    display.showNumberDec((TiempoReposo - ContMin), false, 4, 0);
  }
}

/*
* Esta rutina tiene como proposito  usar el led del pin 13 del arduino
* como un indicador para el cambio de estado por cada llamada de la 
* interrupción de temporizado.
*/
void parpadeo1seg(void) 
{
  if (FlagEst[4])
  {
    if (EstadoLed == LOW) 
      {                               //  Esta rutina tiene como proposito
      EstadoLed = HIGH;               //  usar el led 
    display.setBrightness(7, true);
      } 
    else 
      {                               //  
      EstadoLed = LOW;;               // 
      display.setBrightness(0, true);
      }                                 //  
    digitalWrite(led, EstadoLed); 
  }
}

/*
* Función para decrementar cada segundo la variable de conteo al cabo
* de 1 minuto incrementa la Variable ConMin para ser usada de referencia de
* mas adelante comparación.
*/
void cuentAtras(void)
{
  if (conteo == 59) 
    {                               //  se reinicia la variable conteo y se incrementa
    conteo = -1;                    //  la variable ContMin es es nuestro registro de minutos
    ContMin = ContMin + 1;          //  transcurridos
    }
  conteo  = conteo + 1;
}

/*
* Función de Interupción por temporizado a 1 Seg
*/
void IntSeg(void) 
{                                   //  Cuenta regresiva de 59 segundos, al llegar a cero
  cuentAtras();
  parpadeo1seg();
  Visualizar();
  Serial.print(" --> ");
  Serial.print(ContMin, DEC);
  Serial.print(".");                //  Esta variable es el contador de segundos cuando
  Serial.print(conteo, DEC);        //  Activa la interrupción por temporizador 
  //Visualizar();                   // Se aprovecha la llamada de la interrupción para mostrar en puerto
}

/*
* Función de tiempo en espera por intervención de usuario
* durante la estadia en los menus de configuración
*/
void conteo5Seg(void) 
{
  pos = 4;
  Timer1.start();
  conteo = 0;
  ContMin = 0;
}

/*
* Llamada para activar la salida solo si no lo está antes *
*/
void ActSalida(void)
{
  if (!digitalRead(rele))
  {
//    Visualizar();
    digitalWrite(rele, HIGH);
  }
}

/*
* Llamada para desactivar la salida solo si no lo está antes *
*/
void DesSalida(void)
{
  if (digitalRead(rele))
  {
    digitalWrite(rele, LOW);
//    Visualizar();
  }
}

/*
* Finaliza programa de carga luego de cumplir los ciclos
* y tiempos programados
*/
void FinaldeCarga(void)
{
  Timer1.stop();
  Serial.println (" ");
  Serial.print ("Ciclo de Carga Finalizado");
  display.setSegments(SEG_DONE);
  digitalWrite(rele,LOW);
  FlagEst[4] = false;
}

/*
* Función para imprimir en display la
* variable TiempoCarga del programa
*/
void MostrarTiempoCarga(void)
{
  display.showNumberDec(TiempoCarga);
  display.setSegments(SEG_CARGA, 1, 0);
  Serial.print(" Tiempo de Carga ");
  Serial.print( TiempoCarga, DEC);
  Serial.println(" min");
}

/*
* Función para imprimir en display la
* variable TiempoReposo del programa
*/
void MostrarTiempoReposo(void)
{
  display.showNumberDec(TiempoReposo);
  display.setSegments(SEG_REPOSO, 1, 0);
  Serial.print(" Tiempo de Reposo ");
  Serial.print( TiempoReposo, DEC);
  Serial.println(" min");
}

/*
* Función para imprimir en display la
* variable NroCiclo del programa
*/
void MostrarNroCiclos(void)
{
  display.showNumberDec(NroCiclos);
  display.setSegments(SEG_CICLO, 1, 0);
  Serial.print(" Cantidad de ciclos ");
  Serial.println( NroCiclos, DEC);
}

void resume (void)
{
  Serial.println();
  Serial.println("Reinicio de Banderas activa");
  Serial.println();
  Serial.println(" Resumen:");
  Serial.print("T Carga: ");
  Serial.print(TiempoCarga, DEC);
  Serial.println(" min ");
  Serial.print("T Reposo: ");
  Serial.print(TiempoReposo, DEC);
  Serial.println(" min ");
  Serial.print(" Nro de Ciclos ");
  Serial.println(NroCiclos, DEC);
}

/*
* Saludo de inicio de programa
*/
void bienvenida(void)
{
  display.setSegments(SEG_HOLA);
}

/*
* Función de confifuración del arduino uno
*/
void setup() 
{
  pinMode(rele, OUTPUT);            // Define salida para relé activador
  pinMode(led, OUTPUT);             // Define salida para led indicador
  Timer1.initialize(1000000);       // Inicializa temporizador a 1 seg
  Timer1.attachInterrupt(IntSeg);   // Se habilita interrupción cada 1 segundo
  Timer1.stop();                    // Se detiene contador
  Serial.begin(9600);               // Inicializa Puerto Serial a 9600 baudios
  display.setBrightness(5);
  display.clear();
  bienvenida();
}

/*
* Programa principal
*/
void loop() 
{
// Monitoreo de los pulsasores
  BotonCfg.update();
  BotonEntrada.update();
  BotonSubir.update();
  BotonBajar.update();

  /*
  * Borra las banderdas de los menus al finalizar el tiempo de espera 
  */

  if (conteo == 5 && (FlagEst[1] || FlagEst[2] || FlagEst[3]) && !FlagEst[4]) 
  {
    Timer1.stop();
    memset(FlagEst, false, 4);
    display.clear();
    resume();
  }

  /*
  * Al pulsar Cfg luego de las configuraciones reinicia las banderas y
  * Muestra el resumen de las configuraciones realizadas
  */

  if (BotonCfg.isClick() && FlagEst[1] && FlagEst[2] && FlagEst[3] && !FlagEst[4] ) 
  {
    Timer1.stop();
    memset(FlagEst, false, 4);
    resume();
  } 

  /*
  * Condicionales para reconocer la entrada a los diferentes menus de
  * configuración del programa tiempo de carga, tiempo de reposo y
  * Nro de ciclos junto con temporizado para la permanencia en los
  * FlagEst[1] = Bandera del menu de Tiempo de Carga
  * FlagEst[2] = Bandera del menu de Tiempo de Reposo
  * FlagEst[3] = Bandera del menu de Número de Ciclos
  * FlagEst[4] = Bandera de inicio de programa de Carga
  */

  if (BotonCfg.isClick() && !FlagEst[4])
  {
    conteo5Seg();
    Serial.println();
    if (FlagEst[1] && FlagEst[2] && !FlagEst[3]) 
    {
      FlagEst[3] = true;
      Serial.println("**** Bandera de Número de Ciclo activa ****");
      MostrarNroCiclos();
    }
    if (FlagEst[1] && !FlagEst[2] && !FlagEst[3]) 
    {                                                                           
      FlagEst[2] = true;
      Serial.println("**** Bandera de Tiempo de Reposo activa ****");
      MostrarTiempoReposo();
    }
    if (!FlagEst[1] && !FlagEst[2] && !FlagEst[3])
    {                                                                           
      FlagEst[1] = true;
      Serial.println("**** Bandera de Tiempo de Carga activa ****");
      MostrarTiempoCarga();
    }
    
  }
  


  /*
  * Condicionales para la modificación de las variables de configuración
  * Tiempo de Carga +- 5, entre 10 y 60 minutos 
  * Tiempo de Reposo +- 5, entre 39 y 240 minutos 
  * Nro de Ciclos +- 1, entre 1 y 8
  */

  if (!FlagEst[4])
  {
    if (BotonSubir.isClick() && (FlagEst[1] && !FlagEst[2] && !FlagEst[3]) && TiempoCarga < 60)
    {
      TiempoCarga = TiempoCarga + 5;
      MostrarTiempoCarga();
      conteo5Seg();
    }
    if (BotonBajar.isClick() && (FlagEst[1] && !FlagEst[2] && !FlagEst[3]) && TiempoCarga > 10)
    {
      TiempoCarga = TiempoCarga - 5;
      MostrarTiempoCarga();
      conteo5Seg();
    }
    if (BotonSubir.isClick() && (FlagEst[1] && FlagEst[2] && !FlagEst[3]) && TiempoReposo < 240)
    {
      TiempoReposo = TiempoReposo + 5;
      MostrarTiempoReposo();
      conteo5Seg();
    }
    if (BotonBajar.isClick() && (FlagEst[1] && FlagEst[2] && !FlagEst[3]) && TiempoReposo > 30)
    {
      TiempoReposo = TiempoReposo - 5;
      MostrarTiempoReposo();
      conteo5Seg();
    }
    if (BotonSubir.isClick() && (FlagEst[1] && FlagEst[2] && FlagEst[3]) && NroCiclos < 8)
    {
      NroCiclos = NroCiclos + 1;
      MostrarNroCiclos();
      conteo5Seg();
    }
    if (BotonBajar.isClick() && (FlagEst[1] && FlagEst[2] && FlagEst[3]) && NroCiclos > 1)
    {
      NroCiclos = NroCiclos - 1;
      MostrarNroCiclos();
      conteo5Seg();
    }
  }

  /*
  * Activación de bandera para inicio del programa de carga
  */

  if (BotonEntrada.isClick())
  {
    if (FlagEst[4] == false)
    {
      Serial.println("* * * * * * Cargador  Activado * * * * * *");
      Serial.println();
      resume();
      FlagEst[4] = true;
      memset(FlagEst, false, 4);
      Nciclo = NroCiclos;
      ContMin = 0;
      conteo = 0;
      EstReposo = false;
      Timer1.start();
    }
    else
    {
      Serial.println("* * * * * * Cargador   Desactivando * * * * * *");
      memset(FlagEst, false, 5);
      Serial.println(FlagEst[4]);
      Timer1.stop();
      digitalWrite(rele,LOW);      
    
    }  
  }

  if (FlagEst[4] == true)
  {
    if (Nciclo != 0)
    {
      if (!EstReposo)
      {
        if (ContMin < TiempoCarga)
        {
          if (digitalRead(rele) == false)
          {
            Serial.print("* * * * * * Salida Activa * * * * * *");
            ActSalida();
          }
          
        }
        else
        {
          EstReposo = !EstReposo;
          ContMin = 0;
          Nciclo = Nciclo - 1;
          Serial.print(" * * * * * *  Salida Inactiva * * * * * * * *");
          DesSalida();
        }
        
        
      }
      if (EstReposo)
      {
        if (ContMin < TiempoReposo)
        { 
          if (digitalRead(rele) == true)
          {
            DesSalida();
            
          }
          
        }
        else
        {
          EstReposo = !EstReposo;
          ContMin = 0;
          Nciclo = Nciclo - 1;
          
        }
      }
    }
    else
    {
      FinaldeCarga();
    }
  }
}