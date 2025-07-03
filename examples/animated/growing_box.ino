#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
  tft.init();
  tft.setRotation(1); // Adjust the rotation if needed
  tft.fillScreen(TFT_BLACK);
}

void loop() {
  showJokeAnimation();
  delay(5000); // Delay before showing the next joke
}

void showJokeAnimation() {
  const int JOKE_BG_COLOR = TFT_BLACK;
  const int JOKE_Y = 50;
  const int JOKE_BORDER_RADIUS = 15;
  const int JOKE_WIDTH = tft.width() - 20;
  const int JOKE_HEIGHT = 120;
  const int ANIMATION_STEPS = 20;
  const int ANIMATION_DELAY = 20;

  // Clear the screen
  tft.fillScreen(JOKE_BG_COLOR);

  // Animate the joke container
  for (int i = 0; i <= ANIMATION_STEPS; i++) {
    int width = map(i, 0, ANIMATION_STEPS, 0, JOKE_WIDTH);
    int height = map(i, 0, ANIMATION_STEPS, 0, JOKE_HEIGHT);
    int x = (tft.width() - width) / 2;
    int y = JOKE_Y + (JOKE_HEIGHT - height) / 2;

    tft.fillRoundRect(x, y, width, height, JOKE_BORDER_RADIUS, TFT_DARKGREY);
    tft.drawRoundRect(x, y, width, height, JOKE_BORDER_RADIUS, TFT_WHITE);

    delay(ANIMATION_DELAY);
  }

  // Draw the emoji
  int emojiX = 35;
  int emojiY = JOKE_Y + 30;
  tft.fillCircle(emojiX, emojiY, 15, TFT_YELLOW);
  tft.fillCircle(emojiX - 7, emojiY - 5, 3, TFT_BLACK);
  tft.fillCircle(emojiX + 7, emojiY - 5, 3, TFT_BLACK);
  tft.drawArc(emojiX, emojiY, 10, 8, 45, 135, TFT_BLACK, TFT_YELLOW);

  // Display the joke title
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(60, JOKE_Y + 20);
  tft.print("JOKE OF THE DAY:");

  // Word-wrap and display the joke
  int xPos = 20;
  int yPos = JOKE_Y + 50;
  int maxWidth = JOKE_WIDTH - 20;

  String currentJoke = "Why don't scientists trust atoms? Because they make up everything!";
  String words = currentJoke + " ";
  String line = "";
  int spacePos = words.indexOf(' ');
        
  while (spacePos > -1) {
    String word = words.substring(0, spacePos + 1);
    words = words.substring(spacePos + 1);
    if (tft.textWidth(line + word) <= maxWidth) {
      line += word;
    } else {
      tft.setCursor(xPos, yPos);
      tft.print(line);
      line = word;
      yPos += 22; 
      if (yPos > (JOKE_Y + 110)) {
        tft.setCursor(xPos, yPos);
        tft.print(line + "...");
        break;
      }
    }
    spacePos = words.indexOf(' ');
    if (spacePos == -1 && words.length() > 0) {
      words += " ";
      spacePos = words.length() - 1;
    }
  }
  if (line.length() > 0 && yPos <= (JOKE_Y + 110)) {
    tft.setCursor(xPos, yPos);
    tft.print(line);
  }
}
