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
  Source(const DataHandler& data_handler) :
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
  Sink(const DataHandler& data_handler) :
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

template<typename FromData, typename ToData>
struct Universal : Source<FromData>, Sink<ToData> {

  virtual ~Universal() = default;
  virtual void process() = 0;
};

} // namespace conveyor

#endif