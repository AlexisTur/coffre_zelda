

void setup() {
  pinMode(1, OUTPUT);//vert
  pinMode(11, OUTPUT);//rouge
  pinMode(10, OUTPUT);//bleu
  digitalWrite(1,0);
  digitalWrite(11,1);
  digitalWrite(10,0);
}

void loop() {
  delay(1000);
}













/*
const int ledPin     = 8;
const int freq       = 50;
const int resolution = 14;

int pin_anal =7; //gpio7

int val_anal_max = 1850;
int val_anal_min = 0;

int val_pwm_max = 16100;//16383
int val_pwm_min = 1;

int convert(int anal_value);

void setup() {
  ledcAttach(ledPin, freq, resolution);
  //pinMode(8, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  static int value;
  value = convert(analogRead(pin_anal));
  ledcWrite(ledPin, value);  // pleine luminosité
  Serial.println(value);
  delay(100);

}

int convert(int anal_value){
  int converted_value;
  converted_value = int(anal_value*1600/val_anal_max)+14300;
  return converted_value;
}*/
