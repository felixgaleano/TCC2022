/*
  Emon.cpp - Library for openenergymonitor
  Created by Trystan Lea, April 27 2010
  GNU GPL
  modified to use up to 12 bits ADC resolution (ex. Arduino Due)
  by boredman@boredomprojects.net 26.12.2013
  Low Pass filter for offset removal replaces HP filter 1/1/2015 - RW
*/

// Proboscide99 10/08/2016 - Added ADMUX settings for ATmega1284 e 1284P (644 / 644P also, but not tested) in readVcc function

//#include "WProgram.h" un-comment for use on older versions of Arduino IDE
#include "EmonLib_tri.h"

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif 

//--------------------------------------------------------------------------------------
// Sets the pins to be used for voltage and current sensors
//--------------------------------------------------------------------------------------
void EnergyMonitor::voltage_tri(unsigned int* _inPinV, double* _VCAL, double* _PHASECAL)
{
  this->inPinV = _inPinV;
  this->VCAL = _VCAL;
  this->PHASECAL = _PHASECAL;
  offsetV[1] = ADC_COUNTS>>1;
  offsetV[2] = ADC_COUNTS>>1;
  offsetV[3] = ADC_COUNTS>>1;//Divide a faixa por 2 (Faz um shitf de 1 posicao para direita)
  
}

void EnergyMonitor::current_tri(unsigned int* _inPinI, double* _ICAL)
{
  this->inPinI = _inPinI;
  this->ICAL = _ICAL;
  offsetI[1] = ADC_COUNTS>>1;
  offsetI[2] = ADC_COUNTS>>1;
  offsetI[3] = ADC_COUNTS>>1;//Divide a faixa por 2 (Faz um shitf de 1 posicao para direita)
}

//--------------------------------------------------------------------------------------
// emon_calc procedure
// Calculates realPower,apparentPower,powerFactor,Vrms,Irms,kWh increment
// From a sample window of the mains AC voltage and current.
// The Sample window length is defined by the number of half wavelengths or crossings we choose to measure.
//--------------------------------------------------------------------------------------
void EnergyMonitor::calcVI_tri(unsigned int crossings, unsigned int timeout)
{
  #if defined emonTxV3
  int SupplyVoltage=3300;//3.3v
  #else
  int SupplyVoltage = readVcc();
  #endif

  unsigned int crossCount = 0;                             //Used to measure number of times threshold is crossed.
  unsigned int numberOfSamples = 0;                        //This is now incremented

  //-------------------------------------------------------------------------------------------------------------------------
  // 1) Waits for the waveform to be close to 'zero' (mid-scale adc) part in sin curve.
  //-------------------------------------------------------------------------------------------------------------------------
  boolean st=false;                                  //an indicator to exit the while loop

  unsigned long start = millis();    //millis()-start makes sure it doesnt get stuck in the loop if there is an error.

  while(st==false)                                   //the while loop...
  {
    startV = analogRead(inPinV[1]);                    //using the voltage waveform
                                                    //Nivel de tensão no inicio das medições próximo ao centro da faixa do ADC.
				    //O referencial do tempo é a tensao.
    if ((startV < (ADC_COUNTS*0.55)) && (startV > (ADC_COUNTS*0.45))) st=true;  //check its within range.
						        //ADC_COUNTS=1024(10 bits) ou 4096(12 bits).
    if ((millis()-start)>timeout) st = true;
  }

  //-------------------------------------------------------------------------------------------------------------------------
  // 2) Main measurement loop
  //-------------------------------------------------------------------------------------------------------------------------
  start = millis();

  while ((crossCount < crossings) && ((millis()-start)<timeout))
  {
    numberOfSamples++;                       //Count number of times looped.
    for(nf=0;nf<3;nf++){
    lastFilteredV[nf] = filteredV[nf];               //Used for delay/phase compensation

    //-----------------------------------------------------------------------------
    // A) Read in raw voltage and current samples
    //-----------------------------------------------------------------------------
    sampleV[nf] = analogRead(inPinV[nf]);                 //Read in raw voltage signal
    sampleI[nf] = analogRead(inPinI[nf]);                 //Read in raw current signal

    //-----------------------------------------------------------------------------
    // B) Apply digital low pass filters to extract the 2.5 V or 1.65 V dc offset,
    //     then subtract this - signal is now centred on 0 counts.
    //-----------------------------------------------------------------------------
    offsetV[nf] = offsetV[nf] + ((sampleV[nf]-offsetV[nf])/1024);
    filteredV[nf] = sampleV[nf] - offsetV[nf];
    offsetI[nf] = offsetI[nf] + ((sampleI[nf]-offsetI[nf])/1024);
    filteredI[nf] = sampleI[nf] - offsetI[nf];
    //-----------------------------------------------------------------------------
    // C) Root-mean-square method voltage
    //-----------------------------------------------------------------------------
    sqV[nf]= filteredV[nf] * filteredV[nf];                 //1) square voltage values
    sumV[nf] += sqV[nf];                                //2) sum

    //-----------------------------------------------------------------------------
    // D) Root-mean-square method current
    //-----------------------------------------------------------------------------
    sqI[nf] = filteredI[nf] * filteredI[nf];                //1) square current values
    sumI[nf] += sqI[nf];                                //2) sum

    //-----------------------------------------------------------------------------
    // E) Phase calibration
    //-----------------------------------------------------------------------------
    phaseShiftedV[nf] = lastFilteredV[nf] + PHASECAL[nf] * (filteredV[nf] - lastFilteredV[nf]);
    //-----------------------------------------------------------------------------
    // F) Instantaneous power calc
    //-----------------------------------------------------------------------------
    instP[nf] = phaseShiftedV[nf] * filteredI[nf];          //Instantaneous Power
    sumP[nf] +=instP[nf];                               //Sum
    }
    //-----------------------------------------------------------------------------
    // G) Find the number of times the voltage has crossed the initial voltage
    //    - every 2 crosses we will have sampled 1 wavelength
    //    - so this method allows us to sample an integer number of half wavelengths which increases accuracy
    //-----------------------------------------------------------------------------
    lastVCross = checkVCross;
    if (sampleV[1] > startV) checkVCross = true;
                     else checkVCross = false;
    if (numberOfSamples==1) lastVCross = checkVCross;//Se estiver no primeiro loop, atualize. 

    if (lastVCross != checkVCross) crossCount++;
    
  }
  //-------------------------------------------------------------------------------------------------------------------------
  // 3) Post loop calculations
  //-------------------------------------------------------------------------------------------------------------------------
  //Calculation of the root of the mean of the voltage and current squared (rms)
  //Calibration coefficients applied.
  for( nf=0;nf<3;nf++){
  V_RATIO[nf] = VCAL[nf] *((SupplyVoltage/1000.0) / (ADC_COUNTS));
                                               //É a tensão por nível de amostragem.
  Vrms[nf] = V_RATIO[nf] * sqrt(sumV[nf] / numberOfSamples);//Raiz da aproximação da integral da tensão^2 
                                                //~sum v^2 delta_t / tempo total N*delta_t  
                                                //vezes a razão.
  I_RATIO[nf] = ICAL[nf] *((SupplyVoltage/1000.0) / (ADC_COUNTS));//ICAL*3300/1024 OU ICAL*3300/4096
  Irms[nf] = I_RATIO[nf] * sqrt(sumI[nf] / numberOfSamples);

  //Calculation power values
  realPower[nf] = V_RATIO[nf] * I_RATIO[nf] * sumP[nf] / numberOfSamples;
  apparentPower[nf] = Vrms[nf] * Irms[nf];
  powerFactor[nf]=realPower[nf] / apparentPower[nf];

  //Reset accumulators
  sumV[nf] = 0;
  sumI[nf] = 0;
  sumP[nf] = 0;
   }
//--------------------------------------------------------------------------------------
}

//--------------------------------------------------------------------------------------


//thanks to http://hacking.majenko.co.uk/making-accurate-adc-readings-on-arduino
//and Jérôme who alerted us to http://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/

long EnergyMonitor::readVcc() {
  long result;

  //not used on emonTx V3 - as Vcc is always 3.3V - eliminates bandgap error and need for calibration http://harizanov.com/2013/09/thoughts-on-avr-adc-accuracy/

  #if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328__) || defined (__AVR_ATmega328P__)
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_AT90USB1286__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  ADCSRB &= ~_BV(MUX5);   // Without this the function always returns -1 on the ATmega2560 http://openenergymonitor.org/emon/node/2253#comment-11432
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);

  #endif


  #if defined(__AVR__)
  delay(2);                                        // Wait for Vref to settle
  ADCSRA |= _BV(ADSC);                             // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = READVCC_CALIBRATION_CONST / result;  //1100mV*1024 ADC steps http://openenergymonitor.org/emon/node/1186
  return result;
  #elif defined(__arm__)
  return (3300);                                  //Arduino Due
  #else
  return (3300);                                  //Guess that other un-supported architectures will be running a 3.3V!
  #endif
}

