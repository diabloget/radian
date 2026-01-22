
# Radian makefile :)
APP_NAME = radian
APP_ID = io.github.diabloget.radian

# FLTK Config
FLTK_CONFIG = /usr/local/bin/fltk-config
CXX = g++

APPIMAGETOOL = ./appimagetool-x86_64.AppImage
APPDIR = Radian.AppDir

CXXFLAGS = $(shell $(FLTK_CONFIG) --cxxflags) \
           $(shell pkg-config --cflags libevdev) \
           -O2 -Wall

LDFLAGS = -L/usr/local/lib \
          $(shell $(FLTK_CONFIG) --ldflags) \
          $(shell pkg-config --libs libevdev wayland-client wayland-cursor wayland-egl xkbcommon) \
          -lpthread -ldl

# Desktop file
define DESKTOP_FILE
[Desktop Entry]
Type=Application
Name=Radian
Comment=Sensitivity Matcher
Exec=$(APP_NAME)
Icon=$(APP_NAME)
Categories=Utility;
Terminal=false
StartupWMClass=$(APP_NAME)
endef
export DESKTOP_FILE


all: clean build appimage

build:
	@echo "Compilando $(APP_NAME) con FLTK personalizado..."
	$(CXX) main.cpp -o $(APP_NAME) $(CXXFLAGS) $(LDFLAGS)
	strip $(APP_NAME)

appimage: build
	@echo "Creando estructura AppDir para AppImage..."
	rm -rf $(APPDIR)
	mkdir -p $(APPDIR)/usr/{bin,lib,share/icons/hicolor/256x256/apps,lib/libdecor/plugins-1,share/metainfo,share/applications}
	
	install -m 755 $(APP_NAME) $(APPDIR)/usr/bin/$(APP_NAME)
	
	@echo "Empaquetando dependencias..."
	@ldd $(APP_NAME) | grep "=> /" | awk '{print $$3}' | xargs -I {} cp -L {} $(APPDIR)/usr/lib/ 2>/dev/null || true
	
	# Soporte Wayland (Específico para Solus/Distros modernas)
	@cp -L /usr/lib64/libdecor-0.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	@cp -L /usr/lib64/libdecor/plugins-1/*.so $(APPDIR)/usr/lib/libdecor/plugins-1/ 2>/dev/null || true
	
	@echo "Instalando assets..."
	install -m 644 icon.png $(APPDIR)/$(APP_NAME).png
	install -m 644 icon.png $(APPDIR)/usr/share/icons/hicolor/256x256/apps/$(APP_NAME).png
	install -m 644 $(APP_ID).metainfo.xml $(APPDIR)/usr/share/metainfo/$(APP_ID).metainfo.xml
	
	@echo "Configurando lanzamiento..."
	echo "$$DESKTOP_FILE" > $(APPDIR)/$(APP_ID).desktop
	cp $(APPDIR)/$(APP_ID).desktop $(APPDIR)/usr/share/applications/$(APP_ID).desktop
	install -m 755 AppRun.sh $(APPDIR)/AppRun
	
	@echo "Generando AppImage final..."
	chmod +x $(APPIMAGETOOL)
	ARCH=x86_64 $(APPIMAGETOOL) $(APPDIR) $(APP_NAME)-x86_64.AppImage

# This is for local testing of flatpak

flatpak-build:
	flatpak-builder --force-clean --repo=repo build-dir $(APP_ID).yml

flatpak-install:
	flatpak --user remote-add --if-not-exists --no-gpg-verify radian-repo repo
	flatpak --user install radian

flatpak-run:
	flatpak run $(APP_ID)

# Clean section :P
clean:
	@echo "Limpiando archivos de construcción..."
	# Limpieza de binarios y AppImage
	rm -f $(APP_NAME)
	rm -f $(APP_NAME)-x86_64.AppImage
	rm -rf $(APPDIR)
	
	# Limpieza de Flatpak (Carpetas pesadas)
	rm -rf build-dir
	rm -rf repo
	rm -rf .flatpak-builder
	
	@echo "Limpieza completada."

.PHONY: all build appimage clean flatpak-build flatpak-install flatpak-run