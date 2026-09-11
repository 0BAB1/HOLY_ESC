# Software

The binary in this folder is targeting a STM32F051U8K6 with a somewhat standard pinout group. Here is how it was configure in the AM32 `targets.h` file for the build, if you'd like to build it yourself from [AM32 Source Code](https://github.com/am32-firmware/AM32).

```c
#ifdef HOLYESC_F051

#define FILE_NAME        "HOLYESC_F051"
#define FIRMWARE_NAME    "HOLY_ESC"
#define DEAD_TIME        35
#define HARDWARE_GROUP_F0_B
#define USE_SERIAL_TELEMETRY

#endif
```

> You may have to change their `ARM_SDK_PREFIX` in `tools.mk` to build it yourself.

and built using a command that looks something like that (I don't fully remember tbh lol):

```sh
make HOLYESC_F051 obj/AM32_HOLYESC_F051_2.20.bin
```

> Do not use this binary on other boards than the HOLY ESC and use the specified loading procedure.

## Which file to use ?

I included both the ELF and the BIN files.

ELF already contains address informations but will alter some bootloading memory region from my experience. **So I prefer to load the binary directly and specify the address manually, as stated in the procedure.**

in other words, follow the procedure and use the `.bin` file.