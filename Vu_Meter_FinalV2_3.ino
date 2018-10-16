
#include <Adafruit_NeoPixel.h>
#include <FastLED.h>
#include "water_torture.h"

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif
#define N_PIXELS  40  // Number of pixels in strand
#define N_PIXELS_HALF (N_PIXELS/2)
#define MIC_PIN   A0  // Microphone is attached to this analog pin
#define LED_PIN    6  // NeoPixel LED strand is connected to this pin
#define SAMPLE_WINDOW   10  // Sample window for average level
#define PEAK_HANG 24 //Time of pause before peak dot falls
#define PEAK_FALL 20 //Rate of falling peak dot
#define PEAK_FALL2 8 //Rate of falling peak dot
#define INPUT_FLOOR 10 //Lower range of analogRead input
#define INPUT_CEILING 300 //Max range of analogRead input, the lower the value the more sensitive (1023 = max)300 (150)
#define DC_OFFSET  0  // DC offset in mic signal - if unusure, leave 0
#define NOISE     10  // Noise/hum/interference in mic signal
#define SAMPLES   60  // Length of buffer for dynamic level adjustment
#define TOP       (N_PIXELS + 2) // Allow dot to go slightly off scale
#define SPEED .20       // Amount to increment RGB color by each cycle
#define TOP2      (N_PIXELS + 1) // Allow dot to go slightly off scale
#define LAST_PIXEL_OFFSET N_PIXELS-1
#define PEAK_FALL_MILLIS 10  // Rate of peak falling dot
#define POT_PIN    4
#define BG 0
#define LAST_PIXEL_OFFSET N_PIXELS-1
#define SPARKING 50
#define COOLING  55
#define FRAMES_PER_SECOND 60
#define NUM_BALLS         4                  // Number of bouncing balls you want (recommend < 7, but 20 is fun in its own way)
#define GRAVITY           -9.81              // Downward (negative) acceleration of gravity in m/s^2
#define h0                1                  // Starting height, in meters, of the ball (strip length)

#define BRIGHTNESS  35
#define LED_TYPE    WS2812B     // Only use the LED_PIN for WS2812's
#define COLOR_ORDER GRB        
#define COLOR_MIN           0
#define COLOR_MAX         255
#define DRAW_MAX          100
#define SEGMENTS            4  // Number of segments to carve amplitude bar into
#define COLOR_WAIT_CYCLES  10  // Loop cycles to wait between advancing pixel origin
#define qsubd(x, b)  ((x>b)?b:0)
#define qsuba(x, b)  ((x>b)?x-b:0)                                              // Analog Unsigned subtraction macro. if result <0, then => 0. By Andrew Tuline.
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

const int buttonPin = 4;     // the number of the pushbutton pin

//config for balls
float h[NUM_BALLS] ;                         // An array of heights
float vImpact0 = sqrt( -2 * GRAVITY * h0 );  // Impact velocity of the ball when it hits the ground if "dropped" from the top of the strip
float vImpact[NUM_BALLS] ;                   // As time goes on the impact velocity will change, so make an array to store those values
float tCycle[NUM_BALLS] ;                    // The time since the last time the ball struck the ground
int   pos[NUM_BALLS] ;                       // The integer position of the dot on the strip (LED index)
long  tLast[NUM_BALLS] ;                     // The clock time of the last ground strike
float COR[NUM_BALLS] ;                       // Coefficient of Restitution (bounce damping)

struct CRGB leds[N_PIXELS];

Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

static uint16_t dist;         // A random number for noise generator.
uint16_t scale = 30;          // Wouldn't recommend changing this on the fly, or the animation will be really blocky.
uint8_t maxChanges = 48;      // Value for blending between palettes.

//CRGBPalette16 currentPalette(CRGB::Black);
CRGBPalette16 currentPalette(OceanColors_p);
CRGBPalette16 targetPalette(CloudColors_p);
// Water torture
WaterTorture water_torture = WaterTorture(&strip);

//new ripple VU
uint8_t timeval = 20;                                                           // Currently 'delay' value. No, I don't use delays, I use EVERY_N_MILLIS_I instead.
uint16_t loops = 0;                                                             // Our loops per second counter.
bool     samplepeak = 0;                                                        // This sample is well above the average, and is a 'peak'.
uint16_t oldsample = 0;                                                         // Previous sample is used for peak detection and for 'on the fly' values.
bool thisdir = 0;

// Modes
enum
{
} MODE;
bool reverse = true;
int BRIGHTNESS_MAX = 255;
int brightness = 75;

byte
//  peak      = 0,      // Used for falling dot
//  dotCount  = 0,      // Frame counter for delaying dot-falling speed
volCount  = 0;      // Frame counter for storing past volume data
int
reading,
vol[SAMPLES],       // Collection of prior volume samples
lvl       = 10,      // Current "dampened" audio level
minLvlAvg = 0,      // For dynamic adjustment of graph low & high
maxLvlAvg = 512;
float
greenOffset = 30,
blueOffset = 150;
// cycle variables

int CYCLE_MIN_MILLIS = 2;
int CYCLE_MAX_MILLIS = 1000;
int cycleMillis = 20;
bool paused = false;
long lastTime = 0;
bool boring = true;
bool gReverseDirection = false;
int          myhue =   0;
//VU ripple
uint8_t colour;
uint8_t myfade = 255;                                         // Starting brightness.
#define maxsteps 16                                           // Case statement wouldn't allow a variable.
int peakspersec = 0;
int peakcount = 0;
uint8_t bgcol = 0;
int thisdelay = 20;

// FOR SYLON ETC
uint8_t thisbeat =  23;
uint8_t thatbeat =  28;
uint8_t thisfade =   2;                                     // How quickly does it fade? Lower = slower fade rate.
uint8_t thissat = 255;                                     // The saturation, where 255 = brilliant colours.
uint8_t thisbri = 255;

//FOR JUGGLE
uint8_t numdots = 4;                                          // Number of dots in use.
uint8_t faderate = 2;                                         // How long should the trails be. Very low value = longer trails.
uint8_t hueinc = 16;                                          // Incremental change in hue between each dot.
uint8_t thishue = 0;                                          // Starting hue.
uint8_t curhue = 0;
uint8_t thisbright = 255;                                     // How bright should the LED/display be.
uint8_t basebeat = 5;
uint8_t max_bright = 255;

// Twinkle
float redStates[N_PIXELS];
float blueStates[N_PIXELS];
float greenStates[N_PIXELS];
float Fade = 0.96;
unsigned int sample;

//Samples
#define NSAMPLES 64
unsigned int samplearray[NSAMPLES];
unsigned long samplesum = 0;
unsigned int sampleavg = 0;
int samplecount = 0;
//unsigned int sample = 0;
unsigned long oldtime = 0;
unsigned long newtime = 0;

//Ripple variables
int color;
int center = 0;
int step = -1;
int maxSteps = 16;
float fadeRate = 0.80;
int diff;

//VU 8 variables
int
origin = 0,
color_wait_count = 0,
scroll_color = COLOR_MIN,
last_intensity = 0,
intensity_max = 0,
origin_at_flip = 0;
uint32_t
draw[DRAW_MAX];
boolean
growing = false,
fall_from_left = true;

//background color
uint32_t currentBg = random(256);
uint32_t nextBg = currentBg;
//CRGBPalette16 currentPalette;
//CRGBPalette16 targetPalette;
TBlendType    currentBlending;

//Variables will change:
int buttonPushCounter = 0; // counter for the number of button presses
//int buttonState = 0;         // current state of the button
int lastButtonState = 0;

byte peak = 16;      // Peak level of column; used for falling dots
//    unsigned int sample;

byte dotCount = 0;  //Frame counter for peak dot
byte dotHangCount = 0; //Frame counter for holding peak dot

//---LED FX VARS
int idex = 0;                //-LED INDEX (0 to LED_COUNT-1
int ihue = 0;                //-HUE (0-255)
int ibright = 0;             //-BRIGHTNESS (0-255)
int isat = 0;                //-SATURATION (0-255)

//---Button Stuff
char blue_data = 0;
int buttonState = 28;

int long time_pattern = 0;
int long time_change = 0;
int effect = 0;

float fscale( float originalMin, float originalMax, float newBegin, float newEnd, float inputValue, float curve) {
  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;
  // condition curve parameter
  // limit range
  if (curve > 10) curve = 10;
  if (curve < -10) curve = -10;
  curve = (curve * -.1) ; // - invert and scale - this seems more intuitive - postive numbers give more weight to high end on output
  curve = pow(10, curve); // convert linear scale into lograthimic exponent for other pow function
  // Check for out of range inputValues
  if (inputValue < originalMin) {
    inputValue = originalMin;
  }
  if (inputValue > originalMax) {
    inputValue = originalMax;
  }
  // Zero Refference the values
  OriginalRange = originalMax - originalMin;
  if (newEnd > newBegin) {
    NewRange = newEnd - newBegin;
  }
  else {
    NewRange = newBegin - newEnd;
    invFlag = 1;
  }
  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal  =  zeroRefCurVal / OriginalRange;   // normalize to 0 - 1 float
  // Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine
  if (originalMin > originalMax ) {
    return 0;
  }
  if (invFlag == 0) {
    rangedValue =  (pow(normalizedCurVal, curve) * NewRange) + newBegin;
  }
  else {  // invert the ranges
    rangedValue =  newBegin - (pow(normalizedCurVal, curve) * NewRange);
  }
  return rangedValue;
}

void setup() {
  delay(3000);
  analogReference(EXTERNAL);
  Serial.begin(115200);      // SETUP HARDWARE SERIAL (USB)
  // btSerial.begin(9600);    // SETUP SOFTWARE SERIAL (BLUETOOTH)
  Serial1.begin(9600);    // SETUP SOFTWARE SERIAL (MEGA RX1 & TX1)
  pinMode(buttonPin, INPUT_PULLUP);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, N_PIXELS).setCorrection(TypicalLEDStrip);

  for (int i = 0 ; i < NUM_BALLS ; i++) {    // Initialize variables
    tLast[i] = millis();
    h[i] = h0;
    pos[i] = 0;                              // Balls start on the ground
    vImpact[i] = vImpact0;                   // And "pop" up at vImpact0
    tCycle[i] = 0;
    COR[i] = 0.90 - float(i) / pow(NUM_BALLS, 2);
  }

  FastLED.show();
  strip.begin();
  //strip.show(); // all pixels to 'off'
}

void loop() { 
  buttonLoop();
  while (Serial1.available() > 0) {
    //inbyte = Serial1.read();
    blue_data = Serial1.read();
    blueTooth();     
    switch (blue_data) {
      case 98:      //---"b" - SET MAX BRIGHTNESS
        max_bright = Serial1.parseInt();
        FastLED.setBrightness(max_bright);
        break;
    }
  }
}

////=============================================================================================================================================
////==============================================================Start Button and Bluetooth Loop==============================================================
////=============================================================================================================================================

void buttonLoop() {
  //read the pushbutton input pin:
  buttonState = digitalRead(buttonPin);
  //compare the buttonState to its previous state
  if (buttonState != lastButtonState) {
    // if the state has changed, increment the counter
    if (buttonState == HIGH) {
      // if the current state is HIGH then the button
      // wend from off to on:
      buttonPushCounter++;
      Serial.println("on");
      Serial.print("number of button pushes:  ");
      Serial.println(buttonPushCounter);
      if (buttonPushCounter == 30) {
        buttonPushCounter = 1;
      }
    }
    //    else {
    //      // if the current state is LOW then the button
    //      // wend from on to off:
    //      Serial.println("off");
    //    }
  }
  // save the current state as the last state,
  //for next time through the loop
  lastButtonState = buttonState;

  switch (buttonPushCounter) {
    case 1:
      buttonPushCounter == 1; {
        All2();
        break;
      }
    case 2:
      buttonPushCounter == 2; {
        All();
        break;
      }
    case 3:
      buttonPushCounter == 3; {
        VU(); // NORMAL
        break;
      }
    case 4:
      buttonPushCounter == 4; {
        VU1(); // Centre out
        break;
      }
    case 5:
      buttonPushCounter == 5; {
        VU2(); // Centre Inwards
        break;
      }
    case 6:
      buttonPushCounter == 6; {
        VU3(); // Normal Rainbow
        break;
      }
    case 7:
      buttonPushCounter == 7; {
        VU4(); // Centre rainbow
        break;
      }
    case 8:
      buttonPushCounter == 8; {
        VU5(); // Shooting Star
        break;
      }
    case 9:
      buttonPushCounter == 9; {
        VU6(); // Falling star
        break;
      }
    case 10:
      buttonPushCounter == 10; {
        VU7(); // Ripple with background
        break;
      }
    case 11:
      buttonPushCounter == 11; {
        VU8(); // Shatter
        break;
      }
    case 12:
      buttonPushCounter == 12; {
        VU9(); // Pulse
        break;
      }
    case 13:
      buttonPushCounter == 13; {
        VU10(); // stream
        break;
      }
    case 14:
      buttonPushCounter == 14; {
        VU11(); // Ripple without Background
        break;
      }
    case 15:
      buttonPushCounter == 15; {
        VU12(); // Ripple without Background
        break;
      }
    case 16:
      buttonPushCounter == 16; {
        VU13(); // Ripple without Background
        break;
      }
    case 17:
      buttonPushCounter == 17; {
        ripple();
        break;
      }
    case 18:
      buttonPushCounter == 18; {
        ripple2();
        break;
      }
    case 19:
      buttonPushCounter == 19; {
        Twinkle();
        break;
      }
    case 20:
      buttonPushCounter == 20; {
        pattern2(); // sylon
        break;
      }
    case 21:
      buttonPushCounter == 21; {
        juggle2();
        break;
      }
    case 22:
      buttonPushCounter == 22; {
        pattern3();
        break;
      }
    case 23:
      buttonPushCounter == 23; {
        blur();
        break;
      }
    case 24:
      buttonPushCounter == 24; {
        Balls(); //
        break;
      }
    case 25:
      buttonPushCounter == 25; {
        Drip(); //
        break;
      }
    case 26:
      buttonPushCounter == 26; {
        fireblu();
        break;
      }
    case 27:
      buttonPushCounter == 27; {
        fire();
        break;
      }
    case 28:
      buttonPushCounter == 28; {
        rainbow_rotate();
        //rainbow(20);
        break;
      }
    case 29:
      buttonPushCounter == 29; {
        colorWipe(strip.Color(0, 0, 0)); // Black
        break;
      }
  }
}

void blueTooth() {
  if (blue_data == 36) {
    buttonPushCounter = 1;  //All2
  }
  else if (blue_data == 37) { //All
    buttonPushCounter = 2;
  }
  if (blue_data == 10) { //VU
    buttonPushCounter = 3;
  }
  if (blue_data == 11) { //VU1
    buttonPushCounter = 4;
  }
  if (blue_data == 12) {  //VU2
    buttonPushCounter = 5;
  }
  if (blue_data == 13) {  //VU3
    buttonPushCounter = 6;
  }
  if (blue_data == 14) {  //VU4
    buttonPushCounter = 7;
  }
  if (blue_data == 15) {  //VU5
    buttonPushCounter = 8;
  }
  if (blue_data == 16) {  //VU6
    buttonPushCounter = 9;
  }
  if (blue_data == 17) {  //VU7
    buttonPushCounter = 10;
  }
  if (blue_data == 18) {  //VU8
    buttonPushCounter = 11;
  }
  if (blue_data == 19) {  //VU9
    buttonPushCounter = 12;
  }
  if (blue_data == 20) {  //VU10
    buttonPushCounter = 13;
  }
  if (blue_data == 21) {  //VU11
    buttonPushCounter = 14;
  }
  if (blue_data == 22) {   //V12
    buttonPushCounter = 15;
  }
  if (blue_data == 23) {  //VU13
    buttonPushCounter = 16;
  }
  if (blue_data == 24) {
    buttonPushCounter = 17;
  }
  if (blue_data == 25) {
    buttonPushCounter = 18;
  }
  if (blue_data == 26) {
    buttonPushCounter = 19;
  }
  if (blue_data == 27) {
    buttonPushCounter = 20;
  }
  if (blue_data == 28) {
    buttonPushCounter = 21;
  }
  if (blue_data == 29) {
    buttonPushCounter = 22;
  }
  if (blue_data == 30) {
    buttonPushCounter = 23;
  }
  if (blue_data == 31) {
    buttonPushCounter = 24;
  }
  if (blue_data == 32) {
    buttonPushCounter = 25;
  }
  if (blue_data == 33) {
    buttonPushCounter = 26;
  }
  if (blue_data == 34) {
    buttonPushCounter = 27;
  }
  if (blue_data == 35) {
    buttonPushCounter = 28;
  }
  if (blue_data == 38) {
    buttonPushCounter = 29;
  }
}

//========================================================================================================================================
//======================================================END BUTTON CODE===================================================================
//========================================================================================================================================
//========================================================================================================================================


void one_color_all(int cred, int cgrn, int cblu) {       //-SET ALL LEDS TO ONE COLOR
  for (int i = 0 ; i < N_PIXELS; i++ ) {
    leds[i].setRGB( cred, cgrn, cblu);
  }
}

void one_color_allHSV(int ahue) {    //-SET ALL LEDS TO ONE COLOR (HSV)
  for (int i = 0 ; i < N_PIXELS; i++ ) {
    leds[i] = CHSV(ahue, thissat, 255);
  }
}

void colorWipe(uint32_t c) {
  // void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    //    if (digitalRead(buttonPin) != lastButtonState)  // <------------- add this
    //      return;         // <------------ and this
    // delay(wait);
  }
}


void VU() {
  uint8_t  i;
  uint16_t minLvl, maxLvl;
  int      n, height;
  n   = analogRead(MIC_PIN);                        // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvl = ((lvl * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);

  if (height < 0L)       height = 0;     // Clip output
  else if (height > TOP) height = TOP;
  if (height > peak)     peak   = height; // Keep 'peak' dot at top

  // Color pixels based on rainbow gradient
  for (i = 0; i < N_PIXELS; i++) {
    if (i >= height)               strip.setPixelColor(i,   0,   0, 0);
    else strip.setPixelColor(i, Wheel(map(i, 0, strip.numPixels() - 1, 30, 150)));
  }

  // Draw peak dot
  if (peak > 0 && peak <= N_PIXELS - 1) strip.setPixelColor(peak, Wheel(map(peak, 0, strip.numPixels() - 1, 30, 150)));
  strip.setBrightness(max_bright);
  strip.show(); // Update strip
  // Every few frames, make the peak pixel drop by 1:
  if (++dotCount >= PEAK_FALL) { //fall rate
    if (peak > 0) peak--;
    dotCount = 0;
  }

  vol[volCount] = n;                      // Save sample for dynamic leveling
  if (++volCount >= SAMPLES) volCount = 0; // Advance/rollover sample counter

  // Get volume range of prior frames
  minLvl = maxLvl = vol[0];
  for (i = 1; i < SAMPLES; i++) {
    if (vol[i] < minLvl)      minLvl = vol[i];
    else if (vol[i] > maxLvl) maxLvl = vol[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if ((maxLvl - minLvl) < TOP) maxLvl = minLvl + TOP;
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6; // Dampen min/max levels
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6; // (fake rolling average)
}

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

void VU1() {
  uint8_t  i;
  uint16_t minLvl, maxLvl;
  int      n, height;
  n   = analogRead(MIC_PIN);                        // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvl = ((lvl * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);
  if (height < 0L)       height = 0;     // Clip output
  else if (height > TOP) height = TOP;
  if (height > peak)     peak   = height; // Keep 'peak' dot at top

  // Color pixels based on rainbow gradient
  for (i = 0; i < N_PIXELS_HALF; i++) {
    if (i >= height) {
      strip.setPixelColor(N_PIXELS_HALF - i - 1,   0,   0, 0);
      strip.setPixelColor(N_PIXELS_HALF + i,   0,   0, 0);
    }
    else {
      uint32_t color = Wheel(map(i, 0, N_PIXELS_HALF - 1, 30, 150));
      strip.setPixelColor(N_PIXELS_HALF - i - 1, color);
      strip.setPixelColor(N_PIXELS_HALF + i, color);
    }
  }
  // Draw peak dot
  if (peak > 0 && peak <= N_PIXELS_HALF - 1) {
    uint32_t color = Wheel(map(peak, 0, N_PIXELS_HALF - 1, 30, 150));
    strip.setPixelColor(N_PIXELS_HALF - peak - 1, color);
    strip.setPixelColor(N_PIXELS_HALF + peak, color);
  }
  strip.setBrightness(max_bright);
  strip.show(); // Update strip

  // Every few frames, make the peak pixel drop by 1:
  if (++dotCount >= PEAK_FALL) { //fall rate
    if (peak > 0) peak--;
    dotCount = 0;
  }

  vol[volCount] = n;                      // Save sample for dynamic leveling
  if (++volCount >= SAMPLES) volCount = 0; // Advance/rollover sample counter

  // Get volume range of prior frames
  minLvl = maxLvl = vol[0];
  for (i = 1; i < SAMPLES; i++) {
    if (vol[i] < minLvl)      minLvl = vol[i];
    else if (vol[i] > maxLvl) maxLvl = vol[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if ((maxLvl - minLvl) < TOP) maxLvl = minLvl + TOP;
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6; // Dampen min/max levels
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6; // (fake rolling average)
}

void VU2() {
  unsigned long startMillis = millis(); // Start of sample window
  float peakToPeak = 0;   // peak-to-peak level
  unsigned int signalMax = 0;
  unsigned int signalMin = 1023;
  unsigned int c, y;
  while (millis() - startMillis < SAMPLE_WINDOW) {
    sample = analogRead(MIC_PIN);
    if (sample < 1024) {
      if (sample > signalMax) {
        signalMax = sample;
      }
      else if (sample < signalMin) {
        signalMin = sample;
      }
    }
  }
  peakToPeak = signalMax - signalMin;
  // Serial.println(peakToPeak);

  for (int i = 0; i <= N_PIXELS_HALF - 1; i++) {
    uint32_t color = Wheel(map(i, 0, N_PIXELS_HALF - 1, 30, 150));
    strip.setPixelColor(N_PIXELS - i, color);
    strip.setPixelColor(0 + i, color);
  }

  c = fscale(INPUT_FLOOR, INPUT_CEILING, N_PIXELS_HALF, 0, peakToPeak, 2);

  if (c < peak) {
    peak = c;        // Keep dot on top
    dotHangCount = 0;    // make the dot hang before falling
  }
  if (c <= strip.numPixels()) { // Fill partial column with off pixels
    drawLine(N_PIXELS_HALF, N_PIXELS_HALF - c, strip.Color(0, 0, 0));
    drawLine(N_PIXELS_HALF, N_PIXELS_HALF + c, strip.Color(0, 0, 0));
  }




  y = N_PIXELS_HALF - peak;
  uint32_t color1 = Wheel(map(y, 0, N_PIXELS_HALF - 1, 30, 150));
  strip.setPixelColor(y - 1, color1);
  //strip.setPixelColor(y-1,Wheel(map(y,0,N_PIXELS_HALF-1,30,150)));

  y = N_PIXELS_HALF + peak;
  strip.setPixelColor(y, color1);
  //strip.setPixelColor(y+1,Wheel(map(y,0,N_PIXELS_HALF+1,30,150)));

  strip.setBrightness(max_bright);
  strip.show();

  // Frame based peak dot animation
  if (dotHangCount > PEAK_HANG) { //Peak pause length
    if (++dotCount >= PEAK_FALL2) { //Fall rate
      peak++;
      dotCount = 0;
    }
  }
  else {
    dotHangCount++;
  }
}


void VU3() {
  uint8_t i;
  uint16_t minLvl, maxLvl;
  int n, height;

  n = analogRead(MIC_PIN);             // Raw reading from mic
  n = abs(n - 512 - DC_OFFSET);        // Center on zero
  n = (n <= NOISE) ? 0 : (n - NOISE);  // Remove noise/hum
  lvl = ((lvl * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);

  if (height < 0L)       height = 0;      // Clip output
  else if (height > TOP) height = TOP;
  if (height > peak)     peak   = height; // Keep 'peak' dot at top

  greenOffset += SPEED;
  blueOffset += SPEED;
  if (greenOffset >= 255) greenOffset = 0;
  if (blueOffset >= 255) blueOffset = 0;

  // Color pixels based on rainbow gradient
  for (i = 0; i < N_PIXELS; i++) {
    if (i >= height) {
      strip.setPixelColor(i, 0, 0, 0);
    } else {
      strip.setPixelColor(i, Wheel(
                            map(i, 0, strip.numPixels() - 1, (int)greenOffset, (int)blueOffset)
                          ));
    }
  }
  // Draw peak dot
  if (peak > 0 && peak <= N_PIXELS - 1) strip.setPixelColor(peak, Wheel(map(peak, 0, strip.numPixels() - 1, 30, 150)));


  strip.show(); // Update strip

  // Every few frames, make the peak pixel drop by 1:

  if (++dotCount >= PEAK_FALL) { //fall rate

    if (peak > 0) peak--;
    dotCount = 0;
  }

  strip.setBrightness(max_bright);
  strip.show();  // Update strip

  vol[volCount] = n;
  if (++volCount >= SAMPLES) {
    volCount = 0;
  }

  // Get volume range of prior frames
  minLvl = maxLvl = vol[0];
  for (i = 1; i < SAMPLES; i++) {
    if (vol[i] < minLvl) {
      minLvl = vol[i];
    } else if (vol[i] > maxLvl) {
      maxLvl = vol[i];
    }
  }

  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if ((maxLvl - minLvl) < TOP) {
    maxLvl = minLvl + TOP;
  }
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6; // Dampen min/max levels
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6; // (fake rolling average)
}


void VU4() {
  uint8_t  i;
  uint16_t minLvl, maxLvl;
  int      n, height;

  n   = analogRead(MIC_PIN);                        // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvl = ((lvl * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);

  if (height < 0L)       height = 0;     // Clip output
  else if (height > TOP) height = TOP;
  if (height > peak)     peak   = height; // Keep 'peak' dot at top
  greenOffset += SPEED;
  blueOffset += SPEED;
  if (greenOffset >= 255) greenOffset = 0;
  if (blueOffset >= 255) blueOffset = 0;

  // Color pixels based on rainbow gradient
  for (i = 0; i < N_PIXELS_HALF; i++) {
    if (i >= height) {
      strip.setPixelColor(N_PIXELS_HALF - i - 1,   0,   0, 0);
      strip.setPixelColor(N_PIXELS_HALF + i,   0,   0, 0);
    }
    else {
      uint32_t color = Wheel(map(i, 0, N_PIXELS_HALF - 1, (int)greenOffset, (int)blueOffset));
      strip.setPixelColor(N_PIXELS_HALF - i - 1, color);
      strip.setPixelColor(N_PIXELS_HALF + i, color);
    }

  }

  // Draw peak dot
  if (peak > 0 && peak <= N_PIXELS_HALF - 1) {
    uint32_t color = Wheel(map(peak, 0, N_PIXELS_HALF - 1, 30, 150));
    strip.setPixelColor(N_PIXELS_HALF - peak - 1, color);
    strip.setPixelColor(N_PIXELS_HALF + peak, color);
  }

  strip.setBrightness(max_bright);
  strip.show(); // Update strip

  // Every few frames, make the peak pixel drop by 1:

  if (++dotCount >= PEAK_FALL) { //fall rate

    if (peak > 0) peak--;
    dotCount = 0;
  }


  vol[volCount] = n;                      // Save sample for dynamic leveling
  if (++volCount >= SAMPLES) volCount = 0; // Advance/rollover sample counter

  // Get volume range of prior frames
  minLvl = maxLvl = vol[0];
  for (i = 1; i < SAMPLES; i++) {
    if (vol[i] < minLvl)      minLvl = vol[i];
    else if (vol[i] > maxLvl) maxLvl = vol[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if ((maxLvl - minLvl) < TOP) maxLvl = minLvl + TOP;
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6; // Dampen min/max levels
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6; // (fake rolling average)

}

void VU5()
{
  uint8_t  i;
  uint16_t minLvl, maxLvl;
  int      n, height;

  n   = analogRead(MIC_PIN);                        // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvl = ((lvl * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP2 * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);

  if (height < 0L)       height = 0;     // Clip output
  else if (height > TOP2) height = TOP2;
  if (height > peak)     peak   = height; // Keep 'peak' dot at top


#ifdef CENTERED
  // Color pixels based on rainbow gradient
  for (i = 0; i < (N_PIXELS / 2); i++) {
    if (((N_PIXELS / 2) + i) >= height)
    {
      strip.setPixelColor(((N_PIXELS / 2) + i),   0,   0, 0);
      strip.setPixelColor(((N_PIXELS / 2) - i),   0,   0, 0);
    }
    else
    {
      strip.setPixelColor(((N_PIXELS / 2) + i), Wheel(map(((N_PIXELS / 2) + i), 0, strip.numPixels() - 1, 30, 150)));
      strip.setPixelColor(((N_PIXELS / 2) - i), Wheel(map(((N_PIXELS / 2) - i), 0, strip.numPixels() - 1, 30, 150)));
    }
  }

  // Draw peak dot
  if (peak > 0 && peak <= LAST_PIXEL_OFFSET)
  {
    strip.setPixelColor(((N_PIXELS / 2) + peak), 255, 255, 255); // (peak,Wheel(map(peak,0,strip.numPixels()-1,30,150)));
    strip.setPixelColor(((N_PIXELS / 2) - peak), 255, 255, 255); // (peak,Wheel(map(peak,0,strip.numPixels()-1,30,150)));
  }
#else
  // Color pixels based on rainbow gradient
  for (i = 0; i < N_PIXELS; i++)
  {
    if (i >= height)
    {
      strip.setPixelColor(i,   0,   0, 0);
    }
    else
    {
      strip.setPixelColor(i, Wheel(map(i, 0, strip.numPixels() - 1, 30, 150)));
    }
  }

  // Draw peak dot
  if (peak > 0 && peak <= LAST_PIXEL_OFFSET)
  {
    strip.setPixelColor(peak, 255, 255, 255); // (peak,Wheel(map(peak,0,strip.numPixels()-1,30,150)));
  }

#endif

  // Every few frames, make the peak pixel drop by 1:

  if (millis() - lastTime >= PEAK_FALL_MILLIS)
  {
    lastTime = millis();

    strip.setBrightness(max_bright);
    strip.show(); // Update strip

    //fall rate
    if (peak > 0) peak--;
  }

  vol[volCount] = n;                      // Save sample for dynamic leveling
  if (++volCount >= SAMPLES) volCount = 0; // Advance/rollover sample counter

  // Get volume range of prior frames
  minLvl = maxLvl = vol[0];
  for (i = 1; i < SAMPLES; i++)
  {
    if (vol[i] < minLvl)      minLvl = vol[i];
    else if (vol[i] > maxLvl) maxLvl = vol[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if ((maxLvl - minLvl) < TOP2) maxLvl = minLvl + TOP2;
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6; // Dampen min/max levels
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6; // (fake rolling average)
}

void VU6()
{
  uint8_t  i;
  uint16_t minLvl, maxLvl;
  int      n, height;

  n   = analogRead(MIC_PIN);                        // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvl = ((lvl * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP2 * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);

  if (height < 0L)       height = 0;     // Clip output
  else if (height > TOP2) height = TOP2;
  if (height > peak)     peak   = height; // Keep 'peak' dot at top


#ifdef CENTERED
  // Draw peak dot
  if (peak > 0 && peak <= LAST_PIXEL_OFFSET)
  {
    strip.setPixelColor(((N_PIXELS / 2) + peak), 255, 255, 255); // (peak,Wheel(map(peak,0,strip.numPixels()-1,30,150)));
    strip.setPixelColor(((N_PIXELS / 2) - peak), 255, 255, 255); // (peak,Wheel(map(peak,0,strip.numPixels()-1,30,150)));
  }
#else
  // Color pixels based on rainbow gradient
  for (i = 0; i < N_PIXELS; i++)
  {
    if (i >= height)
    {
      strip.setPixelColor(i,   0,   0, 0);
    }
    else
    {
    }
  }

  // Draw peak dot
  if (peak > 0 && peak <= LAST_PIXEL_OFFSET)
  {
    strip.setPixelColor(peak, 0, 0, 255); // (peak,Wheel(map(peak,0,strip.numPixels()-1,30,150)));
  }

#endif

  // Every few frames, make the peak pixel drop by 1:

  if (millis() - lastTime >= PEAK_FALL_MILLIS)
  {
    lastTime = millis();

    strip.setBrightness(max_bright);
    strip.show(); // Update strip

    //fall rate
    if (peak > 0) peak--;
  }

  vol[volCount] = n;                      // Save sample for dynamic leveling
  if (++volCount >= SAMPLES) volCount = 0; // Advance/rollover sample counter

  // Get volume range of prior frames
  minLvl = maxLvl = vol[0];
  for (i = 1; i < SAMPLES; i++)
  {
    if (vol[i] < minLvl)      minLvl = vol[i];
    else if (vol[i] > maxLvl) maxLvl = vol[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if ((maxLvl - minLvl) < TOP2) maxLvl = minLvl + TOP2;
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6; // Dampen min/max levels
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6; // (fake rolling average)
}

void VU7() {

  EVERY_N_MILLISECONDS(1000) {
    peakspersec = peakcount;                                       // Count the peaks per second. This value will become the foreground hue.
    peakcount = 0;                                                 // Reset the counter every second.
  }

  soundmems();

  EVERY_N_MILLISECONDS(20) {
    ripple3();
  }

  show_at_max_brightness_for_power();

} // loop()


void soundmems() {                                                  // Rolling average counter - means we don't have to go through an array each time.
  newtime = millis();
  int tmp = analogRead(MIC_PIN) - 512;
  sample = abs(tmp);

  int potin = map(analogRead(POT_PIN), 0, 1023, 0, 60);

  samplesum = samplesum + sample - samplearray[samplecount];        // Add the new sample and remove the oldest sample in the array
  sampleavg = samplesum / NSAMPLES;                                 // Get an average
  samplearray[samplecount] = sample;                                // Update oldest sample in the array with new sample
  samplecount = (samplecount + 1) % NSAMPLES;                       // Update the counter for the array

  if (newtime > (oldtime + 200)) digitalWrite(13, LOW);             // Turn the LED off 200ms after the last peak.

  if ((sample > (sampleavg + potin)) && (newtime > (oldtime + 60)) ) { // Check for a peak, which is 30 > the average, but wait at least 60ms for another.
    step = -1;
    peakcount++;
    digitalWrite(13, HIGH);
    oldtime = newtime;
  }
}  // soundmems()



void ripple3() {
  for (int i = 0; i < N_PIXELS; i++) leds[i] = CHSV(bgcol, 255, sampleavg * 2); // Set the background colour.

  switch (step) {

    case -1:                                                          // Initialize ripple variables.
      center = random(N_PIXELS);
      colour = (peakspersec * 10) % 255;                                           // More peaks/s = higher the hue colour.
      step = 0;
      bgcol = bgcol + 8;
      break;

    case 0:
      leds[center] = CHSV(colour, 255, 255);                          // Display the first pixel of the ripple.
      step ++;
      break;

    case maxsteps:                                                    // At the end of the ripples.
      // step = -1;
      break;

    default:                                                             // Middle of the ripples.
      leds[(center + step + N_PIXELS) % N_PIXELS] += CHSV(colour, 255, myfade / step * 2);   // Simple wrap from Marc Miller.
      leds[(center - step + N_PIXELS) % N_PIXELS] += CHSV(colour, 255, myfade / step * 2);
      step ++;                                                         // Next step.
      break;
  } // switch step
} // ripple()


void VU8() {
  int intensity = calculateIntensity();
  updateOrigin(intensity);
  assignDrawValues(intensity);
  writeSegmented();
  updateGlobals();
}

int calculateIntensity() {
  int      intensity;

  reading   = analogRead(MIC_PIN);                        // Raw reading from mic
  reading   = abs(reading - 512 - DC_OFFSET); // Center on zero
  reading   = (reading <= NOISE) ? 0 : (reading - NOISE);             // Remove noise/hum
  lvl = ((lvl * 7) + reading) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  intensity = DRAW_MAX * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);

  return constrain(intensity, 0, DRAW_MAX - 1);
}

void updateOrigin(int intensity) {
  // detect peak change and save origin at curve vertex
  if (growing && intensity < last_intensity) {
    growing = false;
    intensity_max = last_intensity;
    fall_from_left = !fall_from_left;
    origin_at_flip = origin;
  } else if (intensity > last_intensity) {
    growing = true;
    origin_at_flip = origin;
  }
  last_intensity = intensity;

  // adjust origin if falling
  if (!growing) {
    if (fall_from_left) {
      origin = origin_at_flip + ((intensity_max - intensity) / 2);
    } else {
      origin = origin_at_flip - ((intensity_max - intensity) / 2);
    }
    // correct for origin out of bounds
    if (origin < 0) {
      origin = DRAW_MAX - abs(origin);
    } else if (origin > DRAW_MAX - 1) {
      origin = origin - DRAW_MAX - 1;
    }
  }
}

void assignDrawValues(int intensity) {
  // draw amplitue as 1/2 intensity both directions from origin
  int min_lit = origin - (intensity / 2);
  int max_lit = origin + (intensity / 2);
  if (min_lit < 0) {
    min_lit = min_lit + DRAW_MAX;
  }
  if (max_lit >= DRAW_MAX) {
    max_lit = max_lit - DRAW_MAX;
  }
  for (int i = 0; i < DRAW_MAX; i++) {
    // if i is within origin +/- 1/2 intensity
    if (
      (min_lit < max_lit && min_lit < i && i < max_lit) // range is within bounds and i is within range
      || (min_lit > max_lit && (i > min_lit || i < max_lit)) // range wraps out of bounds and i is within that wrap
    ) {
      draw[i] = Wheel(scroll_color);
    } else {
      draw[i] = 0;
    }
  }
}

void writeSegmented() {
  int seg_len = N_PIXELS / SEGMENTS;

  for (int s = 0; s < SEGMENTS; s++) {
    for (int i = 0; i < seg_len; i++) {
      strip.setPixelColor(i + (s * seg_len), draw[map(i, 0, seg_len, 0, DRAW_MAX)]);
    }
  }
  strip.setBrightness(max_bright);
  strip.show();
}

uint32_t * segmentAndResize(uint32_t* draw) {
  int seg_len = N_PIXELS / SEGMENTS;

  uint32_t segmented[N_PIXELS];
  for (int s = 0; s < SEGMENTS; s++) {
    for (int i = 0; i < seg_len; i++) {
      segmented[i + (s * seg_len) ] = draw[map(i, 0, seg_len, 0, DRAW_MAX)];
    }
  }

  return segmented;
}

void writeToStrip(uint32_t* draw) {
  for (int i = 0; i < N_PIXELS; i++) {
    strip.setPixelColor(i, draw[i]);
  }
  strip.setBrightness(max_bright);
  strip.show();
}

void updateGlobals() {
  uint16_t minLvl, maxLvl;

  //advance color wheel
  color_wait_count++;
  if (color_wait_count > COLOR_WAIT_CYCLES) {
    color_wait_count = 0;
    scroll_color++;
    if (scroll_color > COLOR_MAX) {
      scroll_color = COLOR_MIN;
    }
  }

  vol[volCount] = reading;                      // Save sample for dynamic leveling
  if (++volCount >= SAMPLES) volCount = 0; // Advance/rollover sample counter

  // Get volume range of prior frames
  minLvl = maxLvl = vol[0];
  for (uint8_t i = 1; i < SAMPLES; i++) {
    if (vol[i] < minLvl)      minLvl = vol[i];
    else if (vol[i] > maxLvl) maxLvl = vol[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if ((maxLvl - minLvl) < N_PIXELS) maxLvl = minLvl + N_PIXELS;
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6; // Dampen min/max levels
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6; // (fake rolling average)
}

void VU9() {
  //currentBlending = LINEARBLEND;
  currentPalette = OceanColors_p;                             // Initial palette.
  currentBlending = LINEARBLEND;
  EVERY_N_SECONDS(5) {                                        // Change the palette every 5 seconds.
    for (int i = 0; i < 16; i++) {
      targetPalette[i] = CHSV(random8(), 255, 255);
    }
  }

  EVERY_N_MILLISECONDS(100) {                                 // AWESOME palette blending capability once they do change.
    uint8_t maxChanges = 24;
    nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);
  }


  EVERY_N_MILLIS_I(thistimer, 20) {                           // For fun, let's make the animation have a variable rate.
    uint8_t timeval = beatsin8(10, 20, 50);                   // Use a sinewave for the line below. Could also use peak/beat detection.
    thistimer.setPeriod(timeval);                             // Allows you to change how often this routine runs.
    fadeToBlackBy(leds, N_PIXELS, 16);                        // 1 = slow, 255 = fast fade. Depending on the faderate, the LED's further away will fade out.
    sndwave();
    soundble();
  }
  FastLED.setBrightness(max_bright);
  FastLED.show();

} // loop()

void soundble() {                                            // Quick and dirty sampling of the microphone.

  int tmp = analogRead(MIC_PIN) - 512 - DC_OFFSET;
  sample = abs(tmp);

}  // soundmems()



void sndwave() {

  leds[N_PIXELS / 2] = ColorFromPalette(currentPalette, sample, sample * 2, currentBlending); // Put the sample into the center

  for (int i = N_PIXELS - 1; i > N_PIXELS / 2; i--) {     //move to the left      // Copy to the left, and let the fade do the rest.
    leds[i] = leds[i - 1];
  }

  for (int i = 0; i < N_PIXELS / 2; i++) {                // move to the right    // Copy to the right, and let the fade to the rest.
    leds[i] = leds[i + 1];
  }
  addGlitter(sampleavg);
}

void VU10() {

  EVERY_N_SECONDS(5) {                                        // Change the target palette to a random one every 5 seconds.
    static uint8_t baseC = random8();                         // You can use this as a baseline colour if you want similar hues in the next line.

    for (int i = 0; i < 16; i++) {
      targetPalette[i] = CHSV(random8(), 255, 255);
    }
  }

  EVERY_N_MILLISECONDS(100) {
    uint8_t maxChanges = 24;
    nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);   // AWESOME palette blending capability.
  }

  EVERY_N_MILLISECONDS(thisdelay) {                           // FastLED based non-blocking delay to update/display the sequence.
    soundtun();
    FastLED.setBrightness(max_bright);
    FastLED.show();
  }
} // loop()



void soundtun() {

  int n;
  n = analogRead(MIC_PIN);                                    // Raw reading from mic
  n = qsuba(abs(n - 512), 10);                                // Center on zero and get rid of low level noise
  CRGB newcolour = ColorFromPalette(currentPalette, constrain(n, 0, 255), constrain(n, 0, 255), currentBlending);
  nblend(leds[0], newcolour, 128);

  for (int i = N_PIXELS - 1; i > 0; i--) {
    leds[i] = leds[i - 1];
  }

} // soundmems()

void VU11() {

  EVERY_N_MILLISECONDS(1000) {
    peakspersec = peakcount;                                  // Count the peaks per second. This value will become the foreground hue.
    peakcount = 0;                                            // Reset the counter every second.
  }

  soundrip();

  EVERY_N_MILLISECONDS(20) {
    rippled();
  }

  FastLED.show();

} // loop()


void soundrip() {                                            // Rolling average counter - means we don't have to go through an array each time.

  newtime = millis();
  int tmp = analogRead(MIC_PIN) - 512;
  sample = abs(tmp);

  int potin = map(analogRead(POT_PIN), 0, 1023, 0, 60);

  samplesum = samplesum + sample - samplearray[samplecount];  // Add the new sample and remove the oldest sample in the array
  sampleavg = samplesum / NSAMPLES;                           // Get an average

  //Serial.println(sampleavg);


  samplearray[samplecount] = sample;                          // Update oldest sample in the array with new sample
  samplecount = (samplecount + 1) % NSAMPLES;                 // Update the counter for the array

  if (newtime > (oldtime + 200)) digitalWrite(13, LOW);       // Turn the LED off 200ms after the last peak.

  if ((sample > (sampleavg + potin)) && (newtime > (oldtime + 60)) ) { // Check for a peak, which is 30 > the average, but wait at least 60ms for another.
    step = -1;
    peakcount++;
    oldtime = newtime;
  }

}  // soundmems()



void rippled() {

  fadeToBlackBy(leds, N_PIXELS, 64);                          // 8 bit, 1 = slow, 255 = fast

  switch (step) {

    case -1:                                                  // Initialize ripple variables.
      center = random(N_PIXELS);
      colour = (peakspersec * 10) % 255;                      // More peaks/s = higher the hue colour.
      step = 0;
      break;

    case 0:
      leds[center] = CHSV(colour, 255, 255);                  // Display the first pixel of the ripple.
      step ++;
      break;

    case maxsteps:                                            // At the end of the ripples.
      // step = -1;
      break;

    default:                                                  // Middle of the ripples.
      leds[(center + step + N_PIXELS) % N_PIXELS] += CHSV(colour, 255, myfade / step * 2);   // Simple wrap from Marc Miller.
      leds[(center - step + N_PIXELS) % N_PIXELS] += CHSV(colour, 255, myfade / step * 2);
      step ++;                                                // Next step.
      break;
  } // switch step

} // ripple()


//Used to draw a line between two points of a given color
void drawLine(uint8_t from, uint8_t to, uint32_t c) {
  uint8_t fromTemp;
  if (from > to) {
    fromTemp = from;
    from = to;
    to = fromTemp;
  }
  for (int i = from; i <= to; i++) {
    strip.setPixelColor(i, c);
  }
}

void setPixel(int Pixel, byte red, byte green, byte blue) {
  strip.setPixelColor(Pixel, strip.Color(red, green, blue));

}


void setAll(byte red, byte green, byte blue) {

  for (int i = 0; i < N_PIXELS; i++ ) {

    setPixel(i, red, green, blue);

  }
  strip.setBrightness(max_bright);
  strip.show();

}
void VU12() {

  EVERY_N_MILLISECONDS(1000) {
    peakspersec = peakcount;                                  // Count the peaks per second. This value will become the foreground hue.
    peakcount = 0;                                            // Reset the counter every second.
  }

  soundripped();

  EVERY_N_MILLISECONDS(20) {
    rippVU();
  }

  FastLED.show();

} // loop()


void soundripped() {                                            // Rolling average counter - means we don't have to go through an array each time.

  newtime = millis();
  int tmp = analogRead(MIC_PIN) - 512;
  sample = abs(tmp);

  int potin = map(analogRead(POT_PIN), 0, 1023, 0, 60);

  samplesum = samplesum + sample - samplearray[samplecount];  // Add the new sample and remove the oldest sample in the array
  sampleavg = samplesum / NSAMPLES;                           // Get an average

  //Serial.println(sampleavg);


  samplearray[samplecount] = sample;                          // Update oldest sample in the array with new sample
  samplecount = (samplecount + 1) % NSAMPLES;                 // Update the counter for the array

  if (newtime > (oldtime + 200)) digitalWrite(13, LOW);       // Turn the LED off 200ms after the last peak.

  if ((sample > (sampleavg + potin)) && (newtime > (oldtime + 60)) ) { // Check for a peak, which is 30 > the average, but wait at least 60ms for another.
    step = -1;
    peakcount++;
    oldtime = newtime;
  }

}  // soundmems()
void rippVU() {                                                                 // Display ripples triggered by peaks.

  fadeToBlackBy(leds, N_PIXELS, 64);                          // 8 bit, 1 = slow, 255 = fast

  switch (step) {

    case -1:                                                  // Initialize ripple variables.
      center = random(N_PIXELS);
      colour = (peakspersec * 10) % 255;                      // More peaks/s = higher the hue colour.
      step = 0;
      break;

    case 0:
      leds[center] = CHSV(colour, 255, 255);                  // Display the first pixel of the ripple.
      step ++;
      break;

    case maxsteps:                                            // At the end of the ripples.
      // step = -1;
      break;

    default:                                                  // Middle of the ripples.
      leds[(center + step + N_PIXELS) % N_PIXELS] += CHSV(colour, 255, myfade / step * 2);   // Simple wrap from Marc Miller.
      leds[(center - step + N_PIXELS) % N_PIXELS] += CHSV(colour, 255, myfade / step * 2);
      step ++;                                                // Next step.
      break;
  } // switch step
  addGlitter(sampleavg);
} // ripple()



void VU13() {                                                                   // The >>>>>>>>>> L-O-O-P <<<<<<<<<<<<<<<<<<<<<<<<<<<<  is buried here!!!11!1!


  EVERY_N_MILLISECONDS(1000) {
    peakspersec = peakcount;                                  // Count the peaks per second. This value will become the foreground hue.
    peakcount = 0;                                            // Reset the counter every second.
  }

  soundripper();

  EVERY_N_MILLISECONDS(20) {
    jugglep();
  }

  FastLED.show();

} // loop()


void soundripper() {                                            // Rolling average counter - means we don't have to go through an array each time.

  newtime = millis();
  int tmp = analogRead(MIC_PIN) - 512;
  sample = abs(tmp);

  int potin = map(analogRead(POT_PIN), 0, 1023, 0, 60);

  samplesum = samplesum + sample - samplearray[samplecount];  // Add the new sample and remove the oldest sample in the array
  sampleavg = samplesum / NSAMPLES;                           // Get an average

  //Serial.println(sampleavg);


  samplearray[samplecount] = sample;                          // Update oldest sample in the array with new sample
  samplecount = (samplecount + 1) % NSAMPLES;                 // Update the counter for the array

  if (newtime > (oldtime + 200)) digitalWrite(13, LOW);       // Turn the LED off 200ms after the last peak.

  if ((sample > (sampleavg + potin)) && (newtime > (oldtime + 60)) ) { // Check for a peak, which is 30 > the average, but wait at least 60ms for another.
    step = -1;
    peakcount++;
    oldtime = newtime;
    // Change the current pattern function periodically.
    jugglep();
  }

} // loop()


void jugglep() {                                                                // Use the juggle routine, but adjust the timebase based on sampleavg for some randomness.

  // Persistent local variables
  static uint8_t thishue = 0;

  timeval = 40;                                                                 // Our EVERY_N_MILLIS_I timer value.

  leds[0] = ColorFromPalette(currentPalette, thishue++, sampleavg, LINEARBLEND);

  for (int i = N_PIXELS - 1; i > 0 ; i-- ) leds[i] = leds[i - 1];

  addGlitter(sampleavg / 2);                                                    // Add glitter based on sampleavg. By Andrew Tuline.

} // matrix()


void Balls() {
  for (int i = 0 ; i < NUM_BALLS ; i++) {
    tCycle[i] =  millis() - tLast[i] ;     // Calculate the time since the last time the ball was on the ground

    // A little kinematics equation calculates positon as a function of time, acceleration (gravity) and intial velocity
    h[i] = 0.5 * GRAVITY * pow( tCycle[i] / 1000 , 2.0 ) + vImpact[i] * tCycle[i] / 1000;

    if ( h[i] < 0 ) {
      h[i] = 0;                            // If the ball crossed the threshold of the "ground," put it back on the ground
      vImpact[i] = COR[i] * vImpact[i] ;   // and recalculate its new upward velocity as it's old velocity * COR
      tLast[i] = millis();

      if ( vImpact[i] < 0.01 ) vImpact[i] = vImpact0;  // If the ball is barely moving, "pop" it back up at vImpact0
    }
    pos[i] = round( h[i] * (N_PIXELS - 1) / h0);       // Map "h" to a "pos" integer index position on the LED strip
  }

  //Choose color of LEDs, then the "pos" LED on
  for (int i = 0 ; i < NUM_BALLS ; i++) leds[pos[i]] = CHSV( uint8_t (i * 40) , 255, 255);
  FastLED.show();
  //Then off for the next loop around
  for (int i = 0 ; i < NUM_BALLS ; i++) {
    leds[pos[i]] = CRGB::Black;
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.setBrightness(max_bright);
    strip.show();
    //    if (digitalRead(buttonPin) != lastButtonState)  // <------------- add this
    //      return;         // <------------ and this
    delay(wait);
  }
}

void ripple() {

  if (currentBg == nextBg) {
    nextBg = random(256);
  }
  else if (nextBg > currentBg) {
    currentBg++;
  } else {
    currentBg--;
  }
  for (uint16_t l = 0; l < N_PIXELS; l++) {
    leds[l] = CHSV(currentBg, 255, 50);         // strip.setPixelColor(l, Wheel(currentBg, 0.1));
  }

  if (step == -1) {
    center = random(N_PIXELS);
    color = random(256);
    step = 0;
  }

  if (step == 0) {
    leds[center] = CHSV(color, 255, 255);         // strip.setPixelColor(center, Wheel(color, 1));
    step ++;
  }
  else {
    if (step < maxSteps) {
      // Serial.println(pow(fadeRate, step));

      leds[wrap(center + step)] = CHSV(color, 255, pow(fadeRate, step) * 255);     //   strip.setPixelColor(wrap(center + step), Wheel(color, pow(fadeRate, step)));
      leds[wrap(center - step)] = CHSV(color, 255, pow(fadeRate, step) * 255);     //   strip.setPixelColor(wrap(center - step), Wheel(color, pow(fadeRate, step)));
      if (step > 3) {
        leds[wrap(center + step - 3)] = CHSV(color, 255, pow(fadeRate, step - 2) * 255);   //   strip.setPixelColor(wrap(center + step - 3), Wheel(color, pow(fadeRate, step - 2)));
        leds[wrap(center - step + 3)] = CHSV(color, 255, pow(fadeRate, step - 2) * 255);   //   strip.setPixelColor(wrap(center - step + 3), Wheel(color, pow(fadeRate, step - 2)));
      }
      step ++;
    }
    else {
      step = -1;
    }
  }

  LEDS.show();
  delay(50);
}


int wrap(int step) {
  if (step < 0) return N_PIXELS + step;
  if (step > N_PIXELS - 1) return step - N_PIXELS;
  return step;
}


void one_color_allHSV(int ahue, int abright) {                // SET ALL LEDS TO ONE COLOR (HSV)
  for (int i = 0 ; i < N_PIXELS; i++ ) {
    leds[i] = CHSV(ahue, 255, abright);
  }
}

void ripple2() {
  if (BG) {
    if (currentBg == nextBg) {
      nextBg = random(256);
    }
    else if (nextBg > currentBg) {
      currentBg++;
    } else {
      currentBg--;
    }
    for (uint16_t l = 0; l < N_PIXELS; l++) {
      strip.setPixelColor(l, Wheel(currentBg, 0.1));
    }
  } else {
    for (uint16_t l = 0; l < N_PIXELS; l++) {
      strip.setPixelColor(l, 0, 0, 0);
    }
  }

  if (step == -1) {
    center = random(N_PIXELS);
    color = random(256);
    step = 0;
  }

  if (step == 0) {
    strip.setPixelColor(center, Wheel(color, 1));
    step ++;
  }
  else {
    if (step < maxSteps) {
      strip.setPixelColor(wrap(center + step), Wheel(color, pow(fadeRate, step)));
      strip.setPixelColor(wrap(center - step), Wheel(color, pow(fadeRate, step)));
      if (step > 3) {
        strip.setPixelColor(wrap(center + step - 3), Wheel(color, pow(fadeRate, step - 2)));
        strip.setPixelColor(wrap(center - step + 3), Wheel(color, pow(fadeRate, step - 2)));
      }
      step ++;
    }
    else {
      step = -1;
    }
  }
  strip.setBrightness(max_bright);
  strip.show();
  delay(50);
}

void fire() {
#define FRAMES_PER_SECOND 40
  random16_add_entropy( random());

  // Array of temperature readings at each simulation cell
  static byte heat[N_PIXELS];

  // Step 1.  Cool down every cell a little
  for ( int i = 0; i < N_PIXELS; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / N_PIXELS) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for ( int k = N_PIXELS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if ( random8() < SPARKING ) {
    int y = random8(7);
    heat[y] = qadd8( heat[y], random8(160, 255) );
  }

  // Step 4.  Map from heat cells to LED colors
  for ( int j = 0; j < N_PIXELS; j++) {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    byte colorindex = scale8( heat[j], 240);
    CRGB color = ColorFromPalette( CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::Yellow,  CRGB::White), colorindex);
    int pixelnumber;
    if ( gReverseDirection ) {
      pixelnumber = (N_PIXELS - 1) - j;
    } else {
      pixelnumber = j;
    }
    leds[pixelnumber] = color;

  }
  FastLED.show();
}

void fireblu() {
#define FRAMES_PER_SECOND 40
  random16_add_entropy( random());

  // Array of temperature readings at each simulation cell
  static byte heat[N_PIXELS];

  // Step 1.  Cool down every cell a little
  for ( int i = 0; i < N_PIXELS; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / N_PIXELS) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for ( int k = N_PIXELS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if ( random8() < SPARKING ) {
    int y = random8(7);
    heat[y] = qadd8( heat[y], random8(160, 255) );
  }

  // Step 4.  Map from heat cells to LED colors
  for ( int j = 0; j < N_PIXELS; j++) {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    byte colorindex = scale8( heat[j], 240);
    CRGB color = ColorFromPalette( CRGBPalette16( CRGB::Black, CRGB::Blue, CRGB::Aqua,  CRGB::White), colorindex);
    int pixelnumber;
    if ( gReverseDirection ) {
      pixelnumber = (N_PIXELS - 1) - j;
    } else {
      pixelnumber = j;
    }
    leds[pixelnumber] = color;

  }
  FastLED.show();
}

void Drip()
{
MODE_WATER_TORTURE:
  if (cycle())
  {
    strip.setBrightness(255); // off limits
    water_torture.animate(reverse);
    strip.setBrightness(max_bright);
    strip.show();
    //strip.setBrightness(brightness); // back to limited
  }
}

bool cycle()
{
  if (paused)
  {
    return false;
  }

  if (millis() - lastTime >= cycleMillis)
  {
    lastTime = millis();
    return true;
  }
  return false;
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos, float opacity) {
  if (WheelPos < 85) {
    return strip.Color((WheelPos * 3) * opacity, (255 - WheelPos * 3) * opacity, 0);
  }
  else if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color((255 - WheelPos * 3) * opacity, 0, (WheelPos * 3) * opacity);
  }
  else {
    WheelPos -= 170;
    return strip.Color(0, (WheelPos * 3) * opacity, (255 - WheelPos * 3) * opacity);
  }
}

void pattern2() {
  sinelon();                                                  // Call our sequence.
  show_at_max_brightness_for_power();                         // Power managed display of LED's.
} // loop()


void sinelon() {
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, N_PIXELS, thisfade);
  int pos1 = beatsin16(thisbeat, 0, N_PIXELS - 1);
  int pos2 = beatsin16(thatbeat, 0, N_PIXELS - 1);
  leds[(pos1 + pos2) / 2] += CHSV( myhue++ / 64, thissat, thisbri);
}

// Pattern 3 - JUGGLE
void pattern3() {
  ChangeMe();
  juggle();
  show_at_max_brightness_for_power();                         // Power managed display of LED's.
} // loop()

void juggle() {                                               // Several colored dots, weaving in and out of sync with each other
  curhue = thishue;                                          // Reset the hue values.
  fadeToBlackBy(leds, N_PIXELS, faderate);
  for ( int i = 0; i < numdots; i++) {
    leds[beatsin16(basebeat + i + numdots, 0, N_PIXELS - 1)] += CHSV(curhue, thissat, thisbright); //beat16 is a FastLED 3.1 function
    curhue += hueinc;
  }
} // juggle()

void ChangeMe() {                                             // A time (rather than loop) based demo sequencer. This gives us full control over the length of each sequence.
  uint8_t secondHand = (millis() / 1000) % 30;                // IMPORTANT!!! Change '30' to a different value to change duration of the loop.
  static uint8_t lastSecond = 99;                             // Static variable, means it's only defined once. This is our 'debounce' variable.
  if (lastSecond != secondHand) {                             // Debounce to make sure we're not repeating an assignment.
    lastSecond = secondHand;
    if (secondHand ==  0)  {
      numdots = 1;  // You can change values here, one at a time , or altogether.
      faderate = 2;
    }
    if (secondHand == 10)  {
      numdots = 4;
      thishue = 128;
      faderate = 8;
    }
    if (secondHand == 20)  {
      hueinc = 48;  // Only gets called once, and not continuously for the next several seconds. Therefore, no rainbows.
      thishue = random8();
    }
  }
} // ChangeMe()

void juggle2() {                            // Several colored dots, weaving in and out of sync with each other
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, N_PIXELS, 20);
  byte dothue = 0;
  for ( int i = 0; i < 8; i++) {
    leds[beatsin16(i + 7, 0, N_PIXELS - 1)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
  FastLED.show();

}
void addGlitter( fract8 chanceOfGlitter) {                                      // Let's add some glitter, thanks to Mark
  if ( random8() < chanceOfGlitter) {
    leds[random16(N_PIXELS)] += CRGB::White;
  }
} // addGlitter()

void Twinkle() {
  if (random(25) == 1) {
    uint16_t i = random(N_PIXELS);
    if (redStates[i] < 1 && greenStates[i] < 1 && blueStates[i] < 1) {
      redStates[i] = random(256);
      greenStates[i] = random(256);
      blueStates[i] = random(256);
    }
  }

  for (uint16_t l = 0; l < N_PIXELS; l++) {
    if (redStates[l] > 1 || greenStates[l] > 1 || blueStates[l] > 1) {
      strip.setPixelColor(l, redStates[l], greenStates[l], blueStates[l]);

      if (redStates[l] > 1) {
        redStates[l] = redStates[l] * Fade;
      } else {
        redStates[l] = 0;
      }

      if (greenStates[l] > 1) {
        greenStates[l] = greenStates[l] * Fade;
      } else {
        greenStates[l] = 0;
      }

      if (blueStates[l] > 1) {
        blueStates[l] = blueStates[l] * Fade;
      } else {
        blueStates[l] = 0;
      }

    } else {
      strip.setPixelColor(l, 0, 0, 0);
    }
  }
  strip.setBrightness(max_bright);
  strip.show();
  delay(10);
}

void blur() {
  uint8_t blurAmount = dim8_raw( beatsin8(3, 64, 192) );      // A sinewave at 3 Hz with values ranging from 64 to 192.
  blur1d( leds, N_PIXELS, blurAmount);                        // Apply some blurring to whatever's already on the strip, which will eventually go black.

  uint8_t  i = beatsin8(  9, 0, N_PIXELS - 1);
  uint8_t  j = beatsin8( 7, 0, N_PIXELS - 1);
  uint8_t  k = beatsin8(  5, 0, N_PIXELS - 1);

  // The color of each point shifts over time, each at a different speed.
  uint16_t ms = millis();
  leds[(i + j) / 2] = CHSV( ms / 29, 200, 255);
  leds[(j + k) / 2] = CHSV( ms / 41, 200, 255);
  leds[(k + i) / 2] = CHSV( ms / 73, 200, 255);
  leds[(k + i + j) / 3] = CHSV( ms / 53, 200, 255);
  FastLED.show();
} // loop()

void rainbow(uint8_t wait) {
  uint16_t i, j;
  for (j = 0; j < 256; j++) {
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i + j) & 255));
    }
    strip.setBrightness(max_bright);
    strip.show();
    // check if a button pressed
    //    if (digitalRead(buttonPin) != lastButtonState)  // <------------- add this
    //      return;         // <------------ and this
    delay(wait);
  }
}

void rainbow_rotate() {                      //-m88-RAINBOW FADE FROM FAST_SPI2 in rotation
  ihue -= 1;
  fill_rainbow2( leds, N_PIXELS, ihue, 256.0 / double(N_PIXELS) );
  FastLED.setBrightness(max_bright);
  FastLED.show();
  delay(thisdelay);
}

void fill_rainbow2( struct CRGB * pFirstLED, int numToFill, uint8_t initialhue, double deltahue ) {
  CHSV hsv;
  double doublehue = double(initialhue);
  hsv.hue = initialhue;
  hsv.val = 255;
  hsv.sat = 255;
  for ( int i = 0; i < numToFill; i++) {
    hsv.hue = int(doublehue);
    hsv2rgb_rainbow( hsv, pFirstLED[i]);
    doublehue += deltahue;
  }
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = {ripple, ripple2, Twinkle, pattern2, juggle2, pattern3, blur, Balls, Drip, fireblu, fire, rainbow_rotate};
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}
void All()
{
  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();
  EVERY_N_SECONDS( 30 ) {
    nextPattern();  // change patterns periodically
  }
}
// second list

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList qPatterns = {VU, VU1, VU2, VU3, VU4, VU5, VU6, VU7, VU8, VU9, VU10, VU11, VU12, VU13};
uint8_t qCurrentPatternNumber = 0; // Index number of which pattern is current

void nextPattern2()
{
  // add one to the current pattern number, and wrap around at the end
  qCurrentPatternNumber = (qCurrentPatternNumber + 1) % ARRAY_SIZE( qPatterns);
}
void All2()
{
  // Call the current pattern function once, updating the 'leds' array
  qPatterns[qCurrentPatternNumber]();
  EVERY_N_SECONDS( 30 ) {
    nextPattern2();  // change patterns periodically
  }
}

void demo_modeB() {
  int r = 10;
  //one_color_all(0, 0, 0); FastLED.show();
  colorWipe(strip.Color(0, 0, 0));
  thisdelay = 15;
  //thisdelay = 5;
  for (int i = 0; i < r * 120; i++) {
    rainbow_rotate();
  }
  for (int i = 0; i < r * 25; i++) {
    ripple();
  }
  for (int i = 0; i < r * 25; i++) {
    ripple2();
  }
  for (int i = 0; i < r * 25; i++) {
    Twinkle();
  }
  for (int i = 0; i < r * 35; i++) {
    pattern2();
  }
  for (int i = 0; i < r * 25; i++) {
    juggle2();
  }
  for (int i = 0; i < r * 35; i++) {
    pattern3();
  }
  for (int i = 0; i < r * 35; i++) {
    blur();
  }
  for (int i = 0; i < r * 35; i++) {
    Balls();
  }
  for (int i = 0; i < r * 45; i++) {
    Drip();
  }
  for (int i = 0; i < r * 40; i++) {
    fireblu();
  }
  for (int i = 0; i < r * 50; i++) {
    fire();
  }
  //one_color_all(0, 0, 0); FastLED.show();
  colorWipe(strip.Color(0, 0, 0));
}

void demo_modeC() {

  if (millis() - time_change > 12000) {   //code that establishes how often to change effect
    effect++;
    if (effect > 7) {
      effect = 0;
    }
    time_change = millis();
  }

  switch (effect) {
    case 0:
      ripple();
      break;

    case 1:
      if (millis() - time_pattern > 15) {
        ripple2();
        time_pattern = millis();
      }

      break;

    case 2:
      if (millis() - time_pattern > 15) {
        Twinkle();
        time_pattern = millis();
      }
      break;

    case 3:
      if (millis() - time_pattern > 15) {
        pattern2();
        time_pattern = millis();
      }

      break;

    case 4:
      if (millis() - time_pattern > 15) {
        juggle2();
        time_pattern = millis();
      }
      break;
    case 5:
      if (millis() - time_pattern > 15) {
        pattern3();
        time_pattern = millis();
      }
      break;
    case 6:
      if (millis() - time_pattern > 15) {
        blur();
        time_pattern = millis();
      }

      break;

    case 7:
      if (millis() - time_pattern > 15) {
        Drip();
        time_pattern = millis();
      }
    case 8:
      if (millis() - time_pattern > 15) {
        fireblu();
        time_pattern = millis();
      }
    case 9:
      fire();
      break;
  }
}
