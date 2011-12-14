// sfsfssfsf.cc: Implementation of main filesystem
// TODO: preserve metadata
// TODO: check low-order bits for entropy
// TODO: sliding-window entropy estimator
// TODO: be more flexible with atypical ALAC numchannels/etc

#include <sfsfssfsf.h>
#include <fuse_service.h>

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
	fuse_service_ops(&ops);

	struct fuse_args args = FUSE_ARGS_INIT(0, NULL);

	
	/********** USAGE ********/
	if (argc < 4) {
		fprintf(stderr, "Usage: %s superblock_file playlist.m3u fs-args...\n", argv[0]);
		return 1;
	}

	/*************************/

	fuse_opt_add_arg(&args, argv[0]);	
	superblock_file = string(argv[1]);
	audiofile_list_file = string(argv[2]);

	for (int i = 3; i < argc; i++) 
		fuse_opt_add_arg(&args, argv[i]);

	return fuse_main(args.argc, args.argv, &ops, NULL);
}
