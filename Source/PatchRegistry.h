#ifndef __PatchRegistry_h__
#define __PatchRegistry_h__

#include "device.h"
#include "PatchDefinition.hpp"
#include "FlashStorageBlock.hpp"
#include "ResourceHeader.h"
#include "FlashStorage.h"


class PatchRegistry {
public:
  void init();
  /* const char* getName(unsigned int index); */
  const char* getPatchName(unsigned int index);
  const char* getResourceName(unsigned int index);
  PatchDefinition* getPatchDefinition(unsigned int index);
  unsigned int getNumberOfPatches();
  void registerPatch(PatchDefinition* def);
  void setDynamicPatchDefinition(PatchDefinition* def){
    dynamicPatchDefinition = def;
  }
  ResourceHeader* getResource(uint8_t index);
  ResourceHeader* getResource(const char* name);
  StorageBlock* getPatchBlock(uint8_t index);
  void store(uint8_t index, uint8_t* data, size_t size);  
private:
  Storage* storage;
  bool isPresetBlock(StorageBlock block);
  StorageBlock patchblocks[MAX_NUMBER_OF_PATCHES];
  StorageBlock resourceblocks[MAX_NUMBER_OF_RESOURCES];
  PatchDefinition* defs[MAX_NUMBER_OF_PATCHES];
  uint8_t patchCount;
  PatchDefinition* dynamicPatchDefinition;
};


extern PatchRegistry registry;

#endif // __PatchRegistry_h__
