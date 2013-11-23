// This sketch uses a DHT11 sensor to report temperature, humidity and dew point data to http://www.thingspeak.com.
//
// Sketch tested with an Arduino Uno, a HanRun Ethernet shield and a DHT11 temperature and humidity sensor.
//
// See http://playground.arduino.cc/main/DHT11Lib for the origins of the temperature, humidity and dew point functions.

#include <dht11.h>
#include <SPI.h>
#include <Ethernet.h>

// ThingSpeak Settings
char thingSpeakAddress[] = "api.thingspeak.com";
String writeAPIKey = ""; // Add your Thingspeak API key here

EthernetClient client;

// Temperature sensor settings
dht11 DHT11;
#define DHT11PIN 14

const int ONE_MINUTE = 60 * 1000;

int status;
int failedConnectionAttempCounter;

//Rounds down (via intermediary integer conversion truncation)
//See : http://lordvon64.blogspot.co.uk/2012/01/simple-arduino-double-to-string.html
String dblToString(double input, int decimalPlaces)
{
  if( decimalPlaces != 0)
  {
    String string = String((int)(input*pow(10,decimalPlaces)));

    if(abs(input) < 1)
    {
      if(input > 0)
      {
        string = "0" + string;
      }
      else if(input < 0)
      {
        string = string.substring(0,1) + "0" + string.substring(1);
      }
    }

    return string.substring(0,string.length()-decimalPlaces) + "." + string.substring(string.length() - decimalPlaces);
  }
  else 
  {
    return String((int)input);
  }
}

// dewPoint function NOAA
// reference: http://wahiduddin.net/calc/density_algorithms.htm 
double dewPoint(double celsius, double humidity)
{
  double A0= 373.15/(273.15 + celsius);
  double SUM = -7.90298 * (A0-1);
  SUM += 5.02808 * log10(A0);
  SUM += -1.3816e-7 * (pow(10, (11.344*(1-1/A0)))-1) ;
  SUM += 8.1328e-3 * (pow(10,(-3.49149*(A0-1)))-1) ;
  SUM += log10(1013.246);
  double VP = pow(10, SUM-3) * humidity;
  double T = log(VP/0.61078);   // temp var
  return (241.88 * T) / (17.558-T);
}

void setup()
{
  Serial.begin(9600);
  Serial.println("DHT11 Temperature Sensor Program");
  Serial.print("DHT11 library version: ");
  Serial.println(DHT11LIB_VERSION);
  Serial.println();

  connectToInternet();
}

void connectToInternet()
{
  if (client.connected())
  {
    client.stop();
  }

  Serial.println("Connecting to the internet via ethernet...");

  // the media access control (ethernet hardware) address for the shield
  // Leave this as is if your MAC address is not labelled on your ethernet shield
  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore
    for(;;){
      ;
    }
  }

  Serial.println(Ethernet.localIP());
}

void loop()
{
  Serial.println("\n");

  int dht11ReadingStatus = DHT11.read(DHT11PIN);

  Serial.print("Reading sensor...");
  switch (dht11ReadingStatus)
  {
  case DHTLIB_OK: 
    Serial.println("Success!"); 
    break;
  case DHTLIB_ERROR_CHECKSUM: 
    Serial.println("Checksum error"); 
    break;
  case DHTLIB_ERROR_TIMEOUT: 
    Serial.println("Timeout error"); 
    break;
  default: 
    Serial.println("Unknown error"); 
    break;
  }

  double dewPointCelcius = dewPoint(DHT11.temperature, DHT11.humidity);

  ReportTemperatureToSerialOut(DHT11.temperature, DHT11.humidity, dewPointCelcius);
  ReportTemperatureToThingspeak(DHT11.temperature, DHT11.humidity, dewPointCelcius);
}

void ReportTemperatureToSerialOut(int temperature, int humidity, double dewPointCelcius)
{
  Serial.print("Temperature (oC): ");
  Serial.println((float)temperature, 2);

  Serial.print("Humidity (%): ");
  Serial.println((float)humidity, 2);

  Serial.print("Dew Point (oC): ");
  Serial.println(dewPointCelcius);
}

void ReportTemperatureToThingspeak(int temperature, int humidity, double dewPoint)
{
  // Use short field names i.e. 1 instead of field1
  String fields = "1=" + String(temperature, DEC);
  fields += "&2=" + String(humidity, DEC);
  fields += "&3=" + dblToString(dewPoint, 2);
  Serial.println(fields);

  if (client.connect(thingSpeakAddress, 80))
  {
    Serial.println("Connected to thingspeak.com");

    // Create HTTP POST Data            
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: "+writeAPIKey+"\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(fields.length());
    client.print("\n\n");
    client.print(fields);

    Serial.print(fields);
    Serial.print("\n");
    Serial.println("Fields sent sent to www.thingspeak.com");
    delay(ONE_MINUTE);
  }
  else
  {
    Serial.println("Connection to thingSpeak Failed");   
    Serial.println();
    failedConnectionAttempCounter++;

    // Re-start the ethernet connection after three failed connection attempts 
    if (failedConnectionAttempCounter > 3 )
    {
      Serial.println("Re-starting the ethernet connection..."); 
      connectToInternet();
      failedConnectionAttempCounter = 0;
    }
  }
}
