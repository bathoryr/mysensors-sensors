class Light {
    protected:
    bool lightState;
    unsigned long timerLight;
    // Light timeout in minutes
    byte lightTimeout = 5;
    int lightIntensity = 50;

    void ResetTimer() {
        timerLight = millis();
    }


    public:
    Light() {
        lightState = false;
        timerLight = millis();
    }

    bool IsLightOn() {
        return lightState;
    }

    bool MotionDetected() {
        return digitalRead(MOTION_PIN) == HIGH;
    }

    void TurnOff() {
        digitalWrite(OUTPUT_PIN, LOW);
        lightState = false;
    }

    void TurnOn() {
        analogWrite(OUTPUT_PIN, lightIntensity);
        lightState = true;
        ResetTimer();
    }

    void SetIntensity(int percent) {
        lightIntensity = map(percent, 0, 100, 0, 255);
        if (IsLightOn()) {
            analogWrite(OUTPUT_PIN, lightIntensity);
        }
    }

    void SetTimeout(byte minutes) {
        lightTimeout = minutes;
    }

    void CheckMotion() {
        bool motion = MotionDetected();
        digitalWrite(MOTION_LED, motion ? HIGH : LOW);
        if (IsLightOn()) {
            if (millis() - timerLight > lightTimeout * 60000) {
                TurnOff();
            }
            if (motion) {
                ResetTimer();
            }
        }
        else {
            if (motion) {
                TurnOn();
            }
        }
    }
};
