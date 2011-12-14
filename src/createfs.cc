// sfsfssfsf.cc: Implementation of main filesystem
// TODO: preserve metadata
// TODO: check low-order bits for entropy
// TODO: sliding-window entropy estimator
// TODO: be more flexible with atypical ALAC numchannels/etc

#include <sfsfssfsf.h>
#include <fuse_service.h>

extern map <string, string> key_apath_map;

int main(int argc, char *argv[])
{
	/********** USAGE ********/
	if (argc < 3) {
		fprintf(stderr, "Usage: %s superblock_file playlist.m3u\n", argv[0]);
		return 1;
	}

	/*************************/

	superblock_file = string(argv[1]);
	audiofile_list_file = string(argv[2]);

	string line;
	ifstream playlist_file (audiofile_list_file.c_str());

	debug_print("init 1\n");
	while(playlist_file.good()){
		getline(playlist_file, line);
		if(!ifstream(line.c_str()))
			continue;
		key_apath_map[sha256sum(line)] = line;
#ifdef DEBUG
		cout<<"Playlist file line:"<<line<<endl;
#endif
	}
	debug_print("init 2\n");
	playlist_file.close();

	deserialize_superblock();
	serialize_superblock();
	return 0;
}
