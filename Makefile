# March to Ganabhaban — build
#
#   mingw32-make          build build/game.exe
#   mingw32-make run      build and run
#   mingw32-make clean    remove build output
#   mingw32-make web      WebAssembly build (needs emsdk — see README)

RAYLIB   := vendor/raylib
PORTABLE_CXX := .toolchain/winlibs/mingw64/bin/g++.exe
ifneq ($(wildcard $(PORTABLE_CXX)),)
CXX      := $(PORTABLE_CXX)
else
CXX      := g++
endif
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wno-missing-field-initializers \
            -I$(RAYLIB)/include -Isrc
LDFLAGS  := -static-libgcc -static-libstdc++ \
            -L$(RAYLIB)/lib -lraylib -lopengl32 -lgdi32 -lwinmm

SRCS   := $(wildcard src/*.cpp)
TARGET := build/game.exe

all: $(TARGET)

# the leading '-' lets mkdir fail harmlessly when build/ already exists,
# which keeps this working under both cmd.exe and sh
$(TARGET): $(SRCS) $(wildcard src/*.h)
	-mkdir build
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	$(TARGET)

clean:
	-rm -rf build
	-rmdir /s /q build

# ---- WebAssembly (this is what puts it on your phone) ----
# One-time setup:
#   git clone https://github.com/emscripten-core/emsdk vendor/emsdk
#   cd vendor/emsdk && emsdk install latest && emsdk activate latest
# Then rebuild raylib for web:
#   cd vendor/raylib-src/src
#   mingw32-make PLATFORM=PLATFORM_WEB -B
# ASYNCIFY is deliberately NOT used: the game drives itself through
# emscripten_set_main_loop, so it would only add size and overhead.
web:
	-mkdir build
	-mkdir build\web
	emcc $(SRCS) -o build/web/index.html \
	  -I$(RAYLIB)/include -Isrc \
	  vendor/raylib-src/src/libraylib.web.a \
	  -std=c++17 -O3 -DPLATFORM_WEB \
	  -s USE_GLFW=3 -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_MEMORY=67108864 \
	  -s FORCE_FILESYSTEM=1 -s ASSERTIONS=0 \
	  --preload-file assets \
	  --shell-file tools/shell.html
	copy tools\manifest.webmanifest build\web\ >nul
	copy art_source\templates\icon-192.png build\web\ >nul 2>&1 || echo (no icons yet)
	copy art_source\templates\icon-512.png build\web\ >nul 2>&1 || echo (no icons yet)
	@echo.
	@echo Web build ready in build/web  --  serve it, do not open index.html directly:
	@echo     python -m http.server 8000 --directory build/web

.PHONY: all run clean web
