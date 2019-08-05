const byte pushButton1 = 5; 
boolean pushButton1State = 0;
boolean activePushButton1State = 0;


void setup() {
  // put your setup code here, to run once:

pinMode(pushButton1, INPUT_PULLUP);

Serial.begin(9600);


Serial.println("Setup Complete");
}

void loop() {
  // put your main code here, to run repeatedly:

pushButton1State = digitalRead(5);
if (pushButton1State == LOW){
  activePushButton1State = !activePushButton1State;
}
delay(50);
Serial.println("Push Button 1 is now ");
Serial.println(activePushButton1State);
}
