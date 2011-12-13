#ifndef HAVE_FUSESERVICE_H
#define HAVE_FUSESERVICE_H

#define E_SUCCESS 0

#include <map>
#include <string>
#include <iostream>

using namespace std;

//hash->path
map <string, string> MapOfAllFiles;
//hash->vector<hash>
map <string, vector<string> > MapOfDirFiles;
//hash->int (boolean)
map <string, int> DirFileDirtyBitmap;



#endif //HAVE_FUSESERVICE_H
