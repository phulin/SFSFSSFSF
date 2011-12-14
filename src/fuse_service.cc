#include <sfsfssfsf.h>
#include <fuse_service.h>
extern "C"{
#include "sha256.h"
}
using namespace std;

// list of free files
list <string> free_list;
// hash->rpath
map <string, string> key_rpath_map;
// hash->apath
map <string, string> key_apath_map;

string audiofile_list_file;
string superblock_file;

#define apath(key) key_a_path_map[key]

string to_hex(unsigned char s) {
    stringstream ss;
    ss << hex << (int) s;
    return ss.str();
} 

//discard 4 lowest bits; use remaining static hash to indentify file.
static string sha256(string line)
{
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, line.c_str(), line.length());
	SHA256_Final(hash, &sha256);

	string output = "";    
	for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
		output += to_hex(hash[i]);
	}
	return output;
}

// works on file, not string
static string sha256sum(string path){
	
	debug_print("In sha256");
	ostringstream command;
	static uint16_t pcm_buf[SFSFSSFSF_CHUNK];
	static struct pstat pfi;
	int i;
	
	command << "ffmpeg -i \"" << path << "\" -f u16le pipe:";
	FILE *pipein = popen(command.str().c_str(), "r");
	if (!pipein) err("Could not open ffmpeg");
	decode_bits(pipein, (uint8_t *) &pfi, sizeof(struct pstat));
	
	fread(pcm_buf,2,SFSFSSFSF_CHUNK,pipein);
	pclose(pipein);
	
	for (i=0;i<SFSFSSFSF_CHUNK;i++)
		pcm_buf[i] = pcm_buf[i]>>4;
	pcm_buf[SFSFSSFSF_CHUNK] =0;//null terminated
	// TODO: send to Crypt thread for result.
#ifdef DEBUG
	cout << "sha256 sum for file:"<<path<<endl;
	cout << sha256(string((char *)pcm_buf))<<endl;	
#endif
	return sha256(string((char *)pcm_buf));
}

// _init()_       starts all threads (called by _fuse_main_), parses M3U playlist for non-superblock files
static void *fuse_service_init (struct fuse_conn_info *conn)
{
	debug_print("In init()\n");
	// TODO: Start up Encryption Threads
	
	string line;
	map<string, string> key_a_path_map;
	ifstream playlist_file (audiofile_list_file.c_str());
	debug_print("init 1\n");
	while(playlist_file.good()){
		getline(playlist_file, line);
		apath(sha256sum(line)) = line;
#ifdef DEBUG
		cout<<"Playlist file line:"<<line<<endl;
#endif
	}
	debug_print("init 2\n");
	playlist_file.close();
	// TODO: add things to parse in from the superblock;
	string rootfile, freefile;
	ifstream SuperBlock(superblock_file.c_str());
	// TODO: decrypt rest of superblock before trying to use it.
	SuperBlock.seekg(SB_CRYPT_HDR_LENGTH);
	SuperBlock >> rootfile >> freefile;

	










	//	ifstream pathmap(path(rootfile).c_str());
	
	//	while()

	//key_a_path_map: Key->afile_path
	
	debug_print("init 3\n");
 

	// ifstream in(freefile.c_str());
	
	
	// while(!in.eof()){
	// 	debug_print("init 3.5\n");
	// 	getline(in, line);
	// 	FreeList.push_front(line);
	// }


	debug_print("init 4\n");
	//MapOfDirFiles[key].push_back(filename);
	return NULL;
}
// _access()_      do nothing. Assume if we can decrypt, we can access. Maybe better not to implement, in case of readonly? (what's the right answer here?)

static int fuse_service_access (const char *path, int mask) {return -E_SUCCESS;}

// _create()_      add to directory special file, and remove from last freefile specialfile in SLL (singly-linked list). change dirfile

static int fuse_service_create (const char *path, mode_t mode, struct fuse_file_info *fi)
{

	debug_print("In create()\n");
	string afile = free_list.front();
	free_list.pop_front();
	string key = sha256sum(afile);
	SFSFSSFSF_File free_file(afile, NULL);
	free_file.fsync();
	key_rpath_map[key] = string(path);
	
	return -E_SUCCESS;
}


// _mkdir()_       Create a new directory special file, add it to the parent directory's special file. change dirfile
// _unlink()_      change dirfile, add to freelist.
// _rmdir()_       Removes empty directory. May need to return //-ENOTEMPTY//: Directory not empty. Change dirfile, add to freelist
// _rename()_      change dirfile. Don't know how to deal with renaming to different level directory. Maybe we just don't support this.
// _destroy()_     takes return value of init. Should do an _fsync_, then shutdown all ThreadPools

// _getattr()_     if (seebelow),query Data structures above, fill in stbuf. Return //-ENOENT// if file does not exist.

static int fuse_service_getattr(const char *path, struct stat *stbuf)
{
	debug_print("In getattr()\n");
	memset(stbuf, 0, sizeof(struct stat));
    if(strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }
    else if(strcmp(path, "/file") == 0) {
        stbuf->st_mode = S_IFREG | 0777;
        stbuf->st_nlink = 1;
        stbuf->st_size = 0; // FIXME: Implement
    }
    else
        return -ENOENT;

	return -E_SUCCESS;
}

// _open()_        returns //-EACCES//, but not really. returns _path_exists()_
static int fuse_service_open(const char *path, struct fuse_file_info *fi)
{
	debug_print("In open()\n");
	try {
		SFSFSSFSF_File *f = new SFSFSSFSF_File(superblock_file, NULL);
		fi->fh = (uint64_t)f;
		return -E_SUCCESS;
	}
	catch (char *e) { return print_err(e); }
  
}

// _read()_        (following maintains argument order) from rfile at //path//, fill //buf//, with //size// bytes, starting from //offset// to //offset//+size. Read directly from dirty cache if available.
static int fuse_service_read(const char *path, char *buf, size_t size,
                             off_t offset, struct fuse_file_info *fi)
{
	debug_print("In read()\n");
	SFSFSSFSF_File *f = (SFSFSSFSF_File *)(fi->fh);
	debug_print("1\n");
	try {
		f->read(offset, size, (uint8_t *)buf);
		debug_print("2\n");
	}
	catch (char *e) {
		debug_print("3\n");
		return print_err(e); 
	}
	
 
	return 0;
}

static int fuse_service_write(const char *path, const char *buf, size_t size,
                              off_t offset, struct fuse_file_info *fi)
{
	debug_print("write()\n");
	SFSFSSFSF_File *f = (SFSFSSFSF_File *)(fi->fh);

	try { f->write(offset, size, (uint8_t *)buf); }
	catch (char *e) { return print_err(e); }

	return 0;
}

//// queue an _fsync_ if the time is right (what this means is up for determination).
// _fsync()_       write dirty cache. AKA batch job to encryption threads.
static int fuse_service_fsync(const char *path, int sync_metadata, struct fuse_file_info *fi)
{
	debug_print("In fsync()\n");
	SFSFSSFSF_File *f = (SFSFSSFSF_File *)(fi->fh);
	try { f->fsync(); }
	catch (char *e) {
		print_err(e);
		return -1;
	}

	return 0;
}

static int fuse_service_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                             off_t offset, struct fuse_file_info *fi)
{
	debug_print("In readdir()\n");
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	
	map<string, string>::iterator it;
	for (it = key_rpath_map.begin(); it != key_rpath_map.end(); it++) {
		// buffer full -> break
		if (filler(buf, (*it).second.c_str(), NULL, 0))
			break;
	}

	return 0;
}

void fuse_service_ops(struct fuse_operations *ops)
{
	debug_print("In ops()\n");
	ops->init = fuse_service_init;
	ops->getattr = fuse_service_getattr;
	ops->readdir = fuse_service_readdir;
	ops->open = fuse_service_open;
	ops->read = fuse_service_read;
	ops->write = fuse_service_write;
	ops->fsync = fuse_service_fsync;
	ops->access = fuse_service_access;
	ops->create = fuse_service_create;
}
