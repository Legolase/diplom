#ifndef _CONVEYOR_HPP
#define _CONVEYOR_HPP

#include <functional>
#include <memory>
#include <optional>

namespace conveyor {

template<typename Data>
struct Source {
  using UPtr = std::unique_ptr<Source>;
  using DataHandler = std::function<void(const Data&)>;

  Source() = default;
  explicit Source(const DataHandler& data_handler) :
      data_handler(data_handler)
  {}

  virtual ~Source() = default;
  virtual std::optional<Data> getData() final
  {
    auto result_opt = getDataImpl();

    if (data_handler && result_opt.has_value()) {
      data_handler(result_opt.value());
    }

    return result_opt;
  }

protected:
  virtual std::optional<Data> getDataImpl() = 0;

private:
  DataHandler data_handler;
};

template<typename Data>
struct Sink {
  using UPtr = std::unique_ptr<Sink>;
  using DataHandler = std::function<void(const Data&)>;

  Sink() = default;
  explicit Sink(const DataHandler& data_handler) :
      data_handler(data_handler)
  {}

  virtual ~Sink() = default;
  virtual void putData(const Data& data) final
  {
    if (data_handler) {
      data_handler(data);
    }

    putDataImpl(data);
  }

protected:
  virtual void putDataImpl(const Data&) = 0;

private:
  DataHandler data_handler;
};

template<typename Data>
struct Universal {

  Universal(
      std::unique_ptr<Source<Data>>&& source, std::unique_ptr<Sink<Data>>&& sink
  ) noexcept :
      source(std::move(source)),
      sink(std::move(sink))
  {}

  void process()
  {
    while (true) {
      const auto& opt_data = source->getData();
      if (!opt_data.has_value()) {
        break;
      }
      sink->putData(opt_data.value());
    }
  }

private:
  std::unique_ptr<Source<Data>> source;
  std::unique_ptr<Sink<Data>> sink;
};

} // namespace conveyor

#endif