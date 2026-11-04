#pragma once
// Generated from tools/DeviceAccess.h.in. Private NVS records take precedence.
#include "AccessDefaults.h"
#include "CommandCenterUi.h"
#include <Preferences.h>
#include <WebServer.h>
#include <esp_random.h>
#include <string>
#include <cstdio>

namespace device_access {
class Credentials {
    std::string space, user, pass, session;
    bool healthy=false;
    static bool readable(const std::string &u,const std::string &p) {
        if (u.empty() || u.size()>64 || p.empty() || p.size()>256) return false;
        for (unsigned char c:u) if (c<33 || c==127 || c==':') return false;
        for (unsigned char c:p) if (c<32 || c==127) return false;
        return true;
    }
public:
    bool begin(const char *name,const char *initialUser=HOMESERVER_DEFAULT_USER,
               const char *initialPassword=HOMESERVER_DEFAULT_PASSWORD) {
        space=name; user=initialUser; pass=initialPassword; healthy=false;
        Preferences storage;
        if (!storage.begin(name,false)) return false;
        const auto value=storage.getString("credentials",""); storage.end();
        const std::string record(value.c_str());
        if (!record.empty()) {
            const auto split=record.find('\n');
            if (split==std::string::npos) return false;
            user=record.substr(0,split); pass=record.substr(split+1);
        }
        // Preserve previously configured OTA passwords, even outside WPA length.
        healthy=readable(user,pass); rotate(); return healthy;
    }
    void rotate() {
        char token[33];
        std::snprintf(token,sizeof token,"%08lx%08lx%08lx%08lx",
            static_cast<unsigned long>(esp_random()),static_cast<unsigned long>(esp_random()),
            static_cast<unsigned long>(esp_random()),static_cast<unsigned long>(esp_random()));
        session=token;
    }
    const char *username() const { return user.c_str(); }
    const char *password() const { return pass.c_str(); }
    const char *token() const { return session.c_str(); }
    bool authorize(WebServer &server) const {
        if (healthy && server.authenticate(user.c_str(),pass.c_str())) return true;
        server.requestAuthentication(); return false;
    }
    bool cookie(const char *value) const {
        if (!healthy || !value) return false;
        const std::string text(value), wanted="wm_session="+session;
        size_t start=0;
        while (start<text.size()) {
            while (start<text.size() && text[start]==' ') ++start;
            const auto end=text.find(';',start);
            if (text.substr(start,end==std::string::npos ? end : end-start)==wanted) return true;
            if (end==std::string::npos) break;
            start=end+1;
        }
        return false;
    }
    bool save(const std::string &nextUser,const std::string &nextPassword) {
        if (!healthy || !readable(nextUser,nextPassword) || nextPassword.size()<8 || nextPassword.size()>63) return false;
        Preferences storage;
        if (!storage.begin(space.c_str(),false)) return false;
        const auto record=nextUser+"\n"+nextPassword;
        const bool ok=storage.putString("credentials",record.c_str())!=0 &&
            std::string(storage.getString("credentials","").c_str())==record;
        storage.end();
        if (ok) { user=nextUser; pass=nextPassword; rotate(); }
        return ok;
    }
    // Seite in Stuecken senden: Der Command-Center-Kopf (~38 KB) geht direkt aus dem Flash.
    // Frueher entstanden davon zwei bis drei Kopien im RAM; beim LED-Schlauch fehlte dafuer
    // ein zusammenhaengender Block und der ESP stuerzte beim Oeffnen ab (LED-ACCESS-01).
    void page(WebServer &server) const {
        if (!authorize(server)) return;
        static const char start[]="<!doctype html><html lang='de'><meta charset='utf-8'>"
            "<meta name='viewport' content='width=device-width,initial-scale=1'><title>Zugangsdaten</title>";
        const std::string form=std::string("<body class='cc-theme'><main style='max-width:36rem;margin:2rem auto;padding:1rem'>"
            "<h1>Zugangsdaten</h1><form method='post' action='/access'>"
            "<input type='hidden' name='access_token' value='")+session+"'>"
            "<label>Neuer Benutzername<input name='username' required maxlength='64' autocomplete='username'></label>"
            "<label>Neues Passwort (8 bis 63 Zeichen)<input name='password' type='password' required minlength='8' maxlength='63' autocomplete='new-password'></label>"
            "<label>Passwort wiederholen<input name='confirmation' type='password' required autocomplete='new-password'></label>"
            "<button>Speichern</button></form><p>Nach dem Speichern neu anmelden. Das Setup-WLAN verwendet das neue Passwort beim nächsten Start.</p><a href='/'>Zurück</a></main></body></html>";
        server.sendHeader("Cache-Control","no-store");
        server.setContentLength(CONTENT_LENGTH_UNKNOWN);
        server.send(200,"text/html; charset=utf-8","");
        server.sendContent(start,sizeof start-1);
        server.sendContent_P(COMMAND_CENTER_HEAD);
        server.sendContent(form.c_str(),form.size());
        server.sendContent("",0);
    }
    bool change(WebServer &server,bool safe=true) {
        if (!authorize(server)) return false;
        if (!safe) { server.send(409,"text/plain","Zugangsdaten nur bei sicherem Stillstand aendern."); return false; }
        if (!server.hasArg("access_token") || std::string(server.arg("access_token").c_str())!=session) {
            server.send(403,"text/plain","Formular abgelaufen. Seite neu laden."); return false;
        }
        if (!server.hasArg("username") || !server.hasArg("password") || !server.hasArg("confirmation") ||
            server.arg("password")!=server.arg("confirmation") ||
            !save(server.arg("username").c_str(),server.arg("password").c_str())) {
            server.send(400,"text/plain","Zugangsdaten ungueltig oder Speichern fehlgeschlagen."); return false;
        }
        server.sendHeader("Cache-Control","no-store");
        server.send(200,"text/html; charset=utf-8","Zugangsdaten gespeichert. <a href='/'>Neu anmelden</a>");
        return true;
    }
};
}
