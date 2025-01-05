#include <Arduino.h>

//#define LILYGO_T5_V213
#define E_Paper_ESP32_Driver_Board


#include <boards.h>
#include <GxEPD2.h>
#include <SD.h>
#include <FS.h>
#define SdFile File
#define seekSet seek

//#include <GxDEPG0213BN/GxDEPG0213BN.h>    // 2.13" b/w  form DKE GROUP

//#include <GxGDEW0583T7/GxGDEW0583T7.h>    // 5.83" b/w  648x480 FPC-8601B
//#include <GxGDEW075T8/GxGDEW075T8.h>    // 5.8"???????? b/w   from Dalian Good Display Co.

//#include <GxDEPG0750BN/GxDEPG0750BN.h>    // Не работает должным образом 7.5" b/w   form DKE GROUP
//#include <GxGDEW075T7/GxGDEW075T7.h>    // Не работает должным образом 7.5" b/w   from Dalian Good Display Co.
#include <GxGDEW075Z08/GxGDEW075Z08.h>    // 7.5" GDEW075Z08 e-Paper from Dalian Good Display Co.



#include GxEPD_BitmapExamples

// FreeFonts from Adafruit_GFX
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include <WiFi.h>


#if defined(E_Paper_ESP32_Driver_Board)
    GxIO_Class io(EPD_SCLK, EPD_MISO, EPD_MOSI,  EPD_CS, EPD_DC,  EPD_RSET);
    GxEPD_Class display(io, EPD_RSET, EPD_BUSY);
#else
    GxIO_Class io(SPI,  EPD_CS, EPD_DC,  EPD_RSET);
    GxEPD_Class display(io, EPD_RSET, EPD_BUSY);
#endif

#if defined(_HAS_SDCARD_) && !defined(_USE_SHARED_SPI_BUS_)
SPIClass SDSPI(VSPI);
#endif




bool rlst = false;
void LilyGo_logo();
bool setupSDCard(void);
void drawBitmapFromSD(const char *filename, int16_t x, int16_t y, bool with_color);
void drawBitmaps_test();
uint32_t read32(SdFile &f);
uint16_t read16(SdFile &f);
void drawBitmapFromSD(const char *filename, int16_t x, int16_t y, bool with_color = true);
void drawBitmapFrom_SD_ToBuffer(const char *filename, int16_t x, int16_t y, bool with_color);


void drawBitmaps_test()
{
    //int16_t w2 = display.width() / 2;
    // int16_t h2 = display.height() / 2;
    display.setRotation(0);
    drawBitmapFromSD("10d@2x.bmp", 0, 0);
    delay(2000);
    drawBitmapFromSD("output5.bmp", 0, 0);
    delay(2000);
    drawBitmapFromSD("betty_1.bmp", 0, 0);
    delay(2000);
    drawBitmapFromSD("first128x80.bmp", 0, 0);
    delay(2000);
    drawBitmapFromSD("134529.bmp", -50, 0);
    delay(2000);

}


void drawBitmapFromSD(const char *filename, int16_t x, int16_t y, bool with_color)
{
    drawBitmapFrom_SD_ToBuffer(filename, x, y, with_color);
    display.update();
}



bool setupSDCard(void)
{
#if defined(_HAS_SDCARD_) && !defined(_USE_SHARED_SPI_BUS_)
    SDSPI.begin(SDCARD_SCLK, SDCARD_MISO, SDCARD_MOSI);
    return SD.begin(SDCARD_CS, SDSPI);
#elif defined(_HAS_SDCARD_)
    return SD.begin(SDCARD_CS);
#endif
    return false;
}









void LilyGo_logo(void)
{
    display.setRotation(0);
    display.fillScreen(GxEPD_WHITE);
    display.drawExampleBitmap(BitmapExample1, 0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_BLACK);

#if defined(_HAS_SDCARD_)
    display.setRotation(1);
    display.setTextColor(GxEPD_BLACK);
    display.setFont();

    display.setCursor(20, display.height() - 15);
#endif
    String sizeString = "SD:" + String(SD.cardSize() / 1024.0 / 1024.0 / 1024.0) + "G";
    display.println(rlst ? sizeString : "SD:N/A");

    int16_t x1, x2;
    uint16_t w, h;
    String str = GxEPD_BitmapExamplesQ;
    str = str.substring(2, str.lastIndexOf("/"));
    display.getTextBounds(str, 0, 0, &x1, &x2, &w, &h);
    display.setCursor(display.width() - w, display.height() - 15);
    display.println(str);

    display.update();
    
}


static const uint16_t input_buffer_pixels = 20; // may affect performance

static const uint16_t max_palette_pixels = 256; // for depth <= 8

uint8_t input_buffer[3 * input_buffer_pixels]; // up to depth 24
uint8_t mono_palette_buffer[max_palette_pixels / 8]; // palette buffer for depth <= 8 b/w
uint8_t color_palette_buffer[max_palette_pixels / 8]; // palette buffer for depth <= 8 c/w

void drawBitmapFrom_SD_ToBuffer(const char *filename, int16_t x, int16_t y, bool with_color)
{
    SdFile file;
    bool valid = false; // valid format to be handled
    bool flip = true; // bitmap is stored bottom-to-top
    uint32_t startTime = millis();
    if ((x >= display.width()) || (y >= display.height())) return;
    Serial.println();
    Serial.print("Loading image '");
    Serial.print(filename);
    Serial.println('\'');

    file = SD.open(String("/") + filename, FILE_READ);
    if (!file) {
        Serial.print("File not found");
        return;
    }

    // Parse BMP header
    if (read16(file) == 0x4D42) { // BMP signature
        uint32_t fileSize = read32(file);
        uint32_t creatorBytes = read32(file);
        uint32_t imageOffset = read32(file); // Start of image data
        uint32_t headerSize = read32(file);
        uint32_t width  = read32(file);
        uint32_t height = read32(file);
        uint16_t planes = read16(file);
        uint16_t depth = read16(file); // bits per pixel
        uint32_t format = read32(file);
        if ((planes == 1) && ((format == 0) || (format == 3))) { // uncompressed is handled, 565 also
            Serial.print("File size: "); Serial.println(fileSize);
            Serial.print("Image Offset: "); Serial.println(imageOffset);
            Serial.print("Header size: "); Serial.println(headerSize);
            Serial.print("Bit Depth: "); Serial.println(depth);
            Serial.print("Image size: ");
            Serial.print(width);
            Serial.print('x');
            Serial.println(height);
            // BMP rows are padded (if needed) to 4-byte boundary
            uint32_t rowSize = (width * depth / 8 + 3) & ~3;
            if (depth < 8) rowSize = ((width * depth + 8 - depth) / 8 + 3) & ~3;
            if (height < 0) {
                height = -height;
                flip = false;
            }
            uint16_t w = width;
            uint16_t h = height;
            if ((x + w - 1) >= display.width())  w = display.width()  - x;
            if ((y + h - 1) >= display.height()) h = display.height() - y;
            valid = true;
            uint8_t bitmask = 0xFF;
            uint8_t bitshift = 8 - depth;
            uint16_t red, green, blue;
            bool whitish, colored;
            if (depth == 1) with_color = false;
            if (depth <= 8) {
                if (depth < 8) bitmask >>= depth;
                //file.seekSet(54); //palette is always @ 54
                file.seekSet(imageOffset - (4 << depth)); // 54 for regular, diff for colorsimportant
                for (uint16_t pn = 0; pn < (1 << depth); pn++) {
                    blue  = file.read();
                    green = file.read();
                    red   = file.read();
                    file.read();
                    whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                    colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
                    if (0 == pn % 8) mono_palette_buffer[pn / 8] = 0;
                    mono_palette_buffer[pn / 8] |= whitish << pn % 8;
                    if (0 == pn % 8) color_palette_buffer[pn / 8] = 0;
                    color_palette_buffer[pn / 8] |= colored << pn % 8;
                }
            }
            display.fillScreen(GxEPD_WHITE);
            uint32_t rowPosition = flip ? imageOffset + (height - h) * rowSize : imageOffset;
            for (uint16_t row = 0; row < h; row++, rowPosition += rowSize) { // for each line
                uint32_t in_remain = rowSize;
                uint32_t in_idx = 0;
                uint32_t in_bytes = 0;
                uint8_t in_byte = 0; // for depth <= 8
                uint8_t in_bits = 0; // for depth <= 8
                uint16_t color = GxEPD_WHITE;
                file.seekSet(rowPosition);
                for (uint16_t col = 0; col < w; col++) { // for each pixel
                    // Time to read more pixel data?
                    if (in_idx >= in_bytes) { // ok, exact match for 24bit also (size IS multiple of 3)
                        in_bytes = file.read(input_buffer, in_remain > sizeof(input_buffer) ? sizeof(input_buffer) : in_remain);
                        in_remain -= in_bytes;
                        in_idx = 0;
                    }
                    switch (depth) {
                    case 24:
                        blue = input_buffer[in_idx++];
                        green = input_buffer[in_idx++];
                        red = input_buffer[in_idx++];
                        whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                        colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
                        break;
                    case 16: {
                        uint8_t lsb = input_buffer[in_idx++];
                        uint8_t msb = input_buffer[in_idx++];
                        if (format == 0) { // 555
                            blue  = (lsb & 0x1F) << 3;
                            green = ((msb & 0x03) << 6) | ((lsb & 0xE0) >> 2);
                            red   = (msb & 0x7C) << 1;
                        } else { // 565
                            blue  = (lsb & 0x1F) << 3;
                            green = ((msb & 0x07) << 5) | ((lsb & 0xE0) >> 3);
                            red   = (msb & 0xF8);
                        }
                        whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                        colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
                    }
                    break;
                    case 1:
                    case 4:
                    case 8: {
                        if (0 == in_bits) {
                            in_byte = input_buffer[in_idx++];
                            in_bits = 8;
                        }
                        uint16_t pn = (in_byte >> bitshift) & bitmask;
                        whitish = mono_palette_buffer[pn / 8] & (0x1 << pn % 8);
                        colored = color_palette_buffer[pn / 8] & (0x1 << pn % 8);
                        in_byte <<= depth;
                        in_bits -= depth;
                    }
                    break;
                    }
                    if (whitish) {
                        color = GxEPD_WHITE;
                    } else if (colored && with_color) {
                        color = GxEPD_RED;
                    } else {
                        color = GxEPD_BLACK;
                    }
                    uint16_t yrow = y + (flip ? h - row - 1 : row);
                    display.drawPixel(x + col, yrow, color);
                } // end pixel
            } // end line
            Serial.print("loaded in "); Serial.print(millis() - startTime); Serial.println(" ms");
        }
    }
    file.close();
    if (!valid) {
        Serial.println("bitmap format not handled.");
    }
}



uint16_t read16(SdFile &f)
{
    // BMP data is stored little-endian, same as Arduino.
    uint16_t result;
    ((uint8_t *)&result)[0] = f.read(); // LSB
    ((uint8_t *)&result)[1] = f.read(); // MSB
    return result;
}

uint32_t read32(SdFile &f)
{
    // BMP data is stored little-endian, same as Arduino.
    uint32_t result;
    ((uint8_t *)&result)[0] = f.read(); // LSB
    ((uint8_t *)&result)[1] = f.read();
    ((uint8_t *)&result)[2] = f.read();
    ((uint8_t *)&result)[3] = f.read(); // MSB
    return result;
}







void page_wifi_scan(int x, int y, String str)
{
    display.setRotation(0);
    display.fillScreen(GxEPD_WHITE);

    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMonoBold12pt7b);

    display.setCursor(x, y);
    display.println(str);
        
}



void testWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    String WiFi_scan_result;

    Serial.println("scan done");
    if (n == 0) {
        Serial.println("no networks found");
        WiFi_scan_result = "no networks found";
    } else {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
            delay(10);

            // WiFi_scan_result = i + ": "+WiFi.SSID(i)+" ("+WiFi.RSSI(i)+")"+ ((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
            //page_wifi_scan(20, i+10, WiFi_scan_result);
            
        }
    }
    WiFi_scan_result = 1 + ": "+WiFi.SSID(1)+" ("+WiFi.RSSI(1)+")"+ ((WiFi.encryptionType(1) == WIFI_AUTH_OPEN) ? " " : "*");
    page_wifi_scan(20, 20, WiFi_scan_result);
    Serial.println("");
    
}





















void setup(void)
{
    Serial.begin(115200);
    Serial.println(" ");
    Serial.println(F("******************* DISPLAY BRGIN *******************"));
    Serial.println(" ");

    SPI.begin(EPD_SCLK, EPD_MISO, EPD_MOSI);
    display.init(); // enable diagnostic output on Serial

    #ifdef LILYGO_T5_V213
    rlst = setupSDCard();
    #endif

    LilyGo_logo();

    delay(2000);


    // display.setRotation(0);
    // display.fillScreen(GxEPD_WHITE);
    // // testWiFi();
    
    // display.update();
    // ;
    
    display.setRotation(0);
    display.setFont(&FreeMonoBold12pt7b);
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(20, 30);
    display.println("DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD");
    display.setCursor(20, 50);
    display.println("DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD");
    display.update();


    display.powerDown();

    Serial.println(" ");
    Serial.println(F("******************* BEGIN IS OK *********************"));
}



















void loop()
{

    //  drawBitmaps_test();
    // delay(2000);
}



