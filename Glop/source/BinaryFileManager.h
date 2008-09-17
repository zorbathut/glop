// Utilities for reading and wrriting binary data files, supporting "chunk lists", a recursive
// mechanism for consolidating multiple files into one.
//
// Binary file format:
//   *BinaryFile* = BINARY_DATA || *ConsolidatedBinaryFile*
//   *ConsolidatedBinaryFile* = CHUNK_LIST_ID + numChunks (2 bytes) + *ChunkSizes* + *Chunks*
//   *ChunkSizes* = numChunks 4 byte integers specifying the length of each chunk
//   *Chunks* = numChunks *BinaryFile*'s giving each binary file.
//
// A BinaryFileReader can be thought of as a lightweight view of a file or a file chunk. It can
// safely be copied with little or no cost, and this is essential to the interface of functions
// like GetNextChunkReader. The physical file will remain open as long as some BinaryFileReader
// is left that refers to it.

#ifndef GLOP_BINARY_FILE_MANAGER_H__
#define GLOP_BINARY_FILE_MANAGER_H__

// Includes
#include "Base.h"
#include <stack>
using namespace std;

// BinaryFileReader class definition
class BinaryFileReader {
 public:
  // Basic construction and destruction
  BinaryFileReader();
  BinaryFileReader(const string &filename);  // Provide both string, char* constructors for
  BinaryFileReader(const char *filename);    //  implicit conversion
  ~BinaryFileReader() {Close();}
  bool Open(const string &filename);
  void Close();
  bool IsOpen() const {return file_pointer_ != 0;}
  
  // Copy construction. This is available to simplify chunk reading. A FileReader copy is only
  // a view on the original object. It shares the same file pointer, so moving its position either
  // by reads or by seeks affects the original object as well/
  BinaryFileReader(const BinaryFileReader &rhs);
  const BinaryFileReader &operator=(const BinaryFileReader &rhs);
  
  // Chunk utilities. A BinaryFileReader either contains raw data, or is a collection of
  // chunks. Each chunk is then recursively defined in the same way as the original file.
  // If a file contains only raw data, GetNumChunks returns -1 and IsChunky returns false.
  // Otherwise, these utilities can be used to access the individual chunks within the file.
  // Each of these chunks will be logically treated as a file of its own.
  bool IsChunky() const {InitChunks(); return num_chunks_ != -1;}
  short GetNumChunks() const {InitChunks(); return num_chunks_;}
  BinaryFileReader GetNextChunkReader() {return GetChunkReader(last_chunk_read_++);}
  BinaryFileReader GetChunkReader(short chunk);
  
  // Basic file manipulation.
  void *GetFilePointer() {return file_pointer_;}
  const char *GetFilename() const {return filename_;}
  int GetStartPos() const {return data_start_;}
  int GetLength() const {return data_length_;}
  enum SeekBase {Start, Current, End};
  void Seek(int offset, SeekBase base = Start);
  int Tell() const;
  
  // Bulk/safe data reading functions
  bool ReadString(string *data);
  int ReadBools(int count, bool *data);
  int ReadChars(int count, char *data);
  int ReadChars(int count, unsigned char *data) {return ReadChars(count, (char*)data);}
  int ReadShorts(int count, short *data);
  int ReadShorts(int count, unsigned short *data) {return ReadShorts(count, (short*)data);}
  int ReadInts(int count, int *data);
  int ReadInts(int count, unsigned int *data) {return ReadInts(count, (int*)data);}
  int ReadInt64s(int count, int64 *data);
  int ReadFloats(int count, float *data);
  int ReadDoubles(int count, double *data);
  int ReadData(int record_size, int count, void *data);
  
  // Data reading convenience functions - program dies on error
  string ReadString();
  bool ReadBool();
  unsigned char ReadChar();
  short ReadShort();
  int ReadInt();
  int64 ReadInt64();
  float ReadFloat();
  double ReadDouble();

 private:
  void Copy(const BinaryFileReader &rhs, int start_pos, int length);
  void InitChunks() const;
  
  void *file_pointer_;  // Stored as void* so we don't need to define/declare FILE
  char *filename_;
  mutable BinaryFileReader *next_reader_, *prev_reader_;
  mutable short num_chunks_, last_chunk_read_;
  int data_start_, data_length_;
};

// BinaryFileWriter class definition
class BinaryFileWriter {
 public:
  // Construction and destruction
  BinaryFileWriter();
  explicit BinaryFileWriter(const string &filename, int num_chunks = -1,
                            bool fatal_error_on_fail = true);
  ~BinaryFileWriter() {Close();}
  bool Open(const string &filename, int num_chunks = -1);
  void Close();
  bool IsOpen() const {return file_pointer_ != 0;}
  
  // Chunk utilities
  void BeginChunk(int num_sub_chunks = -1);
  void EndChunk();
  
  // Basic file manipulation
  enum SeekBase {Start, Current, End};
  void Seek(int offset, SeekBase base = Start);
  int Tell() const;
  void *GetFilePointer() {return file_pointer_;}
  const string &GetFilename() const {return filename_;}
  
  // Bulk data writing functions
  int WriteBools(int count, const bool *data);
  int WriteChars(int count, const char *data);
  int WriteChars(int count, const unsigned char *data) {
    return WriteChars(count, (const char*)data);
  }
  int WriteShorts(int count, const short *data);
  int WriteShorts(int count, const unsigned short *data) {
    return WriteShorts(count, (const short*)data);
  }
  int WriteInts(int count, const int *data);
  int WriteInts(int count, const unsigned int *data) {
    return WriteInts(count, (const int*)data);
  }
  int WriteInt64s(int count, const int64 *data);
  int WriteFloats(int count, const float *data);
  int WriteDoubles(int count, const double *data);
  int WriteData(int record_size, int count, const void *data);
  
  // Single record data writing functions
  bool WriteString(const string &data);
  bool WriteBool(bool data);
  bool WriteChar(unsigned char data);
  bool WriteShort(short data);
  bool WriteInt(int data);
  bool WriteInt64(int64 data);
  bool WriteFloat(float data);
  bool WriteDouble(double data);
  
 private:
  struct ChunkList {
    int list_location;
    short num_chunks, cur_chunk;
  };
  void *file_pointer_;
  string filename_;
  stack<ChunkList> chunk_stack_;
  DISALLOW_EVIL_CONSTRUCTORS(BinaryFileWriter);
};

#endif // GLOP_BINARY_FILE_MANAGER__
