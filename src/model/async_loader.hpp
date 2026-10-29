#pragma once

#include "core.hpp"
#include "model/model_data.hpp"

#include <mutex>
#include <string>
#include <string_view>
#include <thread>

namespace mikugl {

class AsyncLoader final {
public:
  AsyncLoader() noexcept;
  ~AsyncLoader() noexcept;
  NTF_NO_COPY(AsyncLoader);
  AsyncLoader(AsyncLoader&&) noexcept;
  AsyncLoader& operator=(AsyncLoader&&) noexcept;

public:
  void request_load(std::string_view path);
  bool is_loading() const noexcept;
  std::string_view current_path() const noexcept;
  Optional<ModelExpect<ModelData>> poll_result();

private:
  mutable std::mutex _mutex;
  std::jthread _worker;
  std::string _current_path;
  bool _is_loading;
  std::optional<Expected<ModelData, ModelLoadError>> _result;
};

} // namespace mikugl
