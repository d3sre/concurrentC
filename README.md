# Concurrent C Programming / Seminar ZHAW / FS 2015 / Nico Schottelius

## Termine

* Kick-Off ZL 4.10                          2015-03-11-1700 CET
* Angabe git repositories                   2015-03-15-2359 CET
** Via Mail inklusive ZHAW Kürzel
* EBS-Eintrag                               2015-03-20-2359 CET
* 1. Zwischenstandsbericht                  2015-04-01-2359 KST
* 2. Zwischenstandsbericht                  2015-05-01-2359 CEST
* Abgabe Arbeiten:                          2015-06-21-2359 CEST
* Präsentation  ZL O4.10                    2015-07-01-1700 CEST
* (nur Dozent) Noteneingabe                 KW 30
* (nur Dozent) Späteste Abgabe Notenblatt   2015-07-26

## Abgabe 

### Dokumentation

* Digital
* Fakultativ: Gedruckt/Gebunden

### Abgabebedingungen

* git repo auf github vorhanden
* Applikation lauffähig unter Linux
* Nach "make" Eingabe existiert (somit auch ein Makefile!)
 * "server": Binary des Servers
  * Sollte nicht abstürzen / SEGV auftreten
 * "client": Executable zum Testen des Servers
 * "doc.pdf": Dokumentation - Beispiel für Inhaltsverzeichnis weiter unten
  * Einleitung
  * Anleitung zur Nutzung
  * Weg, Probleme, Lösungen
  * Fazit
  * Keine Prosa - sondern guter technischer Bericht
  * Deutsch oder English möglich

## Präsentation

* Handout Digital
** an alle
* 10-15 Minuten + 5 Minuten Fragen
** + 5 Minuten Client-Strategie (optional)
* Vortrag ist nicht (nur) für den Dozenten
* Zeigen 
 * was gelernt wurde
 * was Probleme waren
 * wie mit Problemen umgegangen

## Lernziele

Die Besucher des Seminars verstehen was Concurrency bedeutet und
welche Probleme und Lösungesansätze es gibt.
Sie sind in der Lage Programme in der Programmiersprache C zu 
schreiben, die auf gemeinsame Ressourcen gleichzeitig zugreifen.

Das Seminar setzt Kenntniss der Programmiersprache C voraus.

## Lerninhalte

Selbstständige Definition des Funktionsumfangs des
Programmes unter Berücksichtigung der verfügbaren Ressourcen
im Seminar.

Konzeption und Entwicklung eines Programms, das gleichzeitig
auf einen Speicherbereich zugreift.

Die Implementation erfolgt mithilfe von Threads oder Forks 
und Shared Memory (SHM).

## Leistungsnachweis

* Mündliche Präsentation der Seminararbeit
* Schriftliche Seminararbeit

## Zwischenbericht

* Fakultativ
* Aber nur bis zum angegebenem Datum

## Aufgabe / Projekt

    Erstelle ein Forking / Multithreaded Client-Server Spiel

## Spielregeln:

### Ziel des Spiels:

Eroberung aller Felder des Spielfeldes durch einen Spieler.

### Spielaufbau

Das Spielfeld ist ein Quadrat der Seitenlänge n, wobei n >= 4 ist.
Die Koordinaten des Spielfeldes sind somit (0..(n-1), 0..(n-1)).

### Spielablauf

* Der Server startet und wartet auf n/2 Spieler
* Sobald n/2 Spieler verbunden sind, kann jeder Spieler versuchen Felder zu erobern
* Es können während des Spiels neue Spieler hinzukommen oder Spieler das Spiel verlassen
* Der Server prüft alle y Sekunden den *konsistenten Spielfeldstatus* , wobei 1 <= y <= 30
 * Wenn ein Spieler zu diesem Zeitpunkt alle Felder besitzt, hat er gewonnen und das Spiel wird beendet

## Protokoll Allgemein

* Befehle werden mit \n abgeschlossen
* Kein Befehl ist länger als 256 Zeichen inklusive dem \n
* Jeder Spieler kann nur 1 Kommando senden und muss auf die Antwort warten


### Anmeldung

Erfolgreiche Anmeldung:

    Client: HELLO\n
    Server: SIZE n\n

Nicht erfolgreiche Anmeldung:

    Client: HELLO\n
    Server: NACK\n
        -> Trennt die Verbindung

### Spielstart

Der Server wartet auf n/2 Verbindungen vor dem Start.

    Server: START\n
    Client: - (erwiedert nichts, weiss das es gestartet hat)

### Feld erobern erfolgreich

Wenn kein anderer Client gerade einen TAKE Befehl für das selbe Feld sendet,
kann ein Client es nehmen.

    Client: TAKE X Y NAME\n
    Server: TAKEN\n

### Feld erobern: nicht erfolgreich

Wenn ein oder mehrere andere Clients gerade einen TAKE Befehl 
für das selbe Feld sendet, sind alle bis auf der erste nicht erfolgreich.

    Client: TAKE X Y NAME\n
    Server: INUSE\n

### Besitz anzeigen

    Client: STATUS X Y\n
    Server: Name-des-Spielers\n


### Spielende

Sobald ein Client alle Felder besitzt wird der Gewinner bekanntgegeben.
Diese Antwort kann auf jeden Client Befehl kommen,
mit Ausnahme der Anmeldung kommen.

    Server: END Name-des-Spielers\n
    Client: - (beendet sich)

## Bedingungen für die Implementation
    
* Es gibt keinen globalen Lock (!)
* Der Server speichert den Namen des Feldbesitzers
* Kommunikation via TCP/IP
* fork + shm (empfohlen)
 * oder pthreads
 * für jede Verbindung einen prozess/thread
 * Hauptthread/prozess kann bind/listen/accept machen
 * Rating Prozess/Thread zusätzlich im Server
* Fokus liegt auf dem Serverteil
 * Client ist hauptsächlich zum Testen und "Spass haben" da
 * Server wird durch Skript vom Dozent getestet
* Locking, gleichzeitiger Zugriff im Server lösen
* Debug-Ausgaben von Client/Server auf stderr


## Tipps

    gcc -Wall -Wpedantic -Wextra
    Optional: valgrind

* IDE-Möglichkeiten
** clion
** vim + gdb + gcc + make


## Bibliographie

    Advanced Programming in the UNIX Environment (3rd Edition)
    W. Richard Stevens (Author), Stephen A. Rago (Author)

    [sehr empfohlen]

    Expert C Programming: Deep C Secrets
    Peter van der Linden

    [empfohlen für interessierte]

    The Open Group Base Specifications Issue 7
    IEEE Std 1003.1, 2013 Edition
    http://pubs.opengroup.org/onlinepubs/9699919799/

    [für portables entwickeln]
