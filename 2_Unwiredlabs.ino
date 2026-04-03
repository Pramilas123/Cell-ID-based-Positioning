#include <Arduino.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>

#define MODEM_RX 27
#define MODEM_TX 26
#define MODEM_PWR 4
HardwareSerial SerialAT(1);  // UART1 for modem

String httpResponse = "";

struct CellTower {
  int mcc;
  int mnc;
  int tac;
  int cellid;
};

void sendAT(String cmd, unsigned long timeout = 3000) {
  SerialAT.println(cmd);
  unsigned long start = millis();
  while (millis() - start < timeout) {
    while (SerialAT.available()) {
      String line = SerialAT.readStringUntil('\n');
      line.trim();
      if (line.length()) {
        Serial.println(line);
        httpResponse += line + "\n";
      }
    }
  }
}

bool parseCPSI(String cpsi, CellTower &cell) {
  int start = cpsi.indexOf("+CPSI:");
  if (start < 0) return false;

  String line = cpsi.substring(start);
  line.replace("+CPSI:", "");
  line.trim();

  String fields[15];
  int fieldCount = 0, lastIndex = 0;
  for (int i = 0; i < line.length(); i++) {
    if (line.charAt(i) == ',') {
      fields[fieldCount++] = line.substring(lastIndex, i);
      lastIndex = i + 1;
    }
  }
  fields[fieldCount++] = line.substring(lastIndex);

  if (fieldCount < 5) return false;

  String mccmnc = fields[2];
  int dash = mccmnc.indexOf('-');
  if (dash == -1) return false;

  cell.mcc = mccmnc.substring(0, dash).toInt();
  cell.mnc = mccmnc.substring(dash + 1).toInt();

  String tacStr = fields[3];
  cell.tac = tacStr.startsWith("0x") ? strtol(tacStr.c_str(), NULL, 16) : tacStr.toInt();

  cell.cellid = fields[4].toInt();

  return true;
}

void setup() {
  Serial.begin(115200);
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  pinMode(MODEM_PWR, OUTPUT);
  digitalWrite(MODEM_PWR, LOW);
  delay(1000);
  digitalWrite(MODEM_PWR, HIGH);
  delay(10000);

  Serial.println("Initializing modem...");
  sendAT("AT");
  sendAT("AT+CFUN=1");
  sendAT("AT+CPIN?");
  sendAT("AT+CSQ");
  sendAT("AT+CREG?");
  sendAT("AT+CGATT=1");

  Serial.println("Fetching LTE cell info...");
  SerialAT.println("AT+CPSI?");
  delay(1000);
  String cpsires = SerialAT.readString();
  Serial.println("Raw +CPSI response:");
  Serial.println(cpsires);

  CellTower primary;
  if (!parseCPSI(cpsires, primary)) {
    Serial.println("Failed to parse primary cell info.");
    return;
  }

  Serial.printf("Primary -> MCC: %d, MNC: %d, TAC: %d, CellID: %d\n",
                primary.mcc, primary.mnc, primary.tac, primary.cellid);

  String jsonPayload = "{\"token\":\"pk.c31b06cebed5d6e677a0a1ddddc9e688\",";
  jsonPayload += "\"radio\":\"lte\",";
  jsonPayload += "\"mcc\":" + String(primary.mcc) + ",";
  jsonPayload += "\"mnc\":" + String(primary.mnc) + ",";
  jsonPayload += "\"cells\":[{\"lac\":" + String(primary.tac) + ",";
  jsonPayload += "\"cid\":" + String(primary.cellid) + ",";
  jsonPayload += "\"psc\":0}],";
  jsonPayload += "\"address\":1}";

  Serial.println("Sending JSON to UnwiredLabs:");
  Serial.println(jsonPayload);

  sendAT("AT+HTTPINIT");
  sendAT("AT+HTTPPARA=\"URL\",\"http://ap1.unwiredlabs.com/v2/process.php\"");
  sendAT("AT+HTTPPARA=\"CONTENT\",\"application/json\"");

  int len = jsonPayload.length();
  sendAT("AT+HTTPDATA=" + String(len) + ",10000");
  delay(100);
  SerialAT.print(jsonPayload);
  delay(1000);

  sendAT("AT+HTTPACTION=1", 10000);
  delay(3000);

  httpResponse = "";
  sendAT("AT+HTTPREAD=0,512", 5000);  // Read first 512 bytes

  int jsonStart = httpResponse.indexOf('{');
  int jsonEnd = httpResponse.lastIndexOf('}');
  if (jsonStart != -1 && jsonEnd != -1) {
    String json = httpResponse.substring(jsonStart, jsonEnd + 1);
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, json);
    if (!error) {
      Serial.println("-- Response --");
      Serial.print("Status: ");   Serial.println(doc["status"].as<const char*>());
      Serial.print("Balance: ");  Serial.println(doc["balance"].as<int>());
      Serial.print("Latitude: "); Serial.println(doc["lat"].as<float>(), 6);
      Serial.print("Longitude: ");Serial.println(doc["lon"].as<float>(), 6);
      Serial.print("Accuracy: "); Serial.println(doc["accuracy"].as<int>());
      Serial.print("Address: ");  Serial.println(doc["address"].as<const char*>());
    } else {
      Serial.println("Failed to parse JSON response!");
      Serial.println(error.c_str());
    }
  } else {
    Serial.println("No valid JSON found in response.");
  }

  sendAT("AT+HTTPTERM");
}

void loop() {
  // Nothing here
}