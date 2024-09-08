// Arduino code for HC-SR04 distance sensor

#define trigPin 9
#define echoPin 10

void setup() {
  Serial.begin(9600); // Start serial communication
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

void loop() {
  long duration, distance;
  digitalWrite(trigPin, LOW);  // Reset the sensor
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); // Trigger the sensor
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH); // Measure echo duration
  distance = (duration / 2) / 29.1;   // Convert echo duration to distance
  if (distance >= 2 && distance <= 400) { // If within range of 2cm - 400cm
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
  } else {
    Serial.println("Out of range"); // If sensor is out of range
  }
  delay(1000); // Measurement interval
}
