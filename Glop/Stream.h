// Utilities for reading from an arbitrary binary stream. It is desirable to work with this rather
// than directly working with a FILE pointer where possible so it is possible to support other
// methods of input. For example, a Stream should be able to work directly with a compressed file,
// an encrypted file, pre-loaded data in memory, or a tarball-style file that contains several
// files condensed into one.
//
// The basic interface is an InputStream, which is an extremely light-weight handle to an
// InputStreamController. Its interface is similar to FILE except that there is no random access
// support - just reading in order. The InputStreamController is an abstract base-class that
// performs the actual logic. InputStream is used instead of InputStreamController as the basic
// utility to streamline the interface (e.g. making it possible to implictly construct a stream
// from a filename string and closing a stream when it goes out of scope).

#ifndef GLOP_STREAM_H__
#define GLOP_STREAM_H__

// Includes
#include "Base.h"

// InputStreamController abstract base class definition. Programs should interact with InputStream
// instead of this.
class InputStreamController {
 public:
  // Returns whether there is valid data in this stream (as opposed to say a file stream with an
  // invalid filename). All other queries may assume IsValid() is true.
  virtual bool IsValid() const = 0;
  
  // Returns the number of bytes already read from this stream.
  virtual int GetPosition() const = 0;

  // GetLength may return -1 to indicate length unknown
  virtual int GetLength() const = 0;

  // Moves forward the given number of bytes in the stream. This is probably (but not necessarily)
  // faster than just reading the data. Returns success or failure.
  virtual bool SkipAhead(int bytes) = 0;

  // Similar to fread, this attempts to read count records of size record_size into an
  // already-allocated array data, and returns the number of records successfully read.
  virtual int ReadData(int record_size, int count, void *data) = 0;

  // Similar to ReadData except we begin the read after skipping offset bytes ahead into the
  // stream, and the current position is not changed at all.
  virtual int LookAheadReadData(int offset, int record_size, int count, void *data) = 0;

 protected:
  InputStreamController(): ref_count_(0) {}
  virtual ~InputStreamController() {}

 private:
  friend class InputStream;
  void AddRef() {ref_count_++;}
  static void RemoveRef(InputStreamController *controller) {
    controller->ref_count_--;
    if (controller->ref_count_ <= 0)
      delete controller;
  }
  int ref_count_;
  DISALLOW_EVIL_CONSTRUCTORS(InputStreamController);
};

// InputStream class definition
class InputStream {
 public:
  // Constructors / destructor. If constructed with a controller, that controller will be deleted
  // should the reference count hit 0. Therefore, a line like new InputStream(new XController()) is
  // perfectly fine.
  InputStream(const string &filename);
  InputStream(const char *filename);
  InputStream(const InputStream &rhs): controller_(rhs.controller_) {
    controller_->AddRef();
  }
  InputStream(InputStreamController *controller): controller_(controller) {
    controller_->AddRef();
  }
  ~InputStream() {
    InputStreamController::RemoveRef(controller_);
  }

  // Meta-data. All queries except for IsValid() assume that IsValid() is true.
  bool IsValid() const {return controller_->IsValid();}
  int GetPosition() const {return controller_->GetPosition();}
  int GetLength() const {return controller_->GetLength();}

  // Puts all data remaining in the stream into *data, and returns the number of bytes read. data
  // will be allocated in this function call, and will be non-zero unless there is a memory error.
  int ReadAllData(void **data);

  // Skip a number of bytes ahead in the stream. This is probably (but not necessarily) faster than
  // reading the data. Returns success or failure.
  bool SkipAhead(int bytes) {return controller_->SkipAhead(bytes);}

  // Reads a string from the stream, formatted as <length> <data> where length is 2 bytes length,
  // and data is length bytes long, and is not null-terminated.
  bool ReadString(string *data);

  // Read one or more records from the stream.
  int ReadBools(int count, bool *data) {
    return controller_->ReadData(1, count, (void*)data);
  }
  int ReadChars(int count, char *data) {
    return controller_->ReadData(1, count, (void*)data);
  }
  int ReadChars(int count, unsigned char *data) {
    return controller_->ReadData(1, count, (void*)data);
  }
  int ReadShorts(int count, short *data) {
    return controller_->ReadData(2, count, (void*)data);
  }
  int ReadShorts(int count, unsigned short *data) {
    return controller_->ReadData(2, count, (void*)data);
  }
  int ReadInts(int count, int *data) {
    return controller_->ReadData(4, count, (void*)data);
  }
  int ReadInts(int count, unsigned int *data) {
    return controller_->ReadData(4, count, (void*)data);
  }
  int ReadInt64s(int count, int64 *data) {
    return controller_->ReadData(8, count, (void*)data);
  }
  int ReadInt64s(int count, uint64 *data) {
    return controller_->ReadData(8, count, (void*)data);
  }
  int ReadFloats(int count, float *data) {
    return controller_->ReadData(4, count, (void*)data);
  }
  int ReadDoubles(int count, double *data) {
    return controller_->ReadData(8, count, (void*)data);
  }
  int ReadData(int record_size, int count, void *data) {
    return controller_->ReadData(record_size, count, data);
  }

  // Convenience functions to read exactly one record from the stream. These functions halt the
  // program if they fail. Use the above interfaces if this is unacceptable.
  string ReadString();
  bool ReadBool();
  unsigned char ReadChar();
  short ReadShort();
  int ReadInt();
  int64 ReadInt64();
  float ReadFloat();
  double ReadDouble();

  // Read one or more records from the stream after skipping offset bytes. The location in the
  // stream is then reset to where it was before this call.
  bool LookAheadReadString(int offset, string *data);
  int LookAheadReadBools(int offset, int count, bool *data) {
    return controller_->LookAheadReadData(offset, 1, count, (void*)data);
  }
  int LookAheadReadChars(int offset, int count, char *data) {
    return controller_->LookAheadReadData(offset, 1, count, (void*)data);
  }
  int LookAheadReadChars(int offset, int count, unsigned char *data) {
    return controller_->LookAheadReadData(offset, 1, count, (void*)data);
  }
  int LookAheadReadShorts(int offset, int count, short *data) {
    return controller_->LookAheadReadData(offset, 2, count, (void*)data);
  }
  int LookAheadReadShorts(int offset, int count, unsigned short *data) {
    return controller_->LookAheadReadData(offset, 2, count, (void*)data);
  }
  int LookAheadReadInts(int offset, int count, int *data) {
    return controller_->LookAheadReadData(offset, 4, count, (void*)data);
  }
  int LookAheadReadInts(int offset, int count, unsigned int *data) {
    return controller_->LookAheadReadData(offset, 4, count, (void*)data);
  }
  int LookAheadReadInt64s(int offset, int count, int64 *data) {
    return controller_->LookAheadReadData(offset, 8, count, (void*)data);
  }
  int LookAheadReadInt64s(int offset, int count, uint64 *data) {
    return controller_->LookAheadReadData(offset, 8, count, (void*)data);
  }
  int LookAheadReadFloats(int offset, int count, float *data) {
    return controller_->LookAheadReadData(offset, 4, count, (void*)data);
  }
  int LookAheadReadDoubles(int offset, int count, double *data) {
    return controller_->LookAheadReadData(offset, 8, count, (void*)data);
  }
  int LookAheadReadData(int offset, int record_size, int count, void *data) {
    return controller_->LookAheadReadData(offset, record_size, count, data);
  }

  // Convenience functions to read exactly one record from the stream as look-ahead (see above).
  // These functions halt the program if they fail. Use the above interfaces if this is
  // unacceptable.
  string LookAheadReadString(int offset);
  bool LookAheadReadBool(int offset);
  unsigned char LookAheadReadChar(int offset);
  short LookAheadReadShort(int offset);
  int LookAheadReadInt(int offset);
  int64 LookAheadReadInt64(int offset);
  float LookAheadReadFloat(int offset);
  double LookAheadReadDouble(int offset);

 private:
  void operator=(const InputStream &rhs);  // Disallowed
  InputStreamController *controller_;
};

// FileInputStreamController class definition.
class FileInputStreamController: public InputStreamController {
 public:
  FileInputStreamController(const string &filename);
  FileInputStreamController(const char *filename);
  virtual ~FileInputStreamController();

  virtual bool IsValid() const {return file_pointer_ != 0;}
  
  virtual int GetPosition() const;
  virtual int GetLength() const;

  virtual bool SkipAhead(int bytes);
  virtual int ReadData(int record_size, int count, void *data);
  virtual int LookAheadReadData(int offset, int record_size, int count, void *data);

 private:
  FILE *file_pointer_;
  DISALLOW_EVIL_CONSTRUCTORS(FileInputStreamController);
};

class MemoryInputStreamController: public InputStreamController {
 public:
  MemoryInputStreamController(void *data, int num_bytes, bool auto_delete_data);
  virtual ~MemoryInputStreamController();

  virtual bool IsValid() const {return data_ != 0;}

  virtual int GetPosition() const {return pos_;}
  virtual int GetLength() const {return num_bytes_;}

  virtual bool SkipAhead(int bytes);
  virtual int ReadData(int record_size, int count, void *data);
  virtual int LookAheadReadData(int offset, int record_size, int count, void *data);

 private:
  unsigned char *data_;
  int pos_, num_bytes_;
  bool auto_delete_data_;
  DISALLOW_EVIL_CONSTRUCTORS(MemoryInputStreamController);
};

#endif  // INPUT_STREAM_H__
