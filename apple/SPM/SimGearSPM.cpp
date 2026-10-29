#include "SimGearSPM.h"
#include <OpenSceneGraphSPM.h>
#include <simgear/version.h>
#define SG_SPM_STRING_IMPL(x) #x
#define SG_SPM_STRING(x) SG_SPM_STRING_IMPL(x)
extern "C" const char *SimGearSPMVersion(void) { return SG_SPM_STRING(SIMGEAR_MAJOR_VERSION) "." SG_SPM_STRING(SIMGEAR_MINOR_VERSION) "." SG_SPM_STRING(SIMGEAR_PATCH_VERSION); }
extern "C" void SimGearSPMInitialize(void) { OSGSPMRegisterPlugins(); }
