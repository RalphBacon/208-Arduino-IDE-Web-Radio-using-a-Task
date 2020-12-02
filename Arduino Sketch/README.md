### ARDUINO IDE

Copy the contents of the sketch folder into your sketches folder.

Copy the contents of the library folder into your libaries folder (usually called "libraries" within your sketches folder).

Update your WiFi details in the file WiFiSecrets.txt in the DATA folder (included).

Load the IDE and load the .ino file from the ESP32-WORVER_Web_Radio. Note that a standard ESP32-WROOM will also work with this sketch as it detects whether PSRAM is present.

See the 'images' folder for the settings used for the ESP32.  

Remember to set your SPIFFS (LittleFS) partition to 2Mb/2Mb and then upload the data partition FIRST. Then upload your project - the data in SPIFFS/LittleFS partition will remain unaffected.

You MUST calibrate the ILI9341 TFT Touch Screen. It will generate the calibration file (example in the DATA folder). I just copied the bytes out of it (by displaying them on screen) so they would not get overwritten if I uploaded the SPIFFS partition again. This will be refined going forward.

If you want to use PSRAM follow the instructions in the ESP32 PSRAM Hack.txt file. I suggest you switch OFF PSRAM initially and get the project running. It will use a 10K buffer.

When it is all running OK, then switch on the PSRAM if your board supports it, recompile and upload. You will then use 150K of PSRAM (about 10 seconds).

