ttyIBIS - IBIS-Telegramme senden unter Linux
============================================

*(Scroll down for English version)*

ttyIBIS ist ein Kommandozeilentool, um unter Linux über die serielle 
Schnittstelle Nachrichten an IBIS-Slaves schicken zu können.

Aufruf
-----
Beim Aufruf erwartet ttyIBIS zwei Parameter - die zu verwendende serielle 
Schnittstelle und das zu sendende Telegramm.

**Beispiel**

	ttyIBIS /dev/ttyUSB0 l001

Das Telegramm "l001" wird über die serielle Schnittstelle /dev/ttyUSB0 gesendet.

ttyIBIS fügt automatisch das Steuerzeichen \<CR> und die Prüfsumme am Ende des 
Telegramms ein. Auch die Codierung von Zeichen außerhalb des 7-Bit 
ASCII-Zeichensatzes werden korrekt umgewandelt.  
Soll ein Telegramm mit Leerzeichen enden, ist es notwendig, den letzten 
Parameter in Anführungszeichen "" zu setzen. Ansonsten werden die Leerzeichen
von der Shell entfernt.

Spezielle Platzhalter im Telegramm
----------------------------------
Mit der Kombination aus dem Unterstrich _ und einem weiteren Zeichen lassen 
sich verschiedene spezielle Zeichen erzeugen:

* _n erzeugt einen Zeilenumbruch
* _0 bis _9 und _A bis _F erzeugen den Wert im "IBIShex"-Format
* __ (doppelter Unterstrich) erzeugt einen einfachen Unterstrich

ttyIBIS kompilieren
-------------------
ttyIBIS benötigt keine speziellen Bibliotheken und sollte sich auf jedem
aktuellen Linux mit GNU make, GCC und Standard-libc erstellen lassen.
Zum Erstellen der Binärdatei im src-Verzeichnis 

	make

ausführen. Die Binärdatei ttyIBIS wird im gleichen Verzeichnis erzeugt und kann
ins gewünschte Verzeichnis verschoben werden. Es bestehen keine lokalen 
Abhängigkeiten

ttyIBIS - send IBIS telegrams in Linux
======================================
ttyIBIS is a command line tool to send messages to IBIS slave devices via the
serial port of a computer running Linux.

Usage
-----
The program expects two parameters - the serial port to use and the message 
to send.

**Example**

	ttyIBIS /dev/ttyUSB0 l001

The message "l001" will be sent via the serial port /dev/ttyUSB0.

ttyIBIS automatically adds the control character \<CR> and the checksum at the 
end of the message. Furthermore, characters outside of 7-bit ASCII (German 
Umlauts and ß) are converted as required.  
If trailing whitespace is to be sent, the second parameter has to be wrapped
in double quotes "", as the shell will otherwise trim the whitespace off.

Special placeholders within a message
-------------------------------------
By combining an underscore: and another character, special characters can be
inserted into the message:

* _n creates a newline character
* _0 through _9 and _A through _F create the numeric value in "IBIShex" format
* __ (double underscore) creates a single underscore

Building ttyIBIS
----------------
ttyIBIS does not require any special libraries and should be possible to build 
on any recent Linux system offring GNU make, GCC and the standard libc.
To build the binary executable, run

	make

in the src directory. The executable, called ttyIBIS, can then be moved to a
directory of the user's choice, there are no local dependencies.