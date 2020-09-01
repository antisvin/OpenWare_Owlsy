#ifndef __PatchRegistry_h__
#define __PatchRegistry_h__

#include "device.h"
#include "PatchDefinition.hpp"
#include "FlashStorageBlock.hpp"
#include "ResourceHeader.h"
#include "FlashStorage.h"


template<class Storage, class StorageBlock>
class ResourceRegistry {
public:
  ResourceRegistry() = default;
  void init(Storage* flash_storage);
  /* const char* getName(unsigned int index); */
  const char* getPatchName(unsigned int index);
  const char* getResourceName(unsigned int index);
  PatchDefinition* getPatchDefinition(unsigned int index);
  unsigned int getNumberOfPatches();
  unsigned int getNumberOfResources();
  bool hasPatches();
  void registerPatch(PatchDefinition* def);
  void setDynamicPatchDefinition(PatchDefinition* def){
    dynamicPatchDefinition = def;
  }
  ResourceHeader* getResource(uint8_t index);
  ResourceHeader* getResource(const char* name);
  unsigned int getSlot(ResourceHeader* resource);
  void* getData(ResourceHeader* resource);
  void store(uint8_t index, uint8_t* data, size_t size);
  void setDeleted(uint8_t index);
  StorageBlock* getPatchBlock(uint8_t index);
private:
  Storage* storage;
  bool isPresetBlock(StorageBlock block);
  StorageBlock patchblocks[MAX_NUMBER_OF_PATCHES];
  StorageBlock resourceblocks[MAX_NUMBER_OF_RESOURCES];
  PatchDefinition* defs[MAX_NUMBER_OF_PATCHES];
  uint8_t patchCount, resourceCount;
  PatchDefinition* dynamicPatchDefinition;
};


#ifndef DAISY
using PatchRegistry = ResourceRegistry<FlashStorage, PatchStorageBlock>;
extern PatchRegistry registry;
#define settings_registry registry
#define patch_registry registry
#else
using PatchRegistry = ResourceRegistry<PatchStorage, QspiStorageBlock>;
using SettingsRegistry = ResourceRegistry<FlashStorage, FlashStorageBlock>;
template class ResourceRegistry<PatchStorage, PatchStorageBlock>;
template class ResourceRegistry<FlashStorage, FlashStorageBlock>;
extern PatchRegistry patch_registry;
extern SettingsRegistry settings_registry;
#endif

#endif // __PatchRegistry_h__
