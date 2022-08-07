#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <MKRGSM.h>
#include "arduino_secrets.h" 

Adafruit_MPU6050 mpu;

int x = 0;

int y = 0;
bool go = true;
int z = 0;

#include <SDHCI.h>
#include <Audio.h>

SDClass theSD;
AudioClass *theAudio;

File myFile;

bool ErrEnd = false;

// Please enter your sensitive data in the Secret tab or arduino_secrets.h (This is just the simple info we need 
// PIN Number
const char PINNUMBER[] = SECRET_PINNUMBER;

lte_initialize sms;

static void audio_attention_cb(const ErrorAttentionParam *atprm)
{
  puts("Attention!");
  
  if (atprm->error_code >= AS_ATTENTION_CODE_WARNING)
    {
      ErrEnd = true;
   }
}

void setup(void) {
  Serial.begin(115200);

//SMS init
  while (!connected) {
    if (gsmAccess.begin(PINNUMBER) == GSM_READY) {
      connected = true;
    } else {
      Serial.println("Not connected");
      delay(1000);
    }
  }

  Serial.println("GSM initialized");

//MPU init
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

//Audio and SD init
  while (!theSD.begin())
    {
      Serial.println("Insert SD card.");
    }

  // start audio system
  theAudio = AudioClass::getInstance();

  theAudio->begin(audio_attention_cb);

  puts("initialization Audio Library");

  theAudio->setRenderingClockMode(AS_CLKMODE_NORMAL);
  theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP, AS_SP_DRV_MODE_LINEOUT);
  err_t err = theAudio->initPlayer(AudioClass::Player0, AS_CODECTYPE_MP3, "/mnt/sd0/BIN", AS_SAMPLINGRATE_AUTO, AS_CHANNEL_MONO);

  /* Verify player initialize */
  if (err != AUDIOLIB_ECODE_OK)
    {
      printf("Player0 initialize error\n");
      exit(1);
    }

  /* Open file placed on SD card */
  myFile = theSD.open("init.mp3"); //plays an audio saying "initalizing"

  /* Verify file open */
  if (!myFile)
    {
      printf("File open error\n");
      exit(1);
    }
  printf("Open! 0x%08lx\n", (uint32_t)myFile);

  /* Send first frames to be decoded */
  err = theAudio->writeFrames(AudioClass::Player0, myFile);

  if ((err != AUDIOLIB_ECODE_OK) && (err != AUDIOLIB_ECODE_FILEEND))
    {
      printf("File Read Error! =%d\n",err);
      myFile.close();
      exit(1);
    }

  puts("Play!");

  /* Main volume set to -16.0 dB  MAXIMM is 120*/
  theAudio->setVolume(120);
  theAudio->startPlayer(AudioClass::Player0);
}

void loop() {
  
if(x >= 8 || x <= -8){
    Serial.println("Fall detected!");
    go = true;
  }
  while (go) {
    int err = theAudio->writeFrames(AudioClass::Player0, myFile);
    if (err == AUDIOLIB_ECODE_FILEEND) {
      Serial.println("File ended!\n");
      myFile.close();
      myFile = theSD.open("ask_help.mp3"); //asks user if theyre okay
      long int t1 = millis();
      while (digitalRead(A4) == HIGH) {
        long int t2 = millis();
        long int t3 = t1-t2;
        if (t3 >= 20000) {
          //sms send
          sms.beginSMS("2035559081"); //numbers must be preloaded
          sms.print("John fell off their bike and has not prevented this message from being send!"); //message must be preloaded
          sms.endSMS();
          Serial.println("\nCOMPLETE!\n");
        }
      }
      go = false;
    }
    usleep(40000);
  }

  
  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  x = g.gyro.x;
  y = g.gyro.y;
  z = g.gyro.z;
}
