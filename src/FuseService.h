#ifndef HAVE_FUSESERVICE_H
#define HAVE_FUSESERVICE_H

#define E_SUCCESS 0

#include <map>
#include <string>
#include <iostream>

using namespace std;

map <string, vector<string> > MapOfDirFiles;
map <string, int> DirFileDirtyBitmap;



#endif //HAVE_FUSESERVICE_H
