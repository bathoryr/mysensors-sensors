//#define PRINT_ANALOG
volatile unsigned int pulseCounter = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(2, INPUT);
  pinMode(7, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(2), ISR_count, RISING);
}

void ISR_count()
{
  pulseCounter++;
}

void loop() {
  #ifdef PRINT_ANALOG
  loop_1s();
  #endif
  loop_5s();
  loop_led();
}

unsigned long timer_5s = 0L;
void loop_5s() {
  if (millis() - timer_5s > 5000) {
    timer_5s = millis();
    Serial.print("Pulse counter: ");
    Serial.println(pulseCounter);
  }
}
const float vref = (float)5 / 1024;
unsigned long timer_1s = 0L;
void loop_1s() {
  if (millis() - timer_1s > 1000) {
    timer_1s = millis();
    Serial.print("A0: ");
    int ar = analogRead(A0);
    Serial.print(ar);
    Serial.print(", ");
    Serial.print((float)ar * vref);
    Serial.println(" V");
  }
}

unsigned long timer_led_on = 0L;
unsigned long timer_led_cycle = 0L;
void loop_led()
{
  if (millis() - timer_led_cycle > 20) {
    timer_led_cycle = millis();
    digitalWrite(7, HIGH);
  }
  if (millis() - timer_led_on > 5) {
    timer_led_on = millis();
    digitalWrite(7, LOW);
  }
  /*
  if (millis() % 20 == 0)
    digitalWrite(7, HIGH);
  if (millis() % 5 == 0)
    digitalWrite(7, LOW);
  */
}

void serialEvent()
{
  if ((char)Serial.read() == '0') {
    pulseCounter = 0;
  }
}

