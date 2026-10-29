#pragma once
#ifdef __cplusplus
extern "C" {
#endif
const char *OSGSPMVersion(void);
/* Call once before loading models or fonts; retains/registers static plugins. */
void OSGSPMRegisterPlugins(void);
#ifdef __cplusplus
}
#endif
