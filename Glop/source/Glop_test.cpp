#include <gtest/gtest.h>
#include "Thread.h"
#include "System.h"
#include "Utils.h"
#include "GlopWindow.h"

#include <vector>
using namespace std;

class GlopEnvironment : public testing::Environment {
 public:
  virtual void SetUp() {
    System::Init();
  }
};
testing::Environment* const global_env = testing::AddGlobalTestEnvironment(new GlopEnvironment);


class AdderThread: public Thread {
 public:
  AdderThread(int *value, int repeats, Mutex *mutex)
  : value_(value), repeats_(repeats), mutex_(mutex) {}

 protected:
  virtual void Run() {
    for (int i = 0; i < repeats_; i++) {
      MutexLock lock(mutex_);
      (*value_)++;
    }
  }

 private:
  int *value_, repeats_;
  Mutex *mutex_;
};

TEST(ThreadTest, TestMutex) {
  int value = 0;
  Mutex mutex;
  vector<Thread*> threads;
  for (int i = 0; i < 10; i++) {
    threads.push_back(new AdderThread(&value, 5000, &mutex));
    threads[i]->Start();
  }
  for (int i = 0; i < (int)threads.size(); i++) {
    threads[i]->Join();
    delete threads[i];
  }
  EXPECT_EQ(50000, value);
}

TEST(UtilsTest, TestBinarySearchFindMatch) {
  vector<int> v;
  for (int i = 0; i < 25000; i+=5) {
    v.push_back(i);
  }
  EXPECT_EQ(  -1, BSFindMatch(v,    -5));
  EXPECT_EQ(  -1, BSFindMatch(v, 50000));
  EXPECT_EQ(  -1, BSFindMatch(v,   501));
  EXPECT_EQ(   0, BSFindMatch(v,     0));
  EXPECT_EQ(2500, BSFindMatch(v, 12500));
  EXPECT_EQ(4999, BSFindMatch(v, 24995));
}

TEST(UtilsTest, TestBinarySearchFindLowerBound) {
  vector<int> v;
  for (int i = 0; i < 25000; i+=5) {
    v.push_back(i);
  }
  EXPECT_EQ(  -1, BSFindLowerBound(v,        -1));
  EXPECT_EQ(   0, BSFindLowerBound(v,         0));
  EXPECT_EQ(   0, BSFindLowerBound(v,         1));
  EXPECT_EQ(4998, BSFindLowerBound(v,     24994));
  EXPECT_EQ(4999, BSFindLowerBound(v,  10000000));
}

TEST(UtilsTest, TestBinarySearchFunctionsOnFlatDistributions) {
  vector<int> v;
  for (int i = 0; i < 500; i++) {
    v.push_back(i/5);
  }

  // BSFindMatch returns the same values for all of the queries as BSFindLowerMatch EXCEPT when
  // querying for something greater than the largest value in the set, in which case there is a
  // lower bound, but no match.
  EXPECT_EQ( -1, BSFindLowerBound(v, -1));
  EXPECT_EQ(  4, BSFindLowerBound(v,  0));
  EXPECT_EQ(  9, BSFindLowerBound(v,  1));
  EXPECT_EQ(494, BSFindLowerBound(v, 98));
  EXPECT_EQ(499, BSFindLowerBound(v, 99));
  EXPECT_EQ(499, BSFindLowerBound(v,100));

  EXPECT_EQ( -1, BSFindMatch(v, -1));
  EXPECT_EQ(  4, BSFindMatch(v,  0));
  EXPECT_EQ(  9, BSFindMatch(v,  1));
  EXPECT_EQ(494, BSFindMatch(v, 98));
  EXPECT_EQ(499, BSFindMatch(v, 99));
  EXPECT_EQ( -1, BSFindMatch(v,100));
}

TEST(WindowTest, TestCreateDestroyCreate) {
  ASSERT_NE((void*)NULL, system());
  ASSERT_NE((void*)NULL, window());
  vector<pair<int,int> > modes = system()->GetFullScreenModes();

  // TODO(jwills): If I recall correctly, switching to fullscreen on windows is a bit slow.  This
  // test can just test maybe two different modes, it doesn't actually have to be exhaustive if it
  // is painfully slow.
  for (int i = 0; i < modes.size(); i++) {
    ASSERT_TRUE(window()->Create(1024, 768, false));
    window()->Destroy();
    ASSERT_TRUE(window()->Create(modes[i].first, modes[i].second, true))
        << "Failed to create full-screen window with dimensions "
        << modes[i].first << 'x' << modes[i].second;
    window()->Destroy();
  }

}
