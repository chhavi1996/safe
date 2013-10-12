/*
  Lockbox: Encrypted File System
  Copyright (C) 2013 Rian Hunter <rian@alum.mit.edu>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "fs_fsio.h"

#include <encfs/base/optional.h>
#include <encfs/fs/FsIO.h>

#include <memory>

#define OUT_VAR

enum {
  DONT_CREATE = false,
  SHOULD_CREATE = true,
};

enum {
  NEED_WRITE = false,
  DONT_NEED_WRITE = true,
};

encfs::FsIO *fs_handle_to_pointer(fs_handle_t h) {
  return (encfs::FsIO *) h;
}

encfs::FileIO *file_handle_to_pointer(fs_file_handle_t h) {
  return (encfs::FileIO *) h;
}

encfs::DirectoryIO *directory_handle_to_pointer(fs_directory_handle_t h) {
  return (encfs::DirectoryIO *) h;
}

fs_handle_t pointer_to_fs_handle(encfs::FsIO *h) {
  static_assert(sizeof(fs_handle_t) >= sizeof(encfs::FsIO *),
                "fs_fsio_handle_t cannot hold encfs::FsIO *");
  return (fs_handle_t) h;
}

fs_file_handle_t pointer_to_file_handle(encfs::FileIO *h) {
  static_assert(sizeof(fs_file_handle_t) >= sizeof(encfs::File *),
                "fs_file_handle_t cannot hold encfs::FileIO *");
  return (fs_file_handle_t) h;
}

fs_directory_handle_t pointer_to_directory_handle(encfs::DirectoryIO *h) {
  static_assert(sizeof(fs_directory_handle_t) >= sizeof(encfs::Directory *),
                "fs_directory_handle_t cannot hold encfs::Directory *");
  return (fs_directory_handle_t) h;
}

static fs_error_t
errc_to_FsIO_error(std::errc e) {
  switch (e) {
    // TODO: fill this in
  default:
    return FS_ERROR_IO;
  }
}

static fs_error_t
get_FsIO_error_or_default(const std::system_error & err) {
  if (err.code().default_error_condition().category() == std::generic_category()) {
    return errc_to_FsIO_error((std::errc) err.code().value());
  }
  else {
    return FS_ERROR_IO;
  }
}

static char *
strdup_x(const std::string & str) {
  auto toret = (char *) malloc(str.length() + 1);
  memcpy(toret, str.c_str(), str.length());
  return toret;
}

extern "C" {

fs_error_t
fs_fsio_open(fs_handle_t fs,
             const char *cpath, bool should_create,
             OUT_VAR fs_file_handle_t *handle,
             OUT_VAR bool *created) {
  auto fsio = fs_handle_to_pointer(fs);

  opt::optional<encfs::Path> path;
  try {
    path = fsio->pathFromString(cpath);
  }
  catch (...) {
    return FS_ERROR_INVALID_ARG;
  }

  assert( path );

  try {
    std::unique_ptr<encfs::FileIO> fio;
    try {
      fio = fsio->openfile(std::move( *path ), NEED_WRITE, DONT_CREATE);
    }
    catch (const std::system_error & err) {
      if (!should_create || err.code() != std::errc::no_such_file_or_directory) throw;
      fio = fsio->openfile(std::move( *path ), NEED_WRITE, SHOULD_CREATE);
      // slight race condition here, we may not have created the file
      // (it could have been created between the two `openfile()` calls
      if (*created) *created = true;
    }
    *handle = pointer_to_file_handle(fio.release());
    return FS_ERROR_SUCCESS;
  }
  catch (const std::system_error & err) {
    return get_FsIO_error_or_default(err);
  }
  catch (...) {
    // TODO: return more specific error code
    return FS_ERROR_IO;
  }
}

static void
fill_attrs(encfs::FsFileAttrs *fs_attrs, OUT_VAR FsAttrs *attrs) {
  *attrs = FsAttrs {
    .modified_time = fs_attrs->mtime,
    .created_time = FS_INVALID_TIME,
    .is_directory = fs_attrs->type == encfs::FsFileType::DIRECTORY,
    .size = fs_attrs->size,
    .file_id = fs_attrs->file_id,
  };
}

fs_error_t
fs_fsio_fgetattr(fs_handle_t /*fs_*/, fs_file_handle_t file_handle,
                 OUT_VAR FsAttrs *attrs) {
  auto fio = file_handle_to_pointer(file_handle);

  try {
    auto attrs_ = fio->get_attrs();
    fill_attrs(&attrs_, attrs);
    return FS_ERROR_SUCCESS;
  }
  catch (const std::system_error & err) {
    return get_FsIO_error_or_default(err);
  }
  catch (...) {
    return FS_ERROR_IO;
  }
}

fs_error_t
fs_fsio_ftruncate(fs_handle_t /*fs*/, fs_file_handle_t file_handle,
                  fs_off_t offset) {
  auto fio = file_handle_to_pointer(file_handle);

  try {
    fio->truncate(offset);
    return FS_ERROR_SUCCESS;
  }
  catch (const std::system_error & err) {
    return get_FsIO_error_or_default(err);
  }
  catch (...) {
    return FS_ERROR_IO;
  }
}

fs_error_t
fs_fsio_read(fs_handle_t /*fs*/, fs_file_handle_t file_handle,
             OUT_VAR char *buf, size_t size, fs_off_t off,
             OUT_VAR size_t *amt_read) {
  auto fio = file_handle_to_pointer(file_handle);

  try {
    *amt_read = fio->read(encfs::IORequest(off, (encfs::byte *) buf, size));
    return FS_ERROR_SUCCESS;
  }
  catch (const std::system_error & err) {
    return get_FsIO_error_or_default(err);
  }
  catch (...) {
    return FS_ERROR_IO;
  }
}

fs_error_t
fs_fsio_write(fs_handle_t /*fs*/, fs_file_handle_t file_handle,
              const char *buf, size_t size, fs_off_t off,
              OUT_VAR size_t *amt_written) {
  auto fio = file_handle_to_pointer(file_handle);

  try {
    fio->write(encfs::IORequest(off, (encfs::byte *) buf, size));
    *amt_written = size;
    return FS_ERROR_SUCCESS;
  }
  catch (const std::system_error & err) {
    return get_FsIO_error_or_default(err);
  }
  catch (...) {
    return FS_ERROR_IO;
  }
}

fs_error_t
fs_fsio_opendir(fs_handle_t fs, const char *path,
                OUT_VAR fs_directory_handle_t *dir_handle) {
  auto fsio = fs_handle_to_pointer(fs);

  opt::optional<encfs::Path> maybePath;
  try {
    maybePath = fsio->pathFromString(path);
  }
  catch (...) {
    return FS_ERROR_INVALID_ARG;
  }

  assert(maybePath);

  try {
    std::unique_ptr<encfs::DirectoryIO> dir = fsio->opendir(*maybePath);
    *dir_handle = pointer_to_directory_handle(dir.release());
    return FS_ERROR_SUCCESS;
  }
  catch (const std::system_error & err) {
    return get_FsIO_error_or_default(err);
  }
  catch (...) {
    return FS_ERROR_IO;
  }
}

fs_error_t
fs_fsio_readdir(fs_handle_t /*fs*/, fs_directory_handle_t dir_handle,
                OUT_VAR char **name,
                OUT_VAR bool *attrs_is_filled,
                OUT_VAR FsAttrs */*attrs*/) {
  auto dirio = directory_handle_to_pointer(dir_handle);

  try {
    auto maybeDirEnt = dirio->readdir();
    if (maybeDirEnt) {
      *name = strdup_x(maybeDirEnt->name);
      if (attrs_is_filled) *attrs_is_filled = false;
    }
    else {
      *name = NULL;
    }

    return FS_ERROR_SUCCESS;
  }
  catch (const std::system_error & err) {
    return get_FsIO_error_or_default(err);
  }
  catch (...) {
    return FS_ERROR_IO;
  }
}

fs_error_t
fs_fsio_closedir(fs_handle_t /*fs*/, fs_directory_handle_t dir_handle) {
  delete directory_handle_to_pointer(dir_handle);
  return FS_ERROR_SUCCESS;
}

/* can remove either a file or a directory,
   removing a directory should fail if it's not empty
*/
fs_error_t
fs_fsio_remove(fs_handle_t fs, const char *path) {
  auto fsio = fs_handle_to_pointer(fs);

  opt::optional<encfs::Path> maybePath;
  try {
    maybePath = fsio->pathFromString(path);
  }
  catch (...) {
    return FS_ERROR_INVALID_ARG;
  }

  assert(maybePath);

  try {
    try {
      fsio->unlink(*maybePath);
    }
    catch (const std::system_error & err) {
      if (err.code() == std::errc::operation_not_permitted) fsio->rmdir(*maybePath);
      else throw;
    }

    return FS_ERROR_SUCCESS;
  }
  catch (const std::system_error & err) {
    return get_FsIO_error_or_default(err);
  }
  catch (...) {
    return FS_ERROR_IO;
  }
}

fs_error_t
fs_fsio_mkdir(fs_handle_t fs, const char *path) {
  auto fsio = fs_handle_to_pointer(fs);

  opt::optional<encfs::Path> maybePath;
  try {
    maybePath = fsio->pathFromString(path);
  }
  catch (...) {
    return FS_ERROR_INVALID_ARG;
  }

  assert(maybePath);

  try {
    fsio->mkdir(*maybePath);
    return FS_ERROR_SUCCESS;
  }
  catch (const std::system_error & err) {
    return get_FsIO_error_or_default(err);
  }
  catch (...) {
    return FS_ERROR_IO;
  }
}

fs_error_t
fs_fsio_getattr(fs_handle_t fs, const char *path,
                OUT_VAR FsAttrs *attrs) {
  auto fsio = fs_handle_to_pointer(fs);

  opt::optional<encfs::Path> maybePath;
  try {
    maybePath = fsio->pathFromString(path);
  }
  catch (...) {
    return FS_ERROR_INVALID_ARG;
  }

  assert(maybePath);

  try {
    auto attrs_ = fsio->openfile(*maybePath).get_attrs();
    fill_attrs(&attrs_, attrs);
    return FS_ERROR_SUCCESS;
  }
  catch (const std::system_error & err) {
    return get_FsIO_error_or_default(err);
  }
  catch (...) {
    return FS_ERROR_IO;
  }
}

fs_error_t
fs_fsio_rename(fs_handle_t fs,
               const char *src, const char *dst) {
  auto fsio = fs_handle_to_pointer(fs);

  opt::optional<encfs::Path> maybeSrcPath, maybeDstPath;
  try {
    maybeSrcPath = fsio->pathFromString(src);
  }
  catch (...) {
    return FS_ERROR_INVALID_ARG;
  }

  try {
    maybeDstPath = fsio->pathFromString(dst);
  }
  catch (...) {
    return FS_ERROR_INVALID_ARG;
  }

  assert(maybeSrcPath);
  assert(maybeDstPath);

  try {
    fsio->rename(*maybeSrcPath, *maybeDstPath);
    return FS_ERROR_SUCCESS;
  }
  catch (const std::system_error & err) {
    return get_FsIO_error_or_default(err);
  }
  catch (...) {
    return FS_ERROR_IO;
  }
}

fs_error_t
fs_fsio_close(fs_handle_t /*fs*/, fs_file_handle_t handle) {
  delete file_handle_to_pointer(handle);
  return FS_ERROR_SUCCESS;
}

bool
fs_fsio_path_is_root(fs_handle_t fs, const char *path) {
  auto fsio = fs_handle_to_pointer(fs);

  try {
    return fsio->pathFromString(path).is_root();
  }
  catch (...) {
    return FS_ERROR_INVALID_ARG;
  }
}

bool
fs_fsio_path_equals(fs_handle_t fs, const char *a, const char *b) {
  auto fsio = fs_handle_to_pointer(fs);

  try {
    return fsio->pathFromString(a) == fsio->pathFromString(b);
  }
  catch (...) {
    return FS_ERROR_INVALID_ARG;
  }
}

bool
fs_fsio_path_is_parent(fs_handle_t fs,
                       const char *potential_parent,
                       const char *potential_child) {
  auto fsio = fs_handle_to_pointer(fs);

  try {
    auto parent_path = fsio->pathFromString(potential_parent);
    auto child_path = fsio->pathFromString(potential_child);
    return path_is_parent(parent_path, child_path);
  }
  catch (...) {
    return FS_ERROR_INVALID_ARG;
  }
}

const char *
fs_fsio_path_sep(fs_handle_t fs) {
  // not totally sold on exposing path sep as part of this API
  return fs_handle_to_pointer(fs)->path_sep().c_str();
}

}