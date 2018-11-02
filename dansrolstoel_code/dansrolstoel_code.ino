/* Software voor de arduino nano in de dansrolstoel van Pluim - geschreven door Ronald Van Ham

connecties:
A0: Batterijspanning via spanningsdeler, om de batterijspanning te controlleren, gekleurde led aan te sturen en indien nodig stoppen om batterij te beschermen
De weerstanden dienen gekozen te worden dat de spanning nooit (ook niet tijdens het opladen) boven de 5 volt aan de ingang komt. 
Nadien via voltmeter en serial monitor calibratiefactor bepalen
A6: Potenriometer onder rechtervoet 
A7: Potenriometer onder linkervoet 
Deze 2 dienen in software aangepast te worden door te kijken via seriele monitor welke waarde de middenpositie heeft, en wat de 2 uiterste zijn)
Deze waarden vul je in bij RMin, RMid, RMax en LMin, LMid, LMax. Dit dient normaal maar 1 keer te gebeuren, maar soms even checken is niet slecht 

PIN 3,9,10,11 PWM outputs, die gekoppeld zijn aan timer 1 en 2. Dus dan blijft Timer 0 beschikbaar als watchdog timer
We kunnen ook de frequentie van Timer 1 en 2 aanpassen zodat de motoren minder lawaai maken. Let op, de H-Brug moet deze hogere frequentie aankunnen.

Pin 6,7,8: GRB LED
*/


#include <avr/interrupt.h>   // we gaan timer0 interrupt als watchdog timer gerbuiken, omdat de watchdogtimer in de chinese arduino nano niet werkt. 
                              // Andere oplossing is om andere bootloader in de Chinese Arduino Nano te steken

// De gebruikte variabelen:
int PotR = 0;        // Waarde van de rechter potentiometer
int PotL = 0;        // vaarde van de liknker potentiometer
float ControlR = 0;         //  Stuursignaal rechtse motor
float ControlRFilter = 0;   //  Gefilterde waaarde, om schokken te vermijden en iets rustiger op te trekken. 
float ControlL = 0;  
float ControlLFilter = 0;
int Marge = 6;              // Marge vanaf wanneer de motoren iets mogen doen, = dode band in het midden
int Voltage = 0;            // gemeten spanning x10
float FilterConst = 0.2;    //  Filterconstante hangt af van hoe precies Pluim zijn voeten kan controleren

int PWMout1L = 0;           // Waardes output PWM (analog out)
int PWMout2L = 0; 
int PWMout1R = 0;        
int PWMout2R = 0; 

const int RMin = 130;        // instelwaardes van de hoeken van de voetpotentiometers. Af te lezen via SerialMonitor
const int RMid = 423;
const int RMax = 663;

const int LMin = 175;
const int LMid = 448;
const int LMax = 692;

void setup() {
  //Starten serial communications op 115200 bps:  Als commentaar in gewone werking, opzetten indien je hoeken wil instellen
  //Serial.begin(115200);
  //Serial.println("rolstoel Pluim");
  TCCR1B = TCCR1B & 0b11111000 | 0x01;   // Frequenties PWM aanpassen om de motoren stiller te maken. Maak hier commentaar van als de Hbrug niet snel genoeg is.
  TCCR2B = TCCR2B & 0b11111000 | 0x01;    // De werking blijft hetzelde, maar met iets meer lawaai van de motoren. Als het niet werkt, eerst beide lijnen commentaar maken
  pinMode(6, OUTPUT);                   // Deze 3 outputs dienen voor RGB led als batterij indicator. Zou ook via neopixel led kunnen
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  
  ControlRFilter = 127;        // 0-126 is de ene richting, 127 stilstand, 128-255 is de andere richting
  ControlLFilter = 127;        // Dus bij opstart zetten we beide motoren af

  //Setup Timer0 to fire every 16ms as watchdog timer, zie internet: Er is blijkbaar een probleem met de bootloader in Chinese Arduino nano bij gebruikt watchdog timer 
  TCCR0B = 0x00;        //Disbale Timer2 while we set it up
  TCNT0  = 130;         //Reset Timer Count to 130 out of 255
  TIFR0  = 0x00;        //Timer2 INT Flag Reg: Clear Timer Overflow Flag
  TIMSK0 = 0x01;        //Timer2 INT Reg: Timer2 Overflow Interrupt Enable
  TCCR0A = 0x00;        //Timer2 Control Reg A: Wave Gen Mode normal
  TCCR0B = 0x07;        //Timer2 Control Reg B: Timer Prescaler set to 1024
}

void loop() {
  TCNT0 = 130;  // reset onze eigen watchdog timer0
 
  Voltage = map(10*analogRead(A0),0, 10240, 0, 214);   // Lees de batterijspanning en laat de kleur van de LED Rood of groen worden
  if (Voltage > 150) {
  digitalWrite(6, LOW);
  digitalWrite(7, HIGH); // Green
  digitalWrite(8, LOW);
  }
  else
  {
  digitalWrite(6, HIGH);  // Red
  digitalWrite(7, LOW);
  digitalWrite(8, LOW);
  }
  TCNT0 = 130;  // reset onze eigen watchdog timer0
  if (PWMout1L > 1 or PWMout1R > 1 or PWMout2L > 1 or PWMout2R > 1) {
  digitalWrite(8, HIGH);  //Blue     Laat de led blauw worden als de motoren aangestuurd worden, dan kunnen we de spanning van de batterij niet juist inschatten
  digitalWrite(7, LOW);
  }
  else {
  digitalWrite(8, LOW);   
  }
  
  PotR = analogRead(A6);  //  Lees beide voetsensoren in
  PotL = analogRead(A7);
  TCNT0 = 130;  // reset onze eigen watchdog timer0
  if(PotR < RMid)                                   // Doe een mapping voor de rechter motor, maar apart voor vooruit en achteruit. 
   {ControlR = map(PotR, RMin, RMid, 0, 127);
   if (PotR < RMin) {ControlR = 0;}
   }
  else
   {ControlR = map(PotR, RMid, RMax, 128, 255);
   if (PotR > RMax) {ControlR = 255;}
   } 
  if (ControlR < 0) {ControlR = 0;}        // limiteer de waarde
  if (ControlR > 255) {ControlR = 255;}

  if(PotL < LMid)                                // Doe een mapping voor de linker motor, maar apart voor vooruit en achteruit.
   {ControlL = map(PotL, LMin, LMid, 0, 127);
   if (PotL < LMin) {ControlL = 0;} 
   } 
  else
   {ControlL = map(PotL, LMid, LMax, 128, 255); 
   if (PotL > LMax) {ControlL = 255;}
   }
  if (ControlL < 0) {ControlL = 0;}     // limiteer ook deze waarde
  if (ControlL > 255) {ControlL = 255;}
  TCNT2 = 00;  // reset timer 2
  ControlRFilter = (FilterConst * ControlR + (1 - FilterConst) * ControlRFilter);  // Filteren omdat Pluim iets minder controle heeft. Dit is te tunen tot het goed voelt
  ControlLFilter = (FilterConst * ControlL + (1 - FilterConst) * ControlLFilter);
  
  //  Zet de motoren af, we gaan ze straks opzetten, maar alleen als de batterij voldoende vol is  
  PWMout1L = 1;        
  PWMout2L = 1;  

  PWMout1R = 1; 
  PWMout2R = 1;
  TCNT0 = 130;  // reset onze eigen watchdog timer0
  if(ControlLFilter > 127+Marge) {PWMout1L = 2*(ControlLFilter-128); PWMout2L = 1;  }  // MAp vooruit en achterui naar de juiste PWM pinnen
  if(ControlLFilter < 127-Marge) {PWMout1L = 1; PWMout2L = 2*(127-ControlLFilter);  }

  if(ControlRFilter > 127+Marge) {PWMout1R = 2*(ControlRFilter-128); PWMout2R = 1;  }  
  if(ControlRFilter < 127-Marge) {PWMout1R = 1; PWMout2R = 2*(127-ControlRFilter);  }
  TCNT0 = 130;  // reset onze eigen watchdog timer0

  if (Voltage > 135) {          // Als de batterij spanning voldoende hoog is, stuur de motoren aan
  analogWrite(3, PWMout1L);      // stuur de waardes effectief naar de H-Bruggen
  analogWrite(9, PWMout2L);  
  
  analogWrite(10, PWMout1R);
  analogWrite(11, PWMout2R);
  }
    TCNT2 = 00;  // reset timer 2
 
 
         //   Stuur alle gegevens naar het scherm, zet deze code actief om de potentiometers te calibreren en te debuggen
  /*
  Serial.println(Voltage);
  Serial.print("\t  ");

  Serial.print(PotL );
  Serial.print("\t  ");
  Serial.print(PotR );
  
  Serial.print("\t  ");
  TCNT2 = 00;  // reset timer 2
  Serial.print(ControlLFilter );
  Serial.print("\t  ");
  Serial.print(ControlRFilter );
  
  Serial.print("\t 1L ");
  Serial.print(PWMout1L );
  Serial.print("\t 2L ");
  Serial.print(PWMout2L );
  TCNT2 = 00;  // reset timer 2
  Serial.print("\t 1R ");
  Serial.print(PWMout1R );
  Serial.print("\t 2R ");
  Serial.print(PWMout2R );
  
  Serial.println( );
  */
}

//  De volgende code is de watchdog timer. Als de Arduino zou crashen, wordt de timer0 niet meer terug op 130 gezet en zal deze na enkele milliseconden op 255 staan 
//  en daarna een 'overflow interrupt' genereren. Dan gaan we gewoon onze arduino resetten. Dit gaat zo snel dat Pluim er zelfs niets van merkt.
//  Deze watchdog timer is niet strikt noodzakelijk, maar maakt het systeem wel betrouwbaarder.

void(* resetFunc) (void) = 0; //declare reset function @ address 0

ISR(TIMER0_OVF_vect) {   // Watchdog interrupt
  TIFR0 = 0x00;        //Timer2 INT Flag Reg: Clear Timer Overflow Flag
  resetFunc();         //call reset, reboot this controller
}
