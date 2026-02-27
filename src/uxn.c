#include "uxn.h"

uint8_t ram[0x10000], dev[0x100], ptr[2], stk[2][0x100];

/* clang-format off */
#define OPC(opc, A, B) {\
	case 0x00|opc: {const Uint8 d=0,r=0;A B} break;\
	case 0x20|opc: {const Uint8 d=1,r=0;A B} break;\
	case 0x40|opc: {const Uint8 d=0,r=1;A B} break;\
	case 0x60|opc: {const Uint8 d=1,r=1;A B} break;\
	case 0x80|opc: {const Uint8 d=0,r=0,k=ptr[0];A ptr[0]=k;B} break;\
	case 0xa0|opc: {const Uint8 d=1,r=0,k=ptr[0];A ptr[0]=k;B} break;\
	case 0xc0|opc: {const Uint8 d=0,r=1,k=ptr[1];A ptr[1]=k;B} break;\
	case 0xe0|opc: {const Uint8 d=1,r=1,k=ptr[1];A ptr[1]=k;B} break;}
#define REM ptr[r] -= 1 + d;
#define DEC(m) stk[m][--ptr[m]]
#define INC(m) stk[m][ptr[m]++]
#define IMM(o) o = ram[pc++] << 8, o |= ram[pc++];
#define MOV(o) { pc = d ? (unsigned short)o : pc + (Sint8)o; }
#define POx(o,m) o = DEC(r); if(m) o |= DEC(r) << 8;
#define PUx(i,m) if(m) c = (i), INC(r) = c >> 8, INC(r) = c; else INC(r) = i;
#define GOT(o) if(d) o[1] = DEC(r); o[0] = DEC(r);
#define PUT(i) PUx(i[0],0) if(d) { PUx(i[1],0) }
#define DEO(o,v) emu_deo(o, v[0]); if(d) emu_deo(o + 1, v[1]);
#define DEI(i,v) v[0] = emu_dei(i); if(d) v[1] = emu_dei(i + 1); PUT(v)
#define POK(o,v,m) ram[o] = v[0]; if(d) ram[(o + 1) & m] = v[1];
#define PEK(i,v,m) v[0] = ram[i]; if(d) v[1] = ram[(i + 1) & m]; PUT(v)

unsigned int
uxn_eval(unsigned short pc)
{
	unsigned int a, b, c, x[2], y[2], z[2];
	for(;;) {
	switch(ram[pc++]) {
	/* BRK */ case 0x00: return 1;
	/* JCI */ case 0x20: if(DEC(0)) { IMM(c) pc += c; } else pc += 2; break;
	/* JMI */ case 0x40: IMM(c) pc += c; break;
	/* JSI */ case 0x60: IMM(c) INC(1) = pc >> 8, INC(1) = pc, pc += c; break;
	/* LI2 */ case 0xa0: INC(0) = ram[pc++]; /* fall-through */
	/* LIT */ case 0x80: INC(0) = ram[pc++]; break;
	/* L2r */ case 0xe0: INC(1) = ram[pc++]; /* fall-through */
	/* LIr */ case 0xc0: INC(1) = ram[pc++]; break;
	/* INC */ OPC(0x01,POx(a,d),PUx(a + 1, d))
	/* POP */ OPC(0x02,REM,{})
	/* NIP */ OPC(0x03,GOT(x) REM,PUT(x))
	/* SWP */ OPC(0x04,GOT(x) GOT(y),PUT(x) PUT(y))
	/* ROT */ OPC(0x05,GOT(x) GOT(y) GOT(z),PUT(y) PUT(x) PUT(z))
	/* DUP */ OPC(0x06,GOT(x),PUT(x) PUT(x))
	/* OVR */ OPC(0x07,GOT(x) GOT(y),PUT(y) PUT(x) PUT(y))
	/* EQU */ OPC(0x08,POx(a,d) POx(b,d),PUx(b == a,0))
	/* NEQ */ OPC(0x09,POx(a,d) POx(b,d),PUx(b != a,0))
	/* GTH */ OPC(0x0a,POx(a,d) POx(b,d),PUx(b > a,0))
	/* LTH */ OPC(0x0b,POx(a,d) POx(b,d),PUx(b < a,0))
	/* JMP */ OPC(0x0c,POx(a,d),MOV(a))
	/* JCN */ OPC(0x0d,POx(a,d) POx(b,0),if(b) MOV(a))
	/* JSR */ OPC(0x0e,POx(a,d),INC(!r) = pc >> 8; INC(!r) = pc; MOV(a))
	/* STH */ OPC(0x0f,GOT(x),INC(!r) = x[0]; if(d) INC(!r) = x[1];)
	/* LDZ */ OPC(0x10,POx(a,0),PEK(a, x, 0xff))
	/* STZ */ OPC(0x11,POx(a,0) GOT(y),POK(a, y, 0xff))
	/* LDR */ OPC(0x12,POx(a,0),PEK(pc + (Sint8)a, x, 0xffff))
	/* STR */ OPC(0x13,POx(a,0) GOT(y),POK(pc + (Sint8)a, y, 0xffff))
	/* LDA */ OPC(0x14,POx(a,1),PEK(a, x, 0xffff))
	/* STA */ OPC(0x15,POx(a,1) GOT(y),POK(a, y, 0xffff))
	/* DEI */ OPC(0x16,POx(a,0),DEI(a, x))
	/* DEO */ OPC(0x17,POx(a,0) GOT(y),DEO(a, y))
	/* ADD */ OPC(0x18,POx(a,d) POx(b,d),PUx(b + a, d))
	/* SUB */ OPC(0x19,POx(a,d) POx(b,d),PUx(b - a, d))
	/* MUL */ OPC(0x1a,POx(a,d) POx(b,d),PUx(b * a, d))
	/* DIV */ OPC(0x1b,POx(a,d) POx(b,d),PUx(a ? b / a : 0, d))
	/* AND */ OPC(0x1c,POx(a,d) POx(b,d),PUx(b & a, d))
	/* ORA */ OPC(0x1d,POx(a,d) POx(b,d),PUx(b | a, d))
	/* EOR */ OPC(0x1e,POx(a,d) POx(b,d),PUx(b ^ a, d))
	/* SFT */ OPC(0x1f,POx(a,0) POx(b,d),PUx(b >> (a & 0xf) << (a >> 4), d))
	}} return 0;
}
/* clang-format on */
