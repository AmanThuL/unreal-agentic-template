#!/usr/bin/env bash
# Checks Markdown docs against the documentation rules in docs/guidelines/10-docs-and-plans.md:
# at most 300 lines per file, and every relative link points at a file that exists.
#
# Usage: Scripts/check-docs.sh [file.md ...]
# With no arguments it checks every tracked or new (not ignored) .md file in the repo.
set -euo pipefail

MAX_LINES=300
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

FILES=()
if [ "$#" -gt 0 ]; then
	FILES=("$@")
else
	while IFS= read -r FILE; do
		FILES+=("$FILE")
	done < <(git ls-files --cached --others --exclude-standard -- '*.md')
fi

FAILED=0
for FILE in "${FILES[@]}"; do
	[ -f "$FILE" ] || continue
	LINES=$(wc -l < "$FILE" | tr -d ' ')
	if [ "$LINES" -gt "$MAX_LINES" ]; then
		echo "$FILE: $LINES lines (limit $MAX_LINES)"
		FAILED=1
	fi
	# Relative links: [text](path) or [text](path#anchor). Web links and pure anchors are skipped.
	DIR="$(dirname "$FILE")"
	while IFS= read -r TARGET; do
		[ -n "$TARGET" ] || continue
		TARGET_PATH="${TARGET%%#*}"
		[ -n "$TARGET_PATH" ] || continue
		if [ ! -e "$DIR/$TARGET_PATH" ]; then
			echo "$FILE: broken link to $TARGET"
			FAILED=1
		fi
	done < <(grep -oE '\]\([^)[:space:]]+\)' "$FILE" | sed -E 's/^\]\(//; s/\)$//' | grep -vE '^(https?:|mailto:|#)' || true)
done

if [ "$FAILED" -ne 0 ]; then
	exit 1
fi
echo "Docs OK (${#FILES[@]} files checked)"
