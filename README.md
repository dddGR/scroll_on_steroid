# Scroll On Steroid

## Overview

This project is a scroll device that emulates mouse scrolling with enhanced smoothness, utilizing a resolution multiplier. It connects via Bluetooth Low Energy (BLE) and incorporates a touch sensor for user interaction. Users can easily adjust scrolling speed and direction (horizontal/vertical) through a touch button interface. The device is built on the ESP32-C3/S3 microcontroller and an AS5600 magnetic encoder. This project is developed using the ESP-IDF framework and is entirely written in C.

## Table of Contents

- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Build guide](#build-guide)
- [Installation](#installation)
- [Usage](#usage)
- [Contributing](#contributing)

## Features

- **Smooth Scrolling**: Utilizes a resolution multiplier for enhanced scrolling experience.

- **Bluetooth BLE Connectivity**: Wireless connection. (Only work on windows for now)

- **Touch Sensor**: Detects user interactions to eliminate false movement.  
User can change scrolling speed or direction via touch interface.

- **Long Battery Life**: Built-in 2000mAh battery for extended use.

- **Easy Assembly**: All components are off-the-shelf parts for straightforward assembly.

- ***Currently, this device only supports Windows.***

## Hardware Requirements

- ESP32-S3/C3 micro sized board
- Magnetic encoder: AS5600
- Touch sensor module: TP223 *(for C3 only)*
- 6809-2RS bearing
- 10345 lipo battery
- LX-LBC3 lipo charging module
- SW-200D vibration switch *(for C3 only)*
- MSK12C02 switch *(*optinal)*
- Other components (screw, wire...)

## Build guide

The complete build guide contain in this [Document](https://docs.google.com/presentation/d/1pcG5sJjjT5bdXRbsl7uQMGZX0v1NefzA5OYjOddV2WQ)

## Installation

For easy installation, a precompiled `firmware.elf` file is provided for direct flashing onto the microcontroller (MCU). However, this file contains only the default configuration and is intended for use with the exact hardware specified in the instruction document. For custom configurations, users will need to modify the source code (either via menuconfig or directly in the `user_config.h` file) and recompile it themselves.

---------------------------------------------------------------------
The following steps are intended for those who have never used ESP-IDF before. If you are already familiar with ESP-IDF and have existing projects, you can simply take the source files and compile them.

1. **Install the ESP-IDF Framework**

   It is recommended to use Visual Studio Code (VSCode) along with the ESP-IDF extension. Espressif provides detailed documentation on installing and using [ESP-IDF](https://github.com/espressif/vscode-esp-idf-extension/blob/master/README.md).

2. **Try a Test Build (Optional but Recommended)**

    If you are not familiar with ESP-IDF, it is advisable to build and flash an example sketch from the ESP-IDF example library. This will also help ensure that your board is functioning correctly. Espressif has a very detail [guide](https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/startproject.html) for this process.  
   > **Note:** I recommend trying to build the `hello_world` example first, as the `blink` example's default configuration may not work for all board variants.

3. **Create Your ESP-IDF Project**

    Download the source code and open it in VSCode (select "Open Folder"). ESP-IDF will notify you if any files are missing; you can ignore this. Open the `ESP-IDF terminal` and type:

    ```bash
    idf.py reconfigure
    ```

    This command will reconfigure your project folder. If any files are missing or settings have not been applied, this will resolve those issues.

    In fact, whenever I make changes (e.g., editing `CMakeLists` or adding new files or components), I often encounter errors. Usually, cleaning the build and running `reconfigure` again resolves these issues.

    > **Note:** You can open the `ESP-IDF terminal` using a shortcut in the bottom bar or by pressing `Ctrl+Shift+P` and typing `Open ESP-IDF terminal`. This terminal is different from the standard VSCode terminal (usually PowerShell) and includes all the necessary commands, making it much easier to use. If you encounter an error stating that a file is not found or a command is unrecognized, you are likely not using the ESP-IDF terminal.

4. **Select Your MCU Target**

   In the bottom bar, locate the IDF-TARGET box (which will initially display "esp32") and change it to your specific board (S3 or C3). If prompted for a path list, simply choose the first option. Wait for the configuration to complete.

   > **Note:** When you have an IDF terminal open and use the icon in the bottom bar to change the IDF-TARGET, the terminal will not automatically switch to the new board. Any commands run in that terminal will result in a target mismatch error. Close the terminal and open a new one to resolve this issue.

5. **Build and Flash the Project**

    If everything is set up correctly, click the build wrench icon labeled `Build Project` or use the command:

    ```bash
    idf.py build
    ```

    If the build is successful, you can now flash the firmware to your board. Connect your board and set the port to the correct one.

    > If you have only one board connected, it will display only that one.

    Set the flash method to UART (the star icon) and click the flash icon labeled `Flash Device` or use the command:

    ```bash
    idf.py flash
    ```

6. **Complete the Flashing Process**

   Wait for the flashing process to complete, and you are done.

## Usage

- ***Right now only support for windows***

- Connect your device via Bluetooth. When prompted for passcode confirmation, simply click OK (or Pair).
- The device features two built-in scroll modes that can be switched with a double press:

> - **Vertical**: Functions like a regular mouse scroll.
> - **Horizontal**: Scrolls in the horizontal direction.

- Additionally, there are two speed settings, fast and slow, which can be changed with a triple press.

> **Note:**
>
> - Only two button functions are implemented for simplicity.
> - Single press is not used for button input to eliminate false button presses during normal usage.
> - Buttons only register when the device is in idle mode. This means that if you want to change settings (e.g., scroll direction) while using the device, you need to lift your finger for about 0.5 seconds before double pressing (or triple pressing).

## Contributing
