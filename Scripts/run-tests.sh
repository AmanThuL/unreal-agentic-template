#!/usr/bin/env bash
# Runs automation tests headless and fails unless every test passed.
#
# Usage: Scripts/run-tests.sh [filter]            (default: "<Project>+AgentRun")
#        Scripts/run-tests.sh --check-log FILE    (only check an existing test log)
# Join several filters with "+". Full log: Saved/Logs/Tests.log
#
# The exit code of the editor alone is not trusted. A run passes only if tests ran, every result is
# Success, the number of results matches the number of tests found, and the editor exited cleanly.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/common.sh"

# Checks a finished test log. Exit 0 only if tests ran and every result is Success.
check_log() {
	local LOG="$1"
	local FILTER="$2"

	local RESULTS
	RESULTS=$(grep -E 'Test Completed\. Result=\{' "$LOG" || true)
	if [ -z "$RESULTS" ]; then
		echo "No tests ran for filter '$FILTER'. See $LOG" >&2
		grep -E 'Error:|Fatal' "$LOG" | head -20 >&2 || true
		return 1
	fi

	local TOTAL
	TOTAL=$(printf '%s\n' "$RESULTS" | wc -l | tr -d ' ')
	local FAILURES
	FAILURES=$(printf '%s\n' "$RESULTS" | grep -v 'Result={Success}' || true)
	if [ -n "$FAILURES" ]; then
		local FAILED
		FAILED=$(printf '%s\n' "$FAILURES" | wc -l | tr -d ' ')
		printf '%s\n' "$FAILURES"
		# Print the logged errors that belong to failed tests.
		grep -E 'LogAutomationController: Error' "$LOG" | head -40 || true
		echo "$FAILED of $TOTAL tests failed. See $LOG"
		return 1
	fi

	# A crash or hang mid-run leaves fewer results than tests found and no completion line.
	local FOUND
	FOUND=$(grep -oE 'Found [0-9]+ automation tests' "$LOG" | head -1 | grep -oE '[0-9]+' || true)
	if [ -n "$FOUND" ] && [ "$TOTAL" -ne "$FOUND" ]; then
		echo "Only $TOTAL of $FOUND tests reported a result; the run stopped early. See $LOG" >&2
		grep -E 'Fatal|Assertion failed|Error:' "$LOG" | tail -5 >&2 || true
		return 1
	fi
	if ! grep -q 'TEST COMPLETE. EXIT CODE: 0' "$LOG"; then
		echo "The run did not finish with exit code 0. See $LOG" >&2
		return 1
	fi
	echo "$TOTAL of $TOTAL tests passed ($FILTER)."
}

if [ "${1:-}" = "--check-log" ]; then
	check_log "${2:?usage: Scripts/run-tests.sh --check-log FILE}" "${3:-log}"
	exit $?
fi

FILTER="${1:-$PROJECT_NAME+AgentRun}"
LOG="$PROJECT_ROOT/Saved/Logs/Tests.log"

require_engine
require_editor_closed

mkdir -p "$(dirname "$LOG")"
rm -f "$LOG"
EDITOR_RC=0
# -abslog: on macOS a plain -log goes to ~/Library/Logs/Unreal Engine/, not to the project.
"$EDITOR_CMD" "$UPROJECT" \
	-ExecCmds="Automation RunTests $FILTER;Quit" \
	-unattended -nullrhi -nosplash -nosound -abslog="$LOG" > /dev/null 2>&1 || EDITOR_RC=$?

if [ ! -f "$LOG" ]; then
	echo "No log at $LOG (editor exit code $EDITOR_RC)." >&2
	exit 1
fi

check_log "$LOG" "$FILTER"
if [ "$EDITOR_RC" -ne 0 ]; then
	echo "The editor exited with code $EDITOR_RC. See $LOG" >&2
	exit 1
fi
