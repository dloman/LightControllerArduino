
#include <Encabulator.h>
#include <Wire.h>
#include <avr/pgmspace.h>

#define SOP '<'
#define EOP '>'

bool Started = false;
bool Ended = false;

int Alpha = 0;
int Red   = 0;
int Green = 0;
int Blue  = 0;

char inData[80];
byte index;
void setup() {
  Serial.begin(9600);
  Encabulator.upUpDownDownLeftRightLeftRightBA();
  // jump all 12V RGB headers up to white

  for (int i = 1 ; i < 5 ; i++)
  {
    Encabulator.stripBankA.jumpHeaderToRGB(i,255,255,255);
    Encabulator.stripBankB.jumpHeaderToRGB(5-i,255,255,255);
    delay(250);
  }

  delay(500);

  // quickly fade all 12V RGB headers down to black

  for (int i = 1 ; i < 5 ; i++)
  {
    Encabulator.stripBankA.fadeHeaderToRGB(i,0,0,0,20);
    Encabulator.stripBankB.fadeHeaderToRGB(5-i,0,0,0,20);
    delay(250);
  }

  delay(500);
}

void loop()
{
  // Read all serial data available, as fast as possible
  while (Serial.available() > 0)
  {
    char inChar = Serial.read();

    if (inChar == SOP)
    {
       index = 0;
       inData[index] = '\0';
       Started = true;
       Ended = false;
    }
    else if (inChar == EOP)
    {
       Ended = true;
       break;
    }
    else
    {
      if (index < 79)
      {
        inData[index] = inChar;
        index++;
        inData[index] = '\0';
      }
    }
  }

  // We are here either because all pending serial
  // data has been read OR because an end of
  // packet marker arrived. Which is it?
  if (Started && Ended)
  {
    // The end of packet marker arrived. Process the packet
    char* Command = strtok(inData, ",");

    if (Command)
    {
       char* Data1 = strtok(NULL, ",");
       char* Data2 = strtok(NULL, ",");
       char* Data3 = strtok(NULL, ",");
       char* Data4 = strtok(NULL, ",");
       if (Data1 && Data2 && Data3 && Data4)
       {
          SendCommand(
            Command,
            Data1,
            Data2,
            Data3,
            Data4);
       }
    }
    // Reset for the next packet
    Started = false;
    Ended = false;
    index = 0;
    inData[index] = '\0';
  }
}

void SendCommand(
  char* Command,
  char* Data1,
  char* Data2,
  char* Data3,
  char* Data4)
{
  if (strcmp(Command,"Color") == 0)
  {
    SetColor(Data1, Data2, Data3, Data4);
  }
}

void SetColor(
  char* Data1,
  char* Data2,
  char* Data3,
  char* Data4)
{
  Alpha = atoi(Data1);
  Red   = atoi(Data1);
  Green = atoi(Data1);
  Blue  = atoi(Data1);
  for (int i = 1; i < 5; i++)
  {
    Encabulator.stripBankA.fadeHeaderToRGB(
      i,
      Red,
      Green,
      Blue,
      20);
  }
}
