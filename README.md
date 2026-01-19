# Radian

Radian is a precision tool for Linux designed to synchronize 360-degree rotation distance across different game engines. By directly interfacing with the kernel's input subsystem using `uinput` and `libevdev`, Radian ensures deterministic and consistent horizontal movement, allowing users to calibrate their sensitivity precisely for each specific game.

## Getting Started

These instructions will help you get a copy of the project up and running on your local machine for development and testing purposes. See the deployment section for notes on how to run the system professionally.

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

## Running the tests

Currently, system tests are performed manually to verify the integrity of communication with virtual hardware and the persistence of configuration files.

### Break down into end to end tests

You should validate that the virtual device is recognized by the operating system and that profiles are saved correctly to disk.

```bash
# Verify the creation of the virtual device in kernel logs
dmesg | grep "Radian-Input"
```

### And coding style tests

To maintain code consistency, it is recommended to follow modern C++ formatting standards.

```bash
# Example of configuration file integrity verification
cat radian.cfg
```

## Deployment

For deployment on production systems or for universal distribution, Radian uses the AppImage format. This allows the application to run on various distributions without worrying about local dependencies.

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

## Versioning

We use [SemVer](http://semver.org/) for versioning. For available versions, see the [tags on this repository](https://github.com/diabloget/radian/tags).

## Authors

* **Diabloget** - *Initial Work* - [diabloget](https://github.com/diabloget)

## License

This project is licensed under the GNU GPLv3 License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

* Artificial intelligence assistance (Gemini and Claude) was used for reviewing and optimizing linear movement algorithms, as well as for technical correction of automation scripts in the Makefile and AppRun.sh structure
* Inspired by Kovaak's Sensitivity Matcher
* Application icon designed with Canva
