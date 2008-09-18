// Includes
#include "BinaryFileManager.h"
#include <cstdio>

// Constants
const int kChunksUninitialized = -2;
const int kChunkListId = 0xB10BB10B;

// BinaryFileReader
// ================

// Constructor - file is unopened.
BinaryFileReader::BinaryFileReader()
: file_pointer_(0),
  filename_(0) {
  next_reader_ = prev_reader_ = this;
}

// Constructor - opens a file with the given name or generate a fatal error.
BinaryFileReader::BinaryFileReader(const string &filename)
: file_pointer_(0),
  filename_(0) {
  next_reader_ = prev_reader_ = this;
  Open(filename);
}
BinaryFileReader::BinaryFileReader(const char *filename)
: file_pointer_(0),
  filename_(0) {
  next_reader_ = prev_reader_ = this;
  Open(filename);
}

// Attempts to open a file with the given name, and returns the result.
// File length is loaded immediately.
bool BinaryFileReader::Open(const string &filename) {
  Close();
  file_pointer_ = (void*)fopen(filename.c_str(), "rb");
  if (file_pointer_ == 0)
    return false;
  filename_ = new char[filename.size() + 1];
  strcpy(filename_, filename.c_str());
  num_chunks_ = last_chunk_read_ = kChunksUninitialized;
  data_start_ = 0;
  fseek((FILE*)file_pointer_, 0, SEEK_END);
  data_length_ = ftell((FILE*)file_pointer_);
  fseek((FILE*)file_pointer_, 0, SEEK_SET);
  return true;
}

// Closes the file pointer if it is open and owned.
void BinaryFileReader::Close() {
  // Unlink this reader and free the memory if appropriate
  if (file_pointer_ != 0) {
    if (next_reader_ == this) {
      fclose((FILE*)file_pointer_);
      delete[] filename_;
    } else {
      prev_reader_->next_reader_ = next_reader_;
      next_reader_->prev_reader_ = prev_reader_;
      next_reader_ = prev_reader_ = this;
    }
  }

  // Reset the reader's values
  file_pointer_ = 0;
  filename_ = 0;
}

// Copy constructor - see BinaryFileManager.h
BinaryFileReader::BinaryFileReader(const BinaryFileReader &rhs)
: file_pointer_(0),
  filename_(0) {
  Copy(rhs, rhs.data_start_, rhs.data_length_);
}

// Assignment operator - see BinaryFileManager.h
const BinaryFileReader &BinaryFileReader::operator=(const BinaryFileReader &rhs) {
  if (this != &rhs)
    Copy(rhs, rhs.data_start_, rhs.data_length_);
  return *this;
}

// See BinaryFileReader.h. If an error is detected, returns an unopened BinaryFileReader.
BinaryFileReader BinaryFileReader::GetChunkReader(short chunk) {
  InitChunks();
  ASSERT(chunk >= 0 && chunk < num_chunks_);
  
  // Compute the starting position of this and the next chunk
  Seek(6 + 4*chunk);
  int chunk_start = ReadInt(), next_chunk_start;
  if (chunk + 1 >= num_chunks_)
    next_chunk_start = data_length_;
  else
    next_chunk_start = ReadInt();
  if (chunk_start < 0 || next_chunk_start < chunk_start || next_chunk_start > data_length_)
    return BinaryFileReader();
  
  // Seek to the beginning of the chunk and return it
  Seek(chunk_start);
  BinaryFileReader result;
  result.Copy(*this, chunk_start + data_start_, next_chunk_start - chunk_start);
  return result;
}

// Moves to the given position in the file, relative to base (and to data_start_, data_length_).
void BinaryFileReader::Seek(int offset, SeekBase base) {
  if (!IsOpen())
    return;
  if (base == Start)
    fseek((FILE*)file_pointer_, offset + data_start_, SEEK_SET);
  else if (base == Current)
    fseek((FILE*)file_pointer_, offset, SEEK_CUR);
  else if (base == End)
    fseek((FILE*)file_pointer_, offset + data_start_ + data_length_, SEEK_SET);
}

// Returns the current position in the file (relative to data_start_).
int BinaryFileReader::Tell() const {
  if (IsOpen())
    return ftell((FILE*)file_pointer_) - data_start_;
  else
    return -1;
}

// Attempts to load a from the input file. Returns whether the result was successful. On success,
// *data will store the string.
bool BinaryFileReader::ReadString(string *data) {
  short length;
  if (!ReadShorts(1, &length) || length < 0 || length > data_length_)
    return false;
  char *buffer = new char[length + 1];
  if (ReadData(1, length, buffer) == length) {
    buffer[length] = 0;
    *data = buffer;
    delete[] buffer;
    return true;
  } else {
    delete[] buffer;
    return false;
  }
}

// Convenience functions for ReadData, specialized to built-in data types.
int BinaryFileReader::ReadBools(int count, bool *data) {
  return ReadData(1, count, data);
}
int BinaryFileReader::ReadChars(int count, char *data) {
  return ReadData(1, count, data);
}
int BinaryFileReader::ReadShorts(int count, short *data) {
  return ReadData(2, count, data);
}
int BinaryFileReader::ReadInts(int count, int *data) {
  return ReadData(4, count, data);
}
int BinaryFileReader::ReadInt64s(int count, int64 *data) {
  return ReadData(8, count, data);
}
int BinaryFileReader::ReadFloats(int count, float *data) {
  return ReadData(4, count, data);
}
int BinaryFileReader::ReadDoubles(int count, double *data) {
  return ReadData(count, 8, data);
}

// Loads a block of data. The number of records read is returned.
int BinaryFileReader::ReadData(int record_size, int count, void *data) {
  if (!IsOpen())
    return 0;
  if (record_size * count + Tell() > data_length_)
    count = (data_length_ - Tell()) / record_size;
  return (int)fread(data, record_size, count, (FILE*)file_pointer_);
}

// Convenience functions for ReadData, specialized to reading one record of a built-in data
// type. Generates a fatal error on error.
string BinaryFileReader::ReadString() {
  string result;
  if (!ReadString(&result))
    FatalError(Format("Error reading string value from file: \"%s\".", filename_));
  return result;
}
bool BinaryFileReader::ReadBool() {
  bool result;
  if (!ReadBools(1, &result))
    FatalError(Format("Error reading bool value from file: \"%s\".", filename_));
  return result;
}
unsigned char BinaryFileReader::ReadChar() {
  unsigned char result;
  if (!ReadChars(1, &result))
    FatalError(Format("Error reading char value from file: \"%s\".", filename_));
  return result;
}
short BinaryFileReader::ReadShort() {
  short result;
  if (!ReadShorts(1, &result))
    FatalError(Format("Error reading short value from file: \"%s\".", filename_));
  return result;
}
int BinaryFileReader::ReadInt() {
  int result;
  if (!ReadInts(1, &result))
    FatalError(Format("Error reading int value from file: \"%s\".", filename_));
  return result;
}
int64 BinaryFileReader::ReadInt64() {
  int64 result;
  if (!ReadInt64s(1, &result))
    FatalError(Format("Error reading int64 value from file: \"%s\".", filename_));
  return result;
}
float BinaryFileReader::ReadFloat() {
  float result;
  if (!ReadFloats(1, &result))
    FatalError(Format("Error reading float value from file: \"%s\".", filename_));
  return result;
}
double BinaryFileReader::ReadDouble() {
  double result;
  if (!ReadDoubles(1, &result))
    FatalError(Format("Error reading double value from file: \"%s\".", filename_));
  return result;
}

// Copying utility - see BinaryFileManager.h. Allows start_pos and length values to be
// overridden, which is the mechanism by which GetChunkReader works.
void BinaryFileReader::Copy(const BinaryFileReader &rhs, int start_pos, int length) {
  Close();

  // Mark that the file is open
  file_pointer_ = rhs.file_pointer_;
  filename_ = rhs.filename_;
  next_reader_ = rhs.next_reader_;
  prev_reader_ = (BinaryFileReader*)&rhs;
  rhs.next_reader_ = this;
  next_reader_->prev_reader_ = this;

  // Update our local valuess
  num_chunks_ = last_chunk_read_ = kChunksUninitialized;
  data_start_ = start_pos;
  data_length_ = length;
}

// Loads chunk information from this file. Initially, we have num_chunks_ = -2. After running
// this function, its value is correctly set.
void BinaryFileReader::InitChunks() const {
  // Make sure the chunks need to be initialized
  if (file_pointer_ == 0 || num_chunks_ != kChunksUninitialized)
    return;
  
  // Read the number of chunks
  int old_position = Tell(), id;
  BinaryFileReader *mutable_this = (BinaryFileReader*)this;
  mutable_this->Seek(0);
  if (mutable_this->ReadInts(1, &id) < 1 || id != kChunkListId ||
      mutable_this->ReadShorts(1, &num_chunks_) < 0 || data_length_ < 6 + 4*num_chunks_) {
    num_chunks_ = -1;
  } else {
    last_chunk_read_ = 0;
  }
  mutable_this->Seek(old_position);
}

// BinaryFileWriter
// ================

// Constructor - file is unopened.
BinaryFileWriter::BinaryFileWriter()
: file_pointer_(0) {
  Close();
}

// Constructor - open a file with the given name or generate fatal error.
BinaryFileWriter::BinaryFileWriter(const string &filename, int num_chunks,
                                   bool fatal_error_on_fail)
: file_pointer_(0) {
  if (!Open(filename, num_chunks) && fatal_error_on_fail)
    FatalError(Format("Could not open file for writing: \"%s\".", filename.c_str()));
}

// Attempt to open a file with the given name with the given number of chunks, and return the
// result. An unchunkified file should have num_chunks = -1. Otherwise, it will be an empty
// chunkified file.
bool BinaryFileWriter::Open(const string &filename, int num_chunks) {
  Close();
  file_pointer_ = (void*)fopen(filename.c_str(), "wb");
  if (file_pointer_ == 0)
    return false;
  filename_ = filename;
  BeginChunk(num_chunks);
  return true;
}

// Close the file pointer if it is open.
void BinaryFileWriter::Close() {
  if (file_pointer_ != 0)
    fclose((FILE*)file_pointer_);
  file_pointer_ = 0;
  filename_ = "";
}

// Begins a new chunk with the given number of sub-chunks.
// If num_sub_chunks is -1, this is just plain data. Otherwise, it is a list of chunk and should
// contain no plain data.
void BinaryFileWriter::BeginChunk(int num_sub_chunks) {
  int position = Tell();

  // Store the start of this chunk in the parent chunk list
  if (chunk_stack_.size() > 0) {
    Seek(chunk_stack_.top().list_location + 6 + 4 * chunk_stack_.top().cur_chunk);
    int diff_position = position - chunk_stack_.top().list_location;
    ASSERT(WriteInt(diff_position));
    Seek(position);
  }
  
  // Create the next chunk list entry
  ChunkList new_list;
  new_list.cur_chunk = 0;
  new_list.list_location = position;
  new_list.num_chunks = num_sub_chunks;
  chunk_stack_.push(new_list);
  
  // If this is not a leaf chunk, create a header in the file
  if (num_sub_chunks != -1) {
    WriteInt(kChunkListId);
    WriteShort(num_sub_chunks);
    for (int i = 0; i < num_sub_chunks; i++)
      WriteInt(-1);
  }
}

// Moves to the next active chunk.
void BinaryFileWriter::EndChunk() {
  chunk_stack_.pop();
  chunk_stack_.top().cur_chunk++;
}

// Moves to the given position in the file, relative to base (and to data_start_, data_length_).
// Extreme care should be taken using this to avoid breaking chunk data.
void BinaryFileWriter::Seek(int offset, SeekBase base) {
  if (!IsOpen())
    return;
  if (base == Start)
    fseek((FILE*)file_pointer_, offset, SEEK_SET);
  else if (base == Current)
    fseek((FILE*)file_pointer_, offset, SEEK_CUR);
  else if (base == End)
    fseek((FILE*)file_pointer_, offset, SEEK_END);
}

// Returns the current position in the file (relative to data_start_).
int BinaryFileWriter::Tell() const {
  if (IsOpen())
    return ftell((FILE*)file_pointer_);
  else
    return -1;
}

// Convenience functions for WriteData, specialized to built-in data types.
int BinaryFileWriter::WriteBools(int count, const bool *data) {
  return WriteData(1, count, data);
}
int BinaryFileWriter::WriteChars(int count, const char *data) {
  return WriteData(1, count, data);
}
int BinaryFileWriter::WriteShorts(int count, const short *data) {
  return WriteData(2, count, data);
}
int BinaryFileWriter::WriteInts(int count, const int *data) {
  return WriteData(4, count, data);
}
int BinaryFileWriter::WriteInt64s(int count, const int64 *data) {
  return WriteData(8, count, data);
}
int BinaryFileWriter::WriteFloats(int count, const float *data) {
  return WriteData(4, count, data);
}
int BinaryFileWriter::WriteDoubles(int count, const double *data) {
  return WriteData(8, count, data);
}

// Save a block of data. It is assumed to be little endian and is adjusted to match the
// current processor. The number of records written is returned.
int BinaryFileWriter::WriteData(int record_size, int count, const void *data) {
  if (!IsOpen())
    return 0;
  return (int)fwrite(data, record_size, count, (FILE*)file_pointer_);
}

// Convenience functions for WriteData, specialized to a single record of built-in data types.
bool BinaryFileWriter::WriteString(const string &data) {
  short length = short(data.size());
  return WriteShort(length) && WriteChars(length, (const unsigned char*)data.c_str()) == length;
}
bool BinaryFileWriter::WriteBool(bool data) {
  return WriteBools(1, &data) > 0;
}
bool BinaryFileWriter::WriteChar(unsigned char data) {
  return WriteChars(1, &data) > 0;
}
bool BinaryFileWriter::WriteShort(short data) {
  return WriteShorts(1, &data) > 0;
}
bool BinaryFileWriter::WriteInt(int data) {
  return WriteInts(1, &data) > 0;
}
bool BinaryFileWriter::WriteInt64(int64 data) {
  return WriteInt64s(1, &data) > 0;
}
bool BinaryFileWriter::WriteFloat(float data) {
  return WriteFloats(1, &data) > 0;
}
bool BinaryFileWriter::WriteDouble(double data) {
  return WriteDoubles(1, &data) > 0;
}
