/*!
  \file plinkbed.h
  \brief fileinterface compatibility for plink bedfiles
  \copyright Released under the MIT License.
  Copyright 2020 Lightning Auriga.
 */

#ifndef PLCO_ANALYSIS_SHARED_SOURCE_FILEINTERFACE_FILEINTERFACE_PLINKBED_H_
#define PLCO_ANALYSIS_SHARED_SOURCE_FILEINTERFACE_FILEINTERFACE_PLINKBED_H_

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include "fileinterface/fileinterface_reader.h"
#include "fileinterface/fileinterface_writer.h"
#include "fileinterface/helper.h"

namespace fileinterface {
/*!
  \brief enumeration of supported compression states
 */
typedef enum { NONE, GZIP, BZIP2 } COMPRESSION_TYPE;
/*!
  \brief convert a character string to a COMPRESSION_TYPE equivalent
  @param s string name of compression to be converted
  \return enumerated interpretation of string compression name
 */
inline COMPRESSION_TYPE interpret_compression(const std::string &s) {
  if (!s.compare("gzip")) return GZIP;
  if (!s.compare("bzip2")) return BZIP2;
  return NONE;
}
/*!
  \brief take genotype in numeric format and convert it to brief string format
  @param val numeric genotype to be converted
  \return genotype in brief string format
 */
inline char plink_packed_to_char(unsigned val) {
  if (val == 0) return '2';
  if (val == 1) return '9';
  if (val == 2) return '1';
  return '0';
}
/*!
  \brief take genotype in brief string format and convert it to numeric format
  @param c brief string genotype to be converted
  \return genotype in numeric format
*/
inline unsigned char char_to_plink_packed(char c) {
  if (c == '2') return (unsigned char)0;
  if (c == '1') return (unsigned char)2;
  if (c == '0') return (unsigned char)3;
  return (unsigned char)1;
}
/*!
  \class plinkbed_reader
  \brief fileinterface compatibility with reading plink bedfiles
 */
class plinkbed_reader : public fileinterface::fileinterface_reader {
 public:
  /*!
    \brief constructor
    @param line_length number of samples in bedfile
    @param cl string name of external compression applied to bedfile
   */
  explicit plinkbed_reader(unsigned line_length, const std::string &cl = "");
  /*!
    \brief destructor
   */
  ~plinkbed_reader() throw() { close(); }
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
    \brief clear internal state flags
  */
  void clear();
  /*!
    \brief determine whether a file is currently open
    \return whether a file is currently open
   */
  bool is_open() const;
  /*!
    \brief get a single character from file
    \return the next single character from file
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

 private:
  fileinterface::fileinterface_reader *_input; /*!< actual reader */
  fileinterface::COMPRESSION_TYPE _compress;   /*!< compression level */
  char *_buf;                                  /*!< internal read buffer */
  unsigned _line_length;        /*!< number of samples in bedfile */
  unsigned _line_length_packed; /*!< length of compressed blocks in bytes */
};
/*!
  \class plinkbed_writer
  \brief fileinterface compatibility with writing plink bedfiles
 */
class plinkbed_writer : public fileinterface::fileinterface_writer {
 public:
  /*!
    \brief constructor
    @param cl string name of external compression applied to bedfile
   */
  explicit plinkbed_writer(const std::string &cl = "");
  /*!
    \brief destructor
   */
  ~plinkbed_writer() throw() { close(); }
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
    \brief clear internal state flags
   */
  void clear();
  /*!
    \brief test whether the file is currently open
    \return whether the file is currently open
   */
  bool is_open() const;
  /*!
    \brief write a single character to file
    @param c character to write to file
   */
  void put(char c);
  /*!
    \brief write a line to file, appending a system-specific newline
    @param s line to write to file
   */
  void writeline(const std::string &s);
  /*!
    \brief write a specified number of characters to file
    @param buf character buffer containing contents to be written
    @param n number of characters from buffer to write to file
   */
  void write(char *buf, std::streamsize n);
  /*!
    \brief test whether end of file has been reached
    \return whether end of file has been reached
   */
  bool eof() const;
  /*!
    \brief test whether connection is currently valid
    \return whether connection is currently valid
   */
  bool good() const;
  /*!
    \brief test whether a write operation has failed for this stream
    \return whether a write operation has failed for this stream
   */
  bool fail() const;
  /*!
    \brief test whether connection is currently invalid
    \return whether connection is currently invalid
   */
  bool bad() const;

 private:
  fileinterface::fileinterface_writer *_output; /*!< actual writer */
  fileinterface::COMPRESSION_TYPE _compress;    /*!< compression level */
  char *_buf;                   /*!< internal character write buffer */
  unsigned _line_length;        /*!< number of samples in bedfile */
  unsigned _line_length_packed; /*!< length of compressed blocks in bytes */
};
}  // namespace fileinterface

#endif  // PLCO_ANALYSIS_SHARED_SOURCE_FILEINTERFACE_FILEINTERFACE_PLINKBED_H_
