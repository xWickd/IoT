/************
  This code was designed by Petros Gateas and Panagiotis Tiktapanidis on December 17th 2023.
  It is intended for academic use at the Department of Informatics and Telecommunications of University of Ioannina


  This code was designed and compiled at Arduino IDE v2.0.0
  It was designed for use with Espressif ESP32-C3-DevKitC-02, DHT22 sensor, 2x16 LCD with I2C, and Thinger.io platform

  This code connects the ESP32 to the Interenet and to the Thinger.io platform through Wi-Fi,
  reads the temperature and humidity from the DHT22 sencsor and presents the measurements,
  to the Serial Console, the local LCD and the Thinger.io platform.
  Also it uploads to the platform measurements of the average temperature and humidity of 1 minute and average humidity
  of 30 seconds, and holds the minute average of the temperature and humidity on a data bucket.
  The platform then represents those data as graphs of current temperature, humidity, 1 hour history of temperature 
  (average of 1 minute temperature) and history of both temperature and humidity.

********************/

#include "DHT.h"                                      // DHT Sensor lib
#include <LiquidCrystal_I2C.h>                        // LCD lib
#include <WiFi.h>                                     // Wi-Fi library
#include <ThingerESP32.h>                             // Thinger.io lib


// DHT sensor information
#define DHTPIN 2                                      // Connect DHT sensor to the D2
#define DHTTYPE DHT22                                 // Declar the DHT 22 sensor type

#define SSID "GameZone"
#define SSID_PASSWORD "den_exei!"

#define USERNAME "PetrosStavros"                      // Define Thinger.io username
#define DEVICE_ID "ESP32AM1878"                       // Define device_id of platform
#define DEVICE_CREDENTIAL "123456789"                 // Define unique credential for device


//LCD information
int lcdColumns = 16;                                  // Set the LCD number of columns
int lcdRows = 2;                                      // Set the LCD number of rows

// Create lcd object of controlling the LCD
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);     // Set LCD address, number of columns and rows

//Create dht object for controlling the DHT sensor
DHT dht(DHTPIN, DHTTYPE);                             // Declare the DHT pin, with the type (initialize the DHT22 sensor)

//Create thing object for controlling the communication to Thinger.io platform
ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL); //Create connection to Thinger.io platform

float avgtemp_onem = 0;                               // Help value to store the average temperature of ~1 minute
float avghumidity_onem = 0;                           // Help value to store the average humidity of ~1 minute
int temp_onem_index = 0;                              // Help index to determine when ~1 minute has passed
int humidity_halfm_index = 0;                         // Help index to determine when ~30 seconds has passed
float avghumidity_halfm = 0;                          // Help value to store the average humidity of ~30 seconds
pson alert;                                           // Value for output for the endpoint

void setup() 
{
  Serial.begin(115200);                               // Initialize the serial baud rate to 9600 per second
  lcd.init();                                         // Initialize the LCD
  lcd.backlight();                                    // Turn on LCD blacklight
  dht.begin();                                        // Begin taking measurements
  WiFi.begin(SSID, SSID_PASSWORD);                    // Begin Wi-Fi module

  // Check if WiFi is connected
  while(WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Connecting to WiFi...");          // Print debugging message to console
    delay(1000);                                      // Delay the display of next message for ~1 second
  }
  Serial.println("Connected to WiFi !");              // Print info message to console
  lcd.setCursor(0,1);                                 // Set the cursor of the LCD to the first row first column
  lcd.print("WiFi Connected");                        // Print info message to LCD

  thing.add_wifi(SSID, SSID_PASSWORD);                // Connect to Thinger.io through Wi-Fi

  Serial.println("Setup Compelte !");                 // Print logs message to console

  delay(10000);                                        // Delay ~1s to clear the LCD
  lcd.clear();                                        // Clear the LCD

  // Define LED_BUILTIN as output for external LED
  pinMode(LED_BUILTIN, OUTPUT);

  // Input from the Thinger.io platform, to control the external LED
  thing["led"] << [](pson& in)
  {
    if(in.is_empty())
    {
      in = (bool) digitalRead(LED_BUILTIN);                    // Send back the pin value to Thinger.io platform
    }
    else
    {
      digitalWrite(LED_BUILTIN, in ? HIGH : LOW);             // If there is input set LED to HIGH value, else to LOW
    }
  };

  // Output to the platform the average values of 1 minute for temperature and humidity to store to a data bucket
  thing["History: "] >> [avgtemp_onem, avghumidity_onem](pson &out)                    
  {
  String str_temp(avgtemp_onem, 2);                           // convert avgtemp_onem to String by holding 2 decimanl points
  String str_hum(avghumidity_onem, 2);                        // convert avghumidity_onem to String by holding 2 decimal points
  out["Temperature History"] = str_temp;                      // Send average temperature of minute as String
  out["Humidity History"] = str_hum;                          // Send average humidity of minute as String
  };
}



void loop() 
{
  thing.handle();                                             // Handle platform connection

  //Zero-ing of the average holding variables
  if(temp_onem_index == 0)
  {
    avgtemp_onem = 0;
  }
  if(humidity_halfm_index == 0)
  {
    avghumidity_halfm = 0;
  }


  // Read Temperature and Humidity
  float temperature = dht.readTemperature();                  // Celsius
  float humidity  = dht.readHumidity();                       // Percentage

  delay(2000);                                                // Delay for ~2 s
  lcd.clear();                                                // Clear the LCD

  // Check if any reads failed
  if(isnan(temperature) || isnan(humidity))
  {
    if(isnan(temperature))
    {
      Serial.println("Failed to read temperature from DHT");   // Debug message to console
      lcd.setCursor(0,0);                                      // Set the cursor to first column and row
      lcd.print("Failed");                                     // Print debugging message to LCD
    }
  }

  // Print measurements on LCD
  lcd.setCursor(0,0);                                           // Set cursor to first column and row
  lcd.print("Temp: ");                                          // Print temperature to LCD
  lcd.print(temperature);
  lcd.print("C");
  lcd.setCursor(0,1);                                           // Set the cursor to the first column and second row
  lcd.print("Humidity: ");                                      //  Print humidity to LCD
  lcd.print(humidity);
  lcd.print("%");

  //One minute average temperature and humidity measurements
  if(temp_onem_index < 29)
  {
    avgtemp_onem += temperature;
    avghumidity_onem += humidity;
    temp_onem_index += 1;
  }
  else
  {
    avgtemp_onem += temperature;
    avghumidity_onem += humidity;
    temp_onem_index = 0;

    // Compuration of average temperature and humidity of 1min
    avgtemp_onem /= 30;
    avghumidity_onem /= 30.0;
  }

  // 30seconds average humidity measurements
  if(humidity_halfm_index < 14)
  {
    avghumidity_halfm += humidity;
    humidity_halfm_index += 1;
  }
  else
  {
    avghumidity_halfm += humidity;
    humidity_halfm_index = 0;

    // Computation of average humidity of 30 seconds
    avghumidity_halfm /= 15;
  }

  //Conditional blocks to send the necessary data to platform when ready  
  if(temp_onem_index == 0 && humidity_halfm_index == 0)
  {
    thing["Measurements:"] >> [temperature, humidity, avgtemp_onem, avghumidity_halfm](pson& out)
    { //Output to the Thinger.io platform the necessary values
      out["Temperature"] = temperature;                                                   //Send temperature
      out["Humidity"] = humidity;                                                         //Send humidity
      out["Average Temp of minute"] = avgtemp_onem;                                       //Send average temperature of minute
      out["Average Humidity 30 seconds"] = avghumidity_halfm;                             //Send average humidity of 30 seconds
      out["Temp of hour"] = avgtemp_onem;                                                 //Send average temperature of minute (and hold for 1 hour)
    };

    thing.stream("History:");                                                             //Stream to the platform the values from "History:" for the data bucket
  }
  else if(humidity_halfm_index == 0)
  {
    thing["Measurements:"] >> [temperature, humidity, avgtemp_onem, avghumidity_halfm](pson& out)
    { //Output to the Thinger.io platform the necessary values 
      out["Temperature"] = temperature;                                                   //Send temperature
      out["Humidity"] = humidity;                                                         //Send humidity
      out["Average Humidity 30 seconds"] = avghumidity_halfm;                             //Send average humidity of 30 seconds
    };
  }
  else
  {
    thing["Measurements:"] >> [temperature, humidity](pson& out)
    { //Output to the Thinger.io platform the necessary values
      out["Temperature"] = temperature; //Send temperature
      out["Humidity"] = humidity; //Send humidity
    };
  }

  //Code for initialization of chart sources on Thinger.io platform
  // thing["Measurements:"] >> [temperature, humidity, avg_temp_one_min, avg_humidity_half_min](pson& out)
  // { //Output to the Thinger.io platform all of the values
  //     out["Temperature"] = temperature; //Send temperature
  //     out["Humidity"] = humidity; //Send humidity
  //     out["Average Temp of minute"] = avg_temp_one_min; //Send average temperature of minute
  //     out["Average Humidity 30 seconds"] = avg_humidity_half_min; //Send average humidity of 30 seconds
  //     out["Temp of hour"] = avg_temp_one_min; //Send average temperature of minute (and hold for 1 hour)
  // };
  // thing["History:"] >> [avg_temp_one_min, avg_humidity_one_min](pson& out)
  // { //Output to the Thinger.io platform the values for the data bucket 
  //   out["Temperature History"] = avg_temp_one_min; //Send average temperature of minute
  //   out["Humidity History"] = avg_humidity_one_min; //Send average humidity of minute
  // };

  //Endpoint call to send email for risen temperature
  if(temperature > 35.0)
  {
    alert["temperature"] = temperature;
    alert["humidity"] = humidity;
    thing.call_endpoint("Temp_High", alert);                                              //Call the "Temp_High" endpoint when the condition is met
  }

  // Print measurements in console
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print("°C ");
  Serial.print("  Humidity: ");
  Serial.print(humidity);
  Serial.print("%");
  Serial.println();  
}