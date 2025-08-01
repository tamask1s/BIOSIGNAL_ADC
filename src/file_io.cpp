#if not defined(ARDUINO)

#include <string>
#include <vector>
#include <dirent.h>
#include <fstream>

#include "file_io.h"

bool read_buffer(const char* filename, std::vector<char>& buffer)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return false;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    buffer.resize(size);
    if (!file.read(buffer.data(), size))
        return false;

    file.close();
    return true;
}

bool get_directory_files(const char* path, std::vector<std::string>& files)
{
    DIR* dir = opendir(path);
    if (!dir)
        return false;

    dirent* entry;
    while ((entry = readdir(dir)) != nullptr)
        files.push_back(entry->d_name);

    closedir(dir);
    return true;
}

bool write_buffer(const char* filename, const std::vector<char>& buffer)
{
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file.is_open())
        return false;

    if (!file.write(buffer.data(), buffer.size()))
        return false;

    file.close();
    return true;
}

#else /// ARDUINO

#include "FS.h"
#include "SPIFFS.h"
#include <string>
#include <vector>
#include "file_io.h"

#define FORMAT_SPIFFS_IF_FAILED true
static bool littlefs_inited = false;

bool write_buffer(const char* filename, const std::vector<char>& buffer)
{
    if (!littlefs_inited)
    {
        littlefs_inited = true;
        if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
        {
            Serial.print("Mount Failed while writing file: ");
            Serial.println(filename);
            return false;
        }
    }

    File file = SPIFFS.open(filename, "w");
    if (!file)
    {
        Serial.println("Failed to open file for writing");
        return false;
    }

    if (file.write((const uint8_t*)buffer.data(), buffer.size()) != buffer.size())
    {
        Serial.println("Failed to write buffer to file");
        file.close();
        return false;
    }

    file.close();
    return true;
}

bool read_buffer(const char* filename, std::vector<char>& buffer)
{
    if (!littlefs_inited)
    {
        littlefs_inited = true;
        if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
        {
            Serial.print("Mount Failed while reading file: ");
            Serial.println(filename);
            return false;
        }
    }

    File file = SPIFFS.open(filename, "r");
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return false;
    }

    size_t size = file.size();
    buffer.resize(size);
    if (file.readBytes(buffer.data(), size) != size)
    {
        Serial.println("Failed to read buffer from file");
        file.close();
        return false;
    }

    file.close();
    return true;
}

#endif /// ARDUINO
