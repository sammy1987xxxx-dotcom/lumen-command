## Türzeichen: Ruhe (53) und ChatGPT Tür + Musik (54), 30.10.2026

Von Claude Opus 5.5 (Anthropic), `DoorScenes.h`, nur in der LED-Webseite. Reagieren auf die UDP-Meldungen
offen/zu einer Tür mit eingeschaltetem „Türlicht“ (Tür über LED 335–370), unabhängig von „Alles AUS“.

| ID | Name | Verhalten |
|---|---|---|
| 53 | Ruhe mit Türzeichen | Sehr dunkles, langsam atmendes Blau-Violett (höchstens 20/255) zum Schlafen/Meditieren. Tür auf: goldene Welle von der Tür über den ganzen Schlauch, danach glüht der Türbereich, solange sie offen ist; nach „zu“ klingt es in 3 s aus. Rechnet die Helligkeit selbst in 1/256-Stufen bei voller Treiberstufe (Regler wird eingerechnet, Ausgangsgrenze bleibt); jede LED rundet an eigener fester Schwelle, damit das Atmen gleitet statt im Sekundentakt zu springen. |
| 54 | ChatGPT Tür + Musik | ChatGPT (36) mit Musik plus dieselbe Begrüßungswelle und Türglühen; solange die Tür offen ist, starten Beats als Mint-Wellen an der Tür. Schickt etwa 30-mal pro Sekunde Lautstärke und Beat-Zähler direkt an die Tür; deren Streifen zeigt im Stillstand ChatGPT im Takt (2 s ohne Paket: eigener Effekt). |

**Alle anderen Effekte (1–52):** Dieselbe goldene Begrüßungswelle und das Türglühen liegen über jedem laufenden
Effekt (außer „Alles AUS“, dort gilt das Türlicht). `Led.ino` zeichnet sie nur auf eine Kopie fürs Anzeigen, damit
Effekte mit Nachleuchten das Gold nicht aufsummieren; die Helligkeit folgt dem Regler.

Prüfung: `tools/test_door_light.py` (PC). Kein Hardware- oder Funknachweis.

Auf der LED-Webseite tragen Effekte, die das Mikrofon auswerten, ein orangefarbenes „A“ (10–19, 21–25, 31–36,
39, 44, 50, 51, 54); die Namen bleiben wegen des Touchdisplays unverändert.

## Opus Resonanz (50), Opus Puls (51), Opus Polarlicht (52), 30.10.2026

Drei Effekte von Claude Opus 5.5 (Anthropic), `OpusEffects.h`, nur in der LED-Webseite
(Touchdisplay weiterhin 1–45). Keine Einstellregler.

| ID | Name | Verhalten |
|---|---|---|
| 50 | Opus Resonanz | Jeder Beat (mind. 150 ms Abstand) schickt eine farbige Wellenfront mit Nachhall von der Mitte nach außen (~1,4 s bis zum Rand); Fronten addieren sich. Farbe wandert in 90 s, mit Musikenergie schneller. Stille: ruhiges, kaum sichtbares Grundglimmen. |
| 51 | Opus Puls | Doppelschlag-Herz aus der Mitte. Lernt das Tempo aus Beats (300–1500 ms), regelt die Phase nach und schlägt bei fehlenden Beats ~3 s im Takt weiter, dann Übergang zum Ruhepuls 40/min. Farbe nach Tempo: langsam tiefrot, schnell bernstein. |
| 52 | Opus Polarlicht | Drei Lichtvorhänge (grün, türkis, violett) wandern langsam, feine Strahlen ziehen hindurch, Überlagerungen heller, einzelne Sterne funkeln weich. Mikrofon ohne Einfluss. |

Prüfung: `tools/test_opus_effects.py` (0..600 LEDs, Ränder, millis()-Überlauf, Stille ohne
Flackern, Wellenlauf, Tempo-Lernen/Weiterschlagen, Mikrofonunabhängigkeit, Dispatcher) schreibt
Zeit-Ort-Vorschauen nach `Temp/Led-Opus-PC/`. Am Gerät von Sammy abgenommen (30.10.).

**Durchsicht aller 52 Effekte, 30.10.2026:** Alle über den echten Dispatcher mit aufgezeichnetem
Musikpegel (Messung Stufe 6) und anschließender Stille gerendert und vermessen. Musik-Effekte
bleiben bei Stille dunkel, Regler sind begrenzt. Behoben: *Claude* (44) brach bei jedem Beat
den laufenden Einfall-Ring hart ab (Ring 2,6 s, Beats alle 0,5 s; größter Bildsprung 123 → 23),
jetzt sechs gleichzeitige Einfälle. *Opus Resonanz* (50) hat 10 statt 6 Wellenplätze, damit auch
Beats im 0,2-s-Abstand keine sichtbare Welle abschneiden.

## Goldtrio (48) und ChatGPT Original (49), 29.10.2026

**48 Goldtrio:** Drei Goldbalken starten nacheinander an einem Ende und ziehen zum
anderen; im nächsten Gang starten sie am anderen Ende und laufen zurück, dann Pause.
Gleiche Regler wie der Goldschweif (Tempo, Pause, Schweiflänge, Farbe). Im PC-Test
leuchten höchstens drei Balken und höchstens rund 27 % der LEDs (120 von 445) zugleich,
etwas mehr als bei 46/47; in der Pause ist alles dunkel.

**49 ChatGPT Original:** der bisherige Ablauf von „ChatGPT ohne Musik“ mit den festen
Ursprungsfarben (mint, violett, gold), ohne Mikrofon und ohne Farbwanderung. Im PC-Test
bildgleich zum unveränderten Renderer; 37 beginnt gleich, dreht aber danach den Farbton.
Beide nur in der LED-Webseite, das Touchdisplay bietet weiter 1–45.

## Goldatem (46), Goldpendel (47) und Farbwanderung bei 37, 29.10.2026

Auf Sammys Wunsch (ruhige Nachtlichter mit mehr Bewegung, immer nur wenige schmale
Balken) zwei Varianten des Goldschweifs. Sie nutzen **dieselben Regler wie der
Goldschweif** (Geschwindigkeit, Pause, Schweiflänge, Farbe Rot/Grün/Blau; auf der
LED-Webseite im Bereich „Goldschweif, Goldatem, Goldpendel“) und bleiben ohne Audio.

| ID | Effekt | Ablauf |
| --- | --- | --- |
| 46 | Goldatem | Zwei Balken laufen von der Mitte nach außen (Ausatmen), danach zwei von den Enden zur Mitte (Einatmen), dann die eingestellte Pause. |
| 47 | Goldpendel | Zwei Balken starten an gegenüberliegenden Enden und laufen aneinander vorbei bis zum anderen Ende; einer startet etwas später, sodass sie sich abwechselnd rechts und links der Mitte begegnen. Ab und zu löst sich von jedem Balken ein kleiner, schwächerer Funke nach links und rechts, fliegt einen kurzen Weg und erlischt (drei Gelegenheiten je Gang, Zeitpunkte aus einem festen Hash und der Durchlaufnummer, kein Zufall). Dann Pause. |

Im PC-Test mit 445 LEDs leuchten höchstens zwei Balken und höchstens rund 80 LEDs
(18 %; mit Funken beim Pendel bis 99, 22 %) zugleich; in der Pause ist alles dunkel. Das Pendel läuft bei gleichem Tempo
doppelt so lange wie ein Goldschweif-Durchgang, weil es den ganzen Streifen quert.
Auswahl in der LED-Webseite; **das Touchdisplay bietet bewusst nur 1–45** (Test
`tools/test_led_web.js` prüft das). `GoldVariants.h`.

**ChatGPT ohne Musik (37)** zeigt denselben Ablauf wie bisher, verschiebt aber pro
32-s-Durchlauf den Farbton um 72° (`HueDrift.h`): erster Durchlauf wie gewohnt, dann
Grün-/Türkis-, Blau-, Rot-/Gold-Töne usw.; nach fünf Durchläufen (160 s) ist der
Farbkreis einmal rum, gleitend und ohne Sprung. Helligkeit etwa gleich. *ChatGPT* mit
Musik (36) und die Tür-Variante sind unverändert. PC-Test: alle sechs Farbfamilien
in 160 s. Werte: Periode 160 000 ms in `Effects.cpp`.
Zum Verwenden LED-Firmware neu bauen/hochladen; hier weder Build noch Upload.
PC-Test: `tools/test_gold_variants.py`; keine Aussage über die reale Lichtwirkung.

## Goldschweif (43), 27.10.2026

Zusätzlicher, vollständig musikunabhängiger Effekt: warmgoldener Impuls auf
schwarzem Hintergrund, von der Mitte symmetrisch zu beiden Enden. Der Schweif
verlässt den Streifen vollständig, danach folgt die eingestellte dunkle Pause.
Start sofort beim Auswählen; bisherige Effekte 1–42 bleiben erhalten.

In der LED-Webseite unter **Goldschweif** auswählen und im gleichnamigen
Einstellungsbereich anpassen: Geschwindigkeit 10–200 (höher = schneller),
Pause 0–15000 ms, Schweiflänge 80–1200 sowie Rot/Grün/Blau jeweils 0–255.
Standard: Tempo 80, Pause 2500 ms, Schweif 350, warmes Gold RGB 235/165/65.
Bei Standardtempo dauert ein Durchlauf 1,8 s; Wiederholung alle 4,3 s.
Die Schweiflänge ist relativ zur Streifenlänge, kein Pixelwert. Die Regler
werden beim Loslassen übernommen und nach zehn Sekunden Ruhe gespeichert.
Globale Helligkeit und Ausgangsgrenze 210 gelten weiterhin.

Touchdisplay ergänzt die Auswahl 43; Detailregler bleiben in der LED-Webseite.
Zum Verwenden LED-Firmware neu bauen und hochladen; für die Displayauswahl
auch das Display separat aktualisieren. Hier kein Firmwarebau oder Upload.
PC-Tests sind kein Nachweis der realen Lichtwirkung oder Speicherung am Gerät.

## Claude (44) und Claude ohne Musik (45), 29.10.2026

Zwei Effekte von Claude (Anthropic) als Bild für Denken und Antworten. Eine
warme Lehmglut atmet von der Mitte nach außen, Lehm-/Dämmerfäden weben sich nur
dort hell zusammen, wo sie sich treffen, und ein Schreibcursor zieht in 20 s
einmal von links nach rechts und lässt ein weiches Spurband zurück. Dazu kommt
der **Einfall**: ein heller Kern und ein gesegmenteter Strahlenring wachsen aus
der Mitte und klingen nach 2,6 s aus. Der Anstieg dauert 250–300 ms, kein Blitz.

| ID | Effekt | Audio |
| --- | --- | --- |
| 44 | Claude | Erkannter Beat löst den Einfall aus (mindestens 0,5 s Abstand); Lautstärke hebt Fäden und Helligkeit. Ohne Musik trotzdem sichtbar belebt, aber ohne Einfälle. |
| 45 | Claude ohne Musik | Mikrofon und Beat ohne Einfluss. Einfälle kommen allein nach Zeitplan (nach 2,5 s, dann alle 9–13 s); nach langer Pause höchstens einer, nie nachgeholt. |

**Regenbogen bei 45, 29.10.2026:** Auf Sammys Wunsch wandern bei *Claude ohne Musik*
die Farbtöne (Lehmglut, Fäden, Spurband, Dämmerblau, Strahlenring) in **240 Sekunden
einmal durch den ganzen Farbkreis**, nie alle Farben gleichzeitig. Nach Sammys
Rückmeldung „farbenfroher“ sind die Farben deutlich satter als das Lehm-Original
(Sättigung ×1,5–1,8 bei etwa gleicher Helligkeit), und Fäden, Spurband und Dämmerton
laufen mit versetztem Farbton (+50° / −40°), sodass je Bild zwei bis drei Farben
zu sehen sind (z. B. Orange/Limette, später Grün/Himmelblau, Violett/Rosa). Dunkler
Grund bleibt, Creme (Kern, Cursor) bekommt einen Hauch der Farbe. Werte in
`ClaudeEffect.h`: Periode 240 s, Sättigung, Versatzwinkel. *Claude* mit Musik (44)
ist byteweise unverändert. Die Dauer ist `hue_`-Periode in
`ClaudeEffect.h` (240 s = 6 × 40 s, Naht unsichtbar). PC-Test erweitert.

Beide haben keine Einstellregler. Alle Zeitphasen teilen 40 s, `ClaudeEffect.h`
braucht weder Heap noch delay noch Zufall und schreibt direkt in den LED-Puffer.
Globale Helligkeit und Ausgangsgrenze 210 gelten weiterhin. Die LED-Webseite
zeigt beide Knöpfe; die Touchdisplay-Auswahl bietet nun 45 Effekte (Display
separat neu bauen/flashen).
Zum Verwenden LED-Firmware neu bauen und hochladen; hier weder Build noch Upload.
PC-Test: `tools/test_claude_effect.py` (Renderer/Dispatcher, 0–600 LEDs, Nähte,
Überlauf); kein Nachweis der realen Lichtwirkung.

# LED-Lichtwelten und stille Goldspuren, 27.10.2026

## Auswahl

In der angemeldeten LED-Webseite unter **Effekte**:

| ID | Effekt | Wirkung |
| --- | --- | --- |
| 37 | ChatGPT ohne Musik | Bekannte Choreografie, jetzt mit gelegentlichen automatischen Goldspuren von der Mitte zu den Enden. Mikrofon bleibt ohne Einfluss. |
| 38 | Sternenfluss | Ruhiger petrol-/violetter Nachthimmel, ziehende Mint-/Goldkometen und weich funkelnde Sterne. Ohne Audioeinfluss. |
| 39 | Prismenpuls | Farbige, sich überlagernde Bänder ziehen von der Mitte nach außen; auch bei Stille aktiv. Vorhandene Lautstärke verstärkt die Bänder, erkannte Beats ergänzen Goldringe. |
| 40 | Gezeiten | Breite, langsam gegenläufige türkis-/blaue Strömungen, ohne Audioeinfluss. |
| 41 | Nachtglut | Sehr dunkles warmrotes Nachtlicht, ohne Blau, Glitzern oder helle Impulse. Ohne Audioeinfluss. |
| 42 | Lichtseide | Langsam verflochtene Mint-/Violettbänder mit Goldfäden, ohne Audioeinfluss. |

Die aktualisierte Touchdisplay-Liste bietet ebenfalls alle 42 Effekte an.
Auswahl allein sendet weiterhin nichts; erst **Übernehmen** senden lassen.
Ältere Display-Firmware zeigt die neuen Namen noch nicht; dafür separat
bauen/aktualisieren. Die LED-Webseite benötigt nur die neue LED-Firmware.
Kein neuer Homeserver, keine neue Handy-App oder Audiohardware erforderlich.

## Nachtglut überarbeitet, 29.10.2026

Sammy fand die Nachtglut „zuckelig“. Ursache im Code: Alle LEDs teilten eine
langsame Welle, die auf ganze Rohstufen abgeschnitten wurde; dadurch sprang der
**ganze Streifen gleichzeitig** um eine Stufe (im PC-Test bis zu 444 LEDs in
einem Bild, über 3000 Hin-und-her-Wechsel in vier Minuten). Zusätzlich vermutet
(nicht am Gerät bewiesen): das zeitliche FastLED-Dithering zappelt bei so kleinen Werten.

Jetzt: Helligkeit in 2^-31-Stufen berechnet (kein Rundungszittern an Wellen-
umkehrpunkten), danach mit fester Schwelle je LED gerundet (räumliches Dithering).
Jede LED wechselt nur, wenn das Feld ihre Schwelle durchquert; benachbarte LEDs
wechseln zu unterschiedlichen Zeiten. PC-Messung über vier Minuten bei 444 LEDs:
alt bis 444 LEDs im selben Bild und 3443 Hin-und-her-Wechsel, neu höchstens 19 LEDs
und 16 Wechsel. Grenzen unverändert (Rot ≤14, Grün ≤2, Blau 0, sehr dunkel),
dieselbe langsame Bewegung (80/120 s), Einblendung fünf Sekunden. Zusätzlich
schaltet `Led.ino` für ID 41 das FastLED-Dithering ab und für alle anderen Effekte
wieder ein. Nur PC-Test (`tools/test_quiet_effects.py`); Optik am Schlauch erst
nach Sammys LED-Build.

**Vereinzelt dunkle LEDs, 29.10.2026:** Auf Sammys Wunsch (nachts Energie sparen) hat
jede LED der Nachtglut eine eigene Phase in einem 60-s-Takt: knapp 6 % der Zeit ist
sie ganz aus, dazwischen blendet sie über etwa 5 s weich aus und ein. Im PC-Test sind
bei 444 LEDs je Bild etwa 27–36 (rund 7 %) komplett dunkel, im Mittel etwa 10 % weniger
Licht; kein Flackern (höchstens 17 LEDs ändern sich je Bild). Mehr oder weniger
Dunkelanteil ist eine Zahl (`lo`/`hi` in `QuietEffects.h`).

## Drei ruhige Szenen 40–42

Gezeiten und Lichtseide beginnen mit einer dreisekündigen Einblendung.
Breite Wasserbewegungen bei Gezeiten werden durch gegenläufige Phasen über
40/60 Sekunden erzeugt; Lichtseide verwebt langsame Farb-/Helligkeitsbänder.
Keine zufälligen Lichtblitze oder Audioimpulse in diesen drei Varianten.

Nachtglut blendet fünf Sekunden ein und verändert seine Glut nur ganz langsam
über 80/120 Sekunden. Seine Rohwerte bleiben auf **Rot ≤14, Grün ≤2, Blau 0**
begrenzt, bevor die bestehende globale Ausgangsskalierung greift. Das ist eine
bewusst sehr dunkle Szene, keine automatische Änderung deines Helligkeitsreglers.
Der Regler kann weiter abdunkeln; andere Effekte behalten ihre Helligkeit.
Die Szene schaltet das Gerät nicht in einen Schlafmodus und enthält keinen
automatischen Abschalttimer.

`QuietEffects.h` speichert nur Zeitphase/Eintrittsdauer und schreibt direkt in
den vorhandenen LED-Puffer. Kein Heap/Bildpuffer/Delay, keine Audioargumente.
Positionen sind normiert, alle konfigurierbaren Längen passen sich automatisch
an. Perioden schließen auch am Vier-Minuten-Zyklus ohne Sprung an; Timer-
Überlauf und Effektwechsel werden behandelt. Bisherige Quellen
`ChatGptEffect.h`, `EffectStudio.h` und `SignatureEffects.h` unverändert.

## Automatische Goldspuren bei 37

Die erste startet etwa **acht Sekunden** nach Auswahl des Effekts. Die
folgenden Abstände wechseln zwischen **elf, neun und acht Sekunden**. Sie
verwenden denselben sanften goldenen Auslauf wie die vorhandenen Musikimpulse.
Die normale Schleife mit ihren vier Szenen und Überblendungen bleibt erhalten.
Auch bei lauter Musik oder Dauerbeats reagiert diese Variante nicht aufs Mikrofon.

Bei einem Effektwechsel wird der Zeitgeber zurückgesetzt; beim Wiedereintritt
beginnt die Wartezeit neu. Nach langen Pausen höchstens ein Impuls, keine
nachgeholte Folge. Zeitpunkte sind durch das tatsächliche Frameintervall
geringfügig verzögert. Original **ChatGPT (36)** bleibt unverändert mit echter
Musik-/Beat-Reaktion; keine Zeitimpulse in dieser Variante. Der Originalrenderer
`ChatGptEffect.h` und die Tür-Choreografie sind unverändert.

## Umsetzung

`SignatureEffects.h` ergänzt einen kleinen Goldzeitgeber und zwei eigenständige
Szenen mit normierten Streifenkoordinaten. Automatische Anpassung an die
konfigurierte Länge, insbesondere 444 LEDs; keine fest verdrahtete Länge.
Neue Szenen blenden beim Eintritt 600 ms weich ein. Vier wandernde Kometen
und 14 deterministische Sterne bei Sternenfluss, drei versetzte Farbbänder
und bis zu sechs kurzlebige Beat-Ereignisse bei Prismenpuls. Keine alte
LED-Position bei einem Längenwechsel; Beats speichern nur Alter, keine Indizes.

Keine Allokation/Bildpuffer, Wartezeit, Zufallssuche oder Nachholschleife pro
Bild. Keine zusätzliche Audio-/Netzwerkverarbeitung. Bisherige Effekte 1–36,
Regler, persistente Datenformate, Anmeldung und Ausgangsgrenze **210** erhalten.
37 ergänzt ausschließlich seine Zeitimpulse; 38/39 sind angehängte neue IDs.
Keine neue Konfiguration oder Parameterregler notwendig. Kein Vollweiß-
Stroboskop; die bestehende globale Helligkeitsbegrenzung gilt für alle Szenen.

## PC-Prüfung und Grenzen

Nachtrag für 40–42: `tools/test_quiet_effects.py` kompiliert den tatsächlichen
Renderer und `Effects.cpp` mit ersetzter Hardwarehülle. Alle Längen 0–600,
Pufferwächter, drei unterscheidbare Szenen, Audio-Unabhängigkeit selbst bei
extremen Pegeln/Dauerbeats, Nacht-RGB-Grenzen/kein Blau und unveränderter
Helligkeitswert, Eintritt, Vier-Minuten-Zyklusgrenze, millis-Überlauf,
Längenwechsel und Ausstieg/Wiedereintritt erfolgreich (Exit 0).
Bei 444 LEDs größte geprüfte 16-ms-Kanalschritte: Gezeiten 3/255,
Nachtglut 2/255, Lichtseide 5/255. Das sind PC-Bildwerte, keine gemessene
Bildrate oder Helligkeit am Schlauch. Der gemeinsame Einstieg --led-effects
enthält diesen neuen Prüfer.

Auch alle unten aufgeführten bisherigen LED-Prüfer erneut Exit 0. Websimulation
und native LVGL-/Transport-/Clientprüfung nun für **42** Effekte, ungültige
ID **43** abgewiesen. Bestehende unveränderte LVGL-Objekte wiederverwendet,
betroffene Anwendungs-/Testquellen neu kompiliert/gelinkt. Nachweis
`Temp/Led-Ruhe-PC/panel/result.json`. Neue 24-s-Vorschau aus tatsächlichen
Dispatcher-Pixeln unter `Temp/Led-Ruhe-PC/Ruhewelten.png` und `Ruhewelten.gif`
visuell geprüft; Nachtglut wird darin absichtlich nicht künstlich aufgehellt.
Sicherung `Temp/Led-Ruhe-vorher/`. Keine Firmware-/Geräteaktion, keine Änderung
vorhandener BINs/Buildinfos durch Codex. Noch offen: reale Wirkung/Dimmbarkeit,
Tempo und Längen-/Auswahl-/Neustartprüfung durch Sammy nach eigenem Build.
Die folgende Liste dokumentiert die vorherige Erweiterung bis ID 39.

Gemeinsamer Einstieg `bash ./run-local-tests.sh --led-effects`; hier die
betroffenen Python-/Node-Prüfer direkt mit lokalem Python/UCRT64-g++ ausgeführt.

- `tools/test_signature_effects.py`: tatsächliches `Effects.cpp` mit
  ersetzter Arduino-/FastLED-Hardwarehülle, beide neuen Renderer und Zeitgeber;
  alle Längen 0–600 mit Pufferwächtern, neue Szenen unterscheidbar, weiche
  Eintritte und 120-s-Zyklusgrenzen, millis-Überlauf, Ausstieg/Wiedereintritt,
  falsche/extreme Eingangspegel über tatsächlichen Dispatcher, Audio/Stille
  und Impulsauslauf. Zeitgeber bei 8/19/28 Sekunden, kein Nachholburst;
  tatsächliche Route 37 setzt Gold in der Mitte und klingt wieder aus.
  Bei 444 LEDs größte geprüfte 16-ms-Kanalschritte ohne Audioimpulse:
  Sternenfluss 13/255, Prismenpuls 27/255. Exit 0, berechnete PC-Werte.
- `tools/test_chatgpt_effect.py`: Original-Choreografie unverändert, Route 36
  einschließlich Audio; Route 37 mit automatischen Goldimpulsen bleibt selbst
  bei extremen Pegeln und Dauerbeats vollständig audio-unabhängig. Exit 0.
- `tools/test_effect_studio.py`: alle 35 bisherigen Studiomotive/Regler,
  Star-Trek-Nachlauf und Rauschreaktion; tatsächlicher Dispatcher jetzt IDs
  1–39, Längenwechsel, Extremparameter und Wiedereintritt. Exit 0.
- `tools/test_led_web.js`: alle 39 Web-/Display-Namen und IDs stimmen überein;
  Klicks 36–39 sowie bisherige Request-/Fehlerfälle simuliert. Exit 0.
- Native Displayprüfung: tatsächliche LVGL-Auswahl aller 39 IDs sowie direkter
  LED-Transport und gemeinsamer Panelclient/Statusvergleich, ungültige ID 40,
  Auswahl ohne Senden und bestehende Sperren. Betroffene Quellen neu kompiliert
  und mit vorhandenen geprüften unveränderten LVGL-Objekten gelinkt. Exit 0,
  `Temp/Led-Signatur-PC/panel/result.json`. Keine Firmwarekompilierung.
- `tools/test_led_output_limit.py`: bisherige Ausgangsgrenze erfolgreich.
  Designgenerator/`--check`, Dokumentationslinks und Diff geprüft.

Berechnete 24-s-Vorschau aus den tatsächlichen Dispatcher-Pixeln visuell
geprüft: `Temp/Led-Signatur-PC/Lichtwelten.png` und `Lichtwelten.gif`.
Prismenpuls verwendet darin simuliertes Audio; keine Geräte-/Mikrofonaufnahme.
Sicherung des Vorstands unter `Temp/Led-Signatur-vorher/`.

SKIP: unveränderte andere Projektsuiten, erneute vollständige Displaygrundlagen,
echter Browser, Arduino-Build und reale LED-/Mikrofon-/Touchabnahme.
Sammy kompiliert selbst. Am Gerät noch prüfen: neue Auswahl, ruhige Bewegung,
Goldspuren nach ca. acht Sekunden, Stille/laute Musik, Längenwechsel,
Helligkeit, Effektwechsel und Neustart. PC-Bildwerte ersetzen keine optische,
elektrische oder zeitliche Hardwareabnahme. Kein Build/Upload/Deployment,
Commit oder Push durch Codex; vorhandene BIN-/Buildinfoänderungen erhalten.
