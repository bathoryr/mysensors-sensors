void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(4, OUTPUT);
  Serial.println("Push '1' or '0' on keyboard");
}

void loop() {
  // put your main code here, to run repeatedly:

}

void serialEvent() {
  char inC = (char)Serial.read();
  if (inC == '0') {
    digitalWrite(4, LOW);
    Serial.println("Low");
  }
  if (inC == '1') {
    digitalWrite(4, HIGH);
    Serial.println("High");
  }
}

