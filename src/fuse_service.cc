#include <sfsfssfsf.h>
#include <fuse_service.h>
extern "C"{
#include "sha256.h"
}
using namespace std;

#define CREATEFS

#define apath(key) key_apath_map[key]


// list of free files
list <string> free_list;
// hash->rpath
map <string, string> key_rpath_map;
// hash->apath
map <string, string> key_apath_map;

map <string, SFSFSSFSF_File*> rpath_fileobj;

string audiofile_list_file;
string superblock_file;


string to_hex(unsigned char s) {
    stringstream ss;
    ss << hex << (int) s;
    return ss.str();
} 

//discard 4 lowest bits; use remaining static hash to indentify file.
static string sha256(string line)
{
	unsigned char hash[SHA256_DIGEST_LENGTH] = {0};
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
string sha256sum(string path){
	
	debug_print("In sha256sum\n");
	ostringstream command;
	static uint16_t pcm_buf[SFSFSSFSF_CHUNK] = {0};
	static struct pstat pfi;
	int i;
	
	command << "ffmpeg -i \"" << path << "\" -f u16le pipe:";
	FILE *pipein = popen(command.str().c_str(), "r");
	if (!pipein) printf("Could not open ffmpeg");
	decode_bits(pipein, (uint8_t *) &pfi, sizeof(struct pstat));
	
	if (!fread(pcm_buf,2,SFSFSSFSF_CHUNK,pipein)){
		err("fread failed" );
		return string("");
	}
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


int deserialize_superblock(){
	debug_print("In deserialize_superblock()\n");
	
	// TODO: add things to parse in from the superblock;
	// TODO: optimize with direct data
	char* buf = new char[SFSFSSFSF_CHUNK * 100];
	memset(buf, 0, SFSFSSFSF_CHUNK * 100);
	stringstream SuperBlock (stringstream::in | stringstream::out);
	SFSFSSFSF_File *f;

	debug_print("deserialize 1\n");
	try {
		f = new SFSFSSFSF_File(superblock_file, string(""));
		
		f->read(SB_CRYPT_HDR_LENGTH, SFSFSSFSF_CHUNK*100, (uint8_t *)buf);
		debug_print("deserialize 2\n");
	}
	catch (char *e) {
		debug_print("deserialize 3\n");
		return print_err(e); 
	}

	SuperBlock << buf;
	debug_print("deserialize a\n");

	// TODO: decrypt rest of superblock before trying to use it.
	int i, key_rpath_map_size, free_list_size;
	string tmp1, tmp2;
	debug_print("deserialize b\n");

	SuperBlock >> key_rpath_map_size;
	SuperBlock >> free_list_size;
	debug_print("deserialize c\n");

	cout<<key_rpath_map_size<<endl<<free_list_size<<endl;
	assert (key_rpath_map_size <= key_apath_map.size());
	assert (free_list_size <= key_apath_map.size());

	debug_print("deserialize d\n");

	
	for (i = 0; i < key_rpath_map_size; i++){
		//SuperBlock>>ws;
		getline(SuperBlock, tmp1);
		getline(SuperBlock, tmp2);
		key_rpath_map[tmp1] = tmp2;
	}
	for (i = 0; i < free_list_size; i++){
		//SuperBlock>>ws;
		getline(SuperBlock, tmp1);
		free_list.push_front(tmp1);
	}

	delete buf;
	delete f;

#ifdef DEBUG
	cout<<"loaded: "<<key_apath_map.size()<<" files"<<endl;
	cout<<"# free files: "<<free_list.size();
#endif
	debug_print("deserialize finished\n");
	return -E_SUCCESS;

}

int serialize_superblock()
{
	debug_print("In serialize_superblock");
	string path = superblock_file;

	string buf, tmp1, tmp2;
	
	stringstream SuperBlock (stringstream::in | stringstream::out);
 
	SuperBlock << key_rpath_map.size() << endl;
	SuperBlock << free_list.size() << endl;
	for(std::map<string, string>::iterator itr = key_rpath_map.begin(), itr_end = key_rpath_map.end(); itr != itr_end; ++itr) {
		tmp1 = itr->first;
		tmp2 = itr->second;
		SuperBlock<<tmp1<<endl<<tmp2<<endl;
	}

	for (list<string>::iterator itr = free_list.begin(), itr_end = free_list.end(); itr != itr_end; ++itr) {
		tmp1 = *itr;
		SuperBlock << tmp1 << endl;
	}
	
	SuperBlock<<'\0';
	while(SuperBlock.peek() != '\0'){
		getline(SuperBlock, tmp1);
		buf+=tmp1+"\n";
	}

	try {
		SFSFSSFSF_File *f = new SFSFSSFSF_File(path, string(""), true);
		f->write(SB_CRYPT_HDR_LENGTH, buf.size(), (uint8_t *)buf.c_str());
		f->fsync();
	}
	catch (char *e) {
		print_err(e);
	}
	debug_print("serialize_superblock finished\n\n");
	return -E_SUCCESS;
}

// _init()_       starts all threads (called by _fuse_main_), parses M3U playlist for non-superblock files
static void *fuse_service_init (struct fuse_conn_info *conn)
{
	debug_print("In init()\n");
	// TODO: Start up Encryption Threads
	
	string line;
	ifstream playlist_file (audiofile_list_file.c_str());

	debug_print("init 1\n");
	while(playlist_file.good()){
		debug_print("reading playlist file\n");
		getline(playlist_file, line);
		if(!ifstream(line.c_str()))
			continue;
		apath(sha256sum(line)) = line;
#ifdef DEBUG
		cout<<"Playlist file line:"<<line<<endl;
#endif
#ifdef CREATEFS
		free_list.push_front(sha256sum(line));
		cout<<"free_list now is size:"<<free_list.size()<<endl;
#endif
	}
	if (!key_apath_map.size()){
		cout<<"No filenames from playlist could be loaded. Specify valid playlist!\n"<<flush;
		exit(-1);
	}
	debug_print("init 2\n");
       	playlist_file.close();

#ifndef CREATEFS
	int err;
	if((err=deserialize_superblock())){
		print_err("createfs in init threw err in deserialize_superblock:\n\tperhaps it's not a valid superblock\n");
	}
#endif	

	return NULL;
}
// _access()_      do nothing. Assume if we can decrypt, we can access. Maybe better not to implement, in case of readonly? (what's the right answer here?)

static int fuse_service_access (const char *path, int mask) {return -E_SUCCESS;}

// _create()_      add to directory special file, and remove from last freefile specialfile in SLL (singly-linked list). change dirfile

static int fuse_service_mknod (const char *path, mode_t mode, dev_t foobar)
{
	debug_print("In create()\n");

	string rpath = string(path);
	if( key_rpath_map.find( rpath ) != key_rpath_map.end() )
		return -ENOENT;
	
	cout<<"path to create:"<<rpath<<"|"<<endl;
	cout<<key_rpath_map.size()<<endl;
	cout<<free_list.size()<<endl;
	debug_print("create 1\n");
	string afile = free_list.front();
	free_list.pop_front();
	debug_print("create 2\n");

	SFSFSSFSF_File *f = new SFSFSSFSF_File(apath(afile), string(""), true);
	rpath_fileobj[rpath] = f;
	debug_print("create 3\n");
	f->fsync();
	debug_print("create 4\n");
	key_rpath_map[string(path)] = afile;
	debug_print("create 5\n\n");
	return serialize_superblock();
}


// _mkdir()_       Create a new directory special file, add it to the parent directory's special file. change dirfile
// _unlink()_      change dirfile, add to freelist.
// _rmdir()_       Removes empty directory. May need to return //-ENOTEMPTY//: Directory not empty. Change dirfile, add to freelist
// _rename()_      change dirfile. Don't know how to deal with renaming to different level directory. Maybe we just don't support this.
// _destroy()_     takes return value of init. Should do an _fsync_, then shutdown all ThreadPools

// _getattr()_     if (seebelow),query Data structures above, fill in stbuf. Return //-ENOENT// if file does not exist.

static int fuse_service_getattr(const char *path, struct stat *stbuf)
{
	string rpath = string(path);
	
	cout<<"getattr for file:"<<rpath<<"|\n";
	memset(stbuf, 0, sizeof(struct stat));
	
	if(rpath=="/") {
		debug_print("getattr 1\n");
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}
	else if( key_rpath_map.find(rpath) != key_rpath_map.end() ){
		debug_print("getattr 2\n");
		SFSFSSFSF_File *f;
		if (rpath_fileobj.find(rpath) != rpath_fileobj.end()){
			f = rpath_fileobj[rpath];
		}
		else{
			f = new SFSFSSFSF_File(apath(key_rpath_map[rpath]), string(""));
			rpath_fileobj[rpath] = f;
		}
		stbuf->st_mode = S_IFREG | 0777;
		stbuf->st_nlink = 1;
		stbuf->st_size = f->get_size();
		debug_print("getattr 3\n");
		
	}
	else{
		debug_print("getattr could not find file\n");	
		return -ENOENT;
	}
	debug_print("getattr 4\n\n");
	
	return -E_SUCCESS;
}

// _open()_        returns //-EACCES//, but not really. returns _path_exists()_
static int fuse_service_open(const char *path, struct fuse_file_info *fi)
{
	string rpath = string(path);
	cout<<"in open(): "<<rpath<<endl<<flush;
       
	if( rpath_fileobj.find(rpath) != rpath_fileobj.end()){
		debug_print("found existing fileobject\n");
		fi->fh = (uint64_t)rpath_fileobj[rpath];
		return -E_SUCCESS;
	}
	
	try {
		debug_print("open 1\n");
		SFSFSSFSF_File *f = new SFSFSSFSF_File(apath(key_rpath_map[rpath]), string(""));
		debug_print("open 2\n");

		rpath_fileobj[rpath] = f;
		debug_print("open 3\n");

		fi->fh = (uint64_t)f;
		debug_print("open 4\n");

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
	debug_print("read 1\n");
	try {
		f->read(offset, size, (uint8_t *)buf);
		debug_print("read 2\n\n");
	}
	catch (char *e) {
		debug_print("read 3\n\n");
		return print_err(e); 
	}
	
	
	return 0;
}

static int fuse_service_write(const char *path, const char *buf, size_t size,
                              off_t offset, struct fuse_file_info *fi)
{
	debug_print("write()\n\n");
	SFSFSSFSF_File *f = (SFSFSSFSF_File *)(fi->fh);

	try { f->write(offset, size, (uint8_t *)buf); }
	catch (char *e) { return print_err(e); }

	return 0;
}

//// queue an _fsync_ if the time is right (what this means is up for determination).
// _fsync()_       write dirty cache. AKA batch job to encryption threads.
static int fuse_service_fsync(const char *path, int sync_metadata, struct fuse_file_info *fi)
{
	debug_print("In fsync()\n\n");
	SFSFSSFSF_File *f = (SFSFSSFSF_File *)(fi->fh);
	try { f->fsync(); }
	catch (char *e) {
		print_err(e);
		return -1;
		debug_print("fuse_fsync failed\n\n");
	}

	return 0;
}

static int fuse_service_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                             off_t offset, struct fuse_file_info *fi)
{
	debug_print("In readdir()\n\n");	
	cout<<key_rpath_map.size()<<endl;
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	
	map<string, string>::iterator it, itend;
	for (it = key_rpath_map.begin(), itend = key_rpath_map.end(); it != key_rpath_map.end(); it++) {
		// buffer full -> break
		if (filler(buf, (*it).first.c_str()+1, NULL, 0))
			break;
	}

	return 0;
}

static int fuse_service_release(const char *path, struct fuse_file_info *fi)
{
	SFSFSSFSF_File *f = (SFSFSSFSF_File *)(fi->fh);
	f->~SFSFSSFSF_File();
	return 0;
}

void fuse_service_ops(struct fuse_operations *ops)
{
	debug_print("In ops()\n\n");
	ops->init = fuse_service_init;
	ops->getattr = fuse_service_getattr;
	ops->readdir = fuse_service_readdir;
	ops->open = fuse_service_open;
	ops->read = fuse_service_read;
	ops->write = fuse_service_write;
	ops->fsync = fuse_service_fsync;
	ops->access = fuse_service_access;
	ops->mknod = fuse_service_mknod;
	ops->release = fuse_service_release;
}
