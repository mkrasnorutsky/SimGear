#include "SimGearSPM.h"
#include <OpenSceneGraphSPM.h>
#include <simgear/version.h>
#include <osgDB/Registry>
USE_OSGPLUGIN(ac)
USE_OSGPLUGIN(btg)
USE_OSGPLUGIN(stg)
USE_OSGPLUGIN(spt)
USE_OSGPLUGIN(gltf)
USE_OSGPLUGIN(pgt)

#define SG_SPM_STRING_IMPL(x) #x
#define SG_SPM_STRING(x) SG_SPM_STRING_IMPL(x)
extern "C" const char *SimGearSPMVersion(void) { return SG_SPM_STRING(SIMGEAR_MAJOR_VERSION) "." SG_SPM_STRING(SIMGEAR_MINOR_VERSION) "." SG_SPM_STRING(SIMGEAR_PATCH_VERSION); }
extern "C" void SimGearSPMInitialize(void) { OSGSPMRegisterPlugins(); }
