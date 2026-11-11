#!/usr/bin/env bash
# Builds, cooks, stages and archives a packaged game into Saved/Packaged/, outside the editor.
# The editor must be closed: the cook reads the same assets and shader cache.
#
# Usage: Scripts/package.sh [--platform Mac|Win64|Linux|LinuxArm64] [--config Development|Test|Shipping]
#                           [extra BuildCookRun args...]
#
#   --platform  the target platform. Default: the platform of this machine.
#   --config    the client configuration. Default: Development.
#
# Output (log: Saved/Logs/Package-<platform>.log):
#   Mac          Saved/Packaged/Mac/<Project>.app
#   Win64        Saved/Packaged/Windows/<Project>.exe
#   Linux        Saved/Packaged/Linux/<Project>.sh
#   LinuxArm64   Saved/Packaged/LinuxArm64/<Project>-Arm64.sh
#
# Which machine can package which platform (an engine limit, not a script limit):
#   macOS host    Mac
#   Windows host  Win64, and Linux or LinuxArm64 with Epic's Linux cross-compile toolchain installed
#   Linux host    Linux, LinuxArm64
# To ship all three desktop platforms, run this script on a Mac and on a Windows machine (or on a
# Mac, a Windows and a Linux machine), for example as three CI jobs.
#
# Agent runs (Scripts/run-agent.sh) need a Development or Test build: the AgentRun plugin is inert
# in Shipping.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/common.sh"

PLATFORM="$HOST_PLATFORM"
CONFIG=Development
EXTRA=()
while [ "$#" -gt 0 ]; do
	case "$1" in
		--platform) PLATFORM="${2:?--platform needs a value}"; shift 2 ;;
		--config) CONFIG="${2:?--config needs a value}"; shift 2 ;;
		-h|--help) sed -n '2,25p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0 ;;
		*) EXTRA+=("$1"); shift ;;
	esac
done

if ! packaged_folder "$PLATFORM" > /dev/null; then
	echo "Unknown platform '$PLATFORM'. Use Mac, Win64, Linux or LinuxArm64." >&2
	exit 2
fi
case "$CONFIG" in
	Development|Test|Shipping|DebugGame|Debug) ;;
	*) echo "Unknown configuration '$CONFIG'. Use Development, Test or Shipping." >&2; exit 2 ;;
esac

# Fail early, with the reason, on a host that cannot build the target.
case "$HOST_PLATFORM:$PLATFORM" in
	Mac:Mac|Win64:Win64|Linux:Linux|Linux:LinuxArm64) ;;
	Win64:Linux|Win64:LinuxArm64)
		if [ -z "${LINUX_MULTIARCH_ROOT:-}" ]; then
			echo "Packaging $PLATFORM on Windows needs Epic's Linux cross-compile toolchain (it sets LINUX_MULTIARCH_ROOT)." >&2
			exit 1
		fi
		;;
	*)
		echo "A $HOST_PLATFORM host cannot package $PLATFORM. See the host table at the top of Scripts/package.sh." >&2
		exit 1
		;;
esac

require_engine
require_editor_closed

LOG="$PROJECT_ROOT/Saved/Logs/Package-$PLATFORM.log"
mkdir -p "$(dirname "$LOG")"
set +e
"$UAT_SCRIPT" BuildCookRun \
	-project="$UPROJECT" -platform="$PLATFORM" -clientconfig="$CONFIG" \
	-build -cook -stage -pak -archive -archivedirectory="$PROJECT_ROOT/Saved/Packaged" \
	-noP4 -unattended -utf8output ${EXTRA[@]+"${EXTRA[@]}"} \
	2>&1 | tee "$LOG"
STATUS="${PIPESTATUS[0]}"
set -e
if [ "$STATUS" -ne 0 ]; then
	echo "Packaging failed with code $STATUS. See $LOG" >&2
	exit "$STATUS"
fi

if [ "$PLATFORM" = Mac ]; then
	# Seen on macOS with UE 5.8: the archive step copies the .app shell without Contents/UE (the paks
	# and third-party dylibs), so the archived game cannot start. Use the complete staged app instead.
	STAGED="$(find "$PROJECT_ROOT/Saved/StagedBuilds/Mac" -maxdepth 1 -name "$PROJECT_NAME*.app" | head -1)"
	ARCHIVED="$(packaged_game Mac)"
	if [ -z "$ARCHIVED" ] || [ ! -d "$ARCHIVED/Contents/UE" ]; then
		if [ -z "$STAGED" ] || [ ! -d "$STAGED/Contents/UE" ]; then
			echo "Neither the archived nor the staged app has Contents/UE; packaging failed." >&2
			exit 1
		fi
		ARCHIVED="$PROJECT_ROOT/Saved/Packaged/Mac/$(basename "$STAGED")"
		rm -rf "$ARCHIVED"
		mkdir -p "$(dirname "$ARCHIVED")"
		ditto "$STAGED" "$ARCHIVED"
	fi
fi

# UAT can exit 0 without leaving something to launch; check for it.
GAME="$(packaged_game "$PLATFORM")"
if [ -z "$GAME" ]; then
	echo "Packaging reported success but there is no game to launch in Saved/Packaged/$(packaged_folder "$PLATFORM")." >&2
	exit 1
fi
echo "Packaged ($PLATFORM, $CONFIG): $GAME"
