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


// Temp, pressure, humidity sensor
Adafruit_BME280 bme;
// 128x32 oled display
Adafruit_SSD1306 display(-1);

// I2C wiring 
#define BME_MOSI D0 // = SDA 
#define BME_SCK D1 // = SCL 

 //onboard led
 int led1 = D7;

// Display update interval in seconds
const int updatePeriod = 5;
unsigned long lastUpdate = 0;

// Repeat time for Publish in seconds
// Example 900 will repeat every 15 minutes at :00, :15, :30, :45
const int period = 1800;

// time sync interval in seconds
// simple interval, repeat every n seconds
// 12 hours = 43200
const int sync_interval = 43200;
time_t next_sync;

time_t current_time;
time_t next_pub;

char buf[64];


void setup() {
	Serial.begin(9600);
    Particle.publish("status", "start", PRIVATE);
    Particle.function("current_conditions", current);

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

		// if we have good readings update screen & publish readings
		if (!isnan(temp) && !isnan(humidity) && !isnan(pressure)) {
      // calculate dew point
      float dewpoint = calcDewpoint(temp, humidity);

      // display results
			display.setTextSize(2);
			display.setTextColor(WHITE);
			
			snprintf(buf, sizeof(buf), "%.1f Deg F", temp * 9.0 / 5.0 + 32.0);
			display.setCursor(0,0);
			display.println(buf);

			snprintf(buf, sizeof(buf), "%.1f DwPtF", dewpoint * 9.0 / 5.0 + 32.0);
      display.setCursor(0,24);
			display.println(buf);

			// snprintf(buf, sizeof(buf), "%.1f %% RH", humidity);
      // display.setCursor(0,24);
			// display.println(buf);

			snprintf(buf, sizeof(buf), "%.1f InHg", pressure);
      display.setCursor(0,48);
			display.println(buf);

      // Publish results
      if(current_time >= next_pub) {
        String result = String::format("{\"Temp_C\": %4.2f, \"Dewpoint_C\": %4.2f, \"RelHum\": %4.2f, \"Press_InHg\": %4.2f}", temp, dewpoint, pressure, humidity);
        Particle.publish("readings", result, PRIVATE);
        next_pub = current_time - (current_time % period) + period;
      }
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

int current(String unit) {
    digitalWrite(led1, HIGH);
    String result = "Invalid unit given. Allowed units are 'c' or 'f' for celsius or fahrenheit.";
    if (unit == "c") {
      float temp = bme.readTemperature(); // degrees C
      float humidity = bme.readHumidity(); // % 
      float pressure = (bme.readPressure() / 3386.39F); // inches-Hg
      float dewpoint = calcDewpoint(temp, humidity); // dew point C
      result = String::format("{\"Temp_C\": %4.2f, \"Dewpoint_C\": %4.2f, \"RelHum\": %4.2f, \"Press_InHg\": %4.2f}", temp, dewpoint, pressure, humidity);
    }
    else if (unit == "f") {
      float temp = bme.readTemperature();
      float tempF = (temp * 9 / 5) + 32; // degrees F
      float humidity = bme.readHumidity(); // % 
      float pressure = (bme.readPressure() / 3386.39F); // inches-Hg
      float dewpointF = (calcDewpoint(temp, humidity) * 9 / 5) + 32; // dew point F
      result = String::format("{\"Temp_F\": %4.2f, \"Dewpoint_F\": %4.2f, \"RelHum\": %4.2f, \"Press_InHg\": %4.2f}", tempF, dewpointF, pressure, humidity);
    }
    Particle.publish("current_conditions", result, PRIVATE);
    digitalWrite(led1, LOW);
    return 1;
}

float calcDewpoint(float temp, float humidity) {
  // calculate dewpoint from temp & relative humidity
  // temp in C, rel humidity as decimal, i.e. .715 for 71.5%
  // using Magnus formula
  // source: https://www.azosensors.com/article.aspx?ArticleID=23

  // table of Magnus coefficients for dew point calculation
  // [minTempC, maxTempC, C1(mbar), C2(-), C3(C)]
  float mgn[2][5] = {{-50.9,0,6.1078,17.84362,254.425},{0,100,6.1078,17.08085,234.175}};

  int ms = -1;
  float dewpoint = 999;
  for (int i = 0; i < 2; i++) {
    if (temp > mgn[i][0] && temp < mgn[i][1]) {
      ms = i;
    }
  }
  if (ms >= 0) {
    // Magnus formula dewpoint calculation
    float ps = mgn[ms][2] * exp((mgn[ms][3] * temp)/(mgn[ms][4] + temp));
    float pd = ps * (humidity / 100);
    dewpoint = ( (-log(pd / mgn[ms][2]) * mgn[ms][4]) / (log(pd / mgn[ms][2]) - mgn[ms][3]) );
  }
  else {
    Particle.publish("error", "Temp out of range for dew point calculation.", PRIVATE);
  }
  return dewpoint;
}