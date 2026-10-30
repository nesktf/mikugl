#include "model/async_loader.hpp"

#include "model/model_loader.hpp"

namespace mikugl {

AsyncLoader::AsyncLoader() noexcept :
    _mutex(), _worker(), _current_path(), _is_loading(false),
    _result(Unexpected(ModelLoadError::NotYetLoaded)) {}

AsyncLoader::~AsyncLoader() noexcept {
  if (_worker.joinable()) {
    _worker.request_stop();
    _worker.join();
  }
}

AsyncLoader::AsyncLoader(AsyncLoader&& other) noexcept :
    _mutex(), _worker(std::move(other._worker)), _current_path(std::move(other._current_path)),
    _is_loading(other._is_loading), _result(std::move(other._result)) {
  other._is_loading = false;
}

AsyncLoader& AsyncLoader::operator=(AsyncLoader&& other) noexcept {
  if (this != &other) {
    if (_worker.joinable()) {
      _worker.request_stop();
      _worker.join();
    }
    std::scoped_lock lock(_mutex, other._mutex);
    _worker = std::move(other._worker);
    _current_path = std::move(other._current_path);
    _is_loading = other._is_loading;
    _result = std::move(other._result);
    other._is_loading = false;
  }
  return *this;
}

void AsyncLoader::request_load(std::string_view path) {
  if (_worker.joinable()) {
    _worker.request_stop();
    _worker.join();
  }

  {
    const std::lock_guard lock(_mutex);
    _current_path = std::string(path);
    _is_loading = true;
    _result = Unexpected(ModelLoadError::NotYetLoaded);
  }

  _worker = std::jthread([this, filepath = std::string(path)](std::stop_token stop_token) {
    if (stop_token.stop_requested()) {
      const std::lock_guard lock(_mutex);
      _is_loading = false;
      return;
    }

    auto load_res = ModelLoader::load_file(filepath);

    if (stop_token.stop_requested()) {
      const std::lock_guard lock(_mutex);
      _is_loading = false;
      return;
    }

    const std::lock_guard lock(_mutex);
    _result = std::move(load_res);
    _is_loading = false;
  });
}

bool AsyncLoader::is_loading() noexcept {
  const std::lock_guard lock(_mutex);
  return _is_loading;
}

std::string_view AsyncLoader::current_path() noexcept {
  const std::lock_guard lock(_mutex);
  return _current_path;
}

ModelExpect<ModelData> AsyncLoader::poll_result() {
  const std::lock_guard lock(_mutex);
  if (_is_loading) {
    return Unexpected(ModelLoadError::NotYetLoaded);
  }

  ModelExpect<ModelData> res = std::move(_result);
  _result = Unexpected(ModelLoadError::NotYetLoaded);
  return res;
}

} // namespace mikugl
