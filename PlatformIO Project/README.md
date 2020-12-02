This is the entire PlatformIO project (updated).

Remember to set your SPIFFS (LittleFS) partition to 2Mb/2Mb and then upload the data partition FIRST. Then upload your project - the data in SPIFFS/LittleFS partition will remain unaffected.

You MUST calibrate the ILI9341 TFT Touch Screen. It will generate the calibration file (example in the DATA folder). I just copied the bytes out of it (by displaying them on screen) so they would not get overwritten if I uploaded the SPIFFS partition again. This will be refined going forward.

