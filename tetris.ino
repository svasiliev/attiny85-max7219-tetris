#include "LedControl.h"

#define XPIN A2
#define YPIN A3

#define DINPIN 0
#define CLKPIN 2
#define CSPIN 1
#define DSPLS 2

LedControl lc = LedControl(DINPIN, CLKPIN, CSPIN, DSPLS);
//LedControl lc = LedControl(12, 10, 11, 2); // pro mini settings

void setup()
{
  delay(1000);

  lc = LedControl(DINPIN, CLKPIN, CSPIN, DSPLS);

  lc.shutdown(0, false);
  lc.shutdown(1, false);
  lc.setIntensity(0, 5);
  lc.setIntensity(1, 5);
  lc.clearDisplay(0);
  lc.clearDisplay(1);

  //Serial.begin(9600);
}

byte getNewX()
{
  int X = analogRead(XPIN);
  byte newX = map(X, 70, 1000, 0, 7);
  return newX;
}

byte getNewY()
{
  int Y = analogRead(YPIN);
  byte newY = map(Y, 70, 1000, 7, 0);
  return newY;
}

#define SCREEN DSPLS * 8 + 1
int screen[SCREEN];

byte t_shape[] =
{
  B0100,
  B1110,
  B0000,
  B0000,

  B1000,
  B1100,
  B1000,
  B0000,

  B1110,
  B0100,
  B0000,
  B0000,

  B0100,
  B1100,
  B0100,
  B0000,
};

byte j_shape[] =
{
  B1110,
  B0010,
  B0000,
  B0000,

  B0100,
  B0100,
  B1100,
  B0000,

  B1000,
  B1110,
  B0000,
  B0000,

  B1100,
  B1000,
  B1000,
  B0000,
};

byte l_shape[] =
{
  B1110,
  B1000,
  B0000,
  B0000,

  B1100,
  B0100,
  B0100,
  B0000,

  B0010,
  B1110,
  B0000,
  B0000,

  B1000,
  B1000,
  B1100,
  B0000,
};

byte o_shape[] =
{
  B1100,
  B1100,
  B0000,
  B0000,

  B1100,
  B1100,
  B0000,
  B0000,

  B1100,
  B1100,
  B0000,
  B0000,

  B1100,
  B1100,
  B0000,
  B0000,
};

byte i_shape[] =
{
  B1111,
  B0000,
  B0000,
  B0000,

  B1000,
  B1000,
  B1000,
  B1000,

  B1111,
  B0000,
  B0000,
  B0000,

  B1000,
  B1000,
  B1000,
  B1000,
};

byte s_shape[] =
{
  B0110,
  B1100,
  B0000,
  B0000,

  B1000,
  B1100,
  B0100,
  B0000,

  B0110,
  B1100,
  B0000,
  B0000,

  B1000,
  B1100,
  B0100,
  B0000,
};

byte z_shape[] =
{
  B1100,
  B0110,
  B0000,
  B0000,

  B0100,
  B1100,
  B1000,
  B0000,

  B1100,
  B0110,
  B0000,
  B0000,

  B0100,
  B1100,
  B1000,
  B0000,
};

byte *shapes[16] = {
  t_shape,
  j_shape,
  l_shape,
  o_shape,
  i_shape,
  s_shape,
  z_shape
};

int i, j;
bool initGame = true;
int x = 0;
byte y = 0;
int prevX = 0;
byte prevY = 0;
byte turn = 0;
byte shapeNum = 0;
byte *prevShape;
unsigned long lastGameLoop = 0;
#define BASE_GAME_DELAY 1000
int baseGameDelay = BASE_GAME_DELAY;
int gameDelay = baseGameDelay;
int systemDelay = 100;

byte *getShape()
{
  byte *shape = shapes[shapeNum];
  return shape + turn * 4;
}

#define BASE_ALGN 2
int algn(byte b, int x)
{
  int res = b << BASE_ALGN;
  if (x < 0)
    res = res << (x * -1);
  else if (x > 0)
    res = res >> x;

  return res;
}

void changeShape()
{
  shapeNum = random(7);
}

void clearScreen()
{
  memset(screen, 0, sizeof(screen));
  screen[SCREEN - 1] = B11111111;
  initGame = false;
}

bool checkHit(byte *shape)
{
  for (i = 0; i < 4; i++)
    if ((screen[y + i] & algn(shape[i], x)) != 0)
      return true;

  return false;
}

bool checkBorder(byte *shape)
{
  bool hit = false;

  for (i = 0; i < 4; i++) {
    int test = algn(shape[i], x);
    // right overflow
    if ((x > BASE_ALGN && (test << (x - BASE_ALGN)) < shape[i])
        // left overflow
        || test > 255)
    {
      hit = true;
      break;
    }
  }

  return hit;
}

void tryDelay(byte newY)
{
  if (newY < 6)
    return;

  gameDelay = gameDelay / 2;
}

void tryTurn(byte newY)
{
  if (newY > 2)
    return;

  byte current = turn;
  turn++;
  if (turn == 4)
    turn = 0;

  byte *tryShape = getShape();

  bool hit = checkHit(tryShape);
  if (!hit)
    hit = checkBorder(tryShape);

  if (hit)
    turn = current;
}

void tryMove(byte newX)
{
  if (newX <= 2)
    x--;
  else if (newX >= 6)
    x++;

  byte *tryShape = getShape();
  bool hit = checkHit(tryShape);
  if (!hit)
    hit = checkBorder(tryShape);

  if (hit)
    x = prevX;
}

void clearPreviousShape()
{
  if (prevShape != NULL)
    for (i = 0; i < 4; i++)
      screen[prevY + i] ^= algn(prevShape[i], prevX);
}

void drawShape(byte *shape)
{
  for (i = 0; i < 4; i++)
    screen[y + i] |= algn(shape[i], x);
}

void drawPreviousShape()
{
  for (i = 0; i < 4; i++)
    screen[prevY + i] |= algn(prevShape[i], prevX);
}

void clearDone()
{
  bool some = false;
  do
  {
    some = false;
    for (i = SCREEN - 2; i >= 0; i--)
    {
      if (screen[i] == 255)
      {
        for (j = i; j >= 1; j--)
        {
          screen[j] = screen[j - 1];
        }
        screen[0] = 0;
        some = true;

        baseGameDelay -= 10;
      }
    }
  } while (some);
}

void checkGameOver(byte *shape)
{
  byte *tryShape = getShape();
  if (checkHit(tryShape))
  {
    initGame = true;
    baseGameDelay = BASE_GAME_DELAY;
  }
}

void draw()
{
  for (i = 0; i < 8; i++)
    for (j = 0; j < DSPLS; j++)
      lc.setRow(j, i, screen[i + j * 8]);
}

void splash()
{
  byte d = 20;

  for (j = 0; j < DSPLS; j++)
  {
    for (i = 1; i < 8; i++)
    {
      lc.setRow(j, i - 1, 0);
      lc.setRow(j, i, 255);
      delay(d);
    }
    lc.setRow(j, 7, 0);
  }

  for (j = DSPLS; j >= 0; j--)
  {
    for (int i = 6; i >= 0; i--)
    {
      lc.setRow(j, i + 1, 0);
      lc.setRow(j, i, 255);
      delay(d);
    }
    lc.setRow(j, 0, 0);
  }
}

void gameLoop(byte newX, byte newY, bool ioLoop)
{
  if (initGame)
  {
    splash();
    clearScreen();
  }

  clearPreviousShape();

  if (ioLoop)
  {
    tryDelay(newY);
    tryTurn(newY);
    tryMove(newX);
  }

  byte *newShape = getShape();

  bool hit = checkHit(newShape);

  if (!hit)
  {
    drawShape(newShape);
    prevX = x;
    prevY = y;
    prevShape = newShape;
    if(!ioLoop)
      y++;
  }
  else
  {
    drawPreviousShape();

    clearDone();

    gameDelay = baseGameDelay;
    x = 0;
    prevX = 0;
    y = 0;
    prevY = 0;
    prevShape = NULL;

    changeShape();

    checkGameOver(newShape);
  }

  draw();
}

void loop()
{
  byte newX = getNewX();
  byte newY = getNewY();

  unsigned long current = millis();

  if (current - lastGameLoop > gameDelay)
  {
    gameLoop(newX, newY, false);
    lastGameLoop = current;
  }

  gameLoop(newX, newY, true);

  delay(systemDelay);
}
