#pragma once

#include <memory>

template <typename T>
using uptr = std::unique_ptr<T>;

using std::make_unique;