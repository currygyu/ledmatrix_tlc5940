/*GR-KURUMI Sketch Template Version: V1.00*/
#include <RLduino78.h>
//#include <spi.h>

#define NUM_5940    2

#define GSCLK_PIN   3

#define COM0_PIN    4
#define COM1_PIN    5
#define COM2_PIN    6

#define VPRG_PIN    7
#define BLANK_PIN   8
#define XLAT_PIN    9

#define SIN_PIN     11
#define SCLK_PIN    13

//void myisr( void );
void set_dc( void );
void set_gs( void );
void proc_gs( void );
void proc_update(unsigned long u32ms);
void disp_update( void );
void set_gsval( void );
void set_row( void );
void get_temp( unsigned long u32ms );
void lifegame( void );
void proc_color( void );

int gsupdate = 0;
int dispindex = 0;

byte dcval[NUM_5940][16] = {
    {0x30, 0x30, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x30},
    {0x30, 0x30, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x30},
};

word gsval[NUM_5940][16];

typedef unsigned long RGB;

#define GetRED(x) (byte)(((x)>>16)&0xff)
#define GetGREEN(x) (byte)(((x)>>8)&0xff)
#define GetBLUE(x) (byte)((x)&0xff)

#define SetRGB(r,g,b) (((r)<<16)|((g)<<8)|(b))
RGB dots[8][8] = {
    { 0x000000, 0x000000, 0x000000, 0xFF8811, 0xFF8811, 0x000000, 0x000000, 0x000000 },
    { 0x000000, 0x000000, 0xFF8811, 0x000000, 0x000000, 0xFF8811, 0x000000, 0x000000 },
    { 0x000000, 0xFF8811, 0x000000, 0x000000, 0x000000, 0x000000, 0xFF8811, 0x000000 },
    { 0xFF8811, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0xFF8811 },
    { 0xFF8811, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0xFF8811 },
    { 0x000000, 0xFF8811, 0x000000, 0x000000, 0x000000, 0x000000, 0xFF8811, 0x000000 },
    { 0x000000, 0x000000, 0xFF8811, 0x000000, 0x000000, 0xFF8811, 0x000000, 0x000000 },
    { 0x000000, 0x000000, 0x000000, 0xFF8811, 0xFF8811, 0x000000, 0x000000, 0x000000 }
};    

typedef struct {
  byte red;
  byte green;
  byte blue;
} s_dir_colors;
s_dir_colors dir_colors;

int row_index = 0;
int current_temp = 0;

// the setup routine runs once when you press reset:
void setup() {
  int i, j;

  row_index = 0;
  gsupdate = 0;
  dispindex = 0;

  dir_colors.red = 1;
  dir_colors.green = 0;
  dir_colors.blue = 0;

  for( i = 0; i < NUM_5940; i ++ ){
    for( j = 0; j < 16; j ++ ){
      gsval[i][j] = 0;
    }
  }

  pinMode(COM0_PIN, OUTPUT);
  pinMode(COM1_PIN, OUTPUT);
  pinMode(COM2_PIN, OUTPUT);

  pinMode(VPRG_PIN, OUTPUT);
  pinMode(BLANK_PIN, OUTPUT);
  pinMode(XLAT_PIN, OUTPUT);        // SS(STCP)

  pinMode(SIN_PIN, OUTPUT);
  pinMode(SCLK_PIN, OUTPUT);

  pinMode(GSCLK_PIN, OUTPUT);

  digitalWrite(COM0_PIN, LOW);
  digitalWrite(COM1_PIN, LOW);
  digitalWrite(COM2_PIN, LOW);

  digitalWrite(VPRG_PIN, HIGH);
  digitalWrite(BLANK_PIN, HIGH);
  digitalWrite(XLAT_PIN, LOW);

  digitalWrite(GSCLK_PIN, LOW);

  // start work
  set_dc();

  gsupdate = 1;
  attachCyclicHandler(0, get_temp, 3005);
  attachCyclicHandler(1, proc_update, 1000);
}

// the loop routine runs over and over again forever:
void loop() {
  // check update
//  if( gsupdate == 1 ){
    disp_update();        // update
//    gsupdate = 0;
//  }

  // set row
  set_row();

  // set grayscale data
  set_gs();

  proc_gs();
  
  // increment row
  row_index = ( row_index + 1 ) & 0x7;
}

void get_temp( unsigned long u32ms )
{
  current_temp = getTemperature( TEMP_MODE_CELSIUS );
}

void set_dc( void )
{
  int i = 0;
  int j = 0;
  byte k = 0;

  // Dot Correction
  noInterrupts();

  P0.BIT.bit0 = 1;  // VPRG
  P1.BIT.bit3 = 0;  // XLAT

  P7.BIT.bit0 = 0;  // SCLK

  for( i = 0; i < NUM_5940; i ++  ){
    for( j = 0; j < 16; j ++ ){
      for( k = 0x20; k != 0x00; k >>= 1 ){
        P7.BIT.bit2 = ( ( dcval[i][j] & k ) == 0 ) ? 0 : 1;  // SIN

        P7.BIT.bit0 = 1;  // SCLK
        P7.BIT.bit0 = 0;  // SCLK
      }
    }
  }

  P1.BIT.bit3 = 1;  // XLAT
  P1.BIT.bit3 = 0;  // XLAT

  interrupts();
}

void set_gs( void )
{
  int i = 0;
  int j = 0;
  word k = 0;

  // Gray Scale
  set_gsval();

  noInterrupts();

  P0.BIT.bit0 = 0;  // VPRG

  P1.BIT.bit3 = 0;  // XLAT
  P0.BIT.bit1 = 1;  // BLANK
  P7.BIT.bit0 = 0;  // SCLK

  for( i = 1; i >= 0; i -- ){
    for( j = 0; j < 16; j ++ ){
      for( k = 0x0800; k != 0x0000; k >>= 1 ){
        P7.BIT.bit2 = ( ( gsval[i][j] & k ) == 0 ) ? 0 : 1;  // SIN
        
        P7.BIT.bit0 = 1;  // SCLK
        P7.BIT.bit0 = 0;  // SCLK
      }
    }
  }

  P1.BIT.bit3 = 1;  // XLAT
  P1.BIT.bit3 = 0;  // XLAT

  P7.BIT.bit0 = 1;  // SCLK
  P7.BIT.bit0 = 0;  // SCLK

  interrupts();
}

void proc_gs( void )
{
  int i = 0;

  noInterrupts();

  P0.BIT.bit1 = 0;  // BLANK

  P1.BIT.bit6 = 0;  // GSCLK

  for( i = 0; i < 4096; i ++ ){
    P1.BIT.bit6 = 1;  // GSCLK
    P1.BIT.bit6 = 0;  // GSCLK
  }

  P0.BIT.bit1 = 1;  // BLANK
  P0.BIT.bit1 = 0;  // BLANK

  interrupts();
}

void set_row( void )
{
  // set ROW
  noInterrupts();

  switch( row_index ){
    case 0:
      P3.BIT.bit1 = 0;  // COM0
      P1.BIT.bit5 = 0;  // COM1
      P1.BIT.bit0 = 0;  // COM2
      break;
      
    case 1:
      P3.BIT.bit1 = 1;  // COM0
      P1.BIT.bit5 = 0;  // COM1
      P1.BIT.bit0 = 0;  // COM2
      break;

    case 2:
      P3.BIT.bit1 = 0;  // COM0
      P1.BIT.bit5 = 1;  // COM1
      P1.BIT.bit0 = 0;  // COM2
      break;

    case 3:
      P3.BIT.bit1 = 1;  // COM0
      P1.BIT.bit5 = 1;  // COM1
      P1.BIT.bit0 = 0;  // COM2
      break;

    case 4:
      P3.BIT.bit1 = 0;  // COM0
      P1.BIT.bit5 = 0;  // COM1
      P1.BIT.bit0 = 1;  // COM2
      break;

    case 5:
      P3.BIT.bit1 = 1;  // COM0
      P1.BIT.bit5 = 0;  // COM1
      P1.BIT.bit0 = 1;  // COM2
      break;

    case 6:
      P3.BIT.bit1 = 0;  // COM0
      P1.BIT.bit5 = 1;  // COM1
      P1.BIT.bit0 = 1;  // COM2
      break;

    case 7:
      P3.BIT.bit1 = 1;  // COM0
      P1.BIT.bit5 = 1;  // COM1
      P1.BIT.bit0 = 1;  // COM2
      break;
  
    default:
      break;
  }
  interrupts();
}

void proc_update( unsigned long u32ms )
{
  gsupdate = 1;
}

void set_gsval( void )
{
  /* set gsval */
  gsval[0][0] = GetRED(dots[row_index][5]) * 16;
  gsval[0][1] = GetBLUE(dots[row_index][4]) * 16;
  gsval[0][2] = GetGREEN(dots[row_index][4]) * 16;
  gsval[0][3] = GetRED(dots[row_index][4]) * 16;
  gsval[0][4] = GetBLUE(dots[row_index][3]) * 16;
  gsval[0][5] = GetGREEN(dots[row_index][3]) * 16;
  gsval[0][6] = GetRED(dots[row_index][3]) * 16;
  gsval[0][7] = GetBLUE(dots[row_index][2]) * 16;
  gsval[0][8] = GetGREEN(dots[row_index][2]) * 16;
  gsval[0][9] = GetRED(dots[row_index][2]) * 16;
  gsval[0][10] = GetBLUE(dots[row_index][1]) * 16;
  gsval[0][11] = GetGREEN(dots[row_index][1]) * 16;
  gsval[0][12] = GetRED(dots[row_index][1]) * 16;
  gsval[0][13] = GetBLUE(dots[row_index][0]) * 16;
  gsval[0][14] = GetGREEN(dots[row_index][0]) * 16;
  gsval[0][15] = GetRED(dots[row_index][0]) * 16;

  gsval[1][0] = 0;
  gsval[1][1] = 0;
  gsval[1][2] = 0;
  gsval[1][3] = 0;
  gsval[1][4] = 0;
  gsval[1][5] = 0;
  gsval[1][6] = 0;
  gsval[1][7] = 0;
  gsval[1][8] = GetBLUE(dots[row_index][7]) * 16;
  gsval[1][9] = GetGREEN(dots[row_index][7]) * 16;
  gsval[1][10] = GetRED(dots[row_index][7]) * 16;
  gsval[1][11] = GetBLUE(dots[row_index][6]) * 16;
  gsval[1][12] = GetGREEN(dots[row_index][6]) * 16;
  gsval[1][13] = GetRED(dots[row_index][6]) * 16;
  gsval[1][14] = GetBLUE(dots[row_index][5]) * 16;
  gsval[1][15] = GetGREEN(dots[row_index][5]) * 16;
}

#if 0
void disp_update( void )
{
  int i = 0;
  int j = 0;

  /* update dots */
  for( i = 0; i < 8; i ++ ){
    for( j = 0; j < 8; j ++ ){
        dots[i][j] = ( dispindex == 0 ) ? dispptn0[i][j] : dispptn1[i][j];
    }
  }
  dispindex = ( dispindex == 0 ) ? 1 : 0;
}
#endif

void disp_update( void )
{
  if( gsupdate == 1 ){
    lifegame();
    gsupdate = 0;
  }
  else{
    proc_color();
  }
}  
void lifegame( void )
{
  int y, x;
  int num = 0;
  RGB work[8][8];
    
  for(y=0; y<8; y++){
    for(x=0; x<8; x++){
      work[y][x] = dots[y][x];
    }
  }

  for(y=0; y<8; y++){
    for(x=0; x<8; x++){
      num = 0;
            
      /* count around */
      if( x != 0 ){
        if( dots[y][x-1] != 0 ){
          num ++;
        }
      }
            
      if( x != 7 ){
        if( dots[y][x+1] != 0 ){
          num ++;
        }                    
      }
            
      if( y != 0 ){
        if( dots[y-1][x] != 0 ){
          num ++;
        }
        if( x != 0 ){
          if( dots[y-1][x-1] != 0 ){
            num ++;
          }
        }
        if( x != 7 ){
          if( dots[y-1][x+1] != 0 ){
            num ++;
          }
        }
      }                    

      if( y != 7 ){
        if( dots[y+1][x] != 0 ){
          num ++;
        }
        if( x != 0 ){
          if( dots[y+1][x-1] != 0 ){
            num ++;
          }
        }
        if( x != 7 ){
          if( dots[y+1][x+1] != 0 ){
            num ++;
          }
        }
      }                    

      /* decide */
      if( dots[y][x] == 0 ){
        if( num == 3 ){
          work[y][x] = 0xFF8811;
        }
      }
      else{        
        if( ( num != 2 ) && ( num != 3 ) ){
          work[y][x] = 0;
        }
      }
    }
  }
  for( y=0; y<8; y++ ){
    for( x=0; x<8; x++ ){
      dots[y][x] = work[y][x];
    }
  }
}

void proc_color( void )
{
  int y, x;
  byte tmpred;
  byte tmpgreen;
  byte tmpblue;
  
  for( y=0; y<8; y++ ){
    for( x=0; x<8; x++ ){
      if( dots[y][x] != 0 ){
        tmpred = GetRED( dots[y][x] );
        tmpgreen = GetGREEN( dots[y][x] );
        tmpblue = GetBLUE( dots[y][x] );
         
        if( dir_colors.red == 0 ){
          tmpred = ( tmpred + 0x11 ) & 0xff;
          if( tmpred == 0xff ){
            dir_colors.red = 1;
          }
        }
        else{
          tmpred = ( tmpred - 0x11 ) & 0xff;
          if( tmpred == 0x11 ){
            dir_colors.red = 0;
          }
        }

        if( dir_colors.green == 0 ){
          tmpgreen = ( tmpgreen + 0x11 ) & 0xff;
          if( tmpgreen== 0xff ){
            dir_colors.green = 1;
          }
        }
        else{
          tmpgreen = ( tmpgreen - 0x11 ) & 0xff;
          if( tmpgreen == 0x11 ){
            dir_colors.green = 0;
          }
        }

        if( dir_colors.blue == 0 ){
          tmpblue= ( tmpblue + 0x11 ) & 0xff;
          if( tmpblue == 0xff ){
            dir_colors.green = 1;
          }
        }
        else{
          tmpblue = ( tmpblue - 0x11 ) & 0xff;
          if( tmpblue == 0x11 ){
            dir_colors.blue= 0;
          }
        }
        dots[y][x] = SetRGB( (RGB)tmpred, (RGB)tmpgreen, (RGB)tmpblue );
      }
    }
  }
}
    
    

