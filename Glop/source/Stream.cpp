#include "Stream.h"

// InputStream
// ===========

InputStream::InputStream(const string &filename)
: controller_(new FileInputStreamController(filename)) {
  controller_->AddRef();
}

InputStream::InputStream(const char *filename)
: controller_(new FileInputStreamController(filename)) {
  controller_->AddRef();
}

int InputStream::ReadAllData(void **data) {
  int length = controller_->GetLength();
  if (length >= 0) {
    // Streams with accurate length information
    int data_left = length - controller_->GetPosition();
    *data = malloc(max(data_left, 1));
    if (*data == 0)
      return 0;
    FILE *f = fopen("thames.ttf", "rb");
    controller_->ReadData(1, data_left, *data);
    return length;
  } else {
    // Streams without accurate length information
    length = 0;
    int capacity = 256, old_capacity = 0;
    *data = 0;
    while (length == old_capacity) {
      *data = realloc(*data, capacity);
      if (*data == 0)
        return 0;
      length += controller_->ReadData(1, capacity - old_capacity, *data);
      old_capacity = capacity;
      capacity = 2 * capacity;
    }
    return length;
  }
}

bool InputStream::ReadString(string *data) {
  short length;
  if (!ReadShorts(1, &length) || length < 0)
    return false;
  char *buffer = new char[length + 1];
  if (buffer == 0)
    return false;
  if (ReadChars(length, buffer) == length) {
    buffer[length] = 0;
    *data = buffer;
    delete[] buffer;
    return true;
  } else {
    delete[] buffer;
    return false;
  }
}

string InputStream::ReadString() {
  string result;
  if (!ReadString(&result)) {
    FatalError(Format("Error reading string value from input stream at position %d.",
                      GetPosition()));
  }
  return result;
}
bool InputStream::ReadBool() {
  bool result;
  if (!ReadBools(1, &result)) {
    FatalError(Format("Error reading bool value from input stream at position %d.",
                      GetPosition()));
  }
  return result;
}
unsigned char InputStream::ReadChar() {
  unsigned char result;
  if (!ReadChars(1, &result)) {
    FatalError(Format("Error reading char value from input stream at position %d.",
                      GetPosition()));
  }
  return result;
}
short InputStream::ReadShort() {
  short result;
  if (!ReadShorts(1, &result)) {
    FatalError(Format("Error reading short value from input stream at position %d.",
                      GetPosition()));
  }
  return result;
}
int InputStream::ReadInt() {
  int result;
  if (!ReadInts(1, &result)) {
    FatalError(Format("Error reading int value from input stream at position %d.",
                      GetPosition()));
  }
  return result;
}
int64 InputStream::ReadInt64() {
  int64 result;
  if (!ReadInt64s(1, &result)) {
    FatalError(Format("Error reading int64 value from input stream at position %d.",
                      GetPosition()));
  }
  return result;
}
float InputStream::ReadFloat() {
  float result;
  if (!ReadFloats(1, &result)) {
    FatalError(Format("Error reading float value from input stream at position %d.",
                      GetPosition()));
  }
  return result;
}
double InputStream::ReadDouble() {
  double result;
  if (!ReadDoubles(1, &result)) {
    FatalError(Format("Error reading double value from input stream at position %d.",
                      GetPosition()));
  }
  return result;
}

bool InputStream::LookAheadReadString(int offset, string *data) {
  short length;
  if (!LookAheadReadShorts(offset, 1, &length) || length < 0)
    return false;
  char *buffer = new char[length + 1];
  if (buffer == 0)
    return false;
  if (LookAheadReadChars(offset + 2, length, buffer) == length) {
    buffer[length] = 0;
    *data = buffer;
    delete[] buffer;
    return true;
  } else {
    delete[] buffer;
    return false;
  }
}
string InputStream::LookAheadReadString(int offset) {
  string result;
  if (!LookAheadReadString(offset, &result)) {
    FatalError(Format("Error reading string value from input stream at position %d.",
                      GetPosition() + offset));
  }
  return result;
}
bool InputStream::LookAheadReadBool(int offset) {
  bool result;
  if (!LookAheadReadBools(offset, 1, &result)) {
    FatalError(Format("Error reading bool value from input stream at position %d.",
                      GetPosition() + offset));
  }
  return result;
}
unsigned char InputStream::LookAheadReadChar(int offset) {
  unsigned char result;
  if (!LookAheadReadChars(offset, 1, &result)) {
    FatalError(Format("Error reading char value from input stream at position %d.",
                      GetPosition() + offset));
  }
  return result;
}
short InputStream::LookAheadReadShort(int offset) {
  short result;
  if (!LookAheadReadShorts(offset, 1, &result)) {
    FatalError(Format("Error reading short value from input stream at position %d.",
                      GetPosition() + offset));
  }
  return result;
}
int InputStream::LookAheadReadInt(int offset) {
  int result;
  if (!LookAheadReadInts(offset, 1, &result)) {
    FatalError(Format("Error reading int value from input stream at position %d.",
                      GetPosition() + offset));
  }
  return result;
}
int64 InputStream::LookAheadReadInt64(int offset) {
  int64 result;
  if (!LookAheadReadInt64s(offset, 1, &result)) {
    FatalError(Format("Error reading int64 value from input stream at position %d.",
                      GetPosition() + offset));
  }
  return result;
}
float InputStream::LookAheadReadFloat(int offset) {
  float result;
  if (!LookAheadReadFloats(offset, 1, &result)) {
    FatalError(Format("Error reading float value from input stream at position %d.",
                      GetPosition() + offset));
  }
  return result;
}
double InputStream::LookAheadReadDouble(int offset) {
  double result;
  if (!LookAheadReadDoubles(offset, 1, &result)) {
    FatalError(Format("Error reading double value from input stream at position %d.",
                      GetPosition() + offset));
  }
  return result;
}

// FileInputStreamController
// =========================

FileInputStreamController::FileInputStreamController(const string &filename)
: file_pointer_(fopen(filename.c_str(), "rb")) {}
  
FileInputStreamController::FileInputStreamController(const char *filename)
: file_pointer_(fopen(filename, "rb")) {}

FileInputStreamController::~FileInputStreamController() {
  if (file_pointer_ != 0)
    fclose(file_pointer_);
}

int FileInputStreamController::GetPosition() const {
  return ftell(file_pointer_);
}

int FileInputStreamController::GetLength() const {
  int pos = ftell(file_pointer_);
  fseek(file_pointer_, 0, SEEK_END);
  int len = ftell(file_pointer_);
  fseek(file_pointer_, pos, SEEK_SET);
  return len;
}

bool FileInputStreamController::SkipAhead(int bytes) {
  return (fseek(file_pointer_, bytes, SEEK_CUR) == 0);
}

int FileInputStreamController::ReadData(int record_size, int count, void *data) {
  return (int)fread(data, record_size, count, file_pointer_);
}

int FileInputStreamController::LookAheadReadData(int offset, int record_size, int count,
                                                 void *data) {
  int pos = ftell(file_pointer_);
  fseek(file_pointer_, offset, SEEK_CUR);
  int result = (int)fread(data, record_size, count, file_pointer_);
  fseek(file_pointer_, pos, SEEK_SET);
  return result;
}

// MemoryInputStreamController
// ===========================

MemoryInputStreamController::MemoryInputStreamController(void *data, int num_bytes,
                                                         bool auto_delete_data)
: data_((unsigned char*)data), pos_(0), num_bytes_(num_bytes),
  auto_delete_data_(auto_delete_data) {}

MemoryInputStreamController::~MemoryInputStreamController() {
  if (auto_delete_data_ && data_ != 0)
    free(data_);
}

bool MemoryInputStreamController::SkipAhead(int bytes) {
  if (pos_ + bytes <= num_bytes_) {
    pos_ += bytes;
    return true;
  } else {
    pos_ = num_bytes_;
    return false;
  }
}

int MemoryInputStreamController::ReadData(int record_size, int count, void *data) {
  int records = min((num_bytes_ - pos_ + 1) / record_size, count);
  memcpy(data, data_ + pos_, record_size * records);
  pos_ += record_size * records;
  return records;
}

int MemoryInputStreamController::LookAheadReadData(int offset, int record_size,
                                                   int count, void *data) {
  int old_pos = pos_;
  SkipAhead(offset);
  int result = ReadData(record_size, count, data);
  pos_ = old_pos;
  return result;
}
