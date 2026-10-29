#include "OpenSceneGraphSPM.h"
#include <osg/Version>
#include <osgDB/Registry>
USE_OSGPLUGIN(dds)
USE_OSGPLUGIN(freetype)
USE_OSGPLUGIN(imageio)
USE_OSGPLUGIN(osg)
USE_OSGPLUGIN(osg2)
USE_OSGPLUGIN(rgb)
USE_OSGPLUGIN(tga)
USE_OSGPLUGIN(txf)
extern "C" void OSGSPMRegisterStaticWrappers(void);
extern "C" const char *OSGSPMVersion(void) { return osgGetVersion(); }
extern "C" void OSGSPMRegisterPlugins(void) { OSGSPMRegisterStaticWrappers(); (void)osgDB::Registry::instance(); }
