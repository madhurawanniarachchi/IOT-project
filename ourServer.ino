/*==================GSM SETUP====================*/
#include <SoftwareSerial.h>
SoftwareSerial gprsSerial(2, 3);

/*================DHT 11 Setup=================*/
#include <dht.h>
dht temp;

/*================CO2 Setup=====================*/
#define READ_SAMPLE_INTERVAL (50)
#define READ_SAMPLE_TIMES    (5)

#define LOG_400 (2.602)
#define LOG_1000 (3)

#define LOG_400_VOLTAGE (0.90)
#define LOG_1000_VOLTAGE (0.10)

double co2_log_slope = (LOG_1000_VOLTAGE - LOG_400_VOLTAGE) / (LOG_1000 - LOG_400);
double v400 = LOG_400_VOLTAGE - co2_log_slope * LOG_400;
//double val, voltage, real, co2ppm;

/*===================================PM SENSOR===============================================*/
#include <Arduino.h>
#include <SoftwareSerial.h>
#define LENG 31   //0x42 + 31 bytes equal to 32 bytes
unsigned char buf[LENG];

int PM01Value=0;          //define PM1.0 value of the air detector module
int PM2_5Value=0;         //define PM2.5 value of the air detector module
int PM10Value=0;         //define PM10 value of the air detector module

SoftwareSerial PMSerial(10,11); // RX, TX

String device_id = "0001";
void setup() {
  Serial.begin(9600);
  /*==================GSM SETUP====================*/
  gprsSerial.begin(9600);
  Serial.println("Config SIM900...");
  delay(2000);
  Serial.println("Done!...");
  gprsSerial.flush();
  Serial.flush();

  /*================DHT 11 Setup===================*/

  /*================CO2 Setup=====================*/
  pinMode(A0, INPUT);

  /*================CO Setup=====================*/
  pinMode(A1, INPUT);

  /*===================================PM SENSOR===============================================*/
  PMSerial.begin(9600);   
  PMSerial.setTimeout(1500);
}



void loop() {
  gprsSerial.listen();
  // put your main code here, to run repeatedly:
  // attach or detach from GPRS service
  gprsSerial.println("AT+CGATT?");
  delay(100);
  toSerial();

  // bearer settings
  gprsSerial.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
  delay(2000);
  toSerial();

  // bearer settings
  gprsSerial.println("AT+SAPBR=3,1,\"APN\",\"mobitel\"");
  delay(2000);
  toSerial();

  //bearer settings
  gprsSerial.println("AT+SAPBR=1,1");
  delay(6000);
  toSerial();

  gprsSerial.println("AT+SAPBR=2,1");
  delay(1000);
  toSerial();

  gprsSerial.println("AT+HTTPINIT");
  delay(1000);

  gprsSerial.println("AT+HTTPPARA=\"CID\",1");
  delay(1000);
  /*========================GSM setUp Complete============================*/

  /*========================DHT 11============================*/
  int val = temp.read11(8);
  String cstr = getDhtT(val);
  String hstr = getDhtH(val);
/*================CO2 Setup=====================*/ 
  String CO2ppm = getCo2();

/*================CO Setup=====================*/
  int val_co = analogRead(A1);
  String reading = String(val_co);
  String coppm = getCO(val_co);

/*===================================PM SENSOR===============================================*/
PMSerial.listen();
  String pm1;
  String pm2_5;
  String pm10;
 if(PMSerial.find(0x42)){    
    PMSerial.readBytes(buf,LENG);

    if(buf[0] == 0x4d){
      if(checkValue(buf,LENG)){
        PM01Value=transmitPM01(buf); //count PM1.0 value of the air detector module
        PM2_5Value=transmitPM2_5(buf);//count PM2.5 value of the air detector module
        PM10Value=transmitPM10(buf); //count PM10 value of the air detector module 
      }           
    } 
  }

  static unsigned long OledTimer=millis();  
    if (millis() - OledTimer >=1000) 
    {
      OledTimer=millis(); 
      
     pm1 = String(PM01Value);                
     pm2_5 = String(PM2_5Value); 
     pm10 = String(PM10Value);   
    }
  gprsSerial.listen();  
  Serial.println("Temperature is " + cstr + "\tHumidity is " + hstr + "\tCO2 " + CO2ppm + "ppm" + "\tCO " + coppm + "ppm " +reading+ "\tpm1.0 "+pm1+"ug"+"\tpm2_5 "+pm2_5+"ug"+"\tpm10 "+pm10+"ug");
  delay(100);
  // set http param value
  gprsSerial.print("AT+HTTPPARA=\"URL\",\"http://example.com/getPara/test_1.php?");
  gprsSerial.print("device_id=");
  gprsSerial.print(device_id);
  gprsSerial.print("&temp=");
  gprsSerial.print(cstr);
  gprsSerial.print("&humi=");
  gprsSerial.print(hstr);
  gprsSerial.print("&co2=");
  gprsSerial.print(CO2ppm);
  gprsSerial.print("&co=");
  gprsSerial.print(coppm);
  gprsSerial.print("&pm25=");
  gprsSerial.print(pm2_5);
  gprsSerial.print("&pm10=");
  gprsSerial.print(pm10);
  gprsSerial.println("\"");
  delay(5000);
  toSerial();

  // set http action type 0 = GET, 1 = POST, 2 = HEAD
  gprsSerial.println("AT+HTTPACTION=0");
  delay(6000);
  toSerial();

  gprsSerial.println("");
  delay(1000);

}


/*==================GSM SETUP====================*/
void toSerial()
{
  while (gprsSerial.available() != 0)
  {
    Serial.write(gprsSerial.read());
  }
}

/*========================DHT 11============================*/
String getDhtT(int x) {

  float c = temp.temperature;
  return String(c);
}

String getDhtH(int x) {

  float h = temp.humidity;
  return String(h);
}

/*================CO2 Setup=====================*/
double MG811ReportRaw() {
  double volts;
  volts = MG811Read(A0);
  Serial.print("Reading: ");
  Serial.print(volts);
  Serial.print("V\n");
  return volts;
}

double MG811Read(int pin) {
  int i;
  double v = 0;
  for (i = 0; i < READ_SAMPLE_TIMES; i++) {
    v += analogRead(pin);
    delay(READ_SAMPLE_INTERVAL);
  }
  // Convert digital to analog value.
  v = (v / READ_SAMPLE_TIMES) * 5 / 1024;
  return v;
}

int MG811ToPPM(double volts) {
  if (volts >= LOG_400_VOLTAGE) {
    return 400;
  } else {
    return pow(10, (volts - v400) / co2_log_slope);
  }
}

String getCo2(){
  double ppmA;
  for (int i = 0; i < 20; i++) {
    double v = MG811ReportRaw();
    double ppm = MG811ToPPM(v);
     ppmA= ppmA + ppm;
    delay(1000);
  }
  int avg= ppmA / 20;
  return String(avg);
  ppmA = 0;
}

/*===========================CO SENSOR===============================*/
String getCO(int val){

  float co = map(val,0,1023,0,100);
  return String(co);  
}
 /*===================================PM SENSOR===============================================*/
 char checkValue(unsigned char *thebuf, char leng)
{  
  char receiveflag=0;
  int receiveSum=0;

  for(int i=0; i<(leng-2); i++){
  receiveSum=receiveSum+thebuf[i];
  }
  receiveSum=receiveSum + 0x42;
 
  if(receiveSum == ((thebuf[leng-2]<<8)+thebuf[leng-1]))  //check the serial data 
  {
    receiveSum = 0;
    receiveflag = 1;
  }
  return receiveflag;
}

int transmitPM01(unsigned char *thebuf)
{
  int PM01Val;
  PM01Val=((thebuf[3]<<8) + thebuf[4]); //count PM1.0 value of the air detector module
  return PM01Val;
}

//transmit PM Value to PC
int transmitPM2_5(unsigned char *thebuf)
{
  int PM2_5Val;
  PM2_5Val=((thebuf[5]<<8) + thebuf[6]);//count PM2.5 value of the air detector module
  return PM2_5Val;
}

//transmit PM Value to PC
int transmitPM10(unsigned char *thebuf)
{
  int PM10Val;
  PM10Val=((thebuf[7]<<8) + thebuf[8]); //count PM10 value of the air detector module  
  return PM10Val;
}
