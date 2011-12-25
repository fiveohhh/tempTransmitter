
#include <VirtualWire.h>
#include <EEPROM.h>
#undef int
#undef abs
#undef double
#undef float
#undef round
/*
  AnalogReadSerial
 Reads an analog input on pin 0, prints the result to the serial monitor 
 
 This example code is in the public domain.
 */
 
 void sendVWmsg(char * msg);

// number of seconds between reads
#define TEMP_INTERVAL 60

#define DEBUG_INTERVAL 5 // number of seconds between reads during if debug pin is set

#define NUMBER_OF_SENSORS 2

#define READ_ATTEMPTS 2 // number of times to try to get two consecutive temps in a row
                        // added this since there were some outliers in the readings
                        // This will make sure we get two in a row that are close before 
                        // recording them
                        // if we don't get expected readings after this, we will record the 
                        // last one

// #define EEPROM_SIZE_IN_BYTES 1000

#define DEBUG_MODE_PIN 8 // pin to pull high if we want debug interval

#define GARAGE_DOOR_PIN "unknown"// figure this out when you get hardware up here

uint8_t pinsWithTempSensors[NUMBER_OF_SENSORS] = {0,1/*,2*/};

// correct sensor 
int8_t adcErrorCorrection[NUMBER_OF_SENSORS] = {-1, -3/*, -2*/};

int doorStatus = 0; // 0 is closed and 1 is open.

unsigned long prevSeconds = 0;

void setup() {
  Serial.begin(57600);
  
  // Initialise the IO and ISR
    vw_set_ptt_inverted(true); // Required for DR3100
    vw_setup(2000);	 // Bits per sec
    analogReference(EXTERNAL);
    pinMode(DEBUG_MODE_PIN, INPUT);
    pinMode(GARAGE_DOOR_PIN, INPUT);
    
}

void loop() { 
  int tempInterval = TEMP_INTERVAL;

  if (digitalRead(DEBUG_MODE_PIN) == HIGH)
  {
     tempInterval = DEBUG_INTERVAL;
  }
  
  if (digitalRead(GARAGE_DOOR_PIN) == HIGH && doorStatus == 1)
  {
    // was open and now closed
    //send update msg
  }
  else if (digitalRead(GARAGE_DOOR_PIN) == LOW && doorStatus == 0)
  {
    // was closed and now open
    //send update msg
  }
  
  unsigned long seconds = millis()/1000;
  if ((prevSeconds == 0) || (seconds - prevSeconds) > (tempInterval))
  {
     prevSeconds = millis()/1000;
    int i = 0;
    for (i = 0; i < NUMBER_OF_SENSORS; i++)
    {
        //*************Read and transmit sensor
      int sensorVal = 0;
      for (int j = 0; j < READ_ATTEMPTS; j++)
      {

        sensorVal = analogRead(i);
        Serial.print("reading1: ");
        Serial.println(sensorVal);
        delay(100)
        int sensorVal2 = analogRead(i);
        sensorVal = analogRead(i);
        Serial.print("reading2: ");
        Serial.println(sensorVal2);

        // if second reading was +/- 5 from first reading, call it good
        if ((sensorVal2 <= (sensorVal + 5)) && (sensorVal2 >= (sensorVal - 5))) // 5 ~2degrees
        {
          Serial.println("Temps close to same, keeping");
          j = READ_ATTEMPTS; //  got a good reading, exit
        }
        else
        {
          Serial.println("Temps NOT close to same,check again");
        }
      }  
      
      // get Kelvin (for lm335 sensor)
      double sensorKel = ((sensorVal/(double)1023)*(5*100)) + adcErrorCorrection[i];
      // get rid of decimal place, truncate anything after 2 places
      int sensorTimeHundred = sensorKel * 100;
      Serial.print("Sensor ");
      Serial.print(i);
      Serial.print(" Kelvin: ");
      Serial.println(sensorKel);
      char msg[12];
      sprintf(msg,"TMP1%d%d", i, sensorTimeHundred);
      
      sendVWmsg(msg);
    }
  }
  Serial.println(prevSeconds, DEC);
  Serial.println(millis()/1000);

  delay(1000);
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

char *ftoa(char *a, double f, int precision)
{
  long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};
  
  char *ret = a;
  long heiltal = (long)f;
  itoa(heiltal, a, 10);
  while (*a != '\0') a++;
  *a++ = '.';
  long desimal = abs((long)((f - heiltal) * p[precision]));
  itoa(desimal, a, 10);
  return ret;
}
