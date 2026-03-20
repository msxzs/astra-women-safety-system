#include <TinyGPS++.h>

// ================= CONFIG =================
#define SOS_BUTTON 4

// Contacts
String contacts[] = {
  "+91XXXXXXXXXX",
  "+91YYYYYYYYYY"
};
const int totalContacts = sizeof(contacts) / sizeof(contacts[0]);

// ================= OBJECTS =================
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);   // GPS
HardwareSerial gsmSerial(1);   // GSM

volatile bool sosPressed = false;

// ================= INTERRUPT =================
void IRAM_ATTR handleSOS() {
  sosPressed = true;
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(SOS_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SOS_BUTTON), handleSOS, FALLING);

  // GPS: RX=16, TX=17
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);

  // GSM: RX=26, TX=27
  gsmSerial.begin(115200, SERIAL_8N1, 26, 27);

  delay(3000);

  sendAT("AT");
  sendAT("AT+CMGF=1"); // SMS text mode

  Serial.println("SYSTEM READY");
}

// ================= LOOP =================
void loop() {
  // Keep reading GPS
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  if (sosPressed) {
    sosPressed = false;
    Serial.println("SOS TRIGGERED");

    float lat = 0, lng = 0;

    // 🔥 Wait up to 10 sec for GPS fix
    unsigned long start = millis();
    while (millis() - start < 10000) {
      while (gpsSerial.available()) {
        gps.encode(gpsSerial.read());
      }
      if (gps.location.isValid()) {
        lat = gps.location.lat();
        lng = gps.location.lng();
        break;
      }
    }

    sendSOS(lat, lng);
  }
}

// ================= FUNCTIONS =================

// Send AT command
void sendAT(String cmd) {
  gsmSerial.println(cmd);
  delay(1000);

  while (gsmSerial.available()) {
    Serial.write(gsmSerial.read());
  }
}

// Send SMS + Call
void sendSOS(float lat, float lng) {

  String message = "🚨 SOS ALERT!\n";

  if (lat != 0 && lng != 0) {
    message += "https://maps.google.com/?q=";
    message += String(lat, 6) + "," + String(lng, 6);
    message += "\nLat:" + String(lat, 6);
    message += "\nLng:" + String(lng, 6);
  } else {
    message += "Location not available";
  }

  // ===== SEND SMS TO ALL =====
  for (int i = 0; i < totalContacts; i++) {
    Serial.println("Sending SMS to: " + contacts[i]);

    gsmSerial.println("AT+CMGS=\"" + contacts[i] + "\"");
    delay(1000);

    gsmSerial.print(message);
    delay(500);

    gsmSerial.write(26); // CTRL+Z
    delay(5000);
  }

  Serial.println("All SMS sent");

  // ===== WAIT 10 SEC =====
  delay(10000);

  // ===== CALL ALL CONTACTS =====
  for (int i = 0; i < totalContacts; i++) {
    Serial.println("Calling: " + contacts[i]);

    gsmSerial.println("ATD" + contacts[i] + ";");
    delay(20000); // call for 20 sec

    gsmSerial.println("ATH"); // hang up
    delay(2000);
  }

  Serial.println("Call sequence done");
}
