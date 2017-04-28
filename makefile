!include "../global.mak"

ALL : "$(OUTDIR)\MQ2Spawns.dll"

CLEAN :
	-@erase "$(INTDIR)\MQ2Spawns.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\MQ2Spawns.dll"
	-@erase "$(OUTDIR)\MQ2Spawns.exp"
	-@erase "$(OUTDIR)\MQ2Spawns.lib"
	-@erase "$(OUTDIR)\MQ2Spawns.pdb"


LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib $(DETLIB) ..\Release\MQ2Main.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\MQ2Spawns.pdb" /debug /machine:I386 /out:"$(OUTDIR)\MQ2Spawns.dll" /implib:"$(OUTDIR)\MQ2Spawns.lib" /OPT:NOICF /OPT:NOREF 
LINK32_OBJS= \
	"$(INTDIR)\MQ2Spawns.obj" \
	"$(OUTDIR)\MQ2Main.lib"

"$(OUTDIR)\MQ2Spawns.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) $(LINK32_FLAGS) $(LINK32_OBJS)


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("MQ2Spawns.dep")
!INCLUDE "MQ2Spawns.dep"
!ELSE 
!MESSAGE Warning: cannot find "MQ2Spawns.dep"
!ENDIF 
!ENDIF 


SOURCE=.\MQ2Spawns.cpp

"$(INTDIR)\MQ2Spawns.obj" : $(SOURCE) "$(INTDIR)"

