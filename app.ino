#include <M5StickC.h>
#include <WiFiClientSecure.h>
#include <ssl_client.h>

#define INPUT_PIN 33
#define PUMP_PIN 32

const char  wifi_ssid[]  = "replase here";
const char  wifi_key[]   = "replase here";
const char* notify_host = "notify-api.line.me";
const char* notify_token1 = "replase here"; //乾いてた時
const char* notify_token2 = "replase here"; //潤ってた時

String sensor_moisture;

float vol, moisture, threshold_moisture;

void setup() {
    M5.begin();
    Serial.begin(115200);

    pinMode(INPUT_PIN, INPUT);
    pinMode(PUMP_PIN,OUTPUT);

    // WiFi接続
    Serial.printf("Connecting to %s ", wifi_ssid);
    WiFi.begin(wifi_ssid, wifi_key);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("CONNECTED");

    M5.Lcd.setRotation(1);
    M5.Axp.ScreenBreath(10);
    M5.lcd.setTextFont(2);
}

void send_notify(String msg, String token) {
    WiFiClientSecure client;
    client.setInsecure();
    Serial.println("Try Line Notify");
    if (!client.connect(notify_host, 443)) {
      delay(2000);
      return;
    }
    String query = String("message=") + String(msg);
    query = query + "&stickerId=10858&stickerPackageId=789";
    String request = String("") +
                 "POST /api/notify HTTP/1.1\r\n" +
                 "Host: " + notify_host + "\r\n" +
                 "Authorization: Bearer " + token + "\r\n" +
                 "Content-Length: " + String(query.length()) +  "\r\n" + 
                 "Content-Type: application/x-www-form-urlencoded\r\n\r\n" +
                  query + "\r\n";
    client.print(request);
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      Serial.println(line);
      if (line == "\r") {
        break;
      }
    }
    
    String line = client.readStringUntil('\n');
    Serial.println(line);
    client.stop();
}

void loop() {
    Serial.println();
    Serial.println("started....................");
    M5.lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE, BLACK);
    
    delay(5*1000);
    float test =  analogRead(INPUT_PIN);
    
    //Get the measured value
    float adc =  analogRead(INPUT_PIN);
          adc += analogRead(INPUT_PIN);
          adc += analogRead(INPUT_PIN);
          adc += analogRead(INPUT_PIN);
          adc += analogRead(INPUT_PIN);
          adc = adc / 5;

    vol = (adc + 1) * 3.3 / (4095 + 1);
    moisture  = 100 * (1.65 - vol) / (1.65 - 1.2);

    Serial.printf("Voltage: %2.2fV  Moisture: %0.2f%%\r\n", vol, moisture);

    //Judge the watering
    threshold_moisture = 20.0;//Turn on when 50% or less

    int32_t drawX=4 ,drawY = 16;
    char valStr[32];
    M5.lcd.drawString("watering ratio",drawX,drawY);
    drawY += 24;
    sprintf(valStr,"value : %2.1f%",moisture);
    M5.lcd.drawString(valStr,drawX,drawY);
  
    if (moisture <= threshold_moisture){
      Serial.println("Needs watering");

      digitalWrite(PUMP_PIN,HIGH);
      delay(1000*5);//pump discharge:8.3 ml/s * 5 (without height difference)
      digitalWrite(PUMP_PIN,LOW);

      sprintf(valStr,"乾いていたから水をあげた: %.2f %",moisture);
      send_notify(valStr, notify_token1);
    }else{
      sprintf(valStr,"土は潤っている: %.2f %",moisture);
      send_notify(valStr, notify_token2);
    }

    Serial.println(valStr);
    delay(5*1000);

    //スリープ
    Serial.println("...............Enter sleep");
    // 画面を消す
    M5.Axp.ScreenBreath(0);
    //60分deep sleep
    esp_sleep_enable_timer_wakeup(SLEEP_MIN(60));
    esp_deep_sleep_start();
}
