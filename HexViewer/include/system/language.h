#pragma once
#include <string>
#include <map>

class Translations {
private:
  static inline std::string defaultLanguage = "English";
  static inline std::map<std::string, std::map<std::string, std::string>> translations;
public:
  static void Initialize() {
    translations["English"]["Options"] = "Options";
    translations["English"]["Dark Mode"] = "Dark Mode";
    translations["English"]["Auto-reload modified file"] = "Auto-reload modified file";
    translations["English"]["Add to context menu (right-click files)"] = "Add to context menu (right-click files)";
    translations["English"]["Language:"] = "Language:";
    translations["English"]["Default bytes per line:"] = "Default bytes per line:";
    translations["English"]["8 bytes"] = "8 bytes";
    translations["English"]["16 bytes"] = "16 bytes";
    translations["English"]["OK"] = "OK";
    translations["English"]["Cancel"] = "Cancel";

    translations["English"]["File"] = "File";
    translations["English"]["Open"] = "Open";
    translations["English"]["Save"] = "Save";
    translations["English"]["Exit"] = "Exit";
    translations["English"]["View"] = "View";
    translations["English"]["Disassembly"] = "Disassembly";
    translations["English"]["Search"] = "Search";
    translations["English"]["Find and Replace..."] = "Find and Replace...";
    translations["English"]["Go To..."] = "Go To...";
    translations["English"]["Tools"] = "Tools";
    translations["English"]["Options..."] = "Options...";
    translations["English"]["Help"] = "Help";
    translations["English"]["About HexViewer"] = "About HexViewer";

    translations["English"]["Find:"] = "Find:";
    translations["English"]["Replace:"] = "Replace:";
    translations["English"]["Replace"] = "Replace";
    translations["English"]["Go"] = "Go";
    translations["English"]["Offset:"] = "Offset:";

    translations["English"]["Update Available!"] = "Update Available!";
    translations["English"]["You're up to date!"] = "You're up to date!";
    translations["English"]["Current:"] = "Current:";
    translations["English"]["Latest:"] = "Latest:";
    translations["English"]["What's New:"] = "What's New:";
    translations["English"]["You have the latest version installed."] = "You have the latest version installed.";
    translations["English"]["Check back later for updates."] = "Check back later for updates.";
    translations["English"]["Update Now"] = "Update Now";
    translations["English"]["Skip"] = "Skip";
    translations["English"]["Close"] = "Close";

    translations["English"]["Version"] = "Version";
    translations["English"]["A modern cross-platform hex editor"] = "A modern cross-platform hex editor";
    translations["English"]["Features:"] = "Features:";
    translations["English"]["Cross-platform support"] = "Cross-platform support";
    translations["English"]["Real-time hex editing"] = "Real-time hex editing";
    translations["English"]["Dark mode support"] = "Dark mode support";
    translations["English"]["Check for Updates"] = "Check for Updates";

    translations["Spanish"]["Options"] = "Opciones";
    translations["Spanish"]["Dark Mode"] = "Modo Oscuro";
    translations["Spanish"]["Auto-reload modified file"] = "Recargar archivo modificado autom�ticamente";
    translations["Spanish"]["Add to context menu (right-click files)"] = "Agregar al men� contextual (clic derecho en archivos)";
    translations["Spanish"]["Language:"] = "Idioma:";
    translations["Spanish"]["Default bytes per line:"] = "Bytes predeterminados por l�nea:";
    translations["Spanish"]["8 bytes"] = "8 bytes";
    translations["Spanish"]["16 bytes"] = "16 bytes";
    translations["Spanish"]["OK"] = "Aceptar";
    translations["Spanish"]["Cancel"] = "Cancelar";

    translations["Spanish"]["File"] = "Archivo";
    translations["Spanish"]["Open"] = "Abrir";
    translations["Spanish"]["Save"] = "Guardar";
    translations["Spanish"]["Exit"] = "Salir";
    translations["Spanish"]["View"] = "Ver";
    translations["Spanish"]["Disassembly"] = "Desensamblado";
    translations["Spanish"]["Search"] = "Buscar";
    translations["Spanish"]["Find and Replace..."] = "Buscar y Reemplazar...";
    translations["Spanish"]["Go To..."] = "Ir a...";
    translations["Spanish"]["Tools"] = "Herramientas";
    translations["Spanish"]["Options..."] = "Opciones...";
    translations["Spanish"]["Help"] = "Ayuda";
    translations["Spanish"]["About HexViewer"] = "Acerca de HexViewer";

    translations["Spanish"]["Find:"] = "Buscar:";
    translations["Spanish"]["Replace:"] = "Reemplazar:";
    translations["Spanish"]["Replace"] = "Reemplazar";
    translations["Spanish"]["Go"] = "Ir";
    translations["Spanish"]["Offset:"] = "Desplazamiento:";

    translations["Spanish"]["Update Available!"] = "�Actualizaci�n Disponible!";
    translations["Spanish"]["You're up to date!"] = "�Est�s actualizado!";
    translations["Spanish"]["Current:"] = "Actual:";
    translations["Spanish"]["Latest:"] = "�ltima:";
    translations["Spanish"]["What's New:"] = "Novedades:";
    translations["Spanish"]["You have the latest version installed."] = "Tienes la �ltima versi�n instalada.";
    translations["Spanish"]["Check back later for updates."] = "Vuelve m�s tarde para buscar actualizaciones.";
    translations["Spanish"]["Update Now"] = "Actualizar Ahora";
    translations["Spanish"]["Skip"] = "Omitir";
    translations["Spanish"]["Close"] = "Cerrar";

    translations["Spanish"]["Version"] = "Versi�n";
    translations["Spanish"]["A modern cross-platform hex editor"] = "Un editor hexadecimal multiplataforma moderno";
    translations["Spanish"]["Features:"] = "Caracter�sticas:";
    translations["Spanish"]["Cross-platform support"] = "Soporte multiplataforma";
    translations["Spanish"]["Real-time hex editing"] = "Edici�n hexadecimal en tiempo real";
    translations["Spanish"]["Dark mode support"] = "Soporte de modo oscuro";
    translations["Spanish"]["Check for Updates"] = "Buscar Actualizaciones";

    translations["French"]["Options"] = "Options";
    translations["French"]["Dark Mode"] = "Mode Sombre";
    translations["French"]["Auto-reload modified file"] = "Recharger automatiquement le fichier modifi�";
    translations["French"]["Add to context menu (right-click files)"] = "Ajouter au menu contextuel (clic droit sur les fichiers)";
    translations["French"]["Language:"] = "Langue:";
    translations["French"]["Default bytes per line:"] = "Octets par ligne par d�faut:";
    translations["French"]["8 bytes"] = "8 octets";
    translations["French"]["16 bytes"] = "16 octets";
    translations["French"]["OK"] = "OK";
    translations["French"]["Cancel"] = "Annuler";

    translations["French"]["File"] = "Fichier";
    translations["French"]["Open"] = "Ouvrir";
    translations["French"]["Save"] = "Enregistrer";
    translations["French"]["Exit"] = "Quitter";
    translations["French"]["View"] = "Affichage";
    translations["French"]["Disassembly"] = "D�sassemblage";
    translations["French"]["Search"] = "Recherche";
    translations["French"]["Find and Replace..."] = "Rechercher et Remplacer...";
    translations["French"]["Go To..."] = "Aller �...";
    translations["French"]["Tools"] = "Outils";
    translations["French"]["Options..."] = "Options...";
    translations["French"]["Help"] = "Aide";
    translations["French"]["About HexViewer"] = "� propos de HexViewer";

    translations["French"]["Find:"] = "Rechercher:";
    translations["French"]["Replace:"] = "Remplacer:";
    translations["French"]["Replace"] = "Remplacer";
    translations["French"]["Go"] = "Aller";
    translations["French"]["Offset:"] = "D�calage:";

    translations["French"]["Update Available!"] = "Mise � jour disponible!";
    translations["French"]["You're up to date!"] = "Vous �tes � jour!";
    translations["French"]["Current:"] = "Actuelle:";
    translations["French"]["Latest:"] = "Derniere:";
    translations["French"]["What's New:"] = "Nouveaut�s:";
    translations["French"]["You have the latest version installed."] = "Vous avez la derni�re version install�e.";
    translations["French"]["Check back later for updates."] = "Revenez plus tard pour les mises � jour.";
    translations["French"]["Update Now"] = "Mettre � jour maintenant";
    translations["French"]["Skip"] = "Ignorer";
    translations["French"]["Close"] = "Fermer";

    translations["French"]["Version"] = "Version";
    translations["French"]["A modern cross-platform hex editor"] = "Un �diteur hexad�cimal multiplateforme moderne";
    translations["French"]["Features:"] = "Fonctionnalit�s:";
    translations["French"]["Cross-platform support"] = "Support multiplateforme";
    translations["French"]["Real-time hex editing"] = "Adition hexadicimale en temps rael";
    translations["French"]["Dark mode support"] = "Support du mode sombre";
    translations["French"]["Check for Updates"] = "Verifier les mises � jour";

    translations["German"]["Options"] = "Optionen";
    translations["German"]["Dark Mode"] = "Dunkler Modus";
    translations["German"]["Auto-reload modified file"] = "Ge�nderte Datei automatisch neu laden";
    translations["German"]["Add to context menu (right-click files)"] = "Zum Kontextmen� hinzuf�gen (Rechtsklick auf Dateien)";
    translations["German"]["Language:"] = "Sprache:";
    translations["German"]["Default bytes per line:"] = "Standard-Bytes pro Zeile:";
    translations["German"]["8 bytes"] = "8 Bytes";
    translations["German"]["16 bytes"] = "16 Bytes";
    translations["German"]["OK"] = "OK";
    translations["German"]["Cancel"] = "Abbrechen";

    translations["German"]["File"] = "Datei";
    translations["German"]["Open"] = "Öffnen";
    translations["German"]["Save"] = "Speichern";
    translations["German"]["Exit"] = "Beenden";
    translations["German"]["View"] = "Ansicht";
    translations["German"]["Disassembly"] = "Disassemblierung";
    translations["German"]["Search"] = "Suchen";
    translations["German"]["Find and Replace..."] = "Suchen und Ersetzen...";
    translations["German"]["Go To..."] = "Gehe zu...";
    translations["German"]["Tools"] = "Werkzeuge";
    translations["German"]["Options..."] = "Optionen...";
    translations["German"]["Help"] = "Hilfe";
    translations["German"]["About HexViewer"] = "Über HexViewer";

    translations["German"]["Find:"] = "Suchen:";
    translations["German"]["Replace:"] = "Ersetzen:";
    translations["German"]["Replace"] = "Ersetzen";
    translations["German"]["Go"] = "Gehe";
    translations["German"]["Offset:"] = "Offset:";

    translations["German"]["Update Available!"] = "Update verfügbar!";
    translations["German"]["You're up to date!"] = "Sie sind auf dem neuesten Stand!";
    translations["German"]["Current:"] = "Aktuell:";
    translations["German"]["Latest:"] = "Neueste:";
    translations["German"]["What's New:"] = "Was ist neu:";
    translations["German"]["You have the latest version installed."] = "Sie haben die neueste Version installiert.";
    translations["German"]["Check back later for updates."] = "Schauen Sie später nach Updates.";
    translations["German"]["Update Now"] = "Jetzt aktualisieren";
    translations["German"]["Skip"] = "Überspringen";
    translations["German"]["Close"] = "Schließen";

    translations["German"]["Version"] = "Version";
    translations["German"]["A modern cross-platform hex editor"] = "Ein moderner plattform�bergreifender Hex-Editor";
    translations["German"]["Features:"] = "Funktionen:";
    translations["German"]["Cross-platform support"] = "Plattformübergreifende Unterstützung";
    translations["German"]["Real-time hex editing"] = "Echtzeit-Hex-Bearbeitung";
    translations["German"]["Dark mode support"] = "Dunkler Modus Unterstützung";
    translations["German"]["Check for Updates"] = "Nach Updates suchen";

    translations["Japanese"]["Options"] = "?????";
    translations["Japanese"]["Dark Mode"] = "??????";
    translations["Japanese"]["Auto-reload modified file"] = "?????????????????";
    translations["Japanese"]["Add to context menu (right-click files)"] = "?????????????(??????????)";
    translations["Japanese"]["Language:"] = "??:";
    translations["Japanese"]["Default bytes per line:"] = "1??????????????:";
    translations["Japanese"]["8 bytes"] = "8???";
    translations["Japanese"]["16 bytes"] = "16???";
    translations["Japanese"]["OK"] = "OK";
    translations["Japanese"]["Cancel"] = "?????";

    translations["Japanese"]["File"] = "????";
    translations["Japanese"]["Open"] = "??";
    translations["Japanese"]["Save"] = "??";
    translations["Japanese"]["Exit"] = "??";
    translations["Japanese"]["View"] = "??";
    translations["Japanese"]["Disassembly"] = "??????";
    translations["Japanese"]["Search"] = "??";
    translations["Japanese"]["Find and Replace..."] = "?????...";
    translations["Japanese"]["Go To..."] = "??...";
    translations["Japanese"]["Tools"] = "???";
    translations["Japanese"]["Options..."] = "?????...";
    translations["Japanese"]["Help"] = "???";
    translations["Japanese"]["About HexViewer"] = "HexViewer????";

    translations["Japanese"]["Find:"] = "??:";
    translations["Japanese"]["Replace:"] = "??:";
    translations["Japanese"]["Replace"] = "??";
    translations["Japanese"]["Go"] = "??";
    translations["Japanese"]["Offset:"] = "?????:";

    translations["Japanese"]["Update Available!"] = "??????????!";
    translations["Japanese"]["You're up to date!"] = "????!";
    translations["Japanese"]["Current:"] = "??:";
    translations["Japanese"]["Latest:"] = "??:";
    translations["Japanese"]["What's New:"] = "???:";
    translations["Japanese"]["You have the latest version installed."] = "?????????????????????";
    translations["Japanese"]["Check back later for updates."] = "??????????????????";
    translations["Japanese"]["Update Now"] = "?????";
    translations["Japanese"]["Skip"] = "????";
    translations["Japanese"]["Close"] = "???";

    translations["Japanese"]["Version"] = "?????";
    translations["Japanese"]["A modern cross-platform hex editor"] = "????????????????16?????";
    translations["Japanese"]["Features:"] = "??:";
    translations["Japanese"]["Cross-platform support"] = "?????????????";
    translations["Japanese"]["Real-time hex editing"] = "??????16???";
    translations["Japanese"]["Dark mode support"] = "????????";
    translations["Japanese"]["Check for Updates"] = "?????????";

    translations["Russian"]["Options"] = "??";
    translations["Russian"]["Dark Mode"] = "????";
    translations["Russian"]["Auto-reload modified file"] = "???????????";
    translations["Russian"]["Add to context menu (right-click files)"] = "????????(??????)";
    translations["Russian"]["Language:"] = "??:";
    translations["Russian"]["Default bytes per line:"] = "???????:";
    translations["Russian"]["8 bytes"] = "8??";
    translations["Russian"]["16 bytes"] = "16??";
    translations["Russian"]["OK"] = "??";
    translations["Russian"]["Cancel"] = "??";

    translations["Russian"]["File"] = "??";
    translations["Russian"]["Open"] = "??";
    translations["Russian"]["Save"] = "??";
    translations["Russian"]["Exit"] = "??";
    translations["Russian"]["View"] = "??";
    translations["Russian"]["Disassembly"] = "???";
    translations["Russian"]["Search"] = "??";
    translations["Russian"]["Find and Replace..."] = "?????...";
    translations["Russian"]["Go To..."] = "??...";
    translations["Russian"]["Tools"] = "??";
    translations["Russian"]["Options..."] = "??...";
    translations["Russian"]["Help"] = "??";
    translations["Russian"]["About HexViewer"] = "??HexViewer";

    translations["Russian"]["Find:"] = "??:";
    translations["Russian"]["Replace:"] = "??:";
    translations["Russian"]["Replace"] = "??";
    translations["Russian"]["Go"] = "??";
    translations["Russian"]["Offset:"] = "???:";

    translations["Russian"]["Update Available!"] = "?????!";
    translations["Russian"]["You're up to date!"] = "???????!";
    translations["Russian"]["Current:"] = "??:";
    translations["Russian"]["Latest:"] = "??:";
    translations["Russian"]["What's New:"] = "???:";
    translations["Russian"]["You have the latest version installed."] = "?????????";
    translations["Russian"]["Check back later for updates."] = "????????";
    translations["Russian"]["Update Now"] = "????";
    translations["Russian"]["Skip"] = "??";
    translations["Russian"]["Close"] = "??";

    translations["Russian"]["Version"] = "??";
    translations["Russian"]["A modern cross-platform hex editor"] = "????????????";
    translations["Russian"]["Features:"] = "??:";
    translations["Russian"]["Cross-platform support"] = "?????";
    translations["Russian"]["Real-time hex editing"] = "????????";
    translations["Russian"]["Dark mode support"] = "??????";
    translations["Russian"]["Check for Updates"] = "????";

    translations["Chinese"]["Options"] = "??";
    translations["Chinese"]["Dark Mode"] = "????";
    translations["Chinese"]["Auto-reload modified file"] = "???????????";
    translations["Chinese"]["Add to context menu (right-click files)"] = "????????(??????)";
    translations["Chinese"]["Language:"] = "??:";
    translations["Chinese"]["Default bytes per line:"] = "???????:";
    translations["Chinese"]["8 bytes"] = "8??";
    translations["Chinese"]["16 bytes"] = "16??";
    translations["Chinese"]["OK"] = "??";
    translations["Chinese"]["Cancel"] = "??";

    translations["Chinese"]["File"] = "??";
    translations["Chinese"]["Open"] = "??";
    translations["Chinese"]["Save"] = "??";
    translations["Chinese"]["Exit"] = "??";
    translations["Chinese"]["View"] = "??";
    translations["Chinese"]["Disassembly"] = "???";
    translations["Chinese"]["Search"] = "??";
    translations["Chinese"]["Find and Replace..."] = "?????...";
    translations["Chinese"]["Go To..."] = "??...";
    translations["Chinese"]["Tools"] = "??";
    translations["Chinese"]["Options..."] = "??...";
    translations["Chinese"]["Help"] = "??";
    translations["Chinese"]["About HexViewer"] = "??HexViewer";

    translations["Chinese"]["Find:"] = "??:";
    translations["Chinese"]["Replace:"] = "??:";
    translations["Chinese"]["Replace"] = "??";
    translations["Chinese"]["Go"] = "??";
    translations["Chinese"]["Offset:"] = "???:";

    translations["Chinese"]["Update Available!"] = "?????!";
    translations["Chinese"]["You're up to date!"] = "???????!";
    translations["Chinese"]["Current:"] = "??:";
    translations["Chinese"]["Latest:"] = "??:";
    translations["Chinese"]["What's New:"] = "???:";
    translations["Chinese"]["You have the latest version installed."] = "?????????";
    translations["Chinese"]["Check back later for updates."] = "????????";
    translations["Chinese"]["Update Now"] = "????";
    translations["Chinese"]["Skip"] = "??";
    translations["Chinese"]["Close"] = "??";

    translations["Chinese"]["Version"] = "??";
    translations["Chinese"]["A modern cross-platform hex editor"] = "????????????";
    translations["Chinese"]["Features:"] = "??:";
    translations["Chinese"]["Cross-platform support"] = "?????";
    translations["Chinese"]["Real-time hex editing"] = "????????";
    translations["Chinese"]["Dark mode support"] = "??????";
    translations["Chinese"]["Check for Updates"] = "????";

    defaultLanguage = "English";
  }

  static void SetLanguage(const std::string& language) {
    if (translations.find(language) != translations.end()) {
      defaultLanguage = language;
    }
  }

  static std::string Get(const std::string& key) {
    auto langIt = translations.find(defaultLanguage);
    if (langIt != translations.end()) {
      auto textIt = langIt->second.find(key);
      if (textIt != langIt->second.end()) {
        return textIt->second;
      }
    }
    auto englishIt = translations.find("English");
    if (englishIt != translations.end()) {
      auto textIt = englishIt->second.find(key);
      if (textIt != englishIt->second.end()) {
        return textIt->second;
      }
    }
    return key;
  }

  static std::string T(const std::string& key) {
    return Get(key);
  }
};
