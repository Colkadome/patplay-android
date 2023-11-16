//
// Created by joe on 16/11/2023.
//

#include "Save.h"

#include <fstream>
#include <string>

// We just save and load to a file for now,
// because i CBF using the android SDK. Some of them save to files under "dataPath" anyway.

std::string getSaveFilePath(const char * dataPath) {
    std::string filePath(dataPath);
    filePath += "/save.txt";
    return filePath;
}

void Save::init(const char * dataPath) {
    std::ifstream file(getSaveFilePath(dataPath));
    if (file.is_open()) {
        file >> pat_count_;
        file.close();
    }
}

unsigned int Save::getPatCount() const {
    return pat_count_;
}

void Save::incrementPatCount(unsigned int pats) {
    pat_count_ += pats;
}

void Save::savePatCount(const char * dataPath) const {
    std::ofstream file(getSaveFilePath(dataPath));
    if (file.is_open()) {
        file << pat_count_;
        file.close();
    }
}
