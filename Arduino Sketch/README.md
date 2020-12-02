### ARDUINO IDE

Copy the contents of the sketch folder into your sketches folder.

Copy the contents of the library folder into your libaries folder (usually called "libraries" within your sketches folder).

Load the IDE and load the .ino file from the ESP32-WORVER_Web_Radio. Note that a standard ESP32-WROOM will also work with this sketch as it detects whether PSRAM is present.

See the 'images' folder for the settings used for the ESP32.  

If you want to use PSRAM follow the instructions in the ESP32 PSRAM Hack.txt file. I suggest you switch OFF PSRAM initially and get the project running. It will use a 10K buffer.

When it is all running OK, then switch on the PSRAM if your board supports it, recompile and upload. You will then use 150K of PSRAM (about 10 seconds).

