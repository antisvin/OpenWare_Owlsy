#ifndef __BOOT_COMMAND_HPP__
#define __BOOT_COMMAND_HPP__

#include <inttypes.h>
#include <string.h>
#include "device.h"
#include "message.h"

enum CommandType : uint32_t {
    CMD_WRITE_PATCH = 0x01, // Write patch from specific address in memory
    CMD_ERASE_BLOCK = 0x02, // Erase a single storage block
    CMD_ERASE_ALL   = 0x04, // Erase whole patch storage
};

/*
 * Certain operations have to be performed in bootloader because QSPI flash on Daisy
 * can't be written while we run firmare on it.
 */

class BootCommand {
public:
    BootCommand(CommandType cmd, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
        args[0] = OWLBOOT_COMMAND_NUMBER;
        args[1] = uint32_t(cmd);
        args[2] = arg1;
        args[3] = arg2;
        args[4] = arg3;
    };

    static BootCommand writePatch(uint32_t index, uint32_t src, uint32_t size) {
        BootCommand cmd = BootCommand(CMD_WRITE_PATCH, index, src, size);
        return cmd;
    }

    static BootCommand eraseBlock(uint32_t index) {
        BootCommand cmd = BootCommand(CMD_ERASE_BLOCK, index, 0, 0);
        return cmd;
    }

    static BootCommand eraseAll() {
        BootCommand cmd = BootCommand(CMD_ERASE_ALL, 0, 0, 0);
        return cmd;
    }

    void store() {
        ASSERT(sizeof(*this) == 20, "Invalid object size");
        memcpy(OWLBOOT_COMMAND_ADDRESS, this, 20);
    }
private:
    uint32_t args[5];
};


#endif