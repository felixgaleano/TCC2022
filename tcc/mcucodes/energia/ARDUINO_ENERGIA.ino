/* 
*  Arduino(1) <==> Esp8266
* D10 (Rx)           D8=GPIO15(Tx)
* D11 (Tx)           D5=GPIO14(Rx)
* 5V                
* GND                  GND
* Para a parte da Energia.
* A0,A1 tensão e corrente da fase 1
* A2,A3 tensão e corrente da fase 2
* A4,A5 tensão e corrente da fase 3
Código para o Arduino:
-------------------------------------------------------------------------------------------------------------------------------------------------
*/
#include "EmonLib_tri.h" 
#include <SoftwareSerial.h>
#define rxPin 10                        // define pino D10 como Rx
#define txPin 11                        // define pino D11 como Tx
SoftwareSerial mySerial(rxPin, txPin); 
union FC8{
  uint8_t byteC[4];
  float val;
} dadost,dadosr;
float med[18];
uint8_t k,nn=0,resT;
uint8_t ini=1,arrayT[5],arrayR[5];
unsigned long tempo,tempos,tempoa[3]={0,0,0},tempop;
////////////////////////////////////////////////////////////////////////////////
///////////////////////////PARA ENERGIA///////////////////////////////////////////
EnergyMonitor classeE;
double rP[3],aP[3],pF[3],Vrms[3],Irms[3];
double VCAL[3],PHASECAL[3],ICAL[3];
unsigned int     inPinV[3],inPinI[3];
double Energia[3]={0,0,0};
uint16_t n,ne,np;
////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200); // Hardware Serial
  pinMode (rxPin, INPUT);   // definir modo do pino Rx
  pinMode (txPin, OUTPUT);  // definir modo do pino Tx
  mySerial.begin(76800);     // definir velocidade de comunicação na porta.  
////////////////////////////Calibracao//////////////////////////////////
 VCAL[0]=136.56;PHASECAL[0]=1.15;ICAL[0]=21.156;//Calibracao da Fase 1.
 VCAL[1]=136.56;PHASECAL[1]=1.15;ICAL[1]=21.156;//Calibracao da Fase 2.
 VCAL[2]=136.56;PHASECAL[2]=1.15;ICAL[2]=21.156;//Calibracao da Fase 3.
 inPinV[0]=0;inPinV[1]=2;inPinV[2]=4;
 inPinI[0]=1;inPinI[1]=3;inPinI[2]=5;
    classeE.voltage_tri(inPinV, VCAL, PHASECAL);  // Voltage: input pin, calibration, phase_shift
    classeE.current_tri(inPinI, ICAL); 
///////////////////////////////////////////////////////////////////////
}
void loop() {
   classeE.calcVI_tri(30,2000);
   for(n=0;n<3;n++){
     rP[n]  = classeE.realPower[n];       
     aP[n]  = classeE.apparentPower[n];   
     pF[n]  = classeE.powerFactor[n];     
     Vrms[n]= classeE.Vrms[n];            
     Irms[n]= classeE.Irms[n];
 /*Serial.print("Vrms[");Serial.print(n);Serial.print("]=");
 Serial.print(Vrms[n]);
 Serial.print("  Irms[");Serial.print(n);Serial.print("]=");
 Serial.println(Irms[n]); */
    }
   if((millis()-tempo)>=100){
    tempo=millis();
    for( ne=0;ne<3;ne++){
     Energia[ne]+=rP[ne]*(double)(millis()-tempoa[ne])*1e-10*2.777777777777;
     tempoa[ne]=millis();
     med[0+6*ne]=   (float)Vrms[ne];
     med[1+6*ne]=   (float)Irms[ne];
     med[2+6*ne]=   (float)  rP[ne];
     med[3+6*ne]=   (float)  aP[ne];
     med[4+6*ne]=   (float)  pF[ne];
     med[5+6*ne]=(float)Energia[ne];
                        }
   }
  if((millis()-tempop)>5000){
    tempop=millis();
    for(np=0;np<18;np++){
      Serial.print("med[");Serial.print(np);Serial.print("]=");
      Serial.println(med[np],4);
    } 
  }
  if(millis()-tempos>5000){
  tempos=millis();
  nn=0;
  while(nn<18){
  dadost.val=med[nn];
  arrayT[0]=nn;
  for(k=1;k<5;k++){
    arrayT[k]=dadost.byteC[k-1];             
    }
  mySerial.write(arrayT,5);
  delay(1);
    if(mySerial.available()>0){
    mySerial.readBytes(arrayR,5);
  }
//  Serial.print("nn=");Serial.println(nn);
  for(k=0;k<5;k++){
    if(abs(arrayT[k]-arrayR[k])!=0){resT=1;}
    else {resT=0;}
  }
  if(resT==0)nn++;
               }
  }

}
