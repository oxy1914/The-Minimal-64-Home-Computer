; ---------------------------------------
; VGA demo drawing 16 x 16 bubble sprites
; by C. Herting (slu4) 2022
; ---------------------------------------
#org 0x8000

start:          LDI 0xfe STA 0xffff
                JPS _Clear                                                          ; clear VGA

  rloop:        JPS _Random ANI 1 STA table+0
  redox:        JPS _Random CPI 192 BCS redox
                  LSL ORB table+0 LDI 0 ROL STA table+1
  redoy:        JPS _Random CPI 224 BCS redoy
                  STA table+2

  drawit:       LDA table+0 PHS
                LDA table+1 PHS
                LDA table+2 PHS
                JPS DrawSprite PLS PLS PLS

                JPA rloop

ptr:            0xffff

; ------------------------------------------------
; Draws a 16x16 pixel sprite at the given position
; push: x_lsb, x_msb, y
; pull: #, #, #
; modifies: X
; ------------------------------------------------
DrawSprite:     LDS 3 LL6 STA addr+0                          ; use ypos
                LDS 3 RL7 ANI 63 ADI 0xc3 STA addr+1
                LDS 4 DEC LDS 5                               ; add xpos
                RL6 ANI 63 ADI 12 ORB addr+0                  ; preprare target address
                LDS 5 ANI 7 STA shift                         ; calc bit pos
                LDI <data STA dptr+0 LDI >data STA dptr+1     ; data is hard-coded
lineloop:       LDR dptr STA buffer+0 INW dptr
                LDR dptr STA buffer+1 CLB buffer+2
                LXA shift DEX BCC shiftdone                   ; shift that buffer to pixel position
  shiftloop:    LLW buffer+0 RLB buffer+2 DEX BCS shiftloop
  shiftdone:    NEB mask BEQ clearit
                  LDA buffer+0 ORR addr STR addr INW addr     ; store line buffer to VRAM addr
                  LDA buffer+1 ORR addr STR addr INW addr
                  LDA buffer+2 ORR addr STR addr
                  JPA common
  clearit:      LDA buffer+0 NOT ANR addr STR addr INW addr   ; store line buffer to VRAM addr
                LDA buffer+1 NOT ANR addr STR addr INW addr
                LDA buffer+2 NOT ANR addr STR addr
  common:       LDI 62 ADW addr                               ; ... and move to the next line
                INW dptr LDA dptr+0 CPI <data+32 BNE lineloop ; haben wir alle sprite daten verarbeitet?
                  RTS

shift:          0xff
dptr:           0xffff
addr:           0xffff
mask:           0x01
data:           0xe0,0x07,0x98,0x1a,0x04,0x34,0x02,0x68,0x62,0x50,0x11,0xa0,0x09,0xd0,0x09,0xa0,
                0x01,0xd0,0x03,0xa8,0x05,0xd4,0x0a,0x6a,0x56,0x55,0xac,0x2a,0x58,0x1d,0xe0,0x07,
buffer:         0xff, 0xff, 0xff

#mute

table:          ; position data for sprite entities

#mute           ; MinOS API definitions generated by 'asm os.asm -s_'

#org 0xb000 _Start:
#org 0xb003 _Prompt:
#org 0xb006 _ReadLine:
#org 0xb009 _ReadSpace:
#org 0xb00c _ReadHex:
#org 0xb00f _SerialWait:
#org 0xb012 _SerialPrint:
#org 0xb015 _FindFile:
#org 0xb018 _LoadFile:
#org 0xb01b _SaveFile:
#org 0xb01e _MemMove:
#org 0xb021 _Random:
#org 0xb024 _ScanPS2:
#org 0xb027 _ReadInput:
#org 0xb02a _WaitInput:
#org 0xb02d _ClearVRAM:
#org 0xb030 _Clear:
#org 0xb033 _ClearRow:
#org 0xb036 _SetPixel:
#org 0xb039 _ClrPixel:
#org 0xb03c _GetPixel:
#org 0xb03f _Char:
#org 0xb042 _Line:
#org 0xb045 _Rect:
#org 0xb048 _Print:
#org 0xb04b _PrintChar:
#org 0xb04e _PrintHex:
#org 0xb051 _ScrollUp:
#org 0xb054 _ScrollDn:
#org 0xbf70 _ReadPtr:
#org 0xbf72 _ReadNum:
#org 0xbf84 _RandomState:
#org 0xbf8c _XPos:
#org 0xbf8d _YPos:
#org 0xbf8e _ReadBuffer:
