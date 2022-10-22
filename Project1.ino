-#include <Adafruit_MotorShield.h>
#include <SoftwareSerial.h>
#include <TFMPlus.h>
#include <Servo.h>
#include <Wire.h>
TFMPlus tfmP;
Servo myservo;
SoftwareSerial TFplus(1,0);
//Motor
#define leftEncoder_A   A0
#define leftEncoder_B   A1
#define rightEncoder_A   A2
#define rightEncoder_B   A3
#define GEARING     20
#define ENCODERMULT 12
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
// And connect a DC motor to port M1&M2
Adafruit_DCMotor *leftMotor = AFMS.getMotor(1);
Adafruit_DCMotor *rightMotor = AFMS.getMotor(2);
volatile float RPM = 0;
volatile uint32_t lastA = 0;
volatile bool motordir = FORWARD;

//Ultrasonic sensor
const int trigPinFront = 3;
const int echoPinFront = 2;
const int trigPinLeft = 5;
const int echoPinLeft = 4;
const int trigPinRight = 7;
const int echoPinRight = 6;


//IR&PIR
int leftIn = 8;
int leftPIn = 9;
int rightIn = 11;
int rightPIn = 10;


//Variables for Ultrasonic Sensors
long durationFront;
int distanceFront;
long durationLeft;
int distanceLeft;
long durationRight;
int distanceRight;
const int minFrontDistance = 40;
const int minSideDistance = 25;
const int stuckDistance = 15;

//Pathfinder
char path[20]; 
int i;
int j;
char retrace[20];

//TF
int16_t tfDist = 0;
int counter = 0;
char dist[11];
int val = 0;


void setup() {
//Direction  
  Serial.begin(9600);
  tfmP.begin(&Serial);
  while (!Serial);
  //Motor
  pinMode(leftEncoder_A, INPUT_PULLUP);
  pinMode(leftEncoder_B, INPUT_PULLUP);
  pinMode(rightEncoder_A, INPUT_PULLUP);
  pinMode(rightEncoder_B, INPUT_PULLUP);
  attachInterrupt(leftEncoder_A, interruptA, RISING);
  attachInterrupt(rightEncoder_A, interruptA, RISING);
  leftMotor->setSpeed(0);
  rightMotor->setSpeed(0);
  //Ultra
  pinMode(trigPinFront, OUTPUT);
  pinMode(echoPinFront, INPUT);
  pinMode(trigPinLeft, OUTPUT);
  pinMode(echoPinLeft, INPUT);
  pinMode(trigPinRight, OUTPUT);
  pinMode(echoPinRight, INPUT);
  //IR
  pinMode(leftIn, INPUT);
  pinMode(rightIn, INPUT);
  //PIR
  pinMode(leftPIn, INPUT);
  pinMode (rightPIn, INPUT);
  //Servo
  myservo.attach(12);
} 

//Sensor pathray |Corresponding action
//  1 0 0 1     following the line - go straight
//  1 0 1 1     only left_center sensor on the line - turn left
//  1 1 0 1     only right_center sensor on the line - turn right
//  1 1 0 0     turn right
//  0 0 1 1     turn left 
//  1 0 0 0     turn right (intersection) - should run an extra inch to understand what kind of intersection, if possible - go straight
//  0 0 0 1     turn left (intersection)
//
//  1 1 1 1     no line found - turn 180 degrees
//  0 0 0 0     continuous line - run an extra inch to understand if it is an intersection or end  and go to the left if an intersection
void loop() {
   myservo.write(87);
   //Ultra();
   TFdata();
   //turn_left();
//  if(!analogRead(left)==0 && digitalRead(left_center)==0 && digitalRead(right_center)==0 && digitalRead(right)==0)
//     { turn_left();
//     }
//  else if(!analogRead(left)==0 && !analogRead(left_center)==0 && digitalRead(right_center)==0 && digitalRead(right)==0)
//     { turn_left();
//     }
//  else if(!analogRead(left)==0 && !analogRead(left_center)==0 && digitalRead(right_center)==0 && !analogRead(right)==0)
//     { turn_left();
//     }
//
//  else if(!analogRead(right)==0 && digitalRead(right_center)==0 && digitalRead(left_center)==0 && digitalRead(left)==0)
//     { runExtraInch();
//       if (!analogRead(right)==0 && !analogRead(right_center)==0 && !analogRead(left_center)==0 && !analogRead(left)==0){
//       turn_right();
//       }
//       else{
//       straight();
//       }
//     }
//  else if(!analogRead(right)==0 && !analogRead(right_center)==0 && digitalRead(left_center)==0 && digitalRead(left)==0)
//     { turn_right();
//     }
//  else if(!analogRead(right)==0 && !analogRead(right_center)==0 && digitalRead(left_center)==0 && !analogRead(left)==0)
//     { turn_right();
//     }
//
//  else if(!analogRead(right)==0 && digitalRead(right_center)==0 && digitalRead(left_center)==0 && !analogRead(left)==0)
//     { straight();
//     }
//   else if(!analogRead(right)==0 && !analogRead(right_center)==0 && !analogRead(left_center)==0 && !analogRead(left)==0)
//     { back();
//     }  
//  else if(digitalRead(right)==0 && digitalRead(right_center)==0 && digitalRead(left_center)==0 && digitalRead(left)==0)
//  {   runExtraInch();
//       if (digitalRead(right)==0 && digitalRead(right_center)==0 && digitalRead(left_center)==0 && digitalRead(left)==0){
//       delay(5000);
//       replay(); 
//       }
//       else{
//       turn_left();
//       }
//  }
}
void interruptA() {
  motordir = digitalRead(leftEncoder_B);
  motordir = digitalRead(rightEncoder_B);
  
  uint32_t currA = micros();
  if (lastA < currA) {
    // did not wrap around
    float rev = currA - lastA;  // us
    rev = 1.0 / rev;            // rev per us
    rev *= 1000000;             // rev per sec
    rev *= 60;                  // rev per min
    rev /= GEARING;             // account for gear ratio
    rev /= ENCODERMULT;         // account for multiple ticks per rotation
    RPM = rev;
  }
  lastA = currA;
}

void Ultra () {
  //Read front sensor value
  digitalWrite(trigPinFront, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPinFront, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPinFront, LOW);
  durationFront = pulseIn(echoPinFront, HIGH);
  distanceFront = durationFront * 0.034 / 2;
  Serial.print("Front:");
  Serial.print(distanceFront);
  //Read left sensor value
  digitalWrite(trigPinLeft, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPinLeft, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPinLeft, LOW);
  durationLeft = pulseIn(echoPinLeft, HIGH);
  distanceLeft = durationLeft * 0.034 / 2;
  Serial.print("  Left:");
  Serial.print(distanceLeft);
  //Read right sensor value
  digitalWrite(trigPinRight, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPinRight, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPinRight, LOW);
  durationRight = pulseIn(echoPinRight, HIGH);
  distanceRight = durationRight * 0.034 / 2;
  Serial.print("  Right:");
  Serial.print(distanceRight);
  delay(300);
}


void TFdata () {
  counter++;
  Serial.print(" Distance");
  Serial.println(tfDist);
  if ( tfmP.getData(tfDist)) // Get data from the device.
  {
    if (counter >= 10) 
      counter = 0;
    }
  }

void turn_left()  
{   
  leftMotor->setSpeed(180); //Define maximum velocity
  leftMotor->run(BACKWARD); //rotate the motor anti-clockwise
  rightMotor->setSpeed(255); //Define maximum velocity
  rightMotor->run(FORWARD);  //rotate the motor clockwise
    
   path[i]='L';
   i++;
   if(path[i-2]=='B')
     { shortcut();
     }
}

void turn_right()                                                 
{ 
  leftMotor->setSpeed(255); //Define maximum velocity
  leftMotor->run(FORWARD); //rotate the motor clockwise
  rightMotor->setSpeed(180); //Define maximum velocity
  rightMotor->run(BACKWARD); //rotate the motor anti-clockwise
      
  path[i]='R';
  if(path[i-2]=='B')
     { shortcut();
     }
} 

void straight()                                             
{ 
  leftMotor->setSpeed(255); //Define maximum velocity
  leftMotor->run(FORWARD); //rotate the motor clockwise
  rightMotor->setSpeed(255); //Define maximum velocity
  rightMotor->run(FORWARD);  //rotate the motor clockwise
    
    path[i]='S';
    i++;
    if(path[i-2]=='B')  
      { shortcut();
      }
 }

 void back()                                                        
{   
  leftMotor->setSpeed(255); //Define maximum velocity
  leftMotor->run(BACKWARD); //rotate the motor anti-clockwise
  rightMotor->setSpeed(255); //Define maximum velocity
  rightMotor->run(BACKWARD);  //rotate the motor anti-clockwise
       
   path[i]=='B';
    i++;
}

void Uturn(){
  leftMotor->setSpeed(255); //Define maximum velocity
  leftMotor->run(FORWARD); //rotate the motor anti-clockwise
  rightMotor->setSpeed(255); //Define maximum velocity
  rightMotor->run(BACKWARD);  //rotate the motor anti-clockwise
}
//function that implements the shortcut and removes the unnecessary moves 
void shortcut()                                                  
{  int done=0;
  if(path[i-3]=='L' && path[i-1]=='R')
     { i=i-3;
        path[i]='B';
        done=1;
     }
   if(path[i-3]=='L' && path[i-1]=='S' && done==0)
     { i=i-3;
        path[i]='R';
        done=1;
     }
    if(path[i-3]=='L' && path[i-1]=='L' && done==0)
     { i=i-3;
        path[i]='S';
        done=1;
     }
   if(path[i-3]=='S' && path[i-1]=='L' && done==0)
     { i=i-3;
        path[i]='R';
        done=1;
     }
   if(path[i-3]=='S' && path[i-1]=='S' && done==0)
     { i=i-3;
        path[i]='B';
        done=1;
     }        
   if(path[i-3]=='R'&&path[i-1]=='L'&&done==0)
     { i=i-3;
        path[i]='B';
        done=1;
     }
}

void replay()                                                
{
   for(j=0,i=0;j<=i,path[i]!='\0';i++,j++)
    { retrace[j]=path[i];
    }
    retrace[j]='\0';
   for(j=0;retrace[j]!='\0';j++)
      {  if(retrace[j]=='L')
         { turn_left();}
        else if(retrace[j]=='R')
          { turn_right();}
        else if(retrace[j]=='S')
          { straight();}
        else if(retrace[j]=='B')
           { back();}
     }
     
  if(((distanceFront<=stuckDistance) || (tfDist<=stuckDistance))  && (distanceLeft<=stuckDistance) && (distanceRight<=stuckDistance))
  stop();
}

//function to stop
void stop()                                                         
{     
  leftMotor->setSpeed(0); //Define minimum velocity
  leftMotor->run(RELEASE); //rotate the motor clockwise
  rightMotor->setSpeed(0); //Define minimum velocity
  rightMotor->run(RELEASE); //stop the motor when release the button
} 
