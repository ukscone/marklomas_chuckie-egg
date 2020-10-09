/*>r6502lib.h
 *
 * BBC 6502 to RISC OS library code
 * by Michel Foot.
 * Version 1.01 (18 Apr 2001).
 *
 * PC port of RISC OS Conversion by Mark Lomas (31 Mar 2007)
 */
#ifndef BBC_R6502LIB_H_INCLUDED
#define BBC_R6502LIB_H_INCLUDED

#include <time.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

//#define TRACE_EXEC

#ifdef TRACE_EXEC
extern int g_traceEnabled;
#define TRACE(x) \
    if(g_traceEnabled) \
    { \
        FILE*fp = fopen("c:\\traceexec.txt", "a"); \
        fprintf(fp, "%s: (a: %02x, x:%02x, y:%02x)\n", x, r6502_a, r6502_x, r6502_y); \
        fclose(fp); \
    }
#else
#define TRACE(x)
#endif

// Table that maps indirect jump address to function calls.
struct JMPIndirectTable
{
    int addr;
    void (*func)(void);
};

extern int initialise(const char* name, const char* icon);
extern void finalise(void);
extern int setLogicalColour(int logical, int physical);
//extern void sleep(unsigned int milliseconds);
extern void delay(unsigned int milliseconds);
extern int PollMessages(void);
extern void flushinputbuffer(void);
extern void bbcvdu(char);
extern void osword(void);
extern void osbyte(void);
extern void oswrch(void);
extern void oscli(void);
extern void r6502adc(int);
extern void r6502jsr(int);
extern char r6502read(int);
extern void r6502sbc(int);
extern void r6502write(int,char);
extern void r6502jmpindirect(int);
extern void r6502SetJMPIndirectionTable(struct JMPIndirectTable* table, size_t rows);

extern unsigned char memory[0x10000];
extern unsigned char r6502_a;
extern unsigned char r6502_x;
extern unsigned char r6502_y;
extern unsigned char r6502_sp;
extern unsigned char r6502_ps;
extern unsigned int address;
extern unsigned char value1, value2;
extern unsigned char nlo, nhi;

extern int quit, escape;

#define NFLAG 0x80
#define VFLAG 0x40
#define UFLAG 0x20
#define BFLAG 0x10
#define DFLAG 0x08
#define IFLAG 0x04
#define ZFLAG 0x02
#define CFLAG 0x01
#define NOTNFLAG 0x7F
#define NOTVFLAG 0xBF
#define NOTUFLAG 0xDF
#define NOTBFLAG 0xEF
#define NOTDFLAG 0xF7
#define NOTIFLAG 0xFB
#define NOTZFLAG 0xFD
#define NOTCFLAG 0xFE

#define STACK_START 0x100
#define STACK_END 0x1FF

#define AND &&
#define NOT !
#define OR ||
#define TRUE 0xFF
#define FALSE 0x00

#define SEI
#define CLI

#if 0

#define PROCESSORSTATUS(n) \
  (r6502_ps | UFLAG | n);

#define STACK_PUSH(n) \
  r6502write(STACK_START+(r6502_sp--), n);

#define STACK_POP(n) \
  n = memory[STACK_START+(++r6502_sp)];

#define SETFLAG(nflag) \
  /*set bit*/ \
  r6502_ps = (r6502_ps | nflag);

#define CLEARFLAG(nflag) \
  /*set bit*/ \
  r6502_ps = (r6502_ps & ~nflag);

#define SETNFLAG(n) \
  /*clear bit 7 (N)*/ \
  r6502_ps = (r6502_ps & NOTNFLAG); \
  /*set bit 7 (N)*/ \
  r6502_ps = (r6502_ps | (n & NFLAG));

#define SETVFLAG(n) \
  /*clear bit 6 (V)*/ \
  r6502_ps = (r6502_ps & NOTVFLAG); \
  /*set bit 6 (V)*/ \
  r6502_ps = (r6502_ps | (n & VFLAG));

#define SETBFLAG(n) \
  /*clear bit 4 (B)*/ \
  r6502_ps = (r6502_ps & NOTBFLAG); \
  /*set bit 4 (B)*/ \
  r6502_ps = (r6502_ps | (n & BFLAG));

#define SETIFLAG(n) \
  /*clear bit 3 (I)*/ \
  r6502_ps = (r6502_ps & NOTIFLAG); \
  /*set bit 3 (I)*/ \
  r6502_ps = (r6502_ps | (n & IFLAG));

#define SETZFLAG(n) \
  if (n == 0) \
    /*set bit 1 (Z)*/ \
    r6502_ps = (r6502_ps | ZFLAG); \
  else \
    /*clear bit 1 (Z)*/ \
    r6502_ps = (r6502_ps & NOTZFLAG);

#define ADDRESSOFABSOLUTEXPLUS(n) \
  address = n + r6502_x;

#define ADDRESSOFABSOLUTEYPLUS(n) \
  address = n + r6502_y;

#define ADDRESSOFPOSTINDEXEDY(n) \
  nlo = memory[n]; \
  value1 = ((n+1) & 0xFF); \
  nhi = memory[(n & 0xFF00) | value1]; \
  address = (nhi << 8) | nlo; \
  address += r6502_y;

#define ADDRESSOFPREINDEXED(n) \
  address = ((n + r6502_x) & 0xFF); \
  nlo = memory[address]; \
  value1 = ((address+1) & 0xFF); \
  nhi = memory[(address & 0xFF00) | value1]; \
  address = (nhi << 8) | nlo;

/* All other osbyte calls. */
#define OSBYTE \
  osbyte();

/* Keyboard scan from 16 decimal */
#define OSBYTE7A \
  osbyte();

/* Read key with time limit (INKEY) */
#define OSBYTE81 \
  osbyte();

/*Read address of key translation table (low byte).*/
#define OSBYTEAC \
  osbyte();

#define OSWORD \
  osword();

#define OSWRCH \
  oswrch();

#define WAIT_FOR_VERTICAL_SYNC /*implement_me;*/

#define ADCxx(n) \
  TRACE("ADCxx") \
  r6502adc(n);

#define ADC_ABSOL(n) \
  TRACE("ADC_ABSOL") \
  value1 = r6502read(n); \
  r6502adc(value1);

#define ADC_ABSXP(n) \
  TRACE("ADC_ABSXP") \
  ADDRESSOFABSOLUTEXPLUS(n) \
  value1 = r6502read(address); \
  r6502adc(value1);

#define ADC_ABSYP(n) \
  TRACE("ADC_ABSYP") \
  ADDRESSOFABSOLUTEYPLUS(n) \
  value1 = r6502read(address); \
  r6502adc(value1);

#define ADC_IMMED(n) \
  TRACE("ADC_IMMED") \
  r6502adc(n);

#define ADC_POSTI(n) \
  TRACE("ADC_POSTI") \
  ADDRESSOFPOSTINDEXEDY(n) \
  value1 = r6502read(address); \
  r6502adc(value1);

#define ADC_ZEROP(n) \
  TRACE("ADC_ZEROP") \
  value1 = memory[n]; \
  r6502adc(value1);

#define ADC_ZEROX(n) \
  TRACE("ADC_ZEROX") \
  address = ((n + r6502_x) & 0xFF); \
  value1 = memory[address]; \
  r6502adc(value1);

#define AND_IMMED(n) \
  TRACE("AND_IMMED") \
  r6502_a &= n; \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define AND_ZEROP(n) \
  TRACE("AND_ZEROP") \
  value1 = memory[n]; \
  r6502_a &= value1; \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define AND_POSTI(n) \
  TRACE("AND_POSTI") \
  ADDRESSOFPOSTINDEXEDY(n) \
  r6502_a &= r6502read(address); \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define AND_ABSYP(n) \
    TRACE("AND_ABSYP") \
    ADDRESSOFABSOLUTEYPLUS(n) \
    r6502_a &= r6502read(address); \
    SETNFLAG(r6502_a) \
    SETZFLAG(r6502_a) \

#define ASL \
  TRACE("ASL") \
  if (r6502_a & 0x80) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  r6502_a = (r6502_a << 1); \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define ASL_ZEROP(n) \
  TRACE("ASL_ZEROP") \
  value1 = memory[n]; \
  if (value1 & 0x80) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  value2 = (value1 << 1); \
  r6502write(n, value2); \
  SETNFLAG(value2) \
  SETZFLAG(value2)

#define BCC(n) \
  TRACE("BCC") \
  if (!(r6502_ps & CFLAG)) \
    goto n;

#define BCS(n) \
  TRACE("BCS") \
  if (r6502_ps & CFLAG) \
    goto n;

#define BEQ(n) \
  TRACE("BEQ") \
  if (r6502_ps & ZFLAG) \
    goto n;

#define BMI(n) \
  TRACE("BMI") \
  if (r6502_ps & NFLAG) \
    goto n;

#define BNE(n) \
  TRACE("BNE") \
  if (!(r6502_ps & ZFLAG)) \
    goto n;

#define BPL(n) \
  TRACE("BPL") \
  if (!(r6502_ps & NFLAG)) \
    goto n;

#define BIT_ZEROP(n) \
  TRACE("BIT_ZEROP") \
  value1 = memory[n]; \
  value2 = (r6502_a & value1); \
  SETNFLAG(value1) \
  SETVFLAG(value1) \
  SETZFLAG(value2)

#define BRK \
  TRACE("BRK") \
  //assert(!"BRK unhandled");

#define CLC \
  TRACE("CLC") \
  CLEARFLAG(CFLAG);

#define CLD \
  TRACE("CLD") \
  CLEARFLAG(DFLAG);

#define CMP_ABSOL(n) \
  TRACE("CMP_ABSOL") \
  value1 = r6502read(n); \
  value2 = (r6502_a - value1); \
  if (r6502_a >= value1) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  SETNFLAG(value2) \
  SETZFLAG(value2)

#define CMP_ABSXP(n) \
  TRACE("CMP_ABSXP") \
  ADDRESSOFABSOLUTEXPLUS(n) \
  value1 = r6502read(address); \
  value2 = (r6502_a - value1); \
  if (r6502_a >= value1) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  SETNFLAG(value2) \
  SETZFLAG(value2)

#define CMP_ABSYP(n) \
  TRACE("CMP_ABSYP") \
  ADDRESSOFABSOLUTEYPLUS(n) \
  value1 = r6502read(address); \
  value2 = (r6502_a - value1); \
  if (r6502_a >= value1) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  SETNFLAG(value2) \
  SETZFLAG(value2)

#define CMP_IMMED(n) \
  TRACE("CMP_IMMED") \
  value2 = (r6502_a - n); \
  if (r6502_a >= n) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  SETNFLAG(value2) \
  SETZFLAG(value2)

#define CMP_ZEROP(n) \
  TRACE("CMP_ZEROP") \
  value1 = memory[n]; \
  value2 = (r6502_a - value1); \
  if (r6502_a >= value1) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  SETZFLAG(value2) \
  SETNFLAG(value2)

#define CMP_POSTI(n) \
  TRACE("CMP_POSTI") \
  ADDRESSOFPOSTINDEXEDY(n) \
  value1 = r6502read(address); \
  value2 = (r6502_a - value1); \
  if (r6502_a >= value1) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  SETZFLAG(value2) \
  SETNFLAG(value2)

#define CPX_IMMED(n) \
  TRACE("CPX_IMMED") \
  value2 = (r6502_x - n); \
  if (r6502_x >= n) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  SETNFLAG(value2) \
  SETZFLAG(value2)

#define CPX_ZEROP(n) \
  TRACE("CPX_ZEROP") \
  value1 = memory[n]; \
  value2 = (r6502_x - value1); \
  if (r6502_x >= value1) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  SETZFLAG(value2) \
  SETNFLAG(value2)

#define CPX_ABSOL(n) \
  TRACE("CPX_ABSOL") \
  value1 = r6502read(n); \
  value2 = (r6502_x - value1); \
  if (r6502_x >= value1) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  SETNFLAG(value2) \
  SETZFLAG(value2)

#define CPY_IMMED(n) \
  TRACE("CPY_IMMED") \
  value2 = (r6502_y - n); \
  if (r6502_y >= n) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  SETNFLAG(value2) \
  SETZFLAG(value2)

#define CPY_ZEROP(n) \
  TRACE("CPY_ZEROP") \
  value1 = memory[n]; \
  value2 = (r6502_y - value1); \
  if (r6502_y >= value1) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  SETZFLAG(value2) \
  SETNFLAG(value2)

#define CPY_ABSOL(n) \
  TRACE("CPY_ABSOL") \
  value1 = r6502read(n); \
  value2 = (r6502_y - value1); \
  if (r6502_y >= value1) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  SETNFLAG(value2) \
  SETZFLAG(value2)

#define DEC_ABSOL(n) \
  TRACE("DEC_ABSOL") \
  value1 = r6502read(n); \
  value1--; \
  r6502write(n,value1); \
  SETNFLAG(value1) \
  SETZFLAG(value1)

#define DEC_ABSXP(n) \
  TRACE("DEC_ABSXP") \
  ADDRESSOFABSOLUTEXPLUS(n) \
  value1 = r6502read(address); \
  value1--; \
  r6502write(address,value1); \
  SETZFLAG(value1) \
  SETNFLAG(value1)

#define DEC_ZEROP(n) \
  TRACE("DEC_ZEROP") \
  value1 = memory[n]; \
  value1--; \
  r6502write(n, value1); \
  SETNFLAG(value1) \
  SETZFLAG(value1)

#define DEC_ZEROX(n) \
  TRACE("DEC_ZEROX") \
  address = ((n + r6502_x) & 0xFF); \
  value1 = memory[address]; \
  value1--; \
  r6502write(address, value1); \
  SETNFLAG(value1) \
  SETZFLAG(value1)

#define DEX \
  TRACE("DEX") \
  r6502_x--; \
  SETNFLAG(r6502_x) \
  SETZFLAG(r6502_x)

#define DEY \
  TRACE("DEY") \
  r6502_y--; \
  SETNFLAG(r6502_y) \
  SETZFLAG(r6502_y)

#define EOR_IMMED(n) \
  TRACE("EOR_IMMED") \
  r6502_a ^= n; \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define EOR_ZEROP(n) \
  TRACE("EOR_ZEROP") \
  r6502_a ^= memory[n]; \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define EOR_POSTI(n) \
  TRACE("EOR_POSTI") \
  ADDRESSOFPOSTINDEXEDY(n) \
  r6502_a ^= r6502read(address); \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define INC_ABSOL(n) \
  TRACE("INC_ABSOL") \
  value1 = r6502read(n); \
  value1++; \
  r6502write(n,value1); \
  SETNFLAG(value1) \
  SETZFLAG(value1)

#define INC_ABSXP(n) \
  TRACE("INC_ABSXP") \
  ADDRESSOFABSOLUTEXPLUS(n) \
  value1 = r6502read(address); \
  value1++; \
  r6502write(address,value1); \
  SETZFLAG(value1) \
  SETNFLAG(value1)

#define INC_ZEROP(n) \
  TRACE("INC_ZEROP") \
  value1 = memory[n]; \
  value1++; \
  r6502write(n, value1); \
  SETNFLAG(value1) \
  SETZFLAG(value1)

#define INC_ZEROX(n) \
  TRACE("INC_ZEROX") \
  address = ((n + r6502_x) & 0xFF); \
  value1 = memory[address]; \
  value1++; \
  r6502write(address, value1); \
  SETNFLAG(value1) \
  SETZFLAG(value1)

#define INX \
  TRACE("INX") \
  r6502_x++; \
  SETNFLAG(r6502_x) \
  SETZFLAG(r6502_x)

#define INY \
  TRACE("INY") \
  r6502_y++; \
  SETNFLAG(r6502_y) \
  SETZFLAG(r6502_y)

// Some programs JMP or BNE to an address at which a subroutine resides. This means that when the
// code is executed and comes across the RTS for that subroutine, it will actually exit the subroutine
// from where the orginal JMP or BNE was invoked. Since we want to keep code to a minimum, we simulate
// this by calling the function that corresponds to our JSR subroutine and follow that with a C/C++ return
// statement.

// JMP_SIMUL simulates the process of doing a JMP_ABSOL to a subroutine address. It just calls
// the function and relies on the next action to be a return. Note that we don't push anything onto the stack.
// It also outputs the proper JMP_ABSOL in the trace.
#define JMP_SIMUL(n) \
  TRACE("JMP_ABSOL") \
  n();

// Ordinay JMP_ABSOL used with addresses that do not correspond to subroutines.
#define JMP_ABSOL(n) \
  TRACE("JMP_ABSOL") \
  goto n;

// We also need to simulate branches to addresses where subroutines reside. This is more complicated because
// a branch makes a decision and we need it to carry on if no decision is made.
// The following two MACROS NOTRACE_JMP_ABSOL and NOTRACE_JMP_SIMUL are used only in this situation.
// Neither of them output a trace.
#define NOTRACE_JMP_ABSOL(n) \
  goto n;

#define NOTRACE_JMP_SIMUL(n) \
  n();

#define JMP_INDIRECT(n) \
  TRACE("JMP_INDIRECT") \
  r6502jmpindirect(n);

#define JSR_ABSOLOR(n, r) \
  TRACE("JSR_ABSOL") \
  STACK_PUSH(((r-1) & 0xFF00)>>8); \
  STACK_PUSH((r-1) & 0x00FF); \
  n();

#define LDA_ABSOL(n) \
  TRACE("LDA_ABSOL") \
  r6502_a = r6502read(n); \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define LDA_ABSXP(n) \
  TRACE("LDA_ABSXP") \
  ADDRESSOFABSOLUTEXPLUS(n) \
  r6502_a = r6502read(address); \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define LDA_ABSYP(n) \
  TRACE("LDA_ABSYP") \
  ADDRESSOFABSOLUTEYPLUS(n) \
  r6502_a = r6502read(address); \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define LDA_IMMED(n) \
  TRACE("LDA_IMMED") \
  r6502_a = n; \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define LDA_POSTI(n) \
  TRACE("LDA_POSTI") \
  ADDRESSOFPOSTINDEXEDY(n) \
  r6502_a = r6502read(address); \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define LDA_PREIN(n) \
  TRACE("LDA_PREIN") \
  ADDRESSOFPREINDEXED(n) \
  r6502_a = r6502read(address); \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define LDA_ZEROP(n) \
  TRACE("LDA_ZEROP") \
  r6502_a = memory[n]; \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define LDA_ZEROX(n) \
  TRACE("LDA_ZEROX") \
  address = ((n + r6502_x) & 0xFF); \
  r6502_a = memory[address]; \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define LDX_ABSOL(n) \
  TRACE("LDX_ABSOL") \
  r6502_x = r6502read(n); \
  SETNFLAG(r6502_x) \
  SETZFLAG(r6502_x)

#define LDX_ABSYP(n) \
  TRACE("LDX_ABSYP") \
  ADDRESSOFABSOLUTEYPLUS(n) \
  r6502_x = r6502read(address); \
  SETNFLAG(r6502_x) \
  SETZFLAG(r6502_x)

#define LDX_IMMED(n) \
  TRACE("LDX_IMMED") \
  r6502_x = n; \
  SETNFLAG(r6502_x) \
  SETZFLAG(r6502_x)

#define LDX_ZEROP(n) \
  TRACE("LDX_ZEROP") \
  r6502_x = memory[n]; \
  SETNFLAG(r6502_x) \
  SETZFLAG(r6502_x)

#define LDX_ZEROY(n) \
  TRACE("LDX_ZEROY") \
  address = ((n + r6502_y) & 0xFF); \
  r6502_x = memory[address]; \
  SETNFLAG(r6502_x) \
  SETZFLAG(r6502_x)

#define LDY_ABSOL(n) \
  TRACE("LDY_ABSOL") \
  r6502_y = r6502read(n); \
  SETNFLAG(r6502_y) \
  SETZFLAG(r6502_y)

#define LDY_ABSXP(n) \
  TRACE("LDY_ABSXP") \
  ADDRESSOFABSOLUTEXPLUS(n) \
  r6502_y = r6502read(address); \
  SETNFLAG(r6502_y) \
  SETZFLAG(r6502_y)

#define LDY_IMMED(n) \
  TRACE("LDY_IMMED") \
  r6502_y = n; \
  SETNFLAG(r6502_y) \
  SETZFLAG(r6502_y)

#define LDY_ZEROP(n) \
  TRACE("LDY_ZEROP") \
  r6502_y = memory[n]; \
  SETNFLAG(r6502_y) \
  SETZFLAG(r6502_y)

#define LDY_ZEROX(n) \
  TRACE("LDY_ZEROX") \
  address = ((n + r6502_x) & 0xFF); \
  r6502_y = memory[address]; \
  SETNFLAG(r6502_y) \
  SETZFLAG(r6502_y)

#define LSR \
  TRACE("LSR") \
  if (r6502_a & 0x01) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  r6502_a = (r6502_a >> 1); \
  CLEARFLAG(NFLAG); \
  SETZFLAG(r6502_a)

#define LSR_ABSOL(n) \
  TRACE("LSR_ABSOL") \
  value1 = r6502read(n); \
  if (value1 & 0x01) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  value1 = (value1 >> 1); \
  r6502write(n,value1); \
  CLEARFLAG(NFLAG) \
  SETZFLAG(value1)

#define LSR_ABSXP(n) \
  TRACE("LSR_ABSXP") \
  ADDRESSOFABSOLUTEXPLUS(n) \
  value1 = r6502read(address); \
  if (value1 & 0x01) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  value1 = (value1 >> 1); \
  r6502write(address,value1); \
  CLEARFLAG(NFLAG) \
  SETZFLAG(value1)

#define LSR_ZEROP(n) \
  TRACE("LSR_ZEROP") \
  value1 = memory[n]; \
  if (value1 & 0x01) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  value1 = (value1 >> 1); \
  r6502write(n, value1); \
  CLEARFLAG(NFLAG) \
  SETZFLAG(value1)

#define ORA_IMMED(n) \
  TRACE("ORA_IMMED") \
  r6502_a |= n; \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define ORA_ZEROP(n) \
  TRACE("ORA_ZEROP") \
  r6502_a |= memory[n]; \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define ORA_ABSOL(n) \
  TRACE("ORA_ABSOL") \
  r6502_a |= r6502read(n); \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define ORA_ABSYP(n) \
  TRACE("ORA_ABSYP") \
  ADDRESSOFABSOLUTEYPLUS(n) \
  r6502_a |= r6502read(address); \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define ORA_POSTI(n) \
  TRACE("ORA_POSTI") \
  ADDRESSOFPOSTINDEXEDY(n) \
  r6502_a |= r6502read(address); \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define PHA \
  TRACE("PHA") \
  STACK_PUSH(r6502_a)

#define PHP \
  TRACE("PHP") \
  value1 = PROCESSORSTATUS(BFLAG) \
  STACK_PUSH(value1)

#define PLA \
  TRACE("PLA") \
  STACK_POP(r6502_a) \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define PLP \
  TRACE("PLP") \
  STACK_POP(r6502_ps);

#define ROL \
  TRACE("ROL") \
  value2 = (r6502_ps & CFLAG); \
  if (r6502_a & 0x80) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  r6502_a = (r6502_a << 1); \
  if (value2) \
    r6502_a |= 1; \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define ROR \
  TRACE("ROR") \
  value2 = (r6502_ps & CFLAG); \
  if(r6502_a & 1) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  r6502_a = (r6502_a >> 1); \
  if(value2) \
    r6502_a |= 0x80; \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define ROR_ABSOL(n) \
  TRACE("ROR_ABSOL") \
  value1 = r6502read(n); \
  value2 = (r6502_ps & CFLAG); \
  if (value1 & 1) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  value1 = (value1 >> 1); \
  if (value2) \
    value1 |= 0x80; \
  r6502write(n, value1); \
  SETNFLAG(value1) \
  SETZFLAG(value1)

#define ROR_ZEROP(n) \
  TRACE("ROR_ZEROP") \
  value1 = r6502read(n); \
  value2 = (r6502_ps & CFLAG); \
  if (value1 & 1) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  value1 = (value1 >> 1); \
  if (value2) \
    value1 |= 0x80; \
  r6502write(n, value1); \
  SETNFLAG(value1) \
  SETZFLAG(value1)


#define ROL_ZEROP(n) \
  TRACE("ROL_ZEROP") \
  value1 = memory[n]; \
  value2 = (r6502_ps & CFLAG); \
  if (value1 & 0x80) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  value1 = (value1 << 1); \
  if (value2) \
    value1 |= 1; \
  r6502write(n, value1); \
  SETNFLAG(value1) \
  SETZFLAG(value1)

#define ROL_ABSOL(n) \
  TRACE("ROL_ABSOL") \
  value1 = r6502read(n); \
  value2 = (r6502_ps & CFLAG); \
  if (value1 & 0x80) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  value1 = (value1 << 1); \
  if (value2) \
    value1 |= 1; \
  r6502write(n, value1); \
  SETNFLAG(value1) \
  SETZFLAG(value1)

#define RLA_ABSOL(n) \
  TRACE("RLA_ABSOL") \
  value1 = (r6502_ps & CFLAG); \
  value2 = r6502read(n); \
  if (value2 & 0x80) \
    SETFLAG(CFLAG) \
  else \
    CLEARFLAG(CFLAG) \
  value2 = (value2 << 1); \
  if (value1) \
    value2 |= 1; \
  r6502write(n,value2); \
  r6502_a &= value2; \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define RTS \
  TRACE("RTS") \
  STACK_POP(nlo) \
  STACK_POP(nhi)

#define HACK_RTS \
  STACK_POP(nlo) \
  STACK_POP(nhi)

#define SBCxx(n) \
  TRACE("SBCxx") \
  r6502sbc(n);

#define SBC_ABSOL(n) \
  TRACE("SBC_ABSOL") \
  value1 = r6502read(n); \
  r6502sbc(value1);

#define SBC_ABSXP(n) \
  TRACE("SBC_ABSXP") \
  ADDRESSOFABSOLUTEXPLUS(n) \
  value1 = r6502read(address); \
  r6502sbc(value1);

#define SBC_ABSYP(n) \
  TRACE("SBC_ABSYP") \
  ADDRESSOFABSOLUTEYPLUS(n) \
  value1 = r6502read(address); \
  r6502sbc(value1);

#define SBC_IMMED(n) \
  TRACE("SBC_IMMED") \
  r6502sbc(n);

#define SBC_POSI(n) \
  TRACE("SBC_POSI") \
  ADDRESSOFPOSTINDEXEDY(n) \
  value1 = r6502read(address); \
  r6502sbc(value1);

#define SBC_ZEROP(n) \
  TRACE("SBC_ZEROP") \
  value1 = memory[n]; \
  r6502sbc(value1);

#define SEC \
  TRACE("SEC") \
  SETFLAG(CFLAG);

#define SED \
  TRACE("SED") \
  SETFLAG(DFLAG);

#define STA_ABSOL(n) \
  TRACE("STA_ABSOL") \
  r6502write(n,r6502_a);

#define STA_ABSXP(n) \
  TRACE("STA_ABSXP") \
  ADDRESSOFABSOLUTEXPLUS(n) \
  r6502write(address,r6502_a);

#define STA_ABSYP(n) \
  TRACE("STA_ABSYP") \
  ADDRESSOFABSOLUTEYPLUS(n) \
  r6502write(address,r6502_a);

#define STA_POSTI(n) \
  TRACE("STA_POSTI") \
  ADDRESSOFPOSTINDEXEDY(n) \
  r6502write(address,r6502_a);

#define STA_ZEROP(n) \
  TRACE("STA_ZEROP") \
  r6502write(n, r6502_a);

#define STA_ZEROX(n) \
  TRACE("STA_ZEROX") \
  address = ((n + r6502_x) & 0xFF); \
  r6502write(address, r6502_a);

#define STX_ABSOL(n) \
  TRACE("STX_ABSOL") \
   r6502write(n,r6502_x);

#define STX_ZEROP(n) \
  TRACE("STX_ZEROP") \
  r6502write(n, r6502_x);

#define STX_ZEROY(n) \
  TRACE("STX_ZEROY") \
  address = ((n + r6502_y) & 0xFF); \
  r6502write(n, r6502_x);

#define STY_ABSOL(n) \
  TRACE("STY_ABSOL") \
   r6502write(n, r6502_y);

#define STY_ZEROP(n) \
  TRACE("STY_ZEROP") \
  r6502write(n, r6502_y);

#define TAX \
  TRACE("TAX") \
  r6502_x = r6502_a; \
  SETNFLAG(r6502_x) \
  SETZFLAG(r6502_x)

#define TAY \
  TRACE("TAY") \
  r6502_y = r6502_a; \
  SETNFLAG(r6502_y) \
  SETZFLAG(r6502_y)

#define TSX \
  TRACE("TSX") \
  r6502_x = r6502_sp; \
  SETNFLAG(r6502_x) \
  SETZFLAG(r6502_x)

#define TXA \
  TRACE("TXA") \
  r6502_a = r6502_x; \
  SETNFLAG(r6502_a) \
  SETZFLAG(r6502_a)

#define TXS \
  TRACE("TXS") \
  r6502_sp = r6502_x;

#define TYA \
  TRACE("TYA") \
  r6502_a = r6502_y; \
  SETZFLAG(r6502_a) \
  SETNFLAG(r6502_a)


#else
extern unsigned char _PROCESSORSTATUS(unsigned char n);
extern void _STACK_PUSH(unsigned char n);
extern unsigned char _STACK_POP(void);
extern void _SETFLAG(unsigned char nflag);
extern void _CLEARFLAG(unsigned char nflag);
extern void _SETNFLAG(unsigned char n);
extern void _CLEARFLAG(unsigned char nflag);
extern void _SETNFLAG(unsigned char n);
extern void _SETVFLAG(unsigned char n);
extern void _SETBFLAG(unsigned char n);
extern void _SETIFLAG(unsigned char n);
extern void _SETZFLAG(unsigned char n);
extern void _ADDRESSOFABSOLUTEXPLUS(unsigned int n);
extern void _ADDRESSOFABSOLUTEYPLUS(unsigned int n);
extern void _ADDRESSOFPOSTINDEXEDY(unsigned int n);
extern void _ADDRESSOFPREINDEXED(unsigned int n);
extern void _ADCxx(unsigned char n);
extern void _ADC_ABSOL(unsigned int n);
extern void _ADC_ABSXP(unsigned int n);
extern void _ADC_ABSYP(unsigned int n);
extern void _ADC_IMMED(unsigned char n);
extern void _ADC_POSTI(unsigned int n);
extern void _ADC_ZEROP(unsigned char n);
extern void _ADC_ZEROX(unsigned char n);
extern void _AND_IMMED(unsigned char n);
extern void _AND_ZEROP(unsigned char n);
extern void _AND_POSTI(unsigned int n);
extern void _AND_ABSYP(unsigned int n);
extern void _ASL(void);
extern void _ASL_ZEROP(unsigned char n);
extern void _BIT_ZEROP(unsigned char n);
extern void _BRK(void);
extern void _CLC(void);
extern void _CLD(void);
extern void _CMP_ABSOL(unsigned int n);
extern void _CMP_ABSXP(unsigned int n);
extern void _CMP_ABSYP(unsigned int n);
extern void _CMP_IMMED(unsigned char n);
extern void _CMP_ZEROP(unsigned char n);
extern void _CMP_POSTI(unsigned int n);
extern void _CPX_IMMED(unsigned char n);
extern void _CPX_ZEROP(unsigned char n);
extern void _CPX_ABSOL(unsigned int n);
extern void _CPY_IMMED(unsigned char n);
extern void _CPY_ZEROP(unsigned char n);
extern void _CPY_ABSOL(unsigned int n);
extern void _DEC_ABSOL(unsigned int n);
extern void _DEC_ABSXP(unsigned int n);
extern void _DEC_ZEROP(unsigned char n);
extern void _DEC_ZEROX(unsigned char n);
extern void _DEX(void);
extern void _DEY(void);
extern void _EOR_IMMED(unsigned char n);
extern void _EOR_ZEROP(unsigned char n);
extern void _EOR_POSTI(unsigned int n);
extern void _INC_ABSOL(unsigned int n);
extern void _INC_ABSXP(unsigned int n);
extern void _INC_ZEROP(unsigned char n);
extern void _INC_ZEROX(unsigned char n);
extern void _INX(void);
extern void _INY(void);
/*
extern void _JMP_SIMUL(unsigned char n);
extern void _JMP_INDIRECT(unsigned char n);
extern void _JSR_ABSOLOR(unsigned char n, unsigned char r);
*/
extern void _LDA_ABSOL(unsigned int n);
extern void _LDA_ABSXP(unsigned int n);
extern void _LDA_ABSYP(unsigned int n);
extern void _LDA_IMMED(unsigned char n);
extern void _LDA_POSTI(unsigned int n);
extern void _LDA_PREIN(unsigned int n);
extern void _LDA_ZEROP(unsigned char n);
extern void _LDA_ZEROX(unsigned char n);
extern void _LDX_ABSOL(unsigned int n);
extern void _LDX_ABSYP(unsigned int n);
extern void _LDX_IMMED(unsigned char n);
extern void _LDX_ZEROP(unsigned char n);
extern void _LDX_ZEROY(unsigned char n);
extern void _LDY_ABSOL(unsigned int n);
extern void _LDY_ABSXP(unsigned int n);
extern void _LDY_IMMED(unsigned char n);
extern void _LDY_ZEROP(unsigned char n);
extern void _LDY_ZEROX(unsigned char n);
extern void _LSR(void);
extern void _LSR_ABSOL(unsigned int n);
extern void _LSR_ABSXP(unsigned int n);
extern void _LSR_ZEROP(unsigned char n);
extern void _ORA_IMMED(unsigned char n);
extern void _ORA_ZEROP(unsigned char n);
extern void _ORA_ABSOL(unsigned int n);
extern void _ORA_ABSYP(unsigned int n);
extern void _ORA_POSTI(unsigned int n);
extern void _PHA(void);
extern void _PHP(void);
extern void _PLA(void);
extern void _PLP(void);
extern void _ROL(void);
extern void _ROR(void);
extern void _ROR_ABSOL(unsigned int n);
extern void _ROR_ZEROP(unsigned char n);
extern void _ROL_ZEROP(unsigned char n);
extern void _ROL_ABSOL(unsigned int n);
extern void _RLA_ABSOL(unsigned int n);
extern void _RTS(void);
extern void _HACK_RTS(void);
extern void _SBCxx(unsigned char n);
extern void _SBC_ABSOL(unsigned int n);
extern void _SBC_ABSXP(unsigned int n);
extern void _SBC_ABSYP(unsigned int n);
extern void _SBC_IMMED(unsigned char n);
extern void _SBC_POSI(unsigned int n);
extern void _SBC_ZEROP(unsigned char n);
extern void _SEC(void);
extern void _SED(void);
extern void _STA_ABSOL(unsigned int n);
extern void _STA_ABSXP(unsigned int n);
extern void _STA_ABSYP(unsigned int n);
extern void _STA_POSTI(unsigned int n);
extern void _STA_ZEROP(unsigned char n);
extern void _STA_ZEROX(unsigned char n);
extern void _STX_ABSOL(unsigned int n);
extern void _STX_ZEROP(unsigned char n);
extern void _STX_ZEROY(unsigned char n);
extern void _STY_ABSOL(unsigned int n);
extern void _STY_ZEROP(unsigned char n);
extern void _TAX(void);
extern void _TAY(void);
extern void _TSX(void);
extern void _TXA(void);
extern void _TXS(void);
extern void _TYA(void);

#define PROCESSORSTATUS(n) \
    _PROCESSORSTATUS(n);

#define STACK_PUSH(n) \
    _STACK_PUSH(n);

#define STACK_POP(n) \
    n = _STACK_POP();

#define SETFLAG(nflag) \
    _SETFLAG(nflag);

#define CLEARFLAG(nflag) \
    _CLEARFLAG(nflag);

#define SETNFLAG(n) \
    _SETNFLAG(n);

#define SETVFLAG(n) \
    _SETVFLAG(n);

#define SETBFLAG(n) \
    _SETBFLAG(n);

#define SETIFLAG(n) \
    _SETIFLAG(n);

#define SETZFLAG(n) \
    _SETZFLAG(n);

#define ADDRESSOFABSOLUTEXPLUS(n) \
    _ADDRESSOFABSOLUTEXPLUS(n);

#define ADDRESSOFABSOLUTEYPLUS(n) \
    _ADDRESSOFABSOLUTEYPLUS(n);

#define ADDRESSOFPOSTINDEXEDY(n) \
    _ADDRESSOFPOSTINDEXEDY(n);

#define ADDRESSOFPREINDEXED(n) \
    _ADDRESSOFPREINDEXED(n);


/* All other osbyte calls. */
#define OSBYTE \
  osbyte();

/* Keyboard scan from 16 decimal */
#define OSBYTE7A \
  osbyte();

/* Read key with time limit (INKEY) */
#define OSBYTE81 \
  osbyte();

/*Read address of key translation table (low byte).*/
#define OSBYTEAC \
  osbyte();

#define OSWORD \
  osword();

#define OSWRCH \
  oswrch();

#define WAIT_FOR_VERTICAL_SYNC /*implement_me;*/

#define ADCxx(n) \
    _ADCxx(n);

#define ADC_ABSOL(n) \
    _ADC_ABSOL(n);

#define ADC_ABSXP(n) \
    _ADC_ABSXP(n);

#define ADC_ABSYP(n) \
    _ADC_ABSYP(n);

#define ADC_IMMED(n) \
    _ADC_IMMED(n);

#define ADC_POSTI(n) \
    _ADC_POSTI(n);

#define ADC_ZEROP(n) \
    _ADC_ZEROP(n);

#define ADC_ZEROX(n) \
    _ADC_ZEROX(n);

#define AND_IMMED(n) \
    _AND_IMMED(n);

#define AND_ZEROP(n) \
    _AND_ZEROP(n);

#define AND_POSTI(n) \
    _AND_POSTI(n);

#define AND_ABSYP(n) \
    _AND_ABSYP(n);

#define ASL \
    _ASL();

#define ASL_ZEROP(n) \
    _ASL_ZEROP(n);

#define BCC(n) \
  TRACE("BCC") \
  if (!(r6502_ps & CFLAG)) \
    goto n;

#define BCS(n) \
  TRACE("BCS") \
  if (r6502_ps & CFLAG) \
    goto n;

#define BEQ(n) \
  TRACE("BEQ") \
  if (r6502_ps & ZFLAG) \
    goto n;

#define BMI(n) \
  TRACE("BMI") \
  if (r6502_ps & NFLAG) \
    goto n;

#define BNE(n) \
  TRACE("BNE") \
  if (!(r6502_ps & ZFLAG)) \
    goto n;

#define BPL(n) \
  TRACE("BPL") \
  if (!(r6502_ps & NFLAG)) \
    goto n;

#define BIT_ZEROP(n) \
    _BIT_ZEROP(n);

#define BRK \
    _BRK();

#define CLC \
    _CLC();

#define CLD \
    _CLD();

#define CMP_ABSOL(n) \
    _CMP_ABSOL(n);

#define CMP_ABSXP(n) \
    _CMP_ABSXP(n);

#define CMP_ABSYP(n) \
    _CMP_ABSYP(n);

#define CMP_IMMED(n) \
    _CMP_IMMED(n);

#define CMP_ZEROP(n) \
    _CMP_ZEROP(n);

#define CMP_POSTI(n) \
    _CMP_POSTI(n);

#define CPX_IMMED(n) \
    _CPX_IMMED(n);

#define CPX_ZEROP(n) \
    _CPX_ZEROP(n);

#define CPX_ABSOL(n) \
    _CPX_ABSOL(n);

#define CPY_IMMED(n) \
    _CPY_IMMED(n);

#define CPY_ZEROP(n) \
    _CPY_ZEROP(n);

#define CPY_ABSOL(n) \
    _CPY_ABSOL(n);


#define DEC_ABSOL(n) \
    _DEC_ABSOL(n);

#define DEC_ABSXP(n) \
    _DEC_ABSXP(n);

#define DEC_ZEROP(n) \
    _DEC_ZEROP(n);

#define DEC_ZEROX(n) \
    _DEC_ZEROX(n);

#define DEX \
    _DEX();

#define DEY \
    _DEY();

#define EOR_IMMED(n) \
    _EOR_IMMED(n);

#define EOR_ZEROP(n) \
    _EOR_ZEROP(n);

#define EOR_POSTI(n) \
    _EOR_POSTI(n);

#define INC_ABSOL(n) \
    _INC_ABSOL(n);

#define INC_ABSXP(n) \
    _INC_ABSXP(n);

#define INC_ZEROP(n) \
    _INC_ZEROP(n);

#define INC_ZEROX(n) \
    _INC_ZEROX(n);

#define INX \
    _INX();

#define INY \
    _INY();

// Some programs JMP or BNE to an address at which a subroutine resides. This means that when the
// code is executed and comes across the RTS for that subroutine, it will actually exit the subroutine
// from where the orginal JMP or BNE was invoked. Since we want to keep code to a minimum, we simulate
// this by calling the function that corresponds to our JSR subroutine and follow that with a C/C++ return
// statement.

// JMP_SIMUL simulates the process of doing a JMP_ABSOL to a subroutine address. It just calls
// the function and relies on the next action to be a return. Note that we don't push anything onto the stack.
// It also outputs the proper JMP_ABSOL in the trace.
#define JMP_SIMUL(n) \
  TRACE("JMP_ABSOL") \
  n();

// Ordinay JMP_ABSOL used with addresses that do not correspond to subroutines.
#define JMP_ABSOL(n) \
  TRACE("JMP_ABSOL") \
  goto n;

// We also need to simulate branches to addresses where subroutines reside. This is more complicated because
// a branch makes a decision and we need it to carry on if no decision is made.
// The following two MACROS NOTRACE_JMP_ABSOL and NOTRACE_JMP_SIMUL are used only in this situation.
// Neither of them output a trace.
#define NOTRACE_JMP_ABSOL(n) \
  goto n;

#define NOTRACE_JMP_SIMUL(n) \
  n();

#define JMP_INDIRECT(n) \
  TRACE("JMP_INDIRECT") \
  r6502jmpindirect(n);

#define JSR_ABSOLOR(n, r) \
  TRACE("JSR_ABSOL") \
  STACK_PUSH(((r-1) & 0xFF00)>>8); \
  STACK_PUSH((r-1) & 0x00FF); \
  n();

#define LDA_ABSOL(n) \
    _LDA_ABSOL(n);

#define LDA_ABSXP(n) \
    _LDA_ABSXP(n);

#define LDA_ABSYP(n) \
    _LDA_ABSYP(n);

#define LDA_IMMED(n) \
    _LDA_IMMED(n);

#define LDA_POSTI(n) \
    _LDA_POSTI(n);

#define LDA_PREIN(n) \
    _LDA_PREIN(n);

#define LDA_ZEROP(n) \
    _LDA_ZEROP(n);

#define LDA_ZEROX(n) \
    _LDA_ZEROX(n);

#define LDX_ABSOL(n) \
    _LDX_ABSOL(n);

#define LDX_ABSYP(n) \
    _LDX_ABSYP(n);

#define LDX_IMMED(n) \
    _LDX_IMMED(n);

#define LDX_ZEROP(n) \
    _LDX_ZEROP(n);

#define LDX_ZEROY(n) \
    _LDX_ZEROY(n);

#define LDY_ABSOL(n) \
    _LDY_ABSOL(n);

#define LDY_ABSXP(n) \
    _LDY_ABSXP(n);

#define LDY_IMMED(n) \
    _LDY_IMMED(n);

#define LDY_ZEROP(n) \
    _LDY_ZEROP(n);

#define LDY_ZEROX(n) \
    _LDY_ZEROX(n);

#define LSR \
    _LSR();

#define LSR_ABSOL(n) \
    _LSR_ABSOL(n);

#define LSR_ABSXP(n) \
    _LSR_ABSXP(n);

#define LSR_ZEROP(n) \
    _LSR_ZEROP(n);

#define ORA_IMMED(n) \
    _ORA_IMMED(n);

#define ORA_ZEROP(n) \
    _ORA_ZEROP(n);

#define ORA_ABSOL(n) \
    _ORA_ABSOL(n);

#define ORA_ABSYP(n) \
    _ORA_ABSYP(n);

#define ORA_POSTI(n) \
    _ORA_POSTI(n);

#define PHA \
    _PHA();

#define PHP \
    _PHP();

#define PLA \
    _PLA();

#define PLP \
    _PLP();

#define ROL \
    _ROL();

#define ROR \
    _ROR();

#define ROR_ABSOL(n) \
    _ROR_ABSOL(n);

#define ROR_ZEROP(n) \
    _ROR_ZEROP(n);

#define ROL_ZEROP(n) \
    _ROL_ZEROP(n);

#define ROL_ABSOL(n) \
    _ROL_ABSOL(n);

#define RLA_ABSOL(n) \
    _RLA_ABSOL(n);

#define RTS \
    _RTS();

#define HACK_RTS \
    _HACK_RTS();

#define SBCxx(n) \
    _SBCxx(n);

#define SBC_ABSOL(n) \
    _SBC_ABSOL(n);

#define SBC_ABSXP(n) \
    _SBC_ABSXP(n);

#define SBC_ABSYP(n) \
    _SBC_ABSYP(n);

#define SBC_IMMED(n) \
    _SBC_IMMED(n);

#define SBC_POSI(n) \
    _SBC_POSI(n);

#define SBC_ZEROP(n) \
    _SBC_ZEROP(n);

#define SEC \
    _SEC();

#define SED \
    _SED();

#define STA_ABSOL(n) \
    _STA_ABSOL(n);

#define STA_ABSXP(n) \
    _STA_ABSXP(n);

#define STA_ABSYP(n) \
    _STA_ABSYP(n);

#define STA_POSTI(n) \
    _STA_POSTI(n);

#define STA_ZEROP(n) \
    _STA_ZEROP(n);

#define STA_ZEROX(n) \
    _STA_ZEROX(n);

#define STX_ABSOL(n) \
    _STX_ABSOL(n);

#define STX_ZEROP(n) \
    _STX_ZEROP(n);

#define STX_ZEROY(n) \
    _STX_ZEROY(n);

#define STY_ABSOL(n) \
    _STY_ABSOL(n);

#define STY_ZEROP(n) \
    _STY_ZEROP(n);

#define TAX \
    _TAX();

#define TAY \
    _TAY();

#define TSX \
    _TSX();

#define TXA \
    _TXA();

#define TXS \
    _TXS();

#define TYA \
    _TYA();

#endif


#ifdef __cplusplus
}
#endif

#endif // BBC_R6502LIB_H_INCLUDED

