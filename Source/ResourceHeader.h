#ifndef __RESOURCE_HEADER_H
#define __RESOURCE_HEADER_H

#include <stdint.h>

#define RESOURCE_NOT_DELETED 0xBADA55C0DE151337
#define RESOURCE_DELETED     0x0U

#ifdef __cplusplus
 extern "C" {
#endif

   struct ResourceHeader {
     uint32_t magic; // 0xDADADEED
     uint32_t size;
#ifdef OWL_ARCH_L4
     uint64_t is_deleted = RESOURCE_NOT_DELETED;
#endif
     char name[24];
   };

#ifdef __cplusplus
}
#endif

#endif /* __RESOURCE_HEADER_H */
