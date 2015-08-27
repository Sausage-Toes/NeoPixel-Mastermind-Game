#include <Bounce.h> //http://playground.arduino.cc/code/bounce
#include <Adafruit_NeoPixel.h> //https://github.com/adafruit/Adafruit_NeoPixel

#define LED_PIN 6
#define NUM_LED 108
#define PIN_BTN1 3
#define PIN_BTN2 2
#define INTRPT_BTN1 1
#define INTRPT_BTN2 0
uint8_t brightness = 96;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LED, LED_PIN, NEO_GRB + NEO_KHZ800);
Bounce selectButton = Bounce( PIN_BTN1, 15 ); 
Bounce enterButton = Bounce( PIN_BTN2, 15 ); 

const uint32_t RED = strip.Color(255, 0, 0);
const uint32_t GREEN = strip.Color(0, 255, 0);
const uint32_t BLUE = strip.Color(0, 0, 255);
const uint32_t WHITE = strip.Color(200, 255, 255);
const uint32_t YELLOW = strip.Color(255, 255, 0);
const uint32_t MAGENTA = strip.Color(255, 0, 255);
const uint32_t OFF = strip.Color(0, 0, 0);

const uint32_t colorArray[] = {RED, YELLOW, GREEN, BLUE, MAGENTA, WHITE};
#define totalNumberColors (sizeof(colorArray)/sizeof(uint32_t))
uint8_t colorIndex = 0;

//Gameboard
const uint8_t gameCols = 4; // assumes and equal number of Scoreboard cols
const uint8_t gameRows = 10; // max number of turns
volatile uint8_t turn = 0;

volatile uint32_t guess[gameCols] = {RED, OFF, OFF, OFF};
uint32_t score[gameCols] = {OFF, OFF, OFF, OFF};
uint32_t solution[gameCols] = {RED, YELLOW, GREEN, BLUE};
//uint32_t solution[gameCols] = {GREEN, GREEN, BLUE, BLUE};
uint32_t blank[gameCols] = {OFF, OFF, OFF, OFF};

volatile uint16_t selectIndex = 0;

long timer = 0;
// Game modes:
// 0: Input Code
// 1: Confirm Code
// 2: Game Over
volatile uint8_t mode = 0;
boolean shouldIncrementTurn = false;

void setup() 
{
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  Serial.begin(9600);
  randomSeed(analogRead(0));
  
  strip.setBrightness(brightness);
  
  pinMode(PIN_BTN1, INPUT_PULLUP);  
  pinMode(PIN_BTN2, INPUT_PULLUP);  
  
  attachInterrupt(INTRPT_BTN1, onSelect, HIGH);
  attachInterrupt(INTRPT_BTN2, onEnter, HIGH);
  
  TestStrip();
  startGame();
}

void TestStrip()
{
  for (uint8_t i = 0; i < totalNumberColors; i++)
  {
    for (uint8_t j = 0; j < NUM_LED; j++)
    {
        strip.setPixelColor(j,colorArray[i]);
        strip.show();
        //delay(1); 
    }
  }
  
  for (uint8_t j = 0; j < NUM_LED; j++)
  {
    strip.setPixelColor(j,OFF);
  }
  strip.show();
  
}

void loop() 
{
  
 // TestStrip();
  
  
  if (mode == 0)
  {
    flashPixel(selectIndex, guess[selectIndex-(turn*gameCols)]);
  }
  else if (mode == 1)
  {
      flashCode(turn*gameCols, guess);
  }
  else if (mode == 2)
  {
    for (uint8_t i = 0 ; i < gameRows ; i++)
    {
      flashCode(i*gameCols, solution);
    }
  }
  if (shouldIncrementTurn)
  {
    incrTurn();
  }
}

void onSelect()
{
  if (selectButton.update())
  {
    //Serial.println("Select Button: ");
    
    if (selectButton.read() == HIGH) //true after button press is released
    {
        //Serial.println("HIGH");
        if (mode == 0)
        {
           //next color
          incrSelection();
          //show Guess
          showCode(turn*gameCols, guess);
        }
        else if (mode == 1) //cancel
        {
          mode = 0;
        }
    }
    if (selectButton.read() == LOW) //true after button pressed
    {
        //Serial.println("LOW");
    }
  }
}// onSelect

void incrSelection()
{
  colorIndex++;
  if (colorIndex > totalNumberColors-1) colorIndex = 0;
  guess[selectIndex-(turn*gameCols)] = colorArray[colorIndex];  
} //incrSelection

void incrTurn()
{
    if (turn < gameRows-1)
    {
        turn = turn+1;
        selectIndex = turn*gameCols;
        
        //blank guess
        for (uint8_t i = 0; i < gameCols; i++)
        {
          guess[i] = OFF;
        }
        guess[0] = RED; //set default color
        colorIndex = 0;
        shouldIncrementTurn = false;
    }
    else
    {
      mode = 2;
    }
}


void onEnter()
{
  if (enterButton.update())
  {
    //Serial.println("Enter Button: ");
    
    if (mode == 0)
    {
      if (enterButton.read() == HIGH) //true after button press is released
      {
          //Serial.println("HIGH");
          if (timer == 0) timer = millis();//HACK: correct startup condition where LOW not always fired on first press
          if ((millis() - timer) >= 500) // long press
          {
            //Serial.println("LONG");
            //Validate code is complete
            uint8_t j = 0;
            for (uint8_t i = 0; i < gameCols; i++)
            {
              if (guess[i] != OFF) j++;
            }
            // start confrimation
            if (j== gameCols) mode = 1;
          } //endif // long press
          else
          {
            //move peg (selectIndex)
            selectIndex++;
            if (selectIndex > (turn*gameCols)+gameCols-1) selectIndex = turn*gameCols;
            
            if (guess[selectIndex-(turn*gameCols)] == OFF)//set default
            {
              guess[selectIndex-(turn*gameCols)] = RED;
            }
            colorIndex = getColorIndex(guess[selectIndex-(turn*gameCols)]);
            
            //show Guess
            showCode(turn*gameCols, guess);
          }
      }
      if (enterButton.read() == LOW) //true after button pressed
      {
          //Serial.println("LOW");
          timer = millis();
      }
    } //endif mode = 0
    else if (mode == 1) //input confirmed, advance turn
    {
      if (enterButton.read() == HIGH)
      {
        mode = 0;
        //compute score
         computeScore();
        
        //advance row
        shouldIncrementTurn = true;
      }//      
    } //endif mode 1
   
  } //endif Enter button updated event
  
} //end onEnter

void startGame()
{
  setRandomCode(solution);
  showCode(gameCols*gameRows, solution);
  guess[0] = RED;
  turn = 0;
  showCode(0, guess);
}

uint8_t getColorIndex(uint32_t color)
{
  for (uint8_t i = 0; i < totalNumberColors; i++)
  {
    if (colorArray[i] == color) return i;
  }
  return 0;
}

void flashPixel(uint16_t pixelNum, uint32_t color)
{
  strip.setPixelColor(pixelNum,OFF);
  strip.show();
  delay(100);
  strip.setPixelColor(pixelNum,color);
  strip.show();
  delay(400);
}

void flashCode (uint16_t pixelStart, volatile  uint32_t code[])
{
    showCode(pixelStart, blank);
    delay(100);
    showCode(pixelStart, code);
    delay(400);
}

void showCode (uint16_t pixelStart, volatile uint32_t code[])
{
    for (uint16_t i = 0; i < gameCols; i++)
    {
      strip.setPixelColor(pixelStart+i,code[i]);
    }
    strip.show();
}

void setRandomCode(uint32_t code[])
{
  for (uint16_t i = 0; i < gameCols; i++)
    {
      code[i] = colorArray[random(totalNumberColors)];
    }
}

void computeScore()
{
  uint8_t exactMatch = 0;
  uint8_t colorCorrect = 0;
  //Using a destructive comparison to calc score, should pass by value but always seems to be reference.
  //make local copies of solution and guess arrays  
  uint32_t s[gameCols];
  uint32_t g[gameCols];
  for (uint8_t i = 0; i < gameCols; i++)
  {
    s[i] = solution[i];
    g[i] = guess[i];
  }
  
  for (uint8_t i = 0; i < gameCols; i++)
  {
    if (s[i] == g[i])
    {
      exactMatch++;
      s[i] = g[i] = OFF;
     }
  }
  
  if (exactMatch != gameCols)
  {
    
    for (uint8_t i = 0; i < gameCols; i++)
    {
      if (g[i] != OFF)
      {
        for (uint16_t j = 0; j < 4 ; ++j)
        {
          if (s[j] == g[i])
          {
            s[j] = g[i] = OFF;
            colorCorrect++;
            break;
          }
        }
      }
    }
    
  } //endif not already won
  
  
//blank out score
  for (uint8_t i = 0; i < gameCols; i++)
  {
    score[i] = OFF;
  }
  
  Serial.print("Turn: ");
  Serial.println(turn);
  Serial.print("colorCorrect: "); 
  Serial.println(colorCorrect); 
  Serial.print("exactMatch: ");
  Serial.println(exactMatch);
  
  for (uint8_t i = 0; i < exactMatch; i++)
  {
    score[i] = RED;
  }
  
  for (uint8_t i = exactMatch; i < colorCorrect+exactMatch; i++)
  {
    score[i] = WHITE;
  }
  
  volatile uint32_t reverse[gameCols];
  uint8_t j =gameCols-1;
  for (uint8_t i = 0; i < gameCols; i++)
  {
    reverse[j] = score[i];
    j--;
  }
  
  //showCode(turn*(gameCols+1+6),reverse);
  //showCode((turn*gameCols*2)+gameCols,reverse);
  //showCode((turn*gameCols*2)+gameCols,score);
  
  showSolution();
  
  
  if (exactMatch == gameCols) mode = 2;
}


void showSolution()
{
  if (turn == 0)
  {
    strip.setPixelColor(44+30,score[0]);
    strip.setPixelColor(44+31,score[1]);
    strip.setPixelColor(44+32,score[2]);
    strip.setPixelColor(44+33,score[3]);
  }
  if (turn == 1)
  {
    strip.setPixelColor(44+27,score[0]);
    strip.setPixelColor(44+28,score[1]);
    strip.setPixelColor(44+35,score[2]);
    strip.setPixelColor(44+36,score[3]);
  }
  if (turn == 2)
  {
    strip.setPixelColor(44+24,score[0]);
    strip.setPixelColor(44+25,score[1]);
    strip.setPixelColor(44+38,score[2]);
    strip.setPixelColor(44+39,score[3]);
  }
  if (turn == 3)
  {
    strip.setPixelColor(44+22,score[0]);
    strip.setPixelColor(44+23,score[1]);
    strip.setPixelColor(44+40,score[2]);
    strip.setPixelColor(44+41,score[3]);
  }
  if (turn == 4)
  {
    strip.setPixelColor(44+19,score[0]);
    strip.setPixelColor(44+20,score[1]);
    strip.setPixelColor(44+43,score[2]);
    strip.setPixelColor(44+44,score[3]);
  }
  if (turn == 5)
  {
    strip.setPixelColor(44+16,score[0]);
    strip.setPixelColor(44+17,score[1]);
    strip.setPixelColor(44+46,score[2]);
    strip.setPixelColor(44+47,score[3]);
  }
  if (turn == 6)
  {
    strip.setPixelColor(44+14,score[0]);
    strip.setPixelColor(44+15,score[1]);
    strip.setPixelColor(44+48,score[2]);
    strip.setPixelColor(44+49,score[3]);
  }
  if (turn == 7)
  {
    strip.setPixelColor(44+11,score[0]);
    strip.setPixelColor(44+12,score[1]);
    strip.setPixelColor(44+51,score[2]);
    strip.setPixelColor(44+52,score[3]);
  }
  if (turn == 8)
  {
    strip.setPixelColor(44+8,score[0]);
    strip.setPixelColor(44+9,score[1]);
    strip.setPixelColor(44+54,score[2]);
    strip.setPixelColor(44+55,score[3]);
  }
  if (turn == 9)
  {
    strip.setPixelColor(44+6,score[0]);
    strip.setPixelColor(44+7,score[1]);
    strip.setPixelColor(44+56,score[2]);
    strip.setPixelColor(44+57,score[3]);
  }
  strip.show();
  
}


//bool gameWon ()
//{
//  bool won = false;
//  uint8_t numCorrect = 0;
//  for (uint8_t i = 0; i < gameCols; i++)
//  {
//    if (score[i] == RED)
//    {
//      numCorrect++;
//    }
//  }
//  if (numCorrect == gameCols) won = true;
//  return won;
//}
