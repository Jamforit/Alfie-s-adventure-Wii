# devkitPro Wii / libogc: make -> boot.dol
ifndef DEVKITPRO
$(error DEVKITPRO is missing. Install devkitPro's wii-dev environment)
endif
ifndef DEVKITPPC
DEVKITPPC := $(DEVKITPRO)/devkitPPC
endif
CC := $(DEVKITPPC)/bin/powerpc-eabi-gcc
ELF2DOL := $(DEVKITPRO)/tools/bin/elf2dol
# The elf2dol binary can also be available on PATH.
ifeq ($(wildcard $(ELF2DOL)),)
ELF2DOL := elf2dol
endif
CFLAGS := -std=gnu11 -O2 -Wall -Wextra -Wno-unused-function -DGEKKO -mrvl -mcpu=750 -meabi -mhard-float -I$(DEVKITPRO)/libogc/include
LDFLAGS := -mrvl -mcpu=750 -meabi -mhard-float -L$(DEVKITPRO)/libogc/lib/wii
LIBS := -lasnd -lwiiuse -lbte -logc -lm
.PHONY: all clean test
all: boot.dol
boot.elf: source/main.c source/game.h source/sprites.h
	$(CC) $(CFLAGS) source/main.c $(LDFLAGS) $(LIBS) -o $@
boot.dol: boot.elf
	$(ELF2DOL) $< $@
test:
	cc -std=gnu11 -Wall -Wextra -Werror tests/test_game.c -lm -o tests/test_game
	./tests/test_game
clean:
	rm -f boot.elf boot.dol tests/test_game
