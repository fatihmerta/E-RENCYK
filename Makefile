# E-RENCYK // Early-2000s Portable MD/MP3 Hi-Fi Deck for Sony PSP
TARGET = erencyk
OBJS   = src/main.o src/gfx.o src/font.o src/cover.o src/audio.o src/browser.o src/playlist.o src/ui.o

INCDIR = src
CFLAGS = -O2 -G0 -Wall -Wextra -ffast-math
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

BUILD_PRX = 1
PSP_FW_VERSION = 600

EXTRA_TARGETS   = EBOOT.PBP
PSP_EBOOT_TITLE = E-RENCYK
PSP_EBOOT_ICON  = assets/ICON0.PNG
PSP_EBOOT_PIC1  = assets/PIC1.PNG

LIBS = -lmpg123 -lpng -lz -ljpeg -lpspgu -lpspaudio -lpsprtc -lpsppower -lm

PSPSDK := $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
