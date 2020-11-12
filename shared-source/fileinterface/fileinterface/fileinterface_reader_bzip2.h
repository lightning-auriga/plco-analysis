/*!
  \file fileinterface_reader_bzip2.h
  \brief bzip2-specific reader class definitions
  \copyright Released under the MIT License.
  Copyright 2020 Lightning Auriga
*/

#ifndef PLCO_ANALYSIS_SHARED_SOURCE_FILEINTERFACE_FILEINTERFACE_FILEINTERFACE_READER_BZIP2_H_
#define PLCO_ANALYSIS_SHARED_SOURCE_FILEINTERFACE_FILEINTERFACE_FILEINTERFACE_READER_BZIP2_H_

#include "fileinterface/config.h"
#ifdef FILEINTERFACE_HAVE_LIBBZ2
#include <bzlib.h>

#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

#include "fileinterface/fileinterface_reader_parent.h"

namespace fileinterface {
/*!
  \class fileinterface_reader_bzip2
  \brief interface for bzip2 file input that doesn't break my brain
 */
class fileinterface_reader_bzip2 : public fileinterface_reader {
 public:
  /*!
    \brief constructor
   */
  fileinterface_reader_bzip2();
  /*!
    \brief destructor

    This cleanly handles memory allocation problems, so it's safe to just delete
    the allocated object in userspace.
   */
  ~fileinterface_reader_bzip2() throw() {
    close();
    delete[] _buf;
  }
  /*!
    \brief open a file
    @param filename name of file to open
   */
  void open(const char *filename);
  /*!
    \brief close the file
   */
  void close();
  /*!
    \brief clear state flags if applicable
   */
  void clear();
  /*!
    \brief test whether the stream is currently open
    \return whether the stream is currently open
   */
  bool is_open() const;
  /*!
    \brief get a single character from the open stream
    \return the next single character from the open stream
   */
  char get();
  /*!
    \brief read a newline-delimited line from file
    @param s where the read line will be stored
    \return whether the line read operation was successful
   */
  bool getline(std::string *s);
  /*!
    \brief determine whether the stream is at the end of the file
    \return whether the stream is at the end of the file
   */
  bool eof() const;
  /*!
    \brief determine whether the stream is in a valid state
    \return whether the stream is in a valid state
   */
  bool good() const;
  /*!
    \brief determine whether the stream is in an invalid state
    \return whether the stream is in an invalid state
   */
  bool bad() const;
  /*!
    \brief read a specified number of characters from file
    @param buf allocated array of characters to which to write result
    @param n number of characters to attempt to read
   */
  void read(char *buf, std::streamsize n);

 protected:
  /*!
    \brief a nightmare-fueled hellscape of read buffering to make this library
    seamless
   */
  void refresh_buffer();

 private:
  FILE *_raw_input;   /*!< C-style file handle used by bz2 library */
  BZFILE *_bz_input;  /*!< bz2 library interface to underlying file pointer */
  bool _eof;          /*!< internal state flag: has end of file been reached */
  char *_buf;         /*!< internal character buffer for read operations */
  unsigned _buf_max;  /*!< size of allocated internal character buffer */
  unsigned _buf_read; /*!< number of characters read from file into buffer */
  unsigned _buf_remaining; /*!< remaining space in read characters in buffer */
};
}  // namespace fileinterface

#endif  // FILEINTERFACE_HAVE_LIBBZ2

#endif  // PLCO_ANALYSIS_SHARED_SOURCE_FILEINTERFACE_FILEINTERFACE_FILEINTERFACE_READER_BZIP2_H_
