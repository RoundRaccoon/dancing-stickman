#include <LCDWIKI_GUI.h>
#include <LCDWIKI_KBV.h>

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFF00
#define WHITE   0xFFFF

LCDWIKI_KBV lcd(ILI9486,A3,A2,A1,A0,A4);

constexpr int16_t width = 320;
constexpr int16_t height = 480;

typedef struct PVector {
  int16_t x, y;
};

#define SKEL_HEAD           0
#define SKEL_NECK           1
#define SKEL_LEFT_SHOULDER  2
#define SKEL_LEFT_ELBOW     3
#define SKEL_LEFT_HAND      4
#define SKEL_RIGHT_SHOULDER 5
#define SKEL_RIGHT_ELBOW    6
#define SKEL_RIGHT_HAND     7
#define SKEL_TORSO          8
#define SKEL_LEFT_HIP       9
#define SKEL_LEFT_KNEE     10
#define SKEL_LEFT_FOOT     11
#define SKEL_RIGHT_HIP     12
#define SKEL_RIGHT_KNEE    13
#define SKEL_RIGHT_FOOT    14

constexpr int NUM_SKEL_POINTS = 15;
PVector skeletonPoints[NUM_SKEL_POINTS];

constexpr int BYTES_PER_FRAME = 1 + 2 * NUM_SKEL_POINTS;

constexpr byte BYTE_START = 'S';
constexpr byte BYTE_NOT_DETECTED = 0;

constexpr byte BYTE_RECOGNISED = 'R';
constexpr byte BYTE_START_PLAYING = 'G';
constexpr byte BYTE_LOST = 'L';

int status;
constexpr int STATUS_NO_PLAYER = 0;
constexpr int STATUS_PLAYER_RECOGNISED = 1;
constexpr int STATUS_PLAYING = 2;

void setup() 
{
  Serial.begin(9600);

  lcd.Init_LCD();
  lcd.Fill_Screen(BLACK);
  lcd.Set_Draw_color(GREEN);

  status = STATUS_NO_PLAYER;
  lcd.Fill_Circle(width / 2, height / 2, height / 4);
}

void loop() 
{
  if (status == STATUS_NO_PLAYER) {
    executeStatusNoPlayer();
    return;
  }

  if (status == STATUS_PLAYER_RECOGNISED) {
    executeStatusPlayerRecognised();
    return;
  }

  if (Serial.available() < BYTES_PER_FRAME) {
    return;
  }

  byte startVal = Serial.read();
  if (startVal == BYTE_LOST) {
    /* Check if the following 31 bytes are all BYTE_LOST */
    bool ok = true;
    for (int i = 1; i < BYTES_PER_FRAME; i++) {
      startVal = Serial.read();
      ok = ok && startVal == BYTE_LOST;
    }

    if (ok == false) {
      return;
    }

    lcd.Fill_Screen(BLACK);
    lcd.Set_Draw_color(GREEN);

    status = STATUS_NO_PLAYER;
    lcd.Fill_Circle(width / 2, height / 2, height / 4);
  }

  if (startVal != BYTE_START) {
    return;
  }

  for (int i = 0; i < NUM_SKEL_POINTS; i++) {
    skeletonPoints[i] = {readCoordinateX(), readCoordinateY()};
  }

  drawSkeleton();
}

void drawSkeleton() {
  lcd.Fill_Screen(BLACK);
  drawHead();

  /* Draw a circle for each skeleton point */
  for (int i = 1; i < NUM_SKEL_POINTS; i++) {
    PVector p = skeletonPoints[i];
    lcd.Fill_Circle(p.x, p.y, 4);
  }

  drawLimb(SKEL_HEAD, SKEL_NECK);

  drawLimb(SKEL_NECK, SKEL_LEFT_SHOULDER);
  drawLimb(SKEL_LEFT_SHOULDER, SKEL_LEFT_ELBOW);
  drawLimb(SKEL_LEFT_ELBOW, SKEL_LEFT_HAND);

  drawLimb(SKEL_NECK, SKEL_RIGHT_SHOULDER);
  drawLimb(SKEL_RIGHT_SHOULDER, SKEL_RIGHT_ELBOW);
  drawLimb(SKEL_RIGHT_ELBOW, SKEL_RIGHT_HAND);

  drawLimb(SKEL_LEFT_SHOULDER, SKEL_TORSO);
  drawLimb(SKEL_RIGHT_SHOULDER, SKEL_TORSO);
  
  drawLimb(SKEL_TORSO, SKEL_LEFT_HIP);
  drawLimb(SKEL_LEFT_HIP, SKEL_LEFT_KNEE);
  drawLimb(SKEL_LEFT_KNEE, SKEL_LEFT_FOOT);
  
  drawLimb(SKEL_TORSO, SKEL_RIGHT_HIP);
  drawLimb(SKEL_RIGHT_HIP, SKEL_RIGHT_KNEE);
  drawLimb(SKEL_RIGHT_KNEE, SKEL_RIGHT_FOOT);

  drawLimb(SKEL_LEFT_HIP, SKEL_RIGHT_HIP);

  drawBodyPart(SKEL_LEFT_SHOULDER, SKEL_RIGHT_SHOULDER, SKEL_TORSO);
  drawBodyPart(SKEL_LEFT_HIP, SKEL_RIGHT_HIP, SKEL_TORSO);
}

void executeStatusNoPlayer() {
  if (Serial.available() < 1) {
    return;
  }

  byte trigger = Serial.read();
  if (trigger != BYTE_RECOGNISED) {
    return;
  }

  status = STATUS_PLAYER_RECOGNISED;
  lcd.Set_Draw_color(YELLOW);
  lcd.Fill_Circle(width / 2, height / 2, height / 4);
}

void executeStatusPlayerRecognised() {
  if (Serial.available() < 1) {
    return;
  }
  
  byte trigger = Serial.read();
  if (trigger != BYTE_START_PLAYING) {
    return;
  }

  status = STATUS_PLAYING;
  lcd.Fill_Screen(BLACK);
  lcd.Set_Draw_color(RED);
  lcd.Fill_Circle(width / 2, height / 2, height / 4);
  delay(1000);
}

void drawHead() {
  lcd.Fill_Circle(skeletonPoints[SKEL_HEAD].x, skeletonPoints[SKEL_HEAD].y, 25);
}

void drawBodyPart(int firstIndex, int secondIndex, int thirdIndex) {
  PVector p1 = skeletonPoints[firstIndex];
  PVector p2 = skeletonPoints[secondIndex];
  PVector p3 = skeletonPoints[thirdIndex];

  lcd.Fill_Triangle(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
}

void drawLimb(int firstIndex, int secondIndex) {
  PVector p1 = skeletonPoints[firstIndex];
  if (p1.x == BYTE_NOT_DETECTED && p1.y == BYTE_NOT_DETECTED) {
    return;
  }

  PVector p2 = skeletonPoints[secondIndex];
  if (p2.x == BYTE_NOT_DETECTED && p2.y == BYTE_NOT_DETECTED) {
    return;
  }

  lcd.Draw_Line(p1.x, p1.y, p2.x, p2.y);
  /* Draw around the point for thicker lines */
  for (int i = 1; i <= 2; i++) {
    lcd.Draw_Line(p1.x + i, p1.y, p2.x + i, p2.y);
    lcd.Draw_Line(p1.x - i, p1.y, p2.x - i, p2.y);
    lcd.Draw_Line(p1.x, p1.y + i, p2.x, p2.y + i);
    lcd.Draw_Line(p1.x, p1.y - i, p2.x, p2.y - i);
  }
}

int readCoordinateX() {
  byte xAsByte = Serial.read();
  int x = (int)map(xAsByte, 0, 255, 0, width);
  Serial.println(x);
  return x;
}

int readCoordinateY() {
  byte yAsByte = Serial.read();
  int y = (int)map(yAsByte, 0, 255, 0, height);
  Serial.println(y);
  return y;
}