/*
  Smart thermostat using ESP8266 arduino compadable breakout
  By William Winters, 2023, released to public domain

  Temperature sensing adapted from code on sunfounder.com 
  other referenced material:

  randomnerdtutorials.com/esp8266-web-server/
  Arduino.cc
  arduino-esp8266.readthedocs.io 
  
  Time setting code from:
  https://www.instructables.com/Getting-Time-From-Internet-Using-ESP8266-NTP-Clock/

  the backend API and code are rudimenty at best, but they are writen in C and run on a potato, so give them a break

  Known Issues: 

  The ESP8266 arduino compadable breakout can not provide much current, so it is best to disconnect external hardware
  while programming, or the computer may not be able to connect to it. This could be filed by using transistors to drive
  the relays from an external scource.
 
*/

#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <NTPClient.h>
#include "WiFiUdp.h"
/*
  Pinouts on the ESP8266 arduino compadible breakout board do not always reflect lables
  consult datasheet or google "ESP8266 arduino pinout"
*/
#define ONE_WIRE_BUS 4
#define HEAT_ENABLE 2  

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

// define global temperature variables
float setpoint = 75.0 ;
float nightsetpoint = 80 ;
float roomTemp = 0.0;

WiFiServer server(80);

String request;

const long utcOffsetInSeconds = -5*60*60;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);



void setup()
{
  
  Serial.begin(9600);

  // setup pin for heat enable relay
  pinMode(HEAT_ENABLE , OUTPUT);

  // Start up the sensor library
  sensors.begin();


  Serial.println();

  WiFi.begin("moto g power (2021)_2140", "hotspot447");

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  // connected
  server.begin();
  
  timeClient.begin();

}

void loop() 
{

  WiFiClient client = server.available();

  if(client)
  {
    serveClient(client);
  }

  update();

  delay(100);
}
  bool isNight()
  {
    // placeholder function to tell which setpoint to u
    
    timeClient.update();
    int hour = timeClient.getHours();
    if (hour < 5 )
    {
      // it is between midnight and 4 AM
      return true;
    }
    else
    {
    return false;
    }
  }

  // act like thermostat
  void update()
  {
    //get temperature
    sensors.requestTemperatures(); 
    roomTemp = sensors.getTempFByIndex(0);
    Serial.print("Room" "Temperature: ");
    Serial.println(roomTemp);

    if(isNight())
    {
      if(roomTemp < nightsetpoint)
      {
        digitalWrite(HEAT_ENABLE, HIGH );
      }
      else
      {
        digitalWrite(HEAT_ENABLE, LOW );
      }      
    }
    else
    {
      if(roomTemp < setpoint)
      {
        digitalWrite(HEAT_ENABLE, HIGH );
      }
      else
      {
        digitalWrite(HEAT_ENABLE, LOW );
      }
    }

    

  }


  // parse updated settings from GET request
  void processRequest(String request)
  {
    if (request.indexOf("setpoint=") >= 0)
    {
      String data = request.substring(request.indexOf('=') + 1 , request.indexOf(' ' , request.indexOf("=")));
      Serial.print("recived data: {");
      setpoint = data.toFloat();
      Serial.print(setpoint);
      Serial.print("}\n");
    }
  }

  // complete network transaction
  void serveClient(WiFiClient client)
  {
    Serial.print("new connection\n");
    
    char lastChar = 0;
    char c = 0;
    while(client.connected())
    {
      if (client.available()) // if data to read
      {
        c = client.read();
        // strip newlines
        if (c == '\r')
        {
          continue;
        }
        request += c;
        Serial.print(c);

        // if request finished
        if (lastChar == '\n' && c == '\n') 
        {
          /*
            begin reply
          */
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println("Connection: close");
          client.println();
          
          //process request
          processRequest(request);        
        
          /*
            send HTML data. 
            escape quotes and newlines
          */
          client.println("<!DOCTYPE html><html>");

          client.println("<head><head><meta charset=\"utf-8\">");
          client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");

          // Web Page Heading
          client.println("<body><h1>Smart Thermostat Dashboard</h1>");

          client.printf(
            "<form action=\"\"  method=\"get\"> \
              <input type=\"number\" name=\"setpoint\" id=\"setpoint\" min=\"50\" max=\"90\" value=\"%f\"> \
              <input type=\"submit\" name=\"Update Setpoint\" >\
            </form>",setpoint);


          // end body of webpage
          client.println("</body></html>");

          // The HTTP response ends with another blank line
          client.println();
          break; 
        }
          
          lastChar = c;
          
        }      
      }
      /*
      out of while look, clean up
      */
      delay(3);
      client.stop();
      Serial.println("client disconnected");
      c = 0;
      lastChar = 0;
      request = "";
      
  }
  
  
