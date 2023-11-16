//
// Created by joe on 16/11/2023.
//

#ifndef PAT_PLAY_SAVE_H
#define PAT_PLAY_SAVE_H

class Save {
public:

    inline Save(): pat_count_(0) {}

    void init(const char * dataPath);
    unsigned int getPatCount() const;
    void incrementPatCount(unsigned int pats);
    void savePatCount(const char * dataPath) const;

private:

    unsigned int pat_count_;

};

#endif //PAT_PLAY_SAVE_H
