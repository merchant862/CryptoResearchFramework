#pragma once
#include <cstddef>
#include <string>
#include <string_view>
namespace crypto_research::bitcoin::address {
enum class PublicAddressType { p2pkh, p2sh, bech32, unknown };
struct PublicAddressFormat final { std::string normalized; PublicAddressType type{PublicAddressType::unknown}; bool valid{}; std::string error; };
[[nodiscard]] PublicAddressFormat inspect_public_address(std::string_view value);
[[nodiscard]] std::string_view to_string(PublicAddressType type) noexcept;
}
