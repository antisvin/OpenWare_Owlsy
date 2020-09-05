#ifndef __FlashStorage_h__
#define __FlashStorage_h__

#include <device.h>
#include <inttypes.h>
#include "BaseStorage.hpp"


#ifndef DAISY
#include "FlashStorageBlock.hpp"
using StorageBlock = FlashStorageBlock;

#else
#include "QspiStorageBlock.hpp"
using StorageBlock = QspiStorageBlock;
#endif /* !DAISY */

using Storage = BaseStorage<StorageBlock>;
extern Storage storage;

#endif // __FlashStorage_h__
