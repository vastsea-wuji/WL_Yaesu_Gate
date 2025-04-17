#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SoftwareSerial.h>

// WiFi配置
const char* ssid = "UR SSID";
const char* password = "URPASS"; 
const char* serverUrl = "http://{UR WAVELOG URL}/api/radio";

String key = "UR API KEY";
String radio = "FT-710";
String frequency = "21000000";
String mode = "SSB";
String power = "5";

bool sthChanged = false;
String serial_Input = "";

SoftwareSerial uart1(14,12);//RX=d5,TX=d6
// 定时器
unsigned long previousTime1 = 0;  // 上次激活时间 --问一下VS和PC
const long interval1 = 500;    // 10秒（单位：毫秒）
unsigned long previousTime2 = 0;  // 上次上传时间 --一直有变动的情况下最快2S上传一次数据
const long interval2 = 1000;    // 两次上传之间的最小间隔 2秒（单位：毫秒）
unsigned long previousTime3 = 0;  // 上次激活时间 --用于保底定时上传
const long interval3 = 15000;    // 15秒（单位：毫秒）
void requestData(){
  uart1.print("VS;");
  uart1.print("PC;");
  // uart1.print("AI1;");
}
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);  // Initialize the LED_BUILTIN pin as an output
  uart1.begin(38400);
  uart1.listen();
  Serial.begin(38400);
  requestData();
  connectWiFi();
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(LED_BUILTIN, HIGH);
}
void loop() {
  // if (millis() - previousTime1 >= interval1) {
  //   previousTime1 = millis(); // 重置计时器
  //   //定时问一下vs和pc
  //   requestData();
  // }
  
  Serial.println("loop start");
  
  requestData();
  Serial.println("requestData end");
  delay(500);
  Serial.println("delay end");
  
  while (uart1.available() > 0) {
  Serial.println("while ing");
    char c = uart1.read();
    serial_Input += c;
    delay(3);
    if(c == ';') {
      serial_Input.replace("\n", "");
      serial_Input.replace("\r", "");
      handlingCallbacks(serial_Input);
      serial_Input = "";
      delay(3);
    }
  }
  Serial.println("while end");

  // 上传服务器
  if((sthChanged && millis() - previousTime2 >= interval2) || millis() - previousTime3 >= interval3) {
    previousTime2 = previousTime3 = millis(); // 重置计时器
    uploadData();
  }
  Serial.println("uploadData end");
}
void uploadData() {
  sthChanged = false;
  WiFiClient client;
  HTTPClient http;
  
  String payload = String() + 
    "{\"frequency\":" + frequency + 
    ",\"key\":\"" + key + "\"" +
    ",\"radio\":\"" + radio + "\"" +
    ",\"mode\":\"" + mode + "\"" +
    ",\"power\":" + power + "}";

  if(http.begin(client, serverUrl)) {
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST(payload);
    if(httpCode == 200) {
      Serial.println("UPDATE成功：" + payload);
    } else {
      Serial.println("UPDATE失败，代码：" + String(httpCode));
      Serial.println(payload);
    }
    http.end();
  }
}
// void serialEvent() {
//   while (Serial.available() > 0) {
//     char c = Serial.read();
//     serial_Input += c;
//     delay(3);
//     if(c == ';') {
//       serial_Input.replace("\n", "");
//       serial_Input.replace("\r", "");
//       handlingCallbacks(serial_Input);
//       serial_Input = "";
//       delay(3);
//     }
//   }
// }
void setFrequency(String freq) {
  //去除前导0
  while(freq.startsWith("0")){
    freq = freq.substring(1);
  }
  frequency = freq;
  sthChanged = true;
  Serial.println("setFreq" + freq);
}
void setPower(String pow) {
  //去除前导0
  while(pow.startsWith("0")){
    pow = pow.substring(1);
  }
  if(pow.toInt() >= 0 && pow.toInt() <= 100){
    power = pow;
  }else{
    power = "0";
  }
  sthChanged = true;
  Serial.println("setPwr" + pow);
}
void setMode(String mod) {
  if(mod == "1"){
    mode = "LSB";
  }else if(mod == "2"){
    mode = "USB";
  }else if(mod == "3"){
    mode = "CW-U";
  }else if(mod == "4"){
    mode = "FM";
  }else if(mod == "5"){
    mode = "AM";
  }else if(mod == "6"){
    mode = "RTTY-L";
  }else if(mod == "7"){
    mode = "CW-L";
  }else if(mod == "8"){
    mode = "DATA-L";
  }else if(mod == "9"){
    mode = "RTTY-U";
  }else if(mod == "A"){
    mode = "DATA-FM";
  }else if(mod == "B"){
    mode = "FM-N";
  }else if(mod == "C"){
    mode = "DATA-U";
  }else if(mod == "D"){
    mode = "AM-N";
  }else if(mod == "E"){
    mode = "PSK";
  }else if(mod == "F"){
    mode = "DATA-FM-N";
  }else{
    mode = "";
  }
  sthChanged = true;
  
  Serial.println("setMode" + mod);
}
void handlingCallbacks(String Input){   
  Serial.println("\n串口收到" + Input);
  //PC030; power=30
  //IF000014069300+000000C00000;
  //IF后前三位舍弃,后9位是Hz为单位的频率，后面一位的符号舍弃，后面六位数字舍弃，之后一位是模式代号，之后的都舍弃

  if(Input.startsWith("VS")){
    if(Input.substring(2,3)=="0"){
      //串口发IF
      uart1.print("IF;");
    }else if(Input.substring(2,3)=="1"){
      //串口发OI
      uart1.print("OI;");
    }
  }else if(Input.startsWith("IF") || Input.startsWith("OI")){
    setFrequency(Input.substring(4, 14));
    setMode(Input.substring(21, 22));
  }else if(Input.startsWith("PC")){
    setPower(Input.substring(2,5));
  }
}
void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while(WiFi.status() != WL_CONNECTED){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print(".");
  }
  Serial.println("\nConnected, IP: " + WiFi.localIP().toString());
}
