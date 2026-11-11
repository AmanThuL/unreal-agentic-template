#!/usr/bin/env bash
# Builds the <Project>Editor target for this machine (Development).
# The editor must be closed. On macOS and Linux there is no Live Coding, so a running editor would
# keep the old module loaded and lock the binaries; on Windows this is still the reliable path after
# header or reflection changes.
#
# Usage: Scripts/build-editor.sh [extra UnrealBuildTool args...]
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/common.sh"

require_engine
require_editor_closed

"$BUILD_SCRIPT" "${PROJECT_NAME}Editor" "$HOST_PLATFORM" Development \
	-Project="$UPROJECT" -WaitMutex "$@"
