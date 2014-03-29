
#include <Encabulator.h>
#include <Wire.h>
#include <avr/pgmspace.h>


#define SOP '<'
#define EOP '>'
#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)

#define MIN(a, b) ((a < b) ? a : b)
#define MAX(a, b) ((a > b) ? a : b)
#define SLOSHVALUE 23

bool Started = false;
bool Ended = false;

int Alpha = 0;
int Red   = 0;
int Green = 0;
int Blue  = 0;
int SloshFrequency   = 0;
int SloshTimeStep    = 300;
bool SloshVertical   = true;
bool SloshHorizontal = true;
bool Slosh;
int LastSloshTime = 0;

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

  int Time = millis();
  if (Slosh && abs(LastSloshTime - Time) > SloshTimeStep)
  {
    LastSloshTime = Time;
    NextSlosh();
  }
}

void SendCommand(
  char* Command,
  char* Data1,
  char* Data2,
  char* Data3,
  char* Data4)
{
  if (strcmp(Command,"fColor") == 0)
  {
    FadeToColor(Data1, Data2, Data3, Data4);
  }
  else if (strcmp(Command, "jColor") == 0)
  {
    JumpToColor(Data1, Data2, Data3, Data4);
  }
  else if (strcmp(Command, "Slosh") == 0)
  {
    StartSlosh(Data1,Data2,Data3);
  }
}

//*****************************************************************************
//*****************************************************************************
void FadeToColor(
  char* Data1,
  char* Data2,
  char* Data3,
  char* Data4)
{
  Alpha = 255 - atoi(Data1);
  Red   = atoi(Data2) - Alpha;
  Green = atoi(Data3)- Alpha;
  Blue  = atoi(Data4)- Alpha;
  for (int i = 1; i < 5; i++)
  {
    Encabulator.stripBankA.fadeHeaderToRGB(i, Red, Green, Blue, 20);
    Encabulator.stripBankB.fadeHeaderToRGB(i, Red, Green, Blue, 20);
  }
}

//*****************************************************************************
//*****************************************************************************
void JumpToColor(
  char* Data1,
  char* Data2,
  char* Data3,
  char* Data4)
{
  Alpha = 255 - atoi(Data1);
  Red   = atoi(Data2) - Alpha;
  Red   = MAX(0, Red);
  Green = atoi(Data3)- Alpha;
  Green   = MAX(0, Green);
  Blue  = atoi(Data4)- Alpha;
  Blue   = MAX(0, Blue);
  for (int i = 1; i < 5; i++)
  {
    Encabulator.stripBankA.fadeHeaderToRGB(i, Red, Green, Blue,4);
    Encabulator.stripBankB.fadeHeaderToRGB(i, Red, Green, Blue,4);
  }
}

//*****************************************************************************
//*****************************************************************************
void StartSlosh(
  char* Data1,
  char* Data2,
  char* Data3)
{
  SloshVertical   = strcasecmp(Data1, "True");
  SloshHorizontal = strcasecmp(Data1, "True");
  SloshFrequency  = atoi(Data3);
  Slosh = true;
  for (int i = 1; i < 5; i++)
  {
    Encabulator.stripBankA.fadeHeaderToRGB(i, 0, 0, 0, 10);
    Encabulator.stripBankB.fadeHeaderToRGB(i, 0, 0, 0, 10);
  }
  Red = 255; Blue = 0; Green = 0;
}



//*****************************************************************************
//*****************************************************************************
void GetNextColor(int& tRed, int& tGreen, int& tBlue)
{
    if ((tBlue == 255) && (tGreen == 0))
    {
      tRed -= SLOSHVALUE;
      tRed = MAX(tRed, 0);
    }
    if ((tBlue == 0) && tGreen == 255)
    {
      tRed += SLOSHVALUE;
      tRed = MIN(tRed, 255);
    }
    if ((tGreen == 255) && (tRed == 0))
    {
      tBlue -= SLOSHVALUE;
      tBlue = MAX(tBlue, 0);
    }
    if ((tRed == 255) && (tGreen ==0))
    {
      tBlue += SLOSHVALUE;
      tBlue = MIN(tBlue, 255);
    }
    if ((tRed == 255) && (tBlue ==0))
    {
     tGreen -= SLOSHVALUE;
     tGreen = MAX(tGreen, 0);
    }
    if ((tBlue == 255) && (tRed == 0))
    {
     tGreen += SLOSHVALUE;
     tGreen = MIN(tGreen, 255);
    }
}

//*****************************************************************************
//*****************************************************************************
void NextSlosh()
{
  GetNextColor(Red, Green, Blue);
  Encabulator.stripBankA.fadeHeaderToRGB(0, Red, Green, Blue, 20);
}

