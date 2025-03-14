#define SENSOR_PIN 1  // Define analog pin
#define samp_siz 4
#define rise_threshold 4

// Pulse Monitor Test Script
void setup() {
    Serial.begin(115200);
}

void loop() {
    float reads[samp_siz], sum;
    long int now, ptr;
    float last, reader, start;
    float first = 0, second = 0, third = 0, before = 0, print_value;
    bool rising = false;
    int rise_count = 0;
    int n;
    long int last_beat = 0;

    for (int i = 0; i < samp_siz; i++)
        reads[i] = 0;
    sum = 0;
    ptr = 0;

    while (1) {
        // Noise reduction: averaging sensor readings over 20ms
        n = 0;
        start = millis();
        reader = 0.;
        do {
            reader += analogRead(SENSOR_PIN);
            n++;
            now = millis();
        } while (now < start + 20);
        reader /= n;

        // Moving average calculation
        sum -= reads[ptr];
        sum += reader;
        reads[ptr] = reader;
        last = sum / samp_siz;

        // Detect rising curve (heartbeat)
        if (last > before) {
            rise_count++;
            if (!rising && rise_count > rise_threshold) {
                rising = true;
                first = millis() - last_beat;
                last_beat = millis();

                // Calculate BPM with weighted average
                print_value = 60000. / (0.4 * first + 0.3 * second + 0.3 * third);

                Serial.println(print_value);

                third = second;
                second = first;
            }
        } else {
            rising = false;
            rise_count = 0;
        }
        before = last;

        ptr++;
        ptr %= samp_siz;
    }
}
