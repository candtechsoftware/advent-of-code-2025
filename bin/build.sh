#!/bin/bash
set -e


SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

# Detect OS
if [[ "$OSTYPE" == "darwin"* ]]; then
    OS="mac"
    CC="clang"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
    CC="gcc"
else
    echo "Unsupported OS: $OSTYPE"
    exit 1
fi

# Default build mode
BUILD_MODE="debug"
TARGET=""
DAY_NUM=""
RUN_AFTER_BUILD=0

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        day)
            TARGET="day"
            DAY_NUM="$2"
            shift 2
            ;;
        meta)
            TARGET="meta"
            shift
            ;;
        clean)
            TARGET="clean"
            shift
            ;;
        -run)
            RUN_AFTER_BUILD=1
            shift
            ;;
        -release)
            BUILD_MODE="release"
            shift
            ;;
        -debug)
            BUILD_MODE="debug"
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: ./bin/build.sh day <N> [-run] [-debug|-release]"
            exit 1
            ;;
    esac
done

# Common flags (matching code_base build)
COMMON_FLAGS="-Wall -Wextra"
COMMON_FLAGS="$COMMON_FLAGS -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter"
COMMON_FLAGS="$COMMON_FLAGS -Wno-missing-braces -Wno-missing-field-initializers"
COMMON_FLAGS="$COMMON_FLAGS -Wno-unused-value -Wno-enum-conversion -Wno-enum-enum-conversion"
COMMON_FLAGS="$COMMON_FLAGS -fno-strict-aliasing"

# Include paths
INCLUDES="-I$PROJECT_ROOT/src -I$PROJECT_ROOT"

# Build mode flags
if [[ "$BUILD_MODE" == "release" ]]; then
    MODE_FLAGS="-O3 -DNDEBUG"
else
    MODE_FLAGS="-g -O0 -DRUN_MODE_DEBUG"
fi

# OS-specific flags
if [[ "$OS" == "mac" ]]; then
    OS_FLAGS="-x objective-c -fobjc-arc -D__OBJC__"
    LIBS="-lpthread -framework Foundation -framework AppKit -framework Cocoa"
elif [[ "$OS" == "linux" ]]; then
    OS_FLAGS="-D_GNU_SOURCE"
    LIBS="-lpthread -lm"
fi

# Ensure build directory exists
mkdir -p "$BUILD_DIR"

case $TARGET in
    clean)
        echo "Cleaning build artifacts..."
        rm -rf "$BUILD_DIR"/*
        echo "Done."
        ;;
    meta)
        echo "Building meta program..."
        $CC $COMMON_FLAGS $MODE_FLAGS $OS_FLAGS $INCLUDES \
            "$PROJECT_ROOT/src/meta/main.c" \
            -o "$BUILD_DIR/meta" $LIBS
        echo "Built: $BUILD_DIR/meta"
        ;;
    day)
        if [[ -z "$DAY_NUM" ]]; then
            echo "Error: Day number required"
            echo "Usage: ./bin/build.sh day <N>"
            exit 1
        fi

        # Pad day number with leading zero for source file
        DAY_PADDED=$(printf "%02d" "$DAY_NUM")
        DAY_SOURCE="$PROJECT_ROOT/src/puzzles/day_${DAY_PADDED}.c"
        OUTPUT_BIN="$BUILD_DIR/day${DAY_NUM}"

        if [[ ! -f "$DAY_SOURCE" ]]; then
            echo "Error: $DAY_SOURCE not found"
            echo "Run: ./build/meta -day=$DAY_NUM to create it"
            exit 1
        fi

        echo "Building day $DAY_NUM ($BUILD_MODE mode)..."
        $CC $COMMON_FLAGS $MODE_FLAGS $OS_FLAGS $INCLUDES \
            -DDAY_NUMBER=$DAY_NUM \
            "$DAY_SOURCE" \
            -o "$OUTPUT_BIN" $LIBS

        echo "Built: $OUTPUT_BIN"

        if [[ $RUN_AFTER_BUILD -eq 1 ]]; then
            echo ""
            echo "Running day $DAY_NUM..."
            echo "----------------------------------------"
            "$OUTPUT_BIN"
        fi
        ;;
    *)
        echo "Advent of Code Build Script"
        echo ""
        echo "Usage:"
        echo "  ./bin/build.sh day <N>         - Build puzzle day N"
        echo "  ./bin/build.sh day <N> -run    - Build and run puzzle day N"
        echo "  ./bin/build.sh meta            - Build the meta program"
        echo "  ./bin/build.sh clean           - Clean build artifacts"
        echo ""
        echo "Options:"
        echo "  -run        Run after building"
        echo "  -debug      Debug build (default)"
        echo "  -release    Release build (optimized)"
        echo ""
        echo "Examples:"
        echo "  ./bin/build.sh day 1"
        echo "  ./bin/build.sh day 1 -run"
        echo "  ./bin/build.sh day 25 -release -run"
        echo "  ./bin/build.sh meta"
        ;;
esac
