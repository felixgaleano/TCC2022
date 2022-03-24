/*
  Modbus IP ESP8266
  https://github.com/emelianov/modbus-esp8266
  LCD I2C SDA D2(CPIO 4)
          SCL D1(GPIO 5)
*/
//RESET  PIN 12
//FS300A PIN  2

void ICACHE_RAM_ATTR pulse();
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else //ESP32
#include <WiFi.h>
#endif
#include <ModbusIP_ESP8266.h> //Biblioteca MODBUS para ESP8266

union FC64 {
  float litros;
  uint16_t unidade[4];
} dadost;
unsigned long tempo = 0;

//////////////FLUXO/////////////
const byte interruptPin = 2; //PINO DO FS300A
float calibrationFactor = 5.5;
unsigned long int contFluxo;
float fluxo;

////////////////WiFi////////////////
ESP8266WebServer server(80);
const char* ssid     = "BRUCE";
const char* password = "adatadat";
int np;
const int buttonPin = 12; //PINO DE RESET
int buttonState = 1;
// Set your Static IP address
IPAddress local_IP(192, 168, 0, 34);
// Set your Gateway IP address
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

////////////ModbusIP object///////////
ModbusIP mb;

///json object//////////
StaticJsonDocument<450> jsonDocument;
char buffer[450];
void getData(){
  jsonDocument["total"] = dadost.litros;
  serializeJson(jsonDocument,buffer);
  server.send(200,"application/json",buffer);
}

void setup() {

  //Inicializando a comunicação serial com PC
  Serial.begin(115200);

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

  //Iniciando o servidor ModBus//////////////
  mb.server();
  //Configurando o número de registros ModBus
  for (int i = 0; i < 4; i++) {
    // mb.addHreg(i);  //Adicionando 4 Holdinbg registers
    // mb.addCoil(i);  //Adicionando 4 Coils
    // mb.addIsts(i,0);//Adicionando 4 Input discrete registers
    mb.addIreg(i, 0); //Adicionando 4 Input registers
  }

  //Configurando a Interrupção./////////////////
  pinMode(interruptPin, INPUT_PULLUP);
  //ISR funçao que irá chamar.
  //Modo que irá interronper, CHANGE, FALLING, RISING.
  attachInterrupt(digitalPinToInterrupt(interruptPin), pulse, RISING);//O pino de interrupção
  contFluxo = 0;
      fluxo = 0;

  /////INICIANDO o SERVIDOR HTTP/////////////
  server.on("/", handle_OnConnect);
  server.on("/data",getData);
  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("Servidor HTTP inicializado");
  Serial.println("HTTP server started");
  ///////////////////////////////////////////

  pinMode(buttonPin, INPUT);
}
void pulse()   //measure the quantity of square wave
{
  contFluxo=contFluxo+1;
}
void loop() {
  int i;
  mb.task();
 ///////////////FLUXO/////////////////////////////////////////
 if((millis()-tempo)>=1000){
  detachInterrupt( digitalPinToInterrupt(interruptPin));
  fluxo=((1000/(millis()-tempo))*contFluxo)/(calibrationFactor*60);
  tempo=millis();
  dadost.litros+=fluxo;
  for (np = 0; np < 4; np++) {
      mb.Ireg(np, dadost.unidade[np]);
    }
  contFluxo=0;
  attachInterrupt(digitalPinToInterrupt(interruptPin), pulse, RISING);
 }
  ///////////////FLUXO/////////////////////////////////////////
  ///////////////RESET//////////////////////////////////////////
  buttonState = digitalRead(buttonPin);
  if (buttonState == 0) {
    delay(5000);
    buttonState = digitalRead(buttonPin);
    if (buttonState == 0) {
      contFluxo = 0;
      dadost.litros = 0;
    }
  }
  ///////////////RESET//////////////////////////////////////////
  ///////////////HTTP///////////////////////////////////////////
  server.handleClient();
}////////////FIM do LOOP///////////////////////////////////////



void handle_NotFound() { //Função para lidar com o erro 404
  server.send(404, "text/plain", "Não encontrado"); //Envia o código 404, especifica o conteúdo como "text/pain" e envia a mensagem "Não encontrado"
}
void handle_OnConnect() {
  server.send(200, "text/html", EnvioHTML(dadost.litros)); //Envia as informações usando o código 200, especifica o conteúdo como "text/html" e chama a função EnvioHTML
}
String EnvioHTML(float litros) { //Exibindo a página da web em HTML
  String ptr = "<!DOCTYPE html> <html>\n"; //Indica o envio do código HTML
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n"; //Torna a página da Web responsiva em qualquer navegador Web
  ptr += "<meta http-equiv='refresh' content='5'>";//Atualizar browser a cada 5 segundos
  ptr += "<title>Monitor de consumo de &aacute;gua </title>\n"; //Define o título da página
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
  ptr += "<h1>Monitor de consumo de &aacute;gua</h1>\n";
  ///////////////////Alarmes/////////////////////////////////////////////////
  // if(med[1]>50){
  //   ptr += "<h1 style=\"color:red;font-size:30px;\">Corrente alta na fase 1!</h1>";
  // }
  //////////////////////////////////////////////////////////////////////////
  //Início da primeira TABELA CONSUMO.
  ptr += "<table width=\"350px\" border=\"1\" align=\"center\" >";
  ptr += "<caption><b> Consumo</b></caption>";
  ptr += "<tr>";
  ptr += "<td>";
  ptr += "Consumo Total =  ";
  ptr += (float)litros;
  ptr += " [litros]";
  ptr += "</td>";
  ptr += "</tr>";
  ptr += "</table>";
  ///////////////////////////////////////////////////////////////////

  ptr += "</div>\n";
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}
