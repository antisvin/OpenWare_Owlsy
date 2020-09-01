#ifndef __FlashStorage_h__
#define __FlashStorage_h__

#include <device.h>
#include <inttypes.h>
#include "BaseStorage.hpp"
#include "FlashStorageBlock.hpp"


#ifndef DAISY
// Normally we use a single flash storage. Patch/settings storages are are aliases
// for backwards compatibility.
using FlashStorage = BaseStorage<FlashStorageBlock>;
using PatchStorageBlock = FlashStorageBlock;
extern FlashStorage storage;
#define settings_storage storage
#define patch_storage storage

#else
// When Daisy is not in bootloader mode, settings are writable, patch storage is readonly.
// Write functions are enabled by template specialization, so not compiling QspiStorage.cpp
// would keep patch storage readonly on Daisy.
using FlashStorage = BaseStorage<FlashStorageBlock>;
using QspiStorage = BaseStorage<QspiStorageBlock>;
using PatchStorage = QspiStorage;
using PatchStorageBlock = QspiStorageBlock;
extern FlashStorage settings_storage;
extern PatchStorage patch_storage;
#endif /* DAISY */

#endif // __FlashStorage_h__
