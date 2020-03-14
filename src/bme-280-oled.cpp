/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#line 1 "c:/Users/Conrad/source/Particle/Photon/bme-280/bme-280-oled/src/bme-280-oled.ino"
/*
 * Project bme-280-oled
 * Description:
 * Author:
 * Date:
 */

#include "Particle.h"
#include "Adafruit_BME280_RK.h"
#include "Adafruit_Sensor.h"
#include "Adafruit_SSD1306.h"
#include <math.h> // isnan()

void setup();
void loop();
#line 14 "c:/Users/Conrad/source/Particle/Photon/bme-280/bme-280-oled/src/bme-280-oled.ino"
Adafruit_BME280 bme; // I2C

Adafruit_SSD1306 display(-1);



// I2C wiring 
#define BME_MOSI D0 // = SDA 
#define BME_SCK D1 // = SCL 

// Display update interval in seconds
const int updatePeriod = 10;
unsigned long lastUpdate = 0;

// Repeat time for Publish in seconds
// Example 900 will repeat every 15 minutes at :00, :15, :30, :45
const int period = 900;

// time sync interval in seconds
// simple interval, repeat every n seconds
const int sync_interval = 3600;
time_t next_sync;

time_t current_time;
time_t next_pub;

char buf[64];
int led1 = D7; //onboard led


void setup() {
	Serial.begin(9600);
    Particle.publish("status", "start", PRIVATE);

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    bme.begin(0x76);

    pinMode(led1, OUTPUT);
    digitalWrite(led1, LOW);

    current_time = Time.now();
    next_pub = current_time - (current_time % period) + period;
    next_sync = current_time + sync_interval;
    delay(2000);
}

void loop() { 
  current_time = Time.now();
  // Update readings
	if (millis() - lastUpdate >= updatePeriod * 1000) {
    digitalWrite(led1, HIGH);
		lastUpdate = millis();
		
    float temp = bme.readTemperature(); // degrees C
    float humidity = bme.readHumidity(); // % 
    float pressure = (bme.readPressure() / 3386.39F); // inches-Hg

		display.clearDisplay();
		
		if (!isnan(temp) && !isnan(humidity) && !isnan(pressure)) {
			display.setTextSize(2);
			display.setTextColor(WHITE);
			
			snprintf(buf, sizeof(buf), "%.1f F", temp * 9.0 / 5.0 + 32.0);
			display.setCursor(0,0);
			display.println(buf);

			snprintf(buf, sizeof(buf), "%.1f %% RH", humidity);
      display.setCursor(0,24);
			display.println(buf);

			snprintf(buf, sizeof(buf), "%.1f InHg", pressure);
      display.setCursor(0,48);
			display.println(buf);

      // Publish results
      if(current_time >= next_pub) {
        String result = String::format("{\"Temp_C\": %4.2f, \"Press_InHg\": %4.2f, \"RelHum\": %4.2f}", temp, pressure, humidity);
        Particle.publish("readings", result, PRIVATE);
        next_pub = current_time - (current_time % period) + period;
      }
      // for debug
	    // Particle.publish("current time", Time.timeStr(current_time), PRIVATE);
      // Particle.publish("next publish", Time.timeStr(next_pub), PRIVATE);
      // Particle.publish("next sync", Time.timeStr(next_sync), PRIVATE);
  	}
    display.display();
    digitalWrite(led1, LOW);
  }

  // sync time
  if(current_time >= next_sync) {
      Particle.syncTime();
      delay(5000);
      if (Particle.syncTimePending()) {
          Particle.publish("status", "time sync failed", PRIVATE);
      }
      else {
          current_time = Time.now();
          next_sync = current_time + sync_interval;
          Particle.publish("status", "time sync success", PRIVATE);
      }
  }
}