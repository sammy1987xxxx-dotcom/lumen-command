#pragma once
#include "CommandCenterUi.h"

// Webseiten des LED-Controllers (Firmware-Update und Hauptseite), aus WifiControl.cpp
// ausgelagert, damit dort nur Netzwerk und Anfrage-Behandlung stehen.

const char update_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP32 Firmware Update</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; text-align: center; background-color: #1a202c; color: #fff; margin: 0; padding: 20px;}
    h1 { color: #f39c12; }
    .container { background: rgba(26, 32, 44, 0.85); padding: 30px; border-radius: 10px; max-width: 500px; margin: 40px auto; }
    input[type="file"] { background-color: #2d3748; border: 1px solid #4a5568; color: #fff; padding: 12px; border-radius: 5px; width: 90%; }
    input[type="submit"] { background-color: #f39c12; border: none; color: white; padding: 15px 30px; font-size: 16px; margin-top: 20px; cursor: pointer; border-radius: 8px; transition: all 0.2s ease; }
    input[type="submit"]:hover { background-color: #e67e22; }
    #status { margin-top: 20px; font-size: 1.1em; color: #00bcd4; }
    a.back-btn { color: #00bcd4; text-decoration: none; margin-top: 20px; display: inline-block; }
  </style>
)rawliteral" COMMAND_CENTER_HEAD R"rawliteral(</head>
<body>
<a href="/access">Zugangsdaten ändern</a>
  <div class="container">
    <h1>Firmware Update (OTA)</h1>
    <p>Wähle eine .bin Datei von deinem Computer aus.</p>
    <form id="uploadForm" method='POST' action='/update-upload' enctype='multipart/form-data'>
      <input type='file' name='update' accept='.bin' required>
      <br>
      <input type='submit' value='Update!'>
    </form>
    <div id="status"></div>
    <a href="/" class="back-btn">&laquo; Zurück zur Steuerung</a>
  </div>
  <script>
    document.getElementById('uploadForm').addEventListener('submit', function(e) {
      e.preventDefault();
      var form = e.target;
      var data = new FormData(form);
      var xhr = new XMLHttpRequest();
      var status = document.getElementById('status');

      status.innerHTML = 'Upload startet... Bitte warte! Der ESP startet danach neu.';

      xhr.open('POST', form.action, true);

      xhr.onload = function () {
        if (xhr.status === 200) {
          status.innerHTML = 'Update erfolgreich! Der ESP startet jetzt neu...';
          setTimeout(() => {
            status.innerHTML = 'Verbindung verloren. Lade die Hauptseite in 10 Sekunden neu.';
            setTimeout(() => { window.location.href = '/'; }, 10000);
          }, 5000);
        } else {
          status.innerHTML = 'Update fehlgeschlagen! Status: ' + xhr.status + ' - ' + xhr.responseText;
        }
      };

      xhr.onerror = function() {
        status.innerHTML = 'Upload fehlgeschlagen! (Netzwerkfehler)';
      };

      xhr.send(data);
    });
  </script>
</body>
</html>
)rawliteral";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP32 LED Steuerung Pro</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; text-align: center; background-color: #1a202c; color: #fff; margin: 0; padding: 20px;}
    h1, h2 { color: #00bcd4; border-bottom: 2px solid #4a5568; padding-bottom: 10px; }

 h3 { color: #bbb; margin-top: 25px; text-align: left; }
    .container { background: rgba(26, 32, 44, 0.85); backdrop-filter: blur(10px); padding: 20px; border-radius: 10px; margin-top: 20px; max-width: 800px; margin-left: auto; margin-right: auto; }
    label { font-size: 1.2em; display: block; margin: 15px 0 5px 0;
}
    .slider { width: 100%; }
    .slider-container { display: flex; align-items: center; gap: 15px;
}
    .slider-value { font-weight: bold; min-width: 35px; text-align: right; }
    .button-grid { display: grid;
grid-template-columns: repeat(auto-fit, minmax(120px, 1fr)); gap: 10px; }
    .button { background-color: #00bcd4; border: none; color: white;
padding: 15px 20px; text-align: center; font-size: 16px; margin: 4px 2px; cursor: pointer; border-radius: 8px; transition: all 0.2s ease;
}
    .button:hover { background-color: #0097a7; transform: translateY(-2px); }
    .effect-btn.audio::before { content: "A"; display: inline-block; margin-right: 6px; padding: 0 5px;
border-radius: 4px; background: #ff9800; color: #111; font-size: 0.75em; font-weight: bold; vertical-align: middle; }
    .button.active { background-color: #2ecc71;
box-shadow: 0 0 10px #2ecc71; transform: translateY(-2px); }
    .settings-form { display: grid; grid-template-columns: auto 1fr;
gap: 12px 10px; align-items: center; }
    .led-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(30px, 1fr)); gap: 5px;
margin-top: 15px; padding: 10px; border: 1px solid #4a5568; border-radius: 8px; }
    .led-box { width: 30px; height: 22px; display: flex; align-items: center; justify-content: center;
font-size: 10px; color: #eee; text-shadow: 0 0 2px #000; user-select: none;
background-color: #555; border-radius: 4px; cursor: pointer; transition: transform 0.1s; }
    .led-box.shown { outline: 2px solid #fff; background-color: #fff; color: #000; text-shadow: none; }
    .led-show { display: flex; flex-wrap: wrap; gap: 6px; align-items: center; }
    .led-show input { width: 5em; }
    .slot-state { font-size: 0.85em; color: #bbb; }
    .led-box:hover { transform: scale(1.2);
}
    .form-footer { grid-column: 1 / -1; text-align: center; margin-top: 10px;
}
    .accordion-title { background-color: #2d3748; cursor: pointer; padding: 15px; border-radius: 5px; margin-top: 10px; font-weight: bold;
}
    .effect-settings { display: none; padding: 15px; border: 1px solid #4a5568; border-top: none;
border-radius: 0 0 5px 5px; }
    input[type="text"], input[type="password"] { background-color: #2d3748; border: 1px solid #4a5568; color: #fff; padding: 10px; border-radius: 5px; width: 100%; box-sizing: border-box; }
  </style>
)rawliteral" COMMAND_CENTER_HEAD R"rawliteral(</head>
<body>
<a href="/access">Zugangsdaten ändern</a>
  <h1>💡 ESP32 LED Steuerung Pro 💡</h1>

  <div class="container" data-cc-fold="color">
    <h2>Feste Farbe</h2>
    <div class="settings-form">
        <label for="solidColorPicker">Farbe wählen:</label>
        <input type="color" id="solidColorPicker" value="#ff0000">
    </div>
    <div class="button-grid" style="margin-top: 15px;">
        <button class="button" onclick="setSolidColor('FFAB5E')">Warmweiß</button>
        <button class="button" onclick="setSolidColor('C2E2FF')">Kaltweiß</button>
        <button class="button" onclick="setSolidColor('ff0000')">Rot</button>

<button class="button" onclick="setSolidColor('00ff00')">Grün</button>
        <button class="button" onclick="setSolidColor('0000ff')">Blau</button>
    </div>
  </div>

  <div class="container">
    <h2>Effekte</h2>
    <p style="font-size:0.9em;color:#bbb;margin-top:0;">Effekte mit <b style="background:#ff9800;color:#111;border-radius:4px;padding:0 5px;">A</b> reagieren auf Musik (Mikrofon).</p>
    <p id="commandNotice" role="status" aria-live="polite">Bereit.</p>
    <div class="button-grid">
      <button class="button effect-btn" data-effect-id="1">Regenbogen</button>
      <button class="button effect-btn" data-effect-id="2">Cylon</button>
      <button class="button effect-btn" data-effect-id="3">Konfetti</button>
      <button class="button effect-btn" data-effect-id="4">Twinkle</button>
      <button class="button effect-btn" data-effect-id="5">Meteor</button>
      <button class="button effect-btn" data-effect-id="6">Lauflicht</button>
      <button class="button effect-btn" data-effect-id="7">Feuer</button>
      <button
class="button effect-btn" data-effect-id="8">Star-Trek</button>
      <button class="button effect-btn" data-effect-id="9">Gemini-Effekt</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="10">Audio Beat-Konfetti</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="11">VU Meter (Mitte)</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="12">Audio Puls</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="13">Audio Welle</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="14">Audio Beat-Welle</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="15">Audio Beat-Implosion</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="16">Audio-Glitzer</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="17">Audio Beat-Strobe</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)"
data-effect-id="18">Audio-Explosion</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="19">Audio-Feuerwerk</button>
      <button class="button effect-btn" data-effect-id="20">Sinelon</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="21">VU-Meter (Links)</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="22">Beat-Komet</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="23">Lautstärke-Rauschen</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="24">Segment-Anzeige</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="25">Beat-Farbwechsel</button>
      <button class="button effect-btn" data-effect-id="26">Atmen</button>
      <button class="button effect-btn" data-effect-id="27">Plasma</button>
      <button class="button effect-btn" data-effect-id="28">Farbwischer</button>
      <button class="button effect-btn" data-effect-id="29">Jagende Punkte</button>
      <button class="button effect-btn" data-effect-id="30">Blaulicht</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="31">Zweifach-VU</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="32">Spektrum-Füller</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="33">Regenbogen-VU</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="34">Beat-Shake</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="35">Sinelon Beat-Drop</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="36">ChatGPT</button>
      <button class="button effect-btn" data-effect-id="37">ChatGPT ohne Musik</button>
      <button class="button effect-btn" data-effect-id="38">Sternenfluss</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="39">Prismenpuls</button>
      <button class="button effect-btn" data-effect-id="40">Gezeiten</button>
      <button class="button effect-btn" data-effect-id="41">Nachtglut</button>
      <button class="button effect-btn" data-effect-id="42">Lichtseide</button>
      <button class="button effect-btn" data-effect-id="43">Goldschweif</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="44">Claude</button>
      <button class="button effect-btn" data-effect-id="45">Claude ohne Musik</button>
      <button class="button effect-btn" data-effect-id="46">Goldatem</button>
      <button class="button effect-btn" data-effect-id="47">Goldpendel</button>
      <button class="button effect-btn" data-effect-id="48">Goldtrio</button>
      <button class="button effect-btn" data-effect-id="49">ChatGPT Original</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="50">Opus Resonanz</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="51">Opus Puls</button>
      <button class="button effect-btn" data-effect-id="52">Opus Polarlicht</button>
      <button class="button effect-btn" data-effect-id="53">Ruhe mit Türzeichen</button>
      <button class="button effect-btn audio" title="Reagiert auf Musik (Mikrofon)" data-effect-id="54">ChatGPT Tür + Musik</button>
      <button class="button effect-btn" title="Atembegleiter: 4 s ein, 1 s halten, 5 s aus" data-effect-id="55">Atemfluss</button>
      <button class="button effect-btn audio" title="Reagiert sanft auf Musik (Mikrofon)" data-effect-id="56">Klangschale</button>
      <button class="button effect-btn audio" title="Reagiert sanft auf Musik (Mikrofon)" data-effect-id="57">Mondsee</button>
      <button class="button effect-btn" title="Abendrot, in 30 Minuten ganz dunkel" data-effect-id="58">Einschlafen</button>
      <button class="button effect-btn" title="Sehr dunkles Nachtlicht" data-effect-id="59">Glühwürmchen</button>
      <button class="button effect-btn audio" title="Nur mit Musik, bei Stille dunkel" data-effect-id="63">Nachtblüte</button>
      <button class="button effect-btn audio" title="Nur mit Musik, bei Stille dunkel" data-effect-id="64">Klangpolarlicht</button>
      <button class="button effect-btn audio" title="Nur mit Musik, bei Stille dunkel" data-effect-id="65">Lichttanz</button>
      <button class="button" onclick="setSolidColor('000000')" style="background-color:#c9302c;">Alles AUS</button>
    </div>
  </div>

  <div class="container">
    <h2>Globale Steuerung</h2>
    <div class="settings-form">
        <label for="brightnessSlider">Helligkeit:</label>
        <div class="slider-container"><input type="range" min="0" max="255" class="slider" id="brightnessSlider"><span id="brightnessValue" class="slider-value">...</span></div>
        <label for="ledsSlider">Anzahl LEDs:</label>
        <div class="slider-container"><input type="range" min="1" max="600" class="slider" id="ledsSlider"><span id="ledsValue" class="slider-value">...</span></div>
        <label for="doorSignalLevel">Türzeichen bei laufendem Effekt:</label>
        <div class="slider-container"><input type="range" min="0" max="255" class="slider" id="doorSignalLevel"><span id="doorSignalLevelValue" class="slider-value">...</span></div>
        <label for="doorLightLevel">Türlicht bei „Alles AUS“:</label>
        <div class="slider-container"><input type="range" min="0" max="255" class="slider" id="doorLightLevel"><span id="doorLightLevelValue" class="slider-value">...</span></div>
        <label for="doorLightBtn">Türlicht:</label>
        <div><button class="button" id="doorLightBtn" type="button">Türlicht AN</button></div>
        <label for="doorFirstInput">Tür über LED (von – bis):</label>
        <div class="led-show">
          <input type="number" id="doorFirstInput" min="1" max="600" value="335">
          <input type="number" id="doorLastInput" min="1" max="600" value="370">
          <button class="button" id="doorRangeBtn" type="button">Übernehmen</button>
        </div>
        <p style="font-size:0.9em;color:#bbb;grid-column: 1 / -1;">Türlicht reagiert auf Türen mit eingeschaltetem Schalter „Türlicht“. Den Bereich über der Tür findest du mit „LED-Nr. zeigen“ (Einzel-LED-Steuerung).</p>

    </div>

    <div style="margin-top: 20px; text-align: center;">
        <a href="/update" class="button" style="background-color: #f39c12; display: inline-block;">Firmware Update (OTA)</a>
    </div>

  </div>

  <div class="container" data-cc-fold="wifi">
    <h2>WLAN Konfiguration</h2>
    <div class="settings-form">
      <label for="wifiSsid">SSID (Netzwerkname):</label>
      <input type="text" id="wifiSsid" class="slider" placeholder="WLAN-Name eingeben">

      <label for="wifiPass">Passwort:</label>
      <input type="password" id="wifiPass" class="slider" placeholder="Passwort eingeben">
    </div>
    <div style="margin-top: 15px; text-align: center;">
      <button class="button" onclick="saveWifi()">Speichern & Neustart</button>
    </div>
  </div>

  <div class="container" data-cc-fold="effects">
    <h2>Effekt-Anpassung</h2>

<div class="accordion">

      <div class="accordion-title" data-target="audio-settings">Mikrofon</div>
      <div id="audio-settings" class="effect-settings">
        <div class="settings-form">
          <label for="micSensitivity">Empfindlichkeit (1 = nur laute Musik, 10 = sehr empfindlich):</label><div class="slider-container"><input type="range" min="1" max="10" class="slider" id="micSensitivity"><span id="micSensitivityValue" class="slider-value">...</span></div>
        </div>
        <div class="settings-form">
          <label for="remoteMicBtn">Würfel als WLAN-Mikrofon (am Würfel dreimal drücken):</label>
          <div><button class="button" id="remoteMicBtn" type="button">Würfel-Mikrofon ERLAUBT</button></div>
        </div>
        <p style="font-size:0.9em;color:#bbb;text-align:center;">Quelle: <span id="micSource" style="color:#00bcd4">...</span> · Mikrofon hört gerade: <span id="livePeak" style="color:#00bcd4">...</span> (Stille etwa 10–20). Die Automatik regelt die Lautstärke selbst nach.</p>
      </div>
      </div>

<div class="accordion-title" data-target="gold-settings">Goldschweif, Goldatem, Goldpendel, Goldtrio</div>
<div id="gold-settings" class="effect-settings"><div class="settings-form"><label for="goldSpeed">Geschwindigkeit (höher = schneller):</label><div class="slider-container"><input type="range" min="10" max="200" id="goldSpeed" class="slider"><span id="goldSpeedValue" class="slider-value">...</span></div><label for="goldPause">Pause nach dem Schweif (ms):</label><div class="slider-container"><input type="range" min="0" max="15000" id="goldPause" class="slider"><span id="goldPauseValue" class="slider-value">...</span></div><label for="goldTail">Schweiflänge:</label><div class="slider-container"><input type="range" min="80" max="1200" id="goldTail" class="slider"><span id="goldTailValue" class="slider-value">...</span></div><label for="goldRed">Rot:</label><div class="slider-container"><input type="range" min="0" max="255" id="goldRed" class="slider"><span id="goldRedValue" class="slider-value">...</span></div><label for="goldGreen">Grün:</label><div class="slider-container"><input type="range" min="0" max="255" id="goldGreen" class="slider"><span id="goldGreenValue" class="slider-value">...</span></div><label for="goldBlue">Blau:</label><div class="slider-container"><input type="range" min="0" max="255" id="goldBlue" class="slider"><span id="goldBlueValue" class="slider-value">...</span></div></div></div>
      <div class="accordion-title" data-target="rainbow-settings">Regenbogen</div>
      <div id="rainbow-settings" class="effect-settings"><div class="settings-form"><label for="rainbowSpeed">Geschwindigkeit:</label><div class="slider-container"><input type="range" min="1" max="255" class="slider" id="rainbowSpeed"><span id="rainbowSpeedValue" class="slider-value">...</span></div><label for="rainbowGap">Farbabstand:</label><div class="slider-container"><input type="range" min="1" max="50" class="slider" id="rainbowGap"><span id="rainbowGapValue" class="slider-value">...</span></div></div></div>

      <div class="accordion-title" data-target="cylon-settings">Cylon</div>
      <div id="cylon-settings" class="effect-settings"><div class="settings-form"><label for="cylonSpeed">Geschwindigkeit:</label><div class="slider-container"><input type="range" min="1" max="255" class="slider" id="cylonSpeed"><span id="cylonSpeedValue" class="slider-value">...</span></div><label for="cylonFade">Schweif:</label><div class="slider-container"><input type="range" min="1" max="200" class="slider" id="cylonFade"><span id="cylonFadeValue" class="slider-value">...</span></div><label for="cylonHueSpeed">Farbwechsel-Geschw.:</label><div class="slider-container"><input type="range" min="1" max="600" class="slider" id="cylonHueSpeed"><span id="cylonHueSpeedValue" class="slider-value">...</span></div><div class="form-footer"><button class="button" id="cylonColorCycleBtn">Farbwechsel AUS</button></div></div></div>

      <div class="accordion-title" data-target="konfetti-settings">Konfetti</div>

    <div id="konfetti-settings" class="effect-settings"><div class="settings-form"><label for="confettiSpeed">Geschwindigkeit:</label><div class="slider-container"><input type="range" min="1" max="255" class="slider" id="confettiSpeed"><span id="confettiSpeedValue" class="slider-value">...</span></div></div></div>

      <div class="accordion-title" data-target="startrek-settings">Star-Trek</div>
      <div id="startrek-settings" class="effect-settings"><div class="settings-form"><label for="starTrekSpeed">Geschwindigkeit:</label><div class="slider-container"><input type="range" min="1" max="255" class="slider" id="starTrekSpeed"><span id="starTrekSpeedValue" class="slider-value">...</span></div><label for="starTrekFade">Schweif:</label><div class="slider-container"><input type="range" min="1" max="100" class="slider" id="starTrekFade"><span id="starTrekFadeValue" class="slider-value">...</span></div><label for="starTrekHueSpeed">Farbwechsel-Geschw.:</label><div class="slider-container"><input type="range" min="1" max="600" class="slider" id="starTrekHueSpeed"><span id="starTrekHueSpeedValue" class="slider-value">...</span></div><div class="form-footer"><button class="button" id="starTrekColorCycleBtn">Farbwechsel AUS</button></div></div></div>

      <div class="accordion-title" data-target="twinkle-settings">Twinkle</div>
      <div id="twinkle-settings" class="effect-settings"><div class="settings-form"><label for="twinkleSpeed">Geschwindigkeit:</label><div class="slider-container"><input type="range" min="1" max="255" class="slider"
id="twinkleSpeed"><span id="twinkleSpeedValue" class="slider-value">...</span></div><label for="twinkleDensity">Dichte:</label><div class="slider-container"><input type="range" min="1" max="100" class="slider" id="twinkleDensity"><span id="twinkleDensityValue" class="slider-value">...</span></div></div></div>

      <div class="accordion-title" data-target="meteor-settings">Meteor</div>
      <div id="meteor-settings" class="effect-settings"><div class="settings-form"><label for="meteorSpeed">Geschwindigkeit:</label><div class="slider-container"><input type="range" min="1" max="255" class="slider" id="meteorSpeed"><span id="meteorSpeedValue" class="slider-value">...</span></div><label for="meteorFade">Schweif:</label><div class="slider-container"><input type="range" min="1" max="255" class="slider" id="meteorFade"><span id="meteorFadeValue" class="slider-value">...</span></div></div></div>

      <div class="accordion-title" data-target="lauflicht-settings">Lauflicht</div>
      <div id="lauflicht-settings" class="effect-settings"><div class="settings-form"><label for="theaterChaseSpeed">Geschwindigkeit:</label><div class="slider-container"><input type="range" min="1" max="255" class="slider" id="theaterChaseSpeed"><span id="theaterChaseSpeedValue" class="slider-value">...</span></div></div></div>

      <div class="accordion-title" data-target="fire-settings">Feuer</div>

    <div id="fire-settings" class="effect-settings"><div class="settings-form"><label for="fireCooling">Kühlung:</label><div class="slider-container"><input type="range" min="20" max="100" class="slider" id="fireCooling"><span id="fireCoolingValue" class="slider-value">...</span></div><label for="fireSparking">Funken:</label><div class="slider-container"><input type="range" min="50" max="200" class="slider" id="fireSparking"><span id="fireSparkingValue" class="slider-value">...</span></div></div></div>

      <div class="accordion-title" data-target="gemini-settings">Gemini-Effekt</div>
      <div id="gemini-settings" class="effect-settings"><div class="settings-form"><label for="geminiSpeed">Geschwindigkeit:</label><div class="slider-container"><input type="range" min="1" max="255" class="slider" id="geminiSpeed"><span id="geminiSpeedValue" class="slider-value">...</span></div><label for="geminiFade">Schweif:</label><div class="slider-container"><input type="range" min="1" max="200" class="slider" id="geminiFade"><span id="geminiFadeValue" class="slider-value">...</span></div></div></div>

      <div class="accordion-title" data-target="confetticlap-settings">Audio Beat-Konfetti</div>
      <div id="confetticlap-settings" class="effect-settings"><div class="settings-form"><label for="confettiClapSpeed">Fade-Geschw.:</label><div class="slider-container"><input type="range" min="1" max="255" class="slider" id="confettiClapSpeed"><span id="confettiClapSpeedValue" class="slider-value">...</span></div></div></div>

      <div class="accordion-title" data-target="vumeter-settings">VU
Meter (Mitte)</div>
      <div id="vumeter-settings" class="effect-settings"><div class="settings-form">
        <label for="vuMeterSpeed">Geschw.:</label><div class="slider-container"><input type="range" min="1" max="255" class="slider" id="vuMeterSpeed"><span id="vuMeterSpeedValue" class="slider-value">...</span></div>
      </div></div>

      <div class="accordion-title" data-target="audiopulse-settings">Audio Puls</div>
      <div id="audiopulse-settings" class="effect-settings"><div class="settings-form">
        <label for="audioPulseFade">Fade (10=schnell, 255=langsam):</label><div class="slider-container"><input type="range" min="10" max="255" class="slider" id="audioPulseFade"><span id="audioPulseFadeValue" class="slider-value">...</span></div>
      </div></div>

      <div class="accordion-title" data-target="audiowave-settings">Audio Welle</div>
      <div id="audiowave-settings" class="effect-settings"><div class="settings-form">

  <label for="audioWaveSpeed">Geschw.-Multiplikator:</label><div class="slider-container"><input type="range" min="1" max="100" class="slider" id="audioWaveSpeed"><span id="audioWaveSpeedValue" class="slider-value">...</span></div>
      </div></div>

      <div class="accordion-title" data-target="audiobeatripple-settings">Effekt 14/15: Welle & Implosion</div>
      <div id="audiobeatripple-settings" class="effect-settings"><div class="settings-form">
        <label for="beatRippleSpeed">Geschwindigkeit (Shift-Tempo):</label><div class="slider-container"><input type="range" min="1" max="255" class="slider" id="beatRippleSpeed"><span id="beatRippleSpeedValue" class="slider-value">...</span></div>
        <label for="audioBeatRippleFade">Welle (14) Schweif (Fade):</label><div class="slider-container"><input type="range" min="1" max="200" class="slider" id="audioBeatRippleFade"><span id="audioBeatRippleFadeValue" class="slider-value">...</span></div>
        <label for="audioBeatImplosionFade">Implosion (15) Schweif (Fade):</label><div class="slider-container"><input type="range" min="1" max="200" class="slider" id="audioBeatImplosionFade"><span id="audioBeatImplosionFadeValue" class="slider-value">...</span></div>
        <label for="beatRippleWidth">Welle (14) Beat-Breite:</label><div class="slider-container"><input type="range" min="1" max="10" class="slider" id="beatRippleWidth"><span id="beatRippleWidthValue" class="slider-value">...</span></div>
        <p style="font-size:0.9em;color:#bbb;grid-column: 1 / -1;">Hinweis: Implosion (15) hat eine feste Breite von 1 Pixel, um den Schweif korrekt darzustellen.</p>
        <div class="form-footer"><button class="button" id="beatRippleColorCycleBtn">Farbwechsel AUS</button></div>
      </div></div>

      <div class="accordion-title" data-target="audiosparkle-settings">Audio-Glitzer</div>
      <div id="audiosparkle-settings" class="effect-settings"><div class="settings-form">
        <label for="audioSparkleFade">Fade-Rate:</label><div class="slider-container"><input type="range" min="10" max="200" class="slider" id="audioSparkleFade"><span
id="audioSparkleFadeValue" class="slider-value">...</span></div>
        <label for="audioSparkleDensity">Dichte (Anzahl Funken):</label><div class="slider-container"><input type="range" min="1" max="255" class="slider" id="audioSparkleDensity"><span id="audioSparkleDensityValue" class="slider-value">...</span></div>
      </div></div>

      <div class="accordion-title" data-target="audiostrobe-settings">Audio Beat-Strobe</div>
      <div id="audiostrobe-settings" class="effect-settings"><div class="settings-form">
        <label for="audioStrobeFade">Fade-Rate:</label><div class="slider-container"><input type="range" min="10" max="255" class="slider" id="audioStrobeFade"><span id="audioStrobeFadeValue" class="slider-value">...</span></div>
      </div></div>

      <div class="accordion-title" data-target="audiofireworks-settings">Audio-Explosion (Effekt 18)</div>
      <div
id="audiofireworks-settings" class="effect-settings"><div class="settings-form">
        <label for="fireworksSparkleDensity">Funken-Dichte:</label><div class="slider-container"><input type="range" min="1" max="255" class="slider" id="fireworksSparkleDensity"><span id="fireworksSparkleDensityValue" class="slider-value">...</span></div>
        <p style="font-size:0.9em;color:#bbb;grid-column: 1 / -1;">Hinweis: Dieser Effekt teilt sich den "Schweif (Fade)"-Slider mit "Welle (14)".</p>
      </div></div>

      <div class="accordion-title" data-target="audiorocketfireworks-settings">Audio-Feuerwerk (Effekt 19)</div>
      <div id="audiorocketfireworks-settings" class="effect-settings"><div class="settings-form">
        <label for="rocketSpeed">Raketen-Geschw.:</label><div class="slider-container"><input type="range" min="1" max="255" class="slider" id="rocketSpeed"><span id="rocketSpeedValue" class="slider-value">...</span></div>
        <label for="rocketExplosionDensity">Explosions-Dichte:</label><div class="slider-container"><input type="range" min="1" max="255" class="slider" id="rocketExplosionDensity"><span id="rocketExplosionDensityValue" class="slider-value">...</span></div>
        <label for="rocketFade">Schweif (Fade):</label><div class="slider-container"><input type="range" min="10" max="200" class="slider" id="rocketFade"><span id="rocketFadeValue" class="slider-value">...</span></div>
      </div></div>

      <div class="accordion-title" data-target="sinelon-settings">Sinelon (Effekt 20)</div>
      <div id="sinelon-settings" class="effect-settings"><div class="settings-form">
        <label for="sinelonSpeed">Geschwindigkeit:</label><div class="slider-container"><input type="range" min="1" max="50" class="slider" id="sinelonSpeed"><span id="sinelonSpeedValue" class="slider-value">...</span></div>
        <label for="sinelonFade">Schweif (Fade):</label><div class="slider-container"><input type="range" min="1" max="200" class="slider" id="sinelonFade"><span id="sinelonFadeValue" class="slider-value">...</span></div>
        <label for="sinelonHueSpeed">Farbwechsel-Geschw.:</label><div class="slider-container"><input type="range" min="0" max="10" class="slider" id="sinelonHueSpeed"><span id="sinelonHueSpeedValue" class="slider-value">...</span></div>
      </div></div>

      <!-- NEUE ACCORDIONS FÜR EFFEKTE 21-30 -->
      <div class="accordion-title" data-target="vumeterleft-settings">VU-Meter (Links)</div>
      <div id="vumeterleft-settings" class="effect-settings"><div class="settings-form"><label for="vuMeterLeftSpeed">Geschwindigkeit:</label><div class="slider-container"><input type="range" min="1" max="255" class="slider" id="vuMeterLeftSpeed"><span id="vuMeterLeftSpeedValue" class="slider-value">...</span></div></div></div>

      <div class="accordion-title" data-target="beatcomet-settings">Beat-Komet</div>
      <div id="beatcomet-settings" class="effect-settings"><div class="settings-form"><label for="beatCometFade">Schweif (Fade):</label><div class="slider-container"><input type="range" min="1" max="100" class="slider" id="beatCometFade"><span id="beatCometFadeValue" class="slider-value">...</span></div></div></div>

      <div class="accordion-title" data-target="loudnessnoise-settings">Lautstärke-Rauschen</div>
      <div id="loudnessnoise-settings" class="effect-settings"><div class="settings-form"><label for="loudnessNoiseDensity">Dichte:</label><div class="slider-container"><input type="range" min="1" max="255" class="slider" id="loudnessNoiseDensity"><span id="loudnessNoiseDensityValue" class="slider-value">...</span></div></div></div>

      <div class="accordion-title" data-target="segmentview-settings">Segment-Anzeige</div>
      <div id="segmentview-settings" class="effect-settings"><div class="settings-form"><label for="segmentViewSegments">Anzahl Segmente:</label><div class="slider-container"><input type="range" min="2" max="40" class="slider" id="segmentViewSegments"><span id="segmentViewSegmentsValue" class="slider-value">...</span></div></div></div>

      <div class="accordion-title" data-target="beatflash-settings">Beat-Farbwechsel</div>
      <div id="beatflash-settings" class="effect-settings"><div class="settings-form"><label for="beatFlashFade">Fade-Geschwindigkeit:</label><div class="slider-container"><input type="range" min="1" max="50" class="slider" id="beatFlashFade"><span id="beatFlashFadeValue" class="slider-value">...</span></div></div></div>

      <div class="accordion-title" data-target="breathing-settings">Atmen</div>
      <div id="breathing-settings" class="effect-settings"><div class="settings-form"><label for="breathingSpeed">Geschwindigkeit:</label><div class="slider-container"><input type="range" min="1" max="50" class="slider" id="breathingSpeed"><span id="breathingSpeedValue" class="slider-value">...</span></div></div></div>

      <div class="accordion-title" data-target="plasma-settings">Plasma</div>
      <div id="plasma-settings" class="effect-settings"><div class="settings-form">
        <label for="plasmaSpeed1">Geschwindigkeit 1:</label><div class="slider-container"><input type="range" min="10" max="200" class="slider" id="plasmaSpeed1"><span id="plasmaSpeed1Value" class="slider-value">...</span></div>
        <label for="plasmaSpeed2">Geschwindigkeit 2:</label><div class="slider-container"><input type="range" min="10" max="200" class="slider" id="plasmaSpeed2"><span id="plasmaSpeed2Value" class="slider-value">...</span></div>
        <label for="plasmaScale1">Skalierung 1:</label><div class="slider-container"><input type="range" min="1" max="50" class="slider" id="plasmaScale1"><span id="plasmaScale1Value" class="slider-value">...</span></div>
        <label for="plasmaScale2">Skalierung 2:</label><div class="slider-container"><input type="range" min="1" max="50" class="slider" id="plasmaScale2"><span id="plasmaScale2Value" class="slider-value">...</span></div>
      </div></div>

      <div class="accordion-title" data-target="colorwipe-settings">Farbwischer</div>
      <div id="colorwipe-settings" class="effect-settings"><div class="settings-form"><label for="colorWipeSpeed">Geschwindigkeit:</label><div class="slider-container"><input type="range" min="1" max="255" class="slider" id="colorWipeSpeed"><span id="colorWipeSpeedValue" class="slider-value">...</span></div></div></div>

      <div class="accordion-title" data-target="chasingdots-settings">Jagende Punkte</div>
      <div id="chasingdots-settings" class="effect-settings"><div class="settings-form">
        <label for="chasingDotsNum">Anzahl Punkte:</label><div class="slider-container"><input type="range" min="1" max="20" class="slider" id="chasingDotsNum"><span id="chasingDotsNumValue" class="slider-value">...</span></div>
        <label for="chasingDotsSpeed">Geschwindigkeit:</label><div class="slider-container"><input type="range" min="1" max="255" class="slider" id="chasingDotsSpeed"><span id="chasingDotsSpeedValue" class="slider-value">...</span></div>
      </div></div>

      <div class="accordion-title" data-target="police-settings">Blaulicht</div>
      <div id="police-settings" class="effect-settings"><div class="settings-form"><label for="policeSpeed">Geschwindigkeit:</label><div class="slider-container"><input type="range" min="1" max="255" class="slider" id="policeSpeed"><span id="policeSpeedValue" class="slider-value">...</span></div></div></div>

    </div>
  </div>

  <div class="container" data-cc-fold="individual">
    <h2>Einzel-LED Steuerung</h2>
    <div class="settings-form">
      <label for="singleLedColor">"Malfarbe":</label>
      <input type="color" id="singleLedColor" value="#00ff00">
      <label for="showLedNr">LED-Nr. zeigen:</label>
      <div class="led-show">
        <button class="button" id="showLedPrev" type="button">&minus;1</button>
        <input type="number" id="showLedNr" min="1" value="1">
        <button class="button" id="showLedNext" type="button">+1</button>
        <button class="button" id="showLedBtn" type="button">Zeigen</button>
      </div>
      <p style="font-size:0.9em;color:#bbb;grid-column: 1 / -1;">Nummer 1 ist die erste LED am Anschluss. „Zeigen“ schaltet nur diese LED weiß und alle anderen aus (Helligkeit wie eingestellt). Der vorige Effekt wird nicht überschrieben gespeichert. Mit gedrückter Umschalttaste im Raster klicken zeigt die angeklickte LED.</p>
      <label for="paintFrom">Bereich färben:</label>
      <div class="led-show">
        <input type="number" id="paintFrom" min="1" value="1"> bis
        <input type="number" id="paintTo" min="1" value="1">
        <button class="button" id="paintRangeBtn" type="button">Färben</button>
        <button class="button" id="paintClearBtn" type="button">Alle schwarz</button>
      </div>
      <label>Bilderlicht:</label>
      <div>
        <div class="led-show"><button class="button effect-btn" data-effect-id="60">Bilderlicht 1</button><button class="button slot-save" type="button" data-slot="1">Speichern</button><span class="slot-state" id="slotState1"></span></div>
        <div class="led-show"><button class="button effect-btn" data-effect-id="61">Bilderlicht 2</button><button class="button slot-save" type="button" data-slot="2">Speichern</button><span class="slot-state" id="slotState2"></span></div>
        <div class="led-show"><button class="button effect-btn" data-effect-id="62">Bilderlicht 3</button><button class="button slot-save" type="button" data-slot="3">Speichern</button><span class="slot-state" id="slotState3"></span></div>
      </div>
      <p style="font-size:0.9em;color:#bbb;grid-column: 1 / -1;">Bilderlicht: „Malfarbe“ wählen, LEDs im Raster anklicken oder einen Bereich färben (Nummern wie im Raster). „Speichern“ merkt sich alle LEDs auf dem Platz – auch nach einem Neustart – und schaltet ihn ein; nicht gemalte LEDs bleiben aus, der Helligkeitsregler wirkt. Ändern: Platz aufrufen (das Raster zeigt dann seine Farben), nachmalen, wieder speichern.</p>
    </div>
    <div id="led-grid" class="led-grid"></div>
  </div>

<script>
  // One HTTP request at a time: the ESP WebServer serves a single client.
  let requestQueue = Promise.resolve();
  let pendingCommands = 0;
  let commandRevision = 0;
  let statusLoading = false;
  function queuedRequest(url, read) {
    const task = requestQueue.then(async () => {
      const controller = new AbortController();
      const timeout = setTimeout(() => controller.abort(), 5000);
      try {
        const response = await fetch(url, {cache: 'no-store', credentials: 'same-origin', signal: controller.signal});
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        return await read(response);
      } finally { clearTimeout(timeout); }
    });
    requestQueue = task.catch(() => {});
    return task;
  }
  function commandNotice(message) {
    document.getElementById('commandNotice').textContent = message;
  }
  function sendRequest(url) {
    ++commandRevision;
    ++pendingCommands;
    commandNotice('Befehl wird übernommen …');
    return queuedRequest(url, response => response.text())
      .then(() => commandNotice('Befehl angenommen. Status wird aktualisiert.'))
      .catch(error => commandNotice(`Übernahme nicht bestätigt (${error.message}). Bitte Verbindung prüfen.`))
      .finally(() => {
        --pendingCommands;
        if (!pendingCommands) loadStatus();
      });
  }

  function saveWifi() {
    const ssid = document.getElementById('wifiSsid').value;
    const pass = document.getElementById('wifiPass').value;

    if (!ssid) {
      alert('Bitte geben Sie eine SSID ein.');
      return;
    }

    if (confirm('Sollen die WLAN-Daten wirklich gespeichert werden? Der Controller startet danach neu.')) {
      // Send POST request
      fetch('/wifi-settings', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded',
        },
        body: `ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(pass)}`
      })
      .then(response => response.text())
      .then(data => {
        alert(data); // "Gespeichert. Starte neu..."
      })
      .catch(error => {
        console.error('Error:', error);
        alert('Fehler beim Speichern.');
      });
    }
  }

  function setSolidColor(colorHex) {
    document.getElementById('solidColorPicker').value = '#' + colorHex;
    sendRequest(`/setall?color=${colorHex}`);
}

  document.getElementById('solidColorPicker').addEventListener('change', (e) => {
    setSolidColor(e.target.value.substring(1));
  });
document.querySelectorAll('.effect-btn').forEach(btn => {
    btn.addEventListener('click', () => sendRequest(`/effect?id=${btn.dataset.effectId}`));
  });
document.querySelectorAll('.accordion-title').forEach(title => {
    title.addEventListener('click', () => {
        const target = document.getElementById(title.dataset.target);
        const isVisible = target.style.display === 'block';
        document.querySelectorAll('.effect-settings').forEach(el => el.style.display = 'none');
        target.style.display = isVisible ? 'none' : 'block';
    });
  });
function setupSlider(sliderId, valueId, urlParam, endpoint = '/settings') {
    const slider = document.getElementById(sliderId);
    const valueSpan = document.getElementById(valueId);
if (!slider || !valueSpan) return;
    slider.addEventListener('input', () => valueSpan.textContent = slider.value);
    slider.addEventListener('change', () => sendRequest(`${endpoint}?${urlParam}=${slider.value}`));
}

  // Richtet die Funktionalität für jeden einzelnen Slider auf der Webseite ein.
  setupSlider('brightnessSlider', 'brightnessValue', 'brightness');
  setupSlider('ledsSlider', 'ledsValue', 'leds');
  setupSlider('doorSignalLevel', 'doorSignalLevelValue', 'doorSignalLevel');
  setupSlider('doorLightLevel', 'doorLightLevelValue', 'doorLightLevel');
// Mikrofon
  setupSlider('micSensitivity', 'micSensitivityValue', 'micSensitivity', '/params');

// Normale Effekt-Slider
  setupSlider('rainbowSpeed', 'rainbowSpeedValue', 'rainbowSpeed', '/params');
  setupSlider('rainbowGap', 'rainbowGapValue', 'rainbowGap', '/params');
  setupSlider('cylonSpeed', 'cylonSpeedValue', 'cylonSpeed', '/params');
  setupSlider('cylonFade', 'cylonFadeValue', 'cylonFade', '/params');
setupSlider('cylonHueSpeed', 'cylonHueSpeedValue', 'cylonHueSpeed', '/params');
  setupSlider('confettiSpeed', 'confettiSpeedValue', 'confettiSpeed', '/params');
  setupSlider('goldSpeed', 'goldSpeedValue', 'goldSpeed', '/params');
  setupSlider('goldPause', 'goldPauseValue', 'goldPause', '/params');
  setupSlider('goldTail', 'goldTailValue', 'goldTail', '/params');
  setupSlider('goldRed', 'goldRedValue', 'goldRed', '/params');
  setupSlider('goldGreen', 'goldGreenValue', 'goldGreen', '/params');
  setupSlider('goldBlue', 'goldBlueValue', 'goldBlue', '/params');
  setupSlider('starTrekSpeed', 'starTrekSpeedValue', 'starTrekSpeed', '/params');
  setupSlider('starTrekFade', 'starTrekFadeValue', 'starTrekFade', '/params');
  setupSlider('starTrekHueSpeed', 'starTrekHueSpeedValue', 'starTrekHueSpeed', '/params');
setupSlider('twinkleSpeed', 'twinkleSpeedValue', 'twinkleSpeed', '/params');
  setupSlider('twinkleDensity', 'twinkleDensityValue', 'twinkleDensity', '/params');
  setupSlider('meteorSpeed', 'meteorSpeedValue', 'meteorSpeed', '/params');
  setupSlider('meteorFade', 'meteorFadeValue', 'meteorFade', '/params');
  setupSlider('theaterChaseSpeed', 'theaterChaseSpeedValue', 'theaterChaseSpeed', '/params');
setupSlider('fireCooling', 'fireCoolingValue', 'fireCooling', '/params');
  setupSlider('fireSparking', 'fireSparkingValue', 'fireSparking', '/params');
  setupSlider('geminiSpeed', 'geminiSpeedValue', 'geminiSpeed', '/params');
  setupSlider('geminiFade', 'geminiFadeValue', 'geminiFade', '/params');
// Audio-Effekt-Slider
  setupSlider('confettiClapSpeed', 'confettiClapSpeedValue', 'confettiClapSpeed', '/params');
  setupSlider('vuMeterSpeed', 'vuMeterSpeedValue', 'vuMeterSpeed', '/params');
  setupSlider('audioPulseFade', 'audioPulseFadeValue', 'audioPulseFade', '/params');
  setupSlider('audioWaveSpeed', 'audioWaveSpeedValue', 'audioWaveSpeed', '/params');
// --- MODIFIZIERT: Effekt 14/15 Slider ---
  setupSlider('beatRippleSpeed', 'beatRippleSpeedValue', 'beatRippleSpeed', '/params'); // NEU
  setupSlider('audioBeatRippleFade', 'audioBeatRippleFadeValue', 'audioBeatRippleFade', '/params');
  setupSlider('audioBeatImplosionFade', 'audioBeatImplosionFadeValue', 'audioBeatImplosionFade', '/params');
  setupSlider('beatRippleWidth', 'beatRippleWidthValue', 'beatRippleWidth', '/params');

  setupSlider('audioSparkleFade', 'audioSparkleFadeValue', 'audioSparkleFade', '/params');
  setupSlider('audioSparkleDensity', 'audioSparkleDensityValue', 'audioSparkleDensity', '/params');
  setupSlider('audioStrobeFade', 'audioStrobeFadeValue', 'audioStrobeFade', '/params');
// --- Slider für Effekt 18 ---
  setupSlider('fireworksSparkleDensity', 'fireworksSparkleDensityValue', 'fireworksSparkleDensity', '/params');
  // --- Slider für Effekt 19 ---
  setupSlider('rocketSpeed', 'rocketSpeedValue', 'rocketSpeed', '/params');
  setupSlider('rocketExplosionDensity', 'rocketExplosionDensityValue', 'rocketExplosionDensity', '/params');
  setupSlider('rocketFade', 'rocketFadeValue', 'rocketFade', '/params');
  // --- Slider für Effekt 20 ---
  setupSlider('sinelonSpeed', 'sinelonSpeedValue', 'sinelonSpeed', '/params');
  setupSlider('sinelonFade', 'sinelonFadeValue', 'sinelonFade', '/params');
  setupSlider('sinelonHueSpeed', 'sinelonHueSpeedValue', 'sinelonHueSpeed', '/params');

  // --- NEU: Slider für Effekte 21-30 ---
  setupSlider('vuMeterLeftSpeed', 'vuMeterLeftSpeedValue', 'vuMeterLeftSpeed', '/params');
  setupSlider('beatCometFade', 'beatCometFadeValue', 'beatCometFade', '/params');
  setupSlider('loudnessNoiseDensity', 'loudnessNoiseDensityValue', 'loudnessNoiseDensity', '/params');
  setupSlider('segmentViewSegments', 'segmentViewSegmentsValue', 'segmentViewSegments', '/params');
  setupSlider('beatFlashFade', 'beatFlashFadeValue', 'beatFlashFade', '/params');
  setupSlider('breathingSpeed', 'breathingSpeedValue', 'breathingSpeed', '/params');
  setupSlider('plasmaSpeed1', 'plasmaSpeed1Value', 'plasmaSpeed1', '/params');
  setupSlider('plasmaSpeed2', 'plasmaSpeed2Value', 'plasmaSpeed2', '/params');
  setupSlider('plasmaScale1', 'plasmaScale1Value', 'plasmaScale1', '/params');
  setupSlider('plasmaScale2', 'plasmaScale2Value', 'plasmaScale2', '/params');
  setupSlider('colorWipeSpeed', 'colorWipeSpeedValue', 'colorWipeSpeed', '/params');
  setupSlider('chasingDotsNum', 'chasingDotsNumValue', 'chasingDotsNum', '/params');
  setupSlider('chasingDotsSpeed', 'chasingDotsSpeedValue', 'chasingDotsSpeed', '/params');
  setupSlider('policeSpeed', 'policeSpeedValue', 'policeSpeed', '/params');


  document.getElementById('cylonColorCycleBtn').addEventListener('click', () => sendRequest('/toggleparam?param=cylonColor'));
  document.getElementById('starTrekColorCycleBtn').addEventListener('click', () => sendRequest('/toggleparam?param=starTrekColor'));
  document.getElementById('beatRippleColorCycleBtn').addEventListener('click', () => sendRequest('/toggleparam?param=beatRippleColor'));

  const ledGrid = document.getElementById('led-grid');

  function generateLedGrid(ledCount) {
    ledGrid.innerHTML = '';
for (let i = 0; i < ledCount; i++) {
      const ledBox = document.createElement('div');
ledBox.className = 'led-box';
      ledBox.dataset.index = i;
      ledBox.textContent = i + 1;
      ledGrid.appendChild(ledBox);
    }
    document.getElementById('showLedNr').max = ledCount;
    ['paintFrom', 'paintTo'].forEach(id => { document.getElementById(id).max = ledCount; });
  }

  // LED-Nummern sind 1-basiert, /showled erwartet wie /setled den 0-basierten Index.
  function showLed(nr) {
    const input = document.getElementById('showLedNr');
    const count = ledGrid.children.length;
    if (!count) return;
    nr = Math.min(Math.max(parseInt(nr, 10) || 1, 1), count);
    input.value = nr;
    for (const box of ledGrid.children) {
      box.classList.toggle('shown', box.dataset.index == nr - 1);
      box.style.backgroundColor = '';
    }
    sendRequest(`/showled?index=${nr - 1}`);
  }
  document.getElementById('doorLightBtn').addEventListener('click', () => sendRequest('/toggleparam?param=doorLight'));
  document.getElementById('remoteMicBtn').addEventListener('click', () => sendRequest('/toggleparam?param=remoteMic'));
  // Türbereich: Eingaben erst nach „Übernehmen“ wieder vom Gerät überschreiben lassen.
  const doorInputs = ['doorFirstInput', 'doorLastInput'].map(id => document.getElementById(id));
  doorInputs.forEach(input => input.addEventListener('input', () => { input.dataset.edited = '1'; }));
  document.getElementById('doorRangeBtn').addEventListener('click', () => {
    doorInputs.forEach(input => { input.dataset.edited = ''; });
    sendRequest(`/settings?doorFirst=${doorInputs[0].value}&doorLast=${doorInputs[1].value}`);
  });
  document.getElementById('showLedBtn').addEventListener('click', () => showLed(document.getElementById('showLedNr').value));
  document.getElementById('showLedPrev').addEventListener('click', () => showLed((parseInt(document.getElementById('showLedNr').value, 10) || 1) - 1));
  document.getElementById('showLedNext').addEventListener('click', () => showLed((parseInt(document.getElementById('showLedNr').value, 10) || 0) + 1));
  document.getElementById('showLedNr').addEventListener('keydown', (e) => { if (e.key === 'Enter') showLed(e.target.value); });

  // Bilderlicht: Bereich färben (1-basiert wie das Raster), Plätze speichern, gespeicherte Farben ins Raster.
  let slotCounts = [0, 0, 0], gridSlot = 0;
  function paintRange(first, last, hex) {
    const count = ledGrid.children.length;
    if (!count) return;
    first = Math.min(Math.max(parseInt(first, 10) || 1, 1), count);
    last = Math.min(Math.max(parseInt(last, 10) || 1, 1), count);
    if (first > last) [first, last] = [last, first];
    for (let i = first - 1; i < last; ++i) {
      ledGrid.children[i].classList.toggle('shown', false);
      ledGrid.children[i].style.backgroundColor = '#' + hex;
    }
    sendRequest(`/setled?from=${first - 1}&to=${last - 1}&color=${hex}`);
  }
  function showSlotInGrid(slot) {
    gridSlot = slot;
    return queuedRequest(`/slotcolors?slot=${slot}`, response => response.text()).then(text => {
      Array.from(ledGrid.children).forEach((box, i) => {
        const hex = text.substr(i * 6, 6);
        box.classList.toggle('shown', false);
        box.style.backgroundColor = '#' + (hex.length === 6 ? hex : '000000');
      });
    }).catch(() => { gridSlot = 0; });
  }
  document.getElementById('paintRangeBtn').addEventListener('click', () => paintRange(
    document.getElementById('paintFrom').value, document.getElementById('paintTo').value,
    document.getElementById('singleLedColor').value.substring(1)));
  document.getElementById('paintClearBtn').addEventListener('click', () => paintRange(1, ledGrid.children.length, '000000'));
  document.querySelectorAll('.slot-save').forEach(btn => btn.addEventListener('click', () => {
    const slot = Number(btn.dataset.slot);
    if (slotCounts[slot - 1] && !confirm(`Bilderlicht ${slot} überschreiben?`)) return;
    gridSlot = 0;
    sendRequest(`/saveslot?slot=${slot}`);
  }));

  ledGrid.addEventListener('click', (e) => {
    if (e.target.classList.contains('led-box') && e.shiftKey) {
      showLed(Number(e.target.dataset.index) + 1);
    } else if (e.target.classList.contains('led-box')) {
      e.target.classList.remove('shown');
      const index = e.target.dataset.index;
      const color = document.getElementById('singleLedColor').value;
      e.target.style.backgroundColor = color;
      sendRequest(`/setled?index=${index}&color=${color.substring(1)}`);
    }
  });
function loadStatus() {
    if (statusLoading || pendingCommands) return Promise.resolve();
    statusLoading = true;
    const revision = commandRevision;
    return queuedRequest('/status.json', response => response.json()).then(data => {
        // A response started before a click must not restore the old selection.
        if (revision !== commandRevision) return;
        window.CommandCenterUI?.status(data);
        const activeEl = document.activeElement;

        function updateSlider(sliderId, valueId, value) {
            const slider = document.getElementById(sliderId);
            if (slider && activeEl !== slider) slider.value = value;
            const valueEl = document.getElementById(valueId);

   if(valueEl) valueEl.textContent = value;
        }

        updateSlider('brightnessSlider', 'brightnessValue', data.brightness);
        updateSlider('ledsSlider', 'ledsValue', data.numLeds);

        for (const key in data) {
            if(document.getElementById(key)) {
                updateSlider(key, key + 'Value', data[key]);

}
        }

        [['doorFirstInput', data.doorFirst], ['doorLastInput', data.doorLast]].forEach(([id, value]) => {
            const input = document.getElementById(id);
            if (input && !input.dataset.edited && value !== undefined) input.value = value;
        });
        const doorBtn = document.getElementById('doorLightBtn');
        doorBtn.textContent = 'Türlicht ' + (data.doorLightEnabled ? 'AN' : 'AUS');
        doorBtn.style.backgroundColor = data.doorLightEnabled ? '#2ecc71' : '#00bcd4';

        const cylonBtn = document.getElementById('cylonColorCycleBtn');
        cylonBtn.textContent = 'Farbwechsel ' + (data.cylonColorCycle ? 'AN' : 'AUS');
        cylonBtn.style.backgroundColor = data.cylonColorCycle ?
'#2ecc71' : '#00bcd4';

        const starTrekBtn = document.getElementById('starTrekColorCycleBtn');
        starTrekBtn.textContent = 'Farbwechsel ' + (data.starTrekColorCycle ? 'AN' : 'AUS');
starTrekBtn.style.backgroundColor = data.starTrekColorCycle ? '#2ecc71' : '#00bcd4';

        const beatRippleBtn = document.getElementById('beatRippleColorCycleBtn');
        beatRippleBtn.textContent = 'Farbwechsel ' + (data.beatRippleColorCycle ? 'AN' : 'AUS');
        beatRippleBtn.style.backgroundColor = data.beatRippleColorCycle ? '#2ecc71' : '#00bcd4';

        const remoteMicBtn = document.getElementById('remoteMicBtn');
        remoteMicBtn.textContent = 'Würfel-Mikrofon ' + (data.remoteMicEnabled ? 'ERLAUBT' : 'AUS');
        remoteMicBtn.style.backgroundColor = data.remoteMicEnabled ? '#2ecc71' : '#00bcd4';
        document.getElementById('micSource').textContent = data.remoteMicActive ? 'Würfel (WLAN)' : 'eingebaut';

        if(document.getElementById('livePeak')) {
             document.getElementById('livePeak').textContent = data.currentPeakToPeak;
        }

        document.querySelectorAll('.effect-btn').forEach(btn => {
            btn.classList.toggle('active', btn.dataset.effectId == data.activeEffect);
        });
const gridChanged = ledGrid.children.length !== data.numLeds;
if (gridChanged) {
            generateLedGrid(data.numLeds);
}
        if (Array.isArray(data.ledSlots)) {
            slotCounts = data.ledSlots;
            slotCounts.forEach((count, i) => {
                const state = document.getElementById(`slotState${i + 1}`);
                if (state) state.textContent = count ? 'gespeichert' : 'leer';
            });
        }
        const activeSlot = data.activeEffect >= 60 && data.activeEffect <= 62 ? data.activeEffect - 59 : 0;
        if (activeSlot && (activeSlot !== gridSlot || gridChanged)) showSlotInGrid(activeSlot);
        if (!activeSlot) gridSlot = 0;

        const solidColorPicker = document.getElementById('solidColorPicker');
if (activeEl !== solidColorPicker && data.activeEffect === 0) {
            solidColorPicker.value = '#' + ('000000' + data.solidColor.toString(16)).slice(-6);
}
    }).catch(() => window.CommandCenterUI?.stale('Offline · keine aktuellen Gerätedaten'))
      .finally(() => { statusLoading = false; });
  }

  document.getElementById('ledsSlider').addEventListener('change', (e) => generateLedGrid(e.target.value));

  loadStatus();
  setInterval(loadStatus, 2000);
</script>
</body>
</html>
)rawliteral";
