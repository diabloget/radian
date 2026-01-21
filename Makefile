APP_NAME = radian
FLTK_CONFIG = /usr/local/bin/fltk-config
CXX = g++
APPIMAGETOOL = ./appimagetool-x86_64.AppImage
APPDIR = Radian.AppDir

CXXFLAGS = $(shell $(FLTK_CONFIG) --cxxflags) \
           $(shell pkg-config --cflags libevdev) \
           -O2 -Wall

LDFLAGS = $(shell $(FLTK_CONFIG) --ldstaticflags) \
          $(shell pkg-config --libs libevdev wayland-client wayland-cursor wayland-egl xkbcommon) \
          -lpthread -ldl

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
	@echo "Compiling $(APP_NAME)..."
	$(CXX) main.cpp -o $(APP_NAME) $(CXXFLAGS) $(LDFLAGS)
	strip $(APP_NAME)
	@echo "Build complete: $(APP_NAME)"

appimage: build
	@echo "Creating AppDir structure..."
	mkdir -p $(APPDIR)/usr/{bin,lib,share/icons/hicolor/256x256/apps,lib/libdecor/plugins-1}
	
	install -m 755 $(APP_NAME) $(APPDIR)/usr/bin/$(APP_NAME)
	
	@echo "Bundling runtime dependencies..."
	@ldd $(APP_NAME) | grep "libstdc++\.so" | awk '{print $$3}' | xargs -I {} cp -L {} $(APPDIR)/usr/lib/ 2>/dev/null || true
	@ldd $(APP_NAME) | grep "libgcc_s\.so" | awk '{print $$3}' | xargs -I {} cp -L {} $(APPDIR)/usr/lib/ 2>/dev/null || true
	@ldd $(APP_NAME) | grep "libevdev\.so" | awk '{print $$3}' | xargs -I {} cp -L {} $(APPDIR)/usr/lib/ 2>/dev/null || true
	
	@cp -L /usr/lib64/libdecor-0.so* $(APPDIR)/usr/lib/ 2>/dev/null || \
	 cp -L /usr/lib/x86_64-linux-gnu/libdecor-0.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	
	@cp -L /usr/lib64/libdecor/plugins-1/*.so $(APPDIR)/usr/lib/libdecor/plugins-1/ 2>/dev/null || \
	 cp -L /usr/lib/x86_64-linux-gnu/libdecor/plugins-1/*.so $(APPDIR)/usr/lib/libdecor/plugins-1/ 2>/dev/null || true
	
	@echo "Installing assets..."
	install -m 644 icon.png $(APPDIR)/radian.png
	install -m 644 icon.png $(APPDIR)/usr/share/icons/hicolor/256x256/apps/radian.png
	install -m 755 AppRun.sh $(APPDIR)/AppRun
	echo "$$DESKTOP_FILE" > $(APPDIR)/radian.desktop
	
	@echo "Generating AppImage..."
	chmod +x $(APPIMAGETOOL)
	ARCH=x86_64 $(APPIMAGETOOL) $(APPDIR) $(APP_NAME)-x86_64.AppImage
	
	@echo "Build complete: $(APP_NAME)-x86_64.AppImage"
	@du -h $(APP_NAME)-x86_64.AppImage

clean:
	rm -f $(APP_NAME)
	rm -rf $(APPDIR)
	rm -f $(APP_NAME)-x86_64.AppImage

.PHONY: all build appimage clean