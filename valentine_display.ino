#include <Elegoo_GFX.h>
#include <Elegoo_TFTLCD.h>
#include <TouchScreen.h>
#include <SD.h>
#include <SPI.h>

// =============================================================================
// USER CONFIGURATION -- Edit this section to customize the experience
// =============================================================================

// --- SD Card Image Files ---
// Place your images on the SD card (FAT32) using these naming conventions:
//
//   Animation frames (RAW, RGB565): ANIMATION_01.RAW, ANIMATION_02.RAW, ...
//       -> flashes briefly before the second YES prompt
//
//   Carousel photos (24-bit BMP):   CAROUSEL_1.BMP, CAROUSEL_2.BMP, ...
//       -> loops forever after the final YES is pressed
//
//   Final still image (24-bit BMP): FINAL.BMP
//       -> shown next to the second YES prompt
//
// File counts are auto-detected at startup -- just drop your files on the SD card.
#define ANIMATION_PREFIX   "ANIMATION_"  // uses %02d format (e.g. ANIMATION_01.RAW)
#define ANIMATION_EXT      ".RAW"
#define CAROUSEL_PREFIX    "CAROUSEL_"   // uses %d format (e.g. CAROUSEL_1.BMP)
#define CAROUSEL_EXT       ".BMP"
#define FINAL_IMAGE        "FINAL.BMP"

// --- Animation image dimensions (RAW files, RGB565) ---
#define ANIMATION_WIDTH    224
#define ANIMATION_HEIGHT   168
#define ANIMATION_X        -30          // negative values crop from the left
#define ANIMATION_Y        30

// --- Carousel timing ---
#define CAROUSEL_FRAME_MS  1500         // delay between carousel frames

// --- Final image position (after animation) ---
#define FINAL_IMAGE_X      115
#define FINAL_IMAGE_Y      45

// --- Interaction tuning ---
#define NO_PRESSES_TO_WIN  3            // how many times "NO" must be chased

// --- Text shown on each screen ---
#define TXT_MAIN_LINE1     "Will you be my"
#define TXT_MAIN_LINE2     "VALENTINE?"
#define TXT_LMAO_LINE1     "not gonna lie"
#define TXT_LMAO_LINE2     "u gotta press no lmao"
#define TXT_WIN_LINE1      "YAY!!"
#define TXT_WIN_LINE2      "meow :3"
#define TXT_ANIM_LINE1     "fine then..."
#define TXT_ANIM_LINE2     "boom"
#define TXT_AFTER_LINE1    "What about"
#define TXT_AFTER_LINE2    "now?"

// =============================================================================
// HARDWARE CONFIGURATION -- usually no need to change below here
// =============================================================================

// --- Pin Definitions ---
#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4
#define SD_CS 10

#define YP A3
#define XM A2
#define YM 9
#define XP 8

// --- Touch Calibration ---
#define TS_MINX 120
#define TS_MAXX 900
#define TS_MINY 70
#define TS_MAXY 920

// --- Performance / Buffers ---
#define BUFFPIXEL     20   // BMP draw buffer (pixels). Keep small for Arduino RAM.
#define RAW_BUFF_SIZE 64   // RAW draw buffer (pixels).

// --- Colors (RGB565) ---
#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define PINK    0xF81F
#define GREEN   0x07E0
#define YELLOW  0xFFE0

// =============================================================================
// GLOBALS
// =============================================================================

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

int noX = 180, noY = 180, yesX = 40, yesY = 180, btnW = 80, btnH = 50;
bool animationActive = false;
bool playedbefore = false;
int counter = 0;

// Auto-detected file counts (populated in setup())
int animationCount = 0;
int carouselCount  = 0;

// =============================================================================
// SETUP
// =============================================================================

void setup() {
  Serial.begin(9600);
  tft.reset();

  uint16_t identifier = 0x9341;
  tft.begin(identifier);
  tft.setRotation(1);

  if (!SD.begin(SD_CS)) {
    tft.fillScreen(RED);
    tft.setCursor(20, 100);
    tft.setTextSize(2);
    tft.println("SD Error! Check FAT32");
    while (1);
  }

  // Auto-detect how many animation and carousel frames are on the SD card
  animationCount = countFiles(ANIMATION_PREFIX, ANIMATION_EXT, true);   // %02d
  carouselCount  = countFiles(CAROUSEL_PREFIX,  CAROUSEL_EXT,  false);  // %d

  Serial.print(F("Animation frames: ")); Serial.println(animationCount);
  Serial.print(F("Carousel frames:  ")); Serial.println(carouselCount);

  randomSeed(analogRead(A5));
  drawMainScreen();
}

// =============================================================================
// MAIN LOOP
// =============================================================================

void loop() {
  if (animationActive && !playedbefore) {
    tft.setCursor(200, 80);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.println(TXT_ANIM_LINE1);
    tft.setCursor(200, 100);
    tft.println(TXT_ANIM_LINE2);

    playAnimation();

    // Show the final still image next to a second YES prompt
    bmpDraw(FINAL_IMAGE, FINAL_IMAGE_X, FINAL_IMAGE_Y);

    tft.fillRect(200, 80, 120, 120, BLACK);
    delay(50);

    tft.setCursor(200, 80);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.println(TXT_AFTER_LINE1);
    tft.setCursor(200, 100);
    tft.println(TXT_AFTER_LINE2);

    tft.fillRect(200, yesY, btnW, btnH, GREEN);
    tft.setCursor(200 + 15, yesY + 15);
    tft.setTextColor(BLACK);
    tft.setTextSize(2);
    tft.print("YES");

    playedbefore = true;
    return;
  }

  TSPoint p = ts.getPoint();
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  if (p.z > 10 && p.z < 1000 && playedbefore) {
    // Second YES button (after animation)
    int x = map(p.y, TS_MINY, TS_MAXY, 320, 0);
    int y = map(p.x, TS_MINX, TS_MAXX, 240, 0);
    if (x > 200 && x < (200 + btnW) && y > yesY && y < (yesY + btnH)) {
      winScreen();
      delay(2000);
      tft.fillScreen(BLACK);
      playCarouselLoop();  // runs forever
      animationActive = true;
    }
  }

  if (p.z > 10 && p.z < 1000 && !playedbefore) {
    int x = map(p.y, TS_MINY, TS_MAXY, 320, 0);
    int y = map(p.x, TS_MINX, TS_MAXX, 240, 0);

    // First YES button
    if (x > yesX && x < (yesX + btnW) && y > yesY && y < (yesY + btnH)) {
      drawlmao();
    }

    // NO button (teleports)
    if (x > noX && x < (noX + btnW) && y > noY && y < (noY + btnH)) {
      teleportNo();
      counter += 1;
      if (counter == NO_PRESSES_TO_WIN) {
        tft.fillScreen(BLACK);
        animationActive = true;
      }
      delay(200);
    }
  }
}

// =============================================================================
// FILE COUNTING
// =============================================================================

// Counts sequentially-numbered files on the SD card starting at index 1.
// Stops at the first missing index. `padded` controls %02d vs %d numbering.
int countFiles(const char *prefix, const char *ext, bool padded) {
  char filename[24];
  int count = 0;

  // Arduino SD library uses 8.3 filenames, so keep prefixes short.
  for (int i = 1; i <= 99; i++) {
    if (padded) {
      snprintf(filename, sizeof(filename), "%s%02d%s", prefix, i, ext);
    } else {
      snprintf(filename, sizeof(filename), "%s%d%s", prefix, i, ext);
    }
    if (SD.exists(filename)) {
      count++;
    } else {
      break;
    }
  }
  return count;
}

// =============================================================================
// ANIMATION (RAW frames shown before second YES)
// =============================================================================

void playAnimation() {
  if (animationCount <= 0) {
    Serial.println(F("No animation frames found."));
    return;
  }

  for (int i = 1; i <= animationCount; i++) {
    char filename[24];
    snprintf(filename, sizeof(filename), "%s%02d%s", ANIMATION_PREFIX, i, ANIMATION_EXT);
    drawRaw(filename, ANIMATION_X, ANIMATION_Y, ANIMATION_WIDTH, ANIMATION_HEIGHT);
  }
}

// =============================================================================
// CAROUSEL (BMP loop shown after final win)
// =============================================================================

void playCarouselLoop() {
  int screenW = 320;
  int screenH = 240;

  if (carouselCount <= 0) {
    tft.fillScreen(BLACK);
    tft.setCursor(10, 100);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.print(F("No carousel frames"));
    while (1);
  }

  while (true) {
    for (int i = 1; i <= carouselCount; i++) {
      char filename[20];
      snprintf(filename, sizeof(filename), "%s%d%s", CAROUSEL_PREFIX, i, CAROUSEL_EXT);

      File f = SD.open(filename);
      if (!f) {
        Serial.print(F("Skip: "));
        Serial.println(filename);
        continue;
      }

      // Read BMP width/height for centering
      f.seek(18);
      uint32_t w = read32(f);
      uint32_t h = read32(f);
      f.close();

      int x = (screenW - w) / 2;
      int y = (screenH - h) / 2;

      bmpDraw(filename, x, y);
      delay(CAROUSEL_FRAME_MS);
      tft.fillScreen(BLACK);
    }
  }
}

// =============================================================================
// BMP DRAWING (24-bit Windows BMP)
// =============================================================================

void bmpDraw(const char *filename, int x, int y) {
  File     bmpFile;
  int      bmpWidth, bmpHeight;
  uint32_t bmpImageoffset;
  uint32_t rowSize;
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0;

  uint8_t  sdbuffer[3 * BUFFPIXEL];
  uint16_t lcdbuffer[BUFFPIXEL];

  if ((bmpFile = SD.open(filename)) == NULL) {
    Serial.print(F("File not found: "));
    Serial.println(filename);
    return;
  }

  if (read16(bmpFile) == 0x4D42) { // 'BM'
    read32(bmpFile); read32(bmpFile);
    bmpImageoffset = read32(bmpFile);
    read32(bmpFile);
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);

    if (read16(bmpFile) == 1 && read16(bmpFile) == 24) {
      rowSize = (bmpWidth * 3 + 3) & ~3;
      w = bmpWidth;
      h = bmpHeight;

      if ((x + w - 1) >= tft.width())  w = tft.width()  - x;
      if ((y + h - 1) >= tft.height()) h = tft.height() - y;

      for (row = 0; row < h; row++) {
        pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
        bmpFile.seek(pos);

        for (col = 0; col < w; col += BUFFPIXEL) {
          int pixelsToRead = BUFFPIXEL;
          if (col + pixelsToRead > w) pixelsToRead = w - col;

          bmpFile.read(sdbuffer, pixelsToRead * 3);

          for (int i = 0; i < pixelsToRead; i++) {
            b = sdbuffer[i * 3];
            g = sdbuffer[i * 3 + 1];
            r = sdbuffer[i * 3 + 2];
            lcdbuffer[i] = tft.color565(r, g, b);
          }

          tft.setAddrWindow(x + col, y + row, x + col + pixelsToRead - 1, y + row);
          tft.pushColors(lcdbuffer, pixelsToRead, true);
        }
      }
    }
  }
  bmpFile.close();
}

// =============================================================================
// RAW DRAWING (RGB565 raw pixel data, fast)
// =============================================================================

void drawRaw(const char *filename, int x, int y, int imgW, int imgH) {
  File rawFile;
  uint8_t sdbuffer[RAW_BUFF_SIZE * 2];

  if ((rawFile = SD.open(filename)) == NULL) {
    Serial.print(F("Not found: "));
    Serial.println(filename);
    return;
  }

  // Crop logic: handle negative X (off-screen left)
  int skipLeft = 0;
  int drawX = x;
  int drawW = imgW;

  if (x < 0) {
    skipLeft = -x;
    drawX = 0;
    drawW = imgW - skipLeft;
  }

  if (drawW <= 0) { rawFile.close(); return; }

  tft.setAddrWindow(drawX, y, drawX + drawW - 1, y + imgH - 1);
  bool first = true;

  for (int row = 0; row < imgH; row++) {
    uint32_t pos = ((uint32_t)row * imgW * 2) + (skipLeft * 2);
    rawFile.seek(pos);

    int pixelsRemaining = drawW;
    while (pixelsRemaining > 0) {
      int pixelsToRead = (pixelsRemaining > RAW_BUFF_SIZE) ? RAW_BUFF_SIZE : pixelsRemaining;

      rawFile.read(sdbuffer, pixelsToRead * 2);

      // Byte swap for correct colors
      for (int k = 0; k < pixelsToRead * 2; k += 2) {
        uint8_t temp = sdbuffer[k];
        sdbuffer[k]     = sdbuffer[k + 1];
        sdbuffer[k + 1] = temp;
      }

      tft.pushColors((uint16_t *)sdbuffer, (uint8_t)pixelsToRead, first);
      first = false;

      pixelsRemaining -= pixelsToRead;
    }
  }

  rawFile.close();
}

// =============================================================================
// UI SCREENS
// =============================================================================

void drawFlower(int x, int y) {
  tft.fillCircle(x - 10, y, 8, PINK);
  tft.fillCircle(x + 10, y, 8, PINK);
  tft.fillCircle(x, y - 10, 8, PINK);
  tft.fillCircle(x, y + 10, 8, PINK);
  tft.fillCircle(x, y, 6, YELLOW);
}

void drawlmao() {
  tft.fillScreen(BLACK);
  drawFlower(40, 40);
  drawFlower(280, 40);
  tft.setCursor(30, 80);
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.println(TXT_LMAO_LINE1);
  tft.setCursor(60, 120);
  tft.setTextColor(RED);
  tft.println(TXT_LMAO_LINE2);
  drawButtons();
}

void drawMainScreen() {
  tft.fillScreen(BLACK);
  drawFlower(40, 40);
  drawFlower(280, 40);
  tft.setCursor(30, 80);
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.println(TXT_MAIN_LINE1);
  tft.setCursor(60, 120);
  tft.setTextColor(RED);
  tft.println(TXT_MAIN_LINE2);
  drawButtons();
}

void drawButtons() {
  tft.fillRect(yesX, yesY, btnW, btnH, GREEN);
  tft.setCursor(yesX + 15, yesY + 15);
  tft.setTextColor(BLACK);
  tft.setTextSize(2);
  tft.print("YES");
  tft.fillRect(noX, noY, btnW, btnH, RED);
  tft.setCursor(noX + 20, noY + 15);
  tft.setTextColor(WHITE);
  tft.print("NO");
}

void teleportNo() {
  tft.fillRect(noX, noY, btnW, btnH, BLACK);
  noX = random(10, 230);
  noY = random(10, 180);
  if (noX < 130 && noY > 150) noX += 100;
  drawButtons();
}

void winScreen() {
  tft.fillScreen(PINK);
  tft.setCursor(60, 100);
  tft.setTextColor(WHITE);
  tft.setTextSize(4);
  tft.println(TXT_WIN_LINE1);
  tft.setCursor(90, 150);
  tft.setTextSize(3);
  tft.println(TXT_WIN_LINE2);
}

// =============================================================================
// FILE HELPERS
// =============================================================================

uint16_t read16(File f) { uint16_t result; f.read(&result, 2); return result; }
uint32_t read32(File f) { uint32_t result; f.read(&result, 4); return result; }
