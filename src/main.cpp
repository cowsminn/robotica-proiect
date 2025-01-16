#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET   -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// PIN BUTTONS
#define BUTTON_LEFT  10
#define BUTTON_RIGHT 2
#define BUTTON_RESTART 3

// GAME STATES
enum GameState{
  MENU,
  PLAYING,
  GAME_OVER,
  WIN
};

GameState currentState = MENU;
bool menuSelection = 0;

// CAR CONSTANTS
const int carWidth  = 18; // 3 squares wide, each 6 pixels => 18
const int carHeight = 24; // 4 squares tall, each 6 pixels => 24

int carPositionIndex = 0; // 0 = left lane, 1 = middle lane
// Lane positions as X-coordinates on the screen
const int lanePositions[] = {
    static_cast<int>(SCREEN_WIDTH * 0.175 - carWidth  / 2),
    static_cast<int>(SCREEN_WIDTH * 0.525 - carWidth / 2)
};

int carY = SCREEN_HEIGHT - 25;

// OBSTACLE VARIABLES
int obstaclePositionIndex; 
int obstacleY = 0;      

// GAME VARIABLES
int  score    = 0;
bool gameOver = false;

// DEBOUNCE
unsigned long lastDebounceTime   = 0;
const unsigned long debounceDelay = 200; // ms
volatile bool restartPressed = false;
unsigned long lastInterruptTime = 0;

// ROAD CONSTANTS
const int LEFT_LINE   = 0;                              
const int DOTTED_LINE = static_cast<int>(SCREEN_WIDTH * 0.35); 
const int RIGHT_LINE  = static_cast<int>(SCREEN_WIDTH * 0.70); 

void updateGame();
void drawGame();
void resetGame();
void drawMenu();
void drawCar(int x, int y);
void drawObstacle(int x, int y);
void drawDottedLine();
void handleMenuInput();
void handleRestartButtonPress();

void setup() {

  pinMode(BUTTON_LEFT,  INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_RESTART), handleRestartButtonPress, FALLING);

  Serial.begin(9600);

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();

  obstaclePositionIndex = random(0, 2);
  obstacleY = 0;
}

void handleRestartButtonPress() {
  unsigned long interruptTime = millis();

  if (interruptTime - lastInterruptTime > debounceDelay) {
    restartPressed = true;
    lastInterruptTime = interruptTime;
  }
}

void loop() {
  if (currentState == PLAYING) {
    if (!gameOver && score < 50) { 
      updateGame();
      drawGame();
      delay(35);
    } else if (score >= 50) { 
      currentState = WIN;
      display.clearDisplay();
      display.display();     
    } else {
      currentState = GAME_OVER;
    }
  } else if (currentState == WIN) { 
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 20);
    display.println("YOU WIN!");

    display.setTextSize(1);
    display.setCursor(40, 45);
    display.print("Score: ");
    display.print(score);

    display.display();
    delay(3000);

    resetGame();
    currentState = MENU;
  } else if (currentState == GAME_OVER) { 
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(8, 20);
    display.println("GAME OVER!");

    display.setTextSize(1);
    display.setCursor(40, 45);
    display.print("Score: ");
    display.print(score);

    display.display();
    delay(3000);

    resetGame();
    currentState = MENU;
  } else if (currentState == MENU) { 
    drawMenu();
    handleMenuInput();
  }
    if (restartPressed) {
    restartPressed = false;
    if (menuSelection == 0) {
      currentState = PLAYING;
    } else if (menuSelection == 1) {
      // turn off the screen
      display.clearDisplay();
      display.display();
      display.ssd1306_command(SSD1306_DISPLAYOFF); 
      delay(5000); 

      display.ssd1306_command(SSD1306_DISPLAYON);  
      display.clearDisplay();
      display.display();

      menuSelection = 0;
      resetGame();
      currentState = MENU;
    }
  }

}

void updateGame() {
  unsigned long currentTime = millis();
  if (digitalRead(BUTTON_LEFT) == LOW && carPositionIndex > 0
      && (currentTime - lastDebounceTime > debounceDelay)) {
    carPositionIndex--;
    lastDebounceTime = currentTime;
  }

  if (digitalRead(BUTTON_RIGHT) == LOW && carPositionIndex < 1
      && (currentTime - lastDebounceTime > debounceDelay)) {
    carPositionIndex++;
    lastDebounceTime = currentTime;
  }

  obstacleY += 4;

  if (obstacleY > SCREEN_HEIGHT) {
    obstacleY = 0;
    obstaclePositionIndex = random(0, 2);
    score++;
    Serial.print("Score: ");
    Serial.println(score);
  }

  // Collision check
  if ((obstacleY + carHeight >= carY) && (obstacleY <= carY + carHeight)) {
    int obstacleX = lanePositions[obstaclePositionIndex];
    int carX      = lanePositions[carPositionIndex];
    if (abs(obstacleX - carX) < carWidth) {
      gameOver = true;
    }
  }
}

void drawMenu() {
  display.clearDisplay();

  if (menuSelection == 0) {
    display.fillRect(10, 20, 108, 16, SSD1306_WHITE); // Highlight background
    display.setTextColor(SSD1306_BLACK);             // Inverted text
  } else {
    display.setTextColor(SSD1306_WHITE);             // Normal text
  }
  display.setTextSize(2);
  display.setCursor(20, 20);
  display.println("START");

  // Draw "EXIT" option
  if (menuSelection == 1) {
    display.fillRect(10, 40, 108, 16, SSD1306_WHITE); // Highlight background
    display.setTextColor(SSD1306_BLACK);             // Inverted text
  } else {
    display.setTextColor(SSD1306_WHITE);             // Normal text
  }
  display.setTextSize(2);
  display.setCursor(20, 40);
  display.println("EXIT");

  display.display();
}


void handleMenuInput() {
  unsigned long currentTime = millis();

  if (digitalRead(BUTTON_LEFT) == LOW && (currentTime - lastDebounceTime > debounceDelay)) {
    menuSelection = (menuSelection == 0) ? 1 : 0;
    lastDebounceTime = currentTime;
  }

  if (digitalRead(BUTTON_RIGHT) == LOW && (currentTime - lastDebounceTime > debounceDelay)) {
    menuSelection = (menuSelection == 0) ? 1 : 0;
    lastDebounceTime = currentTime;
  }
}


void drawGame() {

  display.clearDisplay();

  // 1) Draw the road lines
  display.drawLine(LEFT_LINE,   0, LEFT_LINE,   SCREEN_HEIGHT, SSD1306_WHITE);
  drawDottedLine();
  display.drawLine(RIGHT_LINE,  0, RIGHT_LINE,  SCREEN_HEIGHT, SSD1306_WHITE);

  // 2) Draw the car
  drawCar(lanePositions[carPositionIndex], carY);

  // 3) Draw the obstacle
  drawObstacle(lanePositions[obstaclePositionIndex], obstacleY);

  // 4) Draw the score
  {
    int scoreAreaStartX = (int)(SCREEN_WIDTH * 0.70);  // ~70% of 128 => ~89
    int scoreAreaWidth  = SCREEN_WIDTH - scoreAreaStartX; // ~39

    // DRAW SCORE
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    int labelX = scoreAreaStartX + 7;
    int labelY = 20;
    display.setCursor(labelX, labelY);
    display.println("Score");

    display.setTextSize(2);
    int numberWidth = String(score).length() * 12; 
    int numberX = scoreAreaStartX + (scoreAreaWidth - numberWidth) / 2 + 4; 
    int numberY = 40;  
    display.setCursor(numberX, numberY);
    display.println(score);
  }

  display.display();
}


// RESET GAME
void resetGame() {
  carPositionIndex      = 0;
  obstaclePositionIndex = random(0, 2);
  obstacleY             = 0;
  score                 = 0;
  gameOver              = false;
}

// DRAW CAR
void drawCar(int x, int y) {
  int size = 6;

  // Row 1
  display.fillRect(x + size, y, size, size, SSD1306_WHITE);

  // Row 2
  display.fillRect(x,            y + size, size, size, SSD1306_WHITE);
  display.fillRect(x + size,     y + size, size, size, SSD1306_WHITE);
  display.fillRect(x + size * 2, y + size, size, size, SSD1306_WHITE);

  // Row 3
  display.fillRect(x + size, y + size * 2, size, size, SSD1306_WHITE);

  // Row 4
  display.fillRect(x,            y + size * 3, size, size, SSD1306_WHITE);
  display.fillRect(x + size * 2, y + size * 3, size, size, SSD1306_WHITE);
}

// DRAW OBSTACLE
void drawObstacle(int x, int y) {
  int size = 6;

  // Row 1 (top)
  display.fillRect(x,            y, size, size, SSD1306_WHITE);
  display.fillRect(x + size * 2, y, size, size, SSD1306_WHITE);

  // Row 2
  display.fillRect(x + size, y + size, size, size, SSD1306_WHITE);

  // Row 3
  display.fillRect(x,            y + size * 2, size, size, SSD1306_WHITE);
  display.fillRect(x + size,     y + size * 2, size, size, SSD1306_WHITE);
  display.fillRect(x + size * 2, y + size * 2, size, size, SSD1306_WHITE);

  // Row 4 (bottom)
  display.fillRect(x + size, y + size * 3, size, size, SSD1306_WHITE);
}

// DRAW DOTTED LINE
void drawDottedLine() {
  for (int y = 0; y < SCREEN_HEIGHT; y += 8) {
    display.drawLine(DOTTED_LINE, y, DOTTED_LINE, y + 4, SSD1306_WHITE);
  }
}
