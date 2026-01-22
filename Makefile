APP_NAME = radian
APP_ID = io.github.diabloget.radian

FLTK_CONFIG ?= fltk-config
CXX ?= g++

APPIMAGETOOL = ./appimagetool-x86_64.AppImage
APPDIR = Radian.AppDir

CXXFLAGS = $(shell $(FLTK_CONFIG) --cxxflags) \
           $(shell pkg-config --cflags libevdev) \
           -O2 -Wall -Wextra

LDFLAGS = $(shell $(FLTK_CONFIG) --ldstaticflags) \
          $(shell pkg-config --libs libevdev wayland-client wayland-cursor wayland-egl xkbcommon) \
          -lpthread -ldl

all: build

build:
	$(CXX) main.cpp -o $(APP_NAME) $(CXXFLAGS) $(LDFLAGS)
	strip $(APP_NAME)

appimage: build
	mkdir -p $(APPDIR)/usr/{bin,lib/libdecor/plugins-1,share/{metainfo,applications,icons/hicolor/256x256/apps}}
	
	install -m 755 $(APP_NAME) $(APPDIR)/usr/bin/$(APP_NAME)
	
	@ldd $(APP_NAME) | grep -E "libstdc\+\+|libgcc_s|libevdev" | awk '{print $$3}' | xargs -I {} cp -L {} $(APPDIR)/usr/lib/ 2>/dev/null || true
	
	@cp -L /usr/lib{64,/x86_64-linux-gnu}/libdecor-0.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	@cp -L /usr/lib{64,/x86_64-linux-gnu}/libdecor/plugins-1/*.so $(APPDIR)/usr/lib/libdecor/plugins-1/ 2>/dev/null || true
	
	install -m 644 icon.png $(APPDIR)/radian.png
	install -m 644 icon.png $(APPDIR)/usr/share/icons/hicolor/256x256/apps/$(APP_ID).png
	install -m 644 $(APP_ID).metainfo.xml $(APPDIR)/usr/share/metainfo/$(APP_ID).metainfo.xml
	install -m 644 $(APP_ID).desktop $(APPDIR)/usr/share/applications/$(APP_ID).desktop
	install -m 644 $(APP_ID).desktop $(APPDIR)/$(APP_ID).desktop
	install -m 755 AppRun.sh $(APPDIR)/AppRun
	
	chmod +x $(APPIMAGETOOL)
	ARCH=x86_64 $(APPIMAGETOOL) $(APPDIR) $(APP_NAME)-x86_64.AppImage

flatpak-build:
	flatpak-builder --force-clean --repo=repo build-dir $(APP_ID).yml

flatpak-install: flatpak-build
	flatpak --user remote-add --if-not-exists --no-gpg-verify radian-repo repo
	flatpak --user install radian-repo $(APP_ID) -y

flatpak-run:
	flatpak run $(APP_ID)

clean:
	rm -f $(APP_NAME)
	rm -rf $(APPDIR) $(APP_NAME)-x86_64.AppImage
	rm -rf build-dir repo .flatpak-builder

.PHONY: all build appimage flatpak-build flatpak-install flatpak-run clean