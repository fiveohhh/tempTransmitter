// Requires DallasTemperature library http://milesburton.com/Dallas_Temperature_Control_Library
// virtualWire library http://www.open.com.au/mikem/arduino/
// oneWire Library http://www.pjrc.com/teensy/td_libs_OneWire.html


#include <OneWire.h>
#include <DallasTemperature.h>
#include <VirtualWire.h>
#include <EEPROM.h>

#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#undef int
#undef abs
#undef double
#undef float
#undef round

 
void sendVWmsg(char * msg);

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 4

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);



// device addresses
DeviceAddress garageThermometer = { 0x10, 0xA9, 0x30, 0x1A, 0x02, 0x08, 0x00, 0xAF };
//DeviceAddress outsideThermometer = { 0x10, 0x05, 0xD5, 0x2A, 0x02, 0x08, 0x00, 0xCE };
DeviceAddress outsideThermometer = { 0x28, 0x3F, 0xB3, 0xE3, 0x03, 0x00, 0x00, 0xF3 }; // outdoor waterproof



#define DEBUG_MODE_PIN 8 // pin to pull high if we want debug interval

#define GARAGE_DOOR_PIN 7// figure this out when you get hardware up here

#define GARAGE_DOOR_SENSOR_NUMBER 0 // Garage door sensor number is 0

#define SEND_INTERVAL 10 // every SEND_INTERVAL * 8seconds updates will be sent

#define DEBUG_INTERVAL 1 // interval when debug is set


unsigned long prevSeconds = 0;

bool isDebug = false;

int transmitInterval = SEND_INTERVAL;

volatile int f_wdt=1;

volatile int wdt_cntr = 0;



void setup() {
  Serial.begin(57600);
  
  /*** Setup the WDT ***/
  
  /* Clear the reset flag. */
  MCUSR &= ~(1<<WDRF);
  
  /* In order to change WDE or the prescaler, we need to
   * set WDCE (This will allow updates for 4 clock cycles).
   */
  WDTCSR |= (1<<WDCE) | (1<<WDE);

  /* set new watchdog timeout prescaler value */
  WDTCSR = 1<<WDP0 | 1<<WDP3; /* 8.0 seconds */
  
  /* Enable the WD interrupt (note no reset). */
  WDTCSR |= _BV(WDIE);
  
  if (digitalRead(DEBUG_MODE_PIN) == HIGH)
  {
     isDebug = true;
  }
  
  // Initialise the IO and ISR
    vw_set_ptt_inverted(true); // Required for DR3100
    vw_setup(2000);	 // Bits per sec
    analogReference(EXTERNAL);
    pinMode(DEBUG_MODE_PIN, INPUT);
    digitalWrite(DEBUG_MODE_PIN, HIGH); // turn on pullups
    pinMode(GARAGE_DOOR_PIN, INPUT);
    digitalWrite(GARAGE_DOOR_PIN, HIGH); // turn on pullups
    
  // Start up the library
  sensors.begin();

  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");
  
  // disable if not in debug?
  if (isDebug)
  {
    
    transmitInterval = DEBUG_INTERVAL;
  }
}



ISR(WDT_vect)
{
  if(f_wdt == 0)
  {
    f_wdt=1;
  }
  else
  {
    Serial.println("WDT Overrun!!!");
  }
}



void loop() { 
  if(f_wdt == 1)
  {
    Serial.println(wdt_cntr);
    if ( wdt_cntr++ >= transmitInterval)
    {
      Serial.println("inside");
      checkSensors();
      wdt_cntr = 0;
    }
    /* Don't forget to clear the flag. */
    f_wdt = 0;
    
    /* Re-enter sleep mode. */
    enterSleep();
  }
  else
  {
    /* Do nothing. */
  }

}

void checkSensors()
{
   // store time that this read/transmit is occuring
    prevSeconds = millis()/1000;
     
     
    // get temperature data
    sensors.requestTemperatures();
    delay(1000); // need to delay after request since we are using parasitic power

    // get rid of decimal place, truncate anything after 2 places
    float garageTemp = sensors.getTempC(garageThermometer) + 273.15;
    float outsideTemp = sensors.getTempC(outsideThermometer) + 273.15;
    int garageSensorTimeHundred = garageTemp * 100;
    int outsideSensorTimeHundred = outsideTemp * 100;
    
    Serial.print("Garage ");
    Serial.print(" Kelvin: ");
    Serial.println(garageTemp);
    
    Serial.print("Outside ");
    Serial.print(" Kelvin: ");
    Serial.println(outsideTemp);
    
    
    //transmit temperature status
    char msg[12];
    sprintf(msg,"TMP1%d%d", 0, garageSensorTimeHundred); //0 is garage
    sendVWmsg(msg);
    
    sprintf(msg,"TMP1%d%d", 1, outsideSensorTimeHundred); //1 is outside
    sendVWmsg(msg);
      
      
    // get door status 
    int newDoorStatus = digitalRead(GARAGE_DOOR_PIN);
    char dmsg[12];
    sprintf(dmsg,"DOR1%d%d", GARAGE_DOOR_SENSOR_NUMBER, newDoorStatus);
    
    // transmit door status
    sendVWmsg(dmsg);
}

void enterSleep()
{
  set_sleep_mode(SLEEP_MODE_PWR_SAVE);
  
  sleep_enable();
  
    /* The program will continue from here after the WDT timeout*/
  sleep_disable(); /* First thing to do is disable sleep. */
  
  /* Re-enable the peripherals. */
  power_all_enable();
}


// send message using virtualWire
void sendVWmsg(char * msg)
{
  digitalWrite(13, true); // Flash a light to show transmitting
  Serial.println(msg);
  vw_send((uint8_t *)msg, strlen(msg));
  vw_wait_tx(); // Wait until the whole message is gone
  delay(1000);
  digitalWrite(13, false);
  
}

