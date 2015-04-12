
/****************************************************
* 				Regenmacher V0.1t                         *
*         vom 12.04.2015                            *
* 				Tibor Banvölgyi 2015                      *
* 													                        *
* Ansteuerung Relais-Karte über Arduino mit DFC-Uhr *
* und 2x8 Display									                  *
****************************************************/

#include <DCF77.h>
#include <Utils.h>
#include <Bounce.h>
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <Time.h>
#include <Utils.h>

#define DCF_PIN 2                // DCF77 Empfänger initialisieren
#define DCF_INTERRUPT 0          // Interrupt für Empfänger reservieren
time_t time;
DCF77 DCF = DCF77(DCF_PIN,DCF_INTERRUPT, true);

/*Buttons zuweisen*/

#define BUTTON_Man 13
#define BUTTON_Val 3
#define BUTTON_Enter 8

/*Buttons entprellen*/

Bounce bouncer_Man = Bounce( BUTTON_Man,5 );
Bounce bouncer_Val = Bounce( BUTTON_Val,5 );
Bounce bouncer_Enter = Bounce( BUTTON_Enter,5 );

/*LCD initialisieren*/

LiquidCrystal lcd(12, 11, 7, 6, 5, 4);


int zeiten [7][7][4];/*Array für Tag,Relais,Zeit1-4*/

int minutenzeit; //Variable für Zeit in Minuten

byte tag=1;

boolean on[7]={0,0,0,0,0,0,0}; //Status der Ausgänge
byte addr=0;
byte val=0;
boolean mode=false; //Status Betriebmodus
byte cursor_pos=0; //Cursor Menü
byte zustand=0;
byte stunde_temp;//globale Variablen für das manipulieren Stunde
byte minute_temp;//globale Variablen für das manipulieren Minute
byte tag_temp;//globale Variablen für das manipulieren Tag
byte monat_temp;//globale Variablen für das manipulieren Monat
int jahr_temp;//globale Variablen für das manipulieren Jahr
byte lm35Pin = 5;//Input Pin fue Temp Sensor
int actualTemp; //ausgelesene Sensortemperatur
int avgTemp; //Durchschnittstemperatur
int threshold;//Schwellwert fuer Frostschutz
long frostend_time=0;//Ende des Frostschutzes in Unix Millis
long frostpause_end_time=0;//Ende Pause des Frostschutzes in Unix Millis
int frost_pause; //Pausenzeit aus EEPROM
int temperature_array[5]={0,0,0,0,0};//Temperatur Array fur Mittelwert
byte temperature_index=0;//Index Temperatur Array
byte minute_space=0;//Speicher fuer Minuten der Mittelwertsberechnung fuer Temperatur
boolean frost_status=false;//Status fuer Frostschutz
byte temperature_port;//Speicher fuer Frostschutz Port Relay

/*Ausgabe über den seriellen Port an die Relais definieren*/

SoftwareSerial mySerial(9, 10); // RX, TX

/*Arduino Setup*/

void setup () {


        avgTemp=0;

        //Pausenzeit aus EEPROM laden und in int umrechnen


        byte pause1=EEPROM.read(501);
        byte pause2=EEPROM.read(502);


        if (pause1==255) {
          frost_pause=0;
        } else {
          frost_pause=(pause1*255)+pause2;
        }

        //Port für Frostüberwachung aus EEPROM laden

        temperature_port=EEPROM.read(500);


        //Schwellwert aus EEPROM laden und in int umrechnen

        byte low, high;

        low=EEPROM.read(503);
        high=EEPROM.read(504);

        if ((low==255) && (high==255)) {
            threshold=0;
          } else {
            threshold = low + ((high << 8)&0xFF00);
          }





	Serial.begin(9600);
	mySerial.begin(19200);

  analogReference(INTERNAL);

	DCF.Start();

	setTime(0,0,0,01,01,15); //Uhrzeit  auf Standardzeit stellen

	//Buttons als Eingänge definieren
	pinMode(BUTTON_Man,INPUT);
	pinMode(BUTTON_Val,INPUT);
	pinMode(BUTTON_Enter,INPUT);

	for (byte i=0;i<7;i++)
		for (byte j=0;j<7;j++)
			for (byte k=0;k<4;k++)
				zeiten[i][j][k]=0;


  	lcd.begin(8, 2);
  	lcd.print("Wasser");
  	lcd.setCursor(0, 1);
  	lcd.print("Max 2015");

  	delay(1000);
  	lcd.clear();
  	lcd.setCursor(0, 0);
	lcd.print("Firmware");
	lcd.setCursor(0, 1);
	lcd.print("V0.1t");

    delay(1000);
    lcd.clear();
    lcd.setCursor(0, 0);
  lcd.print("FW Datum");
  lcd.setCursor(0, 1);
  lcd.print("12.04.15");
	 delay(1000);

	 stelle_uhr();

  	delay(1000);
  	lcd.clear();
  	lcd.setCursor(0, 0);
  	lcd.print("lade");
  	lcd.setCursor(0, 1);
  	lcd.print("Daten...");

  	/* lade Zeiten aus EEPROM*/
  	readEEPROM ();
  	delay(1000);

    //alle Ausgänge schliessen und Display initialisieren

  	sendCommand(1,0,0);
    sendCommand(7,0,255);

  	lcd.clear();
  	lcd.setCursor(0, 0);
  	lcd.print("A1234567");
  	lcd.setCursor(0, 1);
  	lcd.print(" 0000000");
  	}

int freeRam ()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}


void setup_menu (){
  //Setupmenü
    int f=1;
    mode=1;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.noBlink();
    lcd.print("1/5");
    lcd.setCursor(0, 1);
    lcd.print("Progr?");
    for (int i=0;i<7;i++){
      on[i]=0;
    }

    while (mode==true){

      if (bouncer_Val.update ( )== true){
          if (bouncer_Val.risingEdge()== true){
              if(f<5){
                f++;
            }
            else{
                f=1;
            }
          switch (f){
            case 1:
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("1/5");
                lcd.setCursor(0, 1);
                lcd.print("Progr.?");
                break;
              case 2:
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("2/5");
                lcd.setCursor(0, 1);
                lcd.print("DCF77");
                break;
              case 3:
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("3/5");
                lcd.setCursor(0, 1);
                lcd.print("Reset?");
                break;
              case 4:
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("4/5");
                lcd.setCursor(0, 1);
                lcd.print("Tempera?");
                break;
              case 5:
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("5/5");
                lcd.setCursor(0, 1);
                lcd.print("Exit?");
                break;

          }
        }
    }

    if (bouncer_Enter.update ( )== true){
      if (bouncer_Enter.risingEdge()== true){              
              switch (f){
                case 1:
                  zeiten_bearbeiten_tag();
                  break;
                case 2:
                  stelle_uhr();
                  break;
                case 3:
                  resetEEPROM ();
                    break;
                case 4:
                  temperatur_stellen();
                  break;
                case 5:
                    mode=false;
                  break;
              }  
              switch (f){
              case 1:
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("1/5");
                lcd.setCursor(0, 1);
                lcd.print("Progr?");
                break;
              case 2:
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("2/5");
                lcd.setCursor(0, 1);
                lcd.print("DCF77?");
                break;
              case 3:
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("3/5");
                lcd.setCursor(0, 1);
                lcd.print("Reset?");
                break;
            case 4:
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("4/5");
                lcd.setCursor(0, 1);
                lcd.print("Tempera?");
                break;
            case 5:
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("5/5");
                lcd.setCursor(0, 1);
                lcd.print("Exit?");
                break;

          }

       }
    }


  }
}


void setup_mode (){
  //Setup oberstes Menü - Abfrage ob
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Setup?");
    lcd.setCursor(0, 1);
    lcd.print("  j/n");
    lcd.setCursor(2, 1);
    lcd.blink();
    cursor_pos=0;
    mode=true;
    sendCommand(1,0,0);
    sendCommand(7,0,255);
    while (mode==true){
      if (bouncer_Val.update ( )== true){
          if (bouncer_Val.risingEdge()== true){
            if(cursor_pos==0){
                cursor_pos=1;
                lcd.setCursor(4, 1);
            }
            else {
                cursor_pos=0;
                lcd.setCursor(2, 1);
              }
        }
      }
      if (bouncer_Enter.update ( )== true){
          if (bouncer_Enter.risingEdge()== true){
            mode=false;
            lcd.noBlink();
            if (cursor_pos==0) {
                setup_menu ();
            }
            else {
                mode=false;
                lcd.noBlink();
            }
      }
    }
  }
  auto_initialise();
}



//Frost pruefen

void check_freeze(int avgTemp){ 

  if (avgTemp<threshold) {

    if ((frostend_time==0)&&(frostpause_end_time==0)) {
      frostend_time=now()+300;
      frostpause_end_time=0;

      frost_status=true;
    }


    if ((now() >= frostend_time) && (frostpause_end_time==0)) {

       frostpause_end_time=now()+(frost_pause*60);
       frostend_time=0;

    }

    if ((now() >= frostpause_end_time) && (frostend_time==0)){

      frostpause_end_time=0;
    }



  } else {

    if ((frostpause_end_time==0) && (frostend_time==0)) {
       frost_status=false;
    }

  }

}


//Hauptschleife
void loop () {

      //Temperatur mitteln

      if (minute_space < minute()){

          minute_space=minute();
          temperature_array[temperature_index]=getTemperature();
          temperature_index++;
          if (temperature_index==5){
            temperature_index=0;
            avgTemp=0;
            for (int x=0;x<5;x++){
              avgTemp=avgTemp+temperature_array[x];
            }
            avgTemp=avgTemp/5;

            check_freeze(avgTemp);
          }
      }

      if  (minute() ==  0) {
        minute_space=0;
      }


	/*Abfrage für den manuellen Modus*/
	if (bouncer_Man.update ( )== true){
		if (bouncer_Man.risingEdge()== true){
			mode=true;
			manual_mode ();
     	}
    }

  	/*Abfrage für Setup*/
  	if (bouncer_Val.update ( )== true){
    	if (bouncer_Val.risingEdge()== true){
       		setup_mode ();
     	}
	}

	/*Anzeige der aktuellen Uhrzeit und Temperatur*/
	if (bouncer_Enter.update ( )== true){
    	if (bouncer_Enter.risingEdge()== true){

       		lcd.clear();
       		lcd.setCursor(0, 0);
       		digitalClockDisplay();
       		lcd.setCursor(0, 1);
       		lcd.print("Tag: ");
       		/*lcd.print(weekday());*/

                switch (weekday()){
          	      case 1:
                          lcd.print("So");
                          break;
                      case 2:
                          lcd.print("Mo");
                          break;
                      case 3:
                          lcd.print("Di");
                          break;
                      case 4:
                          lcd.print("Mi");
                          break;
                      case 5:
                          lcd.print("Do");
                          break;
                      case 6:
                          lcd.print("Fr");
                          break;
                      case 7:
                          lcd.print("Sa");
                          break;
                  }

       		delay(2000);
       		lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Temp:");
        lcd.setCursor(0, 1);
        lcd.print(avgTemp);
        lcd.print((char)223);
        lcd.print("C");


        delay(2000);
        lcd.clear();

        lcd.setCursor(0, 0);
        lcd.print("SRAM:");
        lcd.setCursor(0, 1);
        lcd.print(freeRam ());
        lcd.print("b");
        delay(2000);
        lcd.clear();


  			lcd.setCursor(0, 0);
  			lcd.print("A1234567");
  			lcd.setCursor(0, 1);

  			for (int i=0;i<7;i++){
  				if (on[i]==0){
  					lcd.setCursor(i+1, 1);
  					lcd.print("0");
  				}
  				else {

            if (frostend_time>0){
              lcd.setCursor(i+1, 1);
              lcd.print("*");
            }

             if (frostpause_end_time>0){
              lcd.setCursor(i+1, 1);
              lcd.print("P");
            }

            if ((frostend_time==0) && (frostpause_end_time==0)) {
              lcd.setCursor(i+1, 1);
              lcd.print("X");
            }

  				}

          if ((frost_status==true) && (i==temperature_port) && (on[i]==1) ) {

            if (frostend_time > 0) {
              lcd.setCursor(i+1, 1);
              lcd.print("*");
            }

            if (frostpause_end_time > 0) {
              lcd.setCursor(i+1, 1);
              lcd.print("P");
            }

        }

  			}

     	}
	}


  	/*Uhrzeit in Minuten umrechnen*/
  	minutenzeit=hour()*60+minute();
  	/*Wochentag ermitteln*/
  	tag=weekday();
  	lcd.setCursor(0, 1);

  	/*Schleife für Ausgang (Hauptschleife)*/
    for (int y=0;y<7;y++) {
		  if ((minutenzeit >= zeiten[tag-1][y][0] && minutenzeit < zeiten[tag-1][y][1])||(minutenzeit >= zeiten[tag-1][y][2] && minutenzeit < zeiten[tag-1][y][3])) {
        	if ((on[y]==0)&&(frost_status==false)){
                sendCommand(1,0,0);
                sendCommand(6,0,bit_in_kommando(y+1));
                on[y]=1;
                lcd.setCursor(y+1, 1);
                lcd.print("X");

            }
      } else {
        	if ((on[y]==1)&&(frost_status==false)){
            	sendCommand(1,0,0);
            	sendCommand(7,0,bit_in_kommando(y+1));
            	on[y]=0;
        		 lcd.setCursor(y+1, 1);
        		 lcd.print("0");
        	}
    	  }

    if ((frost_status==true) && (y==temperature_port) ) {

        if (frostend_time > 0) {
          on[y]=1;
          sendCommand(1,0,0);
          sendCommand(6,0,bit_in_kommando(y+1));
          lcd.setCursor(y+1, 1);
          lcd.print("*");
        }

        if (frostpause_end_time > 0) {
          on[y]=1;
          sendCommand(1,0,0);
          sendCommand(7,0,bit_in_kommando(y+1));
          lcd.setCursor(y+1, 1);
          lcd.print("P");
        }
    }
	  }




}

void digitalClockDisplay(){
  	// digital clock display of the time
  	if (hour()<10){
  		lcd.print('0');}
  	lcd.print(hour());
  	lcd.print(":");
  	if (minute()<10){
  		lcd.print('0');}
  	lcd.print(minute());

}


//Temperatur von Sensor ermitteln
int getTemperature(){

         float temperature;
         int reading;

         reading = analogRead(lm35Pin);
         temperature = reading / 9.31;

         return temperature;


}

//

void sendCommand(int command, int cardAddr, int data){
	// Datenset für Relaiskarte formatieren
  	byte bytes[4];
  	bytes[0] = byte(command);
  	bytes[1] = byte(cardAddr);
  	bytes[2] = byte(data);
  	bytes[3] = bytes[0] ^ bytes[1] ^ bytes[2];

  	writeBytes(bytes,4);
  	delay(250);
}

void writeBytes(byte arr[], int len){
	// Bytes an die serielle Schnittstelle senden
  	for(int i = 0; i < len; i++){
    	mySerial.write(arr[i]);
  	}
}

int bit_in_kommando(int stelle) {
		// Umrechnung Bitfolge für Relaiskarte in Bytekommando
	    int kommando_erg=0;
		kommando_erg=bit(stelle-1);
		return kommando_erg;
	}

/*Zeiten im EEPROM lesen*/
void readEEPROM (){
  Serial.println("load");
	int adr=0;
	int byte1=0;
	int byte2=0;
	int wert=0;
	int stunde_anz=0;
	int minute_anz=0;

  	for (int i=0;i<7;i++)
    	for (int j=0;j<7;j++)
      		for (int k=0;k<4;k++){

        		//adr=((i)*7)+((j)*7)+(2*k)+1; // Berechnung der Adresse alt


            adr=((k)*2)+((j)*8)+((i)*64); //Adresse berechnen neu
            Serial.println(adr);
				    byte1=EEPROM.read(adr);
				    byte2=EEPROM.read(adr+1);
				    wert=byte1*255+byte2;
        		zeiten [i][j][k]=wert;

      		}
   Serial.println("load ende");
}

/*Zeiten im EEPROM löschen*/
void resetEEPROM (){
  Serial.println("reset");
	int adr=0;
	int byte1=0;
	int byte2=0;
	int wert=0;
	int stunde_anz=0;
	int minute_anz=0;

  	for (int i=0;i<7;i++)
    	for (int j=0;j<7;j++)
      		for (int k=0;k<4;k++){

            adr=((k)*2)+((j)*8)+((i)*64); 
            Serial.println(adr);
        		
				EEPROM.write(adr,0);
				EEPROM.write(adr+1,0);


      		}
    lcd.clear();
  	lcd.setCursor(0, 0);
  	lcd.print("Reset");
  	lcd.setCursor(0, 1);
  	lcd.print("OK");
  	delay(2000);
     Serial.println("reset ende");

}

//Funktion für den manuellen Modus

void manual_mode (){

  	lcd.clear();
  	lcd.setCursor(0, 0);
  	lcd.print("man.");
  	lcd.setCursor(0, 1);
  	lcd.print("Modus");
  	delay(1000);
  	lcd.clear();
  	lcd.setCursor(0, 0);
  	lcd.print("M1234567");
  	lcd.setCursor(0, 1);
  	lcd.print(" 0000000");
  	lcd.setCursor(1, 1);
  	lcd.blink();
  	int ausgang=1;


  	for (int i=0;i<7;i++){
  		on[i]=0;

  	}
  	sendCommand(1,0,0);
  	sendCommand(7,0,255);

  	while (mode==true){

     	if (bouncer_Val.update ( )== true){
       		if (bouncer_Val.risingEdge()== true){
       			if (ausgang<7){
       					ausgang++;
       					lcd.setCursor(ausgang, 1);
  						lcd.blink();


       			}
       			else {
       					ausgang=1;
       					lcd.setCursor(ausgang, 1);
  						lcd.blink();

       			}
     		}
     	}

     	if (bouncer_Enter.update ( )== true){
       		if (bouncer_Enter.risingEdge()== true){
     			if 	(on[ausgang-1]==0) {
     				on[ausgang-1]=1;
     				sendCommand(1,0,0);

                	sendCommand(6,0,bit_in_kommando(ausgang));
     				lcd.setCursor(ausgang, 1);
     				lcd.print("X");
     				lcd.setCursor(ausgang, 1);

     			}
     			else{
     				on[ausgang-1]=0;
     				sendCommand(1,0,0);
            		sendCommand(7,0,bit_in_kommando(ausgang));

     				lcd.setCursor(ausgang, 1);
     				lcd.print("0");
     				lcd.setCursor(ausgang, 1);
     			}
     		}
     	}



     	if (bouncer_Man.update ( )== true){
       		if (bouncer_Man.risingEdge()== true){
         		lcd.noBlink();
         		mode=false;
         		auto_initialise();


         		for (int i=0;i<7;i++){
  					on[i]=0;
  				sendCommand(1,0,0);
            	sendCommand(7,0,255);
  				lcd.setCursor(0, 1);
  				lcd.print(" 0000000");

  				}

       		}
      	}
  	}
}



//Anzeigereset nach manuellem Modus
void auto_initialise(){

	lcd.clear();
	lcd.setCursor(0, 0);
    lcd.print("A1234567");
    for (int y=0;y<7;y++){
    	if (on[y]){
        	lcd.setCursor(y+1, 1);
            lcd.print("X");
        }
        else {
        	lcd.setCursor(y+1, 1);
            lcd.print("0");
        }
    }
}







//Bearbeiten der Zeiten Auswahl Tag
void zeiten_bearbeiten_tag(){

	boolean mode_zt=true;
    int i_tag=2;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Tag?");
    lcd.setCursor(0, 1);
    lcd.print("Mo");
    while (mode_zt==true){
     	if (bouncer_Val.update ( )== true){

       		if (bouncer_Val.risingEdge()== true){
          		if(i_tag<7){
           			i_tag++;
         		}
         		else{
           			i_tag=1;
         		}
        	switch (i_tag){
          	case 1:
            		lcd.clear();
            		lcd.setCursor(0, 0);
    				lcd.print("Tag?");
    				lcd.setCursor(0, 1);
    				lcd.print("So");
            		break;
          	case 2:
            		lcd.clear();
            		lcd.setCursor(0, 0);
    				lcd.print("Tag?");
    				lcd.setCursor(0, 1);
    				lcd.print("Mo");
            		break;
          	case 3:
            		lcd.clear();
            		lcd.setCursor(0, 0);
    				lcd.print("Tag?");
    				lcd.setCursor(0, 1);
    				lcd.print("Di");
             		break;
          	case 4:
            		lcd.clear();
            		lcd.setCursor(0, 0);
    				lcd.print("Tag?");
    				lcd.setCursor(0, 1);
    				lcd.print("Mi");
            		break;
            	case 5:
            		lcd.clear();
            		lcd.setCursor(0, 0);
    				lcd.print("Tag?");
    				lcd.setCursor(0, 1);
    				lcd.print("Do");
            		break;
            	case 6:
            		lcd.clear();
            		lcd.setCursor(0, 0);
    				lcd.print("Tag?");
    				lcd.setCursor(0, 1);
    				lcd.print("Fr");
            		break;
            	case 7:
            		lcd.clear();
            		lcd.setCursor(0, 0);
    				lcd.print("Tag?");
    				lcd.setCursor(0, 1);
    				lcd.print("Sa");
            		break;
        	}
       	}
	}

    if (bouncer_Enter.update ( )== true){
    	if (bouncer_Enter.risingEdge()== true){

          		switch (i_tag){
            		case 1:
            			mode_zt=false;
            			zeiten_bearbeiten_output(i_tag);
            			break;
            		case 2:
            			mode_zt=false;
            			zeiten_bearbeiten_output(i_tag);
              			break;
            		case 3:
            			mode_zt=false;
            			zeiten_bearbeiten_output(i_tag);
              			break;
                case 4:
                    mode_zt=false;
                    zeiten_bearbeiten_output(i_tag);
                    break;
            		case 5:
              			mode_zt=false;
              			zeiten_bearbeiten_output(i_tag);
               			break;
            		case 6:
              			mode_zt=false;
              			zeiten_bearbeiten_output(i_tag);
               			break;
            		case 7:
              			mode_zt=false;
              			zeiten_bearbeiten_output(i_tag);
               			break;
          		}
       }
    }
  }
}



//Zeiten bearbeiten Ausgang
void zeiten_bearbeiten_output(int tag){

	boolean mode_zto=true;
	int i_out;
	i_out=1;
	lcd.clear();
  lcd.setCursor(0, 0);


    switch (tag) {
        case 1:
          lcd.print("So");
          break;
        case 2:
          lcd.print("Mo");
          break;
        case 3:
          lcd.print("Di");
          break;
        case 4:
          lcd.print("Mi");
          break;
        case 5:
          lcd.print("Do");
          break;
        case 6:
          lcd.print("Fr");
          break;
        case 7:
         lcd.print("Sa");
         break;

    }


    lcd.setCursor(0, 1);
    lcd.print("Ausg. 1");

    while (mode_zto==true){
     	if (bouncer_Val.update ( )== true){
       		if (bouncer_Val.risingEdge()== true){
          		if( i_out<7){
           			 i_out++;
         		}
         		else{
           			 i_out=1;
         		}
        	switch ( i_out){
          		case 1:
    				lcd.setCursor(0, 1);
    				lcd.print("Ausg. 1");
            		break;
          		case 2:
    				lcd.setCursor(0, 1);
    				lcd.print("Ausg. 2");
            		break;
          		case 3:
    				lcd.setCursor(0, 1);
    				lcd.print("Ausg. 3");
             		break;
          		case 4:
    				lcd.setCursor(0, 1);
    				lcd.print("Ausg. 4");
            		break;
            	case 5:
    				lcd.setCursor(0, 1);
    				lcd.print("Ausg. 5");
            		break;
            	case 6:
    				lcd.setCursor(0, 1);
    				lcd.print("Ausg. 6");
            		break;
            	case 7:
    				lcd.setCursor(0, 1);
    				lcd.print("Ausg. 7");
            		break;
        	}
       	}
	}

    if (bouncer_Enter.update ( )== true){
    	if (bouncer_Enter.risingEdge()== true){

          		switch (i_out){
            		case 1:
            			zeiten_bearbeiten_zeiten(tag,1);
            			mode_zto=false;
            			break;
            		case 2:
            			zeiten_bearbeiten_zeiten(tag,2);
            			mode_zto=false;
              			break;
            		case 3:
            			zeiten_bearbeiten_zeiten(tag,3);
            			mode_zto=false;
              			break;
              		case 4:
            			zeiten_bearbeiten_zeiten(tag,4);
            			mode_zto=false;
              			break;
            		case 5:
            			zeiten_bearbeiten_zeiten(tag,5);
              			mode_zto=false;
               			break;
            		case 6:
            			zeiten_bearbeiten_zeiten(tag,6);
              			mode_zto=false;
               			break;
            		case 7:
            			zeiten_bearbeiten_zeiten(tag,7);
              			mode_zto=false;
               			break;
          		}
       }
    }
  }

}

//Zeiten bearbeiten Auswahl der Zeit
void zeiten_bearbeiten_zeiten(int tag,int ausgang){

	int stunde_temp_zbz=0;
	int minute_temp_zbz=0;
	boolean mode_zbz=true;
	boolean mode_zbz2=true;
	String vorgang="";
	int stunde_anz=0;
	int minute_anz=0;
	int wert=0;
	int adr=0;
	int byte1=0;
	int byte2=0;
	int func_iz;

    for (int i_zbz=0;i_zbz<4;i_zbz++){
    	adr=((tag-1)*64)+((ausgang-1)*8)+(2*i_zbz);
		  byte1=EEPROM.read(adr);
		  byte2=EEPROM.read(adr+1);
		  wert=byte1*255+byte2;
		  stunde_anz=wert/60;
		  minute_anz=wert%60;

    	switch (i_zbz){
    		case 0:
    			vorgang="Start 1";
    			break;
    		case 1:
    			vorgang="Stop 1";
    			break;
    		case 2:
    			vorgang="Start 2";
    			break;
    		case 3:
    			vorgang="Stop 2";
    			break;
    		}

    	lcd.clear();
    	lcd.setCursor(0, 0);
    	lcd.print(vorgang);
    	lcd.setCursor(0, 1);
    	if (stunde_anz<10) {
    		lcd.print("0");
    	}
    	lcd.print(stunde_anz);
    	lcd.print(":");
    	if (minute_anz<10) {
    		lcd.print("0");
    	}
    	lcd.print(minute_anz);
    	lcd.setCursor(0, 1);
    	lcd.blink();
    	func_iz=stunde_anz;
		while (mode_zbz==true){

			if (bouncer_Val.update ( )== true){
				if (bouncer_Val.risingEdge()== true){
					if( func_iz<23){
						 func_iz++;
					}
					else{
						 func_iz=0;
					}

					lcd.setCursor(0,1);

					if ( func_iz<10){
						lcd.print("0");
						lcd.print( func_iz);
					}
					else {
						lcd.print( func_iz);
					}
					lcd.setCursor(1,1);
				}
			}

			if (bouncer_Enter.update ( )== true){
				if (bouncer_Enter.risingEdge()== true){
					stunde_temp_zbz=func_iz;

					mode_zbz=false;
					lcd.setCursor(4,1);
					lcd.blink();
				}
			}

		}

		lcd.setCursor(4,1);
		lcd.blink();
		func_iz=minute_anz;
		 while (mode_zbz2==true){

			if (bouncer_Val.update ( )== true){
				if (bouncer_Val.risingEdge()== true){
					if(func_iz<59){
						func_iz++;
					}
					else{
						func_iz=0;
						}

					lcd.setCursor(3,1);

					if (func_iz<10){
						lcd.print("0");
						lcd.print(func_iz);
						}
					else {
						lcd.print(func_iz);
						}
					lcd.setCursor(4,1);
				}
			}

			if (bouncer_Enter.update ( )== true){
				if (bouncer_Enter.risingEdge()== true){
					minute_temp_zbz=func_iz;

					mode_zbz2=false;
				}
			}
		}

		wert=stunde_temp_zbz*60+minute_temp_zbz;
		byte1=wert/255;
		byte2=wert%255;  
   
		adr=((tag-1)*64)+((ausgang-1)*8)+(2*i_zbz);
   
		EEPROM.write(adr, byte1);
		EEPROM.write(adr+1, byte2);

		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print(vorgang);
		lcd.setCursor(0, 1);
		lcd.print("gespeichert");
		delay(1500);
		mode_zbz2=true;
		mode_zbz=true;
		lcd.noBlink();
		readEEPROM ();
	}
}


//Menü für Einstellungen Frostschutz
void temperatur_stellen(){

    int i=1;
    boolean k_mode=1;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.noBlink();
    lcd.print("1/3");
    lcd.setCursor(0, 1);
    lcd.print("Ausgang");


    while (k_mode==true){
      if (bouncer_Val.update ( )== true){
          if (bouncer_Val.risingEdge()== true){
              if(i<4){
                i++;
            }
            else{
                i=1;
            }
          switch (i){
            case 1:
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("1/4");
                lcd.setCursor(0, 1);
                lcd.print("Ausgang");
                break;
              case 2:
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("2/4");
                lcd.setCursor(0, 1);
                lcd.print("Pause");
                break;
              case 3:
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("3/4");
                lcd.setCursor(0, 1);
                lcd.print("Schwelle");
                break;
              case 4:
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("4/4");
                lcd.setCursor(0, 1);
                lcd.print("Exit");
                break;

          }
        }
    }

    if (bouncer_Enter.update ( )== true){
      if (bouncer_Enter.risingEdge()== true){

              switch (i){
                case 1:
                   temp_port();
                  break;
                case 2:
                   temp_pause_zeit();
                  break;
                case 3:
                   temp_threshhold();
                  break;
                case 4:
                  k_mode=false;
                  break;
              }


              switch (i){
              case 1:
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("1/4");
                lcd.setCursor(0, 1);
                lcd.print("Ausgang");
                break;
              case 2:
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("2/4");
                lcd.setCursor(0, 1);
                lcd.print("Pause");
                break;
               case 3:
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("3/4");
                lcd.setCursor(0, 1);
                lcd.print("Schwelle");
                break;
              case 4:
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("4/4");
                lcd.setCursor(0, 1);
                lcd.print("Exit");
                break;

          }

       }
    }

  }
}

void temp_port(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ausgang:");
  lcd.setCursor(0, 1);
  lcd.print(temperature_port+1);
  lcd.blink();

  boolean t_mode=true;

  while (t_mode==true){
     if (bouncer_Val.update ( )== true){
          if (bouncer_Val.risingEdge()== true){

            if (temperature_port<6) {
              temperature_port++;
              lcd.setCursor(0, 1);
              lcd.print(temperature_port+1);
            } else {
              temperature_port=0;
              lcd.setCursor(0, 1);
              lcd.print("1");
            }

          }
      }

      if (bouncer_Enter.update ( )== true){
          if (bouncer_Enter.risingEdge()== true){
              EEPROM.write(500,temperature_port);
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("1/3");
              lcd.setCursor(0, 1);
              lcd.print("Ausgang");
              lcd.noBlink();
              t_mode=false;
          }
      }
  }
}

void temp_threshhold(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Schwelle:");
  lcd.setCursor(0, 1);
  lcd.print(threshold);
  lcd.print((char)223);
  lcd.print("C");
  lcd.blink();

  byte low, high;

  boolean p_mode=true;

  while (p_mode==true){
     if (bouncer_Val.update ( )== true){
          if (bouncer_Val.risingEdge()== true){

            if (threshold<25) {
              threshold++;
              lcd.setCursor(0, 1);
              lcd.print("        ");
              lcd.setCursor(0, 1);
              lcd.print(threshold);
              lcd.print((char)223);
              lcd.print("C");
            } else {
              threshold=-5;
              lcd.setCursor(0, 1);
              lcd.print("-5");
              lcd.print((char)223);
              lcd.print("C");
            }

          }
      }

      if (bouncer_Enter.update ( )== true){
          if (bouncer_Enter.risingEdge()== true){


              low=threshold&0xFF;
              high=(threshold>>8)&0xFF;
              EEPROM.write(503, low);
              EEPROM.write(504, high);

              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("1/3");
              lcd.setCursor(0, 1);
              lcd.print("Ausgang");
              lcd.noBlink();
               p_mode=false;
          }
      }
  }
}

void temp_pause_zeit(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pause:");
  lcd.setCursor(0, 1);
  lcd.print(frost_pause);
  lcd.print(" min");
  lcd.blink();

  int f_mode=true;
  byte mod_frost1;
  byte mod_frost2;

  while (f_mode==true){
     if (bouncer_Val.update ( )== true){
          if (bouncer_Val.risingEdge()== true){

            if (frost_pause<190) {
              frost_pause=frost_pause+10;
              lcd.setCursor(0, 1);
              lcd.print("        ");
              lcd.setCursor(0, 1);
              lcd.print(frost_pause);
              lcd.print(" min");
            } else {
              frost_pause=10;
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Pause:");
              lcd.setCursor(0, 1);
              lcd.print("10 min");
            }

          }
      }

      if (bouncer_Enter.update ( )== true){
          if (bouncer_Enter.risingEdge()== true){

              mod_frost1=(byte)frost_pause / 256;
              mod_frost2=frost_pause % 256;

              EEPROM.write(501,mod_frost1);
              EEPROM.write(502,mod_frost2);
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("2/3");
              lcd.setCursor(0, 1);
              lcd.print("Pause");
              lcd.noBlink();
              f_mode=false;
          }
      }
  }
}










void stelle_uhr() {

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("empfange");
  lcd.setCursor(0, 1);
  lcd.print("DCF77");

  boolean zeitloop=0;
  do  {
    time_t DCFtime = DCF.getTime(); // Prüfen, ob neue DCF Zeit verfügbar ist

    if (bouncer_Man.update ( )== true){
		if (bouncer_Man.risingEdge()== true){

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("DCF77");
      lcd.setCursor(0, 1);
      lcd.print("Abbruch");
      delay (1000);

			zeitloop=1;

     	}
    }


    if (DCFtime!=0)
    {
      setTime(DCFtime);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Uhr");
      lcd.setCursor(0, 1);
      lcd.print("gestellt");
      delay (1000);
      zeitloop=1;

    }
   } while (zeitloop==0);
}
