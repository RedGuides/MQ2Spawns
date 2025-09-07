---
tags:
  - plugin
resource_link: "https://www.redguides.com/community/resources/mq2spawns.184/"
support_link: "https://www.redguides.com/community/threads/mq2spawns.66886/"
repository: "https://github.com/RedGuides/MQ2Spawns"
config: "MQ2Spawns.ini"
authors: "pms, eqmule, ChatWithThisName"
tagline: "Announce all spawns and despawns in a dedicated window"
---

# MQ2Spawns
!!! note "Not to be confused with the core MacroQuest MQ2Spawns, which handles /caption etc." 
<!--desc-start-->
This announces all spawns and despawns in your current zone to a dedicated window.
<!--desc-end-->
There are great spawn tracking plugins like [MQ2SpawnMaster](../mq2spawnmaster/index.md) that offer named mob announcements, custom spawn search arguments, and many more features. This is not intended to be any sort of replacement for that, just real-time tracking of desired spawn types.


## Commands

<a href="cmd-dspwn/">
{% 
  include-markdown "projects/mq2spawns/cmd-dspwn.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2spawns/cmd-dspwn.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2spawns/cmd-dspwn.md') }}

<a href="cmd-spawn/">
{% 
  include-markdown "projects/mq2spawns/cmd-spawn.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2spawns/cmd-spawn.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2spawns/cmd-spawn.md') }}

<a href="cmd-spwn/">
{% 
  include-markdown "projects/mq2spawns/cmd-spwn.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2spawns/cmd-spwn.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2spawns/cmd-spwn.md') }}

## Settings

Example MQ2Spawns.ini,

```ini
[Window]
ChatTop=10
ChatBottom=30
ChatLeft=10
ChatRight=140
Fades=0
Alpha=255
FadeToAlpha=255
Duration=500
Locked=0
Delay=2000
BGType=1
BGTint.red=255
BGTint.green=255
BGTint.blue=255
FontSize=2
WindowTitle=Spawns
BGTint.alpha=255
[Settings]
AutoSave=off
Delay=10
Enabled=on
SaveByChar=off
ShowLoc=off
SpawnID=off
Timestamp=on
Logging=off
LogPath=C:\Users\Redbot\Games\macroquest
[ALL]
Spawn=off
Despawn=off
MinLVL=1
MaxLVL=200
Color=FFFF00
[PC]
Spawn=on
Despawn=off
MinLVL=1
MaxLVL=200
Color=00FF00
[NPC]
Spawn=on
Despawn=off
MinLVL=1
MaxLVL=200
Color=FF0000
[MOUNT]
Spawn=off
Despawn=off
MinLVL=1
MaxLVL=200
Color=BEBEBE
[PET]
Spawn=off
Despawn=off
MinLVL=1
MaxLVL=200
Color=6B8E23
[MERCENARY]
Spawn=off
Despawn=off
MinLVL=1
MaxLVL=200
Color=FFDEAD
[FLYER]
Spawn=off
Despawn=off
MinLVL=1
MaxLVL=200
Color=BEBEBE
[CAMPFIRE]
Spawn=off
Despawn=off
MinLVL=1
MaxLVL=200
Color=BEBEBE
[BANNER]
Spawn=off
Despawn=off
MinLVL=1
MaxLVL=200
Color=BEBEBE
[AURA]
Spawn=off
Despawn=off
MinLVL=1
MaxLVL=200
Color=BEBEBE
[OBJECT]
Spawn=off
Despawn=off
MinLVL=1
MaxLVL=200
Color=BEBEBE
[UNTARGETABLE]
Spawn=off
Despawn=off
MinLVL=1
MaxLVL=200
Color=BEBEBE
[CHEST]
Spawn=off
Despawn=off
MinLVL=1
MaxLVL=200
Color=BEBEBE
[TRAP]
Spawn=off
Despawn=off
MinLVL=1
MaxLVL=200
Color=BEBEBE
[TIMER]
Spawn=off
Despawn=off
MinLVL=1
MaxLVL=200
Color=BEBEBE
[TRIGGER]
Spawn=off
Despawn=off
MinLVL=1
MaxLVL=200
Color=BEBEBE
[CORPSE]
Spawn=off
Despawn=off
MinLVL=1
MaxLVL=200
Color=006400
[ITEM]
Spawn=off
Despawn=off
MinLVL=1
MaxLVL=200
Color=BEBEBE
[UNKNOWN]
Spawn=off
Despawn=off
MinLVL=1
MaxLVL=200
Color=FF69B4
```

Setting explanations can be found on the [/spawn](cmd-spawn.md) command page.
