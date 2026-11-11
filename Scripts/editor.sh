#!/usr/bin/env bash
# Opens, closes and reports the Unreal editor for this project, so agents can run the close, build,
# reopen loop without a person. Closing is always graceful: it never kills the process, so a save
# prompt or a hang leaves the editor running and fails loudly.
#
# Usage: Scripts/editor.sh status
#        Scripts/editor.sh open  [timeout-seconds]   (default 900; the first shader compile is slow)
#        Scripts/editor.sh close [timeout-seconds]   (default 120)
#
# `open` launches the editor in the background and waits until its MCP server listens. On macOS it
# uses `open -g`, which never takes focus from the person working on the machine.
#
# Before `close`, save everything over MCP (AssetTools save_assets with an empty list), or the
# editor shows a save prompt and this script times out.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/common.sh"

port_owner() {
	if [ "$HOST_PLATFORM" = Win64 ]; then
		netstat -ano 2>/dev/null | grep LISTENING | grep -E "[:.]$MCP_PORT[[:space:]]" | awk '{print $NF}' | head -1 || true
	else
		lsof -nP -t -iTCP:"$MCP_PORT" -sTCP:LISTEN 2>/dev/null | head -1 || true
	fi
}

# True only when the editor process itself owns the MCP port.
mcp_listening() {
	local PID
	PID=$(editor_pid)
	[ -n "$PID" ] && [ "$(port_owner)" = "$PID" ]
}

editor_gone() {
	[ -z "$(editor_pid)" ]
}

wait_until() {
	local TIMEOUT="$1"
	shift
	local START=$SECONDS
	while ! "$@"; do
		if [ $((SECONDS - START)) -ge "$TIMEOUT" ]; then
			return 1
		fi
		sleep 2
	done
}

launch_editor() {
	case "$HOST_PLATFORM" in
		Mac)
			open -g -a "$EDITOR_APP" --args "$UPROJECT" -nosplash
			;;
		Win64)
			# `start` returns at once; /MIN keeps the window from covering the person's work.
			cmd //c start "" //MIN "$(cygpath -w "$EDITOR_APP")" "$(cygpath -w "$UPROJECT")" -nosplash
			;;
		Linux)
			nohup "$EDITOR_APP" "$UPROJECT" -nosplash > /dev/null 2>&1 &
			;;
	esac
}

# Asks the editor to quit the way its own Quit menu does.
request_quit() {
	local PID="$1"
	case "$HOST_PLATFORM" in
		Mac)
			osascript -e 'tell application id "com.epicgames.UnrealEditor" to quit' > /dev/null 2>&1 \
				|| osascript -e 'tell application "UnrealEditor" to quit' > /dev/null 2>&1 || true
			;;
		Win64)
			# Without /F, taskkill posts WM_CLOSE: the editor can still ask to save.
			taskkill //PID "$PID" > /dev/null 2>&1 || true
			;;
		Linux)
			kill -TERM "$PID" 2>/dev/null || true
			;;
	esac
}

case "${1:-status}" in
	status)
		PID=$(editor_pid)
		if [ -z "$PID" ]; then
			echo "closed"
		elif mcp_listening; then
			echo "open (pid $PID, MCP listening on $MCP_PORT)"
		else
			echo "starting (pid $PID, MCP not listening yet)"
		fi
		;;
	open)
		require_engine
		TIMEOUT="${2:-900}"
		OWNER=$(port_owner)
		if [ -n "$OWNER" ] && [ "$OWNER" != "$(editor_pid)" ]; then
			# A crashed headless run can leave the crash reporter holding the port.
			echo "Port $MCP_PORT is held by another process (pid $OWNER). Free it first." >&2
			exit 1
		fi
		if [ -z "$(editor_pid)" ]; then
			launch_editor
		fi
		START=$SECONDS
		if ! wait_until "$TIMEOUT" mcp_listening; then
			echo "The editor did not start MCP within ${TIMEOUT}s. Check the editor window for a dialog." >&2
			exit 1
		fi
		echo "Editor open, MCP listening after $((SECONDS - START))s."
		;;
	close)
		TIMEOUT="${2:-120}"
		PID=$(editor_pid)
		if [ -z "$PID" ]; then
			echo "Editor already closed."
			exit 0
		fi
		request_quit "$PID"
		START=$SECONDS
		if ! wait_until "$TIMEOUT" editor_gone; then
			echo "The editor (pid $PID) is still running after ${TIMEOUT}s, probably at a save prompt. It was not killed." >&2
			exit 1
		fi
		echo "Editor closed after $((SECONDS - START))s."
		;;
	*)
		echo "usage: Scripts/editor.sh status|open [timeout]|close [timeout]" >&2
		exit 2
		;;
esac
