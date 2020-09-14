#ifndef __BaseStorageBlock_hpp__
#define __BaseStorageBlock_hpp__

#include <inttypes.h>

// Note: we can't use start/end page as template parameters, because they come from linker

template<uint32_t alignment=4>
class BaseStorageBlock {
protected:
  uint32_t* header; 
  // one byte of the header is used for block magic and deleted status, 
  // 3 bytes for size (24 bits: max 16MB)
public:
  BaseStorageBlock() : header(nullptr) {};

  BaseStorageBlock(uint32_t* h) : header(h){}

  uint8_t getMagick() const {
    // upper 8 bits
    return header == nullptr ? 0 : (*header) >> 24;
  }

  uint32_t getDataSize() const {
    // lower 24 bits
    uint32_t size = header == nullptr ? 0 : (*header) & 0x00ffffff;
    if(size == 0x00ffffff)
      return 0;
    return size;
  }

  void* getData() const {
    return header == nullptr ? nullptr : (void*)(header+1); // data starts 4 bytes (1 word) after header
  }

  void* getBlock() const {
    return (void*)header;
  }

  bool isWritten() const {
    return (getMagick() & 0xf0) == 0xc0;
  }

  bool isFree() const {
    return header != nullptr && (*header) == 0xffffffff;
  }

  bool isDeleted() const {
    return getMagick() == 0xc0;
  }

  uint32_t getBlockSize() const {
    uint32_t size = 4 + getDataSize();
    // size = (size + (32 - 1)) & -32;  // Round up to 32-byte boundary
    // size = (size + (8 - 1)) & -8;  // Round up to 8-byte boundary
    // size = (size + (4 - 1)) & -4;  // Round up to 4-byte boundary
    size = (size + (alignment - 1)) & -alignment;  // Round up to 4-byte boundary
    return size;
  }

  bool verify() const { // true if block is not null, is written, has valid size, and is not deleted
    return isValidSize() && isWritten() && !isDeleted();
  }
  /* Base class is read-only.
   * Valid size depends on storage type that uses boundaries from linker script, we can't define it here.
   * 
   * Note that we don't use pointers to this class, so virtual methods are not required.
   */
  bool isValidSize() const { return true; };
  bool setDeleted() { return false; };
  bool write(void* data, uint32_t size) { return false; };

  static constexpr uint8_t num_trailing_zeros = __builtin_ctz(alignment);
  // I.e. for alignment = 4, we have 2 trailing zeros
  static constexpr uint32_t alignment_mask    = (1 << num_trailing_zeros) - 1;
  // For alignment = 4, mask is 0b000..011
};

#endif // __BaseStorageBlock_hpp__
