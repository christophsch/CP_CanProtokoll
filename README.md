CP Protokoll Beschreibung
===========

CP-Protokoll Features
----------------
- Daten- und Teilnehmererkennung
- Segmentierung, Desegmentierung fuer beliebig grosse Datenpakete
- Fluss-Steuerung
- Daten-Konsistenz
- Wenig Overhead
- Robustheit


Benutzung des CP-Protokolls
------------------------
Das CP-Protokoll selbst unterstuetzt keinen Code fuer jeden Mikrocontroller,
das Protokoll wurde speziell fuer den Mikrocontroller STM32F303K8 angepasst,
und baut auf den von CubeMx: erzeugten Code auf.
CubeMX Version 4.20.0, Firmware-Package: FW F3 V1.8.0


Dokumentation
-------------------------
Der Code ist hauptsaechlich in den Source-Code Files dokumentiert.
Bei Elementen, die im Source-Code nicht verwendet werden, wird in den 
Header-Files dokomentiert.

Grobe Uebersicht des Aufbaus:
-------------------------------------------------
~~~
                            -----------------------
                           |     Programm Start     |
                            -----------------------
                                       |
                            -----------------------
                           |   CP-Protokoll_Init   |
                            -----------------------
                                       |
                            -----------------------
                           |     Main-Routine      |
                           |                       |
                           | - Initialisiere       |
                           |   Sende-Datenobjekt   |
                           | - Starte              |
                           |   Senden Datenobjekt  |
                           |                       |
                           | - Initialisiere       |
                           |   Empf.-Datenobjekt   |
                           | - Starte              |
                           |   Empf. Datenobjekt   |
                           |                       |
                            -----------------------
                                 |     |  
             --------------------      | 
            |                          | 
 -----------------------    ----------------------- 
| CAN Empfangs-Prozess  |  | CAN Sende-Prozess     |
|                       |  |                       |
| - Empfange CAN-       |  | - Segmentierung       |
| - Nachrichten         |  |   des Datenobjekts    |
|   Inerruptgesteuert   |  |   auf einzelne        |
|   zur Auswertung      |  |   Datenpakete         |
|                       |  |                       |
| - Filter ID           |  | - Versende einzelne   |
| - Filter Frame-Nummer |  |   Datenpakete         |
| - Speichern der       |  |   Interrupt getrieben |
|   empfangenen Daten   |  |                       |
|   in Datenobjekt      |  |                       |
|                       |  |                       |
 -----------------------    ----------------------- 
~~~


File structure
--------------
 - **cp_user.c/.h** - Anwender-Interface: Anwenderfunktionen zur Bedienung des CP-Protokolls.
 - **cp_control.c/.h** - Steuerungsschicht: Interrupt-getriebener Sende- und Empfangsprozess.
 - **cp_example.c** - Beispiel-Applikation, wie Anwender-Funtionen genutzt werden.
 - **main.c** - Beispiel-Implementierung zur Beschreibung, wie das CP-Protokoll eingebunden wird.
 - **Doxyfile** - Konfiguration zur Code-Dokumentation mit Doxygen*.
 - **README.md** - This file

Beschreiung des CP-Protokolls:
--------------

### Konzept
Aufbauend auf Protokollschicht 1 und 2 werden in Abstimmung mit der Anforderungsanalyse zur CAN-Kommunikation zwischen den Mikrocontrollern die 3. und 4. Protokollschicht, Vermittlung und Transport, benoetigt. 
Konkret geht es um die 3 Hauptpunkte: Teilnehmer- / Datenerkennung, Segmentierung zum Versenden groesserer Daten als die Kapazitaet einer einzelnen CAN-Nachricht und eine Fluss-Steuerung zur stoerungsfreien Kommunikation. 


### Datenobjekt
Der Austausch von Daten zwischen den Teilnehmern passiert durch identische Speicherstrukturen auf jedem Mikrocontroller. Somit muss waehrend der Laufzeit kein Speicher allokiert werden, da im Voraus bekannt ist welche Daten versendet und empfangen werden. 

Jeder Mikrocontroller besitzt seine eigenen Datenobjekte, sowie Datenobjekte anderer Teilnehmer im CAN-Netzwerk. Die eigenen Datenobjekte werden zur Laufzeit veraendert und koennen auf den CAN-Bus gesendet werden, sodass diese fuer andere Knoten im Netzwerk zur Verfuegung stehen. Die Empfaenger eines Datenobjekts muessen eine identische Datenstruktur im Speicher deklariert haben um dieses @ref Datenobjekt empfangen, beziehungsweise speichern zu koennen. Bei dem Empfang werden die Daten dann automatisch in das deklarierte @ref Datenobjekt geschrieben. Von hier aus koennen die Daten, nach erfolgreichem Empfang, durch den Anwender ausgelesen und verwendet werden.

Die Namensgebung der Datenobjekte ist beliebig und nicht festgeschrieben. Ein zu versendendes @ref Datenobjekt kann dabei ein beliebiger Datentyp sein. Zum Beispiel ist das eine einfache Integer-Variable. Zur Buendelung von mehreren Daten bieten sich ein Array oder ein Struktur-Element an, welches wiederum mit beliebigen Datentypen gefuellt sein kann. 


### Datenerkennung
Zur Erkennung um welches @ref Datenobjekt es sich handelt wird in dem umgesetzten Protokoll festgelegt, dass jedes Datenobjekt durch mindestens einen nicht anderweitig vergebenen Identifier beschrieben wird. Somit ist eine eindeutige Zuordnung von der ID auf das verwendete Datenobjekt moeglich. Die Zuordnung von einer ID zu einem Datenobjekt erfolgt vor dem Versenden / Empfangen von Datenobjekten in der Initialisierungsphase durch den Anwender.
Bei der Laenge von 11-bit des CAN-Identifiers eines Standard-CAN-Frames kann bei einer Anzahl von 128 Bus-Teilnehmern, jeder Knoten 16 unterschiedliche Datenobjekte versenden. Bei einer geringeren Anzahl von Knoten dementsprechend mehr. Die Identifier-Aufteilung zu Datenobjekten wird dem Anwender zur freien Verfuegung ueberlassen. Auch ist der CAN-Identifier nicht fest an ein Datenobjekt gekoppelt. Es wird empfohlen, jedem Datenobjekt einen Identifier zuzuordnen damit es im Programmablauf nicht zu Verwechslungen kommt. Es ist jedoch moeglich ein Datenobjekt abwechselnd durch mehrere verschiedene IDs zu versenden. Umgekehrt darf aber jede ID nur einem Datenobjekt zugeordnet sein.
Ein CAN-Hardware-Empfangsfilter direkt auf dem CAN-Controller wird nicht eingesetzt. Generell wird jede Nachricht auf dem BUS empfangen und anschliessend geprueft ob diese Nachricht und somit das Datenobjekt benoetigt wird oder nicht. Sollte der Identifier nicht initialisiert sein oder der Empfang des Datenobjekts nicht gewuenscht, werden diese Nachrichten nach dessen Pruefung verworfen. 
Eine Teilnehmer-Erkennung erfolgt, wie auch in den unteren CAN-Layers nicht. Diese ist nicht benoetigt und kann durch den Anwender selbst erfolgen. Jedes Datenobjekt kann ueber den Identifier eindeutig gekennzeichnet werden. In dem umgesetzten Protokoll werden keine IDs fuer bestimmte Teilnehmer oder Funktionen blockiert. Somit ist es dem Anwender des Protokolls selbst moeglich Teilnehmer zu erkennen, indem zum Beispiel jeder CAN-Bus-Teilnehmer eine bestimmte ID-Range fuer das Senden von Datenobjekten zugeteilt bekommt. 


### Segmentierung
Eine CAN-Botschaft fasst maximal 8 Bytes an Daten. Da der Versand von Datenmengen schnell 8 Byte (16 ASCII-Zeichen) uebersteigen koennte, wird ein Datenmanagement in der Transportschicht notwendig. Die Nachrichten muessen durch den Sender aufgeteilt und beim Empfaenger wieder zusammengesetzt werden. Um dem Empfaenger in jeder Botschaft mitzuteilen um welchen Teil der Gesamt-Nachricht es sich bei der jeweiligen CAN-Botschaft handelt, werden 2 von 8 Bytes des Datenteils verwendet um diese Information zusaetzlich zu uebermitteln.
Bei diesen 2 Bytes an Adressraum fuer die Frame-Erkennung ergibt sich die Maximal-Groesse eines zu uebertragenden Datenobjekts von 216 * 6 Byte = 384 kbyte. Dies ist bei Weitem ausreichend, alleine dadurch, dass der verfuegbare Speicher des Mikrocontrollers 32 Kbyte besitzt. Ein einzelnes Byte an Overhead waere jedoch wenig bei einer sich dann ergebenden Maximal-Groesse von nur einem Kilobyte.

Fluss-Steuerung
Das Versenden der Datenobjekte soll durch einen gezielten manuellen Aufruf erfolgen. Da auf den einzelnen Mikrocontrollern wie z.B. dem Boost-Converter zeitkritische Funktionen laufen, soll der Versand eines Datenobjekts schnellst moeglich stattfinden und es soll Anwendungen mit hoeherer Prioritaet nicht unnoetig blockieren. Auf der anderen Seite wird eine Flusssteuerung benoetigt um den Empfang von Datenobjekten zu steuern, sodass der Programm-Ablauf nicht unnoetig lang verzoegert wird und gleichzeitig der Verlust von Datenpaketen verhindert wird. Erreicht und Umgesetzt werden diese Anforderung durch einen Interrupt-getriebenen Empfang und Versand der Datenobjekte.

### Daten-Konsistenz
Die Interrupt-getriebene CAN-Kommunikation hat zur Folge, dass es zur Daten-Inkonsistenz kommen kann. Dies geschieht, wenn die Daten eines Datenobjekts im Hauptprogramm geaendert werden, obwohl die Daten dieses Objekts noch nicht vollstaendig versendet wurden oder sofern ein Datenobjekt gelesen wird, obwohl die Daten dieses Objekts noch nicht vollstaendig empfangen wurden. Hierbei kann es zu unvorhersehbaren Fehlinformationen kommen. 
An einem kurzen Beispiel soll dies kurz gezeigt werden. Ein String-Datenobjekt mit dem Inhalt „MOTOR 1 ist in Betrieb“ wird nach 7 Zeichen geteilt und versendet. Der aktuelle Sendezustand von 7 versendeten Zeichen wird gespeichert. Nun wird im Hauptprogramm der String geaendert zu „MOTOR 2 ist ausser Betrieb“. Die Sendefunktion wird nun bei dem 8. Zeichen fortsetzen, was beim Empfaenger zu folgender Nachricht fuehrt: „MOTOR 1 ist ausser Betrieb.“ Eine komplette Fehlinformation. 
Fuer genanntes Problem gibt es unterschiedliche Loesungsszenarien: Eine waere eine Speicherzugriffs-Regelung, die das aendern der Daten verweigert, sofern diese Daten noch nicht komplett versendet wurden. Der Implementierungsaufwand ist hier verhaeltnismaessig hoch. Ein weiterer Loesungsansatz waere, die Daten des Datenobjekts vor dem Senden zu kopieren und die Kopie der Daten zu versenden. Hierzu muss jedoch Speicher zu Verfuegung gestellt werden. Dies kann erfolgen, indem fuer jedes Datenobjekt eine zusaetzliche Datenobjekt-Kopie anlegt wird und der Sende- und Empfangs-Funktion die Adressen beider Datenobjekte mitteilt werden. Besser waere dann eine dynamische Speicherplatz-Reservierung zur Laufzeit des Programms um nicht unnoetig viel Speicherplatz zu verbrauchen.
Da die Wahrscheinlichkeit extrem gering ist, waehrend des kurzen Sendeprozesses die Werte des Datenobjekts zu aendern oder waehrend des Empfangs das Datenobjekt auszulesen wird auf die Implementierung der oben genannten Moeglichkeiten verzichtet und dem Anwender eine einfache Moeglichkeit zur Ueberpruefung  des aktuellen Sende- und Empfangs-Status geliefert. Somit kann sichergestellt werden, das Datenobjekt erst nach erfolgreichem Versand/Empfang zu schreiben/lesen.


###Aufbau und Dateistruktur
Das entwickelte Protokoll besteht aus 2 Ebenen und 4 Dateien. Wobei unterschieden wird zwischen dem Anwender-Interface und der Steuerungs-Ebene. Dabei ist der Name „Anwender-Interface“ nicht gleichzusetzen mit der Anwendungsschicht des oben beschriebenen OSI-Modells. Der Name wurde gewaehlt, da das „Anwender-Interface“ dem Anwender des entwickelten CAN-Protokolls Funktionen zur Verfuegung stellt, die fuer den regulaeren Betrieb der CAN-Protokoll-Kommunikation benoetigt werden. Diese Funktionen werden durch das Einbinden der Datei **cp_user.h** zur Verfuegung gestellt. Nachfolgende Abbildung veranschaulicht die Zusammenhaenge. Die Richtung der Pfeile zeigt die Richtung des Funktionsaufrufs und spiegelt nicht die Richtung des Datenflusses wieder.

In der Source-Datei **cp_user.c** sind die Anwender-Jede Anwenderfunktion erhaelt den Rueckgabetyp #CP_StatusTypeDef. Dieser beschreibt, ob die Funktion erfolgreich ausgefuehrt wurde. Die gleichen Werte werden, wie oben gezeigt, auch fuer den Kommunikations-Zustand (Status) der Datenobjekte verwendet.

Die Steuerungs-Ebene ist die Schicht zwischen den HAL-Funktionen und dem Anwender-Interface. So enthaelt die Datei **cp_control.c** die zwei Kernfunktionen des Interrupt-getriebenen Sende- und Empfangsprozesses. Dazu finden stehen hier Steuerungs- und Verwaltungsfunktionen zur Verfuegung, die von den Anwenderfunktionen benoetigt werden, worauf der Anwender selbst aber nicht zugreifen muss. Zum Beispiel ist das eine Funktion, die ueberprueft ob eine bestimmte ID bereits initialisiert ist, indem eine Liste durchsucht wird. In der dazugehoerigen Header-Datei **cp_control.h** sind diese Funktionen und Variablen deklariert, um den Zugriff durch die Anwenderfunktionen zu ermoeglichen.
Fuer den Anwender des CAN-Protokolls ist nur die Datei **cp_user.h** relevant. Diese enthaelt die Deklaration und damit den Aufbau der Datenobjekte, die vom Anwender manuell im Code bearbeitet werden. Des Weiteren befinden sich hier Definitionen von nutzer-spezifischen Parametern und Verweise auf alle Funktionen, die vom Nutzer verwendet werden koennen. 

Die Funktionen und Dateien des Protokolls werden mit dem Kuerzel „CP_“ zur direkten Erkennung gekennzeichnet (Abkuerzung fuer CAN-Protokoll).

