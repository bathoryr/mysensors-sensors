#define PIN 4

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(PIN, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  
}

void serialEvent() {
  if (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '0') {
      Serial.println("Output low");
      digitalWrite(PIN, LOW);
    }
    if (c == '1') {
      Serial.println("Output high");
      digitalWrite(PIN, HIGH);
    }
  }
}

