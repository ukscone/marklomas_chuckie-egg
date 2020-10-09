/*>r6502main.c
 *
 * BBC 6502 to RISC OS library code
 * by Michael Foot.
 * Version 1.02 (18 Apr 2001).
 *
 * PC port of RISC OS Conversion by Mark Lomas (31 Mar 2007)
 *
 */
#include <time.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include "bbcb.h"

#define NUM_BBCB_KEYS 122
#define KEYBOARD_BUFFER_SIZE 16
#define BACKGROUND 0
#define FOREGROUND 1

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TRACE_EXEC
int g_traceEnabled = 0;
#endif

unsigned char memory[0x10000];

unsigned char r6502_a;
unsigned char r6502_x;
unsigned char r6502_y;
unsigned char r6502_sp;
unsigned char r6502_ps;

unsigned int address;
unsigned char value1, value2;
unsigned char nlo, nhi;

unsigned char buffer1[12];
unsigned char buffer2[12];

/* Jump table used to map indirect jump addresses to function calls. */
struct JMPIndirectTable* jmpIndirectTable = 0;
size_t jmpIndirectTableSize = 0;

int quit, escape;
int escapeDetected = 0; /* Used in code to clear the escape condition. */

int lookupSDLCode[NUM_BBCB_KEYS]; /*  Converts (inkey^255) value into an SDL key code. */
int keysPressed[SDLK_LAST]; /* Table of keys pressed (based on SDL key codes) */
int keyboardBuffer[KEYBOARD_BUFFER_SIZE]; /* Buffers the last N keys pressed. */
int bufferStartOffset = 0;
int bufferEndOffset = 0;

Uint32 baseIntervalTime, elapsedInterval;
SDL_Surface* screen = 0;
SDL_Color logicalPalette[3][16];
SDL_Color physicalPalette[3][16];

/* Table of the number of additional arguments that follow a VDU code. */
static const unsigned char numVDUArgs[] = { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 5, 0, 0, 1, 9, 8, 5, 0, 0, 4, 4, 0, 2 };
unsigned int oswrch_count = 0;
unsigned int oswrch_vdu_code = 0;

/* Graphics cursor state*/
int vdu_gfx_cursor_x = 0;
int vdu_gfx_cursor_y = 0;
unsigned int vdu_gcol[2] = { 0, 1 }; /* background, foreground */
unsigned int vdu_gcol_action = 0;

/*
    Mode Graphics Colour Text
    0 640x256 Two colour display 80x32 text
    1 320x256 Four colour display 40x32 text
    2 160x256 16 colour display 20x32 text
    3 Two colour text only 80x25 text
    4 320x256 Two colour display 40x32 text
    5 160x256 Four colour display 20x32 text
    6 Two colour text only 40x25 text
    7 Teletext display 40x25 text
*/
unsigned int vdu_mode = 7;
static const char vdu_charactersPerRow[8] = { 80, 40, 20, 80, 40, 20, 40, 40 };
static const char vdu_charactersPerColumn[8] = { 32, 32, 32, 25, 32, 32, 25, 25 };
static const unsigned char vdu_coloursAvailable[8] = { 2, 4, 16, 2, 2, 4, 2, 16 };
static const unsigned char vdu_paletteIndexForMode[] = { 0, 1, 2, 0, 0, 1, 0, 2 }; /* Which palette to use for a given mode number. */
static const unsigned short vdu_xrez[8] = { 640, 320, 160, 640, 320, 160, 320, 320 };
static const unsigned int vdu_vram_address[8] = {0x3000, 0x3000, 0x3000, 0x4000, 0x5800, 0x5800, 0x6000, 0x7c00 };
static const unsigned int vdu_vram_size[8] = { 0x5000, 0x5000, 0x5000, 0x3E80, 0x2800, 0x2800, 0x1f40, 0x3e8 };
static const unsigned char vdu_default_logical_col[3][2] =
{
    /* Background, foreground */
    { 0, 1 }, /* Two colour mode */
    { 0, 3 }, /* Four colour mode */
    { 0, 7 }  /* 16 colour mode */
};

/* Text cursor state */
unsigned int vdu_text_cursor = 1;
int vdu_txt_cursor_x = 0;
int vdu_txt_cursor_y = 0;
unsigned int vdu_col_background = 0;
unsigned int vdu_col_foreground = 1;
unsigned int vdu_19_logical;


extern void scankeyboard(void);
extern void scankeyboardfrom16(void);
extern void initialiseIntervalTimer();
extern void readIntervalTimer(unsigned char* address);
extern void writeIntervalTimer(unsigned char* address);
extern void readline(void);
extern void inkey(void);
extern int inkeyToSDLK(char inkeyVal);
extern void clg(void);
extern char readmode2(int address);
extern void writemode2(int address, char value);
extern void print(unsigned char c);
extern int initLogicalPalette(void);
extern void initPhysicalPalette(void);

void oscli(void)
{
#if 0
    int off, c = 0;
    int olda, oldx, oldy;
    char buff[8];

    int strAddr = (r6502_y << 8) | r6502_x;

    // Hopper does a FX126 to clear the escape condition.
    if(memory[strAddr] == 'F' && memory[strAddr+1] == 'X')
    {
        off = strAddr + 2;
        while(memory[off] != 0xd)
        {
            buff[c] = memory[off];
            ++c;
            ++off;
        }
        buff[c] = 0;
        olda = r6502_a;
        oldx = r6502_x;
        oldy = r6502_y;
        r6502_a = atoi(buff);
        osbyte();
        r6502_a = olda;
        r6502_x = oldx;
        r6502_y = oldy;
    }
    else
    {
        assert(!"Unhandled OSCLI command.");
    }
#endif
    HACK_RTS
    return;
}

void osword(void)
{
/*  int address; */
  switch ((unsigned char)r6502_a)
  {
    case 0x00:
        /* read a line from the currently selected input*/
      readline();
      break;
    case 0x03:
      /*read interval timer*/
      /* The five byte clock value is written to the address contained in the X and Y registers */
      /* This clock is incremented every centisecond. */
      readIntervalTimer(memory+((r6502_y << 8) | r6502_x));
      break;
    case 0x04:
      /*write interval timer*/
      /* The five byte clock value is read from the address contained in the X and Y registers */
      /* This clock is incremented every centisecond. */
      writeIntervalTimer(memory+((r6502_y << 8) | r6502_x));
      break;
    case 0x07:
      /*SOUND command*/
      sound(&memory[(r6502_y << 8) | r6502_x]);
      break;
    case 0x08:
      /* Define an envelope */
      /* The ENVELOPE parameter block should contain 14 bytes of data which correspond to the 14 parameters described in the ENVELOPE command. */
      /* This call should be entered with the parameter block address contained in the X and Y registers. */
      envelope(&memory[(r6502_y << 8) | r6502_x]);
      break;
    default:
      /* Unhandled OSWORD call.*/
      assert(0);
      break;
  }
  HACK_RTS
}

void osbyte()
{
  switch ((unsigned char)r6502_a)
  {
    case 0x04:
        /* Disable cursor key editing and make them return normal ASCII values like the other keys. */
        break;
    case 0x0F:
        /* Flush input buffer */
        flushinputbuffer();
        break;
    case 0x79:
        /* Scan keyboard */
        scankeyboard();
        break;
    case 0x7A:
        /* Keyboard scan from 16 decimal */
        scankeyboardfrom16();
        break;
    case 0x7E:
        /* Acknowledge detection of an ESCAPE condition */
        if(escapeDetected)
            r6502_x = 255;
        else
            r6502_x = 0;
        break;
    case 0x80:
        /* Read ADC channel (ADVAL) */
        r6502_x = r6502_y = 0;
        break;
    case 0x81:
        inkey();
        break;
    case 0xAC:
        /* Read address of key translation table (low byte). */
        /* This should be constant for us. */
        r6502_x = 0x2B;
        r6502_y = 0xF0;
        break;
    case 0xC8:
        /* (*fx 200,X) - Read/write ESCAPE, BREAK effect i.e. disable ESCAPE and clear memory on BREAK.  */
        /* TODO: I don't believe that the A&X&Y reg values returned from this call are used by CH-EGG */
        break;
    case 0xD6:
        /* Read/write bell (CTRL G) duration. */
        /* TODO: I don't believe that the A&X&Y reg values returned from this call are used by CH-EGG */
        break;
    case 0xE5:
        /* Read/write status of ESCAPE key. Treat currently selected ESCAPE key as an ASCII code. */
        /* TODO: We don't need to do anything here because the return values are ignored by CH-EGG.*/
        break;
    default:
      /* Unhandled OSBYTE call.*/
      assert(0);
      break;
  }
  HACK_RTS
}

/*
    OSWRCH
    Call address &FFEE Indirected through &20E

    This routine outputs the character in the accumulator to the currently selected output stream(s).

    On exit:
        A, X and Y are preserved.
        C, N, V and Z are undefined.
    The interrupt status is preserved (though interrupts may be enabled during a call).

    VDU Codes
	Dec	hex	CTRL +	bytes	function
	0	0	@	0	Does nothing
	1	1	A	1	Send character to printer (expansion)
	2	2	B	0	Enable printer (expansion)
	3	3	C	0	Disable printer (expansion)
	4	4	D	0	Write text at text cursor
	5	5	E	0	Write text at graphics cursor
	6	6	F	0	Enable VDU drivers
	7	7	G	0	Make a short bleep (BEL)
	8	8	H	0	Move cursor back one character
	9	9	1	0	Move cursor forward one character
	10	A	J	0	Move cursor down one line
	11	B	K	0	Move cursor up one line
	12	C	L	0	Clear text area
	13	D	M	0	Carriage return
	14	E	N	0	Pagedmodeon
	15	F	0	0	Paged mode off
	16	10	P	0	Clear graphics area
	17	11	0	1	Define text colour
	18	12	R	2	Define graphics colour
	19	13	5	5	Define logical colour
	20	14	T	0	Restore default logical colours
	21	15	U	0	Disable VDU drivers/delete current line
	22	16	V	1	Select screen MODE
	23	17	W	9	Re-program display character
	24	18	X	8	Define graphics window
	25	19	Y	5	PLOT K,X,Y
	26	lA	Z	0	Restore default windows
	27	lB	[	0	Reserved
	28	1C	,	4	Define text window
	29	1D	-	4	Define graphics origin
	30	lE		0	Home text cursor to top left of window
	31	1F	/	2	Move text cursor to X,y.
	32-126				Complete set of ASCII characters
	127 7F	DEL		0	Backspace and delete
	128-223				Normally undefined (define using *FX20)
	224-255				User defined characters
*/

void oswrch()
{
    if(oswrch_count>0)
    {
        switch(oswrch_vdu_code)
        {
            case 17:
                if(r6502_a < 128)
                    vdu_col_foreground = r6502_a % vdu_coloursAvailable[vdu_mode];
                else
                    vdu_col_background = (r6502_a - 128) % vdu_coloursAvailable[vdu_mode];
                break;
            case 18:
                switch(oswrch_count)
                {
                    case 1:
                        if(r6502_a < 128)
                            vdu_gcol[FOREGROUND] = r6502_a % vdu_coloursAvailable[vdu_mode];
                        else
                            vdu_gcol[BACKGROUND] = (r6502_a - 128) % vdu_coloursAvailable[vdu_mode];
                        break;
                    case 2:
                        vdu_gcol_action = r6502_a;
                        break;
                }
                break;
            case 19:
                // TODO: Support foreground and background text colour ?
                switch(oswrch_count)
                {
                    case 5: /* logical colour */
                        vdu_19_logical = r6502_a % vdu_coloursAvailable[vdu_mode];
                        break;
                    case 4: /* actual colour */
                        setLogicalColour(vdu_19_logical, r6502_a % vdu_coloursAvailable[vdu_mode]);
                        break;
                };
                break;
            case 22: /* Change mode */
                vdu_mode = r6502_a;
                initLogicalPalette();
                clg();
                vdu_txt_cursor_x = vdu_txt_cursor_y = 0;
                vdu_text_cursor = !(vdu_mode == 2 || vdu_mode == 5);
                break;
            case 23: /* User defined character */
                /*{
                FILE*fp = fopen("c:\\foo.txt", "a");
                switch(oswrch_count)
                {
                    case 9:
                        fprintf(fp, "%d,", r6502_a);
                        break;
                    case 1:
                        fprintf(fp, "%d\n", r6502_a);
                        break;
                    default:
                        fprintf(fp, "%d,", r6502_a);
                        break;
                };
                fclose(fp);
                }*/
                break;
            case 25: /* Set graphics cursor */
                switch(oswrch_count)
                {
                    case 5:
                        assert(r6502_a == 4); // MOVE
                        vdu_gfx_cursor_x = vdu_gfx_cursor_y = 0;
                        break;
                    case 4:
                        vdu_gfx_cursor_x |= (unsigned char)r6502_a;
                        break;
                    case 3:
                        vdu_gfx_cursor_x |= ((unsigned char)r6502_a)<<8;
                        break;
                    case 2:
                        vdu_gfx_cursor_y |= (unsigned char)r6502_a;
                        break;
                    case 1:
                        vdu_gfx_cursor_y |= ((unsigned char)r6502_a)<<8;
                        break;
                };
                break;
            case 31: /* TAB() function */
                if(oswrch_count == 2)
                    vdu_txt_cursor_x = r6502_a;
                else
                    vdu_txt_cursor_y = r6502_a;
                break;
            default:
                break;
        };
        --oswrch_count;
    }
    else
    {
        if(r6502_a < 32)
        {
            oswrch_vdu_code = r6502_a;
            oswrch_count = numVDUArgs[r6502_a];

            switch(r6502_a)
            {
                case 0: /* Does nothing */
                    /* Sometimes terminates a string e.g. in high score table in hopper and gets printed (accidentally?) */
                    break;
                case 4: /* Write text at text cursor */
                    vdu_text_cursor = 1;
                    break;
                case 5: /* Write text at graphics cursor */
                    vdu_text_cursor = 0;
                    break;
                case 8: /* Move cursor back one character */
                    --vdu_txt_cursor_x;
                    if(vdu_txt_cursor_x < 0)
                    {
                        vdu_txt_cursor_x = vdu_charactersPerRow[vdu_mode] - 1;
                        --vdu_txt_cursor_y;
                        if(vdu_txt_cursor_y < 0)
                        {
                            vdu_txt_cursor_y = 0; /* Would normally scroll the screen */
                        }
                    }
                    break;
                case 9: /* Move cursor forward one character */
                    ++vdu_txt_cursor_x;
                    if(vdu_txt_cursor_x >= vdu_charactersPerRow[vdu_mode])
                    {
                        vdu_txt_cursor_x = 0;
                        ++vdu_txt_cursor_y;
                        if(vdu_txt_cursor_y == vdu_charactersPerColumn[vdu_mode])
                        {
                            vdu_txt_cursor_y = vdu_charactersPerColumn[vdu_mode] - 1; /* Would normally scroll the screen */
                        }
                    }
                    break;
                case 10: /* Move cursor down one line */
                    ++vdu_txt_cursor_y;
                    if(vdu_txt_cursor_y == vdu_charactersPerColumn[vdu_mode])
                    {
                        vdu_txt_cursor_y = vdu_charactersPerColumn[vdu_mode] - 1; /* Would normally scroll the screen */
                    }
                    break;
                case 11: /* Move cursor up one line */
                    --vdu_txt_cursor_y;
                    if(vdu_txt_cursor_y < 0)
                    {
                        vdu_txt_cursor_y = 0; /* Would normally scroll the screen */
                    }
                    break;
                case 12: /* Clear text area */
                    assert(!"Not implemented cls yet!"); /*cls();*/
                    vdu_txt_cursor_x = vdu_txt_cursor_y = 0;
                    break;
                case 13:
                    vdu_txt_cursor_x = 0;
                    break;
                case 16: /* Clear graphics screen */
                    clg();
                    break;
                case 17:
                case 18:
                case 19: /* Define logical colour */
                    break;
                case 20: /* Restore default logical colours */
                    initLogicalPalette();
                    break;
                case 22: /* Change mode */
                    break;
                case 23: /* User defined character */
                    {
                    //FILE*fp = fopen("c:\\foo.txt", "a");
                    //fprintf(fp, "VDU23, ");
                    //fclose(fp);
                    }
                    break;
                case 25: /* PLOT k,x,y*/
                case 30:
                    /* moves the text cursor to the top left of the text */
                    vdu_txt_cursor_x = vdu_txt_cursor_y = 0;
                    break;
                case 31: /* TAB() function */
                case 35:
                    break;
                default:
                    //assert(!"Unhandled VDU code");
                    break;
            }
        }
        else if(r6502_a < 128)
        {
            print(r6502_a);
        }
        else
        {
            // Teletext
            //assert(!"Teletext VDU codes are not supported.");
        }
    }

    /* Try to effect status regs in similar way to orginary oswrch. */
    if(r6502_a == 22)
        r6502_ps = 0;
    else
        r6502_ps &= ~1;
    HACK_RTS
}

void r6502SetJMPIndirectionTable(struct JMPIndirectTable* table, size_t rows)
{
    jmpIndirectTable = table;
    jmpIndirectTableSize = rows;
}

void r6502jmpindirect(int n)
{
    unsigned int i;
    int addr = ((memory[n+1] << 8) | memory[n]);

    // Nasty linear search at the moment.
    for(i = 0; i < jmpIndirectTableSize; ++i)
    {
        if(jmpIndirectTable[i].addr == addr)
        {
            jmpIndirectTable[i].func();
            break;
        }
    }

    if(i == jmpIndirectTableSize)
    {
        //printf("JMP_INDIRECT - 0x%04x", memory[n]);
        //assert(!"JMP_INDIRECT not handled.");
    }
}

void r6502adc(int n)
{
  int nval;
  int nlob, nhib;

  if (!(r6502_ps & DFLAG))
  {
    nval = r6502_a+n;
    if (r6502_ps & CFLAG)
      nval++;
    if (nval >= 0x100)
      SETFLAG(CFLAG)
    else
      CLEARFLAG(CFLAG)
    if (!((r6502_a ^ n) & 0x80) AND ((r6502_a ^ nval) & 0x80))
      SETFLAG(VFLAG)
    else
      CLEARFLAG(VFLAG)
    nval &= 0xFF;
    r6502_a = nval;
    SETNFLAG(r6502_a)
    SETZFLAG(r6502_a)
  }
  else
  {
    nval = r6502_a+n;
    if (r6502_ps & CFLAG)
      nval++;
    SETZFLAG(nval)
    nlob = (r6502_a & 0x0F)+(n & 0x0F);
    if (r6502_ps & CFLAG)
      nlob++;
    if (nlob > 9)
      nlob += 6;
    nhib = (r6502_a >> 4)+(n >> 4);
    if (nlob > 0x0F)
      nhib++;
    nval = (nhib << 4) + (nlob & 0x0F);
    SETNFLAG(nval)
    if ((((nhib << 4) ^ r6502_a) & 0x80) AND !((r6502_a ^ n) & 0x80))
      SETFLAG(VFLAG)
    else
      CLEARFLAG(VFLAG)
    if (nhib > 9)
      nhib +=6;
    if (nhib > 0x0F)
      SETFLAG(CFLAG)
    else
      CLEARFLAG(CFLAG)
    r6502_a = (nhib << 4) + (nlob & 0x0F);
  }
}

void r6502jsr(int r6502_pc) /*,int address)*/
{
  r6502_pc--;
  STACK_PUSH((r6502_pc & 0xFF00)>>8);
  STACK_PUSH(r6502_pc & 0x00FF);
  /*r6502_pc = n;*/
  /*goto address;*/
  /*setjmp(jmpbuffer);*/
}

/*void r6502rts(void)
{
  int r6502_pc;
  STACK_POP(nlo);
  STACK_POP(nhi);
  r6502_pc = (nhi<<8)+nlo;
  r6502_pc++;*/
 /* longjump(jmpbuffer,value);*/
/*}*/

void r6502sbc(int n)
{
  int nval;
  int nlob, nhib;
  if (!(r6502_ps & DFLAG))
  {
    nval = r6502_a-n;
    if (!(r6502_ps & CFLAG))
      nval--;
    if (nval >= 0)
      SETFLAG(CFLAG)
    else
      CLEARFLAG(CFLAG)
    if (((r6502_a ^ nval) & 0x80) AND ((r6502_a ^ n) & 0x80))
      SETFLAG(VFLAG)
    else
      CLEARFLAG(VFLAG)
    nval &= 0xFF;
    SETNFLAG(nval)
    SETZFLAG(nval)
    r6502_a = nval;
  }
  else
  {
    nval = r6502_a-n;
    if (!(r6502_ps & CFLAG))
      nval--;
    nlob = (r6502_a & 0x0F)-(n & 0x0F);
    if (!(r6502_ps & CFLAG))
      nlob--;
    nhib = (r6502_a >> 4)-(n >> 4);
    if (nlob & 0x10)
    {
      nlob -= 6;
      nlob &= 0x0F;
      nhib--;
    }
    if (nhib & 0x10)
      nhib -= 6;
    if (nval >= 0)
      SETFLAG(CFLAG)
    else
      CLEARFLAG(CFLAG)
    if (((r6502_a ^ nval) & 0x80) AND ((r6502_a ^ n) & 0x80))
      SETFLAG(VFLAG)
    else
      CLEARFLAG(VFLAG)
    nval &= 0xFF;
    SETNFLAG(nval)
    SETZFLAG(nval)
    r6502_a = (nhib << 4) + (nlob & 0x0F);
  }
}

/*
 * Set the pixel at (x, y) to the given value
 * NOTE: The surface must be locked before calling this!
 */
void putpixel(int x, int y, Uint32 pixel)
{
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)screen->pixels + y * screen->pitch + x;
    *p = pixel;
}

void writemode2(int address, char value)
{
    int row, chunk, nx, ny;
    unsigned char pixel1, pixel2;

    assert(address>=0x3000 && address<0x8000);

    /* Make screen address zero relative */
    address -= 0x3000;
    if (address >= 0x5000)
        address &= 0x4FFF;

    /* Mode2 screen is broken into rectangular chunks of 640 bytes (8 rows of 80 columns). */
    /* Each cell contains two pixels (4bit). */

    /* mask out all but the lower three bits to get the chunk relative row offset. */
    row = (address & 7);
    chunk = address / 640;
    ny = (chunk<<3) + row;
    nx = ((address%640)>>3);

    /* Bits of both pixels are interleaved on the BBC */
    pixel1 = ((value&128)>>4) | ((value&32)>>3) | ((value&8)>>2) | ((value & 2)>>1);
    pixel2 = ((value&64)>>3) | ((value&16)>>2) | ((value&4)>>1) | (value & 1);

    nx*=8;
    ny*=2;
    assert(nx < screen->w);
    assert(ny < screen->h);
    putpixel(nx, ny, pixel1);
    putpixel(nx, ny+1, pixel1);
    putpixel(nx + 1, ny, pixel1);
    putpixel(nx + 1, ny+1, pixel1);
    putpixel(nx + 2, ny, pixel1);
    putpixel(nx + 2, ny+1, pixel1);
    putpixel(nx + 3, ny, pixel1);
    putpixel(nx + 3, ny+1, pixel1);
    putpixel(nx + 4, ny, pixel2);
    putpixel(nx + 4, ny+1, pixel2);
    putpixel(nx + 5, ny, pixel2);
    putpixel(nx + 5, ny+1, pixel2);
    putpixel(nx + 6, ny, pixel2);
    putpixel(nx + 6, ny+1, pixel2);
    putpixel(nx + 7, ny, pixel2);
    putpixel(nx + 7, ny+1, pixel2);
    //SDL_UpdateRect(screen, nx, ny, 8, 2);
}

/* Clear to background colour. */
void clg(void)
{
    /* Initialise bit field for background fill colour. */
    unsigned char val = 0;
    switch(vdu_paletteIndexForMode[vdu_mode])
    {
#if 0 /* Never happens for ch-egg */
        case 0: /* Two colours (1 bit per pixel) */
            val = vdu_gcol[BACKGROUND] ? 0xff : 0x00;
            break;
        case 1: /* Four colours (2 bits per pixel) */
            val = (vdu_gcol[BACKGROUND]<<6) |
                (vdu_gcol[BACKGROUND]<<4) |
                (vdu_gcol[BACKGROUND]<<2) |
                vdu_gcol[BACKGROUND];
            break;
#endif
        case 2: /* 16 colours (4 bits per pixel) */
            val = (vdu_gcol[BACKGROUND]<<4) | vdu_gcol[BACKGROUND];
            break;
    };

    /* BBC video memory */
    memset(memory + vdu_vram_address[vdu_mode], val, vdu_vram_size[vdu_mode]);

    /* PC video memory */
    memset((Uint8 *)screen->pixels, vdu_gcol[BACKGROUND], screen->pitch*screen->h);
    if ( SDL_MUSTLOCK(screen) )
    {
        SDL_UnlockSurface(screen);
    }

	SDL_UpdateRect(screen, 0, 0, screen->w, screen->h);

    if ( SDL_MUSTLOCK(screen) )
    {
        if ( SDL_LockSurface(screen) < 0 )
		{
            return;
        }
    }
}


void plotChar(unsigned int nx, unsigned int ny,
              unsigned char c)
{
    unsigned int x, y, pixel1, pixel2, xpos, ypos, byte;
    assert(nx >= 0 && nx < (unsigned int)screen->w);
    assert(ny >= 0 && ny < (unsigned int)screen->h);

    /* Font map contains 96 characters arranged into 96 rows of 8 columns. The first character is ASCII code 32 */
    /* Each character is 8x8 pixels in size.*/
	c -= 32;
    assert(c >= 0 && c < (128-32));

    for(y = 0; y < 8; ++y)
    {
        byte = font[c*8 + y];
        for(x = 0; x < 4; ++x)
        {
            pixel1 = vdu_gcol[(byte>>(x<<1)) & 1];
            pixel2 = vdu_gcol[(byte>>((x<<1) + 1)) & 1];

            /* Stretch to fit horizontal resolution of mode onto SDL screen surface. */
            switch(vdu_xrez[vdu_mode])
            {
            case 160: /* Scale 160x256 up to 640x512 (4*x, 2*y)*/
                xpos = nx + (x*4) * 2; /* We write two pixels at a time so multiply by 2 again. */
                ypos = ny + (y*2);
                putpixel(xpos, ypos, pixel1);
                putpixel(xpos, ypos+1, pixel1);
                putpixel(xpos+1, ypos, pixel1);
                putpixel(xpos+1, ypos+1, pixel1);
                putpixel(xpos+2, ypos, pixel1);
                putpixel(xpos+2, ypos+1, pixel1);
                putpixel(xpos+3, ypos, pixel1);
                putpixel(xpos+3, ypos+1, pixel1);
                putpixel(xpos+4, ypos, pixel2);
                putpixel(xpos+4, ypos+1, pixel2);
                putpixel(xpos+5, ypos, pixel2);
                putpixel(xpos+5, ypos+1, pixel2);
                putpixel(xpos+6, ypos, pixel2);
                putpixel(xpos+6, ypos+1, pixel2);
                putpixel(xpos+7, ypos, pixel2);
                putpixel(xpos+7, ypos+1, pixel2);
                //SDL_UpdateRect(screen, xpos, ypos, 8, 2);
                break;
#if 0 /* Never happens for ch-egg */
            case 320: /* Scale 320x256 up to 640x512 (2*x, 2*y)*/
                xpos = nx + (x*2) * 2; /* We write two pixels at a time so multiply by 2 again. */
                ypos = ny + (y*2);
                putpixel(xpos, ypos, pixel1);
                putpixel(xpos, ypos+1, pixel1);
                putpixel(xpos+1, ypos, pixel1);
                putpixel(xpos+1, ypos+1, pixel1);
                putpixel(xpos+2, ypos, pixel2);
                putpixel(xpos+2, ypos+1, pixel2);
                putpixel(xpos+3, ypos, pixel2);
                putpixel(xpos+3, ypos+1, pixel2);
                SDL_UpdateRect(screen, xpos, ypos, 4, 2);
                break;
            case 640: /* Scale 640x256 up to 640x512 (1*x, 2*y). */
                xpos = nx + x * 2; /* We write two pixels at a time so multiply by 2. */
                ypos = ny + (y*2);
                putpixel(xpos, ypos, pixel1);
                putpixel(xpos, ypos+1, pixel1);
                putpixel(xpos+1, ypos, pixel2);
                putpixel(xpos+1, ypos+1, pixel2);
                SDL_UpdateRect(screen, xpos, ypos, 1, 2);
                break;
#endif
            };
        }
    }
}

void printGfx(unsigned char c)
{
    /*
       Graphics coordinates are defined from 0..1279 in x and 0..1023 in y
       and the origin is the bottom left of the screen.
       Scale to coordinates fit on 640x512 framebuffer with a graphics origin in the top left
    */
    unsigned int nx = vdu_gfx_cursor_x >> 1;
    unsigned int ny = vdu_gfx_cursor_y >> 1;
    ny = 511 - ny;

    plotChar(nx, ny, c);

    /* Character width is different in different modes. */
    vdu_gfx_cursor_x += (8*(640/vdu_xrez[vdu_mode])) << 1; /* Advance graphics cursor by character width */
}

#if 0 /* Never happens for ch-egg */
void printText(unsigned char c)
{
    /*
       The text origin is the top left of the screen.
       Convert text coordinates into framebuffer coordinates.
    */
    int nx = vdu_txt_cursor_x * (8*(640/vdu_xrez[vdu_mode]));
    int ny = (vdu_txt_cursor_y * 8) << 1;

    plotChar(nx, ny, c);

    /* Advance text cursor forward by one. */
    ++vdu_txt_cursor_x;
}
#endif


void print(unsigned char c)
{
#if 0 /* Never happens for ch-egg */

	if(vdu_text_cursor)
        printText(c);
    else
#else
        printGfx(c);
#endif
}

char r6502read(int address)
{
    address &= 0xFFFF;
    return memory[address];
}

void r6502write(int address, char value)
{
  /*if (address | 0x10000)*/
    address &= 0xFFFF;
  switch (address & 0xF000)
  {
    case 0x0000:
    case 0x1000:
    case 0x2000:
      /*RAM*/
      memory[address] = value;
      break;
    case 0x3000:
    case 0x4000:
    case 0x5000:
    case 0x6000:
    case 0x7000:
      /*RAM*/
      memory[address] = value;
      if(address >= (int)vdu_vram_address[vdu_mode] &&
         address < (int)(vdu_vram_address[vdu_mode]+vdu_vram_size[vdu_mode]))
      {
        writemode2(address,value);
      }
      break;
    case 0x8000:
    case 0x9000:
    case 0xA000:
    case 0xB000:
      /*ROM*/
      break;
    case 0xC000:
    case 0xD000:
    case 0xE000:
      /*ROM*/
      break;
    case 0xF000:
      /*printf("F000 &%X,&%X\n",address,value);*/
      break;
  }
}

void initialiseIntervalTimer()
{
    baseIntervalTime = SDL_GetTicks()/10; /* Convert from milliseconds to centisecond. */
    elapsedInterval = 0;
}

void readIntervalTimer(unsigned char* address)
{
  /* BBC timer is 5 bytes, we will get by with 4. */
  Uint32 currentTicks = SDL_GetTicks()/10; /* Convert from milliseconds to centisecond. */
  Uint32 delta = currentTicks - baseIntervalTime;
  //elapsedInterval += delta;
  //*((Uint32*)address) = delta;
  *((Uint32*)address) = elapsedInterval + delta;
}

void writeIntervalTimer(unsigned char* address)
{
    baseIntervalTime = SDL_GetTicks()/10; /* Convert from milliseconds to centisecond. */
    elapsedInterval = *((Uint32*)address);
}

/*
OSBYTE 129 (Called by Basic's INKEY)
Entry: time         Exit: result
(In Basic: result=INKEY time)

IF time >= 0 AND time <= &7FFF { 0 to 32767 } THEN
  waits for a maximum of time centiseconds for a character from the
    current input stream returns the next character, or -1 (&FFFF) if no
    character returned in the passed time limit.

IF time < 0 AND time >= &FF80 { -1 to -128 } THEN
  scan and return state of specified key, see table, returns -1 (&FFFF) if
    key pressed, 0 (&0000) otherwise.

IF time < &FF80 AND time >= &FF01 { -129 TO -255 } THEN
  scan range of keys starting at key (time EOF &7F) and return internal
    key number in low byte of first pressed key found, or &FF in low byte
    if no key pressed.  High byte of returned value should be ignored.

IF time = &FF00 { -256 } THEN
  returns a value relating to the variant of the host system being run on
    in the low byte.  The high byte of the returned value should be
    ignored.

IF time < &FF00 { -256 } THEN
  call is undefined, undefined value returned.


Internal Key Numbers
      x0   x1  x2 x3  x4  x5  x6  x7  x8  x9   xA  xB   xC   xD   xE   xF
&0x           Alt lSh lCt lAt rSh rCt rAl mL   mM  mR
&0x SHIFT CTRL l8 l7  l6  l5  l4  l3  l2  l1   jF2 jF1  jL   jR   jD   jU
&1x   Q    3   4  5   f4  8   f7  -   ^  left  k6  k7  f11  f12  f10  ScLk
&2x  f0    W   E  T   7   I   9   0   _  down  k8  k9 Break  ~    `    BS
&3x   1    2   D  R   6   U   O   P   [   up   k+  k-  kRet Ins  Home PgUp
&4x Caps   A   X  F   Y   J   K   @   :   Ret  k/ kDel  k.  NmLk PgDn   "
&5x ShLk   S   C  G   H   N   L   ;   ]   Del  k#  k*   k,
&6x Tab    Z  Spc V   B   M   ,   .   /  Copy  k0  k1   k3
&7x Esc    f1  f2 f3  f5  f6  f8  f9  \  right k4  k5   k2

Notes:
left,right,down,up -> cursor keys
kx -> keypad keys
lx -> keyboard links on BBC keyboard
lSh,lCt,lAt -> left Shift,Ctrl,Alt keys on Arc/RiscOS
rSh,rCt,rAt -> right Shift,Ctrl,Alt keys on Arc/RiscOS
mx -> mouse buttons
jx -> joystick buttons (rarely implemented)
Break -> soft Break key on Arc/RiscOS, not the hardware BREAK/RESET key
Negative INKEY numbers formed by EORing with &FF, so the SHIFT is INKEY-1.
*/

/*
    hex.  dec.  key
    &00   0     SHIFT
    &01   1     CTRL
    &02   2     bit 0
    &03   3     bit 1
    &04   4     bit 2
    &05   5     bit 3
    &06   6     bit 4
    &07   7     bit 5
    &08   8     bit 6
    &09   9     bit 7
    &l0   16    Q
    &11   17    3
    &12   18    4
    &13   19    5
    &14   20    f4
    &15   21    8
    &16   22    f7
    &17   23    -
    &18   24    ^
    &19   25    left cursor
    &20   32    f0
    &21   33    W
    &22   34    E
    &23   35    T
    &24   36    7
    &25   37    9
    &26   38    I
    &27   39    0
    &28   40    _
    &29   41    down cursor
    &30   48    1
    &31   49    2
    &32   50    D
    &33   51    R
    &34   52    6
    &35   53    U
    &36   54    0
    &37   55    P
    &38   56    [
    &39   57    up cursor
    &40   64    CAPS LOCK
    &41   65    A
    &42   66    X
    &43   67    F
    &44   68    Y
    &45   69    J
    &46   70    K
    &47   71    @
    &48   72    :
    &49   73    RETURN
    &50   80    SHIFT LOCK
    &51   81    S
    &52   82    C
    &53   83    G
    &54   84    H
    &55   85    N
    &56   86    L
    &57   87    ;
    &58   88    ]
    &59   89    DELETE
    &60   96    TAB
    &61   97    Z
    &62   98    SPACE
    &63   99    V
    &64   100   B
    &65   101   M
    &66   102   ,
    &67   103   .
    &68   104   /
    &69   105   COPY
    &70   112   ESCAPE
    &71   113   fi
    &72   114   f2
    &73   115   f3
    &74   116   f4
    &75   117   f6
    &76   118   f8
    &77   119   f9
    &78   120   \
    &79   121   right cursor

   To convert these internal key numbers to the INKEY numbers they should be EOR (Exclusive ORed) with &FF (255).
*/

/* Translate from BBC B INKEY to SDL key codes. */
int inkeyToSDLK(char inkeyVal)
{
    /* EOR with 0xff to turn negative inkey value into internal key code.*/
    char key = (inkeyVal ^ 0xff);
    assert(key >= 0);
    assert(key < NUM_BBCB_KEYS);
    return lookupSDLCode[(int)key];
}

/* This table allows conversion from inkey values to SDL values */
void initInkeyToSDLKTable(void)
{
    int i;
    /* Not all entires are set during this initialisation, so set the rest to a default key (SHIFT). */
    for(i = 0; i < NUM_BBCB_KEYS; ++i)
        lookupSDLCode[i] = -1;

    /*
        These keys are examples that have two separate codes in SDLK but are the same physical key.
        SDLK_COMMA, SDLK_LESS
        SDLK_PERIOD, SDLK_GREATER
        SDLK_SLASH, SDLK_QUESTION
        SDLK_QUOTE, SDLK_AT
        SDLK_LSHIFT, SDLK_RSHIFT
        SDLK_LCTRL, SDLK_RCTRL
    */

    lookupSDLCode[0  ] = SDLK_LSHIFT;  /* SHIFT       */
    lookupSDLCode[1  ] = SDLK_LCTRL;  /* CTRL        */
    lookupSDLCode[16 ] = SDLK_q;  /* Q           */
    lookupSDLCode[17 ] = SDLK_3;  /* 3           */
    lookupSDLCode[18 ] = SDLK_4;  /* 4           */
    lookupSDLCode[19 ] = SDLK_5;  /* 5           */
    lookupSDLCode[20 ] = SDLK_F4;  /* f4          */
    lookupSDLCode[21 ] = SDLK_8;  /* 8           */
    lookupSDLCode[22 ] = SDLK_F7;  /* f7          */
    lookupSDLCode[23 ] = SDLK_MINUS;  /* -           */
    lookupSDLCode[24 ] = SDLK_HASH;  /* ^  I have mapped this to #   */
    lookupSDLCode[25 ] = SDLK_LEFT;  /* left cursor */
    lookupSDLCode[32 ] = SDLK_F10;  /* f0          */
    lookupSDLCode[33 ] = SDLK_w;  /* W           */
    lookupSDLCode[34 ] = SDLK_e;  /* E           */
    lookupSDLCode[35 ] = SDLK_t;  /* T           */
    lookupSDLCode[36 ] = SDLK_7;  /* 7           */
    lookupSDLCode[37 ] = SDLK_9;  /* 9           */
    lookupSDLCode[38 ] = SDLK_i;  /* I           */
    lookupSDLCode[39 ] = SDLK_0;  /* 0           */
    lookupSDLCode[40 ] = SDLK_UNDERSCORE;  /* _           */
    lookupSDLCode[41 ] = SDLK_DOWN;  /* down cursor */
    lookupSDLCode[48 ] = SDLK_1;  /* 1           */
    lookupSDLCode[49 ] = SDLK_2;  /* 2           */
    lookupSDLCode[50 ] = SDLK_d;  /* D           */
    lookupSDLCode[51 ] = SDLK_r;  /* R           */
    lookupSDLCode[52 ] = SDLK_6;  /* 6           */
    lookupSDLCode[53 ] = SDLK_u;  /* U           */
    lookupSDLCode[54 ] = SDLK_o;  /* O           */
    lookupSDLCode[55 ] = SDLK_p;  /* P           */
    lookupSDLCode[56 ] = SDLK_LEFTBRACKET;  /* [           */
    lookupSDLCode[57 ] = SDLK_UP;  /* up cursor   */
    lookupSDLCode[64 ] = SDLK_CAPSLOCK;  /* CAPS LOCK*/
    lookupSDLCode[65 ] = SDLK_a;  /* A*/
    lookupSDLCode[66 ] = SDLK_x;  /* X*/
    lookupSDLCode[67 ] = SDLK_f;  /* F*/
    lookupSDLCode[68 ] = SDLK_y;  /* Y*/
    lookupSDLCode[69 ] = SDLK_j;  /* J*/
    lookupSDLCode[70 ] = SDLK_k;  /* K*/
    lookupSDLCode[71 ] = SDLK_QUOTE;  /* @*/
    lookupSDLCode[72 ] = SDLK_COLON;  /* :  same key as semicolon */
    lookupSDLCode[73 ] = SDLK_RETURN;  /* RETURN*/
    //lookupSDLCode[80 ] = ;  /* SHIFT LOCK*/
    lookupSDLCode[81 ] = SDLK_s;  /* S*/
    lookupSDLCode[82 ] = SDLK_c;  /* C*/
    lookupSDLCode[83 ] = SDLK_g;  /* G*/
    lookupSDLCode[84 ] = SDLK_h;  /* H*/
    lookupSDLCode[85 ] = SDLK_n;  /* N*/
    lookupSDLCode[86 ] = SDLK_l;  /* L*/
    lookupSDLCode[87 ] = SDLK_SEMICOLON;  /* ;*/
    lookupSDLCode[88 ] = SDLK_RIGHTBRACKET;  /* ]*/
    lookupSDLCode[89 ] = SDLK_BACKSPACE;  /* DELETE*/
    lookupSDLCode[96 ] = SDLK_TAB;  /* TAB*/
    lookupSDLCode[97 ] = SDLK_z;  /* Z*/
    lookupSDLCode[98 ] = SDLK_SPACE;  /* SPACE*/
    lookupSDLCode[99 ] = SDLK_v;  /* V*/
    lookupSDLCode[100] = SDLK_b;  /* B*/
    lookupSDLCode[101] = SDLK_m;  /* M*/
    lookupSDLCode[102] = SDLK_COMMA;  /* ,*/
    lookupSDLCode[103] = SDLK_PERIOD;  /* .*/
    lookupSDLCode[104] = SDLK_SLASH;  /* / */
    lookupSDLCode[105] = SDLK_END;  /* COPY*/
    lookupSDLCode[112] = SDLK_ESCAPE;  /* ESCAPE*/
    lookupSDLCode[113] = SDLK_F1;  /* f1*/
    lookupSDLCode[114] = SDLK_F2;  /* f2*/
    lookupSDLCode[115] = SDLK_F3;  /* f3*/
    lookupSDLCode[116] = SDLK_F4;  /* f4*/
    lookupSDLCode[117] = SDLK_F6;  /* f6*/
    lookupSDLCode[118] = SDLK_F8;  /* f8*/
    lookupSDLCode[119] = SDLK_F9;  /* f9*/
    lookupSDLCode[120] = SDLK_BACKSLASH;  /* \ */
    lookupSDLCode[121] = SDLK_RIGHT;  /* right cursor*/
}

void initialiseKeyboardBuffer(void)
{
  memset(keyboardBuffer, 0, sizeof(keyboardBuffer));
  memset(keysPressed, 0, sizeof(keysPressed));
  flushinputbuffer();
  initInkeyToSDLKTable();
}

void flushinputbuffer(void)
{
  bufferStartOffset = 0;
  bufferEndOffset = 0;
  memset(keysPressed, 0, sizeof(keysPressed));
}

/* Read key with time limit (INKEY) - OS_Byte 81 */
void inkey(void)
{
//    int time;

    if((unsigned char)r6502_y == 0xff)
    {
        if(r6502_x == 0)
        {
            /* Require the OS type */
            r6502_x = 0xff; /*BBC Micro OS1.00/1.20*/
        }
        else
        {
            /* r6502_x is the negative inkey value. */
            /* Scan keyboard for key press */
            if(keysPressed[inkeyToSDLK(r6502_x)])
            {
                r6502_x = r6502_y = 0xff;
                SETFLAG(CFLAG);
            }
            else
            {
                r6502_x = r6502_y = 0x00;
                SETFLAG(CFLAG);
            }
        }
    }
    else
    {
        /* Read key with time limit */
        /* X & Y specify the time limit in centiseconds */
        /*
            The maximum time delay is passed to the subroutine in X and Y. The delay is
            measured in hundredths of a second and Y contains the most significant byte and
            X the least significant byte. The maximum delay is &7FFF hundredths of a
            second which is about five and a half minutes.
            On exit, Y=0 if a character was detected within the time limit. In this case X
            contains the character. Y=&1B indicates that ESCAPE was pressed. This must
            be acknowledged with *FX126. Y=&FF indicates a time-out.

            IF time >= 0 AND time <= &7FFF { 0 to 32767 } THEN
            waits for a maximum of time centiseconds for a character from the
            current input stream returns the next character, or -1 (&FFFF) if no
            character returned in the passed time limit.
        */
#if 0
        time = clock() + ((CLOCKS_PER_SEC/100) * ((r6502_y << 8) | r6502_x));
        do
        {
            /* This must do what PollMessages() does */
            SDL_Event event ;

            /*message pump*/
            /*look for an event*/
            if ( SDL_PollEvent ( &event ) )
            {
                /*an event was found*/
                switch(event.type)
                {
                    case SDL_QUIT:
                        quit = 1;
						finalise();
                        exit(0);
                        break;

                    case SDL_KEYDOWN:
                        if (event.key.keysym.sym == SDLK_ESCAPE)
                        {
                            r6502_y = 0x1b;
                            escapeDetected = 1;
                        }
                        else
                        {
                            /* Translate into ASCII value */
                            if(event.key.keysym.unicode < 0x80 && event.key.keysym.unicode > 0 )
                            {
                                r6502_y = 0;
                                r6502_x = (char)event.key.keysym.unicode;
                            }
                        }
                        return;
                }
            }
        } while(clock() < time);
#endif
        r6502_y = 0xff; /* no key was pressed */
    }
}

/* OS_BYTE &79 */
void scankeyboard(void)
{
    /*
    Keyboard scan
    The keyboard is scanned in ascending numerical order.
    This call returns information about the first pressed key encountered during the scan.
    Other keys may also be pressed and a further call or calls will be needed to complete the entire keyboard scan.
    Entry parameters: X determines the key to be detected and also determines the range of keys to be scanned.
    Key numbers refer to internal key numbers in the table above (see OSBYTE &78).
    To scan a particular key: X=key number EOR &80 on exit X<0 if the key is pressed
    To scan the matrix starting from a particular key number:
    X=key number
    On exit X=key number of any key pressed or &FF if no key pressed
    On exit:
    A is preserved
    X contains key value (see above)
    Y and C are undefined
    */

    /*
    Alternative docs:
    Keyboard Scan
        Y=0 X=internal keynumber EOR&80 for single key check
        On exit X<0 if the key was being pressed
        Y=0 X=lowest internal keynumber to start at (for a range of keys)
        On exit X=earliest keynumber being pressed or &FF for none
    */

    char key;

    /* negative number indicates we are checking for a single key. */
    if(r6502_x & 0x80)
    {
        /* convert number to inkey */
        key = (r6502_x ^ 0x80) ^ 0xff; /* undo the EOR 0x80 applied by the user and negate to make it an inkey code. */
        /* Note ((x ^ 0x80) ^ 0xff) == (x ^ 0x7f)*/
        if(keysPressed[inkeyToSDLK(key)])
            r6502_x = 0xff; /* negative result if pressed */

        r6502_x = 0; /* greater than or equal to zero if not pressed.*/
    }
    else
    {

        /* Scan all keys starting at this key */
        for(key = r6502_x; key < NUM_BBCB_KEYS; ++key)
        {
            int lookup = inkeyToSDLK(key ^ 0xff); /* Convert internal key number into negative INKEY value. */
            if(lookup >= 0)
            {
                if(keysPressed[lookup])
                {
                    r6502_x = key; /* return earliest keynumber being pressed and abort scan */
                    return;
                }
            }
        }
        r6502_x = 0xff; /* no key was pressed */
    }
}

/* OSBYTE &7A */
void scankeyboardfrom16(void)
{
    /*
    Keyboard scan from 16 decimal
    No entry parameters
    Internal key number (see table above) of the key pressed is returned in X.
    This call is directly equivalent to an OSBYTE call with A=&79 and X=16.
    On exit:
    A is preserved
    X contains key number or zero if none pressed (XXX: is this correct? - it is at odds with other docs).
    Y and C are undefined
    */
    /*
        Alternative docs:
        Keyboard Scan from &10
        Simply performs OSByte &79 with X=16
    */
    r6502_x = 16;
    scankeyboard();
    /* Always put 0xEE in the Y register*/
    r6502_y = 0xEE;
}

static int readlineKeyDown = 0;
void OnKeyDownReadLine(unsigned int key)
{
	readlineKeyDown = key;
}

/* Returns number of characters read. */
void readline(void)
{
/*
    This routine takes a specified number of characters from the currently selected input stream.
    Input is terminated following a RETURN or an ESCAPE. DELETE (&7F/127) deletes the previous
    character and CTRL U (&15/21) deletes the entire line. If characters are presented after
    the maximum line length has been reached the characters are ignored and a BEL (ASCII 7)
    character is output.

    The parameter block

    XY+0 Buffer address for input LSB
    +1 Buffer address for input MSB
    +2 Maximum line length
    +3 Minimum acceptable ASCII value
    +4 Maximum acceptable ASCII value

    Only characters greater or equal to XY+3 and less than or equal to XY+4 will be accepted.

    On exit: C=0 if a carriage return terminated input.
    C= 1 if an ESCAPE condition terminated input.
    Y contains line length, excluding carriage return if used.
*/

    /* Read upto  characters*/
    int address = ((r6502_y << 8) | r6502_x);
    unsigned char* destBuffer = (unsigned char*)(memory+((memory[address+1] << 8) | memory[address]));
    unsigned int maxCharacters = memory[address+2];
    unsigned char minASCIIValue = memory[address+3];
    unsigned char maxASCIIValue = memory[address+4];
    unsigned char ascii;
    unsigned int finished = 0, numCharacters = 0, i;

    /* Flush buffer. */
    memset(destBuffer, 0, maxCharacters);

    while(1)
    {
        /* This must do what PollMessages() does */
        SDL_Event event ;

        /*message pump*/
        /*look for an event*/
        while ( SDL_PollEvent ( &event ) )
        {
            /*an event was found*/
            switch(event.type)
            {
                case SDL_QUIT:
                    quit = 1;
					finalise();
                    exit(0);
                    break;

                case SDL_KEYDOWN:
                    if(event.key.keysym.sym == SDLK_RETURN)
                    {
                        /* Store carriage return in buffer too. */
                        if(numCharacters < maxCharacters)
                        {
                            destBuffer[numCharacters] = 0x0d;
                            ++numCharacters;
                        }
                        finished = 1;
                        CLEARFLAG(CFLAG);
                    }
                    else if (event.key.keysym.sym == SDLK_ESCAPE)
                    {
                        numCharacters = 0;
                        finished = 1;
                        SETFLAG(CFLAG);
                    }
                    else if (event.key.keysym.sym == SDLK_u && ((event.key.keysym.mod & KMOD_CTRL) == KMOD_CTRL))
                    {
                        /* Delete whole line */
                        if(numCharacters)
                        {
                            vdu_gfx_cursor_x -= (8<<3) * numCharacters;
                            for(i = 0; i < numCharacters; ++i)
                            {
                                /* Overwrite with a space character */
                                print(32);
                            }
                            vdu_gfx_cursor_x -= (8<<3) * numCharacters;
                            numCharacters = 0;
                        }
                    }
                    else if (event.key.keysym.sym == SDLK_BACKSPACE)
                    {
                        if(numCharacters)
                        {
                            --numCharacters;
                            /* Move graphics cursor back by character width */
                            vdu_gfx_cursor_x -= 8<<3;

                            /* Overwrite with a space character */
                            print(32);
                            vdu_gfx_cursor_x -= 8<<3;
                        }
                    }
                    else
                    {
                        /* Translate into ASCII value */
                        if(event.key.keysym.unicode < 0x80 && event.key.keysym.unicode > 0 )
                        {
                            ascii = (char)event.key.keysym.unicode;

                            /* Insert character into buffer */
                            if(ascii >= minASCIIValue && ascii <= maxASCIIValue)
                            {
                                if(numCharacters < maxCharacters)
                                {
                                    destBuffer[numCharacters] = ascii;
                                    ++numCharacters;
                                    print(ascii);
                                }
                            }
                        }
                    }

                    break;
            }
        }

        if(finished)
            break;
    }

    /* return line length excluding carriage return.*/
    r6502_y = numCharacters;
}

int setLogicalColour(int logical, int physical)
{
    const int pal = vdu_paletteIndexForMode[vdu_mode];
    memcpy(&logicalPalette[pal][logical], &physicalPalette[pal][physical], sizeof(SDL_Color));

    if(screen->format->palette)
    {
        if(!SDL_SetColors(screen, logicalPalette[pal], 0, vdu_coloursAvailable[vdu_mode]))
            return 0;
    }

    return 1;
}

int initLogicalPalette(void)
{
    const int pal = vdu_paletteIndexForMode[vdu_mode];
    memcpy(logicalPalette[pal], physicalPalette[pal], vdu_coloursAvailable[vdu_mode] * sizeof(SDL_Color));

    if(screen->format->palette)
    {
        if(!SDL_SetColors(screen, logicalPalette[pal], 0, vdu_coloursAvailable[vdu_mode]))
            return 0;
    }

    /* Set default logical colours (black and white) */
    vdu_col_background = vdu_gcol[BACKGROUND] = vdu_default_logical_col[ vdu_paletteIndexForMode[vdu_mode] ][0];
    vdu_col_foreground = vdu_gcol[FOREGROUND] = vdu_default_logical_col[ vdu_paletteIndexForMode[vdu_mode] ][1];

    return 1;
}

void initPhysicalPalette(void)
{
    switch(vdu_paletteIndexForMode[vdu_mode])
    {
#if 0 /* Never happens for ch-egg */
        /*
            Two colour MODEs
            0=Black
            1=White
        */
        case 0:
            /* Black */
            physicalPalette[0][0].r = 0;
            physicalPalette[0][0].g = 0;
            physicalPalette[0][0].b = 0;

            /* White */
            physicalPalette[0][1].r = 255;
            physicalPalette[0][1].g = 255;
            physicalPalette[0][1].b = 255;
            break;

        /*
            Four colour MODEs
            0=Black
            1=Red
            2=Yellow
            3=White
        */
        case 1:
            /* Black */
            physicalPalette[1][0].r = 0;
            physicalPalette[1][0].g = 0;
            physicalPalette[1][0].b = 0;

            /* Red */
            physicalPalette[1][1].r = 255;
            physicalPalette[1][1].g = 0;
            physicalPalette[1][1].b = 0;

            /* Yellow */
            physicalPalette[1][2].r = 255;
            physicalPalette[1][2].g = 255;
            physicalPalette[1][2].b = 0;

            /* White */
            physicalPalette[1][3].r = 255;
            physicalPalette[1][3].g = 255;
            physicalPalette[1][3].b = 255;
            break;
#endif
        /*
            16 colour MODE
            0=Black
            1=Red
            2=Green
            3=Yellow
            4=Blue
            5=Magenta
            6=Cyan
            7=White
            8=Flashing black/white
            9=Flashing red/eyan
            10=Flashing green/magenta
            11=Flashing yellow/blue
            12=Flashing blue/yellow
            13=Flashing magenta/green
            14=Flashing cyan/red
            15=Flashing white/black
        */
        case 2:

            /* Black */
            physicalPalette[2][0].r = 0;
            physicalPalette[2][0].g = 0;
            physicalPalette[2][0].b = 0;

            /* Red */
            physicalPalette[2][1].r = 255;
            physicalPalette[2][1].g = 0;
            physicalPalette[2][1].b = 0;

            /* Green */
            physicalPalette[2][2].r = 0;
            physicalPalette[2][2].g = 255;
            physicalPalette[2][2].b = 0;

            /* Yellow */
            physicalPalette[2][3].r = 255;
            physicalPalette[2][3].g = 255;
            physicalPalette[2][3].b = 0;

            /* Blue */
            physicalPalette[2][4].r = 0;
            physicalPalette[2][4].g = 0;
            physicalPalette[2][4].b = 255;

            /* Magenta */
            physicalPalette[2][5].r = 255;
            physicalPalette[2][5].g = 0;
            physicalPalette[2][5].b = 255;

            /* Cyan */
            physicalPalette[2][6].r = 0;
            physicalPalette[2][6].g = 255;
            physicalPalette[2][6].b = 255;

            /* White */
            physicalPalette[2][7].r = 255;
            physicalPalette[2][7].g = 255;
            physicalPalette[2][7].b = 255;

            physicalPalette[2][8].r = 255;
            physicalPalette[2][8].g = 255;
            physicalPalette[2][8].b = 255;

            physicalPalette[2][9].r = 255;
            physicalPalette[2][9].g = 255;
            physicalPalette[2][9].b = 255;

            physicalPalette[2][10].r = 255;
            physicalPalette[2][10].g = 255;
            physicalPalette[2][10].b = 255;

            physicalPalette[2][11].r = 255;
            physicalPalette[2][11].g = 255;
            physicalPalette[2][11].b = 255;

            physicalPalette[2][12].r = 255;
            physicalPalette[2][12].g = 255;
            physicalPalette[2][12].b = 255;

            physicalPalette[2][13].r = 255;
            physicalPalette[2][13].g = 255;
            physicalPalette[2][13].b = 255;

            physicalPalette[2][14].r = 255;
            physicalPalette[2][14].g = 255;
            physicalPalette[2][14].b = 255;

            physicalPalette[2][15].r = 255;
            physicalPalette[2][15].g = 255;
            physicalPalette[2][15].b = 255;
            break;
    };
}


int initialise(const char* title, const char* icon)
{
    initialiseIntervalTimer();
    initialiseKeyboardBuffer();

    /*set up keyboard translation table*/
    memory[0xF02B] = 0x03;
    memory[0xF02C] = 0x8C;
    memory[0xF02D] = 0x40;
    memory[0xF02E] = 0xFE;
    memory[0xF02F] = 0xA0;
    memory[0xF030] = 0x7F;
    memory[0xF031] = 0x8C;
    memory[0xF032] = 0x43;
    memory[0xF033] = 0xFE;
    memory[0xF034] = 0x8E;
    memory[0xF035] = 0x4F;
    memory[0xF036] = 0xFE;
    memory[0xF037] = 0xAE;
    memory[0xF038] = 0x4F;
    memory[0xF039] = 0xFE;
    memory[0xF03A] = 0x60;
    memory[0xF03B] = 0x71;
    memory[0xF03C] = 0x33;
    memory[0xF03D] = 0x34;
    memory[0xF03E] = 0x35;
    memory[0xF03F] = 0x84;
    memory[0xF040] = 0x38;
    memory[0xF041] = 0x87;
    memory[0xF042] = 0x2D;
    memory[0xF043] = 0x5E;
    memory[0xF044] = 0x8C;
    memory[0xF045] = 0x84;
    memory[0xF046] = 0xEC;
    memory[0xF047] = 0x86;
    memory[0xF048] = 0xED;
    memory[0xF049] = 0x60;
    memory[0xF04A] = 0x00;
    memory[0xF04B] = 0x80;
    memory[0xF04C] = 0x77;
    memory[0xF04D] = 0x65;
    memory[0xF04E] = 0x74;
    memory[0xF04F] = 0x37;
    memory[0xF050] = 0x69;
    memory[0xF051] = 0x39;
    memory[0xF052] = 0x30;
    memory[0xF053] = 0x5F;
    memory[0xF054] = 0x8E;
    memory[0xF055] = 0x6C;
    memory[0xF056] = 0xFE;
    memory[0xF057] = 0xFD;
    memory[0xF058] = 0x6C;
    memory[0xF059] = 0xFA;
    memory[0xF05A] = 0x00;
    memory[0xF05B] = 0x31;
    memory[0xF05C] = 0x32;
    memory[0xF05D] = 0x64;
    memory[0xF05E] = 0x72;
    memory[0xF05F] = 0x36;
    memory[0xF060] = 0x75;
    memory[0xF061] = 0x6F;
    memory[0xF062] = 0x70;
    memory[0xF063] = 0x5B;
    memory[0xF064] = 0x8F;
    memory[0xF065] = 0x2C;
    memory[0xF066] = 0xB7;
    memory[0xF067] = 0xD9;
    memory[0xF068] = 0x6C;
    memory[0xF069] = 0x28;
    memory[0xF06A] = 0x02;
    memory[0xF06B] = 0x01;
    memory[0xF06C] = 0x61;
    memory[0xF06D] = 0x78;
    memory[0xF06E] = 0x66;
    memory[0xF06F] = 0x79;
    memory[0xF070] = 0x6A;
    memory[0xF071] = 0x6B;
    memory[0xF072] = 0x40;
    memory[0xF073] = 0x3A;
    memory[0xF074] = 0x0D;
    memory[0xF075] = 0x00;
    memory[0xF076] = 0xFF;
    memory[0xF077] = 0x01;
    memory[0xF078] = 0x02;
    memory[0xF079] = 0x09;
    memory[0xF07A] = 0x0A;
    memory[0xF07B] = 0x02;
    memory[0xF07C] = 0x73;
    memory[0xF07D] = 0x63;
    memory[0xF07E] = 0x67;
    memory[0xF07F] = 0x68;
    memory[0xF080] = 0x6E;
    memory[0xF081] = 0x6C;
    memory[0xF082] = 0x3B;
    memory[0xF083] = 0x5D;
    memory[0xF084] = 0x7F;
    memory[0xF085] = 0xAC;
    memory[0xF086] = 0x44;
    memory[0xF087] = 0x02;
    memory[0xF088] = 0xA2;
    memory[0xF089] = 0x00;
    memory[0xF08A] = 0x60;
    memory[0xF08B] = 0x00;
    memory[0xF08C] = 0x7A;
    memory[0xF08D] = 0x20;
    memory[0xF08E] = 0x76;
    memory[0xF08F] = 0x62;
    memory[0xF090] = 0x6D;
    memory[0xF091] = 0x2C;
    memory[0xF092] = 0x2E;
    memory[0xF093] = 0x2F;
    memory[0xF094] = 0x8B;
    memory[0xF095] = 0xAE;
    memory[0xF096] = 0x41;
    memory[0xF097] = 0x02;
    memory[0xF098] = 0x4C;
    memory[0xF099] = 0xAD;
    memory[0xF09A] = 0xE1;
    memory[0xF09B] = 0x1B;
    memory[0xF09C] = 0x81;
    memory[0xF09D] = 0x82;
    memory[0xF09E] = 0x83;
    memory[0xF09F] = 0x85;
    memory[0xF0A0] = 0x86;
    memory[0xF0A1] = 0x88;
    memory[0xF0A2] = 0x89;
    memory[0xF0A3] = 0x5C;
    memory[0xF0A4] = 0x8D;
    memory[0xF0A5] = 0x6C;
    memory[0xF0A6] = 0x20;
    memory[0xF0A7] = 0x02;
    memory[0xF0A8] = 0xD0;
    memory[0xF0A9] = 0xEB;
    memory[0xF0AA] = 0xA2;

    r6502_a = 0x00;
    r6502_x = 0x00;
    r6502_y = 0x00;
    r6502_ps = 0x00;
    r6502_sp = 0xFF;

    quit = FALSE;

    /* initialize systems */
    if(SDL_Init ( SDL_INIT_VIDEO|SDL_INIT_TIMER )==-1)
    {
        return 0;
    }

#ifdef WIN32
    SDL_WM_SetCaption(title, icon);
#endif

    /* create a window (width, height, bits per pixel, and flags) */
    /* Mode 2 on the BBC was 160x256, but we need to stretch out the horizontal to 320. */
    //screen = SDL_SetVideoMode ( 320 , 256 , 8, SDL_SWSURFACE ) ;

    /* This surface will be capable of supporting any BBCB mode. */
    screen = SDL_SetVideoMode ( 640 , 512 , 8, SDL_SWSURFACE ) ;

    initPhysicalPalette();
    initLogicalPalette();

    if(!AudioInit())
        return 0;

    /* Enable Unicode translation */
    SDL_EnableUNICODE( 1 );

    if ( SDL_MUSTLOCK(screen) )
    {
        if ( SDL_LockSurface(screen) < 0 )
		{
            return 0;
        }
    }

    return 1;
}

void finalise(void)
{
    if ( SDL_MUSTLOCK(screen) )
    {
        SDL_UnlockSurface(screen);
    }

    AudioQuit();

    /* Cleanup SDL. */
    SDL_Quit() ;
}

/* Returns 1 to indicate quit. */
int PollMessages(void)
{
    SDL_Event event ;

    /*message pump*/
    /*look for an event*/
    while ( SDL_PollEvent ( &event ) )
    {
        /*an event was found*/
        switch(event.type)
        {
            case SDL_QUIT:
                quit = 1;
				finalise();
				exit(0);
                return 1;

            case SDL_KEYDOWN:
                keysPressed[event.key.keysym.sym] = 1;
                break;

            case SDL_KEYUP:
                keysPressed[event.key.keysym.sym] = 0;
                break;
        }
    }

    if ( SDL_MUSTLOCK(screen) )
    {
        SDL_UnlockSurface(screen);
    }

	SDL_UpdateRect(screen, 0, 0, screen->w, screen->h);

    if ( SDL_MUSTLOCK(screen) )
    {
        if ( SDL_LockSurface(screen) < 0 )
		{
            return 1;
        }
    }
    return 0;
}
/*
void sleep(unsigned int milliseconds)
{
    usleep(milliseconds * 1000); //SDL_Delay(milliseconds);
}
*/
/* Implements a delay in miliseconds. This will put the thread to sleep if it can. */
void delay(unsigned int milliseconds)
{
    //struct timespec spec;

	/* clock() returns the number of clock ticks since the start of the program */
	/* On PC this is 1000, on ubuntu Linux, this is 1000000 */
    /* Code taken from Larry Bank's programmers corner website: http://www.bitbanksoftware.com/Programmers/code4.html */
sched_yield();
    unsigned int ticksPerMillisecond = 1;//(CLOCKS_PER_SEC/1000);
    int counter = SDL_GetTicks() + (ticksPerMillisecond * milliseconds);
    if(milliseconds >= 2)
    {
        //usleep((milliseconds - 1)*1000);
        //spec.tv_nsec = (milliseconds-1)*1000*1000;
        //spec.tv_sec = 0;
        //nanosleep(&spec, (struct timespec *)NULL);
        //sleep(milliseconds -1); /* Sleep through most of it then spin for the last millisecond. */

        usleep((milliseconds-1)*1000);
    }

    while(SDL_GetTicks() < counter)
    {
        sched_yield();
    }
}


//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////


unsigned char _PROCESSORSTATUS(unsigned char n)
{
  return (r6502_ps | UFLAG | n);
}

void _STACK_PUSH(unsigned char n)
{
  r6502write(STACK_START+(r6502_sp--), n);
}

unsigned char _STACK_POP(void)
{
  return memory[STACK_START+(++r6502_sp)];
}

void _SETFLAG(unsigned char nflag)
{
  /*set bit*/
  r6502_ps = (r6502_ps | nflag);
}

void _CLEARFLAG(unsigned char nflag)
{
  /*set bit*/
  r6502_ps = (r6502_ps & ~nflag);
}

void _SETNFLAG(unsigned char n)
{
  /*clear bit 7 (N)*/
  r6502_ps = (r6502_ps & NOTNFLAG);
  /*set bit 7 (N)*/
  r6502_ps = (r6502_ps | (n & NFLAG));
}

void _SETVFLAG(unsigned char n)
{
  /*clear bit 6 (V)*/
  r6502_ps = (r6502_ps & NOTVFLAG);
  /*set bit 6 (V)*/
  r6502_ps = (r6502_ps | (n & VFLAG));
}

void _SETBFLAG(unsigned char n)
{
  /*clear bit 4 (B)*/
  r6502_ps = (r6502_ps & NOTBFLAG);
  /*set bit 4 (B)*/
  r6502_ps = (r6502_ps | (n & BFLAG));
}

void _SETIFLAG(unsigned char n)
{
  /*clear bit 3 (I)*/
  r6502_ps = (r6502_ps & NOTIFLAG);
  /*set bit 3 (I)*/
  r6502_ps = (r6502_ps | (n & IFLAG));
}

void _SETZFLAG(unsigned char n)
{
  if (n == 0)
    /*set bit 1 (Z)*/
    r6502_ps = (r6502_ps | ZFLAG);
  else
    /*clear bit 1 (Z)*/
    r6502_ps = (r6502_ps & NOTZFLAG);
}

void _ADDRESSOFABSOLUTEXPLUS(unsigned int n)
{
  address = n + r6502_x;
}

void _ADDRESSOFABSOLUTEYPLUS(unsigned int n)
{
  address = n + r6502_y;
}

void _ADDRESSOFPOSTINDEXEDY(unsigned int n)
{
  nlo = memory[n];
  value1 = ((n+1) & 0xFF);
  nhi = memory[(n & 0xFF00) | value1];
  address = (nhi << 8) | nlo;
  address += r6502_y;
}

void _ADDRESSOFPREINDEXED(unsigned int n)
{
  address = ((n + r6502_x) & 0xFF);
  nlo = memory[address];
  value1 = ((address+1) & 0xFF);
  nhi = memory[(address & 0xFF00) | value1];
  address = (nhi << 8) | nlo;
}

void _ADCxx(unsigned char n)
{
  TRACE("ADCxx")
  r6502adc(n);
}

void _ADC_ABSOL(unsigned int n)
{
  TRACE("ADC_ABSOL")
  value1 = r6502read(n);
  r6502adc(value1);
}

void _ADC_ABSXP(unsigned int n)
{
  TRACE("ADC_ABSXP")
  ADDRESSOFABSOLUTEXPLUS(n)
  value1 = r6502read(address);
  r6502adc(value1);
}

void _ADC_ABSYP(unsigned int n)
{
  TRACE("ADC_ABSYP")
  ADDRESSOFABSOLUTEYPLUS(n)
  value1 = r6502read(address);
  r6502adc(value1);
}

void _ADC_IMMED(unsigned char n)
{
  TRACE("ADC_IMMED")
  r6502adc(n);
}

void _ADC_POSTI(unsigned int n)
{
  TRACE("ADC_POSTI")
  ADDRESSOFPOSTINDEXEDY(n)
  value1 = r6502read(address);
  r6502adc(value1);
}

void _ADC_ZEROP(unsigned char n)
{
  TRACE("ADC_ZEROP")
  value1 = memory[n];
  r6502adc(value1);
}

void _ADC_ZEROX(unsigned char n)
{
  TRACE("ADC_ZEROX")
  address = ((n + r6502_x) & 0xFF);
  value1 = memory[address];
  r6502adc(value1);
}

void _AND_IMMED(unsigned char n)
{
  TRACE("AND_IMMED")
  r6502_a &= n;
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _AND_ZEROP(unsigned char n)
{
  TRACE("AND_ZEROP")
  value1 = memory[n];
  r6502_a &= value1;
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _AND_POSTI(unsigned int n)
{
  TRACE("AND_POSTI")
  ADDRESSOFPOSTINDEXEDY(n)
  r6502_a &= r6502read(address);
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _AND_ABSYP(unsigned int n)
{
    TRACE("AND_ABSYP")
    ADDRESSOFABSOLUTEYPLUS(n)
    r6502_a &= r6502read(address);
    SETNFLAG(r6502_a)
    SETZFLAG(r6502_a)
}

void _ASL(void)
{
  TRACE("ASL")
  if (r6502_a & 0x80)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  r6502_a = (r6502_a << 1);
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _ASL_ZEROP(unsigned char n)
{
  TRACE("ASL_ZEROP")
  value1 = memory[n];
  if (value1 & 0x80)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  value2 = (value1 << 1);
  r6502write(n, value2);
  SETNFLAG(value2)
  SETZFLAG(value2)
}

void _BIT_ZEROP(unsigned char n)
{
  TRACE("BIT_ZEROP")
  value1 = memory[n];
  value2 = (r6502_a & value1);
  SETNFLAG(value1)
  SETVFLAG(value1)
  SETZFLAG(value2)
}

void _BRK(void)
{
  TRACE("BRK")
  assert(!"BRK unhandled");
}

void _CLC(void)
{
  TRACE("CLC")
  CLEARFLAG(CFLAG);
}

void _CLD(void)
{
  TRACE("CLD")
  CLEARFLAG(DFLAG);
}

void _CMP_ABSOL(unsigned int n)
{
  TRACE("CMP_ABSOL")
  value1 = r6502read(n);
  value2 = (r6502_a - value1);
  if (r6502_a >= value1)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  SETNFLAG(value2)
  SETZFLAG(value2)
}

void _CMP_ABSXP(unsigned int n)
{
  TRACE("CMP_ABSXP")
  ADDRESSOFABSOLUTEXPLUS(n)
  value1 = r6502read(address);
  value2 = (r6502_a - value1);
  if (r6502_a >= value1)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  SETNFLAG(value2)
  SETZFLAG(value2)
}

void _CMP_ABSYP(unsigned int n)
{
  TRACE("CMP_ABSYP")
  ADDRESSOFABSOLUTEYPLUS(n)
  value1 = r6502read(address);
  value2 = (r6502_a - value1);
  if (r6502_a >= value1)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  SETNFLAG(value2)
  SETZFLAG(value2)
}

void _CMP_IMMED(unsigned char n)
{
  TRACE("CMP_IMMED")
  value2 = (r6502_a - n);
  if (r6502_a >= n)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  SETNFLAG(value2)
  SETZFLAG(value2)
}

void _CMP_ZEROP(unsigned char n)
{
  TRACE("CMP_ZEROP")
  value1 = memory[n];
  value2 = (r6502_a - value1);
  if (r6502_a >= value1)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  SETZFLAG(value2)
  SETNFLAG(value2)
}

void _CMP_POSTI(unsigned int n)
{
  TRACE("CMP_POSTI")
  ADDRESSOFPOSTINDEXEDY(n)
  value1 = r6502read(address);
  value2 = (r6502_a - value1);
  if (r6502_a >= value1)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  SETZFLAG(value2)
  SETNFLAG(value2)
}

void _CPX_IMMED(unsigned char n)
{
  TRACE("CPX_IMMED")
  value2 = (r6502_x - n);
  if (r6502_x >= n)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  SETNFLAG(value2)
  SETZFLAG(value2)
}

void _CPX_ZEROP(unsigned char n)
{
  TRACE("CPX_ZEROP")
  value1 = memory[n];
  value2 = (r6502_x - value1);
  if (r6502_x >= value1)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  SETZFLAG(value2)
  SETNFLAG(value2)
}

void _CPX_ABSOL(unsigned int n)
{
  TRACE("CPX_ABSOL")
  value1 = r6502read(n);
  value2 = (r6502_x - value1);
  if (r6502_x >= value1)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  SETNFLAG(value2)
  SETZFLAG(value2)
}

void _CPY_IMMED(unsigned char n)
{
  TRACE("CPY_IMMED")
  value2 = (r6502_y - n);
  if (r6502_y >= n)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  SETNFLAG(value2)
  SETZFLAG(value2)
}

void _CPY_ZEROP(unsigned char n)
{
  TRACE("CPY_ZEROP")
  value1 = memory[n];
  value2 = (r6502_y - value1);
  if (r6502_y >= value1)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  SETZFLAG(value2)
  SETNFLAG(value2)
}

void _CPY_ABSOL(unsigned int n)
{
  TRACE("CPY_ABSOL")
  value1 = r6502read(n);
  value2 = (r6502_y - value1);
  if (r6502_y >= value1)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  SETNFLAG(value2)
  SETZFLAG(value2)
}

void _DEC_ABSOL(unsigned int n)
{
  TRACE("DEC_ABSOL")
  value1 = r6502read(n);
  value1--;
  r6502write(n,value1);
  SETNFLAG(value1)
  SETZFLAG(value1)
}

void _DEC_ABSXP(unsigned int n)
{
  TRACE("DEC_ABSXP")
  ADDRESSOFABSOLUTEXPLUS(n)
  value1 = r6502read(address);
  value1--;
  r6502write(address,value1);
  SETZFLAG(value1)
  SETNFLAG(value1)
}

void _DEC_ZEROP(unsigned char n)
{
  TRACE("DEC_ZEROP")
  value1 = memory[n];
  value1--;
  r6502write(n, value1);
  SETNFLAG(value1)
  SETZFLAG(value1)
}

void _DEC_ZEROX(unsigned char n)
{
  TRACE("DEC_ZEROX")
  address = ((n + r6502_x) & 0xFF);
  value1 = memory[address];
  value1--;
  r6502write(address, value1);
  SETNFLAG(value1)
  SETZFLAG(value1)
}

void _DEX(void)
{
  TRACE("DEX")
  r6502_x--;
  SETNFLAG(r6502_x)
  SETZFLAG(r6502_x)
}

void _DEY(void)
{
  TRACE("DEY")
  r6502_y--;
  SETNFLAG(r6502_y)
  SETZFLAG(r6502_y)
}

void _EOR_IMMED(unsigned char n)
{
  TRACE("EOR_IMMED")
  r6502_a ^= n;
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _EOR_ZEROP(unsigned char n)
{
  TRACE("EOR_ZEROP")
  r6502_a ^= memory[n];
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _EOR_POSTI(unsigned int n)
{
  TRACE("EOR_POSTI")
  ADDRESSOFPOSTINDEXEDY(n)
  r6502_a ^= r6502read(address);
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _INC_ABSOL(unsigned int n)
{
  TRACE("INC_ABSOL")
  value1 = r6502read(n);
  value1++;
  r6502write(n,value1);
  SETNFLAG(value1)
  SETZFLAG(value1)
}

void _INC_ABSXP(unsigned int n)
{
  TRACE("INC_ABSXP")
  ADDRESSOFABSOLUTEXPLUS(n)
  value1 = r6502read(address);
  value1++;
  r6502write(address,value1);
  SETZFLAG(value1)
  SETNFLAG(value1)
}

void _INC_ZEROP(unsigned char n)
{
  TRACE("INC_ZEROP")
  value1 = memory[n];
  value1++;
  r6502write(n, value1);
  SETNFLAG(value1)
  SETZFLAG(value1)
}

void _INC_ZEROX(unsigned char n)
{
  TRACE("INC_ZEROX")
  address = ((n + r6502_x) & 0xFF);
  value1 = memory[address];
  value1++;
  r6502write(address, value1);
  SETNFLAG(value1)
  SETZFLAG(value1)
}

void _INX(void)
{
  TRACE("INX")
  r6502_x++;
  SETNFLAG(r6502_x)
  SETZFLAG(r6502_x)
}

void _INY(void)
{
  TRACE("INY")
  r6502_y++;
  SETNFLAG(r6502_y)
  SETZFLAG(r6502_y)
}
/*
void _JMP_SIMUL(unsigned int n)
{
  TRACE("JMP_ABSOL")
  n();
}

void _JMP_INDIRECT(unsigned int n)
{
  TRACE("JMP_INDIRECT")
  r6502jmpindirect(n);
}

void _JSR_ABSOLOR(unsigned int n, unsigned int r)
{
  STACK_PUSH(((r-1) & 0xFF00)>>8);
  STACK_PUSH((r-1) & 0x00FF);
  TRACE("JSR_ABSOL")
  n();
}
*/
void _LDA_ABSOL(unsigned int n)
{
  TRACE("LDA_ABSOL")
  r6502_a = r6502read(n);
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _LDA_ABSXP(unsigned int n)
{
  TRACE("LDA_ABSXP")
  ADDRESSOFABSOLUTEXPLUS(n)
  r6502_a = r6502read(address);
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _LDA_ABSYP(unsigned int n)
{
  TRACE("LDA_ABSYP")
  ADDRESSOFABSOLUTEYPLUS(n)
  r6502_a = r6502read(address);
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _LDA_IMMED(unsigned char n)
{
  TRACE("LDA_IMMED")
  r6502_a = n;
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _LDA_POSTI(unsigned int n)
{
  TRACE("LDA_POSTI")
  ADDRESSOFPOSTINDEXEDY(n)
  r6502_a = r6502read(address);
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _LDA_PREIN(unsigned int n)
{
  TRACE("LDA_PREIN")
  ADDRESSOFPREINDEXED(n)
  r6502_a = r6502read(address);
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _LDA_ZEROP(unsigned char n)
{
  TRACE("LDA_ZEROP")
  r6502_a = memory[n];
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _LDA_ZEROX(unsigned char n)
{
  TRACE("LDA_ZEROX")
  address = ((n + r6502_x) & 0xFF);
  r6502_a = memory[address];
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _LDX_ABSOL(unsigned int n)
{
  TRACE("LDX_ABSOL")
  r6502_x = r6502read(n);
  SETNFLAG(r6502_x)
  SETZFLAG(r6502_x)
}

void _LDX_ABSYP(unsigned int n)
{
  TRACE("LDX_ABSYP")
  ADDRESSOFABSOLUTEYPLUS(n)
  r6502_x = r6502read(address);
  SETNFLAG(r6502_x)
  SETZFLAG(r6502_x)
}

void _LDX_IMMED(unsigned char n)
{
  TRACE("LDX_IMMED")
  r6502_x = n;
  SETNFLAG(r6502_x)
  SETZFLAG(r6502_x)
}

void _LDX_ZEROP(unsigned char n)
{
  TRACE("LDX_ZEROP")
  r6502_x = memory[n];
  SETNFLAG(r6502_x)
  SETZFLAG(r6502_x)
}

void _LDX_ZEROY(unsigned char n)
{
  TRACE("LDX_ZEROY")
  address = ((n + r6502_y) & 0xFF);
  r6502_x = memory[address];
  SETNFLAG(r6502_x)
  SETZFLAG(r6502_x)
}

void _LDY_ABSOL(unsigned int n)
{
  TRACE("LDY_ABSOL")
  r6502_y = r6502read(n);
  SETNFLAG(r6502_y)
  SETZFLAG(r6502_y)
}

void _LDY_ABSXP(unsigned int n)
{
  TRACE("LDY_ABSXP")
  ADDRESSOFABSOLUTEXPLUS(n)
  r6502_y = r6502read(address);
  SETNFLAG(r6502_y)
  SETZFLAG(r6502_y)
}

void _LDY_IMMED(unsigned char n)
{
  TRACE("LDY_IMMED")
  r6502_y = n;
  SETNFLAG(r6502_y)
  SETZFLAG(r6502_y)
}

void _LDY_ZEROP(unsigned char n)
{
  TRACE("LDY_ZEROP")
  r6502_y = memory[n];
  SETNFLAG(r6502_y)
  SETZFLAG(r6502_y)
}

void _LDY_ZEROX(unsigned char n)
{
  TRACE("LDY_ZEROX")
  address = ((n + r6502_x) & 0xFF);
  r6502_y = memory[address];
  SETNFLAG(r6502_y)
  SETZFLAG(r6502_y)
}

void _LSR(void)
{
  TRACE("LSR")
  if (r6502_a & 0x01)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  r6502_a = (r6502_a >> 1);
  CLEARFLAG(NFLAG);
  SETZFLAG(r6502_a)
}

void _LSR_ABSOL(unsigned int n)
{
  TRACE("LSR_ABSOL")
  value1 = r6502read(n);
  if (value1 & 0x01)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  value1 = (value1 >> 1);
  r6502write(n,value1);
  CLEARFLAG(NFLAG)
  SETZFLAG(value1)
}

void _LSR_ABSXP(unsigned int n)
{
  TRACE("LSR_ABSXP")
  ADDRESSOFABSOLUTEXPLUS(n)
  value1 = r6502read(address);
  if (value1 & 0x01)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  value1 = (value1 >> 1);
  r6502write(address,value1);
  CLEARFLAG(NFLAG)
  SETZFLAG(value1)
}

void _LSR_ZEROP(unsigned char n)
{
  TRACE("LSR_ZEROP")
  value1 = memory[n];
  if (value1 & 0x01)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  value1 = (value1 >> 1);
  r6502write(n, value1);
  CLEARFLAG(NFLAG)
  SETZFLAG(value1)
}

void _ORA_IMMED(unsigned char n)
{
  TRACE("ORA_IMMED")
  r6502_a |= n;
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _ORA_ZEROP(unsigned char n)
{
  TRACE("ORA_ZEROP")
  r6502_a |= memory[n];
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _ORA_ABSOL(unsigned int n)
{
  TRACE("ORA_ABSOL")
  r6502_a |= r6502read(n);
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _ORA_ABSYP(unsigned int n)
{
  TRACE("ORA_ABSYP")
  ADDRESSOFABSOLUTEYPLUS(n)
  r6502_a |= r6502read(address);
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _ORA_POSTI(unsigned int n)
{
  TRACE("ORA_POSTI")
  ADDRESSOFPOSTINDEXEDY(n)
  r6502_a |= r6502read(address);
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}
void _PHA(void)
{
  TRACE("PHA")
  STACK_PUSH(r6502_a)
}

void _PHP(void)
{
  TRACE("PHP")
  value1 = PROCESSORSTATUS(BFLAG)
  STACK_PUSH(value1)
}

void _PLA(void)
{
  TRACE("PLA")
  STACK_POP(r6502_a)
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _PLP(void)
{
  TRACE("PLP")
  STACK_POP(r6502_ps);
}

void _ROL(void)
{
  TRACE("ROL")
  value2 = (r6502_ps & CFLAG);
  if (r6502_a & 0x80)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  r6502_a = (r6502_a << 1);
  if (value2)
    r6502_a |= 1;
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _ROR(void)
{
  TRACE("ROR")
  value2 = (r6502_ps & CFLAG);
  if(r6502_a & 1)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  r6502_a = (r6502_a >> 1);
  if(value2)
    r6502_a |= 0x80;
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _ROR_ABSOL(unsigned int n)
{
  TRACE("ROR_ABSOL")
  value1 = r6502read(n);
  value2 = (r6502_ps & CFLAG);
  if (value1 & 1)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  value1 = (value1 >> 1);
  if (value2)
    value1 |= 0x80;
  r6502write(n, value1);
  SETNFLAG(value1)
  SETZFLAG(value1)
}

void _ROR_ZEROP(unsigned char n)
{
  TRACE("ROR_ZEROP")
  value1 = r6502read(n);
  value2 = (r6502_ps & CFLAG);
  if (value1 & 1)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  value1 = (value1 >> 1);
  if (value2)
    value1 |= 0x80;
  r6502write(n, value1);
  SETNFLAG(value1)
  SETZFLAG(value1)
}

void _ROL_ZEROP(unsigned char n)
{
  TRACE("ROL_ZEROP")
  value1 = memory[n];
  value2 = (r6502_ps & CFLAG);
  if (value1 & 0x80)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  value1 = (value1 << 1);
  if (value2)
    value1 |= 1;
  r6502write(n, value1);
  SETNFLAG(value1)
  SETZFLAG(value1)
}

void _ROL_ABSOL(unsigned int n)
{
  TRACE("ROL_ABSOL")
  value1 = r6502read(n);
  value2 = (r6502_ps & CFLAG);
  if (value1 & 0x80)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  value1 = (value1 << 1);
  if (value2)
    value1 |= 1;
  r6502write(n, value1);
  SETNFLAG(value1)
  SETZFLAG(value1)
}

void _RLA_ABSOL(unsigned int n)
{
  TRACE("RLA_ABSOL")
  value1 = (r6502_ps & CFLAG);
  value2 = r6502read(n);
  if (value2 & 0x80)
    SETFLAG(CFLAG)
  else
    CLEARFLAG(CFLAG)
  value2 = (value2 << 1);
  if (value1)
    value2 |= 1;
  r6502write(n,value2);
  r6502_a &= value2;
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _RTS(void)
{
  TRACE("RTS")
  STACK_POP(nlo)
  STACK_POP(nhi)
}

void _HACK_RTS(void)
{
  STACK_POP(nlo)
  STACK_POP(nhi)
}

void _SBCxx(unsigned char n)
{
  TRACE("SBCxx")
  r6502sbc(n);
}

void _SBC_ABSOL(unsigned int n)
{
  TRACE("SBC_ABSOL")
  value1 = r6502read(n);
  r6502sbc(value1);
}

void _SBC_ABSXP(unsigned int n)
{
  TRACE("SBC_ABSXP")
  ADDRESSOFABSOLUTEXPLUS(n)
  value1 = r6502read(address);
  r6502sbc(value1);
}

void _SBC_ABSYP(unsigned int n)
{
  TRACE("SBC_ABSYP")
  ADDRESSOFABSOLUTEYPLUS(n)
  value1 = r6502read(address);
  r6502sbc(value1);
}

void _SBC_IMMED(unsigned char n)
{
  TRACE("SBC_IMMED")
  r6502sbc(n);
}

void _SBC_POSI(unsigned int n)
{
  TRACE("SBC_POSI")
  ADDRESSOFPOSTINDEXEDY(n)
  value1 = r6502read(address);
  r6502sbc(value1);
}

void _SBC_ZEROP(unsigned char n)
{
  TRACE("SBC_ZEROP")
  value1 = memory[n];
  r6502sbc(value1);
}

void _SEC(void)
{
  TRACE("SEC")
  SETFLAG(CFLAG);
}

void _SED(void)
{
  TRACE("SED")
  SETFLAG(DFLAG);
}

void _STA_ABSOL(unsigned int n)
{
  TRACE("STA_ABSOL")
  r6502write(n,r6502_a);
}

void _STA_ABSXP(unsigned int n)
{
  TRACE("STA_ABSXP")
  ADDRESSOFABSOLUTEXPLUS(n)
  r6502write(address,r6502_a);
}

void _STA_ABSYP(unsigned int n)
{
  TRACE("STA_ABSYP")
  ADDRESSOFABSOLUTEYPLUS(n)
  r6502write(address,r6502_a);
}

void _STA_POSTI(unsigned int n)
{
  TRACE("STA_POSTI")
  ADDRESSOFPOSTINDEXEDY(n)
  r6502write(address,r6502_a);
}

void _STA_ZEROP(unsigned char n)
{
  TRACE("STA_ZEROP")
  r6502write(n, r6502_a);
}

void _STA_ZEROX(unsigned char n)
{
  TRACE("STA_ZEROX")
  address = ((n + r6502_x) & 0xFF);
  r6502write(address, r6502_a);
}

void _STX_ABSOL(unsigned int n)
{
  TRACE("STX_ABSOL")
   r6502write(n,r6502_x);
}

void _STX_ZEROP(unsigned char n)
{
  TRACE("STX_ZEROP")
  r6502write(n, r6502_x);
}

void _STX_ZEROY(unsigned char n)
{
    TRACE("STX_ZEROY");
    address = ((n + r6502_y) & 0xFF);
    r6502write(n, r6502_x);
}

void _STY_ABSOL(unsigned int n)
{
  TRACE("STY_ABSOL")
   r6502write(n, r6502_y);
}

void _STY_ZEROP(unsigned char n)
{
  TRACE("STY_ZEROP")
  r6502write(n, r6502_y);
}

void _TAX(void)
{
  TRACE("TAX")
  r6502_x = r6502_a;
  SETNFLAG(r6502_x)
  SETZFLAG(r6502_x)
}

void _TAY(void)
{
  TRACE("TAY")
  r6502_y = r6502_a;
  SETNFLAG(r6502_y)
  SETZFLAG(r6502_y)
}

void _TSX(void)
{
  TRACE("TSX")
  r6502_x = r6502_sp;
  SETNFLAG(r6502_x)
  SETZFLAG(r6502_x)
}

void _TXA(void)
{
  TRACE("TXA")
  r6502_a = r6502_x;
  SETNFLAG(r6502_a)
  SETZFLAG(r6502_a)
}

void _TXS(void)
{
  TRACE("TXS")
  r6502_sp = r6502_x;
}

void _TYA(void)
{
  TRACE("TYA")
  r6502_a = r6502_y;
  SETZFLAG(r6502_a)
  SETNFLAG(r6502_a)
}

#ifdef __cplusplus
}
#endif
