#ifndef MYDIR_H
#define MYDIR_H

struct myDir{
    int type;  // 0 - директория, 1 - файл.
    char name[256] = {0};
    int isLoaded;
};

#endif // MYDIR_H
