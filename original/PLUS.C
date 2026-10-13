#include <Goliat1\Goliat1.h>
#include <Goliat1\Utility\Utility.h>

#define  RC8255  0xF6

byte	flag = 0x0D;
byte	far *Aux;

extern far pascal Prog1();
extern far pascal Prog2();
extern far pascal Prog3();
extern far pascal Prog4();

boolean static SegOk(word seg)
  {
    byte  far *p;
    byte  q;

    Aux = p = (byte far *)((unsigned long)seg << 16 + 0xFE00);
    q   = *p;
    *p  = !*p;
    return(q == *p);
  }

boolean static DefinirSeg()
  {
    return(SegOk(0xD000) | SegOk(0xC000) | SegOk(0xB800) | SegOk(0x8000) | SegOk(0x9000));
  }

void static StopZ80()
  {
    outp(RC8255,0xB4);
    outp(RC8255,0x0F);
    outp(RC8255,flag);
  }

void static ContinueZ80()
  {
    outp(RC8255,0x0E);
    _Delay(100);
  }

void static SendProgram(byte far prog[], word count)
  {
    word  i;

    StopZ80();
    for (i = 0; i < count; i++)
      Aux[i] = prog[i];
    ContinueZ80();
  }

boolean ResetPLUS80()
  {
    SendProgram((byte far *)Prog1,24);
    return(*Aux == 0xC9);
  }
