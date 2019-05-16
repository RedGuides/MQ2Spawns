// This plugin is intended for VIP members of MacroQuest2.com only
// and may not be redistributed without permission of the author

#include "../MQ2Plugin.h"
using namespace std;
#include <vector>

const char*  MODULE_NAME    = "MQ2Spawns";
const double MODULE_VERSION = 11.0324;
PreSetup(MODULE_NAME);

const unsigned char ARG_SPAWN   = 1;
const unsigned char ARG_DESPAWN = 2;
const unsigned char ARG_MINLVL  = 3;
const unsigned char ARG_MAXLVL  = 4;
const unsigned char ARG_COLOR   = 5;
const unsigned char ARG_STOGGLE = 6;
const unsigned char ARG_DTOGGLE = 7;
const unsigned char ARG_EXCLUDE = 8;

time_t tSeconds;
__time64_t tCurrentTime;
bool bZoning = true;
bool bLoaded = false;
char szCharName[MAX_STRING] = {0};
vector<string> SpawnFilters;

// added logging features
char szDirPath[MAX_STRING]  = {0};
char szLogFile[MAX_STRING]  = {0};
char szLogPath[MAX_STRING]  = {0};

bool bLogActive             = false;
bool bLogReady              = false;
bool bLogCmdUsed            = false;

FILE* fOurLog;
time_t rawtime;


enum
{
    LOG_FAIL    = 0,
    LOG_SUCCESS = 1,
};

int StartLog()
{
    errno_t err = fopen_s(&fOurLog,szLogPath, "a+");
    if (err)
    {
        WriteChatf("\ay%s\aw:: Unable to open log file. Check your file path and permissions. ( \ag%s \ax)", MODULE_NAME, szLogPath);
        bLogActive = bLogReady = false;
        return LOG_FAIL;
    }
    else
    {
        char szTemp[MAX_STRING] = {0};
        time(&rawtime);
		tm timeinfo = { 0 };
        localtime_s(&timeinfo, &rawtime);
        strftime(szTemp, MAX_STRING, "New Logging Session Started on %Y-%m-%d at %H:%M:%S\n", &timeinfo);
        fprintf(fOurLog, "%s", szTemp);
        fclose(fOurLog);
        bLogReady = true;
    }
    return LOG_SUCCESS;
}

int EndLog()
{
    errno_t err = fopen_s(&fOurLog, szLogPath, "a+");
    if (!err)
    {
        char szTemp[MAX_STRING] = {0};
        time(&rawtime);
		tm timeinfo = { 0 };
		localtime_s(&timeinfo, &rawtime);
        strftime(szTemp, MAX_STRING, "Logging Session Ended on %Y-%m-%d at %H:%M:%S\n\n", &timeinfo);
        fprintf(fOurLog, "%s", szTemp);
        fclose(fOurLog);
        bLogActive = bLogReady = false;
        return LOG_SUCCESS;
    }
    return LOG_FAIL;
}

void WriteToLog(char* szWriteLine)
{
    errno_t err = fopen_s(&fOurLog,szLogPath, "a+");
    if (err)
    {
        WriteChatf("\ay%s\aw:: Unable to open log file. Check your file path and permissions. ( \ag%s \ax)", MODULE_NAME, szLogPath);
        bLogActive = bLogReady = false;
        WriteChatf("\ay%s\aw:: Logging \arDISABLED\aw.", MODULE_NAME);
    }
    else
    {
        char szTemp[MAX_STRING] = {0};
        time(&rawtime);
		tm timeinfo = { 0 };
        localtime_s(&timeinfo,&rawtime);
        strftime(szTemp, MAX_STRING, "[%H:%M:%S] ", &timeinfo);
        strcat_s(szTemp, szWriteLine);
        strcat_s(szTemp, "\n");
        fprintf(fOurLog, "%s", szTemp);
        fclose(fOurLog);
    }
}

// end logging features

// watch type class: new instance created for each type
class CWatchType
{
public:
    CWatchType()
    {
        MinLVL = 1;
        MaxLVL = MAX_NPC_LEVEL;
        Spawn = Despawn = false;
        Color = 0xFFDEAD;
    }

    bool Spawn;
    bool Despawn;
    int MinLVL;
    int MaxLVL;
    unsigned long Color;
};
CWatchType* SpawnFormat = NULL;

// our configuration class: one instance
class CSpawnCFG
{
public:
    CSpawnCFG()
    {
        Delay = 10;
        ON         = Timestamp = true;
        PC.Spawn   = NPC.Spawn = true;
        AutoSave   = Location  = false;
        SaveByChar = Logging   = false;
    }

    CWatchType PC;
    CWatchType NPC;
    CWatchType MOUNT;
    CWatchType PET;
    CWatchType MERCENARY;
    CWatchType CORPSE;
    CWatchType FLYER;
    CWatchType CAMPFIRE;
    CWatchType BANNER;
    CWatchType AURA;
    CWatchType OBJECT;
    CWatchType UNTARGETABLE;
    CWatchType CHEST;
    CWatchType TRAP;
    CWatchType TIMER;
    CWatchType TRIGGER;
    CWatchType ITEM;
    CWatchType UNKNOWN;

    CWatchType ALL;

    bool ON;
    bool AutoSave;
    bool Location;
    bool SaveByChar;
    bool SpawnID;
    bool Timestamp;
    bool Logging;
    unsigned long Delay;
};
CSpawnCFG CFG;

// UI window
class CSpawnWnd;
CSpawnWnd* OurWnd = NULL;
class CSpawnWnd : public CCustomWnd
{
public:
    CStmlWnd *StmlOut;
    CXWnd *OutWnd;
    struct _CSIDLWND *OutStruct;

    CSpawnWnd(CXStr &Template):CCustomWnd(Template)
    {
        SetWndNotification(CSpawnWnd);
        StmlOut = (CStmlWnd *)GetChildItem("CW_ChatOutput");
        OutWnd = (CXWnd*)StmlOut;
        OutWnd->SetClickable(1);
        OutStruct = (_CSIDLWND*)GetChildItem("CW_ChatOutput");
        OutStruct->SetClickable(0);
        RemoveStyle(CWS_CLOSE);
        SetEscapable(0);
		StmlOut->MaxLines = 400;
        //*(DWORD*)&(((PCHAR)StmlOut)[EQ_CHAT_HISTORY_OFFSET]) = 400;
    }

    int WndNotification(CXWnd *pWnd, unsigned int Message, void *data)
    {
        if (pWnd == NULL && Message == XWM_CLOSE)
        {
            SetVisible(1);
            return 0;
        }
        return CSidlScreenWnd::WndNotification(pWnd,Message,data);
    };

    void SetFontSize(unsigned int size)
    {
        struct FONTDATA
        {
            DWORD NumFonts;
            PCHAR* Fonts; 
        };
        FONTDATA* Fonts;    // font array structure
        DWORD* SelFont;     // selected font
        Fonts = (FONTDATA*)&(((char*)pWndMgr)[EQ_CHAT_FONT_OFFSET]);

        if (size < 0 || size >= (int) Fonts->NumFonts)
        {
            return;
        }
        if (Fonts->Fonts == NULL || OurWnd == NULL)
        {
            return;
        }

        SelFont = (DWORD*)Fonts->Fonts[size];
        CXStr str(((CStmlWnd*)OurWnd->OutWnd)->GetSTMLText());
        ((CXWnd*)OurWnd->OutWnd)->SetFont(SelFont);
        ((CStmlWnd*)OurWnd->OutWnd)->SetSTMLText(str, 1, 0);
        ((CStmlWnd*)OurWnd->OutWnd)->ForceParseNow();
        DebugTry(((CXWnd*)OurWnd->OutWnd)->SetVScrollPos(OurWnd->OutWnd->GetVScrollMax()));
        OurWnd->FontSize = size;
    };

    unsigned long FontSize;
};
template <unsigned int _Size>LPSTR SafeItoa(int _Value,char(&_Buffer)[_Size], int _Radix)
{
	errno_t err = _itoa_s(_Value, _Buffer, _Radix);
	if (!err) {
		return _Buffer;
	}
	return "";
}
void SaveOurWnd()
{
    PCSIDLWND UseWnd = (PCSIDLWND)OurWnd;
    if (!UseWnd || !OurWnd) return;

    char szTemp[MAX_STRING]               = {0};

    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "ChatTop",      SafeItoa(UseWnd->GetLocation().top,    szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "ChatBottom",   SafeItoa(UseWnd->GetLocation().bottom, szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "ChatLeft",     SafeItoa(UseWnd->GetLocation().left,   szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "ChatRight",    SafeItoa(UseWnd->GetLocation().right,  szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "Fades",        SafeItoa(UseWnd->GetFades(),           szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "Alpha",        SafeItoa(UseWnd->GetAlpha(),           szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "FadeToAlpha",  SafeItoa(UseWnd->GetFadeToAlpha(),     szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "Duration",     SafeItoa(UseWnd->GetFadeDuration(),    szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "Locked",       SafeItoa(UseWnd->IsLocked(),          szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "Delay",        SafeItoa(UseWnd->GetFadeDelay(),   szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "BGType",       SafeItoa(UseWnd->GetBGType(),          szTemp, 10), INIFileName);
	ARGBCOLOR col = { 0 };
	col.ARGB = UseWnd->GetBGColor();
	WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "BGTint.alpha",   SafeItoa(col.A,       szTemp, 10), INIFileName);
	WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "BGTint.red",   SafeItoa(col.R,       szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "BGTint.green", SafeItoa(col.G,       szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "BGTint.blue",  SafeItoa(col.B,       szTemp, 10), INIFileName);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "FontSize",     SafeItoa(OurWnd->FontSize,   szTemp, 10), INIFileName);

    GetCXStr(UseWnd->CGetWindowText(), szTemp);
    WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "WindowTitle", szTemp, INIFileName);
}

void CreateOurWnd()
{
    if (OurWnd == NULL)
    {
        OurWnd = new CSpawnWnd(CXStr("ChatWindow"));

		OurWnd->SetLocation({ (LONG)GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "ChatLeft",     10,   INIFileName),
			(LONG)GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "ChatTop",      10,   INIFileName),
			(LONG)GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "ChatRight",    410,  INIFileName),
			(LONG)GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "ChatBottom",   210,  INIFileName) });

        OurWnd->SetFades((GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "Fades",        0,    INIFileName) ? true:false));
        OurWnd->SetAlpha(GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "Alpha",        255,  INIFileName));
        OurWnd->SetFadeToAlpha(GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "FadeToAlpha",  255,  INIFileName));
        OurWnd->SetFadeDuration(GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "Duration",     500,  INIFileName));
        OurWnd->SetLocked((GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "Locked",       0,    INIFileName) ? true:false));
        OurWnd->SetFadeDelay(GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "Delay",        2000, INIFileName));
        OurWnd->SetBGType(GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "BGType",       1,    INIFileName));
		ARGBCOLOR col = { 0 };
		col.A			        = GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "BGTint.alpha",   255,  INIFileName);
        col.R			        = GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "BGTint.red",   0,  INIFileName);
        col.G				    = GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "BGTint.green", 0,  INIFileName);
        col.B					= GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "BGTint.blue",  0,  INIFileName);
		OurWnd->SetBGColor(col.ARGB);
        OurWnd->SetFontSize(GetPrivateProfileInt(CFG.SaveByChar ? szCharName : "Window", "FontSize",     2,    INIFileName));

        char szWindowText[MAX_STRING] = {0};
        GetPrivateProfileString(CFG.SaveByChar ? szCharName : "Window", "WindowTitle", "Spawns", szWindowText, MAX_STRING, INIFileName);
        OurWnd->CSetWindowText(szWindowText);
        ((CXWnd*)OurWnd)->Show(1, 1);
        OurWnd->OutStruct->RemoveStyle(CWS_CLOSE);
        //BitOff(OurWnd->OutStruct->WindowStyle, CWS_CLOSE);
    }
}

void KillOurWnd(bool bSave)
{
    if (OurWnd)
    {
        if (bSave) SaveOurWnd();
        delete OurWnd;
        OurWnd = NULL;
    }
}

template <unsigned int _Size>char* itohex(int iNum, CHAR(&szTemp)[_Size])
{
    sprintf_s(szTemp, "%06X", iNum);
    return szTemp;
}

void HandleConfig(bool bSave)
{
    if (bSave)
    {
        char szSave[MAX_STRING] = {0};
        WritePrivateProfileString("Settings",  "AutoSave",   CFG.AutoSave             ? "on" : "off",    INIFileName);
        WritePrivateProfileString("Settings",  "Delay",      SafeItoa((int)CFG.Delay,           szSave, 10), INIFileName);
        WritePrivateProfileString("Settings",  "Enabled",    CFG.ON                   ? "on" : "off",    INIFileName);
        WritePrivateProfileString("Settings",  "SaveByChar", CFG.SaveByChar           ? "on" : "off",    INIFileName);
        WritePrivateProfileString("Settings",  "ShowLoc",    CFG.Location             ? "on" : "off",    INIFileName);
        WritePrivateProfileString("Settings",  "SpawnID",    CFG.SpawnID              ? "on" : "off",    INIFileName);
        WritePrivateProfileString("Settings",  "Timestamp",  CFG.Timestamp            ? "on" : "off",    INIFileName);

        // logging uses savebychar at dewey's request
        WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Settings",  "Logging",    CFG.Logging ? "on" : "off",    INIFileName);
        WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Settings",  "LogPath",    szDirPath,                     INIFileName);
        // end of logging

        WritePrivateProfileString("ALL",          "Spawn",   CFG.ALL.Spawn            ? "on" : "off",    INIFileName);
        WritePrivateProfileString("ALL",          "Despawn", CFG.ALL.Despawn          ? "on" : "off",    INIFileName);
        WritePrivateProfileString("ALL",          "MinLVL",  SafeItoa(CFG.ALL.MinLVL,           szSave, 10), INIFileName);
        WritePrivateProfileString("ALL",          "MaxLVL",  SafeItoa(CFG.ALL.MaxLVL,           szSave, 10), INIFileName);
        WritePrivateProfileString("ALL",          "Color",   itohex(CFG.ALL.Color,          szSave),     INIFileName);
        WritePrivateProfileString("PC",           "Spawn",   CFG.PC.Spawn             ? "on" : "off",    INIFileName);
        WritePrivateProfileString("PC",           "Despawn", CFG.PC.Despawn           ? "on" : "off",    INIFileName);
        WritePrivateProfileString("PC",           "MinLVL",  SafeItoa(CFG.PC.MinLVL,            szSave, 10), INIFileName);
        WritePrivateProfileString("PC",           "MaxLVL",  SafeItoa(CFG.PC.MaxLVL,            szSave, 10), INIFileName);
        WritePrivateProfileString("PC",           "Color",   itohex(CFG.PC.Color,           szSave),     INIFileName);
        WritePrivateProfileString("NPC",          "Spawn",   CFG.NPC.Spawn            ? "on" : "off",    INIFileName);
        WritePrivateProfileString("NPC",          "Despawn", CFG.NPC.Despawn          ? "on" : "off",    INIFileName);
        WritePrivateProfileString("NPC",          "MinLVL",  SafeItoa(CFG.NPC.MinLVL,          szSave, 10),  INIFileName);
        WritePrivateProfileString("NPC",          "MaxLVL",  SafeItoa(CFG.NPC.MaxLVL,          szSave, 10),  INIFileName);
        WritePrivateProfileString("NPC",          "Color",   itohex(CFG.NPC.Color,         szSave),      INIFileName);
        WritePrivateProfileString("MOUNT",        "Spawn",   CFG.MOUNT.Spawn          ? "on" : "off",    INIFileName);
        WritePrivateProfileString("MOUNT",        "Despawn", CFG.MOUNT.Despawn        ? "on" : "off",    INIFileName);
        WritePrivateProfileString("MOUNT",        "MinLVL",  SafeItoa(CFG.MOUNT.MinLVL,         szSave, 10), INIFileName);
        WritePrivateProfileString("MOUNT",        "MaxLVL",  SafeItoa(CFG.MOUNT.MaxLVL,         szSave, 10), INIFileName);
        WritePrivateProfileString("MOUNT",        "Color",   itohex(CFG.MOUNT.Color,        szSave),     INIFileName);
        WritePrivateProfileString("PET",          "Spawn",   CFG.PET.Spawn            ? "on" : "off",    INIFileName);
        WritePrivateProfileString("PET",          "Despawn", CFG.PET.Despawn          ? "on" : "off",    INIFileName);
        WritePrivateProfileString("PET",          "MinLVL",  SafeItoa(CFG.PET.MinLVL,           szSave, 10), INIFileName);
        WritePrivateProfileString("PET",          "MaxLVL",  SafeItoa(CFG.PET.MaxLVL,           szSave, 10), INIFileName);
        WritePrivateProfileString("PET",          "Color",   itohex(CFG.PET.Color,          szSave),     INIFileName);
        WritePrivateProfileString("MERCENARY",    "Spawn",   CFG.MERCENARY.Spawn      ? "on" : "off",    INIFileName);
        WritePrivateProfileString("MERCENARY",    "Despawn", CFG.MERCENARY.Despawn    ? "on" : "off",    INIFileName);
        WritePrivateProfileString("MERCENARY",    "MinLVL",  SafeItoa(CFG.MERCENARY.MinLVL,     szSave, 10), INIFileName);
        WritePrivateProfileString("MERCENARY",    "MaxLVL",  SafeItoa(CFG.MERCENARY.MaxLVL,     szSave, 10), INIFileName);
        WritePrivateProfileString("MERCENARY",    "Color",   itohex(CFG.MERCENARY.Color,    szSave),     INIFileName);
        WritePrivateProfileString("FLYER",        "Spawn",   CFG.FLYER.Spawn          ? "on" : "off",    INIFileName);
        WritePrivateProfileString("FLYER",        "Despawn", CFG.FLYER.Despawn        ? "on" : "off",    INIFileName);
        WritePrivateProfileString("FLYER",        "MinLVL",  SafeItoa(CFG.FLYER.MinLVL,         szSave, 10), INIFileName);
        WritePrivateProfileString("FLYER",        "MaxLVL",  SafeItoa(CFG.FLYER.MaxLVL,         szSave, 10), INIFileName);
        WritePrivateProfileString("FLYER",        "Color",   itohex(CFG.FLYER.Color,        szSave),     INIFileName);
        WritePrivateProfileString("CAMPFIRE",     "Spawn",   CFG.CAMPFIRE.Spawn       ? "on" : "off",    INIFileName);
        WritePrivateProfileString("CAMPFIRE",     "Despawn", CFG.CAMPFIRE.Despawn     ? "on" : "off",    INIFileName);
        WritePrivateProfileString("CAMPFIRE",     "MinLVL",  SafeItoa(CFG.CAMPFIRE.MinLVL,      szSave, 10), INIFileName);
        WritePrivateProfileString("CAMPFIRE",     "MaxLVL",  SafeItoa(CFG.CAMPFIRE.MaxLVL,      szSave, 10), INIFileName);
        WritePrivateProfileString("CAMPFIRE",     "Color",   itohex(CFG.CAMPFIRE.Color,     szSave),     INIFileName);
        WritePrivateProfileString("BANNER",       "Spawn",   CFG.BANNER.Spawn         ? "on" : "off",    INIFileName);
        WritePrivateProfileString("BANNER",       "Despawn", CFG.BANNER.Despawn       ? "on" : "off",    INIFileName);
        WritePrivateProfileString("BANNER",       "MinLVL",  SafeItoa(CFG.BANNER.MinLVL,        szSave, 10), INIFileName);
        WritePrivateProfileString("BANNER",       "MaxLVL",  SafeItoa(CFG.BANNER.MaxLVL,        szSave, 10), INIFileName);
        WritePrivateProfileString("BANNER",       "Color",   itohex(CFG.BANNER.Color,       szSave),     INIFileName);
        WritePrivateProfileString("AURA",         "Spawn",   CFG.AURA.Spawn           ? "on" : "off",    INIFileName);
        WritePrivateProfileString("AURA",         "Despawn", CFG.AURA.Despawn         ? "on" : "off",    INIFileName);
        WritePrivateProfileString("AURA",         "MinLVL",  SafeItoa(CFG.AURA.MinLVL,          szSave, 10), INIFileName);
        WritePrivateProfileString("AURA",         "MaxLVL",  SafeItoa(CFG.AURA.MaxLVL,          szSave, 10), INIFileName);
        WritePrivateProfileString("AURA",         "Color",   itohex(CFG.AURA.Color,         szSave),     INIFileName);
        WritePrivateProfileString("OBJECT",       "Spawn",   CFG.OBJECT.Spawn         ? "on" : "off",    INIFileName);
        WritePrivateProfileString("OBJECT",       "Despawn", CFG.OBJECT.Despawn       ? "on" : "off",    INIFileName);
        WritePrivateProfileString("OBJECT",       "MinLVL",  SafeItoa(CFG.OBJECT.MinLVL,        szSave, 10), INIFileName);
        WritePrivateProfileString("OBJECT",       "MaxLVL",  SafeItoa(CFG.OBJECT.MaxLVL,        szSave, 10), INIFileName);
        WritePrivateProfileString("OBJECT",       "Color",   itohex(CFG.OBJECT.Color,       szSave),     INIFileName);
        WritePrivateProfileString("UNTARGETABLE", "Spawn",   CFG.UNTARGETABLE.Spawn   ? "on" : "off",    INIFileName);
        WritePrivateProfileString("UNTARGETABLE", "Despawn", CFG.UNTARGETABLE.Despawn ? "on" : "off",    INIFileName);
        WritePrivateProfileString("UNTARGETABLE", "MinLVL",  SafeItoa(CFG.UNTARGETABLE.MinLVL,  szSave, 10), INIFileName);
        WritePrivateProfileString("UNTARGETABLE", "MaxLVL",  SafeItoa(CFG.UNTARGETABLE.MaxLVL,  szSave, 10), INIFileName);
        WritePrivateProfileString("UNTARGETABLE", "Color",   itohex(CFG.UNTARGETABLE.Color, szSave),     INIFileName);
        WritePrivateProfileString("CHEST",        "Spawn",   CFG.CHEST.Spawn          ? "on" : "off",    INIFileName);
        WritePrivateProfileString("CHEST",        "Despawn", CFG.CHEST.Despawn        ? "on" : "off",    INIFileName);
        WritePrivateProfileString("CHEST",        "MinLVL",  SafeItoa(CFG.CHEST.MinLVL,         szSave, 10), INIFileName);
        WritePrivateProfileString("CHEST",        "MaxLVL",  SafeItoa(CFG.CHEST.MaxLVL,         szSave, 10), INIFileName);
        WritePrivateProfileString("CHEST",        "Color",   itohex(CFG.CHEST.Color,        szSave),     INIFileName);
        WritePrivateProfileString("TRAP",         "Spawn",   CFG.TRAP.Spawn           ? "on" : "off",    INIFileName);
        WritePrivateProfileString("TRAP",         "Despawn", CFG.TRAP.Despawn         ? "on" : "off",    INIFileName);
        WritePrivateProfileString("TRAP",         "MinLVL",  SafeItoa(CFG.TRAP.MinLVL,          szSave, 10), INIFileName);
        WritePrivateProfileString("TRAP",         "MaxLVL",  SafeItoa(CFG.TRAP.MaxLVL,          szSave, 10), INIFileName);
        WritePrivateProfileString("TRAP",         "Color",   itohex(CFG.TRAP.Color,         szSave),     INIFileName);
        WritePrivateProfileString("TIMER",        "Spawn",   CFG.TIMER.Spawn          ? "on" : "off",    INIFileName);
        WritePrivateProfileString("TIMER",        "Despawn", CFG.TIMER.Despawn        ? "on" : "off",    INIFileName);
        WritePrivateProfileString("TIMER",        "MinLVL",  SafeItoa(CFG.TIMER.MinLVL,         szSave, 10), INIFileName);
        WritePrivateProfileString("TIMER",        "MaxLVL",  SafeItoa(CFG.TIMER.MaxLVL,         szSave, 10), INIFileName);
        WritePrivateProfileString("TIMER",        "Color",   itohex(CFG.TIMER.Color,        szSave),     INIFileName);
        WritePrivateProfileString("TRIGGER",      "Spawn",   CFG.TRIGGER.Spawn        ? "on" : "off",    INIFileName);
        WritePrivateProfileString("TRIGGER",      "Despawn", CFG.TRIGGER.Despawn      ? "on" : "off",    INIFileName);
        WritePrivateProfileString("TRIGGER",      "MinLVL",  SafeItoa(CFG.TRIGGER.MinLVL,       szSave, 10), INIFileName);
        WritePrivateProfileString("TRIGGER",      "MaxLVL",  SafeItoa(CFG.TRIGGER.MaxLVL,       szSave, 10), INIFileName);
        WritePrivateProfileString("TRIGGER",      "Color",   itohex(CFG.TRIGGER.Color,      szSave),     INIFileName);
        WritePrivateProfileString("CORPSE",       "Spawn",   CFG.CORPSE.Spawn         ? "on" : "off",    INIFileName);
        WritePrivateProfileString("CORPSE",       "Despawn", CFG.CORPSE.Despawn       ? "on" : "off",    INIFileName);
        WritePrivateProfileString("CORPSE",       "MinLVL",  SafeItoa(CFG.CORPSE.MinLVL,        szSave, 10), INIFileName);
        WritePrivateProfileString("CORPSE",       "MaxLVL",  SafeItoa(CFG.CORPSE.MaxLVL,        szSave, 10), INIFileName);
        WritePrivateProfileString("CORPSE",       "Color",   itohex(CFG.CORPSE.Color,       szSave),     INIFileName);
        WritePrivateProfileString("ITEM",         "Spawn",   CFG.ITEM.Spawn           ? "on" : "off",    INIFileName);
        WritePrivateProfileString("ITEM",         "Despawn", CFG.ITEM.Despawn         ? "on" : "off",    INIFileName);
        WritePrivateProfileString("ITEM",         "MinLVL",  SafeItoa(CFG.ITEM.MinLVL,          szSave, 10), INIFileName);
        WritePrivateProfileString("ITEM",         "MaxLVL",  SafeItoa(CFG.ITEM.MaxLVL,          szSave, 10), INIFileName);
        WritePrivateProfileString("ITEM",         "Color",   itohex(CFG.ITEM.Color,         szSave),     INIFileName);
        WritePrivateProfileString("UNKNOWN",      "Spawn",   CFG.UNKNOWN.Spawn        ? "on" : "off",    INIFileName);
        WritePrivateProfileString("UNKNOWN",      "Despawn", CFG.UNKNOWN.Despawn      ? "on" : "off",    INIFileName);
        WritePrivateProfileString("UNKNOWN",      "MinLVL",  SafeItoa(CFG.UNKNOWN.MinLVL,       szSave, 10), INIFileName);
        WritePrivateProfileString("UNKNOWN",      "MaxLVL",  SafeItoa(CFG.UNKNOWN.MaxLVL,       szSave, 10), INIFileName);
        WritePrivateProfileString("UNKNOWN",      "Color",   itohex(CFG.UNKNOWN.Color,      szSave),     INIFileName);

        SaveOurWnd();
    }
    else
    {
        char szLoad[MAX_STRING] = {0};
        int iValid = GetPrivateProfileInt("Settings", "Delay", (int)CFG.Delay, INIFileName);
        if (iValid > 0) CFG.Delay = iValid;

        GetPrivateProfileString("Settings", "AutoSave",    CFG.AutoSave   ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.AutoSave   = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("Settings", "Enabled",     CFG.ON         ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.ON         = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("Settings", "SaveByChar",  CFG.SaveByChar ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.SaveByChar = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("Settings", "ShowLoc",     CFG.Location   ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.Location   = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("Settings", "SpawnID",     CFG.SpawnID    ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.SpawnID    = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("Settings", "Timestamp",   CFG.Timestamp  ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.Timestamp  = (!_strnicmp(szLoad, "on", 3));

        //logging addition
        GetPrivateProfileString(CFG.SaveByChar ? szCharName : "Settings", "Logging", CFG.Logging ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.Logging    = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString(CFG.SaveByChar ? szCharName : "Settings", "LogPath", gszINIPath,              szDirPath, MAX_STRING, INIFileName);
        sprintf_s(szLogPath, "%s\\%s", szDirPath, szLogFile);
        if (CFG.Logging) bLogActive = true;
        // end of logging


        GetPrivateProfileString("ALL", "Spawn", CFG.ALL.Spawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.ALL.Spawn = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("ALL", "Despawn", CFG.ALL.Despawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.ALL.Despawn = (!_strnicmp(szLoad, "on", 3));
        CFG.ALL.MinLVL = GetPrivateProfileInt("ALL", "MinLVL", CFG.ALL.MinLVL, INIFileName);
        CFG.ALL.MaxLVL = GetPrivateProfileInt("ALL", "MaxLVL", CFG.ALL.MaxLVL, INIFileName);
        GetPrivateProfileString("ALL", "Color", "FFFF00", szLoad, MAX_STRING, INIFileName);
        sscanf_s(szLoad, "%06X", &CFG.ALL.Color);
        GetPrivateProfileString("PC", "Spawn", CFG.PC.Spawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.PC.Spawn = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("PC", "Despawn", CFG.PC.Despawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.PC.Despawn = (!_strnicmp(szLoad, "on", 3));
        CFG.PC.MinLVL = GetPrivateProfileInt("PC", "MinLVL", CFG.PC.MinLVL, INIFileName);
        CFG.PC.MaxLVL = GetPrivateProfileInt("PC", "MaxLVL", CFG.PC.MaxLVL, INIFileName);
        GetPrivateProfileString("PC", "Color", "00FF00", szLoad, MAX_STRING, INIFileName);
        sscanf_s(szLoad, "%06X", &CFG.PC.Color);
        GetPrivateProfileString("NPC", "Spawn", CFG.NPC.Spawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.NPC.Spawn = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("NPC", "Despawn", CFG.NPC.Despawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.NPC.Despawn = (!_strnicmp(szLoad, "on", 3));
        CFG.NPC.MinLVL = GetPrivateProfileInt("NPC", "MinLVL", CFG.NPC.MinLVL, INIFileName);
        CFG.NPC.MaxLVL = GetPrivateProfileInt("NPC", "MaxLVL", CFG.NPC.MaxLVL, INIFileName);
        GetPrivateProfileString("NPC", "Color", "FF0000", szLoad, MAX_STRING, INIFileName);
        sscanf_s(szLoad, "%06X", &CFG.NPC.Color);
        GetPrivateProfileString("MOUNT", "Spawn", CFG.MOUNT.Spawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.MOUNT.Spawn = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("MOUNT", "Despawn", CFG.MOUNT.Despawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.MOUNT.Despawn = (!_strnicmp(szLoad, "on", 3));
        CFG.MOUNT.MinLVL = GetPrivateProfileInt("MOUNT", "MinLVL", CFG.MOUNT.MinLVL, INIFileName);
        CFG.MOUNT.MaxLVL = GetPrivateProfileInt("MOUNT", "MaxLVL", CFG.MOUNT.MaxLVL, INIFileName);
        GetPrivateProfileString("MOUNT", "Color", "BEBEBE", szLoad, MAX_STRING, INIFileName);
        sscanf_s(szLoad, "%06X", &CFG.MOUNT.Color);
        GetPrivateProfileString("PET", "Spawn", CFG.PET.Spawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.PET.Spawn = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("PET", "Despawn", CFG.PET.Despawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.PET.Despawn = (!_strnicmp(szLoad, "on", 3));
        CFG.PET.MinLVL = GetPrivateProfileInt("PET", "MinLVL", CFG.PET.MinLVL, INIFileName);
        CFG.PET.MaxLVL = GetPrivateProfileInt("PET", "MaxLVL", CFG.PET.MaxLVL, INIFileName);
        GetPrivateProfileString("PET", "Color", "6B8E23", szLoad, MAX_STRING, INIFileName);
        sscanf_s(szLoad, "%06X", &CFG.PET.Color);
        GetPrivateProfileString("MERCENARY", "Spawn", CFG.MERCENARY.Spawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.MERCENARY.Spawn = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("MERCENARY", "Despawn", CFG.MERCENARY.Despawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.MERCENARY.Despawn = (!_strnicmp(szLoad, "on", 3));
        CFG.MERCENARY.MinLVL = GetPrivateProfileInt("MERCENARY", "MinLVL", CFG.MERCENARY.MinLVL, INIFileName);
        CFG.MERCENARY.MaxLVL = GetPrivateProfileInt("MERCENARY", "MaxLVL", CFG.MERCENARY.MaxLVL, INIFileName);
        GetPrivateProfileString("MERCENARY", "Color", "FFDEAD", szLoad, MAX_STRING, INIFileName);
        sscanf_s(szLoad, "%06X", &CFG.MERCENARY.Color);
        GetPrivateProfileString("FLYER", "Spawn", CFG.FLYER.Spawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.FLYER.Spawn = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("FLYER", "Despawn", CFG.FLYER.Despawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.FLYER.Despawn = (!_strnicmp(szLoad, "on", 3));
        CFG.FLYER.MinLVL = GetPrivateProfileInt("FLYER", "MinLVL", CFG.FLYER.MinLVL, INIFileName);
        CFG.FLYER.MaxLVL = GetPrivateProfileInt("FLYER", "MaxLVL", CFG.FLYER.MaxLVL, INIFileName);
        GetPrivateProfileString("FLYER", "Color", "BEBEBE", szLoad, MAX_STRING, INIFileName);
        sscanf_s(szLoad, "%06X", &CFG.FLYER.Color);
        GetPrivateProfileString("CAMPFIRE", "Spawn", CFG.CAMPFIRE.Spawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.CAMPFIRE.Spawn = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("CAMPFIRE", "Despawn", CFG.CAMPFIRE.Despawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.CAMPFIRE.Despawn = (!_strnicmp(szLoad, "on", 3));
        CFG.CAMPFIRE.MinLVL = GetPrivateProfileInt("CAMPFIRE", "MinLVL", CFG.CAMPFIRE.MinLVL, INIFileName);
        CFG.CAMPFIRE.MaxLVL = GetPrivateProfileInt("CAMPFIRE", "MaxLVL", CFG.CAMPFIRE.MaxLVL, INIFileName);
        GetPrivateProfileString("CAMPFIRE", "Color", "BEBEBE", szLoad, MAX_STRING, INIFileName);
        sscanf_s(szLoad, "%06X", &CFG.CAMPFIRE.Color);
        GetPrivateProfileString("BANNER", "Spawn", CFG.BANNER.Spawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.BANNER.Spawn = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("BANNER", "Despawn", CFG.BANNER.Despawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.BANNER.Despawn = (!_strnicmp(szLoad, "on", 3));
        CFG.BANNER.MinLVL = GetPrivateProfileInt("BANNER", "MinLVL", CFG.BANNER.MinLVL, INIFileName);
        CFG.BANNER.MaxLVL = GetPrivateProfileInt("BANNER", "MaxLVL", CFG.BANNER.MaxLVL, INIFileName);
        GetPrivateProfileString("BANNER", "Color", "BEBEBE", szLoad, MAX_STRING, INIFileName);
        sscanf_s(szLoad, "%06X", &CFG.BANNER.Color);
        GetPrivateProfileString("AURA", "Spawn", CFG.AURA.Spawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.AURA.Spawn = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("AURA", "Despawn", CFG.AURA.Despawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.AURA.Despawn = (!_strnicmp(szLoad, "on", 3));
        CFG.AURA.MinLVL = GetPrivateProfileInt("AURA", "MinLVL", CFG.AURA.MinLVL, INIFileName);
        CFG.AURA.MaxLVL = GetPrivateProfileInt("AURA", "MaxLVL", CFG.AURA.MaxLVL, INIFileName);
        GetPrivateProfileString("AURA", "Color", "BEBEBE", szLoad, MAX_STRING, INIFileName);
        sscanf_s(szLoad, "%06X", &CFG.AURA.Color);
        GetPrivateProfileString("OBJECT", "Spawn", CFG.OBJECT.Spawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.OBJECT.Spawn = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("OBJECT", "Despawn", CFG.OBJECT.Despawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.OBJECT.Despawn = (!_strnicmp(szLoad, "on", 3));
        CFG.OBJECT.MinLVL = GetPrivateProfileInt("OBJECT", "MinLVL", CFG.OBJECT.MinLVL, INIFileName);
        CFG.OBJECT.MaxLVL = GetPrivateProfileInt("OBJECT", "MaxLVL", CFG.OBJECT.MaxLVL, INIFileName);
        GetPrivateProfileString("OBJECT", "Color", "BEBEBE", szLoad, MAX_STRING, INIFileName);
        sscanf_s(szLoad, "%06X", &CFG.OBJECT.Color);
        GetPrivateProfileString("UNTARGETABLE", "Spawn",  CFG.UNTARGETABLE.Spawn   ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.UNTARGETABLE.Spawn = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("UNTARGETABLE", "Despawn", CFG.UNTARGETABLE.Despawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.UNTARGETABLE.Despawn = (!_strnicmp(szLoad, "on", 3));
        CFG.UNTARGETABLE.MinLVL = GetPrivateProfileInt("UNTARGETABLE", "MinLVL", CFG.UNTARGETABLE.MinLVL, INIFileName);
        CFG.UNTARGETABLE.MaxLVL = GetPrivateProfileInt("UNTARGETABLE", "MaxLVL", CFG.UNTARGETABLE.MaxLVL, INIFileName);
        GetPrivateProfileString("UNTARGETABLE", "Color",  "BEBEBE", szLoad, MAX_STRING, INIFileName);
        sscanf_s(szLoad, "%06X", &CFG.UNTARGETABLE.Color);
        GetPrivateProfileString("CHEST", "Spawn", CFG.CHEST.Spawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.CHEST.Spawn = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("CHEST", "Despawn", CFG.CHEST.Despawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.CHEST.Despawn = (!_strnicmp(szLoad, "on", 3));
        CFG.CHEST.MinLVL = GetPrivateProfileInt("CHEST", "MinLVL", CFG.CHEST.MinLVL, INIFileName);
        CFG.CHEST.MaxLVL = GetPrivateProfileInt("CHEST", "MaxLVL", CFG.CHEST.MaxLVL, INIFileName);
        GetPrivateProfileString("CHEST", "Color", "BEBEBE", szLoad, MAX_STRING, INIFileName);
        sscanf_s(szLoad, "%06X", &CFG.CHEST.Color);
        GetPrivateProfileString("TRAP", "Spawn", CFG.TRAP.Spawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.TRAP.Spawn = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("TRAP", "Despawn", CFG.TRAP.Despawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.TRAP.Despawn = (!_strnicmp(szLoad, "on", 3));
        CFG.TRAP.MinLVL = GetPrivateProfileInt("TRAP", "MinLVL", CFG.TRAP.MinLVL, INIFileName);
        CFG.TRAP.MaxLVL = GetPrivateProfileInt("TRAP", "MaxLVL", CFG.TRAP.MaxLVL, INIFileName);
        GetPrivateProfileString("TRAP", "Color", "BEBEBE", szLoad, MAX_STRING, INIFileName);
        sscanf_s(szLoad, "%06X", &CFG.TRAP.Color);
        GetPrivateProfileString("TIMER", "Spawn", CFG.TIMER.Spawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.TIMER.Spawn = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("TIMER", "Despawn", CFG.TIMER.Despawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.TIMER.Despawn = (!_strnicmp(szLoad, "on", 3));
        CFG.TIMER.MinLVL = GetPrivateProfileInt("TIMER", "MinLVL", CFG.TIMER.MinLVL, INIFileName);
        CFG.TIMER.MaxLVL = GetPrivateProfileInt("TIMER", "MaxLVL", CFG.TIMER.MaxLVL, INIFileName);
        GetPrivateProfileString("TIMER", "Color", "BEBEBE", szLoad, MAX_STRING, INIFileName);
        sscanf_s(szLoad, "%06X", &CFG.TIMER.Color);
        GetPrivateProfileString("TRIGGER", "Spawn", CFG.TRIGGER.Spawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.TRIGGER.Spawn = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("TRIGGER", "Despawn", CFG.TRIGGER.Despawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.TRIGGER.Despawn = (!_strnicmp(szLoad, "on", 3));
        CFG.TRIGGER.MinLVL = GetPrivateProfileInt("TRIGGER", "MinLVL", CFG.TRIGGER.MinLVL, INIFileName);
        CFG.TRIGGER.MaxLVL = GetPrivateProfileInt("TRIGGER", "MaxLVL", CFG.TRIGGER.MaxLVL, INIFileName);
        GetPrivateProfileString("TRIGGER", "Color", "BEBEBE", szLoad, MAX_STRING, INIFileName);
        sscanf_s(szLoad, "%06X", &CFG.TRIGGER.Color);
        GetPrivateProfileString("CORPSE", "Spawn", CFG.CORPSE.Spawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.CORPSE.Spawn = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("CORPSE", "Despawn", CFG.CORPSE.Despawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.CORPSE.Despawn = (!_strnicmp(szLoad, "on", 3));
        CFG.CORPSE.MinLVL = GetPrivateProfileInt("CORPSE", "MinLVL", CFG.CORPSE.MinLVL, INIFileName);
        CFG.CORPSE.MaxLVL = GetPrivateProfileInt("CORPSE", "MaxLVL", CFG.CORPSE.MaxLVL, INIFileName);
        GetPrivateProfileString("CORPSE", "Color", "006400", szLoad, MAX_STRING, INIFileName);
        sscanf_s(szLoad, "%06X", &CFG.CORPSE.Color);
        GetPrivateProfileString("ITEM", "Spawn", CFG.ITEM.Spawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.ITEM.Spawn = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("ITEM", "Despawn", CFG.ITEM.Despawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.ITEM.Despawn = (!_strnicmp(szLoad, "on", 3));
        CFG.ITEM.MinLVL = GetPrivateProfileInt("ITEM", "MinLVL", CFG.ITEM.MinLVL, INIFileName);
        CFG.ITEM.MaxLVL = GetPrivateProfileInt("ITEM", "MaxLVL", CFG.ITEM.MaxLVL, INIFileName);
        GetPrivateProfileString("ITEM", "Color", "BEBEBE", szLoad, MAX_STRING, INIFileName);
        sscanf_s(szLoad, "%06X", &CFG.ITEM.Color);
        GetPrivateProfileString("UNKNOWN", "Spawn", CFG.UNKNOWN.Spawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.UNKNOWN.Spawn = (!_strnicmp(szLoad, "on", 3));
        GetPrivateProfileString("UNKNOWN", "Despawn", CFG.UNKNOWN.Despawn ? "on" : "off", szLoad, MAX_STRING, INIFileName);
        CFG.UNKNOWN.Despawn = (!_strnicmp(szLoad, "on", 3));
        CFG.UNKNOWN.MinLVL = GetPrivateProfileInt("UNKNOWN", "MinLVL", CFG.UNKNOWN.MinLVL, INIFileName);
        CFG.UNKNOWN.MaxLVL = GetPrivateProfileInt("UNKNOWN", "MaxLVL", CFG.UNKNOWN.MaxLVL, INIFileName);
        GetPrivateProfileString("UNKNOWN", "Color", "FF69B4", szLoad, MAX_STRING, INIFileName);
        sscanf_s(szLoad, "%06X", &CFG.UNKNOWN.Color);

        // add custom exclude list
        char szOurKey[MAX_STRING]    = {0};
        char szOurFilter[MAX_STRING] = {0};
        int  iFilt                   = 1;

        SpawnFilters.clear();
        GetPrivateProfileString("Exclude", SafeItoa(iFilt, szOurKey, 10), NULL, szOurFilter, MAX_STRING, INIFileName);
        for (iFilt = 1; *szOurFilter; iFilt++)
        {
            if (*szOurFilter)
            {
                SpawnFilters.push_back(szOurFilter);
            }
            GetPrivateProfileString("Exclude", SafeItoa(iFilt+1, szOurKey, 10), NULL, szOurFilter, MAX_STRING, INIFileName);
        }
        // end custom exclude list addition

        KillOurWnd(false);
    }
    if (bLoaded) WriteChatf("\ay%s\aw:: Configuration file %s.", MODULE_NAME, bSave ? "saved" : "loaded");
}

void WatchState(bool bSpawns)
{
    char szOn[10] = "\agON\ax";
    char szOff[10] = "\arOFF\ax";
    WriteChatf("\ay%s\aw:: %s - AutoSave(%s) Loc(%s) SpawnID(%s) Timestamp(%s) ZoneDelay(\ag%u\ax)", MODULE_NAME, CFG.ON ? szOn : szOff, CFG.AutoSave ? szOn : szOff, CFG.Location ? szOn : szOff, CFG.SpawnID ? szOn : szOff, CFG.Timestamp ? szOn : szOff, CFG.Delay);
    if (bSpawns)
    {
        WriteChatf("\aw - \ayPC\ax: %s ( \ay%d\ax to \ay%d\ax ) - %06X", (CFG.PC.Despawn ? (CFG.PC.Spawn ? "Spawn & Despawn" : "Despawn Only") : (CFG.PC.Spawn ? "Spawn Only" : "OFF")), CFG.PC.MinLVL, CFG.PC.MaxLVL, CFG.PC.Color);
        WriteChatf("\aw - \ayNPC\ax: %s ( \ay%d\ax to \ay%d\ax ) - %06X", (CFG.NPC.Despawn ? (CFG.NPC.Spawn ? "Spawn & Despawn" : "Despawn Only") : (CFG.NPC.Spawn ? "Spawn Only" : "OFF")), CFG.NPC.MinLVL, CFG.NPC.MaxLVL, CFG.NPC.Color);
        WriteChatf("\aw - \ayMOUNT\ax: %s ( \ay%d\ax to \ay%d\ax ) - %06X", (CFG.MOUNT.Despawn ? (CFG.MOUNT.Spawn ? "Spawn & Despawn" : "Despawn Only") : (CFG.MOUNT.Spawn ? "Spawn Only" : "OFF")), CFG.MOUNT.MinLVL, CFG.MOUNT.MaxLVL, CFG.MOUNT.Color);
        WriteChatf("\aw - \ayPET\ax: %s ( \ay%d\ax to \ay%d\ax ) - %06X", (CFG.PET.Despawn ? (CFG.PET.Spawn ? "Spawn & Despawn" : "Despawn Only") : (CFG.PET.Spawn ? "Spawn Only" : "OFF")), CFG.PET.MinLVL, CFG.PET.MaxLVL, CFG.PET.Color);
        WriteChatf("\aw - \ayMERCENARY\ax: %s ( \ay%d\ax to \ay%d\ax ) - %06X", (CFG.MERCENARY.Despawn ? (CFG.MERCENARY.Spawn ? "Spawn & Despawn" : "Despawn Only") : (CFG.MERCENARY.Spawn ? "Spawn Only" : "OFF")), CFG.MERCENARY.MinLVL, CFG.MERCENARY.MaxLVL, CFG.MERCENARY.Color);
        WriteChatf("\aw - \ayCORPSE\ax: %s ( \ay%d\ax to \ay%d\ax ) - %06X", (CFG.CORPSE.Despawn ? (CFG.CORPSE.Spawn ? "Spawn & Despawn" : "Despawn Only") : (CFG.CORPSE.Spawn ? "Spawn Only" : "OFF")), CFG.CORPSE.MinLVL, CFG.CORPSE.MaxLVL, CFG.CORPSE.Color);
        WriteChatf("\aw - \ayFLYER\ax: %s ( \ay%d\ax to \ay%d\ax ) - %06X", (CFG.FLYER.Despawn ? (CFG.FLYER.Spawn ? "Spawn & Despawn" : "Despawn Only") : (CFG.FLYER.Spawn ? "Spawn Only" : "OFF")), CFG.FLYER.MinLVL, CFG.FLYER.MaxLVL, CFG.FLYER.Color);
        WriteChatf("\aw - \ayCAMPFIRE\ax: %s ( \ay%d\ax to \ay%d\ax ) - %06X", (CFG.CAMPFIRE.Despawn ? (CFG.CAMPFIRE.Spawn ? "Spawn & Despawn" : "Despawn Only") : (CFG.CAMPFIRE.Spawn ? "Spawn Only" : "OFF")), CFG.CAMPFIRE.MinLVL, CFG.CAMPFIRE.MaxLVL, CFG.CAMPFIRE.Color);
        WriteChatf("\aw - \ayBANNER\ax: %s ( \ay%d\ax to \ay%d\ax ) - %06X", (CFG.BANNER.Despawn ? (CFG.BANNER.Spawn ? "Spawn & Despawn" : "Despawn Only") : (CFG.BANNER.Spawn ? "Spawn Only" : "OFF")), CFG.BANNER.MinLVL, CFG.BANNER.MaxLVL, CFG.BANNER.Color);
        WriteChatf("\aw - \ayAURA\ax: %s ( \ay%d\ax to \ay%d\ax ) - %06X", (CFG.AURA.Despawn ? (CFG.AURA.Spawn ? "Spawn & Despawn" : "Despawn Only") : (CFG.AURA.Spawn ? "Spawn Only" : "OFF")), CFG.AURA.MinLVL, CFG.AURA.MaxLVL, CFG.AURA.Color);
        WriteChatf("\aw - \ayOBJECT\ax: %s ( \ay%d\ax to \ay%d\ax ) - %06X", (CFG.OBJECT.Despawn ? (CFG.OBJECT.Spawn ? "Spawn & Despawn" : "Despawn Only") : (CFG.OBJECT.Spawn ? "Spawn Only" : "OFF")), CFG.OBJECT.MinLVL, CFG.OBJECT.MaxLVL, CFG.OBJECT.Color);
        WriteChatf("\aw - \ayUNTARGETABLE\ax: %s ( \ay%d\ax to \ay%d\ax ) - %06X", (CFG.UNTARGETABLE.Despawn ? (CFG.UNTARGETABLE.Spawn ? "Spawn & Despawn" : "Despawn Only") : (CFG.UNTARGETABLE.Spawn ? "Spawn Only" : "OFF")), CFG.UNTARGETABLE.MinLVL, CFG.UNTARGETABLE.MaxLVL, CFG.UNTARGETABLE.Color);
        WriteChatf("\aw - \ayCHEST\ax: %s ( \ay%d\ax to \ay%d\ax ) - %06X", (CFG.CHEST.Despawn ? (CFG.CHEST.Spawn ? "Spawn & Despawn" : "Despawn Only") : (CFG.CHEST.Spawn ? "Spawn Only" : "OFF")), CFG.CHEST.MinLVL, CFG.CHEST.MaxLVL, CFG.CHEST.Color);
        WriteChatf("\aw - \ayTRAP\ax: %s ( \ay%d\ax to \ay%d\ax ) - %06X", (CFG.TRAP.Despawn ? (CFG.TRAP.Spawn ? "Spawn & Despawn" : "Despawn Only") : (CFG.TRAP.Spawn ? "Spawn Only" : "OFF")), CFG.TRAP.MinLVL, CFG.TRAP.MaxLVL, CFG.TRAP.Color);
        WriteChatf("\aw - \ayTIMER\ax: %s ( \ay%d\ax to \ay%d\ax ) - %06X", (CFG.TIMER.Despawn ? (CFG.TIMER.Spawn ? "Spawn & Despawn" : "Despawn Only") : (CFG.TIMER.Spawn ? "Spawn Only" : "OFF")), CFG.TIMER.MinLVL, CFG.TIMER.MaxLVL, CFG.TIMER.Color);
        WriteChatf("\aw - \ayTRIGGER\ax: %s ( \ay%d\ax to \ay%d\ax ) - %06X", (CFG.TRIGGER.Despawn ? (CFG.TRIGGER.Spawn ? "Spawn & Despawn" : "Despawn Only") : (CFG.TRIGGER.Spawn ? "Spawn Only" : "OFF")), CFG.TRIGGER.MinLVL, CFG.TRIGGER.MaxLVL, CFG.TRIGGER.Color);
        WriteChatf("\aw - \ayITEM\ax: %s ( \ay%d\ax to \ay%d\ax ) - %06X", (CFG.ITEM.Despawn ? (CFG.ITEM.Spawn ? "Spawn & Despawn" : "Despawn Only") : (CFG.ITEM.Spawn ? "Spawn Only" : "OFF")), CFG.ITEM.MinLVL, CFG.ITEM.MaxLVL, CFG.ITEM.Color);
        WriteChatf("\aw - \ayUNKNOWN\ax: %s ( \ay%d\ax to \ay%d\ax ) - %06X", (CFG.UNKNOWN.Despawn ? (CFG.UNKNOWN.Spawn ? "Spawn & Despawn" : "Despawn Only") : (CFG.UNKNOWN.Spawn ? "Spawn Only" : "OFF")), CFG.UNKNOWN.MinLVL, CFG.UNKNOWN.MaxLVL, CFG.UNKNOWN.Color);
        WriteChatf("\aw - \ayALL\ax: %s ( \ay%d\ax to \ay%d\ax ) - %06X", (CFG.ALL.Despawn ? (CFG.ALL.Spawn ? "Spawn & Despawn" : "Despawn Only") : (CFG.ALL.Spawn ? "Spawn Only" : "OFF")), CFG.ALL.MinLVL, CFG.ALL.MaxLVL, CFG.ALL.Color);
    }
    if (CFG.ALL.Spawn || CFG.ALL.Despawn) WriteChatf("\ay%s\aw:: \ayOVERRIDE\ax - Currently announcing \agALL\ax types: %s", MODULE_NAME, (CFG.ALL.Despawn ? (CFG.ALL.Spawn ? "Spawn & Despawn" : "Despawn Only") : "Spawn Only"));
}

void ToggleSetting(const char* pszToggleOutput, bool* pbSpawnTypeSet)
{
    char szTheMsg[MAX_STRING] = {0};
    *pbSpawnTypeSet = !*pbSpawnTypeSet;
    sprintf_s(szTheMsg, "\ay%s\aw:: %s %s", MODULE_NAME, *pbSpawnTypeSet ? "\agNow announcing\ax" : "\arNo longer announcing\ax", pszToggleOutput);
    WriteChatf(szTheMsg);
}

void SetSpawnColor(const char* pszSetOutput, unsigned long* pulSpawnTypeSet, unsigned long ulNewColor)
{
    char szTheMsg[MAX_STRING] = {0};
    unsigned long ulOldColor = *pulSpawnTypeSet;
    *pulSpawnTypeSet = ulNewColor;
    sprintf_s(szTheMsg, "\ay%s\aw:: %s color changed from \ay%06X\ax to \ag%06X\ax", MODULE_NAME, pszSetOutput, ulOldColor, ulNewColor);
    WriteChatf(szTheMsg);
}

void SetSpawnLevel(const char* pszSetOutput, int* piSpawnTypeSet, int iNewLevel)
{
    char szTheMsg[MAX_STRING] = {0};
    int iOldLevel = *piSpawnTypeSet;
    *piSpawnTypeSet = iNewLevel;
    sprintf_s(szTheMsg, "\ay%s\aw:: %s level changed from \ay%d\ax to \ag%d\ax", MODULE_NAME, pszSetOutput, iOldLevel, iNewLevel);
    WriteChatf(szTheMsg);
}

void WatchSpawns(PSPAWNINFO pLPlayer, char* szLine)
{
    int iNewLevel               = 0;
    unsigned long ulNewColor    = 0;
    unsigned char ucArgType     = 0;
    char szArg1[MAX_STRING]     = {0};
    char szArg2[MAX_STRING]     = {0};
    char szArg3[MAX_STRING]     = {0};
    GetArg(szArg1, szLine, 1);
    GetArg(szArg2, szLine, 2);
    GetArg(szArg3, szLine, 3);

    if (!*szArg1)
    {
        CFG.ON = !CFG.ON;
        WatchState(false);
        return;
    }

    // added custom exclude
    if (!_strnicmp(szArg1, "exclude", 8))
    {
        _strlwr_s(szArg2);
        SpawnFilters.push_back(szArg2);
        int iNewSize = SpawnFilters.size();
        char szTempFilt[10] = {0};
        WritePrivateProfileString("Exclude", SafeItoa(iNewSize, szTempFilt, 10), szArg2, INIFileName);
        WriteChatf("\ay%s\aw:: Added Exclude # \ay%d\ax: \ag%s", MODULE_NAME, iNewSize, szArg2);
        return;
    }
    // end of custom exclude addition

    if (*szArg2)
    {
        if (!_strnicmp(szArg2,      "spawn",    6))  ucArgType = ARG_SPAWN;
        else if (!_strnicmp(szArg2, "despawn",  8))  ucArgType = ARG_DESPAWN;
        else if (!_strnicmp(szArg2, "minlevel", 9))  ucArgType = ARG_MINLVL;
        else if (!_strnicmp(szArg2, "maxlevel", 9))  ucArgType = ARG_MAXLVL;
        else if (!_strnicmp(szArg2, "color",    6))  ucArgType = ARG_COLOR;
        if (_strnicmp(szArg1, "log", 3)) // weird log exclude, too lazy to clean all this shit up
        {
            if (*szArg3)
            {
                if (ucArgType == ARG_COLOR)
                {
                    sscanf_s(szArg3, "%06X", &ulNewColor);
                }
                else if (ucArgType == ARG_MINLVL || ucArgType == ARG_MAXLVL)
                {
                    char* pNotNum = NULL;
                    iNewLevel = strtoul(szArg3, &pNotNum, 10);
                    if (iNewLevel < 1 || iNewLevel > MAX_NPC_LEVEL || *pNotNum)
                    {
                        WriteChatf("\ay%s\aw:: \arLevel range must be between 1 and %u.", MODULE_NAME, MAX_NPC_LEVEL);
                        return;
                    }
                }
            }
            else
            {
                if (ucArgType == ARG_COLOR)
                {
                    WriteChatf("\ay%s\aw:: \arYou must provide a hexadecimal color code.", MODULE_NAME);
                    return;
                }
                else if (ucArgType == ARG_MINLVL || ucArgType == ARG_MAXLVL)
                {
                    WriteChatf("\ay%s\aw:: \arLevel range must be between 1 and %u.", MODULE_NAME, MAX_NPC_LEVEL);
                    return;
                }
            }
        }
    }

    if (!_strnicmp(szArg1, "on", 3))
    {
        CFG.ON = true;
        WatchState(false);
    }
    else if (!_strnicmp(szArg1, "off", 4))
    {
        CFG.ON = false;
        WatchState(false);
    }
    else if (!_strnicmp(szArg1, "loc", 4))
    {
        CFG.Location = !CFG.Location;
        WatchState(false);
    }
    else if (!_strnicmp(szArg1, "spawnid", 8))
    {
        CFG.SpawnID = !CFG.SpawnID;
        WatchState(false);
    }
    else if (!_strnicmp(szArg1, "timestamp", 10))
    {
        CFG.Timestamp = !CFG.Timestamp;
        WatchState(false);
    }
    else if (!_strnicmp(szArg1, "savebychar", 11))
    {
        CFG.SaveByChar = !CFG.SaveByChar;
        sprintf_s(szCharName, "%s.%s", EQADDR_SERVERNAME, ((PCHARINFO)pCharData)->Name);
        WatchState(false);
    }
    else if (!_strnicmp(szArg1, "autosave", 9))
    {
        CFG.AutoSave = !CFG.AutoSave;
        WatchState(false);
    }
    else if (!_strnicmp(szArg1, "min", 4))
    {
        if (OurWnd) ((CXWnd*)OurWnd)->OnMinimizeBox();
        return;
    }
    else if (!_strnicmp(szArg1, "clear", 6))
    {
        if (OurWnd) ((CChatWindow*)OurWnd)->Clear();
        return;
    }
    else if (!_strnicmp(szArg1, "load", 5))
    {
        HandleConfig(false);
        return;
    }
    else if (!_strnicmp(szArg1, "save", 5))
    {
        HandleConfig(true);
        return;
    }
    else if (!_strnicmp(szArg1, "status", 7))
    {
        WatchState(true);
        return;
    }
    else if (!_strnicmp(szArg1, "delay", 6))
    {
        char* pNotNum = NULL;
        int iValid = (int)strtoul(szArg2, &pNotNum, 10);
        if (iValid < 1 || *pNotNum)
        {
            WriteChatf("\ay%s\aw:: \arError\ax - Delay must be a positive numerical value.", MODULE_NAME);
            return;
        }
        CFG.Delay = iValid;
        WatchState(false);
    }
    else if (!_strnicmp(szArg1, "font", 5))
    {
        char* pNotNum = NULL;
        int iValid = (int)strtoul(szArg2, &pNotNum, 10);
        if (iValid < 1 || *pNotNum)
        {
            WriteChatf("\ay%s\aw:: \arError\ax - Font must be a positive numerical value.", MODULE_NAME);
            return;
        }
        if (OurWnd) OurWnd->SetFontSize(iValid);
        WatchState(false);
    }
    else if (!_strnicmp(szArg1, "help", 5))
    {
        WriteChatf("\ay%s\aw:: \ag/spawn\ax [ \agon\ax | \agoff\ax | \agspawnid\ax | \agloc\ax | \agtimestamp\ax ]", MODULE_NAME);
        WriteChatf("\ay%s\aw:: \ag/spawn\ax [ \agsave\ax | \agload\ax | \agsavebychar\ax | \agstatus\ax | \agautosave\ax | \aghelp\ax ]", MODULE_NAME);
        WriteChatf("\ay%s\aw:: \ag/spawn\ax [ \agclear\ax | \agmin\ax | \agfont #\ax ]", MODULE_NAME);
        WriteChatf("\ay%s\aw:: \ag/spawn\ax [ \agdelay #\ax ] - Sets zone time delay.", MODULE_NAME);
        WriteChatf("\ay%s\aw:: \ag/log\ax   [ \agon\ax | \agoff\ax | \agauto\ax | \agsetpath\ax <\aypath\ax> ]", MODULE_NAME);
        WriteChatf("\ay%s\aw:: \ag/spawn\ax [ \aytogglename\ax ] [ \agspawn\ax | \agdespawn\ax | \agminlevel #\ax | \agmaxlevel #\ax | \agcolor RRGGBB\ax ]", MODULE_NAME);
        WriteChatf("\ay%s\aw:: \agValid toggles:\ax all - pc - npc - mount - pet - merc - flyer - campfire - banner - aura - object - untargetable - chest - trap - timer - trigger - corpse - item - unknown", MODULE_NAME);
    }
    else if (!_strnicmp(szArg1, "all", 4))
    {
        switch(ucArgType)
        {
        case ARG_SPAWN:
            ToggleSetting("ALL TYPES spawn", &CFG.ALL.Spawn);
            break;
        case ARG_DESPAWN:
            ToggleSetting("ALL TYPES despawn", &CFG.ALL.Despawn);
            break;
        case ARG_MINLVL:
            SetSpawnLevel("ALL TYPES min level", &CFG.ALL.MinLVL, iNewLevel);
            break;
        case ARG_MAXLVL:
            SetSpawnLevel("ALL TYPES max level", &CFG.ALL.MaxLVL, iNewLevel);
            break;
        case ARG_COLOR:
            SetSpawnColor("ALL TYPES color", &CFG.ALL.Color, ulNewColor);
            break;
        default:
            WriteChatf("\ay%s\aw:: \arInvalid parameter. Use \ag/spawn help\ax.", MODULE_NAME);
        }
    }
    else if (!_strnicmp(szArg1, "pc", 3))
    {
        switch(ucArgType)
        {
        case ARG_SPAWN:
            ToggleSetting("PC spawn", &CFG.PC.Spawn);
            break;
        case ARG_DESPAWN:
            ToggleSetting("PC despawn", &CFG.PC.Despawn);
            break;
        case ARG_MINLVL:
            SetSpawnLevel("PC min level", &CFG.PC.MinLVL, iNewLevel);
            break;
        case ARG_MAXLVL:
            SetSpawnLevel("PC max level", &CFG.PC.MaxLVL, iNewLevel);
            break;
        case ARG_COLOR:
            SetSpawnColor("PC color", &CFG.PC.Color, ulNewColor);
            break;
        default:
            WriteChatf("\ay%s\aw:: \arInvalid parameter. Use \ag/spawn help\ax.", MODULE_NAME);
        }
    }
    else if (!_strnicmp(szArg1, "npc", 4))
    {
        switch(ucArgType)
        {
        case ARG_SPAWN:
            ToggleSetting("NPC spawn", &CFG.NPC.Spawn);
            break;
        case ARG_DESPAWN:
            ToggleSetting("NPC despawn", &CFG.NPC.Despawn);
            break;
        case ARG_MINLVL:
            SetSpawnLevel("NPC min level", &CFG.NPC.MinLVL, iNewLevel);
            break;
        case ARG_MAXLVL:
            SetSpawnLevel("NPC max level", &CFG.NPC.MaxLVL, iNewLevel);
            break;
        case ARG_COLOR:
            SetSpawnColor("NPC color", &CFG.NPC.Color, ulNewColor);
            break;
        default:
            WriteChatf("\ay%s\aw:: \arInvalid parameter. Use \ag/spawn help\ax.", MODULE_NAME);
        }
    }
    else if (!_strnicmp(szArg1, "mount", 6))
    {
        switch(ucArgType)
        {
        case ARG_SPAWN:
            ToggleSetting("MOUNT spawn", &CFG.MOUNT.Spawn);
            break;
        case ARG_DESPAWN:
            ToggleSetting("MOUNT despawn", &CFG.MOUNT.Despawn);
            break;
        case ARG_MINLVL:
            SetSpawnLevel("MOUNT min level", &CFG.MOUNT.MinLVL, iNewLevel);
            break;
        case ARG_MAXLVL:
            SetSpawnLevel("MOUNT max level", &CFG.MOUNT.MaxLVL, iNewLevel);
            break;
        case ARG_COLOR:
            SetSpawnColor("MOUNT color", &CFG.MOUNT.Color, ulNewColor);
            break;
        default:
            WriteChatf("\ay%s\aw:: \arInvalid parameter. Use \ag/spawn help\ax.", MODULE_NAME);
        }
    }
    else if (!_strnicmp(szArg1, "pet", 4))
    {
        switch(ucArgType)
        {
        case ARG_SPAWN:
            ToggleSetting("PET spawn", &CFG.PET.Spawn);
            break;
        case ARG_DESPAWN:
            ToggleSetting("PET despawn", &CFG.PET.Despawn);
            break;
        case ARG_MINLVL:
            SetSpawnLevel("PET min level", &CFG.PET.MinLVL, iNewLevel);
            break;
        case ARG_MAXLVL:
            SetSpawnLevel("PET max level", &CFG.PET.MaxLVL, iNewLevel);
            break;
        case ARG_COLOR:
            SetSpawnColor("PET color", &CFG.PET.Color, ulNewColor);
            break;
        default:
            WriteChatf("\ay%s\aw:: \arInvalid parameter. Use \ag/spawn help\ax.", MODULE_NAME);
        }
    }
    else if (!_strnicmp(szArg1, "merc", 5))
    {
        switch(ucArgType)
        {
        case ARG_SPAWN:
            ToggleSetting("MERCENARY spawn", &CFG.MERCENARY.Spawn);
            break;
        case ARG_DESPAWN:
            ToggleSetting("MERCENARY despawn", &CFG.MERCENARY.Despawn);
            break;
        case ARG_MINLVL:
            SetSpawnLevel("MERCENARY min level", &CFG.MERCENARY.MinLVL, iNewLevel);
            break;
        case ARG_MAXLVL:
            SetSpawnLevel("MERCENARY max level", &CFG.MERCENARY.MaxLVL, iNewLevel);
            break;
        case ARG_COLOR:
            SetSpawnColor("MERCENARY color", &CFG.MERCENARY.Color, ulNewColor);
            break;
        default:
            WriteChatf("\ay%s\aw:: \arInvalid parameter. Use \ag/spawn help\ax.", MODULE_NAME);
        }
    }
    else if (!_strnicmp(szArg1, "flyer", 6))
    {
        switch(ucArgType)
        {
        case ARG_SPAWN:
            ToggleSetting("FLYER spawn", &CFG.FLYER.Spawn);
            break;
        case ARG_DESPAWN:
            ToggleSetting("FLYER despawn", &CFG.FLYER.Despawn);
            break;
        case ARG_MINLVL:
            SetSpawnLevel("FLYER min level", &CFG.FLYER.MinLVL, iNewLevel);
            break;
        case ARG_MAXLVL:
            SetSpawnLevel("FLYER max level", &CFG.FLYER.MaxLVL, iNewLevel);
            break;
        case ARG_COLOR:
            SetSpawnColor("FLYER color", &CFG.FLYER.Color, ulNewColor);
            break;
        default:
            WriteChatf("\ay%s\aw:: \arInvalid parameter. Use \ag/spawn help\ax.", MODULE_NAME);
        }
    }
    else if (!_strnicmp(szArg1, "campfire", 9))
    {
        switch(ucArgType)
        {
        case ARG_SPAWN:
            ToggleSetting("CAMPFIRE spawn", &CFG.CAMPFIRE.Spawn);
            break;
        case ARG_DESPAWN:
            ToggleSetting("CAMPFIRE despawn", &CFG.CAMPFIRE.Despawn);
            break;
        case ARG_MINLVL:
            SetSpawnLevel("CAMPFIRE min level", &CFG.CAMPFIRE.MinLVL, iNewLevel);
            break;
        case ARG_MAXLVL:
            SetSpawnLevel("CAMPFIRE max level", &CFG.CAMPFIRE.MaxLVL, iNewLevel);
            break;
        case ARG_COLOR:
            SetSpawnColor("CAMPFIRE color", &CFG.CAMPFIRE.Color, ulNewColor);
            break;
        default:
            WriteChatf("\ay%s\aw:: \arInvalid parameter. Use \ag/spawn help\ax.", MODULE_NAME);
        }
    }
    else if (!_strnicmp(szArg1, "banner", 7))
    {
        switch(ucArgType)
        {
        case ARG_SPAWN:
            ToggleSetting("BANNER spawn", &CFG.BANNER.Spawn);
            break;
        case ARG_DESPAWN:
            ToggleSetting("BANNER despawn", &CFG.BANNER.Despawn);
            break;
        case ARG_MINLVL:
            SetSpawnLevel("BANNER min level", &CFG.BANNER.MinLVL, iNewLevel);
            break;
        case ARG_MAXLVL:
            SetSpawnLevel("BANNER max level", &CFG.BANNER.MaxLVL, iNewLevel);
            break;
        case ARG_COLOR:
            SetSpawnColor("BANNER color", &CFG.BANNER.Color, ulNewColor);
            break;
        default:
            WriteChatf("\ay%s\aw:: \arInvalid parameter. Use \ag/spawn help\ax.", MODULE_NAME);
        }
    }
    else if (!_strnicmp(szArg1, "aura", 5))
    {
        switch(ucArgType)
        {
        case ARG_SPAWN:
            ToggleSetting("AURA spawn", &CFG.AURA.Spawn);
            break;
        case ARG_DESPAWN:
            ToggleSetting("AURA despawn", &CFG.AURA.Despawn);
            break;
        case ARG_MINLVL:
            SetSpawnLevel("AURA min level", &CFG.AURA.MinLVL, iNewLevel);
            break;
        case ARG_MAXLVL:
            SetSpawnLevel("AURA max level", &CFG.AURA.MaxLVL, iNewLevel);
            break;
        case ARG_COLOR:
            SetSpawnColor("AURA color", &CFG.AURA.Color, ulNewColor);
            break;
        default:
            WriteChatf("\ay%s\aw:: \arInvalid parameter. Use \ag/spawn help\ax.", MODULE_NAME);
        }
    }
    else if (!_strnicmp(szArg1, "object", 7))
    {
        switch(ucArgType)
        {
        case ARG_SPAWN:
            ToggleSetting("OBJECT spawn", &CFG.OBJECT.Spawn);
            break;
        case ARG_DESPAWN:
            ToggleSetting("OBJECT despawn", &CFG.OBJECT.Despawn);
            break;
        case ARG_MINLVL:
            SetSpawnLevel("OBJECT min level", &CFG.OBJECT.MinLVL, iNewLevel);
            break;
        case ARG_MAXLVL:
            SetSpawnLevel("OBJECT max level", &CFG.OBJECT.MaxLVL, iNewLevel);
            break;
        case ARG_COLOR:
            SetSpawnColor("OBJECT color", &CFG.OBJECT.Color, ulNewColor);
            break;
        default:
            WriteChatf("\ay%s\aw:: \arInvalid parameter. Use \ag/spawn help\ax.", MODULE_NAME);
        }
    }
    else if (!_strnicmp(szArg1, "untargetable", 14))
    {
        switch(ucArgType)
        {
        case ARG_SPAWN:
            ToggleSetting("UNTARGETABLE spawn", &CFG.UNTARGETABLE.Spawn);
            break;
        case ARG_DESPAWN:
            ToggleSetting("UNTARGETABLE despawn", &CFG.UNTARGETABLE.Despawn);
            break;
        case ARG_MINLVL:
            SetSpawnLevel("UNTARGETABLE min level", &CFG.UNTARGETABLE.MinLVL, iNewLevel);
            break;
        case ARG_MAXLVL:
            SetSpawnLevel("UNTARGETABLE max level", &CFG.UNTARGETABLE.MaxLVL, iNewLevel);
            break;
        case ARG_COLOR:
            SetSpawnColor("UNTARGETABLE color", &CFG.UNTARGETABLE.Color, ulNewColor);
            break;
        default:
            WriteChatf("\ay%s\aw:: \arInvalid parameter. Use \ag/spawn help\ax.", MODULE_NAME);
        }
    }
    else if (!_strnicmp(szArg1, "chest", 6))
    {
        switch(ucArgType)
        {
        case ARG_SPAWN:
            ToggleSetting("CHEST spawn", &CFG.CHEST.Spawn);
            break;
        case ARG_DESPAWN:
            ToggleSetting("CHEST despawn", &CFG.CHEST.Despawn);
            break;
        case ARG_MINLVL:
            SetSpawnLevel("CHEST min level", &CFG.CHEST.MinLVL, iNewLevel);
            break;
        case ARG_MAXLVL:
            SetSpawnLevel("CHEST max level", &CFG.CHEST.MaxLVL, iNewLevel);
            break;
        case ARG_COLOR:
            SetSpawnColor("CHEST color", &CFG.CHEST.Color, ulNewColor);
            break;
        default:
            WriteChatf("\ay%s\aw:: \arInvalid parameter. Use \ag/spawn help\ax.", MODULE_NAME);
        }
    }
    else if (!_strnicmp(szArg1, "trap", 5))
    {
        switch(ucArgType)
        {
        case ARG_SPAWN:
            ToggleSetting("TRAP spawn", &CFG.TRAP.Spawn);
            break;
        case ARG_DESPAWN:
            ToggleSetting("TRAP despawn", &CFG.TRAP.Despawn);
            break;
        case ARG_MINLVL:
            SetSpawnLevel("TRAP min level", &CFG.TRAP.MinLVL, iNewLevel);
            break;
        case ARG_MAXLVL:
            SetSpawnLevel("TRAP max level", &CFG.TRAP.MaxLVL, iNewLevel);
            break;
        case ARG_COLOR:
            SetSpawnColor("TRAP color", &CFG.TRAP.Color, ulNewColor);
            break;
        default:
            WriteChatf("\ay%s\aw:: \arInvalid parameter. Use \ag/spawn help\ax.", MODULE_NAME);
        }
    }
    else if (!_strnicmp(szArg1, "timer", 6))
    {
        switch(ucArgType)
        {
        case ARG_SPAWN:
            ToggleSetting("TIMER spawn", &CFG.TIMER.Spawn);
            break;
        case ARG_DESPAWN:
            ToggleSetting("TIMER despawn", &CFG.TIMER.Despawn);
            break;
        case ARG_MINLVL:
            SetSpawnLevel("TIMER min level", &CFG.TIMER.MinLVL, iNewLevel);
            break;
        case ARG_MAXLVL:
            SetSpawnLevel("TIMER max level", &CFG.TIMER.MaxLVL, iNewLevel);
            break;
        case ARG_COLOR:
            SetSpawnColor("TIMER color", &CFG.TIMER.Color, ulNewColor);
            break;
        default:
            WriteChatf("\ay%s\aw:: \arInvalid parameter. Use \ag/spawn help\ax.", MODULE_NAME);
        }
    }
    else if (!_strnicmp(szArg1, "trigger", 8))
    {
        switch(ucArgType)
        {
        case ARG_SPAWN:
            ToggleSetting("TRIGGER spawn", &CFG.TRIGGER.Spawn);
            break;
        case ARG_DESPAWN:
            ToggleSetting("TRIGGER despawn", &CFG.TRIGGER.Despawn);
            break;
        case ARG_MINLVL:
            SetSpawnLevel("TRIGGER min level", &CFG.TRIGGER.MinLVL, iNewLevel);
            break;
        case ARG_MAXLVL:
            SetSpawnLevel("TRIGGER max level", &CFG.TRIGGER.MaxLVL, iNewLevel);
            break;
        case ARG_COLOR:
            SetSpawnColor("TRIGGER color", &CFG.TRIGGER.Color, ulNewColor);
            break;
        default:
            WriteChatf("\ay%s\aw:: \arInvalid parameter. Use \ag/spawn help\ax.", MODULE_NAME);
        }
    }
    else if (!_strnicmp(szArg1, "corpse", 7))
    {
        switch(ucArgType)
        {
        case ARG_SPAWN:
            ToggleSetting("CORPSE spawn", &CFG.CORPSE.Spawn);
            break;
        case ARG_DESPAWN:
            ToggleSetting("CORPSE despawn", &CFG.CORPSE.Despawn);
            break;
        case ARG_MINLVL:
            SetSpawnLevel("CORPSE min level", &CFG.CORPSE.MinLVL, iNewLevel);
            break;
        case ARG_MAXLVL:
            SetSpawnLevel("CORPSE max level", &CFG.CORPSE.MaxLVL, iNewLevel);
            break;
        case ARG_COLOR:
            SetSpawnColor("CORPSE color", &CFG.CORPSE.Color, ulNewColor);
            break;
        default:
            WriteChatf("\ay%s\aw:: \arInvalid parameter. Use \ag/spawn help\ax.", MODULE_NAME);
        }
    }
    else if (!_strnicmp(szArg1, "item", 5))
    {
        switch(ucArgType)
        {
        case ARG_SPAWN:
            ToggleSetting("ITEM spawn", &CFG.ITEM.Spawn);
            break;
        case ARG_DESPAWN:
            ToggleSetting("ITEM despawn", &CFG.ITEM.Despawn);
            break;
        case ARG_MINLVL:
            SetSpawnLevel("ITEM min level", &CFG.ITEM.MinLVL, iNewLevel);
            break;
        case ARG_MAXLVL:
            SetSpawnLevel("ITEM max level", &CFG.ITEM.MaxLVL, iNewLevel);
            break;
        case ARG_COLOR:
            SetSpawnColor("ITEM color", &CFG.ITEM.Color, ulNewColor);
            break;
        default:
            WriteChatf("\ay%s\aw:: \arInvalid parameter. Use \ag/spawn help\ax.", MODULE_NAME);
        }
    }
    else if (!_strnicmp(szArg1, "unknown", 7))
    {
        switch(ucArgType)
        {
        case ARG_SPAWN:
            ToggleSetting("UNKNOWN spawn", &CFG.UNKNOWN.Spawn);
            break;
        case ARG_DESPAWN:
            ToggleSetting("UNKNOWN despawn", &CFG.UNKNOWN.Despawn);
            break;
        case ARG_MINLVL:
            SetSpawnLevel("UNKNOWN min level", &CFG.UNKNOWN.MinLVL, iNewLevel);
            break;
        case ARG_MAXLVL:
            SetSpawnLevel("UNKNOWN max level", &CFG.UNKNOWN.MaxLVL, iNewLevel);
            break;
        case ARG_COLOR:
            SetSpawnColor("UNKNOWN color", &CFG.UNKNOWN.Color, ulNewColor);
            break;
        default:
            WriteChatf("\ay%s\aw:: \arInvalid parameter. Use \ag/spawn help\ax.", MODULE_NAME);
        }
    }
    // added logging 2010-09-04
    else if (!_strnicmp(szArg1, "log", 4))
    {
        if (!*szArg2)
        {
            WriteChatf("\ay%s\aw:: %s - Usage - \aglog\aw [ \ayon\ax | \ayoff\ax | \ayauto\ax | \aysetpath\ax <\aypath\ax> ]", MODULE_NAME);
            return;
        }

        if (!_strnicmp(szArg2, "on", 3))
        {
            if (bLogActive)
            {
                WriteChatf("\ay%s\aw:: \arLogging already active\aw.", MODULE_NAME);
                return;
            }

            bLogCmdUsed = bLogActive = true;
            if (StartLog())
            {
                WriteChatf("\ay%s\aw:: Logging \agENABLED\aw.", MODULE_NAME);
            }
            else
            {
                WriteChatf("\ay%s\aw:: Logging \arDISABLED\aw.", MODULE_NAME);
            }
        }
        else if (!_strnicmp(szArg2, "off", 4))
        {
            if (!bLogActive)
            {
                WriteChatf("\ay%s\aw:: \arLogging not currently active\aw.", MODULE_NAME);
                return;
            }
            EndLog();
            bLogCmdUsed = bLogActive = bLogReady = false;
            WriteChatf("\ay%s\aw:: Logging \arDISABLED\aw.", MODULE_NAME);
        }
        else if (!_strnicmp(szArg2, "auto", 5))
        {
            CFG.Logging = !CFG.Logging;
            WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Settings", "Logging", CFG.Logging ? "on" : "off", INIFileName);
            WriteChatf("\ay%s\aw:: Automatic logging %s", MODULE_NAME, CFG.Logging ? "\agENABLED" : "\arDISABLED");
        }
        else if (!_strnicmp(szArg2, "setpath", 8))
        {
            if (!*szArg3)
            {
                WriteChatf("\ay%s\aw:: \arYou must specify a path\aw.", MODULE_NAME);
                return;
            }

            if (bLogActive)
            {
                EndLog();
                strcpy_s(szDirPath, szArg3);
                sprintf_s(szLogPath, "%s\\%s", szDirPath, szLogFile);
                bLogActive = true;
                StartLog();
            }
            else
            {
                strcpy_s(szDirPath, szArg3);
                sprintf_s(szLogPath, "%s\\%s", szDirPath, szLogFile);
            }
            WritePrivateProfileString(CFG.SaveByChar ? szCharName : "Settings", "LogPath", szDirPath, INIFileName);
            WriteChatf("\ay%s\aw:: Log folder path set to \ag%s", MODULE_NAME, szDirPath);
        }
    }
    // end of logging
    else
    {
        WriteChatf("\ay%s\aw:: Invalid parameter. Use \ag/spawn help\ax for valid options.", MODULE_NAME);
        return;
    }

    if (CFG.AutoSave) HandleConfig(true);
}

// Wrappers to promote laziness
void ToggleSpawns(PSPAWNINFO pLPlayer, char* szLine)
{
    char szArg1[MAX_STRING] = {0};
    GetArg(szArg1, szLine, 1);

    if (!_strnicmp(szArg1, "all", 4))
    {
        WatchSpawns(pLPlayer, "all spawn");
    }
    else if (!_strnicmp(szArg1, "pc", 3))
    {
        WatchSpawns(pLPlayer, "pc spawn");
    }
    else if (!_strnicmp(szArg1, "npc", 4))
    {
        WatchSpawns(pLPlayer, "npc spawn");
    }
    else if (!_strnicmp(szArg1, "mount", 6))
    {
        WatchSpawns(pLPlayer, "mount spawn");
    }
    else if (!_strnicmp(szArg1, "pet", 4))
    {
        WatchSpawns(pLPlayer, "pet spawn");
    }
    else if (!_strnicmp(szArg1, "merc", 5))
    {
        WatchSpawns(pLPlayer, "merc spawn");
    }
    else if (!_strnicmp(szArg1, "flyer", 6))
    {
        WatchSpawns(pLPlayer, "flyer spawn");
    }
    else if (!_strnicmp(szArg1, "campfire", 9))
    {
        WatchSpawns(pLPlayer, "campfire spawn");
    }
    else if (!_strnicmp(szArg1, "banner", 7))
    {
        WatchSpawns(pLPlayer, "banner spawn");
    }
    else if (!_strnicmp(szArg1, "aura", 5))
    {
        WatchSpawns(pLPlayer, "aura spawn");
    }
    else if (!_strnicmp(szArg1, "object", 7))
    {
        WatchSpawns(pLPlayer, "object spawn");
    }
    else if (!_strnicmp(szArg1, "untargetable", 14))
    {
        WatchSpawns(pLPlayer, "untargetable spawn");
    }
    else if (!_strnicmp(szArg1, "chest", 6))
    {
        WatchSpawns(pLPlayer, "chest spawn");
    }
    else if (!_strnicmp(szArg1, "trap", 5))
    {
        WatchSpawns(pLPlayer, "trap spawn");
    }
    else if (!_strnicmp(szArg1, "timer", 6))
    {
        WatchSpawns(pLPlayer, "timer spawn");
    }
    else if (!_strnicmp(szArg1, "trigger", 8))
    {
        WatchSpawns(pLPlayer, "trigger spawn");
    }
    else if (!_strnicmp(szArg1, "corpse", 7))
    {
        WatchSpawns(pLPlayer, "corpse spawn");
    }
    else if (!_strnicmp(szArg1, "item", 5))
    {
        WatchSpawns(pLPlayer, "item spawn");
    }
    else if (!_strnicmp(szArg1, "unknown", 7))
    {
        WatchSpawns(pLPlayer, "unknown spawn");
    }
    else
    {
        WriteChatf("\ay%s\aw:: Invalid parameter. Use \ag/spawn help\ax for valid toggles.", MODULE_NAME);
    }
}

void ToggleDespawns(PSPAWNINFO pLPlayer, char* szLine)
{
    char szArg1[MAX_STRING] = {0};
    GetArg(szArg1, szLine, 1);

    if (!_strnicmp(szArg1, "all", 4))
    {
        WatchSpawns(pLPlayer, "all despawn");
    }
    else if (!_strnicmp(szArg1, "pc", 3))
    {
        WatchSpawns(pLPlayer, "pc despawn");
    }
    else if (!_strnicmp(szArg1, "npc", 4))
    {
        WatchSpawns(pLPlayer, "npc despawn");
    }
    else if (!_strnicmp(szArg1, "mount", 6))
    {
        WatchSpawns(pLPlayer, "mount despawn");
    }
    else if (!_strnicmp(szArg1, "pet", 4))
    {
        WatchSpawns(pLPlayer, "pet despawn");
    }
    else if (!_strnicmp(szArg1, "merc", 5))
    {
        WatchSpawns(pLPlayer, "merc despawn");
    }
    else if (!_strnicmp(szArg1, "flyer", 6))
    {
        WatchSpawns(pLPlayer, "flyer despawn");
    }
    else if (!_strnicmp(szArg1, "campfire", 9))
    {
        WatchSpawns(pLPlayer, "campfire despawn");
    }
    else if (!_strnicmp(szArg1, "banner", 7))
    {
        WatchSpawns(pLPlayer, "banner despawn");
    }
    else if (!_strnicmp(szArg1, "aura", 5))
    {
        WatchSpawns(pLPlayer, "aura despawn");
    }
    else if (!_strnicmp(szArg1, "object", 7))
    {
        WatchSpawns(pLPlayer, "object despawn");
    }
    else if (!_strnicmp(szArg1, "untargetable", 14))
    {
        WatchSpawns(pLPlayer, "untargetable despawn");
    }
    else if (!_strnicmp(szArg1, "chest", 6))
    {
        WatchSpawns(pLPlayer, "chest despawn");
    }
    else if (!_strnicmp(szArg1, "trap", 5))
    {
        WatchSpawns(pLPlayer, "trap despawn");
    }
    else if (!_strnicmp(szArg1, "timer", 6))
    {
        WatchSpawns(pLPlayer, "timer despawn");
    }
    else if (!_strnicmp(szArg1, "trigger", 8))
    {
        WatchSpawns(pLPlayer, "trigger despawn");
    }
    else if (!_strnicmp(szArg1, "corpse", 7))
    {
        WatchSpawns(pLPlayer, "corpse despawn");
    }
    else if (!_strnicmp(szArg1, "item", 5))
    {
        WatchSpawns(pLPlayer, "item despawn");
    }
    else if (!_strnicmp(szArg1, "unknown", 7))
    {
        WatchSpawns(pLPlayer, "unknown despawn");
    }
    else
    {
        WriteChatf("\ay%s\aw:: Invalid parameter. Use \ag/spawn help\ax for valid toggles.", MODULE_NAME);
    }
}

void WriteSpawn(PSPAWNINFO pFormatSpawn, char* szTypeString, char* szLocString, bool bSpawn)
{
    if (!OurWnd) return;

    char szFinalOutput[MAX_STRING] = {0};
    char szColoredSpawn[MAX_STRING] = {0};
    char szStringStart[MAX_STRING] = {0};
    char szSpawnID[MAX_STRING] = {0};
    char szTime[30] = {0};
    char szProcessTemp[MAX_STRING] = {0};
    char szProcessTemp2[MAX_STRING] = {0};

    bool bScrollDown = (OurWnd->OutWnd->GetVScrollPos() == OurWnd->OutWnd->GetVScrollMax()) ? true : false;
    unsigned long ulColor = SpawnFormat->Color;

    //sprintf_s(szColoredSpawn, "[<c \"#%06X\"> %d %s %s</c> ] <<c \"#%06X\"> %s </c>> (<c \"#%06X\">%s</c>)", ulColor, pFormatSpawn->Level, pEverQuest->GetRaceDesc(pFormatSpawn->mActorClient.Race), GetClassDesc(pFormatSpawn->mActorClient.Class), ulColor, pFormatSpawn->DisplayedName, ulColor, szTypeString);
    MQToSTML("[ ", szProcessTemp);
    strcat_s(szColoredSpawn, szProcessTemp);
    sprintf_s(szProcessTemp, "%d %s %s", pFormatSpawn->Level, pEverQuest->GetRaceDesc(pFormatSpawn->mActorClient.Race), GetClassDesc(pFormatSpawn->mActorClient.Class));
    MQToSTML(szProcessTemp, szProcessTemp2, MAX_STRING, ulColor);
    strcat_s(szColoredSpawn, szProcessTemp2);
    MQToSTML(" ] < ", szProcessTemp);
    strcat_s(szColoredSpawn, szProcessTemp);
    sprintf_s(szProcessTemp, "%s", pFormatSpawn->DisplayedName);
    MQToSTML(szProcessTemp, szProcessTemp2, MAX_STRING, ulColor);
    strcat_s(szColoredSpawn, szProcessTemp2);
    MQToSTML(" > (", szProcessTemp);
    strcat_s(szColoredSpawn, szProcessTemp);
    MQToSTML(szTypeString, szProcessTemp, MAX_STRING, ulColor);
    strcat_s(szColoredSpawn, szProcessTemp);
    MQToSTML(")", szProcessTemp);
    strcat_s(szColoredSpawn, szProcessTemp);

    // added logging function 2010-09-04
    if (bLogActive && bLogReady)
    {
        char szLogOut[MAX_STRING] = {0};
        sprintf_s(szLogOut, "%s: [ %d %s %s ] < %s > ( %s ) ( id %d ) %s", bSpawn ? "SPAWNED" : "DESPAWN", pFormatSpawn->Level, pEverQuest->GetRaceDesc(pFormatSpawn->mActorClient.Race), GetClassDesc(pFormatSpawn->mActorClient.Class), pFormatSpawn->DisplayedName, szTypeString, pFormatSpawn->SpawnID, szLocString);
        WriteToLog(szLogOut);
    }
    // end of logging function

    if (bSpawn)
    {
        //sprintf_s(szStringStart, "<c \"#FF0000\">SPAWNED:</c>"); // red
        sprintf_s(szProcessTemp, "SPAWNED: "); // red
        MQToSTML(szProcessTemp, szStringStart, MAX_STRING, 0xFF0000);
    }
    else
    {
        //sprintf_s(szStringStart, "<c \"#00FF00\">DESPAWN:</c>");
        sprintf_s(szProcessTemp, "DESPAWN: ");
        MQToSTML(szProcessTemp, szStringStart, MAX_STRING, 0x00FF00); // green
    }

    if (CFG.Timestamp)
    {
		tm THE_TIME = { 0 };
        _time64(&tCurrentTime);
        _localtime64_s(&THE_TIME,&tCurrentTime);
        strftime(szTime, 20, "[%H:%M:%S]  ", &THE_TIME);
        MQToSTML(szTime, szFinalOutput);
        strcat_s(szFinalOutput, szStringStart);
    }
    else
    {
        strcpy_s(szFinalOutput, szStringStart);
    }

    strcat_s(szFinalOutput, szColoredSpawn);

    if (CFG.SpawnID)
    {
        //sprintf_s(szSpawnID, " (<c \"#FFFF00\">id %d</c>)", pFormatSpawn->SpawnID); // yellow
        MQToSTML(" (", szProcessTemp);
        strcat_s(szFinalOutput, szProcessTemp);
        sprintf_s(szProcessTemp, "id %d", pFormatSpawn->SpawnID);
        MQToSTML(szProcessTemp, szSpawnID, MAX_STRING, ulColor);
        strcat_s(szFinalOutput, szSpawnID);
        MQToSTML(")", szProcessTemp);
        strcat_s(szFinalOutput, szProcessTemp);
    }

    if (CFG.Location)
    {
        strcpy_s(szProcessTemp, szLocString);
        MQToSTML(szProcessTemp, szLocString);
        strcat_s(szFinalOutput, szLocString);
    }


    strcat_s(szFinalOutput,"<br>");
    ((CXWnd*)OurWnd)->Show(1, 1);
    CXStr NewText(szFinalOutput);
    (OurWnd->StmlOut)->AppendSTML(NewText);
    if (bScrollDown)
    {
        (OurWnd->OutWnd)->SetVScrollPos(OurWnd->OutStruct->GetVScrollMax());
    }
}

CWatchType* IsTypeOn(CWatchType* pCheck, bool bSpawn)
{
    bool bAllCheck = (bSpawn ? CFG.ALL.Spawn : CFG.ALL.Despawn);
    if ((bSpawn && !pCheck->Spawn) || (!bSpawn && !pCheck->Despawn))
    {
        if (bAllCheck)
        {
            return &CFG.ALL;
        }
        return NULL;
    }
    return pCheck;
}

void CheckOurType(PSPAWNINFO pNewSpawn, bool bSpawn)
{
    char szType[MAX_STRING]     = {0};
    char szSpawnLoc[MAX_STRING] = {0};

    // add exclude list
    if (pNewSpawn->DisplayedName[0])
    {
        for (vector<string>::iterator it = SpawnFilters.begin() ; it != SpawnFilters.end(); it++)
        {
            string sFilter = *it;
            char szTempName[MAX_STRING] = {0};
            strcpy_s(szTempName, pNewSpawn->DisplayedName);
            _strlwr_s(szTempName);
            if (strstr(szTempName, sFilter.c_str()))
            {
                return;
            }
        }
    }
    // end of exclude list addition

    switch(GetSpawnType(pNewSpawn))
    {
    case PC:
        SpawnFormat = IsTypeOn(&CFG.PC, bSpawn);
        if (!SpawnFormat) return;
        sprintf_s(szType, "PC");
        break;
    case MOUNT:
        SpawnFormat = IsTypeOn(&CFG.MOUNT, bSpawn);
        if (!SpawnFormat) return;
        sprintf_s(szType, "MOUNT");
        break;
    case PET:
        SpawnFormat = IsTypeOn(&CFG.PET, bSpawn);
        if (!SpawnFormat) return;
        sprintf_s(szType, "PET");
        break;
    case MERCENARY:
        SpawnFormat = IsTypeOn(&CFG.MERCENARY, bSpawn);
        if (!SpawnFormat) return;
        sprintf_s(szType, "MERCENARY");
        break;
    case FLYER:
        SpawnFormat = IsTypeOn(&CFG.FLYER, bSpawn);
        if (!SpawnFormat) return;
        sprintf_s(szSpawnLoc, "INVALID");
        sprintf_s(szType, "FLYER");
        break;
    case NPC:
        SpawnFormat = IsTypeOn(&CFG.NPC, bSpawn);
        if (!SpawnFormat) return;
        sprintf_s(szType, "NPC");
        break;
    case CAMPFIRE:
        SpawnFormat = IsTypeOn(&CFG.CAMPFIRE, bSpawn);
        if (!SpawnFormat) return;
        sprintf_s(szType, "CAMPFIRE");
        break;
    case BANNER:
        SpawnFormat = IsTypeOn(&CFG.BANNER, bSpawn);
        if (!SpawnFormat) return;
        sprintf_s(szType, "GUILDBANNER");
        break;
    case AURA:
        SpawnFormat = IsTypeOn(&CFG.AURA, bSpawn);
        if (!SpawnFormat) return;
        sprintf_s(szType, "AURA");
        break;
    case OBJECT:
        SpawnFormat = IsTypeOn(&CFG.OBJECT, bSpawn);
        if (!SpawnFormat) return;
        sprintf_s(szType, "OBJECT");
        break;
    case UNTARGETABLE:
        SpawnFormat = IsTypeOn(&CFG.UNTARGETABLE, bSpawn);
        if (!SpawnFormat) return;
        sprintf_s(szType, "UNTARGETABLE");
        break;
    case CHEST:
        SpawnFormat = IsTypeOn(&CFG.CHEST, bSpawn);
        if (!SpawnFormat) return;
        sprintf_s(szType, "CHEST");
        break;
    case TRAP:
        SpawnFormat = IsTypeOn(&CFG.TRAP, bSpawn);
        if (!SpawnFormat) return;
        sprintf_s(szType, "TRAP");
        break;
    case TIMER:
        SpawnFormat = IsTypeOn(&CFG.TIMER, bSpawn);
        if (!SpawnFormat) return;
        sprintf_s(szType, "TIMER");
        break;
    case TRIGGER:
        SpawnFormat = IsTypeOn(&CFG.TRIGGER, bSpawn);
        if (!SpawnFormat) return;
        sprintf_s(szType, "TRIGGER");
        break;
    case CORPSE:
        SpawnFormat = IsTypeOn(&CFG.CORPSE, bSpawn);
        if (!SpawnFormat) return;
        sprintf_s(szType, "CORPSE");
        break;
    case ITEM:
        SpawnFormat = IsTypeOn(&CFG.ITEM, bSpawn);
        if (!SpawnFormat) return;
        sprintf_s(szType, "ITEM");
        break;
    default:
        SpawnFormat = IsTypeOn(&CFG.UNKNOWN, bSpawn);
        if (!SpawnFormat) return;
        sprintf_s(szSpawnLoc, "UNKNOWN");
        sprintf_s(szType, "UNKNOWN");
        break;
    }

    // level-based filtering done here
    if (pNewSpawn->Level < SpawnFormat->MinLVL || pNewSpawn->Level > SpawnFormat->MaxLVL)
    {
        //DebugSpew("MQ2Spawns:: Filtered %s, %d", pNewSpawn->DisplayedName, pNewSpawn->Level);
        return;
    }
    // if we did not put INVALID or UNKNOWN above
    if (!*szSpawnLoc) sprintf_s(szSpawnLoc, " @ %.2f, %.2f, %.2f", pNewSpawn->Y, pNewSpawn->X, pNewSpawn->Z);

    WriteSpawn(pNewSpawn, szType, szSpawnLoc, bSpawn);
}

PLUGIN_API void OnAddSpawn(PSPAWNINFO pNewSpawn)
{
    if (GetGameState() == GAMESTATE_INGAME && !bZoning && CFG.ON && time(NULL) > tSeconds + CFG.Delay && pNewSpawn->SpawnID)
    {
        CheckOurType(pNewSpawn, true);
    }
}

PLUGIN_API void OnRemoveSpawn(PSPAWNINFO pOldSpawn)
{
    if (GetGameState() == GAMESTATE_INGAME && !bZoning && CFG.ON && time(NULL) > tSeconds + CFG.Delay && pOldSpawn->SpawnID)
    {
        CheckOurType(pOldSpawn, false);
    }
}

PLUGIN_API void OnBeginZone()
{
    bZoning = true;
}

PLUGIN_API void SetGameState(unsigned long ulGameState)
{
    ulGameState = GetGameState();

    if (ulGameState == GAMESTATE_INGAME)
    {
        sprintf_s(szLogFile, "MQ2Spawns-%s.log", GetShortZone(((PSPAWNINFO)pLocalPlayer)->GetZoneID()));
        sprintf_s(szCharName, "%s.%s", EQADDR_SERVERNAME, ((PCHARINFO)pCharData)->Name);
        sprintf_s(szLogPath, "%s\\%s", szDirPath, szLogFile);
        CreateOurWnd();
        tSeconds = time(NULL);
        bZoning = false;

        if (CFG.Logging || bLogCmdUsed) bLogActive = true;

        if (bLogActive)
        {
            StartLog();
        }
    }
    else
    {
        bZoning = true;
        if (ulGameState == GAMESTATE_CHARSELECT)
        {
            KillOurWnd(true);
        }
        else if (ulGameState == GAMESTATE_PRECHARSELECT)
        {
            KillOurWnd(false);
        }
        if (bLogActive) EndLog();
    }
}

PLUGIN_API void OnReloadUI()
{
    if (GetGameState() == GAMESTATE_INGAME)
    {
        sprintf_s(szCharName, "%s.%s", EQADDR_SERVERNAME, ((PCHARINFO)pCharData)->Name);
        CreateOurWnd();
    }
}

PLUGIN_API void OnCleanUI()
{
    KillOurWnd(true);
}

PLUGIN_API void OnPulse()
{
    if(InHoverState() && OurWnd)
    {
        ((CXWnd*)OurWnd)->DoAllDrawing();
    }
}

PLUGIN_API void InitializePlugin()
{
    AddCommand("/spawn", WatchSpawns);
    AddCommand("/spwn",  ToggleSpawns);
    AddCommand("/dspwn", ToggleDespawns);
    tSeconds = time(NULL);
    HandleConfig(false);
    bLoaded = true;
}

PLUGIN_API void ShutdownPlugin()
{
    RemoveCommand("/spawn");
    RemoveCommand("/spwn");
    RemoveCommand("/dspwn");
    if (bLogActive) EndLog();
    HandleConfig(true);
    KillOurWnd(false);
}
