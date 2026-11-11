#!/usr/bin/env bash
# Optional: symlinks a curated subset of the community skill pack kevinpbuckley/unreal-engine-skills
# into .claude/skills/, so agents get Unreal C++ and engine API knowledge on demand.
#
# The upstream repo has no license, so the skills are linked from a local clone instead of being
# committed here. The links are gitignored. Agents other than Claude Code can read the SKILL.md
# files in the clone directly.
#
# Usage:   Scripts/link-ue-skills.sh [path-to-clone]
# Default: ~/.local/share/unreal-engine-skills (cloned if missing)
# Update:  git -C <clone> pull, then run this script again.
set -euo pipefail

UPSTREAM_URL="https://github.com/kevinpbuckley/unreal-engine-skills.git"
CLONE_DIR="${1:-$HOME/.local/share/unreal-engine-skills}"
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SKILLS_DIR="$PROJECT_ROOT/.claude/skills"

# A general set for a single-player 3D game in C++ and Blueprints. Every listed skill adds its
# description to each session's context, so keep the list to what the game needs: add networking,
# GAS, animation or landscape skills when the work calls for them, and drop what it never touches.
# The editor's MCP server ships its own skills for editor toolsets (Niagara, PCG, materials).
SKILLS=(
	# C++ foundations
	ue-coding-standards
	ue-cpp-fundamentals
	ue-memory-and-gc
	ue-core-types-and-containers
	ue-module-and-build-system
	ue-project-structure
	ue-navigating-engine-source
	ue-logging-and-assertions
	ue-delegates-and-events
	# Planning and gameplay
	ue-gameplay-architecture-planning
	ue-gameplay-framework
	ue-actors-and-components
	ue-character-and-movement
	ue-enhanced-input
	ue-subsystems
	ue-timers-and-async
	ue-save-and-load
	ue-physics-and-chaos
	ue-data-driven-design
	ue-blueprint-cpp-integration
	# Visuals and audio
	ue-materials-and-shaders
	ue-lighting-and-lumen
	ue-audio-and-metasounds
	# Quality
	ue-automation-and-testing
	ue-debugging-techniques
	ue-profiling-and-optimization
)

if [[ ! -d "$CLONE_DIR/.git" ]]; then
	echo "Cloning $UPSTREAM_URL into $CLONE_DIR"
	mkdir -p "$(dirname "$CLONE_DIR")"
	git clone --quiet "$UPSTREAM_URL" "$CLONE_DIR"
fi

# Check every skill before touching existing links, so a bad list leaves things as they were.
missing=()
for name in "${SKILLS[@]}"; do
	if [[ ! -f "$CLONE_DIR/skills/core/$name/SKILL.md" ]]; then
		missing+=("$name")
	fi
done
if (( ${#missing[@]} > 0 )); then
	echo "error: not found in $CLONE_DIR/skills/core: ${missing[*]}" >&2
	echo "They may have been renamed upstream. Update SKILLS in $0." >&2
	exit 1
fi

mkdir -p "$SKILLS_DIR"

# Remove previous links (only symlinks named ue-*), so skills dropped from the list disappear.
find "$SKILLS_DIR" -maxdepth 1 -type l -name 'ue-*' -delete

for name in "${SKILLS[@]}"; do
	ln -s "$CLONE_DIR/skills/core/$name" "$SKILLS_DIR/$name"
done

echo "Linked ${#SKILLS[@]} skills from $CLONE_DIR @ $(git -C "$CLONE_DIR" rev-parse --short HEAD) into $SKILLS_DIR"
