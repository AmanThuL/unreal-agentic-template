#pragma once

#include "CoreMinimal.h"

/** Everything the plugin logs starts with "AgentRun:" so Scripts/run-agent.sh can grep for it. */
AGENTRUN_API DECLARE_LOG_CATEGORY_EXTERN(LogAgentRun, Log, All);
