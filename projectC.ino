

/************************Hardware Related Macros************************************/
#define         MQ_PIN                       (5) 
#define         RL_VALUE                     (5)  
#define         RO_CLEAN_AIR_FACTOR          (9.83)  

#define         CALIBARAION_SAMPLE_TIMES     (50)   
#define         CALIBRATION_SAMPLE_INTERVAL  (500) 
#define         READ_SAMPLE_INTERVAL         (50)  
#define         READ_SAMPLE_TIMES            (5)  


#define         GAS_LPG                      (0)
#define         GAS_CO                       (1)
#define         GAS_SMOKE                    (2)

float           LPGCurve[3]  =  {2.3,0.21,-0.47};  
float           COCurve[3]  =  {2.3,0.72,-0.34};
float           SmokeCurve[3] ={2.3,0.53,-0.44};                                                   
float           Ro           =  16;           
int ledPin = 13;

#include "DHT.h"
DHT dht(2, DHT22);
float tempsetting = 10;

//LINE
#include <CytronWiFiShield.h>
#include <CytronWiFiClient.h>
#include <SoftwareSerial.h>


const char* ssid = "ccc";
const char* pass = "02134567";

const char* host = "notify-api.line.me";
const int httpsPort = 443;
const char* lineToken = "Uyxn2WcF4AwxEOmr5AKnHxe0iznAN6fzKiml4ssv8CL";

ESP8266Client client;



void setup()
{
  Serial.begin(9600);                      
  if(!wifi.begin(2, 3))
    Serial.println(F("Error talking to shield"));
  Serial.println(F("Start wifi connection"));
  if(!wifi.connectAP(ssid, pass))
    Serial.println(F("Error connecting to WiFi")); 
  Serial.print(F("Connected to: "));Serial.println(wifi.SSID());
  Serial.print(F("IP address: "));Serial.println(wifi.localIP());
  Serial.print("Calibrating...\n");                
  Ro = MQCalibration(MQ_PIN);             
                  
  Serial.print("Calibration is done...\n"); 
  Serial.print("Ro=");
  Serial.print(Ro);
  Serial.print("kohm");
  Serial.print("\n");
  pinMode(ledPin, OUTPUT); 

  //LINE
    while (!Serial) {
    ; 

  //ไฟ
  dht.begin();

  }
}

void loop()
{
   Serial.print("LPG:"); 
   Serial.print(MQGetGasPercentage(MQRead(MQ_PIN)/Ro,GAS_LPG) );
   Serial.print( "ppm" );
   Serial.print("    ");   
   Serial.print("CO:"); 
   Serial.print(MQGetGasPercentage(MQRead(MQ_PIN)/Ro,GAS_CO) );
   Serial.print( "ppm" );
   Serial.print("    ");   
   Serial.print("SMOKE:"); 
   Serial.print(MQGetGasPercentage(MQRead(MQ_PIN)/Ro,GAS_SMOKE) );
   Serial.print( "ppm" );
   Serial.print("\n");

delay(2000);   
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" *C ");

  if (((MQGetGasPercentage(MQRead(MQ_PIN)/Ro,GAS_LPG)) > 5) || ((MQGetGasPercentage(MQRead(MQ_PIN)/Ro,GAS_CO)) > 5) || (MQGetGasPercentage(MQRead(MQ_PIN)/Ro,GAS_SMOKE) > 5) || (t >= tempsetting)) { 
    digitalWrite(ledPin, HIGH); // สั่งให้ LED ติดสว่าง
      sendLineNotify("ไฟไหม้!!!!!!!!!!!");
      delay(1000);
  }
  else {
    digitalWrite(ledPin, LOW); // สั่งให้ LED ดับ
  }


}

float MQResistanceCalculation(int raw_adc)
{
  return ( ((float)RL_VALUE*(1023-raw_adc)/raw_adc));
}


float MQCalibration(int mq_pin)
{
  int i;
  float val=0;

  for (i=0;i<CALIBARAION_SAMPLE_TIMES;i++) {    
    val += MQResistanceCalculation(analogRead(mq_pin));
    delay(CALIBRATION_SAMPLE_INTERVAL);
  }
  val = val/CALIBARAION_SAMPLE_TIMES;       

  val = val/RO_CLEAN_AIR_FACTOR;     


  return val; 
}

float MQRead(int mq_pin)
{
  int i;
  float rs=0;

  for (i=0;i<READ_SAMPLE_TIMES;i++) {
    rs += MQResistanceCalculation(analogRead(mq_pin));
    delay(READ_SAMPLE_INTERVAL);
  }

  rs = rs/READ_SAMPLE_TIMES;

  return rs;  
}

int MQGetGasPercentage(float rs_ro_ratio, int gas_id)
{
  if ( gas_id == GAS_LPG ) {
     return MQGetPercentage(rs_ro_ratio,LPGCurve);
  } else if ( gas_id == GAS_CO ) {
     return MQGetPercentage(rs_ro_ratio,COCurve);
  } else if ( gas_id == GAS_SMOKE ) {
     return MQGetPercentage(rs_ro_ratio,SmokeCurve);
  }    

  return 0;
}


int  MQGetPercentage(float rs_ro_ratio, float *pcurve)
{
  return (pow(10,( ((log(rs_ro_ratio)-pcurve[1])/pcurve[2]) + pcurve[0])));
}

//LINE
void sendLineNotify(String message) {
  Serial.print("Connecting to ");
  Serial.println(host);
  if (!client.secure_connect(host, httpsPort))
  {
    Serial.println(F("Failed to connect to server."));
    return;
  }
  
  String url = "/api/notify";
  Serial.print("requesting URL: ");
  Serial.println(url);

  String postData = "message=" + message;

  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Authorization: Bearer " + lineToken + "\r\n" +
               "Content-Type: application/x-www-form-urlencoded\r\n" +
               "Content-Length: " + postData.length() + "\r\n" +
               "User-Agent: Arduino\r\n" +
               "Connection: keep-alive\r\n\r\n" +
               postData);

  Serial.println("request sent");

  int i=5000;
  while (client.available()<=0&&i--)
  {
    delay(1);
    if(i==1) {
      Serial.println(F("Timeout"));
      return;
      }
  }
  
}