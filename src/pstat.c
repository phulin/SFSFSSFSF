#include <pstat.h>

void pstat_write(struct stat *s, struct pstat *ps)
{
	ps->pst_mode = s->st_mode;
	ps->pst_size = s->st_size;
	ps->pst_atime = s->st_atime;
	ps->pst_mtime = s->st_mtime;
	ps->pst_ctime = s->st_ctime;
}

void pstat_read(struct pstat *ps, struct stat *s)
{
	s->st_mode = ps->pst_mode;
	s->st_size = ps->pst_size;
	s->st_atime = ps->pst_atime;
	s->st_mtime = ps->pst_mtime;
	s->st_ctime = ps->pst_ctime;
}
