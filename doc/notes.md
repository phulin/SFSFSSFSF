FIRST STEPS:
============
* Decide on our favorite lossless format (codec+container), where favorite means easiest to work with.
* Look at libffmpeg for extracting shit (how ubiquitous is it, on systems where people listen to audio files?)
* Figure out how to extract an ALAC stream from an MPEG-4 part 14 container
* extract the low-order bits from several ALAC files and run them through ent to see if they are random (as we expect)
* store a short text file in the low-order bits of an ALAC file (no encryption)
* mount the low-order bits of a single text file as a very basic filesystem using FUSE (use mmap?)

NEXT STEPS:
===========
* implement our ext2 variant atop ALAC files
* implement random-access gzip (see zran.c) atop ext2 atop ALAC files
  * decide on a file-format for random-access gzip
* use mmap and the audio codec to do read-write efficiently in the filesystem (encoding and decoding on the fly)
* get the encryption right (key derivation, HMAC, correct modes, etc)
  * use mlock(2) to prevent paging-out of sensitive data
  * acquire copies of "Secure Programming Cookbook for C and C++" (O'Reilly), "Cryptography Engineering" (Wiley), and "Applied Cryptography" (Wiley)

ICING:
======
* multithreading
* fix wtimes/atimes so that we don't give ourselves away
* efficient read caching (right now, optimized for memory)
* access in hidden filesystem does the right thing
* make it so afilenames are not the hashing key. Anything else is more complicated:
** doing it by hash of upper-order bits leaves question of collision between essentially same afiles, plus it's slow.
** doing it by hash of written (including lower order bits) afile requires ordering of writes of dirfiles, plus it's slow.
* more than one rfile/afile (tailpacking)
* more than one afile/rfile (large files)
* better parsing of the m3u files (strict structure, currently)

RESOURCES:
==========
* http://mij.oltrelinux.com/devel/autoconf-automake/
* Driver/Fuse/FuseService.cpp of TrueCrypt Source
** Main/Unix/Main.cpp
** Core/Unix/CoreUnix.cpp (look for Mount, FuseService)
** Driver/Fuse/FuseService.cpp
** Main/TextUserInterface.cpp (asks for password, keyfile, etc)
* http://www.cplusplus.com/reference/iostream/istream/getline/ (parsing m3u file)
* http://stuff.mit.edu/iap/2009/fuse/fuse.ppt

STRUCTURE:
==========
* _afile_ refers to audio files
* _rfile_/_rdir_ refers to real files/dirs
* No symlink/hardlink support--strictly a dumb filesystem
* Following system may not scale if we do more than one _rfile_ per _afile_ (start having to deal with dependencies--hard). Thus, for now, 1 rfile/afile.

On Disk Data:
-------------
Superblock has encrypt_info structure at head, followed by pointer to root dir specialfile, and pointer to root freefile specialfile
all other files are either regular files, dirfiles, or freefiles.

Regular File:
-------------
Nothing special. Base case for compression+encryption.

Freefile Specialfile:
---------------------
1st filename is name of next freefile specialfile or empty string
next N filenames are filenames of freefiles.

Dirfile Specialfile:
--------------------
List of names of (all subdirectories, and all regular files) directly in this directory.

In Memory Data:
---------------
* _stl::set_ of simplified FS paths of rfiles
* _set_ (on key of simplified path) of file writes to be synced (last write wins priority)
* cache writes, not reads, for now.
* cache freefile data into seperate structures, singly-linked.
* cache dirfile data

Encryption ThreadPool:
----------------------
* waits for FUSE to send requests (some sort of MPI--consider boost?)
* Requests Handled:
* compress+encrypt (path, buf)->datastream (for encoding into alac)


FUSE Thread:
------------
* single thread, so that we avoid locking problems. Can use multithreaded encryption for things like _fsync_. Otherwise just calls out to one encryption thread.
* By using only one thread, we won't handle new FS calls while handling something like a _fsync_
* DEBUGGING: may need to print out paths until we know how fuse handles relative paths (simplifies for us, or what?)

SFSFSSFSF Functions:
--------------------
* _path_exists()_           Check if path (dir or rfile) exists. Returns *-E_SUCCESS*/*E_SUCCESS* (0) if okay, otherwise *-ENOENT*
* _encode_datastream()_     Take (apath, stream) (from encryption thread), and run it through ffmpeg to disk.

FUSE Functions:
---------------
* _init()_       starts all threads (called by _fuse_main_), parses M3U playlist for non-superblock files
* _access()_      do nothing. Assume if we can decrypt, we can access. Maybe better not to implement, in case of readonly? (what's the right answer here?)

    return 0;
    
* _create()_      add to directory special file, and remove from last freefile specialfile in SLL (singly-linked list). change dirfile
* _mkdir()_       Create a new directory special file, add it to the parent directory's special file. change dirfile
* _unlink()_      change dirfile, add to freelist.
* _rmdir()_       Removes empty directory. May need to return *-ENOTEMPTY*: Directory not empty. Change dirfile, add to freelist
* _rename()_      change dirfile. Don't know how to deal with renaming to different level directory. Maybe we just don't support this.



* _destroy()_     takes return value of init. Should do an _fsync_, then shutdown all ThreadPools
* _getattr()_     if (seebelow),query Data structures above, fill in stbuf. Return *-ENOENT* if file does not exist.

    if( (err = path_exists(path)) ) return err;

* _open()_        returns *-EACCES*, but not really. returns _path_exists()_
* _read()_        (following maintains argument order) from rfile at *path*, fill *buf*, with *size* bytes, starting from *offset* to *offset*+size. Read directly from dirty cache if available.

    if offset>filesize return 0;
    if offset+size>filesize return filesize-offset;
    else return size;

* _readdir()_     Something like below.

    if( (err = path_exists(path)) ) return err;
    filler (buf, ".", NULL, 0);
    filler (buf, "..", NULL, 0);
    filler (buf, "bazbar", NULL, 0);
    filler (buf, "foobar", NULL, 0);

* _write()_       if offset is not zero, don't have a good way to determine clobbering order. Thus, force a _fsync_ if there is an offset write.
** If there is nonzero offset: read original file up to offset, then copy partial into new buffer, append given buffer[0:size].
** if offset==0: write given buffer[0:size]
** queue an _fsync_ if the time is right (what this means is up for determination).
* _fsync()_       write dirty cache. AKA batch job to encryption threads.