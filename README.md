# ESP32 Internet Radio

This repo contains an .ino file ready to upload to your ESP32 to start playing radio stayions from the internet. Required hardware and libraries are below.

The stations in the current file are as follows and are known to be working:

- Jazz 90.1 (https://jazz901.org/)
- FFH Lounge (https://www.radio.net/s/ffhlounge)
- 91.7 KKXT (https://kxt.org/)

As long as you have all the hardware below, or know how to substitute, the included code should work out of the box for you. Please change pins as neccesary for differnt ESP-32s.

Do not forget to change your wifi credentials in the .ino file before uploading to your Arduino.

## Required Hardware:

- ESP32-C3-DevKitC-02
- VS1053 MP3 decoder
- ST7735 TFT display
- PAM8403 amplifier

## Required Libraries:

- [TFT Display: ST7735](https://github.com/PaulStoffregen/Adafruit_ST7735/blob/master/Adafruit_ST7735.h)
- [MP3 Decoder: VS1053](https://github.com/baldram/ESP_VS1053_Library/tree/master)


## Pinout/Circuit Diagram:

## Library Credit:

- TFT Display Library: https://github.com/PaulStoffregen
- MP3 Decoder Library: https://github.com/baldram

## Parts List:

- [ESP32-C3-DevKitC-02](https://www.amazon.com/Espressif-ESP32-C3-DevKitC-02-Development-Board/dp/B09D3S4RPZ)
- [TFT Display: ST7735](https://www.amazon.com/Display-Module-ST7735-128x160-STM32/dp/B07BFV69DZ)
- [MP3 Decoder: VS1053](https://www.amazon.com/dp/B0B1LFZ2DH?ref=ppx_pop_mob_ap_share)
- [PAM8403 Amplifier](https://a.co/d/2N3GC94)
- [Jumpers](https://www.amazon.com/Elegoo-EL-CP-004-Multicolored-Breadboard-arduino/dp/B01EV70C78?sr=8-3)
- [Rotary Encoder](https://www.microcenter.com/product/618904/inland-ks0013-keystudio-rotary-encoder-module)
