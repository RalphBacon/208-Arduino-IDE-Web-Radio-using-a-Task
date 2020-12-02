// Ensure this header file is only included once
#ifndef _ESP32RADIO_
#define _ESP32RADIO_

/*
	Includes go here
*/
// Utility to write to (psuedo) EEPROM
#include <Preferences.h>

// Standard ESP32 WiFi
#include <WiFi.h>

// MP3+ decoder
#include <VS1053.h>

// Standard input/output streams, required for <locale> but this might get removed
#include <iostream>

// SPIFFS (in-memory SPI File System) replacement
#include <LITTLEFS.h>

// Display / TFT
#include <User_Setup.h>
#include "TFT_eSPI.h"
#include "Free_Fonts.h"

// Your preferred TFT / LED screen definition here
// As I'm using the TFT_eSPI from Bodmer they are all in User_Setup.h

// Circular buffer - ESP32 built in (hacked by me to use PSRAM)
#include <cbuf.h>

// EEPROM writing routines (eg: remembers previous radio stn)
Preferences preferences;

// Station index/number
unsigned int currStnNo, prevStnNo;
signed int nextStnNo;

// Secret WiFi stuff from LITTLEFS (SPIFFS+)
std::string ssid;
std::string wifiPassword;
bool wiFiDisconnected = true;

// All connections are assumed insecure http:// not https://
const int8_t stationCnt = 8; // start counting from 1!

/* 
    Example URL: [port optional, 80 assumed]
        stream.antenne1.de[:80]/a1stg/livestream1.aac Antenne1 (Stuttgart)
*/

struct radioStationLayout
{
	char host[64];
	char path[128];
	int port;
	char friendlyName[64];
	uint8_t useMetaData;
};

struct radioStationLayout radioStation[stationCnt] = {

	//0
	"stream.antenne1.de",
	"/a1stg/livestream1.aac",
	80,
	"Antenne1.de",
	1,

	//1
	"bbcmedia.ic.llnwd.net",
	"/stream/bbcmedia_radio4fm_mf_q", // also mf_p works
	80,
	"BBC Radio 4",
	0,

	//2
	"stream.antenne1.de",
	"/a1stg/livestream2.mp3",
	80,
	"Antenne1 128k",
	1,

	//3
	"listen.181fm.com",
	"/181-beatles_128k.mp3",
	80,
	"Beatles 128k",
	1,

	//4
	"stream-mz.planetradio.co.uk",
	"/magicmellow.mp3",
	80,
	"Mellow Magic (Redirected)",
	1,

	//5
	"edge-bauermz-03-gos2.sharp-stream.com",
	"/net2national.mp3",
	80,
	"Greatest Hits 112k (National)",
	1,

	//6
	"airspectrum.cdnstream1.com",
	"/1302_192",
	8024,
	"Mowtown Magic Oldies",
	1,

	//7
	"live-bauer-mz.sharp-stream.com",
	"/magicmellow.aac",
	80,
	"Mellow Magic (48k AAC)",
	1,

};

// Pushbutton connected to this pin to change station
int stnChangePin = 13;
int tftTouchedPin = 15;
uint prevTFTBright;

// Can we use the above button (not in the middle of changing stations)?
bool canChangeStn = true;

// Current state of WiFi (connected, idling)
int status = WL_IDLE_STATUS;

// Do we want to connect with track/artist info (metadata)
bool METADATA = true;

// Whether to request ICY data or not. Overridden if set to 0 in radio stations.
#define ICYDATAPIN 36 // Input only pin, requires pull-up 10K resistor

// The number of bytes between metadata (title track)
uint16_t metaDataInterval = 0; //bytes
uint16_t bytesUntilmetaData = 0;
int bitRate = 0;
bool redirected = false;
bool volumeMax = false;

// Dedicated 32-byte buffer for VS1053 aligned on 4-byte boundary for efficiency
uint8_t mp3buff[32] __attribute__((aligned(4)));

// Circular "Read Buffer" to stop stuttering on some stations
#ifdef BOARD_HAS_PSRAM
#define CIRCULARBUFFERSIZE 150000 // Divide by 32 to see how many 2mS samples this can store
#else
#define CIRCULARBUFFERSIZE 10000
#endif
cbuf circBuffer(10);
#define streamingCharsMax 32

// Internet stream buffer that we copy in chunks to the ring buffer
char readBuffer[100] __attribute__((aligned(4)));

// Wiring of VS1053 board (SPI connected in a standard way) on ESP32 only
#define VS1053_CS 32
#define VS1053_DCS 33
#define VS1053_DREQ 35
#define VOLUME 100 // treble/bass works better if NOT 100 here

// WiFi specific defines
#define WIFITIMEOUTSECONDS 20

// Forward declarations of functions TODO: clean up & describe FIXME:
bool stationConnect(int station_no);
std::string readLITTLEFSInfo(char *itemRequired);
std::string getWiFiPassword();
std::string getSSID();
void connectToWifi();
const char *wl_status_to_string(wl_status_t status);
void initDisplay();
void changeStation(int8_t plusOrMinus);
bool _GLIBCXX_ALWAYS_INLINE readMetaData();
void getRedirectedStationInfo(String header, int currStationNo);
void setupDisplayModule();
void displayStationName(char *stationName);
void displayTrackArtist(std::string);

bool getNextButtonPress(uint16_t x, uint16_t y);
bool getPrevButtonPress(uint16_t x, uint16_t y);

void drawPrevButton();
void drawNextButton();
void drawMuteButton(bool invert);
void drawBrightButton(bool invert);
void drawDimButton(bool invert);
void drawMuteBitmap(bool isMuted);

void getBrightButtonPress();
void getDimButtonPress();

std::string toTitle(std::string s, const std::locale &loc = std::locale());
void drawBufferLevel(size_t bufferLevel, bool override = false);
void checkForStationChange();
void populateRingBuffer();

void taskSetup();

// Not Best Practice but I'm instantiating all objects here to remove them from main.cpp

// MP3 decoder
VS1053 player(VS1053_CS, VS1053_DCS, VS1053_DREQ);

// Instantiate screen (object) using hardware SPI. Defaults to 320H x 240W
TFT_eSPI tft = TFT_eSPI();

// Start the WiFi client here
WiFiClient client;

#endif
