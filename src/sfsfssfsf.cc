// sfsfssfsf.cc: Implementation of main filesystem
// TODO: preserve metadata
// TODO: check low-order bits for entropy
// TODO: sliding-window entropy estimator
// TODO: be more flexible with atypical ALAC numchannels/etc

#include <sfsfssfsf.h>

static string superblock_file, audiofile_list_file;

static int sfsfssfsf_open(const char *path, struct fuse_file_info *fi)
{
	try {
		SFSFSSFSF_File *f = new SFSFSSFSF_File(superblock_file.c_str(), NULL);
		fi->fh = (uint64_t)f;
	}
	catch (char *e) {
		print_err(e);
		return -1;
	}

	return 0;
}

static int sfsfssfsf_getattr(const char *path, struct stat *stbuf)
{
	return -1;
}

static int sfsfssfsf_read(const char *path, char *buf, size_t size,
                          off_t offset, struct fuse_file_info *fi)
{
	SFSFSSFSF_File *f = (SFSFSSFSF_File *)(fi->fh);
	try {
		f->read(offset, size, (uint8_t *)buf);
	}
	catch (char *e) {
		print_err(e);
		return -1;
	}

	return 0;
}

// TODO: implement for real
static int sfsfssfsf_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                             off_t offset, struct fuse_file_info *fi)
{
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, "file", NULL, 0);

	return 0;
}

// TODO: Add threadpool initialization stuff here
static void* sfsfssfsf_init(struct fuse_conn_info *conn)
{

	// TODO: return value is passed as the sole arguement of destroy();
	// TODO: return value is also set as fuse_context->private_data, a pointer to useful info that file operations might need. fuser_context = fuse_get_context()
	return NULL;
}


int main(int argc, char *argv[])
{

	struct fuse_operations ops;
	ops.getattr = sfsfssfsf_getattr;
	ops.open = sfsfssfsf_open;
	ops.read = sfsfssfsf_read;
	ops.readdir = sfsfssfsf_readdir;
	ops.init = sfsfssfsf_init;

	struct fuse_args args = FUSE_ARGS_INIT(0, NULL);

	
	/********** USAGE ********/
	if (argc < 4) {
		fprintf(stderr, "Usage: %s superblock_file playlist.m3u fs-args...", argv[0]);
		return 1;
	}

	/*************************/

	fuse_opt_add_arg(&args, argv[0]);	
	superblock_file = argv[1];
	audiofile_list_file = argv[2];

	for (int i = 3; i < argc; i++) 
		fuse_opt_add_arg(&args, argv[i]);

	return fuse_main(args.argc, args.argv, &ops, NULL);
}
