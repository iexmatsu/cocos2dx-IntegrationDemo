#pragma once

#ifdef SOUND_BANK_PATH
#undef SOUND_BANK_PATH
#define SOUND_BANK_PATH "../../../../../../../../WwiseProject/GeneratedSoundBanks/Mac" // AkOSChar is char. Don't use L-stirng here. CWD on Mac is the POSIX-executable dir.
#endif

#define FONT_SIZE_TITLE  30
#define FONT_SIZE_MENU   24

extern bool g_isLandscape;
#define SCREEN_POS_SCALER_DESCRIPTION_Y 0.90f
#define SCREEN_POS_SCALER_SELBUTTON_X 0.15f
