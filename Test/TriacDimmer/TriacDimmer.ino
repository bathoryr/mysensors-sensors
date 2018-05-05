volatile byte pulseCounter = 0;

void setup() 
{
  Serial.begin(115200);
  pinMode(2, INPUT);
  attachInterrupt(digitalPinToInterrupt(2), ISR_count, RISING);
}

void ISR_count()
{
  pulseCounter++;
}

void loop() 
{
  loop_1s();
}

unsigned long timer_1s = 0L;
void loop_1s() 
{
  if (millis() - timer_1s > 1000) {
    timer_1s = millis();
    Serial.println(pulseCounter);
    pulseCounter = 0;
  }
}

