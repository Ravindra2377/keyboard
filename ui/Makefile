# StealthOS UI — Build System
#
# Compiles the framebuffer-based phone UI.
#
# Usage:
#   make                         # Build for host (Linux desktop testing)
#   make CROSS_COMPILE=arm-linux-gnueabihf-  # Cross-compile for ARM target
#   make clean                   # Remove build artifacts

# ─── Compiler Settings ──────────────────────────────────────────────────────

CROSS_COMPILE ?=
CC      := $(CROSS_COMPILE)gcc
LD      := $(CROSS_COMPILE)gcc
STRIP   := $(CROSS_COMPILE)strip
TARGET  ?= host

# ─── Directories ─────────────────────────────────────────────────────────────

SRCDIR  := src
BUILDDIR := build/$(TARGET)
BINDIR  := bin/$(TARGET)

# ─── Source Files ────────────────────────────────────────────────────────────

SRCS := \
	$(SRCDIR)/main.c \
	$(SRCDIR)/fb.c \
	$(SRCDIR)/font.c \
	$(SRCDIR)/telephony.c \
	$(SRCDIR)/storage.c \
	$(SRCDIR)/keypad.c \
	$(SRCDIR)/t9.c \
	$(SRCDIR)/screen.c \
	$(SRCDIR)/screens/idle.c \
	$(SRCDIR)/screens/dialer.c \
	$(SRCDIR)/screens/home.c \
	$(SRCDIR)/screens/sms_list.c \
	$(SRCDIR)/screens/sms_compose.c \
	$(SRCDIR)/screens/contacts.c \
	$(SRCDIR)/screens/incall.c \
	$(SRCDIR)/screens/settings.c

OBJS := $(SRCS:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)
DEPS := $(OBJS:.o=.d)

# ─── Compiler Flags ─────────────────────────────────────────────────────────

CFLAGS := \
	-Wall -Wextra -Wpedantic \
	-Werror=implicit-function-declaration \
	-Werror=return-type \
	-std=c11 \
	-O2 \
	-D_POSIX_C_SOURCE=200809L \
	-MMD -MP

# Security hardening flags
CFLAGS += \
	-fstack-protector-strong \
	-D_FORTIFY_SOURCE=2 \
	-fPIE

LDFLAGS := \
	-lm \
	-lsqlcipher \
	-lcrypto \
	-pie

# Static linking for target (no shared libraries on minimal rootfs)
ifeq ($(TARGET),arm)
LDFLAGS += -static
CFLAGS  += -DSTEALTH_TARGET_ARM
endif

# Debug build
ifdef DEBUG
CFLAGS  += -g -O0 -DDEBUG
LDFLAGS += -g
endif

# ─── Output Binary ──────────────────────────────────────────────────────────

BIN := $(BINDIR)/stealth-ui

# ─── Targets ─────────────────────────────────────────────────────────────────

.PHONY: all clean install

all: $(BIN)

$(BIN): $(OBJS) | $(BINDIR)
	@echo "  LD    $@"
	@$(LD) $(OBJS) -o $@ $(LDFLAGS)
ifeq ($(TARGET),arm)
	@echo "  STRIP $@"
	@$(STRIP) $@
endif
	@echo ""
	@echo "  Built: $@"
	@ls -lh $@

# Compile C sources
$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	@echo "  CC    $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

# Create build directories
$(BUILDDIR):
	@mkdir -p $(BUILDDIR)/screens

$(BINDIR):
	@mkdir -p $(BINDIR)

# Include dependency files
-include $(DEPS)

# Clean
clean:
	rm -rf build/ bin/

# Install to rootfs overlay (for Buildroot integration)
install: $(BIN)
	install -D -m 0755 $(BIN) \
		$(DESTDIR)/usr/bin/stealth-ui

# ─── Size Analysis ──────────────────────────────────────────────────────────

size: $(BIN)
	@echo ""
	@echo "Binary size breakdown:"
	@$(CROSS_COMPILE)size $(BIN)
	@echo ""
	@echo "Section sizes:"
	@$(CROSS_COMPILE)size -A $(BIN) | head -20

# ─── Simulator ──────────────────────────────────────────────────────────────

sim: CFLAGS += -DSIMULATOR -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2
sim: LDFLAGS += -L/opt/homebrew/lib -lSDL2 -lsqlcipher -lcrypto
sim: SRCS_SIM = $(filter-out $(SRCDIR)/fb.c $(SRCDIR)/keypad.c $(SRCDIR)/telephony.c, $(SRCS)) \
                $(SRCDIR)/fb_sim.c $(SRCDIR)/keypad_sim.c $(SRCDIR)/telephony_stub.c
sim:
	$(CC) $(CFLAGS) $(SRCS_SIM) -o stealth-sim $(LDFLAGS)
	./stealth-sim
