
/*
 * From: x11r6/xc/programs/Xserver/hw/sun/sunKeyMap.c
 */


SunModmapRec *sunModMaps[] = {
    NULL,
    NULL,
    US2Modmap,
    US3Modmap,
    NULL
};

static SunModmapRec Generic5Modmap[] = {
	99,	ShiftMask,
	110,	ShiftMask,
	119,	LockMask,
	76,	ControlMask,
	120,	Mod1Mask,
	122,	Mod1Mask,
	13,	Mod2Mask,
	19,	Mod3Mask,
	98,	Mod4Mask,
	0,	0
};

#ifdef US4

static KeySym US4Keymap[] = {
	SunXK_Stop,  	NoSymbol,  	XK_L1,		NoSymbol,	/*  1*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/*  2*/
	SunXK_Again,  	NoSymbol,  	XK_L2,		NoSymbol,	/*  3*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/*  4*/
	XK_F1,  	NoSymbol,	NoSymbol,	NoSymbol,	/*  5*/
	XK_F2,  	NoSymbol,	NoSymbol,	NoSymbol,	/*  6*/
	XK_F10, 	NoSymbol,	NoSymbol,	NoSymbol,	/*  7*/
	XK_F3,  	NoSymbol,	NoSymbol,	NoSymbol,	/*  8*/
	XK_F11,		NoSymbol,	SunXK_F36,	NoSymbol,	/*  9*/
	XK_F4,  	NoSymbol,	NoSymbol,	NoSymbol,	/* 10*/
	XK_F12,		NoSymbol,	SunXK_F37,	NoSymbol,	/* 11*/
	XK_F5,  	NoSymbol,	NoSymbol,	NoSymbol,	/* 12*/
	SunXK_AltGraph,	NoSymbol,	NoSymbol,	NoSymbol,	/* 13*/
	XK_F6,  	NoSymbol,	NoSymbol,	NoSymbol,	/* 14*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/* 15*/
	XK_F7,  	NoSymbol,	NoSymbol,	NoSymbol,	/* 16*/
	XK_F8,  	NoSymbol,	NoSymbol,	NoSymbol,	/* 17*/
	XK_F9,  	NoSymbol,	NoSymbol,	NoSymbol,	/* 18*/
	XK_Alt_L,	NoSymbol,	NoSymbol,	NoSymbol,	/* 19*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/* 20*/
	XK_Pause,  	NoSymbol,  	XK_R1,		NoSymbol,	/* 21*/
	XK_Print,  	NoSymbol,  	XK_R2,		NoSymbol,	/* 22*/
	XK_Break,  	XK_Scroll_Lock,	XK_R3,		NoSymbol,	/* 23*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/* 24*/
	SunXK_Props,  	NoSymbol,  	XK_L3,		NoSymbol,	/* 25*/
	SunXK_Undo,  	NoSymbol,  	XK_L4,		NoSymbol,	/* 26*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/* 27*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/* 28*/
	XK_Escape,	NoSymbol,	NoSymbol,	NoSymbol,	/* 29*/
	XK_1,   	XK_exclam,	NoSymbol,	NoSymbol,	/* 30*/
	XK_2,   	XK_at,  	NoSymbol,	NoSymbol,	/* 31*/
	XK_3,   	XK_numbersign,	NoSymbol,	NoSymbol,	/* 32*/
	XK_4,   	XK_dollar,	NoSymbol,	NoSymbol,	/* 33*/
	XK_5,   	XK_percent,	NoSymbol,	NoSymbol,	/* 34*/
	XK_6,   	XK_asciicircum,	NoSymbol,	NoSymbol,	/* 35*/
	XK_7,   	XK_ampersand,	NoSymbol,	NoSymbol,	/* 36*/
	XK_8,   	XK_asterisk,	NoSymbol,	NoSymbol,	/* 37*/
	XK_9,   	XK_parenleft,	NoSymbol,	NoSymbol,	/* 38*/
	XK_0,   	XK_parenright,	NoSymbol,	NoSymbol,	/* 39*/
	XK_minus,	XK_underscore,	NoSymbol,	NoSymbol,	/* 40*/
	XK_equal,	XK_plus,	NoSymbol,	NoSymbol,	/* 41*/
	XK_grave,	XK_asciitilde,	XK_acute,	NoSymbol,	/* 42*/
	XK_BackSpace,	NoSymbol,	NoSymbol,	NoSymbol,	/* 43*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/* 44*/
	XK_KP_Equal,  	NoSymbol,  	XK_R4,		NoSymbol,	/* 45*/
	XK_KP_Divide,  	NoSymbol,  	XK_R5,		NoSymbol,	/* 46*/
	XK_KP_Multiply,	NoSymbol,	XK_R6,		NoSymbol,	/* 47*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/* 48*/
	SunXK_Front,  	NoSymbol,  	XK_L5,		NoSymbol,	/* 49*/
	XK_KP_Delete,	XK_KP_Decimal,	NoSymbol,	NoSymbol,	/* 50*/
	SunXK_Copy,  	NoSymbol,  	XK_L6,		NoSymbol,	/* 51*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/* 52*/
	XK_Tab, 	NoSymbol,	NoSymbol,	NoSymbol,	/* 53*/
	XK_Q,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 54*/
	XK_W,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 55*/
	XK_E,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 56*/
	XK_R,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 57*/
	XK_T,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 58*/
	XK_Y,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 59*/
	XK_U,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 60*/
	XK_I,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 61*/
	XK_O,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 62*/
	XK_P,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 63*/
	XK_bracketleft,	XK_braceleft,	NoSymbol,	NoSymbol,	/* 64*/
	XK_bracketright,XK_braceright,	NoSymbol,	NoSymbol,	/* 65*/
	XK_Delete,	NoSymbol,	NoSymbol,	NoSymbol,	/* 66*/
	SunXK_Compose,	NoSymbol,	NoSymbol,	NoSymbol,	/* 67*/
	XK_KP_Home,  	XK_KP_7,  	XK_R7,		NoSymbol,	/* 68*/
	XK_KP_Up,  	XK_KP_8,  	XK_R8,		NoSymbol,	/* 69*/
	XK_KP_Prior,  	XK_KP_9,  	XK_R9,		NoSymbol,	/* 70*/
	XK_KP_Subtract,	NoSymbol,	NoSymbol,	NoSymbol,	/* 71*/
	SunXK_Open,  	NoSymbol,  	XK_L7,		NoSymbol,	/* 72*/
	SunXK_Paste,  	NoSymbol,  	XK_L8,		NoSymbol,	/* 73*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/* 74*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/* 75*/
	XK_Control_L,	NoSymbol,	NoSymbol,	NoSymbol,	/* 76*/
	XK_A,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 77*/
	XK_S,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 78*/
	XK_D,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 79*/
	XK_F,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 80*/
	XK_G,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 81*/
	XK_H,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 82*/
	XK_J,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 83*/
	XK_K,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 84*/
	XK_L,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 85*/
	XK_semicolon,	XK_colon,	NoSymbol,	NoSymbol,	/* 86*/
	XK_apostrophe,	XK_quotedbl,	XK_acute,	NoSymbol,	/* 87*/
	XK_backslash,	XK_bar, 	NoSymbol,	NoSymbol,	/* 88*/
	XK_Return,	NoSymbol,	NoSymbol,	NoSymbol,	/* 89*/
	XK_KP_Enter,	NoSymbol,	NoSymbol,	NoSymbol,	/* 90*/
	XK_KP_Left,	XK_KP_4, 	XK_R10,		NoSymbol,	/* 91*/
	NoSymbol, 	XK_KP_5, 	XK_R11,		NoSymbol,	/* 92*/
	XK_KP_Right,	XK_KP_6, 	XK_R12,		NoSymbol,	/* 93*/
	XK_KP_Insert,	XK_KP_0,	NoSymbol,	NoSymbol,	/* 94*/
	SunXK_Find,  	NoSymbol,  	XK_L9,		NoSymbol,	/* 95*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/* 96*/
	SunXK_Cut, 	NoSymbol, 	XK_L10,		NoSymbol,	/* 97*/
	XK_Num_Lock,	NoSymbol,	NoSymbol,	NoSymbol,	/* 98*/
	XK_Shift_L,	NoSymbol,	NoSymbol,	NoSymbol,	/* 99*/
	XK_Z,   	NoSymbol,	NoSymbol,	NoSymbol,	/*100*/
	XK_X,   	NoSymbol,	NoSymbol,	NoSymbol,	/*101*/
	XK_C,   	NoSymbol,	NoSymbol,	NoSymbol,	/*102*/
	XK_V,   	NoSymbol,	NoSymbol,	NoSymbol,	/*103*/
	XK_B,   	NoSymbol,	NoSymbol,	NoSymbol,	/*104*/
	XK_N,   	NoSymbol,	NoSymbol,	NoSymbol,	/*105*/
	XK_M,   	NoSymbol,	NoSymbol,	NoSymbol,	/*106*/
	XK_comma,	XK_less,	NoSymbol,	NoSymbol,	/*107*/
	XK_period,	XK_greater,	NoSymbol,	NoSymbol,	/*108*/
	XK_slash,	XK_question,	NoSymbol,	NoSymbol,	/*109*/
	XK_Shift_R,	NoSymbol,	NoSymbol,	NoSymbol,	/*110*/
	XK_Linefeed,	NoSymbol,	NoSymbol,	NoSymbol,	/*111*/
	XK_KP_End, 	XK_KP_1, 	XK_R13,		NoSymbol, 	/*112*/
	XK_KP_Down,	XK_KP_2, 	XK_R14,		NoSymbol,	/*113*/
	XK_KP_Next, 	XK_KP_3, 	XK_R15,		NoSymbol,	/*114*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/*115*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/*116*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/*117*/
	XK_Help,	XK_Help,	NoSymbol,	NoSymbol,	/*118*/
	XK_Caps_Lock,	NoSymbol,	NoSymbol,	NoSymbol,	/*119*/
	XK_Meta_L,	NoSymbol,	NoSymbol,	NoSymbol,	/*120*/
	XK_space,	NoSymbol,	NoSymbol,	NoSymbol,	/*121*/
	XK_Meta_R,	NoSymbol,	NoSymbol,	NoSymbol,	/*122*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/*123*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/*124*/
	XK_KP_Add,	NoSymbol,	NoSymbol,	NoSymbol,	/*125*/
};

#define US4Modmap JapanTaiUKUS4Modmap

#else

#define US4Keymap NULL
#define US4Modmap NULL

#endif /* US4 */


#ifdef US5

static KeySym US5Keymap[] = {
	SunXK_Stop,  	NoSymbol,  	XK_L1,		NoSymbol,	/*  1*/
	SunXK_AudioLowerVolume,SunXK_VideoLowerBrightness,NoSymbol,	NoSymbol,	/*  2*/
	SunXK_Again,  	NoSymbol,  	XK_L2,		NoSymbol,	/*  3*/
	SunXK_AudioRaiseVolume,SunXK_VideoRaiseBrightness,NoSymbol,	NoSymbol,	/*  4*/
	XK_F1,  	NoSymbol,	NoSymbol,	NoSymbol,	/*  5*/
	XK_F2,  	NoSymbol,	NoSymbol,	NoSymbol,	/*  6*/
	XK_F10, 	NoSymbol,	NoSymbol,	NoSymbol,	/*  7*/
	XK_F3,  	NoSymbol,	NoSymbol,	NoSymbol,	/*  8*/
	XK_F11,		NoSymbol,	SunXK_F36,	NoSymbol,	/*  9*/
	XK_F4,  	NoSymbol,	NoSymbol,	NoSymbol,	/* 10*/
	XK_F12,		NoSymbol,	SunXK_F37,	NoSymbol,	/* 11*/
	XK_F5,  	NoSymbol,	NoSymbol,	NoSymbol,	/* 12*/
	SunXK_AltGraph,	NoSymbol,	NoSymbol,	NoSymbol,	/* 13*/
	XK_F6,  	NoSymbol,	NoSymbol,	NoSymbol,	/* 14*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/* 15*/
	XK_F7,  	NoSymbol,	NoSymbol,	NoSymbol,	/* 16*/
	XK_F8,  	NoSymbol,	NoSymbol,	NoSymbol,	/* 17*/
	XK_F9,  	NoSymbol,	NoSymbol,	NoSymbol,	/* 18*/
	XK_Alt_L,	NoSymbol,	NoSymbol,	NoSymbol,	/* 19*/
	XK_Up,  	NoSymbol,	NoSymbol,	NoSymbol,	/* 20*/
	XK_Pause,  	NoSymbol,  	XK_Break,	NoSymbol,	/* 21*/
	XK_Print,  	NoSymbol,  	XK_Sys_Req,	SunXK_Sys_Req,	/* 22*/
	XK_Scroll_Lock,	NoSymbol,  	NoSymbol,	NoSymbol,	/* 23*/
	XK_Left,	NoSymbol,	NoSymbol,	NoSymbol,	/* 24*/
	SunXK_Props,  	NoSymbol,  	XK_L3,		NoSymbol,	/* 25*/
	SunXK_Undo,  	NoSymbol,  	XK_L4,		NoSymbol,	/* 26*/
	XK_Down,	NoSymbol,	NoSymbol,	NoSymbol,	/* 27*/
	XK_Right,	NoSymbol,	NoSymbol,	NoSymbol,	/* 28*/
	XK_Escape,	NoSymbol,	NoSymbol,	NoSymbol,	/* 29*/
	XK_1,   	XK_exclam,	NoSymbol,	NoSymbol,	/* 30*/
	XK_2,   	XK_at,  	NoSymbol,	NoSymbol,	/* 31*/
	XK_3,   	XK_numbersign,	NoSymbol,	NoSymbol,	/* 32*/
	XK_4,   	XK_dollar,	NoSymbol,	NoSymbol,	/* 33*/
	XK_5,   	XK_percent,	NoSymbol,	NoSymbol,	/* 34*/
	XK_6,   	XK_asciicircum,	NoSymbol,	NoSymbol,	/* 35*/
	XK_7,   	XK_ampersand,	NoSymbol,	NoSymbol,	/* 36*/
	XK_8,   	XK_asterisk,	NoSymbol,	NoSymbol,	/* 37*/
	XK_9,   	XK_parenleft,	NoSymbol,	NoSymbol,	/* 38*/
	XK_0,   	XK_parenright,	NoSymbol,	NoSymbol,	/* 39*/
	XK_minus,	XK_underscore,	NoSymbol,	NoSymbol,	/* 40*/
	XK_equal,	XK_plus,	NoSymbol,	NoSymbol,	/* 41*/
	XK_grave,	XK_asciitilde,	XK_acute,	NoSymbol,	/* 42*/
	XK_BackSpace,	NoSymbol,	NoSymbol,	NoSymbol,	/* 43*/
	XK_Insert,	NoSymbol,	NoSymbol,	NoSymbol,	/* 44*/
	SunXK_AudioMute,SunXK_VideoDegauss,NoSymbol,	NoSymbol,	/* 45*/
	XK_KP_Divide,  	NoSymbol,  	NoSymbol,	NoSymbol,	/* 46*/
	XK_KP_Multiply,	NoSymbol,  	NoSymbol,	NoSymbol,	/* 47*/
	SunXK_PowerSwitch,SunXK_PowerSwitchShift,	NoSymbol,	NoSymbol,	/* 48*/
	SunXK_Front,  	NoSymbol,  	XK_L5,		NoSymbol,	/* 49*/
	XK_KP_Delete,	XK_KP_Decimal,	NoSymbol,	NoSymbol,	/* 50*/
	SunXK_Copy,  	NoSymbol,  	XK_L6,		NoSymbol,	/* 51*/
	XK_Home,	NoSymbol,	NoSymbol,	NoSymbol,	/* 52*/
	XK_Tab, 	NoSymbol,	NoSymbol,	NoSymbol,	/* 53*/
	XK_Q,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 54*/
	XK_W,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 55*/
	XK_E,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 56*/
	XK_R,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 57*/
	XK_T,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 58*/
	XK_Y,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 59*/
	XK_U,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 60*/
	XK_I,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 61*/
	XK_O,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 62*/
	XK_P,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 63*/
	XK_bracketleft,	XK_braceleft,	NoSymbol,	NoSymbol,	/* 64*/
	XK_bracketright,XK_braceright,	NoSymbol,	NoSymbol,	/* 65*/
	XK_Delete,	NoSymbol,	NoSymbol,	NoSymbol,	/* 66*/
	SunXK_Compose,	NoSymbol,	NoSymbol,	NoSymbol,	/* 67*/
	XK_KP_Home,	XK_KP_7,  	NoSymbol,	NoSymbol,	/* 68*/
	XK_KP_Up,  	XK_KP_8,  	NoSymbol,	NoSymbol,	/* 69*/
	XK_KP_Prior,  	XK_KP_9,  	NoSymbol,	NoSymbol,	/* 70*/
	XK_KP_Subtract,	NoSymbol,  	NoSymbol,	NoSymbol,	/* 71*/
	SunXK_Open,  	NoSymbol,  	XK_L7,		NoSymbol,	/* 72*/
	SunXK_Paste,  	NoSymbol,  	XK_L8,		NoSymbol,	/* 73*/
	XK_End, 	NoSymbol,	NoSymbol,	NoSymbol,	/* 74*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/* 75*/
	XK_Control_L,	NoSymbol,	NoSymbol,	NoSymbol,	/* 76*/
	XK_A,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 77*/
	XK_S,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 78*/
	XK_D,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 79*/
	XK_F,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 80*/
	XK_G,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 81*/
	XK_H,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 82*/
	XK_J,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 83*/
	XK_K,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 84*/
	XK_L,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 85*/
	XK_semicolon,	XK_colon,	NoSymbol,	NoSymbol,	/* 86*/
	XK_apostrophe,	XK_quotedbl,	XK_acute,	NoSymbol,	/* 87*/
	XK_backslash,	XK_bar, 	NoSymbol,	NoSymbol,	/* 88*/
	XK_Return,	NoSymbol,	NoSymbol,	NoSymbol,	/* 89*/
	XK_KP_Enter,	NoSymbol,	NoSymbol,	NoSymbol,	/* 90*/
	XK_KP_Left,	XK_KP_4, 	NoSymbol,	NoSymbol,	/* 91*/
	NoSymbol, 	XK_KP_5, 	NoSymbol,	NoSymbol,	/* 92*/
	XK_KP_Right,	XK_KP_6, 	NoSymbol,	NoSymbol,	/* 93*/
	XK_KP_Insert,	XK_KP_0,	NoSymbol,	NoSymbol,	/* 94*/
	SunXK_Find,  	NoSymbol,  	XK_L9,		NoSymbol,	/* 95*/
	XK_Prior,	NoSymbol,	NoSymbol,	NoSymbol,	/* 96*/
	SunXK_Cut, 	NoSymbol, 	XK_L10,		NoSymbol,	/* 97*/
	XK_Num_Lock,	NoSymbol,	NoSymbol,	NoSymbol,	/* 98*/
	XK_Shift_L,	NoSymbol,	NoSymbol,	NoSymbol,	/* 99*/
	XK_Z,   	NoSymbol,	NoSymbol,	NoSymbol,	/*100*/
	XK_X,   	NoSymbol,	NoSymbol,	NoSymbol,	/*101*/
	XK_C,   	NoSymbol,	NoSymbol,	NoSymbol,	/*102*/
	XK_V,   	NoSymbol,	NoSymbol,	NoSymbol,	/*103*/
	XK_B,   	NoSymbol,	NoSymbol,	NoSymbol,	/*104*/
	XK_N,   	NoSymbol,	NoSymbol,	NoSymbol,	/*105*/
	XK_M,   	NoSymbol,	NoSymbol,	NoSymbol,	/*106*/
	XK_comma,	XK_less,	NoSymbol,	NoSymbol,	/*107*/
	XK_period,	XK_greater,	NoSymbol,	NoSymbol,	/*108*/
	XK_slash,	XK_question,	NoSymbol,	NoSymbol,	/*109*/
	XK_Shift_R,	NoSymbol,	NoSymbol,	NoSymbol,	/*110*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/*111*/
	XK_KP_End, 	XK_KP_1, 	NoSymbol,	NoSymbol, 	/*112*/
	XK_KP_Down,	XK_KP_2, 	NoSymbol,	NoSymbol,	/*113*/
	XK_KP_Next, 	XK_KP_3, 	NoSymbol,	NoSymbol,	/*114*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/*115*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/*116*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/*117*/
	XK_Help,	NoSymbol,	NoSymbol,	NoSymbol,	/*118*/
	XK_Caps_Lock,	NoSymbol,	NoSymbol,	NoSymbol,	/*119*/
	XK_Meta_L,	NoSymbol,	NoSymbol,	NoSymbol,	/*120*/
	XK_space,	NoSymbol,	NoSymbol,	NoSymbol,	/*121*/
	XK_Meta_R,	NoSymbol,	NoSymbol,	NoSymbol,	/*122*/
	XK_Next,	NoSymbol,	NoSymbol,	NoSymbol,	/*123*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/*124*/
	XK_KP_Add,	NoSymbol,	NoSymbol,	NoSymbol,	/*125*/
};

#define US5Modmap Generic5Modmap

#else

#define US5Keymap NULL
#define US5Modmap NULL

#endif /* US5 */

#ifdef US_UNIX5

static KeySym US_UNIX5Keymap[] = {
	SunXK_Stop,  	NoSymbol,  	XK_L1,		NoSymbol,	/*  1*/
	SunXK_AudioLowerVolume,SunXK_VideoLowerBrightness,NoSymbol,	NoSymbol,	/*  2*/
	SunXK_Again,  	NoSymbol,  	XK_L2,		NoSymbol,	/*  3*/
	SunXK_AudioRaiseVolume,SunXK_VideoRaiseBrightness,NoSymbol,	NoSymbol,	/*  4*/
	XK_F1,  	NoSymbol,	NoSymbol,	NoSymbol,	/*  5*/
	XK_F2,  	NoSymbol,	NoSymbol,	NoSymbol,	/*  6*/
	XK_F10, 	NoSymbol,	NoSymbol,	NoSymbol,	/*  7*/
	XK_F3,  	NoSymbol,	NoSymbol,	NoSymbol,	/*  8*/
	XK_F11,		NoSymbol,	SunXK_F36,	NoSymbol,	/*  9*/
	XK_F4,  	NoSymbol,	NoSymbol,	NoSymbol,	/* 10*/
	XK_F12,		NoSymbol,	SunXK_F37,	NoSymbol,	/* 11*/
	XK_F5,  	NoSymbol,	NoSymbol,	NoSymbol,	/* 12*/
	SunXK_AltGraph,	NoSymbol,	NoSymbol,	NoSymbol,	/* 13*/
	XK_F6,  	NoSymbol,	NoSymbol,	NoSymbol,	/* 14*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/* 15*/
	XK_F7,  	NoSymbol,	NoSymbol,	NoSymbol,	/* 16*/
	XK_F8,  	NoSymbol,	NoSymbol,	NoSymbol,	/* 17*/
	XK_F9,  	NoSymbol,	NoSymbol,	NoSymbol,	/* 18*/
	XK_Alt_L,	NoSymbol,	NoSymbol,	NoSymbol,	/* 19*/
	XK_Up,  	NoSymbol,	NoSymbol,	NoSymbol,	/* 20*/
	XK_Pause,  	NoSymbol,  	XK_Break,	NoSymbol,	/* 21*/
	XK_Print,  	NoSymbol,  	XK_Sys_Req,	SunXK_Sys_Req,	/* 22*/
	XK_Scroll_Lock,	NoSymbol,  	NoSymbol,	NoSymbol,	/* 23*/
	XK_Left,	NoSymbol,	NoSymbol,	NoSymbol,	/* 24*/
	SunXK_Props,  	NoSymbol,  	XK_L3,		NoSymbol,	/* 25*/
	SunXK_Undo,  	NoSymbol,  	XK_L4,		NoSymbol,	/* 26*/
	XK_Down,	NoSymbol,	NoSymbol,	NoSymbol,	/* 27*/
	XK_Right,	NoSymbol,	NoSymbol,	NoSymbol,	/* 28*/
	XK_Escape,	NoSymbol,	NoSymbol,	NoSymbol,	/* 29*/
	XK_1,   	XK_exclam,	NoSymbol,	NoSymbol,	/* 30*/
	XK_2,   	XK_at,  	NoSymbol,	NoSymbol,	/* 31*/
	XK_3,   	XK_numbersign,	NoSymbol,	NoSymbol,	/* 32*/
	XK_4,   	XK_dollar,	NoSymbol,	NoSymbol,	/* 33*/
	XK_5,   	XK_percent,	NoSymbol,	NoSymbol,	/* 34*/
	XK_6,   	XK_asciicircum,	NoSymbol,	NoSymbol,	/* 35*/
	XK_7,   	XK_ampersand,	NoSymbol,	NoSymbol,	/* 36*/
	XK_8,   	XK_asterisk,	NoSymbol,	NoSymbol,	/* 37*/
	XK_9,   	XK_parenleft,	NoSymbol,	NoSymbol,	/* 38*/
	XK_0,   	XK_parenright,	NoSymbol,	NoSymbol,	/* 39*/
	XK_minus,	XK_underscore,	NoSymbol,	NoSymbol,	/* 40*/
	XK_equal,	XK_plus,	NoSymbol,	NoSymbol,	/* 41*/
	XK_grave,	XK_asciitilde,	XK_acute,	NoSymbol,	/* 42*/
	XK_BackSpace,	NoSymbol,	NoSymbol,	NoSymbol,	/* 43*/
	XK_Insert,	NoSymbol,	NoSymbol,	NoSymbol,	/* 44*/
	SunXK_AudioMute,SunXK_VideoDegauss,NoSymbol,	NoSymbol,	/* 45*/
	XK_KP_Divide,  	NoSymbol,  	NoSymbol,	NoSymbol,	/* 46*/
	XK_KP_Multiply, NoSymbol,  	NoSymbol,	NoSymbol,	/* 47*/
	SunXK_PowerSwitch,SunXK_PowerSwitchShift,	NoSymbol,	NoSymbol,	/* 48*/
	SunXK_Front,  	NoSymbol,  	XK_L5,		NoSymbol,	/* 49*/
	XK_KP_Delete,	XK_KP_Decimal,	NoSymbol,	NoSymbol,	/* 50*/
	SunXK_Copy,  	NoSymbol,  	XK_L6,		NoSymbol,	/* 51*/
	XK_Home,	NoSymbol,	NoSymbol,	NoSymbol,	/* 52*/
	XK_Tab, 	NoSymbol,	NoSymbol,	NoSymbol,	/* 53*/
	XK_Q,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 54*/
	XK_W,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 55*/
	XK_E,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 56*/
	XK_R,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 57*/
	XK_T,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 58*/
	XK_Y,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 59*/
	XK_U,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 60*/
	XK_I,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 61*/
	XK_O,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 62*/
	XK_P,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 63*/
	XK_bracketleft,	XK_braceleft,	NoSymbol,	NoSymbol,	/* 64*/
	XK_bracketright,XK_braceright,	NoSymbol,	NoSymbol,	/* 65*/
	XK_Delete,	NoSymbol,	NoSymbol,	NoSymbol,	/* 66*/
	SunXK_Compose,	NoSymbol,	NoSymbol,	NoSymbol,	/* 67*/
	XK_KP_Home,  	XK_KP_7,  	NoSymbol,	NoSymbol,	/* 68*/
	XK_KP_Up,  	XK_KP_8,  	NoSymbol,	NoSymbol,	/* 69*/
	XK_KP_Prior,  	XK_KP_9,  	NoSymbol,	NoSymbol,	/* 70*/
	XK_KP_Subtract, NoSymbol,  	NoSymbol,	NoSymbol,	/* 71*/
	SunXK_Open,  	NoSymbol,  	XK_L7,		NoSymbol,	/* 72*/
	SunXK_Paste,  	NoSymbol,  	XK_L8,		NoSymbol,	/* 73*/
	XK_End, 	NoSymbol,	NoSymbol,	NoSymbol,	/* 74*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/* 75*/
	XK_Control_L,	NoSymbol,	NoSymbol,	NoSymbol,	/* 76*/
	XK_A,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 77*/
	XK_S,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 78*/
	XK_D,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 79*/
	XK_F,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 80*/
	XK_G,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 81*/
	XK_H,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 82*/
	XK_J,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 83*/
	XK_K,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 84*/
	XK_L,   	NoSymbol,	NoSymbol,	NoSymbol,	/* 85*/
	XK_semicolon,	XK_colon,	NoSymbol,	NoSymbol,	/* 86*/
	XK_apostrophe,	XK_quotedbl,	XK_acute,	NoSymbol,	/* 87*/
	XK_backslash,	XK_bar, 	NoSymbol,	NoSymbol,	/* 88*/
	XK_Return,	NoSymbol,	NoSymbol,	NoSymbol,	/* 89*/
	XK_KP_Enter,	NoSymbol,	NoSymbol,	NoSymbol,	/* 90*/
	XK_KP_Left,	XK_KP_4, 	NoSymbol,	NoSymbol,	/* 91*/
	NoSymbol, 	XK_KP_5, 	NoSymbol,	NoSymbol,	/* 92*/
	XK_KP_Right,	XK_KP_6, 	NoSymbol,	NoSymbol,	/* 93*/
	XK_KP_Insert,	XK_KP_0,	NoSymbol,	NoSymbol,	/* 94*/
	SunXK_Find,  	NoSymbol,  	XK_L9,		NoSymbol,	/* 95*/
	XK_Prior,	NoSymbol,	NoSymbol,	NoSymbol,	/* 96*/
	SunXK_Cut, 	NoSymbol, 	XK_L10,		NoSymbol,	/* 97*/
	XK_Num_Lock,	NoSymbol,	NoSymbol,	NoSymbol,	/* 98*/
	XK_Shift_L,	NoSymbol,	NoSymbol,	NoSymbol,	/* 99*/
	XK_Z,   	NoSymbol,	NoSymbol,	NoSymbol,	/*100*/
	XK_X,   	NoSymbol,	NoSymbol,	NoSymbol,	/*101*/
	XK_C,   	NoSymbol,	NoSymbol,	NoSymbol,	/*102*/
	XK_V,   	NoSymbol,	NoSymbol,	NoSymbol,	/*103*/
	XK_B,   	NoSymbol,	NoSymbol,	NoSymbol,	/*104*/
	XK_N,   	NoSymbol,	NoSymbol,	NoSymbol,	/*105*/
	XK_M,   	NoSymbol,	NoSymbol,	NoSymbol,	/*106*/
	XK_comma,	XK_less,	NoSymbol,	NoSymbol,	/*107*/
	XK_period,	XK_greater,	NoSymbol,	NoSymbol,	/*108*/
	XK_slash,	XK_question,	NoSymbol,	NoSymbol,	/*109*/
	XK_Shift_R,	NoSymbol,	NoSymbol,	NoSymbol,	/*110*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/*111*/
	XK_KP_End, 	XK_KP_1, 	NoSymbol,	NoSymbol, 	/*112*/
	XK_KP_Down,	XK_KP_2, 	NoSymbol,	NoSymbol,	/*113*/
	XK_KP_Next, 	XK_KP_3, 	NoSymbol,	NoSymbol,	/*114*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/*115*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/*116*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/*117*/
	XK_Help,	NoSymbol,	NoSymbol,	NoSymbol,	/*118*/
	XK_Caps_Lock,	NoSymbol,	NoSymbol,	NoSymbol,	/*119*/
	XK_Meta_L,	NoSymbol,	NoSymbol,	NoSymbol,	/*120*/
	XK_space,	NoSymbol,	NoSymbol,	NoSymbol,	/*121*/
	XK_Meta_R,	NoSymbol,	NoSymbol,	NoSymbol,	/*122*/
	XK_Next,	NoSymbol,	NoSymbol,	NoSymbol,	/*123*/
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,	/*124*/
	XK_KP_Add,	NoSymbol,	NoSymbol,	NoSymbol,	/*125*/
};

#define US_UNIX5Modmap Generic5Modmap

#else

#define US_UNIX5Keymap NULL
#define US_UNIX5Modmap NULL

#endif /* US_UNIX5 */
