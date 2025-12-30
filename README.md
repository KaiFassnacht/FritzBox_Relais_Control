# 📞 ESP32 SIP Relais Gateway (WT32-ETH01)

Dieses Projekt verwandelt einen **WT32-ETH01** (ESP32 mit Ethernet) in ein SIP-Endgerät für die AVM FRITZ!Box. Es ermöglicht das Schalten von bis zu 10 Relais (GPIOs) über die Telefontastatur (DTMF) während eines Anrufs.



---

## 🚀 Features

* **Native SIP-Stack**: Registrierung direkt als IP-Telefon an der FRITZ!Box.
* **Ethernet-Support**: Maximale Stabilität über LAN (statt störanfälligem WLAN).
* **DTMF-Steuerung**: Auswertung von Tastentönen (SIP INFO) zur Relais-Steuerung.
* **Mehrstufige Sicherheit**:
    * **Rufnummern-Whitelist**: Schalten nur von autorisierten Nummern (Caller ID) möglich.
    * **Individuelle PINs**: Jede Taste kann eine eigene, unterschiedlich lange PIN besitzen.
* **Modernes Audio-Feedback**: Akustische Quittierung durch professionelle Tonsequenzen (RTP/G.711a).
* **Automatischer Impuls-Modus**: Relais schalten nach Ablauf der `RELAIS_DAUER` automatisch ab.
* **Sofort-Beenden**: Taste `#` beendet das Gespräch sofort (Sofortiges `BYE`).
---

## 🛠 Hardwareanforderungen

* **WT32-ETH01** (oder ESP32 mit LAN8720 Modul)
* **Relais-Karte** (3.3V kompatibel oder mit Optokopplern)
* **FRITZ!Box** (oder ein anderer SIP-Server)

---

## 🛡️ Sicherheits-Konzept

Das Gateway bietet zwei Schutzebenen, die in der `settings.h` pro Taste konfiguriert werden:

1.  **Whitelist-Prüfung**: Das System extrahiert die Anrufernummer (z. B. `**611` oder Handynummer). Ist die Whitelist für eine Taste aktiv, werden Eingaben von nicht autorisierten Nummern ignoriert.
2.  **PIN-Autorisierung**: Bei geschützten Tasten fordert das Gateway mit einer speziellen Tonfolge zur PIN-Eingabe auf. Erst nach korrekter Eingabe wird das Relais geschaltet.



---

## 📋 Konfiguration & Parameter

Die grundlegenden Einstellungen werden in der `include/settings.h` vorgenommen:

| Parameter | Beschreibung | Standardwert |
| :--- | :--- | :--- |
| `RELAIS_DAUER` | Zeit, die das Relais angezogen bleibt | 1000 ms |
| `INAKTIVITAETS_TIMEOUT` | Automatisches Auflegen nach Inaktivität | 60000 ms |
| `WHITELIST` | Liste der erlaubten Rufnummern | `{"**611", ...}` |
| `PINS[10]` | Array mit PINs für die Tasten 0-9 | `{"", "1234", ...}` |

Du kannst das Verhalten des Gateways über diese Parameter feinjustieren:
•	RELAIS_DAUER: Zeit in Millisekunden, die das Relais angezogen bleibt (Standard: 1000).
•	INAKTIVITAETS_TIMEOUT: Zeit in Millisekunden, nach der das Gateway bei Inaktivität auflegt (Standard: 60000).
•	SIP_PORT: Der lokale Port für die SIP-Kommunikation (Standard: 5060).


---

## 🏗 Funktionsweise

1.  **Boot**: ESP32 baut Ethernet-Verbindung auf und registriert sich an der FRITZ!Box.
2.  **Anruf**: Gateway prüft Caller-ID und sendet bei Annahme ein Bereit-Signal.
3.  **Wahl**: Benutzer drückt eine Taste (z.B. '2').
4.  **Validierung**:
    * Prüfung: Ist die Caller-ID für diese Taste autorisiert?
    * Prüfung: Ist eine PIN erforderlich? (Falls ja: Warte auf PIN-Eingabe).
5.  **Aktion**: GPIO schaltet -> Quittungston wird abgespielt.
6.  **Ende**: Gespräch endet durch `#`, Auflegen des Anrufers oder Inaktivitäts-Timeout.


---

## 💻 Installation

1. **Projekt öffnen**: Öffnen Sie den Projektordner in **PlatformIO** (VS Code).
2. **Konfiguration**: Benennen Sie die `include/settings_example.h` in `include/settings.h` um und passen Sie die SIP-Daten sowie die Pin-Belegung an Ihre Umgebung an.
3. **FRITZ!Box einrichten**: Legen Sie in der FRITZ!Box ein neues **IP-Telefon** an. Wichtig: Benutzername und Passwort müssen exakt mit den Einträgen in der `settings.h` übereinstimmen.
4. **Upload**: Schließen Sie den WT32-ETH01 an und starten Sie den **Upload** über PlatformIO.


## 🛡️ Sicherheitshinweis

Um private Zugangsdaten zu schützen, nutzt dieses Projekt ein Vorlagen-System:
1.  Benenne `include/settings_example.h` um in `include/settings.h`.
2.  Trage dort deine echten Zugangsdaten ein.
3.  Die `settings.h` wird durch die `.gitignore` **nicht** hochgeladen.

---

## ⌨️ Bedienung & Akustisches Feedback

| Signal | Aktion / Zustand | Audio-Feedback | Beschreibung |
| :--- | :--- | :--- | :--- |
| **Start-Ton** | Verbindung steht | Hoher Doppel-Piep | System bereit für Eingaben. |
| **PIN-Request** | PIN erforderlich | Zwei ansteigende Töne | Aufforderung zur PIN-Eingabe. |
| **OK-Ton** | Schalten erfolgreich | Kurzer Bestätigungston | Relais wurde aktiviert. |
| **Error-Ton** | Fehler / Sperre | Tiefer Fehlerton | PIN falsch oder Whitelist-Sperre. |
| **Timeout** | Warnung | Rhythmische Impulse | Automatisches Auflegen in 2 Sek. |
| **Taste #** | Auflegen | - | Sofortige Trennung der Leitung. |

---

## 🔌 Hardware-Anschlussplan

| WT32-Pin | Funktion | Ziel (Relaiskarte) | Hinweis |
| :--- | :--- | :--- | :--- |
| **5V** | VCC | VCC (Relais) | Stromversorgung Relais |
| **GND** | Masse | GND | Gemeinsame Masse |
| **IO04** | Relais 0 | IN 1 | Taste '0' am Telefon |
| **IO12** | Relais 1 | IN 2 | Taste '1' am Telefon |
| **IO14** | Relais 2 | IN 3 | Taste '2' am Telefon |
| **IO15** | Relais 3 | IN 4 | Taste '3' am Telefon |

> [!WARNING]
> Nutzen Sie Relaiskarten mit **Optokopplern**, um den ESP32 vor induktiven Spannungsspitzen zu schützen!

---

## 🔍 Fehlerbehebung (Troubleshooting)

Falls die Kommunikation zwischen dem Gateway und der FRITZ!Box nicht sofort funktioniert, prüfe bitte folgende Punkte:

### 1. Status: "401 Unauthorized" (Dauerschleife)
* **Ursache:** Passwort oder Benutzername in der `settings.h` stimmen nicht mit den Daten in der FRITZ!Box überein.
* **Lösung:** Lösche das IP-Telefon in der FRITZ!Box und lege es neu an. Achte darauf, dass der Benutzername mindestens **8 Zeichen** lang ist (Empfehlung der FRITZ!Box für erhöhte Sicherheit).

### 2. Status: "403 Forbidden"
* **Ursache:** Die FRITZ!Box verweigert die Anmeldung aus dem lokalen Netzwerk.
* **Lösung:** Prüfe in der FRITZ!Box unter **Telefonie** -> **Eigene Rufnummern** -> **Anschlusseinstellungen**, ob die Option *"Anmeldung aus dem Internet erlauben"* (falls der ESP in einem anderen Subnetz steht) oder andere Sicherheitssperren aktiv sind. Meist liegt es jedoch an einer falschen IP-Zuweisung des Clients.

### 3. Kein Audio (Man hört kein Piepen)
* **Ursache:** RTP-Port-Konflikt oder falsche IP-Adresse im SDP-Body (Audio-Handshake).
* **Lösung:** Kontrolliere im Serial Monitor, ob nach dem `200 OK` ein `ACK` empfangen wird. Wenn kein `ACK` kommt, akzeptiert die FRITZ!Box die angebotene IP-Adresse im SDP nicht. Stelle sicher, dass `pMyIp` im Code der echten Ethernet-IP des Moduls entspricht.

### 4. Gerät legt sofort nach dem Abheben auf
* **Ursache:** Der SIP-Handshake ist unvollständig oder fehlerhaft.
* **Lösung:** Prüfe, ob die `Call-ID` und der `To-Tag` in der `Ok()` Funktion korrekt gesetzt werden. Ohne einen eindeutigen `Tag` im `To`-Header betrachtet die FRITZ!Box die Antwort als ungültig und bricht den Ruf ab.

### 5. DTMF-Töne werden nicht erkannt
* **Ursache:** Die FRITZ!Box sendet Tastentöne als "Inband" oder "RTP-Event", das Gateway erwartet jedoch **"SIP INFO"**.
* **Lösung:** In den Merkmalen des IP-Telefons (innerhalb der FRITZ!Box-Oberfläche) sicherstellen, dass die Übertragung von DTMF-Tönen auf **"Automatisch"** oder explizit auf **"INFO"** eingestellt ist.

---

## 📝 Debugging & Überwachung

* **Serial Monitor**: Nutzen Sie 115200 Baud für detaillierte SIP/RTP Logs.
* **FritzBox Web-Interface**: Unter `Telefonie -> Telefoniegeräte` zeigt ein grüner Punkt die erfolgreiche Registrierung an.

---

## 📜 Lizenz

Dieses Projekt ist unter der **MIT-Lizenz** veröffentlicht.