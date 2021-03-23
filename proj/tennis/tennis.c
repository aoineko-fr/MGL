//_____________________________________________________________________________
//  ▄▄▄▄ ▄            ▄▄     ▄▄▄                ▄▄  
//  ██▄  ▄  ██▀▄ ▄▀██ ██    ▀█▄  ▄█▄█ ▄▀██  ██▀ ██▄ 
//  ██   ██ ██ █ ▀▄██ ▀█▄   ▄▄█▀ ██ █ ▀▄██ ▄██  ██ █
//_____________________________________________________________________________

// CMSX
#include "core.h"
#include "vdp.h"
#include "color.h"
#include "bios_hook.h"
#include "bios_port.h"
#include "input.h"
#include "math.h"
#include "string.h"
#include "game.h"
#include "game_menu.h"
#include "print.h"
#include "pt3/pt3_player.h"
#include "ayfx/ayfx_player.h"

//=============================================================================
//
//   D E F I N E S
//
//=============================================================================

//-----------------------------------------------------------------------------
// DEFINES
#define VERSION						"V0.2.01"
#define DEBUG						1
#define MSX2_ENHANCE				0

// VRAM Tables Address - MSX1
#define MSX1_LAYOUT_TABLE			0x3C00
#define MSX1_COLOR_TABLE			0x2000
#define MSX1_PATTERN_TABLE			0x0000
#define MSX1_SPRITE_ATTRIBUTE		0x3F00
#define MSX1_SPRITE_PATTERN_0		0x1800
#define MSX1_SPRITE_PATTERN_1		0x3800

// VRAM Tables Address - MSX2/2+/tR
#define MSX2_LAYOUT_TABLE			0x1800
#define MSX2_COLOR_TABLE			0x2000
#define MSX2_PATTERN_TABLE			0x0000
#define MSX2_SPRITE_ATTRIBUTE		0x1E00
#define MSX2_SPRITE_PATTERN_0		0x3800
#define MSX2_SPRITE_PATTERN_1		0x3800

// Unit conversion (Pixel <> Q10.6)
#define UNIT_TO_PX(a)				(u8)((a) / 64)
#define PX_TO_UNIT(a)				(i16)((a) * 64)

#define MOVE_DIAG					PX_TO_UNIT(1.6 * 0.71)
#define MOVE_MAIN					PX_TO_UNIT(1.6 * 1.00)
#define GRAVITY						PX_TO_UNIT(0.2)

#define PLY_MIN_X					PX_TO_UNIT(8)
#define PLY_MAX_X					PX_TO_UNIT(255 - 8)
#define PLY_UP_MIN_X				PX_TO_UNIT(8)
#define PLY_UP_MAX_X				PX_TO_UNIT(88)
#define PLY_DOWN_MIN_X				PX_TO_UNIT(97 + 8)
#define PLY_DOWN_MAX_X				PX_TO_UNIT(191 + 8)

#define SHOT_DETECT_X				16
#define SHOT_DETECT_Y				4
#define SHOT_DETECT_SMASH			10

// Menu layout
#define MENU_ITEMS_X				12
#define MENU_ITEMS_Y				14
#define MENU_ITEMS_H				(24-MENU_ITEMS_Y)

// Index offset
#define OFFSET_TITLE_LOGO			0
#define OFFSET_TITLE_FONT_DEF		112
#define OFFSET_TITLE_FONT_ALT		176
#define OFFSET_TITLE_MISC			240
#define OFFSET_GAME_COURT			0
#define OFFSET_GAME_SCRFONT			80
#define OFFSET_GAME_FONT			96
#define OFFSET_GAME_SCORE			160
#define OFFSET_GAME_REFEREE			208

#if (DEBUG)
	#define DEBUG_CODE(code) code
#else
	#define DEBUG_CODE(code)
#endif

// Input check macros
#define KEY_PRESS(key)				(((g_KeyRow[KEY_ROW(key)] & KEY_FLAG(key)) == 0) && ((g_PrevRow[KEY_ROW(key)] & KEY_FLAG(key)) != 0))
#define KEY_RELEASE(key)			(((g_KeyRow[KEY_ROW(key)] & KEY_FLAG(key)) != 0) && ((g_PrevRow[KEY_ROW(key)] & KEY_FLAG(key)) == 0))
#define KEY_ON(key)					((g_KeyRow[KEY_ROW(key)] & KEY_FLAG(key)) == 0)
#define KEY_OFF(key)				((g_KeyRow[KEY_ROW(key)] & KEY_FLAG(key)) != 0)

#define SCROLL_BUF_SIZE 22

enum SPIN
{
	SPIN_TOP = 0,
	SPIN_FLAT,
	SPIN_BACK,
	SPIN_MAX,
};

enum SHOT
{
	SHOT_FLAT = 0,	// Flat shot
	SHOT_SLICE,		// Drop shot / lob
	SHOT_ATTACK,	// Top spin shot
	SHOT_MAX,
};

/// 
enum TRAIN
{
	TRAIN_BOTH = 0,
	TRAIN_FLAT,
	TRAIN_LOB,
	TRAIN_MAX,
};

/// 
enum DIR
{
	DIR_CENTER = 0,
	DIR_RIGHT,
	DIR_LEFT,
	DIR_MAX,
};

/// Player Actions
enum ACTION
{
	ACTION_IDLE = 0,
	ACTION_WALK_F,
	ACTION_WALK_R,
	ACTION_WALK_L,
	ACTION_SHOOT_R,
	ACTION_SHOOT_L,
	ACTION_SMASH,
	ACTION_MAX,
};

/// Player Actions Functions
enum FUNCT
{
	FUNCT_NONE = 0,
	FUNCT_PREPARE,
	FUNCT_SHOOT_R,
	FUNCT_SHOOT_L,
	FUNCT_SMASH,
	FUNCT_MAX,
};

/// Sprite list
enum SPRITE
{
	SPRITE_PLY1_BLACK_H = 0,
	SPRITE_PLY1_BLACK_L,
	SPRITE_PLY1_RACKET,

	SPRITE_BALL_OUTLINE,

	SPRITE_PLY1_CLOTH,
	SPRITE_PLY1_WHITE_H,
	SPRITE_PLY1_WHITE_L,
	SPRITE_PLY1_SKIN_H,
	SPRITE_PLY1_SKIN_L,

	SPRITE_NET_LEFT,
	SPRITE_NET_RIGHT, 
	SPRITE_NET_GRAY,

	SPRITE_PLY2_BLACK_H,
	SPRITE_PLY2_BLACK_L,
	SPRITE_PLY2_RACKET,

	SPRITE_PLY2_CLOTH,
	SPRITE_PLY2_WHITE_H,
	SPRITE_PLY2_WHITE_L,
	SPRITE_PLY2_SKIN_H,
	SPRITE_PLY2_SKIN_L,

	SPRITE_LAUNCHER_1,
	SPRITE_LAUNCHER_2,
	SPRITE_LAUNCHER_3,
	SPRITE_LAUNCHER_4,
	SPRITE_LAUNCHER_5,

	SPRITE_BALL_BODY,
	SPRITE_BALL_SHADOW,

	SPRITE_MAX,
};

#if (SPRITE_MAX >= 32)
	Error! Only 32 sprites can be defined.
#endif

/// Menu pages enumeration
enum
{
	MENU_MAIN = 0,
	MENU_MATCH1P,
	MENU_MATCH2P,
	MENU_TRAINING,
	MENU_OPTIONS,
	MENU_CREDIT,
	MENU_MAX,
};

enum POINT_VALIDATION
{
	POINT_PENDING = 0,	// Before first boucne
	POINT_VALIDATED,	// First boucne was valid
	POINT_FINISHED,		// Point already checked
	POINT_MAX,
};

enum POINT
{
	POINT_0 = 0,
	POINT_15,
	POINT_30,
	POINT_40,
	POINT_ADV,
	POINT_WIN,
};

/// Rule event
enum EVENT
{
	EVENT_NONE = 0,
	EVENT_POINT,
	EVENT_GAME,
	EVENT_SET,
	EVENT_MATCH,
	EVENT_OUT,
	EVENT_FAULT,
	EVENT_DBLFAULT,
	EVENT_NET,
	EVENT_LET,
	EVENT_ACE,
};

enum SIDE
{
	SIDE_BOTTOM = 0,
	SIDE_TOP,
};

enum BIND
{
	BIND_KB1A = 0,	// Up, Left, Down, Right + Space, N
	BIND_KB1B,  	// Up, Left, Down, Right + Shift, Ctrl
	BIND_KB1C,  	// Up, Left, Down, Right + Return, BS
	BIND_KB2,   	// E, S, D, F + Shift, Ctrl
	BIND_JOY1,  	// Joystick 1 stick + triggers
	BIND_JOY2,  	// Joystick 2 stick + triggers
	BIND_MAX,
};

//-----------------------------------------------------------------------------
// TYPES

// Functions
typedef bool (*inputFct)(void);	///< Input check function signature

/// 8-bits vector structure
typedef struct
{
	u8			x;	
	u8			y;	
} Vector8;

/// 16-bits vector structure
typedef struct
{
	i16			x;	
	i16			y;	
} Vector16;

/// Binding
typedef struct
{
	const c8*   name;
	inputFct	inUp;
	inputFct	inDown;
	inputFct	inLeft;
	inputFct	inRight;
	inputFct	inButton1;
	inputFct	inButton2;
} Binding;

/// Player structure
typedef struct
{
	Vector16	pos;
	Vector8		srcPos;	// Ball screen position (Q8.0 format)
	u8			id;
	u8			action;
	u8			counter;
	u8			shot;
	u8			shotDir;
	u8			shotCnt;
	u8			step;
	u8			anim;
	u8			prevAnim[2]; // buffered animation frame
	u8			binding;
	// inputFct	inUp;
	// inputFct	inDown;
	// inputFct	inLeft;
	// inputFct	inRight;
	// inputFct	inButton1;
	// inputFct	inButton2;
	inputFct	inLong;
	inputFct	inShort;
} Player;

/// Player structure
typedef struct
{
	Vector16	pos;	// Ball position (Q10.6 format)
	Vector8		srcPos;	// Ball screen position (Q8.0 format)
	i16			height; // Ball height (Q10.6 format)
	u8			dir;	// Ball direction (0-63)
	i16			velXY;  // Ball horizontal velocity (Q10.6 format)
	i16			velZ;   // Ball vertical velocity (Q10.6 format)
	u8          coolDown;
	u8			lastPly;
	u8			bounce;
	u8			point;
	u8			spin;
} Ball;

/// Animation strcture
typedef struct
{
	u8			frame;
	u8			len;
	u8			event;
} Anim;

/// Action strcture
typedef struct
{
	const Anim* animFrames;
	u8			animLen;
	u8			loop;      // looping action?
	u8			interrupt; // can be interrupted?
} Action;

///
typedef struct
{
	Vector8		min;
	Vector8		max;
} Field;

///
typedef struct
{
	i16			velXY;  // Ball horizontal velocity (Q10.6 format)
	i16			velZ;   // Ball vertical velocity (Q10.6 format)
	u8			spin;
} Shot;

///
typedef struct
{
	Vector8		pos;
	u8			dir;
	u8			y;
	u8			pattern;
} TrainSide;

//-----------------------------------------------------------------------------
// FUNCTIONS PROTOTYPE

// States callback
bool State_Init();
bool State_TitleStart();
bool State_TitleUpdate();
bool State_MenuStart();
bool State_MenuUpdate();
bool State_ScoreStart();
bool State_ScoreUpdate();
bool State_CourtRestore();
bool State_MatchStart();
bool State_MatchUpdate();
bool State_WinPoint();
bool State_WinGame();
bool State_WinSet();
bool State_WinMatch();
bool State_TrainingStart();
bool State_TrainingUpdate();

// Menu callback
const char* Menu_StartMatch(u8 op, i8 value);
const char* Menu_StartTrain(u8 op, i8 value);
const char* Menu_SetAI(u8 op, i8 value);
const char* Menu_SetSets(u8 op, i8 value);
const char* Menu_SetShot(u8 op, i8 value);
const char* Menu_SetSide(u8 op, i8 value);
const char* Menu_CreditScroll(u8 op, i8 value);
const char* Menu_SetInput(u8 op, i8 value);
const char* Menu_SetMusic(u8 op, i8 value);

// Input callback
bool KB1_Up();
bool KB1_Down();
bool KB1_Left();
bool KB1_Right();
bool KB1A_Button1();
bool KB1A_Button2();
bool KB1B_Button1();
bool KB1B_Button2();
bool KB1C_Button1();
bool KB1C_Button2();
bool KB2_Up();
bool KB2_Down();
bool KB2_Left();
bool KB2_Right();
bool KB2_Button1();
bool KB2_Button2();
bool Joy1_Up();
bool Joy1_Down();
bool Joy1_Left();
bool Joy1_Right();
bool Joy1_Button1();
bool Joy1_Button2();
bool Joy2_Up();
bool Joy2_Down();
bool Joy2_Left();
bool Joy2_Right();
bool Joy2_Button1();
bool Joy2_Button2();

//-----------------------------------------------------------------------------
// EXTERNAL VARIABLES

#if (TARGET_TYPE == TARGET_TYPE_ROM)
	extern const u8 g_VersionROM;
	extern const u8 g_VersionMSX;
#endif

//_____________________________________________________________________________
//  ▄▄▄   ▄▄  ▄▄▄▄  ▄▄ 
//  ██ █ ██▄█  ██  ██▄█
//  ██▄▀ ██ █  ██  ██ █
//_____________________________________________________________________________

//=============================================================================
//
//  ROM DATA - PAGE 0-2
//
//=============================================================================

// Data
#include "data.h"

// Animation
const Anim g_FramesIdle[] = 
{
	{ 19, 10, FUNCT_NONE },
	{ 18, 10, FUNCT_NONE },
	{ 19, 10, FUNCT_NONE },
	{ 20, 10, FUNCT_NONE },
};
const Anim g_FramesWalkF[] = 
{
	{ 3, 8, FUNCT_NONE },
	{ 4, 8, FUNCT_NONE },
	{ 5, 8, FUNCT_NONE },
	{ 4, 8, FUNCT_NONE },
};
const Anim g_FramesWalkR[] = 
{
	{ 8, 8, FUNCT_NONE },
	{ 7, 8, FUNCT_NONE },
	{ 6, 8, FUNCT_NONE },
	{ 7, 8, FUNCT_NONE },
};
const Anim g_FramesWalkL[] = 
{
	{ 2, 8, FUNCT_NONE },
	{ 1, 8, FUNCT_NONE },
	{ 0, 8, FUNCT_NONE },
	{ 1, 8, FUNCT_NONE },
};
const Anim g_FramesShootR[] = 
{
	{ 15,  4, FUNCT_PREPARE },
	{ 16,  8, FUNCT_SHOOT_R }, // do shoot
	{ 17, 16, FUNCT_NONE },
};
const Anim g_FramesShootL[] = 
{
	{  9,  4, FUNCT_PREPARE },
	{ 10,  8, FUNCT_SHOOT_L }, // do shoot
	{ 11, 16, FUNCT_NONE },
};
const Anim g_FramesSmash[] = 
{
	{ 12,  4, FUNCT_PREPARE },
	{ 13,  8, FUNCT_SMASH }, // do smash
	{ 14, 16, FUNCT_NONE },
};

// Actions list
const Action g_Actions[] =
{
	/* ACTION_IDLE    */ { g_FramesIdle,   numberof(g_FramesIdle),   1, 1 },
	/* ACTION_WALK_F  */ { g_FramesWalkF,  numberof(g_FramesWalkF),  1, 1 },
	/* ACTION_WALK_R  */ { g_FramesWalkR,  numberof(g_FramesWalkR),  1, 1 },
	/* ACTION_WALK_L  */ { g_FramesWalkL,  numberof(g_FramesWalkL),  1, 1 },
	/* ACTION_SHOOT_R */ { g_FramesShootR, numberof(g_FramesShootR), 0, 0 },
	/* ACTION_SHOOT_L */ { g_FramesShootL, numberof(g_FramesShootL), 0, 0 },
	/* ACTION_SMASH   */ { g_FramesSmash,  numberof(g_FramesSmash),  0, 0 },
};

// Direction index
// +---+---+---+
// | 0 | 3 | 5 |
// +---+---+---+
// | 1 |   | 6 |
// +---+---+---+
// | 2 | 4 | 7 |
// +---+---+---+

///
const Field g_Field[2] =
{
	{ { 55, 31 }, { 200, 95 } },	// Top field    (Player 1 target)
	{ { 55, 96 }, { 200, 160 } },	// Bottom field (Player 2 target)
};

/// Movement
const Vector16 g_Move[8] =
{
	{ -MOVE_DIAG, -MOVE_DIAG }, // 🡤
	{ -MOVE_MAIN,          0 }, // 🡠
	{ -MOVE_DIAG,  MOVE_DIAG }, // 🡧
	{          0, -MOVE_MAIN }, // 🡡
	{          0,  MOVE_MAIN }, // 🡣
	{  MOVE_DIAG, -MOVE_DIAG }, // 🡥
	{  MOVE_MAIN,          0 }, // 🡢
	{  MOVE_DIAG,  MOVE_DIAG }, // 🡦
};

///
const u8 g_ColorShadeDefault[8] =
{ 
	COLOR_MERGE(0xF, 0x1), 
	COLOR_MERGE(0xF, 0x1), 
	COLOR_MERGE(0xF, 0x1), 
	COLOR_MERGE(0xF, 0x1), 
	COLOR_MERGE(0xF, 0x1), 
	COLOR_MERGE(0xF, 0x1), 
	COLOR_MERGE(0xE, 0x1), 
	COLOR_MERGE(0xE, 0x1)
};

///
const u8 g_ColorShadeSelect[8] =
{ 
	COLOR_MERGE(9, 0x1), 
	COLOR_MERGE(9, 0x1), 
	COLOR_MERGE(8, 0x1), 
	COLOR_MERGE(8, 0x1), 
	COLOR_MERGE(8, 0x1), 
	COLOR_MERGE(6, 0x1), 
	COLOR_MERGE(6, 0x1), 
	COLOR_MERGE(6, 0x1)
};

// Menu 0 - Main
const MenuEntry g_MenuMain[] =
{
	{ "1P MATCH",	MENU_ITEM_GOTO|MENU_MATCH1P, 0, 0 },
	{ "2P MATCH",	MENU_ITEM_GOTO|MENU_MATCH2P, 0, 0 },
	{ "TRAINING",	MENU_ITEM_GOTO|MENU_TRAINING, 0, 0 },
	{ "OPTIONS", 	MENU_ITEM_GOTO|MENU_OPTIONS, 0, 0 },
	{ "CREDITS", 	MENU_ITEM_GOTO|MENU_CREDIT, 0, 0 },
	{ "",        	MENU_ITEM_DISABLE, 0, 0 },
	{ "",        	MENU_ITEM_DISABLE, 0, 0 },
	{ "",        	MENU_ITEM_DISABLE, 0, 0 },
	{ "",        	MENU_ITEM_DISABLE, 0, 0 },
	{ VERSION,   	MENU_ITEM_DISABLE, 0, 0 },
};

// Menu 1 - Match 1P
const MenuEntry g_MenuMatch1P[] =
{
	{ "START>",		MENU_ITEM_ACTION, Menu_StartMatch, 0 },
	{ "SETS",		MENU_ITEM_ACTION, Menu_SetSets, 0 },
	{ "AI",			MENU_ITEM_ACTION, Menu_SetAI, 0 },
	{ "INPUT",		MENU_ITEM_ACTION, Menu_SetInput, 0 },
	{ "",			MENU_ITEM_DISABLE, 0, 0 },
	{ "",			MENU_ITEM_DISABLE, 0, 0 },
	{ "",			MENU_ITEM_DISABLE, 0, 0 },
	{ "",			MENU_ITEM_DISABLE, 0, 0 },
	{ "",			MENU_ITEM_DISABLE, 0, 0 },
	{ "<BACK",		MENU_ITEM_GOTO|MENU_MAIN, 0, 0 },
};

// Menu 2 - Match 2P
const MenuEntry g_MenuMatch2P[] =
{
	{ "START>",		MENU_ITEM_ACTION, Menu_StartMatch, 0 },
	{ "SETS",		MENU_ITEM_ACTION, Menu_SetSets, 0 },
	{ "P1 INPUT",	MENU_ITEM_ACTION, Menu_SetInput, 0 },
	{ "P2 INPUT",	MENU_ITEM_ACTION, Menu_SetInput, 1 },
	{ "",			MENU_ITEM_DISABLE, 0, 0 },
	{ "",			MENU_ITEM_DISABLE, 0, 0 },
	{ "",			MENU_ITEM_DISABLE, 0, 0 },
	{ "",			MENU_ITEM_DISABLE, 0, 0 },
	{ "",			MENU_ITEM_DISABLE, 0, 0 },
	{ "<BACK",		MENU_ITEM_GOTO|MENU_MAIN, 0, 0 },
};

// Menu 3 - Training
const MenuEntry g_MenuTraining[] =
{
	{ "START>",		MENU_ITEM_ACTION, Menu_StartTrain, 0 },
	{ "SIDE",		MENU_ITEM_ACTION, Menu_SetSide, 0 },
	{ "SHOT",		MENU_ITEM_ACTION, Menu_SetShot, 0 },
	{ "INPUT",		MENU_ITEM_ACTION, Menu_SetInput, 0 },
	{ "",			MENU_ITEM_DISABLE, 0, 0 },
	{ "",			MENU_ITEM_DISABLE, 0, 0 },
	{ "",			MENU_ITEM_DISABLE, 0, 0 },
	{ "",			MENU_ITEM_DISABLE, 0, 0 },
	{ "",			MENU_ITEM_DISABLE, 0, 0 },
	{ "<BACK",		MENU_ITEM_GOTO|MENU_MAIN, 0, 0 },
};

// Menu 4 - Option
const MenuEntry g_MenuOption[] =
{
	{ "MUSIC",		MENU_ITEM_ACTION, &Menu_SetMusic, 0 },
	{ "SFX",		MENU_ITEM_BOOL, &g_PlaySFX, 0 },
	{ "SHADE",		MENU_ITEM_BOOL, &g_FlickerShadow, 0 },
	{ "P1 INPUT",	MENU_ITEM_ACTION, Menu_SetInput, 0 },
	{ "P2 INPUT",	MENU_ITEM_ACTION, Menu_SetInput, 1 },
	{ "",			MENU_ITEM_DISABLE, 0, 0 },
	{ "",			MENU_ITEM_DISABLE, 0, 0 },
	{ "",			MENU_ITEM_DISABLE, 0, 0 },
	{ "",			MENU_ITEM_DISABLE, 0, 0 },
	{ "<BACK",		MENU_ITEM_GOTO|MENU_MAIN, 0, 0 },
};

// Menu 5 - Credits
const MenuEntry g_MenuCredits[] =
{
	{ "PIXEL PHENIX # 2021",	MENU_ITEM_TEXT, 0, (i8)-6 },
	{ "",						MENU_ITEM_DISABLE, 0, 0 },
	{ "CODE:  AOINEKO",			MENU_ITEM_TEXT, 0, (i8)-6 },
	{ "GRAPH: AOINEKO & GFX",	MENU_ITEM_TEXT, 0, (i8)-6 },
	{ "MUSIC: ???",				MENU_ITEM_TEXT, 0, (i8)-6 },
	{ "SFX:   ???",				MENU_ITEM_TEXT, 0, (i8)-6 },
	{ "",						MENU_ITEM_DISABLE, 0, 0 },
	{ "",						MENU_ITEM_DISABLE, 0, 0 },//MENU_ITEM_UPDATE, Menu_CreditScroll, (i8)-6 },
	{ "",						MENU_ITEM_DISABLE, 0, 0 },
	{ "<BACK",					MENU_ITEM_GOTO|MENU_MAIN, 0, 0 },
};

///
const Menu g_Menus[MENU_MAX] =
{
	{ "",		g_MenuMain,			numberof(g_MenuMain) },
	{ "",		g_MenuMatch1P,		numberof(g_MenuMatch1P) },
	{ "",		g_MenuMatch2P,		numberof(g_MenuMatch2P) },
	{ "",  		g_MenuTraining,		numberof(g_MenuTraining) },
	{ "",		g_MenuOption,		numberof(g_MenuOption) },
	{ "",		g_MenuCredits,		numberof(g_MenuCredits) },
};

///
const Shot g_Shots[] =
{
	//	ZY				Z				Spin
	// Normal shot - Long
	{ PX_TO_UNIT(3.5), PX_TO_UNIT(3.0), SPIN_FLAT },
	// Normal shot - Med
	{ PX_TO_UNIT(3.0), PX_TO_UNIT(2.7), SPIN_FLAT },
	// Normal shot - Short
	{ PX_TO_UNIT(2.0), PX_TO_UNIT(3.0), SPIN_FLAT },
	// Normal shot - Smash
	{ PX_TO_UNIT(5.0), PX_TO_UNIT(-0.5), SPIN_FLAT },
	
	// High shot - Long
	{ PX_TO_UNIT(2.2), PX_TO_UNIT(5.0), SPIN_FLAT },	// Lob
	// High shot - Med
	{ PX_TO_UNIT(2.2), PX_TO_UNIT(4.0), SPIN_FLAT },
	// High shot - Short
	{ PX_TO_UNIT(1.7), PX_TO_UNIT(3.0), SPIN_BACK },	// Down shot
	// High shot - Smash
	{ PX_TO_UNIT(4.0), PX_TO_UNIT(0.0), SPIN_FLAT },
	
	// Low shot - Long
	{ PX_TO_UNIT(4.0), PX_TO_UNIT(1.9), SPIN_TOP },
	// Low shot - Med
	{ PX_TO_UNIT(3.0), PX_TO_UNIT(2.3), SPIN_TOP },
	// Low shot - Short
	{ PX_TO_UNIT(2.0), PX_TO_UNIT(2.7), SPIN_FLAT },
	// Low shot - Smash
	{ PX_TO_UNIT(6.0), PX_TO_UNIT(-0.5), SPIN_FLAT },
	
};

///
const TrainSide g_TrainSideData[] = 
{
	{ { 128, 24     }, 44, 0,   40 },
	{ { 128, 192-24 }, 12, 144, 0 },
};

/// Input bindings
const Binding g_Binding[BIND_MAX] =
{
	// BIND_KB1A - Up, Left, Down, Right + Space, N
	{ "[]SPACE",	KB1_Up,	KB1_Down,	KB1_Left,	KB1_Right,	KB1A_Button1,	KB1A_Button2 },
	// BIND_KB1B - Up, Left, Down, Right + Shift, Ctrl
	{ "[]SHFT",		KB1_Up,	KB1_Down,	KB1_Left,	KB1_Right,	KB1B_Button1,	KB1B_Button2 },
	// BIND_KB1C - Up, Left, Down, Right + Return, BS
	{ "[]RET",		KB1_Up,	KB1_Down,	KB1_Left,	KB1_Right,	KB1C_Button1,	KB1C_Button2 },
	// BIND_KB2  - E, S, D, F + Shift, Ctrl
	{ "ESDF+SHFT",	KB2_Up,	KB2_Down,	KB2_Left,	KB2_Right,	KB1B_Button1,	KB1B_Button2 },
	// BIND_JOY1 - Joystick 1 stick + triggers
	{ "^1",			Joy1_Up,	Joy1_Down,	Joy1_Left,	Joy1_Right,	Joy1_Button1,	Joy1_Button2 },
	// BIND_JOY2 - Joystick 2 stick + triggers
	{ "^2",			Joy2_Up,	Joy2_Down,	Joy2_Left,	Joy2_Right,	Joy2_Button1,	Joy2_Button2 },
};

///
const u8 g_CourtNet[32] = { 
	0xFF, 0xFF, 0xFF, 3, 0x80, 1, 0x80, 2, 0x80, 0,    0, 0,    0, 0x80,    1, 0x80,
	2,    0x80,    0, 0,    0, 0, 0x80, 1, 0x80, 2, 0x80, 5, 0x80, 0xFF, 0xFF, 0xFF,
};

///                                                    Adv
const c8* g_ScoreString[] = { "00", "15", "30", "40", ":;" };

///                             P1    P2    AI
const c8* g_PlayerString[] = { ">1", ">2", "<=" };

const c8* g_CrediScroll = "______________________DEDICATED_TO_MY_WONDERFUL_WIFE_AND_SON_$$____THANKS_TO_ALL_MSX-VILLAGE_AND_MRC_MEMBERS_FOR_SUPPORT!____MSX'LL_NEVER_DIE.____PRAISE_THE_HOLY-BRIOCHE!_\\O/";

//=============================================================================
//
//  RAM DATA - PAGE 3
//
//=============================================================================

// Gameplay
Player 		g_Player[2];
Ball		g_Ball;
callback	g_ScoreFct = null;

// Configuration
u8			g_OptionSets = 1;			///< Sets count (0=1 set, 1=3 sets, 2=5 sets)
u8			g_FlickerShadow = true;
u8			g_PlayMusic = false;
u8			g_PlaySFX = false;
u8			g_InputBinding[2];

// System
u16			g_SpritePattern[2];
u8			g_WriteBuffer = 0;
u8			g_DisplayBuffer = 1;
u8			g_NetPostSprite = 0xFF;
#if (MSX2_ENHANCE)
	u8		g_VersionVDP;
#endif
#if (DEBUG)
	u8		g_Debug = 0;
#endif

// Input
u8			g_PrevRow[9];
u8			g_KeyRow[9] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
u8			g_PrevJoy1;
u8			g_PrevJoy2;
u8			g_Joy1 = 0xFF;
u8			g_Joy2 = 0xFF;

// Menu
u8			g_StateFrame = 0;
const Menu*	g_CurrentMenu; 
u8			g_MenuID = 0; 
u8			g_MenuItem = 0; 

// Match
u8			g_Level = 1;			///< AI level (0=Easy, 1=Medium, 2=Hard)
u8			g_EventID;				///< Match event (@see EVENT enum)
u8			g_EventTimer;			///< Match event timer (event start when timer is equal to 0)
u8			g_Points[2];
u8			g_Games[5][2];
u8			g_Sets[2];
u8			g_CurSet;
u8			g_NumSet;

// Training
u8			g_TrainScore;
u8			g_TrainBest = 0;
u8			g_TrainSide = SIDE_BOTTOM;
u8			g_TrainShot = TRAIN_BOTH;
u8			g_TrainSpin = SPIN_FLAT;

// Misc
c8			g_CrediScrollBug[SCROLL_BUF_SIZE];
u8			g_CrediScrollCnt = 0;

//_____________________________________________________________________________
//   ▄▄   ▄▄  ▄▄▄  ▄▄▄▄
//  ██ ▀ ██ █ ██ █ ██▄ 
//  ▀█▄▀ ▀█▄▀ ██▄▀ ██▄▄
//_____________________________________________________________________________

//=============================================================================
//
//   M E N U
//
//=============================================================================

//-----------------------------------------------------------------------------
///
const c8* Menu_StartMatch(u8 op, i8 value)
{
	if(op == MENU_ACTION_SET)
		Game_SetState(State_MatchStart);
	return ""; 
}

//-----------------------------------------------------------------------------
///
const c8* Menu_StartTrain(u8 op, i8 value)
{
	if(op == MENU_ACTION_SET)
		Game_SetState(State_TrainingStart);
	return "";
}

//-----------------------------------------------------------------------------
///
const char* Menu_SetAI(u8 op, i8 value)
{
	switch(op)
	{
	case MENU_ACTION_INC: g_Level = (g_Level + 1) % 3; break;
	case MENU_ACTION_DEC: g_Level = (g_Level + 2) % 3; break;	
	}

	if(g_Level == 0)
		return "EASY";
	else if(g_Level == 1)
		return "MED";

	return "HARD";
}

//-----------------------------------------------------------------------------
///
const char* Menu_SetSets(u8 op, i8 value)
{
	switch(op)
	{
	case MENU_ACTION_INC: g_OptionSets = (g_OptionSets + 1) % 3; break;
	case MENU_ACTION_DEC: g_OptionSets = (g_OptionSets + 2) % 3; break;	
	}

	if(g_OptionSets == 0)
		return "1";
	else if(g_OptionSets == 1)
		return "3";

	return "5";
}

//-----------------------------------------------------------------------------
///
const char* Menu_SetShot(u8 op, i8 value)
{
	switch(op)
	{
	case MENU_ACTION_INC: g_TrainShot = (g_TrainShot + 1) % TRAIN_MAX; break;
	case MENU_ACTION_DEC: g_TrainShot = (g_TrainShot + (TRAIN_MAX-1)) % TRAIN_MAX; break;	
	}
	
	switch(g_TrainShot)
	{
	case TRAIN_BOTH: return "BOTH";
	case TRAIN_FLAT: return "FLAT";
	case TRAIN_LOB:  return "LOB";
	};
}

//-----------------------------------------------------------------------------
///
const char* Menu_SetMusic(u8 op, i8 value)
{
	switch(op)
	{
	case MENU_ACTION_INC:
	case MENU_ACTION_DEC:
		g_PlayMusic = 1 - g_PlayMusic;
		if(g_PlayMusic)
			PT3_Resume();
		else
			PT3_Pause();
		break;	
	}
	
	return g_PlayMusic ? "/" : "\\";
}


//-----------------------------------------------------------------------------
///
const char* Menu_SetInput(u8 op, i8 value)
{
	switch(op)
	{
	case MENU_ACTION_INC: g_InputBinding[value] = (g_InputBinding[value] + 1) % BIND_MAX; break;
	case MENU_ACTION_DEC: g_InputBinding[value] = (g_InputBinding[value] + (BIND_MAX-1)) % BIND_MAX; break;	
	}
	
	return g_Binding[g_InputBinding[value]].name;
}

//-----------------------------------------------------------------------------
///
const char* Menu_SetSide(u8 op, i8 value)
{
	switch(op)
	{
	case MENU_ACTION_INC:
	case MENU_ACTION_DEC:
		 g_TrainSide = 1 - g_TrainSide; break;	
	}
	
	if(g_TrainSide == SIDE_BOTTOM)
		return "BOTTOM";
	else
		return "TOP";
}

//-----------------------------------------------------------------------------
///
const char* Menu_CreditScroll(u8 op, i8 value)
{
	if(op == MENU_ACTION_GET)
	{	
		u8 len = String_Length(g_CrediScroll);
		for(u8 i=0; i < SCROLL_BUF_SIZE-1; i++)
		{
			u8 j = (i + g_CrediScrollCnt) % len;
			g_CrediScrollBug[i] = g_CrediScroll[j];
		}
		g_CrediScrollBug[SCROLL_BUF_SIZE-1] = 0;
		g_CrediScrollCnt++;
		g_CrediScrollCnt %= len;
	}
	return g_CrediScrollBug;
}


#pragma save
#pragma disable_warning	244		///< remove "pointer types incompatible" warning

//-----------------------------------------------------------------------------
///
void Menu_DisplayItem(u8 item) __FASTCALL
{
	// Clean buttom-third
	u16 dst = g_ScreenLayoutLow + ((MENU_ITEMS_Y + item) * 32);
	VDP_FillVRAM_64K(0, dst, 32);
	
	// Draw item
	if(item < g_CurrentMenu->itemNum)
	{
		MenuEntry* pCurEntry = &g_CurrentMenu->items[item];
		if(g_MenuItem == item)
			Print_SelectTextFont(g_DataFont, OFFSET_TITLE_FONT_ALT);
		else
			Print_SelectTextFont(g_DataFont, OFFSET_TITLE_FONT_DEF);
			
		u8 x = MENU_ITEMS_X;
		u8 y = MENU_ITEMS_Y;
		
		if(pCurEntry->type == MENU_ITEM_TEXT)
			x += pCurEntry->value;
			
		Print_SetPosition(x, y + item);
		Print_DrawText(pCurEntry->text);

		Print_SetPosition(x + 9, y + item);

		if(pCurEntry->type == MENU_ITEM_ACTION)
		{
			menuCallback cb = (menuCallback)pCurEntry->action;
			const c8* str = cb(MENU_ACTION_GET, pCurEntry->value);
			if(*str)
			{
				if(g_MenuItem == item)
					Print_DrawChar('<');
				else
					Print_Space();
				Print_DrawText(str);
				if(g_MenuItem == item)
					Print_DrawChar('>');
			}
		}
		else if(pCurEntry->type == MENU_ITEM_INT)
		{
			i8* data = (u8*)pCurEntry->action;
			if(g_MenuItem == item)
				Print_DrawChar('<');
			else
				Print_Space();
			Print_DrawInt(*data);
			if(g_MenuItem == item)
				Print_DrawChar('>');
		}
		else if(pCurEntry->type == MENU_ITEM_BOOL)
		{
			u8* data = (u8*)pCurEntry->action;
			if(g_MenuItem == item)
				Print_DrawChar('<');
			else
				Print_Space();
			Print_DrawChar(*data ? '/' : '\\');
			if(g_MenuItem == item)
				Print_DrawChar('>');
		}
	}
}

//-----------------------------------------------------------------------------
///
void Menu_Initialize(const Menu* menu) __FASTCALL
{
	// Initialize menu
	g_CurrentMenu = menu;
	g_MenuItem = 0;
	while(g_MenuItem < g_CurrentMenu->itemNum)
	{
		if(g_CurrentMenu->items[g_MenuItem].type >= MENU_ITEM_ACTION)
			break;
		g_MenuItem++;
	}
	
	// Display menu items
	for(u8 item = 0; item < MENU_ITEMS_H; item++)
	{
		Menu_DisplayItem(item);
	}
}

//-----------------------------------------------------------------------------
///
void Menu_Update()
{
	// Handle activation
	MenuEntry* pCurEntry = &g_CurrentMenu->items[g_MenuItem];
	if(KEY_PRESS(KEY_SPACE))
	{
		if(pCurEntry->type == MENU_ITEM_ACTION)
		{
			menuCallback cb = (menuCallback)pCurEntry->action;
			cb(MENU_ACTION_SET, pCurEntry->value);
			Menu_DisplayItem(g_MenuItem);
		}
		if((pCurEntry->type & MENU_ITEM_GOTO) != 0)
		{
			Menu_Initialize(&g_Menus[pCurEntry->type & 0x7F]);
		}
		return;
	}

	// Handle change value
	if(KEY_PRESS(KEY_RIGHT))
	{
		if(pCurEntry->type == MENU_ITEM_ACTION)
		{
			menuCallback cb = (menuCallback)pCurEntry->action;
			cb(MENU_ACTION_INC, pCurEntry->value);
		}
		else if(pCurEntry->type == MENU_ITEM_INT)
		{
			i8* data = (u8*)pCurEntry->action;
			*data++;
		}
		else if(pCurEntry->type == MENU_ITEM_BOOL)
		{
			u8* data = (u8*)pCurEntry->action;
			*data = 1 - *data;
		}
		Menu_DisplayItem(g_MenuItem);
	}
	else if(KEY_PRESS(KEY_LEFT))
	{
		if(pCurEntry->type == MENU_ITEM_ACTION)
		{
			menuCallback cb = (menuCallback)pCurEntry->action;
			cb(MENU_ACTION_DEC, pCurEntry->value);
		}
		else if(pCurEntry->type == MENU_ITEM_INT)
		{
			i8* data = (u8*)pCurEntry->action;
			*data--;
		}
		else if(pCurEntry->type == MENU_ITEM_BOOL)
		{
			u8* data = (u8*)pCurEntry->action;
			*data = 1 - *data;
		}
		Menu_DisplayItem(g_MenuItem);
	}
	
	// Handle navigation
	if(KEY_PRESS(KEY_UP))
	{
		u8 i = g_MenuItem;
		while(i > 0)
		{
			i--;
			if(g_CurrentMenu->items[i].type >= MENU_ITEM_ACTION)
			{
				u8 prev = g_MenuItem;
				g_MenuItem = i;
				Menu_DisplayItem(prev);
				Menu_DisplayItem(g_MenuItem);
				break;
			}
		}			
	}
	else if(KEY_PRESS(KEY_DOWN))
	{
		u8 i = g_MenuItem;
		while(i < g_CurrentMenu->itemNum-1)
		{
			i++;
			if(g_CurrentMenu->items[i].type >= MENU_ITEM_ACTION)
			{
				u8 prev = g_MenuItem;
				g_MenuItem = i;
				Menu_DisplayItem(prev);
				Menu_DisplayItem(g_MenuItem);
				break;
			}
		}
	}
	
	// Update menu items
	if((g_Frame & 0x07) == 0) // 8th frame
	{
		Print_SelectTextFont(g_DataFont, OFFSET_TITLE_FONT_DEF);
		for(u8 item = 0; item < g_CurrentMenu->itemNum; item++)
		{
			MenuEntry* pCurEntry = &g_CurrentMenu->items[item];
			if(pCurEntry->type == MENU_ITEM_UPDATE)
			{
				menuCallback cb = (menuCallback)pCurEntry->action;
				const c8* str = cb(MENU_ACTION_GET, pCurEntry->value);
				u8 x = MENU_ITEMS_X;
				x += pCurEntry->value;			
				Print_SetPosition(x, MENU_ITEMS_Y + item);
				Print_DrawText(str);
			}
		}
	}
}

#pragma restore		///< restore "pointer types incompatible" warning


//=============================================================================
//
//   B A L L
//
//=============================================================================

//-----------------------------------------------------------------------------
///
void Ball_ShootRandom()
{
	// Position
	g_Ball.height = PX_TO_UNIT(10);
	g_Ball.pos.x = PX_TO_UNIT(g_TrainSideData[g_TrainSide].pos.x);
	g_Ball.pos.y = PX_TO_UNIT(g_TrainSideData[g_TrainSide].pos.y);
	g_Ball.srcPos.x = UNIT_TO_PX(g_Ball.pos.x);
	g_Ball.srcPos.y = UNIT_TO_PX(g_Ball.pos.y);

	u16 rnd = Math_GetRandom();

	// Direction
	g_Ball.dir = rnd;
	g_Ball.dir %= 9;
	g_Ball.dir += g_TrainSideData[g_TrainSide].dir;

	// Velocity
	if(((g_TrainShot == TRAIN_FLAT)) || ((g_TrainShot == TRAIN_BOTH) && (rnd & BIT_8))) // top spine
	{
		g_Ball.velXY = PX_TO_UNIT(3);
		g_Ball.velZ = PX_TO_UNIT(3);
		g_Ball.spin = SPIN_FLAT;
	}
	else // lob
	{
		g_Ball.velXY = PX_TO_UNIT(2);
		g_Ball.velZ = PX_TO_UNIT(5);
		g_Ball.spin = SPIN_FLAT;
	}

	// Misc
	g_Ball.lastPly = 1 - g_TrainSide;
	g_Ball.coolDown = 0;
	g_Ball.bounce = 0;
	g_Ball.point = POINT_PENDING;
		
	if(g_PlaySFX)
		ayFX_PlayBank(1, 0);
}

//-----------------------------------------------------------------------------
///
void Ball_Prepare()
{
	// Flicker Shadows
	u8 pat = 80;
	if(g_FlickerShadow && (g_Frame & 0x1))
	{
		pat += 4;
	}
	VDP_SetSpritePattern(SPRITE_BALL_OUTLINE, pat);
}

//-----------------------------------------------------------------------------
///
bool Ball_CheckField()
{
	const Field* field;
	if(g_Ball.lastPly == SIDE_BOTTOM)
		field = &g_Field[SIDE_BOTTOM];
	else
		field = &g_Field[SIDE_TOP];

	if(g_Ball.srcPos.x < field->min.x)
		return false;
	if(g_Ball.srcPos.x > field->max.x)
		return false;
	if(g_Ball.srcPos.y < field->min.y)
		return false;
	if(g_Ball.srcPos.y > field->max.y)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
///
void Ball_Update()
{
	// Horizontal force
	i16 dx = g_Ball.velXY * (i16)g_Cosinus64[g_Ball.dir];
	dx /= 64;
	g_Ball.pos.x += dx;
	
	i16 prevY = g_Ball.pos.y;
	i16 dy = g_Ball.velXY * (i16)g_Sinus64[g_Ball.dir];
	dy /= 64;
	g_Ball.pos.y -= dy;
	g_Ball.srcPos.x = UNIT_TO_PX(g_Ball.pos.x);
	g_Ball.srcPos.y = UNIT_TO_PX(g_Ball.pos.y);
	
	// Vertical force - Gravity
	g_Ball.velZ -= GRAVITY;	
	g_Ball.height += g_Ball.velZ;

	// Bounce
	if(g_Ball.height < 0)
	{
		g_Ball.height = 0;
		g_Ball.bounce++;
		g_Ball.velZ = -g_Ball.velZ * 2 / 3;

		/*if(g_Ball.spin == SPIN_TOP)
		{
		}
		else*/ if(g_Ball.spin == SPIN_BACK)
		{
			g_Ball.velXY = g_Ball.velXY * 2 / 3;
		}
		else if(g_Ball.spin == SPIN_FLAT)
		{
			g_Ball.velXY = g_Ball.velXY * 3 / 4;
		}

		if(g_Ball.bounce == 1)
		{
			// g_Ball.velXY = g_Ball.velXY * 3 / 4;
			if(g_PlaySFX)
				ayFX_PlayBank(12, 0);

			if(Ball_CheckField() && (g_Ball.point == POINT_PENDING))
				g_Ball.point = POINT_VALIDATED;
			else
				g_ScoreFct();
		}
		else if(g_Ball.bounce == 2)
		{
			g_ScoreFct();
			if(g_PlaySFX)
				ayFX_PlayBank(12, 0);
		}
	}

	if(g_Ball.coolDown > 0)
		g_Ball.coolDown--;

	// Check out of screen
	if((g_Ball.pos.y < 0) || (g_Ball.pos.y > PX_TO_UNIT(192)) || (g_Ball.pos.x < 0) || (g_Ball.pos.x > PX_TO_UNIT(256)))
	{
		g_Ball.velXY = 0;
		g_Ball.velZ = 0;
		g_Ball.pos.y = PX_TO_UNIT(246);
		g_ScoreFct();
	}
	// Check net collision
	else if(g_Ball.height < PX_TO_UNIT(14))
	{
		if(((prevY < PX_TO_UNIT(96)) && (g_Ball.pos.y >= PX_TO_UNIT(96))) || (prevY > PX_TO_UNIT(96)) && (g_Ball.pos.y <= PX_TO_UNIT(96)))
		{
			g_Ball.velXY = 0;
			g_Ball.velZ = 0;
			g_ScoreFct();
		}
	}

	// Update sprites position
	u8  x = g_Ball.srcPos.x - 3;
	i16 y = g_Ball.srcPos.y - 3;

	// Ball shadow
	if((y >= 82) && (y < 96))
		VDP_SetSpritePosition(SPRITE_BALL_SHADOW,  213, 213);
	else
		VDP_SetSpritePosition(SPRITE_BALL_SHADOW,  x, y);

	// Ball body
	y -= UNIT_TO_PX(g_Ball.height);
	if(y < 0)
	{
		VDP_SetSpritePosition(SPRITE_BALL_OUTLINE, 213, 213);
		VDP_SetSpritePosition(SPRITE_BALL_BODY,    213, 213);
	}
	else
	{
		VDP_SetSpritePosition(SPRITE_BALL_OUTLINE, x, y);
		VDP_SetSpritePosition(SPRITE_BALL_BODY,    x, y);
	}
}

//-----------------------------------------------------------------------------
///
void Ball_Draw()
{	
	const u8* src = g_DataBall + (1 * 3 * 8);
	u16 dst = g_SpritePattern[g_WriteBuffer] + (80 * 8); // Pattern #80
	VDP_WriteVRAM_64K(src, dst, 8);
	
	src += (1 * 8);
	dst += (4 * 8); // Pattern #84
	VDP_WriteVRAM_64K(src, dst, 8);

	src += (1 * 8);
	dst += (4 * 8); // Pattern #88
	VDP_WriteVRAM_64K(src, dst, 8);
}


//=============================================================================
//
//   P L A Y E R S
//
//=============================================================================

//-----------------------------------------------------------------------------
///
void Player_CheckShoot(Player* ply)
{
	if(g_Ball.coolDown > 0)
		return;
		
	u8 event = g_Actions[ply->action].animFrames[ply->step].event;

	// Check Z coordinate
	if(event == FUNCT_SMASH)
	{
		if(g_Ball.height > PX_TO_UNIT(40))
			return;
	}	
	
	// Check X coordinate
	u8 minX, maxX;
	switch(event)
	{
	case FUNCT_SHOOT_R:
		minX = UNIT_TO_PX(ply->pos.x);
		maxX = minX + SHOT_DETECT_X;
		break;
	case FUNCT_SHOOT_L:
		maxX = UNIT_TO_PX(ply->pos.x);
		minX = maxX - SHOT_DETECT_X;
		break;
	case FUNCT_SMASH:
		minX = UNIT_TO_PX(ply->pos.x) - SHOT_DETECT_SMASH;
		maxX = minX + (2 * SHOT_DETECT_SMASH);
		break;
	default:
		return;
	};
	
	u8 ballX = g_Ball.srcPos.x;
	if((ballX < minX) || (ballX > maxX))
		return;

	// Check Y coordinate
	u8 minY = ply->srcPos.y - SHOT_DETECT_Y;
	u8 maxY = ply->srcPos.y + SHOT_DETECT_Y;
	
	u8 ballY = g_Ball.srcPos.y;
	if((ballY < minY) || (ballY > maxY))
		return;
		
	// Shoot Succed !!!
	
	if(g_PlaySFX)
		ayFX_PlayBank(1, 0);

	// Handle shot direction
	ply->shotCnt = ply->counter;
	//ply->shotCnt -= 4;
	// if(ply->shotCnt > 6)
		// ply->shotCnt = 6;

	const Binding* bind = &g_Binding[ply->binding];
	u8 dir = 16;

	// Set shot direction
	if(ply->id == SIDE_BOTTOM)
	{
		if(event == FUNCT_SHOOT_R)
			dir += 2;
		else if(event == FUNCT_SHOOT_L)
			dir -= 2;

		if(bind->inLeft())
			dir += ply->shotCnt;
		else if(bind->inRight())
			dir -= ply->shotCnt;
	}
	else // if(ply->id == SIDE_TOP)
	{
		dir += 32;
	
		if(event == FUNCT_SHOOT_R)
			dir -= 2;
		else if(event == FUNCT_SHOOT_L)
			dir += 2;

		if(bind->inLeft())
			dir -= ply->shotCnt;
		else if(bind->inRight())
			dir += ply->shotCnt;			
	}
	g_Ball.dir = dir;
	
	// Set shot strength
	u8 shotId = ply->shot * 4; // Long shot
	if(event == FUNCT_SMASH)
	{
		shotId += 3;
	}
	else if(ply->inShort()) // Short shot
	{
		shotId += 2;
	}
	else if(!ply->inLong()) // Med shot
	{
		shotId++;
	}
	
	const Shot* shot = &g_Shots[shotId];

	g_Ball.velXY = shot->velXY;
	g_Ball.velZ  = shot->velZ;
	g_Ball.spin  = shot->spin;

	// Misc
	g_Ball.lastPly = ply->id;
	g_Ball.coolDown = 10;
	g_Ball.bounce = 0;
	g_Ball.point = POINT_PENDING;
}




//-----------------------------------------------------------------------------
// GENERIC

/// Set sprite attribute for Sprite Mode 2 and fill color table with unique color
void SetSprite(u8 index, u8 x, u8 y, u8 shape, u8 color)
{
	// Initialize sprite attributes
	#if (MSX2_ENHANCE)
		if(g_VersionVDP == VDP_VERSION_TMS9918A)
			VDP_SetSpriteSM1(index, x, y, shape, color);
		else
			VDP_SetSpriteExUniColor(index, x, y, shape, color);
	#else
		VDP_SetSpriteSM1(index, x, y, shape, color);
	#endif
}


//-----------------------------------------------------------------------------
///
void SetAction(Player* ply, u8 id)
{
	if(ply->action != id)
	{
		ply->action = id;
		ply->counter = 0;
		ply->step = 0;
	}
}

//-----------------------------------------------------------------------------
///
void UpdateAction(Player* ply) __FASTCALL
{
	const Action* act = &g_Actions[ply->action];

	// Finished current animation step
	if(ply->counter >= act->animFrames[ply->step].len)
	{
		ply->counter = 0;
		ply->step++;
	}

	// Finished last animation step
	if(ply->step >= act->animLen)
	{
		if(act->loop) // restart action
		{
			ply->counter = 0;
			ply->step = 0;
		}
		else // stop action and transit to IDLE
		{
			SetAction(ply, ACTION_IDLE);
			UpdateAction(ply);
			return;
		}
	}
	
	// Handle current action's event
	switch(act->animFrames[ply->step].event)
	{
	case FUNCT_PREPARE:
	case FUNCT_SHOOT_R:
	case FUNCT_SHOOT_L:
	case FUNCT_SMASH:
	{
		/*if(ply->inRight())
		{
			if(ply->shotDir != DIR_RIGHT)
			{
				ply->shotDir = DIR_RIGHT;
				ply->shotCnt = 0;
			}
			ply->shotCnt++;
		}
		if(ply->inLeft())
		{
			if(ply->shotDir != DIR_LEFT)
			{
				ply->shotDir = DIR_LEFT;
				ply->shotCnt = 0;
			}
			ply->shotCnt++;
		}*/
		const Binding* bind = &g_Binding[ply->binding];
		if(bind->inButton1() && (ply->shot == SHOT_SLICE))
			ply->shot = SHOT_ATTACK;
		if(bind->inButton2() && (ply->shot == SHOT_FLAT))
			ply->shot = SHOT_ATTACK;
		break;
	}
	default:
		break;
	};

	// Update animation
	ply->anim = act->animFrames[ply->step].frame;
	ply->counter++;
}

//-----------------------------------------------------------------------------
///
void HandleInput(Player* ply) __FASTCALL
{
	const Binding* bind = &g_Binding[ply->binding];
	if(g_Actions[ply->action].interrupt)
	{
		if(bind->inButton1() || bind->inButton2())
		{
			if(g_Ball.height > PX_TO_UNIT(32))
				SetAction(ply, ACTION_SMASH);
			else
			{
				i16 dY;
				if(ply->id == SIDE_BOTTOM)
					dY = ply->srcPos.y - g_Ball.srcPos.y;
				else // if(ply->id == SIDE_TOP)
					dY = g_Ball.srcPos.y - ply->srcPos.y;
				i16 dX = dY * (i16)g_Cosinus64[g_Ball.dir]; // Q16.0 x Q2.6 => Q10.6
				dX /= 64; // Q10.6  => Q16.0
				if((g_Ball.srcPos.x + dX) > ply->srcPos.x)
					SetAction(ply, ACTION_SHOOT_R);
				else
					SetAction(ply, ACTION_SHOOT_L);
			}
			if(bind->inButton1())
				ply->shot = SHOT_FLAT;
			else // if(bind->inButton2())
				ply->shot = SHOT_SLICE;
			//ply->shotCnt = 0;
		}
		else
		{
			u8 dir = 4;
			if(bind->inUp())
				dir--;
			else if(bind->inDown())
				dir++;
			if(bind->inLeft())
				dir -= 3;
			else if(bind->inRight())
				dir += 3;	
				
			if(dir == 4) // No move
			{
				ply->action = ACTION_IDLE;
			}
			else
			{
				if(dir > 3)
					dir--;

				// Update player position
				ply->pos.x += g_Move[dir].x;
				ply->pos.y += g_Move[dir].y;
				
				// Validate new X position
				if(ply->pos.x < PLY_MIN_X)
					ply->pos.x = PLY_MIN_X;
				else if(ply->pos.x > PLY_MAX_X)
					ply->pos.x = PLY_MAX_X;
				
				// Validate new Y position
				if(ply->id == SIDE_BOTTOM)
				{
					if(ply->pos.y < PLY_DOWN_MIN_X)
						ply->pos.y = PLY_DOWN_MIN_X;
					else if(ply->pos.y > PLY_DOWN_MAX_X)
						ply->pos.y = PLY_DOWN_MAX_X;
				}
				else
				{
					if(ply->pos.y < PLY_UP_MIN_X)
						ply->pos.y = PLY_UP_MIN_X;
					else if(ply->pos.y > PLY_UP_MAX_X)
						ply->pos.y = PLY_UP_MAX_X;
				}

				// Precompute screen position
				ply->srcPos.x = UNIT_TO_PX(ply->pos.x);
				ply->srcPos.y = UNIT_TO_PX(ply->pos.y);
				
				// Set movement action
				if((dir == 3) || (dir == 4)) // Front move
					SetAction(ply, ACTION_WALK_F);
				else if(dir < 3) // Left move
					SetAction(ply, ACTION_WALK_L);
				else // Right move
					SetAction(ply, ACTION_WALK_R);				
			}
		}
	}
}

//-----------------------------------------------------------------------------
///
void PreparePlayerBottom()
{
	// Flicker Shadows
	u8 pat = 0;
	if(g_FlickerShadow && (g_Frame & 0x1))
	{
		pat += 4;
	}
	VDP_SetSpritePattern(SPRITE_PLY1_BLACK_H, pat);      // Pattern #0  / #4
	VDP_SetSpritePattern(SPRITE_PLY1_BLACK_L, pat + 20); // Pattern #20 / #24
}

//-----------------------------------------------------------------------------
///
void UpdatePlayerBottom()
{	
	// Update sprite position
	u8 x = g_Player[SIDE_BOTTOM].srcPos.x - 8;
	u8 y = g_Player[SIDE_BOTTOM].srcPos.y - 24;
	VDP_SetSpritePosition(SPRITE_PLY1_BLACK_H, x, y);
	VDP_SetSpritePosition(SPRITE_PLY1_WHITE_H, x, y);
	VDP_SetSpritePosition(SPRITE_PLY1_SKIN_H, x, y);
	y += 8;
	VDP_SetSpritePosition(SPRITE_PLY1_CLOTH, x, y);
	y += 8;
	VDP_SetSpritePosition(SPRITE_PLY1_BLACK_L, x, y);
	VDP_SetSpritePosition(SPRITE_PLY1_WHITE_L, x, y);
	VDP_SetSpritePosition(SPRITE_PLY1_SKIN_L, x, y);

	// Handle special event
	u8 event = g_Actions[g_Player[SIDE_BOTTOM].action].animFrames[g_Player[SIDE_BOTTOM].step].event;
	switch(event)
	{
	case FUNCT_SHOOT_R:
		Player_CheckShoot(&g_Player[SIDE_BOTTOM]);
		VDP_SetSpritePosition(SPRITE_PLY1_RACKET, x+16, y-16);
		break;
	case FUNCT_SHOOT_L:
		Player_CheckShoot(&g_Player[SIDE_BOTTOM]);
		VDP_SetSpritePosition(SPRITE_PLY1_RACKET, x-8, y-16);
		break;
	case FUNCT_SMASH:
		Player_CheckShoot(&g_Player[SIDE_BOTTOM]);
		VDP_SetSpritePosition(SPRITE_PLY1_RACKET, x+8, y-32);
		break;
	default:
		VDP_SetSpritePositionY(SPRITE_PLY1_RACKET, 213);
		break;
	};
}

//-----------------------------------------------------------------------------
///
void DrawPlayerBottom()
{
	u8 frame = g_Player[SIDE_BOTTOM].anim;
	if(g_Player[SIDE_BOTTOM].prevAnim[g_WriteBuffer] == frame)
		return;

	g_Player[SIDE_BOTTOM].prevAnim[g_WriteBuffer] = frame;

	const u8* src;
	u16 dst;

	src = g_DataPlayer1 + (frame * 224);
	dst = g_SpritePattern[g_WriteBuffer] + (0 * 8); // Pattern #0 - 20
	VDP_WriteVRAM_64K(src, dst, 21 * 8);
	
	src += 21 * 8;
	dst += (22 * 8); // Pattern #22
	VDP_WriteVRAM_64K(src, dst, 8);
	
	src += 8;
	dst += (2 * 8); // Pattern #24
	VDP_WriteVRAM_64K(src, dst, 8);
	
	src += 8;
	dst += (2 * 8); // Pattern #26
	VDP_WriteVRAM_64K(src, dst, 8);

	src += 8;
	dst += (2 * 8);  // Pattern #28
	VDP_WriteVRAM_64K(src, dst, 8);

	src += 8;
	dst += (2 * 8);  // Pattern #30
	VDP_WriteVRAM_64K(src, dst, 8);

	src += 8;
	dst += (2 * 8);  // Pattern #32
	VDP_WriteVRAM_64K(src, dst, 8);

	src += 8;
	dst += (2 * 8);  // Pattern #34
	VDP_WriteVRAM_64K(src, dst, 8);

	u8 event = g_Actions[g_Player[SIDE_BOTTOM].action].animFrames[g_Player[SIDE_BOTTOM].step].event;
	if(event != FUNCT_NONE)
	{
		switch(event)
		{
		case FUNCT_SHOOT_R:
			src = g_DataRacket + (6 * 8);
			break;
		case FUNCT_SHOOT_L:
			src = g_DataRacket + (14 * 8);
			break;
		case FUNCT_SMASH:
			src = g_DataRacket + (2 * 8);
			break;
		};
		dst += (3 * 8);  // Pattern #37
		VDP_WriteVRAM_64K(src, dst, 8);	
	}
}

//-----------------------------------------------------------------------------
///
void PreparePlayerTop()
{
	// Flicker Shadows
	u8 pat = 40;
	if(g_FlickerShadow && (g_Frame & 0x1))
	{
		pat += 4;
	}
	VDP_SetSpritePattern(SPRITE_PLY2_BLACK_H, pat);      // Pattern #40 / #44
	VDP_SetSpritePattern(SPRITE_PLY2_BLACK_L, pat + 16); // Pattern #56 / #60
}

//-----------------------------------------------------------------------------
///
void UpdatePlayerTop()
{	
	// Update sprite position
	u8 x = g_Player[SIDE_TOP].srcPos.x - 8;
	u8 y = g_Player[SIDE_TOP].srcPos.y - 32;
	VDP_SetSpritePosition(SPRITE_PLY2_BLACK_H, x, y);
	VDP_SetSpritePosition(SPRITE_PLY2_WHITE_H, x, y);
	VDP_SetSpritePosition(SPRITE_PLY2_SKIN_H, x, y);
	y += 8;
	VDP_SetSpritePosition(SPRITE_PLY2_CLOTH, x, y);
	y += 8;
	VDP_SetSpritePosition(SPRITE_PLY2_BLACK_L, x, y);
	VDP_SetSpritePosition(SPRITE_PLY2_WHITE_L, x, y);
	VDP_SetSpritePosition(SPRITE_PLY2_SKIN_L, x, y);

	// Handle special event
	u8 event = g_Actions[g_Player[SIDE_TOP].action].animFrames[g_Player[SIDE_TOP].step].event;
	switch(event)
	{
	case FUNCT_SHOOT_R:
		Player_CheckShoot(&g_Player[SIDE_TOP]);
		VDP_SetSpritePosition(SPRITE_PLY2_RACKET, x+16, y-8);
		break;                                    
	case FUNCT_SHOOT_L:                           
		Player_CheckShoot(&g_Player[SIDE_TOP]);
		VDP_SetSpritePosition(SPRITE_PLY2_RACKET, x-8, y-8);
		break;                                    
	case FUNCT_SMASH:                             
		Player_CheckShoot(&g_Player[SIDE_TOP]);
		VDP_SetSpritePosition(SPRITE_PLY2_RACKET, x+0, y-24);
		break;                                    
	default:                                      
		VDP_SetSpritePositionY(SPRITE_PLY2_RACKET, 213);
		break;
	};
	
	// Handle net
	VDP_SetSpritePositionY(SPRITE_NET_LEFT, 213);
	VDP_SetSpritePositionY(SPRITE_NET_RIGHT, 213);
	VDP_SetSpritePositionY(SPRITE_NET_GRAY, 213);
	if(g_Player[SIDE_TOP].srcPos.y >= 80)
	{
		x /= 8;
		for(u8 i = 0; i < 2; ++i)
		{
			if(g_CourtNet[x] == 0x80)
				x--;
			if(g_CourtNet[x] != 0xFF)
			{
				if(g_CourtNet[x] < 3)
					VDP_SetSprite(SPRITE_NET_LEFT+i, x * 8, 79, 108 + g_CourtNet[x] * 4);
				else
				{
					if(g_NetPostSprite != g_CourtNet[x])
					{
						const u8* src = g_DataNet + g_CourtNet[x] * 8 * 4;
						u16 dst;
						dst	= g_SpritePattern[g_WriteBuffer] + (120 * 8); // Pattern #120 - #124
						VDP_WriteVRAM_64K(src, dst, 8 * 4 * 2);
						dst	= g_SpritePattern[g_DisplayBuffer] + (120 * 8); // Pattern #120 - #124
						VDP_WriteVRAM_64K(src, dst, 8 * 4 * 2);
						g_NetPostSprite = g_CourtNet[x];
					}
					VDP_SetSprite(SPRITE_NET_LEFT+i, x * 8, 79, 120);
					VDP_SetSprite(SPRITE_NET_GRAY, x * 8, 79, 124);
				}
				x += 2;
			}
			else
				x++;
		}
	}
}

//-----------------------------------------------------------------------------
///
void DrawPlayerTop() __FASTCALL
{
	u8 frame = g_Player[SIDE_TOP].anim;
	if(g_Player[SIDE_TOP].prevAnim[g_WriteBuffer] == frame)
		return;

	g_Player[SIDE_TOP].prevAnim[g_WriteBuffer] = frame;

	const u8* src;
	u16 dst;
		
	src = g_DataPlayer2 + (frame * 224);
	dst = g_SpritePattern[g_WriteBuffer] + (41 * 8); // Pattern #41
	VDP_WriteVRAM_64K(src, dst, 8);
	
	src += 8;
	dst += (2 * 8); // Pattern #43
	VDP_WriteVRAM_64K(src, dst, 8);
	
	src += 8;
	dst += (2 * 8); // Pattern #45
	VDP_WriteVRAM_64K(src, dst, 8);
	
	src += 8;
	dst += (2 * 8); // Pattern #47
	VDP_WriteVRAM_64K(src, dst, 8);

	src += 8;
	dst += (2 * 8);  // Pattern #49
	VDP_WriteVRAM_64K(src, dst, 8);

	src += 8;
	dst += (2 * 8);  // Pattern #51
	VDP_WriteVRAM_64K(src, dst, 8);

	src += 8;
	dst += (2 * 8);  // Pattern #53
	VDP_WriteVRAM_64K(src, dst, 8);

	src += 8;
	dst += (2 * 8);  // Pattern #55 - #75
	VDP_WriteVRAM_64K(src, dst, 21 * 8);

	u8 event = g_Actions[g_Player[SIDE_TOP].action].animFrames[g_Player[SIDE_TOP].step].event;
	if(event != FUNCT_NONE)
	{
		switch(event)
		{
		case FUNCT_SHOOT_R:
			src = g_DataRacket + (6 * 8);
			break;
		case FUNCT_SHOOT_L:
			src = g_DataRacket + (14 * 8);
			break;
		case FUNCT_SMASH:
			src = g_DataRacket + (2 * 8);
			break;
		};
		dst += (22 * 8);  // Pattern #77
		VDP_WriteVRAM_64K(src, dst, 8);	
	}
}

//-----------------------------------------------------------------------------
///
void PrepareLauncher()
{
	// Flicker Shadows
	u8 pat = g_TrainSideData[g_TrainSide].pattern;
	if(g_FlickerShadow && (g_Frame & 0x1))
	{
		pat += 8;
	}
	VDP_SetSpritePattern(SPRITE_LAUNCHER_1, pat);     // Pattern #40 / #48
	VDP_SetSpritePattern(SPRITE_LAUNCHER_2, pat + 4); // Pattern #44 / #52
}


//-----------------------------------------------------------------------------
///
void InitializeCourt()
{
	VDP_SetSpritePatternTable(g_SpritePattern[0]);	
	VDP_SetColor(COLOR_DARK_RED);

	// Load court data to VRAM
	VDP_FillScreen_GM2(0);
	VDP_WriteLayout_GM2(g_DataCourt_Names, 3, 3, 27, 18);
	VDP_LoadPattern_GM2(g_DataCourt_Patterns, sizeof(g_DataCourt_Patterns)/8, OFFSET_GAME_COURT);
	VDP_LoadColor_GM2(g_DataCourt_Colors, sizeof(g_DataCourt_Colors)/8, OFFSET_GAME_COURT);

	// Load Scrore Board data to VRAM
	VDP_LoadPattern_GM2(g_DataScore_Patterns, sizeof(g_DataScore_Patterns)/8, OFFSET_GAME_SCORE);
	VDP_LoadColor_GM2(g_DataScore_Colors, sizeof(g_DataScore_Colors)/8, OFFSET_GAME_SCORE);

	// Load Referee data to VRAM
	VDP_LoadPattern_GM2(g_DataReferee_Patterns, sizeof(g_DataReferee_Patterns)/8, OFFSET_GAME_REFEREE);
	VDP_LoadColor_GM2(g_DataReferee_Colors, sizeof(g_DataReferee_Colors)/8, OFFSET_GAME_REFEREE);

	// Initialize font
	Print_SetTextFont(g_DataFont, OFFSET_GAME_FONT);
	Print_SetColor(COLOR_WHITE, 0x9);
	
	Print_SetTextFont(g_DataSrcFont, OFFSET_GAME_SCRFONT);
	Print_SetColor(COLOR_WHITE, COLOR_BLACK);

	// Initialize sprites
	VDP_FillVRAM(0x00, g_SpritePattern[0], 0, 128*8); // Clear sprite patterns table (only 128 entry used)
	VDP_FillVRAM(0x00, g_SpritePattern[1], 0, 128*8);
	
	// Setup ball sprites
	SetSprite(SPRITE_BALL_OUTLINE, 0, 213, 80, COLOR_BLACK);			// Outline
	SetSprite(SPRITE_BALL_BODY,    0, 213, 88, COLOR_LIGHT_YELLOW);		// Body
	SetSprite(SPRITE_BALL_SHADOW,  0, 213, 88, COLOR_BLACK);			// Shadow

	// Setup net sprites
	u16 dst;
	dst	= g_SpritePattern[g_WriteBuffer] + (108 * 8); // Pattern #108 - #116
	VDP_WriteVRAM_64K(g_DataNet, dst, 8 * 4 * 3);
	dst	= g_SpritePattern[g_DisplayBuffer] + (108 * 8); // Pattern #108 - #116
	VDP_WriteVRAM_64K(g_DataNet, dst, 8 * 4 * 3);
	g_NetPostSprite = 0xFF;
	SetSprite(SPRITE_NET_LEFT,  0, 213, 108, COLOR_WHITE); 	// Net part 1
	SetSprite(SPRITE_NET_RIGHT, 0, 213, 108, COLOR_WHITE); 	// Net part 2
	SetSprite(SPRITE_NET_GRAY,  0, 213, 124, COLOR_GRAY);  	// Net gray part

	VDP_HideSpriteFrom(SPRITE_MAX);

	PT3_Pause();
}

//-----------------------------------------------------------------------------
///
void InitializePlayerBottom(u8 controller) __FASTCALL
{
	// Setup player 1 sprites
	SetSprite(SPRITE_PLY1_BLACK_H, 0, 213, 0,  COLOR_BLACK);			// Outline
	SetSprite(SPRITE_PLY1_BLACK_L, 0, 213, 20, COLOR_BLACK);			// Outline
	SetSprite(SPRITE_PLY1_RACKET,  0, 213, 36, COLOR_BLACK);			// Racket
	SetSprite(SPRITE_PLY1_CLOTH,   0, 213, 8,  COLOR_LIGHT_BLUE);		// Cloth
	SetSprite(SPRITE_PLY1_WHITE_H, 0, 213, 12, COLOR_WHITE);			// White
	SetSprite(SPRITE_PLY1_WHITE_L, 0, 213, 28, COLOR_WHITE);			// White
	SetSprite(SPRITE_PLY1_SKIN_H,  0, 213, 16, COLOR_LIGHT_RED);		// Skin
	SetSprite(SPRITE_PLY1_SKIN_L,  0, 213, 32, COLOR_LIGHT_RED);		// Skin

	// Initialize player 1
	Player* ply = &g_Player[SIDE_BOTTOM];
	ply->id = 0;
	ply->pos.x = PX_TO_UNIT(128+32);
	ply->pos.y = PX_TO_UNIT(192-32);
	ply->srcPos.x = UNIT_TO_PX(ply->pos.x);
	ply->srcPos.y = UNIT_TO_PX(ply->pos.y);
	ply->action = ACTION_IDLE;
	ply->counter = 0;
	ply->anim = 0;
	ply->prevAnim[0] = 0xFF;
	ply->prevAnim[1] = 0xFF;
	ply->step = 0;
	ply->binding = g_InputBinding[controller];
	const Binding* bind = &g_Binding[ply->binding];
	ply->inLong  = bind->inUp;
	ply->inShort = bind->inDown;
}

//-----------------------------------------------------------------------------
///
void InitializePlayerTop(u8 controller) __FASTCALL
{
	// Setup player 2 sprites
	SetSprite(SPRITE_PLY2_BLACK_H, 0, 213, 40, COLOR_BLACK);			// Outline
	SetSprite(SPRITE_PLY2_BLACK_L, 0, 213, 56, COLOR_BLACK);			// Outline
	SetSprite(SPRITE_PLY2_RACKET,  0, 213, 76, COLOR_BLACK);			// Racket
	SetSprite(SPRITE_PLY2_CLOTH,   0, 213, 64, COLOR_MEDIUM_GREEN);		// Cloth
	SetSprite(SPRITE_PLY2_WHITE_H, 0, 213, 48, COLOR_WHITE);			// White
	SetSprite(SPRITE_PLY2_WHITE_L, 0, 213, 68, COLOR_WHITE);			// White
	SetSprite(SPRITE_PLY2_SKIN_H,  0, 213, 52, COLOR_LIGHT_RED);		// Skin
	SetSprite(SPRITE_PLY2_SKIN_L,  0, 213, 72, COLOR_LIGHT_RED);		// Skin

	// Initialize player 2
	Player* ply = &g_Player[SIDE_TOP];
	ply->id = 1;
	ply->pos.x = PX_TO_UNIT(128-32);
	ply->pos.y = PX_TO_UNIT(32);
	ply->srcPos.x = UNIT_TO_PX(g_Player[SIDE_TOP].pos.x);
	ply->srcPos.y = UNIT_TO_PX(g_Player[SIDE_TOP].pos.y);
	ply->action = ACTION_IDLE;
	ply->counter = 0;
	ply->anim = 0;
	ply->prevAnim[0] = 0xFF;
	ply->prevAnim[1] = 0xFF;
	ply->step = 0;
	ply->binding = g_InputBinding[controller];
	const Binding* bind = &g_Binding[ply->binding];
	ply->inLong  = bind->inDown;
	ply->inShort = bind->inUp;

}

//-----------------------------------------------------------------------------
///
void InitializeLauncher()
{
	if(g_TrainSide == SIDE_BOTTOM)
	{
		// Load launcher pattern
		u16 dst;
		dst	= g_SpritePattern[g_WriteBuffer] + (40 * 8); // Pattern #40 - #71
		VDP_WriteVRAM_64K(g_DataLauncher0, dst, 8 * 8 *4);
		dst	= g_SpritePattern[g_DisplayBuffer] + (40 * 8); // Pattern #40 - #71
		VDP_WriteVRAM_64K(g_DataLauncher0, dst, 8 * 8 *4);

		// Setup launcher sprites
		SetSprite(SPRITE_LAUNCHER_1, 128-8, (u8)(-4 +  0), 40 +  0, COLOR_BLACK);			// Black 1&2
		SetSprite(SPRITE_LAUNCHER_2, 128-8, (u8)(-4 + 16), 40 +  4, COLOR_BLACK);			// Black 1&2
		SetSprite(SPRITE_LAUNCHER_3, 128-8, (u8)(-4 +  0), 40 + 16, COLOR_LIGHT_YELLOW);	// Yellow
		SetSprite(SPRITE_LAUNCHER_4, 128-8, (u8)(-4 +  8), 40 + 20, COLOR_MEDIUM_GREEN);	// Green
		SetSprite(SPRITE_LAUNCHER_5, 128-8, (u8)(-4 + 16), 40 + 24, COLOR_WHITE);			// White
	}
	else // if(g_TrainSide == SIDE_TOP)
	{
		// Load launcher pattern
		u16 dst;
		dst	= g_SpritePattern[g_WriteBuffer] + (0 * 8); // Pattern #40 - #71
		VDP_WriteVRAM_64K(g_DataLauncher1, dst, 8 * 8 *4);
		dst	= g_SpritePattern[g_DisplayBuffer] + (0 * 8); // Pattern #40 - #71
		VDP_WriteVRAM_64K(g_DataLauncher1, dst, 8 * 8 *4);

		// Setup launcher sprites
		SetSprite(SPRITE_LAUNCHER_1, 128-8, (u8)(150 +  0), 0 +  0, COLOR_BLACK);			// Black 1&2
		SetSprite(SPRITE_LAUNCHER_2, 128-8, (u8)(150 + 16), 0 +  4, COLOR_BLACK);			// Black 1&2
		SetSprite(SPRITE_LAUNCHER_3, 128-8, (u8)(150 -  1), 0 + 16, COLOR_LIGHT_YELLOW);	// Yellow
		SetSprite(SPRITE_LAUNCHER_4, 128-8, (u8)(150 +  3), 0 + 20, COLOR_MEDIUM_GREEN);	// Green
		SetSprite(SPRITE_LAUNCHER_5, 128-8, (u8)(150 + 15), 0 + 24, COLOR_WHITE);			// White
	}	
}

//-----------------------------------------------------------------------------
///
void HideLauncher()
{
	VDP_SetSpritePositionY(SPRITE_LAUNCHER_1, 213);	// Black 1&2
	VDP_SetSpritePositionY(SPRITE_LAUNCHER_2, 213);	// Black 1&2
	VDP_SetSpritePositionY(SPRITE_LAUNCHER_3, 213);	// Yellow
	VDP_SetSpritePositionY(SPRITE_LAUNCHER_4, 213);	// Green
	VDP_SetSpritePositionY(SPRITE_LAUNCHER_5, 213);	// White
}

//-----------------------------------------------------------------------------
///
void SetEvent(u8 event) __FASTCALL
{
	g_EventID = event;
	g_EventTimer = 25;
}


//-----------------------------------------------------------------------------
///
void TrainingScore()
{
	if(g_Ball.point == POINT_FINISHED)
		return;

	if((g_Ball.point == POINT_VALIDATED) && (g_Ball.lastPly == g_TrainSide))
	{
		if(g_TrainScore < 99)
			g_TrainScore++;
		if(g_TrainScore > g_TrainBest)
			g_TrainBest = g_TrainScore;
	}
	else if(((g_Ball.point == POINT_PENDING) && (g_Ball.lastPly == g_TrainSide))
		|| ((g_Ball.point == POINT_VALIDATED) && (g_Ball.lastPly != g_TrainSide)))
	{
		g_TrainScore = 0;
	}
	
	u8 y = (g_TrainSide == SIDE_BOTTOM) ? 0 : 23;
	
	Print_SelectTextFont(g_DataSrcFont, OFFSET_GAME_SCRFONT);
	
	Print_SetPosition(1, y);
	if(g_TrainScore < 10)
		Print_DrawChar('0');
	Print_DrawInt(g_TrainScore);
	
	Print_SetPosition(29, y);
	if(g_TrainBest < 10)
		Print_DrawChar('0');
	Print_DrawInt(g_TrainBest);
	
	g_Ball.point = POINT_FINISHED;
	
	SetEvent(EVENT_POINT);
}

//-----------------------------------------------------------------------------
///
void DisplayScore()
{
	// Points
	Print_SelectTextFont(g_DataSrcFont, OFFSET_GAME_SCRFONT);

	Print_SetPosition(26, 8);
	Print_DrawText(g_ScoreString[g_Points[1]]);
	Print_SetPosition(26, 10);
	Print_DrawText(g_ScoreString[g_Points[0]]);

	// Current set
	Print_SetPosition(23, 8);
	Print_DrawInt(g_Games[g_CurSet][1]);
	Print_SetPosition(23, 10);
	Print_DrawInt(g_Games[g_CurSet][0]);

	// Player name
	Print_SetPosition(20, 8);
	Print_DrawText(g_PlayerString[2]);
	Print_SetPosition(20, 10);
	Print_DrawText(g_PlayerString[0]);
	
	// Current set
	Print_SetPosition(15, 8);
	Print_DrawInt(g_Sets[1]);
	Print_SetPosition(15, 10);
	Print_DrawInt(g_Sets[0]);

	// Previous sets
	if(g_CurSet > 0)
	{
		for(u8 i = 0; i < g_CurSet; i++)
		{
			Print_SetPosition(5 + (i * 2), 8);
			Print_DrawInt(g_Games[i][1]);
			Print_SetPosition(5 + (i * 2), 10);
			Print_DrawInt(g_Games[i][0]);
		}
	}	
}

//-----------------------------------------------------------------------------
///
void ApplyEvent()
{
	g_StateFrame = 0;
	switch(g_EventID)
	{
	case EVENT_POINT: Game_SetState(State_WinPoint); break;
	case EVENT_GAME:  Game_SetState(State_WinGame);  break;
	case EVENT_SET:   Game_SetState(State_WinSet);   break;
	case EVENT_MATCH: Game_SetState(State_WinMatch); break;
	};
	g_EventID = EVENT_NONE;
}

//-----------------------------------------------------------------------------
///
void MatchScore()
{
	if(g_Ball.point == POINT_FINISHED)
		return;

	u8 event = EVENT_POINT;
		
	// Win the point
	if(g_Ball.point == POINT_VALIDATED)
		g_Points[g_Ball.lastPly]++;
	else
		g_Points[1 - g_Ball.lastPly]++;

	// Deuce
	if((g_Points[0] == POINT_ADV) && (g_Points[1] == POINT_ADV))
	{
		g_Points[0] = POINT_40;
		g_Points[1] = POINT_40;
	}
		
	for(u8 i = 0; i < 2; ++i)
	{
		// Win the current game
		if((g_Points[i] == POINT_WIN) || ((g_Points[i] == POINT_ADV) && (g_Points[1-i] < POINT_40)))
		{
			g_Games[g_CurSet][i]++;
			g_Points[0] = 0;
			g_Points[1] = 0;
			event = EVENT_GAME;
		}
		
		// Win the current set
		if(((g_Games[g_CurSet][i] == 6) && (g_Games[g_CurSet][1-i] < 5))
		|| ((g_Games[g_CurSet][i] > 6) && (g_Games[g_CurSet][i] > g_Games[g_CurSet][1-i] + 1)))
		{
			g_Sets[i]++;
			g_CurSet++;
			g_Points[0] = 0;
			g_Points[1] = 0;
			event = EVENT_SET;
		}

		// Win the match!
		if(g_Sets[i] >= g_OptionSets + 1)
		{
			event = EVENT_MATCH;
			// Game_SetState(State_TitleStart); // return to menu for the moment
		}
	}
	
	#if (DEBUG)
		if(g_Debug)
			DisplayScore();
	#endif
	
	SetEvent(event);
	
	g_Ball.point = POINT_FINISHED;
}


void InitService(u8 side)
{
	InitializePlayerBottom(0);
	InitializePlayerTop(1);
	g_TrainSide = side;
	Ball_ShootRandom();
}

//=============================================================================
//
//   I N P U T
//
//=============================================================================

//-----------------------------------------------------------------------------
///
void UpdateInput()
{
	// Keyboard
	for(u8 i = 0; i < 9; ++i)
	{
		g_PrevRow[i] = g_KeyRow[i];	
		g_KeyRow[i] = Keyboard_Read(i);
	}
	// Joystick
	g_PrevJoy1 = g_Joy1;
	g_PrevJoy2 = g_Joy2;
	g_Joy1 = Joystick_Read(JOY_PORT_1);
	g_Joy2 = Joystick_Read(JOY_PORT_2);
}


//-----------------------------------------------------------------------------
// INPUT CALLBACK

bool KB1_Up()		{ return KEY_ON(KEY_UP); }
bool KB1_Down()		{ return KEY_ON(KEY_DOWN); }
bool KB1_Left()		{ return KEY_ON(KEY_LEFT); }
bool KB1_Right()	{ return KEY_ON(KEY_RIGHT); }
bool KB1A_Button1()	{ return KEY_PRESS(KEY_SPACE); }
bool KB1A_Button2()	{ return KEY_PRESS(KEY_N); }
bool KB1B_Button1()	{ return KEY_PRESS(KEY_SHIFT); }
bool KB1B_Button2()	{ return KEY_PRESS(KEY_CTRL); }
bool KB1C_Button1()	{ return KEY_PRESS(KEY_RET); }
bool KB1C_Button2()	{ return KEY_PRESS(KEY_BS); }

bool KB2_Up()		{ return KEY_ON(KEY_E); }
bool KB2_Down()		{ return KEY_ON(KEY_D); }
bool KB2_Left()		{ return KEY_ON(KEY_S); }
bool KB2_Right()	{ return KEY_ON(KEY_F); }

bool Joy1_Up()		{ return (IS_JOY_PRESSED(g_Joy1, JOY_INPUT_DIR_UP)); }
bool Joy1_Down()	{ return (IS_JOY_PRESSED(g_Joy1, JOY_INPUT_DIR_DOWN)); }
bool Joy1_Left()	{ return (IS_JOY_PRESSED(g_Joy1, JOY_INPUT_DIR_LEFT)); }
bool Joy1_Right()	{ return (IS_JOY_PRESSED(g_Joy1, JOY_INPUT_DIR_RIGHT)); }
bool Joy1_Button1()	{ return (IS_JOY_PRESSED(g_Joy1, JOY_INPUT_TRIGGER_A) && !(IS_JOY_PRESSED(g_PrevJoy1, JOY_INPUT_TRIGGER_A))); }	
bool Joy1_Button2()	{ return (IS_JOY_PRESSED(g_Joy1, JOY_INPUT_TRIGGER_B) && !(IS_JOY_PRESSED(g_PrevJoy1, JOY_INPUT_TRIGGER_B))); }

bool Joy2_Up()		{ return (IS_JOY_PRESSED(g_Joy2, JOY_INPUT_DIR_UP)); }
bool Joy2_Down()	{ return (IS_JOY_PRESSED(g_Joy2, JOY_INPUT_DIR_DOWN)); }
bool Joy2_Left()	{ return (IS_JOY_PRESSED(g_Joy2, JOY_INPUT_DIR_LEFT)); }
bool Joy2_Right()	{ return (IS_JOY_PRESSED(g_Joy2, JOY_INPUT_DIR_RIGHT)); }
bool Joy2_Button1()	{ return (IS_JOY_PRESSED(g_Joy2, JOY_INPUT_TRIGGER_A) && !(IS_JOY_PRESSED(g_PrevJoy2, JOY_INPUT_TRIGGER_A))); }	
bool Joy2_Button2()	{ return (IS_JOY_PRESSED(g_Joy2, JOY_INPUT_TRIGGER_B) && !(IS_JOY_PRESSED(g_PrevJoy2, JOY_INPUT_TRIGGER_B))); }

//=============================================================================
//
//   G A M E   S T A T E S
//
//=============================================================================

//-----------------------------------------------------------------------------
///
bool State_Init()
{
	// Initialize VDP
	#if (MSX2_ENHANCE)
	if(g_VersionVDP == VDP_VERSION_TMS9918A)
	{
	#endif
		VDP_SetMode(VDP_MODE_GRAPHIC2);
		VDP_SetLayoutTable(MSX1_LAYOUT_TABLE);
		VDP_SetColorTable(MSX1_COLOR_TABLE);
		VDP_SetPatternTable(MSX1_PATTERN_TABLE);
		VDP_SetSpritePatternTable(MSX1_SPRITE_PATTERN_0);
		VDP_SetSpriteAttributeTable(MSX1_SPRITE_ATTRIBUTE);
		g_SpritePattern[0] = MSX1_SPRITE_PATTERN_0;
		g_SpritePattern[1] = MSX1_SPRITE_PATTERN_1;
	#if (MSX2_ENHANCE)
	}
	else // VDP_VERSION_V99xx
	{
		VDP_SetMode(VDP_MODE_GRAPHIC3);
		VDP_SetLayoutTable(MSX2_LAYOUT_TABLE);
		VDP_SetColorTable(MSX2_COLOR_TABLE);
		VDP_SetPatternTable(MSX2_PATTERN_TABLE);
		VDP_SetSpritePatternTable(MSX2_SPRITE_PATTERN_0);
		VDP_SetSpriteAttributeTable(MSX2_SPRITE_ATTRIBUTE);
		g_SpritePattern[0] = MSX2_SPRITE_PATTERN_0;
		g_SpritePattern[1] = MSX2_SPRITE_PATTERN_1;
	}
	#endif
	VDP_SetSpriteFlag(VDP_SPRITE_SIZE_16 | VDP_SPRITE_SCALE_1);
	VDP_EnableVBlank(true);
	
	g_InputBinding[0] = BIND_KB1A;
	g_InputBinding[1] = BIND_KB2;
	
	// Initialize PT3
	PT3_Init();
	PT3_SetNoteTable(PT3_NT2);
	PT3_SetLoop(true);
	PT3_InitSong(g_DataMusic);

	// Initialize ayFX
	ayFX_InitBank(g_DataSFX);
	ayFX_SetChannel(PSG_CHANNEL_A);
	ayFX_SetMode(AYFX_MODE_FIXED);

	Game_SetState(State_TitleStart);
	return false;
}

//-----------------------------------------------------------------------------
///
bool State_TitleStart()
{
	VDP_EnableDisplay(false);

	VDP_SetSpritePatternTable(g_SpritePattern[0]);
	VDP_SetColor(COLOR_BLACK);

	// Load screen data
	VDP_FillScreen_GM2(0); // Don't set the Layout table yet
	VDP_LoadPattern_GM2(g_DataLogo_Patterns, sizeof(g_DataLogo_Patterns)/8, OFFSET_TITLE_LOGO);
	VDP_LoadColor_GM2(g_DataLogo_Colors, sizeof(g_DataLogo_Colors)/8, OFFSET_TITLE_LOGO);

	// Load sprites data
	VDP_WriteVRAM_64K(g_DataLogoBall, g_SpritePatternLow, 8 * 32);

	// Initialize sprite attributes
	SetSprite(0, 0, 0, 0,  VDP_SPRITE_EC | COLOR_LIGHT_YELLOW);
	SetSprite(1, 0, 0, 4,  VDP_SPRITE_EC | COLOR_DARK_RED);
	SetSprite(2, 0, 0, 8,  VDP_SPRITE_EC | COLOR_LIGHT_YELLOW);
	SetSprite(3, 0, 0, 12, VDP_SPRITE_EC | COLOR_DARK_RED);
	SetSprite(4, 0, 0, 16, VDP_SPRITE_EC | COLOR_LIGHT_YELLOW);
	SetSprite(5, 0, 0, 20, VDP_SPRITE_EC | COLOR_DARK_RED);
	SetSprite(6, 0, 0, 24, VDP_SPRITE_EC | COLOR_LIGHT_YELLOW);
	SetSprite(7, 0, 0, 28, VDP_SPRITE_EC | COLOR_DARK_RED);
	VDP_HideSpriteFrom(8);

	// Initialize font
	Print_SetTextFont(g_DataFont, OFFSET_TITLE_FONT_DEF);
	Print_SetColorShade(g_ColorShadeDefault);
	u16 dst = (u16)g_ScreenPatternLow + ((OFFSET_TITLE_FONT_DEF - '!' + '_') * 8); // clear '_' character
	VDP_FillVRAM_64K(0, dst + 0 * 256 * 8, 8);
	VDP_FillVRAM_64K(0, dst + 1 * 256 * 8, 8);
	VDP_FillVRAM_64K(0, dst + 2 * 256 * 8, 8);

	Print_SetTextFont(g_DataFont, OFFSET_TITLE_FONT_ALT);
	Print_SetColorShade(g_ColorShadeSelect);
	
	g_StateFrame = 0;

	VDP_EnableDisplay(true);
	Game_SetState(State_TitleUpdate);
	return false;
}

//-----------------------------------------------------------------------------
///
bool State_TitleUpdate()
{
	UpdateInput();

	if(g_StateFrame < 25) // Ball movement
	{
		VDP_SetSpritePosition(0, g_StateFrame * 8 +  0 + 4, 32 +  0);
		VDP_SetSpritePosition(1, g_StateFrame * 8 +  0 + 4, 32 +  0);
		VDP_SetSpritePosition(2, g_StateFrame * 8 + 16 + 4, 32 +  0);
		VDP_SetSpritePosition(3, g_StateFrame * 8 + 16 + 4, 32 +  0);
		VDP_SetSpritePosition(4, g_StateFrame * 8 +  0 + 4, 32 + 16);
		VDP_SetSpritePosition(5, g_StateFrame * 8 +  0 + 4, 32 + 16);
		VDP_SetSpritePosition(6, g_StateFrame * 8 + 16 + 4, 32 + 16);
		VDP_SetSpritePosition(7, g_StateFrame * 8 + 16 + 4, 32 + 16);
	}
	else if(g_StateFrame == 25) // White flash
	{	
		if(g_PlaySFX)
			ayFX_PlayBank(13, 0);
		VDP_SetColor(COLOR_WHITE);
		VDP_FillVRAM_64K(0xFF, g_ScreenColorLow + (0 * 0x800),  8);
		VDP_FillVRAM_64K(0xFF, g_ScreenColorLow + (1 * 0x800),  8);
		VDP_FillVRAM_64K(0xFF, g_ScreenColorLow + (2 * 0x800),  8);
	}
	else if(g_StateFrame == 29) // Title
	{	
		VDP_SetColor(COLOR_BLACK);
		VDP_WriteVRAM_64K(g_DataLogo_Colors, g_ScreenColorLow + (0 * 0x800),  8);
		VDP_WriteVRAM_64K(g_DataLogo_Colors, g_ScreenColorLow + (1 * 0x800),  8);
		VDP_WriteVRAM_64K(g_DataLogo_Colors, g_ScreenColorLow + (2 * 0x800),  8);
		VDP_WriteLayout_GM2(g_DataLogo_Names, 4, 2, 19, 10);
	}
	else if(g_StateFrame == 64) // Title
	{
		if(g_PlayMusic)
			PT3_Resume();

		Print_SelectTextFont(g_DataFont, OFFSET_TITLE_FONT_DEF);
		Print_SetPosition(11, 18);
		Print_DrawText("PRESS A KEY");
	}
	if(g_StateFrame < 255)
		g_StateFrame++;
	
	if(Keyboard_IsKeyPressed(KEY_SPACE)) // Skip
	{
		if(g_PlayMusic)
			PT3_Resume();

		VDP_SetColor(COLOR_BLACK);
		VDP_WriteVRAM_64K(g_DataLogo_Colors, g_ScreenColorLow + (0 * 0x800),  8);
		VDP_WriteVRAM_64K(g_DataLogo_Colors, g_ScreenColorLow + (1 * 0x800),  8);
		VDP_WriteVRAM_64K(g_DataLogo_Colors, g_ScreenColorLow + (2 * 0x800),  8);
		VDP_WriteLayout_GM2(g_DataLogo_Names, 4, 2, 19, 10);

		g_StateFrame = 25;
		VDP_SetSpritePosition(0, g_StateFrame * 8 +  0 - 4, 32 +  0);
		VDP_SetSpritePosition(1, g_StateFrame * 8 +  0 - 4, 32 +  0);
		VDP_SetSpritePosition(2, g_StateFrame * 8 + 16 - 4, 32 +  0);
		VDP_SetSpritePosition(3, g_StateFrame * 8 + 16 - 4, 32 +  0);
		VDP_SetSpritePosition(4, g_StateFrame * 8 +  0 - 4, 32 + 16);
		VDP_SetSpritePosition(5, g_StateFrame * 8 +  0 - 4, 32 + 16);
		VDP_SetSpritePosition(6, g_StateFrame * 8 + 16 - 4, 32 + 16);
		VDP_SetSpritePosition(7, g_StateFrame * 8 + 16 - 4, 32 + 16);

		Game_SetState(State_MenuStart);
	}
	return true;
}

//-----------------------------------------------------------------------------
///
bool State_MenuStart()
{
	Menu_Initialize(&g_Menus[g_MenuID]);
	Game_SetState(State_MenuUpdate);
	return false;
}

//-----------------------------------------------------------------------------
///
bool State_MenuUpdate()
{
	UpdateInput();
	Menu_Update();
	return true;
}

//-----------------------------------------------------------------------------
///
bool State_ScoreStart()
{
	// Display Score board
	// VDP_FillScreen_GM2(0);
	VDP_WriteLayout_GM2(g_DataScore_Names, 3, 3, 27, 10);
	VDP_HideSpriteFrom(0);
	
	DisplayScore();
	
	Game_SetState(State_ScoreUpdate);
	return false;
}

//-----------------------------------------------------------------------------
///
bool State_ScoreUpdate()
{
	UpdateInput();

	if(KEY_PRESS(KEY_F1))
	{
		Game_SetState(State_CourtRestore);
		return false;
	}
	if(KEY_PRESS(KEY_ESC)) // Return to main menu
	{
		Game_SetState(State_TitleStart);
		return false;
	}
	
	return true;
}

//-----------------------------------------------------------------------------
///
bool State_CourtRestore()
{
	// Restore Court
	VDP_FillScreen_GM2(0);
	VDP_WriteLayout_GM2(g_DataCourt_Names, 3, 3, 27, 18);
	
	Game_SetState(State_MatchUpdate);
	return false;
}

//-----------------------------------------------------------------------------
///
bool State_MatchStart()
{
	VDP_EnableDisplay(false);

	InitializeCourt();
	InitializePlayerBottom(0);
	InitializePlayerTop(1);
	HideLauncher();
	
	// Initialize score
	g_ScoreFct = MatchScore;
	g_NumSet = (g_OptionSets * 2) + 1;
	g_CurSet = 0;
	g_Points[0] = 0;
	g_Points[1] = 0;
	g_Sets[0] = 0;
	g_Sets[1] = 0;
	for(u8 i = 0; i < g_NumSet; ++i)
	{
		g_Games[i][0] = 0;
		g_Games[i][1] = 0;
	}

	// Initialize timer
	g_EventID = EVENT_NONE;
	g_EventTimer = 0;

	// Initialize ball
	InitService(0);
	// Ball_ShootRandom();

	VDP_EnableDisplay(true);
	Game_SetState(State_MatchUpdate);
	return false;
}

//-----------------------------------------------------------------------------
///
bool State_MatchUpdate()
{
// VDP_SetColor(COLOR_BLACK);

	//---------------------------------------------------------------------
	// Switch buffer
	g_WriteBuffer = 1 - g_WriteBuffer;
	g_DisplayBuffer = 1 - g_DisplayBuffer;
	VDP_SetSpritePatternTable(g_SpritePattern[g_DisplayBuffer]);
	PreparePlayerBottom();
	PreparePlayerTop();
	Ball_Prepare();

// VDP_SetColor(COLOR_LIGHT_BLUE);

	//---------------------------------------------------------------------
	// Update input

	UpdateInput();

	#if (DEBUG)
		if(KEY_PRESS(KEY_HOME))
			g_Debug = 1 - g_Debug;
	#endif
	
	if(KEY_PRESS(KEY_F1))
	{
		Game_SetState(State_ScoreStart);
		return false;
	}

	if(KEY_PRESS(KEY_F2))
		Ball_ShootRandom();
	if(KEY_PRESS(KEY_F3)) // Activate/deactivate shadows
		g_FlickerShadow = 1 - g_FlickerShadow;
	if(KEY_PRESS(KEY_ESC)) // Return to main menu
	{
		// VDP_HideSpriteFrom(0);
		Game_SetState(State_TitleStart);
		return false;
	}
	
	HandleInput(&g_Player[SIDE_BOTTOM]);
	HandleInput(&g_Player[SIDE_TOP]);

// VDP_SetColor(COLOR_LIGHT_GREEN);
	
	//---------------------------------------------------------------------
	// Update sprites position
	
	UpdatePlayerBottom();
	UpdatePlayerTop();
	Ball_Update();

	UpdateAction(&g_Player[SIDE_BOTTOM]);
	UpdateAction(&g_Player[SIDE_TOP]);

// VDP_SetColor(COLOR_LIGHT_YELLOW);

	//---------------------------------------------------------------------
	// Draw anim
	
	DrawPlayerBottom();
	DrawPlayerTop();
	Ball_Draw();

	// Handle match event
	if(g_EventID != EVENT_NONE)
	{
		if(g_EventTimer == 0)
			ApplyEvent();
		g_EventTimer--;		
	}


// VDP_SetColor(COLOR_DARK_RED);
	return true;
}

//-----------------------------------------------------------------------------
///
bool State_WinPoint()
{
	if(g_StateFrame == 25)
	{
		InitService(0);
		Game_SetState(State_MatchUpdate);
	}
	g_StateFrame++;
	return true;
}

//-----------------------------------------------------------------------------
///
bool State_WinGame()
{
	if(g_StateFrame == 0)
	{
		InitService(0);
		Game_SetState(State_MatchUpdate);
	}
	g_StateFrame++;
	return true;
}

//-----------------------------------------------------------------------------
///
bool State_WinSet()
{
	if(g_StateFrame == 0)
	{
		InitService(0);
		Game_SetState(State_MatchUpdate);
	}
	g_StateFrame++;
	return true;
}

//-----------------------------------------------------------------------------
///
bool State_WinMatch()
{
	if(g_StateFrame == 0)
	{
		InitService(0);
		Game_SetState(State_MatchUpdate);
	}
	g_StateFrame++;
	return true;
}

//-----------------------------------------------------------------------------
///
bool State_TrainingStart()
{
	VDP_EnableDisplay(false);

	InitializeCourt();
	InitializePlayerBottom(g_TrainSide);
	InitializePlayerTop(1-g_TrainSide);
	InitializeLauncher();
	
	// Initialize score
	if(g_TrainSide == SIDE_BOTTOM)
	{
		VDP_WriteLayout_GM2(g_DataScoreL2_Names, 0, 0, 9, 3);
		VDP_WriteLayout_GM2(g_DataScoreL3_Names, 32-9, 0, 9, 3);
	}
	else // if(g_TrainSide == SIDE_TOP)
	{
		VDP_WriteLayout_GM2(g_DataScoreL4_Names, 0, 22, 9, 2);
		VDP_WriteLayout_GM2(g_DataScoreL5_Names, 32-9, 22, 9, 2);
	}
	g_TrainScore = 0;
	g_ScoreFct = TrainingScore;
	TrainingScore(); // Initialize scoring and launch ball

	SetEvent(EVENT_POINT);

	VDP_EnableDisplay(true);
	Game_SetState(State_TrainingUpdate);
	return false;
}

//-----------------------------------------------------------------------------
///
bool State_TrainingUpdate()
{
	//---------------------------------------------------------------------
	// Switch buffer

	g_WriteBuffer = 1 - g_WriteBuffer;
	g_DisplayBuffer = 1 - g_DisplayBuffer;
	VDP_SetSpritePatternTable(g_SpritePattern[g_DisplayBuffer]);

	// Prepare sprite objects
	if(g_TrainSide == SIDE_BOTTOM)
		PreparePlayerBottom();
	else
		PreparePlayerTop();
	PrepareLauncher();
	Ball_Prepare();

	//---------------------------------------------------------------------
	// Update input

	UpdateInput();

	if(KEY_PRESS(KEY_F2))
		Ball_ShootRandom();
	if(KEY_PRESS(KEY_F3)) // Activate/deactivate shadows
		g_FlickerShadow = 1 - g_FlickerShadow;
	if(KEY_PRESS(KEY_ESC)) // Return to main menu
	{
		VDP_HideSpriteFrom(0);
		Game_SetState(State_TitleStart);
	}

	#if (DEBUG)

		if(KEY_PRESS(KEY_HOME))
		{
			g_Debug = 1 - g_Debug;
			if(g_Debug)
			{
				u8 y = 4;
				Print_SelectTextFont(g_DataFont, OFFSET_GAME_FONT);
				Print_SetPosition(1, y++);
				Print_DrawText("ACT:");
				Print_SetPosition(1, y++);
				Print_DrawText("STP:");
				Print_SetPosition(1, y++);
				Print_DrawText("ANI:");
				y++;
				Print_SetPosition(1, y++);
				Print_DrawText("SHT:");
				Print_SetPosition(1, y++);
				Print_DrawText("DIR:");
				Print_SetPosition(1, y++);
				Print_DrawText("CNT:");
				y++;
				Print_SetPosition(1, y++);
				Print_DrawText("LST:");
				Print_SetPosition(1, y++);
				Print_DrawText("STA:");

				#if (TARGET_TYPE == TARGET_TYPE_ROM)
					Print_SetPosition(1, 20);
					Print_DrawText("ROM:");
					Print_DrawHex8(g_VersionROM);
					Print_SetPosition(1, 21);
					Print_DrawText("MSX:");
					Print_DrawHex8(g_VersionMSX);
				#endif
				
				Print_SetPosition(1, 22);
				#if (MSX2_ENHANCE)
					Print_DrawText((g_VersionVDP == VDP_VERSION_TMS9918A) ? "MSX1" : "MSX2");
				#else
					Print_DrawText("MSX1");
				#endif
			}
		}

		if(g_Debug)
		{
			u8 y = 4;
			Print_SetPosition(5, y++);
			Print_DrawHex8(g_Player[0].action);
			Print_SetPosition(5, y++);
			Print_DrawHex8(g_Player[0].step);
			Print_SetPosition(5, y++);
			Print_DrawHex8(g_Player[0].anim);
			y++;
			Print_SetPosition(5, y++);
			Print_DrawHex8(g_Player[0].shot);
			Print_SetPosition(5, y++);
			Print_DrawHex8(g_Player[0].shotDir);
			Print_SetPosition(5, y++);
			Print_DrawHex8(g_Player[0].shotCnt);
			y++;
			Print_SetPosition(5, y++);
			Print_DrawHex8(g_Ball.lastPly);
			Print_SetPosition(5, y++);
			Print_DrawHex8(g_Ball.point);
		}
	#endif
	
	HandleInput(&g_Player[g_TrainSide]);
	
	//---------------------------------------------------------------------
	// Update sprites position
	
	if(g_TrainSide == SIDE_BOTTOM)
		UpdatePlayerBottom();
	else
		UpdatePlayerTop();
		
	Ball_Update();

	UpdateAction(&g_Player[g_TrainSide]);

	//---------------------------------------------------------------------
	// Draw anim
	
	if(g_TrainSide == SIDE_BOTTOM)
		DrawPlayerBottom();
	else
		DrawPlayerTop();
	Ball_Draw();
	
	if(g_EventID != EVENT_NONE)
	{
		if(g_EventTimer == 0)
		{
			Ball_ShootRandom();
			g_EventID = EVENT_NONE;
		}
		g_EventTimer--;
	}
	
	return true;
}

//=============================================================================
//
//   M A I N
//
//=============================================================================

#if (TARGET_TYPE == TARGET_TYPE_ROM)
//-----------------------------------------------------------------------------
/// ISR for 48K ROM
void VDP_InterruptHandler()
{
	Game_VSyncHook();
}
#endif

//-----------------------------------------------------------------------------
/// Main loop
void VSyncCallback()
{
	PT3_Decode();
	ayFX_Update();
	PT3_UpdatePSG();
}

//-----------------------------------------------------------------------------
/// Main loop
void main()
{
	#if (MSX2_ENHANCE)
	g_KeyRow[0] = Keyboard_Read(0); // 0 1 2 3 4 5 6 7
	if(IS_KEY_PRESSED(g_KeyRow[0], KEY_1))
		g_VersionVDP = VDP_VERSION_TMS9918A;
	else
		g_VersionVDP = VDP_GetVersion();
	#endif

	Game_Initialize();
	Game_SetVSyncCallback(VSyncCallback);
	Game_SetState(State_Init);

	while(1)
		Game_Update();
}
