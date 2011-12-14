#ifndef HAVE_FUSESERVICE_H
#define HAVE_FUSESERVICE_H

#define E_SUCCESS 0
// TODO: Check this length--superblock crypt header length (bytes).
#define SB_CRYPT_HDR_LENGTH 64

using namespace std;

/*************** Structure of the Superblock *********************
**  | a(384) |  b(65) | c(65)
**
**  a) initialization crypt header
**  b) rootfile sha256 hash + null byte
**  c) freefile sha256 hash + null byte
**
******************************************************************/

//hash->apath
map <string, string> MapOfAllFiles;
//rpath->hash
map <string, string> MapOfAllPaths;
//hash->vector<hash>
map <string, vector<string> > MapOfDirFiles;
////hash->vector<hash>
//map <string, vector<string> > MapOfFreeFiles;
//hash->int (boolean)
map <string, int> FreeFileBitmap;
list <string> FreeList;
//hash->int (boolean)
map <string, int> DirFileDirtyBitmap;

void fuse_service_ops(struct fuse_operations *ops);

#endif //HAVE_FUSESERVICE_H
