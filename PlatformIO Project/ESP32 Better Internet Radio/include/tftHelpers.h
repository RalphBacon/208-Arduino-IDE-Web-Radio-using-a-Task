// All of the ILI9341 TFT Touch Screen routines are included here
#include "Arduino.h"
#include "main.h"

// Bitmap display helper (Bodmer's)
#include "bitmapHelper.h"

// IMPORTANT: you MUST run the touch calibration sketch to store the values
// for future runs of the sketch (ie run once, in the orientation you expect
// to use the screen)

// Switch position and size
#define FRAME_X 2
#define FRAME_Y 190
#define FRAME_W 40
#define FRAME_H 30

// Red zone size
#define PREV_BUTTON_X FRAME_X
#define PREV_BUTTON_Y FRAME_Y
#define PREV_BUTTON_W FRAME_W
#define PREV_BUTTON_H FRAME_H

#define NEXT_BUTTON_X (PREV_BUTTON_X + PREV_BUTTON_W + 7)
#define NEXT_BUTTON_Y FRAME_Y
#define NEXT_BUTTON_W (FRAME_W)
#define NEXT_BUTTON_H FRAME_H

TFT_eSPI_Button muteBtn, prevBtn, nextBtn, brightBtn, dimBtn;

// Display routines (hardware/protocol dependent)
void initDisplay()
{
	// check if calibration file exists and size is correct
	if (LITTLEFS.begin(false) && LITTLEFS.exists("/TouchCalData1.txt"))
	{
		File f = LITTLEFS.open("/TouchCalData1.txt", "r");
		if (f)
		{
			uint16_t calData[5];
			if (f.readBytes((char *)calData, 14) == 14)
				tft.setTouch(calData);

			Serial.println("TFT Calibration data:");
			for (auto cnt = 0; cnt < 14; cnt++)
			{
				Serial.printf("%04X, ", calData[cnt]);
			}

			f.close();
			Serial.println("\nTouch Calibration completed");
		}
	}
	else
	{
		Serial.println("NO TOUCH CALIBRATION FILE FOUND");
	}

	// TODO: Change "text" buttons to jpg images with "invisible" buttons (eg black on black) to get the
	// functionality of a button but a better visual experience/

	// Prev button
	prevBtn.initButtonUL(
		&tft, PREV_BUTTON_X, PREV_BUTTON_Y, PREV_BUTTON_W, PREV_BUTTON_H, TFT_YELLOW, TFT_BLUE, TFT_WHITE, (char *)"<", 1);

	// Next button
	nextBtn.initButtonUL(
		&tft, NEXT_BUTTON_X, NEXT_BUTTON_Y, NEXT_BUTTON_W, NEXT_BUTTON_H, TFT_YELLOW, TFT_RED, TFT_WHITE, (char *)">", 1);

	// Brighten the screen
	brightBtn.initButtonUL(
		&tft, NEXT_BUTTON_X + 47, NEXT_BUTTON_Y, NEXT_BUTTON_W, NEXT_BUTTON_H, TFT_YELLOW, TFT_BLUE, TFT_WHITE, (char *)"+", 1);

	// Dim the screen
	dimBtn.initButtonUL(
		&tft, NEXT_BUTTON_X + 93, NEXT_BUTTON_Y, NEXT_BUTTON_W, NEXT_BUTTON_H, TFT_YELLOW, TFT_MAROON, TFT_WHITE, (char *)"-", 1);

	// Mute button:	x, y, w, h, outline, fill, text color, label, text size
	//muteBtn.setLabelDatum(0, 2, MC_DATUM);
	//muteBtn.initButtonUL(
	//	&tft, 187, FRAME_Y, 57, 30, TFT_RED, TFT_GREEN, TFT_BLACK, (char *)"MUTE", 1);

	// Black icon, no text
	muteBtn.initButtonUL(
		&tft, 187, FRAME_Y, 57, 30, TFT_BLACK, TFT_BLACK, TFT_BLACK, (char *)"", 1);

	// Run once
	setupDisplayModule();
}

// Put the TFT specific code here to prepare your screen
void setupDisplayModule()
{

	// Initialise
	tft.init();

	// Rotation as required, the touch calibration MUST match this
	// 0 & 2 Portrait. 1 & 3 landscape
	tft.setRotation(1);

	// Write title of app on screen, using font 2 (x, y, font #)
	tft.fillScreen(TFT_BLACK);

	// Border + fill
	tft.fillRect(1, 1, 318, 43, TFT_RED);
	tft.drawRect(0, 0, 320, 45, TFT_YELLOW);

	// Application Title
	tft.setFreeFont(&FreeSansBold12pt7b);
	tft.setTextSize(1);
	tft.setCursor(32, 30);

	// Set text colour and background
	tft.setTextColor(TFT_YELLOW, TFT_RED);
	tft.println("- ESP32 WEB RADIO -");

	// My details
	tft.setTextFont(2);
	tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
	tft.setCursor(5, tft.height() - 20);
	tft.println("Ralph S Bacon - https://youtube.com/ralphbacon");

	// Draw button(s)
	drawNextButton();
	drawPrevButton();

	drawBufferLevel(0);

	drawBrightButton(false);
	drawDimButton(false);

	drawMuteButton(false);
	drawMuteBitmap(false);

	// All done
	Serial.println("TFT Initialised");
	return;
}

// Some track titles are ALL IN UPPER CASE, so ugly, so let's convert them
// Stackoverflow: https://stackoverflow.com/a/64128715
std::string toTitle(std::string s, const std::locale &loc)
{
	// Is the current character a word delimiter (a space)?
	bool last = true;

	// Process each character in the supplied string
	for (char &c : s)
	{
		// If the previous character was a word delimiter (space)
		// then upper case current word start, else make lower case
		c = last ? std::toupper(c, loc) : std::tolower(c, loc);

		// Is this character a space?
		last = std::isspace(c, loc);
	}
	return s;
}

// Draw proof-of-concept NEXT button TODO: expose coordinates
void drawNextButton()
{
	// tft.fillRoundRect(NEXT_BUTTON_X, REDBUTTON_Y, REDBUTTON_W, REDBUTTON_H, 5, TFT_RED);
	// tft.drawRoundRect(FRAME_X, FRAME_Y, FRAME_W, FRAME_H, 5, TFT_YELLOW);
	// tft.setTextColor(TFT_WHITE);
	// tft.setTextSize(2);
	// tft.setTextDatum(MC_DATUM);
	// tft.drawString("NEXT", NEXT_BUTTON_X + (REDBUTTON_W / 2) + 1, REDBUTTON_Y + (REDBUTTON_H / 2));
	tft.setFreeFont(&FreeSansBold12pt7b);
	nextBtn.drawButton();
}

// Draw proof-of-concept PREV button TODO: expose coordinates
void drawPrevButton()
{
	// tft.fillRoundRect(NEXT_BUTTON_X - 100, REDBUTTON_Y, REDBUTTON_W, REDBUTTON_H, 5, TFT_BLUE);
	// tft.drawRoundRect(FRAME_X - 100, FRAME_Y, FRAME_W, FRAME_H, 5, TFT_YELLOW);
	// tft.setTextColor(TFT_WHITE);
	// tft.setTextSize(2);
	// tft.setTextDatum(MC_DATUM);
	// tft.drawString("PREV", NEXT_BUTTON_X - 100 + (REDBUTTON_W / 2) + 1, REDBUTTON_Y + (REDBUTTON_H / 2));
	tft.setFreeFont(&FreeSansBold12pt7b);
	prevBtn.drawButton();
}

// Mute button uses object
void drawMuteButton(bool invert)
{
	//tft.setFreeFont(&FreeSansBold9pt7b);
	//tft.setTextSize(1);
	muteBtn.drawButton(invert);
}

void drawMuteBitmap(bool isMuted)
{
	drawBmp(isMuted ? "/MuteIconOn.bmp" : "/MuteIconOff.bmp", 190, FRAME_Y - 5);
}

void drawBrightButton(bool invert)
{
	tft.setFreeFont(&FreeSansBold12pt7b);
	brightBtn.drawButton(invert);
}

void drawDimButton(bool invert)
{
	tft.setFreeFont(&FreeSansBold12pt7b);
	dimBtn.drawButton(invert);
}

// Draw percentage of buffering (eg 10000 buffer that uses 8000 bytes = 80%)
void drawBufferLevel(size_t bufferLevel, bool override)
{
	static unsigned long prevMillis = millis();
	static int prevBufferPerCent = 0;

	// Only do this infrequently and if the buffer % changes
	if (millis() - prevMillis > 500 || override)
	{
		// Capture current values for next time
		prevMillis = millis();

		// Calculate the percentage (ESP32 has FP processor so shoud be efficient)
		float bufLevel = (float)bufferLevel;
		float arraySize = (float)(CIRCULARBUFFERSIZE);
		int bufferPerCent = (bufLevel / arraySize) * 100.0;

		// Only update the screen on real change (avoids flicker & saves time)
		if (bufferPerCent != prevBufferPerCent || override)
		{
			// Track the buffer percentage
			prevBufferPerCent = bufferPerCent;

			// Print at specific rectangular place
			// TODO: These should not be magic numbers
			uint16_t bgColour, fgColour;
			switch (bufferPerCent)
			{
			case 0 ... 30:
				bgColour = TFT_RED;
				fgColour = TFT_WHITE;
				break;

			case 31 ... 74:
				bgColour = TFT_ORANGE;
				fgColour = TFT_BLACK;
				break;

			case 75 ... 100:
				bgColour = TFT_DARKGREEN;
				fgColour = TFT_WHITE;
				break;

			default:
				bgColour = TFT_WHITE;
				fgColour = TFT_BLACK;
			};

			tft.fillRoundRect(250, FRAME_Y, 60, 30, 5, bgColour);
			tft.drawRoundRect(250, FRAME_Y, 60, 30, 5, TFT_RED);
			tft.setTextColor(fgColour, bgColour);
			tft.setFreeFont(&FreeSans9pt7b);
			tft.setTextSize(1);
			tft.setCursor(261, FRAME_Y + 20);
			tft.printf("%d%%\n", bufferPerCent);
		}
	}
}

// On-screen NEXT button (called from main.cpp when TFT touch detected)
bool getNextButtonPress(uint16_t t_x, uint16_t t_y)
{
	// Check if any key coordinate boxes contain the touch coordinates
	if (nextBtn.contains(t_x, t_y))
	{
		//Serial.printf("Next: x=%d, y=%d\n", t_x, t_y);
		nextBtn.drawButton(true);
		return true;
	}

	return false;
}

// On screen PREV button (called from main.cpp when TFT touch detected)
bool getPrevButtonPress(uint16_t t_x, uint16_t t_y)
{
	if (prevBtn.contains(t_x, t_y))
	{
		//Serial.printf("Prev: x=%d, y=%d\n", t_x, t_y);
		prevBtn.drawButton(true);
		return true;
	}

	return false;
}

// Increase brightness
void getBrightButtonPress()
{
	static unsigned long prevMillis = millis();

	// Only do this infrequently or buffer will stop
	if (millis() - prevMillis > 150)
	{
		uint16_t t_x = 0, t_y = 0; // To store the touch coordinates

		// Update the last time we were here
		prevMillis = millis();

		// Pressed will be set true is there is a valid touch on the screen
		boolean pressed = tft.getTouch(&t_x, &t_y);

		// Check if any key coordinate boxes contain the touch coordinates
		if (pressed && brightBtn.contains(t_x, t_y))
		{
			brightBtn.press(true);
		}
		else
		{
			brightBtn.press(false);
		}

		if (brightBtn.justPressed())
		{
			drawBrightButton(true);
			Serial.println("Bright!");

			if (prevTFTBright <= 235)
			{
				prevTFTBright += 20;
				ledcWrite(0, prevTFTBright);
			}
		}
		else if (brightBtn.justReleased())
		{
			drawBrightButton(false);
		}
		
		//Store brightness level in EEPROM
		preferences.putUInt("Bright", prevTFTBright);
	}
}

// Decrease brightness
void getDimButtonPress()
{
	static unsigned long prevMillis = millis();

	// Only do this infrequently or buffer will stop
	if (millis() - prevMillis > 150)
	{
		uint16_t t_x = 0, t_y = 0; // To store the touch coordinates

		// Update the last time we were here
		prevMillis = millis();

		// Pressed will be set true is there is a valid touch on the screen
		boolean pressed = tft.getTouch(&t_x, &t_y);

		// Check if any key coordinate boxes contain the touch coordinates
		if (pressed && dimBtn.contains(t_x, t_y))
		{
			dimBtn.press(true);
		}
		else
		{
			dimBtn.press(false);
		}

		if (dimBtn.justPressed())
		{
			drawDimButton(true);
			Serial.println("Dim!");

			if (prevTFTBright >= 20)
			{
				prevTFTBright -= 20;
				ledcWrite(0, prevTFTBright);
			}
		}
		else if (dimBtn.justReleased())
		{
			drawDimButton(false);
		}

		//Store brightness level in EEPROM
		preferences.putUInt("Bright", prevTFTBright);
	}
}

// Mute button uses the Button class
static bool isMutedState = false;
void getMuteButtonPress()
{
	static unsigned long prevMillis = millis();

	// Only do this infrequently
	if (millis() - prevMillis > 150)
	{
		uint16_t t_x = 0, t_y = 0; // To store the touch coordinates

		// Update the last time we were here
		prevMillis = millis();

		// Pressed will be set true is there is a valid touch on the screen
		boolean pressed = tft.getTouch(&t_x, &t_y);

		// Check if any key coordinate boxes contain the touch coordinates
		if (pressed && muteBtn.contains(t_x, t_y))
		{
			muteBtn.press(true);
		}
		else
		{
			muteBtn.press(false);
		}

		if (muteBtn.justPressed())
		{
			isMutedState = !isMutedState;

			Serial.printf("Mute: %s", isMutedState ? "muted" : "UNmuted");
			player.setVolume(isMutedState ? 0 : 100);
			drawMuteBitmap(isMutedState);
		}
	}
}

void displayStationName(char *stationName)
{
	// Set text colour and background
	tft.setTextColor(TFT_YELLOW, TFT_BLACK);

	// Clear the remainder of the line from before (eg long title)
	tft.fillRect(0, 56, 320, 40, TFT_BLACK);

	// Write station name (as stored in the sketch)
	tft.setCursor(0, 85);
	tft.setFreeFont(&FreeSansOblique12pt7b);
	tft.setTextSize(1);
	tft.println(stationName);
}

void displayTrackArtist(std::string trackArtist)
{
	// Set text colour and background
	tft.setTextColor(TFT_GREEN, TFT_BLACK);

	// Clear the remainder of the line from before (eg long title)
	tft.fillRect(0, 100, 320, 50, TFT_BLACK);

	// Write artist / track info
	tft.setFreeFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setCursor(0, 120);
	tft.print(trackArtist.c_str());

	// Restore mute button because for this test prog we have splatted it with the artist info
	//drawMuteButton(isMutedState);
}
