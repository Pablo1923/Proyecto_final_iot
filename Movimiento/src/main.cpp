#include <Arduino.h>
#define SendKey 0  //Button to send data Flash BTN on NodeMCU

#define BAUD_SERIAL 115200
#define RXBUFFERSIZE 1024

#define STACK_PROTECTOR  512 // bytes

int regPins[3] = {D6, D7, D8};
//74HC595 pins latchPin = D8; clockPin = D7; dataPin = D6;

int motor[4] = {D1, D2, D3, D4};
//L293D Motor control speed pins

//Startup motor speed
int startSpeed = 160;

int speed = 160;

int maxSpeed = 100;

int lastSpeed = 0;

int contPins[2] = {D0, D5};
//other control pins light etc.

byte dataArray[4] = {39, 216, 149, 106};
//Motor control serial data 39 Forward ---- 216 Backward ---- 149 Left ---- 106 Right

String moveData = "";
String lastMoveData = "";
String Data = "";

void moveDataProcess();
void controlOut(String _data);
void movement(int b);
void setSpeed();
void setSpeed(int sp);
void forward();
void backward();
void left();
void right();
void mstop();

String estado;

void setup() {

  Serial.begin(BAUD_SERIAL);
  Serial.setRxBufferSize(RXBUFFERSIZE);

  pinMode(regPins[0], OUTPUT);
  pinMode(regPins[1], OUTPUT);
  pinMode(regPins[2], OUTPUT);

  pinMode(contPins[0], OUTPUT);
  pinMode(contPins[1], OUTPUT);

  pinMode(motor[0], OUTPUT);
  pinMode(motor[1], OUTPUT);
  pinMode(motor[2], OUTPUT);
  pinMode(motor[3], OUTPUT);

  estado = "aceleraAdelante50";
  moveData = "S";
  speed = 250;
  
}

void loop() {


  if(estado == "aceleraAdelante50"){
    Serial.print("\nIngresó al estado 'aceleraAdelante50' y desacelerará hacia adelante a 50");
    setSpeed(100);
    moveData = "R";
    moveDataProcess();
    delay(2000);
    estado = "aceleraAdelante100";
  }

  if(estado == "aceleraAdelante100"){
    Serial.print("\nIngresó al estado 'aceleraAdelante100' y acelerará hacia adelante a 100");
    setSpeed(200);
    moveData = "R";
    moveDataProcess();
    delay(10000);
    estado = "desaceleraAdeleraAdelante50";
  }

  if(estado == "desaceleraAdeleraAdelante50"){
    Serial.print("\nIngresó al estado 'desaceleraAdeleraAdelante50' y desacelerará hacia adelante a 50");
    setSpeed(100);
    moveData = "R";
    moveDataProcess();
    delay(5000);
    estado = "quieto";
  }

  if(estado == "quieto"){
    Serial.print("\nIngresó al estado 'quieto' y se detendrá");
    moveData = "S";
    setSpeed(0);
    moveDataProcess();
    delay(2000);
    estado = "aceleraAtras50";
  }

  if(estado == "aceleraAtras50"){
    Serial.print("\nIngresó al estado 'aceleraAtras50' y acelerará hacia atras a 50");
    setSpeed(150); 
    moveData = "L";
    moveDataProcess();
    delay(1000);
    estado = "aceleraAtras100";
  }

  if(estado == "aceleraAtras100"){
    Serial.print("\nIngresó al estado 'aceleraAdelante100' y acelerará hacia adelante a 100");
    setSpeed(200);
    moveData = "L";
    moveDataProcess();
    delay(10000);
    estado = "desaceleraAtras50";
  }

  if(estado == "desaceleraAtras50"){
    Serial.print("\nIngresó al estado 'desaceleraAtras50' y desacelerará hacia atras a 50");
    setSpeed(100);
    moveData = "L";
    moveDataProcess();
    delay(5000);
    estado = "quieto";
  }

  if(estado == "quieto"){
    Serial.print("\nIngresó al estado 'quieto' y se detendrá");
    moveData = "S";
    setSpeed(0);
    moveDataProcess();
    delay(2000);
    estado = "aceleraAdelante50";
  }

}

void moveDataProcess()
{
  if (moveData == "F")
    forward();
  else if (moveData == "B")
    backward();
  else if (moveData == "L")
    left();
  else if (moveData == "R")
    right();
  else if (moveData == "S")
    mstop();
  else if (moveData == "Q")
    controlOut(moveData);
  else if (moveData == "W")
    controlOut(moveData);
  else if (moveData == "q")
    controlOut(moveData);
  else if (moveData == "w")
    controlOut(moveData);
}

void controlOut(String _data)
{
  if (_data == "Q")
    digitalWrite(contPins[0], HIGH);
  else if (_data == "q")
    digitalWrite(contPins[0], LOW);
  else if (_data == "W")
    digitalWrite(contPins[1], HIGH);
  else if (_data == "w")
    digitalWrite(contPins[1], LOW);
} 

void movement(int b)
{
  digitalWrite(regPins[2], HIGH);
  shiftOut(regPins[0], regPins[1], MSBFIRST, b);
  digitalWrite(regPins[2], LOW);
}

void setSpeed()
{
  if (maxSpeed < speed)
  {
    if (moveData != "S")
    {
      maxSpeed++;
      Serial.println(maxSpeed);
      analogWrite(motor[0], maxSpeed);
      analogWrite(motor[1], maxSpeed);
      analogWrite(motor[2], maxSpeed);
      analogWrite(motor[3], maxSpeed);
    }
  }
}

void setSpeed(int sp)
{
  maxSpeed = 100;
  analogWrite(motor[0], sp);
  analogWrite(motor[1], sp);
  analogWrite(motor[2], sp);
  analogWrite(motor[3], sp);
}

void forward()
{
  movement(dataArray[0]);  // Corregido para ir hacia adelante
}

void backward()
{
  movement(dataArray[1]);  // Corregido para ir hacia atrás
}

void left()
{
  movement(dataArray[2]);  // Corregido para girar a la izquierda
}

void right()
{
  movement(dataArray[3]);  // Corregido para girar a la derecha
}

void mstop()
{
  movement(0);  // Detener el carro
  setSpeed(0);
}

