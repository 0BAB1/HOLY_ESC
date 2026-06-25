# Software (or "firmwares" for the dudes who wanna sound like they hacked something or whatever)

## Atmega code

Atmega code is for **rev A** of the ESC. Not really tested but gives you an idea of the scheduling. It seems the atmega has problems to keep up on hiogher speeds and deosn't really like the ripple when testing on a perfboard (duh). So the code kinda works, and should perform better on a PCB, perhaps even work "well enough". But it was not tested !

## STM 32 Code

The **REV B** embedds an actual STM32 for better control.