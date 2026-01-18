#include "Arduino.h"
#include "info.h"
#include "zmodem_config.h"
#include "zmodem.h"
#include "zmodem_zm.h"

// Hardware instelling voor Teensy 4 Shield V5
#include <MatrixHardware_Teensy4_ShieldV5.h> 
#include <SmartMatrix.h>
#include <SdFat.h>  
#include <TimeLib.h> 
#include <EEPROM.h>  // Voor het onthouden van instellingen
#include "GifDecoder.h"
#include "FilenameFunctions.h"

bool showDate = false;           // Wisseltussen tijd en datum
unsigned long lastSwitchTime = 0; // Timer voor het wisselen
int switchInterval = 10000;      // Wissel elke 10 seconden (10000 ms)

SdFs sd;

#define DISPLAY_TIME_SECONDS 10
#define error(s) sd.errorHalt(s)
#define DSERIALprintln(_p) ({ DSERIAL.println(_p); }) 
#define fname (&oneKbuf[512])

extern int Filesleft;
extern long Totalleft;

FsFile fout;

int num_files;
int defaultBrightness = 30; 

// --- KLOK VARIABELEN ---
bool showClock = false;
unsigned long lastClockUpdate = 0;
rgb24 clockColor = {255, 120, 0}; 
rgb24 frameColor = {255, 120, 0}; 
rgb24 fillColor = {255, 255, 255}; 
rgb24 shadowColor = {40, 15, 0}; 
int timeOffset = 1; 

rgb24 tColour = {0x00, 0xff, 0x00};
ScrollMode  tMode = wrapForward;   
int tSpeed = 10; 
fontChoices  tFont = font3x5; 
char tText[50] = "Undefined text message";
int tLoopCount = 1;

const rgb24 COLOR_BLACK = { 0, 0, 0 };

// --- MATRIX CONFIGURATIE ---
#define COLOR_DEPTH 24                
const uint16_t kMatrixWidth = 128;        
const uint16_t kMatrixHeight = 32;       
const uint8_t kRefreshDepth = 36;       
const uint8_t kDmaBufferRows = 4;       
const uint8_t kPanelType = SMARTMATRIX_HUB75_32ROW_MOD16SCAN; 
const uint32_t kMatrixOptions = (SMARTMATRIX_OPTIONS_NONE);

SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, SM_BACKGROUND_OPTIONS_NONE);
SMARTMATRIX_ALLOCATE_SCROLLING_LAYER(scrollingLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, SM_SCROLLING_OPTIONS_NONE);

GifDecoder<kMatrixWidth, kMatrixHeight, 12> decoder;

// --- EEPROM OPSLAG ---
void saveSettings() {
    EEPROM.put(10, clockColor);
    EEPROM.put(20, frameColor);
    EEPROM.put(30, fillColor);
    EEPROM.put(40, defaultBrightness);
    DSERIALprintln(F("Settings saved to EEPROM!"));
}

void loadSettings() {
    int checkVal;
    EEPROM.get(40, checkVal);
    if (checkVal > 0 && checkVal <= 255) {
        EEPROM.get(10, clockColor);
        EEPROM.get(20, frameColor);
        EEPROM.get(30, fillColor);
        defaultBrightness = checkVal;
    }
}

// --- CALLBACK FUNCTIES ---
void screenClearCallback(void) { backgroundLayer.fillScreen({0, 0, 0}); }
void updateScreenCallback(void) { backgroundLayer.swapBuffers(); }
void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
  backgroundLayer.drawPixel(x, y, {red, green, blue});
}

// --- KLOK FUNCTIE ---
void drawRaspberryXL(int x, int y) {
    rgb24 berryRed = {220, 0, 0};
    rgb24 leafGreen = {0, 220, 0};
    backgroundLayer.fillTriangle(x+5, y+4, x+8, y+1, x+11, y+4, leafGreen);
    backgroundLayer.fillTriangle(x+13, y+4, x+16, y+1, x+19, y+4, leafGreen);
    backgroundLayer.fillRectangle(x+11, y+3, x+13, y+5, leafGreen);
    backgroundLayer.fillCircle(x+9, y+10, 5, berryRed);  
    backgroundLayer.fillCircle(x+15, y+10, 5, berryRed); 
    backgroundLayer.fillCircle(x+9, y+17, 5, berryRed);  
    backgroundLayer.fillCircle(x+15, y+17, 5, berryRed); 
    backgroundLayer.fillCircle(x+12, y+22, 4, berryRed); 
    backgroundLayer.drawPixel(x+9, y+10, COLOR_BLACK);
    backgroundLayer.drawPixel(x+15, y+10, COLOR_BLACK);
    backgroundLayer.drawPixel(x+12, y+15, COLOR_BLACK);
    backgroundLayer.drawPixel(x+9, y+18, COLOR_BLACK);
    backgroundLayer.drawPixel(x+15, y+18, COLOR_BLACK);
}

void displayClock() {
    if (millis() - lastClockUpdate < 1000) return;
    lastClockUpdate = millis();

    if (millis() - lastSwitchTime > (unsigned long)switchInterval) {
        showDate = !showDate;
        lastSwitchTime = millis();
    }

    backgroundLayer.fillScreen(COLOR_BLACK);
    backgroundLayer.drawRectangle(0, 0, kMatrixWidth - 1, kMatrixHeight - 1, frameColor);
    backgroundLayer.drawRectangle(1, 1, kMatrixWidth - 2, kMatrixHeight - 2, frameColor);

    drawRaspberryXL(4, 3);
    drawRaspberryXL(100, 3);

    time_t t = now();
    char buffer[16];
    int xPos;

    if (showDate) {
        const char *months[] = {"JAN", "FEB", "MRT", "APR", "MEI", "JUN", "JUL", "AUG", "SEP", "OKT", "NOV", "DEC"};
        sprintf(buffer, "%02d %s %d", day(t), months[month(t)-1], year(t));
        backgroundLayer.setFont(font6x10); 
        xPos = 29; 
    } else {
        sprintf(buffer, "%02d:%02d:%02d", hour(t), minute(t), second(t));
        backgroundLayer.setFont(font8x13);
        xPos = 31;
    }

    int yPos = (kMatrixHeight - 13) / 2;
    rgb24 shadow = rgb24(clockColor.red/8, clockColor.green/8, clockColor.blue/8);

    backgroundLayer.drawString(xPos + 2, yPos + 2, shadow, buffer);
    backgroundLayer.drawString(xPos - 1, yPos, clockColor, buffer);
    backgroundLayer.drawString(xPos + 1, yPos, clockColor, buffer);
    backgroundLayer.drawString(xPos, yPos - 1, clockColor, buffer);
    backgroundLayer.drawString(xPos, yPos + 1, clockColor, buffer);
    
    backgroundLayer.drawString(xPos, yPos, fillColor, buffer);

    backgroundLayer.swapBuffers();
}

size_t DSERIALprint(const __FlashStringHelper *ifsh) {
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  size_t n = 0;
  while (1) {
    unsigned char c = pgm_read_byte(p++);
    if (c == 0) break;
    if (DSERIAL.availableForWrite() > SERIAL_TX_BUFFER_SIZE / 2) DSERIAL.flush();
    if (DSERIAL.write(c)) n++;
    else break;
  }
  return n;
}

// NU BIJGEWERKT: Alles wat in loop() staat wordt hier getoond
void help(void) {
  DSERIALprintln(F("--------------------------------------------------"));
  DSERIALprint(Progname);
  DSERIALprint(F(" - Speed: ")); DSERIAL.println(ZMODEM_SPEED);
  DSERIALprintln(F("--------------------------------------------------"));
  DSERIALprintln(F("Display:"));
  DSERIALprintln(F(" ON, OFF           - Scherm aan/uit"));
  DSERIALprintln(F(" CLOCK             - Wissel GIF / Klok"));
  DSERIALprintln(F(" BRI [0-255]       - Helderheid"));
  DSERIALprintln(F(" SAVE              - Instellingen in EEPROM"));
  DSERIALprintln(F("--------------------------------------------------"));
  DSERIALprintln(F("Kleuren:"));
  DSERIALprintln(F(" COLOR [R G B]     - Klok cijfers"));
  DSERIALprintln(F(" FILLCOLOR [R G B] - Klok vulling"));
  DSERIALprintln(F(" FRAMECOLOR [R G B]- Kader kleur"));
  DSERIALprintln(F("--------------------------------------------------"));
  DSERIALprintln(F("Media & Tekst:"));
  DSERIALprintln(F(" DIS [bestand.gif] - Speel GIF"));
  DSERIALprintln(F(" RND               - Willekeurige GIFs"));
  DSERIALprintln(F(" TXT [jouw tekst]  - Scroll tekst"));
  DSERIALprintln(F("--------------------------------------------------"));
  DSERIALprintln(F("Bestand & Systeem:"));
  DSERIALprintln(F(" LS / DIR / PWD    - Bestandsbeheer"));
  DSERIALprintln(F(" CD [map] / DEL    - Navigatie / Verwijderen"));
  DSERIALprintln(F(" RZ / SZ [file]    - ZModem ontvang/stuur"));
  DSERIALprintln(F(" T[timestamp]      - Tijd synchronisatie"));
  DSERIALprintln(F("--------------------------------------------------"));
}

void setup() {
  ZSERIAL.begin(ZMODEM_SPEED);
  ZSERIAL.setTimeout(TYPICAL_SERIAL_TIMEOUT);
  delay(400);

  setSyncProvider([]() { return (time_t)Teensy3Clock.get(); });
  if (timeStatus() != timeSet) {
      setTime(Teensy3Clock.get()); 
  }

  loadSettings(); 

  if (!sd.begin(SdioConfig(FIFO_SDIO))) sd.initErrorHalt(&DSERIAL);
  if (!sd.chdir("/")) sd.errorHalt(F("sd.chdir"));

  decoder.setScreenClearCallback(screenClearCallback);
  decoder.setUpdateScreenCallback(updateScreenCallback);
  decoder.setDrawPixelCallback(drawPixelCallback);
  decoder.setFileSeekCallback(fileSeekCallback);
  decoder.setFilePositionCallback(filePositionCallback);
  decoder.setFileReadCallback(fileReadCallback);
  decoder.setFileReadBlockCallback(fileReadBlockCallback);

  matrix.addLayer(&backgroundLayer);
  matrix.addLayer(&scrollingLayer);
  matrix.setBrightness(defaultBrightness);
  matrix.setRefreshRate(60);
  matrix.begin();

  backgroundLayer.fillScreen(COLOR_BLACK);
  backgroundLayer.swapBuffers(false);

  scrollingLayer.setColor({0x00, 0xff, 0x00});
  scrollingLayer.setMode(wrapForward);
  scrollingLayer.setSpeed(35); 
  scrollingLayer.setFont(font3x5);
  scrollingLayer.start("*** RETROPIE ARCADE SYSTEM READY ***", 1); 
  
  delay(8500);
  help();

  if (openGifFilenameByFilename("/mariobros.gif") >= 0) {
      backgroundLayer.fillScreen(COLOR_BLACK);
      backgroundLayer.swapBuffers();
      decoder.startDecoding();
  }
}

void loop(void) {
  char *cmd = oneKbuf;
  char *param;
  static unsigned long futureTime;
  char curPath[40];

  *cmd = 0;
  while (DSERIAL.available()) DSERIAL.read();

  char c = 0;
  while (1) {
    if (DSERIAL.available() > 0) {
      c = DSERIAL.read();
      if ((c == 8 || c == 127) && strlen(cmd) > 0) cmd[strlen(cmd) - 1] = 0;
      if (c == '\n' || c == '\r') break;
      DSERIAL.write(c);
      if (c != 8 && c != 127) strncat(cmd, &c, 1);
    } else { delay(20); }

    if (showClock) { displayClock(); } 
    else { decoder.decodeFrame(); }
  }

  param = strchr(cmd, 32);
  if (param > 0) { *param = 0; param = param + 1; }
  else { param = &cmd[strlen(cmd)]; }

  strupr(cmd);
  DSERIAL.println();

  if (!strcmp_P(cmd, PSTR("HELP"))) { help(); }
  else if (!strcmp_P(cmd, PSTR("SAVE"))) { saveSettings(); } 
  else if (!strcmp_P(cmd, PSTR("CLOCK"))) {
    showClock = !showClock;
    DSERIALprint(F("Clock mode: ")); DSERIALprintln(showClock ? "ON" : "OFF");
  }
  else if (!strcmp_P(cmd, PSTR("COLOR"))) {
    char* rStr = strtok(param, " ");
    char* gStr = strtok(NULL, " ");
    char* bStr = strtok(NULL, " ");
    if(rStr && gStr && bStr) {
        clockColor = rgb24((uint8_t)atoi(rStr), (uint8_t)atoi(gStr), (uint8_t)atoi(bStr));
        DSERIALprintln(F("Clock color updated!"));
    }
  } 
  else if (!strcmp_P(cmd, PSTR("FRAMECOLOR"))) {
    char* rStr = strtok(param, " ");
    char* gStr = strtok(NULL, " ");
    char* bStr = strtok(NULL, " ");
    if(rStr && gStr && bStr) {
        frameColor = rgb24((uint8_t)atoi(rStr), (uint8_t)atoi(gStr), (uint8_t)atoi(bStr));
        DSERIALprintln(F("Frame color updated!"));
    }
  }
  else if (!strcmp_P(cmd, PSTR("FILLCOLOR"))) {
    char* rStr = strtok(param, " ");
    char* gStr = strtok(NULL, " ");
    char* bStr = strtok(NULL, " ");
    if(rStr && gStr && bStr) {
        fillColor = rgb24((uint8_t)atoi(rStr), (uint8_t)atoi(gStr), (uint8_t)atoi(bStr));
        DSERIALprintln(F("Fill color updated!"));
    }
  }
  else if (cmd[0] == 'T' && isdigit(cmd[1])) {
    unsigned long pctime = atol(&cmd[1]);
    if (pctime >= 1357041600) { 
      setTime(pctime); 
      Teensy3Clock.set(pctime); 
      DSERIALprintln(F("Time Synchronized and RTC Updated!")); 
    }
  }
  else if (!strcmp_P(cmd, PSTR("DIR")) || !strcmp_P(cmd, PSTR("LS"))) {
    char filename[40];
    FsFile cwd = sd.open(".");
    cwd.getName(fname, 13);
    cwd.close();
    snprintf(curPath, sizeof(curPath), "/%s/", fname);
    DSERIALprint(F("Directory Listing for: ")); DSERIALprintln(curPath);
    FsFile directory = sd.open(curPath);
    FsFile file;
    while (file.openNext(&directory)) {
      file.getName(filename, sizeof(filename));
      DSERIALprintln(filename);
      file.close();
    }
    directory.close();
  }
  else if (!strcmp_P(cmd, PSTR("BRI"))) {
    defaultBrightness = atoi(param);
    matrix.setBrightness(defaultBrightness);
    DSERIALprint(F("Brightness set to: ")); DSERIAL.println(defaultBrightness);
  }
  else if (!strcmp_P(cmd, PSTR("OFF"))) {
    matrix.setBrightness(0); 
    backgroundLayer.fillScreen(COLOR_BLACK);
    backgroundLayer.swapBuffers();
    DSERIALprintln(F("Display disabled (LEDs OFF)"));
  }
  else if (!strcmp_P(cmd, PSTR("ON"))) {
    matrix.setBrightness(defaultBrightness);
    backgroundLayer.swapBuffers();
    DSERIALprint(F("Display enabled."));
  }
  else if (!strcmp_P(cmd, PSTR("PWD"))) {
    FsFile cwd = sd.open(".");
    cwd.getName(fname, 13);
    cwd.close();
    DSERIALprint(F("Current directory: ")); DSERIAL.println(fname);
  }
  else if (!strcmp_P(cmd, PSTR("CD"))) {
    if (!sd.chdir(param)) { DSERIALprintln(F("Directory not found")); }
    else { DSERIALprint(F("Changed to: ")); DSERIAL.println(param); }
  }
  else if (!strcmp_P(cmd, PSTR("DEL"))) {
    if (!sd.remove(param)) { DSERIALprintln(F("Delete failed")); }
    else { DSERIALprint(F("Deleted: ")); DSERIAL.println(param); }
  }
  else if (!strcmp_P(cmd, PSTR("DIS"))) {
    showClock = false;
    char filename[40];
    FsFile cwd = sd.open(".");
    cwd.getName(fname, 13);
    cwd.close();
    snprintf(filename, sizeof(filename), "/%s/%s", fname, param);
    if (openGifFilenameByFilename(filename) >= 0) {
      matrix.setBrightness(defaultBrightness);
      decoder.startDecoding();
      DSERIALprint(F("Now playing: ")); DSERIAL.println(param);
    } else { DSERIALprintln(F("File not found!")); }
  }
  else if (!strcmp_P(cmd, PSTR("RND"))) {
    DSERIALprintln(F("Starting Random Mode..."));
    num_files = enumerateGIFFiles("/", false);
    if (num_files > 0) {
      randomSeed(micros());
      futureTime = 0; 
      while (1) {
        if (millis() > futureTime) {
          int index = random(num_files);
          if (openGifFilenameByIndex("/", index) >= 0) {
            decoder.startDecoding();
            futureTime = millis() + (DISPLAY_TIME_SECONDS * 1000);
          }
        }
        decoder.decodeFrame();
        if (DSERIAL.available() > 0) {
          char rc = DSERIAL.read();
          if (rc == '\n' || rc == '\r') break;
        }
      }
    }
  }
  else if (!strcmp_P(cmd, PSTR("TXT"))) {
    showClock = false;
    scrollingLayer.setColor(tColour);
    scrollingLayer.start(param, tLoopCount);
  }
  else if (!strcmp_P(cmd, PSTR("RZ"))) {
    wcreceive(0, 0);
  }
  else if (!strcmp_P(cmd, PSTR("SZ"))) {
    if (fout.open(param, O_READ)) {
      Filesleft = 1; Totalleft = fout.fileSize();
      ZSERIAL.print(F("rz\r")); sendzrqinit(); delay(200);
      wcs(param); saybibi(); fout.close();
    }
  }
  else { DSERIALprintln(F("Unknown command.")); }
}
