//LED Controller Data
#include "FastLED.h"
#define NUM_LEDS 300
#define updateLEDS 8
#define COLOR_SHIFT 30000

struct color {
  int r;
  int g;
  int b;
};
typedef struct color Color;

//Pin Data
#define DATA_PIN 9
#define POT_PIN 2
#define MIC_PIN 0

double MAX_BRIGHTNESS = 0.8;
double MIN_BRIGHTNESS = 0.3;

double voltMax = 0.0;
double voltMin = 5.0;
int val;

double currentb = 0.0;
double targetb = 0.0;
double targetRed = 0.0;
double currentRed = 0.0;
double prevRed = 0.0;
double targetGreen = 0.0;
double currentGreen = 0.0;
double prevGreen = 0.0;
double targetBlue = 0.0;
double currentBlue = 0.0;
double prevBlue = 0.0;
double previousb = 0.0;
double wW = 0.0;

double bStep = 2.0;
double cStep = 4.0;

double wWLevel = 0.5;

double maxB1 = 0.0;

//FFT variables
#include "arduinoFFT.h"

#define SAMPLES 32
#define SAMPLING_FREQUENCY 4000

arduinoFFT FFT = arduinoFFT();

unsigned int sampling_period_us;
unsigned long microseconds;

double vReal[SAMPLES];
double vImag[SAMPLES];

double b1 = 0.0;
double t1 = 0.0;
double t2 = 0.0;

//Volume variables
int sampleTime = 50;

unsigned long setTime = COLOR_SHIFT;
int phase = 0;
int interval = 1;

//LED Data Structure
CRGB leds[NUM_LEDS];

void setup() {

  Serial.begin(115200);
  
  sampling_period_us = round(1000000*(1.0/SAMPLING_FREQUENCY));
  
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);

  pinMode(MIC_PIN, INPUT);
  pinMode(POT_PIN, INPUT);

  for(int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB(0, 0, 0);
  }
  FastLED.show();
}

void loop() {
  unsigned long time = millis();
  if(phase == 0) interval = 1;
  if(phase == 7) interval = -1;
  
  if(time / (double)setTime >= 1)
  {
    phase += interval;
    setTime = setTime + COLOR_SHIFT;
    maxB1 = 0.0;
  }
  

  
  for(int i = NUM_LEDS; i >= NUM_LEDS/2 + updateLEDS/2; i--)
  {
    leds[i] = leds[i - updateLEDS/2];
  }

  for(int i = 0; i < NUM_LEDS/2 - updateLEDS/2; i++)
  {
    leds[i] = leds[i + updateLEDS/2];
  }
  
  int signalMax = 0;
  int signalMin = 1024;

  for(int i=0; i<SAMPLES; i++)
  {
    microseconds = micros();    //Overflows after around 70 minutes!
    val = analogRead(MIC_PIN);
    vReal[i] = val;
    vImag[i] = 0;

    if(val < 1024)
    {
      if (val > signalMax) signalMax = val;
      else if (val < signalMin) signalMin = val;
    }
    
    while(micros() < (microseconds + sampling_period_us)){
    }
  }

    
  //Calculate volts for volume
  int peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
  double volts = 5.0 * peakToPeak / 1024;

  //Serial.print(volts);
  //Serial.print("--");
  if(volts < voltMin) voltMin = volts;
  if(volts > voltMax) voltMax = volts;

  //Serial.print("--");
  //Serial.print(voltMax);
  //Serial.print("---");
  //Serial.println(voltMin);
  
  //Map volt to 
  double b = ((volts - voltMin) * 0.9) / ((voltMax - voltMin) + 0.1) / 2;
  if(b < 0.1) b = 0.0;
  if(b > 0.5) b = 0.5;
  //Serial.print(b);
  //Serial.print("--");

  b = b + getPotBrightness();
  if(b < 30) b = b / 2.0;
  //Serial.println(b);

  targetb = b;
  currentb = previousb + (targetb - previousb) / bStep;
  previousb = currentb;

  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
  double peak = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);
  
  double bass2 = 0.0;
  double bass1 = 0.0;
  double treble2 = 0.0;
  double treble1 = 0.0;
    
  for(int i=0; i<(SAMPLES/2); i++)
  {
    //Serial.print((int) vReal[i]);
    //Serial.print(" ");
    
    if(i < SAMPLES/8 && i > 1)
    {
      bass2 += vReal[i];
    }
    else if(i < SAMPLES/4 && i > 1)
    {
      bass1 += vReal[i];
    }
    else if(i < 3 * SAMPLES/8 && i > 1)
    {
      treble2 += vReal[i];
    }
    else if(i > 1)
    {
      treble1 += vReal[i];
    }
    /*View all these three lines in serial terminal to see which frequencies has which amplitudes*/
    //Serial.print((i * 1.0 * SAMPLING_FREQUENCY) / SAMPLES, 1);
    //Serial.print(" ");
    //Serial.println(vReal[i], 1);    //View only this line in serial plotter to visualize the bins
  }

  int p = 0;
  for(int i = 0; i < 16; i++)
  {
    double maxF = (i+1) * (SAMPLING_FREQUENCY / SAMPLES);
    double minF = i * (SAMPLING_FREQUENCY / SAMPLES);

    if(peak > minF && peak < maxF) p = i;
  }
  if(p > 7) p = interval;
  else p = 0;
  //Serial.print(" ");
  //Serial.println(p);
  
  //COLOR STUFF
  
  //BASS BRIGHTNESS
  if(bass2 > 560) b = b * 0.5;
  if(bass1 > maxB1) maxB1 = bass1;
  
  //SMOOTHERS
  if(bass1 > b1) b1 = bass1;
  else b1 = b1 - (b1 - bass1) / 10.0;
  if(treble2 > t2) t2 = treble2;
  else t2 = t2 - (t2 - treble2) / 10.0;
  if(treble1 > t1) t1 = treble1;
  else t1 = t1 - (t1 - treble1) / 10.0;
  Serial.print(currentb);
  Serial.print(" - ");
  Serial.print(b1);
  Serial.print(" - ");
  //WHITEWASH
  //double wW = map(b1 / 4, 0.0, 1280.0, 0.0, currentb);
  wW = wWLevel * (b1 / maxB1);
  
  Serial.println(wW);

  //Treble phase shift
  if(t1/4 + t2/4 > 320) p += interval;

  //Total phase shift
  int currentPhase = phase + p;

  //SINGLE-CASE COLOR SELECTOR

  makeColor(currentPhase);
  
  for(int i = NUM_LEDS/2 - updateLEDS/2; i < NUM_LEDS/2 + updateLEDS/2; i++)
  {
    leds[i] = CRGB((int) currentRed, (int) currentGreen, (int) currentBlue);
  }

  FastLED.show();
  delay(1);
}

double getPotBrightness()
{
  int sum = 0;
  for(int i  = 0; i < 5; i++)
  {
    sum += analogRead(POT_PIN);
  }
  int pot = sum / 5;
  double br = 1.0;
  
  if(pot < 1024 / 4) br = 0.1;
  else if(pot < 1024 / 2) br = 0.3;
  else if(pot < 1024 * 3 / 4) br = 0.5;
  else br = 0.7;
  
  //Serial.print(pot);
  //Serial.print(", ");
  //Serial.println(br);
  
  return br;
}

void makeColor(int phase)
{
  if(phase == 0)
  {
    targetRed = 255 * currentb;
    targetGreen = 255 * currentb;
    if(wW > 0.3) targetBlue = 127 * currentb * wW;
    else targetBlue = 0;
  }
  else if(phase == 1)
  {
    targetRed = 255 * currentb;
    targetGreen = 127 * currentb;
    if(wW > 0.3) targetBlue = 127 * currentb * wW;
    else targetBlue = 0;
  }
  else if(phase == 2)
  {
    targetRed = 255 * currentb;
    if(wW > 0.3) targetGreen = 127 * currentb * wW;
    else targetGreen = 0;
    if(wW > 0.3) targetBlue = 127 * currentb * wW;
    else targetBlue = 0;
  }
  else if(phase == 3)
  {
    targetRed = 255 * currentb;
    if(wW > 0.3) targetGreen = 127 * currentb * wW;
    targetGreen = 0;
    targetBlue = 127 * currentb;
  }
  else if(phase == 4)
  {
    targetRed = 255 * currentb;
    if(wW > 0.3) targetGreen = 127 * currentb * wW;
    else currentGreen = 0;
    targetBlue = 255 * currentb;
  }
  else if(phase == 5)
  {
    if(wW > 0.3) targetRed = 127 * currentb * wW;
    else targetRed = 0;
    if(wW > 0.3) targetGreen = 127 * currentb * wW;
    else targetGreen = 0;
    targetBlue = 255 * currentb;
  }
  else if(phase == 6)
  {
    if(wW > 0.3) targetRed = 127 * currentb * wW;
    else targetRed = 0;
    targetGreen = 255 * currentb;
    targetBlue = 255 * currentb;
  }
  else if(phase == 7)
  {
    if(wW > 0.3) targetRed = 127 * currentb * wW;
    else targetRed = 0;
    targetGreen = 255 * currentb;
    if(wW > 0.3) targetBlue = 127 * currentb * wW;
    else targetBlue = 0;
  }

  currentRed = prevRed + (targetRed - prevRed) / cStep;
  prevRed = currentRed;
  currentGreen = prevGreen + (targetGreen - prevGreen) / cStep;
  prevGreen = currentGreen;
  currentBlue = prevBlue + (targetBlue - prevBlue) / cStep;
  prevBlue = currentBlue;
}
