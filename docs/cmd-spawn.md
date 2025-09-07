---
tags:
  - command
---

# /spawn

## Syntax

<!--cmd-syntax-start-->
```eqcommand
/spawn [option] [setting]
```
<!--cmd-syntax-end-->

## Description

<!--cmd-desc-start-->
Configure settings for the plugin: excluding certain names, toggling timestamps, etc.
<!--cmd-desc-end-->

## Options

| Option | Description |
|--------|-------------|
| `help` | displays help text |
| `on|off` | Turns spawn announcements on and off. If no option is given, `/spawn` will toggle this setting. |
| `loc` | toggle spawn location suffix in output |
| `spawnid` | toggles display of mob spawnid suffix in output |
| `timestamp` | toggles timestamp prefix in output |
| `autosave` | toggles autosaving of INI file upon setting change |
| `delay <#>` | sets zone time delay |
| `log [on|off|auto|setpath <path>]` | Turns logging on, off, or enables automatic logging. `/spawn setpath <path>` will set a new location for the log file. |
| `status` | displays current settings |
| `savebychar` | toggle saving settings to a server.charname INI section |
| `save|load` | save or load configuration from the INI |
| `clear` | clears UI text |
| `min` | minimizes UI window |
| `font <#>` | sets UI font size |
| `exclude <"name">` | Adds a spawn name to the exclude list. Use quotes if a space is needed. |
| `<toggletype> [spawn | despawn | minlevel <#> | maxlevel <#> | color <FF00FF>]` | Toggles output of this type of spawn. minlevel and maxlevel limit the level of the type shown. color changes the display color, and can be any RGB hex value. e.g. `/spawn pc color 00ff5c`<br><br>Valid toggle types<br>:all - pc - npc - mount - pet - merc - flyer - campfire - banner - aura - object - untargetable - chest - trap - timer - trigger - corpse - item - unknown |
