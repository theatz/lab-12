// Copyright 2020 Your Name <your_email>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <list>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#ifndef INCLUDE_HEADER_HPP_
#define INCLUDE_HEADER_HPP_

struct Item {
  std::string id;
  std::string name;
  float score = 0;
};

class Log {
 public:
  Log(Log& other) = delete;
  void operator=(const Log& other) = delete;

  void Write(std::string_view message) const { *out_ << message << std::endl; }

  void WriteDebug(std::string_view message) const {
    if (level_ > 0) *out_ << message << std::endl;
  }

  static Log* singleton_;

  static Log* GetInstance(size_t level) {
    if (singleton_ == nullptr) {
      singleton_ = new Log(level);
    }
    return singleton_;
  }

 protected:

  explicit Log(size_t level) : level_(level) { out_ = &std::cout; }

 private:
  size_t level_ = 0;
  mutable std::ostream* out_;
};

Log* Log::singleton_ = nullptr;

constexpr size_t kMinLines = 10;

class IObserver1 {
 public:
//  virtual ~IObserver1() = 0;
  virtual void OnDataLoad(const std::vector<Item>& old_items,
                          const std::vector<Item>& new_items) = 0;
  virtual void OnRawDataLoad(const std::vector<std::string>& old_items,
                             const std::vector<std::string>& new_items) = 0;
};

class IObserver2 {
 public:
//  virtual ~IObserver2() = 0;

  virtual void OnLoaded(const std::vector<Item>& new_items) = 0;
  virtual void Skip(const Item& item) = 0;
};

class IObserver3 {
 public:
//  virtual ~IObserver3() = 0;
  virtual void OnDataLoad(const std::vector<Item>& old_items,
                          const std::vector<Item>& new_items) = 0;
  virtual void OnRawDataLoad(const std::vector<std::string>& old_items,
                             const std::vector<std::string>& new_items) = 0;
  virtual void OnSkipped() = 0;
};

class ISubject1 {
 public:
//  virtual ~ISubject1() = 0;
  virtual void Attach(IObserver1* observer) = 0;
  virtual void Detach(IObserver1* observer) = 0;

  virtual void Attach(IObserver2* observer) = 0;
  virtual void Detach(IObserver2* observer) = 0;

  virtual void Attach(IObserver3* observer) = 0;
  virtual void Detach(IObserver3* observer) = 0;
};

class UsedMemory : public IObserver1 {
 public:
  explicit UsedMemory(const Log* log) : log_(log) {}

  void OnDataLoad(const std::vector<Item>& old_items,
                  const std::vector<Item>& new_items) override {
    log_ ->WriteDebug("UsedMemory::OnDataLoad");
    for (const auto& item : old_items) {
      used_ -= item.id.capacity();
      used_ -= item.name.capacity();
      used_ -= sizeof(item.score);
    }

    for (const auto& item : new_items) {
      used_ += item.id.capacity();
      used_ += item.name.capacity();
      used_ += sizeof(item.score);
    }
    log_->Write("UsedMemory::OnDataLoad: new size = " + std::to_string(used_));
  }

  void OnRawDataLoad(const std::vector<std::string>& old_items,
                     const std::vector<std::string>& new_items) override {
    log_->WriteDebug("UsedMemory::OnRawDataLoads");
    for (const auto& item : old_items) {
      used_ -= item.capacity();
    }

    for (const auto& item : new_items) {
      used_ += item.capacity();
    }
    log_->Write("UsedMemory::OnDataLoad: new size = " + std::to_string(used_));
  }

  void clear()
  {
    used_ = 0;
  }

  [[nodiscard]] size_t used() const { return used_; }

 private:
  const Log* log_;
  size_t used_ = 0;
};

class StatSender : public IObserver2 {
 public:
  explicit StatSender(const Log* log) : log_(log) {}

  void OnLoaded(const std::vector<Item>& new_items) override {
    log_->WriteDebug("StatSender::OnDataLoad");

    AsyncSend(new_items, "/items/loaded");
  }

  void Skip(const Item& item) override { AsyncSend({item}, "/items/skiped"); }

 private:
  void AsyncSend(const std::vector<Item>& items, std::string_view path) {
    log_->Write(path);
    log_->Write("send stat " + std::to_string(items.size()));

    for (const auto& item : items) {
      log_->WriteDebug("send: " + item.id);
      // ... some code

      fstr << item.id << item.name << item.score;
      fstr.flush();
    }
  }

  const Log* log_;
  std::ofstream fstr{"network", std::ios::binary};
};


class Histogram : public IObserver3 {
 public:
  Histogram(Log* log) : log_(log) {}
  void OnDataLoad(const std::vector<Item>& old_items,
                  const std::vector<Item>& new_items) override {
    for (const auto& new_item : new_items) {
      average_ += static_cast<float>(new_item.score);
    }
    average_ /= static_cast<float>(new_items.size());

    log_->Write("Average: " + std::to_string(average_) +
                               " Thrown: " + std::to_string(thrown_));
    thrown_ = 0;
  }

  void OnRawDataLoad(const std::vector<std::string>&,
                     const std::vector<std::string>&) override {}

  void OnSkipped() override { thrown_++; };

 private:
  Log* log_;
  float average_ = 0;
  size_t thrown_ = 0;
};

class PageContainer : ISubject1 {
 public:
  void Load(std::istream& io, float threshold) {
    std::vector<std::string> raw_data;

    while (!io.eof()) {
      std::string line;
      std::getline(io, line, '\n');
      raw_data.push_back(std::move(line));
    }

    if (raw_data.size() < kMinLines) {
      throw std::runtime_error("too small input stream");
    }

    for (auto& list : list_observer1) list->OnRawDataLoad(raw_data_, raw_data);

    raw_data_ = std::move(raw_data);

    std::vector<Item> data;
    std::set<std::string> ids;
    for (const auto& line : raw_data_) {
      std::stringstream stream(line);

      Item item;
      stream >> item.id >> item.name >> item.score;

      if (auto&& [_, inserted] = ids.insert(item.id); !inserted) {
        throw std::runtime_error("already seen111");
      }

      if (item.score > threshold) {
        data.push_back(std::move(item));
      } else {
        for (auto& list : list_observer2) list->Skip(item);
      }
    }

    if (data.size() < kMinLines) {
      throw std::runtime_error("oops");
    }

    for (auto& list : list_observer1) list->OnDataLoad(data_, data);
    for (auto& list : list_observer3) list->OnDataLoad(data_, data);


    for (auto& list : list_observer2) list->OnLoaded(data);
    data_ = std::move(data);
  }

  const Item& ByIndex(size_t i) const { return data_[i]; }

  const Item& ById(const std::string& id) const {
    auto it = std::find_if(std::begin(data_), std::end(data_),
                           [&id](const auto& i) { return id == i.id; });
    return *it;
  }

  void Reload(float threshold) {
    std::vector<Item> data;
    std::set<std::string> ids;
    for (const auto& line : raw_data_) {
      std::stringstream stream(line);

      Item item;
      stream >> item.id >> item.name >> item.score;

      if (auto&& [_, inserted] = ids.insert(item.id); !inserted) {
        throw std::runtime_error("already seen111");
      }

      if (item.score > threshold) {
        data.push_back(std::move(item));
      } else {
        for (auto& list : list_observer2) list->Skip(item);
      }
    }

    if (data.size() < kMinLines) {
      throw std::runtime_error("oops");
    }

    for (auto& list : list_observer1) list->OnDataLoad(data_, data);
    for (auto& list : list_observer2) list->OnLoaded(data);
    data_ = std::move(data);
  }
  void Attach(IObserver1* observer) override {
    list_observer1.push_back(observer);
  }

  void Detach(IObserver1* observer) override {
    list_observer1.remove(observer);
  }

  void Attach(IObserver2* observer) override {
    list_observer2.push_back(observer);
  }

  void Detach(IObserver2* observer) override {
    list_observer2.remove(observer);
  }

  void Attach(IObserver3* observer) override {
    list_observer3.push_back(observer);
  }

  void Detach(IObserver3* observer) override {
    list_observer3.remove(observer);
  }

  PageContainer(const Log* log) : log_(log) {}

  size_t data_size()
  {
    return data_.size();
  }


 private:
  const Log* log_;
  std::list<IObserver1*> list_observer1;
  std::list<IObserver2*> list_observer2;
  std::list<IObserver3*> list_observer3;
  std::vector<Item> data_;
  std::vector<std::string> raw_data_;
};

#endif  // INCLUDE_HEADER_HPP_
