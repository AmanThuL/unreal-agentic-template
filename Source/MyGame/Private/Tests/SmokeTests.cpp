#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"

#if WITH_DEV_AUTOMATION_TESTS

// The first test of the project: proves the build, the module and the headless test run all work.
// Name tests MyGame.<Area>.<Case> so Scripts/run-tests.sh picks them up with its default filter.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSmokeModuleLoadedTest, "MyGame.Smoke.ModuleLoaded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSmokeModuleLoadedTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("The MyGame module is loaded"), FModuleManager::Get().IsModuleLoaded(TEXT("MyGame")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
