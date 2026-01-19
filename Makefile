# Configuración de Radian
APP_NAME = radian
CXX = g++
CXXFLAGS = $(shell fltk-config --cxxflags) $(shell pkg-config --cflags libevdev) -O2 -Wall
LDFLAGS = $(shell fltk-config --ldflags) $(shell pkg-config --libs libevdev)
APPDIR = Radian.AppDir

# --- ARCHIVO DESKTOP ---
define DESKTOP_FILE
[Desktop Entry]
Type=Application
Name=Radian
Comment=Sensitivity Matcher
Exec=radian
Icon=radian
Categories=Utility;
Terminal=false
StartupWMClass=radian
endef
export DESKTOP_FILE

all: clean build appimage

build:
	@echo ">> Compilando binario..."
	$(CXX) main.cpp -o $(APP_NAME) $(CXXFLAGS) $(LDFLAGS)
	strip $(APP_NAME)

appimage: build
	@echo ">> Preparando estructura AppDir..."
	mkdir -p $(APPDIR)/usr/bin
	mkdir -p $(APPDIR)/usr/share/icons/hicolor/256x256/apps
	
	# Instalar binario con permisos correctos
	install -m 755 $(APP_NAME) $(APPDIR)/usr/bin/$(APP_NAME)
	
	# Ícono
	if [ -f icon.png ]; then \
		install -m 644 icon.png $(APPDIR)/radian.png; \
		install -m 644 icon.png $(APPDIR)/usr/share/icons/hicolor/256x256/apps/radian.png; \
	else \
		magick -size 256x256 xc:transparent -fill "#007acc" -draw "circle 128,128 128,5" $(APPDIR)/radian.png; \
		install -m 644 $(APPDIR)/radian.png $(APPDIR)/usr/share/icons/hicolor/256x256/apps/radian.png; \
	fi
	
	# Desktop File
	@echo "$$DESKTOP_FILE" > $(APPDIR)/radian.desktop
	chmod 644 $(APPDIR)/radian.desktop
	
	# AppRun Script (copiar desde archivo)
	install -m 755 AppRun.sh $(APPDIR)/AppRun
	
	@echo ">> Empaquetando AppImage..."
	ARCH=x86_64 ./appimagetool-x86_64.AppImage $(APPDIR) Radian-x86_64.AppImage
	chmod +x Radian-x86_64.AppImage

clean:
	rm -f $(APP_NAME)
	rm -rf $(APPDIR)
	rm -f Radian-x86_64.AppImage

.PHONY: all build appimage clean