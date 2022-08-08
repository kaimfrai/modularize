# Modularize

## Überblick
Diese Projekt erstellt das Programm modularize. Sein primäres Ziel ist dabei zu helfen, ein C++ Projekt mit C++20 Modulen zu modernisieren. Es stehen 3 Optionen zur Auswahl:

```--find-unsued```

Findet Dateien im Projekt, die nicht benötigt werden. Es ist möglich, eine Liste von explizit benötigten Dateien anzulegen. Hierfür wird standardmäßig eine Datei ExplicitUse.fuf geladen.

```--analyse```

Gruppiert Dateien in Module entsprechend zyklischer Abhängigkeiten. Geht von einer Datei aus und unterteil Dateien in verschiedene Kategorien, entsprechend ihrer Relation zur ersten Datei.

```--modularize```

Konvertiert alle Dateien eines Projektes anhand von #include Anweisungen. Konvertierte Dateien benötigen gegebenenfalls manuelle Anpassungen.

## Abhängigkeiten

- cmake >3.20
- clang =15
- ninja-build

## Erstellen

Für das Erstellen des Projektes folgende Befehle im Projektordner ausführen: 

```
> mkdir build
> cmake -S ./ -B ./build  --toolchain=CMake/toolchain/clang.cmake -G="Ninja"
> cd build
> ninja -d keepdepfile
``` 

## Voraussetzung
Abhängigkeits-Dateien müssen im Zielprojekt existieren. Mit Ninja kann dies mittels
```
> ninja -d keepdepfile
```
erreicht werden.

## Anwendung
Im Ordner build nach Erstellen des Programmes einen folgender Befehle Ausführen:
```
./modularize --find-unused "Pfad/zum/Quellcode" "relativer/Pfad/zu/BinärDateien" "relativer/Pfad/zu/expliziten/Dateien (optional)"
./modularize --analyze "Pfad/zum/Quellcode" "relativer/Pfad/zu/BinärDateien" "relativer/Pfad/zu/einem/Header"
./modularize --convert "Pfad/zum/Quellcode" "relativer/Pfad/zu/BinärDateien" "Gewünschter.Wurzel.Modul.Name"
```
