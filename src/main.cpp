#include <Arduino.h>
#include <TimedAction.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <ESP8266wifi.h>
#include <WiFiUdp.h>
#include <fastLED.h>
#include <NTPClient.h>
#include "arduino_secrets.h"
#include "XYMatrix.h"
#include "WifiFunctions.h"
#include "words.h"

#define STATUS_PIN D4 // set led pin
#define LED_PIN D7
#define CHIPSET WS2811
#define COLOR_ORDER GRB
#define BRIGHTNESS 128

#define GRID_SIZE (kMatrixWidth * kMatrixHeight)
#define SURROUNDING_LEDS 2
#define NUM_LEDS (GRID_SIZE + 2 * SURROUNDING_LEDS)

size_t DEFAULT_STACK_SIZE = 128;

char ssid[] = SECRET_SSID;   // your network SSID (name)
char pass[] = SECRET_PASS;   // your network password (use for WPA, or use as key for WEP)
char ssidap[] = "wordclock"; // your AP network SSID (name)

int status = WL_IDLE_STATUS; // the Wifi radio's status

unsigned int localPort = 2390; // local port to listen for UDP packets

TimeChangeRule myCEST = {"CEST", Last, Sun, Mar, 2, 120}; // CEST
TimeChangeRule myCET = {"CET", Last, Sun, Oct, 3, 60};    // CET
Timezone myTZ(myCEST, myCET);                             // Timezonerule

TimeChangeRule *tcr; // pointer to the time change rule, use to get TZ abbrev

CRGB leds[NUM_LEDS];

WiFiUDP Udp; // A UDP instance to let us send and receive packets over UDP
NTPClient timeClient(Udp);

enum clockAction
{
  turnOn,
  turnOff,
  keep
};
bool clockIsOn = true;

time_t prevDisplay = 0;    // when the digital clock was displayed
time_t currentDisplay = 0; // current display time

unsigned long sendNTPpacket(IPAddress &address);
void digitalClockDisplay(time_t t, const char *tz);

void setLedsclock(time_t t, CRGB color);
void showTime(time_t t);
void resetAllLeds();
void setLightGrid(int number, CRGB color);
void resetLightGrid();
void setLightGridRow(int from, int to, CRGB color);

void setPossibleTime();
void setClock();

void setMinutes(time_t t, CRGB color);
void setHours(time_t t, CRGB color);
void setMinutesLeds(int ledNumber, CRGB color);
void setLightGrid(int row, int col, CRGB color);

int ledNumbers[4] = {1, 0, GRID_SIZE + 2, GRID_SIZE + 3};

CRGB wifiColor = CRGB::DarkBlue;
void setGeneral(clockWord *clockWords, CRGB color);

void setCommLed(CRGB color)
{
  leds[0] = color;
  FastLED.show();
}

WiFiServer server(80); // server socket

void showWifiLetters(int counter)
{
  setLightGrid(Wifi[counter], wifiColor);
  FastLED.show();
}

time_t getNtpTime()
{
  timeClient.update();
  return timeClient.getEpochTime();
}

bool connectToWifi()
{
  int outerCounter = 0;
  int defaultdelay = 10000;
  int delayms = defaultdelay;
  while (status != WL_CONNECTED and outerCounter < 12)
  {
    Serial.print(F("Attempting to connect to SSID: "));
    Serial.println(ssid);
    //  Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 2 seconds for connection:
    int counter = 0;
    clockWord HourWord[2] = {HourWords[outerCounter], Stop};
    setGeneral(HourWord, wifiColor);
    while (counter != 4)
    {
      showWifiLetters(counter);
      counter++;
      delay(delayms >> 2);
    }
    for (int letter : Wifi)
      setLightGrid(letter, CRGB::Black);
    FastLED.show();
    counter = 0;
    outerCounter++;
    setGeneral(HourWord, CRGB::Black);
    delayms += 0;
  }
  return (status == WL_CONNECTED);
}

void setupAP()
{
  // Create open network. Change this line if you want to create an WEP network:
  Serial.println("Test setup AP");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid);

  // wait 10 seconds for connection:
  delay(10000);

  // start the web server on port 80
  server.begin();

  // you're connected now, so print out the status
  printWifiStatus();
}

void serveAP()
{
  WiFiClient client = server.accept(); // listen for incoming clients

  if (client)
  {                               // if you get a client,
    Serial.println("new client"); // print a message out the serial port
    String currentLine = "";      // make a String to hold incoming data from the client
    while (client.connected())
    { // loop while the client's connected
      if (client.available())
      {                         // if there's bytes to read from the client,
        char c = client.read(); // read a byte, then
        Serial.write(c);        // print it out the serial monitor
        if (c == '\n')
        { // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0)
          {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("This is a test page<br>");
            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          }
          else
          { // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        }
        else if (c != '\r')
        {                   // if you got anything else but a carriage return character,
          currentLine += c; // add it to the end of the currentLine
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

// the setup function runs once when you press reset or power the board
void setup()
{
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(STATUS_PIN, OUTPUT);

  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  // Open serial communications and wait for port to open:
  setCommLed(CRGB::Red);

  Serial.begin(9600);
  while (!Serial)
    ; // wait for serial port to connect. Needed for native USB port only
  Serial.println("Starting Arduino");

  setCommLed(CRGB::Green);
  bool connected = connectToWifi();
  Serial.println(connected);
  if (!connected)
  {
    FastLED.clear();
    setCommLed(CRGB::Red);
    setupAP();
    while (true)
      serveAP();
  }
  setCommLed(CRGB::Gold);

  Serial.println(F("Connected to wifi"));
  server.begin();
  printWifiStatus();

  Serial.println(F("\nStarting connection to server..."));
  Udp.begin(localPort);

  timeClient.begin();
  timeClient.update();

  setSyncInterval(86400); // Re-sync every day
  setSyncProvider(getNtpTime);
}

TimedAction setClockThread = TimedAction(1000, setClock);

void loop()
{
  setClockThread.check();
  // serveWebsiteThread.check();
}

void digitalClockDisplay(time_t t, const char *tz)
{
  Serial.print(F("The datetime is "));

  char buf[32];
  char m[4];

  strcpy(m, monthShortStr(month(t)));

  sprintf(buf, "%.2d:%.2d:%.2d %s %.2d %s %d %s",
          hour(t), minute(t), second(t), dayShortStr(weekday(t)), day(t), m, year(t), tz);
  Serial.println(buf);
}

void setPossibleTime()
{
  time_t t = getNtpTime();
  if (t != 0)
    setTime(t);
}

void setClock()
{
  if (timeStatus() != timeNotSet)
  {
    currentDisplay = myTZ.toLocal(now(), &tcr);
    if (currentDisplay != prevDisplay)
    {
      prevDisplay = currentDisplay;
      showTime(prevDisplay);
    }
  }
  else
  {
    setPossibleTime();
  }
}

void showTime(time_t t)
{
  // digitalClockDisplay(prevDisplay, tcr->abbrev);
  unsigned int sec = second(prevDisplay);
  digitalWrite(STATUS_PIN, sec & 0x01);  // change LED state every second
  setLedsclock(prevDisplay, CRGB::Blue); // set the leds according to the clock time
}

void setMinutes(time_t t, CRGB color);
void setHours(time_t t, CRGB color);
void setMinutesLeds(int ledNumber, CRGB color);
void setLightGrid(int row, int col, CRGB color);

void setLedsclock(time_t t, CRGB color)
{
  setMinutes(t, color);
  setHours(t, color);
  FastLED.show();
}

void setMinutes(time_t t, CRGB color)
{
  int fiveMinutes = minute(t) % 5;
  for (int i = 0; i < 4; i++)
  {
    CRGB setColor = (fiveMinutes > i) ? color : CRGB::Black;
    setMinutesLeds(i, setColor);
  }
}

void setMinutesLeds(int ledNumber, CRGB color)
{
  leds[ledNumbers[ledNumber]] = color;
}

clockWord getHour(time_t t);
clockWord *getClockWords(time_t t);

void setLightGrid(int number, CRGB color);
void setLightGrid(int col, int row, CRGB color);

void setHours(time_t t, CRGB color)
{
  resetLightGrid();

  clockWord *clockWords = getClockWords(t);
  setGeneral(clockWords, color);
}

void setGeneral(clockWord *clockWords, CRGB color)
{
  int i = 0;
  while ((*(clockWords + i)).to != Stop.to)
  {
    clockWord word = *(clockWords + i);
    setLightGridRow(word.from, word.to, color);
    i++;
  }
}

clockWord allWords[7] = {Het, Is};

clockWord *getClockWords(time_t t)
{
  int minutes = minute(t) / 5;
  if (minutes == 0)
  {
    allWords[2] = getHour(t);
    int i = 0;
    while ((i < 3) & (MinutesSentences[minutes * 3 + i].to != Stop.to))
    {
      allWords[3 + i] = MinutesSentences[minutes * 3 + i];
      i++;
    }
    allWords[3 + i] = Stop;
  }
  else
  {
    int i = 0;
    while ((i < 3) & (MinutesSentences[minutes * 3 + i].to != Stop.to))
    {
      allWords[2 + i] = MinutesSentences[minutes * 3 + i];
      i++;
    }
    allWords[2 + i] = getHour(t);
    allWords[2 + ++i] = Stop;
  }

  return allWords;
}

clockWord getHour(time_t t)
{
  int hours = hour(t);
  if (minute(t) >= 20)
    hours++;
  hours = hours % 12;
  hours = (hours == 0) ? 12 : hours;

  return HourWords[--hours];
}

void resetLightGrid()
{
  setLightGridRow(0, GRID_SIZE, CRGB::Black);
}

void setLightGridRow(int from, int to, CRGB color)
{
  for (int i = from; i < to; i++)
  {
    setLightGrid(i, color);
  }
}

void setLightGrid(int number, CRGB color)
{
  setLightGrid(number % kMatrixWidth, number / kMatrixWidth, color);
}

void setLightGrid(int col, int row, CRGB color)
{
  leds[XY(col, row) + SURROUNDING_LEDS] = color;
}

void resetAllLeds()
{
  resetLightGrid();
  for (int i = 0; i < 4; i++)
  {
    setMinutesLeds(i, CRGB::Black);
  }
  FastLED.show();
}