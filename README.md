# WasserMax-2015
Bewässerungssystem und Frostschutzberegnung für Arduino Uno

Die Frostschutzberegnung ist ein Verfahren, dass es ermöglicht Frostschäden an Obstbäumen im Frühling zu vermeiden. 
Mit Hilfe eines Arduinos, einer 8-fach Relaiskarte für die Ansteuerung von Magnetventilen und einem Temperatursensor, 
kann dieses Projekt leicht umgesetzt werden.
Zusätzlich übernimmt das Programm die Funktion einer 7-kanaligen zeitgesteuerten Bewässerung, ein DCF77-Modul übernimmt
das Stellen der Uhr.

Unterstützte Funktionen
- Ansteuerung einer <a href="http://www.conrad.de/ce/de/product/197720/8fach-Relaiskarte-Baustein-12-24-VDC-8-Relaisausgaenge">8-fach Relais Karte </a>(für die direkte Ansteuerung ist eine Modifikation notwendig)
- Temperaturmessung über LM35 Sensor
- automaische Zeiteinstellung über DCF77 Empfänger<a href="http://www.conrad.de/ce/de/product/641138/CE-DCF-Empfaengerplatine/?ref=search&rt=search&rb=1">DVF77 Empänger</a>
- Setup / Programmierung / Info Betriebszustand  über 8fach LCD Display
- einfache Programmierung über 3 Tasten
- Programm für 7 Tage und 2 Schaltzeiten pro Kanal
- Wahl der Temperaturschwelle für Frostschutzberegnung
- Frei wählbarer Ausgang für Frostschutzberegnung
- einstellbare Pause der Frostschutzberegnung

Benötigte Libraries
- Bounce V1.5 by Thomas Ouellet Fredericks | http://www.arduino.cc/playground/Code/Bounce
- DCF77 V0.97 Thijs Elenbaas | https://github.com/thijse/Arduino-Libraries/downloads
- Time V1.4 by Michael Margolis | https://www.pjrc.com/teensy/td_libs_Time.html

Programmablauf
Nach dem Einschalten des Arduinos wird zunächst versucht die Uhr über das DCF77-Signal zu stellen. Danach wird das
Bewässerungsprogramm aus dem EEPROM geladen. Die Steuerung beginnt dann im Minutentakt die Umgebungstemperatur
zu messen, um nach 5 Minuten einen Mittelwert zu berechnen. Bei Unterschreitung des Schwellwertes wird am gewählten 
Ausgang die Frostschutzberegnung gestartet.
Die Behandlung des Frostschutzes ist prioritär, ein laufendes Programm des zugeordneten Relaisausganges wird in 
jedem Fall unterbrochen. 


