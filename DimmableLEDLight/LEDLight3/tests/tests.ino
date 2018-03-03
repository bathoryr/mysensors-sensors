#define OUTPUT_PIN 5

void setup() {
    Serial.begin(115200);
    pinMode(OUTPUT_PIN, OUTPUT);

}

void loop() {

}

void serialEvent() {
    while(Serial.available()) {
        char c = (char)Serial.read();
        if (c == 'w')
            analogWrite(OUTPUT_PIN, 100);
        if (c == 'a')
            digitalWrite(OUTPUT_PIN, HIGH);
        if (c == 'n')
            digitalWrite(OUTPUT_PIN, LOW);
        if (c == 'r') {
            Serial.print("Read dgpin: ");
            Serial.println(digitalRead(OUTPUT_PIN));
        }
    }
}