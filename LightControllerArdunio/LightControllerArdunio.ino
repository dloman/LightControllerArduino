#include <Encabulator.h>
#include <Wire.h>
#include <avr/pgmspace.h>

#define SOP '<'
#define EOP '>'
#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)

#define SLOSHVALUE 23

bool Started = false;
bool Ended = false;

int mAlpha[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int mRed[8]   = {0, 0, 0, 0, 0, 0, 0, 0};
int mGreen[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int mBlue[8]  = {0, 0, 0, 0, 0, 0, 0, 0};

enum LarsonColor {
  eRed,
  eGreen,
  eBlue,
};

enum Mode {
  eAllFade,
  eRollingColor,
  eLarsonScan,
  eColorWheel,
  eColorRing
};
//Larson Scan Vars
LarsonColor mLarsonColor = eRed;
bool mLarsonDirection = true;
int mLarsonPosition = 0;

//Rolling Color Vars
Mode mMode = eRollingColor;
int SloshFrequency   = 0;
int SloshTimeStep    = 300;
bool SloshVertical   = true;
bool SloshHorizontal = true;
bool Slosh;
int LastSloshTime = 0;

char inData[80];
byte index;

//*****************************************************************************
//*****************************************************************************
void setup()
{
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

//*****************************************************************************
//*****************************************************************************
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

//*****************************************************************************
//*****************************************************************************
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
  else if (strcmp(Command, "Mode") == 0)
  {
    StartMode(Data1,Data2);
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
  int Alpha = 255 - atoi(Data1);
  int Red   = atoi(Data2) - Alpha;
  Red   = MAX(0, Red);
  int Green = atoi(Data3)- Alpha;
  Green   = MAX(0, Green);
  int Blue  = atoi(Data4)- Alpha;
  Blue   = MAX(0, Blue);
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
  int Alpha = 255 - atoi(Data1);
  int Red   = atoi(Data2) - Alpha;
  Red   = MAX(0, Red);
  int Green = atoi(Data3)- Alpha;
  Green   = MAX(0, Green);
  int Blue  = atoi(Data4)- Alpha;
  Blue   = MAX(0, Blue);
  for (int i = 1; i < 5; i++)
  {
    Encabulator.stripBankA.fadeHeaderToRGB(i, Red, Green, Blue,4);
    Encabulator.stripBankB.fadeHeaderToRGB(i, Red, Green, Blue,4);
  }
}

//*****************************************************************************
//*****************************************************************************
void WriteColor()
{
  for (int i = 0; i < 4; i++)
  {
    Encabulator.stripBankA.fadeHeaderToRGB(i+1, mRed[i], mGreen[i], mBlue[i],4);
    Encabulator.stripBankB.fadeHeaderToRGB(
      i+1,
      mRed[i+4],
      mGreen[i+4],
      mBlue[i+4],4);
  }
}

//*****************************************************************************
//*****************************************************************************
void StartRoll()
{
  mRed[0] = 255; mBlue[0] = 0; mGreen[0] = 0;
  for (int i = 1; i < 8; i++)
  {
    mRed[i] = 0; mBlue[i] = 0; mGreen[i] = 0;
  }
}

//*****************************************************************************
//*****************************************************************************
void StartFade()
{
  StartRoll();
  for (int i = 1; i < 8; i++)
  {
    mRed[i] = mRed[0]; mGreen[i] = mGreen[0]; mBlue[i] = mBlue[0];
  }
}

//*****************************************************************************
//*****************************************************************************
void StartLarsonScan()
{
  LarsonColor mLarsonColor = eRed;
  bool mLarsonDirection = true;
  int mLarsonPosition = 0;
}

//*****************************************************************************
//*****************************************************************************
void StartColorWheel()
{
  mRed[0] = 255; mGreen[0] =   0; mBlue[0] =   0;
  mRed[1] =   0; mGreen[1] = 255; mBlue[1] = 255;
  mRed[2] = 255; mGreen[2] =   0; mBlue[2] = 255;
  mRed[3] =   0; mGreen[3] = 255; mBlue[3] =   0;
  mRed[4] = 128; mGreen[4] = 128; mBlue[4] = 128;
  mRed[5] =   0; mGreen[5] =   0; mBlue[5] = 255;
  mRed[6] = 255; mGreen[6] = 255; mBlue[6] =   0;
  mRed[7] = 128; mGreen[7] = 255; mBlue[7] = 128;
}

//*****************************************************************************
//*****************************************************************************
void StartColorRing()
{
  mRed[0] = 255; mGreen[0] =   0; mBlue[0] =   0;
  mRed[1] =   0; mGreen[1] =   0; mBlue[1] =   0;
  mRed[2] = 255; mGreen[2] =   0; mBlue[2] = 255;
  mRed[3] =   0; mGreen[3] =   0; mBlue[3] =   0;
  mRed[4] =   0; mGreen[4] =   0; mBlue[4] = 255;
  mRed[5] =   0; mGreen[5] =   0; mBlue[5] =   0;
  mRed[6] =   0; mGreen[6] = 255; mBlue[6] =   0;
  mRed[7] =   0; mGreen[7] =   0; mBlue[7] =   0;
}

//*****************************************************************************
//*****************************************************************************
void GetNextColor(int& tRed, int& tGreen, int& tBlue, int SloshValue)
{
    if ((tBlue == 255) && (tGreen == 0))
    {
      tRed -= SloshValue;
      tRed = MAX(tRed, 0);
    }
    if ((tBlue == 0) && tGreen == 255)
    {
      tRed += SloshValue;
      tRed = MIN(tRed, 255);
    }
    if ((tGreen == 255) && (tRed == 0))
    {
      tBlue -= SloshValue;
      tBlue = MAX(tBlue, 0);
    }
    if ((tRed == 255) && (tGreen ==0))
    {
      tBlue += SloshValue;
      tBlue = MIN(tBlue, 255);
    }
    if ((tRed == 255) && (tBlue ==0))
    {
     tGreen -= SloshValue;
     tGreen = MAX(tGreen, 0);
    }
    if ((tBlue == 255) && (tRed == 0))
    {
     tGreen += SloshValue;
     tGreen = MIN(tGreen, 255);
    }
}

//*****************************************************************************
//*****************************************************************************
void MoveLeft(int OldRed, int OldGreen, int OldBlue)
{
  int Temp;
  for (int i = 0; i < 7; i++)
  {
    Temp = mRed[i+1];
    mRed[i+1] = OldRed;
    OldRed = Temp;
    Temp = mBlue[i+1];
    mBlue[i+1] = OldBlue;
    OldBlue = Temp;
    Temp = mGreen[i+1];
    mGreen[i+1] = OldGreen;
    OldGreen = Temp;
  }
}

//*****************************************************************************
//*****************************************************************************
void AllFade()
{
  GetNextColor(mRed[0], mGreen[0], mBlue[0],SLOSHVALUE);
  for (int i = 1; i < 8; i++)
  {
    mRed[i] = mRed[0]; mGreen[i] = mGreen[0]; mBlue[i]= mBlue[0];
  }
}

//*****************************************************************************
//*****************************************************************************
void RollingColor()
{
  int OldRed = mRed[0];
  int OldGreen = mGreen[0];
  int OldBlue = mBlue[0];
  GetNextColor(mRed[0], mGreen[0], mBlue[0],SLOSHVALUE);
  MoveLeft(OldRed, OldGreen, OldBlue);
}

//*****************************************************************************
//*****************************************************************************
void ColorWheel()
{
  int OldRed = mRed[0];
  int OldGreen = mGreen[0];
  int OldBlue = mBlue[0];
  mRed[0] = mRed[7]; mGreen[0] = mGreen[7]; mBlue[0]= mBlue[7];
  MoveLeft(OldRed, OldGreen, OldBlue);
}

//*****************************************************************************
//*****************************************************************************
void NextLarsonPosition()
{
  if (mLarsonPosition < 1)
  {
    if (!mLarsonDirection)
    {
      mLarsonDirection = true;
    }
    else
    {
      mLarsonPosition++;
    }
  }
  else if (mLarsonPosition > 6)
  {
    if (mLarsonDirection)
    {
      mLarsonDirection = false;
    }
    else
    {
      mLarsonPosition--;
    }
  }
  else
  {
    if (mLarsonDirection)
    {

      mLarsonPosition++;
    }
    else
    {
      mLarsonPosition--;
    }
  }
}

//*****************************************************************************
//*****************************************************************************
void NextLarsonColor()
{
  if ((mLarsonPosition == 0) && (!mLarsonDirection))
  {
    switch (mLarsonColor)
    {
      case eRed:
        mLarsonColor = eGreen;
        break;
      case eGreen:
        mLarsonColor = eBlue;
        break;
      case eBlue:
        mLarsonColor = eRed;
        break;
    }
  }
}

  //*****************************************************************************
//*****************************************************************************
void LarsonDraw(int* Color)
{
  for (int i = 0; i < 8; i++)
  {
    mRed[i] = 0; mGreen[i] = 0; mBlue[i] = 0;
  }
  Color[mLarsonPosition] = 255;
  if (mLarsonPosition + 1 < 8)
  {
    Color[mLarsonPosition + 1] = 128;
  }
  if (mLarsonPosition - 1 >= 0)
  {
    Color[mLarsonPosition - 1] = 128;
  }

}

//*****************************************************************************
//*****************************************************************************
void LarsonScan()
{
  switch (mLarsonColor)
  {
    case eRed:
      LarsonDraw(mRed);
      break;
    case eGreen:
      LarsonDraw(mGreen);
      break;
    case eBlue:
      LarsonDraw(mBlue);
      break;
  }
  NextLarsonColor();
  NextLarsonPosition();
}

//*****************************************************************************
//*****************************************************************************
void NextSlosh()
{
  switch (mMode)
  {
    case eAllFade:
      AllFade();
      break;
    case eRollingColor:
      RollingColor();
      break;
    case eLarsonScan:
      LarsonScan();
      break;
    case eColorWheel:
      ColorWheel();
      break;
    case eColorRing:
      ColorWheel();
      break;
  }
}

//*****************************************************************************
//*****************************************************************************
void StartMode(
  char* Data1,
  char* Data2)
{
  //TODO Change this to read input from packet
  SloshFrequency  = atoi(Data2);
  if (strcmp(Data1,"AllFade") == 0)
  {
    StartFade();
    mMode = eAllFade;
  }
  else if (strcmp(Data1,"RollingColor") == 0)
  {
    StartRoll();
    mMode = eRollingColor;
  }
  else if (strcmp(Data1,"LarsonScan") == 0)
  {
      StartLarsonScan();
      mMode = eLarsonScan;
  }
  else if (strcmp(Data1,"ColorWheel") == 0)
  {
      StartColorWheel();
      mMode = eColorWheel;
  }
  else if (strcmp(Data1,"ColorRing") == 0)
  {
      StartColorRing();
      mMode = eColorRing;
  }
}

