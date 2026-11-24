/* HuCard-only probe: explicitly reject file access. CD support will replace
 * this with bounded frontend file handles; no Linux VFS inside the blob. */
#include <streams/file_stream.h>
#include <stddef.h>
void filestream_vfs_init(const struct retro_vfs_interface_info *i){(void)i;}
RFILE *filestream_open(const char *p,unsigned m,unsigned h){(void)p;(void)m;(void)h;return NULL;}
int64_t filestream_get_size(RFILE *s){(void)s;return -1;}
int64_t filestream_seek(RFILE *s,int64_t n,int w){(void)s;(void)n;(void)w;return -1;}
int64_t filestream_tell(RFILE *s){(void)s;return -1;}
int64_t filestream_read(RFILE *s,void *p,int64_t n){(void)s;(void)p;(void)n;return -1;}
int filestream_close(RFILE *s){(void)s;return -1;}
char *filestream_gets(RFILE *s,char *p,size_t n){(void)s;(void)p;(void)n;return NULL;}
int64_t filestream_read_file(const char *p,void **buf,int64_t *n)
{(void)p;if(buf)*buf=NULL;if(n)*n=0;return -1;}
