void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.print("setup(), millis()=");
  Serial.println(millis());
}

void loop() {
  // put your main code here, to run repeatedly:
  loop60s();
  loop15min();
}

unsigned long timer60s = 0;
void loop60s()
{
  if (millis() - timer60s > (unsigned long)60 * 1000) {
    timer60s = millis();
    Serial.print("Timer at 60s. millis()=");
    Serial.println(timer60s);
  }
}

unsigned long timer15min = 0;
void loop15min()
{
  if (millis() - timer15min > (unsigned long)15 * 10 * 100) {
    timer15min = millis();
    Serial.print("Timer at 15s. millis()=");
    Serial.println(timer15min);
  }
}

