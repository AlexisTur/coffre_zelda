const int ledPin     = 13;
const int freq       = 50;
const int resolution = 14;

int pin_anal =7; //gpio7

int val_anal_max = 1850;
int val_anal_min = 0;

int val_pwm_max = 16100;//16383
int val_pwm_min = 1;

int convert(int anal_value);

void setup() {
  pinMode(12,OUTPUT);
  ledcAttach(ledPin, freq, resolution);
  Serial.begin(9600);
  digitalWrite(12,1);
}

void loop() {
  static int value;
  value = convert(analogRead(pin_anal));
  value = 15000;
  ledcWrite(ledPin, value);
  Serial.println(value);
  delay(100);

}

int convert(int anal_value){
  int converted_value;
  converted_value = int(anal_value*1600/val_anal_max)+14300;
  return converted_value;
}
