#include <Arduino.h>
#include "main.h"
#include "tftHelpers.h"
#include "wifiHelpers.h"
#include "taskHelper.h"

// ==================================================================================
// setup	setup	setup	setup	setup	setup	setup	setup	setup
// ==================================================================================
void setup()
{
	// Debug monitor TODO: disable if not in debugging mode
	Serial.begin(115200);

	// Are we using PSRAM?
	circBuffer.resize(CIRCULARBUFFERSIZE);
	log_d("Total heap: %d", ESP.getHeapSize());
	log_d("Free heap: %d", ESP.getFreeHeap());
	log_d("Total PSRAM: %d", ESP.getPsramSize());
	log_d("Free PSRAM: %d", ESP.getFreePsram());
	Serial.printf("Used PSRAM: %d\n", ESP.getPsramSize() - ESP.getFreePsram());

	// Station change pin TODO: move to interrupt pin, no polling required, simplifies coding
	pinMode(stnChangePin, INPUT_PULLUP);
	pinMode(tftTouchedPin, INPUT);
	pinMode(ICYDATAPIN, INPUT); // Cannot be pullup, input only pin

	// initialize SPI bus;
	Serial.println("Starting SPI");
	SPI.begin();

	// Start the display so we can show connection/hardware errors on it
	initDisplay();

	//How much SRAM free (heap memory)
	Serial.printf("Free memory: %d bytes\n", ESP.getFreeHeap());

	// Initialise LITTLEFS system
	if (!LITTLEFS.begin(false))
	{
		Serial.println("LITTLEFS Mount Failed.");
		tft.println("LITTLEFS Mount Failed.");
		while (1)
			delay(1);
	}
	else
	{
		Serial.println("LITTLEFS Mount SUCCESSFUL.");
	}

	// VS1053 MP3 decoder
	Serial.println("Starting player");
	player.begin();

	// Wait for the player to be ready to accept data
	Serial.println("Waiting for VS1053 initialisation to complete.");
	while (!player.data_request())
	{
		delay(1);
	}

	// You MIGHT have to set the VS1053 to MP3 mode. No harm done if not required!
	Serial.println("Switching player to MP3 mode");
	player.switchToMp3Mode();

	// Set the volume here to MAX (100)
	player.setVolume(100);

	// Set the equivalent of "Loudness" (increased bass & treble)
	// Bits 15:12	treble control in 1.5dB steps (-8 to +7, 0 = off)
	// Bits 11:8	lower limit frequency in 1kHz steps (1kHz to 15kHz)
	// Bits 7:4		Bass Enhancement in 1dB steps (0 - 15, 0 = off)
	// Bits 3:0		Lower limit frequency in 10Hz steps (2 - 15)
	char trebleCutOff = 3; // 3kHz cutoff
	char trebleBoost = 3 << 4 | trebleCutOff;
	char bassCutOff = 10; // times 10 = 100Hz cutoff
	char bassBoost = 3 << 4 | bassCutOff;

	uint16_t SCI_BASS = trebleBoost << 8 | bassBoost;
	// equivalent of player.setTone(0b0111001110101111);
	player.setTone(SCI_BASS);

	// Some sort of startup message (I just recorded this using
	// https://onlinetonegenerator.com/voice-generator.html)
	File file = LITTLEFS.open("/Intro.mp3");
	if (file)
	{
		uint8_t audioBuffer[32] __attribute__((aligned(4)));
		while (file.available())
		{
			int bytesRead = file.read(audioBuffer, 32);
			player.playChunk(audioBuffer, bytesRead);
		}
		file.close();
	}
	else
	{
		Serial.println("Unable to open greetings file");
	}

	// Set the volume here to MIN (0)
	//player.setVolume(0);
	//volumeMax = false;

	// Start WiFi
	ssid = getSSID();
	wifiPassword = getWiFiPassword();
	connectToWifi();
	if (wiFiDisconnected)
	{
		// TODO: Print message to TFT otherwise won't know it's failed
		Serial.printf("Unable to connect to WiFi network: '%s'\n", ssid.c_str());
		while (1)
			delay(1);
	}

	// Whether we want MetaData or not. Connect the pin to GND to skip METADATA.
	METADATA = digitalRead(ICYDATAPIN) == HIGH;

	// Get the station number that was previously playing
	preferences.begin("WebRadio", false);
	currStnNo = preferences.getUInt("currStnNo", 0);
	if (currStnNo > stationCnt - 1)
	{
		currStnNo = 0;
	}
	prevStnNo = currStnNo;

	// Connect to that station
	Serial.printf("Current station number: %u\n", currStnNo);
	while (!stationConnect(currStnNo))
	{
		checkForStationChange();
		if (!client.connected())
		{
			connectToWifi();
		}
	};

	// Set screen brightness to previous level
	ledcSetup(0, 5000, 8);
	ledcAttachPin(TFT_BL, 0);

	// 0 (off) to 255(bright) duty cycle
	prevTFTBright = preferences.getUInt("Bright", 255);
	Serial.printf("Restored screen brightness to %d\n", prevTFTBright);
	ledcWrite(0, prevTFTBright);

	// We need to set up independent task that plays the music from the circular buffer
	taskSetup();

	//Now how much SRAM free (heap memory)
	Serial.printf("Free memory: %d bytes\n", ESP.getFreeHeap());
}

// ==================================================================================
// loop     loop     loop     loop    loop     loop     loop     loop    loop    loop
// ==================================================================================
void loop()
{
	// Data to read from mainBuffer?
	if (client.available())
	{
		populateRingBuffer();

		// If we've read all data between metadata bytes, check to see if there is track title to read
		if (bytesUntilmetaData == 0 && METADATA)
		{
			// TODO: if cant read valid metadata we need to reconnect
			if (!readMetaData())
			{
				while (!stationConnect(currStnNo))
				{
					checkForStationChange();
					if (!client.connected())
					{
						connectToWifi();
					}
				};
			}

			// reset byte count for next bit of metadata
			bytesUntilmetaData = metaDataInterval;
		}
	}
	else
	{
		// Sometimes we get randomly disconnected from WiFi BUG Why?
		if (!client.connected())
		{
			// TODO: if client suddenly not connected we will have to reconnect
			Serial.println("Client not connected.");
			connectToWifi();
			while (!stationConnect(currStnNo))
			{
				checkForStationChange();
				if (!client.connected())
				{
					connectToWifi();
				}
			};
		}
	}

	// // If we (no longer) need to buffer the streaming data (after a station change)
	// // allow the buffer to be played
	// if (canPlayMusicFromBuffer)
	// {
	// 	// If we failed to play 32b from the buffer (insufficient data) set the flag again
	// 	if (!playMusicFromRingBuffer())
	// 	{
	// 		canPlayMusicFromBuffer = false;
	// 	};
	// }
	// else
	// {
	// 	// Otherwise, check whether we now have enough data to start playing
	// 	checkBufferForPlaying();
	// }

	// So how many bytes have we got in the buffer (should hover around 90%)
	drawBufferLevel(circBuffer.available());

	// Has CHANGE STATION button been pressed?
	checkForStationChange();

	//Mute/Unmute
	getMuteButtonPress();

	// Screen brightness
	getBrightButtonPress();
	getDimButtonPress();
}

// Populate ring buffer with streaming data
void populateRingBuffer()
{
	// Signed because we might -1 returned
	signed int bytesReadFromStream = 0;

	// Room in the ring buffer for (up to) X bytes?
	if (circBuffer.room() >= streamingCharsMax)
	{
		// Read either the maximum available (max 100) or the number of bytes to the next meata data interval
		bytesReadFromStream = client.read((uint8_t *)readBuffer, min(streamingCharsMax, METADATA ? (int)bytesUntilmetaData : streamingCharsMax));

		// If we get -1 here it means nothing could be read from the stream
		// TODO: find out why this might be. Remote server not streaming?
		if (bytesReadFromStream > 0)
		{
			// Add them to the circular buffer
			circBuffer.write(readBuffer, bytesReadFromStream);

			// If we didn't read the amount we "expected" debug that here
			// if (bytesReadFromStream < streamingCharsMax && bytesReadFromStream != bytesUntilmetaData)
			// {
			// 	Serial.printf("%db to circ\n", bytesReadFromStream);
			// }

			// Subtract bytes actually read from incoming http data stream from the bytesUntilmetaData
			bytesUntilmetaData -= bytesReadFromStream;
		}
	}
}

// Copy streaming data to our ring buffer and check wheter there's enough to start playing yet
// This is only excuted after a station connect to give the buffer a chance to fill up.
void checkBufferForPlaying()
{
	// If we have now got enough in the ring buffer to allow playing to start without stuttering?
	if (circBuffer.available() > CIRCULARBUFFERSIZE / 3)
	{
		// Reset the flag, allowing data to be played, won't get reset again until station change
		canPlayMusicFromBuffer = true;
	}
}

// Connect to the station list number
bool stationConnect(int stationNo)
{
	Serial.println("--------------------------------------");
	Serial.printf("        Connecting to station %d\n", stationNo);
	Serial.println("--------------------------------------");

	// Flag to indicate we need to buffer data before allowing player to stream audio
	canPlayMusicFromBuffer = false;

	// Clear down the streaming buffer and optionally reset the player (to flush it)
	circBuffer.flush();
	
	//How much SRAM free (heap memory)
	Serial.printf("Free memory: %d bytes\n", ESP.getFreeHeap());

	// Determine whether we want ICY metadata
	METADATA = digitalRead(ICYDATAPIN) == HIGH;

	// For THIS radio station have we FORCED meta data to be ignored?
	METADATA = METADATA ? radioStation[stationNo].useMetaData : METADATA;
	if (!radioStation[stationNo].useMetaData)
	{
		Serial.println("METADATA ignored for this radio station.");
	}

	// Set the metadataInterval value to zero so we can detect that we found a valid one
	metaDataInterval = 0;

	// Clear down any screen info
	displayStationName(radioStation[stationNo].friendlyName);
	displayTrackArtist((char *)"");
	drawBufferLevel(0, true);

	// We try a few times to connect to the station
	bool connected = false;
	int connectAttempt = 0;

	while (!connected && connectAttempt < 5)
	{
		if (redirected)
		{
			Serial.printf("REDIRECTED URL DETECTED FOR STATION %d\n", stationNo);
		}

		connectAttempt++;
		Serial.printf("Host: %s Port:%d\n", radioStation[stationNo].host, radioStation[stationNo].port);

		// Connect to the redirected URL
		if (client.connect(radioStation[stationNo].host, radioStation[stationNo].port))
		{
			connected = true;
		}
	}

	// If we could not connect (eg bad URL) just exit
	if (!connected)
	{
		Serial.printf("Could not connect to %s\n", radioStation[stationNo].host);
		return false;
	}
	else
	{
		Serial.printf("Connected to %s (%s%s)\n",
					  radioStation[stationNo].host, radioStation[stationNo].friendlyName,
					  redirected ? " - redirected" : "");
	}

	// Get the data stream plus any metadata (eg station name, track info between songs / ads)
	// TODO: Allow retries here (BBC Radio 4 very finicky before streaming).
	// We might also get a redirection URL given back.
	Serial.printf("Getting data from %s (%s Metadata)\n", radioStation[stationNo].path, (METADATA ? "WITH" : "WITHOUT"));
	client.print(
		String("GET ") + radioStation[stationNo].path + " HTTP/1.1\r\n" +
		"Host: " + radioStation[stationNo].host + "\r\n" +
		(METADATA ? "Icy-MetaData:1\r\n" : "") +
		"Connection: close\r\n\r\n");

	// Give the client a chance to connect
	Serial.println("Waiting for header data");
	int retryCnt = 30;
	while (client.available() == 0 && --retryCnt > 0)
	{
		delay(100);
	}

	if (client.available() < 1)
		return false;

	// Keep reading until we read two LF bytes in a row.
	//
	// The ICY (I Can Yell, a precursor to Shoutcast) format:
	// In the http response we can look for icy-metaint: XXXX to tell us how far apart
	// the header information (in bytes) is sent.

	// Process all responses until we run out of header info (blank header)
	while (client.available())
	{
		// Delimiter char is not included in data returned
		String responseLine = client.readStringUntil('\n');

		if (responseLine.indexOf("Status: 200 OK") > 0)
		{
			// TODO: we really should check we have a response of 200 before anything else
			Serial.println("200 - OK response");
			continue;
		}

		// If we have an EMPTY header (or just a CR) that means we had two linefeeds
		if (responseLine[0] == (uint8_t)13 || responseLine == "")
		{
			break;
		}

		// If the header is not empty process it
		//Serial.print("HEADER: ");
		//Serial.println(responseLine);

		// Critical value for this whole sketch to work: bytes between "frames"
		// Sometimes you can't get this first time round so we just reconnect
		// (Actually it's only BCC Radio 4 that doesn't always give this out)
		if (responseLine.startsWith("icy-metaint"))
		{
			metaDataInterval = responseLine.substring(12).toInt();
			Serial.printf("NEW Metadata Interval:%d\n", metaDataInterval);
			continue;
		}

		// The bit rate of the transmission (FYI) eye candy
		if (responseLine.startsWith("icy-br:"))
		{
			bitRate = responseLine.substring(7).toInt();
			Serial.printf("Bit rate:%d\n", bitRate);
			continue;
		}

		// TODO: Remove this testing override for station 4 (always redirects!)
		// The URL we used has been redirected
		if (!redirected && stationNo == 4)
		{
			responseLine = "location: http://stream.antenne1.de:80/a1stg/livestream1.aac";
		}

		if (responseLine.startsWith("location: http://"))
		{
			getRedirectedStationInfo(responseLine, stationNo);
			redirected = true;
			return false;
		}
	}

	// If we didn't find required metaDataInterval value in the headers, abort this connection
	if (metaDataInterval == 0 && METADATA)
	{
		Serial.println("NO METADATA INTERVAL DETECTED - RECONNECTING");
		return false;
	}

	// Update the count of bytes until the next metadata interval (used in loop) and exit
	bytesUntilmetaData = metaDataInterval;

	// All done here
	return true;
}

// LITTLEFS card reader (done ONCE in setup)
// Format of data is:
//          #comment line
//          <KEY><DATA>
std::string readLITTLEFSInfo(char *itemRequired)
{
	char startMarker = '<';
	char endMarker = '>';
	char *receivedChars = new char[32];
	int charCnt = 0;
	char data;
	bool foundKey = false;

	Serial.printf("Looking for key '%s'.\n", itemRequired);

	// Get a handle to the file
	File configFile = LITTLEFS.open("/WiFiSecrets.txt", FILE_READ);
	if (!configFile)
	{
		// TODO: Display error on screen
		Serial.println("Unable to open file /WiFiSecrets.txt");
		while (1)
			;
	}

	// Look for the required key
	while (configFile.available())
	{
		charCnt = 0;

		// Read until start marker found
		while (configFile.available() && configFile.peek() != startMarker)
		{
			// Do nothing, ignore spurious chars
			data = configFile.read();
			//Serial.print("Throwing away preMarker:");
			//Serial.println(data);
		}

		// If EOF this is an error
		if (!configFile.available())
		{
			// Abort - no further data
			continue;
		}

		// Throw away startMarker char
		configFile.read();

		// Read all following characters as the data (key or value)
		while (configFile.available() && configFile.peek() != endMarker)
		{
			data = configFile.read();
			receivedChars[charCnt] = data;
			charCnt++;
		}

		// Terminate string
		receivedChars[charCnt] = '\0';

		// If we previously found the matching key then return the value
		if (foundKey)
			break;

		//Serial.printf("Found: '%s'.\n", receivedChars);
		if (strcmp(receivedChars, itemRequired) == 0)
		{
			//Serial.println("Found matching key - next string will be returned");
			foundKey = true;
		}
	}

	// Terminate file
	configFile.close();

	// Did we find anything
	Serial.printf("LITTLEFS parameter '%s'\n", itemRequired);
	if (charCnt == 0)
	{
		Serial.println("' not found.");
		return "";
	}
	else
	{
		//Serial.printf("': '%s'\n", receivedChars);
	}

	return receivedChars;
}

void changeStation(int8_t btnValue)
{
	Serial.println("--------------------------------------");
	Serial.print("       Change Station ");
	Serial.println(btnValue > 0 ? "(NEXT)" : "(PREV)");
	Serial.println("--------------------------------------");

	// Make button inactive
	canChangeStn = false;

	// Reset any redirection flag
	redirected = false;

	// Get the next/prev station (in the list)
	nextStnNo = currStnNo + btnValue;
	if (nextStnNo > stationCnt - 1)
	{
		nextStnNo = 0;
	}
	else
	{
		if (nextStnNo < 0)
		{
			nextStnNo = stationCnt - 1;
		}
	}

	// Whether connected to this station or not, update the variable otherwise
	// we would 'stick' at old station
	currStnNo = nextStnNo;

	if (prevStnNo != nextStnNo)
	{
		prevStnNo = nextStnNo;
		//player.softReset();

		// Now actually connect to the new URL for the station
		while (!stationConnect(nextStnNo))
		{
			checkForStationChange();
			if (!client.connected())
			{
				connectToWifi();
			}
		};

		// Next line might not be required for VS051
		player.switchToMp3Mode();

		// Store (new) current station in EEPROM
		preferences.putUInt("currStnNo", nextStnNo);
		Serial.printf("Current station now stored: %u\n", nextStnNo);

		// Button active again
		canChangeStn = true;
	}
}

inline bool readMetaData()
{
	// The first byte is going to be the length of the metadata
	int metaDataLength = client.read();

	// Usually there is none as track/artist info is only updated when it changes
	// It may also return the station URL (not necessarily the same as we are using).
	// Example:
	//  'StreamTitle='Love Is The Drug - Roxy Music';StreamUrl='https://listenapi.planetradio.co.uk/api9/eventdata/62247302';'
	if (metaDataLength < 1) // includes -1
	{
		// Warning sends out about 4 times per second!
		//Serial.println("No metadata to read.");
		return "";
	}

	// The actual length is 16 times bigger to allow from 16 up to 4080 bytes (255 * 16) of metadata
	metaDataLength = (metaDataLength * 16);
	Serial.printf("Metadata block size: %d\n", metaDataLength);

	// Wait for the entire MetaData to become available in the stream
	Serial.println("Waiting for METADATA");
	while (client.available() < metaDataLength)
	{
		delayMicroseconds(1);
	}

	// Temporary buffer for the metadata
	char metaDataBuffer[metaDataLength + 1];

	// Initialise this temp buffer
	memset(metaDataBuffer, 0, metaDataLength + 1);

	// Populate it from the internet stream
	client.readBytes((char *)metaDataBuffer, metaDataLength);
	Serial.print("MetaData:");
	Serial.println(metaDataBuffer);

	for (auto cnt = 0; cnt < metaDataLength; cnt++)
	{
		if (metaDataBuffer[cnt] > 0 && metaDataBuffer[cnt] < 8)
		{
			Serial.printf("Corrupt METADATA found:%02X\n", metaDataBuffer[cnt]);

			// Terminate the string right after the corrupt char so we can print it
			metaDataBuffer[cnt + 1] = '\0';
			Serial.println(metaDataBuffer);
			return false;
		}
	}

	// Extract track Title/Artist from this string
	char *startTrack = NULL;
	char *endTrack = NULL;
	std::string streamArtistTitle = "";

	startTrack = strstr(metaDataBuffer, "StreamTitle");
	if (startTrack != NULL)
	{
		// We have found the streamtitle so just skip over "StreamTitle="
		startTrack += 12;

		// Now look for the end marker
		endTrack = strstr(startTrack, ";");
		if (endTrack == NULL)
		{
			// No end (very weird), so just set it as the string length
			endTrack = (char *)startTrack + strlen(startTrack);
		}

		// There might be an opening and closing quote so skip over those (reduce data width) too
		if (startTrack[0] == '\'')
		{
			startTrack += 1;
			endTrack -= 1;
		}

		// We MUST terminate the 'string' (character array) with a null to denote the end
		endTrack[0] = '\0';

		// Extract the data by adjusting pointers
		ptrdiff_t startIdx = startTrack - metaDataBuffer;
		ptrdiff_t endIdx = endTrack - metaDataBuffer;
		std::string streamInfo(metaDataBuffer, startIdx, endIdx);
		streamArtistTitle = streamInfo;

		// Debug only if there is something to see
		if (streamArtistTitle != "")
			Serial.printf("%s\n", streamArtistTitle.c_str());

		// Always output the Artist/Track information even if just to clear it from screen
		displayTrackArtist(toTitle(streamArtistTitle));
	}

	// All done
	return true;
}

// Our streaming station has been 'redirected' to another URL
// The header will look like: Location: http://<new host / path>[:<port>]
void getRedirectedStationInfo(String header, int currStationNo)
{
	Serial.println("--------------------------------------");
	Serial.println(" Extracting redirection information");
	Serial.println("--------------------------------------");

	// Placeholders for the new host/path
	String redirectedHost = "";
	String redirectedPath = "";

	// We'll assume the port is 80 unless we find one in the host name
	int redirectedPort = 80;

	// Skip the "redirected http://" bit at the front
	header = header.substring(17);
	Serial.printf("Redirecting to: %s\n", header.c_str());

	// Split the header into host and path constituents
	int pathDelimiter = header.indexOf("/");
	if (pathDelimiter > 0)
	{
		redirectedPath = header.substring(pathDelimiter);
		redirectedHost = header.substring(0, pathDelimiter);
	}
	// Look to split host into host and port number
	// Example: stream/myradio.de:8080
	int portDelimter = header.indexOf(":");
	if (portDelimter > 0)
	{
		redirectedPort = header.substring(portDelimter + 1).toInt();

		// Adjust the host name to exclude the port information
		redirectedHost = redirectedHost.substring(0, portDelimter);
	}

	// Just overwrite the current entry for this station (reverts on reboot)
	// TODO: consider writing all this to EEPROM / SPIFFS
	Serial.println("New address: " + redirectedHost + redirectedPath + ":" + redirectedPort);
	strncpy(radioStation[currStationNo].host, redirectedHost.c_str(), 64);
	strncpy(radioStation[currStationNo].path, redirectedPath.c_str(), 128);

	return;
}

// Change station button / screen button pressed?
void checkForStationChange()
{
	// Only allow this function to run infrequently or we skip music
	static unsigned long prevMillis = millis();

	// X/Y coordinates of any screen press
	uint16_t x, y;

	// Physical button(s) go LOW when active
	if (millis() - prevMillis > 100)
	{
		prevMillis = millis();

		// First check for physical button press
		if (!digitalRead(stnChangePin) && canChangeStn)
		{
			changeStation(+1);
		}
		else
		{
			// Otherwise check for on-screen button
			if (!digitalRead(tftTouchedPin) && canChangeStn)
			{
				//Serial.println("TFT Touch!");
				boolean btnPressed = tft.getTouch(&x, &y, 50U);

				//Serial.printf("Prev: x=%d to %d, y=%d to %d\n", PREV_BUTTON_X, PREV_BUTTON_X + PREV_BUTTON_W, PREV_BUTTON_Y, PREV_BUTTON_Y + PREV_BUTTON_H);
				//Serial.printf("Next: x=%d to %d, y=%d to %d\n", NEXT_BUTTON_X, NEXT_BUTTON_X + NEXT_BUTTON_W, NEXT_BUTTON_Y, NEXT_BUTTON_Y + NEXT_BUTTON_H);

				// The TFT has registered a "touch" but was it an actual screen touch
				// in the right place and of sufficent pressure?
				if (btnPressed)
				{
					// Stop further channel hopping
					canChangeStn = false;

					// Did we press the NEXT button?
					if (getNextButtonPress(x, y))
					{
						changeStation(+1);
						nextBtn.drawButton(false);
					}
					else
					// Did we press the PREV button?
					{
						if (getPrevButtonPress(x, y))
						{
							changeStation(-1);
							prevBtn.drawButton(false);
						}
					}

					// Allow futher NEXT/PREV presses now
					canChangeStn = true;
				}
				else
				{
					// Press sensitivity controlled by getTouch function
					Serial.println("Press not hard enough!");
				}
			}
		}
	}
}