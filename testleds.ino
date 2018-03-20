#define FASTLED_ALLOW_INTERRUPTS 0

#include<FastLED.h>

#include "checkbutton.h"

#define NUM_CHUNKS 4
#define NUM_LEDS 16

#define NUM_CHUNKS 1
#define NUM_LEDS 64

#define TOT_LEDS (NUM_LEDS*NUM_CHUNKS)
#define NUM_PATT 64
#define FRAMES_PER_SECOND 40

int gnum_leds = NUM_LEDS;
int gnum_chunks = NUM_CHUNKS;
int gnum_patt = NUM_PATT;
int fps = FRAMES_PER_SECOND;

unsigned long gpos;
unsigned long ppos = 0;

btn_t b5 = { 5, 0, 0, 0, 0, 0 };

int onoff = 1;
int mode = 0;
int submode = 0;
unsigned long cycle_last = 0;
int cycle[] = { 5000, 5000, 5000, 5000 };

CRGBArray<TOT_LEDS> leds;
CRGB patt[NUM_PATT];
//CRGBArray<NUM_PATT> patt;
//unsigned long patt[NUM_PATT];

typedef struct mode {
  int num_leds;
  int num_chunks;
  
  int num_patt;
  CRGB *patt;

  int flags;
  
  void (*mfunc)(struct mode *,int);
} mode_t;


void mm(mode_t *m,int mode);
void mrand(mode_t *m,int mode);
void mdrip(mode_t *m,int mode);
void mdraw(mode_t *m,int mode);

CRGB white[] = { 0xffffff };
CRGB purple[] = { 0xFF00FF };
CRGB red[] = { 0xE00000 };
CRGB blue[] = { 0x0000FF };
CRGB yellow[] = { 0xFFFF00 };
CRGB black[] = { 0x000000 };
CRGB greens[] = { 0x00E00, 0x004000, 0x000400, 0x000200, 0x001000 };

mode_t modes[] = {
  { 64, 1, gnum_patt, patt, 3, mdraw },
  { 64, 1, gnum_patt, patt, 0, mdraw },
  { 64, 1, gnum_patt, patt, 1, mdraw },
  { 64, 1, 5, greens, 1, mdrip },
  { 64, 1, gnum_patt, patt, 1, mdrip },
  { 64, 1, gnum_patt, patt, 0, mdrip },
  
  { 64, 1, gnum_patt, patt, 0, mm },
  { 16, 4, gnum_patt, patt, 0, mm },
  { 4, 16, gnum_patt, patt, 0, mm },
  { 8, 8, gnum_patt, patt, 0, mm },
  { 8, 8, gnum_patt, patt, 2, mm },

  { 64, 1, gnum_patt, patt, 0, mrand },
  { 64, 1, gnum_patt, patt, 1, mrand },
  { 16, 4, gnum_patt, patt, 0, mrand },

  //{ 1, 64, gnum_patt, patt, 0, mm },
  { 1, 64, 1, white, 1, mm },
  { 1, 64, 1, purple, 1, mm },
  { 1, 64, 1, red, 1, mm },
  { 1, 64, 1, blue, 1, mm },
  { 1, 64, 1, yellow, 1, mm },
};
int nmodes = 14;

CRGB grid[16][4];


void cleargrid() {
  for(int i=0;i<16;i++) {
    for(int j=0;j<4;j++) {
      grid[i][j] = black[0];
    }
  }
}

int v[256];
void setup() {
  Serial.begin(9600);

  cleargrid();
  pinMode(5,INPUT_PULLUP);
  pinMode(6,OUTPUT);

  Serial.print("on=");
  Serial.println(onoff);
  if(onoff) digitalWrite(6,HIGH);
  else digitalWrite(6,LOW);
  
  FastLED.addLeds<WS2812B,4>(leds, TOT_LEDS);
  int type = 3;

  if(type == 0) {
    for(int i=0;i<NUM_PATT;i++) {
      patt[i] = 0xf0f00f;
    }    
  } else if((type == 1) && (NUM_PATT > 8)) {
    int b = 0x000040;
    int g = 0x000000;
    patt[0] = 0x400000 | g | b;
    patt[1] = 0x800000 | g | b;
    patt[2] = 0xC00000 | g | b;
    patt[3] = 0xF00000 | g | b;
    patt[4] = patt[2];
    patt[5] = patt[1];
    patt[6] = patt[0];
    for(int i=7;i<NUM_PATT;i++) {
      patt[i] = 0x0000a0;
    }
  } else if(type == 1) {
    patt[0] = 0x4000ff;
    patt[1] = 0xF000ff;
    patt[2] = 0x4000ff;
    for(int i=3;i<NUM_PATT;i++) {
      patt[i] = 0x0100ff;
    }    
  } else if(type == 2) {
    for(int i=0;i<NUM_PATT;i++) {
      float f = (float)(i+1) / NUM_PATT;
      int v = 200;//(millis()/200) % 255;
      int h = (int)(f*255.0);
      h = 127;
      if((h < 0) || (h > 255)) h = 127;
      patt[i] = 0x200020; //CHSV(h,255,v);
    }
  } else if(type == 3) {
    for(int i=0;i<NUM_PATT;i++) {
      float f = (float)(i+1) / NUM_PATT;
      int v = 200;//(millis()/200) % 255;
      int h = (int)(f*255.0);
      //h = 127;
      if((h < 0) || (h > 255)) h = 127;
      patt[i] = CHSV(h,255,v);
    }
  } else if(type == 4) {
    gnum_patt = 16;
    gnum_leds = 16;
    gnum_chunks = 4;

    for(int i=0;i<gnum_leds;i++) {
      patt[i] = 0x00ff00;
    }
  }

}

int pcount = 0;

unsigned long slowpos(unsigned long p) {
  return p/4;
}

void drawgrid(mode_t *m) {
  for(int i=0;i<16;i++) {
    for(int j=0;j<4;j++) {
      int off = j * 16;
      int ii = i;
      if((j%2)==1) ii = 15-i;

      int row = j;
      if(m->flags & 1) row = 3 - j;
      leds[ii+off] = grid[i][row];
    }
  }
}

void mdraw(mode_t *m,int mode) {
  unsigned int pos = gpos;// % m->num_patt;
  if(pos % 5 != 0) return;
  
  int on = random(0,16);
  for(int j=3;j>0;j--) {
    for(int i=0;i<16;i++) {
      grid[i][j] = grid[i][j-1];
    }
  }
  for(int i=0;i<16;i++) {
    if((m->flags & 2) && (grid[i][3] != black[0])) {
      grid[i][0] = black[0];
    } else {
      grid[i][0] = grid[i][1];
    }
  }

  int loc = random(0,m->num_patt);
  grid[on][0] = m->patt[loc];

  drawgrid(m);
}

void mdrip(mode_t *m,int mode) {
  unsigned int pos = gpos;// % m->num_patt;
  if(pos % 6 != 0) return;
  
  int on = random(0,16);
  for(int j=3;j>0;j--) {
    for(int i=0;i<16;i++) {
      grid[i][j] = grid[i][j-1];
    }
  }
  for(int i=0;i<16;i++) {
    grid[i][0] = black[0];
  }
  int loc = random(0,m->num_patt);
  grid[on][0] = m->patt[loc];

  drawgrid(m);
/*
  for(int i=0;i<16;i++) {
    for(int j=0;j<4;j++) {
      int off = j * 16;
      int ii = i;
      if((j%2)==1) ii = 15-i;

      int row = j;
      if(m->flags & 1) row = 3 - j;
      leds[ii+off] = grid[i][row];
    }  
  }
  */
}

void mrand(mode_t *m,int mode) {
  unsigned int pos = gpos;// % m->num_patt;
  if(pos % 8 != 0) return;
  int on = random(0,m->num_leds);
  
  for(int i = 0; i < m->num_leds; i++) {
    int loc;
    loc = random(0,m->num_patt);
        
    for(int j=0;j<m->num_chunks;j++) {
      int off = j*m->num_leds;
      int ii = i;
      if((j%2)==1) ii = (m->num_leds-1)-i;
      
      CRGB color = m->patt[loc];
      if((m->flags & 1) && (on != i)) color = black[0];
      leds[ii+off] = color; //patt[loc];
    }
  }
}

void mm(mode_t *m,int mode) {
  unsigned int pos = gpos;// % m->num_patt;

  if(b5.state) {
    ppos++;
    pos = slowpos(ppos);// % m->num_patt;
  } else if(submode) {
    pos = slowpos(ppos);// % m->num_patt;
  }

  CRGB ocolor = m->patt[0];
  if(m->num_patt == 1) {
    // Single color, change brightness
    pos %= 10000;
    pos *= 5;
    pos = pos%500-250;
    pos = abs(pos);
    ocolor.subtractFromRGB(pos);
  }

  pos %= m->num_patt;
  
  for(int i = 0; i < m->num_leds; i++) {
    int loc;
    if(m->flags & 2) {
      loc = (pos+(i*8))%m->num_patt;
    } else {
      loc = (pos+i)%m->num_patt;
    }
    
    //if(mode == 0) loc = (pos+i)%m->num_patt; 
    //else loc = pos%m->num_patt;
    
    for(int j=0;j<m->num_chunks;j++) {
      int off = j*m->num_leds;
      int ii = i;
      if((j%2)==1) ii = (m->num_leds-1)-i;
      
      CRGB color = m->patt[loc];
      if(m->num_patt == 1) color = ocolor;
      //if(mode == 2) color = 0xffffff;
      leds[ii+off] = color; //patt[loc];
    }
  }
}

void m0(mode_t *m,int mode) { mm(m,mode); }

void loop(){
  checkButton(&b5);
   
  if(!onoff) {
    if(b5.state) {
      onoff = 1;
      digitalWrite(6,HIGH);
      while(checkButton(&b5)) {
        delay(50);
      }
      b5.changed = false;
    } else {
      delay(75);
      return;
    }
  }

  if(b5.state) {
    Serial.println(b5.ptime);
    if(b5.ptime > 2000) {
      onoff = 0;
      Serial.println("OFF");
      digitalWrite(6,LOW);
      while(checkButton(&b5)) {
        delay(50);
      }

      return;
    }
  }
  
  //loop2(); return;
  
  static int j = 0;
  static float h = 0.0;
  static uint8_t hue = 0;
  static float dir = 0.01;

  gpos++;
  //if(gpos >= NUM_PATT) gpos = 0;
  int pos = gpos%NUM_PATT;

  if(!b5.state && b5.changed) {
    b5.changed = false;
    if(b5.ptime < 800) {
      if(submode == 0) {
        mode++;
        mode %= nmodes;
        cleargrid();
      } else {
        gpos = slowpos(ppos);
      }
      submode = 0;
    } else {
      submode++;
    }
    
    //if(mode > 2) mode = 0;
    //cycle_last = millis();
  }

  (*modes[mode].mfunc)(modes+mode,mode);
  /*
  switch(mode) {
    case 0: m0(modes+mode,mode); break;
    default: mm(modes+mode,mode); break;
  }
  */

  /*
  for(int i = 0; i < num_leds; i++) {
    int loc;
    if(mode == 0) loc = (pos+i)%num_patt;
    else loc = pos%num_patt;
    
    for(int j=0;j<num_chunks;j++) {
      int off = j*num_leds;
      int ii = i;
      if((j%2)==1) ii = (num_leds-1)-i;
      
      CRGB color = patt[loc];
      if(mode == 2) color = 0xffffff;
      leds[ii+off] = color; //patt[loc];
    }
  }
*/
  FastLED.show();
  FastLED.delay(1000/fps); 
}

