/* 
*  Arduino <==>  Esp8266
* D10 (Rx)       D8=GPIO15(Tx)
* D11 (Tx)       D5=GPIO14(Rx)
* 5V             Vin   
* GND            GND
* Para o LCD:
* SCL  <--> D1
* SDA  <--> D2
Código para o ESP8266:
-------------------------------------------------------------------------------------------------------------------------------------------------
*/
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
//#include "/home/frasson/alunos/2021-2/Topicos/Arduino/MEGA/modbus-arduino-ENERGIA-web-modbus/index.h" 
#ifdef ESP8266
 #include <ESP8266WiFi.h>
#else //ESP32
 #include <WiFi.h>
#endif
#include <ModbusIP_ESP8266.h>
#include <LiquidCrystal_I2C.h> //Biblioteca para LCD com I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);
const char* ssid     = "BRUCE";
const char* password = "adatadat";
ESP8266WebServer server(80);
// Set your Static IP address
IPAddress local_IP(192, 168, 0, 35);
// Set your Gateway IP address
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);  
////////////ModbusIP object///////////
ModbusIP mb;

union FC8{
  uint8_t byteC[4];
  float val;
} dadost,dadosr;
union FC16{
  uint16_t uM[2];
  float valm;
} mt,mr;
float med[18];
int nb,k,nm,np;
const int buttonPin = 13;
int buttonState = 0;
uint8_t arrayR[5],arrayT[5];
///////////Para testes.////
uint8_t repetido; 
unsigned long tempoe=0,tempolcd=0,tempop=0;
////////////////////////////
#define rxPin 14                      // define pino D5=GPIO14(Rx) como Rx
#define txPin 15                      // define pino D8=GPIO15(Tx) como Tx
SoftwareSerial mySerial(rxPin, txPin);
unsigned long tempo;
//char array0[10],array1[10],array2[10],array3[10],array4[10],array5[10];

////json object/////
StaticJsonDocument<450> jsonDocument;
char buffer[450];
void getData() {
  jsonDocument.clear();
  jsonDocument["rP1"] = med[2];
  jsonDocument["rP2"] = med[8];
  jsonDocument["rP3"] = med[14];
  jsonDocument["aP1"] = med[3];
  jsonDocument["aP2"] = med[9];
  jsonDocument["aP3"] = med[15];
  jsonDocument["pF1"] = med[4];
  jsonDocument["pF2"] = med[10];
  jsonDocument["pF3"] = med[16];
  jsonDocument["Vrms1"] = med[0];
  jsonDocument["Vrms2"] = med[6];
  jsonDocument["Vrms3"] = med[12];
  jsonDocument["Irms1"] = med[1];
  jsonDocument["Irms2"] = med[7];
  jsonDocument["Irms3"] = med[13];
  jsonDocument["Energia1"] = med[5];
  jsonDocument["Energia2"] = med[11];
  jsonDocument["Energia3"] = med[17];
  serializeJson(jsonDocument, buffer);
  server.send(200, "application/json", buffer);
}


void setup() {
    Serial.begin(115200);
    mySerial.begin(76800);
 //Configurando o WiFi///////////////////////////
    if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  /////INICIANDO o SERVIDOR HTTP/////////////
  server.on("/", handle_OnConnect); 
  server.on("/data", getData);
  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("Servidor HTTP inicializado");
  Serial.println("HTTP server started");
  ///////////////////////////////////////////
  //Iniciando o servidor ModBus//////////////
  mb.server();
  //Configurando o número de registros ModBus
   for(int i=0;i<36;i++){
   // mb.addHreg(i);  //Adicionando 30 Holdinbg registers
   // mb.addCoil(i);  //Adicionando 30 Coils
   // mb.addIsts(i,0);//Adicionando 30 Input discrete registers
    mb.addIreg(i,0);//Adicionando 30 Input registers
    }
    //Inicializando o LCD
    lcd.init();
    // turn on LCD backlight                      
    lcd.backlight(); 
    lcd.setBacklight(HIGH);
    pinMode(buttonPin, INPUT);     
}

void loop() {
mb.task();
   if((tempo-millis())>200){
     tempo=millis();
     for(nm=0;nm<18;nm++){
      mt.valm=med[nm];
      mb.Ireg(nm*2  ,mt.uM[0]);
      mb.Ireg(nm*2+1,mt.uM[1]);
     }
  
   }
nb=mySerial.available();
//if(nb!=0)Serial.println(nb);
  if(nb>4){
    mySerial.readBytes(arrayR,5);
    arrayT[0]=arrayR[0];
    if(arrayR[0]>=0&arrayR[0]<18){
      for(k=1;k<5;k++){
        dadosr.byteC[k-1]=arrayR[k];
        arrayT[k]=arrayR[k];             
       }
      if(arrayR[0]==repetido){
         Serial.print("dt_erro=");Serial.print( (int)((millis()-tempoe)/5000) );
         tempoe=millis();
         Serial.print(" med[");Serial.print(arrayR[0]);Serial.print("]=");
         Serial.println(dadosr.val,4);
      }     
      med[arrayR[0]]=dadosr.val;
    }
    repetido=arrayR[0];

    mySerial.write(arrayT,5);
    mySerial.flush();
    delay(2);

  }
  buttonState = digitalRead(buttonPin);
  if(np==3)np=0;
  if(buttonState==1){
    if((millis()-tempolcd)>3000){
     tempolcd=millis(); 
     lcd.setCursor(0,0);
     lcd.print("V");
     lcd.setCursor(1,0);
     lcd.print(np+1); 
     lcd.setCursor(2,0);
     lcd.print("=");
     lcd.setCursor(3,0);
     lcd.print(med[np*6]);
     lcd.setCursor(8,0);
     lcd.print(" ");
     lcd.setCursor(9,0);
     lcd.print("I");
     lcd.setCursor(10,0);
     lcd.print(np+1);
     lcd.setCursor(11,0);
     lcd.print("=");
     lcd.setCursor(12,0);
     lcd.print(med[np*6+1]);
     lcd.setCursor(0,1);
     lcd.print("P");
     lcd.setCursor(1,1);
     lcd.print(np+1); 
     lcd.setCursor(2,1);
     lcd.print("=");
     lcd.setCursor(3,1);
     lcd.print(med[np*6+2]);
     lcd.setCursor(8,1);
     lcd.print(" ");
     lcd.setCursor(9,1);
     lcd.print("FP=");
     lcd.setCursor(12,1);
     lcd.print(med[np*6+4]);
     np++;    
    }
  }
  else{
      lcd.setCursor(0,0);
      lcd.print("IP=");
      lcd.setCursor(3,0); 
      lcd.print(WiFi.localIP());
      lcd.setCursor(15,0);
      lcd.print(" /");
      lcd.setCursor(0,1);
      lcd.print("Consumo=");
      lcd.setCursor(8,1);
      lcd.print(med[5]+med[11]+med[17]);
      lcd.setCursor(13,1);
      lcd.print("kWh");
      tempolcd=millis();
  }
  server.handleClient();

}
void handle_NotFound() { //Função para lidar com o erro 404
  server.send(404, "text/plain", "Não encontrado"); //Envia o código 404, especifica o conteúdo como "text/pain" e envia a mensagem "Não encontrado"
}
void handle_OnConnect() {
  server.send(200, "text/html", EnvioHTML(med)); //Envia as informações usando o código 200, especifica o conteúdo como "text/html" e chama a função EnvioHTML
}
String EnvioHTML(float *med) { //Exibindo a página da web em HTML
  String ptr = "<!DOCTYPE html> <html>\n"; //Indica o envio do código HTML
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n"; //Torna a página da Web responsiva em qualquer navegador Web
  ptr += "<meta http-equiv='refresh' content='2'>";//Atualizar browser a cada 2 segundos
  ptr += "<title>Monitor de Dados El&eacutetricos </title>\n"; //Define o título da página
  //Configurações de fonte do título e do corpo do texto da página web
  ptr += "<style>html { font-family: 'Open Sans', sans-serif; display: block; margin: 0px auto; text-align: center;color: #000000;}\n";
  ptr += "body{margin-top: 50px;}\n";
  ptr += "h1 {margin: 50px auto 30px;}\n";
  ptr += "h2 {margin: 40px auto 20px;}\n";
  ptr += "p {font-size: 24px;color: #000000;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<div id=\"webpage\">\n";
  ptr += "<h2>Monitor de Dados El&eacutetricos na Entrada da casa</h2>\n";
///////////////////Alarmes/////////////////////////////////////////////////
  if(med[1]>50){
    ptr += "<h1 style=\"color:red;font-size:30px;\">Corrente alta na fase 1!</h1>";
  }
    if(med[7]>50){
    ptr += "<h1 style=\"color:red;font-size:30px;\">Corrente alta na fase 2!</h1>";
  }
    if(med[13]>50){
    ptr += "<h1 style=\"color:red;font-size:30px;\">Corrente alta na fase 3!</h1>";
  }
//////////////////////////////////////////////////////////////////////////
  //Início da primeira TABELA CONSUMO.
  ptr += "<table width=\"350px\" border=\"1\" align=\"center\" >";
  ptr += "<caption><b> Consumo</b></caption>";
  ptr += "<tr>";
  ptr += "<td>";
  ptr += "Consumo Total =  ";
  ptr += (float)(med[5]+med[11]+med[17]);
  ptr += " [kWh]";
  ptr += "</td>";
  ptr += "</tr>";
  ptr += "</table>";
///////////////////////////////////////////////////////////////////
  //Início da primeira TABELA.
  ptr += "<table width=\"350px\" border=\"1\" align=\"center\" >";
  ptr += "<caption><b> Fase 1 </b></caption>";
  ptr += "<tr>";
  ptr += "<td>";
  ptr += "Vrms<sub>1</sub> =  ";
  ptr += (float)med[0];
  ptr += " [V]";
  ptr += "</td>";
  ptr += "<td>";
  ptr += "Irms<sub>1</sub> = ";
  ptr += (float)med[1];
  ptr += " [A]";
  ptr += "</td>";
  ptr += "</tr>";
  ptr += "<tr >";
  ptr += "<td>";
  ptr += "PR<sub>1</sub> = ";
  ptr += (float)med[2];
  ptr += " [W]";
  ptr += "</td><td>" ;
  ptr += "PA<sub>1</sub> = ";
  ptr += (float)med[3];
  ptr += " [VA]";
  ptr += "</td>";
  ptr += "</tr>";

 ptr += "<tr>";
  ptr += "<td>" ;
  ptr += "FP<sub>1</sub> = ";
  ptr += (float)med[4];
  ptr += "</td><td>";
  ptr += "<mark>Energia<sub>1</sub> = ";
  ptr += (float)med[5];
  ptr += " [kWh]</mark>";
  ptr += "</td></tr>";
  
  ptr += "</table>";
///////Fim da primeira tabela
  //Início da SEGUNDA TABELA.
  ptr += "<table width=\"350px\" border=\"1\" align=\"center\">";
  ptr += "<caption><b> Fase 2 </b></caption>";
  ptr += "<tr>";
  ptr += "<td>";
  ptr += "Vrms<sub>2</sub> =  ";
  ptr += (float)med[6];
  ptr += " [V]";
  ptr += "</td>";
  ptr += "<td>";
  ptr += "Irms<sub>2</sub> = ";
  ptr += (float)med[7];
  ptr += " [A]";
  ptr += "</td>";
  ptr += "</tr>";
  ptr += "<tr>";
  ptr += "<td>";
  ptr += "PR<sub>2</sub> = ";
  ptr += (float)med[8];
  ptr += " [W]";
  ptr += "</td><td>" ;
  ptr += "PA<sub>2</sub> = ";
  ptr += (float)med[9];
  ptr += " [VA]";
  ptr += "</td>";
  ptr += "</tr>";

 ptr += "<tr>";
  ptr += "<td>" ;
  ptr += "FP<sub>2</sub> = ";
  ptr += (float)med[10];
  ptr += "</td><td>";
  ptr += "<mark>Energia<sub>2</sub> = ";
  ptr += (float)med[11];
  ptr += " [kWh]</mark>";
  ptr += "</td></tr>";
  
  ptr += "</table>";
///////Fim da segunda tabela
  //Início da TERCEIRA TABELA.
  ptr += "<table width=\"350px\" border=\"1\" align=\"center\">";
  ptr += "<caption><b> Fase 3 </b></caption>";
  ptr += "<tr>";
  ptr += "<td>";
  ptr += "Vrms<sub>3</sub> =  ";
  ptr += (float)med[12];
  ptr += " [V]";
  ptr += "</td>";
  ptr += "<td>";
  ptr += "Irms<sub>3</sub> = ";
  ptr += (float)med[13];
  ptr += " [A]";
  ptr += "</td>";
  ptr += "</tr>";
  ptr += "<tr>";
  ptr += "<td>";
  ptr += "PR<sub>3</sub> = ";
  ptr += (float)med[14];
  ptr += " [W]";
  ptr += "</td><td>" ;
  ptr += "PA<sub>3</sub> = ";
  ptr += (float)med[15];
  ptr += " [VA]";
  ptr += "</td>";
  ptr += "</tr>";

 ptr += "<tr>";
  ptr += "<td>" ;
  ptr += "FP<sub>3</sub> = ";
  ptr += (float)med[16];
  ptr += "</td><td>";
  ptr += "<mark>Energia<sub>3</sub> = ";
  ptr += (float)med[17];
  ptr += " [kWh]</mark>";
  ptr += "</td></tr>";
  
  ptr += "</table>";
///////Fim da terceira tabela

  ptr += "</div>\n";
  ptr += "</body>\n";
  ptr += "</html>\n";

  return ptr;

}
