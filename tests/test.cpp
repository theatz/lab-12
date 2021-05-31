// Copyright 2020 Your Name <your_email>

#include <gmock/gmock.h>

#include <header.hpp>

TEST(MemoryUsage, MemUsingTest) {
  std::vector<std::string> old_raw_data{"first"},
      new_raw_data{"first", "second", "third", "fourth"};
  UsedMemory memory(Log::GetInstance(0));
  memory.OnRawDataLoad(old_raw_data, new_raw_data);
  EXPECT_EQ(memory.used(), 45);

  memory.clear();

  std::vector<Item> old_data{};
  std::vector<Item> new_data{
      {"1", "rand", 10}, {"2", "rand", 20}, {"3", "rand", 30}};
  memory.OnDataLoad(old_data, new_data);
  EXPECT_EQ(memory.used(), 102);
}

void prepare_stringstream(std::stringstream& ss, size_t lines = 20) {
  for (size_t i = 0; i < lines; ++i) {
    ss << (i + 1) << " cor " << (i < 6 ? 25 : 50) << '\n';
  }
  ss << 21 << " inc "
  << "ls" << '\n';
  ss << 22 << " inc "
  << "la" << '\n';
}

void prep()
{
  for (size_t i = 0; i < 20; ++i) {
    std::cout << (i + 1) << " cor " << (i < 6 ? 25 : 50) << '\n';
  }
  std::cout << 21 << " inc "
     << "ls" << '\n';
  std::cout << 22 << " inc "
     << "la" << '\n';
}
TEST(PageContainer_Test, Data_Test) {

  srand(time(nullptr));
  std::stringstream ss{};
  prepare_stringstream(ss);

  PageContainer page(Log::GetInstance(0));
  page.Load(ss, 0);
  EXPECT_EQ(page.data_size(), 20);
}


TEST(PageContainer_Test, TooSmallInputStream_LoadData){
  std::stringstream ss{};
  prepare_stringstream(ss);

//  prep();
  PageContainer page(Log::GetInstance(0));
  page.Load(ss, 0);
  EXPECT_THROW(page.Load(ss,1),std::runtime_error);
}