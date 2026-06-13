#include <cstddef>

size_t Sys_GetModuleSize(void *pModule);
// Use "?" for wildcard. Example "FF FF FF ? FF FF ?? FF"
void *Sys_FindPattern(void *pModule, const char *pattern, void *start = nullptr);