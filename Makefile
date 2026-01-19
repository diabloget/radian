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
	$(CXX) main.cpp -o $(APP_NAME) $(CXXFLAGS) $(LDFLAGS)
	strip $(APP_NAME)

appimage: build
	mkdir -p $(APPDIR)/usr/bin
	mkdir -p $(APPDIR)/usr/share/icons/hicolor/256x256/apps
	mkdir -p $(APPDIR)/usr/share/metainfo
	install -m 755 $(APP_NAME) $(APPDIR)/usr/bin/$(APP_NAME)
	install -m 644 icon.png $(APPDIR)/radian.png
	install -m 644 icon.png $(APPDIR)/usr/share/icons/hicolor/256x256/apps/radian.png
	@echo "$$DESKTOP_FILE" > $(APPDIR)/radian.desktop
	chmod 644 $(APPDIR)/radian.desktop
	@echo '<?xml version="1.0" encoding="UTF-8"?><component type="desktop"><id>io.github.diabloget.radian</id><metadata_license>CC0-1.0</metadata_license><project_license>GPL-3.0</project_license><name>Radian</name><summary>Mouse Sensitivity Matcher</summary><description><p>Radian is a lightweight utility designed for Linux users to match their mouse sensitivity across different games. It provides precise control over raw counts and allows saving multiple sensitivity profiles for quick switching.</p></description><launchable type="desktop-id">radian.desktop</launchable><url type="homepage">https://github.com/diabloget/radian</url><developer_name>Diabloget</developer_name><content_rating type="oars-1.1" /></component>' > $(APPDIR)/usr/share/metainfo/io.github.diabloget.radian.appdata.xml
	install -m 755 AppRun.sh $(APPDIR)/AppRun
	ARCH=x86_64 ./appimagetool-x86_64.AppImage $(APPDIR) Radian-x86_64.AppImage
	chmod +x Radian-x86_64.AppImage

clean:
	rm -f $(APP_NAME)
	rm -rf $(APPDIR)
	rm -f Radian-x86_64.AppImage

.PHONY: all build appimage clean