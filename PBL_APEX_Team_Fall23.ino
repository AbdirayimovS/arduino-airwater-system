#include <SoftwareSerial.h> // Esp library
#include <LiquidCrystal_I2C.h> //LCD Library
#include <DHT.h> //Library from AdaFruit for DHT11 Temp and Hum Sensor


#define DHT11_PIN 8 // tempreature sensor pin
#define AO_PIN A0 //gas sensor pin
DHT dht(DHT11_PIN, DHT11);
// float sensorres;
LiquidCrystal_I2C lcd(0x27,8,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
// bool system_ok = true;
String apiKey = "B5AC2FNKI34F40R1"; // thingspeak API to write data
SoftwareSerial ser(6,7); // RX, TX

byte co2Sign[8] = {
    B01110,
    B10001,
    B10001,
    B10001,
    B10001,
    B10001,
    B01110,
    B00000
  };
byte gasSign[8] = {
    B00100,
    B01110,
    B11111,
    B11111,
    B11111,
    B01110,
    B00100,
    B00000
  };
byte temperatureSign[8] = {
  B00100,
  B01010,
  B01010,
  B01110,
  B01110,
  B11111,
  B11111,
  B01110
};
byte loveSign[8] = {
	0b00000,
	0b01010,
	0b11111,
	0b11111,
	0b01110,
	0b00100,
	0b00000,
	0b00000
};
byte humiditySign[8] = {
  B00100,
  B01010,
  B01010,
  B00100,
  B10001,
  B10001,
  B01110,
  B00000
};
byte networkSymbol[8] = {
  B00000,
  B00000,
  B10000,
  B10000,
  B10100,
  B10100,
  B10101,
  B10101
  };

void errorLCD(String err){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(err);
}

void updateLCD(float tem, float hum, int gas, float photoresistor, int i){
  lcd.clear(); 
  // Upper Left side of LCD
  lcd.setCursor(0,0);
  lcd.write((byte)1);
  lcd.print(String(tem));
  // Upper Right side of LCD
  lcd.setCursor(7, 1);
  lcd.write((byte)4); // gas sign
  lcd.print(String(gas));
  // Lower right of LCD
  lcd.setCursor(0,1);
  lcd.write((byte)2);
  lcd.print(String(hum));
  if (i > 9){
    lcd.setCursor(14, 1);
    lcd.print(String(i));
    }
  else{
    lcd.setCursor(15, 1);
    lcd.print(String(i));
  }
  // Network Status
  lcd.setCursor(15,0);
  lcd.write((byte)3);
  
  // Lower left side of LCD
  lcd.setCursor(7,0);
  lcd.write((byte)0); //photoresistor values
  lcd.print(String(photoresistor));
  // Working Condition
  // lcd.setCursor(15,1);
  // lcd.write((byte)0); // Liveness icon

}


void setup(){
  pinMode(A2, INPUT); // input port for photoresistor
  dht.begin(); // Initialize DHT Sensors
  lcd.init();     
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("AirWatch System");
  lcd.setCursor(9,1);
  lcd.print("by APEX");
  lcd.createChar(0, loveSign);
  lcd.createChar(1, temperatureSign);
  lcd.createChar(2, humiditySign);
  lcd.createChar(3, networkSymbol);
  lcd.createChar(4, gasSign);
  lcd.createChar(5, co2Sign);
  // Gas Sensor must get warm, It is recomended to wait 15 seconds
  delay(4000); //2 seconds of delay
  for (int i=0; i<4; i++){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Initializing");
    if (i == 1) {
      lcd.print(".");}
    else if (i == 2){
      lcd.print("..");
    }
    else if (i == 3){
      lcd.print("...");
    }
    lcd.setCursor(13,1);
    lcd.print(i);
    lcd.print("/3");
    delay(4000);
  }
  // Start Communication between Arduino & ESP-01
  ser.begin(9600);
  Serial.begin(9600);  
  unsigned char check_connection=0;
  unsigned char times_check=0;
  Serial.println("Connecting to Wifi");

  while(check_connection==0){
    Serial.print("..");
    ser.print("AT+CWJAP=\"4Lab\",\"\"\r\n"); // use 4LAB SSID and password ""
    ser.setTimeout(5000);
    if(ser.find("WIFI CONNECTED\r\n")==1 ){
      Serial.println("WIFI CONNECTED");
      break;
      }
    times_check++;
    if(times_check>3) 
      {
        times_check=0;
        Serial.println("Trying to Reconnect...");
        errorLCD("Trying to Reconnect...");
        }
      }
    delay(5000);
  }

void loop(){
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int gasValue = analogRead(AO_PIN); // it returns only int values. yes i wanted float. but not float
  float photoresistor = analogRead(A2); // Photoresistor Update
  // Check if any reads failed and exit early (to try again):
  if (isnan(h) || isnan(t) || isnan(gasValue) || isnan(photoresistor)) {
    errorLCD("Failed DHT sensor!");
    return;
  }
  // TCP connection
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += "184.106.153.149"; // api.thingspeak.com
  cmd += "\",80";
  ser.println(cmd);

  if(ser.find("Error")){
    Serial.println("AT+CIPSTART error");
    errorLCD("AT+CIPSTART error"); //print to lcs the error if exists
    return;
  }
  // prepare GET string
  String getStr = "GET /update?api_key=";
  getStr += apiKey;
  getStr +="&field1=";
  getStr += String(t); // for temperature
  getStr +="&field2=";
  getStr += String(h); // for humidity
  getStr +="&field3=";
  getStr += String(gasValue); // for gas sensor
  getStr +="&field4=";
  // QOYILBEKKKKKKKKKKKKKKK LOOOOOOOOKKKKKK HEREREEEEEE!!!!!!!!!
  getStr += String(photoresistor); // for photoresistor
  getStr += "\r\n\r\n";
  // send data length
  cmd = "AT+CIPSEND=";
  cmd += String(getStr.length());
  String len_of_package = String(getStr.length()); // len of Sending package
  Serial.println(cmd);
  ser.println(cmd);
  if(ser.find(">")){
    ser.print(getStr);
    Serial.println(getStr);
  }
  else{
    ser.println("AT+CIPCLOSE");
    Serial.println("CIPCLOSE");
  }

  for (int i=0; i<61; i++){
    updateLCD(t, h, gasValue, photoresistor, i); //giving humidity and tempterature as celsious
    // thingspeak needs 16 sec delay between updates
    delay(1000);
    }
   // we will update it to 5 minutes or 30 minutes
  } 