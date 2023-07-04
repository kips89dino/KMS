#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SoftwareSerial.h>

const char* ssid = "Kips_dino_2.4G";// wifi ssid
const char* password = "Sariqbal";// password wifi
const char* host = "kytronkms.000webhostapp.com";
SoftwareSerial mySerial(D5, D6); // RX, TX pins

void setup() {
  Serial.begin(9600); // Initialize Serial communication
  mySerial.begin(9600); // Initialize SoftwareSerial communication

  connectToWiFi();
}

void loop() {
  if (mySerial.available()) {
    String data = mySerial.readStringUntil('\n'); // Read data from SoftwareSerial until newline character is received
    // Do something with the received data
    // For example, split the string into individual values
    String sendval1 = getValue(data, 0); // Extract first value
    String sendval2 = getValue(data, 1); // Extract second value
    String sendval3 = getValue(data, 2); // Extract third value
    
    // Print the received values
    Serial.println("Received data:");
    Serial.println("Value 1: " + sendval1);
    Serial.println("Value 2: " + sendval2);
    Serial.println("Value 3: " + sendval3);
    sendPOSTRequest(sendval1, sendval2, sendval3);
  }
}

String getValue(String data, int index) {
  int separatorIndex = 0;
  for (int i = 0; i < index + 1; i++) {
    separatorIndex = data.indexOf('&', separatorIndex + 1);
    if (separatorIndex == -1) {
      return "";
    }
  }
  int nextSeparatorIndex = data.indexOf('&', separatorIndex + 1);
  if (nextSeparatorIndex == -1) {
    nextSeparatorIndex = data.length();
  }
  return data.substring(separatorIndex + 1, nextSeparatorIndex);
}

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to ");
  Serial.print(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void sendPOSTRequest(const String& val1, const String& val2, const String& val3) {
  String postData = "sendval1=" + val1 + "&sendval2=" + val2 + "&sendval3=" + val3;

  HTTPClient http;
  WiFiClient client;

  http.begin(client, "http://kytronkms.000webhostapp.com/testpost.php");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpCode = http.POST(postData);

  if (httpCode == 200) {
    String response = http.getString();
    Serial.println("POST request sent successfully");
    Serial.println(response);
  } else {
    Serial.print("Error sending POST request. HTTP error code: ");
    Serial.println(httpCode);
  }

  http.end();
}
