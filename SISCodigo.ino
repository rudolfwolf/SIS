

#include <LiquidCrystal_I2C.h>    //Se Incluye la Libreria d eLiquid Crystal para utilizar el Display.
LiquidCrystal_I2C lcd (0x27,16,2);    //Se Crea un Onjeto de Nombre lcd con los parametros de 0x27 (direccion del display), 16 (el número de caracters por renglon) y 2 (El Numero de Filas).
#include "FirebaseESP8266.h"          //Se incluye la Libreria para Establecer Conexion y utilizar FireBase.
#include <ESP8266WiFi.h>              //Se Incluye la Libreria para Utilizar WiFi.
#include <WifiUDP.h>
#include <NTPClient.h>
#include <Time.h>
#include <TimeLib.h>
#include <Timezone.h>

// Definir propiedades NTP
#define NTP_OFFSET   -21600   //La zona horaria de Guadalajara va 6 horas por detras d ela universal, por lo tanto -6(3600)= -21600 En segundos
#define NTP_ADDRESS  "pool.ntp.org"                                                                                        // URL Del NTP del cual obtendremos la Fecha.

WiFiUDP ntpUDP;                                                                                                            // Configura el cliente NTP UDP 
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET);
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 60};                                                                     // Hora de Verano de Europa Central
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 0};                                                                       // Hora Estandar de Europa Central
Timezone CE(CEST, CET);
time_t local, utc;

const char * days[] = {"Domingo", "Lunes", "Martes", "Miercoles", "Jueves", "Viernes", "Sabado"} ;                        // Configurar Fecha y hora
const char * months[] = {"Ene", "Feb", "Mar", "Abr", "May", "Jun", "Jul", "Ago", "Sep", "Oct", "Nov", "Dic"} ;            // Configurar Fecha y hora

#define FIREBASE_HOST "sunflowerisystem-e3e0d-default-rtdb.firebaseio.com" //Sin http:// o https:// 
#define FIREBASE_AUTH "blkVfG5Oq2Zvqc8EpvrTbaptRP2X4n7tDHrekbOf"

#define WIFI_SSID "Navarro"
#define WIFI_PASSWORD "3310648666@_@."

#define SensorHum A0        //Se Define que el Sensor de Humedad Pertenece al pin Analogico 0.
#define Bomba D0            //Se Define que la Bomba de Agua Pertenece al Pin Digital 2.

String path = "/SIS";       //Este es El Nombre que Tendra la Tabla del Proyecto en Firebase.
FirebaseData firebaseData;  //Se define un Objeto Direbase.

int Contador=0;

void setup() {
  Serial.begin(115200); //Se Inicializa el Serial.

  lcd.init();           //Inicializa el Display LCD
  lcd.backlight();      //Prende la Luz del Fondo del Display LCD

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);   //Se Inicia la Conexión WiFi.
  Serial.print("Conectando a ....");
  lcd.setCursor(0,0);       //Indica que va a Imprimir en la Primer Linea del LCD.
  lcd.print("WiFi: ");
  lcd.print(WIFI_SSID);
  lcd.setCursor(0,1);       //Indica que va a Imprimir en la Segunda Linea del LCD.
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    lcd.print(".");
    delay(300);
  }
  
  Serial.println();
  Serial.print("Conectado con la IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  
  lcd.clear();    //Limpia el Display LCD.
  lcd.setCursor(0,0);       //Indica que va a Imprimir en la Primer Linea del LCD.
  lcd.print("WiFi: ");
  lcd.print(WIFI_SSID);
  lcd.setCursor(0,1);       //Indica que va a Imprimir en la Segunda Linea del LCD.
  lcd.print("Conexion Exitosa");
  delay(1500);
  lcd.clear();    //Limpia el Display LCD.

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  //Establezca el tiempo de espera de lectura de la base de datos en 1 minuto (máximo 15 minutos)
  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  
  //Tamaño y  tiempo de espera de escritura, tiny (1s), small (10s), medium (30s) and large (60s).
  //tiny, small, medium, large and unlimited.
  Firebase.setwriteSizeLimit(firebaseData, "tiny");

  Firebase.setString(firebaseData, path + "/Irrigador", "false");
  Firebase.setInt(firebaseData, path + "/Humedad", 0);
  
  pinMode(SensorHum, INPUT);  //Se Declara que el pin del SensordeHum es de entrada.
  pinMode(Bomba,OUTPUT);      //Se Declara que el pin de la Bomba de Agua es de Entrada.

  Firebase.getInt(firebaseData, path + "/Contador" ); //Obtiene el Valor del Contador en la base de datos, y lo almacena en firebase.Data.
  Contador=firebaseData.intData();             //Almacena el Valor Int Obtenido en la Variable Contador.

}

void loop() {
  
  int Humedad = analogRead(SensorHum);  //Se declara la variable donde se Almacenara los Datos de la Humedad, y Se Guarda en ella Los valores obtenidos por el Sensor de Humedad.
  //El Sensor de Humedad en modo Analogico, capta valores de 0 a 1023, por lo que, para su mejor uso,
  //Se utiliza la funcion map, para convertir esos valores en un rango de porcentade del 0% al 100%
  //Sin embargo, en las pruebas realizadas el dato mas bajo que se senso fue de 140, y el mas alto de 800, cuando la tierra habia sido recien regada.

  Serial.print("Nivel de Sequía: ");
  Serial.print(Humedad);  //Se Imprime por Serial Los Valores de la Humedad. (Solo para pruebas)
  Serial.println("%");
  
  Humedad = map(Humedad,390,800,0,100);  //Y el Resultado, se vuelve a Almacenar en la Variable Humedad.

  lcd.setCursor(0,0);       //Indica que va a Imprimir en la Primer Linea del LCD.
  lcd.clear();    //Limpia el Display LCD.
  lcd.print("Sequia Al ");
  
  if(Humedad>100){          //Si la Sequia es Mayor al 100% (Estado que se da cuando la sonda esta al aire libre), Imprimir un 100, esto para no confundir al usuario.
    lcd.print(100);       //Imprime el % de Sequia en el Display LCD.
    Firebase.setInt(firebaseData, path + "/Humedad", 100);  //Manda 100 a la Base De Datos.
    Firebase.setInt(firebaseData, path + "/Historial" + Contador + "/HumedadReg", 100);  //Manda 100 a la Base De Datos.
    }else{                  //De lo contrario, Imprimir el Valor de Sequia REAL.
      if(Humedad<0){  //Si la Sequia es menos al 0%, el Display imprime 0% para no confundir al usuario.
        lcd.print(0);
        Firebase.setInt(firebaseData, path + "/Humedad", 0);  //Manda 0 a La Base de Datos.
        Firebase.setInt(firebaseData, path + "/Historial" + Contador + "/HumedadReg", 0);  //Manda 0 a la Base De Datos.
        }else{
          lcd.print(Humedad);       //Imprime el % de Sequia en el Display LCD.
          Firebase.setInt(firebaseData, path + "/Humedad", Humedad);  //Manda la Humedad Real a la Base De Datos.
            Firebase.setInt(firebaseData, path + "/Historial" + Contador + "/HumedadReg", Humedad);  //Manda la Humedad Real a la Base De Datos.
          }
      }
  lcd.print("%");

  if(Humedad <= 80){                //Valida si el Nivel de Sequia es Optimo Para Regar. FALTA SEGUIR SENSANDO LA HUMEDAD
        Serial.println("No Se Riega");
        digitalWrite(Bomba,LOW);        //Apaga la Bomba de Agua.
        lcd.setCursor(0,1);             //Indica que va a Imprimir en la Segunda Linea del LCD.
        lcd.print("No Se Riega");       //Imprime que no se regara en el Display LCD.
        }else if(Humedad > 80){
            Serial.println("Si Se Riega");
            digitalWrite(Bomba,HIGH);     //Prende la Bomba de Agua.
            lcd.setCursor(0,1);           //Indica que va a Imprimir en la Segunda Linea del LCD.
            lcd.print("Si Se Riega");       //Imprime que si se regara en el Display LCD.
            delay(3000);
          }

  Firebase.getString(firebaseData, path + "/Irrigador" ); //Obtiene el Valor de Irrigador en la base de datos, y lo almacena en firebase.Data.
  Serial.println(firebaseData.stringData());
  if(firebaseData.stringData()=="true"){  //Valida si el valor de firebase.Data es false, si es true, irriga.
        Serial.println("Regando Mediante APP");
        digitalWrite(Bomba,HIGH);        //Prende la Bomba de Agua.
        lcd.clear();                     //Limpia el Display LCD.
        lcd.setCursor(0,0);              //Indica que va a Imprimir en la Primer Linea del LCD.
        lcd.print("Regando Via APP");    //Imprime que no se regara en el Display LCD.
        lcd.setCursor(0,1);              //Indica que va a Imprimir en la Segunda Linea del LCD.
        for(int i=0;i<8;i++){
              delay(375);
              lcd.print(" .");    //Imprime que no se regara en el Display LCD.
            }
        digitalWrite(Bomba,LOW);        //Apaga la Bomba de Agua.
        Firebase.setString(firebaseData, path + "/Irrigador", "false");
      }

    timeClient.update();                                                                                                // Actualizar el cliente NTP y obtener la marca de tiempo UNIX UTC
    unsigned long utc =  timeClient.getEpochTime();
    local = CE.toLocal(utc);                                                                                            // Convertir marca de tiempo UTC UNIX a hora local
    Serial.println("");
    Serial.print("Fecha local: ");
    Serial.print(convertirTimeATextoFecha(local));
    Serial.println("");
    Serial.print("Hora local: ");
    Serial.print(convertirTimeATextoHora(local));                                                                                                   // Enviar Fecha y hora por puerto serie

        
  Firebase.setInt(firebaseData, path + "/Contador", Contador);
  Firebase.setString(firebaseData, path + "/Historial" + Contador + "/Fecha", convertirTimeATextoFecha(local));
  Firebase.setString(firebaseData, path + "/Historial" + Contador + "/Hora", convertirTimeATextoHora(local));
  Firebase.setInt(firebaseData, path + "/Historial" + Contador + "/Clave", Contador);
  Contador=Contador+1;
  if(Contador>5){
    Contador=0;
    }
    delay(1500);    //Se Realiza una Pausa, para no desgastar tanto a la Sonda del Sensor de Humedad.
}

String convertirTimeATextoFecha(time_t t)                                                                               // Funcion para formatear en texto la fecha  
{
  String date = "";
  date += days[weekday(t)-1];
  date += "_";
  date += day(t);
  //date += " ";
  date += months[month(t)-1];
  date += "_";
  date += year(t);
  return date;
}

String convertirTimeATextoHora(time_t t)                                                                              // Funcion para formatear en texto la hora                                                                          
{
  String hora ="";                                                                                                    // Funcion para formatear en texto la hora  
  if(hour(t) < 10)
  hora += "0";
  hora += hour(t);
  hora += "-";
  if(minute(t) < 10)                                                                                                  // Agregar un cero si el minuto es menor de 10
    hora += "0";
  hora += minute(t);
  return hora;
}
