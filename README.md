# F16-DED

This code is used to create an F16 DED using the following hardware
 - 2.8" OLED Module 256x64 display. Must be SSD1322 compatible 
    - Amazon: https://www.amazon.com/gp/product/B0CHPCRLVF 
    - See: [docs/7-1.jpg](docs/7-1.jpg) &
      [docs/8-1.jpg](docs/7-1.jpg) for display manual
 - ESP32 Dev Board (esp32doit-devkit-v1)
    - Amazon: https://www.amazon.com/gp/product/B08246MCL5
    - Newer ESP32-S3 boards will likely work. Just check V_SPI pins
        - esp32-s3 - https://www.amazon.com/LuatOS-Development-Bluetooth-Interfaces-Compatible/dp/B0CDX87YX4
          - MACOS - install serial driver https://github.com/WCHSoftGroup/ch34xser_macos
          - platformis board: esp32-s3-devkitc-1

Code inspiration came from the following repos/threads:
- https://github.com/jg-storey/ded
- https://github.com/wiggles5289/Orange-Viper-Simulations/tree/main/F16
- https://forum.dcs.world/topic/261806-f16-ded-with-ssd1322-and-dcs-bios/page/3/

# TODO
- write unit tests

# Software
### Software Build requirements
- platformio - https://platformio.org/install


### Software Build Steps
1. Clone this repo
    ```
    git clone https://github.com/peterb154/F16-DED.git
    cd F16-DED
    ```
2. Create/update your wifi settings in [src/secrets.h](src/secrets.h).
    - See an example in  [src/secrets-example.h](src/secrets-example.h)
3. build and upload DCS-BIOS WiFi enabled versionto your board 
    ```
    pio run -e WIFI -t upload && pio device monitor
    ```

# Hardware
### ESP32 DevKit v1 HW SPI Wiring

    | OLED (Pin)  | Wire * | ESP32 (Pin)    | #define ** |
    | ----------- | ------ | -------------- | ---------- |
    | VDD (2)     | WHT    | 3v3            |            |
    | GND (1)     | BLK    | GND            |            |
    | SDIK D1 (5) | PPL    | V_SPI_D (23)   |            |
    | SCLK D0 (4) | GRY    | V_SPI_CLK (18) |            |
    | CS  (16)    | YEL    | V_SPI_CS0 (5)  | V_SPI_CS0  |
    | DC  (14)    | BLU    | GPIO (27)      | SPI_DC     |
    | RES (15)    | GRN    | GPIO (26)      | SPI_RESET  |

   - Note *: These are my wire colors, yours will likely be different
   - Note **: Update `#define` statements as required in [src/main.cpp](src/main.cpp)
   - See: https://mischianti.org/wp-content/uploads/2020/11/ESP32-DOIT-DEV-KIT-v1-pinout-mischianti.png for ESP32 dev kit v1 pinout

### DED Case
See [case/](case/) for stl files. Print at normal 20% infill, no supports. Its a very simple design that requires no fasteners.


# Sim configuration

### DCS Bios
1. On your DCS machine, Install latest Skunkworks version of DCS-BIOS - https://github.com/DCS-Skunkworks/dcs-bios/releases
   1. You dont need to so the socat steps, we'll use wifi baby
2. On your DCS machchine, Install DCS-Nexus mdns router - https://github.com/DCSBIOSKit/DCS-Nexus
   1. As of 2023-10-28: clone the repo `git clone https://github.com/DCSBIOSKit/DCS-Nexus && cd DCS-Nexus`
   2. create a virtual environment `python -m venv venv`
   3. run `venv\Scripts\python Nexus.pyw`
3. Fly the F16 in DCS and enjoythe DED
