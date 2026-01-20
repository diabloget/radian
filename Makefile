# Radian AppImage Build System
APP_NAME = radian
CXX = g++
CXXFLAGS = $(shell fltk-config --cxxflags) $(shell pkg-config --cflags libevdev) -O2 -Wall
LDFLAGS = $(shell fltk-config --ldflags) $(shell pkg-config --libs libevdev)
APPDIR = Radian.AppDir

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
	@echo ">> Compiling binary..."
	$(CXX) main.cpp -o $(APP_NAME) $(CXXFLAGS) $(LDFLAGS)
	strip $(APP_NAME)

appimage: build
	@echo ">> Preparing AppDir structure..."
	mkdir -p $(APPDIR)/usr/bin
	mkdir -p $(APPDIR)/usr/lib
	mkdir -p $(APPDIR)/usr/share/icons/hicolor/256x256/apps
	
	install -m 755 $(APP_NAME) $(APPDIR)/usr/bin/$(APP_NAME)
	
	@echo ">> Copying shared libraries..."
	# Copy all dependencies from ldd output
	ldd $(APP_NAME) | grep "=> /" | awk '{print $$3}' | while read lib; do \
		if [ -f "$$lib" ]; then \
			cp -v "$$lib" $(APPDIR)/usr/lib/ 2>/dev/null || true; \
		fi; \
	done
	
	# Ensure critical libraries are included
	cp -v /usr/lib*/libfltk*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -v /usr/lib/x86_64-linux-gnu/libfltk*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -v /usr/lib*/libevdev*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -v /usr/lib/x86_64-linux-gnu/libevdev*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -v /usr/lib*/libX*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -v /usr/lib/x86_64-linux-gnu/libX*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -v /usr/lib*/libfontconfig*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -v /usr/lib*/libfreetype*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -v /usr/lib/x86_64-linux-gnu/libfontconfig*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	cp -v /usr/lib/x86_64-linux-gnu/libfreetype*.so* $(APPDIR)/usr/lib/ 2>/dev/null || true
	
	@echo ">> Installing icon..."
	install -m 644 icon.png $(APPDIR)/radian.png
	install -m 644 icon.png $(APPDIR)/usr/share/icons/hicolor/256x256/apps/radian.png
	
	@echo ">> Installing desktop file..."
	@echo "$$DESKTOP_FILE" > $(APPDIR)/radian.desktop
	chmod 644 $(APPDIR)/radian.desktop
	
	install -m 755 AppRun.sh $(APPDIR)/AppRun
	
	@echo ">> Packaging AppImage..."
	ARCH=x86_64 ./appimagetool-x86_64.AppImage $(APPDIR) Radian-x86_64.AppImage
	chmod +x Radian-x86_64.AppImage
	
	@echo ">> Verifying dependencies..."
	@./Radian-x86_64.AppImage --appimage-extract >/dev/null 2>&1
	@ldd squashfs-root/usr/bin/radian | grep "not found" && echo "WARNING: Missing dependencies!" || echo "All dependencies OK!"
	@rm -rf squashfs-root

clean:
	rm -f $(APP_NAME)
	rm -rf $(APPDIR)
	rm -f Radian-x86_64.AppImage

.PHONY: all build appimage clean