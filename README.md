# 208 Arduino IDE Web Radio using a Task
I ported the updated **PlatformIO** version of this project to the **Arduino ID**E and introduced a playback dedicated task too.

### Video: https://

The ESP32 Web Radio project needs an independent task to stream music to the VS1053.

I noticed that whilst the processor was servicing button presses on the screen (or any number of other tasks) it could occasionally introduce stutter in the music playback, as the VS1053 (MP3 Decoder) routine was not being called quickly enough.

Introducing an independent task running the same code as before solved that problem. Tune in to see how it was done and how I ported the PlatformIO project to the Arduino IDE. Amongst many other things.

### List of all my videos  
(Special thanks to Michael Kurt Vogel for compiling this)  
http://bit.ly/YouTubeVideoList-RalphBacon

All about the **ILI9341 2.8" Touch TFT Screen** (about $7 from Banggood)    
http://www.lcdwiki.com/2.8inch_SPI_Module_ILI9341_SKU:MSP2807  
https://www.banggood.com/custlink/v33EPJT6cF  

The **AUKEY Ground Loop Isolator** from Amazon  
https://amzn.to/3of3xdb  

**USB to TTL Serial Cable** by DSD TECH SH-U09BL, with CP2102N Chip 1.2M/4FT  
What I have been using to monitor the Web Radio project for days on end!  
https://amzn.to/2Jx13Id  

**TTGO T8 V1.7.1 ESP32 Rev 1 4MB or 16MB FLASH with 8MB PSRAM**  
https://s.click.aliexpress.com/e/_A26OkW  

If you like this video please give it a thumbs up, share it and if you're not already subscribed please consider doing so and joining me on my Arduinite journey

My channel, GitHub and blog are here:  
\------------------------------------------------------------------  
https://www.youtube.com/RalphBacon  
https://ralphbacon.blog  
https://github.com/RalphBacon  
\------------------------------------------------------------------
