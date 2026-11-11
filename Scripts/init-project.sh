#!/usr/bin/env bash
# Renames the template project from MyGame to your game's name. Run it once, right after creating a
# repository from the template, before opening the editor.
#
# Usage: Scripts/init-project.sh <ProjectName>
#   <ProjectName> is a C++ identifier in PascalCase, for example Starfall or DeepCurrent.
#
# It renames the .uproject, the Source/MyGame module and its targets, and replaces MyGame and
# MYGAME in source, config, docs and agent files. The AgentRun plugin and the scripts need no
# rename: the scripts find the project from the .uproject file.
set -euo pipefail

OLD=MyGame
NEW="${1:?usage: Scripts/init-project.sh <ProjectName>}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if ! printf '%s' "$NEW" | grep -Eq '^[A-Z][A-Za-z0-9]*$'; then
	echo "'$NEW' is not a PascalCase C++ identifier (letters and digits, starting with a capital)." >&2
	exit 1
fi
if [ ! -f "$OLD.uproject" ]; then
	echo "No $OLD.uproject here: the project was already renamed." >&2
	exit 1
fi
if pgrep -f "UnrealEditor.*$OLD.uproject" > /dev/null 2>&1; then
	echo "Close the editor first." >&2
	exit 1
fi

OLD_UPPER="$(printf '%s' "$OLD" | tr '[:lower:]' '[:upper:]')"
NEW_UPPER="$(printf '%s' "$NEW" | tr '[:lower:]' '[:upper:]')"

# Text files that may name the project. Generated folders and the plugin are left alone.
FILES=()
while IFS= read -r FILE; do
	FILES+=("$FILE")
done < <(grep -rIlE "$OLD|$OLD_UPPER" . \
	--exclude-dir=.git --exclude-dir=Binaries --exclude-dir=Intermediate --exclude-dir=Saved \
	--exclude-dir=DerivedDataCache --exclude-dir=Plugins --exclude=init-project.sh)
for FILE in "${FILES[@]}"; do
	perl -pi -e "s/\\Q$OLD\\E/$NEW/g; s/\\Q$OLD_UPPER\\E/$NEW_UPPER/g" "$FILE"
done

mv "$OLD.uproject" "$NEW.uproject"
mv "Source/$OLD.Target.cs" "Source/$NEW.Target.cs"
mv "Source/${OLD}Editor.Target.cs" "Source/${NEW}Editor.Target.cs"
mv "Source/$OLD/$OLD.Build.cs" "Source/$OLD/$NEW.Build.cs"
mv "Source/$OLD/Private/$OLD.cpp" "Source/$OLD/Private/$NEW.cpp"
mv "Source/$OLD" "Source/$NEW"

# Stale binaries and makefiles still carry the old module name.
rm -rf Binaries Intermediate Plugins/AgentRun/Binaries Plugins/AgentRun/Intermediate

echo "Renamed $OLD to $NEW (${#FILES[@]} files edited)."
echo "Next: fill in the Project section of AGENTS.md, then Scripts/build-editor.sh && Scripts/run-tests.sh"
echo "You can delete Scripts/init-project.sh now."
