#!/usr/bin/env bash
# Shared by every script in Scripts/: finds the project, the engine and the platform's tool paths.
# Source it; do not run it.
#
# Environment:
#   UE_ENGINE_DIR  the engine's Engine/ folder. Defaults to the launcher install for UE_VERSION.
#   UE_VERSION     defaults to the EngineAssociation in the .uproject.
#   UE_MCP_PORT    the editor's MCP port (default 8000; see Config/DefaultEditorPerProjectUserSettings.ini).
#
# Verified on macOS (Apple Silicon). The Windows (Git Bash) and Linux branches follow the engine's
# documented layout but have not been exercised; fix them here if your machine differs.

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

UPROJECT="$(find "$PROJECT_ROOT" -maxdepth 1 -name '*.uproject' | head -1)"
if [ -z "$UPROJECT" ]; then
	echo "No .uproject file in $PROJECT_ROOT." >&2
	exit 1
fi
PROJECT_NAME="$(basename "$UPROJECT" .uproject)"
MCP_PORT="${UE_MCP_PORT:-8000}"

if [ -z "${UE_VERSION:-}" ]; then
	UE_VERSION="$(sed -n 's/.*"EngineAssociation"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "$UPROJECT" | head -1)"
fi

case "$(uname -s)" in
	Darwin)
		HOST_PLATFORM=Mac
		ENGINE_DIR="${UE_ENGINE_DIR:-/Users/Shared/Epic Games/UE_$UE_VERSION/Engine}"
		EDITOR_APP="$ENGINE_DIR/Binaries/Mac/UnrealEditor.app"
		EDITOR_CMD="$ENGINE_DIR/Binaries/Mac/UnrealEditor-Cmd"
		BUILD_SCRIPT="$ENGINE_DIR/Build/BatchFiles/Mac/Build.sh"
		UAT_SCRIPT="$ENGINE_DIR/Build/BatchFiles/RunUAT.sh"
		;;
	Linux)
		HOST_PLATFORM=Linux
		ENGINE_DIR="${UE_ENGINE_DIR:-$HOME/UnrealEngine/Engine}"
		EDITOR_APP="$ENGINE_DIR/Binaries/Linux/UnrealEditor"
		EDITOR_CMD="$ENGINE_DIR/Binaries/Linux/UnrealEditor-Cmd"
		BUILD_SCRIPT="$ENGINE_DIR/Build/BatchFiles/Linux/Build.sh"
		UAT_SCRIPT="$ENGINE_DIR/Build/BatchFiles/RunUAT.sh"
		;;
	MINGW*|MSYS*|CYGWIN*)
		HOST_PLATFORM=Win64
		ENGINE_DIR="${UE_ENGINE_DIR:-/c/Program Files/Epic Games/UE_$UE_VERSION/Engine}"
		EDITOR_APP="$ENGINE_DIR/Binaries/Win64/UnrealEditor.exe"
		EDITOR_CMD="$ENGINE_DIR/Binaries/Win64/UnrealEditor-Cmd.exe"
		BUILD_SCRIPT="$ENGINE_DIR/Build/BatchFiles/Build.bat"
		UAT_SCRIPT="$ENGINE_DIR/Build/BatchFiles/RunUAT.bat"
		;;
	*)
		echo "Unsupported host: $(uname -s)." >&2
		exit 1
		;;
esac

require_engine() {
	if [ ! -d "$ENGINE_DIR" ]; then
		echo "No engine at $ENGINE_DIR. Install Unreal Engine $UE_VERSION or set UE_ENGINE_DIR to its Engine/ folder." >&2
		exit 1
	fi
}

# The pid of the editor that has this project open, or nothing.
editor_pid() {
	if [ "$HOST_PLATFORM" = Win64 ]; then
		# One editor per machine is assumed on Windows: tasklist cannot see command lines.
		tasklist //FI "IMAGENAME eq UnrealEditor.exe" //FO CSV //NH 2>/dev/null | grep -i UnrealEditor | head -1 | cut -d, -f2 | tr -d '"' || true
	else
		pgrep -f "UnrealEditor.*$PROJECT_NAME.uproject" | head -1 || true
	fi
}

# Exits with a message when the editor has the project open: builds, headless tests and cooks all
# need it closed (it locks the binaries and shares the asset registry and shader cache).
require_editor_closed() {
	if [ -n "$(editor_pid)" ]; then
		echo "The editor is running with $PROJECT_NAME.uproject. Save over MCP, run Scripts/editor.sh close, then try again." >&2
		exit 1
	fi
}

# The folder under Saved/Packaged/ that BuildCookRun archives a target platform into.
packaged_folder() {
	case "$1" in
		Mac) echo Mac ;;
		Win64) echo Windows ;;
		Linux) echo Linux ;;
		LinuxArm64) echo LinuxArm64 ;;
		*) return 1 ;;
	esac
}

# The thing to launch in a packaged build of platform $1: the .app on Mac, the bootstrap .exe on
# Windows, the bootstrap .sh on Linux. Prints nothing when there is no such build.
packaged_game() {
	local DIR="$PROJECT_ROOT/Saved/Packaged/$(packaged_folder "$1")"
	[ -d "$DIR" ] || return 0
	case "$1" in
		Mac) find "$DIR" -maxdepth 1 -name "$PROJECT_NAME*.app" | head -1 ;;
		Win64) find "$DIR" -maxdepth 1 -name "$PROJECT_NAME*.exe" | head -1 ;;
		Linux|LinuxArm64) find "$DIR" -maxdepth 1 -name "$PROJECT_NAME*.sh" | head -1 ;;
	esac
}
