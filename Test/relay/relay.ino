void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available() > 0) {
    byte c = Serial.read();
    if (c == '1')
      digitalWrite(3, HIGH);
    if (c == '2')
      digitalWrite(3, LOW);
    if (c == '3')
      digitalWrite(4, HIGH);
    if (c == '4')
      digitalWrite(4, LOW);
  }
}
