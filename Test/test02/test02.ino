unsigned int i = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(38400);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println(i++);
  delay(2000);
}
