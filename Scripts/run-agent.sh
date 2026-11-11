#!/usr/bin/env bash
# Runs an agent script in the packaged game without taking the person's mouse, keyboard or focus,
# then collects the evidence and says whether the run passed.
#
# Usage: Scripts/run-agent.sh <script.txt> [timeout-seconds] [extra game args...]
#
# The game starts in the background, windowed, with -AgentRun (real input ignored, no mouse capture,
# rendering while unfocused; see Plugins/AgentRun/README.md) and plays the script. The script should
# end with `quit`. After the timeout (default 600 s) the game is asked to quit, never killed.
#
# Evidence lands in Saved/AgentRuns/<script name>/: the game log, bench.csv and Shots/*.png.
# Exit code 0 only if the log says "script done" with 0 failed steps and the game exited by itself.
#
# Environment: AGENT_RES=WIDTHxHEIGHT (default 1920x1080; bench at the resolution players use),
#              AGENT_APP=path to a packaged game other than the one Scripts/package.sh makes.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/common.sh"

SCRIPT="${1:?usage: Scripts/run-agent.sh <script.txt> [timeout-seconds] [extra game args...]}"
TIMEOUT="${2:-600}"
shift || true
shift || true
RES="${AGENT_RES:-1920x1080}"
NAME="$(basename "$SCRIPT")"
RUN="${NAME%.*}"

if [ ! -f "$SCRIPT" ]; then
	echo "No script at $SCRIPT." >&2
	exit 1
fi

# The packaged game for this machine, its process pattern and its Saved/ folder (not the project's).
APP="${AGENT_APP:-$(packaged_game "$HOST_PLATFORM")}"
if [ -z "$APP" ] || [ ! -e "$APP" ]; then
	echo "No packaged game for $HOST_PLATFORM. Run Scripts/package.sh first." >&2
	exit 1
fi
case "$HOST_PLATFORM" in
	Mac)
		PROCESS="$(basename "$APP")/Contents/MacOS/"
		BUNDLE_ID="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$APP/Contents/Info.plist")"
		if codesign -d --entitlements - "$APP" 2>/dev/null | grep -q 'app-sandbox'; then
			# A sandboxed app can only read and write inside its container.
			GAME_SAVED="$HOME/Library/Containers/$BUNDLE_ID/Data/Library/Application Support/Epic/$PROJECT_NAME/Saved"
		else
			GAME_SAVED="$HOME/Library/Application Support/Epic/$PROJECT_NAME/Saved"
		fi
		;;
	Win64)
		# Development and Test builds keep Saved/ next to the game; Shipping does not run agent scripts.
		PROCESS="$PROJECT_NAME"
		GAME_SAVED="$(dirname "$APP")/$PROJECT_NAME/Saved"
		;;
	Linux)
		PROCESS="$PROJECT_NAME/Binaries/Linux/$PROJECT_NAME"
		GAME_SAVED="$(dirname "$APP")/$PROJECT_NAME/Saved"
		;;
esac

game_pid() {
	if [ "$HOST_PLATFORM" = Win64 ]; then
		tasklist //FO CSV //NH 2>/dev/null | grep -i "^\"$PROCESS" | head -1 | cut -d, -f2 | tr -d '"' || true
	else
		pgrep -f "$PROCESS" | head -1 || true
	fi
}

if [ -n "$(game_pid)" ]; then
	echo "A packaged $PROJECT_NAME is already running." >&2
	exit 1
fi

# Fresh inputs and outputs inside the game's Saved/AgentRuns/.
GAME_RUNS="$GAME_SAVED/AgentRuns"
GAME_LOG="$GAME_RUNS/$RUN.log"
mkdir -p "$GAME_RUNS"
rm -rf "$GAME_RUNS/Shots" "$GAME_RUNS/bench.csv" "$GAME_LOG"
cp "$SCRIPT" "$GAME_RUNS/$NAME"

ARGS=(-AgentRun "-AgentScript=$NAME" -nosplash -windowed "-ResX=${RES%x*}" "-ResY=${RES#*x}" "-abslog=$GAME_LOG" "$@")
case "$HOST_PLATFORM" in
	Mac)
		# -g: in the background. -n: a new instance even if the app was opened before.
		open -g -n "$APP" --args "${ARGS[@]}"
		;;
	*)
		"$APP" "${ARGS[@]}" > /dev/null 2>&1 &
		;;
esac

# On macOS, count how often the game is the frontmost app while it runs: it should be never.
front_is_game() {
	[ "$HOST_PLATFORM" = Mac ] && lsappinfo info -only bundleid "$(lsappinfo front)" 2>/dev/null | grep -q "\"$BUNDLE_ID\""
}

START=$SECONDS
TIMED_OUT=0
FRONT=0
sleep 3
while [ -n "$(game_pid)" ]; do
	if front_is_game; then
		FRONT=$((FRONT + 1))
	fi
	if [ $((SECONDS - START)) -ge "$TIMEOUT" ] && [ "$TIMED_OUT" -eq 0 ]; then
		TIMED_OUT=1
		echo "The run passed ${TIMEOUT}s; asking the game to quit." >&2
		case "$HOST_PLATFORM" in
			Mac) osascript -e "tell application id \"$BUNDLE_ID\" to quit" > /dev/null 2>&1 || true ;;
			Win64) taskkill //PID "$(game_pid)" > /dev/null 2>&1 || true ;;
			Linux) kill -TERM "$(game_pid)" 2>/dev/null || true ;;
		esac
	fi
	if [ $((SECONDS - START)) -ge $((TIMEOUT + 60)) ]; then
		echo "The game is still running 60s after it was asked to quit. It was not killed (pid $(game_pid))." >&2
		break
	fi
	sleep 2
done
ELAPSED=$((SECONDS - START))

# Collect the evidence next to the project, where agents and reviewers look.
OUT="$PROJECT_ROOT/Saved/AgentRuns/$RUN"
rm -rf "$OUT"
mkdir -p "$OUT"
[ -f "$GAME_LOG" ] && cp "$GAME_LOG" "$OUT/$RUN.log"
[ -f "$GAME_RUNS/bench.csv" ] && cp "$GAME_RUNS/bench.csv" "$OUT/bench.csv"
[ -d "$GAME_RUNS/Shots" ] && cp -R "$GAME_RUNS/Shots" "$OUT/Shots"

if [ ! -f "$OUT/$RUN.log" ]; then
	echo "The game wrote no log at $GAME_LOG." >&2
	exit 1
fi

grep -E 'AgentRun: (bench|selftest|FAILED)' "$OUT/$RUN.log" | sed 's/^.*AgentRun: /  /' || true
if [ "$FRONT" -gt 0 ]; then
	echo "Warning: the game was the frontmost app in $FRONT of the polls. Agent runs should stay in the background." >&2
fi
DONE="$(grep -oE 'AgentRun: script done, [0-9]+ steps, [0-9]+ failed' "$OUT/$RUN.log" | tail -1 || true)"
if [ -z "$DONE" ]; then
	echo "The script did not finish (${ELAPSED}s). See $OUT/$RUN.log" >&2
	exit 1
fi
if [ "$TIMED_OUT" -ne 0 ] || ! printf '%s' "$DONE" | grep -q ', 0 failed'; then
	echo "Run failed after ${ELAPSED}s: ${DONE#AgentRun: }. See $OUT/$RUN.log" >&2
	exit 1
fi
echo "Run passed after ${ELAPSED}s: ${DONE#AgentRun: }. Evidence: $OUT"
