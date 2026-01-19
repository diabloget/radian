# Radian

Radian is a precision tool for Linux designed to synchronize 360-degree rotation distance across different games. By directly interfacing with the kernel's input subsystem using `uinput` and `libevdev`, Radian ensures deterministic and consistent horizontal movement, allowing users to calibrate their sensitivity precisely for each specific game.

![Project Showcase](./docs/showcase.png)

## Usage
1. Download the .AppImage file in the Release section.
2. Executing it will ask you for your sudo password. Radian needs this in order to create a virtual device.
3. You can change any keybind within the menu tab in the App. Useful if the default ones are already in use by the game.
4. If the game allows for it, go into a local match or training session, set the center of the screen or game crosshair on top of an object as reference.
5. By default number 8 will attempt a 360, 9 decreases, 0 increases. Both 9 and 0 move by 50, while the following - and + keys allows for units (- or + 1).
6. Once you nailed the 360 in your first game, you can launch a different game and use the current 360 value by pressing 8 and adjust the sensitivity multiplier of this game to match the 360 as close as possible.
7. You can change the Profile Name and click the save profile button for future uses. All the settings and profiles shoudl be saved in a radian.cfg file.
Note: The radian.cfg file will store the keybinds in the first line separated by space, but any line after that represents a saved profile in a classic CSV format 'Profile Name','Raw Counts Value'.

## Getting Started

These instructions will help you get a copy of the project up and running on your local machine for development and testing purposes.

### Prerequisites

To compile and run Radian, you need a Linux environment (tested on Solus and KDE/GNOME-based environments) and the following development libraries installed:

* C++ Compiler (GCC/G++)
* FLTK (Fast Light Toolkit)
* libevdev
* Packaging tools (optional for AppImage): appimagetool and ImageMagick

On Solus-based systems, you can install the necessary dependencies with the following command:

```bash
sudo eopkg install fltk-devel libevdev-devel imagemagick
```

### Installing

Follow these steps to set up your development environment:

1. Clone the repository to your local machine:

```bash
git clone https://github.com/diabloget/radian.git
cd radian
```

2. Compile the binary using the provided Makefile:

```bash
make build
```

3. Verify that the binary has been generated correctly:

```bash
ls -l radian
```

To use the program, run the binary with superuser permissions to allow access to the kernel's `uinput` module:

```bash
sudo ./radian
```

### Checking proper creation of virtual device

You should validate that the virtual device is recognized by the operating system and that profiles are saved correctly to disk.

```bash
# Verify the creation of the virtual device in kernel logs
dmesg | grep "Radian-Input"
```
Note that you can manually go into your DE's settings and look for a new "mouse" named 'Radian-Input-Device' or similar. Dissabling mouse acceleration here could be useful, but since acceleration is deterministic, it should stay the same across games which makes it irrelevant for the main purpose of the App.

## Deployment

Radian uses the AppImage format. This allows the application to run on various distributions without worrying about local dependencies.

To generate the distributable package, make sure you have `appimagetool` in the project root and run:

```bash
make appimage
```

The result will be a `Radian-x86_64.AppImage` file ready to be shared.

## Built With

* [FLTK](https://www.fltk.org/) - The graphical user interface framework
* [libevdev](https://www.freedesktop.org/wiki/Software/libevdev/) - Interface for kernel event handling
* [uinput](https://kernel.org/doc/html/latest/input/uinput.html) - Kernel module for creating virtual input devices
* [Canva](https://www.canva.com/) - Tool used for creating the application icon

## Contributing

This is a personal proyect, but feel free to fork the project or submit an issue. You could also add a possible solution, in case I can validate that it does fix the mentioned issue I will give you credit for it both within the code and in this README.md file. 

## Special Thanks to

Anyone who points critical errors or suggests relevant fixes will be shocased here :)

## Versioning

I am using [SemVer](http://semver.org/) for versioning. For available versions, see the [tags on this repository](https://github.com/diabloget/radian/tags).

## Authors

* **Diabloget** - *Initial Work* - [diabloget](https://github.com/diabloget)

## License

This project is licensed under the GNU GPLv3 License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

* Artificial intelligence (Gemini, Claude...) was used for reviewing and optimizing linear movement algorithms, as well as for technical correction of automation scripts in the Makefile and AppRun.sh structure
* Inspired by Kovaak's Sensitivity Matcher
* Application icon designed using Canva
