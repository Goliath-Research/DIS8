CODE	SEGMENT
	ASSUME	CS:CODE

	JMP	Start

	DB	'Plus Vers 1.2'

Start:	MOV	CS:__DS,DS
	MOV	CS:__SS,SS
	MOV	AX,CS
	MOV	DS,AX
	MOV	SS,AX
	MOV	SP,OFFSET L0900[0F0h]

	MOV	DX,OFFSET Presentacion
	MOV	CL,9
	CALL	CPM				; CP/M Print String

	CALL	Definir8DS			; Definir _8DS
	OR	AX,AX
	JZ	LoadError			; No se encontr¢ segmento
	MOV	CS:_8DS,AX
	CALL	ResetPLUS80			; Reset a la PLUS-80
	CALL	SendBIOSAndCOMM
	JNZ	LoadError
	CALL	SetCommonDTA
	CALL	PrepareFCB
	CMP	AL,1				; ¨ Para grabar desde PLUS-80 ?
	JZ	GetAndSave			; S¡
	CALL	LoadAndRun			; No. Cargar y emular
	MOV	DX,OFFSET LoadErrorMsg

ErrorExit:
	PUSH	CS
	POP	DS
	MOV	CL,9				; Print String
	CALL	CPM

Terminate:
	MOV	CL,0
	MOV	DL,CL
	JMP	CPM

LoadError:
	MOV	DX,OFFSET LoadErrorMsg
	JMP	ErrorExit

Presentacion	DB	'$   '
LoadErrorMsg	DB	0Dh,0Ah,'BAD LOAD$'

GetAndSave:
	MOV	DS,CS:__DS
	CALL	CheckParameter
	JB	ParameterError
	PUSH	AX

	PUSH	DS
	POP	ES
	SUB	AX,AX
	MOV	CX,0Ch
	MOV	DI,78h
	REPZ	STOSW				; Clear __DS:78 (18)

	MOV	DX,6Ch
	MOV	AH,16h
	INT	21h				; Create File for FCB2

	POP	CX				; CX = First Parameter
	OR	AL,AL
	JNZ	SpaceError
	JCXZ	CloseDUMP

	MOV	ES,CS:_8DS
	MOV	BYTE PTR ES:[0FE00h],1		; PLUS-80... Read from
	SHL	CX,1

ReadNextBlock:
	MOV	BYTE PTR ES:[0FE01h],0		; Solicitud a PLUS-80 (R)
	CALL	Send				; Send. Analizar solicitud
	CALL	Receive				; Recibir confirmaci¢n

	MOV	DX,6Ch
	MOV	AH,15h
	INT	21h				; Sequential Write
	OR	AL,AL				;   Received from PLUS-80
	JNZ	SpaceError
	LOOP	ReadNextBlock

	MOV	BYTE PTR ES:[0FE01h],1
	CALL	Send				; End

CloseDUMP:
	MOV	DX,6Ch
	MOV	AH,10h
	INT	21h				; Close File
	OR	AL,AL
	JNZ	SpaceError
	JMP	Terminate

ParameterError:
	MOV	DX,OFFSET ParameterErrorMsg
	JMP	ErrorExit

SpaceError:
	MOV	DX,OFFSET SpaceErrorMsg
	JMP	ErrorExit

ParameterErrorMsg	DB	'PARAMETER ERROR$'
SpaceErrorMsg		DB	'NO SPACE$'

CheckParameter	PROC
	MOV	BL,0Ah
	MOV	SI,5Ch
	LODSB
	OR	AL,AL
	JNZ	L0112
L0102:	MOV	CL,[SI]
	SUB	CL,30h				;'0'
	CMP	CL,BL
	JNB	L0114
	INC	SI
	MUL	BL
	ADD	AL,CL
	JNB	L0102
L0112:	STC
	RET
L0114:	CMP	BYTE PTR [SI],21h		;'!'
	CMC
	RET
CheckParameter	ENDP

SetCommonDTA	PROC
	PUSH	DS
	MOV	DS,CS:_8DS
	MOV	DX,0FE29h			; Communication Area
	MOV	AH,1Ah
	INT	21h				; Set DTA
	POP	DS
	RET
SetCommonDTA	ENDP

PrepareFCB	PROC
	PUSH	DS
	MOV	DS,CS:__SS
	MOV	SI,0Bh
	PUSH	CS
	POP	ES
	MOV	DI,OFFSET UnOpFCB
	MOV	AX,2909h
	INT	21h				; Parse File Name

	MOV	DI,OFFSET UnOpFCB[9]		; Extension
	MOV	DS,CS:__DS
	MOV	SI,7Dh
	MOV	AL,[SI]
	CMP	AL,1				; ¨ [__DS:7D] = 1 ?
	JZ	ClearFCB			; S¡. Get and Save
	MOVSW					; No. Copiar extensi¢n.
	MOVSB					; Es Load and Run

ClearFCB:
	PUSH	AX
	MOV	DI,OFFSET FCB
	XOR	AX,AX
	MOV	CX,0Ch
	REPZ	STOSW
	POP	AX
	POP	DS
	RET
PrepareFCB	ENDP

SendBIOS	PROC
	MOV	SI,OFFSET Prog3
	MOV	CX,75h
	CALL	SendProgram

	CLD
	MOV	SI,OFFSET Prog4
	MOV	CX,375h
	MOV	AX,[SI]				; Start Address FA00
	ADD	AX,6
	MOV	WORD PTR CS:Prog2[3],AX		; First byte to SUM
	PUSH	SI
	PUSH	CX
NextByte:
	LODSB
	CALL	SendNoCommand
	LOOP	NextByte

	MOV	AL,1				; SUM Command
	CALL	SendCommand

	POP	CX
	POP	SI
	ADD	SI,8
	SUB	CX,8
	MOV	WORD PTR CS:Prog2[5],CX		; Count to SUM
	MOV	DX,0				; SUM Variable

SumNextByte:
	LODSB
	XOR	AH,AH
	ADD	DX,AX
	LOOP	SumNextByte

	MOV	WORD PTR CS:Prog2[7],DX		; Control SUM
	RET
SendBIOS	ENDP

SendBIOSAndCOMM	PROC
	CALL	SendBIOS
	MOV	SI,OFFSET Prog2
	MOV	CX,97h
	CALL	SendProgram
	CALL	Receive
	AND	AX,0FFh
	RET
SendBIOSAndCOMM	ENDP

SendProgram	PROC
	CALL	Init8255
	MOV	ES,CS:_8DS
	MOV	DI,0FE00h
	CLD
	REPZ	MOVSB
	CALL	ReInit8255
	RET
SendProgram	ENDP

Init8255	PROC
	MOV	AL,0B4h
	OUT	0F6h,AL
	MOV	AL,0Fh
	OUT	0F6h,AL
	MOV	AL,CS:Flag
	OUT	0F6h,AL
	RET
Init8255	ENDP

ReInit8255	PROC
	MOV	AL,0Eh
	OUT	0F6h,AL
	MOV	CX,01D6h
Here:	LOOP	Here
	RET
ReInit8255	ENDP

ResetPLUS80	PROC
	MOV	SI,OFFSET Prog1
	MOV	CX,18h
	CALL	SendProgram
	CMP	BYTE PTR ES:[0FE00h],0C9h
	JNZ	NoPLUS
	XOR	BYTE PTR CS:Flag,1		; Est  activa
NoPLUS:	RET
ResetPLUS80	ENDP

Flag	DB	0Dh


Prog1	DB	0Dh				; Programa inicial de la PLUS-80
	DB 	0C3h,3,0FEh,3Eh,20h,0D3h,0,31h
	DB	0,0,0EDh,56h,3Eh,0C9h,32h,38h
	DB	0,0FBh,76h,32h,0,0FEh,0F3h,76h

;		JP	LFE03
;	LFE03:	LD	A,#20
;		OUT	(#00),A
;		LD	SP,0000
;		IM	1
;		LD	A,#C9
;		LD	(#0038),A
;		EI
;		HALT
;		LD	(#FE00),A
;		DI
;		HALT

Definir8DS	PROC
	MOV	AX,0D000h
	CALL	CheckSEG
	JZ	Ok
	MOV	AX,0C000h
	CALL	CheckSEG
	JZ	Ok
	MOV	AX,0B800h
	CALL	CheckSEG
	JZ	Ok
	MOV	AX,8000h
	CALL	CheckSEG
	JZ	NoHay
	MOV	AX,09000h
	CALL	CheckSEG
	JZ	Ok
NoHay:	XOR	AX,AX
Ok:	RET
Definir8DS	ENDP

CheckSEG	PROC
	PUSH	DS
	MOV	DS,AX
	MOV	SI,0FE00h
	MOV	AX,[SI]
	MOV	CX,AX
	NOT	AX
	NOT	WORD PTR [SI]
	CMP	AX,[SI]
	MOV	[SI],CX
	MOV	AX,DS
	POP	DS
	RET
CheckSEG	ENDP

LoadAndRun	PROC
	CALL	SendBIOSAndCOMM

	MOV	DX,OFFSET UnOpFCB
	MOV	AH,0Fh
	INT	21h				; Open File
	OR	AL,AL
	JNZ	ReadError

	MOV	BYTE PTR ES:[0FE00h],0		; PLUS-80... Write to
	MOV	BL,CS:Prog2[4]			; ???
	SUB	BH,BH
	DEC	BL
	SHL	BX,1

ReadAndSendNextBlock:
	MOV	DX,OFFSET UnOpFCB
	MOV	AH,14h
	INT	21h				; Sequential Read
	CMP	AL,2
	JZ	ReadError			; DTA too small
	CMP	AL,3
	JZ	L0281				; EOF. Partial record read
	OR	AL,AL
	JNZ	Execute				; EOF

L0281:	CMP	CX,DX				; Ok
	JNB	ReadError
	INC	CX
	MOV	BYTE PTR ES:[0FE01h],0		; Solicitud a PLUS-80 (W)
	CALL	Send				; Analizar solicitud
	CALL	Receive				; Recibir confirmaci¢n
	JMP	SHORT ReadAndSendNextBlock

ReadError:
	STC
	RET

Execute:
	MOV	BYTE PTR ES:[0FE02h],0		; ??? Algo en el PSP
	MOV	AX,0FFFFh
	INT	21h				; ?? devuelve 5833
	CMP	AX,5833h
	MOV	AL,0
	JNZ	L02B9
	MOV	AX,0FFF0h
	MOV	DL,AH
	INT	21h				; ?? devuelve FF00
	ADD	AL,AL
	ADD	AL,AL
	ADD	AL,AL
	ADD	AL,AL
	AND	AL,0F0h

L02B9:	MOV	BL,AL
	MOV	AH,19h
	INT	21h				; Current Disk
	OR	AL,BL
	MOV	ES:[0FE03h],AL			; ??? Current Disk del PSP

	PUSH	DS
	MOV	DS,CS:__DS
	MOV	SI,5Ch
	MOV	DI,0FE05h
	MOV	CX,0A4h
	CLD
	REPZ	MOVSB				; Copiar el PSP desde 5C
	POP	DS				;   hacia FE05
	MOV	BYTE PTR ES:[0FE01h],1		; Solicitud de atenci¢n
	CALL	Send				; PLUS-80 recibe el PSP

	MOV	DX,OFFSET L08A8
	MOV	CL,1Ah
	CALL	CPM				; Set File Buffer Address (offset)

	MOV	DX,DS
	MOV	CL,33h
	CALL	CPM				; Set File Buffer Segment Base

CPMRequest:
	CALL	Receive				; Esperar por solicitud de
						;   servicios de la PLUS-80
	MOV	CL,ES:[0FE00h]
	MOV	DL,ES:[0FE01h]
	MOV	DH,ES:[0FE02h]
	CMP	CL,33h
	JNB	Unknown
	MOV	BL,CL
	XOR	BH,BH
	ADD	BX,BX
	CALL	WORD PTR CS:[BX + Table]
	JMP	CPMRequest

Unknown:
	CALL	StatusServices
	JMP	CPMRequest
LoadAndRun	ENDP

StatusServices	PROC
	CALL	CPM
	JMP	SendServiceStatus
StatusServices	ENDP

DirectConsoleIO	PROC
	CMP	DL,0FEh				; Si FE retornar flag
	JZ	StatusServices
DirectConsoleIO	ENDP

UserCode	PROC
	CMP	DL,0FFh				; Si FF retornar flag
	JZ	StatusServices			; si no retornar 0
UserCode	ENDP

DiscardedServices	PROC
	XOR	AX,AX
	CALL	SendServiceStatus
DiscardedServices	ENDP

CPM	PROC
	PUSH	ES
	INT	0E0h
	POP	ES
	RET
CPM	ENDP

DiskServices	PROC
	MOV	DI,OFFSET UnOpFCB
	MOV	SI,0FE05h
	MOV	CX,0A4h
	MOV	DX,DI
	PUSH	DS
	PUSH	ES
	PUSH	ES
	MOV	AX,DS
	POP	DS
	MOV	ES,AX
	CLD
	REPZ	MOVSB
	POP	ES
	POP	DS				; Obtener copia del FCB

	PUSH	DX
	MOV	CL,ES:[0FE00h]
	CALL	CPM				; Ejecutar el servicio

	POP	SI
	MOV	DI,0FE05h
	MOV	CX,0A4h
	CLD
	REPZ	MOVSB				; Entregar el resultado
DiskServices	ENDP

SendServiceStatus	PROC
	MOV	ES:[0FE03h],AL
	MOV	ES:[0FE04h],AH
	CALL	Send
	RET
SendServiceStatus	ENDP

CallBIOS	PROC
	MOV	CL,32
	JMP	DiskServices
CallBIOS	ENDP

SendCommand	PROC
	PUSH	AX
	MOV	AL,0F8h
	CALL	Send
	POP	AX
	JMP	Send
SendCommand	ENDP

SendNoCommand	PROC
	CMP	AL,0F8h				; Command Prefix
	JNZ	Send
	CALL	Send
	XOR	AL,AL
SendNoCommand	ENDP

Send	PROC
	PUSH	AX
Wait1:	IN	AL,0F4h
	AND	AL,2
	JZ	Wait1
	POP	AX
	OUT	0F2h,AL
	RET
Send	ENDP

Receive	PROC
	IN	AL,0F4h
	AND	AL,20h
	JZ	Receive
	IN	AL,0F0h
	RET
Receive	ENDP

Table	DW	OFFSET Terminate		; 00
	DW	OFFSET StatusServices		; 01
	DW	OFFSET DiscardedServices	; 02
	DW	OFFSET StatusServices		; 03
	DW	OFFSET DiscardedServices	; 04
	DW	OFFSET DiscardedServices	; 05
	DW	OFFSET DirectConsoleIO		; 06
	DW	OFFSET StatusServices		; 07
	DW	OFFSET DiscardedServices	; 08
	DW	OFFSET StatusServices		; 09
	DW	OFFSET DiskServices		; 0A
	DW	OFFSET StatusServices		; 0B
	DW	OFFSET StatusServices		; 0C
	DW	OFFSET DiscardedServices	; 0D
	DW	OFFSET DiscardedServices	; 0E
	DW	OFFSET DiskServices		; 0F
	DW	OFFSET DiskServices		; 10
	DW	OFFSET DiskServices		; 11
	DW	OFFSET DiskServices		; 12
	DW	OFFSET DiskServices		; 13
	DW	OFFSET DiskServices		; 14
	DW	OFFSET DiskServices		; 15
	DW	OFFSET DiskServices		; 16
	DW	OFFSET DiskServices		; 17
	DW	OFFSET StatusServices		; 18
	DW	OFFSET StatusServices		; 19
	DW	OFFSET StatusServices		; 1A
	DW	OFFSET StatusServices		; 1B
	DW	OFFSET DiscardedServices	; 1C
	DW	OFFSET StatusServices		; 1D
	DW	OFFSET DiskServices		; 1E
	DW	OFFSET StatusServices		; 1F
	DW	OFFSET UserCode			; 20
	DW	OFFSET DiskServices		; 21
	DW	OFFSET DiskServices		; 22
	DW	OFFSET DiskServices		; 23
	DW	OFFSET DiskServices		; 24
	DW	OFFSET DiscardedServices	; 25
	DW	OFFSET StatusServices		; 26
	DW	OFFSET StatusServices		; 27
	DW	OFFSET DiskServices		; 28
	DW	OFFSET StatusServices		; 29
	DW	OFFSET StatusServices		; 2A
	DW	OFFSET StatusServices		; 2B
	DW	OFFSET StatusServices		; 2C
	DW	OFFSET StatusServices		; 2D
	DW	OFFSET StatusServices		; 2E
	DW	OFFSET StatusServices		; 2F
	DW	OFFSET StatusServices		; 30
	DW	OFFSET StatusServices		; 31
	DW	OFFSET CallBIOS			; 32

Prog2	DB	0C3h,009h,0FEh
	DB	000h,000h,000h,000h,000h,000h
	DB	03Eh,020h,0D3h,000h,031h,000h
	DB	000h,021h,086h,0FEh,011h,0F9h
	DB	0FDh,001h,007h,000h,0EDh,0B0h
	DB	0CDh,038h,0FEh,028h,009h,0CDh
	DB	058h,0FEh,0CDh,038h,0FEh,020h
	DB	001h,03Ch,0CDh,08Dh,0FEh,02Ah
	DB	003h,0FEh,023h,023h,023h,0E5h
	DB	03Ch,0C0h,076h,018h,0FDh,02Ah
	DB	003h,0FEh,0EDh,04Bh,005h,0FEh
	DB	011h,000h,000h,07Eh,083h,05Fh
	DB	07Ah,0CEh,000h,057h,023h,00Bh
	DB	079h,0B0h,020h,0F3h,02Ah,007h
	DB	0FEh,0AFh,0EDh,052h,0C8h,03Dh
	DB	0C9h,021h,000h,002h,0EDh,05Bh
	DB	003h,0FEh,0EDh,04Bh,005h,0FEh
	DB	0CDh,06Fh,0FEh,012h,023h,013h
	DB	00Bh,079h,0B0h,020h,0F5h,0C9h
	DB	0E5h,07Ch,00Fh,0E6h,003h,0F6h
	DB	020h,008h,07Ch,0E6h,001h,0F6h
	DB	0FEh,067h,03Eh,020h,008h,0CDh
	DB	0F9h,0FDh,008h,0E1h,0C9h,0D3h
	DB	000h,07Eh,008h,0D3h,000h,0C9h
	DB	04Fh,0DBh,020h,017h,030h,0FBh
	DB	079h,0D3h,010h,0C9h

;		ORG	0FE00H
;	XFDF9	EQU	0FDF9H
;		JP	AFE09
;	;
;	; >> NO EXECUTION PATH TO HERE <<
;	DFE03:	DEFB	000H
;		DEFB	000H
;	DFE05:	DEFB	000H
;		DEFB	000H
;	DFE07:	DEFB	000H
;		DEFB	000H
;	AFE09:	LD	A,020H
;		OUT	(000H),A
;		LD	SP,00000H
;		LD	HL,TFE86
;		LD	DE,0FDF9H
;		LD	BC,00007H
;		LDIR
;		CALL	AFE38
;		JR	Z,AFE29
;		CALL	AFE58
;		CALL	AFE38
;		JR	NZ,AFE29
;		INC	A
;	AFE29:	CALL	AFE8D
;		LD	HL,(DFE03)
;		INC	HL
;		INC	HL
;		INC	HL
;		PUSH	HL
;		INC	A
;		RET	NZ
;	AFE35:	HALT
;		JR	AFE35
;	;
;	AFE38:	LD	HL,(DFE03)
;		LD	BC,(DFE05)
;		LD	DE,00000H
;	AFE42:	LD	A,(HL)
;		ADD	A,E
;		LD	E,A
;		LD	A,D
;		ADC	A,000H
;		LD	D,A
;		INC	HL
;		DEC	BC
;		LD	A,C
;		OR	B
;		JR	NZ,AFE42
;		LD	HL,(DFE07)
;		XOR	A
;		SBC	HL,DE
;		RET	Z
;		DEC	A
;		RET
;	;
;	AFE58:	LD	HL,00200H
;		LD	DE,(DFE03)
;		LD	BC,(DFE05)
;	AFE63:	CALL	AFE6F
;		LD	(DE),A
;		INC	HL
;		INC	DE
;		DEC	BC
;		LD	A,C
;		OR	B
;		JR	NZ,AFE63
;		RET
;	;
;	AFE6F:	PUSH	HL
;		LD	A,H
;		RRCA
;		AND	003H
;		OR	020H
;		EX	AF,AF'
;		LD	A,H
;		AND	001H
;		OR	0FEH
;		LD	H,A
;		LD	A,020H
;		EX	AF,AF'
;		CALL	XFDF9
;		EX	AF,AF'
;		POP	HL
;		RET
;	;
;	; >> NO EXECUTION PATH TO HERE <<
;	TFE86:	OUT	(000H),A
;		LD	A,(HL)
;		EX	AF,AF'
;		OUT	(000H),A
;		RET
;	;
;	AFE8D:	LD	C,A
;	AFE8E:	IN	A,(020H)
;		RLA
;		JR	NC,AFE8E
;		LD	A,C
;		OUT	(010H),A
;		RET
;		END


Prog3	DB	0C3h,003h,0FEh,03Eh,020h,0D3h
	DB	000h,031h,000h,000h,021h,052h
	DB	0FEh,011h,0F7h,0FDh,001h,009h
	DB	000h,0EDh,0B0h,0CDh,05Bh,0FEh
	DB	06Fh,0CDh,05Bh,0FEh,067h,0E5h
	DB	001h,0FAh,0FFh,0CDh,05Bh,0FEh
	DB	038h,005h,077h,023h,003h,018h
	DB	0F6h,0D1h,0D5h,013h,013h,013h
	DB	013h,013h,013h,021h,000h,002h
	DB	0E5h,07Ch,00Fh,0E6h,003h,0F6h
	DB	020h,008h,07Ch,0E6h,001h,0F6h
	DB	0FEh,067h,01Ah,008h,0CDh,0F7h
	DB	0FDh,0E1h,023h,013h,00Bh,079h
	DB	0B0h,020h,0E5h,0C9h,0D3h,000h
	DB	008h,077h,03Eh,020h,0D3h,000h
	DB	0C9h,0CDh,06Ch,0FEh,0FEh,0F8h
	DB	020h,008h,0CDh,06Ch,0FEh,0B7h
	DB	037h,0C0h,03Eh,0F8h,0B7h,0C9h
	DB	0DBh,020h,017h,017h,030h,0FAh
	DB	0DBh,010h,0C9h

;		.Z80
;	
;		ORG	0FE00H
;	XFDF7	EQU	0FDF7H
;		JP	AFE03
;	;
;	AFE03:	LD	A,020H
;		OUT	(000H),A
;		LD	SP,00000H
;		LD	HL,TFE52
;		LD	DE,0FDF7H
;		LD	BC,00009H
;		LDIR
;		CALL	AFE5B
;		LD	L,A
;		CALL	AFE5B
;		LD	H,A
;		PUSH	HL
;		LD	BC,0FFFAH
;	AFE21:	CALL	AFE5B
;		JR	C,AFE2B
;		LD	(HL),A
;		INC	HL
;		INC	BC
;		JR	AFE21
;	;
;	AFE2B:	POP	DE
;		PUSH	DE
;		INC	DE
;		INC	DE
;		INC	DE
;		INC	DE
;		INC	DE
;		INC	DE
;		LD	HL,00200H
;	AFE36:	PUSH	HL
;		LD	A,H
;		RRCA
;		AND	003H
;		OR	020H
;		EX	AF,AF'
;		LD	A,H
;		AND	001H
;		OR	0FEH
;		LD	H,A
;		LD	A,(DE)
;		EX	AF,AF'
;		CALL	XFDF7
;		POP	HL
;		INC	HL
;		INC	DE
;		DEC	BC
;		LD	A,C
;		OR	B
;		JR	NZ,AFE36
;		RET
;	;
;	; >> NO EXECUTION PATH TO HERE <<	Se transfiere a FDF7
;	TFE52:	OUT	(000H),A
;		EX	AF,AF'
;		LD	(HL),A
;		LD	A,020H
;		OUT	(000H),A
;		RET
;	;
;	AFE5B:	CALL	AFE6C
;		CP	0F8H
;		JR	NZ,AFE6A
;		CALL	AFE6C
;		OR	A
;		SCF
;		RET	NZ
;		LD	A,0F8H
;	AFE6A:	OR	A
;		RET
;	;
;	AFE6C:	IN	A,(020H)
;		RLA
;		RLA
;		JR	NC,AFE6C
;		IN	A,(010H)
;		RET
;		END


Prog4	DB	000h,0FAh,0C3h,0A5h,0FBh,000h
	DB	000h,000h,0C3h,0DAh,0FBh,0C3h
	DB	000h,0FBh,023h,001h,006h,0FAh
	DB	07Ch,090h,0C0h,07Dh,091h,0C0h
	DB	021h,03Bh,000h,0C9h,02Ah,075h
	DB	0FDh,001h,000h,000h,011h,003h
	DB	0FEh,0E5h,0D5h,0EDh,05Bh,073h
	DB	0FDh,0AFh,0EDh,052h,0D1h,0E1h
	DB	028h,014h,07Eh,012h,0C5h,0CDh
	DB	00Ch,0FAh,0C1h,013h,003h,0E5h
	DB	021h,0FDh,001h,0AFh,0EDh,042h
	DB	0E1h,020h,0E0h,02Fh,032h,000h
	DB	0FEh,0EDh,043h,001h,0FEh,04Fh
	DB	0CDh,02Ah,0FDh,00Ch,028h,0CBh
	DB	0C3h,0A5h,0FBh,03Dh,020h,0C2h
	DB	032h,077h,0FDh,021h,03Bh,000h
	DB	05Dh,054h,022h,073h,0FDh,022h
	DB	075h,0FDh,0FBh,0D9h,021h,000h
	DB	003h,011h,026h,000h,04Dh,044h
	DB	0D9h,03Ah,077h,0FDh,0B7h,020h
	DB	01Fh,0DBh,020h,0E6h,040h,028h
	DB	019h,0F3h,0DBh,010h,077h,0CDh
	DB	00Ch,0FAh,022h,073h,0FDh,0E5h
	DB	0CDh,00Ch,0FAh,0AFh,0EDh,052h
	DB	0E1h,020h,004h,02Fh,032h,077h
	DB	0FDh,0FBh,0D9h,00Bh,079h,0B0h
	DB	0D9h,020h,0D4h,0DBh,020h,017h
	DB	030h,016h,0E5h,0AFh,0EDh,052h
	DB	0E1h,028h,01Bh,0F3h,032h,077h
	DB	0FDh,0EBh,07Eh,0D3h,010h,0CDh
	DB	00Ch,0FAh,022h,075h,0FDh,0EBh
	DB	0F3h,03Eh,030h,0D3h,001h,0E3h
	DB	0E3h,03Eh,020h,0D3h,001h,0FBh
	DB	0D9h,0E5h,030h,00Dh,0AFh,0EDh
	DB	052h,038h,00Fh,0EDh,052h,038h
	DB	00Bh,0C1h,019h,018h,099h,019h
	DB	001h,001h,003h,0EDh,042h,03Fh
	DB	0E1h,030h,0F3h,018h,08Dh,03Dh
	DB	0C2h,055h,0FAh,0EBh,03Ah,001h
	DB	0FEh,0B7h,0C2h,0A5h,0FBh,0D5h
	DB	0C5h,0EDh,0B0h,0CDh,02Ah,0FDh
	DB	0C1h,0D1h,018h,0EEh,000h,000h
	DB	000h,000h,000h,000h,000h,000h
	DB	0C3h,033h,0FBh,0C3h,09Ah,0FBh
	DB	0C3h,0C4h,0FBh,0C3h,0B2h,0FBh
	DB	0C3h,0BEh,0FBh,0C3h,0B8h,0FBh
	DB	0C3h,0BBh,0FBh,0C3h,0B5h,0FBh
	DB	0C3h,0B1h,0FBh,0C3h,0ACh,0FBh
	DB	0C3h,0B1h,0FBh,0C3h,0B1h,0FBh
	DB	0C3h,0B1h,0FBh,0C3h,0AFh,0FBh
	DB	0C3h,0AFh,0FBh,0C3h,0C1h,0FBh
	DB	0C3h,0B1h,0FBh,0F3h,031h,000h
	DB	0FEh,03Eh,0C3h,032h,038h,000h
	DB	021h,0A5h,0FBh,022h,039h,000h
	DB	032h,000h,000h,032h,005h,000h
	DB	021h,003h,0FBh,022h,001h,000h
	DB	021h,006h,0FAh,022h,006h,000h
	DB	0EDh,056h,0CDh,02Dh,0FDh,011h
	DB	000h,001h,021h,029h,0FEh,001h
	DB	080h,000h,03Ah,000h,0FEh,0B7h
	DB	0C2h,0E1h,0FAh,03Ah,001h,0FEh
	DB	0B7h,020h,00Bh,0C5h,0E5h,0EDh
	DB	0B0h,0E1h,0C1h,0CDh,02Ah,0FDh
	DB	018h,0EFh,02Ah,002h,0FEh,022h
	DB	003h,000h,021h,005h,0FEh,011h
	DB	05Ch,000h,001h,0A4h,000h,0EDh
	DB	0B0h,021h,080h,000h,022h,073h
	DB	0FDh,022h,075h,0FDh,031h,006h
	DB	0FAh,0CDh,000h,001h,0F3h,031h
	DB	000h,0FEh,0AFh,032h,000h,0FEh
	DB	0CDh,036h,0FDh,031h,000h,0FEh
	DB	0F3h,076h,018h,0F9h,021h,000h
	DB	000h,03Eh,001h,0C9h,03Eh,003h
	DB	021h,03Eh,007h,021h,03Eh,005h
	DB	021h,03Eh,006h,021h,03Eh,004h
	DB	021h,03Eh,00Fh,021h,03Eh,002h
	DB	032h,005h,0FEh,0EDh,043h,006h
	DB	0FEh,03Eh,032h,032h,000h,0FEh
	DB	0CDh,02Ah,0FDh,02Ah,003h,0FEh
	DB	07Dh,0C9h,0EDh,073h,077h,0FDh
	DB	031h,000h,0FEh,0AFh,032h,0A9h
	DB	0FEh,06Fh,067h,022h,003h,0FEh
	DB	079h,032h,000h,0FEh,0EDh,053h
	DB	001h,0FEh,0FEh,029h,038h,013h
	DB	0FEh,0FFh,028h,01Fh,00Eh,029h
	DB	0FEh,02Fh,028h,009h,0FEh,035h
	DB	038h,00Bh,0FEh,03Bh,030h,007h
	DB	00Ch,021h,039h,0FCh,0CDh,02Eh
	DB	0FCh,02Ah,003h,0FEh,07Dh,044h
	DB	0EDh,07Bh,077h,0FDh,0C9h,0EBh
	DB	07Eh,011h,005h,0FEh,001h,008h
	DB	000h,0EDh,0B0h,04Fh,021h,08Fh
	DB	0FCh,0FEh,008h,0DCh,02Eh,0FCh
	DB	018h,0E1h,006h,000h,009h,009h
	DB	0D5h,05Eh,023h,056h,0EBh,0D1h
	DB	0E9h,09Ah,0FBh,02Ah,0FDh,02Ah
	DB	0FDh,02Ah,0FDh,02Ah,0FDh,02Ah
	DB	0FDh,02Ah,0FDh,013h,0FDh,01Dh
	DB	0FDh,09Fh,0FCh,0B2h,0FCh,02Ah
	DB	0FDh,02Ah,0FDh,023h,0FDh,02Ah
	DB	0FDh,0F8h,0FCh,0F8h,0FCh,0C9h
	DB	0FCh,0CDh,0FCh,0F8h,0FCh,0D2h
	DB	0FCh,0EDh,0FCh,0F8h,0FCh,0F8h
	DB	0FCh,02Ah,0FDh,02Ah,0FDh,0C4h
	DB	0FCh,02Ah,0FDh,02Ah,0FDh,02Ah
	DB	0FDh,0F8h,0FCh,02Ah,0FDh,02Ah
	DB	0FDh,0D2h,0FCh,0EDh,0FCh,0F8h
	DB	0FCh,0F8h,0FCh,02Ah,0FDh,018h
	DB	0FCh,018h,0FCh,0EDh,0FCh,0EDh
	DB	0FCh,00Dh,0FDh,02Ah,0FDh,02Ah
	DB	0FDh,02Ah,0FDh,02Ah,0FDh,050h
	DB	0FDh,06Dh,0FDh,040h,0FDh,046h
	DB	0FDh,021h,000h,0FEh,036h,002h
	DB	023h,01Ah,0FEh,024h,0C8h,013h
	DB	077h,03Eh,002h,0CDh,02Ah,0FDh
	DB	018h,0F3h,01Ah,021h,005h,0FEh
	DB	077h,079h,0CDh,02Ah,0FDh,023h
	DB	04Eh,006h,000h,003h,013h,0EDh
	DB	0B0h,0C9h,0EDh,053h,073h,0FDh
	DB	0C9h,0EBh,022h,075h,0FDh,02Ah
	DB	075h,0FDh,0EBh,0F6h,037h,0EBh
	DB	0F5h,0CDh,0F9h,0FCh,0F1h,030h
	DB	005h,03Ah,003h,0FEh,0B7h,0C0h
	DB	0EDh,05Bh,073h,0FDh,021h,029h
	DB	0FEh,001h,080h,000h,0EDh,0B0h
	DB	0C9h,0D5h,02Ah,073h,0FDh,011h
	DB	029h,0FEh,0CDh,0E7h,0FCh,0D1h
	DB	0EBh,001h,024h,000h,011h,005h
	DB	0FEh,0E5h,0D5h,0C5h,0EDh,0B0h
	DB	0CDh,02Ah,0FDh,0C1h,0E1h,0D1h
	DB	0EDh,0B0h,0C9h,0EBh,001h,005h
	DB	000h,018h,0E9h,0CDh,02Ah,0FDh
	DB	03Ah,003h,0FEh,032h,003h,000h
	DB	0C9h,021h,003h,000h,073h,018h
	DB	007h,021h,080h,000h,022h,073h
	DB	0FDh,079h,0CDh,036h,0FDh,0DBh
	DB	020h,017h,017h,030h,0FAh,0DBh
	DB	010h,0C9h,0F5h,0DBh,020h,017h
	DB	030h,0FBh,0F1h,0D3h,010h,0C9h
	DB	02Ah,007h,0FEh,022h,00Ah,0FEh
	DB	03Eh,012h,032h,00Ch,0FEh,0CDh
	DB	06Dh,0FDh,018h,003h,0CDh,02Ah
	DB	0FDh,0B7h,0F5h,03Ah,00Ch,0FEh
	DB	04Fh,006h,000h,0B7h,020h,001h
	DB	004h,021h,029h,0FEh,0EDh,05Bh
	DB	00Ah,0FEh,0F1h,030h,001h,0EBh
	DB	0EDh,0B0h,0C9h,037h,0CDh,054h
	DB	0FDh,018h,0B7h

__DS	DW	0
__SS	DW	0
_8DS	DW	0
UnOpFCB	DB	0Ch DUP(0)
FCB	DB	18h DUP(0)
L08A8	DW	0
L0900	DB	0

CODE	ENDS

	END
