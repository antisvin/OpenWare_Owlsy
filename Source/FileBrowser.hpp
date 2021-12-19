#ifndef __FILE_BROWSER_HPP__
#define __FILE_BROWSER_HPP__

#include "fatfs.h"
#include "string.h"
#include "message.h"
#include "BitState.hpp"

/*
 * Class for fetching a subset of files in a directory
 * Note: max_files must be <= 32 as directory flags are
 * stored in a bitmap
 */
template <size_t max_files, class DirMap = BitState32>
class FileBrowser {
public:
    enum State {
        FF_RESET,
        FF_OPEN,
        FF_NO_CARD,
    };

    FileBrowser() {
        dir_name[0] = 0;
        reset();
    }
    void reset() {
        state = FF_RESET;
        for (int i = 0; i < max_files; i++) {
            file_names[i][0] = 0;
        }
        dir_flags.clear();
        file_offset = 0;
        end_reached = false;
        num_files = 0;
    }
    bool prev() {
        bool result = false;
        if (file_offset > 0 && ffOpen()) {
            for (int i = 0; i < file_offset - 1; i++) {
                if (!ffNext(nullptr)) {
                    break;
                }
            }
            FILINFO tmp;
            if (ffNext(&tmp)) {
                int i = (--file_offset) % max_files;
                updateFile(i, tmp);
                result = true;
            }
        }
        return result;
    }
    bool next() {
        bool result = false;
        if ((!end_reached || file_offset < num_files - (int)max_files) && ffOpen()) {
            // Skip files until offset is reached
            for (int i = 0; i < file_offset + max_files; i++) {
                if (!ffNext(nullptr)) {
                    break;
                }
            }
            // Read next file info
            FILINFO tmp;
            if (ffNext(&tmp)) {
                int i = (file_offset++) % max_files;
                updateFile(i, tmp);
                result = true;
            }
            else {
                end_reached = true;
                num_files = file_offset;
                //if (num_files > 0)
                    num_files += max_files;
                file_offset++;
                result = true;
            }
        }
        else if (file_offset < num_files - 1) {
            file_offset++;
            result = true;
        }
        return result;
    }
    void openDir(bool root = true) {
        if (!root) {
            strncpy(dir_name, file_names[file_offset % max_files], 20);
        }
        else {
            dir_name[0] = 0;
        }
        if (ffOpen()) {
            reset();
            FILINFO tmp;
            for (int i = 0; i < max_files; i++) {
                if (ffNext(&tmp)) {
                    updateFile(i, tmp);
                }
                else {
                    // Might be due to an error here too?..
                    end_reached = true;
                    num_files = i;
                    break;
                }
            }
            if (state == FF_RESET)
                state = FF_OPEN;
            ffClose();
        }
        else {
            state = FF_NO_CARD;
        }
    }
    const char* getDirName() const {
        return dir_name;
    }
    bool isDir(size_t index) const {
        if (index < file_offset || index >= file_offset + max_files)
            return false;
        else
            return dir_flags.get(index % max_files);
    }
    bool isEndReached() const {
        return end_reached;
    }
    State getState() const {
        return state;
    }
    uint32_t getIndex() const {
        return file_offset;
    }
    const char* getFileName(size_t index) const {
        if (index < file_offset || index >= file_offset + max_files)
            return "";
        else
            return file_names[index % max_files];
    }
    uint32_t getNumFiles() const {
        return num_files;
    }
    bool isRoot() const {
        return dir_name[0] == 0;
    }

private:
    char full_path[64] = "";
    char dir_name[21] = "";
    DIR dir CACHE_ALIGNED;
    char file_names[max_files][21];
    DirMap dir_flags; // Bitmap field
    int num_files;
    int file_offset;
    bool end_reached;
    static constexpr const char* default_dir = "0:/OWL/storage";
    State state;

    /*
     * Open current active directory and scroll to current index
     */
    bool ffOpen() {
        if (f_mount(&SDFatFS, (TCHAR const*)SDPath, 1) == FR_OK) {
            const char* new_dir;
            if (dir_name[0] == 0) {
                new_dir = default_dir;
            }
            else {
                strcpy(full_path, default_dir);
                strncat(full_path, "/", 1);
                strncat(full_path, dir_name, 21);
                new_dir = full_path;
            }
            if (f_opendir(&dir, new_dir) == FR_OK) {
                return true;
            }
        }
        return false;
    }
    /*
     * Move to next file. If dst is given, copy next file to it
     */
    bool ffNext(FILINFO* dst) {
        FILINFO tmp;
        if (f_readdir(&dir, &tmp) == FR_OK && tmp.fname[0] != 0) {
            if (dst != nullptr)
                *dst = tmp;
            return true;
        }
        return false;
    }
    void ffClose() {
        f_closedir(&dir);
    }
    void updateFile(int i, FILINFO& tmp) {
        strncpy(file_names[i], tmp.fname, 20);
        dir_flags.set(i, tmp.fattrib & AM_DIR);
    }
};

#endif
