/* Copyright (C) 2022 Komeil Majidi.*/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <utime.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include "xalloc.h"
#include "main.h"
#include "traverse.h"
#include "leanocrypt.h"
#include "unixcryptlib.h"
#include "platform.h"
#include "gettext.h"

#define _(String) gettext (String)
#define IGNORE_RESULT(x) if ((int)(x)) {;}
static void traverse_files(char **filelist, int count);

/* ---------------------------------------------------------------------- */
/* an "object" for keeping track of a list of inodes that we have seen */

struct inode_dev_s {
  ino_t inode;  /* an inode */
  dev_t dev;    /* a device */
  int success;  /* was encryption/decryption successful for this inode? */
};
typedef struct inode_dev_s inode_dev_t;

/* inode_list: a list of inode/device pairs. inode_num is number of
   nodes in the list, and inode_size is its allocated size */

static inode_dev_t *inode_list = NULL;
static int inode_num = 0;
static int inode_size = 0;

/* error_flags: collect statistics on all error and warning messages
   that occur (not including interactive and verbose messages). */ 

static int key_errors = 0;
static int io_errors = 0;
static int symlink_warnings = 0;
static int hardlink_warnings = 0;
static int isreg_warnings = 0;
static int strict_warnings = 0;

/* add an inode/device pair to the list and record success or failure */
static void add_inode(ino_t ino, dev_t dev, int success) {
  if (inode_list==NULL) {
    inode_size = 100;
    inode_list = (inode_dev_t *)xalloc(inode_size*sizeof(inode_dev_t), cmd.name);
  }
  if (inode_num >= inode_size) {
    inode_size += 100;
    inode_list = (inode_dev_t *)xrealloc(inode_list, inode_size*sizeof(inode_dev_t), cmd.name);
  }
  inode_list[inode_num].inode = ino;
  inode_list[inode_num].dev = dev;
  inode_list[inode_num].success = success;
  inode_num++;
}

/* look up ino/dev pair in list. Return -1 if not found, else 0 if
   success=0, else 1 */
static int known_inode(ino_t ino, dev_t dev) {
  int i;

  /* have we already seen this inode/device pair? */
  for (i=0; i<inode_num; i++) {
    if (inode_list[i].inode == ino && inode_list[i].dev == dev) {
      return inode_list[i].success ? 1 : 0;
    }
  }
  return -1;
}

/* ---------------------------------------------------------------------- */
/* suffix handling */

/* return 1 if filename ends in, but is not equal to, suffix. */
static int has_suffix(char *filename, char *suffix) {
  int flen = strlen(filename);
  int slen = strlen(suffix);
  return flen>slen && strcmp(filename+flen-slen, suffix)==0;
}

/* add suffix to filename. Returns an allocated string, or NULL on
   error with errno set. */
static char *add_suffix(char *filename, char *suffix) {
  static char *outfile;
  int flen = strlen(filename);
  int slen = strlen(suffix);

  outfile = (char *)malloc(flen+slen+1);
  if (!outfile) {
    return NULL;
  }
  strncpy (outfile, filename, flen);
  strncpy (outfile+flen, suffix, slen+1);
  return outfile;
}

/* remove suffix from filename. Returns an allocated string, or NULL
   on error with errno set. */
static char *remove_suffix(char *filename, char *suffix) {
  static char *outfile;
  int flen = strlen(filename);
  int slen = strlen(suffix);

  if (suffix[0]==0 || !has_suffix(filename, suffix)) {
    return strdup(filename);
  }
  outfile = (char *)malloc(flen-slen+1);
  if (!outfile) {
    return NULL;
  }
  strncpy (outfile, filename, flen-slen);
  outfile[flen-slen] = 0;
  return outfile;
}

/* ---------------------------------------------------------------------- */
/* some helper functions */

/* read a yes/no response from the user */
static int prompt(void) {
  char *line;
  FILE *fin;
  int r;

  fin = fopen("/dev/tty", "r");
  if (fin==NULL) {
    fin = stdin;
  }
  
  line = xreadline(fin, cmd.name);
  r = line && (!strcmp(line, "y") || !strcmp(line, "yes"));
  free(line);
  return r;
}

/* check whether named file exists */
static int file_exists(char *filename) {
  struct stat buf;
  int st;

  st = lstat(filename, &buf);

  if (st) {
    return 0;
  } else {
    return 1;
  }
}

/* ---------------------------------------------------------------------- */
/* read a whole directory into a data structure. This is because we
   change directory entries while traversing the directory; this could
   otherwise lead to strange behavior on Solaris. After done with the
   file list, it should be freed with free_filelist. */

static int get_filelist(char *dirname, char*** filelistp, int *countp) {
  DIR *dir;
  struct dirent *dirent;
  char **filelist = NULL;
  int count = 0;

  dir = opendir(dirname);
  if (dir==NULL) {
    fprintf(stderr, "%s: %s: %s\n", cmd.name, dirname, strerror(errno));
    io_errors++;
    *filelistp = NULL;
    *countp = 0;
    return 0;
  }
  
  while ((dirent = readdir(dir)) != NULL) {
    if (strcmp(dirent->d_name, "..")!=0 && strcmp(dirent->d_name, ".")!=0) {
      char *strbuf = (char *)xalloc(strlen(dirname)+strlen(dirent->d_name)+2, cmd.name);
      strcpy (strbuf, dirname);
      strcat (strbuf, "/");
      strcat (strbuf, dirent->d_name);
      filelist = (char **)xrealloc(filelist, (count+1)*sizeof(char *), cmd.name);
      filelist[count] = strbuf;
      count++;
    }
  }
  
  closedir(dir);

  *filelistp = filelist;
  *countp = count;
  return count;
}

static void free_filelist(char** filelist, int count) {
  int i;

  for (i=0; i<count; i++) {
    free(filelist[i]);
  }
  free(filelist);
}

/* ---------------------------------------------------------------------- */

/* file actions for the individual modes. */

/* local signal handler for overwrite mode - catch interrupt signal */
static int sigint_flag = 0;

static void sigint_overwrite(int dummy) {
  static time_t sigint_time = 0;
  int save_errno = errno;
  
  /* exit if two SIGINTS are received in one second */
  if ((time(NULL)-sigint_time) <= 1) {
    fprintf(stderr, _("%s: interrupted.\n"), cmd.name);
    exit(6);
  }

  /* otherwise, schedule to exit at the end of the current file. Note:
     this signal handler is only in use if we're not in cat,
     unixcrypt, or filter mode */
  sigint_time = time(NULL);
  sigint_flag = 1;
  fprintf(stderr, 
  _("Interrupt - will exit after current file.\n"
    "Press CTRL-C twice to exit now (warning: this can lead to loss of data).\n"));
  errno = save_errno;
}

/* this function is called to act on a file in overwrite mode. */
static void action_overwrite(char *infile, char *outfile) {
  int st;
  struct stat buf;
  int do_chmod = 0;
  int r;
  int fd;
  int save_errno;
  int s;

  /* read file attributes */
  st = stat(infile, &buf);
  if (st) {
    fprintf(stderr, "%s: %s: %s\n", cmd.name, infile, strerror(errno));
    io_errors++;
    return;
  }

  /* check whether this file is write protected */
  if ((buf.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0) {
    /* file is write-protected. In this case, we prompt the user to
       see if they want to operate on it anyway. Or if they give the
       "-f" option, we just do it without asking. */
    if (!cmd.force) {
      switch (cmd.mode) {
      case ENCRYPT:
	fprintf(stderr, _("%s: encrypt write-protected file %s (y or n)? "), cmd.name, infile);
	break;
      case DECRYPT: default:
	fprintf(stderr, _("%s: decrypt write-protected file %s (y or n)? "), cmd.name, infile);
	break;
      case KEYCHANGE:
	fprintf(stderr, _("%s: perform keychange on write-protected file %s (y or n)? "), cmd.name, infile);
	break;
      }
      fflush(stderr);
      if (prompt()==0) {
	fprintf(stderr, _("Not changed.\n"));
	add_inode(buf.st_ino, buf.st_dev, 0);
	return;
      }
    }
    /* we will attempt to change the mode just before encrypting it. */
    do_chmod = 1;
  }
  
  /* check whether this inode was already handled under another filename */
  r = known_inode(buf.st_ino, buf.st_dev);
  if (r != -1 && cmd.verbose>0) {
    fprintf(stderr, _("Already visited inode %s.\n"), infile);
  }
  if (r == 0) {
    /* previous action on this inode failed - do nothing */
    return;
  } else if (r == 1) {
    /* previous action on this inode succeeded - rename only */
    goto rename;
  }
  
  /* act on this inode now */
  if (buf.st_nlink>1 && cmd.verbose>=0) {
    fprintf(stderr, _("%s: warning: %s has %d links\n"), cmd.name, 
	    infile, (int)buf.st_nlink);
    hardlink_warnings++;
  }
  if (do_chmod) {
    chmod(infile, buf.st_mode | S_IWUSR);
  }
  
  /* open file */
  fd = open(infile, O_RDWR | O_BINARY);
  if (fd == -1) {
    /* could not open file. */
    fprintf(stderr, "%s: %s: %s\n", cmd.name, infile, strerror(errno));
    io_errors++;
    add_inode(buf.st_ino, buf.st_dev, 0);
    return;
  }
  
  /* set local signal handler for SIGINT */
  signal(SIGINT, sigint_overwrite);

  /* crypt */
  switch (cmd.mode) {   /* note: can't be CAT or UNIXCRYPT */

  case ENCRYPT: default:
    if (cmd.verbose>0) {
      fprintf(stderr, _("Encrypting %s\n"), infile);
    }
    r = leanoencrypt_file(fd, cmd.keyword);
    break;
    
  case DECRYPT:
    if (cmd.verbose>0) {
      fprintf(stderr, _("Decrypting %s\n"), infile);
    }
    r = leanodencrypt_file(fd, cmd.keyword);
    break;
    
  case KEYCHANGE:
    if (cmd.verbose>0) {
      fprintf(stderr, _("Changing key for %s\n"), infile);
    }
    r = cckeychange_file(fd, cmd.keyword, cmd.keyword2);
    break;
    
  }    
  save_errno = errno;
  
  /* restore the original file attributes for this file descriptor. 
     Ignore failures silently */
  IGNORE_RESULT(fchown(fd, buf.st_uid, buf.st_gid));
  fchmod(fd, buf.st_mode);

  /* close file */
  s = close(fd);  /* i/o errors from previous writes may appear here */
  if (!r && s) {
    r = -3;
    save_errno = errno;
  }

  /* now restore original modtime */
  {
    struct utimbuf ut;
    ut.actime = buf.st_atime;
    ut.modtime = buf.st_mtime;
    
    utime(infile, &ut);
  }
  
  /* restore default signal handler */
  signal(SIGINT, SIG_DFL);

  errno = save_errno;
  if (r==-2 && (leanocrypt_errno == leanocrypt_EFORMAT || leanocrypt_errno == leanocrypt_EMISMATCH)) {
    fprintf(stderr, _("%s: %s: %s -- unchanged\n"), cmd.name, infile, leanocrypt_error(r));
    key_errors++;
    add_inode(buf.st_ino, buf.st_dev, 0);
    return;
  } else if (r) {  
    fprintf(stderr, "%s: %s: %s\n", cmd.name, infile, leanocrypt_error(r));
    if (r == -3) { /* i/o error */
      exit(3);  
    } else {
      exit(2);
    }
  } else {
    add_inode(buf.st_ino, buf.st_dev, 1);
  }

 rename:
  /* rename file if necessary */
  if (strcmp(infile, outfile)) {
    r = rename(infile, outfile);
    if (r) {
      fprintf(stderr, _("%s: could not rename %s to %s: %s\n"), cmd.name, 
	      infile, outfile, strerror(errno));
      io_errors++;
    }
  }
  
  if (sigint_flag) {  /* SIGINT received while crypting - delayed exit */
    exit(6);
  }
  return;
}

/* local signal handler for SIGINT for tmpfiles mode */
static char *sigint_tmpfilename;

static void sigint_tmpfiles(int dummy) {
  unlink(sigint_tmpfilename);
  exit(6);
}

/* this function is called to act on a file in tmpfiles mode. */
static void action_tmpfiles(char *infile, char *outfile) {
  int st;
  struct stat buf;
  char *tmpfile;
  int fdout;
  int r;
  int save_errno;
  FILE *fin, *fout;
  int s;
  /* read infile attributes */
  st = stat(infile, &buf);
  if (st) {
    fprintf(stderr, "%s: %s: %s\n", cmd.name, infile, strerror(errno));
    io_errors++;
    return;
  }
  
  /* if infile==outfile or outfile exists, need to make a new
     temporary file name. Else, just use outfile. */
  if (strcmp(infile, outfile)==0 || file_exists(outfile)) {
    tmpfile = (char *)xalloc(strlen(outfile)+8, cmd.name);
    strcpy(tmpfile, outfile);
    strcat(tmpfile, ".XXXXXX");
    fdout = mkstemp(tmpfile);
    if (fdout == -1) {
      fprintf(stderr, _("%s: could not create temporary file for %s: %s\n"), cmd.name, outfile, strerror(errno));
      io_errors++;
      free(tmpfile);
      return;
    }
  } else {
    tmpfile = strdup(outfile);
    fdout = open(tmpfile, O_CREAT | O_EXCL | O_WRONLY | O_BINARY, S_IRUSR | S_IWUSR);
    if (fdout == -1) {
      fprintf(stderr, "%s: %s: %s\n", cmd.name, tmpfile, strerror(errno));
      io_errors++;
      free(tmpfile);
      return;
    }
  }

  /* set local signal handler to remove tmpfile on SIGINT */
  sigint_tmpfilename = tmpfile;
  signal(SIGINT, sigint_tmpfiles);
  
  /* tmpfile: allocated string, fdout: open (and newly created) file */

  /* open file */
  fin = fopen(infile, "rb");
  if (fin == NULL) {
    /* could not open file. */
    fprintf(stderr, "%s: %s: %s\n", cmd.name, infile, strerror(errno));
    io_errors++;
    goto fail_with_fdout;
  }
  fout = fdopen(fdout, "wb");
  if (fout == NULL) {
    /* oops? */
    fprintf(stderr, "%s: %s: %s\n", cmd.name, tmpfile, strerror(errno));
    fclose(fin);
    io_errors++;
    goto fail_with_fdout;
  }

  /* crypt */
  switch (cmd.mode) {   /* note: can't be CAT or UNIXCRYPT */
  case ENCRYPT: default:
    if (cmd.verbose>0) {
      fprintf(stderr, _("Encrypting %s\n"), infile);
    }
    r = leanoencrypt_streams(fin, fout, cmd.keyword);
    break;
  case DECRYPT:
    if (cmd.verbose>0) {
      fprintf(stderr, _("Decrypting %s\n"), infile);
    }
    r = leanodencrypt_streams(fin, fout, cmd.keyword);
    break;
  case KEYCHANGE:
    if (cmd.verbose>0) {
      fprintf(stderr, _("Changing key for %s\n"), infile);
    }
    r = cckeychange_streams(fin, fout, cmd.keyword, cmd.keyword2);
    break;
  }    
  save_errno = errno;

  /* restore the original file attributes for this file descriptor. 
     Ignore failures silently */
  IGNORE_RESULT(fchown(fdout, buf.st_uid, buf.st_gid));
  fchmod(fdout, buf.st_mode);

  /* close files */
  fclose(fin);
  s = fclose(fout);  /* this also closes the underlying fdout */
  if (!r && s) {  /* check for errors due to buffered write */
    r = -3;
    save_errno = errno;
  }

  /* now restore original modtime */
  {
    struct utimbuf ut;
    ut.actime = buf.st_atime;
    ut.modtime = buf.st_mtime;

    utime(tmpfile, &ut);
  }
  
  errno = save_errno;

  /* handle errors */
  if (r==-2 && (leanocrypt_errno == leanocrypt_EFORMAT || leanocrypt_errno == leanocrypt_EMISMATCH)) {
    fprintf(stderr, _("%s: %s: %s -- unchanged\n"), cmd.name, infile, leanocrypt_error(r));
    key_errors++;
    goto fail_with_tmpfile;
  } else if (r) { 
    fprintf(stderr, "%s: %s: %s\n", cmd.name, infile, leanocrypt_error(r));
    unlink(tmpfile);
    if (r == -3) {
      exit(3);
    } else {
      exit(2);
    }
  }

  /* restore default signal handler */
  signal(SIGINT, SIG_DFL);

  /* crypting was successful. Now rename new file if necessary */
  if (strcmp(tmpfile, outfile) != 0) {
    r = rename(tmpfile, outfile);
    if (r == -1) {
      fprintf(stderr, _("%s: could not rename %s to %s: %s\n"), cmd.name, tmpfile, outfile, strerror(errno));
      io_errors++;
    }
  }
  free(tmpfile);

  /* unlink original file, if necessary */
  if (strcmp(infile, outfile) != 0) {
    r = unlink(infile);
    if (r == -1) {
      fprintf(stderr, _("%s: could not remove %s: %s\n"), cmd.name, infile, strerror(errno));
      io_errors++;
    }
  }

  return;
  
 fail_with_fdout:
  close(fdout);
 fail_with_tmpfile:
  unlink(tmpfile);
  free(tmpfile);

  /* restore default signal handler */
  signal(SIGINT, SIG_DFL);
  return;
}

/* this function is called to act on a file if cmd.mode is CAT or
   UNIXCRYPT. Return values are as for action_tmpfiles. */
static void action_cat(char *infile) {
  int r, s;
  FILE *fin;
  int save_errno;

  /* open file */
  fin = fopen(infile, "rb");
  if (fin == NULL) {
    fprintf(stderr, "%s: %s: %s\n", cmd.name, infile, strerror(errno));
    io_errors++;
    return;
  }

  /* crypt */

  if (cmd.verbose>0) {
    fprintf(stderr, _("Decrypting %s\n"), infile);
    fflush(stderr);
  }
  
  if (cmd.mode==UNIXCRYPT) {
    r = unixcrypt_streams(fin, stdout, cmd.keyword);
  } else {
    r = leanodencrypt_streams(fin, stdout, cmd.keyword);
  }
  save_errno = errno;

  s = fflush(stdout);
  if (!r && s) {
    r = -3;
    save_errno = errno;
  }

  /* close input file, ignore errors */
  fclose(fin);

  errno = save_errno;

  /* handle errors */
  if (r==-2 && (leanocrypt_errno == leanocrypt_EFORMAT || leanocrypt_errno == leanocrypt_EMISMATCH)) {
    fprintf(stderr, _("%s: %s: %s -- ignored\n"), cmd.name, infile, leanocrypt_error(r));
    key_errors++;
    return;
  } else if (r) {
    fprintf(stderr, "%s: %s: %s\n", cmd.name, infile, leanocrypt_error(r));
    if (r == -3) {
      exit(3);
    } else {
      exit(2);
    }
  }
  return;
}

static void file_action(char *filename) {
  struct stat buf;
  int st;
  char *outfile = NULL;
  char *infile;
  char *buffer = NULL;

  infile = filename;  /* but it may be changed below */

  st = lstat(infile, &buf);

  if (st) {
    int save_errno = errno;
    
    /* if file didn't exist and decrypting, try if suffixed file exists */
    if (errno==ENOENT 
	&& (cmd.mode==DECRYPT || cmd.mode==CAT || cmd.mode==KEYCHANGE 
	    || cmd.mode==UNIXCRYPT) 
	&& cmd.suffix[0]!=0) {
      buffer = (char *)xalloc(strlen(filename)+strlen(cmd.suffix)+1, cmd.name);

      strcpy(buffer, infile);
      strcat(buffer, cmd.suffix);
      infile=buffer;
      st = lstat(infile, &buf);
    }
    if (st) {
      fprintf(stderr, "%s: %s: %s\n", cmd.name, filename, strerror(save_errno));
      io_errors++;
      goto done;
    }
  }
  
  /* if link following is enabled, follow links */
  if (cmd.symlinks && S_ISLNK(buf.st_mode)) {
    st = stat(infile, &buf);
    if (st) {
      fprintf(stderr, "%s: %s: %s\n", cmd.name, infile, strerror(errno));
      io_errors++;
      goto done;
    }
  }

  /* assert st==0 */

  /* if file is not a regular file, skip */
  if (S_ISLNK(buf.st_mode)) {
    if (cmd.verbose>=0) {
      fprintf(stderr, _("%s: %s: is a symbolic link -- ignored\n"), cmd.name, infile);
      symlink_warnings++;
    }
    goto done;
  }
  if (!S_ISREG(buf.st_mode)) {
    if (cmd.verbose>=0) {
      fprintf(stderr, _("%s: %s: is not a regular file -- ignored\n"), cmd.name, 
	      infile);
      isreg_warnings++;
    }
    goto done;
  }
  
  /* now we have a regular file, and we have followed a link if
     appropriate. */

  if (cmd.mode==ENCRYPT || cmd.mode==DECRYPT || cmd.mode==KEYCHANGE) {
    /* determine outfile name */
    switch (cmd.mode) {
    case ENCRYPT: default:
      if (cmd.strictsuffix && cmd.suffix[0] != 0 && has_suffix(infile, cmd.suffix)) {
	if (cmd.verbose>=0) {
	  fprintf(stderr, _("%s: %s already has %s suffix -- ignored\n"), cmd.name, infile, cmd.suffix); 
	  strict_warnings++;
	}
	goto done;
      }
      outfile = add_suffix(infile, cmd.suffix);
      break;
    case DECRYPT:
      outfile = remove_suffix(infile, cmd.suffix);
      break;
    case KEYCHANGE:
      outfile = strdup(infile);
      break;
    }
    if (!outfile) {
      fprintf(stderr, "%s: %s\n", cmd.name, strerror(errno));
      exit(2);
    }

    /* if outfile exists and cmd.force is not set, prompt whether to
       overwrite */
    if (!cmd.force && strcmp(infile, outfile) && 
	file_exists(outfile)) {
      fprintf(stderr, _("%s: %s already exists; overwrite (y or n)? "), cmd.name, 
	      outfile);
      fflush(stderr);
      if (prompt()==0) {
	fprintf(stderr, _("Not overwritten.\n"));
	goto done;
      }
    }
    if (cmd.tmpfiles) {
      action_tmpfiles(infile, outfile);
    } else {
      action_overwrite(infile, outfile);
    }
  } else {
    action_cat(infile);
  }
 done:
  free(outfile);
  free(buffer);
  return;
}

static void traverse_file(char *filename) {
  struct stat buf;
  int st;
  int link = 0;
  int r;
  
  st = lstat(filename, &buf);
  if (!st && S_ISLNK(buf.st_mode)) {  /* is a symbolic link */
    link = 1;
    st = stat(filename, &buf);
  }
  if (st || !S_ISDIR(buf.st_mode)) {
    file_action(filename);
    return;
  }
  
  /* is a directory */
  if (cmd.recursive<=1 && link==1) { /* ignore link */
    if (cmd.verbose>=0) {
      fprintf(stderr, _("%s: %s: directory is a symbolic link -- ignored\n"), cmd.name, filename);
      symlink_warnings++;
    }
    return;
  }

  if (cmd.recursive==0) {  /* ignore */
    if (cmd.verbose>=0) {
      fprintf(stderr, _("%s: %s: is a directory -- ignored\n"), cmd.name, filename);
      isreg_warnings++;
    }
    return;
  } 

  r = known_inode(buf.st_ino, buf.st_dev);

  if (r != -1) { /* already traversed */
    if (cmd.verbose>0) {
      fprintf(stderr, _("Already visited directory %s -- skipped.\n"), filename);
    }
    return;
  }
  
  add_inode(buf.st_ino, buf.st_dev, 1);

  /* recursively traverse directory */
  {
    char **filelist;
    int count;

    get_filelist(filename, &filelist, &count);
    traverse_files(filelist, count);
    free_filelist(filelist, count);
  }
  return;
}

static void traverse_files(char **filelist, int count) {
  while (count > 0) {
    traverse_file(*filelist);
    ++filelist, --count;
  }
}

int traverse_toplevel(char **filelist, int count) {

  /* reset inode list (redundant) */
  free(inode_list);
  inode_list = NULL;
  inode_num = 0;
  inode_size = 0;

  /* reset error stats */
  key_errors = 0;
  io_errors = 0;
  symlink_warnings = 0;
  hardlink_warnings = 0;
  isreg_warnings = 0;
  strict_warnings = 0;
  
  traverse_files(filelist, count);

  free(inode_list);

  return (key_errors ? 1 : 0) | (io_errors ? 2 : 0);
}
