#include <WiFi.h>             // Include WiFi library for ESP32's WiFi functionality
#include <VS1053.h>           // Library for MP3 Decoder
#include <Adafruit_GFX.h>     // Core graphics library
#include <Adafruit_ST7735.h>  // Hardware-specific library for ST7735
#include <SPI.h>

// Define the VS1053 MP3 decoder pins
#define VS1053_CS 8    // Chip Select for VS1053
#define VS1053_DCS 3   // Data Command Select for VS1053
#define VS1053_DREQ 21  // Data Request pin for VS1053 (prev GPIO pin 9)
#define potDT 18       // Potentiometer data
#define potCLK 19      // Potentiometer clock
#define potSW 10       // Potentiometer button

// TFT Variables
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 128

// Potentiometer variables
int lastCLK = HIGH;
int lastDT = HIGH;
int currentCLK;
int currentDT;
int encoderPos = 0;
bool buttonPressed = false;
bool isMuted = false;

#if defined(ARDUINO_FEATHER_ESP32)  
#define TFT_CS 0                    
#define TFT_RST 2                   
#define TFT_DC 1                    

#elif defined(ESP8266)
#define TFT_CS 0
#define TFT_RST 2
#define TFT_DC 1

#else
// For the breakout board, you can use any 2 or 3 pins.
// These pins will also work for the 1.8" TFT shield.
#define TFT_CS 0
#define TFT_RST 2  // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC 1
#endif

// For 1.44" and 1.8" TFT with ST7735 use:
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// WiFi settings: replace with your own network credentials
const char* ssid = "?????";              // Your WiFi network name
const char* password = "?????";         // Your WiFi network password

// Radio station details                                                                           // Paths to the radio streams
const char* stationNames[] = { "FFH Lounge", "Jazz 90.1", "91.7 KXT" };                            // Array of station names
const char* stationHosts[] = { "mp3.ffh.de", "greece-media.monroe.edu", "kera.streamguys1.com" };  // Host URLs for the stations
const char* stationPaths[] = { "/ffhchannels/hqlounge.mp3", "/wgmc.mp3", "/kxtlive128" };          // Paths to the radio streams

// Radio station metadata
const char* stationMetadataLinks[] = { "api.tunegenie.com", "jazz901.traxdb.com", "mp3.ffh.de" };
const char* stationMetadataPaths[] = { "/v2/brand/nowplaying/?apiid=m2g_bar&b=kkxt&count=1", "/api/now-playing", "/metadata?type=json&cb=82016660448&details=1" };

int currentStation = 0;                                                    // Index of the currently playing station
const int totalStations = sizeof(stationNames) / sizeof(stationNames[0]);  // Calculate the number of available stations

// VS1053 MP3 player object
VS1053 player(VS1053_CS, VS1053_DCS, VS1053_DREQ);  // Create VS1053 object to control MP3 playback
WiFiClient client;                                  // WiFi client object to connect to the radio stream
WiFiClient metaclient;                              // WiFi client object to connect to radio stream metadata

// Variables for scrolling text on the OLED display
int textPosition = 128;            // Initial text position for scrolling
unsigned long previousMillis = 0;  // Store the last time the display was updated
const long interval = 50;          // Time interval for updating the display (50 ms)

void setup() {

  Serial.begin(115200);  // Start the serial monitor for debugging

  // Potentiometer init.
  pinMode(potCLK, INPUT);
  pinMode(potDT, INPUT);
  pinMode(potSW, INPUT_PULLUP);   // Enable internal pull-up for button
  lastCLK = digitalRead(potCLK);  // Read initial state for data pin
  lastDT = digitalRead(potDT);    // // Read initial state for clock pin

  // Wait for VS1053 and PAM8403 amplifier to power up
  delay(3000);

  // -----------------------------------------------------------------------------------------------------  //

  tft.initR(INITR_BLACKTAB);     // Init ST7735S chip, black tab
  tft.fillScreen(ST77XX_BLACK);  // Set screen to black
  tft.setRotation(1);            // Rotate 180 degrees
  // tft.setTextWrap(false);       // Disable text wrap
  tft.setTextColor(ST77XX_WHITE);  // Set text color
  tft.setTextSize(2);              // Set text size
  // end ST77355 init

  // Display startup messages on TFT
  tft.setCursor(0, 0);
  tft.fillScreen(ST77XX_BLACK);
  tft.println("Starting Radio...");  // Initial message
  delay(2000);

  tft.setCursor(0, 0);
  tft.fillScreen(ST77XX_BLACK);
  tft.println("Starting Engine...");  // Second message
  delay(2000);

  tft.setCursor(0, 0);
  tft.fillScreen(ST77XX_BLACK);
  tft.println("Connecting to WiFi..");  // WiFi connection message
  delay(2000);

  Serial.println("\n\User's Internet Radio!");  // Debug message in the serial monitor

  // -----------------------------------------------------------------------------------------------------  //

  SPI.begin();  // Initialize SPI communication for VS1053

  player.begin();                       // Start the VS1053 decoder
  if (player.getChipVersion() == 4) {   // Check for correct version of VS1053
    player.loadDefaultVs1053Patches();  // Load patches for MP3 decoding if needed
  }
  player.switchToMp3Mode();  // Switch VS1053 to MP3 decoding mode
  player.setVolume(100);     // Set the volume (range: 0-100)

  // -----------------------------------------------------------------------------------------------------  //

  Serial.print("Connecting to SSID: ");
  Serial.println(ssid);        // Debug message: attempting WiFi connection
  WiFi.begin(ssid, password);  // Start WiFi connection

  // Disable WiFi power saving mode for a more stable connection
  WiFi.setSleep(false);

  // Attempt to connect to WiFi with retries
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");  // Print dots to indicate connection progress
    attempts++;
  }

  // Check if WiFi connection is successful
  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());  // Print the assigned IP address

    // Display success message on OLED
    tft.setCursor(0, 0);
    tft.fillScreen(ST77XX_BLACK);
    tft.println("Connected to WiFi!");  // WiFi connection message
    delay(2000);

  } else {

    // Display failure message on OLED if WiFi connection fails
    tft.setCursor(0, 0);
    tft.fillScreen(ST77XX_BLACK);
    tft.println("Could not connect to WiFi!");  // WiFi connection message
    Serial.println("Could not connect to WiFi!");
    delay(2000);
  }

  // -----------------------------------------------------------------------------------------------------  //

  displayStation();  // Display the initial station name on the TFT
  connectToHost();   // Connect to the radio station stream
}

void loop() {
  // Reconnect if the WiFi client is disconnected
  if (!client.connected()) {
    Serial.println("Reconnecting..");
    connectToHost();  // Attempt to reconnect to the stream
  }

  // Read data from the radio stream and send it to the VS1053 decoder for playback
  if (client.available() > 0) {
    uint8_t buffer[32];
    size_t bytesRead = client.readBytes(buffer, sizeof(buffer));  // Read data from stream
    player.playChunk(buffer, bytesRead);                          // Play the received audio data
  }

  // -----------------------------------------------------------------------------------------------------  //

  // Potentiometer reads
  currentCLK = digitalRead(potCLK);
  currentDT = digitalRead(potDT);
  buttonPressed = !digitalRead(potSW);  // Button is active low

  // Check if the encoder has been rotated (CLK changed)
  if (currentCLK != lastCLK) {
    if (currentDT != currentCLK) {
      // Rotated clockwise, increment currentStation
      currentStation = (currentStation + 1) % 3;  // Cycle through 0, 1, 2
    } else {
      // Rotated counter-clockwise, decrement currentStation
      currentStation = (currentStation + 2) % 3;  // Cycle through 0, 1, 2
    }

    // Output the current station value
    Serial.print("Current Station: ");
    Serial.println(currentStation);
    displayStation();
    connectToHost();
  }

  if (buttonPressed && !isMuted) {
    isMuted = true;
    Serial.println("Muted!");
    player.setVolume(0);
    displayMuted();
    delay(500);  // Debounce delay
  } else if (buttonPressed && isMuted) {
    isMuted = false;
    Serial.println("Unmuted!");
    player.setVolume(100);
    displayStation();
    delay(500);  // Debounce delay
  }

  lastCLK = currentCLK;
  lastDT = currentDT;

  // -----------------------------------------------------------------------------------------------------  //
}

void connectToHost() {
  // Connect to the current radio station's server
  Serial.print("Connecting to ");
  Serial.println(stationHosts[currentStation]);

  if (!client.connect(stationHosts[currentStation], 80)) {
    Serial.println("Connection failed");  // Display error if connection fails
    return;
  }

  // Send HTTP request to the server to get the radio stream
  Serial.print("Requesting stream: ");
  Serial.println(stationPaths[currentStation]);

  client.print(String("GET ") + stationPaths[currentStation] + " HTTP/1.1\r\n" + "Host: " + stationHosts[currentStation] + "\r\n" + "Connection: close\r\n\r\n");
  metaclient.print(String("GET ") + stationMetadataPaths[currentStation] + " HTTP/1.1\r\n" + "Host: " + stationMetadataLinks[currentStation] + "\r\n" + "Connection: close\r\n\r\n");

  // Skip the HTTP headers in the response
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;  // End of headers
    }
  }

  // Skip the HTTP headers in the response
  String metaline;
  while (metaclient.connected()) {
    metaline = metaclient.readStringUntil('\n');
    if (metaline == "\r") {
      break;  // End of headers
    }
  }

  Serial.println("Song Metadata: ");
  Serial.print(metaline);

  Serial.println("Headers received");  // Debug message: headers successfully received
}

// void displayStation() {
//   tft.setCursor(0, 0);
//   tft.fillScreen(ST77XX_BLACK);
//   tft.setTextColor(ST77XX_WHITE);
//   tft.setTextSize(2);
//   tft.println(stationNames[currentStation]);  // Print current station
// }

void displayStation() {
  int screenWidth = tft.width();
  int screenHeight = tft.height();
  int textSize = 2;
  int textHeight = 8 * textSize;  // Approximate height of text for size 2
  int textWidth = strlen(stationNames[currentStation]) * 6 * textSize;  // Estimate width of text for size 2

  tft.fillScreen(ST77XX_BLACK);
  tft.fillRect(0, (screenHeight - textHeight) / 2 - 10, screenWidth, 10, ST77XX_WHITE);
  tft.fillRect(0, (screenHeight + textHeight) / 2, screenWidth, 10, ST77XX_WHITE);

  tft.setCursor((screenWidth - textWidth) / 2, (screenHeight - textHeight) / 2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(textSize);
  tft.println(stationNames[currentStation]);
}

void displayMuted() {
  tft.setCursor(0, 0);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(3);
  tft.println("Muted!");  // Print current station
}
